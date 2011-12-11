
; configuration
; set .defines to non-0 to enable the corresponding features

; Note:
;   Please keep in mind that most of the following parameters change the code size.
;   For a minimum-size resident portion in the host's memory,
; set all parameters in the 'increase code size' section to 0,
; and those in the 'code size reduction' sections to non-0,
; set FILESYSTEM = FILESYSTEMS::TRACK_SECTOR,
; and set DECOMPRESSOR = DECOMPRESSORS::NONE

; parameters

; these change installer code

.define MIN_DEVICE_NO             08 ; these two settings define the device number range when scanning for devices in install.s,
.define MAX_DEVICE_NO             30 ; devices beyond this range will not be recognized or useable

; these change the host-side code

.ifndef PLATFORM
PLATFORM                           = diskio::platform::COMMODORE_64; currently available are COMMODORE_64 and COMMODORE_16
.endif

; these change the drive-side code

FILESYSTEM                         = FILESYSTEMS::DIRECTORY_NAME; currently available are DIRECTORY_NAME and TRACK_SECTOR

.define LOAD_ONCE                  0 ; opens a file using the KERNAL API on initialization,
                                     ; loads it whenever the loader is called,
                                     ; then uninstalls the loader automatically;
                                     ; requires FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME
                                     ; reduces code size

.define DIRTRACK                  18 ; for FILESYSTEM::DIRECTORY_NAME, on 1541/41-C/41-II/70/71/71CR,
                                     ; starting track and sector location of the directory
