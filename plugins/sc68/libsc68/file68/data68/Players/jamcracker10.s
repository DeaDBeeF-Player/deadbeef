;*
;* JamCracker V1.0 Replay routine, written by M. Gemmel
;*
;* Make sure you have read the ReadMe file on this disk too.
;*
;* This is not a demonstration source, showing how to write
;* a song-play routine.  This source is merely ment for
;* inclusion in other sources, or as assembly module for
;* linkage with other programs.
;*
;* This source was written for the GenIm V2.12 assembler
;* and can be easily modified for other assemblers.
;* The song should be inserted at 'mysong' near the bottom of
;* this source, in chip memory.
;*
;* This is the correct calling procedure
;* Don't forget to save any important registers
;* because I don't care

	include "lib/org.s"

	bra	pp_init
	bra	pp_end
	bra	pp_play

;*** This is the actual replay routine

wavesize=$40

pp_init:
	addq.w	#4,a0
	move.w	(a0)+,d0
	move.w	d0,d1
	move.l	a0,instable
	mulu	#it_sizeof,d0
	adda.w	d0,a0

	move.w	(a0)+,d0
	move.w	d0,d2
	move.l	a0,patttable
	mulu	#pt_sizeof,d0
	adda.w	d0,a0

	move.w	(a0)+,d0
	move.w	d0,songlen
	move.l	a0,songtable
	lsl.w	#1,d0
	adda.w	d0,a0

	movea.l	patttable,a1
	move.w	d2,d0
	subq.w	#1,d0
.l0	move.l	a0,pt_address(a1)
	move.w	pt_size(a1),d3
	mulu	#nt_sizeof*4,d3
	adda.w	d3,a0
	add.w	#pt_sizeof,a1
	dbf	d0,.l0

	movea.l	instable,a1
	move.w	d1,d0
	subq.w	#1,d0
.l1	move.l	a0,it_address(a1)
	move.l	it_size(a1),d2
	adda.l	d2,a0
	adda.w	#it_sizeof,a1
	dbf	d0,.l1


	move.l	songtable,pp_songptr
	move.w	songlen,pp_songcnt
	movea.l	pp_songptr,a0
	move.w	(a0),d0
	mulu	#pt_sizeof,d0
	add.l	patttable,d0
	movea.l	d0,a0
	move.l	a0,pp_pattentry
	move.b	pt_size+1(a0),pp_notecnt
	move.l	pt_address(a0),pp_address
	move.b	#6,pp_wait
	move.b	#1,pp_waitcnt
	clr.w	pp_nullwave
	move.w	#$000f,$dff096

	lea	pp_variables,a0
	lea	$dff0a0,a1
	move.w	#$0001,d1
	move.w	#2*wavesize,d2
	move.w	#3,d0
.l2	clr.w	8(a1)
	move.w	d2,pv_waveoffset(a0)
	move.w	d1,pv_dmacon(a0)
	move.l	a1,pv_custbase(a0)
	move.l	#pp_periods,pv_peraddress(a0)
	move.w	#1019,pv_pers(a0)
	clr.w	pv_pers+2(a0)
	clr.w	pv_pers+4(a0)
	clr.l	pv_por(a0)
	clr.w	pv_porlevel(a0)
	clr.l	pv_vib(a0)
	clr.l	pv_vol(a0)
	move.w	#$0040,pv_vollevel(a0)
	clr.l	pv_phase(a0)
	clr.w	pv_vibcnt(a0)
	clr.b	pv_flags(a0)
	adda.w	#pv_sizeof,a0
	adda.w	#$0010,a1
	lsl.w	#1,d1
	addi.w	#wavesize,d2
	dbf	d0,.l2
	rts


pp_end	clr.w	$dff0a8
	clr.w	$dff0b8
	clr.w	$dff0c8
	clr.w	$dff0d8
	move.w	#$000f,$dff096
	rts


pp_play	subq.b	#1,pp_waitcnt
	bne.s	.l0
	bsr	pp_nwnt
	move.b	pp_wait,pp_waitcnt

.l0	lea	pp_variables,a1
	bsr.s	pp_uvs
	lea	pp_variables+pv_sizeof,a1
	bsr.s	pp_uvs
	lea	pp_variables+2*pv_sizeof,a1
	bsr.s	pp_uvs
	lea	pp_variables+3*pv_sizeof,a1


pp_uvs	movea.l	pv_custbase(a1),a0

.l0	move.w	pv_pers(a1),d0
	bne.s	.l1
	bsr	pp_rot
	bra.s	.l0
