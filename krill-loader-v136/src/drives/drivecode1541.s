
.include "cpu.inc"
.include "via.inc"

.ifdef STANDALONE
    __NO_LOADER_SYMBOLS_IMPORT = 1
    .include "loader.inc"

    .export LOWMEMEND  : absolute
    .export RUNMODULE  : absolute
    .export checkchg   : absolute
    .export ddliteon   : absolute
    .export gcrdecode  : absolute
    .export dgetbyte   : absolute
    .export drivebusy  : absolute
    .export driveidle  : absolute
    .export dsctcmps   : absolute
    .export gcrencode  : absolute
    .export getblkchid : absolute
    .export getblkstid : absolute
    .export getblcurtr : absolute
    .export getblkscan : absolute
    .export trackseek  : absolute
    .export getnumscts : absolute
    .export duninstall : absolute
.endif; !STANDALONE

.include "drives/drivecode1541-kernel.inc"


.if UNINSTALL_RUNS_DINSTALL
    .ifndef STANDALONE
        .export drvcodebeg41 : absolute
    .endif
    .export drvprgend41 : absolute

    DINSTALLBUFFER = KERNEL - drvprgend41 + dinstall
.endif; UNINSTALL_RUNS_DINSTALL


.macro GETBYTE_IMPL_NOWDOG
            lda #%10000000; CLK OUT lo: drive is ready
            GETBYTE_IMPL_COMMON
.endmacro

.macro GETBYTE_IMPL
            lda #%10000000; CLK OUT lo: drive is ready
            sta VIA2_T1C_H; reset watchdog time-out
            GETBYTE_IMPL_COMMON
.endmacro

.macro GETBYTE_IMPL_COMMON
            ; must not trash x
            sta VIA1_PRB
            ldy #DATA_OUT | DATA_IN
@0:         cpy VIA1_PRB; wait for CLK high
            bcs @0
            ldy VIA1_PRB
            cpy #CLK_IN | DATA_IN
            ror
@1:         cpy VIA1_PRB; wait for CLK low
            beq @1
            ldy VIA1_PRB
            cpy #DATA_IN
            ror
            bcc @0
.endmacro


.if !UNINSTALL_RUNS_DINSTALL
            .org $001b
.else
            .org $001a
.endif

.ifdef STANDALONE
            .word * + 2
.endif; STANDALONE

            .assert * > ROMOS_HEADER_SECTOR, error, "***** 1541 drivecode starts too low in memory. *****"
drvcodebeg41:

.ifndef STANDALONE
            .byte .hibyte(drvcodeend41 - * + $0100 - $01); init transfer count hi-byte
            .byte .hibyte(drvprgend41 - * + $0100 - $01); init transfer count hi-byte for re-install
.endif; !STANDALONE

dcodinit:

.if !UNINSTALL_RUNS_DINSTALL
            lda #READ_MODE | BYTE_SYNC_ENABLE
            sta VIA2_PCR

            ; before loading the first file, the current track number is
            ; retrieved by reading any block header on the disk -
            ; however, if the loader is uninstalled before loading anything,
            ; it needs to know the more or less correct current track number
            ; in order to seek to track 18 before reset
            lda ROMOS_HEADER_TRACK; $18
            sta CURTRACK; $13

            ; watchdog initialization

            lda #$00
    .if INSTALL_FROM_DISK
            ldx #JOBCODE0400 - JOBCODE0400 + $80
    .else
            ldx #JOBCODE0600 - JOBCODE0400 + $80
    .endif
:           sta .lobyte(JOBCODE0400 - $80),x; clear job queue
            dex
            bmi :-

           ;ldx #IRQ_CLEAR_FLAGS | IRQ_ALL_FLAGS; $7f
            stx VIA1_IER; no irqs from via 1
            stx VIA2_IER; no irqs from via 2

            lda #IRQ_SET_FLAGS | IRQ_TIMER_1
            sta VIA2_IER; timer 1 irqs from via 2

            lda #JOBCODE_EXECUTE
            sta JOBCODE0300; execute watchdog handler at $0300 on watchdog time-out

            lda #NUMMAXSECTORS
            sta NUMSECTORS

           ;ldx #$7f
            stx LOADEDMODULE
.endif; !UNINSTALL_RUNS_DINSTALL

           ;ldx #$7f
            stx DISKCHANGED

            lda #CLK_OUT; drivebusy, signal idle to the computer
            sta VIA1_PRB

            lda #CLK_OUT | CLK_IN | DATA_IN | ATN_IN
