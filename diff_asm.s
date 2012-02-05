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

.export _xor_prev

; void xor_prev(unsigned char* screen_ptr) {
;    unsigned int offset = 0;
;    unsigned char* prev_ptr = screen_ptr - 0x400;
;    while (offset < 40 * 25) {
;        *screen_ptr++ ^= *prev_ptr++;
;        ++offset;
;    }
; }
_xor_prev:
    sta rd + 1
    sta wr + 1
    sta or + 1
    stx rd + 2
    stx wr + 2

    txa
    dex
    dex
    dex
    dex
    stx or + 2
    tax

loop:
rd: lda $8400
or: eor $8000
wr: sta $8400

    inc rd + 1
    inc wr + 1
    inc or + 1
    bne loop
    inc rd + 2
    inc wr + 2
    inc or + 2
    txa
    cmp or + 2
    bne loop
    rts
