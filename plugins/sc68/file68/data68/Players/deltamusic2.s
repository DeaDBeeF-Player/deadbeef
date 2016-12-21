;	Delta Music 2 replay routine
;	adapted for sc68 by Gerald Schnabel <gschnabel@gmx.de>

	bra.w	deltamusic_init
	rts
	rts
	bra.w	deltamusic_play

deltamusic_init:
	moveq	#1,d0
	bra.b	deltamusic_tune

deltamusic_play:
	moveq	#0,d0
	bra.b	deltamusic_tune+4

deltamusic_tune:
