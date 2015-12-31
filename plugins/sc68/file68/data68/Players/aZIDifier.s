;;; aZIDifier - sc68 stub
;;;
;;; Add ZID synthesis to classic music.
;;; 
;;; Init-stamp: <?>
;;; Time-stamp: <2015-09-05 21:45:40 ben>
;;;
;;; by Ben / OVR
;;;
;;; <https://sourceforge.net/users/benjihan>
;;;

NOISE_LATCH   = 5
NOSOUND_LATCH = 4
BUZZ_LATCH    = 4
ARPE_LATCH    = 1
PERIOD_MIN    = 6
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; 
;;; Header:
;;;
;;;  Classic sc68/sndh (...) music driver header.
;;; 
	bra.w	init
	bra.w	exit
	bra.w	play

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; DO NOT CHANGE PLACE AND ORDER
	
music_ptr:
	dc.l	0	; Pointer to current music replay
aSid_activ:
	dc.w	0	; $0000:off $FFFF:on $00FF:enable $FF00:disable

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; 
;;; Init code:
;;; 
;;; - init aSIDfier.
;;; - save MFP.
;;; - run music replay init code.
;;; 
;;; In:
;;; 
;;;  specific to aSIDifier:
;;;	a6: original music replay [+0 init, +4 stop, +8 play]
;;; 	d6: bit#0-#1: replay rate (0:50 1:100 2:200 3:reserved)
;;;	d7: timer selection. Use token like 'abcx' where a,b,c,x is
;;;	    one of 'A','B','C' or 'D' the timer to use for respectively
;;;           canal a,b,c. 'x' is the unused timer and *MUST* be set too.
;;;	    Default is 'ACDB'.
;;;
;;;  specific to sc68:
;;;	a0: music data (for external replay)
;;;	d0: sub-song number
;;;	d1: ste-selection (0:stf, 1:ste)
;;;	d2: music data size (size of buffer pointed by a0)
;;; 
;;; Out:
;;; 	none, all registers restored 
	
init:
	;; save/push registers
	movem.l	d0-a6,-(a7)

	;; Clear timer
	lea	timers(pc),a0
	moveq.l	#timer_sz*4,d0
	bsr	clear

	;; Clear voices info
	lea	ym_status(pc),a0
	moveq.l	#ym_sz,d0
	bsr	clear

	;; Init latch timings (depend on replay rate)
	and	#3,d6
	
	lea	aSIDtimings(pc),a0

	move	#NOISE_LATCH,d0
	lsl	d6,d0
	move.w	d0,(a0)+

	move	#NOSOUND_LATCH,d0
	lsl	d6,d0
	move.w	d0,(a0)+

	move	#BUZZ_LATCH,d0
	lsl	d6,d0
	move.w	d0,(a0)+

	move	#ARPE_LATCH,d0
	lsl	d6,d0
	move.w	d0,(a0)+
	
	;; init timer tables
	bsr	init_timers
	
	;; compute timer table
	bsr	compute_timer_table

	;; compute period to chord table
	bsr	compute_per2chd_table
	
	;; save MFP
	bsr	save_mfp

	;; set nusic pointer
	lea	music_ptr(pc),a5
	move.l	a6,(a5)
	
	;; restore registers
	movem.l	(a7),d0-a5

	;; call music init
	jsr	(a6)

	;; set active to enable
  	lea	aSid_activ(pc),a0
	move.w	#$00ff,(a0)
	
	;;  reset ym status buffer
	bsr	get_ym_status

	;; setup voice info structs
	lea	ym_status(pc),a0
	move.w	d7,ym_m7(a0)
	movem.w	d0-d1,ym_A+ym_per(a0)
	movem.w	d2-d3,ym_B+ym_per(a0)
	movem.w	d4-d5,ym_C+ym_per(a0)
	moveq	#2,d0
.lp_voice:
	move.w	#128,ym_sid(a0)
	st.b	ym_chd+1(a0)	; set an invalid chd
	move.l	ym_tim(a0),a1
	moveq	#10,d1
	sub	d0,d1
	ror.l	#8,d1
	move.l	d1,timer_volH(a1)
	move.l	d1,timer_volL(a1)
	lea	ym_vsz(a0),a0
	dbf	d0,.lp_voice

	;; restore/pop registers
	movem.l	(a7)+,d0-a6
	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; 
;;; Exit code:
;;; 
;;; - run music stop code.
;;; - stop aSIDifier.
;;; - restore MFP.
;;; - cut YM.
;;;
;;; In:  none
;;; Out: none
;;; 

exit:
	movem.l	d0-a6,-(a7)
	
	lea	music_ptr(pc),a6
	move.l	(a6),d0
	beq.s	.exit

	;; clear music pointer and aSid status
	clr.l	(a6)+
	clr.w	(a6)+
	
	;; call music stop
	move.l	d0,a6
	jsr	4(a6)

	;; restore mfp
	bsr	restore_mfp
	
	;; Mute YM (no volume, no pulse and no noise on all chans)
	lea	$ffff8800.w,a0
	lea	$ffff8802.w,a1
	move	sr,-(a7)
	moveq	#$00,d0
	move	#$2700,sr
	move.b	#$08,(a0)
	move.b	d0,(a1)
	move.b	#$09,(a0)
	move.b	d0,(a1)
	move.b	#$0A,(a0)
	move.b	d0,(a1)
	moveq	#$C0,d0
	move.b	#$7,(a0)
	and.b	(a0),d0
	or.b	#$3F,d0
	move.b	d0,(a1)
	move	(a7)+,sr
	
.exit:
	movem.l	(a7)+,d0-a6
	rts
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;  
;;; Play code:
;;;
;;;   The is a bit of code here to handle aSID enable/disable on the
;;;   fly. Specially the ym_restore() function that restores the previous
;;;   YM status prior to run the replay code (some players need that,
;;;   I don't remember which specifically).
;;; 

play:
	;; check music ptr
	move.l	music_ptr(pc),d0
	bne.s	.running
	rts

.running:
	;; What to do depends on aSid_activ value.
	lea	aSid_activ(pc),a1
	move.w	(a1),d7
	bne.s	.not_off

	;; aSid_activ == $0000 -> aSid OFF
.asid_off:	
	move.l	d0,a0
	jmp	8(a0)
	
.not_off:
	cmp.w	#$FFFF,d7
	bne.s	.changed
	
	;; aSid_activ == $FFFF -> aSid ON
.asid_on:
	bsr.s	ym_restore
.no_restore:
	move.l	d0,a0
	jsr	8(a0)
	move	sr,-(a7)
	move	#$2700,sr
	bsr	aSidifier
	move	(a7)+,sr
	rts

.changed:
	tst.b	d7
	beq.s	.disable
	
	;; aSid_activ=$00FF -> enable aSid
.enable:
	move	#$FFFF,(a1)
	bra.s	.no_restore

	;; aSid_activ=$FF00 -> disable aSid
.disable:
	clr.w	(a1)
	bsr.s	disable_timers
	bsr.s	ym_restore
	bra.s	.asid_off

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; Disable timers:
;;;
;;;   Disable timer interruptions on aSidfied voices
;;;

disable_timers:
	movem.l	d0-d2/a0-a2,-(a7)
	move	sr,-(a7)
	move	#$2700,sr
	
	;; clear intena and intmsk for each voice timer
	moveq	#3-1,d2
	lea	ym_status+ym_A(pc),a2
	lea	$fffffa00.w,a1
.lp_voice:
	move.l	ym_tim(a2),a0
	moveq	#$7,d1
	moveq	#0,d0
	move.b	timer_channel(a0),d0
	and.b	d0,d1			; d0 = bit number
	lsr	#3,d0			; d0 is channel [0/2]
	bclr	d1,$07(a1,d0)
	bclr	d1,$13(a1,d0)
	lea	ym_vsz(a2),a2
	dbf	d2,.lp_voice

	move	(a7)+,sr
	movem.l	(a7)+,d0-d2/a0-a2
	rts


;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; YM restore:
;;;
;;;   Restore YM registers modified by aSid (7,8,9,A).
;;;

ym_restore:
	move.l	a1,-(a7)
	move	d7,-(a7)
	move	sr,-(a7)
	
	;; Restore mixer register but port-A and port-B
	move	#$2700,sr
	move	#%11000000,d7
	lea	$ffff8800.w,a1
	move.b	#7,(a1)
	and.b	(a1),d7
	or.w	ym_status+ym_m7(pc),d7
	move.b	d7,2(a1)
	move.w	ym_status+ym_A+ym_vol(pc),d7
  	movep.w	d7,0(a1)
	move.w	ym_status+ym_B+ym_vol(pc),d7
  	movep.w	d7,0(a1)
	move.w	ym_status+ym_C+ym_vol(pc),d7
  	movep.w	d7,0(a1)
	
	move.w	(a7)+,sr
	move	(a7)+,d7
	move.l	(a7)+,a1
	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; Init timer:
;;;
;;;    - Init timers table.
;;;    - Assign timers to voices.
;;; 
;;; In:  d7: timer assignment ('ACDB')
;;;

init_timers:
	movem.l	d0-a6,-(a7)

	;; scan requested timers
	moveq	#0,d6
	moveq	#3,d1
.lp_test:	
	rol.l	#8,d7
	sub.b	#'A',d7
	moveq	#3,d5
	and.b	d7,d5
	bset	d5,d6
	dbf	d1,.lp_test

	cmp.b	#15,d6
	beq.s	.ok

	;; all timers were not set properly, reset to default
	move.l	#$00020301,d7
.ok:
	;; copy timer info struct
	lea	timer_def_table(pc),a0
	lea	timers(pc),a1
	moveq	#3,d1
.lp_copy:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	lea	timer_sz-10(a1),a1
	dbf	d1,.lp_copy

	;; assign timer to voice info
	lea	ym_status+ym_A(pc),a0
	lea	timers(pc),a1
	moveq	#2,d1
.lp_assign:
	rol.l	#8,d7
	moveq	#3,d5
	and.b	d7,d5
	mulu	#timer_sz,d5
	lea	0(a1,d5),a2
	move.l	a2,ym_tim(a0)
	lea	ym_vsz(a0),a0
	dbf	d1,.lp_assign

	movem.l	(a7)+,d0-a6
	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; clear memory block
;;; 
;;; In:	a0.l:	memory to clear (aligned)
;;;	d0.l:	number of bytes (word)
;;;

clear:
	movem.l	d0-d1/a0,-(a7)
	moveq	#3,d1
	and.l	d0,d1
	lsr.l	#2,d0
	beq.s	.nolong
	subq.w	#1,d0
.clr:
	clr.l	(a0)+
	dbf	d0,.clr
.nolong:
	lsr	#1,d1
	bcc.s	.nobyte
	clr.b	(a0)+
	tst.b	d1
.nobyte:	
	beq.s	.noword
	clr.w	(a0)+
.noword:
	movem.l	(a7)+,d0-d1/a0
	rts
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------

	RSRESET
timersv_vector:	rs.w	1
timersv_irq:	rs.l	1
timersv_ctrlr:	rs.w	1
timersv_mask:	rs.b	1
timersv_ctrl:	rs.b	1
timersv_sz:	rs.b	0	
	
	RSRESET
mfp_timer0:	rs.b	timersv_sz
mfp_timer1:	rs.b	timersv_sz
mfp_timer2:	rs.b	timersv_sz
mfp_timer3:	rs.b	timersv_sz
mfp_intena:	rs.w	1
mfp_intmsk:	rs.w	1
mfp_17:		rs.w	1
mfp_size:	rs.b	0	

; MFP save buffer
mfp_buffer:
	ds.b	mfp_size
	even

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; Save MFP interruption.
;;; 

save_mfp:
	movem.l	d0-d1/a0-a3,-(a7)
	move	sr,-(a7)
	move	#$2700,sr
	
	lea	mfp_buffer(pc),a0
	lea	$fffffa00.w,a1
	lea	timers(pc),a2

	moveq	#0,d0
	moveq	#3,d1
.loop:		
	move.w	timer_vector(a2),a3
	move.w	a3,(a0)+
	move.l	(a3),(a0)+
	move.w	timer_ctrlreg(a2),a3
	move.w	a3,(a0)+
	move.b	timer_mask(a2),d0
	move.b	d0,(a0)+
	and.b	(a3),d0
	move.b	d0,(a0)+
	lea	timer_sz(a2),a2
	dbf	d1,.loop
	
	movep.w	$07(a1),d0
	and.w	#$2130,d0
	move.w	d0,(a0)+	; enable bits
	movep.w	$13(a1),d0
	and.w	#$2130,d0
	move.w	d0,(a0)+	; mask bits
	move.b	$17(a1),(a0)+	; SEI

	move	(a7)+,sr
	movem.l	(a7)+,d0-d1/a0-a3
	
	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; Restore MFP interruption.
;;; 

restore_mfp:
	movem.l	d0-d1/a0-a4,-(a7)
	
	move	sr,-(a7)
	move	#$2700,sr
	
	lea	mfp_buffer(pc),a0
	lea	$fffffa00.w,a1

	;; cut intena for timers
	movep.w	$07(a1),d0
	and.w	#~$2130,d0
	movep.w	d0,$07(a1)

	moveq	#0,d0
	moveq	#2,d1		; Only restore 3 used timers !
.loop:
	move.w	(a0)+,a2	; timer vector
	move.l	(a0)+,a3	; saved vector
	move.w	(a0)+,a4	; control reg
	move.b	(a0)+,d0	; saved mask
	not.b	d0		; invert to mask other
	and.b	(a4),d0
	or.b	(a0)+,d0
	move.b	d0,(a4)		; restore control reg
	move.l	a3,(a2)		; restore vector
	dbf	d1,.loop

	lea	timersv_sz(a0),a0 ; skip 4th timer

	;; Restore intena
	movep.w	$07(a1),d0
	and.w	#~$2130,d0
	or.w	(a0)+,d0
	movep.w	d0,$07(a1)
	
	;; Restore intmsk
	movep.w	$13(a1),d0
	and.w	#~$2130,d0
	or.w	(a0)+,d0
	movep.w	d0,$13(a1)

	;; Restore SEI/AEI 
	move.b	(a0)+,$17(a1)

	move	(a7)+,sr
	movem.l	(a7)+,d0-d1/a0-a4

	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; aSIDdifier private data
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------

	RSRESET
	
ym_per:	rs.w	1		; period
ym_vol:	rs.w	1		; volume
ym_an1:	rs.w	1		; angle #1
ym_ad1:	rs.w	1		; increment #1
ym_an2:	rs.w	1		; angle #2
ym_ad2:	rs.w	1		; increment #2
ym_sid:	rs.w	1		; Cyclic ratio
ym_esd:	rs.w	1		; Effective SID
ym_lat:	rs.w	1		; aSID latch
ym_chd:	rs.w	1		; current chord
ym_cct:	rs.w	1		; change chord counter
ym_tim:	rs.l	1		; pointer to timer struct
ym_vsz:	rs.w	0

	RSRESET
ym_A:	rs.b	ym_vsz
ym_B:	rs.b	ym_vsz
ym_C:	rs.b	ym_vsz
ym_m7:	rs.w	1
ym_sz:	rs.w	0

ym_status:
	ds.b	ym_sz


;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; Get YM status (periods,volumes and mixer)
;;; 
;;; Out:
;;; 	d0,d1: voice-A per,vol
;;;	d2,d3: voice-B per,vol
;;;	d4,d5: voice-C per,vol
;;;	d7:    mixer

get_ym_status:
	lea	$ffff8800.w,a0

	;; Get voice A
	moveq	#$0f,d0
	move.b	#$1,(a0)
	and.b	(a0),d0
	
	lsl	#8,d0
	move.b	#$0,(a0)
	or.b	(a0),d0
	
	move	#$081f,d1
	move.b	#$08,(a0)
	and.b	(a0),d1

	;; Get voice B
	moveq	#$0f,d2
	move.b	#$3,(a0)
	and.b	(a0),d2
	
	lsl	#8,d2
	move.b	#$2,(a0)
	or.b	(a0),d2
	
	move	#$091f,d3
	move.b	#$09,(a0)
	and.b	(a0),d3

	;; Get voice C
	moveq	#$0f,d4
	move.b	#$5,(a0)
	and.b	(a0),d4
	
	lsl	#8,d4
	move.b	#$4,(a0)
	or.b	(a0),d4
	
	move	#$0a1f,d5
	move.b	#$0a,(a0)
	and.b	(a0),d5

	;; Get mixer stat
	move.w	#$073f,d7
	move.b	#$7,(a0)
	and.b	(a0),d7
		
	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; aSIDifier (o_O)
;;; 

aSidifier:

	;; Read and store current ym status
	bsr	get_ym_status
	lea	ym_status(pc),a6
	movem.w	d0-d1,ym_A+ym_per(a6)
	movem.w	d2-d3,ym_B+ym_per(a6)
	movem.w	d4-d5,ym_C+ym_per(a6)
	move.w	d7,ym_m7(a6)

;;; mixer rules:
;;; -----------
;;; noise | tone | aSIDdifier
;;; 0       0      0
;;; 0       1      0
;;; 1       0      1
;;; 1       1      0
;;;
;;; aSIDifier = (noise^tone)&noise

	
	moveq	#0,d6
	moveq	#0,d2
	moveq	#0,d0
	lea	ym_A(a6),a5
	lea	per2chd(pc),a4

	;; For each channel
.loop_latch:

	;; Test noise / sound / envelop and set latch

	move	d7,d5
	lsr	d6,d5

	btst	#3,d5
	bne.s	.nonoise
	move	noise_latch(pc),d5
	bra.s	.set_latch
.nonoise:
	btst	#0,d5
	beq.s	.sound
	move	nosound_latch(pc),d5
	bra.s	.set_latch
	
.sound:
 	btst	#4,ym_vol+1(a5)
 	beq.s	.nobuzz
 	move	buzz_latch(pc),d5
 	bra.s	.set_latch
.nobuzz:
	move	ym_lat(a5),d5
	beq.s	.aSIDactiv
	subq.w	#1,d5

	;; Set new latch
.set_latch:
 	move.w	d5,ym_lat(a5)
	beq.s	.aSIDactiv
	
	;; Not activ, reset some stuff
	st.b	ym_chd+1(a5)		; set an invalid chord
	bra.s	.next_latch
	
.aSIDactiv:
	;; Chord change detect

	move	ym_per(a5),d2		; d2: new period
	move.b	0(a4,d2),d0		; d0: new chord
	bsr	get_chord_period	; d1: new period

	sub	d1,d2			; d2: delta period
	subx	d3,d3
	eor	d3,d2
	sub.w	d3,d2			; d2: |delta period|
	addq	#1,d2
	
	move	d0,d3
	lsr	#4,d3			; octave [0-9]
	addq	#2,d3			; 
	lsl	d3,d2

	move	ym_ad2(a5),d3
	add	d3,d2		; a + b
	add	d3,d3		; 2*b
	add	d3,d2		; a + 3*b
	add	d3,d3		; 4*b
	add	d3,d2		; a + 7*b
	lsr	#3,d2
	move	d2,ym_ad2(a5)

	cmp.w	ym_chd(a5),d0
	beq.s	.no_chord_change
	
.chord_change:
	move	d0,d2
	move	d0,ym_chd(a5)		; store new chord
	moveq	#15,d1
	and	d0,d1			; d1: note [0-11]
	lsr.b	#4,d0			; d0: octave [0-9]
	add.b	d0,d1			; [0-20]
	add	d1,d1
	move.w	chord_stp+2(pc,d1),d4	; d4: another random step
	move.w	chord_stp(pc,d1),d1	; d1: new random step
	
	;; move.w	ym_cct(a5),d3
	;; cmp.w	arpe_latch(pc),d3
	;; bpl.s	.arpeggio

	
	add.w	ym_ad1(a5),d4
	lsr	#1,d4
	move.w	d1,ym_ad1(a5)
	move.w	d4,ym_ad2(a5)
	
     	;; clr.w	ym_an1(a5)	; reset cyclic ratio 1 to middle point
	;; clr.w	ym_an2(a5)	; reset cyclic ratio 2 to middle point

.arpeggio:
	clr.w	ym_cct(a5)
	
.no_chord_change:
	addq.w	#1,ym_cct(a5)
	bpl.s	.next_latch
	move	#$7fff,ym_cct(a5)
.next_latch:
	lea	ym_vsz(a5),a5
	addq	#1,d6
	cmp.w	#3,d6
	bne	.loop_latch

	;; reset aSIDfied voice
	moveq	#0,d7

	;; voice A
	moveq	#0,d6
	lea	ym_status+ym_A(pc),a6
	bsr	aSIDvoicify

	;; voice B
	moveq	#1,d6
	lea	ym_status+ym_B(pc),a6
	bsr	aSIDvoicify

	;; voice C
	moveq	#2,d6
	lea	ym_status+ym_C(pc),a6
	bsr	aSIDvoicify

	;; Cut aSIDdified voice in mixer, keep port-A and port-B bits
	move	sr,-(a7)
	move	#$2700,sr
	
	lea	$fffff8800.w,a6
	move.b	#7,(a6)
	moveq	#%11000000,d6
	and.b	(a6),d6
  	and	#%111,d7
  	or	ym_status+ym_m7(pc),d7
 	or.b	d7,d6
     	move.b	d6,2(a6)
	
	move	(a7)+,sr
	
	rts

CHORD_STP_MIN	=	$053
CHORD_STP_MSK	=	$1FF

chord_step:	MACRO
	{
	dc.w	((\1&CHORD_STP_MSK)+CHORD_STP_MIN);; *(1-(\2&2))
	;; print	((\1&CHORD_STP_MSK)+CHORD_STP_MIN);; *(1-(\2&2))
	}
	
RND_1	=	0
RND_2	=	$17299283
	
chord_stp:
	RPT	24
	{
RND_1	=	RND_1+RND_2
RND_2	=	(RND_2<<3)+(RND_2>>5)^RND_1
	chord_step	RND_1,RND_1
	}


;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; 
;;; aSIDify one voice.
;;; 
;;; In:
;;;	d6: num voice
;;;	a6: voice struct
;;;	d7: current SIDfied flags
;;; 
;;; Out:
;;;	d7: new SIDfied flags
;;; 
aSIDvoicify:
	
	;; Get timer table
	move.l	ym_tim(a6),a2
	lea	timer_irq_base(pc),a3
	add	timer_irq(a2),a3
	
	tst.w	ym_lat(a6)
	bne	.noA

	;; period/volume
	movem.w	ym_per(a6),d0-d1

	;; 
	and.w	#$f,d1
	;; beq	.noA		;; Don't aSIDify 0 volume
				;; Don't aSIDdify high pitched note
	cmp.w	#PERIOD_MIN,d0
	ble	.noA

	;; Compute SID value
	lea	sinus(pc),a5

	move	ym_an2(a6),d3
	add	ym_ad2(a6),d3
	move	d3,ym_an2(a6)	; d3: new ang 2

	lsr	#16-SINFIX,d3
	moveq	#0,d2
	move.b	0(a5,d3.w),d2	; sin(an2) (16-240)

	;; << TEMP
	;; move	d2,d5
	;; bra.s	.smooth_d5
	;; reset
	;; >> TEMP
	
	add	ym_ad1(a6),d2
	lsr	#1,d2
	add	ym_an1(a6),d2
	move	d2,ym_an1(a6)	; d2: new ang 1

	lsr	#16-SINFIX,d2
	moveq	#0,d5
	move.b	0(a5,d2.w),d5	; d5:  sid = sin(an1) (16-240)


	;; Modulation
	;; mulu	d4,d5
	;; lsr	#8,d5
	;; add	#15,d5		; d5 = v1 * v2

.smooth_d5:	
	;; Smooth SID value
	move	ym_sid(a6),d4	; x1
	move	d4,d3
	add	d3,d3		; x2
	add	d3,d4		; x3
	add	d3,d3		; x4
	add	d3,d4		; x7
	add	d4,d5		; x8
	lsr	#3,d5

.ok_d5:
	move	d5,ym_sid(a6)	; new sid !
	tst.b	d5
	smi	d3
	eor.b	d3,d5		; $80-$7f ... $ff -> $00
	move	d5,ym_esd(a6)	; new effective sid !


	;; timer control/data
	move	d0,d3
	add	d3,d3
	lea	Tper2MFP(pc),a5
	move.w	0(a5,d3),d2	; ctrl+data
	beq	.noA		; sorry, not available for this period 

	;; control reg
	move	d2,d3
	lsr	#8,d3
	and.b	timer_mask(a2),d3	; d3: control reg

	;; data regs (d2:hi / d4:lo)
	and.w	#255,d2
	subq.b	#1,d2		; 1->0, 2->1, 0->255
	addq.w	#1,d2		; [V] [1..256]
	move	d2,d4		; [N]
	
	mulu	ym_sid(a6),d2
	;; mulu	ym_esd(a6),d2
	lsr.w	#8,d2		; [V]
	seq	d5
	sub.b	d5,d2		; if 0 -> 1

;; .not_empty:
 	sub	d2,d4
	seq	d5
	sub.b	d5,d4		; if 0 -> 1
	
	
	;; Set volumes
 	move.b	d1,timer_volH+2(a2)	; set volume HI

	
	;; Try to compensated for power lost by adjusting the SID low
	;; level.

	;; move	ym_sid(a6),d5		; 0-255
	;; sub.b	#128,d5			; -128-127
	;; smi	d0
	;; eor.b	d0,d5			; 0-127
	;; and	#15,d1
	;; mulu	d1,d5			; 0-1905
	;; lsr	#4+4,d5			; 


	;; moveq	#15,d5
	;; sub	d1,d5	      		; 15-vol
	;; lsl	#4,d5			; $f0..$00
	;; sub	ym_esd(a6),d5		; 0-127
	;; asr	#4,d5
	;; spl	d0
	;; and	d0,d5
	
	move	ym_esd(a6),d5		; 0-127
	mulu	d1,d5			; 0-1905
	lsr	#4+4,d5			; 0-7

	;; move	ym_esd(a6),d5		; 0-127
	;; lsr	#4,d5			; 0-7

	;; move	ym_esd(a6),d5		; 0-127
	;; lsr	#3,d5			; 0-15
	;; add	d1,d5			; 0-30
	;; lsr	#2,d5			; 0-7

	move.b	d5,timer_volL+2(a2)	; set volume LO
 	;; clr.b	timer_volL+2(a2)	; set volume LO

	;; Set data reg in routines
	move.b	d2,timer_dataH(a2)
	move.b	d4,timer_dataL(a2)

	;; Start timer
	move.w	timer_ctrlreg(a2),a5	; control reg addrx
	move.b	(a5),d4			; d4: ctrl all
	move.b	d4,d5

	moveq	#0,d0
	move.b	timer_mask(a2),d0	; d0: mask other
	and.w	d0,d4			; d4: me
	not.b	d0			; d0: mask me
	and.w	d0,d5			; d5: other
	
	cmp.b	d4,d3
	beq.s	.no_progA

	move.b	d5,(a5)			; stop
	move.w	timer_vector(a2),a1	; vector addr
	move.l	a3,(a1)			; Set vector
	move.w	timer_datareg(a2),a1	; data reg addr
 	move.b	d2,(a1)			; start with data hi
	or.b	d3,d5	
	move.b	d5,(a5)			; GO timer ! GO !
	bra.s	.ok_progA
	
.no_progA:
	;; Not programmed, we should take care to retrieve good level

	moveq	#8,d2
	add.b	d6,d2
	swap	d2		; $..VR....
	move.b	timer_volL+2(a2),d2
	move.w	(a2),a1		; vector addr
	move.l	(a1),d5		; current routine
	sub.l	a3,d5		; 0 -> next routine is HI, current is LO
	beq.s	.oklow
	move.b	timer_volH+2(a2),d2
.oklow:	
	lsl.l	#8,d2
	move.l	d2,$ffff8800.w
	
.ok_progA:		
	;; set intena and intmsk
	moveq	#$7,d0
	moveq	#0,d1
	move.b	timer_channel(a2),d1
	and.b	d1,d0			; d0 = bit number
	lsr	#3,d1			; d1 is channel [0/2]
	lea	$fffffa00.w,a1
	bset	d0,$07(a1,d1)
	bset	d0,$13(a1,d1)
	bset	#3,$17(a1)		; set MFP to AEI
	bset	d6,d7
	
	rts
	
.noA:
	bclr	d6,d7
	move.w	timer_ctrlreg(a2),a5	; control reg addr
	move.b	timer_mask(a2),d0	; d0: mask other
	not.b	d0			; d0: mask me
	and.b	d0,(a5)			; stop timer
	rts


; timer info struct (aligned to 32 bytes)
	RSRESET
timer_vector:	rs.w	1	; vector address
timer_ctrlreg:	rs.w	1	; timer control register
timer_datareg:	rs.w	1	; timer data register
timer_mask:	rs.b	1	; bit used in ctrl reg
timer_channel:	rs.b	1	; MFP channel and bit
timer_irq:	rs.w	1	; offset from timer_irq_base
timer_volH:	rs.l	1	; Value to set YM HI
timer_volL:	rs.l	1	; Value to set YM LO
timer_dataH:	rs.b	1	; data register value for HI
timer_dataL:	rs.b	1	; data register value for LO
timer_sz:	rs.w	0
	
; vector, ctrl-reg, data-reg, msk.b+chan.q+bit.q
timer_def_table:
tAdef:	dc.w	$134, $fa19, $fa1f, $0f05, timerA_irq-timer_irq_base
tBdef:	dc.w	$120, $fa1b, $fa21, $0f00, timerB_irq-timer_irq_base
tCdef:	dc.w	$114, $fa1d, $fa23, $f015, timerC_irq-timer_irq_base
tDdef:	dc.w	$110, $fa1d, $fa25, $0f14, timerD_irq-timer_irq_base

timers:
timerA:	ds.b	timer_sz
timerB:	ds.b	timer_sz
timerC:	ds.b	timer_sz
timerD:	ds.b	timer_sz
	
;;; ------------------------------------------------------------
;;; Timer interruption routines
;;; - name (A/B/C) referes to the sound channel not the timer
;;; ------------------------------------------------------------


	
;;; \1:	'A','B','C','D'
;;; \2:	timer vector
;;; \3:	timer data reg
timerN:	MACRO
	{
timer\1_irq:
timer\1_irq_H:
	move.l	timer\1+timer_volH(pc),$ffff8800.w
	pea	timer\1_irq_L(pc)
	move.l	(a7)+,\2
 	move.b	timer\1+timer_dataL(pc),\3
	rte
	
timer\1_irq_L:
	move.l	timer\1+timer_volL(pc),$ffff8800.w
	pea	timer\1_irq_H(pc)
	move.l	(a7)+,\2
 	move.b	timer\1+timer_dataH(pc),\3
	rte
	}

	dc.w	$1234
timer_irq_base:	
	timerN	A,$134.w,$fffffa1f.w
	timerN	B,$120.w,$fffffa21.w
	timerN	C,$114.w,$fffffa23.w
	timerN	D,$110.w,$fffffa25.w
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------

;;; per = 125000/frq
;;; frq = 125000/per

;;; timer_fmin = 2457600/(200*256)	; 48 Hz
;;; timer_fmax = 2457600/(4*1)		; 614400 Hz

;;; frq = 125000/per
;;; frq = 2457600/width
;;; 125000 / p = 2457600 / w
;;; w * 125000 / p = 2457600
;;; w = 2457600 * p / 125000
;;; w = 12288 * p / 625	
;;; w = d * r
;;; d * r = 12288 * p / 625
;;; d = 12288 * p / (625*r)

Tper2MFP:
	ds.w	$1000
	
;;; Timer prediviser table
timer_prediv:
	dc.w	0*625, 4/2*625, 10/2*625, 16/2*625
	dc.w	50/2*625, 64/2*625, 100/2*625, 200/2*625
	
compute_timer_table:
	movem.l	d0-d3/a0-a1,-(a7)
	
	lea	Tper2MFP(pc),a0
	lea	timer_prediv+2(pc),a1
	move.w	#$1100,d1
	moveq	#0,d3
	moveq	#1,d0		; skip period 0 :)
 	clr	(a0)+
.lp:
	mulu	#12288/2,d0
.retry:		
	move.l	d0,d2
	divu	(a1),d2
	cmp.w	#256,d2
	ble.s	.ok
.advance:
	addq	#2,a1
	add.w	#$1100,d1
	and.w	#$7700,d1
	bne.s	.retry
	
.clear:
	clr.w	(a0)+
	addq	#1,d3
	cmp.w	#$1000,d3
	bne.s	.clear
	bra.s	.finish

.ok:	
	move.b	d2,d1
	move.w	d1,(a0)+
	addq	#1,d3
	move.l	d3,d0
	
	cmp.w	#$1000,d3
	bne.s	.lp
	
.finish:
	movem.l	(a7)+,d0-d3/a0-a1
	rts

;;; In:
;;;	d0: chords.q octave.q
;;; Out:
;;;	d1: YM period
get_chord_period:
	moveq	#15,d1
	and	d0,d1
	add	d1,d1
	move.w	chords(pc,d1.w),d1
	ror	#4,d0
	lsr	d0,d1
	rol	#4,d0
	rts

;;; Various timings to trigger aSID fx depends on replay rate.
;;; !!! DO NOT ChANGE ORDER !!! (or edit the init part accordingly)
aSIDtimings:
noise_latch:	dc.w	NOISE_LATCH
nosound_latch:	dc.w	NOSOUND_LATCH
buzz_latch:	dc.w	BUZZ_LATCH
arpe_latch:	dc.w	ARPE_LATCH
	
	
;;; Chords table:
;;; - One octave at lowest frequency available for the YM tone generator
chords:	
	dc.w	$EEE,$E17,$D4D,$C8E,$BD9,$B2F,$A8E,$9F7,$967,$8E0,$861,$7E8
	dc.w	$EEE/2	

;;; Sinus table [512 entries from [16..240]]
SINFIX:	= 9
sinus:	dc.b 128, 129, 131, 132, 133, 135, 136, 138, 139, 140, 142
	dc.b 143, 144, 146, 147, 149, 150, 151, 153, 154, 155, 157, 158
	dc.b 159, 161, 162, 163, 164, 166, 167, 168, 170, 171, 172, 173
	dc.b 175, 176, 177, 178, 180, 181, 182, 183, 184, 186, 187, 188
	dc.b 189, 190, 191, 192, 194, 195, 196, 197, 198, 199, 200, 201
	dc.b 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213
	dc.b 214, 215, 215, 216, 217, 218, 219, 220, 220, 221, 222, 223
	dc.b 223, 224, 225, 225, 226, 227, 227, 228, 229, 229, 230, 230
	dc.b 231, 231, 232, 232, 233, 233, 234, 234, 235, 235, 236, 236
	dc.b 236, 237, 237, 237, 238, 238, 238, 238, 239, 239, 239, 239
	dc.b 239, 239, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240
	dc.b 240, 240, 240, 240, 240, 239, 239, 239, 239, 239, 239, 238
	dc.b 238, 238, 238, 237, 237, 237, 236, 236, 236, 235, 235, 234
	dc.b 234, 233, 233, 232, 232, 231, 231, 230, 230, 229, 229, 228
	dc.b 227, 227, 226, 225, 225, 224, 223, 223, 222, 221, 220, 220
	dc.b 219, 218, 217, 216, 215, 215, 214, 213, 212, 211, 210, 209
	dc.b 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197
	dc.b 196, 195, 194, 192, 191, 190, 189, 188, 187, 186, 184, 183
	dc.b 182, 181, 180, 178, 177, 176, 175, 173, 172, 171, 170, 168
	dc.b 167, 166, 164, 163, 162, 161, 159, 158, 157, 155, 154, 153
	dc.b 151, 150, 149, 147, 146, 144, 143, 142, 140, 139, 138, 136
	dc.b 135, 133, 132, 131, 129, 128, 127, 125, 124, 123, 121, 120
	dc.b 118, 117, 116, 114, 113, 112, 110, 109, 107, 106, 105, 103
	dc.b 102, 101, 99, 98, 97, 95, 94, 93, 92, 90, 89, 88, 86, 85, 84
	dc.b 83, 81, 80, 79, 78, 76, 75, 74, 73, 72, 70, 69, 68, 67, 66
	dc.b 65, 64, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50
	dc.b 49, 48, 47, 46, 45, 44, 43, 42, 41, 41, 40, 39, 38, 37, 36
	dc.b 36, 35, 34, 33, 33, 32, 31, 31, 30, 29, 29, 28, 27, 27, 26
	dc.b 26, 25, 25, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 20, 19
	dc.b 19, 19, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17, 16, 16, 16
	dc.b 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17
	dc.b 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21
	dc.b 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 29, 29
	dc.b 30, 31, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38, 39, 40, 41
	dc.b 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55
	dc.b 56, 57, 58, 59, 60, 61, 62, 64, 65, 66, 67, 68, 69, 70, 72
	dc.b 73, 74, 75, 76, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89, 90
	dc.b 92, 93, 94, 95, 97, 98, 99, 101, 102, 103, 105, 106, 107, 109
	dc.b 110, 112, 113, 114, 116, 117, 118, 120, 121, 123, 124, 125
	dc.b 127


;;; Compute per2chd table (see per2chd for more info).
;;; 
compute_per2chd_table:
	movem.l	d0-a1,-(a7)

	lea	chords(pc),a0
	lea	per2chd+$1000(pc),a1

	moveq	#0,d1		; d1: octave
	moveq	#0,d2		; d2: note in octave
	move.w	(a0)+,d3
	move	(a0)+,d4
	add	d4,d3
	lsr	#1,d3		; d3: limit
	move	#$fff,d7	; d7: look up period
.loop:
	cmp.w	d3,d7
	bge.s	.ok
	
	addq	#1,d2
	cmp	#12,d2
	bne.s	.same_octave
	
	lea	chords+2(pc),a0
	moveq	#0,d2
	addq	#1,d1

.same_octave:
	move.w	(a0)+,d3
	lsr	d1,d3
	exg	d4,d3
	add	d4,d3
	lsr	#1,d3
.ok:		
	move	d1,d0
	lsl.b	#4,d0
	or.b	d2,d0
	move.b	d0,-(a1)
	dbf	d7,.loop
		
	movem.l	(a7)+,d0-a1
	rts

;;; Table to convert YM periods to the nearest chord.
;;; Each value is $XY
;;; where:
;;; - X is octave in the range [$0..$9].
;;; - Y is chord in the range [$0..$b]
;;;
;;; note:
;;; 	YM periods = chords[Y]>>X

per2chd:
	ds.b	$1000
