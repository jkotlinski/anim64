
.fileopt comment, "Loader dynamic link code portion"
.fileopt compiler, "CA65"
.fileopt author, "Gunnar Ruthenberg"

TEST_FALLBACK = 0

.include "cpu.inc"

.include "basic.inc"

__NO_LOADER_SYMBOLS_IMPORT = 1
.include "loader.inc"

.if PLATFORM = diskio::platform::COMMODORE_16
.include "ted.inc"
.endif

.segment "DISKIO_ZP" : zeropage

.export __ZP_START__ : absolute
__ZP_START__ = __DISKIO_ZP_RUN__

.export regbank
.export sp
.export ptr1
.export ptr2
.export ptr3
.export tmp1
.export mode

regbank:    .res 6
sp:         .res 2
ptr1:       .res 2
ptr2:       .res 2
ptr3:       .res 2
pointer1   = ptr1
pointer2   = ptr2
pointer3   = ptr3
pointer4:   .res 2
pointer5:   .res 2
tmp1:       .res 2
temp1      = tmp1
temp2:      .res 1
mode:       .res 1

readdest:   .res 2

Module    = regbank + 0
TPtr      = regbank + 4


.segment "CODE"; the import routines are not in an extra segment because the linked funtions coming from the ca65 asm library
               ; are defined to run in the CODE segment, and because there is no rename function for segments in ld65

.include "kernal.inc"

.include "../asminc/modload.inc"
.include "diskio.inc"

.import popax
.import init_decruncher
.import get_decrunched_byte
.import __STACK_START__
.import __STACK_SIZE__
.import __DISKIO_ZP_RUN__

.export importldr
.export _malloc
.export _free
.export get_crunched_byte
.export buffer_start_hi : absolute; library decompression
.export buffer_len_hi   : absolute; ring buffer

.macpack longbranch


DIRFILENO         = $01
LIBFILENO         = $01

BUFFERSIZE        = $0200

FUNCTION_IMPORTED = $80


            ; Import the loader

            ; in:  a/x - lo/hi to import request structure
            ; out: c - set on error
            ;      a - status, on diskio::status::GENERIC_KERNAL_ERROR, the KERNAL error code is returned in x
            ;      x - if status is diskio::status::OK, lo-byte of version string address,
            ;          if status is diskio::status::WRONG_VERSION, required major version number
            ;      y - if status is diskio::status::OK, hi-byte of version string address,
            ;          if status is diskio::status::WRONG_VERSION, found (but bad) major version number

importldr:  sta modctrl + MOD_CTRL::CALLERDATA + 0
            sta sp + 0
            stx modctrl + MOD_CTRL::CALLERDATA + 1
            stx sp + 1

            lda #.lobyte(readfunc)
            sta modctrl + MOD_CTRL::READ + 0
            lda #.hibyte(readfunc)
            sta modctrl + MOD_CTRL::READ + 1

            ; find out how many requested functions there are and
            ; prepare check list to mark imported functions,
            ; it is stored backwards at the end of the install memory block

            clc
            ldy #diskio::IMPORT::INSTALL_BASE + 0
            lda (sp),y
            ldy #diskio::IMPORT::INSTALL_SIZE + 0
            adc (sp),y
            sta pointer4 + 0
            ldy #diskio::IMPORT::INSTALL_BASE + 1
            lda (sp),y
            ldy #diskio::IMPORT::INSTALL_SIZE + 1
            adc (sp),y
            sta pointer4 + 1

            ldx #$00
            stx numrqinstf
            stx numrqresif
            stx numrqfuncs
            stx featsmatch
            ldy #.sizeof(diskio::IMPORT) - 1
countinstf: iny
            bne :+
            inx
            inc sp + 1
:           lda #diskio::END_OF_LIST
            cmp (sp),y
            beq countresif; branch when install symbols done

            inc numrqinstf
            inc numrqfuncs
            inc featsmatch
            lda pointer4 + 0
            bne :+
            dec pointer4 + 1
:           dec pointer4 + 0
            txa
            pha
            tya
            ldx #$00
            sta (pointer4,x); store offset lo into the table
            pla
            tax
            lda pointer4 + 0
            bne :+
            dec pointer4 + 1
:           dec pointer4 + 0
            txa
            pha
            ldx #$00
            sta (pointer4,x); store offset hi into the table
            pla
            tax
            ; skip features
            lda #diskio::END_OF_LIST
:           iny
            bne :+
            inx
            inc sp + 1
:           cmp (sp),y
            bne :--
            beq countinstf

countresif: iny
            bne :+
            inx
            inc sp + 1
:           lda #diskio::END_OF_LIST
            cmp (sp),y
            beq endcountf

            inc numrqresif
            inc numrqfuncs
            inc featsmatch
            lda pointer4 + 0
            bne :+
            dec pointer4 + 1
:           dec pointer4 + 0
            txa
            pha
            tya
            ldx #$00
            sta (pointer4,x); store offset lo into the table
            pla
            tax
            lda pointer4 + 0
            bne :+
            dec pointer4 + 1
:           dec pointer4 + 0
            txa
            pha
            ldx #$00
            sta (pointer4,x); store offset hi into the table
            pla
            tax

            ; skip features
            lda #diskio::END_OF_LIST
:           iny
            bne :+
            inx
            inc sp + 1
:           cmp (sp),y
            bne :--
            jmp countresif

endcountf:  ; set the c stack pointer
            lda #.lobyte(__STACK_START__ + __STACK_SIZE__ - 1)
            sta sp + 0
            lda #.hibyte(__STACK_START__ + __STACK_SIZE__ - 1)
            sta sp + 1

.if TEST_FALLBACK
            jmp fallback
.endif

            ; try to import a file from disk,
            ; this just tries file by file until any of them works
            lda #$01
            sta fileindex
importloop: lda fileindex
            sta filecount

            ; the nice way is to open a file for the directory,
            ; then read a filename, try to import the corresponding file
            ; while leaving the directory file open, then continue with
            ; the next name in the dir if failed, etc.
            ; however, there seems to be a bug in the drive's firmwares
            ; when having 2 files open at the same time, so the dir file
            ; starts over at entry #7.
            ; so the awkward method of re-opening the directory after each
            ; failed import and then skipping the already processed files
            ; has to be used when not using a dir buffer.

            lda #dirnameend - dirname
            ldx #.lobyte(dirname)
            ldy #.hibyte(dirname)
            jsr SETNAM
            lda #DIRFILENO
            ldx FA
            ldy #$00
            jsr SETLFS
            jsr OPEN
            bcc :+
            jsr READST
            tax
            lda #diskio::status::GENERIC_KERNAL_ERROR
            sec
            rts

:           ldx #DIRFILENO
            jsr CHKIN

            ; skip header
            ldx #$06
:           jsr CHRIN
            dex
            bne :-
:           jsr CHRIN
            tax
            bne :-

dirloop:    jsr CHRIN; BASIC line link lo
            jsr CHRIN; BASIC line link hi
            jsr CHRIN; file size lo
            jsr CHRIN; file size hi
:           jsr READST
            bne fallback
            jsr CHRIN
            cmp #'"'
            bne :-
            ldx #$00
            jsr CHRIN
:           sta name,x
            inx
            jsr CHRIN
            cmp #'"'
            beq :++
            cpx #nameend - name
            bne :-
:           jsr CHRIN
            cmp #'"'
            bne :-
:           txa
            dec filecount
            bne dirloop

            pha
            lda #DIRFILENO
            jsr CLOSE
            pla

            jsr importlibd

            inc fileindex
            bcs importloop

            ; library successfully imported
            lda #diskio::status::OK
importdone: ldx version + 0
            ldy version + 1
           ;clc
            rts

            ; library import failed, so
            ; import the statically linked
            ; fallback library
fallback:   lda #$00
            sta STATUS

            lda IBASIN + $00
            pha
            lda IBASIN + $01
            pha

.if PLATFORM = diskio::platform::COMMODORE_16
            lda TED_CHARGEN_ADDR
            lsr
            bcc :+
            ; if ROM was enabled, use a getmembyte routine which enables RAM for
            ; fetching bytes, as the statically linked fallback library in the
            ; DISKIO_IMPORT segment might be located in the upper 32K of RAM            
            lda #.lobyte(getmembytx)
            sta IBASIN + $00
            lda #.hibyte(getmembytx)
            sta IBASIN + $01
            lda #.lobyte(fallbackso)
            sta getmembadr + $01
            lda #.hibyte(fallbackso)
            sta getmembadr + $02
            bcs doimport; jmp