.define DIRSECTOR                 00 ; (i.e., the loader's directory can be relocated to hide it from the normal directory command);
                                     ; ineffective for LOAD_ONCE and when loading via KERNAL with LOAD_VIA_KERNAL_FALLBACK != 0
.define FILENAME_MAXLENGTH        16 ; maximum length of file name, if a directory is capable of holding longer names, extra characters are ignored

; track jump settings
.define MINSTPSP                 $18 ; min. r/w head stepping speed on 1541/41-C/41-II/70/71/71CR
.define MAXSTPSP                 $10 ; max. r/w head stepping speed on 1541/41-C/41-II/70/71/71CR
.define STEPRACC                 $1c ; r/w head stepping acceleration on 1541/41-C/41-II/70/71/71CR


; features

; features, won't change host-side code size but change some functionality

; changes drive-side code
.define UNINSTALL_RUNS_DINSTALL    1 ; this is a patch until the CUSTOM_DRIVE_CODE_API is implemented, use it only if you really know what you are doing.
                                     ; calling uninstall will run dinstall on the drive side, enabling the caller to upload own custom drive code
                                     ; to the .org of the loader, with the size of the loader's drive code.
                                     ; to upload and run the code in the drive, use the UPLOAD_DRIVECODE_15x1 <address> macro after including
                                     ; include/loader.inc
                                     ; requires UNINSTALL_API

.define DISABLE_WATCHDOG           0 ; disable the drive-side watchdog - the watchdog will reset the drive safely when the host resets at any point,
                                     ; however, the drive hardware (1541 and 1571) only allows for a maximum time-out period of 65536 cycles: this means
                                     ; that letting the loader starve for a few video frames (about 1.5 frames since the time-out is often just 32768
                                     ; cycles due to size optimization) will reset the drive, which can be prevented using this option.
                                     ; Note: On Plus/4, this option must be enabled for VICE compatibility, as VICE suffers from spurious watchdog interrupts.


; following settings are independent from the installed drive code,  several host-side
; binaries with different features may be used with the same installed drive code

; features, increase code size

.define JUMP_TABLE                 1 ; add a jump table to the beginning of the DISKIO segment, useful when importing the binaries without symbols

.define LOAD_UNDER_D000_DFFF       0 ; C-64 only: enable loading (and decompression) to the RAM at $D000-$DFFF, this slows down loading quite a bit.
                                     ; the IRQ handlers will need to change $01 to enable the I/O-RAM at $D000..$DFFF,
                                     ; so make sure the IRQ handlers restore the $01 status to the value as when they are called.
                                     ; the IRQs must run via $FFFE/F, since the ROM is disabled when accessing the RAM at $D000-$DFFF
                                     ; this is not needed when only memdecompressing to $D000..$DFFF (simply set $01 to $30 and jsr memdecomp in that case)

.define LOAD_TO_API                0 ; if the carry flag is set on load, the loader will use the address set in loadaddrlo/loadaddrhi as loading address
                                     ; for raw files and the streambuffer, and ignore the file's loading address.
                                     ; this feature affects the loading address of compressed data when loading using loadcompd,
                                     ; it does not affect the address of the decompressed data

.define END_ADDRESS_API            0 ; after loading, the file's end address (last file byte + 1) is stored in endaddrlo and endaddrhi.
                                     ; for loading compressed files using loadcompd, the end address of the compressed data is returned.
                                     ; (the file's loading address can always be found in loadaddrlo/loadaddrhi after loading)

.define LOAD_RAW_API               1 ; include the loadraw routine to load files without decompressing

.define LOAD_COMPD_API             1 ; include the loadcompd routine to load and depack compressed files on the fly
                                     ; requires DECOMPRESSOR != DECOMPRESSORS::NONE

.define OPEN_FILE_POLL_BLOCK_API   1 ; include the openfile and pollblock calls for polled loading

.define GETC_API                   1 ; include the getc call to get a byte from the open file's stream;
                                     ; the getc call does not return the file's first two bytes (stream buffer loading address)
                                     ; requires OPEN_FILE_POLL_BLOCK_API != 0

.define GETCHUNK_API               1 ; include the getchunk call to get a chunk from the open file's stream;
                                     ; the getchunk call does not return the file's first two bytes (stream buffer loading address)
                                     ; currently, the GETCHUNK_API only supports extracting uncompressed data from compressed stream files
                                     ; requires OPEN_FILE_POLL_BLOCK_API != 0 and currently DECOMPRESSOR != DECOMPRESSORS::NONE

.define MEM_DECOMP_API             1 ; include routines for memory decompression; that is, loading and decompression can be separated,
                                     ; these are memdecomp (first crunched subfile) and cmemdecomp (consecutive crunched subfiles,
                                     ; just skips loading address bytes at the current compressed address and ignores decompression address parameters);
                                     ; decompress to $D000..$DFFF doesn't need to have LOAD_UNDER_D000_DFFF enabled, just enable 64 kB of ram before jsr
                                     ; memdecomp,
                                     ; requires DECOMPRESSOR != DECOMPRESSORS::NONE
                                     ; this option does not implicitly turn on the LOAD_RAW_API

.define MEM_DECOMP_TO_API          0 ; if carry is set on decompression, the decompressor will use the address set in decdestlo/decdesthi as
                                     ; decompression destination address and ignore the file's decompression address.
                                     ; requires MEM_DECOMP_API != 0

.define CHAINED_COMPD_FILES        1 ; if non-0, it is possible to load files that are several compressed files glued one after another
                                     ; (simply concatenated compressed files with load address each);
                                     ; with these files, the loading address of the separate sub-files must be rising;
                                     ; simply load these chained compressed files using the loadcompd call as usual,
                                     ; or load raw and use memdecomp for first, then cmemdecomp for subsequent
                                     ; compressed files (cmemdecomp will simply skip the load address);
                                     ; take care that the memory hole between the last and second-to-last chunk, or more, may be trashed, since
                                     ; the raw data may occupy these holes - this does not apply if NO_DECOMPLOAD_OPTIMIZATION != 0;
                                     ; requires DECOMPRESSOR != DECOMPRESSORS::NONE

.define LOAD_VIA_KERNAL_FALLBACK   1 ; loads via the KERNAL API if the loader's installation was not successful
                                     ; (i.e., if the loader is not installed due to an incompatible drive (currently everything that is not
                                     ; a 1541, 1541-C, 1541-II, 1570, 1571, 1571CR, or 1581),
                                     ; or true drive emulation being disabled,
                                     ; or more than one device on the serial bus with PROTOCOL::TWO_BITS_ATN).
                                     ; the IRQ handlers can be delayed for some rasterlines up to several frames due to KERNAL routines
                                     ; temporarily disabling IRQ (but that is unlikely for devices not using the serial bus).
                                     ; for the sake of compatibility, only disable this option if the space is really needed.
                                     ; C-64:
                                     ; Attention: KERNAL, BASIC, and possible cartridge ROMs are enabled, so IRQ handlers are not
                                     ; allowed in the ranges $8000..$BFFF and $D000..$FFFF.
                                     ; Attention: KERNAL routines might execute a CLI, so make sure to have valid IRQ vectors and handlers,
                                     ; or disable all IRQ sources (not via SEI), also make sure to correctly handle the different
                                     ; IRQ conditions when called via KERNAL vector ($0314) vs. non-KERNAL vector ($FFFE) - best have KERNAL and
                                     ; BASIC enabled before calling the loader, so only the KERNAL vector IRQ handler is needed (please note that
                                     ; the handler code is delayed a little when called via $0314 rather than $FFFE).
                                     ; Plus/4:
                                     ; Attention: The ROM space in the upper memory half is enabled, so IRQ handlers are not allowed
                                     ; in the range $8000..$FFFF.
                                     ; Attention: The ROM routines may execute a CLI, so make sure to have valid IRQ vectors and handlers,
                                     ; or disable all IRQ sources (not via SEI), also make sure to correctly handle the different
                                     ; IRQ conditions when called via ROM vector ($0314) vs. non-ROM vector ($FFFE) - best have ROM enabled
                                     ; before calling the loader, so only the ROM vector IRQ handler is needed (please note that
                                     ; the handler code is delayed a little when being called via $0314 rather than $FFFE).
                                     ; requires FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME

.define LOAD_PROGRESS_API          0 ; keeps updating bytesloadedlo/hi while loading with the amount
                                     ; of bytes that have been loaded (not decompressed!) so far;
                                     ; to be used for progress displays

.define IDLE_BUS_LOCK              1 ; C-64 only: allow for arbitrary $DD00 writes ($00-$FF) when the loader
                                     ; is idle (good for raster routines with LDA #value:STA $D018:STA $DD00, e.g.),
                                     ; the writes must have a minimum distance of 15 µs (i.e., 15 cycles at 1 MHz).
                                     ; however, if one of these $DD00 writes enables the ATN OUT bit
                                     ; (bit 3 in $DD00), only one device on the bus is allowed for all protocols.

.define NONBLOCKING_API            1 ; include non-blocking calls, so it is possible to run code in the main thread while timer NMIs are used
                                     ; to poll and download the blocks - the loader works in interrupt handlers instead of the main thread

.define NONBLOCKING_WITH_KERNAL_ON 1 ; C-64 only: if non-0, will set NMI vectors at both $FFFA/B and $0318/9
                                     ; only effective if NONBLOCKING_API is non-0

.define UNINSTALL_API              1 ; include an uninstallation routine


; parameters

LC_SPEED                       =   6 ; Taboo Levelcrush speed, irrelevant if using other compressors

; non-blocking operation: these intervals are timed for 1541/1541-C/1541-II loading speed, unit is charlines (= 8 scanlines)
POLLINGINTERVAL_STARTLOAD      = $0a ; spin-up and track seek
POLLINGINTERVAL_TRACKCHANGE    = $27 ; 1 PAL frame, track seek
POLLINGINTERVAL_GETBLOCK       = $12 ; time to load and decode the next block
POLLINGINTERVAL_BLOCKSOON      = $02 ; timer interval after one of the bigger intervals listed above
POLLINGINTERVAL_REPOLL         = $01 ; maximum polling frequency, block is ready but downloading
                                     ; is postponed because of a running IRQ handler

DECOMPRESSOR                   = DECOMPRESSORS::EXOMIZER; available are NONE, BYTEBOOZER, LEVELCRUSH, PUCRUNCH, EXOMIZER


; INEFFECTIVE or DISENCOURAGED
; following assignments and defines are ineffective or disencouraged in this version

; DISENCOURAGED (unfinished)
; changes drive-side code and host-side install code
.define INSTALL_FROM_DISK          0 ; enabling this option will reduce the size of host-side install code - the drive-side code is loaded
                                     ; by the drive itself from the disk, rather than being sent over to the drive by the host


; the following two options are combineable
; both options enabled at the same time accelerate the loader by ~33% on 1541/41-C/41-II, and ~17% on 1570/71/71CR, and 0% for other drives

; INEFFECTIVE
.define FAST_FILE_FORMAT           0 ; load all files encoded with the fast file format, this saves up to 1 revolution per track and loading is ~17% faster
                                     ; compared to normal encoding on a 1541/41-C/41-II/70/71/71CR - this has no influence (i.e., only minimal negative influence)
                                     ; on the speed of loading on a 1581/fd/hd,
                                     ; this reduces a block's data capacity by 2 bytes,
                                     ; loading files encoded in standard format or fast encoding format only is not possible with this option enabled
                                     ; use cc1541 to copy files onto d64/d71 images in the fast file format

; INEFFECTIVE
.define FAST_ENCODING_FORMAT       0 ; load all files encoded with the fast encoding format, this saves 1 revolution per track and loading is ~17% faster
                                     ; compared to normal encoding on a 1541/41-C/41-II (not 1570/71/71CR!),
                                     ; this reduces a block's data capacity by 96 bytes,
                                     ; loading files encoded in standard format or fast file format only is not possible with this option enabled.
                                     ; use cc1541 to copy files onto d64/d71 images in the fast encoding format


; features, increase code size
; INEFFECTIVE
.define DECOMPLOAD_TO_API          0 ; if carry is set on load, the loader will use the address set in decdestlo/decdesthi as loading address
                                     ; for compressed files and ignore the decompressed address
                                     ; stored in the file

; INEFFECTIVE
.define CUSTOM_DRIVE_CODE_API      0 ; support routines for custom drive code that allows uploading custom
                                     ; code to the drive and run it to use the drive's cpu as a co-processor;
                                     ; this won't work if the loader's drive code is not installed, with the loader being running in the
                                     ; KERNAL fallback mode

; code size reduction to the cost of speed

; INEFFECTIVE
.define NO_DECOMPLOAD_OPTIMIZATION 0 ; disables the decompression while loading speed optimization and stalls the drive until a whole block of data
                                     ; is decompressed,
                                     ; setting this feature to non-0 will make the loadcompd call not overwrite the first few bytes
                                     ; after the raw uncompressed data;
                                     ; decreases combined loading + decompressing speed
                                     ; requires DECOMPRESSOR != DECOMPRESSORS::NONE, and SEPARATE_DECOMP = 0

; parameters

; INEFFECTIVE
PROTOCOL = PROTOCOLS::TWO_BITS_ATN

; end ineffective assignments and defines
