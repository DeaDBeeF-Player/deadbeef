;;; Atari ST GEMDOS and XBIOS emulator
;;;
;;; (C) COPYRIGHT 2011-2013 Benjamin Gerard
;;;
;;; Gemdos (trap #1) and Xbios (trap #14) functions
;;;

	;; Unhandled trap vector and function will execute a stop with a
	;; special value followed by a reset. The value is used to detect
	;; this code and to identify which vector was called.

StackSize = 1024	
STOP_VAL  = $2F20

	;; Machine definitions for '_MCH' cookie
MACHINE_FALCON30	= $00030000
MACHINE_STE 		= $00010000
MACHINE_MEGA_STE	= $00010010
MACHINE_TT		= $00020000

	;; This is the address used by libsc68/api.c but it should be
	;; PIC anyway. Still mksc68 rely on this code to be loaded at
	;; $1000 and timerc being at $1002
	
	org	$1000
	
	bra.s	install_trap

	;; timerc MUST BE at ORG+2 or mksc68 detection will FAILED
timerc:
	btst	#3,$fffffa17.w	; SEI or AEI ?
	beq.s	.aei
	move.b	#%11011111,$fffffa11.w ; release in service
.aei:	
	rte

	
;;; Install trap vectors
;;; 
install_trap:
	movem.l	d0/a0-a2,-(a7)

	;; Used by Zounddrager for fade out
	move.w	#16,$41a.w

	;; Install cookie jar for machine detection
	;;
	;; $$$ Disable ATM because it breaks sndh quartet replay
	;; 
	;; lea	cookie_jar(pc),a0
	;; move.l	a0,$5a0.w

	;; Install trap vectors for GEMDOS and XBIOS
	lea	irte(pc),a0
	move.l	a0,$14.w	; divide-by-0
	move.l	a0,$80.w	; trap #0 (sc68 special)
	lea	gemdos(pc),a0
	move.l	a0,$84.w
	lea	xbios(pc),a0
	move.l	a0,$B8.w

	;; Install system timer C
	lea	$fffffa00.w,a1
	clr.b	$1d(a1)		; stop timer-C/D
	lea	timerc(pc),a0
	move.l	a0,$114.w	; interrupt vector
	move.w	#$0020,d0	; set TC bit only (all-timer=$2130)
	movep.w	d0,$07(a1)	; IER
	movep.w	d0,$13(a1)	; IMR
	move.b	#2457600/(64*200),$fffffa23.w ; $c0 for 200hz timer
	move.b  #$50,$fffffa1d.w ; start TC with  prediv 64

	;; Init dummy malloc system
	lea	malloc(pc),a0
	lea	-StackSize(a7),a1 ; a7 is at near the end of memory
	move.l	a1,-(a0)

	movem.l	(a7)+,d0/a0-a2
	rts


open:	macro
	{
	movem.l	d0-a6,-(a7)		; 15*4
	lea	6+15*4(a7),a6
	move.w	(a6)+,d0		; function
	}

close:	macro
	{
	movem.l	(a7)+,d0-a6
	rte
	}	

return:	macro
	{
	move.l	\1,(a7)
	}
	
ret_d0:	macro
	{
	return	d0
	}
	
trap_n:	macro
	{
trap_\1:
	stop	#STOP_VAL+'\1'
	reset
	rte
	}
	
;;; Gemdos functions
;;;
;;; 0(a7).w -> SR
;;; 2(a7).l -> PC
;;; 6(a7).w -> Function !!! Cou

trap_close:
	close
irte:	= *-2
	
	;; trap_n 0
	;; trap_n 1
	;; trap_n 2
	;; trap_n 3
	;; trap_n 4
	;; trap_n 5
	;; trap_n 6
	;; trap_n 7
	;; trap_n 8
	;; trap_n 9
	;; trap_n A
	;; trap_n B
	;; trap_n C
	;; trap_n D
	;; trap_n E
	;; trap_n F
	
;;; ======================================================================
;;; TRAP #1 GEMDOS
;;; ======================================================================
	
gemdos:
	open

	cmp.w	#$09,d0
	beq.s	cconws
	
	cmp.w	#$20,d0
	beq.s	super
	
	cmp.w	#$30,d0
	beq.s	sversion

	cmp.w	#$48,d0
	beq.s	malloc

	cmp.w	#$49,d0
	beq.s	mfree

	stop	#STOP_VAL+$1
	reset
	
	bra	trap_close

;;; ======================================================================
;;; cconws(string.l)
;;; trap #1 function 09 ($09)
;;; 
;;; @param  string   pointer to string to display
;;; @retval >0       number of char written
;;; @retval 0        on error
cconws:
	move.l	(a6)+,a0		; string.l
	moveq	#0,d0
.loop:
	addq.w	#1,d0
	beq.s	.done			; string too long
	tst.b	(a0)+
	bne.s	.loop
.done:
	ret_d0
	bra	trap_close
	
	
;;; ======================================================================
;;; super(stack.l)
;;; trap #1 function 32 ($20)
;;; 
;;; @param  stack   0:superuser mode, 1:query state, >1 restore user mode
;;; @retval >1           old stack value to be restored by later call
;;; @retval SUP_USER(0)  on read-state and state is user mode
;;; @retval SUP_SUPER(1) on read-state and state is super-user mode

SUP_USER    = 0		      ; query return code for user mode
SUP_SUPER   = 1		      ; query return code for superuser mode
SUP_SET     = 0		      ; query code to set superuser mode
SUP_INQUIRE = 1		      ; query current mode
	
super:
	move.l	(a6)+,d0		; stack.l
	beq.s	.sup_set		; stack.l == 0 : set user
	cmp.l	#1,d0
	beq.s	.sup_inquire		; stack.l == 1 : inquire current mode

;;; restore user mode and superuser stack
	moveq	#0,d0
	beq.s	.done
	
;;; inquire current mode
.sup_inquire:
	move.w	sr,d0
	and.w 	#$2000,d0
	sne	d0
	and.w	#SUP_SUPER,d0
	beq.s	.done

;;; set superuser mode (this is not the real deal but we don't care much)
.sup_set:
	moveq	#SUP_SUPER,d0
	
.done:	
	ret_d0
	bra	trap_close
	
;;; ======================================================================
;;; sversion()
;;; trap #1 function 48 ($30)
;; @retval
;; 0x1300	TOS 1.00, TOS 1.02
;; 0x1500	TOS 1.04, TOS 1.06
;; 0x1700	TOS 1.62
;; 0x1900	TOS 2.01, TOS 2.05, TOS 2.06, TOS 3.01, TOS 3.05, TOS 3.06
;; 0x3000	TOS 4.00, TOS 4.01, TOS 4.02, TOS 4.03, TOS 4.04,
;;	 	MultiTOS 1.00, MultiTOS 1.08
sversion:
	move.l	#$1500,d0
	ret_d0
	bra	trap_close

;;; ======================================================================
;;; malloc(amount.l)
;;; trap #1 function 72 ($48)
	dc.l 0				; malloc_ptr
malloc:
	lea	malloc(pc),a0
	move.l	-(a0),d0		; read malloc_ptr
	sub.l	(a6)+,d0		; alloc amount
	bclr	#0,d0			; ensure even ptr
	move.l	d0,(a0)			; save malloc_ptr
	ret_d0
	bra	trap_close

;;; ======================================================================
;;; mfree(ptr)
;;; trap #1 function 73 ($49)
mfree:
	moveq	#0,d0
	ret_d0
	bra	trap_close
	
;;; ======================================================================
;;; mxmalloc(amount.l,mode.w)
;;; trap #1 function 68 ($44)


;;; ======================================================================
;;; TRAP #E XBIOS
;;; ======================================================================

xbios:
	open

	cmp.w	#$1f,d0
	beq	xbtimer

	cmp.w	#$20,d0
	beq.s	dosound

	cmp.w	#$26,d0
	beq.s	superexec

	cmp.w	#$80,d0
	beq.s	locksnd

	cmp.w	#$81,d0
	beq.s	unlocksnd

	stop	#STOP_VAL+$E
	reset
	bra	trap_close

;;; ======================================================================
;;; Superexec(addr.l)
;;; trap #14 function 38 ($26)
superexec:
	pea	trap_close(pc)
	move.l	(a6)+,-(a7)
	rts
	
;;; ======================================================================
;;; Locksnd()
;;; trap #14 function 128 ($80)
	dc.w	0
locksnd:
	move.w	#-129,d0		; SNDLOCKED (-129)
	lea	locksnd(pc),a0
	tas.b	-(a0)
	bne.s	.locked
	moveq	#1,d0
.locked:
	ret_d0
	bra	trap_close

;;; ======================================================================
;;; Unlocksnd()
;;; trap #14 function 129 ($81)
unlocksnd:
	moveq	#-128,d0		; SNDNOTLOCK (-128)
	lea	locksnd(pc),a0
	tst.b	-(a0)
	beq.s	.notlocked
	clr.b	(a0)
	moveq	#0,d0
.notlocked:
	ret_d0
	bra	trap_close

	
;;; ======================================================================
;;; Dosound(addr.l)
;;; trap #14 function 32 ($20)
	dc.w	0		; -6: tmp storage
	dc.l	0		; -4: ptr storage
dosound:
	lea	dosound(pc),a0
	move.l	(a6)+,a1	; sound commands ptr 
	cmpa.l	#1,a1
	bne.s	.no_inquire
	return	-(a0)
	bra.s	.bye
.no_inquire:
	lea	$ffff8800.w,a2
	moveq	#0,d0
	move	-4(a0),d1	; tmp register
.next:
	move.b	(a1)+,d0
	bmi.s	.commands
	lsl	#8,d0
	move.b	(a1)+,d0
	movep	d0,0(a2)
	bra.s	.next
.commands:
	cmp.b	#$80,d0
	bne.s	.no80
	move.b	(a1)+,d1
	bra.s	.next
.no80:
	cmp.b	#$81,d0
	bne.s	.no81

	move.b	(a1)+,d0	; YM register to program
	move.b	(a1)+,d2	; increments (2nd-complement)
	move.b	(a1)+,d3	; stop value
.lp80:
	move.b	d0,d4
	lsl	#8,d4
	move.b	d1,d4
	movep	d4,0(a2)
	cmp.b	d1,d3
	beq.s	.finish
	sub.b	d2,d1
	bra.s	.lp80
.no81:
	move.b	(a1)+,d0
	beq.s	.finish
	;; d0 is a number of frame to wait, we can't do that here
	stop	#STOP_VAL+$E
	reset
	bra.s	.bye
.finish:
	move.l	a1,-(a0)	; store ptr
	move.w	d1,-(a0)	; store tmp
.bye:	
	bra	trap_close


;;; ======================================================================
;;; Xbtimer(timer.w, ctrl.w, data.w, vector.l)
;;; trap #14 function 31 ($1f)

	RSRESET
timer_vector:	rs.w	1
ti_ctrlreg:	rs.w	1
ti_datareg:	rs.w	1
ti_mask:	rs.b	1
ti_channel:	rs.b	1
ti_sz:		rs.b	0	

;;; vector, ctrl-reg, data-reg, msk.b+chan.q+bit.q
timers:
tAdef:	dc.w	$134, $fa19, $fa1f, $f005
tBdef:	dc.w	$120, $fa1b, $fa21, $f000
tCdef:	dc.w	$114, $fa1d, $fa23, $0f15
tDdef:	dc.w	$110, $fa1d, $fa25, $f014

xbtimer:
	lea	timers(pc),a2

	;; get timer
	moveq	#3,d0
	and	(a6)+,d0	; timer number (param)
	lsl	#3,d0		; offset
	add	d0,a2		; a2: timer definition

	;; stop the timer
	move.w	ti_ctrlreg(a2),a5	; a5: control reg addr
	move.b	(a5),d4
	move.b	ti_mask(a2),d0	; d0: mask
	and.b	d0,d4
	move.b	d4,(a5)		; timer stopped

	;; prepare ctrl
	move	(a6)+,d1	; ctrl (param)
	not.b	d0
	smi	d2
	and	#4,d2		; d2: shift
	lsl	d2,d1
	and	d0,d1
	or	d1,d4		; d4: ctrl register (ready)

	;; set timer count (data register)
	move.w	ti_datareg(a2),a4 ; a4: data reg addr
	move	(a6)+,d0
	move.b	d0,(a4)		; data reg (done)

	;; set intena and intmsk
	moveq	#$7,d0
	moveq	#0,d1
	move.b	ti_channel(a2),d1
	and.b	d1,d0			; d0 = bit number
	lsr	#3,d1			; d1 is channel [0/2]
	lea	$fffffa00.w,a1
	bset	d0,$07(a1,d1)
	bset	d0,$13(a1,d1)
	
	;; This should not be needed as the Atari system is in SEI
	;; mode. The program should take care of this if it wants to be
	;; in AEI.
	;; bclr	#3,$17(a1)	; set MFP to AEI

	;; set vector and start
	move	(a2),a2		; timer vector
	move.l	(a6)+,(a2)	; set vector (param)
	move.b	d4,(a5)		; start the timer
	
	bra	trap_close
	
cookie_jar:
	dc.l	"_MCH",MACHINE_STE,0
