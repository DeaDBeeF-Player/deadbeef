;		нннннннннннннннннннннннннннннннннннннннннннн
;		      pumatracker v0.99 replay-routine
;		нннннннннннннннннннннннннннннннннннннннннннн
;
;		that routine was ripped from toki (ocean), resourced,
;	improved, and optimized by flyspy/agile. use it in your programs...
;	these kinda modules can be useful for intros coz' a module can't exceed
;	8kb and that replay-routine is fuck'n fast.
;


; **************************** demonstration irq ******************************
	include "lib/org.s"

	bra	begin
	bra	mt_end
  	bra	play

_mt_data:	dc.l	0

begin:
	bsr	mt_init
	move.w	#$7fff,$dff09a
	move.w	#$7fff,$dff096
	move.w	#$c020,$dff09a
	move.w	#$8200,$dff096
	rts


play:	bsr	mt_music
;	move.w	#$20,$dff09c
	rts

mt_init:
	move.l	a0,a4
	move.l	a0,_mt_data

;	lea	mt_data,a4
	lea	musicdata(pc),a2
	lea	mt_voice1(pc),a0
	bclr	#2,$22(a0)
	move.b	#1,$12(a0)
	lea	mt_voice2(pc),a0
	bclr	#2,$22(a0)
	move.b	#1,$12(a0)
	lea	mt_voice3(pc),a0
	bclr	#2,$22(a0)
	move.b	#1,$12(a0)
	lea	mt_voice4(pc),a0
	bclr	#2,$22(a0)
	move.b	#1,$12(a0)
	move.l	a4,(a2)
	move.w	12(a4),d0
	addq.w	#1,d0
	move.w	d0,$134(a2)
	st 	$132(a2)
	move.b	#1,$130(a2)
	move.b	#$20,$131(a2)
	moveq	#9,d0
	lea	$3c(a4),a0
	lea	musicdata9(pc),a1
	lea	$14(a4),a6
	lea	musicdata8(pc),a3
lbc005f20:
	move.w	(a0)+,(a1)+
	move.l	(a6)+,a5
	add.l	musicdata(pc),a5
	move.l	a5,(a3)+
	dbra	d0,lbc005f20

	moveq	#$29,d0
	lea	musicdata10(pc),a5
lbc005f38:
	move.l	a5,(a3)+
	lea	$20(a5),a5
	dbra	d0,lbc005f38

	move.w	14(a4),d0
	lea	$50(a4),a0
	move.w	12(a4),d1
	addq.w	#1,d1
	mulu	#14,d1
	subq.w	#4,d1
	add.w	d1,a0
	lea	musicdata7(pc),a1
	move.l	#$70617474,d2
	addq.w	#4,a0
lbc005f66:
	cmp.l	(a0)+,d2
	bne.s	lbc005f66

	move.l	a0,(a1)+
	dbra	d0,lbc005f66

	lea	musicdata6(pc),a1
	move.l	#$696e7374,d2
	move.l	#$696e7366,d1
	move.w	$10(a4),d0
	subq.w	#1,d0
	subq.w	#2,a0
lbc005f8a:
	addq.w	#2,a0
	cmp.l	(a0),d2
	bne.s	lbc005f8a

	addq.w	#4,a0
	move.l	a0,(a1)+
	subq.w	#2,a0
lbc005f96:
	addq.w	#2,a0
	cmp.l	(a0),d1
	bne.s	lbc005f96

	addq.w	#4,a0
	move.l	a0,(a1)+
	dbra	d0,lbc005f8a

	lea	musicdata1(pc),a6
	move.b	#$40,$15(a6)
	clr.w	$38(a6)
	rts

mt_music:
	lea	musicdata(pc),a2
	move.w	$13c(a2),d0
	bne.s	lbc005fc8

	move.w	#$8000,d5
	bra.w	lbc0061b2
lbc005fc8:
	cmp.b	#$20,$131(a2)
	bne.w	lbc0060c2

	addq.b	#1,$132(a2)
	move.w	$134(a2),d0
	cmp.b	$132(a2),d0
	bne.s	lbc005fe4

	sf	$132(a2)
