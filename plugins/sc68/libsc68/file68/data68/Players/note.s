;;; sc68 wrapper for Hippel Fullscreen Replay
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:06:47 ben>
;;; 

				;
; 8 bytes / frame
;

	bra	init
	rts
	rts

;------------------------
	lea	_song(pc),a1
	move.l	(a1),a0
read:
	move.l	(a0)+,d0
	move.l	(a0)+,d1
	move.l	d0,d2
	or.l	d1,d2
	bne.s	cont
	move.l	_resong(pc),a0
	bra.s	read
cont:
	move.l	a0,(a1)

; format is VAAA VBBB VCCC NN MI
	lea	$ffff8800.w,a0
	lea	$ffff8802.w,a1

	moveq	#1,d2
	bsr.s	send
	swap	d0
	moveq	#0,d2
	bsr.s	send

	move.b	#6,(a0)
	move.w	d1,(a1)
	move.b	#7,(a0)
	move.b	d1,(a1)

	move.l	d1,d0
	swap	d0
	moveq	#2,d2

send:
	add	d2,d2
	move.b	d2,(a0)
	move.b	d0,(a1)
	addq	#1,d2
	move.b	d2,(a0)
	move.w	d0,(a1)

	lsr	#1,d2
	addq	#8,d2
	lsr	#4,d0
	move.b	d2,(a0)
	move.w	d0,(a1)
	rts

_song:	dc.l	0
_resong	dc.l	0

init:	lea	_song(pc),a1
	move.l	a0,(a1)+
	move.l	a0,(a1)+
	rts
