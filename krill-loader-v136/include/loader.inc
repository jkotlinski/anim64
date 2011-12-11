
.ifndef _LOADER_INC_
_LOADER_INC_ = 1

.pushseg

.segment "CODE"; import symbols as absolute by default

.include "diskio.inc"


; available protocols, file systems, and decompressors

.enum PROTOCOLS     ; the available transfer protocols - in this version, only TWO_BITS_ATN is implemented

   ;ONE_BIT         ; clocking every bit. allows for arbitrary interrupts, yet it's slower than TWO_BITS_ATN
                    ; and TWO_BITS but has no iec bus restrictions - not implemented yet
   ;TWO_BITS        ; clocking every byte, the 8 bits within a byte are transferred strictly synchronously
                    ; and in groups of 2, no iec bus restrictions
                    ; allows for badlines, but does not allow for arbitrary
                    ; interrupts, so sprites are forbidden in the main loop, also sei and cli are used,
                    ; so interrupts are delayed for maximum 2 rasterlines - not implemented yet
   ;TWO_BITS_NOBL   ; same as TWO_BITS but without bad line checks, so bad lines are not allowed outside irq handlers,
                    ; has the same drive-side routine as TWO_BITS - not implemented yet
   ;TWO_BITS_BURST  ; same as TWO_BITS but with bigger irq delay,
                    ; the fastest available protocol - not implemented yet
    TWO_BITS_ATN    ; the fastest protocol that allows for arbitrary interrupts (sprites, bad lines, etc.)
                    ; but it works with only 1 iec device on the bus.
.endenum

.enum FILESYSTEMS   ; the available file systems - in this version, only DIRECTORY_NAME, and TRACK_SECTOR is implemented

    DIRECTORY_NAME  ; operation on single files that are referenced using the directory, where the filenames
                    ; can be 1 to 16 chars long - no wildcard matching, so * and ? are treated as normal characters;
                    ; each file of the maximum 144 per 1541 disk side is accessible
   ;DIRECTORY_INDEX ; operation on single files that are identified by their position in the directory,
                    ; non-files (track 0) are counted
    TRACK_SECTOR    ; operation on single files that are referenced using track and sector position,
                    ; the filename does not have to appear in the directory
   ;DIRECTORY_HASH  ; operation on single files that are referenced using the directory, where the "filenames"
                    ; are a 16 bit hash number on the actual file names
   ;IFFL            ; operation on large iffl files (file databases in single files, one for each disk side)
                    ; where the files are referenced by indices into the iffl files. one big file looks cooler
                    ; than lots of small single files and also removes sector allocation overhead - not implemented yet
.endenum

.enum DECOMPRESSORS ; the available decompression routines - in this version, EXOSTREAM and GPACKER are not implemented yet

    EXOMIZER        ; Magnus Lind's Exomizer - no fast decompression, but nearly optimal compression ratio (rle+lz+huffman)
    PUCRUNCH        ; Pasi Ojala's Pucrunch - not so fast decompression but very good compression ratio (rle+lz+huffman)
    BYTEBOOZER      ; HCL/Booze Design's ByteBoozer - pretty fast decompression and good compression ratio (lz, a little better than Levelcrush)
    LEVELCRUSH      ; Taboo Levelcrush - pretty fast decompression and good compression ratio (lz)
   ;GPACKER         ; Graham/Oxyron's G-Packer - very fast decompression and okay compression ratio (rle) - not implemented yet

    NONE            ; no decompressor used

.endenum

.enum MODULES
    LOADFILE
.endenum


.ifdef EXTCONFIGPATH
    .include "loaderconfig.inc"
.else
    .include "config.inc"
.endif; !EXTCONFIGPATH

.include "option-check.inc"


.if FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME
    FILE_NOT_FOUND = diskio::status::INVALID_PARAMETERS
.elseif FILESYSTEM = FILESYSTEMS::TRACK_SECTOR
    ILLEGAL_TRACK_OR_SECTOR = diskio::status::INVALID_PARAMETERS
.endif


.ifndef __NO_LOADER_SYMBOLS_IMPORT

.macro IMPORT symbol
    .if (!(.defined(DYNLINK_IMPORT) && DYNLINK_IMPORT))
        .import symbol
    .endif
.endmacro

.macro GENERATE_IMPORT_REQUEST diskioreq
diskioreq:
    .tag diskio::IMPORT

    ; for unknown reasons, these include files will be included
    ; in reverse order by ca65, hence their inclusion is inverted here
    .include "resident-jumptable.inc"
    .include "install-jumptable.inc"
.endmacro


.if PLATFORM <> diskio::platform::COMMODORE_16
    ; Set the VIC bank
    ; in:  a - VIC bank (0..3)
    ; out: undefined
    .macro SET_VIC_BANK bank
            lda #bank & 3
            sta CIA2_PRA
    .endmacro
.else
    ; To be run at the start of every IRQ handler
    IMPORT pollhandlr
    .macro LOADER_IRQ_HANDLER_PROLOGUE irqend
            ; buffer force single clock flag
            ; and allow double clock
            lda TED_CHARGEN_ADDR
            pha
            and #~FORCE_SINGLE_CLOCK
            sta TED_CHARGEN_ADDR

        .if NONBLOCKING_API
            .local poll
            .local nopoll

            lda #RASTER_IRQ | COUNTER_2_IRQ | COUNTER_3_IRQ
            and TED_IRR
            and TED_IMR
            bne nopoll
poll:       jsr pollhandlr
            jmp irqend
nopoll:
        .endif; NONBLOCKING_API
    .endmacro

    ; To be run at the end of every IRQ handler
    .macro LOADER_IRQ_HANDLER_EPILOGUE
            ; restore force single clock flag
            .local fast

            pla
            lsr
            lsr
            lda TED_CHARGEN_ADDR
            and #~FORCE_SINGLE_CLOCK
            bcc fast
            ora #FORCE_SINGLE_CLOCK
fast:       sta TED_CHARGEN_ADDR
    .endmacro
.endif

; Import the loader code from a dynamically linked library file
; in:  a/x - lo/hi to import request structure
; out: c   - set on error
;      a   - diskio::status, on diskio::status::GENERIC_KERNAL_ERROR, the KERNAL error code is returned in x
;      x   - if status is diskio::status::OK or diskio::status::DYNLINK_FALLBACK_USED, lo-byte of version string address,
;            if status is diskio::status::WRONG_VERSION, required major version number
;      y   - if status is diskio::status::OK or diskio::status::DYNLINK_FALLBACK_USED, hi-byte of version string address,
;            if status is diskio::status::WRONG_VERSION, found (but bad) major version number
.import importldr; does not use the IMPORT macro: needs to be imported always
.macro IMPORT_LOADER request
            lda #.lobyte(request)
            ldx #.hibyte(request)
            jsr importldr
.endmacro

; Install the loader
; in:  a   - if NONBLOCKING_API is non-0, nmi line mod 8
;      x/y - if LOAD_ONCE, lo/hi to 0-terminated filename string
; out: c   - set on error
;      a   - status
;      x   - drive type (one of diskio::drivetypes)
;      y   - if status is diskio::status::OK, zeropage address of version string address

IMPORT install
.if !NONBLOCKING_API
    .if LOAD_ONCE | INSTALL_FROM_DISK
        .macro LOADER_INSTALL filename_lo, filename_hi
            ldx filename_lo
            ldy filename_hi
            jsr install
        .endmacro
    .else
        .macro LOADER_INSTALL
            jsr install
        .endmacro
    .endif
.else; NONBLOCKING_API
    .if LOAD_ONCE | INSTALL_FROM_DISK
        .macro LOADER_INSTALL nmi_line, filename_lo, filename_hi
            lda nmi_line
            ldx filename_lo
            ldy filename_hi
            jsr install
        .endmacro
    .else
        .macro LOADER_INSTALL nmi_line
            lda nmi_line
            jsr install
        .endmacro
    .endif
