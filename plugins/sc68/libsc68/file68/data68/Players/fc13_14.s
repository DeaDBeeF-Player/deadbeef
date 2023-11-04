;;; futur composer PIC and unified version
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 14:53:22 ben>
;;; 

;;; a0 = address of module

	bra	init_music
	bra 	end_music
	bra	play		;15-20 raster lines

end_music:
	clr.l $dff0a6
	clr.l $dff0b6
	clr.l $dff0c6
	clr.l $dff0d6
	move.w #$000f,$dff096
	lea	onoff(pc),a1
	clr.b	(a1)
	rts

init_music:
	lea	onoff(pc),a1
	st.b	(a1)

	lea 100(a0),a1
	cmp.l #'SMOD',(a0)
	beq.s .nofc14
	lea 80(a1),a1
.nofc14:
	lea SEQpoint(pc),a6
	move.l a1,(a6)
	move.l a0,a1
	add.l 8(a0),a1
	move.l a1,PATpoint-SEQpoint(a6)
	move.l a0,a1
	add.l 16(a0),a1
	move.l a1,FRQpoint-SEQpoint(a6)
	move.l a0,a1
	add.l 24(a0),a1
	move.l a1,VOLpoint-SEQpoint(a6)
	move.l 4(a0),d0
	divu #13,d0

	lea 40(a0),a1
	lea SOUNDINFO+4(pc),a2
	moveq #10-1,d1
initloop:
	move.w (a1)+,(a2)+
	move.l (a1)+,(a2)+
	adda.w #10,a2
	dbf d1,initloop
	move.l a0,d1
	add.l 32(a0),d1
	lea SOUNDINFO(pc),a3 ; Init sample ( 0 - 9 )
	move.l d1,(a3)+
	moveq #9-1,d3
	moveq #0,d2
initloop1:
	move.w (a3),d2
	add.l d2,d1
	add.l d2,d1
	addq.l #2,d1
	adda.w #12,a3
	move.l d1,(a3)+
	dbf d3,initloop1

	lea SOUNDINFO+(10*16)(pc),a2

	cmp.l #'SMOD',(a0)
	bne.s relocinst14

; --- 1.0 - 1.3 Get synth in player ---

relocinst13:
	lea waveforms(pc),a3
	lea fc13synth(pc),a1
	moveq #47-1,d1
.initloop2:
	move.l (a1)+,d2
	add.l a3,d2
	move.l d2,(a2)+
	move.l (a1)+,(a2)+
	move.w (a1)+,(a2)+
	addq.w #6,a2
	dbf d1,.initloop2
	bra.s	nextinit

; --- 1.4 Get synth in module ---
relocinst14:
	lea 100(a0),a1
	move.l a0,a3
	add.l 36(a0),a3

	moveq #80-1,d1
	moveq #0,d2
.initloop2:
	move.l a3,(a2)+
	move.b (a1)+,d2
	move.w d2,(a2)+
	clr.w (a2)+
	move.w d2,(a2)+
	addq.w #6,a2
	add.w d2,a3
	add.w d2,a3
	dbf d1,.initloop2

nextinit:
	move.l SEQpoint(pc),a0
	moveq #0,d2
	move.b 12(a0),d2		;Get replay speed
	bne.s speedok
	move.b #3,d2			;Set default speed
speedok:
	lea respcnt(pc),a6
	move.w d2,(a6)		;Init repspeed counter
	move.w d2,repspd-respcnt(a6)
INIT2:
	lea audtemp(pc),a6
	clr.w (a6)
	move.w #$000f,$dff096		;Disable audio DMA
	move.w #$0780,$dff09a		;Disable audio IRQ
	moveq #0,d7
	mulu #13,d0
	moveq #4-1,d6			;Number of soundchannels-1
	lea V1data(pc),a0		;Point to 1st voice data area
	lea silent(pc),a1
	lea Chandata(pc),a2
