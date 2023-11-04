;	SIDMon 2 replay routine
;	adapted for sc68 by Gerald Schnabel <gschnabel@gmx.de>

	include "lib/org.s"

	bra.w	initmuzak
	rts
	rts
	bra.w	playmuzak

initmuzak:
	movem.l	d0-d7/a0-a6,-(a7)
	bset	#$1,$bfe001
	lea	$dff000,a6

	move.w	#$0,$a8(a6)
	move.w	#$0,$b8(a6)
	move.w	#$0,$c8(a6)
	move.w	#$0,$d8(a6)
	move.w	#$f,$96(a6)

	moveq	#$0,d6
	lea	header(pc),a0
	lea	midimode(pc),a2
	moveq	#$3a,d0
	add.l	-6(a2),d0
	move.l	d0,(a0)
	move.l	-6(a2),a1
	move.w	(a1)+,(a2)+
	move.b	(a1)+,d6
	move.b	d6,(a2)
	move.b	(a1)+,1(a2)
	move.w	(a1)+,d0
	lsr.w	#$6,d0
	subq.w	#$1,d0
	move.w	d0,-4(a2)
	moveq	#$40,d0
	move.l	d0,2(a2)
	clr.b	6(a2)

	moveq	#$a,d0
addloop:move.l	(a0)+,d1
	add.l	(a1)+,d1
	move.l	d1,(a0)
	dbf	d0,addloop

	move.l	a2,a0

	lea	voice1(pc),a2
	moveq	#$3,d7
	addq.w	#$1,d6
	moveq	#$0,d5
findloop:
	clr.w	72(a2)
	move.l	d5,(a2)
	add.l	d6,d5
	bsr	findnote
	lea	voice2-voice1(a2),a2
	dbf	d7,findloop

	moveq	#$0,d0
	move.l	patterns(pc),a0
	move.w	-(a0),d0
	add.l	patterns(pc),d0
	move.l	d0,a1

	lea	voice1(pc),a2
	moveq	#$3f,d3
plus:	bsr	getnote2
	dbf	d3,plus
	clr.w	68(a2)

	move.l	a1,d0
	addq.l	#$1,d0
	bclr	#$0,d0
	move.l	d0,a0

	move.l	sampletab(pc),a1
	move.w	sampleno(pc),d0
calcaddloop:
	move.l	a0,(a1)
	moveq	#$0,d1
	move.w	4(a1),d1
	add.l	d1,d1
	add.l	d1,a0
	lea	$40(a1),a1
	dbf	d0,calcaddloop
	movem.l	(a7)+,d0-d7/a0-a6
	rts

header:		dc.l	$0
songlen:	dc.l	$0
positions:	dc.l	$0
ntransposes:	dc.l	$0
itransposes:	dc.l	$0
ins1:		dc.l	$0
wavelists:	dc.l	$0
arpeggiolists:	dc.l	$0
vibratolists:	dc.l	$0
sampletab:	dc.l	$0
patternpointer:	dc.l	$0
patterns:	dc.l	$0

playmuzak:
	movem.l	d0-d7/a0-a6,-(a7)
	lea	$dff000,a6

	lea	length(pc),a0
	addq.b	#$1,6(a0)
	cmp.b	#$3,6(a0)
	bne.s	notthree
	clr.b	6(a0)
notthree:
	addq.b	#$1,4(a0)
	move.b	4(a0),d0
	cmp.b	1(a0),d0
	blt	doeffects
	clr.b	4(a0)
	clr.b	6(a0)

	lea	dma(pc),a5
	clr.w	(a5)

	lea	voice1(pc),a2
	bsr	getnote
	lea	voice2(pc),a2
	bsr	getnote
	lea	voice3(pc),a2
	bsr	getnote
	lea	voice4(pc),a2
	bsr	getnote

	move.w	(a5),$96(a6)
	add.w	#$8000,(a5)
	lea	voice1(pc),a2
	bsr	playvoice
	lea	voice2(pc),a2
	bsr	playvoice
	lea	voice3(pc),a2
	bsr	playvoice
	lea	voice4(pc),a2
	bsr	playvoice

	bsr	donegation

	move.b	$6(a6),d0
raster:	cmp.b	$6(a6),d0
	beq.s	raster

	move.w	(a5),$96(a6)

	move.b	$6(a6),d0
raster2:cmp.b	$6(a6),d0
	beq.s	raster2

	lea	voice1(pc),a2
	moveq	#$3,d0
repeatloop:
	move.w	16(a2),d4
	move.l	26(a2),(a6,d4.w)
	move.w	30(a2),4(a6,d4.w)
	lea	voice2-voice1(a2),a2
	dbf	d0,repeatloop

	addq.b	#$1,3(a0)
	move.b	5(a0),d0
	cmp.b	3(a0),d0
	bne.s	doeffects
	clr.b	3(a0)
	move.b	(a0),d0

	cmp.b	2(a0),d0
	bne.s	addlater
	move.b	#-$1,2(a0)
addlater:
	addq.b	#$1,2(a0)

	lea	voice1(pc),a2
	bsr	findnote
	lea	voice2(pc),a2
	bsr	findnote
	lea	voice3(pc),a2
	bsr	findnote
	lea	voice4(pc),a2
	bsr	findnote

doeffects:
	lea	voice1(pc),a2
	bsr	doeffect
	lea	voice2(pc),a2
	bsr	doeffect
	lea	voice3(pc),a2
	bsr	doeffect
	lea	voice4(pc),a2
	bsr	doeffect
	tst.b	4(a0)
	beq.s	nonega
	bsr.s	donegation
nonega:	movem.l	(a7)+,d0-d7/a0-a6
	rts

donegation:
	movem.l	d0-d4/a0-a3,-(a7)
	lea	waveadds(pc),a3
	lea	voice1(pc),a1
	moveq	#$3,d0
negationloop:
	move.w	72(a1),d1
	lsl.w	#$6,d1
	move.l	sampletab(pc),a0
	lea	(a0,d1.w),a0
	move.l	a0,(a3)+
	tst.w	26(a0)
	bne.s	nonegation
	not.w	26(a0)
	tst.w	24(a0)
	beq.s	checknegation
	subq.w	#$1,24(a0)
	and.w	#$1f,24(a0)
	bra.s	nonegation

checknegation:
	move.w	14(a0),24(a0)
	move.w	16(a0),d4
	beq.s	nonegation
	move.l	(a0),a2
	moveq	#$0,d1
	moveq	#$0,d2
	move.w	10(a0),d1
	move.w	12(a0),d2
	add.l	d1,d1
	add.l	d2,d2
	subq.l	#$1,d2
	add.l	d1,a2
	add.l	20(a0),a2
	not.b	(a2)
	moveq	#$0,d3
	move.w	18(a0),d3
	ext.l	d3
	add.l	d3,20(a0)
	tst.l	20(a0)
	bmi.s	noright
	cmp.l	20(a0),d2
	bhs.s	nonegation
checkmode:
	cmp.w	#$1,d4
	bne.s	noleft
	clr.l	20(a0)
	bra.s	nonegation

noright:cmp.w	#$2,d4
	bne.s	noleft
	move.l	d2,20(a0)
	bra.s	nonegation

noleft:	neg.l	d3
	add.l	d3,20(a0)
	neg.w	18(a0)

nonegation:
	lea	voice2-voice1(a1),a1
	dbf	d0,negationloop

	sub.w	#$10,a3
	moveq	#$3,d0
joho:	move.l	(a3)+,a0
	clr.w	26(a0)
	dbf	d0,joho

	movem.l	(a7)+,d0-d4/a0-a3
	rts

findnote:
	moveq	#$0,d0
	moveq	#$0,d1
	moveq	#$0,d2
	move.b	2(a0),d0

	move.l	positions(pc),a1
	add.l	(a2),a1
	move.b	(a1,d0.w),d2
	add.w	d2,d2
	move.l	patternpointer(pc),a1
	move.w	(a1,d2.w),d2
	add.l	patterns(pc),d2
	move.l	d2,64(a2)

	move.l	ntransposes(pc),a1
	add.l	(a2),a1
	move.b	(a1,d0.w),71(a2)
	move.l	itransposes(pc),a1
	add.l	(a2),a1
	move.b	(a1,d0.w),57(a2)
	clr.b	69(a2)
	rts

getnote:move.l	64(a2),a1
	bsr.s	getnote2
	move.l	a1,64(a2)

	move.w	46(a2),d0
	beq.s	noteok
	move.w	14(a2),d1
	add.w	d1,(a5)
	add.b	71(a2),d0
	move.w	d0,46(a2)
