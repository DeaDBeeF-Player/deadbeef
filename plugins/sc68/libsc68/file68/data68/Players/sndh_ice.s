; file: "sndh_ice"
; from: $8000
; to:   7fd92
;
; sourcer68 - Copyright (C) 2001-2003 Benjamin Gerard
;
; Original sndh ice replay for sc68 written by Abyss Of Cream 2002
;  
; This version is a hack of Ver. 0.90 that depack the data after
; compressed buffer.
; 
Start: 
    BRA Init			; +0
    MOVEQ #4,D6			; +4
    BRA.S Exec
Play:				; +8
    MOVEQ #8,D6
Exec:
    MOVE.L Music(PC),D7
    BEQ.S  Return
    MOVE.L D7,A6
    MOVE.W SR,-(A7)		; Becoz some musics (tao) breaks it.
    JSR    (A6,D6)
    MOVE.W (A7)+,SR
Return:
    RTS
	
Music:
    DC.L 0

Init:
    MOVE.L A0,A1		; Save original address
    CMP.L #$49434521,(A0)
    BNE .no_depack
	
    MOVE.L D0,-(A7)
    MOVEQ #-2,D0
    AND.L 4(A0),D0		; Get packed size
    LEA 16(A0,D0.L),A1		; Get "uncompressed destination"
    MOVE.L (A7)+,D0
    BSR.S Depack		; Do unice !
.no_depack:
    LEA Music(PC),A6		; Store depacked address
    MOVE.L A1,(A6)
    JMP (A1)			; jump to init code.  

Depack: 
    MOVEM.L D0-A6,-(A7)
    BSR.S L008082
    CMPI.L #$49434521,D0
    BNE L00807C
    BSR.S L008082
    LEA -$8(A0,D0.L),A5
    BSR.S L008082
    MOVEA.L A1,A4
    MOVEA.L A1,A6
    ADDA.L D0,A6
    MOVEA.L A6,A3
    MOVE.B -(A5),D7
    BSR L00808E
    MOVEA.L A3,A6
    BSR L0080BA
    BCC.S L00807C
    MOVE.W #$F9F,D7
    BSR L0080BA
    BCC.S L008056
    MOVEQ #$F,D0
    BSR L0080C4
    MOVE.W D1,D7
L008056: 
    MOVEQ #$3,D6
L008058: 
    MOVE.W -(A3),D4
    MOVEQ #$3,D5
L00805C: 
    ADD.W D4,D4
    ADDX.W D0,D0
    ADD.W D4,D4
    ADDX.W D1,D1
    ADD.W D4,D4
    ADDX.W D2,D2
    ADD.W D4,D4
    ADDX.W D3,D3
    DBF D5,L00805C
    DBF D6,L008058
    MOVEM.W D0-D3,(A3)
    DBF D7,L008056
L00807C: 
    MOVEM.L (A7)+,D0-A6
    RTS

L008082: 
    MOVEQ #$3,D1
L008084: 
    LSL.L #8,D0
    MOVE.B (A0)+,D0
    DBF D1,L008084
    RTS

L00808E: 
    BSR.S L0080BA
    BCC.S L0080B4
    MOVEQ #$0,D1
    BSR.S L0080BA
    BCC.S L0080AE
    LEA L008152(PC),A1
    MOVEQ #$4,D3
L00809E: 
    MOVE.L -(A1),D0
    BSR.S L0080C4
    SWAP D0
    CMP.W D0,D1
    DBNE D3,L00809E
    ADD.L $14(A1),D1
L0080AE: 
    MOVE.B -(A5),-(A6)
    DBF D1,L0080AE
L0080B4: 
    CMPA.L A4,A6
    BGT.S L0080D6
    RTS

L0080BA: 
    ADD.B D7,D7
    BNE.S L0080C2
    MOVE.B -(A5),D7
    ADDX.B D7,D7
L0080C2: 
    RTS

L0080C4: 
    MOVEQ #$0,D1
L0080C6: 
    ADD.B D7,D7
    BNE.S L0080CE
    MOVE.B -(A5),D7
    ADDX.B D7,D7
L0080CE: 
    ADDX.W D1,D1
    DBF D0,L0080C6
    RTS

L0080D6: 
    LEA L008166(PC),A1
    MOVEQ #$3,D2
L0080DC: 
    BSR.S L0080BA
    DBCC D2,L0080DC
    MOVEQ #$0,D4
    MOVEQ #$0,D1
    MOVE.B $1(A1,D2.W),D0
    EXT.W D0
    BMI.S L0080F0
    BSR.S L0080C4
L0080F0: 
    MOVE.B $6(A1,D2.W),D4
    ADD.W D1,D4
    BEQ.S L00811A
    LEA L008170(PC),A1
    MOVEQ #$1,D2
L0080FE: 
    BSR.S L0080BA
    DBCC D2,L0080FE
    MOVEQ #$0,D1
    MOVE.B $1(A1,D2.W),D0
    EXT.W D0
    BSR.S L0080C4
    ADD.W D2,D2
    ADD.W $6(A1,D2.W),D1
    BPL.S L00812C
    SUB.W D4,D1
    BRA.S L00812C
L00811A: 
    MOVEQ #$0,D1
    MOVEQ #$5,D0
    MOVEQ #-$1,D2
    BSR.S L0080BA
    BCC.S L008128
    MOVEQ #$8,D0
    MOVEQ #$3F,D2
L008128: 
    BSR.S L0080C4
    ADD.W D2,D1
L00812C: 
    LEA $2(A6,D4.W),A1
    ADDA.W D1,A1
    MOVE.B -(A1),-(A6)
L008134: 
    MOVE.B -(A1),-(A6)
    DBF D4,L008134
    BRA L00808E
L00813E: 
    dc.w $7fff,$000e,$00ff,$0007,$0007,$0002,$0003,$0001
    dc.w $0003,$0001
L008152: 
    dc.w $0000,$010d,$0000,$000e,$0000,$0007,$0000,$0004
    dc.w $0000,$0001
L008166: 
    dc.w $0901,$00ff,$ff08,$0402,$0100
L008170: 
    dc.w $0b04,$0700,$011f,$ffff,$001f

