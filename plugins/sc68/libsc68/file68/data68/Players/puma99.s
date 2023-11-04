;;; sc68 wrapper for PUMA 0.99
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:09:18 ben>
;;; 
	
;	нннннннннннннннннннннннннннннннннннннннннннн
;      PumaTracker V0.99 Replay-Routine
;	нннннннннннннннннннннннннннннннннннннннннннн
;That routine was ripped from Toki (Ocean), resourced,
;improved, and optimized by Flyspy/Agile. Use it in your programs...
;These kinda modules can be useful for intros coz' a module can't exceed
;8Kb and that replay-routine is fuck'n fast.

;
; relocatable version by ben
;

; ----------------------------
; a0 = entry point music data
; ----------------------------
	bra	init
	bra	Mt_end
	bra	play

init:
	move.l	a0,a4
	lea	$315a(a0),a0
	lea	reloc(pc),a1
	move.l	a0,(a1)
	bra	Mt_init

;	Move.w	#$7fff,$dff09a
;	Move.w	#$7fff,$dff096
;	Move.l	#Irq,$6c.w
;	Move.w	#$c020,$dff09a
;	Move.w	#$8200,$dff096

play:
	Bsr.w	Mt_Music
	Move.w	#$20,$dff09c
	rts

; ************************* Real Replay-Routine ****************************

Mt_init:
;	Lea	Mt_Data,a4
	Lea	MusicData(pc),a2
	Lea	Mt_Voice1(pc),A0
	Bclr	#2,$22(A0)
	Move.B	#1,$12(A0)
	Lea	Mt_Voice2(pc),A0
	Bclr	#2,$22(A0)
	Move.B	#1,$12(A0)
	Lea	Mt_Voice3(pc),A0
	Bclr	#2,$22(A0)
	Move.B	#1,$12(A0)
	Lea	Mt_Voice4(pc),A0
	Bclr	#2,$22(A0)
	Move.B	#1,$12(A0)
	Move.L	A4,(a2)
	Move.W	12(A4),D0
	AddQ.W	#1,D0
	Move.W	D0,$134(a2)
	St 	$132(a2)
	Move.B	#1,$130(a2)
	Move.B	#$20,$131(a2)
	MoveQ	#9,D0
	Lea	$3C(A4),A0
	Lea	MusicData9(pc),A1
	Lea	$14(A4),A6
	Lea	MusicData8(pc),A3
lbC005F20:
	Move.W	(A0)+,(A1)+
	Move.L	(A6)+,A5
	Add.L	MusicData(pc),A5
	Move.L	A5,(A3)+
	Dbra	D0,lbC005F20

	MoveQ	#$29,D0
	Lea	MusicData10(pc),A5
lbC005F38:
	Move.L	A5,(A3)+
	Lea	$20(A5),A5
	Dbra	D0,lbC005F38

	Move.W	14(A4),D0
	Lea	$50(A4),A0
	Move.W	12(A4),D1
	AddQ.W	#1,D1
	Mulu	#14,D1
	SubQ.W	#4,D1
	Add.W	D1,A0
	Lea	MusicData7(pc),A1
	Move.L	#$70617474,D2
	AddQ.W	#4,A0
lbC005F66:
	Cmp.L	(A0)+,D2
	Bne.S	lbC005F66

	Move.L	A0,(A1)+
	Dbra	D0,lbC005F66

	Lea	MusicData6(pc),A1
	Move.L	#$696E7374,D2
	Move.L	#$696E7366,D1
	Move.W	$10(A4),D0
	SubQ.W	#1,D0
	SubQ.W	#2,A0
lbC005F8A:
	AddQ.W	#2,A0
	Cmp.L	(A0),D2
	Bne.S	lbC005F8A

	AddQ.W	#4,A0
	Move.L	A0,(A1)+
	SubQ.W	#2,A0
lbC005F96:
	AddQ.W	#2,A0
	Cmp.L	(A0),D1
	Bne.S	lbC005F96

	AddQ.W	#4,A0
	Move.L	A0,(A1)+
	Dbra	D0,lbC005F8A

	Lea	MusicData1(pc),A6
	Move.B	#$40,$15(A6)
	Clr.W	$38(A6)
	Rts

