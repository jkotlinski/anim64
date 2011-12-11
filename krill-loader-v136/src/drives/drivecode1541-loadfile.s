
; this module loads a file

.ifdef MODULE
    .include "cpu.inc"
    .include "via.inc"

    __NO_LOADER_SYMBOLS_IMPORT = 1
    .include "loader.inc"
    .include "drives/drivecode1541-kernel.inc"
    .include "kernelsymbols1541.inc"

    .org RUNMODULE - 3
    .word * - 2

    .byte (MODULEEND - MODULESTART + 3) / 256 + 1; number of module blocks, not quite accurate, but works for now
.endif; MODULE


TEMPTRACKLINKTAB         = $0780
TEMPSECTORLINKTAB        = $07c0

NEXTSECTOR               = UNUSED_ZP0
SECTORCOUNT              = UNUSED_ZP1

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (LOAD_ONCE = 0)
    HASHVALUE0LO         = TRACKLINKTAB + 1
    HASHVALUE0HI         = TRACKLINKTAB + 2
    HASHVALUE1LO         = TRACKLINKTAB + 3
    HASHVALUE1HI         = TRACKLINKTAB + 4
    FILENAME             = TRACKLINKTAB + 5; max. $10 bytes

    CURRDIRBLOCKSECTOR   = UNUSED_ZP2
    CYCLESTARTENDSECTOR  = UNUSED_ZP3
    NEXTDIRBLOCKSECTOR   = LOWMEM + 0
    DIRCYCLEFLAG         = LOWMEM + 1
    WRAPFILEINDEX        = LOWMEM + 2

    DIRBUFFER            = LOWMEM + 3
    DIRBUFFSIZE          = (LOWMEMEND - DIRBUFFER) / 4;
    DIRTRACKS            = DIRBUFFER
    DIRSECTORS           = DIRTRACKS + DIRBUFFSIZE
    FILENAMEHASHVAL0     = DIRSECTORS + DIRBUFFSIZE
    FILENAMEHASHVAL1     = FILENAMEHASHVAL0 + DIRBUFFSIZE
    DIRBUFFEND           = FILENAMEHASHVAL1 + DIRBUFFSIZE

    DIRBUFFSIZE41        = DIRBUFFSIZE
    .export DIRBUFFSIZE41

    NEW_DISK_VALUE       = $ff
    LOAD_FILE_VALUE      = $7f

            .assert !(((LOWMEMEND - DIRBUFFER) & 3) = 1), error, "***** 1 wasted code byte. *****"
            .assert !(((LOWMEMEND - DIRBUFFER) & 3) = 2), error, "***** 2 wasted code bytes. *****"
            .assert !(((LOWMEMEND - DIRBUFFER) & 3) = 3), error, "***** 3 wasted code bytes. *****"
            
            .assert DIRBUFFSIZE >= 9, error, "***** Dir buffer too small. *****"

