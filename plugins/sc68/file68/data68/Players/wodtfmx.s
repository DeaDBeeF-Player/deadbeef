;;; sc68 wrapper for wings of Death special TFMX
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:32:06 ben>
;;; 

; a0 = coso tfmx
; d0 = song
; d1 = 0:ste 1:stf

	bra	init
	bra	rep+$b56
	bra	rep+$2cc

init:
	movem.l	d0-a6,-(a7)
	bsr	rep+$23e
	movem.l	(a7)+,d0-a6
	tst.b	d1
	seq	d0
	and	#2,d0
	addq	#1,d0
	bra	rep+$a8a

rep:
	incbin	"org/wodtfmx"
	even
	dc.w	0
