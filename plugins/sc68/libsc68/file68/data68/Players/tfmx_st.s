;;; sc68 wrapper for TFMX ST (Chris Huelsbeck version ?)
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:26:59 ben>
;;; 

	bra	init
	bra	MUS+$28
	bra	MUS+$24

init:
	subq.l	#1,d0
	and.l	#$ff,d0
	move.l	d0,-(a7)
	move.l	a0,d0
	BSR	MUS+$34		;INIT
	move.l	(a7)+,d0
	bra	MUS+$2C		;STARTMUS

MUS:	INCBIN	"org/tfmx_st"
