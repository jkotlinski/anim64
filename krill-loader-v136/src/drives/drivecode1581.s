
.include "cpu.inc"
.include "cia.inc"
.include "via.inc"

BUFFER                = $00
SYS_SP                = $01
JOBCODESTABLE         = $02; fixed in ROM
JOBTRKSCTTABLE        = $0b; fixed in ROM - $0b..$1c
FILETRACK             = $0b
FILESECTOR            = $0c
FILENAMEHASH0         = FILETRACK
FILENAMEHASH1         = FILESECTOR
.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (!LOAD_ONCE)
FILEINDEX             = FILETRACK
.endif
HASHVALUE0LO          = $0d
HASHVALUE0HI          = $0e
HASHVALUE1LO          = $0f
HASHVALUE1HI          = $10
NUMFILES              = $11
CURRDIRBLOCKTRACK     = $12
CURRDIRBLOCKSECTOR    = $13
CYCLESTARTENDTRACK    = $14
CYCLESTARTENDSECTOR   = $15
DIRCYCLEFLAG          = $16
;BLOCKBUFFERJOBTRACK  = $17; fixed in ROM - track for job at buffer 6 ($0900)
;BLOCKBUFFERJOBSECTOR = $18; fixed in ROM - sector for job at buffer 6 ($0900)
BLOCKINDEX            = $19
NEXTDIRBLOCKTRACK     = $1a
NEXTDIRBLOCKSECTOR    = $1b
FIRSTDIRSECTOR        = $1c

DRIVESTATE            = $26; fixed in ROM
DRIVEOFF              = $00; literal
OPEN_FILE_TRACK       = $4c; fixed in ROM
SYSIRQVECTORBUF_LO    = $5e
SYSIRQVECTORBUF_HI    = $5f
WRAPFILEINDEX         = $63

LED_FLAG              = $79
IRQVECTOR_LO          = $0192
IRQVECTOR_HI          = $0193
HDRS2                 = $01bc
DIRTRACK81            = $022b
OPEN_FILE_SECTOR      = $028b
DIRSECTOR81           = $00; literal

STROBE_CONTROLLER     = $ff54

READ_DV               = $80
MOTOFFI_DV            = $8a
SEEK_DV               = $8c

OK_DV                 = $00

BUFFER0               = $0300
BUFFERSIZE            = $0100
TRACKOFFSET           = $00
SECTOROFFSET          = $01

BLOCKBUFFER           = $0900
SENDTABLELO           = $0a00
SENDTABLEHI           = $0b00

LINKTRACK             = BLOCKBUFFER + $00
LINKSECTOR            = BLOCKBUFFER + $01

BINARY_NIBBLE_MASK    = %00001111

ROMOS_MAXTRACK        = $8f; MAXTRACK81 - 1
ROMOS_MAXSECTOR       = $75; MAXSECTOR81 + 1
MAXTRACK81            = 80; literal
MAXSECTOR81           = 39; literal

.if !DISABLE_WATCHDOG
RESET_TIMERB          = $cb9f
WATCHDOG_PERIOD       = $20; 32 * 65536 cycles at 2 MHz = 1.049 s
CONTROLLERIRQPERIODFD = $4e20
    .macro INIT_CONTROLLER
            jsr initcontrl
    .endmacro
.else
    .macro INIT_CONTROLLER
    .endmacro
.endif

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (!LOAD_ONCE)
NEW_DISK_VALUE        = $ff
LOAD_FILE_VALUE       = $7f
.endif

BUFFERINDEX           = (BLOCKBUFFER - BUFFER0) / BUFFERSIZE


            .org $0300


.if UNINSTALL_RUNS_DINSTALL
    .export drvcodebeg81 : absolute
    .export drivebusy81  : absolute
.endif; UNINSTALL_RUNS_DINSTALL


.export cmdfdfix0 : absolute
.export cmdfdfix1 : absolute
.export cmdfdfix2 : absolute
.export cmdfdfix3 : absolute
.export cmdfdfix4 : absolute


drvcodebeg81: .byte .hibyte(drivebusy81 - * + $0100 - $01); init transfer count hi-byte

SENDNIBBLETAB:
            BIT0DEST = 3
            BIT1DEST = 1
            BIT2DEST = 2
            BIT3DEST = 0

            .repeat $10, I
                .byte (((~I >> 0) & 1) << BIT0DEST) | (((~I >> 1) & 1) << BIT1DEST) | (((~I >> 2) & 1) << BIT2DEST) | (((~I >> 3) & 1) << BIT3DEST)
            .endrep

filename:   ; note: this is not in the zero-page

dcodinit:   tsx
            stx SYS_SP
.if !DISABLE_WATCHDOG
            lda IRQVECTOR_LO
            sta SYSIRQVECTORBUF_LO
            lda IRQVECTOR_HI
            sta SYSIRQVECTORBUF_HI
.endif

.if LOAD_ONCE
            jsr drivebusy81; signal idle to the computer
:           lda CIA_PRB
            and #ATN_IN | ATNA_OUT | CLK_OUT | CLK_IN | DATA_OUT | DATA_IN
            cmp #ATN_IN |            CLK_OUT | CLK_IN |            DATA_IN
            bne :-; no watchdog
            ; the busy led might be enabled
            lda #DRIVE_LED
            bit CIA_PRA
            beq :++
            ; turn it off if so
            ldx #$ff
:           jsr lightsub
            txa
            bne :-
:
.else; !LOAD_ONCE

            jsr motrledoff

.endif; LOAD_ONCE

            ldx #$00
:           txa
            and #BINARY_NIBBLE_MASK
            tay
            lda SENDNIBBLETAB,y
            sta SENDTABLELO,x
            txa
            lsr
            lsr
            lsr
            lsr
            tay
            lda SENDNIBBLETAB,y
            sta SENDTABLEHI,x
            inx
            bne :-

.if !DISABLE_WATCHDOG
            lda cmdfdfix2; 0 for FD
            beq :+
            ; watchdog initialization
            lda #$ff
            sta CIA_TA_LO
            sta CIA_TA_HI
            lda #COUNT_PHI2 | FORCE_LOAD | CONTINUOUS | TIMER_START
            sta CIA_CRA
            jsr initwatchd
            lda #CIA_CLR_INTF | EVERY_IRQ
            sta CIA_ICR
            lda #CIA_SET_INTF | TIMERB_IRQ
            sta CIA_ICR
            bne :++
:           jsr initwatchd
:
.endif

.if UNINSTALL_RUNS_DINSTALL
            lda #.hibyte(drvcodebeg81 - $01)
            sta dgetputhi
            lda #OPC_BIT_ZP
            sta instalwait
.endif; UNINSTALL_RUNS_DINSTALL

            ldx #$00

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (!LOAD_ONCE)
            stx NUMFILES
.endif

.if !LOAD_ONCE
            jsr drivebusy81; signal idle to the computer
:           lda CIA_PRB
            and #ATN_IN | ATNA_OUT | CLK_OUT | CLK_IN | DATA_OUT | DATA_IN
            cmp #ATN_IN |            CLK_OUT | CLK_IN |            DATA_IN
            bne :-; no watchdog
.endif; !LOAD_ONCE

drividle:   jsr lightsub; fade off the busy led
            lda CIA_PRB
            and #ATN_IN | ATNA_OUT | CLK_OUT | CLK_IN | DATA_OUT | DATA_IN
            cmp #ATN_IN |            CLK_OUT | CLK_IN |            DATA_IN
            beq drividle; wait until there is something to do

            cmp #                    CLK_OUT | CLK_IN |            DATA_IN
            beq :+
            jmp duninstall; check for reset or uninstallation

            ; load a file

:           txa
            beq beginload; check whether the busy led has been completely faded off
            jsr ddliteon ; if not, turn it on
beginload:

.if !LOAD_ONCE
    .if !DISABLE_WATCHDOG
            jsr enablewdog; enable watchdog, the computer might be reset while sending over a
                          ; byte, leaving the drive waiting for handshake pulses
    .endif; !DISABLE_WATCHDOG

            ; get starting track and sector of the file to load

    .if ::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME

            ldx #LOAD_FILE_VALUE
            lda DIRCYCLEFLAG; if #files < dirbuffsize (DIRCYCLEFLAG = $00),
            beq getfilenam  ; don't reset DIRCYCLEFLAG to $7f
            stx DIRCYCLEFLAG
