;;; sidsound designer sc68 wrapper
;;; 
;;; by Benjamin Gerard <http://sourceforge.net/users/benjihan>
;;;
;;; Time-stamp: <2014-11-14 18:29:49 ben>
;;; 
;;; sidsound designer original routines by 
;;; synergy and animal mine. patched and updated
;;; by defjam/checkpoint and FroST/Loud.
;;; timer a, c & d are used for the sid-waveform 
;;; emulation. this means you can use 
;;; sid-instruments on all three voices.
;;; ae@atari.org / dhs.atari.org

;;; ----------------------------------------------------------------------
;;; 
;;; SidSoundDesigner file for sc68.
;;; 
;;; One music format:   
;;; ---------------- 
;;; +0            'SSD1'
;;; +4            TVSsz, size of .tvs file
;;; +8            TRIsz, size of .tri file
;;; +12           .tvs file (voice)
;;; +12+TVSsz     .tri file (song)
;;;
;;; Multi music format
;;; ------------------
;;; +0            'SSd1'
;;; +4            number of music
;;; per musics (8 bytes per music):      
;;; +0            offset to TVS from beginning of this table
;;; +4          offset to TRI from beginning of this table
;;; ... data ...
;;;
;;; ----------------------------------------------------------------------
	
begin:
	bra	init		; +0
	bra     driver+8	; +4
	bra	driver+4	; +8
play:	= begin+8

	dc.b	"ssd1 replay for sc68",0
	dc.b	VERSION
	even

init:
	lea	play(pc),a1
	move.w	#$4e75,(a1)	; 'RTS' at play location
	move.l	(a0)+,d7
	cmp.l	#'SSD1',d7
	beq.s	ok_SSD1
	cmp.l	#'SSd1',d7
	beq.s	ok_multi_SDD1
	rts
ok_multi_SDD1:
	move.w	#$6000,(a1)	; 'BRA' at play location
	move	(a0)+,d7	; Get number of music
	subq.w	#1,d0
	divu	d7,d0
	swap	d0		; Ensure track number is in range
	lsl	#3,d0
	move.l	a0,a2
	movem.l	0(a2,d0.w),a0-a1
	add.l	a2,a0
	add.l	a2,a1
        bra     driver
ok_SSD1:
	move.w	#$6000,(a1)	; 'BRA' at play location
	movem.l	(a0)+,a1-a2	; get TVSsz, TRIsz
	add.l	a0,a1		; .tri address (tvs is a0)

	;; Original SSD replay
	;; a0 : sound (.tvs)
	;; a1 : music (.tri)
driver:
	incbin	"ssd.bin"
	even
	dc.b	"1dss"