lbc005fe4:
	sf	$131(a2)
	moveq	#0,d0
	move.b	musicdata2(pc),d0
	add.w	d0,d0
	move.w	d0,d1
	lsl.w	#3,d0
	sub.w	d1,d0
	move.l	musicdata(pc),a5
	lea	$50(a5),a5
	add.w	d0,a5
	lea	musicdata7(pc),a4
	lea	mt_voice1(pc),a6
	or.b	#1,$14(a6)
	moveq	#0,d1
	move.b	(a5)+,d1
	add.w	d1,d1
	add.w	d1,d1
	move.l	(a4,d1.w),14(a6)
	subq.l	#4,14(a6)
	move.b	(a5)+,$18(a6)
	move.b	(a5)+,$19(a6)
	move.b	#1,$12(a6)
	lea	mt_voice2(pc),a6
	or.b	#1,$14(a6)
	moveq	#0,d1
	move.b	(a5)+,d1
	add.w	d1,d1
	add.w	d1,d1
	move.l	(a4,d1.w),14(a6)
	subq.l	#4,14(a6)
	move.b	(a5)+,$18(a6)
	move.b	(a5)+,$19(a6)
	move.b	#1,$12(a6)
	lea	mt_voice3(pc),a6
	or.b	#1,$14(a6)
	moveq	#0,d1
	move.b	(a5)+,d1
	add.w	d1,d1
	add.w	d1,d1
	move.l	(a4,d1.w),14(a6)
	subq.l	#4,14(a6)
	move.b	(a5)+,$18(a6)
	move.b	(a5)+,$19(a6)
	move.b	#1,$12(a6)
	lea	mt_voice4(pc),a6
	or.b	#1,$14(a6)
	moveq	#0,d1
	move.b	(a5)+,d1
	add.w	d1,d1
	add.w	d1,d1
	move.l	(a4,d1.w),14(a6)
	subq.l	#4,14(a6)
	move.b	(a5)+,$18(a6)
	move.b	(a5)+,$19(a6)
	move.b	#1,$12(a6)
	move.b	(a5),d1
	beq.s	lbc0060c2

	move.l	_mt_data(pc),a1
	lea	$315a(a1),a1
	move.b	d1,(a1)
lbc0060c2:
	lea	musicdata4(pc),a1
	subq.b	#1,$130(a2)
	bne.b	lbc006140

	addq.b	#1,$131(a2)
	move.l	_mt_data(pc),a6
	lea	$315a(a6),a6
	move.b	(a6),$130(a2)
	lea	mt_voice1(pc),a6
	subq.b	#1,$12(a6)
	bne.s	lbc0060f2

	addq.l	#4,14(a6)
	and.b	#$df,$14(a6)
	moveq	#1,d1
	bsr.w	mt_read_sample
lbc0060f2:
	lea	mt_voice2(pc),a6
	subq.b	#1,$12(a6)
	bne.s	lbc00610c

	addq.l	#4,14(a6)
	and.b	#$df,$14(a6)
	moveq	#2,d1
	bsr.w	mt_read_sample
lbc00610c:
	lea	mt_voice3(pc),a6
	subq.b	#1,$12(a6)
	bne.s	lbc006126

	addq.l	#4,14(a6)
	and.b	#$df,$14(a6)
	moveq	#4,d1
	bsr.w	mt_read_sample
lbc006126:
	lea	mt_voice4(pc),a6
	subq.b	#1,$12(a6)
	bne.s	lbc006140

	addq.l	#4,14(a6)
	and.b	#$df,$14(a6)
	moveq	#8,d1
	bsr.w	mt_read_sample
lbc006140:
	move.w	#$8000,d5
	lea	mt_voice1(pc),a6
	move.b	$22(a6),d7
	btst	#2,d7
	beq.s	lbc00615e

	addq.b	#1,d5
	bsr.w	lbc006330
	move.b	d7,$22(a6)
lbc00615e:
	lea	mt_voice2(pc),a6
	move.b	$22(a6),d7
	btst	#2,d7
	beq.s	lbc006178

	addq.b	#2,d5
	bsr.w	lbc006330
	move.b	d7,$22(a6)
lbc006178:
	lea	mt_voice3(pc),a6
	move.b	$22(a6),d7
	btst	#2,d7
	beq.s	lbc006192

	addq.b	#4,d5
	bsr.w	lbc006330
	move.b	d7,$22(a6)
