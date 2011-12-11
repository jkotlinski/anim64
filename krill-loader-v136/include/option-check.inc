
.ifndef _OPTION_CHECK_INC_
_OPTION_CHECK_INC_ = 1

.if (DECOMPRESSOR <> DECOMPRESSORS::NONE)
    HAS_DECOMPRESSOR = 1
.else
    HAS_DECOMPRESSOR = 0
.endif

.if GETC_API | GETCHUNK_API | LOAD_COMPD_API
    BYTESTREAM = 1
.else
    BYTESTREAM = 0
.endif

.if LOAD_COMPD_API | GETCHUNK_API
    EXCEPTIONS = 1
.else
    EXCEPTIONS = 0
.endif

.if END_ADDRESS_API | GETC_API | ((GETCHUNK_API | LOAD_COMPD_API) & CHAINED_COMPD_FILES)
    END_ADDRESS = 1
.else
    END_ADDRESS = 0
.endif

.if LOAD_PROGRESS_API | (LOAD_VIA_KERNAL_FALLBACK & GETC_API)
    MAINTAIN_BYTES_LOADED = 1
.else
    MAINTAIN_BYTES_LOADED = 0
.endif

.if LOAD_VIA_KERNAL_FALLBACK & (DIRTRACK <> 18)
    .error "***** Option LOAD_VIA_KERNAL_FALLBACK requires DIRTRACK to be 18 *****"
.endif

.if LOAD_VIA_KERNAL_FALLBACK & (DIRSECTOR <> 0)
    .error "***** Option LOAD_VIA_KERNAL_FALLBACK requires DIRSECTOR to be 0 *****"
.endif

.if LOAD_VIA_KERNAL_FALLBACK & (FILESYSTEM <> FILESYSTEMS::DIRECTORY_NAME)
    .error "***** Option LOAD_VIA_KERNAL_FALLBACK requires FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME *****"
.endif

.if LOAD_ONCE & (FILESYSTEM <> FILESYSTEMS::DIRECTORY_NAME)
    .error "***** Option LOAD_ONCE requires FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME *****"
.endif


.if LOAD_COMPD_API & (!HAS_DECOMPRESSOR)
    .error "***** Option LOAD_COMPD_API requires DECOMPRESSOR != DECOMPRESSORS::NONE *****"
.endif

.if MEM_DECOMP_API & (!HAS_DECOMPRESSOR)
    .error "***** Option MEM_DECOMP_API requires DECOMPRESSOR != DECOMPRESSORS::NONE *****"
.endif

.if MEM_DECOMP_TO_API & (!MEM_DECOMP_API)
    .error "***** Option MEM_DECOMP_TO_API requires MEM_DECOMP_API *****"
.endif

.if HAS_DECOMPRESSOR & (!LOAD_COMPD_API) & (!MEM_DECOMP_API) & (!GETCHUNK_API)
    .error "***** Decompressor included but not used, as neither LOAD_COMPD_API nor MEM_DECOMP_API nor GETCHUNK_API are enabled *****"
.endif


.if GETC_API & (!OPEN_FILE_POLL_BLOCK_API)
    .error "***** The GETC_API requires the OPEN_FILE_POLL_BLOCK_API, please enable the OPEN_FILE_POLL_BLOCK_API *****"
.endif

.if GETCHUNK_API & (!OPEN_FILE_POLL_BLOCK_API)
    .error "***** The GETCHUNK_API requires the OPEN_FILE_POLL_BLOCK_API, please enable the OPEN_FILE_POLL_BLOCK_API *****"
.endif

.if GETCHUNK_API & (!HAS_DECOMPRESSOR)
    .error "***** The GETCHUNK_API currently only works with compressed files, please set DECOMPRESSOR to something else than DECOMPRESSORS::NONE *****"
.endif


.if (!LOAD_RAW_API) & (!LOAD_COMPD_API) & (!OPEN_FILE_POLL_BLOCK_API) & (!NONBLOCKING_API)
    .error "***** No actual loading calls enabled, please select LOAD_RAW_API, LOAD_COMPD_API, OPEN_FILE_POLL_BLOCK_API, and/or NONBLOCKING_API *****"
.endif

.if (LOAD_ONCE & INSTALL_FROM_DISK)
    .error "***** both LOAD_ONCE and INSTALL_FROM_DISK selected, please select only one of these options *****"
.endif

.if UNINSTALL_RUNS_DINSTALL & (!UNINSTALL_API)
    .error "***** UNINSTALL_RUNS_DINSTALL requires UNINSTALL_API *****"
.endif

.endif; _OPTION_CHECK_INC_
