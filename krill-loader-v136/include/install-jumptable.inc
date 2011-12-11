
.include "loader.inc"
.include "option-check.inc"
.include "common-features.inc"

.ifdef DYNLINK_EXPORT
    .out "Exported install dynamic link symbols:"
.elseif DYNLINK_IMPORT
    .out "Requested install dynamic link symbols:"
.elseif JUMP_TABLE
    .out "Install jump table:"
.endif

;------------------------------------------------------------------------------

.macro DEFAULT_INTERRUPT_INSTFEATS
    ; when installing the drive routines, screen on is usually sufficient for applications
    .byte diskio::INTERRUPT_CONSTRAINTS, diskio::SCREEN_REMAINS_ON
    .if PLATFORM = diskio::platform::COMMODORE_16
    .out "  interrupts may be delayed"
    .else
    .out "  interrupts may be delayed, no sprites or nmis used"
    .endif
.endmacro

.macro INTERRUPT_INSTFEATS
    .ifdef DYNLINK_EXPORT
        .byte diskio::INTERRUPT_CONSTRAINTS, diskio::INTERRUPTS_POSSIBLE
        .if PLATFORM = diskio::platform::COMMODORE_16
        .out "  interrupts may be delayed"
        .else
        .out "  interrupts may be delayed, no nmis allowed, no sprites in non-irq space"
        .endif
    .else
        .ifdef USER_INTERRUPT_INSTFEATS
            .if USER_INTERRUPT_INSTFEATS <> 0
                USER_INTERRUPT_INSTFEATS_IMPL
            .else
                DEFAULT_INTERRUPT_INSTFEATS
            .endif
        .else
            DEFAULT_INTERRUPT_INSTFEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.if !NONBLOCKING_API

    .macro DEFAULT_NON_BLOCKING_INSTFEATS
        .out "  does not enable non-blocking routines"
    .endmacro

    .macro NON_BLOCKING_INSTFEATS
        .ifdef DYNLINK_EXPORT
            .out "  does not enable non-blocking routines"
        .else
            .ifdef USER_NON_BLOCKING_INSTFEATS
                .if USER_NON_BLOCKING_INSTFEATS <> 0
                    USER_NON_BLOCKING_INSTFEATS_IMPL
                .else
                    DEFAULT_NON_BLOCKING_INSTFEATS
                .endif
            .else
                DEFAULT_NON_BLOCKING_INSTFEATS
            .endif
        .endif
    .endmacro

.else

    .macro DEFAULT_NON_BLOCKING_INSTFEATS
        .byte diskio::NON_BLOCKING, diskio::IS_NONBLOCKING
        .out "  enables non-blocking routines"
    .endmacro

    .macro NON_BLOCKING_INSTFEATS
        .ifdef DYNLINK_EXPORT
            .byte diskio::NON_BLOCKING, diskio::IS_NONBLOCKING
            .out "  enables non-blocking routines"
        .else
            .ifdef USER_NON_BLOCKING_INSTFEATS
                .if USER_NON_BLOCKING_INSTFEATS <> 0
                    USER_NON_BLOCKING_INSTFEATS_IMPL
                .else
                    DEFAULT_NON_BLOCKING_INSTFEATS
                .endif
            .else
                DEFAULT_NON_BLOCKING_INSTFEATS
            .endif
        .endif
    .endmacro

.endif

;------------------------------------------------------------------------------

.macro INSTALL_FEATURES
    .out " install"
    PLATFORM_FEATS
    SERIAL_BUS_FEATS
    SUPPORTED_DRIVES_FEATS
    SUPPORTED_DEVICE_NUMBERS_FEATS
    FORMAT_FEATS
    INTERRUPT_INSTFEATS
    .if LOAD_ONCE
        FILE_FEATS
        LOAD_TO_FEATS
    .endif
    REENTRANCE_FEATS
    NON_BLOCKING_INSTFEATS
    .if !NONBLOCKING_API
    PALNTSCACCEL_FEATS
    TIMER_FEATS
    .else
    PALNTSCACCEL_FEATS_NB
    TIMER_FEATS_NB
    .endif
    .byte diskio::END_OF_LIST
.endmacro

.ifdef DYNLINK_EXPORT
    .byte diskio::INSTALL, .lobyte(install - base), .hibyte(install - base)
    INSTALL_FEATURES
.elseif DYNLINK_IMPORT
    .byte diskio::INSTALL
    INSTALL_FEATURES

    install = __DISKIO_INSTALL_RUN__
    _0_ = 3
.else
    .export install
    .if JUMP_TABLE
        .out " install"
install:    jmp install2
    .endif
.endif

.if DYNLINK_IMPORT
    .byte diskio::END_OF_LIST

    INSTALLJUMPTABLESIZE    = _0_
    NUMIMPORTEDINSTALLFUNCS = INSTALLJUMPTABLESIZE / 3
.endif