noteok:	rts

getnote2:
	moveq	#$0,d1
	move.l	d1,46(a2)
	move.l	d1,50(a2)

	tst.b	69(a2)
	beq.s	readnote
	subq.b	#$1,69(a2)
	rts

readnote:
	move.b	(a1)+,d1
	beq.s	nonotebutslide
	bpl.s	simplenote
negativvalue:
	not.b	d1
	move.b	d1,69(a2)
	rts

simplenote:
	cmp.b	#$70,d1
	blt.s	simplenote2
	move.b	d1,51(a2)
	move.b	(a1)+,53(a2)
	rts

simplenote2:
	move.b	d1,47(a2)
	move.b	(a1)+,d1
	bmi.s	negativvalue
	cmp.b	#$70,d1
	blt.s	simpleins
	move.b	d1,51(a2)
	move.b	(a1)+,53(a2)
	rts

simpleins:
	move.b	d1,49(a2)
	move.b	(a1)+,d1
	bmi.s	negativvalue
	move.b	d1,51(a2)
	move.b	(a1)+,53(a2)
	rts

nonotebutslide:
	move.b	(a1)+,51(a2)
	move.b	(a1)+,53(a2)
	rts

playvoice:
	clr.w	58(a2)

	move.w	46(a2),d0
	beq	nonote

	clr.w	12(a2)
	clr.l	34(a2)
	clr.w	38(a2)
	clr.l	40(a2)
	clr.w	44(a2)
	clr.w	54(a2)
	clr.w	62(a2)

	move.w	#$4,18(a2)
	clr.w	20(a2)

	moveq	#$0,d1
	move.w	48(a2),d1
	beq.s	noinschange
	subq.b	#$1,d1
	add.b	57(a2),d1
	lsl.w	#$5,d1
	move.l	ins1(pc),a1
	add.l	d1,a1
	move.l	a1,22(a2)
	moveq	#$0,d5
	move.b	(a1),d5
	lsl.w	#$4,d5
	move.l	wavelists(pc),a1
	add.l	d5,a1
	moveq	#$0,d5
	move.b	(a1),d5
	move.b	d5,73(a2)
	lsl.w	#$6,d5
	move.l	sampletab(pc),a1
	add.l	d5,a1
	move.l	(a1)+,4(a2)
	move.w	(a1)+,8(a2)
	move.l	4(a2),26(a2)
	moveq	#$0,d5
	move.w	(a1)+,d5
	add.l	d5,d5
	add.l	d5,26(a2)
	move.w	(a1),30(a2)
noinschange:
	move.l	22(a2),a1
	moveq	#$0,d5
	move.b	4(a1),d5
	lsl.w	#$4,d5
	move.l	arpeggiolists(pc),a1
	moveq	#$0,d1
	move.b	(a1,d5.w),d1
	ext.w	d1
	add.w	d1,d0
	move.w	d0,32(a2)
	lea	playperiods(pc),a3
	add.w	d0,d0
	move.w	16(a2),d4
	move.w	(a3,d0.w),10(a2)
	move.l	4(a2),(a6,d4.w)
	move.w	8(a2),4(a6,d4.w)
	move.w	10(a2),6(a6,d4.w)
nonote:	rts

doeffect:
	move.w	16(a2),d4
	bsr	doadsrcurve
	bsr	dowaveform
	bsr	doarpeggio
	bsr.s	dosoundtracker
	bsr	dovibrato
	bsr.s	dopitchbend
	bsr	donoteslide

	move.w	58(a2),d0
	add.w	d0,10(a2)

	cmp.w	#$5f,10(a2)
	bgt.s	notlow
	move.w	#$5f,10(a2)
	move.w	10(a2),6(a6,d4.w)
	rts

notlow:	cmp.w	#$1680,10(a2)
	blt.s	pitchok
	move.w	#$1680,10(a2)
pitchok:move.w	10(a2),6(a6,d4.w)
	rts

dopitchbend:
	move.l	22(a2),a4
	moveq	#$0,d0
	move.b	12(a4),d0
	beq.s	nopitch
	move.b	13(a4),d1
	cmp.b	55(a2),d1
	bne.s	pitchdelay
	ext.w	d0
	add.w	d0,58(a2)
