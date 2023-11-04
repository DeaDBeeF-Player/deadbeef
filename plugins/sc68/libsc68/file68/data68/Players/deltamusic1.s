;	Delta Music 1 replay routine
;	adapted for sc68 by Gerald Schnabel <gschnabel@gmx.de>

	include "lib/org.s"
	
	bra.w	DM_init
	rts
	rts
	bra.w	DM_play

; -------------------------------------------
; ----      DELTA MUSIC REPLAY V1.0      ----
; ----      ~~~~~~~~~~~~~~~~~~~~~~~      ----
; ----                                   ----
; ----  coded by : Bent Nielsen (SHOGUN) ----
; ----             Kyradservej 19B       ----
; ----             8700 Horsens          ----
; ----             Denmark               ----
; ----       tlf.  75-601-868            ----
; -------------------------------------------
; ---- contact me if you want the editor ----
; ---- sources coded .........           ----
; -------------------------------------------

speed = 6	; play speed

; ////  hardware  \\\\
h_sound     = 0
h_length    = 4
h_frequency = 6
h_volume    = 8

; ////  instrument  \\\\
s_attack_step     = 0
s_attack_delay    = 1
s_decay_step      = 2
s_decay_delay     = 3
s_sustain         = 4
s_release_step    = 6
s_release_delay   = 7
s_volume          = 8
s_vibrator_wait   = 9
s_vibrator_step   = 10
s_vibrator_length = 11
s_bendrate        = 12
s_portamento      = 13
s_sample          = 14
s_table_delay     = 15
s_arpeggio        = 16
s_sound_length    = 24
s_repeat          = 26
s_repeat_length   = 28
s_table           = 30
s_sounddata       = 78

; ////  channel  \\\\
c_hardware        = 0
c_dma             = 4
c_sounddata       = 6
c_frequency       = 10
c_sound_table     = 12
c_sound_table_cnt = 16
c_sound_table_del = 17
c_track           = 18
c_track_cnt       = 22
c_block           = 24
c_block_cnt       = 28
c_vibrator_wait   = 32
c_vibrator_length = 33
c_vibrator_pos    = 34
c_vibrator_cmp    = 35
c_vibrator_freq   = 36
c_old_frequency   = 38
c_frequency_data  = 40
c_actual_volume   = 41
c_attack_delay    = 42
c_decay_delay     = 43
c_sustain         = 44
c_release_delay   = 46
c_play_speed      = 47
c_bendrate_freq   = 48
c_transpose       = 50
c_status          = 51
c_arpeggio_cnt    = 52
c_arpeggio_data   = 53
c_arpeggio_on     = 54
c_effect_number   = 55
c_effect_data     = 56
; next 57

DM_play:
	movem.l	d0-d7/a0-a6,-(a7)
	lea	channel1(pc),a6
	bsr	DM_calc_frequency
	lea	channel2(pc),a6
	bsr	DM_calc_frequency
	lea	channel3(pc),a6
	bsr	DM_calc_frequency
	lea	channel4(pc),a6
	bsr	DM_calc_frequency

	move.w	#$800f,$dff096
DM_sample_handler:
	move.w	#200,d0
DM_swait:
	dbra	d0,DM_swait

	lea	channel1(pc),a6
	move.l	c_hardware(a6),a4
	move.l	c_sounddata(a6),a5
	tst.b	s_sample(a5)
	beq.s	DM_no_sample1
	move.w	s_repeat_length(a5),h_length(a4)
	moveq	#0,d7
	move.w	s_repeat(a5),d7
	add.l	a5,d7
	add.l	#s_table,d7
	move.l	d7,h_sound(a4)
DM_no_sample1:

	lea	channel2(pc),a6
	move.l	c_hardware(a6),a4
	move.l	c_sounddata(a6),a5
	tst.b	s_sample(a5)
	beq.s	DM_no_sample2
	move.w	s_repeat_length(a5),h_length(a4)
	moveq	#0,d7
	move.w	s_repeat(a5),d7
	add.l	a5,d7
	add.l	#s_table,d7
	move.l	d7,h_sound(a4)
DM_no_sample2:

	lea	channel3(pc),a6
	move.l	c_hardware(a6),a4
	move.l	c_sounddata(a6),a5
	tst.b	s_sample(a5)
	beq.s	DM_no_sample3
	move.w	s_repeat_length(a5),h_length(a4)
	moveq	#0,d7
	move.w	s_repeat(a5),d7
	add.l	a5,d7
	add.l	#s_table,d7
	move.l	d7,h_sound(a4)
DM_no_sample3:

	lea	channel4(pc),a6
	move.l	c_hardware(a6),a4
	move.l	c_sounddata(a6),a5
	tst.b	s_sample(a5)
	beq.s	DM_no_sample4
	move.w	s_repeat_length(a5),h_length(a4)
	moveq	#0,d7
	move.w	s_repeat(a5),d7
	add.l	a5,d7
	add.l	#s_table,d7
	move.l	d7,h_sound(a4)
DM_no_sample4:
	movem.l	(a7)+,d0-d7/a0-a6
	rts


DM_calc_frequency:
	move.l	c_hardware(a6),a4
	move.l	c_sounddata(a6),a5

	subq.b	#1,c_play_speed(a6)
	bne	DM_block_con
	move.b	play_speed,c_play_speed(a6)

	tst.l	c_block_cnt(a6)
	bne.s	DM_check_block
DM_track_step:
	move.l	c_track(a6),a0
	move.w	c_track_cnt(a6),d7
	move.w	(a0,d7.w),d0
	cmp.w	#-1,d0
	bne.s	DM_track_con
	move.w	2(a0,d7.w),d0
	and.w	#$7ff,d0
	asl.w	#1,d0
	move.w	d0,c_track_cnt(a6)
	bra.s	DM_track_step
DM_track_con:
	move.b	d0,c_transpose(a6)
	asr.l	#2,d0
	and.l	#%00000000000000000011111111000000,d0
	add.l	blocks(pc),d0
	move.l	d0,c_block(a6)
	addq.w	#2,c_track_cnt(a6)

DM_check_block:
	move.l	c_block(a6),a0
	add.l	c_block_cnt(a6),a0

	tst.b	2(a0)
	beq.s	DM_no_new_effect
	move.b	2(a0),c_effect_number(a6)
	move.b	3(a0),c_effect_data(a6)

DM_no_new_effect:
	moveq	#0,d0
	move.b	1(a0),d0
	beq	DM_test_effect
	add.b	c_transpose(a6),d0
	move.b	d0,c_frequency_data(a6)

	move.w	c_dma(a6),d0
	sub.w	#$8000,d0
	move.w	d0,$dff096

	moveq	#0,d0
	move.b	d0,c_status(a6)
	move.w	d0,c_bendrate_freq(a6)
	move.w	d0,c_arpeggio_cnt(a6)
	move.b	d0,c_arpeggio_on(a6)

	move.b	2(a0),c_effect_number(a6)
	move.b	3(a0),c_effect_data(a6)

	lea	snd_table(pc),a1
	move.b	(a0),d0
	asl.l	#2,d0
	move.l	(a1,d0.l),d0
	move.l	d0,c_sounddata(a6)

	move.l	d0,a5
	add.l	#s_table,d0
	move.l	d0,c_sound_table(a6)
	clr.b	c_sound_table_cnt(a6)
	tst.b	s_sample(a5)
	beq.s	DM_no_sample_clear
	clr.w	s_table(a5)
	move.l	d0,h_sound(a4)
DM_no_sample_clear:

	move.w	s_sound_length(a5),d0
	asr.w	#1,d0
	move.w	d0,h_length(a4)

	move.b	s_vibrator_wait(a5),c_vibrator_wait(a6)
	move.b	s_vibrator_length(a5),d0
	move.b	d0,c_vibrator_length(a6)
	move.b	d0,c_vibrator_pos(a6)
	asl.b	#1,d0
	move.b	d0,c_vibrator_cmp(a6)
	clr.b	c_actual_volume(a6)
	clr.b	c_sound_table_del(a6)
	clr.b	c_sound_table_cnt(a6)
	clr.b	c_attack_delay(a6)
	clr.b	c_decay_delay(a6)
	move.w	s_sustain(a5),c_sustain(a6)
	clr.b	c_release_delay(a6)

DM_test_effect:
	addq.l	#4,c_block_cnt(a6)
	cmp.l	#64,c_block_cnt(a6)
	bne.s	DM_block_con
	clr.l	c_block_cnt(a6)
DM_block_con:

	tst.b	s_sample(a5)
	bne.s	DM_portamento_handler
	tst.b	c_sound_table_del(a6)
	beq.s	DM_sound_table_handler
	subq.b	#1,c_sound_table_del(a6)
	bra.s	DM_portamento_handler