.l1	add.w	pv_por(a1),d0
	tst.w	pv_por(a1)
	beq.s	.l1c
	bpl.s	.l1a
	cmp.w	pv_porlevel(a1),d0
	bge.s	.l1c
	bra.s	.l1b
.l1a	cmp.w	pv_porlevel(a1),d0
	ble.s	.l1c
.l1b	move.w	pv_porlevel(a1),d0

.l1c	add.w	pv_vib(a1),d0
	cmpi.w	#135,d0
	bge.s	.l1d
	move.w	#135,d0
	bra.s	.l1e
.l1d	cmpi.w	#1019,d0
	ble.s	.l1e
	move.w	#1019,d0
.l1e	move.w	d0,6(a0)
	bsr	pp_rot


	move.w	pv_deltapor(a1),d0
	add.w	d0,pv_por(a1)
	cmpi.w	#-1019,pv_por(a1)
	bge.s	.l3
	move.w	#-1019,pv_por(a1)
	bra.s	.l5
.l3	cmpi.w	#1019,pv_por(a1)
	ble.s	.l5
	move.w	#1019,pv_por(a1)


.l5	tst.b	pv_vibcnt(a1)
	beq.s	.l7
	move.w	pv_deltavib(a1),d0
	add.w	d0,pv_vib(a1)
	subq.b	#1,pv_vibcnt(a1)
	bne.s	.l7
	neg.w	pv_deltavib(a1)
	move.b	pv_vibmax(a1),pv_vibcnt(a1)


.l7	move.w	pv_dmacon(a1),d0
	move.w	pv_vol(a1),8(a0)
	move.w	pv_deltavol(a1),d0
	add.w	d0,pv_vol(a1)
	tst.w	pv_vol(a1)
	bpl.s	.l8
	clr.w	pv_vol(a1)
	bra.s	.la
.l8	cmpi.w	#$40,pv_vol(a1)
	ble.s	.la
	move.w	#$40,pv_vol(a1)


.la	btst	#1,pv_flags(a1)
	beq.s	.l10
	movea.l	pv_insaddress(a1),a0
	move.w	pv_waveoffset(a1),d0
	neg.w	d0
	lea	0(a0,d0.w),a2
	movea.l	a2,a3
	move.w	pv_phase(a1),d0
	lsr.w	#2,d0
	adda.w	d0,a3

	move.w	#wavesize-1,d0
.lb	move.b	(a2)+,d1
	ext.w	d1
	move.b	(a3)+,d2
	ext.w	d2
	add.w	d1,d2
	asr.w	#1,d2
	move.b	d2,(a0)+
	dbf	d0,.lb

	move.w	pv_deltaphase(a1),d0
	add.w	d0,pv_phase(a1)
	cmpi.w	#wavesize<<2,pv_phase(a1)
	blt.s	.l10
	subi.w	#wavesize<<2,pv_phase(a1)

.l10	rts


pp_rot	move.w	pv_pers(a1),d0
	move.w	pv_pers+2(a1),pv_pers(a1)
	move.w	pv_pers+4(a1),pv_pers+2(a1)
	move.w	d0,pv_pers+4(a1)
	rts


pp_nwnt	movea.l	pp_address,a0
	addi.l	#4*nt_sizeof,pp_address
	subq.b	#1,pp_notecnt
	bne.s	.l5

.l0	addq.l	#2,pp_songptr
	subq.w	#1,pp_songcnt
	bne.s	.l1
	move.l	songtable,pp_songptr
	move.w	songlen,pp_songcnt
.l1	movea.l	pp_songptr,a1
	move.w	(a1),d0
	mulu	#pt_sizeof,d0
	add.l	patttable,d0
	movea.l	d0,a1
	move.b	pt_size+1(a1),pp_notecnt
	move.l	pt_address(a1),pp_address


.l5	clr.w	pp_tmpdmacon
	lea	pp_variables,a1
	bsr	pp_nnt
	add.w	#nt_sizeof,a0
	lea	pp_variables+pv_sizeof,a1
	bsr	pp_nnt
	add.w	#nt_sizeof,a0
	lea	pp_variables+2*pv_sizeof,a1
	bsr	pp_nnt
	add.w	#nt_sizeof,a0
	lea	pp_variables+3*pv_sizeof,a1
	bsr	pp_nnt


	move.w	pp_tmpdmacon,$dff096
	move.w	#300,d0