.endif; (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (LOAD_ONCE = 0)


MODULESTART:
            ; get starting track and sector of the file to load

.if !LOAD_ONCE
    .if ::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME

            ldx #LOAD_FILE_VALUE
            lda DIRCYCLEFLAG; if #files < dirbuffsize (DIRCYCLEFLAG = $00),
            beq getfilenam  ; don't reset DIRCYCLEFLAG to $7f
            stx DIRCYCLEFLAG
getfilenam: jsr dgetbyte; get filename
            beq :+
            inx
            sta <(FILENAME - LOAD_FILE_VALUE - 1),x
            cpx #<(FILENAME_MAXLENGTH - 2 - LOAD_FILE_VALUE)
            bne getfilenam
:           jsr drwaitrkch; disables watchdog
            jsr gethashval
            sta FILENAMEHASH1
            stx FILENAMEHASH0

    .elseif ::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR

            jsr dgetbyte; get starting track
            sta FILETRACK
            jsr dgetbyte; get starting sector
            jsr drwaitrkch; disables watchdog
            sta FILESECTOR

    .else
            .error "***** Error: The selected file system option is not yet implemented. *****"
    .endif
.endif; !LOAD_ONCE

.if (::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR) || LOAD_ONCE
            ; check for illegal track or sector
            ldy FILETRACK
            beq toillegal + $00
            cpy #MAXTRACK41 + 1
            bcs toillegal + $01
            jsr getnumscts
            dex
            cpx FILESECTOR
            bcs :+
toillegal:  sec
            jmp illegalts
:
.endif; (::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR) || LOAD_ONCE

            lda #OPC_RTS; disable retry
            sta dsctcmps; on ID mismatch
spinuploop: ldy #ANYSECTOR; get any block on the current track, no sector link sanity check,
            jsr getblcurtr; don't store id, check after return
            bcs spinuploop; retry until any block has been loaded correctly

           ;clc
            beq :+; branch if disk id is the same, if not, re-read the dir
            sec; set the new disk flag when disks have been changed
.if LOAD_ONCE
:           ror DISKCHANGED
.else
:           lda #OPC_BNE; enable retry
            ror DISKCHANGED
            sta dsctcmps; on ID mismatch
.endif
            beq samedisk; branches to samedisk if no disk changes have happened

newdisk:    ; a new disk has been inserted

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (!LOAD_ONCE)

            lda #DIRTRACK
            ldy #DIRSECTOR
            jsr getblkstid; store id, sector link sanity check
            bcs newdisk
           ;ldy LINKSECTOR
            sty CYCLESTARTENDSECTOR

            ; filldirbuf -
            ; directory cycling: fill the dir buffer,
            ; this is also executed upon file not found in
            ; the current directory segment buffered (with y
            ; containing NEXTDIRBLOCKSECTOR's value)

            ldx #NEW_DISK_VALUE
            SKIPWORD
filldirbuf: ldx #LOAD_FILE_VALUE
            stx DIRCYCLEFLAG
            ldx #$ff
            stx WRAPFILEINDEX
            inx
            stx NUMFILES
nextdirsct: jsr checkchg; does not change y
            bne newdisk; check light sensor and start over if its state changed

            ; y contains the current dir sector number
            lda #DIRTRACK
            jsr getblkchid; compare id, sector link sanity check
            bcs newdisk; start over on error
           ;ldy LINKSECTOR
            stx CURRDIRBLOCKSECTOR
           ;lda LINKTRACK
            bne :+
            ; wrap around to first dir block
            ldy #DIRSECTOR + 1
:           sty NEXTDIRBLOCKSECTOR

            ldy #$03
getdirloop: jsr gcrdecode; get file's start track
           ;ldx NUMFILES
            sta DIRTRACKS,x
            tax          ; skip non-files
            beq dnotafile; denoted by track 0
            jsr gcrdecode; get file's start sector
            sta DIRSECTORS,x

            jsr fnamehash
            pha
            txa
            ldx NUMFILES; x is trashed in fnamehash
            sta FILENAMEHASHVAL0,x
            pla
            sta FILENAMEHASHVAL1,x

            ; advance to next file or quit loop
            inc NUMFILES
            cpx #DIRBUFFSIZE - 1
            lda CURRDIRBLOCKSECTOR
            bcs dirbuffull
            
            ; little flaw for the sake of saving on code size:
            ; when starting to cycle through the directory, the
            ; files in the dir block the last file currently in the dir
            ; buffer is in, will all be added to the buffer when it will
            ; be filled on the subsequent file load - this is why the
            ; minimum dir buffer size is 9 files
dnotafile:  tya
            and #%11100000; 8 entries per block, $20 bytes per entry
           ;clc
            adc #$23
            tay
            bcc getdirloop ; process all entries in a dir block

            ldy NEXTDIRBLOCKSECTOR; y is getblk sector argument
            cpy #DIRSECTOR + 1
            bne :+
            stx WRAPFILEINDEX
:           cpy CYCLESTARTENDSECTOR
            bne nextdirsct
            
            ; cycle complete
            inc DIRCYCLEFLAG; $ff->$00 or $7f->$80
            SKIPWORD
dirbuffull: sta NEXTDIRBLOCKSECTOR

            ; the disk was not changed, or the dir has just been read
samedisk:   lda #$00; clear new disk flag
            sta DISKCHANGED
            ldx NUMFILES
nextfile:   dex
            bpl findfile; check all dir entries in the buffer

            ; the dir buffer does not contain the file,
            ; so cycle through the directory to find it

            ldy NEXTDIRBLOCKSECTOR
            lda DIRCYCLEFLAG; cycle until the
            lsr             ; cycle is
            bcs filldirbuf  ; complete
            sec
            jmp filenotfnd

            ; must not change y
fnamehash:  ldx #-$01 - LOAD_FILE_VALUE - 1
:           stx GCRBUFFER + $00
            jsr gcrdecode
            ldx GCRBUFFER + $00
            cmp #' ' | $80; $a0 = end of filename
            beq gethashval
            inx
            sta <(FILENAME - LOAD_FILE_VALUE - 1),x
            cpx #<(FILENAME_MAXLENGTH - 2 - LOAD_FILE_VALUE)
            bne :-

            ; must not change y
            ; x = length of filename + LOAD_FILE_VALUE + 1
gethashval: clc
            stx HASHVALUE0LO
            stx HASHVALUE0HI
            stx HASHVALUE1LO
            stx HASHVALUE1HI
hashloop:   lda <(FILENAME - LOAD_FILE_VALUE - 1),x
            adc HASHVALUE0LO
            sta HASHVALUE0LO
            bcc :+
            inc HASHVALUE0HI
           ;clc; saved for size reasons            
:           adc HASHVALUE1LO
            sta HASHVALUE1LO
            lda HASHVALUE0HI
            adc HASHVALUE1HI
            sta HASHVALUE1HI
            dex
            bmi hashloop
            adc HASHVALUE1LO
            tax
            lda HASHVALUE0LO
            adc HASHVALUE0HI
            rts

findfile:   lda FILENAMEHASH0
            eor FILENAMEHASHVAL0,x
            bne nextfile
            lda FILENAMEHASH1
            eor FILENAMEHASHVAL1,x
            bne nextfile

            ; file found

            stx FILEINDEX; store index of file to jump to the track of the file
                         ; following this one in the dir, after loading

            ; store number of the dir block loaded last,
            ; it is used to start the dir check cycle if
            ; the next file is not found in the dir buffer;
            ; it is also checked on the subsequent load to determine if the
            ; dir check cycle is complete and the file be said to be not found
            lda CURRDIRBLOCKSECTOR
            sta CYCLESTARTENDSECTOR
            sta NEXTDIRBLOCKSECTOR
            jsr ddliteon; passes errorret, returns with carry set
            lda DIRTRACKS,x
            ldy DIRSECTORS,x

            ; there should be a check for illegal track or sector
            ; here - unfortunately, there is no memory left for it

.else; ::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR && (!LOAD_ONCE)

            ; a new disk has been inserted

    .if LOAD_ONCE
:           lda CURTRACK
            ldy #ANYSECTOR; no sector link sanity check
            jsr getblkchid; compare id
            bcs :-
            lda #$00; clear new disk flag
            sta DISKCHANGED
            bcc spinuploop
    .else
            ; store new disk id
:           lda CURTRACK
            ldy #ANYSECTOR; no sector link sanity check
            jsr getblkstid; store id
            bcs :-
            lda #$00; clear new disk flag
            sta DISKCHANGED
    .endif; !LOAD_ONCE

samedisk:
    .if LOAD_ONCE
            lda #OPC_BNE; enable retry
            sta dsctcmps; on ID mismatch
    .endif
            jsr ddliteon; passes errorret, returns with carry set
            lda FILETRACK
            ldy FILESECTOR

.endif; !(::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR) || LOAD_ONCE

            ; a contains the file's starting track here
            ; y contains the file's starting sector here
            ldx #$00
            stx BLOCKINDEXBASE
           ;sec; load first block in order to determine
               ; the file's load address
trackloop:  php; out-of-order flag
            sty SECTORTOFETCH
            ; x = amount of blocks on the previous track
            pha; next track
            txa
            clc
            adc BLOCKINDEXBASE
            sta BLOCKINDEXBASE
            pla; next track
            jsr trackseek

            ; scan the track for the file links
            ; x contains the total number of blocks on this track
           ;lda #SECTORISPROCESSED      ; the accu contains a negative number
:           sta TRACKLINKTAB - $01,x    ; mark all sectors as processed, the scan loop will
            sta TEMPTRACKLINKTAB - $01,x; put sector numbers to the corresponding indices
            dex
            bne :-
scantrloop: lda #OPC_BIT_ZP; only fetch the first few bytes to track the block links
            ; this is a weak point since there is no data checksumming here -
            ; however, sector link sanity is checked
            ldy #ANYSECTORSANELINK; sector link sanity check
            sty REQUESTEDSECTOR
            jsr getblkscan
            bcs scantrloop; branch until fetch successful
           ;ldx LOADEDSECTOR
           ;lda LINKTRACK; illegal tracks are checked after
                         ; this track has been processed
            sta TEMPTRACKLINKTAB,x; store the sector's track link and mark it
                                  ; as processed
           ;ldy LINKSECTOR
            tya
            sta TEMPSECTORLINKTAB,x

            ; go through the link list to find the blocks's order on the track
            ldy #$00
            ldx SECTORTOFETCH; first file block on this track
            stx LINKSECTOR
:           lda TEMPTRACKLINKTAB,x
            bmi scantrloop; branch if not all of the file's blocks on this track
                          ; have been scanned yet
            sty TRACKLINKTAB,x; store sector index
            iny; increase sector index
            pha; link track
            lda TEMPSECTORLINKTAB,x; get link sector
            tax
            pla; link track
            cmp CURTRACK; check whether link track is the current track
            beq :-      ; branch until all the file's blocks on the current
                        ; track have been ordered
                        ; loops in the link graph are not detected and will cause an endless loop

            ; the track's sector links are scanned now

            plp; out-of-order flag
            ; read and transfer all the blocks that belong to the file
            ; on the current track, the blocks are read in quasi-random order´
            pha         ; next track
            tya         ; amount of the file's blocks on the current track
            pha
            stx NEXTSECTOR; first sector on the next track
            sty SECTORCOUNT; amount of the file's blocks on the current track

blockloop:  ldy #UNPROCESSEDSECTOR; find any yet unprocessed block belonging to the file
            bcc :+; carry clear: load out-of-order
            ldy LINKSECTOR; load the next block in order
:           sty SECTORTOFETCH

:           ldy SECTORTOFETCH; negative: any unprocessed sector, positive: this specific sector; sector link sanity check
            jsr getblcurtr   ; read any of the files's sectors on the current track, compare id
            bcs :-           ; retry until a block has been successfully loaded

            ; send the block over
           ;ldx LOADEDSECTOR
            ldy #SECTORISPROCESSED; $ff
            sty TRACKLINKTAB,x; mark the loaded block as processed
           ;ldy #$ff
           ;lda LINKTRACK
            tax
            bne :+
            ldy LINKSECTOR; the file's last block's length
:           sty dsendcmp + $01
            dey
            dey
            tya
            ldy #$01
            jsr gcrencode; block length
            clc
            lda BLOCKINDEX
            adc BLOCKINDEXBASE
            jsr sendblock; send the block over, this decreases SECTORCOUNT
            ; carry-flag is cleared if the next block may be loaded out of order
            lda SECTORCOUNT; sector count for the current track
            bne blockloop

            ldy NEXTSECTOR
            pla; amount of the file's blocks on the current track
            tax
            pla; next track
            ; carry-flag is cleared if the next block may be loaded out of order
            bne trackloop; process next track

            ; loading is finished

.if (::FILESYSTEM = FILESYSTEMS::DIRECTORY_NAME) && (!LOAD_ONCE)

            ldx FILEINDEX
            inx
            lda DIRTRACKS,x
            cpx WRAPFILEINDEX
            beq :+
            cpx NUMFILES
            bcc :++
:           lda #DIRTRACK
:           jsr trackseek; move head to the start track of the next file in the directory

            clc; all ok after loading

filenotfnd: ; branches here with carry set
.else
            clc; all ok after loading
.endif

illegalts:  ; file not found or illegal t or s
            jsr sendstatus

            ldy #$01; turn motor and busy led off
            lda #BUSY_LED; check if busy led is lit
            and VIA2_PRB
            beq :+
            ldy #$ff; fade off the busy led, then turn motor off
.if LOAD_ONCE
:           jmp duninstall
.else
:           ENABLE_WATCHDOG
:           bit VIA1_PRB; check for ATN in to go high:
            bpl :-; wait until the computer has acknowledged the file transfer
            sei; disable watchdog
            jmp driveidle
.endif

.if ::FILESYSTEM = FILESYSTEMS::TRACK_SECTOR
            .res 64; prevent page-boundary crossing below
.endif

            ; carry: clear = ok, set = load error
sendstatus: lda #$00
            sta SECTORCOUNT; make sure DATA OUT (track change) is not raised after transferring the status
            sta dsendcmp + $01; just send over one byte
            sbc #$01; carry clear: result is $00 - $02 = $fe - loading finished successfully
                    ; carry set:   result is $00 - $01 = $ff - load error

            ; accu: block index or status byte
sendblock:  ldy #$00
            jsr gcrencode; block index or status byte

            ldx #$ff
            ldy #$10; here, the watchdog timer is polled manually because
                    ; an extra-long time-out period is needed since the computer may
                    ; still be busy decompressing a large chunk of data;
                    ; this is the round counter
            lda #DATA_OUT
            sta VIA1_PRB; block ready signal
            bne :+
            ; a watchdog is used because the computer might be reset while sending
            ; over the block, leaving the drive waiting forever for handshake pulses
waitready:  lda VIA2_T1C_H; see if the watchdog barked
            bne :++
            dey           ; if yes, decrease the round counter
.if DISABLE_WATCHDOG
            beq nowatchdog
nowatchdog:
.else
            beq timeout; and trigger watchdog on time-out
.endif
:           stx VIA2_T1C_H; reset watchdog time-out and clear irq flag
:           bit VIA1_PRB
            bpl waitready; wait for ATN strobe
            stx VIA2_T1C_H; reset watchdog time-out and clear possibly set irq flag

timeout:    ENABLE_WATCHDOG
            ldy #$00
sendloop:   ldx LONIBBLES,y    ; 4
            lda SENDGCRTABLE,x ; 4 - zp access
                               ; = 22 (20 on computer)

:           bit VIA1_PRB       ; 4 - sync 4: wait for ATN low
            bmi :-             ; 3 - first byte: wait for end of ATN strobe
            sta VIA1_PRB       ; 4
            asl                ; 2
            ora #ATNA_OUT      ; 2 - next bit pair will be transferred with ATN high - if not set, DATA OUT will be low
            ldx HINIBBLES,y    ; 4
                               ; = 19 (22 on computer)

:           bit VIA1_PRB       ; 4 - sync 1: wait for ATN high
            bpl :-             ; 3
            sta VIA1_PRB       ; 4
            lda SENDGCRTABLE,x ; 4 - zp access
            sta VIA2_T1C_H     ; 4 ; reset watchdog time-out
                               ; = 19 (19 on computer)

:           bit VIA1_PRB       ; 4 - sync 2: wait for ATN low
            bmi :-             ; 3
            sta VIA1_PRB       ; 4
            asl                ; 2
            ora #ATNA_OUT      ; 2 - next bit pair will be transferred with ATN high - if not set, DATA OUT will be low
dsendcmp:   cpy #$00           ; 2
            iny                ; 2
                               ; = 19 (19 on computer)

:           bit VIA1_PRB       ; 4 - sync 3: wait for ATN high
            bpl :-             ; 3
            sta VIA1_PRB       ; 4
            bcc sendloop       ; 3
                               ; = 79, 7 more than the theoretical limit at 18 cycles per bitpair

            .assert .hibyte(*) = .hibyte(sendloop), error, "***** Page boundary crossing in byte send loop, fatal cycle loss. *****"

:           bit VIA1_PRB; wait for acknowledgement
            bmi :-      ; of the last data bit pair

            ldy #CLK_OUT
            dec SECTORCOUNT
            bne :+      ; pull DATA_OUT high when changing tracks
drwaitrkch: ldy #CLK_OUT | DATA_OUT; flag track change
:           clc; load blocks out of order
            ; it is only possible to load the file's first blocks in order and then switch
            ; to loading out of order - switching back to loading in order will cause
            ; the stream code to hiccup and thus faulty file data to be loaded
            jmp drivebusy; will announce busy and track change flag, disable watchdog and perform rts
MODULEEND:

LOADFILE41END = *
.export LOADFILE41END

            .assert * <= LONIBBLES, error, "***** 1541 drive code too large. *****"
