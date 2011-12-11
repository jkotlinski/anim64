
.fileopt comment, "Loader install code portion"
.fileopt compiler, "CA65"
.fileopt author, "Gunnar Ruthenberg"

__NO_LOADER_SYMBOLS_IMPORT = 1
.include "loader.inc"
.include "../version.inc"

.include "cpu.inc"
.include "cia.inc"
.include "basic.inc"; for PETSCII_RETURN
.include "kernal.inc"
.if NONBLOCKING_API & (PLATFORM <> diskio::platform::COMMODORE_16)
    .include "vic.inc"
.endif

; including via.inc would redefine several symbols from cia.inc
VIA2_T1L_H = $1c07; this symbol is used for the fast head stepping m-w for seeking using plain KERNAL routines
VIA_T1C_H  = $1c05; this symbol is used for watchdog servicing on CMD FD


.include "hal/hal.inc"

.importzp BLOCKDESTLO

.import c1570fix0
.import c1570fix1
.import c1570fix2

.import cmdfdfix0
.import cmdfdfix1
.import cmdfdfix2
.import cmdfdfix3
.import cmdfdfix4


USE_GENERIC_DRIVE = 0


.macro itoa4 value
            .if (value & $0f > 9)
                .byte (value & $0f) + 'a' - 10
            .else
                .byte (value & $0f) + '0'
            .endif
.endmacro

.macro itoa1 value
            itoa4 value <> 0
.endmacro

.macro itoa8 value
            itoa4 value >> 4
            itoa4 value & $0f
.endmacro

.macro itoa16 value
            itoa8 value >> 8
            itoa8 value & $ff
.endmacro



.segment "EXTZP"; not used otherwise, the EXTZP segment is not
                ; optional in the o65 built-in ld65 config

.ifdef DYNLINK_EXPORT
    .macro DYNLINKEXPORT function, label
        .byte function
        .word -1; don't import now, only regard the function as available
    .endmacro

    .segment "JUMPTABLE"; this segment is required by the o65 built-in linker config file
    .segment "DATA"

    .word endinstjmptable - *; table size, i.e., offset to the code
    .word version - endinstjmptable; offset to version string

    .include "install-jumptable.inc"
    .include "resident-jumptable.inc"
endinstjmptable:
.else
    .segment "DISKIO_INSTALL"

    .ifdef INSTADDR
            .org INSTADDR - 2
            .word * + 2; load address
    .endif

    .include "install-jumptable.inc"
.endif

GENERIC_INSTALL = 0; INSTALL_FROM_DISK; XXX TODO

.if GENERIC_INSTALL = 0
            ; unfortunately, scopes must be defined before using them, this is why the install code is moved to after the drive code

.scope cbm1541
drivecode41:
            .include "drives/drivecode1541.s"
    .if UNINSTALL_RUNS_DINSTALL
            .export drivecode41 : absolute
    .endif
.endscope

.scope cbm1571
drivecode71:
            .include "drives/drivecode1571.s"
    .if UNINSTALL_RUNS_DINSTALL
            .export drivecode71 : absolute
    .endif
.endscope

.scope cbm1581
drivecode81:
            .include "drives/drivecode1581.s"
    .if UNINSTALL_RUNS_DINSTALL
            .export drivecode81 : absolute
    .endif
.endscope
.endif; !GENERIC_INSTALL



            ; Install the loader

            ; in:  x/y - if LOAD_ONCE, lo/hi to 0-terminated filename string
            ; out: c - set on error
            ;      a - status
            ;      x - drive type (one of diskio::drivetype)
            ;      y - if status is diskio::status::OK, zp address of version string address
.if NONBLOCKING_API
_install:
.else
    .ifdef install
install2:
    .else
install:
    .endif
.endif

.if LOAD_ONCE | INSTALL_FROM_DISK
            stx namestrpos + $00
            sty namestrpos + $01
.endif
            lda #.lobyte(version)
            sta BLOCKDESTLO + 0
            lda #.hibyte(version)
            sta BLOCKDESTLO + 1

            BRANCH_IF_NOT_IDLE :+
            jmp isinstalld

