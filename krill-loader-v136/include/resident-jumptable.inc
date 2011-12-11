
.include "loader.inc"
.include "option-check.inc"
.include "common-features.inc"

.ifdef DYNLINK_EXPORT
    .out "Exported resident dynamic link symbols:"
.elseif DYNLINK_IMPORT
    .out "Requested resident dynamic link symbols:"
.elseif JUMP_TABLE
    .out "Resident jump table:"
.endif

;------------------------------------------------------------------------------

.macro DEFAULT_INTERRUPT_RESIFEATS
    ; import constraints: screen on, interrupts may be delayed, no nmis allowed, no sprites in non-irq space, is usually sufficient for applications
    ; however, this loader does not impose any restrictions on sprite, IRQ, or DMA usage when PROTOCOL = PROTOCOLS::TWO_BITS_ATN, so
    ; ARBITRARY_INTERRUPTS       = 4     ; screen on, no interrupt delays, nmis allowed, no sprite limitations
    ; is exported as offered feature (see below)

    .byte diskio::INTERRUPT_CONSTRAINTS, diskio::INTERRUPTS_POSSIBLE
    .if PLATFORM = diskio::platform::COMMODORE_16
    .out "  interrupts may be delayed, no interrupts used"    
    .else
    .out "  interrupts may be delayed, no sprites or interrupts used"
    .endif
.endmacro

.macro INTERRUPT_RESIFEATS
    .ifdef DYNLINK_EXPORT
        .byte diskio::INTERRUPT_CONSTRAINTS, diskio::ARBITRARY_INTERRUPTS
        .if PLATFORM = diskio::platform::COMMODORE_16
        .out "  screen on, no interrupt delays"
        .else
        .out "  screen on, no interrupt delays, nmis allowed, no sprite limitations"
        .endif
    .else
        .ifdef USER_INTERRUPT_RESIFEATS
            .if USER_INTERRUPT_RESIFEATS <> 0
                USER_INTERRUPT_RESIFEATS_IMPL
            .else
                DEFAULT_INTERRUPT_RESIFEATS
            .endif
        .else
            DEFAULT_INTERRUPT_RESIFEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.if HAS_DECOMPRESSOR
    .macro DEFAULT_COMPRESSION_RESIFEATS
        .byte diskio::COMPRESSION
        .if DECOMPRESSOR = DECOMPRESSORS::LEVELCRUSH
            .if LC_SPEED = 0
                .byte diskio::TABOO_LEVELCRUSH_SPEED_0
                .out "  decompression with Taboo Levelcrush at speed 0"
            .elseif LC_SPEED = 1
                .byte diskio::TABOO_LEVELCRUSH_SPEED_1
                .out "  decompression with Taboo Levelcrush at speed 1"
            .elseif LC_SPEED = 2
                .byte diskio::TABOO_LEVELCRUSH_SPEED_2
                .out "  decompression with Taboo Levelcrush at speed 2"
            .elseif LC_SPEED = 3
                .byte diskio::TABOO_LEVELCRUSH_SPEED_3
                .out "  decompression with Taboo Levelcrush at speed 3"
            .elseif LC_SPEED = 4
                .byte diskio::TABOO_LEVELCRUSH_SPEED_4
                .out "  decompression with Taboo Levelcrush at speed 4"
            .elseif LC_SPEED = 5
                .byte diskio::TABOO_LEVELCRUSH_SPEED_5
                .out "  decompression with Taboo Levelcrush at speed 5"
            .elseif LC_SPEED = 6
                .byte diskio::TABOO_LEVELCRUSH_SPEED_6
                .out "  decompression with Taboo Levelcrush at speed 6"
            .endif
        .elseif DECOMPRESSOR = DECOMPRESSORS::BYTEBOOZER
            .byte diskio::BYTEBOOZER
            .out "  decompression with ByteBoozer"
        .elseif DECOMPRESSOR = DECOMPRESSORS::PUCRUNCH
            .byte diskio::PUCRUNCH
            .out "  decompression with PuCrunch"
        .elseif DECOMPRESSOR = DECOMPRESSORS::EXOMIZER
            .byte diskio::EXOMIZER
            .out "  decompression with Exomizer"
        .endif
    .endmacro

    .macro COMPRESSION_RESIFEATS
        .ifdef DYNLINK_EXPORT
            DEFAULT_COMPRESSION_RESIFEATS
        .else
            .ifdef USER_COMPRESSION_RESIFEATS
                .if USER_COMPRESSION_RESIFEATS <> 0
                    USER_COMPRESSION_RESIFEATS_IMPL
                .else
                    DEFAULT_COMPRESSION_RESIFEATS
                .endif
            .else
                DEFAULT_COMPRESSION_RESIFEATS
            .endif
        .endif
    .endmacro
.endif

;------------------------------------------------------------------------------

.macro DEFAULT_LOAD_UNDER_IO_RAM_RESIFEATS
    .if LOAD_UNDER_D000_DFFF
        .byte diskio::UNDER_IO_RAM, diskio::D000_DFFF_SETS_RAM_CONFIG
        .out "  has to be able to load to $d000..$dfff"
    .else
        .out "  does not have to be able to load to $d000..$dfff"
    .endif
.endmacro

.macro LOAD_UNDER_IO_RAM_RESIFEATS
    .if PLATFORM <> diskio::platform::COMMODORE_16
        .ifdef DYNLINK_EXPORT
            .if LOAD_UNDER_D000_DFFF
                .byte diskio::UNDER_IO_RAM, diskio::D000_DFFF_SETS_RAM_CONFIG
                .out "  loads to $d000..$dfff"
            .else
                .out "  no loading to $d000..$dfff"
            .endif
        .else
            .ifdef USER_LOAD_UNDER_IO_RAM_RESIFEATS
                .if USER_LOAD_UNDER_IO_RAM_RESIFEATS <> 0
                    USER_LOAD_UNDER_IO_RAM_RESIFEATS_IMPL
                .else
                    DEFAULT_LOAD_UNDER_IO_RAM_RESIFEATS
                .endif
            .else
                DEFAULT_LOAD_UNDER_IO_RAM_RESIFEATS
            .endif
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_NON_BLOCKING_RESIFEATS
    .out "  is blocking"
.endmacro

.macro NON_BLOCKING_RESIFEATS
    .if DYNLINK_IMPORT
        .out "  is blocking"
    .else
        .ifdef USER_NON_BLOCKING_RESIFEATS
            .if USER_NON_BLOCKING_RESIFEATS <> 0
                USER_NON_BLOCKING_RESIFEATS_IMPL
            .else
                DEFAULT_NON_BLOCKING_RESIFEATS
            .endif
        .else
            DEFAULT_NON_BLOCKING_RESIFEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.if NONBLOCKING_API
    .macro DEFAULT_NON_BLOCKING_RESIFEATS_NB
        .byte diskio::NON_BLOCKING, diskio::IS_NONBLOCKING
        .out "  is non-blocking"
    .endmacro

    .macro NON_BLOCKING_RESIFEATS_NB
        .ifdef DYNLINK_EXPORT
            .byte diskio::NON_BLOCKING, diskio::IS_NONBLOCKING
            .out "  is non-blocking"
        .else
            .ifdef USER_NON_BLOCKING_RESIFEATS_NB
                .if USER_NON_BLOCKING_RESIFEATS_NB <> 0
                    USER_NON_BLOCKING_RESIFEATS_NB_IMPL
                .else
                    DEFAULT_NON_BLOCKING_RESIFEATS_NB
                .endif
            .else
                DEFAULT_NON_BLOCKING_RESIFEATS_NB
            .endif
        .endif
    .endmacro
.endif

;------------------------------------------------------------------------------

.macro DEFAULT_MEMDECOMP_TO_RESIFEATS
    .if MEM_DECOMP_TO_API
        .byte diskio::DESTINATION_OVERRIDE, diskio::DEST_OVERRIDE
        .out "  with destination override"
    .else
        .out "  without destination override"
    .endif
.endmacro

.macro MEMDECOMP_TO_RESIFEATS
    .ifdef DYNLINK_EXPORT
        .if MEM_DECOMP_TO_API
            .byte diskio::DESTINATION_OVERRIDE, diskio::DEST_OVERRIDE
            .out "  with destination override"
        .else
            .out "  without destination override"
        .endif
    .else
        .ifdef USER_MEMDECOMP_TO_RESIFEATS
            .if USER_MEMDECOMP_TO_RESIFEATS <> 0
                USER_MEMDECOMP_TO_RESIFEATS_IMPL
            .else
                DEFAULT_MEMDECOMP_TO_RESIFEATS
            .endif
        .else
            DEFAULT_MEMDECOMP_TO_RESIFEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_CHAINED_COMPD_FILES_RESIFEATS
    .if CHAINED_COMPD_FILES
        .byte diskio::CHAINED_FILES, diskio::CHAINED_FILES_POSSIBLE
        .out "  must allow for chained files"
    .else
        .out "  does not need to allow for chained files"
    .endif
.endmacro

.macro CHAINED_COMPD_FILES_RESIFEATS
    .ifdef DYNLINK_EXPORT
        .if CHAINED_COMPD_FILES
            .byte diskio::CHAINED_FILES, diskio::CHAINED_FILES_POSSIBLE
            .out "  allows for chained files"
        .else
            .out "  does not nallow for chained files"
        .endif
    .else
        .ifdef USER_CHAINED_COMPD_FILES_RESIFEATS
            .if USER_CHAINED_COMPD_FILES_RESIFEATS <> 0
                USER_CHAINED_COMPD_FILES_RESIFEATS_IMPL
            .else
                DEFAULT_CHAINED_COMPD_FILES_RESIFEATS
            .endif
        .else
            DEFAULT_CHAINED_COMPD_FILES_RESIFEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_LOAD_RAW_ENDADDR_RESIFEATS
    .if END_ADDRESS_API
        .byte diskio::RETURNS_ENDADDR, diskio::ENDADDR_RETURNED
        .out "  must return end address"
    .else
        .out "  does not need to return end address"
    .endif
.endmacro

.macro LOAD_RAW_ENDADDR_RESIFEATS
    .ifdef DYNLINK_EXPORT
        .if END_ADDRESS_API
            .byte diskio::RETURNS_ENDADDR, diskio::ENDADDR_RETURNED
            .out "  returns end address"
        .else
            .out "  does not return end address"
        .endif
    .else
        .ifdef USER_LOAD_RAW_ENDADDR_RESIFEATS
            .if USER_LOAD_RAW_ENDADDR_RESIFEATS <> 0
                USER_LOAD_RAW_ENDADDR_RESIFEATS_IMPL
            .else
                DEFAULT_LOAD_RAW_ENDADDR_RESIFEATS
            .endif
        .else
            DEFAULT_LOAD_RAW_ENDADDR_RESIFEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro LOADFEATS_COMMON
    PLATFORM_FEATS
    SERIAL_BUS_FEATS
    SUPPORTED_DRIVES_FEATS
    SUPPORTED_DEVICE_NUMBERS_FEATS
    FORMAT_FEATS
    INTERRUPT_RESIFEATS
    FILE_FEATS
    LOAD_UNDER_IO_RAM_RESIFEATS
    REENTRANCE_FEATS
.endmacro


; generate jump table or module function table

.if LOAD_RAW_API

    .macro LOAD_RAW_FEATURES
        .out " loadraw"
        LOADFEATS_COMMON
        LOAD_TO_FEATS
        LOAD_RAW_ENDADDR_RESIFEATS
        TIMER_FEATS
        PALNTSCACCEL_FEATS
        NON_BLOCKING_RESIFEATS
        .out "  loads unpacked"
        .out "  chained files not possible"
        .byte diskio::END_OF_LIST
    .endmacro

    .ifdef DYNLINK_EXPORT
        DYNLINKEXPORT diskio::LOAD_RAW, loadraw
        LOAD_RAW_FEATURES
    .elseif DYNLINK_IMPORT
        .byte diskio::LOAD_RAW
        LOAD_RAW_FEATURES

        loadraw = __DISKIO_RUN__
        _A_ = 3
    .else
        .export loadraw

        .if JUMP_TABLE
            .out " loadraw"
loadraw:    jmp loadraw2
        .endif
    .endif
.else
    _A_ = 0
.endif

.if LOAD_COMPD_API

    .macro LOAD_COMPRESSED_FEATURES
        .out " loadcompd"
        LOADFEATS_COMMON
        COMPRESSION_RESIFEATS
        CHAINED_COMPD_FILES_RESIFEATS
        TIMER_FEATS
        PALNTSCACCEL_FEATS
        NON_BLOCKING_RESIFEATS
        .byte diskio::END_OF_LIST
    .endmacro

    .ifdef DYNLINK_EXPORT
        DYNLINKEXPORT diskio::LOAD_COMPRESSED_EXO, loadcompd
        LOAD_COMPRESSED_FEATURES
    .elseif DYNLINK_IMPORT
        .byte diskio::LOAD_COMPRESSED_EXO
        LOAD_COMPRESSED_FEATURES

        loadcompd = __DISKIO_RUN__+_A_
        _B_ = 3
    .else
        .export loadcompd

        .if JUMP_TABLE
            .out " loadcompd"
loadcompd:  jmp loadcompd2
        .endif
    .endif
.else
    _B_ = 0
.endif

.if NONBLOCKING_API

    .macro LOAD_RAW_FEATURES_NB
        .out " loadrawnb"
        LOADFEATS_COMMON
        LOAD_TO_FEATS
        LOAD_RAW_ENDADDR_RESIFEATS
        TIMER_FEATS_NB
        PALNTSCACCEL_FEATS_NB
        NON_BLOCKING_RESIFEATS_NB
        .out "  loads unpacked"
        .out "  chained files not possible"
        .byte diskio::END_OF_LIST
    .endmacro

    .macro POLLHANDLER_FEATURES
        .out " pollhandlr"
        .byte diskio::END_OF_LIST                
    .endmacro

    .ifdef DYNLINK_EXPORT
        DYNLINKEXPORT diskio::LOAD_FILE_RAW_NB, loadrawnb
        LOAD_RAW_FEATURES_NB
        .if PLATFORM = diskio::platform::COMMODORE_16
            DYNLINKEXPORT diskio::NON_BLOCKING_IRQ_HANDLER, pollhandlr
            POLLHANDLER_FEATURES
        .endif
    .elseif DYNLINK_IMPORT
        .byte diskio::LOAD_FILE_RAW_NB
        LOAD_RAW_FEATURES_NB

        loadrawnb = __DISKIO_RUN__+_A_+_B_
        _C_ = 3
        .if PLATFORM = diskio::platform::COMMODORE_16
            .byte diskio::NON_BLOCKING_IRQ_HANDLER
            POLLHANDLER_FEATURES

            pollhandlr = __DISKIO_RUN__+_A_+_B_+_C_
            _D_ = 3
        .else
            _D_ = 0
        .endif
    .else
        .export loadrawnb
        .if PLATFORM = diskio::platform::COMMODORE_16
            .export pollhandlr
        .endif
        .if JUMP_TABLE
            .out " loadrawnb"
loadrawnb:  jmp loadrawnb2
            .if PLATFORM = diskio::platform::COMMODORE_16
            .out " pollhandlr"
pollhandlr: jmp pollhandlr2
            .endif
        .endif
    .endif
