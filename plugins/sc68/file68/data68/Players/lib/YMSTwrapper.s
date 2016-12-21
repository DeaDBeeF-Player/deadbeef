;;; Time-stamp: <2011-09-12 14:42:15 ben>
;;; 
;;; YMST wrapper (sc68 wrapper for MYST (YMST) emulator files.
;;; by Benjamin Gerard
;;;
;;; This wrapper should work for both replay and custom players !
;;;
;;; List of MYST tag
;;;
;;; 00 : ?
;;; 01 : Pointer to end ?
;;; 02 : ?
;;; 03 : Buffer de 3 long [ YM-buffer , Song, ? ]
;;; 04 : Dc.l 1 ?
;;; 05 : ?
;;; 06 : Name
;;; 07 : Author
;;; 08 : Ripper
;;; 09 : ?
;;; 0A : Remark
;;; 0B : Relocation routine
;;; 0C : Patch routine
;;; 0D : Init routine (out:a0 = song)
;;; 0E : Play
;;; 0F : Copy buffer -> MYST YM shadow
;;; 10 : First song number
;;; 11 : Last song number
;;; 12 : Default Song
;;; 14 : Song/Replay data
;;; 15 : ?
;;; 16 : Buffer 8 bytes ?
;;; 17 : ?

start:
	bra 		Init
.ret:	
	rts
	rts
	move.l 	TagTable+(4*$E)(pc),d7
	beq.s 	.ret
	movea.l d7,a6
	jmp 		(a6)
	
	ALIGN		16

TagTable:				; Tag table for replay
	dcb.l		4*32

SngTable:				; Tag table for song
	dcb.l		4*32

Init:
	lea 		YMSTfile(pc),a6
	lea			TagTable(pc),a2
	bsr.s		GetTag
	bne.s		.failed
	
; To know test internal replay has a0 = to replay address
	lea			start(pc),a6
	cmp.l		a0,a6
	bne.s		.ExternalReplay

; Internal replay
	move.l	TagTable+$2c(pc),d7	; Relocation 
	bsr 		run_d7_save
	move.l 	TagTable+$34(pc),d7	; Init
	bra 		run_d7_save

.ExternalReplay:
; External replay
	move.l	a0,a6
	move.l	a2,a3
	lea			SngTable(pc),a2
	bsr.s		GetTag
	bne.s		.failed

; Relocation	
	move.l	4*$b(a2),d7		; Run relocation code (song)
	bsr			run_d7_save
	move.l	4*$b(a3),d7		; Run relocation code (replay)
	bsr			run_d7_save
	
	move.l	4*$14(a2),a0	; get default file data as default song data
	move.l	4*$d(a2),d7		; Run song init code (return a0:song address)
	bsr			run_d7_out_a0
	
	move.l	TagTable+4*$3(pc),d7	; Get address of song info buffer
	beq.s		.NoSIbuffer
	move.l	d7,a6
	move.l	a0,4(a6)							; Put song address into song ptr
.NoSIbuffer:

	move.l	4*$d(a3),d7			; Run (replay) init code (in:a0, song address)
	bsr			run_d7_save
	rts
	
.failed:
	lea 	TagTable+(4*$E)(pc),a6
	clr.l	(a6)
	rts
		
GetTag:	
	movea.l a6,a5
	move.l 	(a6)+,d7
	cmp.l 	#'YMST',d7
	beq.s 	ReadTag
	rts
	
ReadTag:
	move.w 	(a6)+,d7
	cmp.w 	#$8000,d7
	bne.s 	.NoMoreTag
	
	move.w 	(a6)+,d7
	subi.w 	#$594d,d7
	lsl.w 	#2,d7
	move.l 	(a6)+,d6
	add.l 	a5,d6
	move.l 	d6,$0(a2,d7.w)
	bra.s 	ReadTag
.NoMoreTag:
	cmp.w		d0,d0
	rts



	move.l	TagTable+$2c(pc),d7
	bsr 		run_d7_save

	move.l 	TagTable+$34(pc),d7
	bra 		run_d7_save

run_d7_save:
	tst.l		d7
	beq.s 	.ret
	movem.l d0-a6,-(a7)
	movea.l d7,a6
	jsr 		(a6)
	movem.l (a7)+,d0-a6
.ret:
	rts

run_d7_out_a0:
	tst.l		d7
	beq.s 	.ret
	movem.l d0-d7/a1-a6,-(a7)
	movea.l d7,a6
	jsr 		(a6)
	movem.l (a7)+,d0-d7/a1-a6
.ret:
	rts


;run_d7:
;	tst.l		d7
;	beq.s 	.ret
;	lea			.chgjmp+2(pc),a6
;	move.l	d7,(a6)
;.chgjmp:
;	jmp 		$1234567
;.ret:
;	rts

YMSTfile:
; Where YMST file will be located