initloop3:
	move.l a1,10(a0)
	move.l a1,18(a0)
	clr.w 4(a0)
	move.w #$000d,6(a0)
	clr.w 8(a0)
	clr.l 14(a0)
	move.b #$01,23(a0)
	move.b #$01,24(a0)
	clr.b 25(a0)
	clr.l 26(a0)
	clr.w 30(a0)
	clr.l 38(a0)
	clr.w 42(a0)
	clr.l 44(a0)
	clr.l 48(a0)
	clr.w 56(a0)
	moveq #$00,d3
	move.w (a2)+,d1
	move.w (a2),d3
	divu #$0003,d3
	moveq #0,d4
	bset d3,d4
	move.w d4,32(a0)
	move.w (a2)+,d3
	andi.l #$00ff,d3
	andi.l #$00ff,d1
	lea $dff0a0,a6
	add.w d1,a6
	move.l #$0000,(a6)
	move.w #$0100,4(a6)
	move.w #$0000,6(a6)
	move.w #$0000,8(a6)
	move.l a6,60(a0)
	move.l SEQpoint(pc),(a0)
	move.l SEQpoint(pc),52(a0)
	add.l d0,52(a0)
	add.l d3,52(a0)
	add.l d7,(a0)
	add.l d3,(a0)
	move.l (a0),a3
	move.b (a3),d1
	andi.l #$00ff,d1
	lsl.w #6,d1
	move.l PATpoint(pc),a4
	adda.w d1,a4
	move.l a4,34(a0)
	move.b 1(a3),44(a0)
	move.b 2(a3),22(a0)
	lea $4a(a0),a0		;Point to next voice's data area
	dbf d6,initloop3
	rts


play:
	lea audtemp(pc),a5
	tst.w 8(a5)
	bne.s music_on
	rts
music_on:
	subq.w #1,4(a5)			;Decrease replayspeed counter
	bne.s nonewnote
	move.w 6(a5),4(a5)		;Restore replayspeed counter
	moveq #0,d5
	moveq #6,d6
	lea V1data(pc),a0		;Point to voice1 data area
	bsr new_note
	lea V2data(pc),a0		;Point to voice2 data area
	bsr new_note
	lea V3data(pc),a0		;Point to voice3 data area
	bsr new_note
	lea V4data(pc),a0		;Point to voice4 data area
	bsr new_note
nonewnote:
	clr.w (a5)
	lea $dff000,a6
	lea V1data(pc),a0
	bsr effects
	move.l d0,$a6(a6)
	lea V2data(pc),a0
	bsr effects
	move.l d0,$b6(a6)
	lea V3data(pc),a0
	bsr effects
	move.l d0,$c6(a6)
	lea V4data(pc),a0
	bsr effects
	move.l d0,$d6(a6)
	lea V1data(pc),a0
	move.l 68+(0*74)(a0),a1		;Get samplepointer
	adda.w 64+(0*74)(a0),a1		;add repeat_start
	move.l 68+(1*74)(a0),a2
	adda.w 64+(1*74)(a0),a2
	move.l 68+(2*74)(a0),a3
	adda.w 64+(2*74)(a0),a3
	move.l 68+(3*74)(a0),a4
	adda.w 64+(3*74)(a0),a4
	move.w 66+(0*74)(a0),d1		;Get repeat_length
	move.w 66+(1*74)(a0),d2
	move.w 66+(2*74)(a0),d3
	move.w 66+(3*74)(a0),d4
	moveq #2,d0
	moveq #0,d5
	move.w (a5),d7
	ori.w #$8000,d7			;Set/clr bit = 1
	move.w d7,$dff096		;Enable audio DMA
chan1:
	lea V1data+72(pc),a0
	move.w (a0),d7
	beq.s chan2
	subq.w #1,(a0)
	cmp.w d0,d7
	bne.s chan2
	move.w d5,(a0)
	move.l a1,$a0(a6)		;Set samplestart
	move.w d1,$a4(a6)		;Set samplelength
chan2:
	lea V2data+72(pc),a0
	move.w (a0),d7
	beq.s chan3
	subq.w #1,(a0)
	cmp.w d0,d7
	bne.s chan3
	move.w d5,(a0)
	move.l a2,$b0(a6)
	move.w d2,$b4(a6)
chan3:
	lea V3data+72(pc),a0
	move.w (a0),d7
	beq.s chan4
	subq.w #1,(a0)
	cmp.w d0,d7
	bne.s chan4
	move.w d5,(a0)
	move.l a3,$c0(a6)
	move.w d3,$c4(a6)
chan4:
	lea V4data+72(pc),a0
	move.w (a0),d7
	beq.s endplay
	subq.w #1,(a0)
	cmp.w d0,d7
	bne.s endplay
	move.w d5,(a0)
	move.l a4,$d0(a6)
	move.w d4,$d4(a6)
endplay:
	rts

new_note:
	move.l 34(a0),a1
	adda.w 40(a0),a1
	cmp.b #$49,(a1)		;Check "END" mark in pattern
	beq.s patend
	cmp.w #64,40(a0)		;Have all the notes been played?
	bne samepat
patend:
	move.w d5,40(a0)
	move.l (a0),a2
	adda.w 6(a0),a2		;Point to next sequence row
	cmpa.l 52(a0),a2	;Is it the end?
	bne.s notend
	move.w d5,6(a0)		;yes!
	move.l (a0),a2		;Point to first sequence
notend:
	lea spdtemp(pc),a3
	moveq #1,d1
	addq.b #1,(a3)
	cmpi.b #5,(a3)
	bne.s nonewspd
	move.b d1,(a3)
	move.b 12(a2),d1	;Get new replay speed
	beq.s nonewspd
	move.w d1,2(a3)		;store in counter
	move.w d1,4(a3)
nonewspd:
	move.b (a2)+,d1		;Pattern to play
	move.b (a2)+,44(a0)	;Transpose value
	move.b (a2)+,22(a0)	;Soundtranspose value
	lsl.w d6,d1
	move.l PATpoint(pc),a1	;Get pattern pointer
	add.w d1,a1
	move.l a1,34(a0)
	addi.w #$000d,6(a0)
samepat:
	move.b 1(a1),d1		;Get info byte
	move.b (a1)+,d0		;Get note
	bne.s ww1
	andi.w #%11000000,d1
	beq.s noport
	bra.s ww11
ww1:
	move.w d5,56(a0)
ww11:
	move.b d5,47(a0)
	btst #7,d1
	beq.s noport
	move.b 2(a1),47(a0)
noport:
	andi.w #$007f,d0
	beq nextnote
	move.b d0,8(a0)
	move.b (a1),d1
	move.b d1,9(a0)
	move.w 32(a0),d3
	or.w d3,(a5)
	move.w d3,$dff096
	andi.w #$003f,d1	;Max 64 instruments
	add.b 22(a0),d1		;add Soundtranspose
	move.l VOLpoint(pc),a2
	lsl.w d6,d1
	adda.w d1,a2
	move.w d5,16(a0)
	move.b (a2),23(a0)
	move.b (a2)+,24(a0)
	moveq #0,d1
	move.b (a2)+,d1
	move.b (a2)+,27(a0)
	move.b #$40,46(a0)
	move.b (a2),28(a0)
	move.b (a2)+,29(a0)
	move.b (a2)+,30(a0)
	move.l a2,10(a0)
	move.l FRQpoint(pc),a2
	lsl.w d6,d1
	adda.w d1,a2
	move.l a2,18(a0)
	move.w d5,50(a0)
	move.b d5,25(a0)
	move.b d5,26(a0)
nextnote:
	addq.w #2,40(a0)
	rts

effects:
	moveq #0,d7
testsustain:
	tst.b 26(a0)		;Is sustain counter = 0
	beq.s sustzero
	subq.b #1,26(a0)	;if no, decrease counter
	bra VOLUfx
sustzero:		;Next part of effect sequence
	move.l 18(a0),a1	;can be executed now.
	adda.w 50(a0),a1
testeffects:
	cmpi.b #$e1,(a1)	;E1 = end of FREQseq sequence
	beq VOLUfx
	move.b (a1),d0
	cmpi.b #$e0,d0		;E0 = loop to other part of sequence
	bne.s testnewsound
	move.b 1(a1),d1		;loop to start of sequence + 1(a1)
	andi.w #$003f,d1
	move.w d1,50(a0)
	move.l 18(a0),a1
	adda.w d1,a1
	move.b (a1),d0
testnewsound:
	cmpi.b #$e2,d0		;E2 = set waveform
	bne.s testE4
	move.w 32(a0),d1
	or.w d1,(a5)
	move.w d1,$dff096
	moveq #0,d0
	move.b 1(a1),d0
	lea SOUNDINFO(pc),a4
	lsl.w #4,d0
	adda.w d0,a4
	move.l 60(a0),a3
	move.l (a4)+,d1
	move.l d1,(a3)
	move.l d1,68(a0)
	move.w (a4)+,4(a3)
	move.l (a4),64(a0)
	move.w #$0003,72(a0)
	move.w d7,16(a0)
	move.b #$01,23(a0)
	addq.w #2,50(a0)
	bra transpose
testE4:
	cmpi.b #$e4,d0
	bne.s testE9
	moveq #0,d0
	move.b 1(a1),d0
	lea SOUNDINFO(pc),a4
	lsl.w #4,d0
	adda.w d0,a4
	move.l 60(a0),a3
	move.l (a4)+,d1
	move.l d1,(a3)
	move.l d1,68(a0)
	move.w (a4)+,4(a3)
	move.l (a4),64(a0)
	move.w #$0003,72(a0)
	addq.w #2,50(a0)
	bra transpose
testE9:
	cmpi.b #$e9,d0
	bne testpatjmp
	move.w 32(a0),d1
	or.w d1,(a5)
	move.w d1,$dff096
	moveq #0,d0
	move.b 1(a1),d0
	lea SOUNDINFO(pc),a4
	lsl.w #4,d0
	adda.w d0,a4
	move.l (a4),a2
	cmpi.l #"SSMP",(a2)+
	bne.s nossmp
	lea 320(a2),a4
	moveq #0,d1
	move.b 2(a1),d1
	lsl.w #4,d1
	add.w d1,a2
	add.l (a2),a4
	move.l 60(a0),a3
	move.l a4,(a3)
	move.l 4(a2),4(a3)
	move.l a4,68(a0)
	move.l 6(a2),64(a0)
	move.w d7,16(a0)
	move.b #1,23(a0)
	move.w #3,72(a0)
nossmp:
	addq.w #3,50(a0)
	bra.s transpose
testpatjmp:
	cmpi.b #$e7,d0
	bne.s testpitchbend
	moveq #0,d0
	move.b 1(a1),d0
	lsl.w d6,d0
	move.l FRQpoint(pc),a1
	adda.w d0,a1
	move.l a1,18(a0)
	move.w d7,50(a0)
	bra testeffects
testpitchbend:
	cmpi.b #$ea,d0
	bne.s testnewsustain
	move.b 1(a1),4(a0)
	move.b 2(a1),5(a0)
	addq.w #3,50(a0)
	bra.s transpose
testnewsustain:
	cmpi.b #$e8,d0
	bne.s testnewvib
	move.b 1(a1),26(a0)
	addq.w #2,50(a0)
	bra testsustain
testnewvib:
	cmpi.b #$e3,(a1)+
	bne.s transpose
	addq.w #3,50(a0)
	move.b (a1)+,27(a0)
	move.b (a1),28(a0)
transpose:
	move.l 18(a0),a1
	adda.w 50(a0),a1
	move.b (a1),43(a0)
	addq.w #1,50(a0)