.else
    _C_ = 0
    _D_ = 0
.endif; NONBLOCKING_API

.if OPEN_FILE_POLL_BLOCK_API
    .macro OPEN_FILE_FEATURES
        .out " openfile"
        LOADFEATS_COMMON
        LOAD_TO_FEATS
        TIMER_FEATS
        PALNTSCACCEL_FEATS
        NON_BLOCKING_RESIFEATS
        .byte diskio::END_OF_LIST
    .endmacro

    .macro POLL_LOADING_FEATURES
        .out " pollblock"
        PLATFORM_FEATS
        LOAD_UNDER_IO_RAM_RESIFEATS
        LOAD_RAW_ENDADDR_RESIFEATS
        INTERRUPT_RESIFEATS
        TIMER_FEATS
        PALNTSCACCEL_FEATS
        REENTRANCE_FEATS
        NON_BLOCKING_RESIFEATS
        .byte diskio::END_OF_LIST
    .endmacro

    .ifdef DYNLINK_EXPORT
        DYNLINKEXPORT diskio::OPEN_FILE, openfile
        OPEN_FILE_FEATURES

        DYNLINKEXPORT diskio::POLL_LOADING, pollblock
        POLL_LOADING_FEATURES
    .elseif DYNLINK_IMPORT
        .byte diskio::OPEN_FILE
        OPEN_FILE_FEATURES

        openfile = __DISKIO_RUN__+_A_+_B_+_C_+_D_
        _E_ = 3

        .byte diskio::POLL_LOADING
        POLL_LOADING_FEATURES

        pollblock = __DISKIO_RUN__+_A_+_B_+_C_+_D_+_E_
        _F_ = 3
    .else
        .export openfile
        .export pollblock

        .if JUMP_TABLE & (!NONBLOCKING_API)
            .out " openfile"
openfile:   jmp openfile2
            .out " pollblock"
pollblock:  jmp pollblock2
        .endif
    .endif
.else
    _E_ = 0
    _F_ = 0
.endif

.if GETC_API

    .macro GETC_FEATURES
        .out " getc"
        PLATFORM_FEATS
        LOAD_UNDER_IO_RAM_RESIFEATS
        LOAD_RAW_ENDADDR_RESIFEATS
        INTERRUPT_RESIFEATS
        TIMER_FEATS
        PALNTSCACCEL_FEATS
        REENTRANCE_FEATS
        NON_BLOCKING_RESIFEATS
        .byte diskio::END_OF_LIST
    .endmacro

    .ifdef DYNLINK_EXPORT
        DYNLINKEXPORT diskio::READ_BYTE, getc
        GETC_FEATURES
    .elseif DYNLINK_IMPORT
        .byte diskio::READ_BYTE
        GETC_FEATURES

        getc = __DISKIO_RUN__+_A_+_B_+_C_+_D_+_E_+_F_
        _G_ = 3
    .else
        .export getc

        .if JUMP_TABLE
            .out " getc"
getc:       jmp getc2
        .endif
    .endif
.else
_G_ = 0
.endif; GETC_API

.if GETCHUNK_API

    .macro GETCHUNK_FEATURES
        .out " getchunk"
        PLATFORM_FEATS
        LOAD_UNDER_IO_RAM_RESIFEATS
        LOAD_RAW_ENDADDR_RESIFEATS
        INTERRUPT_RESIFEATS
        TIMER_FEATS
        PALNTSCACCEL_FEATS
        REENTRANCE_FEATS
        NON_BLOCKING_RESIFEATS
        .byte diskio::END_OF_LIST
    .endmacro

    .ifdef DYNLINK_EXPORT
        DYNLINKEXPORT diskio::GET_BYTES, getchunk
        GETCHUNK_FEATURES
    .elseif DYNLINK_IMPORT
        .byte diskio::GET_BYTES
        GETCHUNK_FEATURES

        getchunk = __DISKIO_RUN__+_A_+_B_+_C_+_D_+_E_+_F_+_G_
        _H_ = 3
    .else
        .export getchunk

        .if JUMP_TABLE
            .out " getchunk"
