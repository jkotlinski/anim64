
.ifndef _COMMON_FEATURES_INC_
_COMMON_FEATURES_INC_ = 1

; building the dynamically linked library: symbol DYNLINK_EXPORT is defined, DYNLINK_IMPORT = 0
; linking to the dynamically linked library: symbol DYNLINK_EXPORT is not defined, DYNLINK_IMPORT <> 0

; features that are shared by both the install and resident functions

;------------------------------------------------------------------------------

.macro ALL_PLATFORMS_FEATS
    .byte diskio::PLATFORM, diskio::platform::COMMODORE_64
    .out "  runs on Commodore 64"
    .byte diskio::PLATFORM, diskio::platform::COMMODORE_128
    .out "  runs on Commodore 128"
    .byte diskio::PLATFORM, diskio::platform::COMMODORE_16
    .out "  runs on Commodore 16"
    .byte diskio::PLATFORM, diskio::platform::COMMODORE_VIC20
    .out "  runs on Commodore VIC20"
.endmacro

.macro DEFAULT_PLATFORM_FEATS
    .if PLATFORM = diskio::platform::COMMODORE_16
    .byte diskio::PLATFORM, diskio::platform::COMMODORE_16
    .out "  runs on Commodore +4"
    .else
    .byte diskio::PLATFORM, diskio::platform::COMMODORE_64
    .out "  runs on Commodore 64"
    .byte diskio::PLATFORM, diskio::platform::COMMODORE_128
    .out "  runs on Commodore 128"
    .endif
.endmacro

.macro PLATFORM_FEATS
    .ifdef DYNLINK_EXPORT
        .if PLATFORM = diskio::platform::COMMODORE_16
        .byte diskio::PLATFORM, diskio::platform::COMMODORE_16
        .out "  runs on Commodore 16"
         .else
        .byte diskio::PLATFORM, diskio::platform::COMMODORE_64
        .out "  runs on Commodore 64"
        .byte diskio::PLATFORM, diskio::platform::COMMODORE_128
        .out "  runs on Commodore 128"
        .endif
    .else
        .ifdef USER_PLATFORM_FEATS
            .if USER_PLATFORM_FEATS <> 0
                USER_PLATFORM_IMPL
            .else
                DEFAULT_PLATFORM_FEATS
            .endif
        .else
            DEFAULT_PLATFORM_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_SERIAL_BUS_FEATS
    ; one drive on the bus is usually sufficient for applications
    .out "  may be restricted to one drive on serial bus"
.endmacro

.macro SERIAL_BUS_FEATS
    .ifdef DYNLINK_EXPORT
        .if PROTOCOL <> PROTOCOLS::TWO_BITS_ATN
            .byte diskio::SERIAL_BUS, diskio::MANY_DRIVES_ON_SERIAL_BUS
            .out "  many drives on serial bus"
        .else
            .out "  one drive on serial bus"
        .endif
    .else
        .ifdef USER_SERIAL_BUS_FEATS
            .if USER_SERIAL_BUS_FEATS <> 0
                USER_SERIAL_BUS_FEATS_IMPL
            .else
                DEFAULT_SERIAL_BUS_FEATS
            .endif
        .else
            DEFAULT_SERIAL_BUS_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_SUPPORTED_DRIVES_FEATS
    ; generic drive is usually sufficient for applications
    .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_GENERIC
    .out "  compatible with generic drives"
.endmacro

.macro SUPPORTED_DRIVES_FEATS
    .ifdef DYNLINK_EXPORT
        .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_1541
        .out "  compatible with CBM 1541"
        .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_1541_C
        .out "  compatible with CBM 1541-C"
        .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_1541_II
        .out "  compatible with CBM 1541-II"
        .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_1570
        .out "  compatible with CBM 1570"
        .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_1571
        .out "  compatible with CBM 1571"
        .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_1571CR
        .out "  compatible with CBM 1571CR"
        .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_1581
        .out "  compatible with CBM 1581"
        .byte diskio::SUPPORTED_DRIVE, diskio::drivetype::DRIVE_GENERIC
        .out "  compatible with generic drives"
    .else
        .ifdef USER_SUPPORTED_DRIVES_FEATS
            .if USER_SUPPORTED_DRIVES_FEATS <> 0
                USER_SUPPORTED_DRIVES_FEATS_IMPL
            .else
                DEFAULT_SUPPORTED_DRIVES_FEATS
            .endif
        .else
            DEFAULT_SUPPORTED_DRIVES_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_SUPPORTED_DEVICE_NUMBERS_FEATS
    ; device #8 is usually sufficient for applications
    .byte diskio::SUPPORTED_DEVICE_NUMBER, $08
    .out "  must work with device #8"
.endmacro

.macro SUPPORTED_DEVICE_NUMBERS_FEATS
    .ifdef DYNLINK_EXPORT
        .repeat MAX_DEVICE_NO - MIN_DEVICE_NO + 1, I
            .byte diskio::SUPPORTED_DEVICE_NUMBER, MIN_DEVICE_NO + I
        .endrep
    .else
        .ifdef USER_SUPPORTED_DEVICE_NUMBERS_FEATS
            .if USER_SUPPORTED_DEVICE_NUMBERS_FEATS <> 0
                USER_SUPPORTED_DEVICE_NUMBERS_FEATS_IMPL
            .else
                DEFAULT_SUPPORTED_DEVICE_NUMBERS_FEATS
            .endif
        .else
            DEFAULT_SUPPORTED_DEVICE_NUMBERS_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_FORMAT_FEATS
    ; standard format is usually sufficient for applications
    .out "  at least standard CBM format required"
    .byte diskio::FORMAT, diskio::CBM_STANDARD
.endmacro

.macro FORMAT_FEATS
    .ifdef DYNLINK_EXPORT
        .byte diskio::FORMAT, diskio::CBM_STANDARD
        .out "  standard CBM format"
        .byte diskio::FORMAT, diskio::CBM_EXTENDED
        .out "  extended CBM format"
        .byte diskio::FORMAT, diskio::CBM1571_TWOSIDED
        .out "  twosided standard 1571 format"
    .else
        .ifdef USER_FORMAT_FEATS
            .if USER_FORMAT_FEATS <> 0
                USER_FORMAT_FEATS_IMPL
            .else
                DEFAULT_FORMAT_FEATS
            .endif
        .else
            DEFAULT_FORMAT_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_FILE_FEATS
    .byte diskio::FILE_IDENTIFICATION
    .if FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME
        .byte diskio::NAME
        .out "  file identification by name"
        .byte diskio::FILENAME_MAX, FILENAME_MAXLENGTH
        .out "  max. file name length FILENAME_MAXLENGTH chars"
        .byte diskio::MAX_OPEN, 1
        .out "  max. 1 file open at the same time"
        ; applications usually load files with matching filename lengths
        .out "  extra filename chars ignored not checked"
        ; applications usually don't need wildcards
        .out "  no wildcards needed"
        ; the number of first files accessible is usually sufficient for applications
        .out "  first files accessible ignored"
    .elseif FILESYSTEM = FILESYSTEMS::TRACK_SECTOR
        .byte diskio::TRACK_SECTOR
        .out "  file identification by track and sector"
        .byte diskio::MAX_OPEN, 1
        .out "  max. 1 file open at the same time"
    .endif
.endmacro

.macro FILE_FEATS
    .ifdef DYNLINK_EXPORT
        .byte diskio::FILE_IDENTIFICATION
        .if FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME
            .byte diskio::NAME
            .out "  file identification by name"
            .byte diskio::FILENAME_MAX, FILENAME_MAXLENGTH
            .out "  max. file name length FILENAME_MAXLENGTH chars"
            .byte diskio::MAX_OPEN, 1
            .out "  max. 1 file open at the same time"
            .out "  extra filename chars not ignored"
            .byte diskio::FILENAME_WILDCARDS, diskio::WILDCARDS_ALLOWED
            .out "  wildcards in the file name allowed"
            .byte diskio::FIRST_FILES_ACCESSIBLE, diskio::NO_FIRST_FILES_LIMITATION
            .out "  all files in a directory accessible"
        .elseif FILESYSTEM = FILESYSTEMS::TRACK_SECTOR
            .byte diskio::TRACK_SECTOR
            .out "  file identification by track and sector"
            .byte diskio::MAX_OPEN, 1
            .out "  max. 1 file open at the same time"
        .endif
    .else
        .ifdef USER_FILE_FEATS
            .if USER_FILE_FEATS <> 0
                USER_FILE_FEATS_IMPL
            .else
                DEFAULT_FILE_FEATS
            .endif
        .else
            DEFAULT_FILE_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_TIMER_FEATS
    ; timers are usually not used by applications
    .if PLATFORM = diskio::platform::COMMODORE_16
    .out "  may use TED T1 IRQ"
    .out "  may use TED T2 IRQ"
    .out "  may use TED T3 IRQ"
    .else
    .out "  may use CIA 1 TA IRQ"
    .out "  may use CIA 1 TB IRQ"
    .out "  may use CIA 2 TA NMI"
    .out "  may use CIA 2 TB NMI"
    .endif
.endmacro

.macro TIMER_FEATS
    .ifdef DYNLINK_EXPORT
        .if PLATFORM = diskio::platform::COMMODORE_16
        .byte diskio::DOES_NOT_USE_TED_T1_IRQ, diskio::TED_T1_IRQ_NOT_USED
        .out "  does not use TED T1 IRQ"
        .byte diskio::DOES_NOT_USE_TED_T2_IRQ, diskio::TED_T2_IRQ_NOT_USED
        .out "  does not use TED T2 IRQ"
        .byte diskio::DOES_NOT_USE_TED_T3_IRQ, diskio::TED_T3_IRQ_NOT_USED
        .out "  does not use TED T3 IRQ"
        .else
        .byte diskio::DOES_NOT_USE_CIA1_TA_IRQ, diskio::CIA1_TA_IRQ_NOT_USED
        .out "  does not use CIA 1 TA IRQ"
        .byte diskio::DOES_NOT_USE_CIA1_TB_IRQ, diskio::CIA1_TB_IRQ_NOT_USED
        .out "  does not use CIA 1 TB IRQ"
        .byte diskio::DOES_NOT_USE_CIA2_TA_NMI, diskio::CIA2_TA_NMI_NOT_USED
        .out "  does not use CIA 2 TA NMI"
        .byte diskio::DOES_NOT_USE_CIA2_TB_NMI, diskio::CIA2_TB_NMI_NOT_USED
        .out "  does not use CIA 2 TB NMI"
        .endif
    .else
        .ifdef USER_TIMER_FEATS
            .if USER_TIMER_FEATS <> 0
                USER_TIMER_FEATS_IMPL
            .else
                DEFAULT_TIMER_FEATS
            .endif
        .else
            DEFAULT_TIMER_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_PALNTSCACCEL_FEATS
    ; PAL compatibility is usually enough for applications
    .byte diskio::PAL, diskio::PAL_COMPATIBLE
    .out "  must be PAL compatible"
    .out "  may be NTSC compatible"
    .out "  may be acceleration compatible"
.endmacro

.macro PALNTSCACCEL_FEATS
    .ifdef DYNLINK_EXPORT
        .byte diskio::PAL, diskio::PAL_COMPATIBLE
        .out "  PAL compatible"
        .byte diskio::NTSC, diskio::NTSC_COMPATIBLE
        .out "  NTSC compatible"
        .out "  not acceleration compatible"
    .else
        .ifdef USER_PALNTSCACCEL_FEATS
            .if USER_PALNTSCACCEL_FEATS <> 0
                USER_PALNTSCACCEL_FEATS_IMPL
            .else
                DEFAULT_PALNTSCACCEL_FEATS
            .endif
        .else
            DEFAULT_PALNTSCACCEL_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.if NONBLOCKING_API
    .macro DEFAULT_TIMER_FEATS_NB
        ; timers are usually not used by applications
        .if PLATFORM = diskio::platform::COMMODORE_16
        .out "  may use TED T1 IRQ"
        .out "  may use TED T2 IRQ"
        .out "  may use TED T3 IRQ"
        .else
        .out "  may use CIA 1 TA IRQ"
        .out "  may use CIA 1 TB IRQ"
        .out "  may use CIA 2 TA NMI"
        .out "  may use CIA 2 TB NMI"
        .endif
    .endmacro

    .macro TIMER_FEATS_NB
        .ifdef DYNLINK_EXPORT
            .if PLATFORM = diskio::platform::COMMODORE_16
                .out "  uses TED T1 IRQ"
                .byte diskio::DOES_NOT_USE_TED_T2_IRQ, diskio::TED_T2_IRQ_NOT_USED
                .out "  does not use TED T2 IRQ"
                .byte diskio::DOES_NOT_USE_TED_T3_IRQ, diskio::TED_T3_IRQ_NOT_USED
                .out "  does not use TED T3 IRQ"
            .else
                .byte diskio::DOES_NOT_USE_CIA1_TA_IRQ, diskio::CIA1_TA_IRQ_NOT_USED
                .out "  does not use CIA 1 TA IRQ"
                .byte diskio::DOES_NOT_USE_CIA1_TB_IRQ, diskio::CIA1_TB_IRQ_NOT_USED
                .out "  does not use CIA 1 TB IRQ"
                .out "  uses CIA 2 TA NMI"
                .out "  uses CIA 2 TB NMI"
            .endif
        .else
            .ifdef USER_TIMER_FEATS_NB
                .if USER_TIMER_FEATS_NB <> 0
                    USER_TIMER_FEATS_NB_IMPL
                .else
                    DEFAULT_TIMER_FEATS_NB
                .endif
            .else
                DEFAULT_TIMER_FEATS_NB
            .endif
        .endif
    .endmacro
.endif

;------------------------------------------------------------------------------

.if NONBLOCKING_API
    .macro DEFAULT_PALNTSCACCEL_FEATS_NB
        ; PAL compatibility is usually enough for applications
        .byte diskio::PAL, diskio::PAL_COMPATIBLE
        .out "  must be PAL compatible"
        .out "  may be NTSC compatible"
        .out "  may be acceleration compatible"
    .endmacro
    
    .macro PALNTSCACCEL_FEATS_NB
        .ifdef DYNLINK_EXPORT; PAL compatibility is usually sufficient for applications
            .byte diskio::PAL, diskio::PAL_COMPATIBLE
            .out "  PAL compatible"
            .out "  not NTSC compatible"
            .out "  not acceleration compatible"
        .else
            .ifdef USER_PALNTSCACCEL_FEATS_NB
                .if USER_PALNTSCACCEL_FEATS_NB <> 0
                    USER_PALNTSCACCEL_FEATS_IMPL_NB
                .else
                    DEFAULT_PALNTSCACCEL_FEATS_NB
                .endif
            .else
                DEFAULT_PALNTSCACCEL_FEATS_NB
            .endif
        .endif
    .endmacro
.endif

;------------------------------------------------------------------------------

.macro DEFAULT_LOAD_TO_FEATS
    .if LOAD_TO_API
        .byte diskio::DESTINATION_OVERRIDE, diskio::DEST_OVERRIDE
        .out "  with destination override"
    .else
        .out "  no destination override"
    .endif
.endmacro

.macro LOAD_TO_FEATS
    .ifdef DYNLINK_EXPORT
        .if LOAD_TO_API
            .byte diskio::DESTINATION_OVERRIDE, diskio::DEST_OVERRIDE
            .out "  with destination override"
        .endif
    .else
        .ifdef USER_LOAD_TO_FEATS
            .if USER_LOAD_TO_FEATS <> 0
                USER_LOAD_TO_FEATS_IMPL
            .else
                DEFAULT_LOAD_TO_FEATS
            .endif
        .else
            DEFAULT_LOAD_TO_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.macro DEFAULT_REENTRANCE_FEATS
    .out "  not re-entrant, no guards"
.endmacro

.macro REENTRANCE_FEATS
    .ifdef DYNLINK_EXPORT
        .out "  not re-entrant, no guards"
    .else
        .ifdef USER_REENTRANCE_FEATS
            .if USER_REENTRANCE_FEATS <> 0
                USER_REENTRANCE_FEATS_IMPL
            .else
                DEFAULT_REENTRANCE_FEATS
            .endif
        .else
            DEFAULT_REENTRANCE_FEATS
        .endif
    .endif
.endmacro

;------------------------------------------------------------------------------

.endif; !_COMMON_FEATURES_INC_
