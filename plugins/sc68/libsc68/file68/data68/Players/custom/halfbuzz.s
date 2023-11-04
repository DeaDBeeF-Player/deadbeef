;;; half buzzer test
;;;
;;; $Id$
;;; 
	bra	init
	bra	shutdown
	bra	play

init:
	lea	$ffff8800.w,a0

	move.l	#$0700FF00,(a0)	;cut noise and tone all chans
	
	move.l	#$08001000,(a0)	;vol A (control by envelop)
	move.l	#$09000000,(a0)	;vol B
	move.l	#$0A000000,(a0)	;vol C
	
	move.l	#$00000000,(a0)	;per A
	move.l	#$01000000,(a0)
	move.l	#$02000000,(a0)	;per B
	move.l	#$03000000,(a0)
	move.l	#$04000000,(a0)	;per C
	move.l	#$05000000,(a0)

	move.l	#$0B003000,(a0)
	move.l	#$0C000000,(a0)
	
	move.l	#$0D000A00,(a0)

	lea	counter(pc),a0
	move	#1,(a0)
shutdown:
	rts

play:
	lea	counter(pc),a0
	subq	#1,(a0)+
	bne.s	.finish
	move	(a0),-(a0)
	lea	$ffff8800.w,a0

	move.b	#7,(a0)
	move.b	(a0),d0
	bchg	#0,d0		; toggle tone
	move.b	d0,2(a0)
.finish:
	rts
	
counter:
	dc.w	1		; count-down
	dc.w	100		; reset value