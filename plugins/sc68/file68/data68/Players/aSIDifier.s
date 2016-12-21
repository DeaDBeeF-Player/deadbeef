;;; aSIDifier - sc68 stub
;;;
;;; Add SID synthesis to classic music.
;;; 
;;; Init-stamp: <2015-09-03 16:47:02 ben>
;;; Time-stamp: <2015-09-05 21:45:09 ben>
;;;
;;; by Ben / OVR
;;;
;;; <https://sourceforge.net/users/benjihan>
;;;

;;; ----------------------------------------------------------------------
;;;
;;; Note on registers usage (most of the time):
;;;
;;; a6= globales
;;; a5= voice struct
;;; a4= timer interface (associate to the voice)
;;; a3= YM hardware (8800.w)
;;;
;;; ----------------------------------------------------------------------

;;; TODO: an interface for player aSID aware
	
;;; Default latch (disable aSID for that many play 1/50 second)
NOISE_LATCH:	SET 5
NOSOUND_LATCH:	SET 4
BUZZ_LATCH:	SET 4
PERIOD_MIN:	SET 6

PER_PERVAL0:	SET ($7E+$77)>>1
PER_PERVAL2:	SET ($3F4+$3BB)>>1
PER_PERVAL1:	SET (PER_PERVAL0+PER_PERVAL2)>>1

SID_PERFIX0:	SET 0
SID_PERFIX1:	SET 0
SID_PERFIX2:	SET 2
SID_PERFIX3:	SET 1

;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; 
;;; Header:
;;;
;;;  Classic sc68/sndh (...) music driver header.
;;; 
	bra.w	init
	bra.w	exit
	bra.w	play

;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; DO NOT CHANGE PLACE AND ORDER (sc68 requirement)
	
music_ptr:
	dc.l	0	; Pointer to current music replay
aSid_activ:
	dc.w	0	; $0000:off $FFFF:on $00FF:enable $FF00:disable

;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; 
;;; Init code:
;;; 
;;; - init aSIDfier.
;;; - save MFP.
;;; - run music replay init code.
;;; 
;;; Inp:
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

	lea	music_ptr(pc),a0
	move.l	a6,(a0)
	
	;; Globales
	lea	globales(pc),a6
		
	;; Clear variables
	move.l	a6,a0
	moveq.l	#g_size,d0
	bsr	clear
	
	;; Setup default parameters for asidifier
	and	#3,d6

	move	#NOISE_LATCH,d3
	lsl	d6,d3
	move.w	d3,g_noilat(a6)
	
	move	#NOSOUND_LATCH,d3
	lsl	d6,d3
	move.w	d3,g_tonlat(a6)
	
	move	#BUZZ_LATCH,d3
	lsl	d6,d3
	move.w	d3,g_buzlat(a6)

	move.w	#PERIOD_MIN,g_permin(a6)
	move.w	#SID_PERFIX0,g_perfix0(a6)
	move.w	#PER_PERVAL0,g_perval0(a6)
	move.w	#SID_PERFIX1,g_perfix1(a6)
	move.w	#PER_PERVAL1,g_perval1(a6)
	move.w	#SID_PERFIX2,g_perfix2(a6)
	move.w	#PER_PERVAL2,g_perval2(a6)
	move.w	#SID_PERFIX3,g_perfix3(a6)

	;; init the timer interface
	bsr	timers_init

	;; call music init
	movem.l	(a7),d0-a6
	jsr	(a6)
	movem.l	(a7),d0-a6

	lea	globales(pc),a6
	lea	$ffff8800.w,a3
	
	;; set active to enable
	move.w	#$00ff,aSid_activ-globales(a6)

	;; setup the timers
	bsr.s	voices_setup

	;; restore/pop registers
	movem.l	(a7)+,d0-a6
	rts

;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;;
;;; Set voices and their timers.
;;;
;;; Inp: a6= globales
;;;      d7= 'ABCD' timer allocation string
;;; 
	
voices_setup:
	
	moveq	#0,d2
	lea	g_ymA(a6),a5
.loop:	
	moveq	#'A',d1
	rol.l	#8,d7
	sub.b	d7,d1
	neg.b	d1
	add.b	d1,d1
	lea	timerS(pc,d1),a4
	adda.w	(a4),a4		; a4= timer interface
	move	d2,d0
	addq	#8,d0		; d0= volume
	
	bsr.s	voice_setup

	lea	ym_size(a5),a5	; a5= next voice
	addq	#1,d2
	cmp	#3,d2
	bne.s	.loop

	move.b	$fffffa17.w,g_saveVR(a6)
	bset	#3,$fffffa17.w	; AEI
	
	rts

timerS:
	dc.w	timerA-*,timerB-*,timerC-*,timerD-*
	
	;; a5= voice, a4= timer, d0= volume-reg
voice_setup:

	;; copy global parameter
	move.w	g_noilat(a6),ym_noilat(a5)
	move.w	g_tonlat(a6),ym_tonlat(a5)
	move.w	g_buzlat(a6),ym_buzlat(a5)
	move.w	g_permin(a6),ym_permin(a5)
	move.w	g_perfix0(a6),ym_perfix0(a5)
	move.w	g_perfix1(a6),ym_perfix1(a5)
	move.w	g_perfix2(a6),ym_perfix2(a5)
	move.w	g_perfix3(a6),ym_perfix3(a5)
	move.w	g_perval0(a6),ym_perval0(a5)
	move.w	g_perval1(a6),ym_perval1(a5)
	move.w	g_perval2(a6),ym_perval2(a5)
	
	move.l	a4,ym_timer(a5)

	move.b	d0,tm_reg(a4)
	jsr	tm_stop(a4)
	moveq	#0,d0
	jsr	tm_enable(a4)
	movea.w	tm_vect(a4),a2
	move.l	tm_fct_h(a4),(a2)
	moveq	#3,d0
	jsr	tm_enable(a4)
	
	rts
	
;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; 
;;; Exit code:
;;; 
;;; - run music stop code.
;;; - stop aSIDifier.
;;; - restore MFP.
;;; - cut YM.
;;;
;;; Inp: none
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
	bsr	timers_restore
	
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
	moveq	#-64,d0		;$C0
	move.b	#$7,(a0)
	and.b	(a0),d0
	or.b	#$3F,d0
	move.b	d0,(a1)
	move	(a7)+,sr
	
.exit:
	movem.l	(a7)+,d0-a6
	rts
	
;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;;  
;;; Play code:
;;;
;;;   The is a bit of code here to handle aSID enable/disable on the
;;;   fly. Specially the ym_restore() function that restores the previous
;;;   YM status prior to run the replay code (some players need that,
;;;   I don't remember which specifically).
;;; 
;;;   In the 2015 version the ym_restore() as been removed. I reckon
;;;   it was a problem with the mizer register only. we'll see !
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
	move.l	d0,a0
	jmp	8(a0)
	
.not_off:
	cmp.w	#$FFFF,d7
	bne.s	.changed
	
	;; aSid_activ == $FFFF -> aSid ON
.asid_on:
	bsr	ym_restore

.no_restore:

;;; There's the dirty tricks : mask the timer interrupts during the
;;; player pass. This will prevent the SID interrupt but will at the
;;; same time probably have a negative impact on the sound.
;;;
;;; This is not a problem with sc68 as the player pass is virtually
;;; timeless.
;;; 
;;; A clean solution to that problem could be to have special
;;; interrupt routines during the pass that reccord that value the
;;; player wrote in the volume registers.

	move.l	d0,a0

	lea	globales(pc),a6

	;; Mask timer interrupts
	move.l	g_ymA+ym_timer(a6),a4
	moveq	#1,d0
	jsr	tm_enable(a4)

	move.l	g_ymB+ym_timer(a6),a4
	moveq	#1,d0
	jsr	tm_enable(a4)

	move.l	g_ymC+ym_timer(a6),a4
	moveq	#1,d0
	jsr	tm_enable(a4)
	
	jsr	8(a0)

	;; Read the YM registers of interest
	lea	globales(pc),a6
	lea	$ffff8800.w,a3
	bsr	ym_save

	;; Unmask timer interrupts
	move.l	g_ymA+ym_timer(a6),a4
	moveq	#3,d0
	jsr	tm_enable(a4)

	move.l	g_ymB+ym_timer(a6),a4
	moveq	#3,d0
	jsr	tm_enable(a4)

	move.l	g_ymC+ym_timer(a6),a4
	moveq	#3,d0
	jsr	tm_enable(a4)
	
	bra	aSidifier

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
	lea	globales(pc),a6
	lea	$ffff8800.w,a3
	bsr	timers_disable
	bsr	ym_restore
	move.l	music_ptr(pc),a0
	jmp	8(a0)

;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;;
;;; Disable timers:
;;;
;;;   Disable timer interruptions on aSidfied voices
;;;

timers_disable:
	move.l	g_ymA+ym_timer(a6),a4
	moveq	#0,d0
	jsr	tm_enable(a4)

	move.l	g_ymB+ym_timer(a6),a4
	moveq	#0,d0
	jsr	tm_enable(a4)

	move.l	g_ymC+ym_timer(a6),a4
	moveq	#0,d0
	jmp	tm_enable(a4)


;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;;
;;; clear memory block
;;; 
;;; Inp: a0.l:	memory to clear (aligned)
;;;	 d0.l:	number of bytes (word)
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

;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;;
;;; Restore MFP timers (and system 200hz timer-C if neccessary).
;;;
;;; Inp: a6= globales
;;; 
timers_restore:
	move	sr,-(a7)
	move	#$2700,sr
	
	lea	g_ymA(a6),a5
	moveq	#3-1,d7
.lpvoice:	
	move.l	ym_timer(a5),d6
	beq.s	.notimer
	move.l	d6,a4
	jsr	tm_stop(a4)
	moveq	#0,d0
	jsr	tm_enable(a4)
	bsr	timer_restore
.notimer:
	lea	ym_size(a5),a5
	dbf	d7,.lpvoice

	move.b	g_saveVR(a6),$fffffa17.w
	
	move	(a7)+,sr
	rts
	
timer_restore:
	movea.w	tm_vect(a4),a0
	move.l	tm_save_vect(a4),(a0)
	cmpa.w	#$114,a0
	bne.s	.not_C

	;; restore system 200 hz timer
	move.b	#$c0,$fffffa23.w ; $c0 for 200hz timer
	move.b  #$50,$fffffa1d.w ; start TC with  prediv 64

.not_C:

	;; restore intena & intmsk
	moveq	#1,d0
	and.b	tm_save_iena(a4),d0
	
	moveq	#2,d1
	and.b	tm_save_imsk(a4),d1

	or	d1,d0
	jmp	tm_enable(a4)
	
	
;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; aSIDifier private data
;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------

	;; ---------------------------------------------------------------
	;; ---------------------------------------------------------------
	;; Timers interface
	rsreset

tm_stop:	rs.w	1	; bra.s
tm_start:	rs.w	1	; bra.s
tm_cont:	rs.w	1	; bra.s
tm_enable:	rs.w	1	; bra.s
tm_reg:		rs.w	1	; Ym-reg.b + filler.b 
tm_vol:		rs.w	1	; Ym-vol.b + filler.b
tm_fct_l:	rs.l	1	; address.l of sid lo-pulse routine 
tm_fct_h:	rs.l	1	; address.l of sid hi-pulse routine
tm_vect:	rs.w	1	; address.w of interrupt vector
tm_mfp_chn:	rs.b	1	; 0 or 2
tm_mfp_bit:	rs.b	1	; bit number in channel
tm_save_vect:	rs.l	1	; vector.l
tm_save_iena:	rs.b	1	; save intena
tm_save_imsk:	rs.b	1	; save intmsk
tm_size:	rs.b	0


	;; ---------------------------------------------------------------
	;; ym_voice_t
	rsreset

ym_timer:	rs.l	1	; timer interface
ym_period:	rs.w	0	; period [0-fff]
ym_per_hi:	rs.b	1	;
ym_per_lo:	rs.b	1	;
ym_volume:	rs.b	1	; volume [0-1f]
ym_mixer:	rs.b	1	; mixer bits {0,1,8,9}
ym_latch:	rs.w	1	; aSID latch

ym_permin:	rs.w	1	; minimum acceptable period

ym_perfix0:	rs.w	1
ym_perfix1:	rs.w	1
ym_perfix2:	rs.w	1
ym_perfix3:	rs.w	1
ym_perval0:	rs.w	1
ym_perval1:	rs.w	1
ym_perval2:	rs.w	1

ym_noilat:	rs.w	1
ym_tonlat:	rs.w	1
ym_buzlat:	rs.w	1
	
ym_size:	rs.w	0

	;; ---------------------------------------------------------------
	;; globales
	rsreset
g_ymA:		rs.b	ym_size
g_ymB:		rs.b	ym_size
g_ymC:		rs.b	ym_size

g_permin:	rs.w	1
g_perfix0:	rs.w	1
g_perfix1:	rs.w	1
g_perfix2:	rs.w	1
g_perfix3:	rs.w	1
g_perval0:	rs.w	1
g_perval1:	rs.w	1
g_perval2:	rs.w	1
	
g_noilat:	rs.w	1
g_tonlat:	rs.w	1
g_buzlat:	rs.w	1

g_saveVR:	rs.b	1

g_size:		rs.b	0
	
globales:
	ds.b	g_size
	even


;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;;
;;; Save YM status (periods,volumes and mixer)
;;;
;;; Inp: a6= globales
;;;      a3= $ffff8800.w
;;; Out: ym_status fill
;;; Use: d0-d1/a0
;;; 
ym_save:
	move.w	sr,-(a7)
	
	moveq	#$3f,d1
	
	;; Get mixer
	move.w	#$2700,sr	; DnD
	move.b	#$7,(a3)
	and.b	(a3),d1		; d1= mixer bits
	move	(a7),sr		; restore sr for a short while
	
	;; Get voice A
.loop:

V:	SET	0
	REPT	3
B:	SET	V*ym_size+g_ymA

;;; ----------------------------------------------------------------------

	;; period
	moveq	#$0f,d0
	move	#$2700,sr	; DnD
	move.b	#V*2+1,(a3)	; period Hi
	and.b	(a3),d0
	move.b	d0,ym_per_hi+B(a6)
	move.b	#V*2+0,(a3)	; period Lo
	move.b	(a3),ym_per_lo+B(a6)
	
	;; volume
	moveq	#$1f,d0
	move.b	#V+8,(a3)
	and.b	(a3),d0
	move.b	d0,ym_volume+B(a6)
	move	(a7),sr		; Unmask IRQs For a short while
	
	;; mixer
	moveq	#9,d0
	and	d1,d0
	move.b	d0,ym_mixer+B(a6)
	lsr	#1,d1

;;; ----------------------------------------------------------------------
V:	SET	V+1
	ENDR

	addq	#2,a7		; sr already restored, just skip
		
	rts


;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;;
;;; YM restore:
;;;
;;;   Restore YM registers modified by aSid (igmored for now).
;;;
;;; Inp: a6: globales

ym_restore:
	rts
	
;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; aSIDifier (o_O)
;;; 

aSidifier:
	moveq	#0,d7
	lea	g_ymA(a6),a5
	bsr	aSIDvoicify

	moveq	#1,d7
	lea	g_ymB(a6),a5
	bsr	aSIDvoicify
	
	moveq	#2,d7
	lea	g_ymC(a6),a5
	;; bsr	aSIDvoicify
	;; rts			; $$$ OPT RTS

;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; 
;;; aSIDify one voice.
;;; 
;;; Inp:
;;;	d7: num voice
;;;	a6: globales
;;;	a5: ym_voice_t
;;; 
aSIDvoicify:
	;; ---------------------------------------------------------------
	;; The latch

	move.w	ym_latch(a5),d0	; d0= latch
	beq.s	.ok_latch
	bpl.s	.enable
	rts

.enable:
	subq	#1,d0

.ok_latch:
	move.l	ym_timer(a5),a4 ; a4= timer interface
	move.b	ym_volume(a5),d6
	move	ym_period(a5),d5
	
	;; Buzz
	btst	#4,d6
	beq.s	.ok_buzz
	move.w	ym_buzlat(a5),d0
	bra.s	.set_latch
.ok_buzz:
	move.b	d6,tm_vol(a4)
	
	;; mixer
	move.b	ym_mixer(a5),d1
	cmp.b	#$8,d1
	beq.s	.set_latch

	move.w	ym_noilat(a5),d0
	btst	#3,d1		; noise ?
	beq.s	.set_latch
	move.w	ym_tonlat(a5),d0

.set_latch:
	move.w	d0,ym_latch(a5)
	bne	nosid
	
	;; d5= period, d2=fix
	cmp.w	ym_permin(a5),d5
	bls	nosid

	move.w	ym_perfix0(a5),d2
	cmp.w	ym_perval0(a5),d5
	bls.s	.ok_fix

	move.w	ym_perfix1(a5),d2
	cmp.w	ym_perval1(a5),d5
	bls.s	.ok_fix
	
	move.w	ym_perfix2(a5),d2
	cmp.w	ym_perval2(a5),d5
	bls.s	.ok_fix
	
	move.w	ym_perfix3(a5),d2
.ok_fix:

	move	d5,d0
	
	tst.w	d2
	bpl.s	.fixpos
	neg.w	d2
	lsl	d2,d0
.fixpos:	
	lsr	d2,d5

	;; dephase timer
	;; move	d0,d3
	;; lsr	#8,d3
	;; sub	d3,d0

	;; d0.w Period
	bsr	per2mfp		; d0.l=data.w | cntl.w

	
	move.l	d0,d1
	swap	d1		; d0.w=timer cntl, d1.w=timer data

sidcont:
	jsr	tm_cont(a4)
	moveq	#3,d0
	jsr	tm_enable(a4)

	movea.w tm_vect(a4),a2	; vector address
	movea.l (a2),a2		; next interrupt routine
	cmpa.l	tm_fct_l(a4),a2	; Is it low ?
	seq	d6
	and.b	ym_volume(a5),d6

prg_vol_and_per:
	;; program volume
	addq.b	#8,d7
	move.b	d7,(a3)
	move.b	d6,2(a3)
	subq.b	#8,d7

prg_per_only:
	;; program period
	add.b	d7,d7
	move.b	d7,(a3)		; perl
	move.b	d5,2(a3)

	addq.b	#1,d7
	lsr	#8,d5
	move.b	d7,(a3)
	move.b	d5,2(a3)	; PerH

	rts

nosid:
	;; move	sr,-(a7)
	;; move	#$2700,sr
	jsr	tm_stop(a4)
	bra.s	prg_vol_and_per
	
	
	
	
	IF 0
;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; 
;;; Convert MFP timer paramaters to YM pulse period
;;; 
;;; Inp: d0.l data.w|cntl.w
;;; Out: d0.w ym period
;;; 
mfp2per:
	move.w	d1,-(a7)
	clr.w	d1
	move.b	.mfpdiv(pc,d0.w),d1 ; d1.w timer predivisor
	swap	d0
	tst.b	d0		; timer data 0 is in fact 256
	beq.s	.fix8
	mulu	d1,d0		; d0=prediv*data
.cont:
	mulu	#834,d0
	add.l	#1<<12,d0	; round or not ?
	lsl.l	#3,d0
	swap	d0
	move.w	(a7)+,d1
	rts
.fix8:	
	move d1,d0
	lsl #8,d0		; d0=prediv*256
	bra.s .cont
.mfpdiv:
	dc.b 0,4,10,16,50,64,100,200
	ENDC

;;; ----------------------------------------------------------------------
;;; ----------------------------------------------------------------------
;;; 
;;; Convert YM pulse period to MFP timer parameters
;;; 
;;; Inp: d0.w ym period
;;; Out: d0.l data.w|cntl.w
;;; 
per2mfp:
	cmp.w	#$1a1,d0	;  417
	bgt	.gt1a1

	cmp.w	#$068,d0	;  104
	ble	.068_000

	cmp.w	#$104,d0	;  260
	ble	.104_069

.1a1_105:
	;; [$1a1-$105] prediv 3/16, fixed point 16
	mulu	#40265,d0
	add.l	#1<<15,d0
	move	#3,d0		; data.w|cntl.w
	rts

.104_069:
	;; [$104-$069] prediv 2/10, fixed point 16
	mulu	#64424,d0
	add.l	#1<<15,d0
	move	#2,d0		; data.w|cntl.w
	rts

.068_000:
	;; [$068-$000] prediv 1/4, fixed point 14
	mulu	#40265,d0
	add.l	#1<<13,d0
	lsl.l	#2,d0
	move	#1,d0		; data.w|cntl.w
	rts

.gt1a1:
	cmp.w	#$685,d0	; 1669
	bgt	.gt685
	cmp.w	#$518,d0	; 1304
	ble	.518_1a2

.685_519:
	;; [$685-$519] prediv 5/64, fixed point 18
	mulu	#40265,d0
	lsr.l	#2,d0
	move	#5,d0		; data.w|cntl.w
	rts

.518_1a2:
	;; [$518-$1a2] prediv 4/50, fixed point 18
	mulu	#51540,d0
	add.l	#1<<17,d0
	lsr.l	#2,d0
	move	#4,d0		; data.w|cntl.w
	rts

.gt685:
	cmp.w	#$a31,d0	; 2609
	ble	.a31_686

.fff_a32:
	;; [$fff-$a32] prediv 7/200, fixed point 20
	mulu	#51540,d0
	add.l	#1<<19,d0
	lsr.l	#4,d0
	move	#7,d0		; data.w|cntl.w
	rts

.a31_686:
	;; [$a31-$686] prediv 6/100, fixed point 19
	mulu	#51540,d0
	add.l	#1<<18,d0
	lsr.l	#3,d0
	move	#6,d0		; data.w|CNTL.W
	rts

;;; ----------------------------------------------------------------------
;;; 
;;; Timers
;;;
;;; ----------------------------------------------------------------------


;;; ----------------------------------------------------------------------
;;; Initialize the 4 timer interfaces. !!! DO NOT PROGRAM THE TIMER !!!
;;; 
timers_init:
	movem.l d0-d1/a0-a1/a4,-(a7)

	lea	timerA(pc),a4
	lea	tA_irq_l,a0
	lea	tA_irq_h,a1
	bsr.s	timer_init

	lea	timerB(pc),a4
	lea	tB_irq_l,a0
	lea	tB_irq_h,a1
	bsr.s	timer_init
	
	lea	timerC(pc),a4
	lea	tC_irq_l,a0
	lea	tC_irq_h,a1
	bsr.s	timer_init

	lea	timerD(pc),a4
	lea	tD_irq_l,a0
	lea	tD_irq_h,a1
	bsr.s	timer_init
	
	movem.l (a7)+,d0-d1/a0-a1/a4
	rts

timer_init:
	;; set lo and hi sid irqs
	move.l	a0,tm_fct_l(a4)
	move.l	a1,tm_fct_h(a4)

	;; save vector
	movea.w	tm_vect(a4),a0
	move.l	(a0),tm_save_vect(a4)

	;; save intena and intmsk
	moveq	#0,d0
	move.b	tm_mfp_chn(a2),d0
	move.b	tm_mfp_bit(a2),d1
	lea	$fffffa07.w,a0
	add	d0,a0
	btst	d1,(a0)
	sne	tm_save_iena(a4)
	btst	d1,$13-$07(a0)
	sne	tm_save_iena(a4)
	
	rts


;;; ----------------------------------------------------------------------
;;; Declare timer interface
;;;
;;; \1 Letter  {A B C D}
;;; \2 Vector  {$134 $120 $114 $110}
;;; \3 Channel {0,0,2,2}
;;; \4 Bit     {5,0,5,4}
	
DEC_TIMER:	MACRO 


	;; More compact version than the jump table
t\1_enable:
	btst	#0,d0
	beq.s	.ena0
	bset	#\4,$fffffa07+\3.w ; set intena
	btst	#1,d0
	beq.s	.msk0
.msk1:	
	bset	#\4,$fffffa13+\3.w ; set intmsk
	rts

.ena0:	
	bclr	#\4,$fffffa07+\3.w ; clr intena
	btst	#1,d0
	bne.s	.msk1
.msk0:
	bclr	#\4,$fffffa13+\3.w ; clr intmsk
	rts
	
	
;; t\1_enable:
;; 		lsl #4,d0
;; 		jmp .tjmp(pc,d0)
;; .tjmp:	
;; 		bclr #\4,$fffffa07+\3.w	; =6
;; 		bclr #\4,$fffffa13+\3.w	; +6 = 12
;; 		rts			; +2 = 14
;; 		nop			; +2 = 16

;; 		bset #\4,$fffffa07+\3.w	; =6
;; 		bclr #\4,$fffffa13+\3.w	; +6 = 12
;; 		rts			; +2 = 14
;; 		nop			; +2 = 16

;; 		bclr #\4,$fffffa07+\3.w	; =6
;; 		bset #\4,$fffffa13+\3.w	; +6 = 12
;; 		rts			; +2 = 14
;; 		nop			; +2 = 16

;; 		bset #\4,$fffffa07+\3.w	; =6
;; 		bset #\4,$fffffa13+\3.w	; +6 = 12
;; 		rts			; +2 = 14
;; 		nop			; +2 = 16

t\1_irq_l:
		move.b t\1_reg(pc),$ffff8800.w
		sf.b $ffff8802.w
		move.l _t\1_h(pc),\2.w
		rte

t\1_irq_h:
		move.b t\1_reg(pc),$ffff8800.w
		move.b t\1_vol(pc),$ffff8802.w
		move.l _t\1_l(pc),\2.w
		rte
	
timer\1:
		bra.s t\1_stop
		bra.s t\1_start
		bra.s t\1_cont
		bra.s t\1_enable
t\1_reg:	dc.w 0
t\1_vol: 	dc.w 0
_t\1_l:		dc.l 0
_t\1_h:		dc.l 0
t\1_vec:	dc.w \2
t\1_chn:	dc.b \3
t\1_bit:	dc.b \4
t\1_sav:	dc.l 0
t\1_ena:	dc.b 0
t\1_msk:	dc.b 0
		ENDM

	
	;; ---------------------------------------------------------------
	;;  Timer-A Interface
	;; 
	DEC_TIMER A,$134,0,5
	
tA_stop:
	clr.b	$fffffa19.w
	rts

tA_start:
	clr.b	$fffffa19.w
	move.l	_tA_h(pc),$134.w
	move.b	d1,$fffffa1f.w
	move.b	d0,$fffffa19.w
	rts

tA_cont:
	cmp.b	$fffffa19.w,d0
	beq.s	.noreprog
	clr.b	$fffffa19.w
	move.b	d1,$fffffa1f.w
	move.b	d0,$fffffa19.w
	rts
.noreprog:
	move.b	d1,$fffffa1f.w
	rts

	;; ---------------------------------------------------------------
	;;  Timer-B Interface
	;; 
	DEC_TIMER B,$120,0,0
	
tB_stop:
	clr.b	$fffffa1b.w
	rts

tB_start:
	clr.b	$fffffa1b.w
	move.l	_tB_h(pc),$120.w
	move.b	d1,$fffffa21.w
	move.b	d0,$fffffa1b.w
	rts

tB_cont:
	cmp.b	$fffffa1b.w,d0
	beq.s	.noreprog
	clr.b	$fffffa1b.w
	move.b	d1,$fffffa21.w
	move.b	d0,$fffffa1b.w
	rts
.noreprog:
	move.b	d1,$fffffa21.w
	rts

	;; ---------------------------------------------------------------
	;;  Timer-C Interface
	;; 
	DEC_TIMER C,$114,2,5

tC_stop:
	and.b	#$07,$fffffa1d.w
	rts

tC_start:
	moveq #$07,d2
	and.b	$fffffa1d.w,d2	; d2: timer-d part
	move.b	d2,$fffffa1d.w	; stop timer-c
	move.l	_tC_h(pc),$114.w
	move.b	d1,$fffffa23.w	; tc-dr
	lsl.b	#4,d0
	or.b	d2,d0
	move.b	d0,$fffffa1d.w
	rts

tC_cont:
	lsl.b	#4,d0
	move.b	$fffffa1d.w,d2
	moveq  #$07,d3
	and.b	d2,d3		; d3: timer-d part
	sub.b	d3,d2		; d2: timer-c part
	cmp.b	d0,d2
	beq.s	.noreprog
	move.b	d3,$fffffa1d.w	; stop timer-c
	move.b	d1,$fffffa23.w	; tc-dr
	or.b	d3,d0
	move.b	d0,$fffffa1d.w
	rts
.noreprog:
	move.b	d1,$fffffa23.w	; tc-dr
	rts

	;; ---------------------------------------------------------------
	;;  Timer-D Interface
	;; 
	DEC_TIMER D,$110,2,4

tD_stop:
	and.b	#$70,$fffffa1d.w
	rts

tD_start:
	moveq	#$70,d2
	and.b	$fffffa1d.w,d2	; d2: timer-c part
	move.b	d2,$fffffa1d.w	; stop timer-d
	move.l	_tD_h(pc),$110.w
	move.b	d1,$fffffa25.w	; td-dr
	or.b	d2,d0
	move.b	d0,$fffffa1d.w
	rts

tD_cont:
	move.b	$fffffa1d.w,d2
	moveq	#$07,d3
	and.b	d2,d3		; d3: timer-d part
	sub.b	d3,d2		; d2: timer-c part
	cmp.b	d0,d3
	beq.s	.noreprog
	move.b	d2,$fffffa1d.w	; stop timer-d
	move.b	d1,$fffffa25.w	; td-dr
	or.b	d2,d0
	move.b	d0,$fffffa1d.w
	rts
.noreprog:
	move.b	d1,$fffffa25.w	; td-dr
	rts
