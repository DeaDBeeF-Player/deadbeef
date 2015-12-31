;;; sc68 wrapper for ninja remix TFMX
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:05:19 ben>
;;; 

; entry:
;  a0 = buffer dc.l spl-coso ... coso ... spl
;

	bra	init
	bra	rep+12
	bsr	rep+4
	lea	rep(pc),a0
	tst.b	$ac0(a0)	; digi on/off
	beq.s	nodigit
	move.l	#$07002400,d0	
	or.w	$c58+$1e(a0),d0
	move.l	d0,$ffff8800.w
nodigit;
	rts

modif:
	move.w	d7,(a6)+	;move.l	dn(pc),a0
	move.l	a2,d7
	sub.l	a6,d7
	move	d7,(a6)
	rts

init:
	move.w	#$207a,d7
	lea	_tspl(pc),a2
	lea	rep+$904(pc),a6
	bsr.s	modif

	move.w	#$227a,d7
	lea	rep+$104(pc),a6
	bsr.s	modif

	move.w	#$207a,d7
	lea	_coso(pc),a2
	lea	rep+$80(pc),a6
	bsr.s	modif

	move.l	a0,a1
	add.l	(a0)+,a1	; a0=coso
				; a1=digit
	move.l	a0,(a2)+
	move.l	a1,(a2)+

	bsr	rep+0		; init co
	bra	rep+8		; save & init irq
				; and samples

_coso:	dc.l	$87654321
_tspl:	dc.l	$12345678

rep:	incbin	"org/ninjatfmx"
	even
