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

.importzp ptr1

.export _xor_prev
.export _copy_colors_to_d800

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

    ; prev_ptr = screen_ptr - 0x400
    ; also, keep screen_ptr MSB in x
    txa
    dex
    dex
    dex
    dex
    stx or + 2
    tax 

loop:
rd: lda $8400  ; a = *screen_ptr
or: eor $8000  ; a ^= *prev_ptr
wr: sta $8400  ; *screen_ptr = a

    inc rd + 1  ; ++screen_ptr
    inc wr + 1
    inc or + 1  ; ++prev_ptr
    bne loop
    inc rd + 2
    inc wr + 2
    inc or + 2
    txa
    cmp or + 2  ; Has prev_ptr reached original screen_ptr?
    bne loop
    rts         ; Yes - done!

; void copy_colors_to_d800(const unsigned char* src) {
;     unsigned char* dst = (unsigned char*)0xd800;
;     while (dst != (unsigned char*)(0xd800 + 40 * 25)) {
;         const unsigned char colors = *src;
;         ++src;
;         *dst = colors;
;         ++dst;
;         *dst = colors >> 4;
;         ++dst;
;     }
; }
dst	= ptr1 ; Borrows cc65 temp pointer.
_copy_colors_to_d800:
    sta @src
    stx @src + 1
    ; dst = 0xd800;
    ldy #0
    sty dst
    lda #$d8
    sta dst + 1
@loop:
@src = @loop + 1
    ; *dst = *src;
    lda $1234  ; src
    sta (dst),y
    ; *++dst = *src >> 4;
    lsr a
    lsr a
    lsr a
    lsr a
    inc dst
    sta (dst),y

    ; ++dst;
    inc dst
    bne :+
    inc dst + 1
    ; if (dst == 0xdc00) then return;
    lda dst + 1
    cmp #$dc
    bne :+
    rts 
:
    ; ++src;
    inc @src
    bne :+
    inc @src + 1
:
    jmp @loop
