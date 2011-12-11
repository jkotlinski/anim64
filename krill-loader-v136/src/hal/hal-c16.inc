
.ifndef _HAL_C16_INC_
_HAL_C16_INC_ = 1

.include "cpu.inc"
.include "ted.inc"


.define CHECK_INSTALL_END_ADDRESS        .assert * <= $8000, error, "***** Error: the install code must not exceed $8000, please make sure the DISKIO_INSTALL segment ends below $8000. *****"
.if LOAD_VIA_KERNAL_FALLBACK
    .define CHECK_RESIDENT_START_ADDRESS; no restrictions
    .define CHECK_RESIDENT_END_ADDRESS   .assert * <= $8000, error, "***** Error: The resident code must not exceed $8000, please make sure the DISKIO segment ends below $8000. *****"
.else
    .define CHECK_RESIDENT_START_ADDRESS .assert * <= $fd00, error, "***** Error: The resident code must not exceed $fd00, please make sure the DISKIO segment ends below $fd00. *****"
    .define CHECK_RESIDENT_END_ADDRESS   .assert * <= $fd00, error, "***** Error: The resident code must not exceed $fd00, please make sure the DISKIO segment ends below $fd00. *****"
.endif


.macro INIT_KERNAL_SERIAL_ROUTINES
            ; pucrunch trashes $f9, restore it
            lda #$00
            sta USE4DY
.endmacro

.macro PREPARE_DRIVE_DISTURBACE_VALIDATION
            ; disregard drive if it is a 1551, as
            ; it is not connected to the serial bus
            lda #$01
            sta USE4DY
.endmacro

.macro BRANCH_IF_DRIVE_DOES_NOT_DISTURB_SERIAL_BUS to
            lda USE4DY; calling LISTEN will set USE4DY to $00 if 1551 at #9,
            eor #$01  ; $30 if 1551 at #8, and leave it at $01 otherwise,
            bne to    ; then drvlistn will return with $80, $b0, or $01
.endmacro


.macro ENABLE_KERNAL_SERIAL_ROUTINES
            sta TED_ROM_ENABLE
.endmacro

.macro ENABLE_KERNAL_SERIAL_ROUTINES_Y
            sta TED_ROM_ENABLE
.endmacro

.macro ENABLE_ALL_RAM
            sta TED_RAM_ENABLE
.endmacro

.macro ENABLE_ALL_RAM_Y
            sty TED_RAM_ENABLE
.endmacro

.macro GET_MEMCONFIG
            lda #ROM_IS_ENABLED
            and TED_CHARGEN_ADDR
.endmacro

.macro SET_MEMCONFIG
            .local use_ram

            sta TED_RAM_ENABLE
            beq use_ram
            sta TED_ROM_ENABLE
use_ram:
.endmacro

.macro SET_MEMCONFIG_Y
            .local use_ram

            sty TED_RAM_ENABLE
            beq use_ram
            sty TED_ROM_ENABLE
use_ram:
.endmacro


.ifndef IO_PORT_DIR_COMMON
    IO_PORT_DIR_COMMON = IO_PORT_SERIAL_DATA_IN_INPUT | IO_PORT_SERIAL_CLK_IN_INPUT | IO_PORT_CST_MTR_OUTPUT | IO_PORT_SERIAL_DATA_OUT_OUTPUT | IO_PORT_SERIAL_CLK_OUT_OUTPUT; $0b

                                            ; effectively, this is          ; effectively, this is  
                                            ; the idle flag:                ; the KERNAL flag:
                                            ; 0 = input  = idle             ; 0 = input  = KERNAL,  
                                            ; 1 = output = not idle         ; 1 = output = loader   
    IO_PORT_DIR_KERNAL = IO_PORT_DIR_COMMON | IO_PORT_SERIAL_ATN_OUT_OUTPUT | IO_PORT_CST_RD_INPUT ; $0f

    IO_PORT_DIR_IDLE   = IO_PORT_DIR_COMMON | IO_PORT_SERIAL_ATN_OUT_INPUT  | IO_PORT_CST_RD_OUTPUT; $1b

    IO_PORT_DIR_WAKEUP = IO_PORT_DIR_COMMON | IO_PORT_SERIAL_ATN_OUT_OUTPUT | IO_PORT_CST_RD_OUTPUT; $1f
.endif

CLOCK                  = (0 & IO_PORT_SERIAL_DATA_OUT) | (0 & IO_PORT_SERIAL_CLK_OUT)
CLOCK_ATN_HI           = CLOCK |      IO_PORT_SERIAL_ATN_OUT ; 1st and 3rd bit pairs
CLOCK_ATN_LO           = CLOCK | (0 & IO_PORT_SERIAL_ATN_OUT); 2nd and 4th bit pairs


.macro ENABLE_WAITBUSY_KERNAL
            lda #~IO_PORT_SERIAL_CLK_OUT
            and IO_PORT
            sta IO_PORT
.endmacro

.macro INIT_CLEAR_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            lda #IO_PORT_CST_MTR | (0 & IO_PORT_SERIAL_DATA_OUT) | (0 & IO_PORT_SERIAL_CLK_OUT) | (0 & IO_PORT_SERIAL_ATN_OUT); $08
            sta IO_PORT
.endmacro

.macro CLEAR_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            INIT_CLEAR_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
.endmacro

.ifndef SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT_DEFINED
    SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT_DEFINED = 1
    .macro SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            lda #IO_PORT_CST_MTR | IO_PORT_SERIAL_DATA_OUT | (0 & IO_PORT_SERIAL_CLK_OUT) | (0 & IO_PORT_SERIAL_ATN_OUT); $09
            sta IO_PORT
    .endmacro
.endif

.macro BRANCH_IF_IDLE to
            ldx #IO_PORT_DIR_IDLE
            cpx IO_PORT_DIRECTION
            beq to
.endmacro

.macro BRANCH_IF_NOT_IDLE to
            lda #IO_PORT_DIR_IDLE
            cmp IO_PORT_DIRECTION
            bne to
.endmacro

.if LOAD_VIA_KERNAL_FALLBACK | LOAD_ONCE | INSTALL_FROM_DISK
            KERNALFILENO = $02
.endif

.if LOAD_VIA_KERNAL_FALLBACK
    .macro BRANCH_IF_INSTALLED to
            lda #IO_PORT_DIR_KERNAL
            cmp IO_PORT_DIRECTION
            bne to
    .endmacro

    .macro BRANCH_IF_NOT_INSTALLED to
            lda #IO_PORT_DIR_KERNAL
            cmp IO_PORT_DIRECTION
            beq to
    .endmacro
.endif

.ifndef PUSH_CLOCKCONFIG_AND_FORCE_SINGLE_CLOCK_DEFINED
    PUSH_CLOCKCONFIG_AND_FORCE_SINGLE_CLOCK_DEFINED = 1
    .macro PUSH_CLOCKCONFIG_AND_FORCE_SINGLE_CLOCK
            .local singleclk

            lda TED_CHARGEN_ADDR
            and #FORCE_SINGLE_CLOCK
            pha
            bne singleclk
            lda #FORCE_SINGLE_CLOCK
            php
            sei                                 ; 2
            ora TED_CHARGEN_ADDR                ; 4
            sta TED_CHARGEN_ADDR                ; 4
            plp                                 ; 4
singleclk:                                      ; = 14
    .endmacro
.endif

.ifndef POP_CLOCKCONFIG_DEFINED
    POP_CLOCKCONFIG_DEFINED = 1
    .macro POP_CLOCKCONFIG
            .local singleclk

            pla
            bne singleclk
            lda #~FORCE_SINGLE_CLOCK
            php
            sei                                 ; 2
            and TED_CHARGEN_ADDR                ; 4
            sta TED_CHARGEN_ADDR                ; 4
            plp                                 ; 4
singleclk:                                      ; = 14
    .endmacro
.endif