:
.endif
            lda #.lobyte(getmembyte)
            sta IBASIN + $00
            lda #.hibyte(getmembyte)
            sta IBASIN + $01
            lda #.lobyte(fallbackso)
            sta getmembyte + $01
            lda #.hibyte(fallbackso)
            sta getmembyte + $02

doimport:   jsr importlib

            sta temp1 + 0
            pla
            sta IBASIN + $01
            pla
            sta IBASIN + $00
            lda temp1 + 0
            bcc :+
            rts
:           lda #diskio::status::DYNLINK_FALLBACK_USED
           ;clc
            jmp importdone

.if PLATFORM = diskio::platform::COMMODORE_16
:           inc getmembadr + $02
            rts
getmembytx: sei
            sta TED_RAM_ENABLE
getmembadr: lda a:$00
            sta TED_ROM_ENABLE
            cli
            inc getmembadr + $01
            beq :-
            rts; one extra byte for one cycle less
.endif
:           inc getmembyte + $02
            rts
getmembyte: lda a:$00
            inc getmembyte + $01
            beq :-
            rts; one extra byte for one cycle less

importlibd: ldx #.lobyte(name)
            ldy #.hibyte(name)
            jsr SETNAM
            lda #LIBFILENO
            ldx FA
            ldy #$02
            jsr SETLFS
            jsr OPEN
            bcc :+
            jsr READST
            tax
            lda #diskio::status::GENERIC_KERNAL_ERROR
            sec
            rts
:           ldx #LIBFILENO
            jsr CHKIN
            jmp :+
            
importlib:  lda #$00
            sta STATUS

:           tsx
            stx hwsp1

            ; compare library name, expected is "Disk I/O Library"
            ldx #$00
:           jsr getbyte
            cmp soname,x
            beq :+
            lda #diskio::status::HEADER_ERROR
            sec
            bcs errorret; jmp
:           cmp #$00
            beq :+
            inx
            bne :--
:
            ; compare version numbers
            jsr getbyte
            ldx #diskio::VERSION_MAJOR
            stx xret
            tay
            cmp #diskio::VERSION_MAJOR
            bne :+
            jsr getbyte
            cmp #diskio::VERSION_MINOR; smaller minor version numbers are ignored since the fallback library is newer
            bcs :++
:           lda #diskio::status::WRONG_VERSION
            sec
            bcs errorret
:
            jsr init_decruncher

            ; the install segment must precede
            ; the resident segment in the library

            lda #diskio::IMPORT::INSTALL_SIZE
            sta mode
            jsr importsegment; import install functions
            stx xret
            cmp #diskio::status::OK + 1
            bcs errorret
            
            jsr get_decrunched_byte; skip load
            jsr get_decrunched_byte; address

            lda #diskio::IMPORT::RESIDENT_SIZE
            sta mode
            jsr importsegment; import resident functions
            stx xret

            cmp #diskio::status::OK + 1
            bcs errorret

            ; library import successful
           ;clc
            php
            pha

            ; store install and resident function block sizes
            lda modctrl + MOD_CTRL::CALLERDATA + 0
            sta sp + 0
            lda modctrl + MOD_CTRL::CALLERDATA + 1
            sta sp + 1
            ldy #diskio::IMPORT::INSTALL_SIZE + 0
            lda instsize + 0
            sta (sp),y
            iny
            lda instsize + 1
            sta (sp),y
            ldy #diskio::IMPORT::RESIDENT_SIZE + 0
            lda resisize + 0
            sta (sp),y
            iny
            lda resisize + 1
            sta (sp),y

            lda #diskio::status::OK
           ;clc
            SKIPWORD
errorret:   php
            pha
            lda #LIBFILENO
            jsr CLOSE
            pla
            ldx xret
            plp
            rts

importsegment:
            lda #$00
            sta parsswitch

            tsx
            stx hwsp0

            lda #.lobyte(modctrl)
            ldx #.hibyte(modctrl)
            jmp _mod_load

