;**********************
; The SoundMon Player Routine V2.2 (X)1991
; Modules in CHIP mem!
;
; SC68 version by Benjamin Gerard
;
; IN: a0 = bp file addresss
;
	include "lib/org.s"

	bra	init
	rts
	rts
	bra	bpmusic

_bpsong: dc.l	$12345678

init:
	lea			_bpsong(pc),a1
	move.l	a0,(a1)
	move.l	a0,a1
	
bpinit:
 lea		samples(pc),a0
; lea		bpsong,a1
 clr.b		numtables
 cmp.w		#'V.',26(a1)
 bne.s		bpnotv2
 cmp.b		#'3',28(a1)
 bne.s		bpnotv2
 move.b		29(a1),numtables
bpnotv2:
 move.l		#512,d0
 move.w		30(a1),d1	;d1 now contains length in steps
 move.l		#1,d2		;1 is highest pattern number
 lsl.w		#2,d1		;x4
 subq.w		#1,d1		;correction for DBRA
 and.l		#$ffff,d1
findhighest:
 cmp.w		(a1,d0),d2	;Is it higher
 bge.s		nothigher	;No
 move.w		(a1,d0),d2	;Yes, so let D2 be highest
nothigher:
 addq.l		#4,d0		;Next Voice
 dbra		d1,findhighest	;And search
 move.w		30(a1),d1
 lsl.w		#4,d1		;x16
 and.l		#$ffff,d1
 move.l		#512,d0		;header is 512 bytes
 mulu		#48,d2		;48 bytes per pattern
 add.l		d2,d0
 add.l		d1,d0		;offset for samples

; add.l		#bpsong,d0
 add.l		_bpsong(pc),d0
 move.l		d0,tables
 moveq		#0,d1
 move.b		numtables,d1	;Number of tables
 lsl.l		#6,d1		;x 64
 add.l		d1,d0
 moveq		#14,d1		;15 samples
 lea		32(a1),a1
initloop:
 move.l		d0,(a0)+
 cmp.b		#$ff,(a1)
 beq.s		bpissynth
 move.w		24(a1),d2
 add.w		d2,d2		;x2 is length in bytes
 and.l		#$ffff,d2
 add.l		d2,d0		;offset next sample
bpissynth:
 lea		32(a1),a1	;Length of Sample Part in header
 dbra		d1,initloop
 rts

bpmusic:
 subq.b		#1,arpcount
 and.b		#3,arpcount
 moveq		#3,d0
 lea		bpcurrent(pc),a0
 lea		$dff0a0,a1
 addq.l		#2,lfopointer
 cmp.l		#lfotable+16,lfopointer
 bne.s		bpnotlfotend
 move.l		#lfotable,lfopointer
bpnotlfotend:
 move.l		lfopointer(pc),a5
bploop1:
 move.b		12(a0),d4
 ext.w		d4
 add.w		d4,(a0)
 tst.b		34(a0)
 beq.s		bpnovibr
 moveq		#0,d4
 move.b		34(a0),d4
 move.w		(a5),d5
 ext.l		d5
 divs		d4,d5
 add.w		(a0),d5
 move.w		d5,6(a1)
 bra.s		bpvibr
bpnovibr:
 move.w		(a0),6(a1)
bpvibr:
 move.l		4(a0),(a1)
 move.w		8(a0),4(a1)
 tst.b		11(a0)
 bne.s		bpdoarp
 tst.b		13(a0)
 beq.s		not2
bpdoarp:
 tst.b		arpcount
 bne.s		not0
 move.b		11(a0),d3
 move.b		13(a0),d4
 and.w		#240,d4
 and.w		#240,d3
 lsr.w		#4,d3
 lsr.w		#4,d4
 add.w		d3,d4
 add.b		10(a0),d4
 bsr		bpplayarp
 bra.s		not2
not0:
 cmp.b		#1,arpcount
 bne.s		not1
 move.b		11(a0),d3
 move.b		13(a0),d4
 and.w		#15,d3
 and.w		#15,d4
 add.w		d3,d4
 add.b		10(a0),d4
 bsr		bpplayarp
 bra.s		not2
not1:
 move.b		10(a0),d4
 bsr		bpplayarp
not2:
 lea		$10(a1),a1
 lea		$24(a0),a0
 dbra		d0,bploop1
 bsr		bpsynth
 subq.b		#1,bpcount
 beq.s		bpskip1
 rts
bpskip1:
 move.b		bpdelay(pc),bpcount
bpplay:
 bsr		bpnext
 move.w		dma(pc),$dff096
 moveq		#127,d0
bpxx:
 dbra		d0,bpxx

 moveq		#3,d0
 lea		bpbuffer(pc),a5
 lea		bpcurrent(pc),a2
bploopel:
 btst		#15,(a2)
 beq.s		bpnosir
 tst.l		(a5)
 beq.s		bpnosir
 move.l		(a5),a4
 clr.l		(a5)
 movem.l	4(a5),d1/d2/d3/d4/d5/d6/d7/a0
 movem.l	d1/d2/d3/d4/d5/d6/d7/a0,(a4)
bpnosir:
 lea		$24(a5),a5
 lea		$24(a2),a2
 dbra		d0,bploopel

 moveq		#3,d0
 lea		$dff0a0,a1
 move.w		#1,d1
 lea		bpcurrent(pc),a2
 lea		bpbuffer(pc),a5
 clr.w		dma
bploop2:
 btst		#15,(a2)
 beq.s		bpskip7
 bsr		bpplayit
bpskip7:
 asl.w		#1,d1
 lea		$10(a1),a1
 lea		$24(a2),a2
 lea		$24(a5),a5
 dbra		d0,bploop2

 or.w		#$8000,dma
 move.w		dma(pc),$dff096
 rts

bpnext:
 clr.w		dma
; lea		bpsong,a0
 move.l	_bpsong(pc),a0
 lea		$dff0a0,a3
 moveq		#3,d0
 move.w		#1,d7
 lea		bpcurrent(pc),a1
bploop3:
 moveq		#0,d1
 move.w		bpstep(pc),d1
 lsl.w		#4,d1
 move.l		d0,d2
 lsl.l		#2,d2
 add.l		d2,d1
 add.l		#512,d1
 move.w		(a0,d1),d2
 move.b		2(a0,d1),_st
 move.b		3(a0,d1),tr
 subq.w		#1,d2
 mulu		#48,d2
 moveq		#0,d3
 move.w		30(a0),d3
 lsl.w		#4,d3
 add.l		d2,d3
 move.l	  	#$00000200,d4
 move.b		bppatcount(pc),d4
 add.l		d3,d4
 move.l		d4,a2
 add.l		a0,a2

 moveq		#0,d3
 move.b		(a2),d3
 tst.b		d3
 beq		bpoptionals
bpskip4:
 clr.w		12(a1)	  ;Clear autoslide/autoarpeggio
 clr.b		34(a1)	  ;Clear vibrato
 move.b		1(a2),d5
 and.b		#15,d5
 cmp.b		#10,d5    ;Option 10->transposes off
 bne.s		bp_do1
 move.b		2(a2),d4
 and.b		#240,d4	  ;Higher nibble=transpose
 bne.s		bp_not1
bp_do1:
 add.b		tr(pc),d3
 ext.w		d3
bp_not1:
 move.b		d3,10(a1) ; Voor Arpeggio's
 lea		bpper(pc),a4
 lsl.w		#1,d3
 move.w		-2(a4,d3.w),(a1)
 cmp.b		#13,d5
 bge.s		bp_not3
 bset		#15,(a1)
 move.b		#$ff,2(a1)
bp_not3:
 move.b		1(a2),d3
 lsr.b		#4,d3
 and.w		#15,d3
 tst.b		d3
 bne.s		bpskip5
 move.b		3(a1),d3
bpskip5:
 move.b		1(a2),d4
 and.b		#15,d4
 cmp.b		#10,d4		;option 10
 bne.s		bp_do2
 move.b		2(a2),d4
 and.b		#15,d4
 bne.s		bp_not2
bp_do2:
 add.b		_st(pc),d3
bp_not2:
 move.b		1(a2),d4
 and.b		#15,d4
 cmp.b		#13,d4
 bge.s		bpoptionals
 tst.b		27(a1)
 beq.s		bpsamplechange
 cmp.b		3(a1),d3
 beq.s		bpoptionals
bpsamplechange:
 move.b		d3,3(a1)
 or.w		d7,dma

bpoptionals:
 clr.w		d4
 move.b		1(a2),d3
 move.b		2(a2),d4
 and.w		#15,d3
 bne.s		notopt0
 move.b		d4,11(a1)
 bra		bpskip2
notopt0:
 cmp.b		#1,d3
 bne.s		bpskip3
 move.b		d4,2(a1) ; Volume ook in BPCurrent
 tst.b		27(a1)
 bne		bpskip2
 move.w		d4,8(a3)
 bra		bpskip2
bpskip3:
 cmp.b		#2,d3  ; Set Speed
 bne.s		bpskip9
 move.b		d4,bpcount
 move.b		d4,bpdelay
 bra		bpskip2
bpskip9:
 cmp.b		#3,d3 ; Filter = LED control
 bne.s		bpskipa
 tst.b		d4
 bne.s		bpskipb
 bset		#1,$bfe001
 bra		bpskip2
bpskipb:
 bclr		#1,$bfe001
 bra		bpskip2
bpskipa:
 cmp.b		#4,d3 ; PortUp
 bne.s		noportup
 sub.w		d4,(a1) ; Slide data in BPCurrent
 clr.b		11(a1) ; Arpeggio's uit
 bra.s		bpskip2
noportup:
 cmp.b		#5,d3 ; PortDown
 bne.s		noportdn
 add.w		d4,(a1) ; Slide down
 clr.b		11(a1)
 bra.s		bpskip2
noportdn:
 cmp.b		#6,d3
 bne.s		notopt6
 move.b		d4,34(a1)
 bra.s		bpskip2
notopt6:
 cmp.b		#7,d3	; DBRA repcount
 bne.s		notopt7
 move.b		d4,bpnextstep+1
 move.b		#$ff,bpnextstep
 bra.s		bpskip2
notopt7:
 cmp.b		#8,d3	;Set AutoSlide
 bne.s		notopt8
 move.b		d4,12(a1)
 bra.s		bpskip2
notopt8:
 cmp.b		#11,d3
 bne.s		notopt11
 move.b		d4,35(a1)
notopt11:
 cmp.b		#13,d3	;14/15 no reset EG, 15=No reset ADSR
 bge.s		bpdoautoarp
 cmp.b		#9,d3	;Set AutoArpeggio
 bne.s		notopt9
bpdoautoarp:
 move.b		d4,13(a1)
 cmp.b		#13,d3
 bne.s		notopt13
 eor.b		#1,35(a1)
notopt13:
 cmp.b		#15,d3
 beq.s		notopt9
 clr.w		18(a1)		;reset adsr
 tst.b		31(a1)	 	;ADSR OOC
 bne.s		notopt9
 move.b		#1,31(a1)	;ADSR Once
notopt9:
bpskip2:
 lea		$10(a3),a3
 lea		$24(a1),a1
 asl.w		#1,d7
 dbra		d0,bploop3
 tst.b		bpnextstep
 beq.s		bp_is_ma
 clr.b		bpnextstep
 clr.b		bppatcount
 move.b		bpnextstep+1(pc),bpstep+1
 bra.s		bpskip8
bp_is_ma:
 addq.b		#3,bppatcount
 cmp.b		#48,bppatcount
 bne.s		bpskip8
 clr.b		bppatcount
 addq.w		#1,bpstep
; lea		bpsong,a0
 move.l	_bpsong(pc),a0
 move.w		30(a0),d1
 cmp.w		bpstep(pc),d1
 bne.s		bpskip8
 clr.w		bpstep
bpskip8:
 rts

;
; Entry point :
;
; a1 = HHardware
; a2 = BP current ( per.w, vol.b (-1=def), inst.b
; a5 = BP buffer
; d1 = DMA bits
;
bpplayit:
 bclr		#15,(a2)
 move.w		(a2),6(a1)	;Period from bpcurrent
 moveq		#0,d7
 move.b		3(a2),d7	;Instrument number
 move.l		d7,d6		;Also in d6
 lsl.l		#5,d7		;Header offset
; lea		bpsong,a3
 move.l	_bpsong(pc),a3

 cmp.b		#$ff,(a3,d7.w)	;Is synthetic
 beq		bpplaysynthetic	;Yes ??
 clr.b		27(a2)		;Synthetic mode off
 clr.b		30(a2)		;Lfo Off
 add.l		#24,d7		;24 is name->ignore
 lsl.l		#2,d6		;x4 for sample offset
 lea		samples(pc),a4
 move.l		-4(a4,d6.w),d4	;Fetch sample pointer
 beq.s		bp_nosamp	;is zero->no sample
 move.l		d4,(a1)		;Sample pointer in hardware
 move.w		(a3,d7.w),4(a1)	;length in hardware
 move.b		2(a2),9(a1)	;and volume from bpcurrent
 cmp.b		#$ff,2(a2)	;Use default volume
 bne.s		skipxx		;No ??
 move.w		6(a3,d7.w),8(a1);Default volume in hardware
skipxx:
 move.w		4(a3,d7.w),8(a2);Length in bpcurrent
 moveq		#0,d6
 move.w		2(a3,d7.w),d6	;Calculate repeat
 add.l		d6,d4
 lsr.w		#1,d6
 move.l		d4,4(a2)	;sample start in bpcurrent
 cmp.w		#1,8(a2)	;has sample repeat part
 bne.s		bpskip6		;Yes ??
bp_nosamp:
 move.l		#null,4(a2)	;Play no sample
 bra.s		bpskip10
bpskip6:
 tst.w		d6
 beq.s		bpfinal
 add.w		8(a2),d6
 move.w		d6,4(a1)
 bra.s		bpskip10
bpfinal:
 move.w		8(a2),4(a1)	;Length to hardware
 move.l		4(a2),(a1)	;pointer to hardware
bpskip10:
 or.w		d1,dma
 rts

bpplaysynthetic:
 move.b		#1,27(a2)	;Synthetic mode on
 clr.l		14(a2)		;EG & LFO Pointer restart
 clr.l		18(a2)		;ADSR & MOD Pointer restart
 move.b		20(a3,d7.w),22(a2);EG Delay
 addq.b		#1,22(a2)	;0 is nodelay
 move.b		14(a3,d7.w),23(a2);LFO Delay
 addq.b		#1,23(a2)	;So I need correction
 move.b		#1,24(a2)	;ADSR Delay->Start immediate
 move.b		28(a3,d7.w),25(a2);MOD delay
 addq.b		#1,25(a2)	;correction
 move.b		24(a3,d7.w),26(a2);filter delay
 add.b		#1,26(a2)	;correction
 move.b		22(a3,d7.w),35(a2);filter control
 move.b		16(a3,d7.w),29(a2);EG OOC
 move.b		9(a3,d7.w),30(a2);LFO OOC
 move.b		4(a3,d7.w),31(a2);ADSR OOC
 move.b		25(a3,d7.w),32(a2);MOD control
 clr.b		28(a2)		;current EG value

 move.l		tables(pc),a4	;Pointer to waveform tables
 moveq		#0,d3
 move.b		1(a3,d7.w),d3	;Which waveform
 lsl.l		#6,d3		;x64 is length waveform table
 add.l		d3,a4
 move.l		a4,(a1)		;Sample Pointer
 move.l		a4,4(a2)	;In bpcurrent
 move.w		2(a3,d7.w),4(a1);Length in words
 move.w		2(a3,d7.w),8(a2);Length in bpcurrent
 tst.b		31(a2)		;Is ADSR on
 beq.s		bpadsroff	;No ??
 move.l		tables(pc),a4	;Tables
 moveq		#0,d3
 move.b		5(a3,d7.w),d3	;ADSR table number
 lsl.l		#6,d3		;x64 for length
 add.l		d3,a4		;Add it
 move.w		#128,d3		;for centering
 add.b		(a4),d3		;Get table value
 lsr.w		#2,d3		;Divide by 4->0..63
 cmp.b		#$ff,2(a2)
 bne.s		bpskip99
 move.b		29(a3,d7.w),2(a2)
bpskip99:
 clr.w		d4
 move.b		2(a2),d4	;Default volume
 mulu		d4,d3		;default maal init volume
 lsr.w		#6,d3		;divide by 64
 move.w		d3,8(a1)	;is new volume
 bra.s		bpflipper
bpadsroff:
 move.b		2(a2),9(a1)
 cmp.b		#$ff,2(a2)
 bne.s		bpflipper	;No ADSR
 move.b		29(a3,d7.w),9(a1);So use default volume
bpflipper:
 tst.b		29(a2)		;EG off
 bne.s		bpcopyzooi
 tst.b		32(a2)		;MOD off
 bne.s		bpcopyzooi
 tst.b		35(a2)		;FX off
 beq		bpskip10
bpcopyzooi:
 move.l		4(a2),a4	;Pointer on waveform
 move.l		a4,(a5)		;Save it
 movem.l	(a4),d2/d3/d4/d5
 movem.l	d2/d3/d4/d5,4(a5)
 movem.l	16(a4),d2/d3/d4/d5
 movem.l	d2/d3/d4/d5,20(a5)
 bra		bpskip10

bpplayarp:
 lea		bpper(pc),a4
 ext.w		d4
 asl.w		#1,d4
 move.w		-2(a4,d4.w),6(a1)
 move.w		-2(a4,d4.w),(a0)
 rts

bpsynth:
 moveq		#3,d0
 lea		bpcurrent(pc),a2
 lea		$dff0a0,a1
; lea		bpsong,a3
 move.l	_bpsong(pc),a3
 lea		bpbuffer(pc),a5
bpsynthloop:
 tst.b		27(a2)		;Is synthetic sound
 beq.s		bpnosynth	;No ??
 bsr.s		bpyessynth	;Yes !!
bpnosynth:
 lea		$24(a5),a5
 lea		$24(a2),a2
 lea		$10(a1),a1
 dbra		d0,bpsynthloop
 rts
bpyessynth:
 clr.w		d7
 move.b		3(a2),d7	;Which instr. was I playing
 lsl.w		#5,d7		;x32, is length of instr.
 tst.b		31(a2)		;ADSR off
 beq.s		bpendadsr	;Yes ??
 subq.b		#1,24(a2)	;Delay,May I
 bne.s		bpendadsr	;No ??
 moveq		#0,d3
 move.b		8(a3,d7.w),24(a2)
 move.l		tables(pc),a4
 move.b		5(a3,d7.w),d3	;Which ADSR table
 lsl.l		#6,d3		;x64
 add.l		d3,a4		;This is my table
 move.w		18(a2),d3	;Get ADSR table pointer
 move.w		#128,d4		;centering
 add.b		(a4,d3.w),d4	;Value from table
 lsr.w		#2,d4		;And now from 0..63
 clr.w		d3
 move.b		2(a2),d3	;Current Volume
 mulu		d3,d4		;MultiPly with table volume
 lsr.w		#6,d4		;Divide by 64=New volume
 move.w		d4,8(a1)	;Volume in hardware
 addq.w		#1,18(a2)	;Increment of ADSR pointer
 move.w		6(a3,d7.w),d4	;Length of adsr table
 cmp.w		18(a2),d4	;End of table reached
 bne.s		bpendadsr	;No ??
 clr.w		18(a2)		;Clear ADSR Pointer
 cmp.b		#1,31(a2)	;Once
 bne.s		bpendadsr	;No ??
 clr.b		31(a2)		;ADSR off
bpendadsr:
 tst.b		30(a2)		;LFO On
 beq.s		bpendlfo	;No ??
 subq.b		#1,23(a2)	;LFO delay,May I
 bne.s		bpendlfo	;No
 moveq		#0,d3
 move.b		15(a3,d7.w),23(a2)
 move.l		tables(pc),a4
 move.b		10(a3,d7.w),d3	;Which LFO table
 lsl.l		#6,d3		;x64
 add.l		d3,a4
 move.w		16(a2),d3	;LFO pointer
 move.b		(a4,d3.w),d4	;That's my value
 ext.w		d4		;Make it a word
 ext.l		d4		;And a longword
 moveq		#0,d5
 move.b		11(a3,d7.w),d5	;LFO depth
 tst.b		d5
 beq.s		bpnotx
 divs		d5,d4		;Calculate it
bpnotx:
 move.w		(a2),d5		;Period
 add.w		d4,d5		;New Period
 move.w		d5,6(a1)	;In hardware
 addq.w		#1,16(a2)	;Next position
 move.w		12(a3,d7.w),d3	;LFO table Length
 cmp.w		16(a2),d3	;End Reached
 bne.s		bpendlfo	;NO ??
 clr.w	 	16(a2)		;Reset LFO Pointer
 cmp.b		#1,30(a2)	;Once LFO
 bne.s		bpendlfo	;NO ??
 clr.b		30(a2)		;LFO Off
bpendlfo:
 tst.l		(a5)
 beq		bpnomod
 tst.b		29(a2)		;EG On
 beq		bpendeg		;No ??
 subq.b		#1,22(a2)	;EG delay,May I
 bne.s		bpendeg		;No
 tst.l		(a5)
 beq.s		bpendeg
 moveq		#0,d3
 move.b		21(a3,d7.w),22(a2)
 move.l		tables(pc),a4
 move.b		17(a3,d7.w),d3	;Which EG table
 lsl.l		#6,d3		;x64
 add.l		d3,a4
 move.w		14(a2),d3	;EG pointer
 moveq		#0,d4
 move.b		(a4,d3.w),d4	;That's my value
 move.l		(a5),a4		;Pointer to waveform
 add.b		#128,d4		;0..255
 lsr.l		#3,d4		;0..31
 moveq		#0,d3
 move.b		28(a2),d3	;Old EG Value
 move.b		d4,28(a2)
 add.l		d3,a4		;WaveForm Position
 move.l		a5,a6		;Buffer
 add.l		d3,a6		;Position
 addq.l		#4,a6		;For adress in buffer
 cmp.b		d3,d4		;Compare old with new value
 beq.s		bpnexteg	;no change ??
 bgt.s		bpishigh	;new value is higher
bpislow:
 sub.l		d4,d3		;oldvalue-newvalue
 subq.l		#1,d3		;Correction for DBRA
bpegloop1a:
 move.b		-(a6),d4
 move.b		d4,-(a4)
 dbra		d3,bpegloop1a
 bra.s		bpnexteg
bpishigh:
 sub.l		d3,d4		;Newvalue-oldvalue
 subq.l		#1,d4		;Correction for DBRA
bpegloop1b:
 move.b		(a6)+,d3
 neg.b		d3
 move.b		d3,(a4)+	;DoIt
 dbra		d4,bpegloop1b
bpnexteg:
 addq.w		#1,14(a2)	;Next position
 move.w		18(a3,d7.w),d3	;EG table Length
 cmp.w		14(a2),d3	;End Reached
 bne.s		bpendeg		;NO ??
 clr.w	 	14(a2)		;Reset EG Pointer
 cmp.b		#1,29(a2)	;Once EG
 bne.s		bpendeg		;NO ??
 clr.b		29(a2)		;EG Off
bpendeg:
 cmp.b		#1,35(a2)
 bne.s		bpnofilter
 subq.b		#1,26(a2)		;delay
 bne		bpnotthis2		;not yet...
 move.b		23(a3,d7.w),26(a2)	;speed

 move.l		(a5),a4
 moveq		#32/2-1,d5
 move.b		-1(a4),d3
 ext.w		d3
avloop:
 move.b		1(a4),d4
 ext.w		d4
 add.w		d3,d4
 asr.w		#1,d4
 move.b		d4,(a4)+
 move.b		1(a4),d3
 ext.w		d3
 add.w		d4,d3
 asr.w		#1,d3
 move.b		d3,(a4)+
 dbra		d5,avloop

bpnofilter:
 cmp.b		#2,35(a2)
 bne.s		bpnotthis
 move.b		23(a3,d7.w),d4
 lea		32+4(a5),a4
 move.l		(a5),a6
 moveq		#32-1,d5
bpthisloop:
 move.b		-(a4),d3
 cmp.b		(a6)+,d3
 beq.s		bpnietnu
 bge.s		bpgroter
 sub.b		d4,-1(a6)
 bra.s		bpnietnu
bpgroter:
 add.b		d4,-1(a6)
bpnietnu:
 dbra		d5,bpthisloop
bpnotthis:
 cmp.b		#3,35(a2)
 bne.s		bpnotthis2
 move.b		23(a3,d7.w),d4		;speed
 lea		4(a5),a4
 move.l		(a5),a6
 moveq		#32-1,d5
bpthisloop2:
 move.b		(a4)+,d3
 cmp.b		(a6)+,d3
 beq.s		bpnietnu2
 bge.s		bpgroter2
 sub.b		d4,-1(a6)
 bra.s		bpnietnu2
bpgroter2:
 add.b		d4,-1(a6)
bpnietnu2:
 dbra		d5,bpthisloop2
bpnotthis2:
 cmp.b		#4,35(a2)
 bne.s		bpnotthis3
 move.b		23(a3,d7.w),d4
 move.l		(a5),a4
 lea		64(a4),a4
 move.l		(a5),a6
 moveq		#32-1,d5
bpthisloop3:
 move.b		(a4)+,d3
 cmp.b		(a6)+,d3
 beq.s		bpnietnu3
 bge.s		bpgroter3
 sub.b		d4,-1(a6)
 bra.s		bpnietnu3
bpgroter3:
 add.b		d4,-1(a6)
bpnietnu3:
 dbra		d5,bpthisloop3
bpnotthis3:
 cmp.b		#5,35(a2)
 bne.s		bpnotthis4
 move.b		23(a3,d7.w),d4
 lea		4(a5),a4
 move.l		(a5),a6
 moveq		#32-1,d5
bpthisloop4:
 move.b		(a4)+,d3
 cmp.b		(a6)+,d3
 beq.s		bpnietnu4
 bge.s		bpgroter4
 sub.b		d4,-1(a6)
 bra.s		bpnietnu4
bpgroter4:
 add.b		d4,-1(a6)
bpnietnu4:
 dbra		d5,bpthisloop4
bpnotthis4:
 cmp.b		#6,35(a2)
 bne.s		bpnotthis5
 subq.b		#1,26(a2)		;delay
 bne		bpnotthis5		;not yet...
 clr.b		35(a2)
 move.b		#1,26(a2)
 move.l		(a5),a4
 moveq		#8-1,d5
bpopt6loop:
 move.l		64(a4),(a4)+
 dbra		d5,bpopt6loop

bpnotthis5:
 tst.b		32(a2)			;mod control
 beq.s		bpnomod			;no mod active
 subq.b		#1,25(a2)		;delay
 bne.s		bpnomod			;not yet...
 move.b		27(a3,d7.w),25(a2)	;speed

 move.l		tables,a4
 moveq		#0,d3
 move.b		26(a3,d7.w),d3	;Which MOD table
 lsl.l		#6,d3		;x64
 add.l		d3,a4
 move.w		20(a2),d3	;MOD pointer
 move.l		(a5),a6
 move.b		(a4,d3.w),32(a6)

 addq.w		#1,20(a2)
 move.w		30(a3,d7.w),d3
 cmp.w		20(a2),d3
 bne.s		bpnomod
 clr.w		20(a2)
 cmp.b		#1,32(a2)		;once ??
 bne.s		bpnomod			;no
 clr.b		32(a2)			;yes, so no more..
bpnomod:
 rts

null: dc.w 0
bpcurrent: dc.w 0,0	;periode,instrument =(volume.b,instr nr.b)
	   dc.l null	;start
           dc.w 1	;length (words)
	   dc.b 0,0,0,0 ;noot,arpeggio,autoslide,autoarpeggio
	   dc.w 0,0,0,0	;EG,LFO,ADSR,MOD pointers
	   dc.b 0,0,0,0	;EG,LFO,ADSR,MOD count
	   dc.b	0,0	;FILTER count, Synthetic yes/no
	   dc.b	0,0	;Current EG value,EG control
	   dc.b	0,0,0,0	;LFO,ADSR,MOD control, volume
	   dc.b 0,0	;vibrato, FILTER control

	   dc.w 0,0	;periode,instrument =(volume.b,instr nr.b)
	   dc.l null	;start
           dc.w 1	;length (words)
	   dc.b 0,0,0,0 ;noot,arpeggio,autoslide,autoarpeggio
	   dc.w 0,0,0,0	;EG,LFO,ADSR,MOD pointers
	   dc.b 0,0,0,0	;EG,LFO,ADSR,MOD count
	   dc.b	0,0	;FILTER count, Synthetic yes/no
	   dc.b	0,0	;Current EG value,EG control
	   dc.b	0,0,0,0	;LFO,ADSR,MOD control, volume
	   dc.b 0,0	;vibrato, dummy

	   dc.w 0,0	;periode,instrument =(volume.b,instr nr.b)
	   dc.l null	;start
           dc.w 1	;length (words)
	   dc.b 0,0,0,0 ;noot,arpeggio,autoslide,autoarpeggio
	   dc.w 0,0,0,0	;EG,LFO,ADSR,MOD pointers
	   dc.b 0,0,0,0	;EG,LFO,ADSR,MOD count
	   dc.b	0,0	;FILTER count, Synthetic yes/no
	   dc.b	0,0	;Current EG value,EG control
	   dc.b	0,0,0,0	;LFO,ADSR,MOD control, volume
	   dc.b 0,0	;vibrato, dummy

	   dc.w 0,0	;periode,instrument =(volume.b,instr nr.b)
	   dc.l null	;start
           dc.w 1	;length (words)
	   dc.b 0,0,0,0 ;noot,arpeggio,autoslide,autoarpeggio
	   dc.w 0,0,0,0	;EG,LFO,ADSR,MOD pointers
	   dc.b 0,0,0,0	;EG,LFO,ADSR,MOD count
	   dc.b	0,0	;FILTER count, Synthetic yes/no
	   dc.b	0,0	;Current EG value,EG control
	   dc.b	0,0,0,0	;LFO,ADSR,MOD control, volume
	   dc.b 0,0	;vibrato, dummy

bpstep: dc.w 0
bppatcount: dc.b 0
_st: dc.b 0
tr: dc.b 0
bpcount: dc.b 1
bpdelay: dc.b 6
arpcount: dc.b 1
bprepcount: dc.b 1
numtables: dc.b	0
 even
bpnextstep:dc.w	0
lfopointer: dc.l lfotable
lfotable: dc.w	0,64,128,64,0,-64,-128,-64
dma: dc.w 0
tables:	dc.l 0

bpbuffer:
 dcb.b	144,0

 dc.w	6848,6464,6080,5760,5440,5120,4832,4576,4320,4064,3840,3616
 dc.w	3424,3232,3040,2880,2720,2560,2416,2288,2160,2032,1920,1808
 dc.w	1712,1616,1520,1440,1360,1280,1208,1144,1080,1016,0960,0904
bpper:
 dc.w	0856,0808,0760,0720,0680,0640,0604,0572,0540,0508,0480,0452
 dc.w	0428,0404,0380,0360,0340,0320,0302,0286,0270,0254,0240,0226
 dc.w	0214,0202,0190,0180,0170,0160,0151,0143,0135,0127,0120,0113
 dc.w	0107,0101,0095,0090,0085,0080,0076,0072,0068,0064,0060,0057
samples:
 dcb.l	15,0