DM_sound_table_handler:
	move.b	s_table_delay(a5),c_sound_table_del(a6)

DM_sound_read_again:
	move.l	c_sound_table(a6),a0
	moveq	#0,d6
	move.b	c_sound_table_cnt(a6),d6
	cmp.b	#48,d6
	bmi.s	DM_sound_read_c
	clr.b	c_sound_table_cnt(a6)
	moveq	#0,d6
DM_sound_read_c:
	add.l	d6,a0
	moveq	#0,d7
	move.b	(a0),d7

	bpl.s	DM_new_sounddata
	cmp.b	#$ff,d7
	bne.s	DM_sound_new_speed
	move.b	1(a0),d7
	move.b	d7,c_sound_table_cnt(a6)
	bra.s	DM_sound_read_again
DM_sound_new_speed:
	and.b	#127,d7
	move.b	d7,s_table_delay(a5)	
	addq.b	#1,c_sound_table_cnt(a6)
	bra.s	DM_sound_read_again

DM_new_sounddata:
	asl.l	#5,d7
	add.l	#s_sounddata,d7
	add.l	c_sounddata(a6),d7
	move.l	d7,h_sound(a4)		; Set hardware sound data
	addq.b	#1,c_sound_table_cnt(a6)

DM_portamento_handler:
	tst.b	s_portamento(a5)
	beq.s	DM_vibrator_handler
	move.w	c_frequency(a6),d1
	bne.s	DM_porta_con
	moveq	#0,d0
	lea	freq_table(pc),a1
	move.b	c_frequency_data(a6),d0
	asl.w	#1,d0
	move.w	(a1,d0.w),d0
	add.w	c_bendrate_freq(a6),d0
	move.w	d0,c_frequency(a6)
	bra.s	DM_vibrator_handler

DM_porta_con:
	moveq	#0,d0
	moveq	#0,d2
	move.b	s_portamento(a5),d2
	lea	freq_table(pc),a1
	move.b	c_frequency_data(a6),d0
	asl.w	#1,d0
	move.w	(a1,d0.w),d0
	add.w	c_bendrate_freq(a6),d0
	cmp.w	d0,d1
	beq.s	DM_vibrator_handler
	blo.s	DM_porta_low
	sub.w	d2,d1
	cmp.w	d0,d1
	bpl.s	DM_porta_high_con
	move.w	d0,c_frequency(a6)
	bra.s	DM_vibrator_handler
DM_porta_high_con:
	move.w	d1,c_frequency(a6)
	bra.s	DM_vibrator_handler
DM_porta_low:
	add.w	d2,d1
	cmp.w	d0,d1
	bmi.s	DM_porta_low_con
	move.w	d0,c_frequency(a6)
	bra.s	DM_vibrator_handler
DM_porta_low_con:
	move.w	d1,c_frequency(a6)


DM_vibrator_handler:
	tst.b	c_vibrator_wait(a6)
	beq.s	DM_calc_vibrator
	subq.b	#1,c_vibrator_wait(a6)
	bra.s	DM_bendrate_handler
DM_calc_vibrator:
	moveq	#0,d0
	moveq	#0,d1
	move.b	c_vibrator_pos(a6),d0
	move.b	d0,d2
	move.b	s_vibrator_step(a5),d1
	mulu	d1,d0
	move.w	d0,c_vibrator_freq(a6)

	btst	#0,c_status(a6)
	bne.s	DM_vibrator_minus
DM_vibrator_plus:
	addq.b	#1,d2
	cmp.b	c_vibrator_cmp(a6),d2
	bne.s	DM_vibrator_no_reset
	eor.b	#1,c_status(a6)
DM_vibrator_no_reset:
	move.b	d2,c_vibrator_pos(a6)
	bra.s	DM_bendrate_handler

DM_vibrator_minus:
	subq.b	#1,d2
	bne.s	DM_vibrator_no_reset2
	eor.b	#1,c_status(a6)
DM_vibrator_no_reset2:
	move.b	d2,c_vibrator_pos(a6)

DM_bendrate_handler:
	moveq	#0,d0
	move.l	c_sounddata(a6),a1
	move.b	s_bendrate(a1),d0
	bpl.s	DM_rate_minus
	neg.b	d0

	add.w	d0,c_bendrate_freq(a6)
	bra.s	DM_effect_handler

