;
; SIDSOUND DESIGNER (c) '93 by ENIM LAMINA/THE INDEPENDENT
;
;

; a0 = address of TSSS, TSST buffer
start:
	bra	init
	rts
	rts
	bra	replay+8

init:
;	lea	song(pc),a0
	
;	lea	replay(pc),a5
;	or.w	#64,$10(a5)
	
	move.l	a0,a5
	move.l	a0,a6
	add.l	#128*1024,a6
	move.l	#"TSST",d0
	bsr.s	finstr
	bne.s	notfound
	move.l	a5,a1

	exg.l	a0,a1
	bsr	replay	; reloc
	
	clr.b	$fffffa19.w
	clr.b	$fffffa1b.w
	clr.b	$fffffa1d.w
	
	rts

notfound:
	lea	start+8(pc),a0
	move.w	#$4e75,(a0)
	rts

; a5 = start
; a6 = end
; d0 = value
; => a5 = found addr , Z flag if found

finstr:
	cmp.l	(a5),d0
	beq.s	.ok
	addq	#2,a5
	cmp.l	a5,a6
	bhi.s	finstr
	move.l	d0,d0
	rts
.ok:
	cmp	d0,d0
	rts

replay:
	incbin	"org/sidsound"
	even
	dc.w	0

;song:	incbin	"dba*"
