;;; sc68 wrapper for turrican2's special TFMX
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:31:24 ben>
;;; 
	
; a0 = coso-tfmx file 
; d0 = six in file 
; d1 = 0:STE 1:STF 
; d1(org) :1:STF, 2:EXT, 3:STE

	bra	init
kill:	bra	replay+$b04-$56
play:	bra	replay+$270-$56

init:
	subq	#1,d1
	sne	d1
	and	#2,d1
	addq	#1,d1
	
	movem.l	d0/a0,-(a7)
	move.l	d1,d0
	bsr	replay+$a20-$56
	movem.l	(a7)+,d0/a0
	bra	replay+$1de-$56

replay:	incbin	"org/tur2tfmx"
