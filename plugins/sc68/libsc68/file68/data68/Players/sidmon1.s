;	SIDMon 1 replay routine
;	adapted for sc68 by Gerald Schnabel <gschnabel@gmx.de>

	include "lib/org.s"

	bra.w   sidmon1_init
	bra.w   sidmon1_stop
	bra.w   sidmon1_play

;	* init *

sidmon1_init:
	lea	sidmon1_init_ok(pc),a0
	clr.w	(a0)

	; search for start of sound module ($41fa) within the
	; first $80 words.

	lea	sidmon1_tune(pc),a0
	moveq	#$80-1,d0
	move.w	#$41fa,d1
sidmon1_search_startofmodule:
	cmp.w	(a0),d1
	beq.b	sidmon1_searchstartofmodule_found

	addq.w	#2,a0
	dbf	d0,sidmon1_search_startofmodule

	; not found...

	bra.w	sidmon1_init_end

sidmon1_searchstartofmodule_found:
	lea	sidmon1_tune_start(pc),a1
	move.l	a0,(a1)

	; Check for jump into Kickstart ROM (argh!) at the end of replay
	; routine and replace it with a rts. This can be at different
	; positions.
	; I've seen some tunes that already have a rts there (done by a
	; music ripper(?!) ), so we have to take care of it...

	move.l	#$7fff4ef9,d0
	move.l	#$7fff4e75,d1
	moveq	#1,d2

	cmp.l	$0230-2(a0),d1
	beq.w	sidmon1_init_startofmodule_found

	cmp.l	$0230-2(a0),d0
	bne.b	sidmon1_check2

	move.w	d1,$0230(a0)

	bra.b	sidmon1_init_startofmodule_found

sidmon1_check2:
	moveq	#2,d2

	cmp.l	$023e-2(a0),d1
	beq.b	sidmon1_init_startofmodule_found

	cmp.l	$023e-2(a0),d0
	bne.b	sidmon1_check3

	move.w	d1,$023e(a0)

	bra.b	sidmon1_init_startofmodule_found

sidmon1_check3:
	moveq	#3,d2

	cmp.l	$0290-2(a0),d1
	beq.b	sidmon1_init_startofmodule_found

	cmp.l	$0290-2(a0),d0
	bne.b	sidmon1_check4

	move.w	d1,$0290(a0)

	bra.b	sidmon1_init_startofmodule_found

sidmon1_check4:
	moveq	#4,d2

	cmp.l	$0294-2(a0),d1
	beq.b	sidmon1_init_startofmodule_found

	cmp.l	$0294-2(a0),d0
	bne.b	sidmon1_check5

	move.w	d1,$0294(a0)

	bra.b	sidmon1_init_startofmodule_found

sidmon1_check5:
	moveq	#5,d2

	cmp.l	$029c-2(a0),d1
	beq.b	sidmon1_init_startofmodule_found

	cmp.l	$029c-2(a0),d0
	bne.b	sidmon1_check6

	move.w	d1,$029c(a0)

	bra.b	sidmon1_init_startofmodule_found

sidmon1_check6:
	moveq	#6,d2

	cmp.l	$02a4-2(a0),d1
	beq.b	sidmon1_init_startofmodule_found

	cmp.l	$02a4-2(a0),d0
	bne.b	sidmon1_check7

	move.w	d1,$02a4(a0)

	bra.b	sidmon1_init_startofmodule_found

sidmon1_check7:
	moveq	#7,d2

	cmp.l	$02be-2(a0),d1
	beq.b	sidmon1_init_startofmodule_found

	cmp.l	$02be-2(a0),d0
	bne.b	sidmon1_init_end

	move.w	d1,$02be(a0)

sidmon1_init_startofmodule_found:
	lea	sidmon1_init_ok(pc),a1
	move.w	d2,(a1)

	jmp	(a0)

sidmon1_init_end:
	rts

;	* stop *

sidmon1_stop:
	lea	sidmon1_init_ok(pc),a0
	move.w	(a0),d0
	beq.b   sidmon1_stop_end

	move.l	sidmon1_tune_start(pc),a0
	lea	sidmon1_tune_stopoffset(pc),a1
	subq.w	#1,d0
	add.w	d0,d0
	move.w	(a1,d0.w),d0
	tst.w	d0
	beq.b	sidmon1_stop_end

	add.w	d0,a0
	jmp	(a0)

sidmon1_stop_end:
	rts

;	* play *

sidmon1_play:
	lea	sidmon1_init_ok(pc),a0
	move.w	(a0),d0
	beq.b   sidmon1_play_end

	move.l	sidmon1_tune_start(pc),a0
	lea	sidmon1_tune_playoffset(pc),a1
	subq.w	#1,d0
	add.w	d0,d0
	move.w	(a1,d0.w),d0
	tst.w	d0
	beq.b	sidmon1_play_end

	add.w	d0,a0
	jmp	(a0)

sidmon1_play_end:
	rts

sidmon1_tune_start:
	dc.l	0
sidmon1_tune_playoffset:
	dc.w	$013e,$0118,$016a,$016a,$016a,$0164,$0164
sidmon1_tune_stopoffset:
	dc.w	$0120,$0000,$0144,$0144,$0144,$013e,$013e

sidmon1_init_ok:
	dc.w	0

sidmon1_tune:
