;;; sc68 wrapper for Tao's ms25
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:22:21 ben>
;;; 

	bra		init
	bra		player+12
	move	sr,-(a7)	; Player set IPL 7 when writting YM
	bsr.s	player+8	; but forget to restore previous status.
	move	(a7)+,sr	; 
	rts

patch:						; Patch to remove timer A installation.
	sub.l	a0,a0			; TimerA is used to call the replay routine
	nop							; at 200 Hz.
	
init:
	lea			player+$8a9a-$801A(pc),a6
	move.l	patch(pc),(a6)
player:
	incbin	"org/tao_ms25"
