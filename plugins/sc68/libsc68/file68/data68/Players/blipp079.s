;;; blippblop079 sc68 wrapper
;;; 
;;; by Benjamin Gerard
;;;
;;; Play a .TUN file [fourCC = 'Elof']
;;;
;;; Time-stamp: <2011-09-12 14:44:54 ben>
	
	bra	m+0
	rts
	rts
	bra	m+16

m:
	incbin	"org/blipp079.bin"
tune:
	; Tune must be located here !!