DM_rate_minus:
	sub.w	d0,c_bendrate_freq(a6)

DM_effect_handler:
	moveq	#0,d0
	moveq	#0,d1
	move.b	c_effect_data(a6),d0
	move.b	c_effect_number(a6),d1
	lea	effect_table(pc),a1
	and.b	#$1f,d1
	asl.l	#2,d1
	move.l	(a1,d1.w),a1
	jsr	(a1)

DM_arpeggio_handler:
	move.l	a5,a1
	add.l	#s_arpeggio,a1
	moveq	#0,d0
	move.b	c_arpeggio_cnt(a6),d0
	move.b	(a1,d0.w),d1
	addq.b	#1,c_arpeggio_cnt(a6)
	and.b	#%00000111,c_arpeggio_cnt(a6)

DM_store_frequency:
	lea	freq_table(pc),a1
	move.b	c_frequency_data(a6),d0
	add.b	d1,d0
	asl.w	#1,d0
	move.w	(a1,d0.l),d0

	moveq	#0,d1
	moveq	#0,d2
	move.b	c_vibrator_length(a6),d1
	move.b	s_vibrator_step(a5),d2
	mulu	d2,d1
	sub.w	d1,d0
	add.w	c_bendrate_freq(a6),d0
	tst.b	s_portamento(a5)
	beq.s	DM_store_no_port
	move.w	c_frequency(a6),d0
	bra.s	DM_store_port
DM_store_no_port:
	clr.w	c_frequency(a6)
DM_store_port:
	add.w	c_vibrator_freq(a6),d0
	move.w	d0,h_frequency(a4)

DM_volume_handler:
	moveq	#0,d1		; actual volume
	move.b	c_actual_volume(a6),d1

	move.b	c_status(a6),d0
	and.b	#%00001110,d0

	tst.b	d0
	bne.s	DM_test_decay

	tst.b	c_attack_delay(a6)
	beq.s	DM_attack_handler
	subq.b	#1,c_attack_delay(a6)
	bra	DM_volume_exit
DM_attack_handler:
	move.b	s_attack_delay(a5),c_attack_delay(a6)
	add.b	s_attack_step(a5),d1
	cmp.b	#64,d1
	blo.s	DM_attack_con
	or.b	#%00000010,d0
	or.b	#%00000010,c_status(a6)
	move.b	#64,d1
DM_attack_con:


DM_test_decay:
	cmp.b	#%00000010,d0
	bne.s	DM_test_sustain

	tst.b	c_decay_delay(a6)
	beq.s	DM_decay_handler
	subq.b	#1,c_decay_delay(a6)
	bra.s	DM_volume_exit
DM_decay_handler:
	move.b	s_decay_delay(a5),c_decay_delay(a6)
	move.b	s_volume(a5),d2
	sub.b	s_decay_step(a5),d1
	cmp.b	d2,d1
	bhi.s	DM_decay_con
	move.b	s_volume(a5),d1
	or.b	#%00000110,d0
	or.b	#%00000110,c_status(a6)
DM_decay_con:

DM_test_sustain:
	cmp.b	#%00000110,d0
	bne.s	DM_test_release

	tst.w	c_sustain(a6)
	beq.s	DM_sustain_handler
	subq.w	#1,c_sustain(a6)
	bra.s	DM_volume_exit
DM_sustain_handler:
	or.b	#%00001110,d0
	or.b	#%00001110,c_status(a6)

DM_test_release:
	cmp.b	#%00001110,d0
	bne.s	DM_volume_exit

	tst.b	c_release_delay(a6)
	beq.s	DM_release_handler
	subq.b	#1,c_release_delay(a6)
	bra.s	DM_volume_exit
DM_release_handler:
	move.b	s_release_delay(a5),c_release_delay(a6)
	sub.b	s_release_step(a5),d1
	bpl.s	DM_release_con
	and.b	#%00001001,c_status(a6)
	moveq	#0,d1
DM_release_con:

DM_volume_exit:
	move.b	d1,c_actual_volume(a6)

	move.w	d1,h_volume(a4)
	rts


; ----  INIT MUSIC  ----

all_check = 0
trk1      = 4


DM_init:
	lea	DM_data(pc),a0
	lea	26*4(a0),a0
	lea	track1(pc),a1
	moveq	#24,d7