Mt_Music:
	Lea	MusicData(PC),A2
	Move.W	$13c(a2),d0
	Bne.S	lbC005FC8

	Move.W	#$8000,D5
	Bra.w	lbC0061B2
lbC005FC8:
	Cmp.B	#$20,$131(a2)
	Bne.w	lbC0060C2

	AddQ.B	#1,$132(a2)
	Move.W	$134(a2),d0
	Cmp.B	$132(a2),d0
	Bne.S	lbC005FE4

	Sf	$132(a2)
lbC005FE4:
	Sf	$131(a2)
	MoveQ	#0,D0
	Move.B	MusicData2(pc),D0
	Add.W	D0,D0
	Move.W	D0,D1
	Lsl.W	#3,D0
	Sub.W	D1,D0
	Move.L	MusicData(pc),A5
	Lea	$50(A5),A5
	Add.W	D0,A5
	Lea	MusicData7(pc),A4
	Lea	Mt_Voice1(pc),A6
	Or.B	#1,$14(A6)
	MoveQ	#0,D1
	Move.B	(A5)+,D1
	Add.W	D1,D1
	Add.W	D1,D1
	Move.L	(A4,D1.W),14(A6)
	SubQ.L	#4,14(A6)
	Move.B	(A5)+,$18(A6)
	Move.B	(A5)+,$19(A6)
	Move.B	#1,$12(A6)
	Lea	Mt_Voice2(pc),A6
	Or.B	#1,$14(A6)
	MoveQ	#0,D1
	Move.B	(A5)+,D1
	Add.W	D1,D1
	Add.W	D1,D1
	Move.L	(A4,D1.W),14(A6)
	SubQ.L	#4,14(A6)
	Move.B	(A5)+,$18(A6)
	Move.B	(A5)+,$19(A6)
	Move.B	#1,$12(A6)
	Lea	Mt_Voice3(pc),A6
	Or.B	#1,$14(A6)
	MoveQ	#0,D1
	Move.B	(A5)+,D1
	Add.W	D1,D1
	Add.W	D1,D1
	Move.L	(A4,D1.W),14(A6)
	SubQ.L	#4,14(A6)
	Move.B	(A5)+,$18(A6)
	Move.B	(A5)+,$19(A6)
	Move.B	#1,$12(A6)
	Lea	Mt_Voice4(pc),A6
	Or.B	#1,$14(A6)
	MoveQ	#0,D1
	Move.B	(A5)+,D1
	Add.W	D1,D1
	Add.W	D1,D1
	Move.L	(A4,D1.W),14(A6)
	SubQ.L	#4,14(A6)
	Move.B	(A5)+,$18(A6)
	Move.B	(A5)+,$19(A6)
	Move.B	#1,$12(A6)
	Move.B	(A5),D1
	Beq.S	lbC0060C2


	Lea	MusicData+$315a(pc),A1
	Move.B	D1,(a1)
lbC0060C2:
	Lea	MusicData4(pc),A1
	SubQ.B	#1,$130(a2)
	Bne.B	lbC006140

	AddQ.B	#1,$131(a2)
reloc:	= *+2
;	Move.B	Mt_Data+$315a(pc),$130(a2)
	Move.B	$12345678,$130(a2)
	Lea	Mt_Voice1(pc),A6
	SubQ.B	#1,$12(A6)
	Bne.S	lbC0060F2

	AddQ.L	#4,14(A6)
	And.B	#$DF,$14(A6)
	MoveQ	#1,D1
	Bsr.w	Mt_read_sample
lbC0060F2:
	Lea	Mt_Voice2(pc),A6
	SubQ.B	#1,$12(A6)
	Bne.S	lbC00610C

	AddQ.L	#4,14(A6)
	And.B	#$DF,$14(A6)
	MoveQ	#2,D1
	Bsr.w	Mt_read_sample
lbC00610C:
	Lea	Mt_Voice3(pc),A6
	SubQ.B	#1,$12(A6)
	Bne.S	lbC006126

	AddQ.L	#4,14(A6)
	And.B	#$DF,$14(A6)
	MoveQ	#4,D1
	Bsr.w	Mt_read_sample
lbC006126:
	Lea	Mt_Voice4(pc),A6
	SubQ.B	#1,$12(A6)
	Bne.S	lbC006140

	AddQ.L	#4,14(A6)
	And.B	#$DF,$14(A6)
	MoveQ	#8,D1
	Bsr.w	Mt_read_sample
lbC006140:
	Move.W	#$8000,D5
	Lea	Mt_Voice1(pc),A6
	Move.B	$22(A6),D7
	Btst	#2,D7
	Beq.S	lbC00615E

	AddQ.B	#1,D5
	Bsr.w	lbC006330
	Move.B	D7,$22(A6)
lbC00615E:
	Lea	Mt_Voice2(pc),A6
	Move.B	$22(A6),D7
	Btst	#2,D7
	Beq.S	lbC006178

	AddQ.B	#2,D5
	Bsr.w	lbC006330
	Move.B	D7,$22(A6)
lbC006178:
	Lea	Mt_Voice3(pc),A6
	Move.B	$22(A6),D7
	Btst	#2,D7
	Beq.S	lbC006192

	AddQ.B	#4,D5
	Bsr.w	lbC006330
	Move.B	D7,$22(A6)
lbC006192:
	Move.W	$136(a2),d6
	Bne.S	lbC0061B2

	Lea	Mt_Voice4(pc),A6
	Move.B	$22(A6),D7
	Btst	#2,D7
	Beq.S	lbC0061B2

	AddQ.B	#8,D5
	Bsr.w	lbC006330
	Move.B	D7,$22(A6)
lbC0061B2:
	Move.W	$13a(a2),d0
	Beq.S	lbC00620A

	Move.W	$136(a2),d0
	Beq.S	lbC006222

	Lea	MusicData1(pc),A6
	Move.W	$138(a2),d1
	Beq.S	lbC006200

	Clr.W	$138(a2)
	Add.W	D0,D0
	Move.W	D0,D1
	Add.W	D0,D0
	Add.W	D0,D0
	Add.W	D1,D0
	Ext.L	D0
	Lea	MusicData11(pc),A0
	Add.L	D0,A0
	Move.W	(A0)+,D0
	Move.B	D0,$3A(A6)
	Move.L	(A0)+,$1A(A6)
	Move.L	(A0),$1E(A6)
	Move.L	#$7000000,$22(A6)
	Move.W	#8,$DFF096
lbC006200:
	Move.B	$22(A6),D7
	Btst	#2,D7
	Bne.S	lbC006210

lbC00620A:
	Clr.W	$136(a2)
	Bra.S	lbC006222
lbC006210:
	Lea	MusicData4(pc),A1
	Bsr.w	lbC006330
	Move.B	D7,$22(A6)
	Or.W	#8,D5
lbC006222:
	Lea	$562(a2),a6
	Bra.S	lbC00623E
	Beq.S	lbC006236

	SubQ.L	#1,(A6)+
	Move.L	(A6),A1
	Move.L	(A1)+,D0
	Move.L	A1,(A6)+
	Sub.L	D0,(A6)
	Bra.S	lbC00623E
lbC006236:
	Move.L	8(A6),D0
	Add.L	D0,$55e(a2)
lbC00623E:
	Move.B	$DFF006,D0
	AddQ.B	#1,D0
lbC006246:
	Cmp.B	$DFF006,D0
	Bne.S	lbC006246

	MoveQ	#15,D0
	And.B	D5,D0
	Not.W	D0
	And.W	#15,D0

	pea	(a0)
	lea	MusicData3(pc),a0
	Tst.W	(a0)
	lea	start_sign(pc),a0
	Beq.S	lbC006274
	Move.W	D0,$DFF096
	Move.W	D5,$DFF096
	Clr.B	(a0)
	bra.s	release

lbC006274:

	Tst.B	(a0)
	Bne.S	lbC006290
	St 	(a0)
	Move.W	D0,$DFF096
	Move.W	D5,$DFF096
	bra.s	release