getfilenam: jsr dgetbyte; get filename
            beq :+
            inx
            sta filename - LOAD_FILE_VALUE - 1,x
            cpx #.lobyte(FILENAME_MAXLENGTH - 2 - LOAD_FILE_VALUE)
            bne getfilenam
:           jsr drwaitrkch; disables watchdog
            jsr gethashval
            sta FILENAMEHASH1
            stx FILENAMEHASH0

            lda NUMFILES
            beq newdisk
            lda CIA_PRA; query DISK_CHANGE
            bpl newdisk
            jmp samedisk

newdisk:    ; a new disk has been inserted
            jsr getdirtrk
            ldy #DIRSECTOR81
            jsr getblock
            bcs newdisk
  
           ;lda LINKTRACK
            sta CYCLESTARTENDTRACK
           ;ldy LINKSECTOR
            sty CYCLESTARTENDSECTOR
            sty FIRSTDIRSECTOR

            ; filldirbuf -
            ; directory cycling: fill the dir buffer,
            ; this is also executed upon file not found in
            ; the current directory segment buffered (with y
            ; containing NEXTDIRBLOCKSECTOR's value)
            ldx #NEW_DISK_VALUE
            SKIPWORD
filldirbuf: ldx #LOAD_FILE_VALUE
            stx DIRCYCLEFLAG
            ldx #$ff
            stx WRAPFILEINDEX
            inx
            stx NUMFILES
nextdirsct: bit CIA_PRA; query DISK_CHANGE
            bpl newdisk

            ; a contains the current dir track number
            ; y contains the current dir sector number
            sta CURRDIRBLOCKTRACK
            jsr getblock
            bcs newdisk

           ;ldx CURRSECTOR
            stx CURRDIRBLOCKSECTOR
           ;ldy LINKSECTOR
           ;lda LINKTRACK
            bne :+
            jsr getdirtrk
            ldy FIRSTDIRSECTOR
:           sta NEXTDIRBLOCKTRACK
            sty NEXTDIRBLOCKSECTOR

            ldy #$03
dgdirloop:  ldx NUMFILES
            lda BLOCKBUFFER + TRACKOFFSET,y; get file's start track
            beq dnotafile; skip non-files denoted by track 0
            sta DIRTRACKS,x
            lda BLOCKBUFFER + SECTOROFFSET,y; get file's start sector
            sta DIRSECTORS,x

            jsr fnamehash
            pha
            txa
            ldx NUMFILES; x is trashed in fnamehash
            sta FILENAMEHASHVAL0,x
            pla
            sta FILENAMEHASHVAL1,x

            inc NUMFILES
            cpx #DIRBUFFSIZE - 1
            bcs dirbuffull

            ; little flaw for the sake of saving on code size:
            ; when starting to cycle through the directory, the
            ; files in the dir block the last file currently in the dir
            ; buffer is in, will all be added to the buffer when it will
            ; be filled on the subsequent file load - this is why the
            ; minimum dir buffer size is 9 files
dnotafile:  tya
            and #%11100000
           ;clc
            adc #$23
            tay
            bcc dgdirloop; process all entries in a dir block

            ldy NEXTDIRBLOCKSECTOR
            jsr getdirtrk
            cmp NEXTDIRBLOCKTRACK
            bne :+
            cpy #DIRSECTOR81
            bne :+
            stx WRAPFILEINDEX
:           lda NEXTDIRBLOCKTRACK
            cmp CYCLESTARTENDTRACK
            bne nextdirsct
            cpy CYCLESTARTENDSECTOR
            bne nextdirsct

            ; cycle complete
            inc DIRCYCLEFLAG; $ff->$00 or $7f->$80
            bcs samedisk; jmp

dirbuffull: lda CURRDIRBLOCKTRACK
            sta NEXTDIRBLOCKTRACK
            lda CURRDIRBLOCKSECTOR
            sta NEXTDIRBLOCKSECTOR

            ; the disk was not changed, or the dir has just been read
samedisk:   ldx NUMFILES
nextfile:   dex
            bpl findfile; check all dir entries in the buffer

            ; the dir buffer does not contain the file,
            ; so cycle through the directory to find it

            lda DIRCYCLEFLAG; cycle until the
            lsr             ; cycle is
            bcc :+
            lda NEXTDIRBLOCKTRACK
            ldy NEXTDIRBLOCKSECTOR
            jmp filldirbuf  ; complete
:           sec
            jmp filenotfnd

findfile:   lda FILENAMEHASH0
            eor FILENAMEHASHVAL0,x
            bne nextfile
            lda FILENAMEHASH1
            eor FILENAMEHASHVAL1,x
            bne nextfile

            stx FILEINDEX; store index of file to jump to the track of the file
                         ; following this one in the dir, after loading

            ; store number of the dir block loaded last,
            ; it is used to start the dir check cycle if
            ; the next file is not found in the dir buffer;
            ; it is also checked on the subsequent load to determine if the
            ; dir check cycle is complete and the file be said to be not found
            lda CURRDIRBLOCKTRACK
            sta CYCLESTARTENDTRACK
            sta NEXTDIRBLOCKTRACK
            lda CURRDIRBLOCKSECTOR
            sta CYCLESTARTENDSECTOR
            sta NEXTDIRBLOCKSECTOR

            lda DIRTRACKS,x
            ldy DIRSECTORS,x
            tax

    .elseif ::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR

            jsr dgetbyte; get starting track
            tax
            jsr dgetbyte; get starting sector
            jsr drwaitrkch; disables watchdog
            tay
            txa

    .endif

.else; LOAD_ONCE
            ldy OPEN_FILE_SECTOR
            ldx OPEN_FILE_TRACK
.endif; LOAD_ONCE

            ; check for illegal track or sector
            beq toillegal + $00

            dex; 79->77 cmp 79 -> bcc
            dex; 80->78 cmp 79 -> bcc
               ; 81->80 cmp 79 -> bcs
            cpx ROMOS_MAXTRACK; #MAXTRACK81 - 1
            inx
            inx
            bcs toillegal + $01
            cpy ROMOS_MAXSECTOR; #MAXSECTOR81 + 1
            bcc :+
toillegal:  sec
cmdfdfix0:  jmp illegalts; is changed to bit illegalts on FD2000/4000 to disable illegal track or sector error,
                         ; ROM variables for logical track/sector boundaries aren't known (probably around MAXTRACKFD = $54)

:           tya
            pha
            jsr ddliteon
            pla
            tay; FILESECTOR
            txa; FILETRACK

            ldx #$00
            stx BLOCKINDEX
loadblock:  sta JOBTRKSCTTABLE + (2 * BUFFERINDEX) + TRACKOFFSET
            sty JOBTRKSCTTABLE + (2 * BUFFERINDEX) + SECTOROFFSET
:           jsr getblockag
            bcs :-

           ;ldy LINKSECTOR
           ;lda LINKTRACK
            pha
            beq :+
            ldy #$ff
:           lda LINKSECTOR
            pha
            sty dsendcmp + $01
            dey
            dey
            sty BLOCKBUFFER + $01; block length
            lda BLOCKINDEX
            jsr sendblock; send the block over
            inc BLOCKINDEX
            pla; LINKSECTOR
            tay
            pla; LINKTRACK
            bne loadblock

            ; loading is finished

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (LOAD_ONCE = 0)

            ldx FILEINDEX
            inx
            lda DIRTRACKS,x
            cpx WRAPFILEINDEX
            beq :+
            cpx NUMFILES
            bcc :++
:           jsr getdirtrk
:           jsr trackseek; move head to the start track of the next file in the directory

            clc; all ok after loading

filenotfnd: ; branches here with carry set on file not found

.else
            clc; all ok after loading
.endif

illegalts:  ; or illegal t or s

            jsr sendstatus

            ldx #$01; turn motor and busy led off
            lda #DRIVE_LED; check if busy led is lit
            and CIA_PRA
            beq :+
            ldx #$ff; fade off the busy led, then turn motor off
.if LOAD_ONCE
:
            jmp duninstall
.else
    .if !DISABLE_WATCHDOG
:           jsr enablewdog
    .endif
:           bit CIA_PRB; check for ATN in to go high:
            bpl :-; wait until the computer has acknowledged the file transfer
            sei; disable watchdog
            jmp drividle
.endif

.if !DISABLE_WATCHDOG
initcontrl: lda SYSIRQVECTORBUF_LO
            sta IRQVECTOR_LO
            lda SYSIRQVECTORBUF_HI
            sta IRQVECTOR_HI
            lda cmdfdfix2; 0 for FD
            beq :+
            jmp RESET_TIMERB
:           lda #.lobyte(CONTROLLERIRQPERIODFD)
            sta VIA_T1C_L
            lda #.hibyte(CONTROLLERIRQPERIODFD)
            sta VIA_T1C_H
            rts
.endif

cmdfdfix1 = * + 1
cmdfdfix2 = * + 2
getdirtrk:  lda DIRTRACK81
            rts

trackseek:  tax
            dex
            stx HDRS2 + (2 * BUFFERINDEX)
            INIT_CONTROLLER
            lda #SEEK_DV
            ldx #BUFFERINDEX
.if DISABLE_WATCHDOG
            jmp STROBE_CONTROLLER; move head to the start track of the next file in the
                                 ; directory

.else; !DISABLE_WATCHDOG
            jsr STROBE_CONTROLLER; move head to the start track of the next file in the
                                 ; directory

            ; fall through

initwatchd: ; the i-flag is set here
            lda #.lobyte(watchdgirq)
            sta IRQVECTOR_LO
            lda #.hibyte(watchdgirq)
            sta IRQVECTOR_HI
            lda cmdfdfix2; 0 for FD
            beq :+
            lda #.lobyte(WATCHDOG_PERIOD)
            sta CIA_TB_LO
            lda #.hibyte(WATCHDOG_PERIOD)
            sta CIA_TB_HI
:           rts

enablewdog: lda cmdfdfix2; 0 for FD
            beq :+
            lda #COUNT_TA_UNDF | FORCE_LOAD | ONE_SHOT | TIMER_START
            sta CIA_CRB
            bit CIA_ICR
            ENABLE_WATCHDOG
            rts            
:           lda #IRQ_CLEAR_FLAGS | IRQ_ALL_FLAGS
            sta VIA_IER; no irqs from via
            lda #IRQ_SET_FLAGS | IRQ_TIMER_1
            sta VIA_IER; timer 1 irqs from via
            lda #$ff
            sta VIA_T1C_H
            ENABLE_WATCHDOG
            rts
.endif; !DISABLE_WATCHDOG

lightsub:   txa
            tay
            beq lightisoff
:           nop
            bit OPC_BIT_ZP
            iny
            bne :-
            pha
            jsr ddliteon
            pla
            tay
:           nop
            bit OPC_BIT_ZP
            dey
            bne :-
            dex
            bne ddliteoff

motrledoff: ; turn off motor
            txa
            pha
            INIT_CONTROLLER
            lda #MOTOFFI_DV
            ldx #BUFFERINDEX
            jsr STROBE_CONTROLLER
            lda #DRIVEOFF
            sta DRIVESTATE
            pla
            tax

ddliteoff:  lda CIA_PRA
            and #~DRIVE_LED; turn off drive led
            ldy #$00
store_cia:  sta CIA_PRA
            sty LED_FLAG
lightisoff: rts

ddliteon:   lda #DRIVE_LED
            ora CIA_PRA
            ldy #$ff
            bne store_cia; jmp

getblock:   sta JOBTRKSCTTABLE + (2 * BUFFERINDEX) + TRACKOFFSET
            sty JOBTRKSCTTABLE + (2 * BUFFERINDEX) + SECTOROFFSET
getblockag: INIT_CONTROLLER
            lda #READ_DV
            ldx #BUFFERINDEX
            jsr STROBE_CONTROLLER

.if !DISABLE_WATCHDOG
            jsr initwatchd
.endif

            lda JOBCODESTABLE + BUFFERINDEX; FD does not return the error status in the accu
            cmp #OK_DV + 1

            ; the link track is returned last so that the z-flag
            ; is set if this block is the file's last one
            ldy LINKSECTOR
            ldx JOBTRKSCTTABLE + (2 * BUFFERINDEX) + SECTOROFFSET; LOADEDSECTOR
            lda LINKTRACK
            rts

.if !UNINSTALL_RUNS_DINSTALL

    .if !DISABLE_WATCHDOG

watchdgirq: ldx #$ff

    .endif; !DISABLE_WATCHDOG

duninstall: txa
            beq :+
            jsr ddliteon
            lda #$ff; fade off the busy led
:           pha
            jsr getdirtrk
            jsr trackseek
            pla
            tax
:           jsr lightsub
            txa
            bne :-
            ldx SYS_SP
            txs
            INIT_CONTROLLER
            rts

.else; UNINSTALL_RUNS_DINSTALL

    .if !DISABLE_WATCHDOG

watchdgirq: jsr ddliteon
            jsr getdirtrk
            jsr trackseek
            ; fade off the busy led and reset the drive
            ldx #$ff
:           jsr lightsub
            txa
            bne :-
            ldx SYS_SP
            txs
            INIT_CONTROLLER
            rts

    .endif; !DISABLE_WATCHDOG

duninstall:
:           jsr lightsub
            txa
            bne :-
            ldx SYS_SP
            txs
            INIT_CONTROLLER
            jmp dinstall

.endif; UNINSTALL_RUNS_DINSTALL

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (!LOAD_ONCE)

fnamehash:  ldx #-$01 - LOAD_FILE_VALUE - 1
:           lda BLOCKBUFFER + $02,y
            iny
            cmp #' ' | $80; $a0 = end of filename
            beq gethashval
            inx
            sta filename - LOAD_FILE_VALUE - 1,x
            cpx #.lobyte(FILENAME_MAXLENGTH - 2 - LOAD_FILE_VALUE)
            bne :-

gethashval: clc
            stx HASHVALUE0LO
            stx HASHVALUE1LO
            stx HASHVALUE0HI
            stx HASHVALUE1HI
hashloop:   lda filename - LOAD_FILE_VALUE - 1,x
            adc HASHVALUE0LO
            sta HASHVALUE0LO
            bcc :+
            inc HASHVALUE0HI
            clc
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

.endif; (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (!LOAD_ONCE)

            ; carry: clear = ok, set = load error
sendstatus: lda #$00
            sta dsendcmp + $01
            sbc #$01; carry clear: result is $00 - $02 = $fe - loading finished successfully
                    ; carry set:   result is $00 - $01 = $ff - load error
            sta JOBTRKSCTTABLE + (2 * BUFFERINDEX) + TRACKOFFSET; make sure DATA OUT (track change) is not
                                                                ; raised after transferring the status

sendblock:  sta BLOCKBUFFER + $00; block index
.if !DISABLE_WATCHDOG
            jsr enablewdog
.endif
            lda #DATA_OUT
            sta CIA_PRB; block ready signal
waitready:  bit CIA_PRB
            bpl waitready
            ldy #$00
sendloop:
.if !DISABLE_WATCHDOG
cmdfdfix3 = * + 1
            lda #COUNT_TA_UNDF | FORCE_LOAD | ONE_SHOT | TIMER_START
cmdfdfix4 = * + 1
            sta CIA_CRB       ; 2 + 4; reset watchdog time-out
.endif
            ldx BLOCKBUFFER,y ; 4
            lda SENDTABLELO,x ; 4
                              ; = 22 (+6 with watchdog)

:           bit CIA_PRB       ; 4
            bmi :-            ; 3
            sta CIA_PRB       ; 4
            asl               ; 2
            and #~ATNA_OUT    ; 2
                              ; = 15

:           bit CIA_PRB       ; 4
            bpl :-            ; 3
            sta CIA_PRB       ; 4
            ldx BLOCKBUFFER,y ; 4
            lda SENDTABLEHI,x ; 4
                              ; = 19

:           bit CIA_PRB       ; 4
            bmi :-            ; 3
            sta CIA_PRB       ; 4
            asl               ; 2
            and #~ATNA_OUT    ; 2
dsendcmp:   cpy #$00          ; 2
            iny               ; 2
                              ; = 19

:           bit CIA_PRB       ; 4
            bpl :-            ; 3
            sta CIA_PRB       ; 4
            bcc sendloop      ; 3
                              ; = 75

:           bit CIA_PRB; wait for acknowledgement
            bmi :-     ; of the last data bit pair

            lda LINKTRACK
            cmp JOBTRKSCTTABLE + (2 * BUFFERINDEX) + TRACKOFFSET
            beq drivebusy81; pull DATA_OUT high when changing tracks
drwaitrkch: ldy #CLK_OUT | DATA_OUT; flag track change
            SKIPWORD

            ; following code is transferred using KERNAL routines, then it is
            ; run and gets the rest of the code

drivebusy81:
            ldy #CLK_OUT
            sty CIA_PRB
            sei; disable watchdog
            rts

            ; must not trash x
dgetbyte:   lda #%10000000; CLK OUT lo: drive is ready
            sta BUFFER
            sta CIA_PRB
:           lda #CLK_IN
:           bit CIA_PRB
            beq :-
            lda CIA_PRB
            lsr
            ror BUFFER
            lda #CLK_IN
:           bit CIA_PRB
            bne :-
            lda CIA_PRB
            lsr
            ror BUFFER
            bcc :---
            lda BUFFER
            rts

DRVCODE81END = *
.export DRVCODE81END

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (!LOAD_ONCE) & (!UNINSTALL_RUNS_DINSTALL)
    DIRBUFFSIZE      = (BLOCKBUFFER - *) / 4
    DIRTRACKS        = *
    DIRSECTORS       = DIRTRACKS + DIRBUFFSIZE
    FILENAMEHASHVAL0 = DIRSECTORS + DIRBUFFSIZE
    FILENAMEHASHVAL1 = FILENAMEHASHVAL0 + DIRBUFFSIZE
    DIRBUFFEND       = FILENAMEHASHVAL1 + DIRBUFFSIZE

            .assert DIRBUFFSIZE >= 9, error, "***** Dir buffer too small. *****"

    DIRBUFFSIZE81 = DIRBUFFSIZE
    .export DIRBUFFSIZE81
.endif

.if !UNINSTALL_RUNS_DINSTALL
            .assert * <= BLOCKBUFFER, error, "***** 1581 drive code too large. *****"
.endif
            ; entry point

dinstall:   jsr drivebusy81; does sei, also signal to the computer
                           ; that the custom drive code has taken over

:           lda CIA_PRB; wait for DATA IN = high
            lsr
instalwait: bcc :-
            ldx #.lobyte(drvcodebeg81 - $01)
dgetrout:   inx
            bne :+
            inc dgetputhi
:           jsr dgetbyte
dgetputhi = * + $02
            sta a:.hibyte(drvcodebeg81 - $01) << 8,x
            cpx #.lobyte(drivebusy81 - $01)
            bne dgetrout
            dec drvcodebeg81
            bne dgetrout
            jmp dcodinit

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) & (LOAD_ONCE = 0) & (UNINSTALL_RUNS_DINSTALL)
    DIRBUFFSIZE      = (BLOCKBUFFER - *) / 4
    DIRTRACKS        = *
    DIRSECTORS       = DIRTRACKS + DIRBUFFSIZE
    FILENAMEHASHVAL0 = DIRSECTORS + DIRBUFFSIZE
    FILENAMEHASHVAL1 = FILENAMEHASHVAL0 + DIRBUFFSIZE
    DIRBUFFEND       = FILENAMEHASHVAL1 + DIRBUFFSIZE

            .assert DIRBUFFSIZE >= 9, error, "***** Dir buffer too small. *****"

    DIRBUFFSIZE81 = DIRBUFFSIZE
    .export DIRBUFFSIZE81
.endif

.if UNINSTALL_RUNS_DINSTALL
            .assert * <= BLOCKBUFFER, error, "***** 1581 drive code too large. *****"
.endif

drvprgend81:
            .reloc