:           php; i-flag buffer

            INIT_KERNAL_SERIAL_ROUTINES

            ; try the drive as denoted by FA (current drive) first
            lda FA
            cmp #MIN_DEVICE_NO
            bcc :+
            cmp #MAX_DEVICE_NO + 1
            bcc :++
:           lda #MIN_DEVICE_NO; FA does not contain a drive address (MIN_DEVICE_NO..MAX_DEVICE_NO), try MIN_DEVICE_NO first
:           pha

.if PROTOCOL = PROTOCOLS::TWO_BITS_ATN
            ; check if there is more than 1 drive on the serial bus,
            ; to make sure the 2bit+ATN protocol can work alright,
            ; this is done via the low-level serial bus routines,
            ; so non-serial bus devices won't respond
            ; (1551 on Plus/4 does respond though, so a little extra
            ; treatment is done through the drive disturbance HAL macros)

            ldx #MIN_DEVICE_NO
checkbus:   stx FA
            lda #$00
            sta STATUS            
            PREPARE_DRIVE_DISTURBACE_VALIDATION
            jsr drvlistn
            BRANCH_IF_DRIVE_DOES_NOT_DISTURB_SERIAL_BUS nodrive
            jsr READST
            bmi nodrive
            pla
            eor #%10000000
            bmi firstdrive

            ; more than 1 drive on the bus or generic serial devices present
            sta FA
            jsr UNLSTN

    .if LOAD_ONCE & LOAD_VIA_KERNAL_FALLBACK
            jsr openfile; using load via KERNAL fallback, the loader can still load
    .endif
    
            plp; i-flag restore
            lda #diskio::status::TOO_MANY_DEVICES
            ldx #diskio::drivetype::DRIVE_GENERIC
            ldy #BLOCKDESTLO
    .if LOAD_VIA_KERNAL_FALLBACK
            clc; this is not to be regarded as an error
    .else
            sec
    .endif
            rts

firstdrive: pha
nodrive:    jsr UNLSTN
            ldx FA
            inx
            cpx #MAX_DEVICE_NO + 1
            bne checkbus
.endif; PROTOCOL <> PROTOCOLS::TWO_BITS_ATN

            ; find first available drive,
            ; this is done via the high-level open/read/close routines,
            ; so non-serial bus devices will also respond
            lda #diskio::status::OK
            sta STATUS
            pla; current drive
.if PROTOCOL = PROTOCOLS::TWO_BITS_ATN
            and #%01111111; clear (more-than)-one-drive flag
.endif; PROTOCOL <> PROTOCOLS::TWO_BITS_ATN
            ; find first available drive
            sta FA
find1stdrv: pha; device number
            lda #drvchkmr - drvchkon
            ldx #.lobyte(drvchkon)
            ldy #.hibyte(drvchkon)
            jsr SETNAM
            lda #COMMAND_ERROR_CHANNEL
            ldx FA
            tay
            jsr SETLFS
            jsr OPEN
            bcc drivefound; drive present
            ; drive not present, try next address
            lda #COMMAND_ERROR_CHANNEL
            jsr CLOSE
            jsr CLRCHN
            ldx FA
            inx
            cpx #MAX_DEVICE_NO + 1
            bne :+
            ldx #MIN_DEVICE_NO
:           stx FA
            pla
            cmp FA
            bne find1stdrv

            plp; i-flag restore
            lda #diskio::status::DEVICE_NOT_PRESENT
            ldx #diskio::drivetype::DEVICE_NONE
            ldy #BLOCKDESTLO
            sec
            rts

drivefound: ; read error channel to stop potentially blinking error LED
            ldx #COMMAND_ERROR_CHANNEL
            jsr CHKIN
:           jsr CHRIN
            cmp #PETSCII_RETURN
            bne :-
            lda #COMMAND_ERROR_CHANNEL
            jsr CLOSE
            jsr CLRCHN

            pla; device number

.if GENERIC_INSTALL

            ; XXX TODO

.else; !GENERIC_INSTALL

            ; check if drive allows code upload and execution
            lda #.lobyte($0300)
            ldx #.hibyte($0300)
            jsr memreadbyt
            eor #$ff
            sta drvchkval
            jsr drvlistn
            ldx #$00
:           lda drvchkmw,x
            jsr IECOUT
            inx
            cpx #drvchkme - drvchkmw
            bne :-
            jsr drvlistn - $03
            ldx #$04
:           lda drvchkme,x
            jsr IECOUT
            dex
            bpl :-
            jsr UNLSTN
            lda #.lobyte($0300)
            ldx #.hibyte($0300)
            jsr memreadbyt
            cmp drvchkval
            bne usegeneric
            
    .if USE_GENERIC_DRIVE
            jmp usegeneric
    .endif
            ; check which model the drive is and upload corresponding drive code

            ; check if running on a 1541/70/71 compatible drive
            lda #.lobyte($e5c6)
            ldx #.hibyte($e5c6)
            jsr memreadbyt

            cmp #'4'
            beq is1541
            cmp #'7'
            beq is157x

            ; neither 1541 nor 157x
            
            ; try fd2000/fd4000
            lda #.lobyte($fea4)
            ldx #.hibyte($fea4)
            jsr memreadbyt
            cmp #'f'
            bne check1581
            lda #OPC_BIT_ABS
            sta cmdfdfix0 - cbm1581::drvcodebeg81 + cbm1581::drivecode81
            lda #.lobyte($54); DIRTRACKFD
            sta cmdfdfix1 - cbm1581::drvcodebeg81 + cbm1581::drivecode81
            lda #.hibyte($54); DIRTRACKFD
            sta cmdfdfix2 - cbm1581::drvcodebeg81 + cbm1581::drivecode81
            lda #.lobyte($fef0)
            ldx #.hibyte($fef0)
            jsr memreadbyt
            ldy #diskio::drivetype::DRIVE_CMD_FD_2000
            cmp #'4'
            bne isfd2000
            iny; diskio::drivetype::DRIVE_CMD_FD_4000
isfd2000:   lda #$ff
            ldx #.lobyte(VIA_T1C_H)
            bne iscmdfd; jmp

            ; check if 1581
check1581:  lda #.lobyte($a6e9)
            ldx #.hibyte($a6e9)
            jsr memreadbyt
            ldy #diskio::drivetype::DRIVE_1581
            cmp #'8'
            beq is1581

usegeneric: ; no compatible drive found
    .if LOAD_VIA_KERNAL_FALLBACK & LOAD_ONCE
            jsr openfile; exception on error
    .endif

            plp; i-flag restore
            lda #diskio::status::DEVICE_INCOMPATIBLE
    .if diskio::status::DEVICE_INCOMPATIBLE = diskio::drivetype::DRIVE_GENERIC
            tax
    .else
            ldx #diskio::drivetype::DRIVE_GENERIC
    .endif
            ldy #BLOCKDESTLO
    .if LOAD_VIA_KERNAL_FALLBACK
            clc; this is not to be regarded as an error
    .else
            sec
    .endif
            rts

            ; select appropriate drive code

is1541:     ; find out if 1541, or 1541-C/1541-II
            lda #.lobyte($eaa3); is $fe only with 1541-C,
            ldx #.hibyte($eaa3); where it defines the data direction for
            jsr memreadbyt     ; the pin connected to the track 0 sensor
            ldy #diskio::drivetype::DRIVE_1541_C
            cmp #$fe
            beq selectdcod; branch if 1541-C
            ; 1541 or 1541-II
            lda #.lobyte($c002)
            ldx #.hibyte($c002)
            jsr memreadbyt
            ldy #diskio::drivetype::DRIVE_1541
            cmp #'c'
            bne selectdcod; 1541: branch if no 'c' at $c002 (from 'COPYRIGHT' etc.)
            ldy #diskio::drivetype::DRIVE_1541_II
            bne selectdcod; jmp; 1541-II: 'c' at $c002 (from 'COPYRIGHT' etc.)

            ; find out if 1570 or 1571
