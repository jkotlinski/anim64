; Copyright (c) 2004, Per Olofsson
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;
; * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
; * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
; FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
; CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
; IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
; OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	.export _rle_pack, rle_pack


	.import rle_store, rle_read
	.importzp src, dest
	.import lastbyte
	.import destlen

	.import popax


	.bss

srclen:		.res 2		; length of source data


	.code


; cc65 interface to rle_pack
; unsigned int __fastcall__ rle_pack(unsigned char *dest, unsigned char *src, unsigned int length);
_rle_pack:
	sta srclen		; save length arg
	stx srclen + 1
	jsr popax		; get src arg
	sta src
	stx src + 1
	jsr popax		; get dest arg
	sta dest
	stx dest + 1
	jsr rle_pack		; execute
	lda destlen		; return length
	ldx destlen + 1
	rts


; run length encode a stream
rle_pack:
	ldy #0
	sty destlen		; reset the byte counter
	sty destlen + 1
	jsr rle_read		; read the first byte
	sta lastbyte		; save for reference
	jsr rle_store		; store it
	jsr @decsrclen		; decrease source count
	beq @end		; if you're trying to pack a single byte, this the end
@pack:
	jsr rle_read		; grab a byte
	cmp lastbyte		; same as last byte?
	beq @rle		; then count bytes and store run length
	sta lastbyte		; save for reference
	jsr rle_store		; store byte
	jsr @decsrclen		; decrease source count
	bne @pack		; next
@end:
	lda lastbyte		; store last byte...
	jsr rle_store
	lda #0			; ...with a 0 count as the terminator
	jsr rle_store
	rts			; done
@rle:
	ldx #1			; start with a count of 1
	jsr @decsrclen
	beq @rleend
@rlenext:
	jsr rle_read		; grab a byte
	cmp lastbyte		; make sure it's the same
	bne @newbyte		; no, then terminate
	inx 			; inc counter
	beq @stop		; overflow?
	jsr @decsrclen		; check for end of data
	bne @rlenext
@rleend:			; end of data
	lda lastbyte		; store double byte
	jsr rle_store
	txa			; and counter
	jsr rle_store
	jmp @end
@stop:	    			; overflow
	lda lastbyte		; store the double byte
	jsr rle_store
	lda #$ff		; $ff as the byte count
	jsr rle_store
	inx			; start over with a counter of 1
	jsr @decsrclen
	beq @rleend
	bne @rlenext
@newbyte:			; new byte detected
	pha			; save
	lda lastbyte		; store double byte
	jsr rle_store
	txa			; and counter
	jsr rle_store
	pla			; restore new byte
	sta lastbyte		; save for reference
	jsr rle_store		; store it
	jsr @decsrclen		; data left?
	bne @pack		; yep, pack
	beq @end		; nope, end
; decrease number of bytes left, return 0 when done
@decsrclen:
	lda srclen
	bne :+
	dec srclen + 1
:	sec
	sbc #1
	sta srclen
	ora srclen + 1
	rts