:           cmp VIA1_PRB
            bne :-; no watchdog

            ; fade off the busy led if lit
            lda VIA2_PRB
            and #BUSY_LED
            beq :+
            lda #$ff
:           tay
            ; before spinning up the motor and finding the current track,
            ; wait until a file is requested to be loaded at all
:           jsr lightsub
            lda VIA1_PRB
            cmp #CLK_OUT | CLK_IN | DATA_IN | ATN_IN
            beq :-; wait until there is something to do

            ; check for reset or uninstallation
            cmp #CLK_OUT | CLK_IN | DATA_IN
            beq :+
            jmp duninstall
:
            lda #WRITE_PROTECT
            and VIA2_PRB
            sta DISKCHANGEBUFFER; store light sensor state for disk removal detection

            ldy #$0f

            .assert * >= SENDGCRTABLE + $1f, error, "***** 'mkgcrdec' overwrites itself. *****"

mkgcrdec:   lda sendgcrraw,y
            ldx GCRENCODE,y
            sta SENDGCRTABLE,x
            dey
            bpl mkgcrdec

            lda INITBUF_TRACK_DIFF
            bne findtrackn; branch if the drive had already seeked before the loader has been started
            ; the drive was reset immediately before running the loader -
            ; step down a track: this works normally if the stepping bits are congruent with the stepper motor;
            ; however, it may happen that the bits are misaligned (opposite to the actual stepper position, bit 1
            ; reversed), this alone does not move the head but stepping makes it go into the direction opposite to
            ; the one desired when moving; the stepping down two halftracks will actually step up and step down one
            ; halftrack each and thus will end up on the same track as before, but align the stepper bits to the motor.
            ldx #$02
            stx CURTRACK
            dex
            jsr trackseekx

            ; find current track number
            ; this assumes the head is on a valid half track
findtrackn: lda #-$01; invalid track number -> no track step
            ldy #ANYSECTOR
            jsr getblkstid; no sector link sanity check, set CURTRACK
            bcs :+

            lda GCRBUFFER + $04
            lsr
            asl GCRBUFFER + $03
            rol
            asl GCRBUFFER + $03
            rol
            asl GCRBUFFER + $03
            rol
            and #GCR_NIBBLE_MASK
            tay
            lda GCRBUFFER + $03
            jsr decodesub; track
            cmp CURTRACK; getblkstid sets CURTRACK at this stage,
                        ; the value is inferred by eoring the header checksum
                        ; and all header field values except the current track
            beq initdone
:           clc
            lda #1 << BITRATE_SHIFT
            adc VIA2_PRB
            sta VIA2_PRB; cycle through the 4 bit-rates
            bne findtrackn; jmp

initdone:   lda #OPC_EOR_ZP
            sta headerchk - $02
            lda #OPC_BNE
            sta headerchk
            lda #OPC_STA_ABS
            sta putbitrate
            lda #OPC_STX_ZP
            sta putnumscts
            jmp setbitrate; returns to runmodule

LOWMEMEND = *
            ; moving LOWMEMEND lower in memory will
            ; reduce the dir buffer size for
            ; the loadfile module
KERNEL:

.if UNINSTALL_RUNS_DINSTALL
GETDRIVECODE = getdrvcode - gcrencode + $0800

runcodeget: ldx #.lobyte(stackend - $07)
            txs
:           lda .lobyte(gcrencode - $0100),x
            sta $0700,x
            inx
            bne :-
            ldx #.lobyte(drvcodebeg41 - $01)
            jmp GETDRIVECODE

getdrvcode: inx
            bne :+
getroutputhi = * + $08
            inc getroutputhi - getdrvcode + GETDRIVECODE
            ; there is no watchdog while receiving the code
:           jsr dgetbyte - gcrencode + $0800
            sta a:.hibyte(drvcodebeg41 - $01) << 8,x
            cpx #.lobyte(drvprgend41 - $01)
            bne getdrvcode
            dec drvcodebeg41 + $01
            bne getdrvcode
            jmp restart
.endif; UNINSTALL_RUNS_DINSTALL

            ; common code for all configurations

dgetbyte:   GETBYTE_IMPL
            rts

; 5
            ; must not change the state of the carry flag
drivebusy:  sty VIA1_PRB
            sei; disable watchdog
            rts

