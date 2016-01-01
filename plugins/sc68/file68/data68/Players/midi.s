;;; sc68 wrapper for midi replay (from cartoon)
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:01:26 ben>
;;; 

; sc68 'MIDI' file format 
;
; 00 'MIDI'
; 04 number of songs
; 08 1st track : +00 offset from start to 1st voice-set
;                +04 offset from start to 1st midi file
; 10 2nd track ...

	bra		init
	bra		kill
play:	
	rts
	
init:
	cmp.l		#'MIDI',(a0)
	beq.s		.okMidi
	rts
	
.okMidi:
	move.l	a0,a2
	move.l	4(a2),d1
	subq.l	#1,d0
	divu		d1,d0
	swap 		d0
	lsl			#3,d0
	lea			8(a2,d0.w),a1
	move.l	(a1)+,a0
	move.l	(a1),a1
	add.l		a2,a0
	add.l		a2,a1

; Push for PlayMusic command
	move.w	#1,-(a7)				;NbreRepeat.w
	move.l	a0,-(a7)				;InstrumentsFile.l
	move.l	a1,-(a7)				;MidiFile.l

	bsr			MidiManager+28	;InitSoundCard
	move.w	#31,-(a7)				;Volume.w (0-31)
	bsr			Expander+32		;MainVolum
	addq.l	#2,a7
	
	bsr			MidiManager+36	;PlayMusic
	lea			10(a7),a7
	rts
	
kill:
	bsr			MidiManager+40	;EndMusic:	macro
	rts

MidiManager:
	incbin	"org/midi_seqprg.bin"
	even
Expander:
	incbin	"org/midi_exp_sp4e.bin"
	even
	
	