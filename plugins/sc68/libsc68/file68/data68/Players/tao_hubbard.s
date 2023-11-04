;;; sc68 wrapper for Tao's hubbard player
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:20:50 ben>
;;; 

	bra	init
	bra	player+12
	bra	player+8

init:
	lea	player(pc),a2
	move.l	a0,$dd2(a2)
	move.l	#$4e714e71,$18(a2)
	subq	#1,d0
player:
	incbin	"org/tao_hubbard"
	even
	dc.w	0
	