lbc006192:
	move.w	$136(a2),d6
	bne.s	lbc0061b2

	lea	mt_voice4(pc),a6
	move.b	$22(a6),d7
	btst	#2,d7
	beq.s	lbc0061b2

	addq.b	#8,d5
	bsr.w	lbc006330
	move.b	d7,$22(a6)

lbc0061b2:
	move.w	$13a(a2),d0
	beq.s	lbc00620a

	move.w	$136(a2),d0
	beq.s	lbc006222

	lea	musicdata1(pc),a6
	move.w	$138(a2),d1
	beq.s	lbc006200

	clr.w	$138(a2)
	add.w	d0,d0
	move.w	d0,d1
	add.w	d0,d0
	add.w	d0,d0
	add.w	d1,d0
	ext.l	d0
	lea	musicdata11(pc),a0
	add.l	d0,a0
	move.w	(a0)+,d0
	move.b	d0,$3a(a6)
	move.l	(a0)+,$1a(a6)
	move.l	(a0),$1e(a6)
	move.l	#$7000000,$22(a6)
	move.w	#8,$dff096
lbc006200:
	move.b	$22(a6),d7
	btst	#2,d7
	bne.s	lbc006210

lbc00620a:
	clr.w	$136(a2)
	bra.s	lbc006222
lbc006210:
	lea	musicdata4(pc),a1
	bsr.w	lbc006330
	move.b	d7,$22(a6)
	or.w	#8,d5
lbc006222:
	lea	$562(a2),a6
	bra.s	lbc00623e
	beq.s	lbc006236

	subq.l	#1,(a6)+
	move.l	(a6),a1
	move.l	(a1)+,d0
	move.l	a1,(a6)+
	sub.l	d0,(a6)
	bra.s	lbc00623e
lbc006236:
	move.l	8(a6),d0
	add.l	d0,$55e(a2)
lbc00623e:
	move.b	$dff006,d0
;	addq.b	#1,d0
lbc006246:
;	cmp.b	$dff006,d0
;	bne.s	lbc006246

	moveq	#15,d0
	and.b	d5,d0
	not.w	d0
	and.w	#15,d0
	tst.w	musicdata3
	beq.s	lbc006274

	move.w	d0,$dff096
	move.w	d5,$dff096
	clr.b	start_sign
	rts

lbc006274:
	tst.b	start_sign
	bne.s	lbc006290

	st 	start_sign
	move.w	d0,$dff096
	move.w	d5,$dff096
	rts

lbc006290:
	and.w	#8,d0
	and.w	#$8008,d5
	move.w	d0,$dff096
	move.w	d5,$dff096
	rts

mt_read_sample:
	clr.w	d0
	move.l	14(a6),a0
	move.b	(a0)+,d0
	beq.s	lbc0062de

	move.w	d1,$dff096
	add.b	$19(a6),d0
	move.b	d0,$3a(a6)
	move.b	(a0),d0
	add.b	$18(a6),d0
	lsl.b	#3,d0
	lea	musicdata5(pc),a5
	add.w	d0,a5
	move.l	(a5)+,$1a(a6)
	move.l	(a5),$1e(a6)
	move.l	#$7000000,$22(a6)
lbc0062de:
	move.b	2(a0),$12(a6)
	moveq	#-$20,d0
	and.b	(a0)+,d0
	bne.s	lbc0062f6

	move.b	#$40,$15(a6)
	clr.w	$38(a6)
	rts

lbc0062f6:
	cmp.b	#$60,d0
	bne.s	lbc00630e

	moveq	#0,d0
	move.b	(a0),d0
	neg.w	d0
	move.w	d0,$38(a6)
	move.b	#$40,$15(a6)
	rts

lbc00630e:
	cmp.b	#$40,d0
	bne.s	lbc006326

	move.b	(a0),d0
	and.w	#$ff,d0
	move.w	d0,$38(a6)
	move.b	#$40,$15(a6)
	rts

lbc006326:
	move.b	(a0),$15(a6)
	clr.w	$38(a6)
	rts

lbc006330:
	clr.w	d6
	move.b	$24(a6),d6
lbc006336:
	move.l	$1a(a6),a0
	add.w	d6,a0
	move.b	(a0)+,d1
	cmp.b	#$a0,d1
	beq.s	lbc00639e

	cmp.b	#$c0,d1
	beq.s	lbc006360

	cmp.b	#$b0,d1
	beq.s	lbc00635c

	clr.w	12(a6)
	bclr	#2,d7
	bra.w	lbc006404
lbc00635c:
	move.b	(a0),d6
	bra.s	lbc006336
lbc006360:
	clr.w	d1
	move.b	(a0)+,d1
	lea	$34(a6),a5
	move.b	(a0),(a5)+
	move.b	(a0)+,(a5)+
	move.b	(a0),(a5)+
	add.w	d1,d1
	lea	musicdata9(pc),a0
	move.w	0(a0,d1.w),d2
	move.w	d2,8(a6)
	cmp.w	#$50,d2
	bmi.s	lbc006388

	bset	#3,d7
lbc006388:
	add.w	d1,d1
	lea	musicdata8(pc),a0
	move.l	(a0,d1.w),4(a6)
	addq.b	#4,d6
	bset	#0,d7
	bra.s	lbc006336
lbc00639e:
	bclr	#0,d7
	beq.s	lbc0063d8

	move.b	(a0)+,d1
	move.b	(a0)+,d2
	move.b	(a0),$26(a6)
	addq.b	#1,$26(a6)
	move.b	d1,13(a6)
	sf	$27(a6)
	move.b	#1,$2a(a6)
	sub.b	d1,d2
	bcc.s	lbc0063c8

	neg.b	d2
	st 	$2a(a6)
lbc0063c8:
	move.b	d2,$28(a6)
	bra.s	lbc006404
lbc0063ce:
	addq.b	#4,d6
	bset	#0,d7
	bra.w	lbc006336
lbc0063d8:
	subq.b	#1,$26(a6)
	beq.s	lbc0063ce

	move.b	2(a0),d4
	move.b	$27(a6),d1
	add.b	$28(a6),d1
	sub.b	d4,d1
	bmi.s	lbc0063fe

	clr.w	d2
	move.b	$2a(a6),d3
lbc0063f4:
	add.b	d3,d2
	sub.b	d4,d1
	bpl.s	lbc0063f4

	add.b	d2,13(a6)
lbc0063fe:
	add.b	d4,d1
	move.b	d1,$27(a6)
lbc006404:
	move.b	d6,$24(a6)
	move.b	$25(a6),d6
lbc00640c:
	move.l	$1e(a6),a0
	add.w	d6,a0
	move.b	(a0)+,d1
	cmp.b	#$a0,d1
	beq.s	lbc006458

	cmp.b	#$d0,d1
	beq.s	lbc00642c

	cmp.b	#$b0,d1
	bne.w	lbc0064ca

	move.b	(a0),d6
	bra.s	lbc00640c
lbc00642c:
	bclr	#1,d7
	bne.s	lbc006442

	subq.b	#1,$29(a6)
	bne.w	lbc0064ca

	bset	#1,d7
	addq.b	#4,d6
	bra.s	lbc00640c
lbc006442:
	clr.w	d1
	move.b	(a0),d1
	move.b	2(a0),$29(a6)
	add.b	$3a(a6),d1
	move.w	(a1,d1.w),10(a6)
	bra.s	lbc0064ca
lbc006458:
	bclr	#1,d7
	beq.s	lbc0064b6

	clr.w	d0
	move.b	(a0)+,d1
	move.b	(a0)+,d2
	move.b	(a0),$29(a6)
	move.b	$3a(a6),d0
	move.w	0(a1,d0.w),d0
	ext.w	d1
	ext.w	d2
	add.w	d0,d1
	add.w	d0,d2
	move.w	d1,10(a6)
	move.w	d1,$2c(a6)
	clr.w	$2e(a6)
	sub.w	d1,d2
	clr.w	d1
	move.b	$29(a6),d1
	ext.l	d2
	asl.l	#8,d2
	divs	d1,d2
	bvs.s	lbc00649e

	ext.l	d2
	asl.l	#8,d2
	move.l	d2,$30(a6)
	bra.s	lbc0064ca
lbc00649e:
	asr.l	#8,d2
	divs	d1,d2
	swap	d2
	clr.w	d2
	move.l	d2,$30(a6)
	bra.s	lbc0064ca
lbc0064ac:
	bset	#1,d7
	addq.b	#4,d6
	bra.w	lbc00640c
lbc0064b6:
	subq.b	#1,$29(a6)
	beq.s	lbc0064ac

	move.l	$30(a6),d1
	add.l	d1,$2c(a6)
	move.w	$2c(a6),10(a6)
