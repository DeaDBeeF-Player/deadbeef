toto:	
	;stop #$2300
	bsr.s	m+8
	move.w	#$7ffe,d0
.lp:
	nop
	nop
	nop
	nop
	dbf		d0,.lp
	bra.s	toto
m:
	