; 21 - loadfile code
gcrencode:  pha
            and #BINARY_NIBBLE_MASK
            tax
            lda GCRENCODE,x
            sta LONIBBLES,y
            pla
            lsr
            lsr
            lsr
            lsr
            tax
            lda GCRENCODE,x
            sta HINIBBLES,y
            rts

; 16
gcrdecode:  ldx HINIBBLES,y
            lda GCRDECODEHI,x
            ldx LONIBBLES,y
            ora GCRDECODELO,x
            ldx NUMFILES; loadfile code
            iny
            rts

; 29
waitsync:   ldx #$ff
            stx VIA2_T1C_H; reset the sync time-out
            inx
            sec
:           lda VIA2_T1C_H
            beq wsynctmout; will return $00 in the accu
            bit VIA2_PRB
            bmi :-
            jsr read1stdat; reads $ff
            ; VIA2_PRA never reads $00 here but usually $52 (header) or $55 (data)
read1stdat: lda VIA2_PRA
            alr #(GCR_NIBBLE_MASK << 1) | 1;    ;   ...22222:3 - and + lsr
            clv
            bvc *
wsynctmout: rts

            ; * >= $0100
stack:
            .assert stack >= $0100, error, "***** 1541 stack too low in memory. *****"

.if LOAD_ONCE
            .res 3; padding, best used for bigger stack
.endif
            .word $00, $00, dcodinit - $01, runmodule - $01; return addresses for install
stackend:   ; stacktop + 1
            .assert stackend < $0200, error, "***** 1541 stack too high in memory. *****"

            ; getblock calls
            ; in: a: track
            ;     y: sector, ANYSECTORSANELINK, ANYSECTOR or UNPROCESSEDSECTOR

            ; get the next block passing, check against stored id
getblcurtr: lda CURTRACK

.if LOAD_ONCE
            ; get the block at track a, sector y, check against stored id
getblkchid:
getblkstid: ldx #OPC_CMP_ZP
.else; !LOAD_ONCE
            ; get the block at track a, sector y, check against stored id
getblkchid: ldx #OPC_CMP_ZP
            SKIPWORD
            ; get the block at track a, sector y, store read id
getblkstid: ldx #OPC_STA_ZP
.endif; !LOAD_ONCE
            sty REQUESTEDSECTOR
            stx storputid1
            dex; OPC_STA_ZP/OPC_CMP_ZP -> OPC_STY_ZP/OPC_CPY_ZP
            stx storputid0
            jsr trackseek; stores the number of blocks on
                         ; the current track in NUMSECTORS

            lda #OPC_BNE; full gcr fetch and checksumming
getblkscan: sta scanswt0
            sta scanswt1

            ; the disk spins at approximately 300 rpm,
            ; so a revolution takes about 1,000,000 * 60 / 300 = 200,000 cycles,
            ; so the timeout counter cannot be set to one revolution -
            ; it is reset upon waiting for every new sync,
            ; thus a timeout only indicates a sync-less track range
            ; (about 65536 / 200,000 * 19 = 6.23 sectors), but exclusively non-header
            ; syncs or missing sectors or similar will leave the loader spinning forever
readblock:  jsr waitsync
            beq wsynctmout; returns with carry set on time-out
            ; check if the sync is followed by a sector header
            bcs readblock; if not, wait for next sync mark

            ; read the sector header
gotheader:  ldx #$06
getheader:  bvc *
            lda VIA2_PRA
            clv
            sta GCRBUFFER + $00,x
            dex
            bpl getheader

            ; check if the sector header's field values match the expectations -
            ; the header is only checksummed after the data fetch since there
            ; is not enough time to do it now

            ; decode sector number
            asl GCRBUFFER + $04
            lda GCRBUFFER + $05
            rol
            and #GCR_NIBBLE_MASK
            tay
            lda GCRBUFFER + $04
            jsr decodesub + $00
            cmp NUMSECTORS; check if sector number is within range of the allowed
                          ; sector numbers for the current track
            bcs readblock
            sta LOADEDSECTOR; store away the sector number, it is returned in the
                            ; x-register on success
            tax             ; current sector number
            lda TRACKLINKTAB,x

            cpx REQUESTEDSECTOR
            beq waitdatahd; branch if requested sector

            ; bit:bpl:bvc won't work because the v-flag
            ; is unstable while the disk is spinning
            ldy REQUESTEDSECTOR
            bmi waitdatahd; branch if ANYSECTOR or ANYSECTORSANELINK
            iny
            bpl readblock ; branch if not UNPROCESSEDSECTOR

            ; no specific sector requested -
            ; out-of-order sector fetch
           ;lda TRACKLINKTAB,x
            tax          ; check whether the current block has already been
                         ; loaded into the computer's memory
            bmi readblock; if yes, wait for next sector

