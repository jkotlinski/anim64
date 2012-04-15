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

.export _irq_handler_v2
.export _caught_irqs  ; counter

_caught_irqs: .byte 0

_irq_handler_v2:
    pha
    txa
    pha
    tya
    pha

    inc _caught_irqs

    ; Calls music subroutine.
    lda #0
    tax
    tay
    jsr $1003

    ; #define SCX_REG *(char*)0xd016
    ; SCX_REG = (SCX_REG & ~3) | (*(char*)0xd41c >> 6); 
    lda $d41c
    lsr
    lsr
    lsr
    lsr
    lsr
    lsr
    sta :+ + 1
    lda $d016
    and #~3
:   eor #0
    sta $d016

    ; Flips to UPPERCASE when SID channel 3 has max amplitude.
    lda $d41c
    cmp #$ff
    bne :+
    lda $d018
    ora #2
    sta $d018
    jmp :++
:
    lda $d018
    and #~2
    sta $d018
:

    asl $d019  ; Acknowledges interrupt.

    pla
    tay
    pla
    tax
    pla
    rti