.endif; NONBLOCKING_API

.if LOAD_RAW_API
; Load a file without decompression
; in:  x/y - if FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME, lo/hi to 0-terminated filename string
;            if FILESYSTEM = FILESYSTEMS::TRACK_SECTOR, a: track, x: sector
;      c   - if LOAD_TO_API != 0, c = 0: load to address as stored in the file
;                                 c = 1: load to caller-specified address (loadaddrlo/hi)
; out: c   - set on error
;      a   - status
IMPORT loadraw
.if !LOAD_ONCE
    .macro LOADRAW namelo_or_track, namehi_or_sector
        .if LOAD_TO_API
            clc
        .endif
            ldx namelo_or_track
            ldy namehi_or_sector
            jsr loadraw
    .endmacro

    .if LOAD_TO_API
        .macro LOADRAW_LOADTO namelo_or_track, namehi_or_sector, dest_lo, dest_hi
            sec
            lda dest_lo
            sta loadaddrlo
            lda dest_hi
            sta loadaddrhi
            ldx namelo_or_track
            ldy namehi_or_sector
            jsr loadraw
        .endmacro
    .endif;  LOAD_TO_API
.else; LOAD_ONCE
    .macro LOADRAW
        .if LOAD_TO_API
            clc
        .endif
            jsr loadraw
    .endmacro

    .if LOAD_TO_API
        .macro LOADRAW_LOADTO dest_lo, dest_hi
            sec
            lda dest_lo
            sta loadaddrlo
            lda dest_hi
            sta loadaddrhi
            jsr loadraw
        .endmacro
    .endif; LOAD_TO_API
.endif; LOAD_ONCE
.endif; LOAD_RAW_API

.if LOAD_COMPD_API
; Load a compressed file
; in:  x/y - if FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME, lo/hi to 0-terminated filename string
;            if FILESYSTEM = FILESYSTEMS::TRACK_SECTOR, a: track, x: sector
;      c   - if DECOMPLOAD_TO_API != 0, c = 0: load to address as stored in the file
;                                       c = 1: load to caller-specified address (loadaddrlo/hi)
; out: c   - set on error
;      a   - status
;      x/y - if DECOMPRESSOR = DECOMPRESSORS::PUCRUNCH, lo/hi of the loaded file's execution address
IMPORT loadcompd
.if !LOAD_ONCE
    .macro LOADCOMPD namelo_or_track, namehi_or_sector
            ldx namelo_or_track
            ldy namehi_or_sector
            jsr loadcompd
    .endmacro
.else; LOAD_ONCE
    .macro LOADCOMPD
            jsr loadcompd
    .endmacro
.endif
.endif; LOAD_COMPD_API

.if NONBLOCKING_API
; non-blockingly Load a file without decompression
; in:  x/y - if FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME, lo/hi to 0-terminated filename string
;            if FILESYSTEM = FILESYSTEMS::TRACK_SECTOR, a: track, x: sector
;      c   - if LOAD_TO_API != 0, c = 0: load to address as stored in the file
;                                 c = 1: load to caller-specified address (loadaddrlo/hi)
; out: c   - set on error
;      a   - status
IMPORT loadrawnb
.if !LOAD_ONCE
    .macro LOADRAW_NB namelo_or_track, namehi_or_sector
        .if LOAD_TO_API
            clc
        .endif
            ldx namelo_or_track
            ldy namehi_or_sector
            jsr loadrawnb
    .endmacro

    .if LOAD_TO_API
        .macro LOADRAW_LOADTO_NB namelo_or_track, namehi_or_sector, dest_lo, dest_hi
            sec
            lda dest_lo
            sta loadaddrlo
            lda dest_hi
            sta loadaddrhi
            ldx namelo_or_track
            ldy namehi_or_sector
            jsr loadrawnb
        .endmacro
    .endif;  LOAD_TO_API
.else; LOAD_ONCE
    .macro LOADRAW_NB
        .if LOAD_TO_API
            clc
        .endif
            jsr loadrawnb
    .endmacro

    .if LOAD_TO_API
        .macro LOADRAW_LOADTO_NB dest_lo, dest_hi
            sec
            lda dest_lo
            sta loadaddrlo
            lda dest_hi
            sta loadaddrhi
            jsr loadrawnb
        .endmacro
    .endif; LOAD_TO_API
.endif; LOAD_ONCE

; Loop until a non-blocking operation is finished
; in:  nothing
; out: c - set on error
;      a - status
.macro LOOP_BUSY loop
            .local error

            lda loadstatus
            cmp #diskio::status::BUSY
            beq loop
            cmp #diskio::status::OK
            bne error
            clc
error:
.endmacro
.endif; NONBLOCKING_API

.if OPEN_FILE_POLL_BLOCK_API
; Open a file
; in:  x/y - if FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME, lo/hi to 0-terminated filename string
;            if FILESYSTEM = FILESYSTEMS::TRACK_SECTOR, a: track, x: sector
;      c   - if LOAD_TO_API != 0, c = 0: load to address as stored in the file
;                                 c = 1: load to caller-specified address (loadaddrlo/hi)
; out: c - set on error
;      a - status
IMPORT openfile
.if !LOAD_ONCE
    .macro OPENFILE namelo_or_track, namehi_or_sector
        .if LOAD_TO_API
            clc
        .endif
            ldx namelo_or_track
            ldy namehi_or_sector
            jsr openfile
    .endmacro

    .if LOAD_TO_API
        .macro OPENFILE_LOADTO namelo_or_track, namehi_or_sector, dest_lo, dest_hi
            sec
            lda dest_lo
            sta loadaddrlo
            lda dest_hi
            sta loadaddrhi
            ldx namelo_or_track
            ldy namehi_or_sector
            jsr openfile
        .endmacro
    .endif; LOAD_TO_API
.else; LOAD_ONCE
    .macro OPENFILE
        .if LOAD_TO_API
            clc
        .endif
            jsr openfile
    .endmacro

    .if LOAD_TO_API
        .macro OPENFILE_LOADTO dest_lo, dest_hi
            sec
            lda dest_lo
            sta loadaddrlo
            lda dest_hi
            sta loadaddrhi
            jsr openfile
        .endmacro
    .endif; LOAD_TO_API
.endif;  LOAD_ONCE

; Check if a block is ready to download
; in:  nothing
; out: v - set if a block is ready to download, cleared otherwise
.if PLATFORM = diskio::platform::COMMODORE_16
.macro BLOCKREADY
            bit $01; IO_PORT
.endmacro
.else
.macro BLOCKREADY
            bit $dd00; CIA2_PRA
.endmacro
.endif

; Poll for a block after successfully opening a file
; in:  nothing
; out: c - set on error or eof, cleared otherwise
;      a - status
IMPORT pollblock
.macro POLLBLOCK
            jsr pollblock
.endmacro
.endif; OPEN_FILE_POLL_BLOCK_API

.if GETC_API
; Get a byte from the raw file stream
; in:  nothing
; out: a - value if c = 0, status otherwise
;      c - error or EOF
IMPORT getc
.macro GETC
            jsr getc
.endmacro
.endif; GETC_API

.if GETCHUNK_API
; Get an uncompressed chunk from the compressed file stream
; in:  x/y - lo/hi of chunk size
; out: a     - status
;      x/y   - lo/hi of chunk address
;      p4/p5 - lo/hi of retrieved chunk size
;      c     - set on error
IMPORT getchunk
.macro GETCHUNK sizelo, sizehi
            ldx sizelo
            ldy sizehi
            jsr getchunk
.endmacro
.endif; GETCHUNK_API

.if MEM_DECOMP_API
; Decompress a compressed file from memory
; in:  x/y - lo/hi of compressed file in memory
;      c   - if MEMDECOMP_TO_API != 0, c = 0: decompress to address as stored in the file
;                                      c = 1: decompress to caller-specified address (loadaddrlo/hi)
; out: x/y - if DECOMPRESSOR = DECOMPRESSORS::PUCRUNCH, lo/hi of the file's execution address
IMPORT memdecomp
.macro MEMDECOMP source_lo, source_hi
    .if MEM_DECOMP_TO_API
            clc
    .endif
            ldx source_lo
            ldy source_hi
            jsr memdecomp
.endmacro

.if MEM_DECOMP_TO_API
    .macro MEMDECOMP_TO source_lo, source_hi, dest_lo, dest_hi
            sec
            lda dest_lo
            sta decdestlo
            lda dest_hi
            sta decdesthi
            ldx source_lo
            ldy source_hi
            jsr memdecomp
    .endmacro
.endif

.if CHAINED_COMPD_FILES
; Decompress a subsequent compressed file from memory
; in:  c - if MEMDECOMP_TO_API != 0, c = 0: decompress to address as stored in the file
;                                    c = 1: decompress to caller-specified address (loadaddrlo/hi)
; out: x/y - if DECOMPRESSOR = DECOMPRESSORS::PUCRUNCH, lo/hi of the file's execution address
IMPORT cmemdecomp
.macro CMEMDECOMP
    .if MEM_DECOMP_TO_API
            clc
    .endif
            jsr cmemdecomp
.endmacro

.if MEM_DECOMP_TO_API
    .macro CMEMDECOMP_TO dest_lo, dest_hi
            sec
            lda dest_lo
            sta decdestlo
            lda dest_hi
            sta decdesthi
            jsr cmemdecomp
    .endmacro
.endif; MEM_DECOMP_TO_API
.endif; CHAINED_COMPD_FILES
.endif; MEM_DECOMP_API

.if UNINSTALL_API
; Uninstall the loader
; in:  nothing
; out: undefined
IMPORT uninstall
.macro LOADER_UNINSTALL
            jsr uninstall
.endmacro
.endif; UNINSTALL_API


; linker-generated loader segments symbols

.import __DISKIO_ZP_START__
.import __DISKIO_ZP_END__
.import __DISKIO_ZP_LOAD__
.import __DISKIO_ZP_RUN__
.import __DISKIO_ZP_SIZE__

.import __DISKIO_INSTALL_START__
.import __DISKIO_INSTALL_END__
.import __DISKIO_INSTALL_LOAD__
.import __DISKIO_INSTALL_RUN__
.import __DISKIO_INSTALL_SIZE__

.import __DISKIO_START__
.import __DISKIO_END__
.import __DISKIO_LOAD__
.import __DISKIO_RUN__
.import __DISKIO_SIZE__

.endif; !__NO_LOADER_SYMBOLS_IMPORT


.ifndef DYNLINK_IMPORT
            DYNLINK_IMPORT = 0
.endif

.if DYNLINK_IMPORT
    .macro PREPARE_IMPORT request
            lda #.lobyte(__DISKIO_INSTALL_RUN__)
            sta request + diskio::IMPORT::INSTALL_BASE + 0
            lda #.hibyte(__DISKIO_INSTALL_RUN__)
            sta request + diskio::IMPORT::INSTALL_BASE + 1
            lda #.lobyte(__DISKIO_INSTALL_SIZE__)
            sta request + diskio::IMPORT::INSTALL_SIZE + 0
            lda #.hibyte(__DISKIO_INSTALL_SIZE__)
            sta request + diskio::IMPORT::INSTALL_SIZE + 1

            lda #.lobyte(__DISKIO_RUN__)
            sta request + diskio::IMPORT::RESIDENT_BASE + 0
            lda #.hibyte(__DISKIO_RUN__)
            sta request + diskio::IMPORT::RESIDENT_BASE + 1
            lda #.lobyte(__DISKIO_SIZE__)
            sta request + diskio::IMPORT::RESIDENT_SIZE + 0
            lda #.hibyte(__DISKIO_SIZE__)
            sta request + diskio::IMPORT::RESIDENT_SIZE + 1
    .endmacro
.endif

.ifndef __NOIMPORTVARS
    .if !DYNLINK_IMPORT
        .importzp loadstatus

        .importzp loadaddrlo
        .importzp loadaddrhi

        .importzp decdestlo
        .importzp decdesthi

        .importzp endaddrlo
        .importzp endaddrhi

        .importzp bytesloadedlo
        .importzp bytesloadedhi

        .importzp param4
        .importzp param5

        .importzp pollyoffset
    .endif
.endif  ; !__NOIMPORTVARS

.if UNINSTALL_RUNS_DINSTALL
    ; Custom drive code macros,
    ; will be removed as soon as the custom drive code API is implemented

    ;.include "hal.inc"
    ; copied from hal.inc to avoid having that file in /loader/include

    .if PLATFORM = diskio::platform::COMMODORE_16

        .ifndef IO_PORT_DIR_COMMON
            .include "cpu.inc"
            .include "ted.inc"

            IO_PORT_DIR_COMMON = IO_PORT_SERIAL_DATA_IN_INPUT | IO_PORT_SERIAL_CLK_IN_INPUT | IO_PORT_CST_MTR_OUTPUT | IO_PORT_SERIAL_DATA_OUT_OUTPUT | IO_PORT_SERIAL_CLK_OUT_OUTPUT; $0b

                                                    ; effectively, this is          ; effectively, this is  
                                                    ; the idle flag:                ; the KERNAL flag:
                                                    ; 0 = input  = idle             ; 0 = input  = KERNAL,  
                                                    ; 1 = output = not idle         ; 1 = output = loader   
            IO_PORT_DIR_KERNAL = IO_PORT_DIR_COMMON | IO_PORT_SERIAL_ATN_OUT_OUTPUT | IO_PORT_CST_RD_INPUT ; $0f

            IO_PORT_DIR_IDLE   = IO_PORT_DIR_COMMON | IO_PORT_SERIAL_ATN_OUT_INPUT  | IO_PORT_CST_RD_OUTPUT; $1b

            IO_PORT_DIR_WAKEUP = IO_PORT_DIR_COMMON | IO_PORT_SERIAL_ATN_OUT_OUTPUT | IO_PORT_CST_RD_OUTPUT; $1f
        .endif

        .ifndef SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT_DEFINED
            SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT_DEFINED = 1
            .macro SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            lda #IO_PORT_CST_MTR | IO_PORT_SERIAL_DATA_OUT | (0 & IO_PORT_SERIAL_CLK_OUT) | (0 & IO_PORT_SERIAL_ATN_OUT); $09
            sta IO_PORT
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

        .ifndef BRANCH_IF_CLK_IN_SET_DEFINED
            BRANCH_IF_CLK_IN_SET_DEFINED = 1
            .macro BRANCH_IF_CLK_IN_SET to
            bit IO_PORT
            bvs to
            .endmacro
        .endif

        .ifndef INSTALL_IDLE_DEFINED
            INSTALL_IDLE_DEFINED = 1
            .macro INSTALL_IDLE
            lda #IO_PORT_CST_MTR | IO_PORT_SERIAL_DATA_OUT | IO_PORT_SERIAL_CLK_OUT | IO_PORT_SERIAL_ATN_OUT; $0f
            sta IO_PORT
            lda #IO_PORT_DIR_IDLE
            sta IO_PORT_DIRECTION
            .endmacro
        .endif

    .else ; PLATFORM <> diskio::platform::COMMODORE_16

        .ifndef CIA2_DDRA_COMMON

            .include "cia.inc"
            .ifdef _CIA_INC_; is not defined if via.inc rather than cia.inc had been included before

            ; CIA2 DDRA ($DD02) definitions
            CIA2_DDRA_COMMON   = CIA_SERIAL_DATA_IN_INPUT | CIA_SERIAL_CLK_IN_INPUT | CIA_VIC2_BANK_OUTPUT; $03
                                                                                                                                       ; effectively, this is the kernal flag;
                                                                                                                                       ; 0 = input  = loader,
            ; DATA OUT, CLK OUT, ATN out are low; RS232 is low                                                                         ; 1 = output = kernal
            CIA2_DDRA_KERNAL   = CIA2_DDRA_COMMON | CIA_SERIAL_DATA_OUT_OUTPUT | CIA_SERIAL_CLK_OUT_OUTPUT | CIA_SERIAL_ATN_OUT_OUTPUT | CIA_RS232_OUTPUT; $3f

            ; DATA OUT, CLK OUT, ATN OUT are high; RS232 is high; all bits except bank bits are inputs so that $dd00 writes do not change the bus state
            CIA2_DDRA_IDLE     = CIA2_DDRA_COMMON | CIA_SERIAL_DATA_OUT_INPUT  | CIA_SERIAL_CLK_OUT_INPUT  | CIA_SERIAL_ATN_OUT_INPUT  | CIA_RS232_INPUT ; $03

            ; DATA OUT is high, CLK OUT, ATN out are low; RS232 is high
            CIA2_DDRA_WAKEUP   = CIA2_DDRA_COMMON | CIA_SERIAL_DATA_OUT_INPUT  | CIA_SERIAL_CLK_OUT_OUTPUT | CIA_SERIAL_ATN_OUT_OUTPUT | CIA_RS232_INPUT ; $1b

            .endif; _CIA_INC_
        .endif

        .ifndef SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT_DEFINED
            SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT_DEFINED = 1
            .macro SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT
            lda #CIA2_DDRA_WAKEUP
            sta CIA2_DDRA
            .endmacro
        .endif

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

        .ifndef BRANCH_IF_CLK_IN_SET_DEFINED
            BRANCH_IF_CLK_IN_SET_DEFINED = 1
            .macro BRANCH_IF_CLK_IN_SET to
            bit CIA2_PRA
            bvs to
            .endmacro
        .endif

        .ifndef INSTALL_IDLE_DEFINED
            INSTALL_IDLE_DEFINED = 1
            .macro INSTALL_IDLE
            lda #CIA2_DDRA_IDLE
            sta CIA2_DDRA
            .endmacro
        .endif
    .endif

    .ifndef __NO_LOADER_SYMBOLS_IMPORT
        .import drivecode41
        .import drvcodebeg41
        .import drvprgend41

        .import drivecode71
        .import drvcodebeg71
        .import drivebusy71

        .import drivecode81
        .import drvcodebeg81
        .import drivebusy81
    .endif; !__NO_LOADER_SYMBOLS_IMPORT

    .macro __UPLOAD_DRIVECODE drivecode_address, driveprgend, driveprg

            .local uploadloop
            .local bitset
            .local noincaddrh

            jsr uninstall

            SET_DATA_OUT_CLEAR_CLK_OUT_CLEAR_ATN_OUT

            lda #.hibyte(drivecode_address)
            sta uploadloop + $02
            ldy #.lobyte(drivecode_address)
uploadloop: lda a:$0000,y
            SENDBYTE
            iny
            bne noincaddrh
            inc uploadloop + $02
noincaddrh: cpy #.lobyte(drivecode_address + driveprgend - driveprg)
            bne uploadloop
            ldx uploadloop + $02
            cpx #.hibyte(drivecode_address + driveprgend - driveprg)
            bne uploadloop
    .endmacro

    .macro UPLOAD_DRIVECODE_1541 drivecode_address
        __UPLOAD_DRIVECODE drivecode_address, drvprgend41, drvcodebeg41
    .endmacro; UPLOAD_DRIVECODE

    .macro UPLOAD_DRIVECODE_1571 drivecode_address
        __UPLOAD_DRIVECODE drivecode_address, drivebusy71, drvcodebeg71
    .endmacro; UPLOAD_DRIVECODE

    .macro UPLOAD_DRIVECODE_1581 drivecode_address
        __UPLOAD_DRIVECODE drivecode_address, drivebusy81, drvcodebeg81
    .endmacro; UPLOAD_DRIVECODE
.endif; UNINSTALL_RUNS_DINSTALL

.popseg

.endif; _LOADER_INC_