_malloc:    sta pointer1 + 0
            stx pointer1 + 1

            inc parsswitch; 0 -> 1

            ; get table size
            jsr get_decrunched_byte
            sta bytesparsd + 0
            jsr get_decrunched_byte
            sta bytesparsd + 1

            ldy #diskio::IMPORT::INSTALL_SIZE
            cpy mode
            bne :+
            ; get version offset
            jsr get_decrunched_byte
            sta version + 0
            jsr get_decrunched_byte
            sta version + 1
:
            ; the requested size is larger than needed
            ; subtract function table size
            sec
            lda pointer1 + 0; malloc size lo
            sbc bytesparsd + 0
            tay
            lda pointer1 + 1; malloc size hi
            sbc bytesparsd + 1
            tax
            tya

            ; add generated jump table size
            ldy #diskio::IMPORT::RESIDENT_SIZE - 1
            cpy mode
            ldy numrqinstf
            bcs :+
            ldy numrqresif
:           sty pointer1 + 0
            ldy #$03; 3 bytes per entry
:           clc
            adc pointer1 + 0
            bcc :+
            inx
:           dey
            bne :--

            ldy #diskio::IMPORT::INSTALL_SIZE
            cpy mode
            bne :+
            sta instsize + 0
            stx instsize + 1
            beq :++
:           sta resisize + 0
            stx resisize + 1
:
            ; for the installer, also add the
            ; space occupied by the temporary
            ; functions import table
            ldy #diskio::IMPORT::INSTALL_SIZE
            cpy mode
            bne :++
            ; 2 bytes per function
            clc
            adc numrqfuncs
            bcc :+
            inx
            clc
:           adc numrqfuncs
            bcc :+
            inx
:
            ldy modctrl + MOD_CTRL::CALLERDATA + 0
            sty pointer1 + 0
            ldy modctrl + MOD_CTRL::CALLERDATA + 1
            sty pointer1 + 1
            ldy mode; diskio::IMPORT::INSTALL_SIZE or diskio::IMPORT::RESIDENT_SIZE
            clc
            sbc (pointer1),y
            txa
            iny
            sbc (pointer1),y
            bcc :+
            ; not enough space
            lda #$00
            tax
            rts

:           ldy mode
            dey
            lda (pointer1),y; diskio::IMPORT::INSTALL_BASE or diskio::IMPORT::RESIDENT_BASE
            tax
            dey
            lda (pointer1),y; diskio::IMPORT::INSTALL_BASE or diskio::IMPORT::RESIDENT_BASE
            ; a/x contains address of allocated chunk
_free:      rts

            ; modload read function, is called repeatedly by _mod_load
readfunc:   sta readsize + 0
            stx readsize + 1

            tsx
            stx hwsp1

            jsr popax
            sta readdest + 0
            stx readdest + 1

            jsr popax; callerdata
            sta pointer1 + 0
            stx pointer1 + 1

            ldx readsize + 0
            lda readsize + 1
            pha
            lda parsswitch
            jeq noparse

            dec parsswitch; 1 -> 0

            ; read and parse the feature/jump table,
            ; compare with what is requested,
            ; and build the generated jump table

            ; calculate generated jump table size
            lda #diskio::IMPORT::RESIDENT_SIZE
            cmp mode
            beq :+; see which jump table's size to calculate
            ; install jump table
            ; jumptablesize = numrqinstf * 3
            lda numrqinstf
            asl
           ;clc
            adc numrqinstf
            bne :++
            ; resident jump table
            ; jumptablesize = numrqresif * 3
:           lda numrqresif
            sta featsmatch
            asl
           ;clc
            adc numrqresif
:           sta temp1 + 0; generated jump table size, maximum 255 bytes

            lda #.hibyte($02); function table size word
            sta bytesparsd + 1
            lda #.lobyte($02)
            ldy #diskio::IMPORT::INSTALL_SIZE
            cpy mode
            bne :+
            lda #.lobyte($04)
:           sta bytesparsd + 0

            ; get the next present dynamically
            ; linkable function
parseloop:  jsr getlistbyt
            cmp #diskio::END_OF_LIST
            jeq endparse; the present function list in the library file
                        ; has only one end-of-list marker

            pha; function id
            jsr getlistbyt; .lobyte(offset)
            sta pointer3 + 0
            jsr getlistbyt; .hibyte(offset)
            sta pointer3 + 1

            ; see if it is a requested function,
            ; check function type first
            pla; function id as given in the library to be linked
            ldy numrqfuncs
            sty temp1 + 1
            SKIPWORD