waitdatahd: sta BLOCKINDEX; store sector index

            ; wait for data block sync
            jsr waitsync
            ; check if the sync is followed by a data block
            bcc gotheader; if not, treat as new header
                         ; error or cycle candidate

            ; read and partially inflate the gcr nibbles to 8 bits
            lda VIA2_PRA                   ;    ;   11222223
            alr #(GCR_NIBBLE_MASK << 1) | 1;    ;   ...22222:3 - and + lsr
            clv  ; this portion can't be replaced by calling the similar bit
            bvc *; at the end of waitsync because of too big calling overhead
           ;ldx #$00
loaddata:   ldy VIA2_PRA                   ; 14 ;   33334444      1 - cycle 14 in [0..25]
            sta HINIBBLES + $00,x          ; 19
            tya                            ; 21
            ror                            ; 23 ;   33333444
            lsr                            ; 25 ;   .3333344
            lsr                            ; 27 ;   ..333334
            lsr                            ; 29 ;   ...33333   - final: 3
            sta LONIBBLES + $00,x          ; 34
            txa                            ; 36
            axs #-$03                      ; 38 ; x = x + 3
            tya                            ; 40 ;   33334444
            ldy VIA2_PRA                   ; 44 ;   45555566     2 - cycle 44 in [32..51]
            clv                            ; 46                    - cycle 46 in [32..51]
            cpy #$80                       ; 48 ; 4:45555566
            rol                            ; 50 ;   33344444
            and #GCR_NIBBLE_MASK           ; 52 ;   ...44444   - final: 4
               ; 52 cycles in [0..51]

            bvc *                          ;  3 ;   3 cycles variance
            sta HINIBBLES + $01 - 3,x      ;  8
            tya                            ; 10 ;   45555566
            alr #%01111111                 ; 12 ;   ..555556:6 - and + lsr
            sta LONIBBLES + $01 - 3,x      ; 17
            lda VIA2_PRA                   ; 21 ;   66677777     3 - cycle 16 in [0..25]
            tay                            ; 23
            ror                            ; 25 ;   66667777
            lsr LONIBBLES + $01 - 3,x      ; 32 ;   ...55555:6 - final: 5
            ror                            ; 34 ;   66666777
            lsr                            ; 36 ;   .6666677
            lsr                            ; 38 ;   ..666667
            lsr                            ; 40 ;   ...66666   - final: 6
            sta HINIBBLES + $02 - 3,x      ; 45
            lda VIA2_PRA                   ; 49 ;   00000111     4 - cycle 49 in [32..51]
            lsr                            ; 51 ;   .0000011:1
            sta HINIBBLES + $03 - 3,x      ; 56
            tya                            ; 58 ;   66677777
            and #GCR_NIBBLE_MASK           ; 60 ;   ...77777   - final: 7
            sta LONIBBLES + $02 - 3,x      ; 65
            lda VIA2_PRA                   ; 69 ;   11222223     0 - cycle 69 in [64..77]
            clv                            ; 71                    - cycle 71 in [64..77]
            ror                            ; 73 ;   11122222:3
            sta LONIBBLES + $03 - 3,x      ; 78
               ; 78 cycles in [0..77]

            bvc *                          ;  3 ; 3 cycles variance
            and #GCR_NIBBLE_MASK           ;  5 ;   ...22222   - final: 2
            inx                            ;  7
scanswt0:   bne loaddata                   ; 10

               ; a 5-GCR-bytes cycle minimally takes approximately 130 cycles in speed zone 11, 52 + 78 = 130

            .assert .hibyte(*) = .hibyte(loaddata), error, "***** Page boundary crossing in GCR fetch loop, fatal cycle loss. *****"

            tay                            ; 11 ;   ...22222
            lda VIA2_PRA                   ; 15 ;   33334444     1 - cycle 15 in [0..25]
            jsr decodesub - $01            ; decode data checksum
            tay

            ; finish gcr inflation and checksum the data
            ldx #$00
