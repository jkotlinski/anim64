; Copyright (c) 2011, Johan Kotlinski
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.

.export _irq_handler
.export _caught_irqs  ; counter
.export _ticks_per_frame
.export _anim_screen
.export _screen_ptr

_anim_screen: .byte 0
_ticks_per_frame: .byte 0
frame_delay: .byte 0
_caught_irqs: .byte 0
_screen_ptr: .byte 0

anim_next_screen:
    ; *(char*)0xd018 = 4 | (anim_screen << 4);  // Point video to 0x8000.
    lda _anim_screen
    asl
    asl
    asl
    asl
    ora #4
    sta $d018

    ; *(char*)0xd020 = base[BORDER_OFFSET];
    ; *(char*)0xd021 = base[BG_OFFSET];
    lda _anim_screen
    asl  ; *= $4
    asl
    adc _screen_ptr
    adc #3
    sta :+ + 2
    sta :++ + 2
:   lda $83e8
    sta $d020
:   lda $83e9
    sta $d021

    ; --------- Color copy - setup.
    lda _anim_screen
    asl  ; *= 4
    asl
    adc _screen_ptr
    adc #$10
    tax
    stx @colcpy_d8_src + 2
    inx
    stx @colcpy_d9_src + 2
    inx
    stx @colcpy_da_src + 2
    inx
    stx @colcpy_db_src + 2

    ; Color copy - main loop.
    ldy #0
@loop:
@colcpy_d8_src:
    lda $9000, y
    sta $d800, y
@colcpy_d9_src:
    lda $9100, y
    sta $d900, y
    iny
    bne @loop
    ; Since the copy is very slow, we copy upper half first just to make sure it's done
    ; at the time the raster gets there.
@loop2:
@colcpy_da_src:
    lda $9200, y
    sta $da00, y
@colcpy_db_src:
    lda $9300, y
    sta $db00, y
    iny
    bne @loop2
    ; ------------ Color copy - done!

    ; if (anim_screen & 3 == 3) {
    ;   anim_screen &= ~3;
    ; } else {
    ;   ++anim_screen;
    ; }
    lda _anim_screen
    and #3
    cmp #3
    bne @inc_anim
    lda _anim_screen
    and #~3
    sta _anim_screen
    rts
@inc_anim:
    inc _anim_screen
    rts

_irq_handler:
    pha
    txa
    pha
    tya
    pha

    ldx frame_delay
    bne @wait
    jsr anim_next_screen
    ldx _ticks_per_frame
    jmp @done
@wait:
    dex
@done:
    stx frame_delay

    ; Calls music subroutine.
    lda #0
    tax
    tay
    jsr $1003

    inc _caught_irqs

    asl $d019  ; Acknowledges interrupt.

    pla
    tay
    pla
    tax
    pla
    rti