findfunc:   eor #diskio::FUNCTION_DESIRED
            ldy temp1 + 1
            bne :++

            ; no match, just disregard the function,
            ; it will merely senselessly occupy some memory
:           jsr getlistbyt; skip features
            cmp #diskio::END_OF_LIST
            bne :-
            beq parseloop; jmp

:           pha; function id as given in the library to be linked
            dey
            sty temp1 + 1; flag/offset table position
            tya
            asl
            tay
            clc
            lda (pointer4),y; flag/offset table: .hibyte(offset)
            and #~FUNCTION_IMPORTED
            adc pointer1 + 1; .hibyte(callerdata)
            sta pointer2 + 1; .hibyte(function request)
            lda pointer1 + 0; .lobyte(callerdata)
            sta pointer2 + 0; .lobyte(function request)
            iny
            lda (pointer4),y; flag/offset table: .lobyte(offset)
            tay
            pla; function id as given in the library to be linked
            cmp (pointer2),y; compare with requested function id
            beq funcfound
            eor #diskio::FUNCTION_DESIRED
            cmp (pointer2),y; compare with requested function id
            bne findfunc

            ; the function kind matches,
            ; so now, check features
funcfound:  and #~diskio::FUNCTION_DESIRED
            sta temp2; function type
            sec; skip function type
            tya
            adc pointer2 + 0
            sta pointer2 + 0
            bcc :+
            inc pointer2 + 1
:
            lda pointer4 + 0; flag/offset table
            sta pointer5 + 0
            lda pointer4 + 1; flag/offset table
            sta pointer5 + 1
            lda #$00
            sta ybuf
copyfeats:  lda pointer5 + 0
            dec pointer5 + 0
            tay
            bne :+
            dec pointer5 + 1
:           lda pointer5 + 0
            ldy #diskio::IMPORT::INSTALL_BASE
            cmp (pointer1),y
            iny
            lda pointer5 + 1
            sbc (pointer1),y
            bcs :+
            ; bail out if underflow with regard to instbase,
            ; as the copied feature list is situated at the end
            ; of the install memory region (but before the flag/offset table)
            jmp notenoughm
            
:           jsr getlistbyt
            ldy #$00
            sta (pointer5),y
            cmp #diskio::END_OF_LIST
            beq checkfeats
            inc ybuf
            bne copyfeats; jmp

checkfeats: lda ybuf
            lsr
            bcc chfeatloop
            ; ybuf is not even
            sec
            lda temp2; function type
            sta xret
            lda #diskio::status::INVALID_PARAMETERS
            ldx hwsp0
            txs
            jmp errorret

chfeatloop: ldy ybuf
            sty ybuf2
nextcurrf:  ldy #$00; feature type
            lda (pointer2),y; requested features
            cmp #diskio::END_OF_LIST
            beq featsdone
            ldy ybuf2
findfeat:   cmp (pointer5),y; features of function to be imported
            beq featfound
            dey
            dey
            bne findfeat; at index 0 is the end of list marker

            ; requested feature not found or not present -
            ; bail out
            ldy #$00
            lda (pointer2),y; requested feature
            tay
            ldx temp2; function type
            jmp notsupport

featfound:  dey
            dey
            sty ybuf2
            asl; feature type
            iny
            bcc checkequal
            lda (pointer5),y; factual capability of function to be imported
            ldy #$01
            cmp (pointer2),y; requested capability of function to be imported
            bcc nextcurrf
            bcs capabokay; factual >= requested capability
checkequal: lda (pointer5),y; factual capability of function to be imported
            ldy #$01
            cmp (pointer2),y; requested capability of function to be imported
            bne nextcurrf

capabokay:  ; next requested feature
            clc
            lda #$02
            adc pointer2 + 0
            sta pointer2 + 0
            bcc chfeatloop
            inc pointer2 + 1
            bcs chfeatloop; jmp

            ldy #diskio::IMPORT::INSTALL_SIZE
            cpy mode
            bne :+
            lda instsize + 0
            ldy instsize + 1
            beq :++
:           lda resisize + 0
            ldy resisize + 1
:
            ; all required features are present -
            ; the function is requested, so mark it as found
            ; in the import flag/offset table and update its
            ; corresponding entry in the generated jump table
