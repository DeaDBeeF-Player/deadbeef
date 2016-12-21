;;; sc68 wrapper for Tao's ms211
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:21:21 ben>
;;; 

	bra	init
	bra	player+12
	bra	player+8

init:
	lea	player(pc),a2
	move.w	#$2fff,$3f0(a2)
	move.w	#$21cd,$436(a2)
	sub.l	a2,a0
	move.l	a0,$1854(a2)
	moveq	#1,d0

player:
	incbin	"org/tao_ms211"
	even
	dc.w	0
	