.ifndef SENDBYTE_DEFINED
    SENDBYTE_DEFINED = 1
    .macro SENDBYTE
            ; y is not changed
            ; CLK OUT must be 0

            .local sendbyte
            .local bitset

            tax
            PUSH_CLOCKCONFIG_AND_FORCE_SINGLE_CLOCK
            txa
            ldx #$08
sendbyte:   lsr
            pha
            lda IO_PORT
            ora #IO_PORT_SERIAL_DATA_OUT
            bcs bitset
            and #~IO_PORT_SERIAL_DATA_OUT
bitset:     eor #IO_PORT_SERIAL_CLK_OUT
            sta IO_PORT
            pla
            dex
            bne sendbyte
            POP_CLOCKCONFIG

            ; CLK OUT is 0
    .endmacro
.endif

.macro BRANCH_IF_DATA_IN_CLEAR to
            bit IO_PORT
            bpl to
.endmacro

.macro BRANCH_IF_CLK_IN_CLEAR to
            bit IO_PORT
            bvc to
.endmacro

.ifndef BRANCH_IF_CLK_IN_SET_DEFINED
    BRANCH_IF_CLK_IN_SET_DEFINED = 1
    .macro BRANCH_IF_CLK_IN_SET to
            bit IO_PORT
            bvs to
    .endmacro
.endif

.macro BRANCH_IF_CLK_IN_OR_DATA_IN_CLEAR to
            bit IO_PORT
            bpl to
            bvc to
.endmacro

.ifndef INSTALL_IDLE_DEFINED
    INSTALL_IDLE_DEFINED = 1
    .macro INSTALL_IDLE
            lda #IO_PORT_CST_MTR | IO_PORT_SERIAL_DATA_OUT | IO_PORT_SERIAL_CLK_OUT | IO_PORT_SERIAL_ATN_OUT; $0f
            sta IO_PORT
            lda #IO_PORT_DIR_IDLE
            sta IO_PORT_DIRECTION
    .endmacro
.endif

.macro IDLE
    .if LOAD_ONCE
            ; uninstall
            ldx #IO_PORT_CST_MTR | (0 & IO_PORT_SERIAL_DATA_OUT) | (0 & IO_PORT_SERIAL_CLK_OUT) | (0 & IO_PORT_SERIAL_ATN_OUT); $08
            stx IO_PORT
            ldx #IO_PORT_DIR_KERNAL
            stx IO_PORT_DIRECTION
    .else
            ldx #IO_PORT_CST_MTR | IO_PORT_SERIAL_DATA_OUT | IO_PORT_SERIAL_CLK_OUT | IO_PORT_SERIAL_ATN_OUT; $0f
            stx IO_PORT
            ldx #IO_PORT_DIR_IDLE
            stx IO_PORT_DIRECTION
    .endif
.endmacro

.macro WAKEUP
            SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            lda #IO_PORT_DIR_WAKEUP
            sta IO_PORT_DIRECTION
.endmacro

.macro PREPARE_ATN_STROBE
            ldx #CLOCK_ATN_HI
.endmacro

.macro BEGIN_ATN_STROBE
            stx IO_PORT
.endmacro

.macro END_ATN_STROBE
            PUSH_CLOCKCONFIG_AND_FORCE_SINGLE_CLOCK
            lda #CLOCK_ATN_LO
            sta IO_PORT
.endmacro

.macro GETBYTE
            ldx #CLOCK_ATN_HI                   ; 2
            lda IO_PORT                         ; 3
            stx IO_PORT; ATN high               ; 3
                                                ; = 18 (22 @1MHz)

            lsr                                 ; 2
            lsr                                 ; 2
            nop                                 ; 2 - delay
            nop                                 ; 2 - delay
            ldx #CLOCK_ATN_LO                   ; 2
            eor IO_PORT                         ; 3
            stx IO_PORT; ATN low                ; 3
                                                ; = 16 (19 @1MHz)

            lsr                                 ; 2
            lsr                                 ; 2
            nop                                 ; 2 - delay
            nop                                 ; 2 - delay
            ldx #CLOCK_ATN_HI                   ; 2
            eor IO_PORT                         ; 3
            stx IO_PORT; ATN high               ; 3
                                                ; = 16 (19 @1MHz)

            lsr                                 ; 2
            lsr                                 ; 2
            nop                                 ; 2 - delay
            eor #IO_PORT_SERIAL_ATN_OUT         ; 2
            ldx #CLOCK_ATN_LO                   ; 2
            eor a:IO_PORT                       ; 4 + delay
            stx IO_PORT; ATN low                ; 3
                                                ; = 17 (20 @1MHz)

                                                ; + 10 - store + loop
                                                ; = 67 (80 @1MHz)
.endmacro

.macro ENDGETBLOCK
            tax; special block number (irrelevant for regular blocks)
            POP_CLOCKCONFIG
            txa; special block number (irrelevant for regular blocks)
.endmacro

.macro SET_IO_KERNAL
            CLEAR_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            lda #IO_PORT_DIR_KERNAL
            sta IO_PORT_DIRECTION
.endmacro

.macro SETUP_TIMER
            ; nothing to do
.endmacro

.macro SET_TIMER timeout
            .local debounce
            .local column
            .local polldelayl
            .local polldelayh
            .local done

            php
            sei
            lda TED_CHARGEN_ADDR
            pha
            ora #FORCE_SINGLE_CLOCK
            sta TED_CHARGEN_ADDR
debounce:   lda TED_RASTERLINE
            ldx TED_RASTERCOLUMN
            cmp TED_RASTERLINE
            bne debounce
           ;sec
            sbc pollyoffset
            and #VERTSUBCOUNT_MASK
            tay
            txa
            lsr
            lsr
            sta column
            sec
            lda polldelayl,y
column = * + $01
            sbc #$00
            tax
            lda polldelayh,y
            sbc #$00
            bcc debounce
            stx TED_COUNTER1_LO
            sta TED_COUNTER1_HI
            pla
            sta TED_CHARGEN_ADDR
            plp
            jmp done

            ; timeout granularity is 8 scanlines XXX TODO NTSC: user-supplied flag or check?
            .ifndef SET_TIMER_ADJUST
            SET_TIMER_ADJUST = 57
            .endif
polldelayl: .repeat $08, I
                .byte .lobyte((8 - I) * CYCLES_PER_LINE_PAL + (timeout - 1) * 8 * CYCLES_PER_LINE_PAL - SET_TIMER_ADJUST)
            .endrep
polldelayh: .repeat $08, I
                .byte .hibyte((8 - I) * CYCLES_PER_LINE_PAL + (timeout - 1) * 8 * CYCLES_PER_LINE_PAL - SET_TIMER_ADJUST)
            .endrep
done:
.endmacro

.macro ENABLE_TIMER_IRQ
            lda #COUNTER_1_IRQ
            php
            sei                                 ; 2
            ora TED_IMR                         ; 4
            sta TED_IMR                         ; 4
            plp                                 ; 4
                                                ; = 14
.endmacro

.macro DISABLE_TIMER_IRQ
            lda #~COUNTER_1_IRQ
            and TED_IMR
            sta TED_IMR
.endmacro

.macro ACK_TIMER_IRQ
            lda #COUNTER_1_IRQ
            sta TED_IRR
.endmacro

.macro ACK_TIMER_IRQ_P
            pha
            lda #COUNTER_1_IRQ
            sta TED_IRR
            pla
.endmacro

.macro CONTROL_IRQ_MASK
            .local postvector

            cli
            jmp postvector

.ifdef pollhandlr
pollhandlr2:
.else
pollhandlr:
.endif
            jmp $0000
postvector:
.endmacro

.macro BRANCH_IF_IRQS_PENDING to
            lda #RASTER_IRQ | COUNTER_2_IRQ | COUNTER_3_IRQ
            and TED_IRR
            and TED_IMR
            bne to
.endmacro

.macro SET_IRQ_VECTOR address
            lda #.lobyte(address)
            sta pollhandlr + $01
            lda #.hibyte(address)
            sta pollhandlr + $02
.endmacro

.macro LEAVE_POLL_HANDLER
            rts
.endmacro

.endif; !_HAL_C16_INC_