featsdone:  clc
            lda numrqfuncs
            sbc temp1 + 1; flag/offset table position
            ldy #diskio::IMPORT::RESIDENT_SIZE
            cpy mode
            bne :+
           ;sec
            sbc numrqinstf; is a multiple of 3
:           sta temp2; jump table entry

            ldy temp1 + 1; flag/offset table position
            tya
            asl
            tay
            lda #FUNCTION_IMPORTED
            ora (pointer4),y; flag/offset table
            sta (pointer4),y; flag/offset table

            lda #-1
            cmp pointer3 + 0
            bne :+
            cmp pointer3 + 1
            beq funcimprtd; offset of -1, so don't generate a jump table entry yet

:           lda temp2; jump table entry
            asl
           ;clc
            adc temp2
            tay
            lda #OPC_JMP_ABS
            sta (readdest),y
            iny
            clc
            lda pointer3 + 0; offset lo
            adc temp1 + 0; generated jump table size
            php
            clc
            adc readdest + 0; segment base lo
            sta (readdest),y
            iny
            lda pointer3 + 1; offset hi
            adc readdest + 1; segment base hi
            plp
            adc #$00
            sta (readdest),y

funcimprtd: dec featsmatch
            jmp parseloop

endparse:   lda featsmatch; is 0 when all requested functions have been imported,
            jeq funcscompl; not all segments must have been processed at this point

            ; not all functions were imported,
            ; so see if those not found were merely
            ; desired and not demanded
            ldy numrqfuncs; position in the imported functions flag/offset table
            lda #diskio::IMPORT::INSTALL_SIZE
            cmp mode
            beq chkdesired
            ldy numrqresif; position in the imported functions flag/offset table

chkdesired: sty temp1 + 1; position in the imported functions flag table
            dey
            tya
            asl
            tay
            lda (pointer4),y; imported functions flag/offset table
            cmp #FUNCTION_IMPORTED
            bcs isimported; see if the function is already imported

            ; not, so now check if it was desired only
           ;and #~FUNCTION_IMPORTED
            adc pointer1 + 1; .hibyte(callerdata)
            sta pointer2 + 1
            lda pointer1 + 0; .lobyte(callerdata)
            sta pointer2 + 0
            iny
            lda (pointer4),y; imported functions flag/offset table
            tay
            lda (pointer2),y
            and #diskio::FUNCTION_DESIRED
            bne :+
            ; if the function was demanded, this library import failed
            tax; function type
            ldy featsmatch; amount of missing imports
            jmp notsupport

:           ; otherwise, put lda #diskio::status::NOT_SUPPORTED : rts in place of the
            ; jmp in the jumptable for desired (but not demanded) functions that
            ; could not be imported

            lda temp1 + 1; position in the imported functions flag table
            sec
            sbc #$01
            ldy #diskio::IMPORT::INSTALL_SIZE
            cpy mode
            beq :+
            ; resident routines
            cmp numrqresif
            bcs desirimprt; don't put the code if the install routines are just being imported
           ;clc
            adc numrqinstf
            jmp :++
            ; install routines
:           cmp numrqresif
            bcc desirimprt; don't put the code if the install routines are just being imported
:           eor #$ff
            clc
            adc numrqfuncs
            sta temp2
            asl
            adc temp2
            tay
            lda #OPC_LDA_IMM
            sta (readdest),y
            iny
            lda #diskio::status::NOT_SUPPORTED
            sta (readdest),y
            iny
            lda #OPC_RTS
            sta (readdest),y

desirimprt: dec featsmatch
            beq funcscompl; all requested functions imported for this segment

isimported: ldy temp1 + 1; position in the imported functions flag table
            dey
            bne chkdesired

            ldy featsmatch
            beq funcscompl
            ; should not get here
            ldx #$ff
            jmp notsupport

            ; all functions imported or just desired, so proceed
funcscompl:
            ; subtract number of bytes occupied
            ; by magic number and present functions list;
            ; add size of generated jump table
            clc
            lda temp1 + 0; generated jump table size
            adc Module + 0
            ldy Module + 1
            bcc :+
            iny
:           sec
            sbc bytesparsd + 0; magic word size and present functions list
            sta Module + 0
            tya
            sbc bytesparsd + 1
            sta Module + 1

            clc
            lda temp1 + 0; generated jump table size
            adc TPtr + 0
            ldy TPtr + 1
            bcc :+
            iny
:           sec
            sbc bytesparsd + 0; magic word size and present functions list
            sta TPtr + 0
            tya
            sbc bytesparsd + 1
            tay
            sty TPtr + 1

            ldy #diskio::IMPORT::INSTALL_SIZE
            cpy mode; skip END_OF_LIST mark of the requested install functions list
            bne :+
            clc
            adc numrqfuncs; size of imported functions flag table
            sta TPtr + 0
            bcc :+
            inc TPtr + 1

:           ; for the bytes left to read,
            ; skip the memory area occupied
            ; by the generated jump table
            clc
            lda temp1 + 0; generated jump table size
            adc readdest + 0
            sta readdest + 0
            bcc :+
            inc readdest + 1
:
            ldy #diskio::IMPORT::INSTALL_SIZE
            cpy mode
            bne :+
            ; add segment base to version offset to form version pointer
            clc
            lda readdest + 0
            adc version + 0
            sta version + 0
            lda readdest + 1
            adc version + 1
            sta version + 1
:
            ; from the number of bytes left to read,
            ; subtract number of bytes occupied
            ; by magic word and present features list
            sec
            txa
            sbc bytesparsd + 0
            tax
            pla
            sbc bytesparsd + 1
            pha

noparse:    txa
            bne :+
            pla
            sec
            sbc #$01
            pha
            SKIPWORD
readloop:   tya
            pha
            txa
:           pha
            jsr get_decrunched_byte
            ldy #$00
            sta (readdest),y
            pla
            tax
            pla
            tay

            inc readdest + 0
            bne :+
            inc readdest + 1

:           dex
            bne readloop
            dey
            cpy #-$01
            bne readloop

            lda readsize + 0
            ldx readsize + 1
            rts

notenoughm: lda #diskio::status::MLOAD_ERR_MEM; x and y are undefined upon return
            SKIPWORD
notsupport: lda #diskio::status::NOT_SUPPORTED
            stx notsuppx
            ldx hwsp0
            txs
            ldx notsuppx
            sec
            rts

readerror:  ldx hwsp1
            txs
            lda #$00
            tax
            rts

getlistbyt: inc bytesparsd + 0
            bne :+
            inc bytesparsd + 1
:           stx xbuf
            jsr get_decrunched_byte
            ldx xbuf
            rts

get_crunched_byte:
getbyte:    php; carry flag needs to be preserved for exostreamdecr
            jsr READST
            bne readerror
            jsr CHRIN
            plp; carry flag needs to be preserved for exostreamdecr
            rts

dirname:    .byte '$'
dirnameend:
soname:     .byte DISKIO_SONAME
            .byte $00

.segment "DISKIO_IMPORT"

fallbackso:
.if PLATFORM = diskio::platform::COMMODORE_16
            .incbin "../build/loader-c16.so"
.else
            .incbin "../build/loader-c64.so"
.endif

.segment "BSS"

modctrl:    .tag MOD_CTRL

fileindex:  .res 1
filecount:  .res 1

hwsp0:      .res 1
hwsp1:      .res 1
notsuppx:   .res 1
xbuf:       .res 1
ybuf:       .res 1
ybuf2:      .res 1
xret:       .res 1

parsswitch: .res 1
numrqinstf: .res 1
numrqresif: .res 1
numrqfuncs: .res 1
featsmatch: .res 1
readsize:   .res 2
bytesparsd: .res 2
instsize:   .res 2
resisize:   .res 2
version:    .res 2

name:       .res 16
nameend:
            
            ; using .align $0100 only aligns data relative to their in-segment offset,
            ; which means that the BSS segment itself must also be aligned to page
            ; boundaries in order for the .align directive to work as desired -
            ; this is no option, so alignment is done the wasteful way
            .assert .lobyte(BUFFERSIZE) = 0, error, "***** Error: BUFFERSIZE must be a multiple of $0100. *****"
decompbuffer    = *
            .res BUFFERSIZE + $0100
buffer_start_hi = .hibyte(decompbuffer + $0100)
buffer_len_hi   = .hibyte(BUFFERSIZE)
