;;; sc68 wrapper for musmon2
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:04:24 ben>
;;; 

	bra			init
	bra			kill
	move.l	_zic(pc),d0
	beq.s		.ok
	bra			routine+16
.ok:
	rts	

_zic:	dc.l	0
_cpy:	dc.l	0
_114:	dc.l	-1

kill:
	lea			_zic(pc),a6
	move.l	_114(pc),d7
	not.l		d7
	beq.s		.ok
	not.l		d7
	move.l	d7,$114.w
.ok:
	clr.l		(a6)+
	clr.l		(a6)+
	move.l	#-1,(a6)+
	move.l	#$0700FF00,$ffff8800.w	
	move.l	#$08000000,$ffff8800.w	
	move.l	#$09000000,$ffff8800.w	
	move.l	#$0A000000,$ffff8800.w	
	rts

init:
	lea			_zic(pc),a6
	move.l	a0,a1
	add.l		#$20000,a1
	move.l	$114.w,a2
	movem.l	a0-a2,(a6)
	bsr.s		routine
	cmp.l		#-1,a0
	beq.s		.initB
	movem.l	_zic(pc),a0/a1
	bsr.s		routine+8
	move.l	_cpy(pc),a0
	bra.s		routine+4

.initB:
	move.l	_zic(pc),a0
	bra.s			routine+4

routine:
	INCBIN	"org/musmon2"
