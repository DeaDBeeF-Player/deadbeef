;	FRED replay routine
;	adapted for sc68 by Gerald Schnabel <gschnabel@gmx.de>

	bra.w	fred_init
	bra.w	fred_stop
	bra.w	fred_data+4

fred_init:
	moveq	#0,d0
	bra.b	fred_data

fred_stop:
	moveq	#0,d1
	bra.b	fred_data+8

fred_data:
