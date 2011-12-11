
.include "cpu.inc"
.include "via.inc"

INITBUF_MAXTRK        = $02
INITBUF_TRACK_DIFF    = $03

DISKCHANGEBUFFER      = $00
DISKCHANGED           = $01
GCRBUFFER             = $02; $07 bytes
CURSTPSL              = $03
TRACKINC              = $04
GCRBUFFER0            = $09
GCRBUFFER1            = $0a
GCRBUFFER2            = $0b
LOADEDSECTOR          = $0c
BLOCKINDEX            = $0d
NUMSECTORS            = $0e
NUMSECTORSTEMP        = GCRBUFFER
BLOCKINDEXBASE        = $0f
NEXTSECTOR            = $10
SECTORTOFETCH         = $11
SECTORCOUNT           = $12
CURTRACK              = $13
REQUESTEDSECTOR       = $14
CHECKSUM              = $15

ID0                   = $16; = ROMOS_HEADER_ID0
ID1                   = $17; = ROMOS_HEADER_ID1
FILETRACK             = $18; = ROMOS_HEADER_TRACK, set by ROM on LOAD_ONCE
FILESECTOR            = $19; = ROMOS_HEADER_SECTOR, set by ROM on LOAD_ONCE
FILENAMEHASH0         = FILETRACK
FILENAMEHASH1         = FILESECTOR
FILEINDEX             = FILETRACK
MAXTRACK              = $1a
MAXTRACK_A            = $1b
TRACKLINKTAB          = $1c; $15 bytes

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (!LOAD_ONCE)
HASHVALUE0LO          = $1c
HASHVALUE0HI          = $1d
HASHVALUE1LO          = $1e
HASHVALUE1HI          = $1f
FILENAME              = $20; max. $10 bytes

DIRCYCLEFLAG          = $31
CYCLESTARTENDSECTOR   = $32
CURRDIRBLOCKSECTOR    = $33
NEXTDIRBLOCKSECTOR    = $34
NUMFILES              = $35
WRAPFILEINDEX         = $36
DIRBUFFER             = $37
.endif

BLOCKBUFFER           = $0700
TEMPTRACKLINKTAB      = $0780
TEMPSECTORLINKTAB     = $07c0

ROMOS_HEADER_ID0      = $16
ROMOS_HEADER_ID1      = $17
ROMOS_HEADER_TRACK    = $18
ROMOS_HEADER_SECTOR   = $19
ROMOS_TRACK_DIFF      = $42
ROMOS_MAXTRK          = $02ac

DECGCRTAB10ZZZ432LO   = $9f0d
DECGCRTAB3210ZZZ4LO   = $9f0f
DECGCRTAB0ZZZ4321HI   = $9f1d
DECGCRTAB210ZZZ43HI   = $9f2a
DECGCRTAB43210XXXHI   = $a00d
DECGCRTABXX43210XHI   = $a10d
DECGCRTABX43210XXLO   = $a20d
DECGCRTABXXX43210LO   = $a30d

GCRDECODEHI           = $f8a0
GCRDECODELO           = $f8c0

LINKTRACK             = BLOCKBUFFER + $00
LINKSECTOR            = BLOCKBUFFER + $01


BINARY_NIBBLE_MASK    = %00001111
GCR_NIBBLE_MASK       = %00011111

NUMMAXSECTORS         = 21
NUMTRACKS_SINGLESIDED = 41
NUMTRACKS_A           = 35
NUMTRACKS_B           = NUMTRACKS_SINGLESIDED
MAXTRACK71            = NUMTRACKS_A + NUMTRACKS_B

ANYSECTOR             = $ff; no sector link sanity check
ANYSECTORSANELINK     = $80; sector link sanity check
UNPROCESSEDSECTOR     = $7f; sector link sanity check

SECTORISPROCESSED     = $ff

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (!LOAD_ONCE)
NEW_DISK_VALUE        = $ff
LOAD_FILE_VALUE       = $7f
.endif


            .org $0028

.if UNINSTALL_RUNS_DINSTALL
    .export drvcodebeg71 : absolute
    .export drivebusy71  : absolute
.else
            .org * + $0a
.endif; UNINSTALL_RUNS_DINSTALL


.export c1570fix0 : absolute
.export c1570fix1 : absolute
.export c1570fix2 : absolute


drvcodebeg71: .byte .hibyte(drivebusy71 - * + $0100 - $01); init transfer count hi-byte

dcodinit:   lda #~MOTOR ; the motor is on with LOAD_ONCE because
            and VIA2_PRB; of the KERNAL file open operation
            sta VIA2_PRB; immediately before running this code

            lda #T1_FREE_RUNNING | PA_LATCHING_ENABLE; watchdog irq: count phi2 pulses, 16-bit free-running,
            sta VIA2_ACR                             ; port a latching should not be needed here
                                                     ; (IC rather than discrete logic),
                                                     ; but it is enabled just to be sure
            lda #READ_MODE; BYTE_SYNC is disabled because this is not done via the v-flag here
            sta VIA2_PCR  ; but rather using bit 7 of VIA1_PRA

            ; before loading the first file, the current track number is
            ; retrieved by reading any block header on the disk -
            ; however, if the loader is uninstalled before loading anything,
            ; it needs the more or less correct current track number to
            ; seek to track 18
            lda ROMOS_HEADER_TRACK; $18
            sta CURTRACK; $13
            
            ; set seek boundaries according to number of disk sides
            lda #MAXTRACK71 + 1
            ldx #NUMTRACKS_A
            ldy INITBUF_MAXTRK
            cpy #NUMTRACKS_SINGLESIDED + 2; + 2 because + 1 would misinterpret MAXTRACK = NUMTRACKS_SINGLESIDED + 1 as indicator for double-sidedness upon reinstall
c1570fix0:  bcs :+
            lda #NUMTRACKS_SINGLESIDED + 1
            ldx #NUMTRACKS_SINGLESIDED
:           sta MAXTRACK
            stx MAXTRACK_A

            ; watchdog initialization
            lda #IRQ_CLEAR_FLAGS | IRQ_ALL_FLAGS
            sta VIA1_IER; no irqs from via 1
            sta VIA2_IER; no irqs from via 2
            sta DISKCHANGED
            lda #IRQ_SET_FLAGS | IRQ_TIMER_1
            sta VIA2_IER; timer 1 irqs from via 2

.if UNINSTALL_RUNS_DINSTALL
            lda #.hibyte(drvcodebeg71 - $01)
            sta dgetputhi
            lda #OPC_BIT_ZP
            sta instalwait
.endif; UNINSTALL_RUNS_DINSTALL

            jsr drivebusy71; signal idle to the computer
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
            jsr two_mhz

            lda #WRITE_PROTECT
            and VIA2_PRB
            sta DISKCHANGEBUFFER; store light sensor state for disk removal detection

            lda #NUMMAXSECTORS
            sta NUMSECTORS

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
            jsr setbitrate
            jmp beginload

SENDNIBBLETAB:
            BIT0DEST = 3
            BIT1DEST = 1
            BIT2DEST = 2
            BIT3DEST = 0

            .repeat $10, I
                .byte (((~I >> 0) & 1) << BIT0DEST) | (((~I >> 1) & 1) << BIT1DEST) | (((~I >> 2) & 1) << BIT2DEST) | (((~I >> 3) & 1) << BIT3DEST)
            .endrep

            .assert * <= $0100, error, "***** 1571 SENDNIBBLETAB too high in memory. *****"

checkchgl:  jsr lightsub; fade off the busy led
checkchg:   ; must not change y
            lax VIA2_PRB; check light sensor for disk removal
            eor DISKCHANGEBUFFER
            and #WRITE_PROTECT
            stx DISKCHANGEBUFFER
            beq :+
            sta DISKCHANGED; set the fetch dir flag when disks have been changed
            sec
:           rts

            ; * >= $0100
stack:
            .assert stack >= $0100, error, "***** 1571 stack too low in memory. *****"

.if LOAD_ONCE
            .res 3; padding, best used for bigger stack
.endif
            .word $00, $00, $00, dcodinit - $01
stackend:   ; stacktop + 1
            .assert stackend < $0200, error, "***** 1571 stack too high in memory. *****"

            ; getblock calls
            ; in: a: track
            ;     y: sector, ANYSECTORSANELINK, ANYSECTOR or UNPROCESSEDSECTOR

getblcurts: ; loadfile code
            ldy SECTORTOFETCH; negative: any unprocessed sector, positive: this specific sector; sector link sanity check

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
            stx storputid1
            dex; OPC_STA_ZP/OPC_CMP_ZP -> OPC_STY_ZP/OPC_CPY_ZP
            stx storputid0
            sty REQUESTEDSECTOR
            jsr trackseek; stores the number of blocks on
                         ; the current track in NUMSECTORS

            lda #OPC_JMP_ABS; full gcr fetch and checksumming
getblkscan: sta scanswitch

            ; the disk spins at approximately 300 rpm,
            ; so a revolution takes about 2,000,000 * 60 / 300 = 400,000 cycles at 2 MHz,
            ; so the timeout counter cannot be set to one revolution -
            ; it is reset upon waiting for every new sync,
            ; thus a timeout only indicates a sync-less track range
            ; (about 65536 / 400,000 * 19 = 3.11 sectors), but exclusively non-header
            ; syncs or missing sectors or similar will leave the loader spinning forever
readblock:  jsr waitsync
            beq checkchg; returns with carry set on time-out
chkheader:  cmp #%01010010; check if the sync is followed by a sector header
            bne readblock ; if not, wait for next sync mark

            ; read the sector header
            ldx #$06
getheader:  bit VIA1_PRA
            bmi getheader
            lda VIA2_PRA
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

            bit REQUESTEDSECTOR
            bmi waitdatahd; branch if ANYSECTOR or ANYSECTORSANELINK
            bvc readblock ; branch if not UNPROCESSEDSECTOR

            ; no specific sector requested -
            ; out-of-order sector fetch
           ;lda TRACKLINKTAB,x
            tax          ; check whether the current block has already been
                         ; loaded into the computer's memory
            bmi readblock; if yes, wait for next sector

waitdatahd: sta BLOCKINDEX; store sector index

            ; wait for data block sync
            jsr waitsync; reset the time-out timer here
            cmp #%01010101; check if the sync is followed by a data block
            bne chkheader ; if not, treat as new header

            ldy #$00
            sty CHECKSUM
:           bit VIA1_PRA
            bmi :-
            ldx VIA2_PRA   ; 11222223
            cpx #%11000000
            bcc readblock

loaddata:   lda DECGCRTABXX43210XHI,x; x = [$00..$ff], %2222....
               ; 54 cycles

:           bit VIA1_PRA
            bmi :-
            sta GCRBUFFER0
            txa            ; 11222223
            lsr
            lda VIA2_PRA   ; 33334444
            sta GCRBUFFER1
            and #%11110000 ; 3333....
            adc #%00000000 ; 3333...3
            tax
            lda GCRBUFFER0 ; %2222....            
            ora DECGCRTAB3210ZZZ4LO,x; x = [($00..$ff) & $f1], %22223333
            sta BLOCKBUFFER + $00,y
            eor CHECKSUM
            asl GCRBUFFER1 ; 3334444.
               ; 47 cycles

:           bit VIA1_PRA
            bmi :-
            sta CHECKSUM
            lda VIA2_PRA   ; 45555566
            sta GCRBUFFER0
            asl            ; 5555566.
            lda #%00011110
            and GCRBUFFER1 ; ...4444.
            ror            ; 4...4444
            tax            
            lda DECGCRTAB0ZZZ4321HI,x; x = [($00..$ff) & $8f], %4444....
            ldx GCRBUFFER0 ; 45555566
            ora DECGCRTABX43210XXLO,x; x = [$00..$ff], %44445555
            sta BLOCKBUFFER + $01,y
               ; 48 cycles

:           bit VIA1_PRA
            bmi :-
            eor CHECKSUM
            sta CHECKSUM
            txa            ; 45555566
            and #%00000011 ; ......66
            sta GCRBUFFER0
            lda VIA2_PRA   ; 66677777
            sta GCRBUFFER1
            and #%11100000 ; 666.....
            ora GCRBUFFER0 ; 666...66
            tax
            lda DECGCRTAB210ZZZ43HI,x; x = [($00..$ff) & $e3], %6666....
            ldx GCRBUFFER1 ; 66677777
            ora DECGCRTABXXX43210LO,x; x = [$00..$ff], %66667777
               ; 46 cycles

:           bit VIA1_PRA
            bmi :-
            sta BLOCKBUFFER + $02,y
            eor CHECKSUM
            sta CHECKSUM
            ldx VIA2_PRA   ; 00000111
            lda DECGCRTAB43210XXXHI,x; x = [$00..$ff], %0000....
            sta GCRBUFFER1
            txa
            and #%00000111 ; .....111
            sta GCRBUFFER2
            iny
            iny
            iny
               ; 42 cycles

:           bit VIA1_PRA
            bmi :-
            lda VIA2_PRA   ; 11222223
            sta GCRBUFFER0
            and #%11000000 ; 11......
            ora GCRBUFFER2 ; 11...111
            tax
            lda DECGCRTAB10ZZZ432LO,x; x = [($00..$ff) & $87]; %....1111
            ora GCRBUFFER1 ; %00001111
            sta BLOCKBUFFER + $00,y
            eor CHECKSUM
            sta CHECKSUM
            ldx GCRBUFFER0 ; 11222223
            iny
            beq :+
scanswitch: jmp loaddata
               ; 49 cycles
            
:           bne :++; don't checksum if only the first few bytes have been
                   ; decoded for scanning
            lda DECGCRTABXX43210XHI,x; x = [$00..$ff], %2222....
            sta GCRBUFFER0
            txa            ; 11222223
            lsr            ; .1122222
:           bit VIA1_PRA
            bmi :-
            lda VIA2_PRA   ; 33334444
            and #%11110000 ; 3333....
            adc #%00000000 ; 3333...3
            tax
            lda GCRBUFFER0 ; %2222....
            ora DECGCRTAB3210ZZZ4LO,x; x = [($00..$ff) & $f1], %22223333
            eor CHECKSUM
            bne errorret; branch if data checksum is not ok
:
            ; checksum sector header
            ; this is done only now because there is no time for that between
            ; the sector header and data block
            lda GCRBUFFER + $06
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
            lda GCRBUFFER + $00
            jsr decodesub - $01; ID0
            tay
            eor GCRBUFFER + $05; ID1
            eor LOADEDSECTOR
            eor GCRBUFFER + $06; checksum
            sta CURTRACK; is changed to eor CURTRACK after init
headerchk:  .byte OPC_BIT_ZP, .lobyte(errorret - * - $01); is changed to bne errorret
                                                         ; after init, wait for next sector if
                                                         ; sector header checksum was not ok
            lda GCRBUFFER + $05; ID1
            ldx #$00; set z-flag which won't be altered by the store opcodes
storputid0: cpy ID0; cpy ID0/sty ID0
            bne :+
storputid1: cmp ID1; cmp ID1/sta ID1

:           clc; the next opcode may be an rts, so denote operation successful here
dsctcmps:   bne errorret; branch if the disk ID does not match

            ldx REQUESTEDSECTOR
            inx
            beq :+; branch on ANYSECTOR
            ; gets here on ANYSECTORSANELINK, UNPROCESSEDSECTOR, or requested sector
            ; sector link sanity check
checklink:  ldy LINKTRACK
            beq :+
            cpy MAXTRACK; check whether track link is within the valid range
            bcs errorret; if not, return error
            jsr getnumscts
            dex
            cpx LINKSECTOR; check whether sector link is within the valid range
            bcc errorret; branch if sector number too large

            ; the link track is returned last so that the z-flag
            ; is set if this block is the file's last one
:           ldy LINKSECTOR  ; return the loaded block's sector link sector number
            ldx LOADEDSECTOR; return the loaded block's sector number
            lda LINKTRACK   ; return the loaded block's sector link track number
            clc             ; operation successful
            rts

.if !DISABLE_WATCHDOG
            .assert * >= $02a9, error, "***** 1571 watchdog IRQ vector located below $02a9. *****"
            .assert * <= $02a9, error, "***** 1571 watchdog IRQ vector located above $02a9. *****"

            .word watchdgirq
.endif; !DISABLE_WATCHDOG

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
motrledoff: and #~MOTOR   ; turn off motor
:           and #~BUSY_LED; turn off busy led
store_via2: sta VIA2_PRB
errorret:   sec
:           rts

ddliteon:   lda #BUSY_LED
            ora VIA2_PRB
            bne store_via2

waitsync:   ldx #$ff
            stx VIA2_T1C_H
            inx
:           lda VIA2_T1C_H
            beq wsynctmout; will return $00 in the accu
            bit VIA2_PRB
            bmi :-
            bit VIA2_PRA
:           bit VIA1_PRA
            bmi :-
            lda VIA2_PRA; is never $00 but usually $52 (header) or $55 (data)
wsynctmout: rts

            ror
decodesub:  lsr
            lsr
            lsr
            tax
            lda GCRDECODEHI,y
            ora GCRDECODELO,x
            rts

trackseek:  tax; destination track
trackseekx: lda #MOTOR; turn on the motor
            jsr ddliteon + $02
            txa; destination track
            beq setbitrate; don't do anything if invalid track
            cmp MAXTRACK
            bcs setbitrate; don't do anything if invalid track
            ; do the track jump
            sec
            lda CURTRACK
            sbc MAXTRACK_A
            beq :+
            bcc :+
            sta CURTRACK; the current track is on the 2nd side,
                        ; temporarily store the 2nd side physical track number
:           sec
            txa; destination track
            sbc MAXTRACK_A
            beq :+
            bcs :++; branch if the destination track is on the 2nd side
:           clc
            txa; the destination track is on the 1st side
:           pha
            lda VIA1_PRA
            ora #SIDE_B  ; no idea why this is needed
c1570fix1:  sta VIA1_PRA ; but it won't work without on 1571
            and #~SIDE_SELECT
            bcc :+
            ora #SIDE_B
:
c1570fix2:  sta VIA1_PRA
            pla
            sec
            sbc CURTRACK
            stx CURTRACK; store new track number
            beq setbitrate

            ldy #$00
            sty CURSTPSL
            bcs :+
            eor #~$00; invert track difference
            adc #$01
            iny
:           sty TRACKINC
            asl
            tay
            jsr one_mhz

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
            lda CURSTPSL
            sbc #STEPRACC
            sta CURSTPSL
            pla
            sbc #$00
noheadacc:  cpx VIA2_T1C_H
            beq noheadacc; wait until the counter hi-byte has decreased by 1
            dex
            bmi headaccl
            dey
            bne trackstep

            ; bit-rates:
            ; 31-35/66+   (17): 00 (innermost)
            ; 25-30/60-65 (18): 01
            ; 18-24/53-59 (19): 10
            ;  1-17/36-52 (21): 11 (outermost)
setbitrate: ldy CURTRACK
            jsr getnumscts
putbitrate: bit VIA2_PRB  ; is set to sta VIA2_PRB after init
putnumscts: bit NUMSECTORS; is changed to stx NUMSECTORS after init

            ; fall through

two_mhz:    lda #TWO_MHZ | BYTE_READY; the accu must contain a negative number upon return
            ora VIA1_PRA
            bne :+

            ; for normal busy led fading speed and correct head
            ; stepping speed
one_mhz:    lda #~TWO_MHZ
            and VIA1_PRA
:           sta VIA1_PRA
            rts

getnumscts: tya
:           tay
            sec
            sbc MAXTRACK_A
            beq :+
            bcs :-
:           lda VIA2_PRB
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

.if !UNINSTALL_RUNS_DINSTALL

    .if !DISABLE_WATCHDOG

watchdgirq: ldx #$ff

    .endif; !DISABLE_WATCHDOG

duninstall: tya
            beq :+
            jsr ddliteon
            lda #$ff; fade off the busy led
:           pha
            lda #$12; ROM dir track
            jsr trackseek; ignore error (should not occur)
            pla
            tay
:           jsr lightsub
            tya
            bne :-
            jmp (RESET_VECTOR)

.else; UNINSTALL_RUNS_DINSTALL

    .if !DISABLE_WATCHDOG

watchdgirq: jsr ddliteon
            lda #$12; ROM dir track
            jsr trackseek; ignore error (should not occur)
            ; fade off the busy led and reset the drive
            ldy #$ff
:           jsr lightsub
            tya
            bne :-
            jmp (RESET_VECTOR)

    .endif; !DISABLE_WATCHDOG

duninstall:
:           jsr lightsub
            tya
            bne :-
            lda MAXTRACK
            jmp reinstall

.endif; UNINSTALL_RUNS_DINSTALL

            ; upon first load, this is skipped,
            ; and beginload is executed directly
driveidle:  jsr one_mhz; 1 MHz so the LED fades at the same speed as on 1541
idleloop:   jsr checkchgl; fade off busy led and check light sensor for disk removal
            lda VIA1_PRB
            cmp #CLK_OUT | CLK_IN | DATA_IN | ATN_IN
            beq idleloop; wait until there is something to do

            cmp #CLK_OUT | CLK_IN | DATA_IN
            bne duninstall; check for reset or uninstallation

            jsr two_mhz

            ; load a file

            tya
            beq beginload; check whether the busy led has been completely faded off
            jsr ddliteon; if not, turn it on

beginload:

.if !LOAD_ONCE ; not with LOAD_ONCE because then, there is no danger of getting stuck
               ; because there is no serial transfer to retrieve the file id
            sec
            ror VIA2_T1C_H; reset watchdog time-out, this also clears the possibly
                          ; pending timer 1 irq flag
            ENABLE_WATCHDOG; enable watchdog, the computer might be reset while sending over
                           ; a byte, leaving the drive waiting for handshake pulses

            ; get starting track and sector of the file to load

    .if ::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME

            ldx #LOAD_FILE_VALUE
            lda DIRCYCLEFLAG; if #files < dirbuffsize (DIRCYCLEFLAG = $00),
            beq getfilenam  ; don't reset DIRCYCLEFLAG to LOAD_FILE_VALUE = $7f
            stx DIRCYCLEFLAG
getfilenam: jsr dgetbyte; get filename
            beq :+
            inx
            sta <(FILENAME - LOAD_FILE_VALUE - 1),x
            cpx #<(FILENAME_MAXLENGTH - 2 - LOAD_FILE_VALUE)
            bne getfilenam
:           jsr drwaitrkch; disables watchdog
            jsr gethashval
            sta FILENAMEHASH1
            stx FILENAMEHASH0

    .elseif ::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR

            jsr dgetbyte; get starting track
            sta FILETRACK
            jsr dgetbyte; get starting sector
            jsr drwaitrkch; disables watchdog
            sta FILESECTOR

    .else
            .error "***** Error: The selected file system option is not yet implemented. *****"
    .endif
.endif

.if (::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR) || LOAD_ONCE

            ; check for illegal track or sector
            ldy FILETRACK
            beq toillegal + $00
            cpy MAXTRACK
            bcs toillegal + $01
            jsr getnumscts
            dex
            cpx FILESECTOR
            bcs :+
toillegal:  sec
            jmp illegalts
:
.endif; (::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR) || LOAD_ONCE

            lda #OPC_RTS; disable retry
            sta dsctcmps; on ID mismatch
spinuploop: ldy #ANYSECTOR; get any block on the current track, no sector link sanity check,
            jsr getblcurtr; don't store id, check after return
            bcs spinuploop; retry until any block has been loaded correctly

           ;clc
            beq :+; branch if disk id is the same, if not, re-read the dir
            sec; set the new disk flag when disks have been changed
.if LOAD_ONCE
:           ror DISKCHANGED
.else
:           lda #OPC_BNE; enable retry
            ror DISKCHANGED
            sta dsctcmps; on ID mismatch
.endif
            beq samedisk; branches to samedisk if no disk changes have happened

newdisk:    ; a new disk has been inserted

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (!LOAD_ONCE)

            lda #DIRTRACK
            ldy #DIRSECTOR
            jsr getblkstid; store id, sector link sanity check
            bcs newdisk
           ;ldy LINKSECTOR
            sty CYCLESTARTENDSECTOR

            ; filldirbuf -
            ; directory cycling: fill the dir buffer,
            ; this is also executed upon file not found in
            ; the current directory segment buffered (with y
            ; containing NEXTDIRBLOCKSECTOR's value)

            ldx #NEW_DISK_VALUE ; $ff
            SKIPWORD
filldirbuf: ldx #LOAD_FILE_VALUE; $7f
            stx DIRCYCLEFLAG
            ldx #$ff
            stx WRAPFILEINDEX
            inx
            stx NUMFILES
nextdirsct: jsr checkchg; does not change y
            bne newdisk; check light sensor and start over if its state changed

            ; y contains the current dir sector number
            lda #DIRTRACK
            jsr getblkchid; compare id, sector link sanity check
            bcs newdisk; start over on error
           ;ldy LINKSECTOR
            stx CURRDIRBLOCKSECTOR
           ;lda LINKTRACK
            bne :+
            ; wrap around to first dir block
            ldy #DIRSECTOR + 1
:           sty NEXTDIRBLOCKSECTOR

            ldy #$03
dgdirloop:  ldx NUMFILES
            lda BLOCKBUFFER + $00,y; get file's start track
            beq dnotafile; skip non-files denoted by track 0

            sta .lobyte(DIRTRACKS),x
            lda BLOCKBUFFER + $01,y; get file's start sector
            sta .lobyte(DIRSECTORS),x
            
            jsr fnamehash
            pha
            txa
            ldx NUMFILES; x is trashed in fnamehash
            sta FILENAMEHASHVAL0,x
            pla
            sta FILENAMEHASHVAL1,x

            inc NUMFILES
            cpx #DIRBUFFSIZE - 1
            lda CURRDIRBLOCKSECTOR
            bcs dirbuffull

            ; little flaw for the sake of saving on code size:
            ; when starting to cycle through the directory, the
            ; files in the dir block the last file currently in the dir
            ; buffer is in, will all be added to the buffer when it will
            ; be filled on the subsequent file load - this is why the
            ; minimum dir buffer size is 9 files
dnotafile:  tya
            and #%11100000; 8 entries per block, $20 bytes per entry
           ;clc
            adc #$23
            tay
            bcc dgdirloop ; process all entries in a dir block

            ldy NEXTDIRBLOCKSECTOR
            cpy #DIRSECTOR + 1
            bne :+
            stx WRAPFILEINDEX
:           cpy CYCLESTARTENDSECTOR
            bne nextdirsct

            ; cycle complete
            inc DIRCYCLEFLAG; $ff->$00 or $7f->$80
            SKIPWORD
dirbuffull: sta NEXTDIRBLOCKSECTOR

            ; the disk was not changed, or the dir has just been read
samedisk:   lda #$00; clear new disk flag
            sta DISKCHANGED
            ldx NUMFILES
nextfile:   dex
            bpl findfile; check all dir entries in the buffer

            ; the dir buffer does not contain the file,
            ; so cycle through the directory to find it

            ldy NEXTDIRBLOCKSECTOR
            lda DIRCYCLEFLAG; cycle until the
            lsr             ; cycle is
            bcs filldirbuf  ; complete
            sec
            jmp filenotfnd

findfile:   lda FILENAMEHASH0
            eor FILENAMEHASHVAL0,x
            bne nextfile
            lda FILENAMEHASH1
            eor FILENAMEHASHVAL1,x
            bne nextfile

            ; file found

            stx FILEINDEX; store index of file to jump to the track of the file
                         ; following this one in the dir, after loading

            ; store number of the dir block loaded last,
            ; it is used to start the dir check cycle if
            ; the next file is not found in the dir buffer;
            ; it is also checked on the subsequent load to determine if the
            ; dir check cycle is complete and the file be said to be not found
            lda CURRDIRBLOCKSECTOR
            sta CYCLESTARTENDSECTOR
            sta NEXTDIRBLOCKSECTOR

            jsr ddliteon
            ; check for illegal track or sector
            ldy .lobyte(DIRTRACKS),x
            beq toillegal + $00
            cpy MAXTRACK
            bcs toillegal + $01
            jsr getnumscts
            stx NUMSECTORSTEMP
            ldx FILEINDEX
            lda .lobyte(DIRTRACKS),x
            ldy .lobyte(DIRSECTORS),x
            cpy NUMSECTORSTEMP
            bcc :+
toillegal:  sec
            jmp illegalts
:
            sec; load first block in order as to determine
               ; the file's load address

.else; ::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR && (!LOAD_ONCE)

            ; a new disk has been inserted

    .if LOAD_ONCE
:           lda CURTRACK
            ldy #ANYSECTOR; no sector link sanity check
            jsr getblkchid; compare id
            bcs :-
            lda #$00; clear new disk flag
            sta DISKCHANGED
            bcc spinuploop
    .else
            ; store new disk id
:           lda CURTRACK
            ldy #ANYSECTOR; no sector link sanity check
            jsr getblkstid; store id
            bcs :-
            lda #$00; clear new disk flag
            sta DISKCHANGED
    .endif; !LOAD_ONCE

samedisk:
    .if LOAD_ONCE
            lda #OPC_BNE; enable retry
            sta dsctcmps; on ID mismatch
    .endif
            jsr ddliteon; passes errorret, returns with carry set
            lda FILETRACK
            ldy FILESECTOR

.endif; !(::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR) || LOAD_ONCE

            ; a contains the file's starting track here
            ; y contains the file's starting sector here
            ldx #$00
            stx BLOCKINDEXBASE
           ;sec; load first block in order as to determine
               ; the file's load address
trackloop:  php; out-of-order flag
            sty SECTORTOFETCH
            pha; next track
            txa
            clc
            adc BLOCKINDEXBASE
            sta BLOCKINDEXBASE
            pla; next track
            jsr trackseek

            ; scan the track for the file links
            ; x contains the number of blocks on this track
           ;lda #SECTORISPROCESSED      ; the accu contains a negative number
:           sta TRACKLINKTAB - $01,x    ; mark all sectors as not processed
            sta TEMPTRACKLINKTAB - $01,x; mark all sectors as not processed
            dex
            bne :-
scantrloop: lda #OPC_LDA_ABS; only fetch the first few bytes to track the links
            ; this is a weak point since there is no data checksumming here
            ; however, sector link sanity is checked
            ldy #ANYSECTORSANELINK; sector link sanity check
            sty REQUESTEDSECTOR
            jsr getblkscan
            bcs scantrloop; branch until fetch successful

           ;ldx LOADEDSECTOR
           ;lda LINKTRACK; illegal tracks are checked after
                         ; this track has been processed
            sta TEMPTRACKLINKTAB,x; store the sector's track link and mark it
                                  ; as processed
           ;ldy LINKSECTOR
            tya
            sta TEMPSECTORLINKTAB,x

            ; go through the link list to find the blocks's order on the track
            ldy #$00
            ldx SECTORTOFETCH; first file block on this track
            stx LINKSECTOR
:           lda TEMPTRACKLINKTAB,x
            bmi scantrloop; branch if not all of the file's blocks on this track
                          ; have been scanned yet
            sty TRACKLINKTAB,x; store sector index
            iny; increase sector index
            pha; link track
            lda TEMPSECTORLINKTAB,x; get link sector
            tax
            pla; link track
            cmp CURTRACK; check whether link track is the current track
            beq :-      ; branch until all the file's blocks on the current
                        ; track have been ordered
                        ; loops in the link graph are not detected and will cause an endless loop

            ; the track's sector links are scanned now

            plp; out-of-order flag
            ; read and transfer all the blocks on the current track that belong
            ; to the file, the blocks are read in quasi-random order
            pha         ; next track
            tya         ; amount of the file's blocks on the current track
            pha
            stx NEXTSECTOR; first sector on the next track
            sty SECTORCOUNT; amount of the file's blocks on the current track

blockloop:  ldy #UNPROCESSEDSECTOR; find any yet unprocessed block belonging to the file
            bcc :+; carry clear: load out-of-order
            ldy LINKSECTOR; load the next block in order
:           sty SECTORTOFETCH

:           jsr getblcurts   ; read any of the files's sectors on the current track, compare id
            bcs :-           ; retry until a block has been successfully loaded

            ; send the block over
           ;ldx LOADEDSECTOR
            ldy #SECTORISPROCESSED; $ff
            sty TRACKLINKTAB,x; mark the loaded block as processed
           ;ldy #$ff
           ;lda LINKTRACK
            tax
            bne :+
            ldy LINKSECTOR; the file's last block's length
:           sty dsendcmp + $01
            dey
            dey
            lda BLOCKBUFFER + $01; = LINKSECTOR
            pha; LINKSECTOR
            jsr sendblockl; send the block over, this decreases SECTORCOUNT
            ; carry-flag is set if the next block may be loaded out of order
            pla; LINKSECTOR
            sta BLOCKBUFFER + $01; = LINKSECTOR
            lda SECTORCOUNT; sector count for the current track
            bne blockloop

            ldy NEXTSECTOR
            pla; amount of the file's blocks on the current track
            tax
            pla; next track
            ; carry-flag is set if the next block may be loaded out of order
            bne trackloop; process next track

            ; loading is finished

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (!LOAD_ONCE)

            ldx FILEINDEX
            inx
            lda .lobyte(DIRTRACKS),x
            cpx WRAPFILEINDEX
            beq :+
            cpx NUMFILES
            bcc :++
:           lda #DIRTRACK
:           jsr trackseek; move head to the start track of the next file in the directory

            clc; all ok after loading

filenotfnd: ; branches here with carry set on file not found

.else
            clc; all ok after loading
.endif

illegalts:  ; or illegal t or s

            ; carry: clear = ok, set = load error
sendstatus: lda #$00
            sta SECTORCOUNT; make sure DATA OUT (track change) is not raised after transferring the status
            sta dsendcmp + $01; just send over one byte
            sbc #$01; carry clear: result is $00 - $02 = $fe - loading finished successfully
                    ; carry set:   result is $00 - $01 = $ff - load error
            jsr sendblock

            ldy #$01; turn motor and busy led off
            lda #BUSY_LED; check if busy led is lit
            and VIA2_PRB
            beq :+
            ldy #$ff; fade off the busy led, then turn motor off
.if LOAD_ONCE
:
            jmp duninstall
.else
:           ENABLE_WATCHDOG
:           bit VIA1_PRB; check for ATN in to go high:
            bpl :-; wait until the computer has acknowledged the file transfer
            sei; disable watchdog
            jmp driveidle
.endif

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (!LOAD_ONCE)

fnamehash:  ldx #-$01 - LOAD_FILE_VALUE - 1
:           lda BLOCKBUFFER + $02,y
            iny
            cmp #' ' | $80; $a0 = end of filename
            beq gethashval
            inx
            sta <(FILENAME - LOAD_FILE_VALUE - 1),x
            cpx #<(FILENAME_MAXLENGTH - 2 - LOAD_FILE_VALUE)
            bne :-

gethashval: clc
            stx HASHVALUE0LO
            stx HASHVALUE0HI
            stx HASHVALUE1LO
            stx HASHVALUE1HI
hashloop:   lda <(FILENAME - LOAD_FILE_VALUE - 1),x
            adc HASHVALUE0LO
            sta HASHVALUE0LO
            bcc :+
            inc HASHVALUE0HI
           ;clc; saved for size reasons
:           adc HASHVALUE1LO
            sta HASHVALUE1LO
            lda HASHVALUE0HI
            adc HASHVALUE1HI
            sta HASHVALUE1HI
            dex
            bmi hashloop
            adc HASHVALUE1LO
            tax
            lda HASHVALUE0LO
            adc HASHVALUE0HI
            rts

.endif; (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (!LOAD_ONCE)

sendblockl: ; loadfile code
            sty BLOCKBUFFER + $01; block length
            clc
            lda BLOCKINDEX
            adc BLOCKINDEXBASE

            ; accu: block index or status byte
sendblock:  sta BLOCKBUFFER + $00; block index or status byte
            ldx #$ff
            ldy #$20; here, the watchdog timer is polled manually because
                    ; an extra-long time-out period is needed since the computer may
                    ; still be busy decompressing a large chunk of data;
                    ; this is the round counter
            stx VIA2_T1C_H; reset watchdog time-out, this also clears the possibly
                          ; pending timer 1 irq flag
            lda #DATA_OUT
            sta VIA1_PRB; block ready signal
            ; a watchdog is used because the computer might be reset while sending
            ; over the block, leaving the drive waiting for handshake pulses
waitready:  lda VIA2_T1C_H; see if the watchdog barked
            bne :+
            dey           ; if yes, decrease the round counter
    .if DISABLE_WATCHDOG
            beq nowatchdog
nowatchdog:
    .else
            beq timeout; and trigger watchdog on time-out
    .endif
            stx VIA2_T1C_H; reset watchdog time-out and clear irq flag
:           bit VIA1_PRB
            bpl waitready; wait for ATN in = high
            stx VIA2_T1C_H; reset watchdog time-out and clear possibly set irq flag; reset watchdog time-out
timeout:    ENABLE_WATCHDOG

            ldy #$00
sendloop:
.if !DISABLE_WATCHDOG
            lda #$ff                ; 2
            sta VIA2_T1C_H          ; 4 ; reset watchdog time-out
.endif
            lda BLOCKBUFFER,y       ; 4
            and #BINARY_NIBBLE_MASK ; 2
            tax                     ; 2
            lda SENDNIBBLETAB,x     ; 4
                                    ; = 32

:           bit VIA1_PRB            ; 4
            bmi :-                  ; 3
            sta VIA1_PRB            ; 4
            asl                     ; 2
            ora #ATNA_OUT           ; 2
                                    ; = 15

:           bit VIA1_PRB            ; 4
            bpl :-                  ; 3
            sta VIA1_PRB            ; 4
            lda BLOCKBUFFER,y       ; 4
            lsr                     ; 2
            lsr                     ; 2
            lsr                     ; 2
            lsr                     ; 2
            tax                     ; 2
            lda SENDNIBBLETAB,x     ; 4
                                    ; = 29

:           bit VIA1_PRB            ; 4
            bmi :-                  ; 3
            sta VIA1_PRB            ; 4
            asl                     ; 2
            ora #ATNA_OUT           ; 2
dsendcmp:   cpy #$00                ; 2
            iny                     ; 2
                                    ; = 19

:           bit VIA1_PRB            ; 4
            bpl :-                  ; 3
            sta VIA1_PRB            ; 4
            bcc sendloop            ; 3
                                    ; = 95

:           bit VIA1_PRB; wait for acknowledgement
            bmi :-      ; of the last data bit pair

            dec SECTORCOUNT
            bne drivebusy71; pull DATA_OUT high when changing tracks
drwaitrkch: ldy #CLK_OUT | DATA_OUT; flag track change
            sty VIA1_PRB; this is needed here to set the register in time
            SKIPWORD

            ; following code is transferred using KERNAL routines, then it is
            ; run and gets the rest of the code

drivebusy71:
            ldy #CLK_OUT
            sty VIA1_PRB

            clc; out-of-order flag
            ; it is only possible to load the file's first blocks in order and then switch
            ; to loading out of order - switching back to loading in order will cause
            ; faulty file data to be loaded
            sei; disable watchdog
            rts

            ; must not trash x
dgetbyte:   lda #%10000000; CLK OUT lo: drive is ready
            sta VIA2_T1C_H; reset watchdog time-out
            sta VIA1_PRB
            ldy #DATA_OUT | DATA_IN
:           cpy VIA1_PRB
            bcs :-
            ldy VIA1_PRB
            cpy #CLK_IN | DATA_IN
            ror
:           cpy VIA1_PRB
            beq :-
            ldy VIA1_PRB
            cpy #DATA_IN
            ror
            bcc :--
            rts

DRVCODE71END = *
.export DRVCODE71END

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (!LOAD_ONCE) & (!UNINSTALL_RUNS_DINSTALL)
    LOWDIRBUFFSIZE    = SENDNIBBLETAB - DIRBUFFER;
    HIGHDIRBUFFSIZE   = BLOCKBUFFER - *
    .if LOWDIRBUFFSIZE < HIGHDIRBUFFSIZE
        DIRBUFFSIZE   = LOWDIRBUFFSIZE / 2
    .else
        DIRBUFFSIZE   = HIGHDIRBUFFSIZE / 2
    .endif
    DIRTRACKS         = DIRBUFFER
    DIRSECTORS        = DIRTRACKS + DIRBUFFSIZE
    FILENAMEHASHVAL0  = *
    FILENAMEHASHVAL1  = FILENAMEHASHVAL0 + DIRBUFFSIZE
    DIRBUFFEND        = FILENAMEHASHVAL1 + DIRBUFFSIZE

    DIRBUFFSIZE71     = DIRBUFFSIZE
    .export DIRBUFFSIZE71
    LOWDIRBUFFSIZE71  = LOWDIRBUFFSIZE
    .export LOWDIRBUFFSIZE71
    HIGHDIRBUFFSIZE71 = HIGHDIRBUFFSIZE
    .export HIGHDIRBUFFSIZE71

            .assert DIRBUFFSIZE >= 9, error, "***** Dir buffer too small. *****"
.endif

.if !UNINSTALL_RUNS_DINSTALL
            .assert * <= BLOCKBUFFER, error, "***** 1571 drive code too large. *****"
.endif
            ; entry point

dinstall:   lda ROMOS_MAXTRK
reinstall:  sei
            sta INITBUF_MAXTRK
            lda ROMOS_TRACK_DIFF
            sta INITBUF_TRACK_DIFF

            jsr drivebusy71; does sei

            lda #VIA_ATN_IN_INPUT | VIA_DEVICE_NUMBER_OUTPUT | VIA_ATNA_OUT_OUTPUT | VIA_CLK_OUT_OUTPUT | VIA_CLK_IN_INPUT | VIA_DATA_OUT_OUTPUT | VIA_DATA_IN_INPUT
            sta VIA1_DDRB

:           lda VIA1_PRB; wait for DATA IN = high
            lsr
instalwait: bcc :-
            ldx #.lobyte(stackend - $03)
            txs
            ldx #.lobyte(drvcodebeg71 - $01)
dgetrout:   inx
            bne :+
            inc dgetputhi
:           jsr dgetbyte; there is no watchdog while receiving the code
dgetputhi = * + $02
            sta a:.hibyte(drvcodebeg71 - $01) << 8,x
            cpx #.lobyte(drivebusy71 - $01)
            bne dgetrout
            dec drvcodebeg71
            bne dgetrout

            rts; jumps to dcodinit

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (LOAD_ONCE = 0) & (UNINSTALL_RUNS_DINSTALL)
    LOWDIRBUFFSIZE    = SENDNIBBLETAB - DIRBUFFER;
    HIGHDIRBUFFSIZE   = BLOCKBUFFER - *
    .if LOWDIRBUFFSIZE < HIGHDIRBUFFSIZE
        DIRBUFFSIZE   = LOWDIRBUFFSIZE / 2
    .else
        DIRBUFFSIZE   = HIGHDIRBUFFSIZE / 2
    .endif
    DIRTRACKS         = DIRBUFFER
    DIRSECTORS        = DIRTRACKS + DIRBUFFSIZE
    FILENAMEHASHVAL0  = *
    FILENAMEHASHVAL1  = FILENAMEHASHVAL0 + DIRBUFFSIZE
    DIRBUFFEND        = FILENAMEHASHVAL1 + DIRBUFFSIZE

    DIRBUFFSIZE71     = DIRBUFFSIZE
    .export DIRBUFFSIZE71
    LOWDIRBUFFSIZE71  = LOWDIRBUFFSIZE
    .export LOWDIRBUFFSIZE71
    HIGHDIRBUFFSIZE71 = HIGHDIRBUFFSIZE
    .export HIGHDIRBUFFSIZE71

            .assert DIRBUFFSIZE >= 9, error, "***** Dir buffer too small. *****"
.endif

.if UNINSTALL_RUNS_DINSTALL
            .assert * <= BLOCKBUFFER, error, "***** 1571 drive code too large. *****"
.endif

drvprgend71:
            .reloc