gcrfinish:  lda LONIBBLES + $03,x          ;        11122222     4
            lsr HINIBBLES + $03,x          ;        ..000001:1   7
            ror                            ;        11112222     2
            lsr HINIBBLES + $03,x          ;        ...00000:1   7 - final: 0
            ror                            ;        11111222     2
            lsr                            ;        .1111122     2
            lsr                            ;        ..111112     2
            lsr                            ;        ...11111     2 - final: 1
            sta LONIBBLES + $03,x          ;                     5 = 30
            tya                            ;                     2
            ldy HINIBBLES + $00,x          ;                     4
            eor GCRDECODEHI,y              ;                     4
            ldy LONIBBLES + $00,x          ;                     4
            eor GCRDECODELO,y              ;                     4
            ldy HINIBBLES + $01,x          ;                     4
            eor GCRDECODEHI,y              ;                     4
            ldy LONIBBLES + $01,x          ;                     4
            eor GCRDECODELO,y              ;                     4
            ldy HINIBBLES + $02,x          ;                     4
            eor GCRDECODEHI,y              ;                     4
            ldy LONIBBLES + $02,x          ;                     4
            eor GCRDECODELO,y              ;                     4
            ldy HINIBBLES + $03,x          ;                     4
            eor GCRDECODEHI,y              ;                     4
            ldy LONIBBLES + $03,x          ;                     4
            eor GCRDECODELO,y              ;                     4
            tay                            ;                     2
            txa                            ;                     2
            axs #-$04; x = x + 4           ;                     2
scanswt1:   bne gcrfinish                  ;                     3 = 75
                                           ;                       = 105

            .assert .hibyte(*) = .hibyte(gcrfinish), error, "***** Page boundary crossing in GCR finishing loop, unnecessary cycle loss. *****"

            tya
            beq :+; check whether data checksum is ok
            txa
            beq errorret; only return an error if the full block has been checksummed

:           ; this is done only now because there is no time for that between
            ; the sector header and data block
            jsr decodehdr

            ; checksum sector header
            tay; ID0
            eor GCRBUFFER + $05; ID1
            eor LOADEDSECTOR
            eor GCRBUFFER + $06; checksum
            sta CURTRACK; is changed to eor CURTRACK after init
headerchk:  .byte OPC_BIT_ZP, .lobyte(errorret - * - $01); is changed to bne errorret
                                                         ; after init, wait for next sector if
                                                         ; sector header checksum was not ok
                                                         ; error or cycle candidate

            lda GCRBUFFER + $05; ID1
            ldx #$00; set z-flag which won't be altered by the store opcodes
storputid0: cpy ID0; cpy ID0/sty ID0
            bne :+
storputid1: cmp ID1; cmp ID1/sta ID1

:           clc; the next opcode may be an rts, so denote operation successful here
dsctcmps:   bne errorret; branch if the disk ID does not match

            ldy #$00
            jsr gcrdecode; decode the block's first byte (track link)
            sta LINKTRACK
            jsr gcrdecode; decode the block's second byte (sector link)

            ldx REQUESTEDSECTOR
            inx
            beq :++; branch on ANYSECTOR: no sector link sanity check
            ; gets here on ANYSECTORSANELINK, UNPROCESSEDSECTOR, or requested sector

            sta LINKSECTOR
            ; sector link sanity check
            ldy LINKTRACK
            beq :+
            cpy #MAXTRACK41 + 1; check whether track link is within the valid range
            bcs errorret; if not, return error
            jsr getnumscts
            dex
            cpx LINKSECTOR; check whether sector link is within the valid range
            bcc errorret; branch if sector number too large

            ; the link track is returned last so that the z-flag
            ; is set if this block is the file's last one
:           ldy LINKSECTOR  ; return the loaded block's sector link sector number
:           ldx LOADEDSECTOR; return the loaded block's sector number
            lda LINKTRACK   ; return the loader block's sector link track number
            clc             ; operation successful
            rts

lightsub:   tya
            tax
            beq :++++
:           inx
            bne :-
            tax
            jsr ddliteon
:           dex
            bne :-
            dey
            bne :+
            and #~MOTOR   ; turn off motor
:           and #~BUSY_LED; turn off busy led
store_via2: sta VIA2_PRB
errorret:   sec
:           rts

ddliteon:   lda #BUSY_LED
            ora VIA2_PRB
            bne store_via2

decodehdr:  lda GCRBUFFER + $06
            alr #(GCR_NIBBLE_MASK << 1) | 1; and + lsr
            tay
            lda GCRBUFFER + $05
            jsr decodesub - $01; checksum
            sta GCRBUFFER + $06
            lax GCRBUFFER + $02
            lsr
            lsr
            lsr
            tay
            txa
            asl GCRBUFFER + $01
            rol
            asl GCRBUFFER + $01
            rol
            and #GCR_NIBBLE_MASK
            jsr decodesub + $03; ID1
            sta GCRBUFFER + $05
            lda GCRBUFFER + $01
            lsr
            lsr
            lsr
            tay
            lda GCRBUFFER + $00; ID0
            ror
decodesub:  lsr
            lsr
            lsr
            tax
            lda GCRDECODEHI,y
            ora GCRDECODELO,x
            rts

; 15
checkchg:   ; must not change y
            lax VIA2_PRB; check light sensor for disk removal
            eor DISKCHANGEBUFFER
            and #WRITE_PROTECT
            stx DISKCHANGEBUFFER
            beq :+
            sta DISKCHANGED; set the new disk flag when disks have been changed
:           rts

            ; configuration-dependent code

.if !UNINSTALL_RUNS_DINSTALL

uninstallc: tay
uninstfade: jsr lightsub
            tya
            bne uninstfade
            jmp (RESET_VECTOR)

    .if !DISABLE_WATCHDOG
            .assert * >= $0300, error, "***** 1541 watchdog IRQ/BRK handler located below $0300. *****"
            .assert * <= $0300, error, "***** 1541 watchdog IRQ/BRK handler located above $0300. *****"
	.endif

watchdgirq: ldy #$ff

duninstall: tya
            beq :+
            jsr ddliteon
            lda #$ff; fade off the busy led
:           pha
            lda #$12; ROM dir track
            jsr trackseek; ignore error (should not occur)
            pla
            jmp uninstallc

.else ; UNINSTALL_RUNS_DINSTALL

            .byte 0, 0; padding

duninstall:
:           jsr lightsub
            tya
            bne :-
            jmp runcodeget

    .if !DISABLE_WATCHDOG
            .assert * >= $0300, error, "***** 1541 watchdog IRQ/BRK handler located below $0300. *****"
            .assert * <= $0300, error, "***** 1541 watchdog IRQ/BRK handler located above $0300. *****"
    .endif

watchdgirq: jsr ddliteon
            lda #$12; ROM dir track
            jsr trackseek; ignore error (should not occur)
            ; fade off the busy led and reset the drive
            ldy #$ff
:           jsr lightsub
            tya
            bne :-
            jmp (RESET_VECTOR)
            
.endif; UNINSTALL_RUNS_DINSTALL

trackseek:  tax; destination track
trackseekx: lda #MOTOR; turn on the motor
            jsr ddliteon + $02
            txa; destination track
            beq setbitrate; don't do anything if invalid track

            cmp #MAXTRACK41 + 1
            bcs setbitrate; don't do anything if invalid track
            sec
            sbc CURTRACK
            beq setbitrate
            ; do the track jump
            stx CURTRACK
            ldy #$00
            sty CURRSTEPSPEEDLOW
            bcs :+
            eor #~$00; invert track difference
            adc #$01
            iny
:           sty TRACKINC
            asl; half-tracks
            tay

            ; TRACKINC         step bits ... store
            ; $00 (move up)    %00 %00 %01 -> %01
            ;     (inwards)    %01 %01 %11 -> %10
            ;                  %10 %10 %01 -> %11
            ;                  %11 %11 %11 -> %00
            ; $01 (move down)  %00 %01 %11 -> %11
            ;     (outwards)   %01 %00 %01 -> %00
            ;                  %10 %11 %11 -> %01
            ;                  %11 %10 %01 -> %10

            lda #$80 | (MINSTPSP + 1)
trackstep:  sta VIA2_T1C_H
            tax
            lda TRACKINC
            eor VIA2_PRB
            sec
            rol
            and #TRACK_STEP
            eor VIA2_PRB
            sta VIA2_PRB
            txa
headaccl:   cmp #$80 | MAXSTPSP
            beq noheadacc
            pha
           ;sec
            lda CURRSTEPSPEEDLOW
            sbc #STEPRACC
            sta CURRSTEPSPEEDLOW
            pla
            sbc #$00
noheadacc:  cpx VIA2_T1C_H
            beq noheadacc; wait until the counter hi-byte has decreased by 1
            dex
            bmi headaccl
            dey
            bne trackstep; jmp
            SKIPWORD
bneuninstl: bne duninstall; jmp

            ; bit-rates:
            ; 31+   (17): 00 (innermost)
            ; 25-30 (18): 01
            ; 18-24 (19): 10
            ;  1-17 (21): 11 (outermost)
