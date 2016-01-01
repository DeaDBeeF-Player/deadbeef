;;; sc68 wrapper for musmon1
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:04:02 ben>
;;; 

	bra		init
	bra		kill
play
	lea			routine+$886e-$8060+6(pc),a6
	pea			.ok(pc)
	move.l	(a7)+,(a6)
	bra			routine+($8112-$8038)
.ok:	
	rts

init:
;	lea				mod(pc),a0

	bsr.s			patch
	move.w    #1,-(a7)  ; 1 = Loop-Play Modus
	pea       (a0)			; Song Address
	bsr       routine+6
  addq	    #6,a7
  rts

patch:
	move			#$4e71,d6
	move.w		#($8064-$8042)/2-1,d7
	lea				routine+($8042-$8038)(pc),a6
.clr1:
	move.w	d6,(a6)+
	dbf			d7,.clr1	
	
	move.w		#($810c-$80ee)/2-1,d7
	lea				routine+($80ee-$8038)(pc),a6
.clr2:
	move.w	d6,(a6)+
	dbf			d7,.clr2	
	rts  
kill:  
  clr.w     -(a7)			; 0 = Stop Musik
  clr.l     -(a7)     ; unused
  bsr       routine+6	; Routine nochmal anspringen
  addq	    #6,a7
  rts

routine:
	INCBIN	"org/musmon1"
	
;	EVEN
;mod:
;	INCBIN	"demo.mod"