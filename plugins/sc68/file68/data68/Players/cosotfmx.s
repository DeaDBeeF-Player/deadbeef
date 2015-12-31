;;; COSO - TFMX original - for sc68
;;; 
	bra	init
	bra	rep+4
	bra.s	rep+8
init:
	lea	rep+$68(pc),a1
	move.l	#$4e714e71,(a1)
rep:	
	incbin	"org/cosotfmx"
	even
	dc.l	0
