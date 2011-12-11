
; the following zeropage variables can be
; overwritten when the loader is idle

__NOIMPORTVARS = 1
.include "loader.inc"

.segment "EXTZP"; not used otherwise, segment is not optional
                ; in the o65 built-in ld65 config

.macro alloc_zpvar symbol
symbol:       .res 1
    .exportzp symbol
.endmacro

.macro alloc_zpvar_2 symbol
symbol:       .res 2
    .exportzp symbol
.endmacro

.macro alloc_decompvars

    .if GETCHUNK_API
GETCHUNK_VARS:
    .exportzp GETCHUNK_VARS
              .res 9
    .endif

DECOMPVARS:
    .exportzp DECOMPVARS

    .if DECOMPRESSOR = DECOMPRESSORS::PUCRUNCH
              .res 3
    .elseif DECOMPRESSOR = DECOMPRESSORS::BYTEBOOZER
              .res 6
    .elseif DECOMPRESSOR = DECOMPRESSORS::LEVELCRUSH
              .res 2
    .elseif DECOMPRESSOR = DECOMPRESSORS::EXOMIZER
              .res 5
    .endif
.endmacro

.ifdef DYNLINK_EXPORT
    ; dynamic linking
    ; allocate zeropage variables
    ; caring for the
    ; fixed dynlink variable offsets
    ; while minimizing memory overhead
    ; by filling the gaps as good as possible

    .segment "ZEROPAGE" : zeropage

    .macro alloc_next_zpvar
        .if BYTESTREAM
            .ifndef LOADYBUF
                alloc_zpvar LOADYBUF
                .exitmacro
            .endif
            .ifndef YPNTRBUF
                alloc_zpvar YPNTRBUF
                .exitmacro
            .endif
            .ifndef LASTBLKIDX
                alloc_zpvar LASTBLKIDX
                .exitmacro
            .endif
            .ifndef LASTBLKSIZ
                alloc_zpvar LASTBLKSIZ
                .exitmacro
            .endif
        .endif

        .if NONBLOCKING_API
            .ifndef PACCUBUF
                alloc_zpvar PACCUBUF
                .exitmacro
            .endif
        .endif

        .ifndef fixed_zpvars_end
            .res 1; dummy variable to keep offsets
            .exitmacro
        .endif

        ; the following definitions occur in alloc_next_zpvars_2
        ; but are included here, too, because they might not be
        ; included yet using alloc_next_zpvars_2
        .ifndef BLOCKDESTLO
            alloc_zpvar BLOCKDESTLO
            .exitmacro
        .endif
        .ifndef BLOCKINDEX
            alloc_zpvar BLOCKINDEX; this one must be there after BLOCKDESTLO
            .exitmacro
        .endif

        .if (!HAS_DECOMPRESSOR) & LOAD_VIA_KERNAL_FALLBACK
            .ifndef LOADDESTPTR
                alloc_zpvar_2 LOADDESTPTR
                .exitmacro
            .endif
        .endif
    .endmacro

    .macro alloc_next_zpvars_2
        .ifndef BLOCKDESTLO
            alloc_zpvar BLOCKDESTLO
            alloc_zpvar BLOCKINDEX; this one must be there after BLOCKDESTLO
            .exitmacro
        .endif

        .if (!HAS_DECOMPRESSOR) & LOAD_VIA_KERNAL_FALLBACK
            .ifndef LOADDESTPTR
                alloc_zpvar_2 LOADDESTPTR
                .exitmacro
            .endif
        .endif

        alloc_next_zpvar
        alloc_next_zpvar
    .endmacro

    alloc_zpvar loadstatus  ; LOADSTATUSOFFS   = 0

    alloc_zpvar param4      ; PARAM4OFFS       = 1
    alloc_zpvar param5      ; PARAM5OFFS       = 2

    alloc_zpvar loadaddrlo  ; LOADADDRLOOFFS   = 3
    alloc_zpvar loadaddrhi  ; LOADADDRHIOFFS   = 4

    .if HAS_DECOMPRESSOR
    alloc_zpvar decdestlo   ; DECDESTLOOFFS    = 5
    alloc_zpvar decdesthi   ; DECDESTHIOFFS    = 6
    .else
    alloc_next_zpvars_2
    .endif

    .if END_ADDRESS_API || GETC_API || ((GETCHUNK_API || LOAD_COMPD_API) && CHAINED_COMPD_FILES)
    alloc_zpvar endaddrlo   ; ENDADDRLOOFFS    = 7
    alloc_zpvar endaddrhi   ; ENDADDRHIOFFS    = 8
    .else
        .if LOAD_PROGRESS_API
            .if DECOMPRESSOR = DECOMPRESSORS::LEVELCRUSH && (!GETCHUNK_API)
            alloc_decompvars
            .else
            alloc_next_zpvars_2
            .endif
        .endif
    .endif

    ; bytesloadedlo/hi is not endaddr - loadaddr while loading
    ; because blocks may be fetched out of order
    .if MAINTAIN_BYTES_LOADED
    alloc_zpvar bytesloadedlo; BYTESLOADEDLOOFFS = 9
    alloc_zpvar bytesloadedhi; BYTESLOADEDHIOFFS = 10
    .endif

    .if NONBLOCKING_API
    alloc_zpvar pollyoffset; POLLYOFFSETOFFS = 11
    .endif

    fixed_zpvars_end:

    ; decompressor
    .ifndef DECOMPVARS
    alloc_decompvars
    .endif

    .if HAS_DECOMPRESSOR & LOAD_VIA_KERNAL_FALLBACK
    LOADDESTPTR = DECOMPVARS
    .exportzp LOADDESTPTR
    .endif

    .repeat 32, I
        alloc_next_zpvar
    .endrep

.else

    ; no dynamic linking
    ; allocate zeropage variables
    ; without caring for the
    ; fixed dynlink variable offsets

    .segment "DISKIO_ZP" : zeropage

    loader_zp_first = *
    .export loader_zp_first

    .if NONBLOCKING_API
    alloc_zpvar pollyoffset
    alloc_zpvar loadstatus
    .endif

    alloc_zpvar loadaddrlo
    alloc_zpvar loadaddrhi

    .if HAS_DECOMPRESSOR
    alloc_zpvar decdestlo
    alloc_zpvar decdesthi
    .endif

    .if END_ADDRESS_API || GETC_API || ((GETCHUNK_API || LOAD_COMPD_API) && CHAINED_COMPD_FILES)
    alloc_zpvar endaddrlo
    alloc_zpvar endaddrhi
    .endif

    ; bytesloadedlo/hi is not endaddr - loadaddr while loading
    ; because blocks are be fetched out of order
    .if MAINTAIN_BYTES_LOADED
    alloc_zpvar bytesloadedlo
    alloc_zpvar bytesloadedhi
    .endif

    .if BYTESTREAM
    alloc_zpvar LOADYBUF
    alloc_zpvar YPNTRBUF
    alloc_zpvar LASTBLKIDX
    alloc_zpvar LASTBLKSIZ
    .endif

    .if GETCHUNK_API
    alloc_zpvar param4
    alloc_zpvar param5
    .endif

    .if NONBLOCKING_API
    alloc_zpvar PACCUBUF
    .endif

    alloc_zpvar BLOCKDESTLO
    alloc_zpvar BLOCKINDEX; this one must be there after BLOCKDESTLO

    .if (!HAS_DECOMPRESSOR) & LOAD_VIA_KERNAL_FALLBACK
    alloc_zpvar_2 LOADDESTPTR
    .endif

    ; decompressor
    alloc_decompvars

    .if HAS_DECOMPRESSOR & LOAD_VIA_KERNAL_FALLBACK
    LOADDESTPTR = DECOMPVARS
    .exportzp LOADDESTPTR
    .endif

    loader_zp_last = * - 1
    .export loader_zp_last

    .assert BLOCKINDEX = BLOCKDESTLO + 1, error, "BLOCKINDEX != BLOCKDESTLO + 1"

.endif
