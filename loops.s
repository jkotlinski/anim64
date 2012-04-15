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
.importzp ptr2

.import popax

.export _xor_v2
.export _xor_prev
.export _unpack_colors

; void unpack_colors(char* dst, const unsigned char* src) {
;     char* end = dst + 0x400;
;     while (dst != end) {
;         const unsigned char colors = *src;
;         ++src;
;         *dst = colors;
;         ++dst;
;         *dst = colors >> 4;
;         ++dst;
;     }
; }
dst	= ptr1 ; Borrows cc65 temp pointer.
_unpack_colors:
    sta @src
    stx @src + 1
	jsr popax
    sta dst
    stx dst + 1
    inx
    inx
    inx
    inx
    stx @end + 1
    ldy #0
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
@end:
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

;void xor(char* screen, const char* prev_screen) {
;    unsigned int i;
;    for (i = 0; i < SCREEN_SIZE; ++i) {
;        screen[i] ^= prev_screen[i];
;    }
;}
_xor_v2:
@target = ptr1
@prev_screen = @or + 1
	sta @prev_screen			; save prev_screen arg
	stx @prev_screen + 1
	jsr popax		; get screen arg
	sta @target
	stx @target + 1

    ldy #0
    sty ptr2
    sty ptr2 + 1

@loop:
    lda (@target),y
@or:
    eor $1234
    sta (@target),y

    inc @prev_screen
    bne :+
    inc @prev_screen + 1
:
    inc @target
    bne :+
    inc @target + 1
:

    inc ptr2
    bne :+
    inc ptr2 + 1
:
@screen_size = 1501
    lda ptr2
    cmp #<@screen_size
    bne @loop
    lda ptr2 + 1
    cmp #>@screen_size
    bne @loop

    rts


.segment "EDITCODE"  ; =========================================

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