DM_init_loop:
	move.l	a0,(a1)+
	dbra	d7,DM_init_loop

	moveq	#23,d6
	lea	track1+(24*4)(pc),a1
DM_init_loop2:
	lea	DM_data(pc),a0
	lea	4(a0),a0
	move.l	d6,d7
DM_init_loop3:
	move.l	(a0)+,d0
	add.l	d0,(a1)
	dbra	d7,DM_init_loop3
	subq.l	#4,a1
	dbra	d6,DM_init_loop2

	lea	$dff0a0,a0
	lea	channel1(pc),a6
	bsr.s	DM_setup
	add.l	#16,a0
	lea	channel2(pc),a6
	bsr.s	DM_setup
	add.l	#16,a0
	lea	channel3(pc),a6
	bsr.s	DM_setup
	add.l	#16,a0
	lea	channel4(pc),a6
	bsr.s	DM_setup
	move.w	#$8001,channel1+c_dma
	move.w	#$8002,channel2+c_dma
	move.w	#$8004,channel3+c_dma
	move.w	#$8008,channel4+c_dma
	move.l	track1(pc),channel1+c_track
	move.l	track2(pc),channel2+c_track
	move.l	track3(pc),channel3+c_track
	move.l	track4(pc),channel4+c_track
	rts

DM_setup:
	move.l	a0,c_hardware(a6)
	move.w	#16,h_length(a0)
	clr.w	h_volume(a0)
	move.l	#safe_zero,c_sounddata(a6)
	clr.w	c_frequency(a6)
	move.l	snd_table(pc),d0
	add.l	#16,d0
	move.l	d0,c_sound_table(a6)
	clr.w	c_sound_table_cnt(a6)
	clr.w	c_track_cnt(a6)
	move.l	blocks(pc),c_block(a6)
	clr.l	c_block_cnt(a6)
	clr.l	c_vibrator_wait(a6)
	clr.l	c_vibrator_freq(a6)
	clr.l	c_frequency_data(a6)
	move.l	#1,c_sustain(a6)
	clr.l	c_bendrate_freq(a6)
	clr.l	c_arpeggio_cnt(a6)
	clr.w	c_effect_data(a6)
	rts

; ----  EFFECT ROUTINES  ----

eff0:
	rts
eff1:
	and.b	#15,d0			; set play speed
	beq.s	eff1_exit
	move.b	d0,play_speed
eff1_exit:
	rts
eff2:
	sub.w	d0,c_bendrate_freq(a6)	; slide freq up
	rts
eff3:
	add.w	d0,c_bendrate_freq(a6)	; slide freq down
	rts
eff4:
;	tst.b	d0
;	beq	led_off
;	bset	#1,$bfe001		; led on/off
;	rts
led_off:
;	bclr	#1,$bfe001
	rts
eff5:
	move.b	d0,s_vibrator_wait(a5)	; set vibrator wait
	rts
eff6:
	move.b	d0,s_vibrator_step(a5)	; set vibrator step
	rts
eff7:
	move.b	d0,s_vibrator_length(a5); set vibrator length
	rts
eff8:
	move.b	d0,s_bendrate(a5)	; set bendrate
	rts
eff9:
	move.b	d0,s_portamento(a5)	; set portamento
	rts
effA:
	cmp.b	#65,d0
	bmi.s	effA_con
	move.b	#64,d0
effA_con:
	move.b	d0,s_volume(a5)		; set volume
	rts
effB:
	move.b	d0,s_arpeggio(a5)	; set arp 1
	rts
effC:
	move.b	d0,s_arpeggio+1(a5)	; set arp 2
	rts
effD:
	move.b	d0,s_arpeggio+2(a5)	; set arp 3
	rts
effE:
	move.b	d0,s_arpeggio+3(a5)	; set arp 4
	rts
effF:
	move.b	d0,s_arpeggio+4(a5)	; set arp 5
	rts
eff10:
	move.b	d0,s_arpeggio+5(a5)	; set arp 6
	rts
eff11:
	move.b	d0,s_arpeggio+6(a5)	; set arp 7
	rts
eff12:
	move.b	d0,s_arpeggio+7(a5)	; set arp 8
	rts
eff13:
	move.b	d0,s_arpeggio(a5)	; set arp 1 / 5
	move.b	d0,s_arpeggio+4(a5)
	rts
