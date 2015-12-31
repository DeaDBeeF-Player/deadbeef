; Tos relocation 
; by Ben(jamin) Gerard <benjihan [] users sourceforge net>
;

load_prg:
; a0 : prg
	cmp.w		#$601a,(a0)		; Text magic value
	beq.s		.okprg
	rts
.okprg:	
	movem.l	d0-a6,-(a7)
	
	movem.l	2(a0),d0-d3		; [text,data,bss,symbol] section size
	
	move.l	d0,d4
	add.l		d1,d4
	add.l		d3,d4					; d4 = size of [text,data,symbol]
	
	lea			$1c(a0),a1			; Text section
	lea			$1c(a0,d4.l),a2	; a2 = Relocation table
	
	move.l	a1,a4
	add.l		d0,a4
	add.l		d1,a4						; a4 = BSS (text-base + text-size + data-size)
	
	move.l	a1,a3						; a3 = text section to reloc
	move.l	a3,d4

; --- Relocation
;
	move.l	(a2)+,d5
	beq.s		.end
	bra.s   .reloc

.skip:
	lea			254-1(a3),a3
	bra.s   .read
	
.reloc:
	add.l		d5,a3
	cmp.b		#1,d5
	beq.s		.skip
	add.l		d4,(a3)
.read:
	moveq	#0,d5
	move.b	(a2)+,d5
	bne.s		.reloc
.end:

; --- Clear BSS
;
	tst.l	d2
	beq.s		.nobss
.clearbss:
	clr.b		(a4)+
	subq.l	#1,d2
	bne.s		.clearbss
.nobss:
	movem.l	(a7)+,d0-a6
	rts

make_basepage:
; a0 : prg
; a1 : basepage (256 bytes)
	cmp.w		#$601a,(a0)		; Text magic value
	bne.s		.notprg

	movem.l	d0-d5,-(a7)
	
	movem.l	2(a0),d1/d3/d5		; [text,data,bss,symbol] section size
	moveq		#$1c,d0
	add.l		a0,d0						; Text section
	move.l	d0,d2
	add.l		d1,d2						; Data section
	move.l	d2,d4
	add.l		d3,d4						; Bss section
	
	movem.l	d0-d5,8(a1)			; store in basepage
	
	movem.l	(a7)+,d0-d5
.notprg:	
	rts
