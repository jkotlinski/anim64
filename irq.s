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

; Defined in colcpy.s.
.import _colcpy_9000
.import _colcpy_9400
.import _colcpy_9800
.import _colcpy_9c00

_anim_screen:
    .byte 0

_ticks_per_frame:
    .byte 0

frame_delay:
    .byte 0

_caught_irqs:
    .byte 0

color_copy:
    lda _anim_screen
    bne :+
    jmp _colcpy_9000
:   cmp #1
    bne :+
    jmp _colcpy_9400
:   cmp #2
    bne :+
    jmp _colcpy_9800
:   jmp _colcpy_9c00

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
    clc  ; += 83
    adc #$83
    sta :+ + 2
    sta :++ + 2
:   lda $83e8
    sta $d020
:   lda $83e9
    sta $d021

    jsr color_copy

    ; anim_screen = (anim_screen + 1) & 3;
    ldx _anim_screen
    inx
    txa
    and #3
    sta _anim_screen

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
