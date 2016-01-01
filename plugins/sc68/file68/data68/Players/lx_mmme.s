;;; sc68 wrapper Lethal Xcess MMME
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 14:57:37 ben>
;;; 
	bra	init
	bra	RPL+12
	bra.s	RPL+8

; a0 = six
; d1 = 0:STE  1:STF
init:
	lea	RPL(pc),a1
RPL	incbin	"org/lx_mmme"
	even
	dc.l	0