VOLUfx:
	tst.b 25(a0)
	beq.s volsustzero
	subq.b #1,25(a0)
	bra calcperiod
volsustzero:
	tst.b 15(a0)
	bne.s do_VOLbend
	subq.b #1,23(a0)
	bne.s calcperiod
	move.b 24(a0),23(a0)
volu_cmd:
	move.l 10(a0),a1
	adda.w 16(a0),a1
	move.b (a1),d0
testvoluend:
	cmpi.b #$e1,d0
	beq.s calcperiod
	cmpi.b #$ea,d0
	bne.s testVOLsustain
	move.b 1(a1),14(a0)
	move.b 2(a1),15(a0)
	addq.w #3,16(a0)
	bra.s do_VOLbend
testVOLsustain:
	cmpi.b #$e8,d0
	bne.s testVOLloop
	addq.w #2,16(a0)
	move.b 1(a1),25(a0)
	bra.s calcperiod
testVOLloop:
	cmpi.b #$e0,d0
	bne.s setvolume
	move.b 1(a1),d0
	andi.w #$003f,d0
	subq.b #5,d0
	move.w d0,16(a0)
	bra.s volu_cmd
do_VOLbend:
	not.b 38(a0)
	beq.s calcperiod
	subq.b #1,15(a0)
	move.b 14(a0),d1
	add.b d1,45(a0)
	bpl.s calcperiod
	moveq #0,d1
	move.b d1,15(a0)
	move.b d1,45(a0)
	bra.s calcperiod
setvolume:
	move.b (a1),45(a0)
	addq.w #1,16(a0)
calcperiod:
	move.b 43(a0),d0
	bmi.s lockednote
	add.b 8(a0),d0
	add.b 44(a0),d0
lockednote:
	moveq #$7f,d1
	and.l d1,d0
	lea PERIODS(pc),a1
	add.w d0,d0
	move.w d0,d1
	adda.w d0,a1
	move.w (a1),d0

	move.b 46(a0),d7
	tst.b 30(a0)		;Vibrato_delay = zero ?
	beq.s vibrator
	subq.b #1,30(a0)
	bra.s novibrato
vibrator:
	moveq #5,d2
	move.b d1,d5
	move.b 28(a0),d4
	add.b d4,d4
	move.b 29(a0),d1
	tst.b d7
	bpl.s vib1
	btst #0,d7
	bne.s vib4
vib1:
	btst d2,d7
	bne.s vib2
	sub.b 27(a0),d1
	bcc.s vib3
	bset d2,d7
	moveq #0,d1
	bra.s vib3
vib2:
	add.b 27(a0),d1
	cmp.b d4,d1
	bcs.s vib3
	bclr d2,d7
	move.b d4,d1
vib3:
	move.b d1,29(a0)
vib4:
	lsr.b #1,d4
	sub.b d4,d1
	bcc.s vib5
	subi.w #$0100,d1
vib5:
	addi.b #$a0,d5
	bcs.s vib7
vib6:
	add.w d1,d1
	addi.b #$18,d5
	bcc.s vib6
vib7:
	add.w d1,d0
novibrato:
	eori.b #$01,d7
	move.b d7,46(a0)

; DO THE PORTAMENTO THING
	not.b 39(a0)
	beq.s pitchbend
	moveq #0,d1
	move.b 47(a0),d1	;get portavalue
	beq.s pitchbend		;0=no portamento
	cmpi.b #$1f,d1
	bls.s portaup
portadown:
	andi.w #$1f,d1
	neg.w d1
portaup:
	;sub.w d1,56(a0)
pitchbend:
	not.b 42(a0)
	beq.s addporta
	tst.b 5(a0)
	beq.s addporta
	subq.b #1,5(a0)
	moveq #0,d1
	move.b 4(a0),d1
	bpl.s pitchup
	ext.w d1
pitchup:
	sub.w d1,56(a0)
