;;; digitfmx wrapper for sc68
;;; 
;;; Time-stamp: <2011-09-12 14:51:38 ben>
;;; 
	bra	init
	bra	replay+12
	bra.s	replay

init:
	moveq	#1,d0
	bsr.s	replay+4
	bra.s	replay+8
replay:
	incbin	"org/digitfmx"
	
