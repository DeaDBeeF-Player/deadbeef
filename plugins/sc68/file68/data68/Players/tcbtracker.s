;;; sc68 wrapper for The CareBear's TCB tracker
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:24:29 ben>
;;; 

	bra		init
	bra		kill
	bra		play

save_ints:
	move.l	a0,-(a7)
	lea			mfpa07(pc),a0
	move.b	$fffffa07.w,(a0)+
	move.b	$fffffa09.w,(a0)+
	move.b	$fffffa13.w,(a0)+
	move.b	$fffffa15.w,(a0)+
	move.b	$fffffa19.w,(a0)+
	move.b	$fffffa1f.w,(a0)+
	move.l	(a7)+,a0
	rts

; a0 = module 
; 
init_ints:
	movem.l	d0-a6,-(a7)

	clr.b	$fffffa09.w
	clr.b	$fffffa15.w
	
	move.w	$90(a0),d0
	move.b	mfp_freqs(pc,d0.w),$fffffa1f.w
	move.b	#2,$fffffa19.w
	move.b	#32,$fffffa07.w
	move.b	#32,$fffffa13.w
	bclr		#3,$fffffa17.w
	
	lea			replay_rot(pc),a1
	add.l		#$8918,a1
	move.l	a1,$134.w	
	
	movem.l	(a7)+,d0-a6
	rts
	
mfp_freqs:
	dc.b	24			;10 and 8.3 Khz
	dc.b	29
	
ret_ints:	
	lea			mfpa07(pc),a0
	move.b	(a0)+,$fffffa07.w
	move.b	(a0)+,$fffffa09.w
	move.b	(a0)+,$fffffa13.w
	move.b	(a0)+,$fffffa15.w
	move.b	(a0)+,$fffffa19.w
	move.b	(a0)+,$fffffa1f.w
	clr.l		$134.w
	rts
	
mfpa07:	dc.b	0
mfpa09:	dc.b	0
mfpa13:	dc.b	0
mfpa15:	dc.b	0
mfpa19:	dc.b	0
mfpa1f:	dc.b	0

_module:		dc.l	0
;module_off:	dc.l	module-module_off

init:
;	lea		module_off(pc),a0
;	add.l	(a0),a0
;	lea		replay_rot(pc),a2

	lea			_module(pc),a6
	move.l	a0,(a6)					; save module addr
	moveq		#-1,d0					; -1=the default soundtable 0=the other
	bsr			replay_rot+$24	; init. replay routine
;	bsr			replay_rot+$20	; init. replay routine
	
	; don't use d6-d7/a2-a6 from here
	bsr	save_ints
	move.l	_module(pc),a0
	bsr	init_ints
	rts
	
play:
	movem.l	d0-d5/a0-a1,-(a7)		;save registers
	move.l	_module(pc),d0
	beq.s		.noplay
	bsr			replay_rot+$28			;call replay routine
.noplay	:
	movem.l	(a7)+,d0-d5/a0-a1		;pop registers
	rts
	
kill:
	bsr				ret_ints
	rts
	
replay_rot:				;the replay routine
	incbin	"org/TCBtracker.rot"
;	incbin	"tracker.rot"
;	incbin	"trackere.rot"
	even
	
;module:					;the module
;	incbin	"../TCBmodules/brain.tcb"
;	incbin	"../TCBmodules/TCB1/phenomen.tcb"
	even