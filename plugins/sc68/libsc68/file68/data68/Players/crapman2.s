;;; crapman 2 wrapper for sc68
;;; 
;;; Time-stamp: <2011-09-12 14:50:41 ben>
;;; 
	bra	init
	bra	crapman2+12
	bra	crapman2+8

init:
	bsr	crapman2
	bra	crapman2+4
	
crapman2:
	incbin	"org/crapman2"