is157x:     cpx #'1' | $80; 71
            lda #OPC_BIT_ZP
            ldx #OPC_BIT_ABS; no VIA2_PRA writes to switch sides
            ldy #diskio::drivetype::DRIVE_1570
            bcc :+; branch if 1570
            ; 1571 or 1571CR
            jsr chk2sidedx
            lda #.lobyte($e5c2)
            ldx #.hibyte($e5c2)
            jsr memreadbyt
            cmp #'1'; 3.1
            lda #OPC_BCS
            ldx #OPC_STA_ABS
            ldy #diskio::drivetype::DRIVE_1571
            bcc :+
            iny; diskio::drivetype::DRIVE_1571CR
:           sta c1570fix0 - cbm1571::drvcodebeg71 + cbm1571::drivecode71
            stx c1570fix1 - cbm1571::drvcodebeg71 + cbm1571::drivecode71
            stx c1570fix2 - cbm1571::drvcodebeg71 + cbm1571::drivecode71

            ; fall through

is1581:     lda #OPC_JMP_ABS
            sta cmdfdfix0 - cbm1581::drvcodebeg81 + cbm1581::drivecode81
            lda #.lobyte($022b); DIRTRACK81
            sta cmdfdfix1 - cbm1581::drvcodebeg81 + cbm1581::drivecode81
            lda #.hibyte($022b); DIRTRACK81
            sta cmdfdfix2 - cbm1581::drvcodebeg81 + cbm1581::drivecode81
            lda #COUNT_TA_UNDF | FORCE_LOAD | ONE_SHOT | TIMER_START
            ldx #.lobyte(CIA_CRB)
iscmdfd:    sta cmdfdfix3 - cbm1581::drvcodebeg81 + cbm1581::drivecode81
            stx cmdfdfix4 - cbm1581::drvcodebeg81 + cbm1581::drivecode81


selectdcod: sty drivetype
            ldx dcodeseltb - diskio::drivetype::DRIVE_1541,y
            lda dcodeselt0,x
            sta dcodesel0
            lda dcodeselt1,x
            sta dcodesel1
            lda dcodeselt2,x
            sta dcodesel2
            lda dcodeselt3,x
            sta dcodesel3
            lda dcodeselt4,x
            sta dcodesel4
            lda dcodeselt5,x
            sta dcodesel5
            lda dcodeselt6,x
            sta dcodesel6
            lda dcodeselt7,x
            sta dcodesel7
            lda dcodeselt8,x
            sta dcodesel8
.endif; !GENERIC_INSTALL

            cpy #diskio::drivetype::DRIVE_1581
            bcs :++
            ; quicker head stepping on 1541-71
            jsr drvlistn
            ldx #$06
:           lda drvfaststp,x
            jsr IECOUT
            dex
            bpl :-
            jsr UNLSTN
:
.if LOAD_ONCE | INSTALL_FROM_DISK
            jsr openfile; exception on error
.endif
            jsr drvlistn

            ldx #$00
install1:   ldy #$05
:           lda drvrutmw,y
            jsr IECOUT
            dey
            bpl :-

            ldy #$23
loadcopy:
dcodesel0 = * + $01
dcodesel1 = * + $02
            lda a:$00,x
            jsr IECOUT
            inx
dcodesel2 = * + $01
            cpx #$00
            beq :+
            dey
            bne loadcopy
            jsr drvlistn - $03
            clc
            lda #$23
            adc drvrutmw + $02
            sta drvrutmw + $02
            bcc install1
            inc drvrutmw + $01
            bne install1

:           jsr drvlistn - $03
            ldx #$04
:           lda droutrun,x
            jsr IECOUT
            dex
            bpl :-
            jsr UNLSTN

            INIT_CLEAR_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
:           BRANCH_IF_CLK_IN_SET :-
            SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
:           BRANCH_IF_CLK_IN_CLEAR :-

dcodesel3 = * + $01
            ldy #$00
dcodesel4 = * + $02
fastinst:   lda a:$00,y
            SENDBYTE
            iny
            bne :+
            inc fastinst + $02
:           cpy dcodesel0
            bne fastinst
            ldx fastinst + $02
            cpx dcodesel1
            bne fastinst

:           BRANCH_IF_CLK_IN_SET :-; wait for the custom drive code to signal idle
            INSTALL_IDLE

            plp; i-flag restore
