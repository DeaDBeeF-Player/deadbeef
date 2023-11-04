;;; sc68 wrapper for PSG1 
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:07:52 ben>
;;; 

	BRA Init
	BRA Exit
Play:
	MOVEM.L D0-D1/A0-A1,-(A7)
	MOVE.L SongPtr(PC),D0
	BEQ .finish
	MOVE.L D0,A0
	
	LEA $E(A0),A0
	LEA CurPos(PC),A1
	MOVE.L (A1),D0
	CMP.L -$A(A0),D0
	BLT.S .noloop
	MOVEQ #0,D0
.noloop: 
	ADDA.L D0,A0
	ADDQ.L #1,D0
	MOVE.L D0,(A1)

	LEA $FFFF8800.W,A1
		
	MOVE.L NbPos(PC),D0
	MOVE.L D0,D1
	CLR.B (A1)
	MOVE.B (A0),2(A1)
	MOVE.B #$1,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$2,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$3,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$4,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$5,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$6,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$7,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$8,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$9,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$A,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$B,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$C,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
	ADD.L D0,D1
	MOVE.B #$D,(A1)
	MOVE.B $0(A0,D1.L),2(A1)
.finish:	
	MOVEM.L (A7)+,D0-D1/A0-A1
	RTS

Init:
	LEA SongPtr(PC),A1
	CMP.L #'PSG1',(A0)
	BNE.S .not
	MOVE.L A0,(A1)
	MOVE.L 4(A0),-(A1)
	SUBQ.W #4,A1
.not:
	CLR.L	(A1)
	RTS
	
Exit:
	LEA SongPtr(pc),A0
	CLR.L (A0)
	LEA $FFFF8800.W,A0
	MOVE.L #$08080000,(A0)
	MOVE.L #$09090000,(A0)
	MOVE.L #$0A0A0000,(A0)
	RTS

CurPos: 
	DC.L $00000000
NbPos: 
	DC.L $00000000
SongPtr: 
	DC.L $00000000