getchunk:   jmp getchunk2
        .endif
    .endif
.else
_H_ = 0
.endif; GETCHUNK_API

.if MEM_DECOMP_API

    .macro MEMDECOMP_FEATS_COMMON
        ALL_PLATFORMS_FEATS
        COMPRESSION_RESIFEATS
        .ifdef DYNLINK_EXPORT
            .byte diskio::PAL, diskio::PAL_COMPATIBLE
            .out "  PAL compatible"
            .byte diskio::NTSC, diskio::NTSC_COMPATIBLE
            .out "  NTSC compatible"
            .byte diskio::ACCELERATION, diskio::ACCELERATION_COMPATIBLE
            .out "  acceleration compatible"
        .else
            ; PAL compatibility is usually sufficient for applications
            .byte diskio::PAL, diskio::PAL_COMPATIBLE
            .out "  must be PAL compatible"
            .out "  may be NTSC compatible"
            .out "  may be acceleration compatible"
        .endif
        .if PLATFORM <> diskio::platform::COMMODORE_16
            .byte diskio::UNDER_IO_RAM, diskio::D000_DFFF_POSSIBLE
            .out " decompression to $d000..$dfff when $01 is set accordingly"
        .endif
        MEMDECOMP_TO_RESIFEATS
        INTERRUPT_RESIFEATS
        TIMER_FEATS
        REENTRANCE_FEATS
        NON_BLOCKING_RESIFEATS
    .endmacro

    .ifdef DYNLINK_EXPORT
        .out " memdecomp"
        DYNLINKEXPORT diskio::MEM_DECOMPRESS_EXO, memdecomp
        MEMDECOMP_FEATS_COMMON
        .byte diskio::END_OF_LIST
    .elseif DYNLINK_IMPORT
        .out " memdecomp"
        .byte diskio::MEM_DECOMPRESS_EXO
        MEMDECOMP_FEATS_COMMON
        .byte diskio::END_OF_LIST

        memdecomp = __DISKIO_RUN__+_A_+_B_+_C_+_D_+_E_+_F_+_G_+_H_
        _I_ = 3
    .else
        .export memdecomp

        .if JUMP_TABLE
            .out " memdecomp"
memdecomp:  jmp memdecomp2
        .endif
    .endif

    .if CHAINED_COMPD_FILES
        .ifdef DYNLINK_EXPORT
            .out " cmemdecomp"
            DYNLINKEXPORT diskio::MEM_CDECOMPRESS_EXO, cmemdecomp
            MEMDECOMP_FEATS_COMMON
            CHAINED_COMPD_FILES_RESIFEATS
            .byte diskio::END_OF_LIST
        .elseif DYNLINK_IMPORT
            .out " cmemdecomp"
            .byte diskio::MEM_CDECOMPRESS_EXO
            MEMDECOMP_FEATS_COMMON
            CHAINED_COMPD_FILES_RESIFEATS
            .byte diskio::END_OF_LIST

            cmemdecomp = __DISKIO_RUN__+_A_+_B_+_C_+_D_+_E_+_F_+_G_+_H_+_I_
            _J_ = 3
        .else
            .export cmemdecomp

            .if JUMP_TABLE
                .out " cmemdecomp"
cmemdecomp: jmp cmemdecomp2
            .endif
        .endif
    .else
        _J_ = 0
    .endif
.else
    _I_ = 0
    _J_ = 0
.endif

