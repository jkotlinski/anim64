
.ifndef _HAL_C64_INC_
_HAL_C64_INC_ = 1

.include "cia.inc"


.define CHECK_INSTALL_END_ADDRESS        .assert * <= $d000, error, "***** Error: the install code must not exceed $d000, please make sure the DISKIO_INSTALL segment ends below $d000. *****"
.if LOAD_VIA_KERNAL_FALLBACK
    .define CHECK_RESIDENT_START_ADDRESS; no restrictions
    .define CHECK_RESIDENT_END_ADDRESS   .assert * <= $d000, error, "***** Error: The resident code must not exceed $d000, please make sure the DISKIO segment ends below $d000. *****"
.else
    .define CHECK_RESIDENT_START_ADDRESS .assert (* <= $d000) | (* >= $e000), error, "***** Error: The resident code must not reside at $d000..$dfff, please make sure the DISKIO segment does not overlap with that memory range. *****"
    .define CHECK_RESIDENT_END_ADDRESS   .assert (* <= $d000) | (* >= $e000), error, "***** Error: The resident code must not reside at $d000..$dfff, please make sure the DISKIO segment does not overlap with that memory range. *****"
.endif


.macro INIT_KERNAL_SERIAL_ROUTINES
            ; nothing to do
.endmacro

.macro PREPARE_DRIVE_DISTURBACE_VALIDATION
            ; nothing to do
.endmacro

.macro BRANCH_IF_DRIVE_DOES_NOT_DISTURB_SERIAL_BUS to
            ; nothing to do
.endmacro


.macro ENABLE_KERNAL_SERIAL_ROUTINES
            lda #MEMCONFIG_IO_KERNAL
            sta IO_PORT
.endmacro

.macro ENABLE_KERNAL_SERIAL_ROUTINES_Y
            ldy #MEMCONFIG_IO_KERNAL
            sty IO_PORT
.endmacro

.macro ENABLE_IO_SPACE
            lda #MEMCONFIG_IO
            sta IO_PORT
.endmacro

.macro ENABLE_IO_SPACE_Y
            ldy #MEMCONFIG_IO
            sty IO_PORT
.endmacro

.macro ENABLE_ALL_RAM
            lda #MEMCONFIG_ALL_RAM
            sta IO_PORT
.endmacro

.macro ENABLE_ALL_RAM_X
            ldx #MEMCONFIG_ALL_RAM
            stx IO_PORT
.endmacro

.macro ENABLE_ALL_RAM_Y
            ldy #MEMCONFIG_ALL_RAM
            sty IO_PORT
.endmacro

.macro GET_MEMCONFIG
            lda IO_PORT
.endmacro

.macro SET_MEMCONFIG
            sta IO_PORT
.endmacro

.macro SET_MEMCONFIG_X
            stx IO_PORT
.endmacro

.macro SET_MEMCONFIG_Y
            sty IO_PORT
.endmacro


.ifdef _CIA_INC_; is not defined if via.inc had been included before cia.inc

    .ifndef CIA2_DDRA_COMMON
        ; CIA2 DDRA ($DD02) definitions
        CIA2_DDRA_COMMON   = CIA_SERIAL_DATA_IN_INPUT | CIA_SERIAL_CLK_IN_INPUT | CIA_VIC2_BANK_OUTPUT; $03
                                                                                                                                   ; effectively, this is
                                                                                                                                   ; the KERNAL flag:
                                                                                                                                   ; 0 = input  = loader,
        ; DATA OUT, CLK OUT, ATN out are low; RS232 is low                                                                         ; 1 = output = KERNAL
        CIA2_DDRA_KERNAL   = CIA2_DDRA_COMMON | CIA_SERIAL_DATA_OUT_OUTPUT | CIA_SERIAL_CLK_OUT_OUTPUT | CIA_SERIAL_ATN_OUT_OUTPUT | CIA_RS232_OUTPUT; $3f

        ; DATA OUT, CLK OUT, ATN OUT are high; RS232 is high; all bits except bank bits are inputs so that $dd00 writes do not change the bus state
        CIA2_DDRA_IDLE     = CIA2_DDRA_COMMON | CIA_SERIAL_DATA_OUT_INPUT  | CIA_SERIAL_CLK_OUT_INPUT  | CIA_SERIAL_ATN_OUT_INPUT  | CIA_RS232_INPUT ; $03

        ; DATA OUT is high, CLK OUT, ATN out are low; RS232 is high
        CIA2_DDRA_WAKEUP   = CIA2_DDRA_COMMON | CIA_SERIAL_DATA_OUT_INPUT  | CIA_SERIAL_CLK_OUT_OUTPUT | CIA_SERIAL_ATN_OUT_OUTPUT | CIA_RS232_INPUT ; $1b
    .endif; !CIA2_DDRA_COMMON

    CLOCK                  = CIA2_DDRA_COMMON | CIA_SERIAL_DATA_OUT_OUTPUT | CIA_SERIAL_CLK_OUT_OUTPUT
    CLOCK_ATN_HI           = CLOCK | CIA_SERIAL_ATN_OUT_INPUT  | CIA_RS232_INPUT; 1st and 3rd bit pairs ; $33
    CLOCK_ATN_LO           = CLOCK | CIA_SERIAL_ATN_OUT_OUTPUT | CIA_RS232_INPUT; 2nd and 4th bit pairs ; $3b

    .macro ENABLE_WAITBUSY_KERNAL
            lda #~CIA_SERIAL_CLK_OUT_OUTPUT
            php
            sei
            and CIA2_PRA
            sta CIA2_PRA
            plp
    .endmacro

    .macro INIT_CLEAR_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            ; this is executed during install and openfile if IDLE_BUS_LOCK != 0
            ; hence it accesses the port register rather than the data direction register
            lda #(~CIA_SERIAL_DATA_OUT_OUTPUT) & (~CIA_SERIAL_CLK_OUT_OUTPUT) & (~CIA_SERIAL_ATN_OUT_OUTPUT)
            and CIA2_PRA
            sta CIA2_PRA
    .endmacro

    .macro CLEAR_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            lda #CLOCK_ATN_LO
            sta CIA2_DDRA
    .endmacro

    .ifndef SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT_DEFINED
        SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT_DEFINED = 1
        .macro SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            lda #CIA2_DDRA_WAKEUP
            sta CIA2_DDRA
        .endmacro
    .endif

    .macro BRANCH_IF_IDLE to
            ldx #CIA2_DDRA_IDLE
            cpx CIA2_DDRA
            beq to
    .endmacro

    .macro BRANCH_IF_NOT_IDLE to
            lda #CIA2_DDRA_IDLE
            cmp CIA2_DDRA
            bne to
    .endmacro

    .if LOAD_VIA_KERNAL_FALLBACK | LOAD_ONCE | INSTALL_FROM_DISK
        KERNALFILENO = $02
    .endif

    .if LOAD_VIA_KERNAL_FALLBACK
        .macro BRANCH_IF_INSTALLED to
            lda #CIA2_DDRA_KERNAL
            cmp CIA2_DDRA
            bne to
        .endmacro

        .macro BRANCH_IF_NOT_INSTALLED to
            lda #CIA2_DDRA_KERNAL
            cmp CIA2_DDRA
            beq to
        .endmacro
    .endif; LOAD_VIA_KERNAL_FALLBACK

    .ifndef SENDBYTE_DEFINED
        SENDBYTE_DEFINED = 1
        .macro SENDBYTE
            ; y is not changed
            ; CLK OUT must be 0

            .local sendbyte
            .local bitset

            ldx #$08
sendbyte:   lsr
            pha
            lda CIA2_DDRA
            and #~CIA_SERIAL_DATA_OUT_OUTPUT
            bcs bitset
            ora #CIA_SERIAL_DATA_OUT_OUTPUT
bitset:     eor #CIA_SERIAL_CLK_OUT_OUTPUT
            sta CIA2_DDRA
            pla
            nop
            dex
            bne sendbyte

            ; CLK OUT is 0
        .endmacro
    .endif

    .macro BRANCH_IF_DATA_IN_CLEAR to
            bit CIA2_PRA
            bpl to
    .endmacro

    .macro BRANCH_IF_CLK_IN_CLEAR to
            bit CIA2_PRA
            bvc to
    .endmacro

    .ifndef BRANCH_IF_CLK_IN_SET_DEFINED
        BRANCH_IF_CLK_IN_SET_DEFINED = 1
        .macro BRANCH_IF_CLK_IN_SET to
            bit CIA2_PRA
            bvs to
        .endmacro
    .endif

    .macro BRANCH_IF_CLK_IN_OR_DATA_IN_CLEAR to
            bit CIA2_PRA
            bpl to
            bvc to
    .endmacro

    .ifndef INSTALL_IDLE_DEFINED
        INSTALL_IDLE_DEFINED = 1
        .macro INSTALL_IDLE
            lda #CIA2_DDRA_IDLE
            sta CIA2_DDRA
        .endmacro
    .endif

    .macro IDLE
        .if LOAD_ONCE
            ; uninstall
            ldx #CIA2_DDRA_KERNAL
        .else
            ldx #CIA2_DDRA_IDLE
        .endif
            stx CIA2_DDRA
    .endmacro

    .macro WAKEUP
        .if IDLE_BUS_LOCK
            ; when the loader is idle, the user is
            ; allowed to write anything to CIA2_DDRA ($dd00) -
            ; set it to a known and valid state here
            INIT_CLEAR_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
        .endif
            ldy #CIA2_DDRA_WAKEUP
            sty CIA2_DDRA
    .endmacro

    .macro PREPARE_ATN_STROBE
            ldx #CLOCK_ATN_HI
    .endmacro

    .macro BEGIN_ATN_STROBE
            stx CIA2_DDRA
    .endmacro

    .macro END_ATN_STROBE
            lda #CLOCK_ATN_LO
            sta CIA2_DDRA - CLOCK_ATN_HI,x
    .endmacro

    .macro GETBYTE
            ; note: between each store to CIA2_DDRA to signal ready for the next bit pair,
            ;       10 cycles must pass before read access of CIA2_PRA:
            ;       the drive needs max. 14 cycles to respond -
            ;       this means 18 cycles per bit pair are minimum

            ldx #CLOCK_ATN_HI                   ; 2
            lda CIA2_PRA                        ; 4 - 10...X..
            stx CIA2_DDRA; sync 1: set ATN high ; 4
                                                ; = 22 (19 on drive), 3 too many

            lsr                                 ; 2 - .10...X.
            lsr                                 ; 2 - ..10...X
            nop                                 ; 2 - delay
            nop                                 ; 2 - delay
            ldx #CLOCK_ATN_LO                   ; 2
            ora CIA2_PRA - CLOCK_ATN_LO,x       ; 5 - 3210...X + delay
            stx CIA2_DDRA; sync 2: set ATN low  ; 4
                                                ; = 19 (19 on drive), 1 more than the theoretical limit

            lsr                                 ; 2 - .3210...
            lsr                                 ; 2 - ..3210..
            nop                                 ; 2 - delay
            eor #RS232_OUT                      ; 2 - ..321O.., KERNAL flag, is read as 1 in non-KERNAL mode
            ldx #CLOCK_ATN_HI                   ; 2
            eor CIA2_PRA - CLOCK_ATN_HI,x       ; 5 - 543210.. + delay
            stx CIA2_DDRA; sync 3: set ATN high ; 4
                                                ; = 19 (19 on drive), 1 more than the theoretical limit

            lsr                                 ; 2 - .543210.
            lsr                                 ; 2 - ..543210
            sta :+ + $01                        ; 4
            lda #SERIAL_DATA_IN | SERIAL_CLK_IN ; 2 - mask
            ldx #CLOCK_ATN_LO                   ; 2
            and CIA2_PRA                        ; 4 - 76......
            stx CIA2_DDRA; sync 4: set ATN low  ; 4
                                                ; = 20 (22 on drive), 2 too few

:           ora #$00; this is done to make sure ; 2 - 76543210
                    ; the VIC bank currently    ;
                    ; active is irrelevant      : + 10 - store + loop (sta,y iny bne)
                                                ; = 80 (79 on drive), 8 more than the theoretical lower limit
    .endmacro

    .macro ENDGETBLOCK
            ; nothing to do
    .endmacro

    .macro SET_IO_KERNAL
            lda #CIA2_DDRA_KERNAL
            sta CIA2_DDRA
    .endmacro

.endif; !_CIA_INC_

.macro SETUP_TIMER
            .local ntsc

            ; time-out granularity is 8 scanlines XXX TODO NTSC: user-supplied flag or check?
            ;lda #.lobyte(CYCLES_PER_LINE_NTSC * 8 - 1)
            ;bcs ntsc
            lda #.lobyte(CYCLES_PER_LINE_PAL * 8 - 1)
ntsc:       sta CIA2_TA_LO
            lda #.hibyte(CYCLES_PER_LINE_PAL * 8 - 1)
            sta CIA2_TA_HI
            lda #$00
            sta CIA2_TB_LO
            sta CIA2_TB_HI
            lda #FORCE_LOAD | TIMER_START
            sta CIA2_CRA
.endmacro

.macro SET_TIMER timeout
            lda #timeout - 1
            sta CIA2_TB_LO
            lda #COUNT_TA_UNDF | FORCE_LOAD | TIMER_START
            sta CIA2_CRB
.endmacro

.macro ENABLE_TIMER_IRQ
            lda #CIA_SET_INTF | TIMERB_IRQ
            sta CIA2_ICR
.endmacro

.macro DISABLE_TIMER_IRQ
            lda #CIA_CLR_INTF | TIMERB_IRQ
            sta CIA2_ICR
.endmacro

.macro ACK_TIMER_IRQ
            bit CIA2_ICR
.endmacro

.macro ACK_TIMER_IRQ_P
            bit CIA2_ICR
.endmacro

.macro CONTROL_IRQ_MASK
            .local noirqs

            php
            pla
            ldx #OPC_NOP
            and #FLAG_I
            bne noirqs
            ; irqs are enabled
            lda #OPC_BNE
            ldx #OPC_CLI
            SKIPWORD
noirqs:     lda #OPC_BIT_ZP
            sta pseiclisw1
            sta pseiclisw2
            stx pseiclisw3
.endmacro

.macro BRANCH_IF_IRQS_PENDING to
            pla
            pha
            and #FLAG_I
    .ifdef pseiclisw1
pseiclisw2: bne to
    .else
pseiclisw1: bne to
    .endif
.endmacro

NB_KERNAL_ON = LOAD_VIA_KERNAL_FALLBACK | NONBLOCKING_WITH_KERNAL_ON

.macro SET_IRQ_VECTOR address
            lda #.lobyte(address)
    .if NB_KERNAL_ON
            sta NMINV + $00
    .endif
            sta NMI_VECTORLO
            lda #.hibyte(address)
    .if NB_KERNAL_ON
            sta NMINV + $01
    .endif
            sta NMI_VECTORHI
.endmacro

.macro LEAVE_POLL_HANDLER
            rti
.endmacro

.endif; !_HAL_C64_INC_
