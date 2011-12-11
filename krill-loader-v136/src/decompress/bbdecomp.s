; ByteBoozer Decruncher    /HCL may.2003
; with slight modifications by Krill

zp_base     = DECOMPVARS

;variables..         #bytes

put         = decdestlo

cpl         = zp_base+0 ;1
cur         = zp_base+1 ;1
get         = zp_base+2 ;2
cps         = zp_base+4 ;2


.macro SETDECOMPGETBYTE
            sta toloadb0 + $01
            sty toloadb0 + $02
            sta toloadb1 + $01
            sty toloadb1 + $02
            sta toloadb2 + $01
            sty toloadb2 + $02
            sta toloadb3 + $01
            sty toloadb3 + $02
            sta toloadb4 + $01
            sty toloadb4 + $02
.endmacro

decompress:
            CHUNKENTRY
toloadb0:   jsr getcmem
            sta cur
toloadb1:   jsr getcmem
storedadrl: sta put
toloadb2:   jsr getcmem
storedadrh: sta put+1
            CHUNKSETUP
d_loop:
            jsr d_get
dl_1:
            php
            lda #1
dl_2:
            jsr d_get
            bcc dl_2e
            jsr d_get
            rol a
            bpl dl_2
dl_2e:
            plp
            bcs d_copy

d_plain:
            sta cpl

            ; literal run
            ldy #0
toloadb3:   jsr getcmem
            sta (put),y
            iny
            cpy cpl
            bne toloadb3

            ldx #get
            jsr d_add
            ldx #put
            jsr d_add
            CHUNKCHECK
            iny
            beq d_loop
            sec
            bcs dl_1

d_copy:
            adc #0
            beq d_end
            sta cpl
            cmp #3

            lda #0
            sta cps
            sta cps+1

            rol a
            jsr d_get
            rol a
            jsr d_get
            rol a
            tax
dc_1s:
            ldy depacktab,x
dc_1:
            jsr d_get
            rol cps
            rol cps+1
            dey
            bne dc_1
            txa
            dex
            and #3
            beq dc_1e
            inc cps
            bne dc_1s
            inc cps+1
            bne dc_1s
dc_1e:

            ; copy sequence
            sec
            lda put
            sbc cps
            sta cps
            lda put+1
            sbc cps+1
            sta cps+1

            lda (cps),y
            sta (put),y
            iny
            cpy cpl
            bne *-7

            ldx #put
            jsr d_add
            CHUNKCHECK
            jmp d_loop


d_get:
            asl cur
            bne dg_end
            pha
toloadb4:   jsr getcmem
            sec
            rol a
            sta cur
            pla
dg_end:
            rts

d_add:
            clc
            tya
            adc $00,x
            sta $00,x
            bcc :+
            inc $01,x
:
            ; decompression finished
.if GETCHUNK_API
            rts
d_end:      CHUNKEOF
            rts
.else
d_end:      rts
.endif

depacktab:  .byte 4,2,2,2,5,2,2,3
