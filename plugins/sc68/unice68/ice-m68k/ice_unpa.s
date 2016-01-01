;;; unpacking routine of PACK-ICE (2.40)
;;; a0 = Pointer to packed data
;;; a1 = Address to which the data is unpacked
;;; "bsr" of "jsr" to ice_decrunch with registers a0 and a1 prepared.

ice_decrunch:
	movem.l	d0-a6,-(a7)
	bsr.s	getinfo
	cmpi.l	#'ICE!',d0
	bne	not_packed
	bsr.s	getinfo	
	lea.l	-8(a0,d0.l),a5
	bsr.s	getinfo
	move.l	d0,(a7)
	move.l	a1,a4
	move.l	a1,a6
	adda.l	d0,a6
	move.l	a6,a3
	move.b	-(a5),d7
	bsr	normal_bytes

	move.l	a3,a6
	bsr	get_1_bit
	bcc.s	not_packed
	move.w	#$0f9f,d7
	bsr	get_1_bit
	bcc.s	ice_00
	moveq	#15,d0	
	bsr	get_d0_bits
	move.w	d1,d7
ice_00:	moveq	#3,d6
ice_01:	move.w	-(a3),d4
	moveq	#3,d5
ice_02:	add.w	d4,d4
	addx.w	d0,d0
	add.w	d4,d4
	addx.w	d1,d1
	add.w	d4,d4
	addx.w	d2,d2
	add.w	d4,d4
	addx.w	d3,d3
	dbra	d5,ice_02
	dbra	d6,ice_01
	movem.w	d0-d3,(a3)
	dbra	d7,ice_00

not_packed:
	movem.l	(a7)+,d0-a6
	rts

getinfo:
	moveq	#3,d1
getbytes:
	lsl.l	#8,d0
	move.b	(a0)+,d0
	dbf	d1,getbytes
	rts

normal_bytes:	
	bsr.s	get_1_bit
	bcc.s	test_if_end
	moveq.l	#0,d1
	bsr.s	get_1_bit
	bcc.s	copy_direkt
	lea.l	direkt_tab+20(pc),a1
	moveq.l	#4,d3
nextgb:	move.l	-(a1),d0
	bsr.s	get_d0_bits
	swap.w	d0
	cmp.w	d0,d1
	dbne	d3,nextgb
no_more:
	add.l	20(a1),d1
copy_direkt:	
	move.b	-(a5),-(a6)
	dbf	d1,copy_direkt
test_if_end:	
	cmpa.l	a4,a6
	bgt.s	strings
	rts	


get_1_bit:add.b	d7,d7
	bne.s	bitfound
	move.b	-(a5),d7
	addx.b	d7,d7
bitfound:	rts	

get_d0_bits:	
	moveq.l	#0,d1
hole_bit_loop:	
	add.b	d7,d7
	bne.s	on_d0
	move.b	-(a5),d7
	addx.b	d7,d7
on_d0:	addx.w	d1,d1
	dbf	d0,hole_bit_loop
	rts	



strings:
	lea.l	length_tab(pc),a1
	moveq.l	#3,d2
get_length_bit:	
	bsr.s	get_1_bit
	dbcc	d2,get_length_bit
no_length_bit:	
	moveq.l	#0,d4
	moveq.l	#0,d1
	move.b	1(a1,d2.w),d0
	ext.w	d0
	bmi.s	no_｜er
get_｜er: bsr.s	get_d0_bits
no_｜er:	move.b	6(a1,d2.w),d4
	add.w	d1,d4
	beq.s	get_offset_2


	lea.l	more_offset(pc),a1
	moveq.l	#1,d2
getoffs:	bsr.s	get_1_bit
	dbcc	d2,getoffs
	moveq.l	#0,d1
	move.b	1(a1,d2.w),d0
	ext.w	d0
	bsr.s	get_d0_bits
	add.w	d2,d2
	add.w	6(a1,d2.w),d1
	bpl.s	depack_bytes
	sub.w	d4,d1
	bra.s	depack_bytes


get_offset_2:	
	moveq.l	#0,d1
	moveq.l	#5,d0
	moveq.l	#-1,d2
	bsr.s	get_1_bit
	bcc.s	less_40
	moveq.l	#8,d0
	moveq.l	#$3f,d2
less_40:
	bsr.s	get_d0_bits
	add.w	d2,d1

depack_bytes:
	lea.l	2(a6,d4.w),a1
	adda.w	d1,a1
	move.b	-(a1),-(a6)
dep_b:	move.b	-(a1),-(a6)
	dbf	d4,dep_b
	bra	normal_bytes


direkt_tab:
	dc.l $7fff000e,$00ff0007,$00070002,$00030001,$00030001
	dc.l     270-1,	15-1,	 8-1,	 5-1,	 2-1

length_tab:
	dc.b 9,1,0,-1,-1
	dc.b 8,4,2,1,0

more_offset:
	dc.b	  11,   4,   7,  0
	dc.w	$11f,  -1, $1f