lbC006290:
	And.W	#8,D0
	And.W	#$8008,D5
	Move.W	D0,$DFF096
	Move.W	D5,$DFF096

release:
	move.l	(a7)+,a0
	Rts

Mt_read_sample:
	Clr.W	D0
	Move.L	14(A6),A0
	Move.B	(A0)+,D0
	Beq.S	lbC0062DE

	Move.W	D1,$DFF096
	Add.B	$19(A6),D0
	Move.B	D0,$3A(A6)
	Move.B	(A0),D0
	Add.B	$18(A6),D0
	Lsl.B	#3,D0
	Lea	MusicData5(pc),A5
	Add.W	D0,A5
	Move.L	(A5)+,$1A(A6)
	Move.L	(A5),$1E(A6)
	Move.L	#$7000000,$22(A6)
lbC0062DE:
	Move.B	2(A0),$12(A6)
	MoveQ	#-$20,D0
	And.B	(A0)+,D0
	Bne.S	lbC0062F6

	Move.B	#$40,$15(A6)
	Clr.W	$38(A6)
	Rts

lbC0062F6:
	Cmp.B	#$60,D0
	Bne.S	lbC00630E

	MoveQ	#0,D0
	Move.B	(A0),D0
	Neg.W	D0
	Move.W	D0,$38(A6)
	Move.B	#$40,$15(A6)
	Rts

lbC00630E:
	Cmp.B	#$40,D0
	Bne.S	lbC006326

	Move.B	(A0),D0
	And.W	#$FF,D0
	Move.W	D0,$38(A6)
	Move.B	#$40,$15(A6)
	Rts

lbC006326:
	Move.B	(A0),$15(A6)
	Clr.W	$38(A6)
	Rts

lbC006330:
	Clr.W	D6
	Move.B	$24(A6),D6
lbC006336:
	Move.L	$1A(A6),A0
	Add.W	D6,A0
	Move.B	(A0)+,D1
	Cmp.B	#$A0,D1
	Beq.S	lbC00639E

	Cmp.B	#$C0,D1
	Beq.S	lbC006360

	Cmp.B	#$B0,D1
	Beq.S	lbC00635C

	Clr.W	12(A6)
	Bclr	#2,D7
	Bra.w	lbC006404
lbC00635C:
	Move.B	(A0),D6
	Bra.S	lbC006336
lbC006360:
	Clr.W	D1
	Move.B	(A0)+,D1
	Lea	$34(A6),A5
	Move.B	(A0),(A5)+
	Move.B	(A0)+,(A5)+
	Move.B	(A0),(A5)+
	Add.W	D1,D1
	Lea	MusicData9(pc),A0
	Move.W	0(A0,D1.W),D2
	Move.W	D2,8(A6)
	Cmp.W	#$50,D2
	Bmi.S	lbC006388

	Bset	#3,D7
lbC006388:
	Add.W	D1,D1
	Lea	MusicData8(pc),A0
	Move.L	(A0,D1.W),4(A6)
	AddQ.B	#4,D6
	Bset	#0,D7
	Bra.S	lbC006336
lbC00639E:
	Bclr	#0,D7
	Beq.S	lbC0063D8

	Move.B	(A0)+,D1
	Move.B	(A0)+,D2
	Move.B	(A0),$26(A6)
	AddQ.B	#1,$26(A6)
	Move.B	D1,13(A6)
	Sf	$27(A6)
	Move.B	#1,$2A(A6)
	Sub.B	D1,D2
	Bcc.S	lbC0063C8

	Neg.B	D2
	St 	$2A(A6)
lbC0063C8:
	Move.B	D2,$28(A6)
	Bra.S	lbC006404
lbC0063CE:
	AddQ.B	#4,D6
	Bset	#0,D7
	Bra.w	lbC006336
lbC0063D8:
	SubQ.B	#1,$26(A6)
	Beq.S	lbC0063CE

	Move.B	2(A0),D4
	Move.B	$27(A6),D1
	Add.B	$28(A6),D1
	Sub.B	D4,D1
	BMI.S	lbC0063FE

	Clr.W	D2
	Move.B	$2A(A6),D3
lbC0063F4:
	Add.B	D3,D2
	Sub.B	D4,D1
	Bpl.S	lbC0063F4

	Add.B	D2,13(A6)
lbC0063FE:
	Add.B	D4,D1
	Move.B	D1,$27(A6)
lbC006404:
	Move.B	D6,$24(A6)
	Move.B	$25(A6),D6
lbC00640C:
	Move.L	$1E(A6),A0
	Add.W	D6,A0
	Move.B	(A0)+,D1
	Cmp.B	#$A0,D1
	Beq.S	lbC006458

	Cmp.B	#$D0,D1
	Beq.S	lbC00642C

	Cmp.B	#$B0,D1
	Bne.w	lbC0064CA

	Move.B	(A0),D6
	Bra.S	lbC00640C
lbC00642C:
	Bclr	#1,D7
	Bne.S	lbC006442

	SubQ.B	#1,$29(A6)
	Bne.w	lbC0064CA

	Bset	#1,D7
	AddQ.B	#4,D6
	Bra.S	lbC00640C
lbC006442:
	Clr.W	D1
	Move.B	(A0),D1
	Move.B	2(A0),$29(A6)
	Add.B	$3A(A6),D1
	Move.W	(A1,D1.W),10(A6)
	Bra.S	lbC0064CA
lbC006458:
	Bclr	#1,D7
	Beq.S	lbC0064B6

	Clr.W	D0
	Move.B	(A0)+,D1
	Move.B	(A0)+,D2
	Move.B	(A0),$29(A6)
	Move.B	$3A(A6),D0
	Move.W	0(A1,D0.W),D0
	Ext.W	D1
	Ext.W	D2
	Add.W	D0,D1
	Add.W	D0,D2
	Move.W	D1,10(A6)
	Move.W	D1,$2C(A6)
	Clr.W	$2E(A6)
	Sub.W	D1,D2
	Clr.W	D1
	Move.B	$29(A6),D1
	Ext.L	D2
	Asl.L	#8,D2
	Divs	D1,D2
	Bvs.S	lbC00649E

	Ext.L	D2
	Asl.L	#8,D2
	Move.L	D2,$30(A6)
	Bra.S	lbC0064CA
lbC00649E:
	Asr.L	#8,D2
	Divs	D1,D2
	Swap	D2
	Clr.W	D2
	Move.L	D2,$30(A6)
	Bra.S	lbC0064CA
lbC0064AC:
	Bset	#1,D7
	AddQ.B	#4,D6
	Bra.w	lbC00640C
lbC0064B6:
	SubQ.B	#1,$29(A6)
	Beq.S	lbC0064AC

	Move.L	$30(A6),D1
	Add.L	D1,$2C(A6)
	Move.W	$2C(A6),10(A6)
lbC0064CA:
	Move.B	D6,$25(A6)
	Move.W	$38(A6),D1
	Add.W	D1,10(A6)
	Lea	(A6),A0
	Move.L	(A0)+,A5
	Move.L	(A0)+,A3
	Move.B	$34(A6),D0
	Beq.S	lbC006502

	Move.B	$35(A6),D1
	Ble.S	lbC0064F0

	Cmp.B	$36(A6),D1
	Bmi.S	lbC0064F6

lbC0064F0:
	Neg.B	D0
	Move.B	D0,$34(A6)
lbC0064F6:
	Add.B	D0,D1
	Move.B	D1,$35(A6)
	Ext.W	D1
	Lsl.W	#5,D1
	Add.W	D1,A3
lbC006502:
	Move.L	A3,(A5)+
	Move.L	(A0),(A5)+
	Bclr	#3,D7
	Beq.S	lbC006510

	Move.W	#1,(A0)
lbC006510:
	AddQ.L	#4,A0
	Move.W	(A0),D6
	Add.B	$15(A6),D6
	Sub.B	#$40,D6
	Bpl.S	lbC006520

	Clr.W	D6
lbC006520:
	Move.W	D6,(A5)
	Rts

