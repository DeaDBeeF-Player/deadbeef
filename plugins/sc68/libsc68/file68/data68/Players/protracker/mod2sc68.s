
endian:	macro
	dc.b	(\1)&255
	dc.b	((\1)>>8)&255
	dc.b	((\1)>>16)&255
	dc.b	((\1)>>24)&255
	endm

chtag:	macro
	dc.l	\1
	endian	\2
	endm
	
chint:	macro
	dc.l	\1
	endian	4
	endian	\2
	endm


begdat:	macro
data\@:	
	dc.l	\1
	endian	(.tad-.dat)
.dat:
	endm

enddat:	macro
	even
.tad:
	endm


;;; ======================================================================
;;; 
	
sc68:
	dc.b	"SC68 Music-file / (c) (BeN)jamin Gerard / SasHipA-Dev  ",0
begin:	
	chtag	"SC68",end-begin
	
	begdat	"SCFN"
	dc.b	"Protracker AGA test",0
	enddat

	chtag	"SCMU",0
	chint	"SCTY",4
	
	begdat	"SCRE"
	dc.b	"protracker",0
	even
	enddat
	
	begdat	"SCDA"
	incbin	"mod.mod"
	even
	enddat
	
	chtag	"SCEF",0
end:
