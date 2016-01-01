;;; sc68 wrapper for timer-C replay (emulate timer C replay)
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:28:22 ben>
;;; 

	
;	bra	init
;	bra	m+4
;	bra	m+8
;init:
;	bsr	m
;	move	#$2700,sr
;	bclr	#5,$fffffa09.w
;	bclr	#5,$fffffa15.w
	
;	moveq		#$0f,d7
;	and.b		$fffffa1d.w,d7
;	move.b	d7,$fffffa1d.w			; stop timer C
;	move.b	#245,$fffffa23.w		; Go at 200 hz (original ST frq)
;	or.b		#$40,d7
;	move.b	d7,$fffffa1d.w	; start timer C
	
;	bset	#5,$fffffa09.w
;	bset	#5,$fffffa15.w
;	move	(a7)+,sr
;	rts
		
	bra	init
	bra	m+4
	bclr	#5,$fffffa09.w
	bclr	#5,$fffffa15.w
	move.l	$114.w,d7
	beq.s	.notimer
	pea	.ok(pc)
	move	sr,-(a7)
	move.l	d7,a6
	move	#$2600,sr
	jmp	(a6)	
.ok
	rts	

.notimer:
	bra	m+8
	
init:
;	move.l	#$40000,d1	
m:
