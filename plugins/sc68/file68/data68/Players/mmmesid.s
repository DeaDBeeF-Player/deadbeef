;;; sc68 wrapper for MMME+SID original version
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:03:19 ben>
;;; 

	bra	init
	bra	rep+4
	bra.s	rep+8
init:
	lea	rep+$70(pc),a1
	move.l	#$4e714e71,(a1)
rep:	
	incbin	"org/mmmesid"
	even
	dc.l	0
	
	