.if UNINSTALL_API
    .macro UNINSTALL_FEATURES
        .out " uninstall"
        PLATFORM_FEATS
        INTERRUPT_RESIFEATS
        .ifdef DYNLINK_EXPORT
            .byte diskio::PAL, diskio::PAL_COMPATIBLE
            .out "  PAL compatible"
            .byte diskio::NTSC, diskio::NTSC_COMPATIBLE
            .out "  NTSC compatible"
            .byte diskio::ACCELERATION, diskio::ACCELERATION_COMPATIBLE
            .out "  acceleration compatible"
        .else
            ; PAL compatibility is usually sufficient for applications
            .byte diskio::PAL, diskio::PAL_COMPATIBLE
            .out "  must be PAL compatible"
            .out "  may be NTSC compatible"
            .out "  may be acceleration compatible"
        .endif
        REENTRANCE_FEATS
        .if NONBLOCKING_API
        TIMER_FEATS_NB
        .byte diskio::NON_BLOCKING, diskio::IS_NONBLOCKING
        .else
        .out "  for non-blocking routines"
        TIMER_FEATS
        NON_BLOCKING_RESIFEATS
        .endif
        .byte diskio::END_OF_LIST
    .endmacro

    .ifdef DYNLINK_EXPORT
        DYNLINKEXPORT diskio::UNINSTALL, uninstall
        UNINSTALL_FEATURES
    .elseif DYNLINK_IMPORT
        .byte diskio::UNINSTALL
        UNINSTALL_FEATURES

        uninstall = __DISKIO_RUN__+_A_+_B_+_C_+_D_+_E_+_F_+_G_+_H_+_I_+_J_
        _K_ = 3
    .else
        .export uninstall

        .if JUMP_TABLE
            .out " uninstall"
uninstall:  jmp uninstall2
        .endif
    .endif
.else
    _K_ = 0
.endif; UNINSTALL_API


.if .defined(DYNLINK_EXPORT) | DYNLINK_IMPORT
    .byte diskio::END_OF_LIST
    .ifdef DYNLINK_EXPORT
base:
    .endif

    .if DYNLINK_IMPORT
        RESIDENTJUMPTABLESIZE    = _A_+_B_+_C_+_D_+_E_+_F_+_G_+_H_+_I_+_J_+_K_
        NUMIMPORTEDRESIDENTFUNCS = RESIDENTJUMPTABLESIZE / 3
        .if JUMP_TABLE
        DYNLINKOVERHEAD          = (NUMIMPORTEDINSTALLFUNCS + NUMIMPORTEDRESIDENTFUNCS) * 2
        .else
        DYNLINKOVERHEAD          = RESIDENTJUMPTABLESIZE + (NUMIMPORTEDINSTALLFUNCS + NUMIMPORTEDRESIDENTFUNCS) * 2
        .endif

        loadstatus               = .lobyte(__DISKIO_ZP_RUN__ + diskio::LOADSTATUSOFFS)

        loadaddrlo               = .lobyte(__DISKIO_ZP_RUN__ + diskio::LOADADDRLOOFFS)
        loadaddrhi               = .lobyte(__DISKIO_ZP_RUN__ + diskio::LOADADDRHIOFFS)

        decdestlo                = .lobyte(__DISKIO_ZP_RUN__ + diskio::DECDESTLOOFFS)
        decdesthi                = .lobyte(__DISKIO_ZP_RUN__ + diskio::DECDESTHIOFFS)

        endaddrlo                = .lobyte(__DISKIO_ZP_RUN__ + diskio::ENDADDRLOOFFS)
        endaddrhi                = .lobyte(__DISKIO_ZP_RUN__ + diskio::ENDADDRHIOFFS)

        bytesloadedlo            = .lobyte(__DISKIO_ZP_RUN__ + diskio::BYTESLOADEDLOOFFS)
        bytesloadedhi            = .lobyte(__DISKIO_ZP_RUN__ + diskio::BYTESLOADEDHIOFFS)

        param4                   = .lobyte(__DISKIO_ZP_RUN__ + diskio::PARAM4OFFS)
        param5                   = .lobyte(__DISKIO_ZP_RUN__ + diskio::PARAM5OFFS)
    .endif
.endif