addporta:
	add.w 56(a0),d0
	cmpi.w #$0070,d0
	bhi.s nn1
	move.w #$0071,d0
nn1:
	cmpi.w #$0d60,d0
	bls.s nn2
	move.w #$0d60,d0
nn2:
	swap d0
	move.b 45(a0),d0
	rts



V1data:  dcb.b 64,0	;Voice 1 data area
offset1: dcb.b 02,0	;Is added to start of sound
ssize1:  dcb.b 02,0	;Length of sound
start1:  dcb.b 06,0	;Start of sound

V2data:  dcb.b 64,0	;Voice 2 data area
offset2: dcb.b 02,0
ssize2:  dcb.b 02,0
start2:  dcb.b 06,0

V3data:  dcb.b 64,0	;Voice 3 data area
offset3: dcb.b 02,0
ssize3:  dcb.b 02,0
start3:  dcb.b 06,0

V4data:  dcb.b 64,0	;Voice 4 data area
offset4: dcb.b 02,0
ssize4:  dcb.b 02,0
start4:  dcb.b 06,0

audtemp: dc.w 0		;DMACON
spdtemp: dc.w 0
respcnt: dc.w 0		;Replay speed counter
repspd:  dc.w 0		;Replay speed counter temp
onoff:   dc.w 0		;Music on/off flag.

Chandata: dc.l $00000000,$00100003,$00200006,$00300009
SEQpoint: dc.l 0
PATpoint: dc.l 0
FRQpoint: dc.l 0
VOLpoint: dc.l 0


silent: dc.w $0100,$0000,$0000,$00e1

PERIODS:dc.w $06b0,$0650,$05f4,$05a0,$054c,$0500,$04b8,$0474
	dc.w $0434,$03f8,$03c0,$038a,$0358,$0328,$02fa,$02d0
	dc.w $02a6,$0280,$025c,$023a,$021a,$01fc,$01e0,$01c5
	dc.w $01ac,$0194,$017d,$0168,$0153,$0140,$012e,$011d
	dc.w $010d,$00fe,$00f0,$00e2,$00d6,$00ca,$00be,$00b4
	dc.w $00aa,$00a0,$0097,$008f,$0087,$007f,$0078,$0071
	dc.w $0071,$0071,$0071,$0071,$0071,$0071,$0071,$0071
	dc.w $0071,$0071,$0071,$0071,$0d60,$0ca0,$0be8,$0b40
	dc.w $0a98,$0a00,$0970,$08e8,$0868,$07f0,$0780,$0714
	dc.w $1ac0,$1940,$17d0,$1680,$1530,$1400,$12e0,$11d0
	dc.w $10d0,$0fe0,$0f00,$0e28,$06b0,$0650,$05f4,$05a0
	dc.w $054c,$0500,$04b8,$0474,$0434,$03f8,$03c0,$038a
	dc.w $0358,$0328,$02fa,$02d0,$02a6,$0280,$025c,$023a
	dc.w $021a,$01fc,$01e0,$01c5,$01ac,$0194,$017d,$0168
	dc.w $0153,$0140,$012e,$011d,$010d,$00fe,$00f0,$00e2
	dc.w $00d6,$00ca,$00be,$00b4,$00aa,$00a0,$0097,$008f
	dc.w $0087,$007f,$0078,$0071

SOUNDINFO:
;Start.l , Length.w , Repeat start.w , Repeat-length.w , dcb.b 6,0
	dcb.b 10*16,0	;Reserved for samples
	dcb.b 80*16,0	;Reserved for waveforms

fc13synth:
;Reserved for synth sounds
;Offset.l , Sound-length.w , Start-offset.w , Repeat-length.w
	dc.w $0000,$0000 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0020 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0040 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0060 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0080 ,$0010 ,$0000 ,$0010
	dc.w $0000,$00a0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$00c0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$00e0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0100 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0120 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0140 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0160 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0180 ,$0010 ,$0000 ,$0010
	dc.w $0000,$01a0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$01c0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$01e0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0200 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0220 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0240 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0260 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0280 ,$0010 ,$0000 ,$0010
	dc.w $0000,$02a0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$02c0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$02e0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0300 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0320 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0340 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0360 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0380 ,$0010 ,$0000 ,$0010
	dc.w $0000,$03a0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$03c0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$03e0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$0400 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0410 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0420 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0430 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0440 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0450 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0460 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0470 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0480 ,$0010 ,$0000 ,$0010
	dc.w $0000,$04a0 ,$0008 ,$0000 ,$0008
	dc.w $0000,$04b0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$04d0 ,$0010 ,$0000 ,$0010
	dc.w $0000,$04f0 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0500 ,$0008 ,$0000 ,$0008
	dc.w $0000,$0510 ,$0018 ,$0000 ,$0018
	dcb.b 5*2*(80-47),0	; filled to 80 sounds

waveforms:
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $3f37,$2f27,$1f17,$0f07,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c037,$2f27,$1f17,$0f07,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$2f27,$1f17,$0f07,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b027,$1f17,$0f07,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$1f17,$0f07,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a017,$0f07,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$0f07,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9007,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9088,$ff07,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9088,$8007,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9088,$8088,$0f17,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9088,$8088,$9017,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9088,$8088,$9098,$1f27,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9088,$8088,$9098,$a027,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9088,$8088,$9098,$a0a8,$2f37
  dc.w $c0c0,$d0d8,$e0e8,$f0f8,$00f8,$f0e8,$e0d8,$d0c8
  dc.w $c0b8,$b0a8,$a098,$9088,$8088,$9098,$a0a8,$b037
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $817f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$817f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$817f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$8181,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$8181,$817f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$8181,$8181,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$8181,$8181,$817f,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$8181,$8181,$8181,$7f7f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$8181,$8181,$8181,$817f,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$7f7f,$7f7f
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$8181,$8181
  dc.w $8181,$8181,$8181,$8181,$8181,$8181,$817f,$7f7f
  dc.w $8080,$8080,$8080,$8080,$8080,$8080,$8080,$8080
  dc.w $8080,$8080,$8080,$8080,$8080,$8080,$8080,$7f7f
  dc.w $8080,$8080,$8080,$8080,$8080,$8080,$8080,$8080
  dc.w $8080,$8080,$8080,$8080,$8080,$8080,$8080,$807f
  dc.w $8080,$8080,$8080,$8080,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8080,$8080,$8080,$807f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8080,$8080,$8080,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8080,$8080,$807f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8080,$8080,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8080,$807f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8080,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8080,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f,$7f7f
  dc.w $8080,$9098,$a0a8,$b0b8,$c0c8,$d0d8,$e0e8,$f0f8
  dc.w $0008,$1018,$2028,$3038,$4048,$5058,$6068,$707f
  dc.w $8080,$a0b0,$c0d0,$e0f0,$0010,$2030,$4050,$6070
  dc.w $4545,$797d,$7a77,$7066,$6158,$534d,$2c20,$1812
  dc.w $04db,$d3cd,$c6bc,$b5ae,$a8a3,$9d99,$938e,$8b8a
  dc.w $4545,$797d,$7a77,$7066,$5b4b,$4337,$2c20,$1812
  dc.w $04f8,$e8db,$cfc6,$beb0,$a8a4,$9e9a,$9594,$8d83
  dc.w $0000,$4060,$7f60,$4020,$00e0,$c0a0,$80a0,$c0e0
  dc.w $0000,$4060,$7f60,$4020,$00e0,$c0a0,$80a0,$c0e0
  dc.w $8080,$9098,$a0a8,$b0b8,$c0c8,$d0d8,$e0e8,$f0f8
  dc.w $0008,$1018,$2028,$3038,$4048,$5058,$6068,$707f
  dc.w $8080,$a0b0,$c0d0,$e0f0,$0010,$2030,$4050,$6070