lbc0064ca:
	move.b	d6,$25(a6)
	move.w	$38(a6),d1
	add.w	d1,10(a6)
	lea	(a6),a0
	move.l	(a0)+,a5
	move.l	(a0)+,a3
	move.b	$34(a6),d0
	beq.s	lbc006502

	move.b	$35(a6),d1
	ble.s	lbc0064f0

	cmp.b	$36(a6),d1
	bmi.s	lbc0064f6

lbc0064f0:
	neg.b	d0
	move.b	d0,$34(a6)
lbc0064f6:
	add.b	d0,d1
	move.b	d1,$35(a6)
	ext.w	d1
	lsl.w	#5,d1
	add.w	d1,a3
lbc006502:
	move.l	a3,(a5)+
	move.l	(a0),(a5)+
	bclr	#3,d7
	beq.s	lbc006510

	move.w	#1,(a0)
lbc006510:
	addq.l	#4,a0
	move.w	(a0),d6
	add.b	$15(a6),d6
	sub.b	#$40,d6
	bpl.s	lbc006520

	clr.w	d6
lbc006520:
	move.w	d6,(a5)
	rts

mt_end:	move.w	#$f,$dff096
	rts

;	*******************
;	*** replay data ***
;	*******************

start_sign:	dc.w	0
musicdata:	dc.l	0
mt_voice1:	dc.l	$dff0a0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
mt_voice2:	dc.l	$dff0b0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
mt_voice3:	dc.l	$dff0c0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
mt_voice4:	dc.l	$dff0d0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

musicdata1:
	dcb.w	31,0
musicdata2:
	dcb.w	5,0
musicdata3:
	dc.w	1
musicdata4:
	dc.l	$1ac0,$194017d0,$16801530,$140012e0,$11d010d0
	dc.l	$fe00f00,$e280d60,$ca00be8,$b400a98,$a000970
	dc.l	$8e80868,$7f00780,$71406b0,$65005f4,$5a0054c
	dc.l	$50004b8,$4740434,$3f803c0,$38a0358,$32802fa
	dc.l	$2d002a6,$280025c,$23a021a,$1fc01e0,$1c501ac
	dc.l	$194017d,$1680153,$140012e,$11d010d,$fe00f0,$e200d6
	dc.l	$ca00be,$b400aa,$a00097,$8f0087,$7f0078
	dc.w	$0071
musicdata5:
	dcb.w	4,0
musicdata6:
	dcb.w	132,0
musicdata7:
	dcb.w	160,0
musicdata8:
	dcb.w	104,0
musicdata9:
	dc.w	0,0,0,0,0,0,0,0,0

	dc.l	$10,$100010,$100010,$100010,$100010,$100010,$100010
	dcb.l	14,$100010
	dc.l	$100000,0,0,$539d0002,$9f32e2cb
	dc.w	$0053
musicdata10:
	dc.w	$c0c0,$d0d8,$e0e8
	dc.l	$f0f800f8,$f0e8e0d8,$d0c83f37,$2f271f17,$f07ff07
	dc.l	$f171f27,$2f37c0c0,$d0d8e0e8,$f0f800f8,$f0e8e0d8
	dc.l	$d0c8c037,$2f271f17,$f07ff07,$f171f27,$2f37c0c0
	dc.l	$d0d8e0e8,$f0f800f8,$f0e8e0d8,$d0c8c0b8,$2f271f17
	dc.l	$f07ff07,$f171f27,$2f37c0c0,$d0d8e0e8,$f0f800f8
	dc.l	$f0e8e0d8,$d0c8c0b8,$b0271f17,$f07ff07,$f171f27
	dc.l	$2f37c0c0,$d0d8e0e8,$f0f800f8,$f0e8e0d8,$d0c8c0b8
	dc.l	$b0a81f17,$f07ff07,$f171f27,$2f37c0c0,$d0d8e0e8
	dc.l	$f0f800f8,$f0e8e0d8,$d0c8c0b8,$b0a8a017,$f07ff07
	dc.l	$f171f27,$2f37c0c0,$d0d8e0e8,$f0f800f8,$f0e8e0d8
	dc.l	$d0c8c0b8,$b0a8a098,$f07ff07,$f171f27,$2f37c0c0
	dc.l	$d0d8e0e8,$f0f800f8,$f0e8e0d8,$d0c8c0b8,$b0a8a098
	dc.l	$9007ff07,$f171f27,$2f37c0c0,$d0d8e0e8,$f0f800f8
	dc.l	$f0e8e0d8,$d0c8c0b8,$b0a8a098,$9088ff07,$f171f27
	dc.l	$2f37c0c0,$d0d8e0e8,$f0f800f8,$f0e8e0d8,$d0c8c0b8
	dc.l	$b0a8a098,$90888007,$f171f27,$2f37c0c0,$d0d8e0e8
	dc.l	$f0f800f8,$f0e8e0d8,$d0c8c0b8,$b0a8a098,$90888088
	dc.l	$f171f27,$2f37c0c0,$d0d8e0e8,$f0f800f8,$f0e8e0d8
	dc.l	$d0c8c0b8,$b0a8a098,$90888088,$90171f27,$2f37c0c0
	dc.l	$d0d8e0e8,$f0f800f8,$f0e8e0d8,$d0c8c0b8,$b0a8a098
	dc.l	$90888088,$90981f27,$2f37c0c0,$d0d8e0e8,$f0f800f8
	dc.l	$f0e8e0d8,$d0c8c0b8,$b0a8a098,$90888088,$9098a027
	dc.l	$2f37c0c0,$d0d8e0e8,$f0f800f8,$f0e8e0d8,$d0c8c0b8
	dc.l	$b0a8a098,$90888088,$9098a0a8,$2f37c0c0,$d0d8e0e8
	dc.l	$f0f800f8,$f0e8e0d8,$d0c8c0b8,$b0a8a098,$90888088
	dc.l	$9098a0a8,$b0378181,$81818181,$81818181,$81818181
	dc.l	$81817f7f,$7f7f7f7f,$7f7f7f7f,$7f7f7f7f,$7f7f8181
	dcb.l	3,$81818181
	dc.l	$8181817f,$7f7f7f7f,$7f7f7f7f,$7f7f7f7f,$7f7f8181
	dcb.l	4,$81818181
	dcb.l	3,$7f7f7f7f
	dc.l	$7f7f8181,$81818181,$81818181,$81818181,$81818181
	dc.l	$817f7f7f,$7f7f7f7f,$7f7f7f7f,$7f7f8181,$81818181
	dcb.l	3,$81818181
	dc.l	$81817f7f,$7f7f7f7f,$7f7f7f7f,$7f7f8181,$81818181
	dcb.l	3,$81818181
	dc.l	$8181817f,$7f7f7f7f,$7f7f7f7f,$7f7f8181,$81818181
	dcb.l	4,$81818181
	dcb.l	2,$7f7f7f7f
	dc.l	$7f7f8181,$81818181,$81818181,$81818181,$81818181
	dc.l	$81818181,$817f7f7f,$7f7f7f7f,$7f7f8181,$81818181
	dcb.l	4,$81818181
	dc.l	$81817f7f,$7f7f7f7f,$7f7f8181,$81818181,$81818181
	dcb.l	3,$81818181
	dc.l	$8181817f,$7f7f7f7f,$7f7f8181,$81818181,$81818181
	dcb.l	4,$81818181
	dc.l	$7f7f7f7f,$7f7f8181,$81818181,$81818181,$81818181
	dcb.l	3,$81818181
	dc.l	$817f7f7f,$7f7f8181,$81818181,$81818181,$81818181
	dcb.l	3,$81818181
	dc.l	$81817f7f,$7f7f8181,$81818181,$81818181,$81818181
	dcb.l	3,$81818181
	dc.l	$8181817f,$7f7f8080,$80808080,$80808080,$80808080
	dcb.l	4,$80808080
	dc.l	$7f7f8080,$80808080,$80808080,$80808080,$80808080
	dcb.l	3,$80808080
	dc.l	$807f8080,$80808080,$80807f7f,$7f7f7f7f,$7f7f8080
	dc.l	$80808080,$807f7f7f,$7f7f7f7f,$7f7f8080,$80808080
	dcb.l	2,$7f7f7f7f
	dc.l	$7f7f8080,$8080807f,$7f7f7f7f,$7f7f7f7f,$7f7f8080
	dc.l	$80807f7f,$7f7f7f7f,$7f7f7f7f,$7f7f8080,$807f7f7f
	dcb.l	2,$7f7f7f7f
	dc.l	$7f7f8080,$7f7f7f7f,$7f7f7f7f,$7f7f7f7f,$7f7f8080
	dcb.l	3,$7f7f7f7f
	dc.l	$7f7f8080,$9098a0a8,$b0b8c0c8,$d0d8e0e8,$f0f80008
	dc.l	$10182028,$30384048,$50586068,$707f8080,$a0b0c0d0
	dc.l	$e0f00010,$20304050,$60704545,$797d7a77,$70666158
	dc.l	$534d2c20,$181204db,$d3cdc6bc,$b5aea8a3,$9d99938e
	dc.l	$8b8a4545,$797d7a77,$70665b4b,$43372c20,$181204f8
	dc.l	$e8dbcfc6,$beb0a8a4,$9e9a9594,$8d830000,$40607f60
	dc.l	$402000e0,$c0a080a0,$c0e00000,$40607f60,$402000e0
	dc.l	$c0a080a0,$c0e08080,$9098a0a8,$b0b8c0c8,$d0d8e0e8
	dc.l	$f0f80008,$10182028,$30384048,$50586068,$707f8080
	dc.l	$a0b0c0d0,$e0f00010,$20304050
	dc.w	$6070
musicdata11:
	dc.w	$0000,$0002,$6b70
	dc.l	$26b7a,$7a0002,$6b700002,$6b7a007e,$26b80,$26b8a
	dc.l	$760002,$6b940002,$6b9e0068,$26ba8,$26bbe,$300002
	dc.l	$6bcc0002,$6bd6008a,$26be0,$26bf2,$780002,$6bfc0002
	dc.l	$6c0a0078,$26c18,$26c22,$5e0002,$6c280002,$6c360058
	dc.l	$26c44,$26c62,$780002,$6c740002,$6c7e0048,$26c84
	dc.l	$26c9a,$8c0002,$6cc40002,$6cd2c01a,$4a028,$10e000
	dc.l	$a07f0010,$e000c01f,$a023,$8e000,$d0f80001
	dc.l	$a0f67f0a,$e000c01a,$10fa01e,$ae000,$a064ce02
	dc.l	$a0ce7808,$e000c01a,$fa028,$503a023,$205a014,$8a00a
	dc.l	$6e000,$a0813204,$a0007f03,$a07f0003,$b004c02f
	dc.l	$8a040,$4014e004,$a0140014,$a0007814,$b000c01c
	dc.l	$fa023,$1404a01e,$a04a00a,$8e000,$d0e80001
	dc.l	$d0000001,$e000c01c,$fa028,$1408a014,$14e000
	dc.l	$d0080003,$d0000003,$d00a0001,$e000c01c,$fa028
	dc.l	$1408e000,$d0080003,$e000c01c,$10fa01e,$3203a019
	dc.l	$8e000,$a0880004,$d0e80002,$d0000001,$b004c01c
	dc.l	$10fa028,$3c03a01e,$4a032,$4a014,$2a01e,$4a00a
	dc.l	$3e000,$d00c0002,$a07f8106,$d0f40002,$d0180001
	dc.l	$b008c01c,$fa028,$1408e000,$d0080003,$e000c01a
	dc.l	$fa028,$1404a014,$2806a01e,$a14a00a,$28e000
	dc.l	$d0080001,$a07f810a,$d0080002,$d00a0002,$d00c0002
	dc.l	$d00e0002,$d0120002,$d0180002,$d0200002,$d0260001
	dc.l	$e000c01a,$fa028,$1404a00a,$8e000,$d0000001
	dc.l	$d0020001,$e0000001,$80001612,$c050001,$5000130c
	dc.l	$16050001,$1000190e,$15020001,$190a,$d020000
	dc.l	$90001b18,$10020000,$85000a15,$f010000,$8000121f
	dc.l	$a010000,$75000e1b,$16010000,$70001c1d,$e010000
	dc.l	$6500101e,$12010000,$60001d0a,$1b010000,$55000b18
	dc.l	$1e010000,$5000130e,$a010000,$45000f0a,$b010000
	dc.l	$40001d0e,$18010000,$35000a1b,$16010000,$30000a1b
	dc.l	$12010000,$2500130a,$c010000,$2000190a,$1d010000
	dc.l	$1500131e
	dc.w	$1501


;mt_data:  incbin	"pumasix.cyf3final2"
