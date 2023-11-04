;;; crapman 1 wrapper for sc68
;;; 
;;; Time-stamp: <2011-09-12 14:50:11 ben>
;;; 
	bra	init
	bra	crapman1+12
	bra	crapman1+8

init:
	bsr	crapman1
	bra	crapman1+4
	
crapman1:
	incbin	"org/crapman1"
