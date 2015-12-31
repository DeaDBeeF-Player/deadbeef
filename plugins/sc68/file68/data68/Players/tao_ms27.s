;;; sc68 wrapper for Tao's ms27
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:22:55 ben>
;;; 

	bra	init
	bra	kill
	bra	player+4
patch:
	move.l	a0,a6
	nop	
kill:	
	moveq	#0,d0
init:
	lea			player+$94(pc),a6
	move.l	patch(pc),(a6)
player:
	INCBIN	"org/tao_ms27"
	
	