setbitrate: ldy CURTRACK
            jsr getnumscts
putbitrate: bit VIA2_PRB  ; is changed to sta VIA2_PRB after init
putnumscts: bit NUMSECTORS; is changed to stx NUMSECTORS after init

            ; fall through

getnumscts: lda VIA2_PRB
            ora #SYNC_MARK | BITRATE_MASK; $e0
            ldx #21
            cpy #18
            bcc :+
            dex
            dex; 19
            sbc #1 << BITRATE_SHIFT; $60 -> $40
            cpy #25
            bcc :+
            dex; 18
            sbc #1 << BITRATE_SHIFT; $40 -> $20
            cpy #31
            bcc :+
            dex; 17
            sbc #1 << BITRATE_SHIFT; $20 -> $00
:           rts

            ; upon first load, this is skipped,
            ; and runmodule is executed directly
driveidle:  jsr lightsub; fade off the busy led
            jsr checkchg; check light sensor for disk removal
            lda VIA1_PRB
            cmp #CLK_OUT | CLK_IN | DATA_IN | ATN_IN
            beq driveidle; wait until there is something to do

            cmp #CLK_OUT | CLK_IN | DATA_IN
            bne bneuninstl; check for reset or uninstallation

            ; execute command

            tya; led fade counter
            beq runmodule; check whether the busy led has been completely faded off
            jsr ddliteon ; if not, turn it on

runmodule:
.if !LOAD_ONCE
            sec
            ror VIA2_T1C_H; reset watchdog time-out, this also clears the possibly
                          ; pending timer 1 irq flag
            ENABLE_WATCHDOG; enable watchdog, the computer might be reset while sending over a
                           ; byte, leaving the drive waiting for handshake pulses
.endif; !LOAD_ONCE

.if INSTALL_FROM_DISK
            jsr dgetbyte
            cmp LOADEDMODULE
            bne RUNMODULE
            ;sta LOADEDMODULE
            jam
            nop
            brk; padding
            brk; padding

            ; load module loader
            ; the module library track and sector are stored in
            ; LIBTRACK and LIBSECTOR, respectively

            ldy #CLK_OUT
            jsr drivebusy; disables watchdog

:           lda LIBTRACK
            ldy LIBSECTOR ; sector link sanity check
            jsr getblkchid; compare id
            bcs :-
            ldy #$02
:           jsr gcrdecode
            sta RUNMODULE - $02,y
            bne :-

            ; fall through
.endif; INSTALL_FROM_DISK

            ; module space
RUNMODULE:

.ifndef STANDALONE
    .if INSTALL_FROM_DISK
            ldy #CLK_OUT
            jsr drivebusy; disables watchdog

            ; load a module
            ; the module file track and sector are stored in
            ; LIBTRACK and LIBSECTOR, respectively

LOADMODULE:
:           lda LIBTRACK
            ldy LIBSECTOR ; sector link sanity check
            jsr getblkchid; check id
            bcs :-

            ldy #$02
:           jsr gcrdecode
            sta POINTER + $00 - 3,y; POINTER + 0: lo, POINTER + 1: hi, POINTER + 2: page count
            cpy #$05
            bne :-

            ldx #loadmodend - loadmodule - 1
:           lda loadmodule,x
            sta LOWMEM + $03,x
            dex
            bpl :-
            jmp LOWMEM + $03

loadmodule: jsr gcrdecode; always decodes the whole block
            dey
            sta (POINTER),y
            iny
            bne loadmodule
            clc
            lda #$fe
            adc POINTER + $00
            sta POINTER + $00
            bcc :+
            inc POINTER + $01
            dec POINTER + $02
            beq modloaded
:           lda LINKTRACK
            beq modloaded
            ldy LINKSECTOR
            jsr getblkchid; compare id, sector link sanity check
            bcs :-
            ldy #$02
            bne loadmodule; jmp
modloaded:  jmp RUNMODULE
loadmodend:

            .assert loadmodend - loadmodule + LOWMEM + $03 < LOWMEMEND, error, "***** loadmodule too large. *****"

    .else; !INSTALL_FROM_DISK
            .include "drives/drivecode1541-loadfile.s"
    .endif; !INSTALL_FROM_DISK
.endif

DRVCODE41END = *
.export DRVCODE41END