eff14:
	move.b	d0,s_arpeggio+1(a5)	; set arp 2 / 6
	move.b	d0,s_arpeggio+5(a5)
	rts
eff15:
	move.b	d0,s_arpeggio+2(a5)	; set arp 3 / 7
	move.b	d0,s_arpeggio+6(a5)
	rts
eff16:
	move.b	d0,s_arpeggio+3(a5)	; set arp 4 / 8
	move.b	d0,s_arpeggio+7(a5)
	rts
eff17:
	cmp.b	#65,d0			; set attack step
	bmi.s	eff17_con
	move.b	#64,d0
eff17_con:
	move.b	d0,s_attack_step(a5)
	rts
eff18:
	move.b	d0,s_attack_delay(a5)	; set attack delay
	rts
eff19:
	cmp.b	#65,d0			; set decay step
	bmi.s	eff19_con
	move.b	#64,d0
eff19_con:
	move.b	d0,s_decay_step(a5)
	rts
eff1A:
	move.b	d0,s_decay_delay(a5)	; set decay delay
	rts
eff1B:
	move.b	d0,s_sustain(a5)	; set sustain byte 1
	rts
eff1C:
	move.b	d0,s_sustain+1(a5)	; set sustain byte 2
	rts
eff1D:
	cmp.b	#65,d0			; set release step
	bmi.s	eff1D_con
	move.b	#64,d0
eff1D_con:
	move.b	d0,s_release_step(a5)
	rts
eff1E:
	move.b	d0,s_release_delay(a5)	; set release delay
	rts


effect_table:
	dc.l	eff0,eff1,eff2,eff3,eff4,eff5,eff6,eff7
	dc.l	eff8,eff9,effA,effB,effC,effD,effE,effF
	dc.l	eff10,eff11,eff12,eff13,eff14,eff15,eff16,eff17
	dc.l	eff18,eff19,eff1A,eff1B,eff1C,eff1D,eff1E,eff0

play_speed:
	dc.b	speed
	even
safe_zero:
	dcb.b	16,0

freq_table:
 dc.w	0000,6848,6464,6096,5760,5424,5120,4832,4560,4304,4064,3840
 dc.w	3616,3424,3232,3048,2880,2712,2560,2416,2280,2152,2032,1920
 dc.w	1808,1712,1616,1524,1440,1356,1280,1208,1140,1076,0960,0904
 dc.w	0856,0808,0762,0720,0678,0640,0604,0570,0538,0508,0480,0452
 dc.w	0428,0404,0381,0360,0339,0320,0302,0285,0269,0254,0240,0226
 dc.w	0214,0202,0190,0180,0170,0160,0151,0143,0135,0127,0120,0113
 dc.w	0113,0113,0113,0113,0113,0113,0113,0113,0113,0113,0113,0113

channel1:
	dc.l	$dff0a0		; hardware pointer
	dc.w	$8001		; DMA value
	dc.l	0		; instrument pointer
	dc.w	0		; frequency
	dc.l	0		; sound table pointer
	dc.b	0		; sound table counter
	dc.b	0		; sound table delay
	dc.l	0		; track pointer
	dc.w	0		; track counter
	dc.l	0		; block pointer
	dc.l	0		; block counter
	dc.b	0		; vibrator wait
	dc.b	0		; vib length
	dc.b	0		; vib position
	dc.b	0		; vib length + & - (compare)
	dc.w	0		; vib add freq
	dc.w	0		; old freq for portamento
	dc.b	0		; freq_dat
	dc.b	0		; actual volume
	dc.b	0		; attack delay
	dc.b	0		; decay delay
	dc.w	0		; sustain
	dc.b	0		; release delay
	dc.b	1		; play speed
	dc.w	0		; bendrate freq
	dc.b	0		; transpose
	dc.b	0		; status
	dc.b	0		; arpeggio counter
	dc.b	0		; arpeggio data
	dc.b	0		; arpeggio on/off
	dc.b	0		; effect number
	dc.b	0		; effect data
	even

channel2:
	dcb.b	channel2-channel1,0
channel3:
	dcb.b	channel2-channel1,0
channel4:
	dcb.b	channel2-channel1,0

track1:
	dc.l	0
track2:
	dc.l	0
track3:
	dc.l	0
track4:
	dc.l	0
blocks:
	dc.l	0

snd_table:
	dcb.l	20,0

;data:
	dcb.b	2,0

DM_data:
