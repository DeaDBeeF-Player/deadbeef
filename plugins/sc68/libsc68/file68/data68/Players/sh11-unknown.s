;;; sc68 loader for JARI NIKULA's sommerhack2011 compo
	bra	init
	bra	quiet
	bra	play
	
init:
	bsr.s	reloc
	bra	prg+$FE	;Init

quiet:
	bsr.s	reloc
	bra	prg+$0C0	;YM-OFF

play:
	bra	prg+$1F4	;VBL
	
isreloc dc.w 0
reloc:
	lea	isreloc(pc),a0
	tst.w	(a0)
	bne.s	.ok_reloc
	st	(a0)
	lea	prg(pc),a0
	bsr.s	load_prg
.ok_reloc:
	rts

	include "custom/TosReloc.s"
prg:
	incbin "org/sh11-unknown.prg"