sendgcrraw:
            BIT0DEST = 3
            BIT1DEST = 1
            BIT2DEST = 2
            BIT3DEST = 0

            ; $ef instead of $0f because these values are also used as watchdog time-outs, which must be as long as possible
            .repeat $10, I
                .byte $ef ^ ((((I >> 0) & 1) << BIT0DEST) | (((I >> 1) & 1) << BIT1DEST) | (((I >> 2) & 1) << BIT2DEST) | (((I >> 3) & 1) << BIT3DEST))
            .endrep

drvcodeend41:
            ; following code is transferred using KERNAL routines, then it is
            ; run and gets the rest of the code

            ; entry point
dinstall:
            sei
.if LOAD_ONCE
            lda #~MOTOR ; the motor is on because of the
            and VIA2_PRB; file open operation immediately
            sta VIA2_PRB; before running this code
.endif
.if INSTALL_FROM_DISK
            lda ROMOS_HEADER_TRACK
            sta LIBTRACK
            lda ROMOS_HEADER_SECTOR
            sta LIBSECTOR
.endif
            lda ROMOS_TRACK_DIFF; $42
            sta INITBUF_TRACK_DIFF

            lda #CLK_OUT
            sta VIA1_PRB
            lda #VIA_ATN_IN_INPUT | VIA_DEVICE_NUMBER_OUTPUT | VIA_ATNA_OUT_OUTPUT | VIA_CLK_OUT_OUTPUT | VIA_CLK_IN_INPUT | VIA_DATA_OUT_OUTPUT | VIA_DATA_IN_INPUT
            sta VIA1_DDRB

:           lda VIA1_PRB; wait for DATA IN = high
            lsr
instalwait: bcc :-

.ifndef STANDALONE

            ldx #.lobyte(drvcodebeg41 - $01)
dgetrout:   inx
            bne :+
            inc dgetputhi
:           GETBYTE_IMPL_NOWDOG; there is no watchdog while installing
dgetputhi = * + $02
            sta a:.hibyte(drvcodebeg41 - $01) << 8,x
            cpx #.lobyte(drvcodeend41 - $01)
            bne dgetrout
            dec drvcodebeg41
            bne dgetrout

.endif; !STANDALONE

restart:    ldx #.lobyte(stackend - $05)
            txs

.if UNINSTALL_RUNS_DINSTALL

            lda #T1_FREE_RUNNING | PA_LATCHING_ENABLE; watchdog irq: count phi2 pulses, 16-bit free-running,
                                                     ; enable port a latching to grab one gcr byte at a time
                                                     ; rather than letting the gcr bitstream scroll through
                                                     ; port a (applies to 1541 and Oceanic OC-118, but not
                                                     ; 1541-II)
            sta VIA2_ACR
            lda #READ_MODE | BYTE_SYNC_ENABLE
            sta VIA2_PCR

            ; before loading the first file, the current track number is
            ; retrieved by reading any block header on the disk -
            ; however, if the loader is uninstalled before loading anything,
            ; it needs to know the more or less correct current track number
            ; in order to seek to track 18 before reset
            lda ROMOS_HEADER_TRACK; $18
            sta CURTRACK; $13

            ; watchdog initialization

            lda #$00
    .if INSTALL_FROM_DISK
            ldx #JOBCODE0400 - JOBCODE0400 + $80
    .else
            ldx #JOBCODE0600 - JOBCODE0400 + $80
    .endif
:           sta .lobyte(JOBCODE0400 - $80),x; clear job queue
            dex
            bmi :-

           ;ldx #IRQ_CLEAR_FLAGS | IRQ_ALL_FLAGS; $7f
            stx VIA1_IER; no irqs from via 1
            stx VIA2_IER; no irqs from via 2

            lda #IRQ_SET_FLAGS | IRQ_TIMER_1
            sta VIA2_IER; timer 1 irqs from via 2

            lda #JOBCODE_EXECUTE
            sta JOBCODE0300; execute watchdog handler at $0300 on watchdog time-out

            lda #NUMMAXSECTORS
            sta NUMSECTORS

           ;ldx #$7f
            stx LOADEDMODULE 
.else
            lda #T1_FREE_RUNNING | PA_LATCHING_ENABLE; watchdog irq: count phi2 pulses, 16-bit free-running,
                                                     ; enable port a latching to grab one gcr byte at a time
                                                     ; rather than letting the gcr bitstream scroll through
                                                     ; port a (applies to 1541 and Oceanic OC-118, but not
                                                     ; 1541-II)
            sta VIA2_ACR
.endif; UNINSTALL_RUNS_DINSTALL
            rts; returns to dcodinit

drvprgend41:
            .reloc