.l6	dbf	d0,.l6

	lea	pp_variables,a1
	bsr	pp_scr
	lea	pp_variables+pv_sizeof,a1
	bsr.s	pp_scr
	lea	pp_variables+2*pv_sizeof,a1
	bsr.s	pp_scr
	lea	pp_variables+3*pv_sizeof,a1
	bsr.s	pp_scr

	bset	#7,pp_tmpdmacon
	move.w	pp_tmpdmacon,$dff096
	move.w	#300,d0
.l7	dbf	d0,.l7


	move.l	pp_variables+pv_insaddress,$dff0a0
	move.w	pp_variables+pv_inslen,$dff0a4
	move.l	pp_variables+pv_sizeof+pv_insaddress,$dff0b0
	move.w	pp_variables+pv_sizeof+pv_inslen,$dff0b4
	move.l	pp_variables+2*pv_sizeof+pv_insaddress,$dff0c0
	move.w	pp_variables+2*pv_sizeof+pv_inslen,$dff0c4
	move.l	pp_variables+3*pv_sizeof+pv_insaddress,$dff0d0
	move.w	pp_variables+3*pv_sizeof+pv_inslen,$dff0d4

	rts


pp_scr	move.w	pp_tmpdmacon,d0
	and.w	pv_dmacon(a1),d0
	beq.s	.l5

	movea.l	pv_custbase(a1),a0
	move.l	pv_insaddress(a1),(a0)
	move.w	pv_inslen(a1),4(a0)
	move.w	pv_pers(a1),6(a0)
	btst	#0,pv_flags(a1)
	bne.s	.l5
	move.l	#pp_nullwave,pv_insaddress(a1)
	move.w	#1,pv_inslen(a1)

.l5	rts


pp_nnt	move.b	nt_period(a0),d1
	beq	.l5


	andi.l	#$000000ff,d1
	lsl.w	#1,d1
	addi.l	#pp_periods-2,d1
	movea.l	d1,a2

	btst	#6,nt_speed(a0)
	beq.s	.l2
	move.w	(a2),pv_porlevel(a1)
	bra	.l5


.l2	move.w	pv_dmacon(a1),d0
	or.w	d0,pp_tmpdmacon

	move.l	a2,pv_peraddress(a1)
	move.w	(a2),pv_pers(a1)
	move.w	(a2),pv_pers+2(a1)
	move.w	(a2),pv_pers+4(a1)

	clr.w	pv_por(a1)

	move.b	nt_instr(a0),d0
	ext.w	d0
	mulu	#it_sizeof,d0
	add.l	instable,d0
	movea.l	d0,a2
	tst.l	it_address(a2)
	bne.s	.l1
	move.l	#pp_nullwave,pv_insaddress(a1)
	move.w	#1,pv_inslen(a1)
	clr.b	pv_flags(a1)
	bra.s	.l5

.l1	movea.l	it_address(a2),a3
	btst	#1,it_flags(a2)
	bne.s	.l0a
	move.l	it_size(a2),d0
	lsr.l	#1,d0
	move.w	d0,pv_inslen(a1)
	bra.s	.l0
.l0a	move.w	pv_waveoffset(a1),d0
	adda.w	d0,a3
	move.w	#wavesize>>1,pv_inslen(a1)
.l0	move.l	a3,pv_insaddress(a1)
	move.b	it_flags(a2),pv_flags(a1)
	move.w	pv_vollevel(a1),pv_vol(a1)


.l5	move.b	nt_speed(a0),d0
	andi.b	#$0f,d0
	beq.s	.l6
	move.b	d0,pp_wait


.l6	movea.l	pv_peraddress(a1),a2
	move.b	nt_arpeggio(a0),d0
	beq.s	.l9
	cmpi.b	#$ff,d0
	bne.s	.l7
	move.w	(a2),pv_pers(a1)
	move.w	(a2),pv_pers+2(a1)
	move.w	(a2),pv_pers+4(a1)
	bra.s	.l9
.l7	andi.b	#$0f,d0
	lsl.b	#1,d0
	ext.w	d0
	move.w	0(a2,d0.w),pv_pers+4(a1)
	move.b	nt_arpeggio(a0),d0
	lsr.b	#4,d0
	lsl.b	#1,d0
	ext.w	d0
	move.w	0(a2,d0.w),pv_pers+2(a1)
	move.w	(a2),pv_pers(a1)


.l9	move.b	nt_vibrato(a0),d0
	beq.s	.ld
	cmpi.b	#$ff,d0
	bne.s	.la
	clr.l	pv_vib(a1)
	clr.b	pv_vibcnt(a1)
	bra.s	.ld
.la	clr.w	pv_vib(a1)
	andi.b	#$0f,d0
	ext.w	d0
	move.w	d0,pv_deltavib(a1)
	move.b	nt_vibrato(a0),d0
	lsr.b	#4,d0
	move.b	d0,pv_vibmax(a1)
	lsr.b	#1,d0
	move.b	d0,pv_vibcnt(a1)


.ld	move.b	nt_phase(a0),d0
	beq.s	.l10
	cmpi.b	#$ff,d0
	bne.s	.le
	clr.l	pv_phase(a1)
	bra.s	.l10
.le	andi.b	#$0f,d0
	ext.w	d0
	move.w	d0,pv_deltaphase(a1)
	clr.w	pv_phase(a1)


.l10	move.b	nt_volume(a0),d0
	bne.s	.l10a
	btst	#7,nt_speed(a0)
	beq.s	.l16
	bra.s	.l11a
.l10a	cmpi.b	#$ff,d0
	bne.s	.l11
	clr.w	pv_deltavol(a1)
	bra.s	.l16
.l11	btst	#7,nt_speed(a0)
	beq.s	.l12
.l11a	move.b	d0,pv_vol+1(a1)
	move.b	d0,pv_vollevel+1(a1)
	clr.w	pv_deltavol(a1)
	bra.s	.l16
.l12	bclr	#7,d0
	beq.s	.l13
	neg.b	d0
.l13	ext.w	d0
	move.w	d0,pv_deltavol(a1)


.l16	move.b	nt_porta(a0),d0
	beq.s	.l1a
	cmpi.b	#$ff,d0
	bne.s	.l17
	clr.l	pv_por(a1)
	bra.s	.l1a
.l17	clr.w	pv_por(a1)
	btst	#6,nt_speed(a0)
	beq.s	.l17a
	move.w	pv_porlevel(a1),d1
	cmp.w	pv_pers(a1),d1
	bgt.s	.l17c
	neg.b	d0
	bra.s	.l17c
.l17a	bclr	#7,d0
	bne.s	.l18
	neg.b	d0
	move.w	#135,pv_porlevel(a1)
	bra.s	.l17c
.l18	move.w	#1019,pv_porlevel(a1)
.l17c	ext.w	d0
.l18a	move.w	d0,pv_deltapor(a1)


.l1a	rts


;* Replayer data

pp_periods	dc.w	1019,962,908,857,809,763,720,680,642,606,572,540
		dc.w	509,481,454,428,404,381,360,340,321,303,286,270
		dc.w	254,240,227,214,202,190,180,170,160,151,143,135
		dc.w	135,135,135,135,135,135,135,135,135
		dc.w	135,135,135,135,135,135

songlen		ds.w	1
songtable	ds.l	1
instable	ds.l	1
patttable	ds.l	1

pp_wait		ds.b	1
pp_waitcnt	ds.b	1
pp_notecnt	ds.b	1
pp_address	ds.l	1
pp_songptr	ds.l	1
pp_songcnt	ds.w	1
pp_pattentry	ds.l	1
pp_tmpdmacon	ds.w	1
pp_variables	ds.b	4*48

pp_nullwave	ds.w	1

		rsreset
it_name		rs.b	31
it_flags	rs.b	1
it_size		rs.l	1
it_address	rs.l	1
it_sizeof	rs.w	0
		rsreset
pt_size		rs.w	1
pt_address	rs.l	1
pt_sizeof	rs.w	0
		rsreset
nt_period	rs.b	1
nt_instr	rs.b	1
nt_speed	rs.b	1
nt_arpeggio	rs.b	1
nt_vibrato	rs.b	1
nt_phase	rs.b	1
nt_volume	rs.b	1
nt_porta	rs.b	1
nt_sizeof	rs.w	0
		rsreset
pv_waveoffset	rs.w	1
pv_dmacon	rs.w	1
pv_custbase	rs.l	1
pv_inslen	rs.w	1
pv_insaddress	rs.l	1
pv_peraddress	rs.l	1
pv_pers		rs.w	3
pv_por		rs.w	1
pv_deltapor	rs.w	1
pv_porlevel	rs.w	1
pv_vib		rs.w	1
pv_deltavib	rs.w	1
pv_vol		rs.w	1
pv_deltavol	rs.w	1
pv_vollevel	rs.w	1
pv_phase	rs.w	1
pv_deltaphase	rs.w	1
pv_vibcnt	rs.b	1
pv_vibmax	rs.b	1
pv_flags	rs.b	1
pv_sizeof	rs.w	0
