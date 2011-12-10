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

.export _colcpy_9000
.export _colcpy_9400
.export _colcpy_9800
.export _colcpy_9c00

_colcpy_9000:
    ldy #0
@loop:
    lda $9000, y
    sta $d800, y
    lda $9100, y
    sta $d900, y
    iny
    bne @loop
    ; Since the copy is very slow, we copy upper half first just to make sure it's done
    ; at the time the raster gets there.
@loop2:
    lda $9200, y
    sta $da00, y
    lda $9300, y
    sta $db00, y
    iny
    bne @loop2
    rts

_colcpy_9400:
    ldy #0
@loop:
    lda $9400, y
    sta $d800, y
    lda $9500, y
    sta $d900, y
    iny
    bne @loop
    ; Since the copy is very slow, we copy upper half first just to make sure it's done
    ; at the time the raster gets there.
@loop2:
    lda $9600, y
    sta $da00, y
    lda $9700, y
    sta $db00, y
    iny
    bne @loop2
    rts

_colcpy_9800:
    ldy #0
@loop:
    lda $9800, y
    sta $d800, y
    lda $9900, y
    sta $d900, y
    iny
    bne @loop
    ; Since the copy is very slow, we copy upper half first just to make sure it's done
    ; at the time the raster gets there.
@loop2:
    lda $9a00, y
    sta $da00, y
    lda $9b00, y
    sta $db00, y
    iny
    bne @loop2
    rts

_colcpy_9c00:
    ldy #0
@loop:
    lda $9c00, y
    sta $d800, y
    lda $9d00, y
    sta $d900, y
    iny
    bne @loop
    ; Since the copy is very slow, we copy upper half first just to make sure it's done
    ; at the time the raster gets there.
@loop2:
    lda $9e00, y
    sta $da00, y
    lda $9f00, y
    sta $db00, y
    iny
    bne @loop2
    rts