nopitch:rts

pitchdelay:
	addq.b	#$1,55(a2)
	rts

dosoundtracker:
	move.w	50(a2),d0
	cmp.w	#$70,d0
	blt.s	noarp
	and.w	#$f,d0
	tst.b	4(a0)
	bne.s	egal
	cmp.b	#$5,d0
	blt.s	noarp
egal:	add.w	d0,d0
	lea	steffect(pc),a1
	move.w	(a1,d0.w),d0
	lea	arpeggio(pc),a1
	jmp	(a1,d0.w)
noarp:	rts

steffect:
	dc.w	arpeggio-arpeggio
	dc.w	pitchup-arpeggio
	dc.w	pitchdown-arpeggio
	dc.w	volumeup-arpeggio
	dc.w	volumedown-arpeggio
	dc.w	setadsrattack-arpeggio
	dc.w	setpatternlen-arpeggio
	dc.w	novolchange-arpeggio
	dc.w	novolchange-arpeggio
	dc.w	novolchange-arpeggio
	dc.w	novolchange-arpeggio
	dc.w	novolchange-arpeggio
	dc.w	volumechange-arpeggio
	dc.w	novolchange-arpeggio
	dc.w	novolchange-arpeggio
	dc.w	speedchange-arpeggio

donoteslide:
	move.w	50(a2),d0
	beq.s	nodestnote
	cmp.w	#$70,d0
	bge.s	nodestnote
	move.w	52(a2),d1
	beq.s	nodestnote
	add.w	d0,d0
	lea	playperiods(pc),a1
	move.w	(a1,d0.w),60(a2)
	move.w	60(a2),d0
	sub.w	10(a2),d0
	beq.s	noslider
	bpl.s	itshigher
	neg.w	d1
itshigher:
	move.w	d1,62(a2)
nodestnote:
	move.w	62(a2),d1
	beq.s	noslider
	bmi.s	downwithit
	add.w	d1,10(a2)
	move.w	10(a2),d0
	cmp.w	60(a2),d0
	blt.s	noslider
	clr.w	62(a2)
	move.w	60(a2),10(a2)
	rts

downwithit:
	add.w	d1,10(a2)
	move.w	10(a2),d0
	cmp.w	60(a2),d0
	bgt.s	noslider
	clr.w	62(a2)
	move.w	60(a2),10(a2)
noslider:
	rts

arpeggio:
	lea	myatab(pc),a1
	move.w	52(a2),d0
	move.b	d0,2(a1)
	and.b	#$f,2(a1)
	lsr.w	#$4,d0
	move.b	d0,(a1)
	move.b	6(a0),d0
	move.b	(a1,d0.w),d0
	add.w	32(a2),d0
	add.w	d0,d0
	lea	playperiods(pc),a1
	move.w	(a1,d0.w),d0
	move.w	d0,10(a2)
	rts

myatab:	dc.l	$0

pitchup:move.w	52(a2),d0
	neg.w	d0
	move.w	d0,58(a2)
	rts

pitchdown:
	move.w	52(a2),d0
	move.w	d0,58(a2)
	rts

volumeup:
	tst.w	18(a2)
	bne.s	novolchange
	tst.b	4(a0)
	bne.s	noinsset
	tst.w	48(a2)
	beq.s	noinsset
	move.l	22(a2),a4
	move.b	17(a4),13(a2)
noinsset:
	move.w	52(a2),d1
	add.w	d1,d1
	add.w	d1,d1
	move.w	12(a2),d0
	add.w	d1,d0
	cmp.w	#$100,d0
	blt.s	not256
	moveq	#$0,d0
	not.b	d0
not256:	move.w	d0,12(a2)
	rts

volumedown:
	tst.w	18(a2)
	bne.s	novolchange
	tst.b	4(a0)
	bne.s	noinsset2
	tst.w	48(a2)
	beq.s	noinsset2
	move.l	22(a2),a4
	move.b	17(a4),13(a2)
noinsset2:
	move.w	52(a2),d1
	add.w	d1,d1
	add.w	d1,d1
	move.w	12(a2),d0
	sub.w	d1,d0
	bpl.s	not00
	clr.w	d0
not00:	move.w	d0,12(a2)
novolchange:
	rts

setadsrattack:
	move.l	22(a2),a4
	move.w	52(a2),d0
	move.b	d0,16(a4)
	move.b	d0,17(a4)
	rts

setpatternlen:
	move.b	53(a2),5(a0)
	rts

volumechange:
	move.w	52(a2),d0
	move.w	d0,8(a6,d4.w)
	add.w	d0,d0
	add.w	d0,d0
	cmp.w	#$ff,d0
	blt.s	notff
	move.w	#$ff,d0
notff:	move.w	d0,12(a2)
	rts

speedchange:
	move.b	53(a2),d0
	and.b	#$f,d0
	beq.s	novolchange
	move.b	d0,1(a0)
	rts

dovibrato:
	move.l	22(a2),a4
	tst.b	9(a4)
	beq.s	long03

	move.b	11(a4),d6
	cmp.b	43(a2),d6
	beq.s	novdelay
	addq.b	#$1,43(a2)
long03:	rts

novdelay:
	move.b	10(a4),d7
	sub.b	d7,d6
	move.b	d6,43(a2)
	move.b	9(a4),d6
	cmp.b	45(a2),d6
	bne.s	notvsame
	move.b	#-$1,45(a2)
notvsame:
	addq.b	#$1,45(a2)
	clr.w	d7
	move.w	44(a2),d6
	move.b	8(a4),d7
	lsl.w	#$4,d7
	add.w	d6,d7
	move.l	vibratolists(pc),a4
	move.b	(a4,d7.w),d6
	ext.w	d6
	add.w	d6,10(a2)
	rts

doarpeggio:
	move.l	22(a2),a4
	tst.b	5(a4)
	beq.s	long02

	move.b	7(a4),d6
	cmp.b	39(a2),d6
	beq.s	noadelay
	addq.b	#$1,39(a2)
long02:	rts

noadelay:
	move.b	6(a4),d7
	sub.b	d7,d6
	move.b	d6,39(a2)
	clr.w	d6
	move.b	5(a4),d6
	cmp.b	41(a2),d6
	bne.s	notasame
	move.b	#-$1,41(a2)
notasame:
	addq.b	#$1,41(a2)
	move.w	40(a2),d6
	clr.w	d7
	move.b	4(a4),d7
	lsl.w	#$4,d7
	add.w	d6,d7
	move.l	arpeggiolists(pc),a4
	move.b	(a4,d7.w),d6
	ext.w	d6
	add.w	32(a2),d6
	lea	playperiods(pc),a4
	add.w	d6,d6
	move.w	(a4,d6.w),10(a2)
	rts

dowaveform:
	move.l	22(a2),a4
	tst.b	1(a4)
	beq.s	long0

	move.b	3(a4),d6
	cmp.b	35(a2),d6
	beq.s	nowdelay
	addq.b	#$1,35(a2)
long0:	rts

nowdelay:
	move.b	2(a4),d7
	sub.b	d7,d6
	move.b	d6,35(a2)
	move.b	1(a4),d6
	cmp.b	37(a2),d6
	bne.s	notsame
	move.b	#-$1,37(a2)
notsame:addq.b	#$1,37(a2)
	move.w	36(a2),d6
	clr.w	d7
	move.b	(a4),d7
	lsl.w	#$4,d7
	add.w	d6,d7
	moveq	#$0,d6
	move.l	wavelists(pc),a4
	move.b	(a4,d7.w),d6
	bpl.s	allwaveok
	subq.b	#$1,37(a2)
	rts

allwaveok:
	move.b	d6,73(a2)
	lsl.w	#$6,d6
	move.l	sampletab(pc),a4
	add.l	d6,a4
	move.l	(a4)+,26(a2)
	move.w	(a4),30(a2)
	move.l	26(a2),(a6,d4.w)
	move.w	30(a2),4(a6,d4.w)
	rts

doadsrcurve:
	bsr.s	doadsrcalc
	move.w	12(a2),d0
	lsr.w	#$2,d0
	move.w	d0,8(a6,d4.w)
	rts 

doadsrcalc:
	move.l	22(a2),a4
	lea	$10(a4),a4

	tst.w	18(a2)
	beq.s	noadsr
	clr.w	d6
	clr.w	d7
	cmp.w	#$4,18(a2)
	beq.s	attack
	cmp.w	#$3,18(a2)
	beq.s	decay
	cmp.w	#$2,18(a2)
	beq.s	sustain
	cmp.w	#$1,18(a2)
	beq.s	release