isinstalld: lda #diskio::status::OK
drivetype = * + $01
            ldx #$00
            ldy #BLOCKDESTLO
            clc
            rts

.if NONBLOCKING_API
            ; Install the loader and enable non-blocking routines

            ; in:  a - irq line mod 8
            ;      x/y - if LOAD_ONCE, lo/hi to 0-terminated filename string
            ; out: c - set on error
            ;      a - status
            ;      x - drive type (one of diskio::drivetype)
            ;      y - if status is diskio::status::OK, zp address of version string address

    .ifdef install
install2:
    .else
install:
    .endif

            sec
            sbc #$01
            and #%00000111
            sta pollyoffset

            jsr _install

    .if LOAD_VIA_KERNAL_FALLBACK
            sta initstatus
            bcc :+
            cmp #diskio::status::DEVICE_INCOMPATIBLE
            beq :+
        .if PROTOCOL = PROTOCOLS::TWO_BITS_ATN
            cmp #diskio::status::TOO_MANY_DEVICES
            sec
            bne piniterr
        .endif
:
    .else
            bcs piniterr
    .endif
            txa
            pha

            DISABLE_TIMER_IRQ
            ACK_TIMER_IRQ
            php
            ldx #$00
:           lda RASTERLINE
:           cmp RASTERLINE
            bne :-
            cmp #pollyoffset
            beq :+
            dex
            bne :--; this is done so the irq is blocked as late and shortly as possible

:           sei
            lda #pollyoffset
:           cmp RASTERLINE
            beq :-
:           cmp RASTERLINE; the timer is started at the approximate beginning of a raster line
            bne :-

            SETUP_TIMER

            plp
            pla
            tax
    .if LOAD_VIA_KERNAL_FALLBACK
            lda #diskio::status::OK
            sta loadstatus
initstatus = * + $01
            lda #diskio::status::OK
            cmp #diskio::status::OK + 1
            rts
    .else
            clc
            lda #diskio::status::OK
    .endif
piniterr:   sta loadstatus
            rts

.endif; NONBLOCKING_API

            jsr UNLSTN
drvlistn:   lda FA
            jsr LISTEN
            lda #SA_OPENCHANNEL | COMMAND_ERROR_CHANNEL
            jmp LSTNSA

memreadbyt: sta drvchkmr + $03
            stx drvchkmr + $04
            lda #drvchkmw - drvchkmr
            ldx #.lobyte(drvchkmr)
            ldy #.hibyte(drvchkmr)
            jsr SETNAM
            lda #COMMAND_ERROR_CHANNEL
            ldx FA
            tay
            jsr SETLFS
            jsr OPEN
            bcc :+
kernalerr:  pla
            pla
            plp; i-flag restore
            lda #diskio::status::GENERIC_KERNAL_ERROR
            ldx #diskio::drivetype::DEVICE_UNKNOWN
            ldy #BLOCKDESTLO
            sec
            rts
:           ldx #COMMAND_ERROR_CHANNEL
            jsr CHKIN
            jsr CHRIN
            pha
            jsr CHRIN
            pha
            lda #COMMAND_ERROR_CHANNEL
            jsr CLOSE
            jsr CLRCHN
            pla
            tax
            pla
            clc
            rts

.if LOAD_ONCE | INSTALL_FROM_DISK

openfile:   jsr chk2sided

            lda #KERNALFILENO
            ldx FA
            ldy #$00
            jsr SETLFS
            ldx #$ff
:           inx
namestrpos = * + $01
            lda a:$00,x
            bne :-
            txa; length
            ldx namestrpos + $00
            ldy namestrpos + $01
            jsr SETNAM
            jsr OPEN
            bcs jpkernale

            ldx #KERNALFILENO
            jsr CHKIN

            lda #COMMAND_ERROR_CHANNEL
            ldx FA
            tay
            jsr SETLFS
            lda #$00
            jsr SETNAM
            jsr OPEN
            bcc :+
            lda #KERNALFILENO
            jsr CLOSE
jpkernale:  jmp kernalerr
:
            ; read error status through the error channel
            ; so that the led won't keep flashing
            ldx #COMMAND_ERROR_CHANNEL
            jsr CHKIN
            jsr CHRIN
            cmp #'0'
            bne fileerror
            jsr CHRIN
            cmp #'0'
            beq :+
fileerror:  jsr CHRIN
            cmp #$0d; newline
            bne fileerror

            pla
            pla
            lda #COMMAND_ERROR_CHANNEL
            jsr CLOSE
            lda #KERNALFILENO
            jsr CLOSE
            jsr CLRCHN
            lda #diskio::status::FILE_NOT_FOUND
            ldx drivetype
            ldy #BLOCKDESTLO
            plp; i-flag restore
            sec
            rts
:           lda #COMMAND_ERROR_CHANNEL
            jsr CLOSE
            jmp CLRCHN

.endif; LOAD_ONCE | INSTALL_FROM_DISK

chk2sided:  lda drivetype
            cmp #diskio::drivetype::DRIVE_1571
            beq chk2sidedx
            cmp #diskio::drivetype::DRIVE_1571CR
            bne no1571

chk2sidedx: jsr drvlistn
            ldx #$04
:           lda twosided,x
            jsr IECOUT
            dex
            bpl :-
            jsr UNLSTN

no1571:     rts

drvchkon:   .byte "m-r", .lobyte($0300), .hibyte($0300)

drvchkmr:   .byte "m-r", $00, $00, $02; read forward
drvchkmw:   .byte "m-w", .lobyte($0300), .hibyte($0300), drvchkme - drvchkcd; read forward
drvchkval = * + $01
drvchkcd:   lda #$00
            sta $0300
            rts
drvchkme:   .byte .hibyte($0300), .lobyte($0300), "e-m"; read backward

dcodesel5 = * + $01
dcodesel6 = * + $02
drvrutmw:   .byte $23, $00, $00, "w-m"; read backward
dcodesel7 = * + $00
dcodesel8 = * + $01
droutrun:   .byte $00, $00, "e-m"; read backward
drvfaststp: .byte MINSTPSP, $01, .hibyte(VIA2_T1L_H), .lobyte(VIA2_T1L_H), "w-m"; read backward
twosided:   .byte "1m>0u"; read backward

.if GENERIC_INSTALL = 0

dcodeseltb: .byte diskio::drivetype::DRIVES_1541    , diskio::drivetype::DRIVES_1541    , diskio::drivetype::DRIVES_1541    ; drivecode1541 for 1541, 1541-C, 1541-II
            .byte 0; 1551
            .byte diskio::drivetype::DRIVES_157X - 1, diskio::drivetype::DRIVES_157X - 1, diskio::drivetype::DRIVES_157X - 1; drivecode1571 for 1570, 1571, 1571CR
            .byte diskio::drivetype::DRIVES_1581_CMD - 1, diskio::drivetype::DRIVES_1581_CMD - 1, diskio::drivetype::DRIVES_1581_CMD - 1; drivecode1581 for 1581, FD2000, FD4000

dcodeselt0: .byte .lobyte(cbm1541::dinstall  - cbm1541::drvcodebeg41 + cbm1541::drivecode41)
            .byte .lobyte(cbm1571::drivebusy71 - cbm1571::drvcodebeg71 + cbm1571::drivecode71)
            .byte .lobyte(cbm1581::drivebusy81 - cbm1581::drvcodebeg81 + cbm1581::drivecode81)
dcodeselt1: .byte .hibyte(cbm1541::dinstall  - cbm1541::drvcodebeg41 + cbm1541::drivecode41)
            .byte .hibyte(cbm1571::drivebusy71 - cbm1571::drvcodebeg71 + cbm1571::drivecode71)
            .byte .hibyte(cbm1581::drivebusy81 - cbm1581::drvcodebeg81 + cbm1581::drivecode81)
dcodeselt2: .byte .lobyte(cbm1541::drvprgend41 - cbm1541::drvcodeend41)
            .byte .lobyte(cbm1571::drvprgend71 - cbm1571::drivebusy71)
            .byte .lobyte(cbm1581::drvprgend81 - cbm1581::drivebusy81)
dcodeselt3: .byte .lobyte(cbm1541::drivecode41)
            .byte .lobyte(cbm1571::drivecode71)
            .byte .lobyte(cbm1581::drivecode81)
dcodeselt4: .byte .hibyte(cbm1541::drivecode41)
            .byte .hibyte(cbm1571::drivecode71)
            .byte .hibyte(cbm1581::drivecode81)
dcodeselt5: .byte .hibyte(cbm1541::dinstall)
            .byte .hibyte(cbm1571::drivebusy71)
            .byte .hibyte(cbm1581::drivebusy81)
dcodeselt6: .byte .lobyte(cbm1541::dinstall)
            .byte .lobyte(cbm1571::drivebusy71)
            .byte .lobyte(cbm1581::drivebusy81)
dcodeselt7: .byte .hibyte(cbm1541::dinstall)
            .byte .hibyte(cbm1571::dinstall)
            .byte .hibyte(cbm1581::dinstall)
dcodeselt8: .byte .lobyte(cbm1541::dinstall)
            .byte .lobyte(cbm1571::dinstall)
            .byte .lobyte(cbm1581::dinstall)

.endif; !GENERIC_INSTALL


version:    .byte "Krill's Loader, version ", REPOSITORY_VERSION, ", configuration "
            itoa4 MIN_DEVICE_NO
            itoa4 MAX_DEVICE_NO
            .byte '.'
            itoa8 PLATFORM
            .byte '.'
            itoa4 FILESYSTEM
            .byte '.'
            itoa1 LOAD_ONCE
            .byte '.'
            itoa8 DIRTRACK
            itoa8 DIRSECTOR
            itoa8 FILENAME_MAXLENGTH
            .byte '.'
            itoa8 MINSTPSP
            itoa8 MAXSTPSP
            itoa8 STEPRACC
            .byte '.'
            itoa1 UNINSTALL_RUNS_DINSTALL
            itoa1 DISABLE_WATCHDOG
            itoa1 JUMP_TABLE
            itoa1 LOAD_UNDER_D000_DFFF
            itoa1 LOAD_TO_API
            itoa1 END_ADDRESS_API
            itoa1 LOAD_RAW_API
            itoa1 LOAD_COMPD_API
            itoa1 OPEN_FILE_POLL_BLOCK_API
            itoa1 GETC_API
            itoa1 GETCHUNK_API
            itoa1 MEM_DECOMP_API
            itoa1 MEM_DECOMP_TO_API
            itoa1 CHAINED_COMPD_FILES
            itoa1 LOAD_VIA_KERNAL_FALLBACK
            itoa1 LOAD_PROGRESS_API
            itoa1 IDLE_BUS_LOCK
            itoa1 NONBLOCKING_API
            itoa1 NONBLOCKING_WITH_KERNAL_ON
            itoa1 UNINSTALL_API
            .byte '.'
            itoa4 LC_SPEED
            .byte '.'
            itoa8 POLLINGINTERVAL_STARTLOAD
            itoa8 POLLINGINTERVAL_TRACKCHANGE
            itoa8 POLLINGINTERVAL_GETBLOCK
            itoa8 POLLINGINTERVAL_GETBLOCK
            itoa8 POLLINGINTERVAL_BLOCKSOON
            itoa8 POLLINGINTERVAL_REPOLL
            .byte '.'
            itoa4 DECOMPRESSOR
            .byte '.'
            itoa1 INSTALL_FROM_DISK
           ;.byte '.'
           ;itoa1 FAST_FILE_FORMAT
           ;itoa1 FAST_ENCODING_FORMAT
           ;.byte '.'
           ;itoa1 DECOMPLOAD_TO_API
           ;itoa1 CUSTOM_DRIVE_CODE_API
           ;.byte '.'
           ;itoa1 NO_DECOMPLOAD_OPTIMIZATION
           ;.byte '.'
           ;itoa4 PROTOCOL
           ;.byte '.'
           ;itoa16 STREAM_BUFFERSIZE
            .byte 0

.ifndef DYNLINK_EXPORT
            CHECK_INSTALL_END_ADDRESS
.endif
