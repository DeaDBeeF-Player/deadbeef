;;; sc68 wrapper for Quartet (original singsong.prg)
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:39:02 ben>
;;; 

TEST=0

	bra	init
	bra kill
play:
	tst.l		$114.w
	beq.s		.rts
	move.l	_quartet_prg(pc),a6
	move.b	#$3b,$85ee-$8570(a6)	; ??? keyboard 3b/3c : replay mode YM/8 bit port
					; OR cut sound ?
	move.w	#$2500,sr
	move.l	$114.w,a6
	pea			.return(pc)
	move		#$2500,-(a7)	; RTE
	jmp			(a6)
.return:
.rts:
	rts	; Nothing to do : quartet replay used internal timer
	
kill:
	move.l	_quartet_prg(pc),d0
	beq.s		.rts
	move.l	d0,a6
	lea			$1c(a6),a6
	tst.l		12(a6)
	beq.s		.rts
	jsr			8(a6)
	
	move.l	_quartet_prg(pc),a6
	lea			$1c(a6),a6
	clr.l		12(a6)	; song ptr
	clr.l		16(a6)	; voice set ptr
.rts:	
	rts
	
init:	;
	move	#$2300,sr

	if TEST
	{	
	lea		quartet_file(pc),a0
	}

	move.w	reloc_flag(pc),d7
	bne.s		.ok_reloc
	
	movem.l	d0-a6,-(a7)
	lea			quartet_off(pc),a0
	move.l	a0,a1
	add.l		(a1),a0
	move.l	a0,_quartet_prg-quartet_off(a1)
	
	bsr			load_prg
	movem.l	(a7)+,d0-a6
	
.ok_reloc:
	move.l	_quartet_prg(pc),a6
	lea			$1c(a6),a6
	clr.l		12(a6)	; song ptr
	clr.l		16(a6)	; voice set ptr

	
	movem.l	(a0),d5-d7

; 00 'QUAR'
; 04 offset from start to voice-set
; 08 number of songs
; 0C 1st song, offset from start
; 10 2nd song, ...
	
	cmp.l		#'QUAR',d5	;[ QUAR, Offset to voice set, max-song]
	bne.s		.rts
	subq.l	#1,d0
	divu		d7,d0
	swap		d0
	lsl			#2,d0
	move.l	$0c(a0,d0.w),d7
	
	add.l		a0,d6
	add.l		a0,d7
	
	move.l	d7,12(a6)	; song ptr
	move.l	d6,16(a6)	; voice set ptr
	jsr			4(a6)
	
;/*               Vector cti cpp rem tdr  tcr  level  bit    chan */
;/* Timer A */  { 0x34,  0,  0,  0,  256, 0,    6,    1<<5,  0 },
;/* Timer B */  { 0x20,  0,  0,  0,  256, 0,    6,    1<<0,  0 },
;/* Timer C */  { 0x14,  0,  0,  0,  256, 0,    6,    1<<5,  2 },
;/* Timer D */  { 0x10,  0,  0,  0,  256, 0,    6,    1<<4,  2 },

	moveq		#$0f,d7
	and.b		$fffffa1d.w,d7
	move.b	d7,$fffffa1d.w			; stop timer C
	bclr	#5,$fffffa09.w
	bclr	#5,$fffffa15.w
	move.l  #$0707FFFF,$ffff8800.w  ; Cut sound (quartet did not !)

	IF 0
	{
		move	sr,-(a7)
		move.l	d7,-(a7)
		
		move	#$2700,sr
		bclr	#5,$fffffa09.w
		bclr	#5,$fffffa15.w
		
		moveq		#$0f,d7
		and.b		$fffffa1d.w,d7
		move.b	d7,$fffffa1d.w			; stop timer C
		move.b	#244,$fffffa23.w		; Go at 200 hz (original ST frq)
		or.b		#$40,d7
		move.b	d7,$fffffa1d.w	; start timer C
		
		bset	#5,$fffffa09.w
		bset	#5,$fffffa15.w
		
		move.l	(a7)+,d7
		move		(a7)+,sr
	}
.rts:	
	rts
	
reloc_flag:		dc.w	0
quartet_off:	dc.l	quartet_prg-quartet_off
_quartet_prg:	dc.l	0

	INCLUDE	"lib/TosReloc.s"
	
	
	if TEST
	{
quartet_file:
	dc.l	'QUAR'

	dc.l	voiceset-quartet_file
	dc.l	song1-quartet_file
	ds.w	256
voiceset:
	incbin	"VOICE.SET"	
	even
	ds.b	512
song1:	
	incbin	"DEMO2.4V"
	even
	}

	ds.w	512
quartet_prg:
	incbin	"org/singsong.prg"
	even
	ds.w	512