noadsr:	rts

attack:	move.b	(a4),d6
	move.b	1(a4),d7
	add.w	d7,12(a2)
	cmp.w	12(a2),d6
	bgt.s	returnadsr
	move.w	d6,12(a2)
	subq.w	#$1,18(a2)
	rts

decay:	move.b	2(a4),d6
	move.b	3(a4),d7
	beq.s	nodecay
	sub.w	d7,12(a2)
	cmp.w	12(a2),d6
	blt.s	returnadsr
	move.w	d6,12(a2)
nodecay:subq.w	#$1,18(a2)
	rts

sustain:move.b	4(a4),d6
	cmp.w	20(a2),d6
	bne.s	contsustain
	subq.w	#$1,18(a2)
	rts

release:move.b	5(a4),d6
	move.b	6(a4),d7
	beq.s	norelease
	sub.w	d7,12(a2)
	cmp.w	12(a2),d6
	blt.s	returnadsr
	move.w	d6,12(a2)
norelease:
	subq.w	#$1,18(a2)
returnadsr:
	rts

contsustain:
	addq.w	#$1,20(a2)
	rts

dma:	dc.w	$0

voice1:	dc.l	$0	; pos. and trans. offset
	dc.l	$0	; samplestard
	dc.w	$0	; samplelength
	dc.w	$0	; sampleperiod
	dc.w	$0	; samplevolume
	dc.w	$1	; dma-enable bit
	dc.w	$a0	; channel-register offset
	dc.w	$0	; adsr-status
	dc.w	$0	; counter for sustain
	dc.l	$0	; inst. address
	dc.l	$0	; repeatstart
	dc.w	$0	; repeatlength
	dc.w	$0	; original-note
	dc.w	$0	; counter for wavelist delay
	dc.w	$0	; counter for wavelist offset
	dc.w	$0	; counter for arpeggio delay
	dc.w	$0	; counter for arpeggio offset
	dc.w	$0	; counter for vibrato delay
	dc.w	$0	; counter for vibrato offcet
	dc.w	$0	; current-note
	dc.w	$0	; current-ins.
	dc.w	$0	; current-fx
	dc.w	$0	; current-fx-info
	dc.w	$0	; counter for pitchbend
	dc.w	$0	; inst. transp
	dc.w	$0	; pitchbend-value
	dc.w	$0	; note-slide-note
	dc.w	$0	; note-slide-speed
	dc.l	$0	; note-address
	dc.w	$0	; empty-notes-counter
	dc.w	$0	; note-transpose
	dc.w	$0	; current-waveform used

voice2:	dc.l	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$2
	dc.w	$b0
	dc.w	$0
	dc.w	$0
	dc.l	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0

voice3:	dc.l	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$4
	dc.w	$c0
	dc.w	$0
	dc.w	$0
	dc.l	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0

voice4:	dc.l	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$8
	dc.w	$d0
	dc.w	$0
	dc.w	$0
	dc.l	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0
	dc.l	$0
	dc.w	$0
	dc.w	$0
	dc.w	$0

playperiods:
	dc.w	$0
	dc.w	$1680,$1530,$1400,$12e0,$11d0,$10d0,$fe0,$f00,$e20,$d60,$ca0,$be8
	dc.w	$b40,$a98,$a00,$970,$8e8,$868,$7f0,$780,$710,$6b0,$650,$5f4
	dc.w	$5a0,$54c,$500,$4b8,$474,$434,$3f8,$3c0,$388,$358,$328,$2fa
	dc.w	$2d0,$2a6,$280,$25c,$23a,$21a,$1fc,$1e0,$1c5,$1ac,$194,$17d
	dc.w	$168,$153,$140,$12e,$11d,$10d,$fe,$f0,$e2,$d6,$ca,$be
	dc.w	$b4,$aa,$a0,$97,$8f,$87,$7f,$78,$71,$06b,$065,$05f

waveadds:	ds.l	4,0

song:		dc.l	muzakmodule
sampleno:	dc.w	0
midimode:	dc.w	0
length:		dc.b	0
speed:		dc.b	0
currentpos:	dc.b	0
currentnot:	dc.b	0
currentrast:	dc.b	0
patlength:	dc.b	0
currentrast2:	dc.b	0

		even
muzakmodule:
