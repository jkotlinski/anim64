;
; Marc 'BlackJack' Rintsch, 18.03.2001
;
; void __fastcall__ cbm_close (unsigned char lfn);
;

        .import _cbm_k_close
        .export _mycbm_close

.segment "EDITCODE"  ; =========================================

_mycbm_close = _cbm_k_close
