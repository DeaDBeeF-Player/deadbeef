;;; Reloc code using as68 relocation table.
;;;
;;; by Benjamin Gerard <http://sourceforge.net/users/benjihan>
;;;
;;; Time-stamp: <2011-09-12 01:24:14 ben>
	
; a0: relocation table
; a1: binary
reloc:
	movem.l	d0/d1/a0/a1,-(a7)
	move.l	a1,d1
	sub.l	_reloc(pc),d1	; substract previous location
	beq.s	.done
.loop:	
	move.l  (a0)+,d0	; get offset
	cmp.l   #-1,d0
	beq.s   .finish
	add.l   d1,0(a1,d0.l)	; do relocation
	bra.s   .loop
.finish:
	lea	_reloc(pc),a0	; store new location
	move.l	a1,(a0)
.done:
	movem.l	(a7)+,d0/d1/a0/a1
	rts
_reloc:	
	dc.l	0