Mt_end:	Move.w	#$f,$dff096
	Rts

;	*******************
;	*** Replay Data ***
;	*******************

start_sign:	Dc.w	0
MusicData:	Dc.l	0
Mt_Voice1:	Dc.l	$DFF0A0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
Mt_Voice2:	Dc.l	$DFF0B0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
Mt_Voice3:	Dc.l	$DFF0C0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
Mt_Voice4:	Dc.l	$DFF0D0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

MusicData1:
	Dcb.w	31,0
MusicData2:
	Dcb.w	5,0
MusicData3:
	Dc.w	1
MusicData4:
	Dc.l	$1AC0,$194017D0,$16801530,$140012E0,$11D010D0
	Dc.l	$FE00F00,$E280D60,$CA00BE8,$B400A98,$A000970
	Dc.l	$8E80868,$7F00780,$71406B0,$65005F4,$5A0054C
	Dc.l	$50004B8,$4740434,$3F803C0,$38A0358,$32802FA
	Dc.l	$2D002A6,$280025C,$23A021A,$1FC01E0,$1C501AC
	Dc.l	$194017D,$1680153,$140012E,$11D010D,$FE00F0,$E200D6
	Dc.l	$CA00BE,$B400AA,$A00097,$8F0087,$7F0078
	Dc.w	$0071
MusicData5:
	Dcb.w	4,0
MusicData6:
	Dcb.w	132,0
MusicData7:
	Dcb.w	160,0
MusicData8:
	Dcb.w	104,0
MusicData9:
	Dc.w	0,0,0,0,0,0,0,0,0

	Dc.l	$10,$100010,$100010,$100010,$100010,$100010,$100010
	Dcb.l	14,$100010
	Dc.l	$100000,0,0,$539D0002,$9F32E2CB
	Dc.w	$0053
MusicData10:
	Dc.w	$C0C0,$D0D8,$E0E8
	Dc.l	$F0F800F8,$F0E8E0D8,$D0C83F37,$2F271F17,$F07FF07
	Dc.l	$F171F27,$2F37C0C0,$D0D8E0E8,$F0F800F8,$F0E8E0D8
	Dc.l	$D0C8C037,$2F271F17,$F07FF07,$F171F27,$2F37C0C0
	Dc.l	$D0D8E0E8,$F0F800F8,$F0E8E0D8,$D0C8C0B8,$2F271F17
	Dc.l	$F07FF07,$F171F27,$2F37C0C0,$D0D8E0E8,$F0F800F8
	Dc.l	$F0E8E0D8,$D0C8C0B8,$B0271F17,$F07FF07,$F171F27
	Dc.l	$2F37C0C0,$D0D8E0E8,$F0F800F8,$F0E8E0D8,$D0C8C0B8
	Dc.l	$B0A81F17,$F07FF07,$F171F27,$2F37C0C0,$D0D8E0E8
	Dc.l	$F0F800F8,$F0E8E0D8,$D0C8C0B8,$B0A8A017,$F07FF07
	Dc.l	$F171F27,$2F37C0C0,$D0D8E0E8,$F0F800F8,$F0E8E0D8
	Dc.l	$D0C8C0B8,$B0A8A098,$F07FF07,$F171F27,$2F37C0C0
	Dc.l	$D0D8E0E8,$F0F800F8,$F0E8E0D8,$D0C8C0B8,$B0A8A098
	Dc.l	$9007FF07,$F171F27,$2F37C0C0,$D0D8E0E8,$F0F800F8
	Dc.l	$F0E8E0D8,$D0C8C0B8,$B0A8A098,$9088FF07,$F171F27
	Dc.l	$2F37C0C0,$D0D8E0E8,$F0F800F8,$F0E8E0D8,$D0C8C0B8
	Dc.l	$B0A8A098,$90888007,$F171F27,$2F37C0C0,$D0D8E0E8
	Dc.l	$F0F800F8,$F0E8E0D8,$D0C8C0B8,$B0A8A098,$90888088
	Dc.l	$F171F27,$2F37C0C0,$D0D8E0E8,$F0F800F8,$F0E8E0D8
	Dc.l	$D0C8C0B8,$B0A8A098,$90888088,$90171F27,$2F37C0C0
	Dc.l	$D0D8E0E8,$F0F800F8,$F0E8E0D8,$D0C8C0B8,$B0A8A098
	Dc.l	$90888088,$90981F27,$2F37C0C0,$D0D8E0E8,$F0F800F8
	Dc.l	$F0E8E0D8,$D0C8C0B8,$B0A8A098,$90888088,$9098A027
	Dc.l	$2F37C0C0,$D0D8E0E8,$F0F800F8,$F0E8E0D8,$D0C8C0B8
	Dc.l	$B0A8A098,$90888088,$9098A0A8,$2F37C0C0,$D0D8E0E8
	Dc.l	$F0F800F8,$F0E8E0D8,$D0C8C0B8,$B0A8A098,$90888088
	Dc.l	$9098A0A8,$B0378181,$81818181,$81818181,$81818181
	Dc.l	$81817F7F,$7F7F7F7F,$7F7F7F7F,$7F7F7F7F,$7F7F8181
	Dcb.l	3,$81818181
	Dc.l	$8181817F,$7F7F7F7F,$7F7F7F7F,$7F7F7F7F,$7F7F8181
	Dcb.l	4,$81818181
	Dcb.l	3,$7F7F7F7F
	Dc.l	$7F7F8181,$81818181,$81818181,$81818181,$81818181
	Dc.l	$817F7F7F,$7F7F7F7F,$7F7F7F7F,$7F7F8181,$81818181
	Dcb.l	3,$81818181
	Dc.l	$81817F7F,$7F7F7F7F,$7F7F7F7F,$7F7F8181,$81818181
	Dcb.l	3,$81818181
	Dc.l	$8181817F,$7F7F7F7F,$7F7F7F7F,$7F7F8181,$81818181
	Dcb.l	4,$81818181
	Dcb.l	2,$7F7F7F7F
	Dc.l	$7F7F8181,$81818181,$81818181,$81818181,$81818181
	Dc.l	$81818181,$817F7F7F,$7F7F7F7F,$7F7F8181,$81818181
	Dcb.l	4,$81818181
	Dc.l	$81817F7F,$7F7F7F7F,$7F7F8181,$81818181,$81818181
	Dcb.l	3,$81818181
	Dc.l	$8181817F,$7F7F7F7F,$7F7F8181,$81818181,$81818181
	Dcb.l	4,$81818181
	Dc.l	$7F7F7F7F,$7F7F8181,$81818181,$81818181,$81818181
	Dcb.l	3,$81818181
	Dc.l	$817F7F7F,$7F7F8181,$81818181,$81818181,$81818181
	Dcb.l	3,$81818181
	Dc.l	$81817F7F,$7F7F8181,$81818181,$81818181,$81818181
	Dcb.l	3,$81818181
	Dc.l	$8181817F,$7F7F8080,$80808080,$80808080,$80808080
	Dcb.l	4,$80808080
	Dc.l	$7F7F8080,$80808080,$80808080,$80808080,$80808080
	Dcb.l	3,$80808080
	Dc.l	$807F8080,$80808080,$80807F7F,$7F7F7F7F,$7F7F8080
	Dc.l	$80808080,$807F7F7F,$7F7F7F7F,$7F7F8080,$80808080
	Dcb.l	2,$7F7F7F7F
	Dc.l	$7F7F8080,$8080807F,$7F7F7F7F,$7F7F7F7F,$7F7F8080
	Dc.l	$80807F7F,$7F7F7F7F,$7F7F7F7F,$7F7F8080,$807F7F7F
	Dcb.l	2,$7F7F7F7F
	Dc.l	$7F7F8080,$7F7F7F7F,$7F7F7F7F,$7F7F7F7F,$7F7F8080
	Dcb.l	3,$7F7F7F7F
	Dc.l	$7F7F8080,$9098A0A8,$B0B8C0C8,$D0D8E0E8,$F0F80008
	Dc.l	$10182028,$30384048,$50586068,$707F8080,$A0B0C0D0
	Dc.l	$E0F00010,$20304050,$60704545,$797D7A77,$70666158
	Dc.l	$534D2C20,$181204DB,$D3CDc6BC,$B5AEA8A3,$9D99938E
	Dc.l	$8B8A4545,$797D7A77,$70665B4B,$43372C20,$181204F8
	Dc.l	$E8DBCFC6,$BEB0A8A4,$9E9A9594,$8D830000,$40607F60
	Dc.l	$402000E0,$C0A080A0,$C0E00000,$40607F60,$402000E0
	Dc.l	$C0A080A0,$C0E08080,$9098A0A8,$B0B8C0C8,$D0D8E0E8
	Dc.l	$F0F80008,$10182028,$30384048,$50586068,$707F8080
	Dc.l	$A0B0C0D0,$E0F00010,$20304050
	Dc.w	$6070
MusicData11:
	Dc.w	$0000,$0002,$6B70
	Dc.l	$26B7A,$7A0002,$6B700002,$6B7A007E,$26B80,$26B8A
	Dc.l	$760002,$6B940002,$6B9E0068,$26BA8,$26BBE,$300002
	Dc.l	$6BCC0002,$6BD6008A,$26BE0,$26BF2,$780002,$6BFC0002
	Dc.l	$6C0A0078,$26C18,$26C22,$5E0002,$6C280002,$6C360058
	Dc.l	$26C44,$26C62,$780002,$6C740002,$6C7E0048,$26C84
	Dc.l	$26C9A,$8C0002,$6CC40002,$6CD2C01A,$4A028,$10E000
	Dc.l	$A07F0010,$E000C01F,$A023,$8E000,$D0F80001
	Dc.l	$A0F67F0A,$E000C01A,$10FA01E,$AE000,$A064CE02
	Dc.l	$A0CE7808,$E000C01A,$FA028,$503A023,$205A014,$8A00A
	Dc.l	$6E000,$A0813204,$A0007F03,$A07F0003,$B004C02F
	Dc.l	$8A040,$4014E004,$A0140014,$A0007814,$B000C01C
	Dc.l	$FA023,$1404A01E,$A04A00A,$8E000,$D0E80001
	Dc.l	$D0000001,$E000C01C,$FA028,$1408A014,$14E000
	Dc.l	$D0080003,$D0000003,$D00A0001,$E000C01C,$FA028
	Dc.l	$1408E000,$D0080003,$E000C01C,$10FA01E,$3203A019
	Dc.l	$8E000,$A0880004,$D0E80002,$D0000001,$B004C01C
	Dc.l	$10FA028,$3C03A01E,$4A032,$4A014,$2A01E,$4A00A
	Dc.l	$3E000,$D00C0002,$A07F8106,$D0F40002,$D0180001
	Dc.l	$B008C01C,$FA028,$1408E000,$D0080003,$E000C01A
	Dc.l	$FA028,$1404A014,$2806A01E,$A14A00A,$28E000
	Dc.l	$D0080001,$A07F810A,$D0080002,$D00A0002,$D00C0002
	Dc.l	$D00E0002,$D0120002,$D0180002,$D0200002,$D0260001
	Dc.l	$E000C01A,$FA028,$1404A00A,$8E000,$D0000001
	Dc.l	$D0020001,$E0000001,$80001612,$C050001,$5000130C
	Dc.l	$16050001,$1000190E,$15020001,$190A,$D020000
	Dc.l	$90001B18,$10020000,$85000A15,$F010000,$8000121F
	Dc.l	$A010000,$75000E1B,$16010000,$70001C1D,$E010000
	Dc.l	$6500101E,$12010000,$60001D0A,$1B010000,$55000B18
	Dc.l	$1E010000,$5000130E,$A010000,$45000F0A,$B010000
	Dc.l	$40001D0E,$18010000,$35000A1B,$16010000,$30000A1B
	Dc.l	$12010000,$2500130A,$C010000,$2000190A,$1D010000
	Dc.l	$1500131E
	Dc.w	$1501
	ds.b	256

