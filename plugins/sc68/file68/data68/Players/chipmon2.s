;;; chipmon wrapper for sc68
;;;
;;; by Benjamin Gerard
;;;
;;; Time-stamp: <2011-09-12 14:46:26 ben>
	
	bra	chipmon
	bra	chipmon+8
	bra	chipmon+4
	
chipmon:
	incbin	"org/chipmon2"
