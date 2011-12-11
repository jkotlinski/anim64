
JOBCODE0300          = $00
JOBCODE0400          = $01
JOBCODE0500          = $02
JOBCODE0600          = $03
JOBCODE0700          = $04
JOBCODE0800          = $05
LIBTRACK             = JOBCODE0500; these can be used because
LIBSECTOR            = JOBCODE0600; both track and sector values
LOADEDSECTOR         = JOBCODE0700; are positive and thus not
NUMSECTORS           = JOBCODE0800; interpreted as job codes
LINKTRACK            = $06
LINKSECTOR           = $07
GCRBUFFER            = $08; $08..$0e, $07 bytes
NUMSECTORSTEMP       = $08
CURRSTEPSPEEDLOW     = $08; these two bytes are only clobbered between
TRACKINC             = $09; fetching and decoding blocks
LOADEDMODULE         = $0f
BLOCKINDEX           = $10
SENDGCRTABLE         = $11
DISKCHANGEBUFFER     = $11
DISKCHANGED          = $12
CURTRACK             = $13
REQUESTEDSECTOR      = $14
NUMFILES             = $15
ID0                  = $16; = ROMOS_HEADER_ID0
ID1                  = $17; = ROMOS_HEADER_ID1
FILETRACK            = $18; = ROMOS_HEADER_TRACK, set by ROM on LOAD_ONCE
FILESECTOR           = $19; = ROMOS_HEADER_SECTOR, set by ROM on LOAD_ONCE
FILENAMEHASH0        = FILETRACK
FILENAMEHASH1        = FILESECTOR
FILEINDEX            = FILETRACK
;GCR_8               = $1a; $09
;GCR_0               = $1b; $0a
;GCR_1               = $1c; $0b
SECTORTOFETCH        = $1d
;GCR_C               = $1e; $0d
;GCR_4               = $1f; $0e
;GCR_5               = $20; $0f
BLOCKINDEXBASE       = $21
UNUSED_ZP0           = $22
;GCR_2               = $23; $12
;GCR_3               = $24; $13
UNUSED_ZP1           = $25
;GCR_F               = $26; $15
;GCR_7               = $27; $16
;GCR_8               = $28; $17
UNUSED_ZP2           = $29
;GCR_9               = $2a; $19
;GCR_A               = $2b; $1a
;GCR_B               = $2c; $1c
UNUSED_ZP3           = $2d
;GCR_D               = $2e; $1d
;GCR_E               = $2f; $1e

INITBUF_TRACK_DIFF   = NUMFILES

TRACKLINKTAB         = $30; $15 bytes

LOWMEM               = $45

POINTER              = LOWMEM; $02 bytes

LONIBBLES            = $0600
HINIBBLES            = $0700

ROMOS_HEADER_ID0     = $16
ROMOS_HEADER_ID1     = $17
ROMOS_HEADER_TRACK   = $18
ROMOS_HEADER_SECTOR  = $19
ROMOS_TRACK_DIFF     = $42

GCRENCODE            = $f77f
GCRDECODEHI          = $f8a0
GCRDECODELO          = $f8c0


BINARY_NIBBLE_MASK   = %00001111
GCR_NIBBLE_MASK      = %00011111

NUMMAXSECTORS        = 21
MAXTRACK41           = 41


JOBCODE_EXECUTE      = $d0

ANYSECTOR            = $ff; no sector link sanity check
ANYSECTORSANELINK    = $80; sector link sanity check
UNPROCESSEDSECTOR    = $7f; sector link sanity check

SECTORISPROCESSED    = $ff


.if DISABLE_WATCHDOG
    .macro ENABLE_WATCHDOG
        sei
    .endmacro
.else
    .macro ENABLE_WATCHDOG
        cli
    .endmacro
.endif
