;;; 
;;; Pack-ice, a great packer written by Axe of Delight!
;;; 
;;; Time-stamp: <2013-07-21 07:46:43 ben>
;;; 
;;; PACK-ICE is simply the best packer that's available on the ST. I
;;; used especially much time to get the depacking-routine as
;;; small and fast (!!!) as possible. Assemble this source with
;;; Devpac 2 and set TAB-position to 10. An update of this version
;;; is planned and will be finished at the beginning of march.
;;; 
;;; 2013/07 Benjamin Gerard AKA Ben of Overlanders
;;; 
;;;  - Strip all system code to keep only the core routine.
;;;  - PIC (Position Independant Code) and reentrant.
;;;  - m68k ABI ("C" calling convention)


	bra.s IcePackerBin
	
IcePackerCcall:


	RSRESET
st_stk	rs.b	4*11+8
st_dst	rs.l	1
st_max	rs.l	1
st_src	rs.l	1
st_len	rs.l	1
st_opt	rs.l	1
st_end	rs.l	0	
	
	;; d0/d1/a0/a1 don't need to be saved (m68k ABI)
	movem.l d2-d7/a2-a6,-(a7)

	;; retrieve parameters from stack
	lea	st_dst(a7),a1
	move.l	(a1)+,d2
	move.l	(a1)+,d0
	move.l	(a1)+,a0
	move.l	(a1)+,d1
	move.l	(a1)+,a1
	bsr.s	IcePacker
	movem.l	(a7)+,d2-d7/a2-a6
	rts

IcePackerBin:	
	movem.l	d1-a6,-(a7)
	bsr.s	IcePacker
exit:	
	movem.l	(a7)+,d1-a6
	rts

;;; ----------------------------------------------------------------------
;;; Ice Packer call
;;; 
;;; IN:  a0: source buffer (data to compress)
;;;      d0: source length
;;;
;;;      a1: destination buffer (compressed data storage)
;;;      d1: destination maximum length (currently ignored !!!)
;;;
;;;      d2: options (undefined bits must be clear)
;;;          bit#0 choose magic identifer 0:'ICE!' 1:'Ice!'
;;; 
;;; OUT: d0: packed size (-1 on error)

	
IcePacker:

	;; a2: allocated private data in the stack 
	lea	-size(a7),a2
	move.l	a2,a7

	;; clear privates
	moveq	#size/2-1,d7
.clear:
	clr.w	(a2)+
	dbf	d7,.clear
	lea	-size(a2),a2

	;; setup privates
	move.l	a0,srcbuf(a2)
	move.l	d0,srclen(a2)
	add.l	d0,a0
	move.l	a0,srcend(a2)
	
	move.l	a1,dstbuf(a2)
	tst.l	d1
	bne.s	.gotd1
	move.l	d0,d1
.gotd1:
	move.l	d1,dstlen(a2)
	add.l	d1,a1
	move.l	d1,dstend(a2)
	move.l	#$1580,optim(a2) ; ???

	;; Choose which magic identifier to use.
	;; 
	;; 2.35 original is 'Ice!' but the 2.40 'ICE!' value is much
	;; more common. As the 2.40 unpacker will properlly decode
	;; 2.35 packed data, it is probably better to use the 2.40 'ICE!'
	;; identifier.
	
	move.l	#'ICE!',d7
	btst	#0,d2
	beq.s	.id240
	move.l	#'Ice!',d7
.id240:	
	move.l	d7,magic(a2)

	;; Let's go !!
	bsr	crunch
	move.l	d7,d0
	tst	error(a2)
	beq.s	outofhere
	move	#-1,d0
	
outofhere:
	adda.w	#size,a7
	rts
 
;; Private data structure
	RSRESET
magic:	rs.l 1
srcbuf:	rs.l 1
srcend: rs.l 1
srclen: rs.l 1
dstbuf:	rs.l 1
dstend: rs.l 1
dstlen: rs.l 1
optim: 	rs.l 1	; $1580
error:	rs.w 1
same:	rs.w 1
length:	rs.w 1
offset:	rs.w 1
size:	rs.w 0
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

crunch:
	movea.l	srcbuf(a2),a0	; a0 = Data to be packed
	move.l	srclen(a2),d0	; d0 = Data size
	movea.l	dstbuf(a2),a1 	; a1 = Destination buffer for packed data
	move.l	magic(a2),d7
	bsr	longword_store
	addq.l	#4,a1		; Leave space for packed length
	move.l	srclen(a2),d7
	bsr	longword_store
	moveq	#0,d5		; d5 = Stored Bytes
	moveq	#7,d6		; d6 = Counter for stored bits
	moveq	#0,d7		; d7 = Information longword
	moveq	#0,d4		; Put off 1 Bit (Pictureflag)
	moveq	#0,d1		; Pictureflag always 0
	bsr	put_bits

mainloop:
; 1. Sequence of identical bytes are looking for pay
	lea	$409(a0),a4	; a4 = End of the search range
	cmpa.l	srcend(a2),a4	; Exceeded end of file?
	ble.s	gleich_ok	; No!
	movea.l	srcend(a2),a4	; End of data
gleich_ok:
	move.l	a0,a3		; a3 = Beginning of the search range
	move.b	(a3)+,d0	; current byte
gleich_compare:
	cmp.b	(a3)+,d0	; Byte mit akt. Byte vergleichen
	bne.s	gleich_ende	; nicht mehr gleich
	cmpa.l	a4,a3		; Ende des Suchbereichs erreicht?
	blt.s	gleich_compare	; nein, suche noch weiter
gleich_ende:
	move.l	a3,d1		; a1 = Ende des Strings
	sub.l	a0,d1		; a0 = Anfang
	subq.l	#2,d1		; dem Byte folgen d1 gleiche Kopien
	move.w	d1,same(a2)	; bisheriger lÑngste Bytefolge
	cmp.w	#$409,d1		; lÑnger als lÑngster String?
	beq	gleich_ablegen	; ja: String gar nicht erst testen

;;; 2. Search string with the greatest possible length and a small offset

	move.l	a0,a3		; a3 = Anfang des Suchbereichs ...
	adda.l	optim(a2),a3	; + LÑnge = Ende des Suchbereichs
	cmpa.l	srcend(a2),a3	; Dateiende Åberschritten?
	ble.s	offset_ende	; nein!
	movea.l	srcend(a2),a3	; Dateiende
offset_ende:
	moveq	#1,d4		; d4 = maximale StringlÑnge
	lea	2(a0),a4	; a4 = a0 + 1

weiter_mit_string:
	move.b	(a0),d0		; current nach d0
	move.b	1(a0),d1	; current + 1 nach d1
search_string:
	cmp.b	(a4)+,d0	; nach gleichem Byte suchen
	beq.s	ein_byte_stimmt
no_string:
	cmpa.l	a4,a3		; sucht stringanfang innerhalb
	bgt.s	search_string	; current + offset
	bra.s	string_suche_fertig

ein_byte_stimmt:
	cmp.b	(a4),d1		; current + n+1 mit current + 1
	bne.s	no_string 	; string stimmt nicht mehr
string_start_found:
	move.l	a4,a6		; a6: String an dieser Adresse
	subq.w	#1,a6		;     genauer untersuchen
	move.l	a6,d0		; d0 = auch Start des Strings
	movea.l	a0,a5		; a5 = current Pointer
string_compare:
	cmpm.b	(a5)+,(a6)+	; vergleiche soweit es geht
	bne.s	string_zu_ende	; nicht mehr gleich
	cmp.l	d0,a5		; sich selbst gefunden?
	bhi.s	string_zu_ende	; ja: dann Ende des Strings
	cmpa.l	a6,a3		; Ende des Suchbereichs erreicht?
	bgt.s	string_compare	; nein, suche noch weiter
string_zu_ende:
	move.l	a5,d1		; a5 = Ende des Strings
	sub.l	a0,d1		; a0 = Anfang
	subq.l	#1,d1		; => d1 = LÑnge
	move.l	a6,d2		; Ende der Kopie
	sub.l	a5,d2		; Ende Kopie minus Ende Original
	sub.l	d1,d2		; => d2 = Bytes zwischen Strings
	addq.l	#1,d2		; 0 Byte bei gleichen Bytes

	move.l	#$409,d0		; d0: maximale StringlÑnge
	cmp.l	d0,d4		; maximale LÑnge erreicht?
	beq.s	string_suche_fertig ; ja: aufhîren
	cmp.l	d0,d1		; maximale LÑnge Åberschritten?
	bls.s	not_too_large	; nein!
	sub.l	d0,d1		; um wieviel zu groû?
	add.l	d1,d2		; Das wird zum Offset addiert
	move.l	d0,d1		; und LÑnge auf Maximalwert
not_too_large:

	cmp.l	d1,d4		; lÑnger als bisheriger lÑngster
	bge.s	string_lohnt_nicht	; nein, dann lohnt es noch nicht

	cmp.w	#2,d1		; Wenn der String lÑnger ...
	bls.s	shortstring	; als 2 Bytes ist, und
	cmp.w	#$111f,d2 	; d2 grîûer als $111f ist, soll
	bhi.s	string_lohnt_nicht	; nicht gepackt werden!
	
shortstring:
	move.l	d1,d4		; neue maximale StringlÑnge ...
	move.w	d1,length(a2)	; in d4 und maxlength und ...
	move.w	d2,offset(a2)	; dann zugehîrigen Offset merken
string_lohnt_nicht:
	cmpa.l	a4,a3		; Schon Åber die Grenze hinaus?
	bgt	weiter_mit_string	; nein!

string_suche_fertig:
	move.w	same(a2),d0
	cmp.w	#1,d0		; nur ein gleiches Byte?
	ble.s	nogleich		; ja: nichts machen
	cmp.w	d4,d0		; vergleiche Bytes mit Strings?
	bge.s	gleich_ablegen	; mehr gleiche Bytes
nogleich:
	move.w	offset(a2),d0	; Parameter des lÑngsten
	move.w	d4,d1		; Strings nach d0 und d1
	cmpi.w	#1,d1		; Ist String ein Byte lang?
	beq.s	ein_byte_ablegen	; ja
	cmpi.w	#2,d1		; Ist String zwei Bytes lang?
	beq.s	zwei_byte_ablegen	; ja
	bra.s	mehr_bytes_ablegen	; nein: Dann ist er lÑnger!

;**************************************************************************

ein_byte_ablegen:
	move.b	(a0)+,(a1)+	; ein Byte unverÑndert ablegen
	addq.l	#1,d5		; d5 = BytezÑhler
	bra.s	kein_byte_ablegen	; Byte wurde abgelegt

gleich_ablegen:
	move.w	same(a2),d1	; Anzahl gleicher Bytes
	move.w	d1,length(a2)
	moveq	#0,d0		; Offset ist immmer 0
	subq.w	#2,d1		; Anzahl gleicher Bytes = 2?
	bne.s	mehr_bytes_ablegen	; nein, also mehr

zwei_byte_ablegen:
	cmpi.w	#$23f,d0	  ; liegen Strings weit auseinander?
	bhi.s	ein_byte_ablegen  ; ja
	bsr	make_normal_bytes ; Information der vorherigen Bytes

	cmpi.w	#$23f,d0		; Abstand zwischen Strings zu groû?
	bhi	ein_byte_ablegen	; ja: nichts packen
	bsr	make_offset_2	  ; Offset zu String in Bits ablegen
	
	bra.s	drop_length	  ; String length ablegen

mehr_bytes_ablegen:
	bsr	make_normal_bytes	; Information der vorherigen Bytes
	bsr	make_offset_mehr	; Offset fÅr lange Strings
drop_length:
	bsr	make_stringlength	; String-LÑnge in Bits ablegen
	moveq	#0,d5		; ab jetzt wieder 0 Bytes abgelegt
	move.w	length(a2),d0	; d0 Bits wurden eben gepackt
	add.w	d0,a0		; und werden Åbersprungen

kein_byte_ablegen:
	move.l	srcend(a2),d0 	; Kann noch gepackt werden?
	sub.l	a0,d0		; Ist Ende erreicht?
	subq.l	#3,d0		; 3 Bytes vor Ende ist Schluû
	bpl	mainloop		; nein, noch davor

still_packing:
	cmp.l	srcend(a2),a0	; Wurden alle Bytes gepackt?
	bge.s	all_packed	; ja!
	move.b	(a0)+,(a1)+	; nur noch ein Byte ablegen
	addq.l	#1,d5		; BytezÑhler erhîhen
	bra.s	still_packing
all_packed:
	bsr	make_normal_bytes	; Informationsbyte ablegen
	bset	d6,d7		; letztes Info-Byte erzeugen
	move.b	d7,(a1)+		; und ablegen
	sub.l	dstbuf(a2),a1	; minus Anfang des Puffers
	move.l	a1,d0		; nach d0
	move.l	d0,d7
	move.l	dstbuf(a2),a1
	addq.l	#4,a1
	bsr	longword_store
	rts	

make_normal_bytes:
	cmp.l	#$810d,d5 	; stored for many individual bytes ?
	bls.s	noerror		; yes ! can do no more
	move.w	#-1,error(a2) 	; report error
	move.l	a0,srcend(a2)	; and abort
noerror:
	lea.l	table1(pc),a3	; a3 = Byte table
	moveq	#6,d3		; d3 = Pointer in table
kleiner:
	move.w	-(a3),d4
	cmp.w	d4,d5		; Number of already stored
	dbge	d3,kleiner	; Search bytes in table
	sub.w	d4,d5		; d5 = Excess of table
	add.w	d3,d3		; d3 = word pointer
	lea	table1(pc),a3
	adda.w	d3,a3
	move.b	(a3)+,d2

	ext.w	d2		; Treated as word
	moveq	#-1,d1		; Set all bits
	lsl.l	d2,d1		; Follow up on the right zeros
	or.w	d5,d1		; Now in d1: 
				; %11111..1110 d5 (all Bits)
	moveq	#0,d5		; all bytes stored: new count!
	move.b	(a3)+,d4	; d4 now in the Bit amount byte
	ext.w	d4		; fetch and extend to word
	subq.w	#1,d4
	bra	put_bits	; d4+1 Bits in Information byte


;----------------------------------------------------------------------
;       Number Bytes    stored          shifted assigned
;----------------------------------------------------------------------
;       0               %0                 1       1
;       1               %10                1       2
;       2               %1100              2       4
;       3               %1101
;       4               %1110
;       5               %111100            2       6
;       6               %111101
;       7               %111110
;       8               %111111000         3       9
;       9               %111111001
;       10              %111111010
;       11              %111111011
;       12              %111111100
;       13              %111111101
;       14              %111111110
;       15              %11111111100000000 8      17
;       16              %11111111100000001
;         ..                  ..
;       269   (=$10d)   %11111111111111110
;       270   (=$10e)   %11111111111111111000000000000000
;       271   (=$110)   %11111111111111111000000000000001
;         ..                  ..
;       33037 (=$810d)  %11111111111111111111111111111111
;----------------------------------------------------------------------

	dc.w     0,    1,   2,    5,    8,   15,  270
table1:	dc.w $0101,$0102,$0204,$0206,$0309,$0811,$0f20
; 1. List of all standard offsets
; 2. Number of bit shifted, number of bit assigned

make_offset_mehr:
	lea.l	table3(pc),a3	; a3 = Zeiger auf Tabelle
	moveq	#2,d3		; d3 = Zeiger in Tabelle
look_on:
	add.w	d3,d3		; d3 verdoppeln (mîglich: 4,2,0)
	move.w	0(a3,d3.w),d4	; d4 = Wert aus Tabelle
	lsr.w	#1,d3		; d3 zurÅcksetzen
	cmp.w	d4,d0		; mit gesuchtem Offset vergleichen
	dbge	d3,look_on	; nur wenn kleiner: nÑchsten Wert
	sub.w	d4,d0		; d0: Um wieviel war Offset zu groû
	add.w	d3,d3		; d3 wieder auf Wort positionieren
	move.w	6(a3,d3.w),d3	; aber diesmal in der zweiten Tab.
	move.w	d3,d4		; d4 = Anzahl zu Åbertragene Bits
	lsr.w	#8,d3		; d3 = Shift-Register
	moveq	#-1,d1		; d1 = alle Bits gesetzt
	lsl.w	d3,d1		; d1 nach links shiften
	or.w	d0,d1		; und d0 in LÅcke setzen
	andi.w	#$f,d4		; hîchstens 15 Bits Åbertragen
	bra	put_bits		; Bits als Information speichern

;----------------------------------------------------------------------
;	Offset		stored		shiften   Åbertragen
;----------------------------------------------------------------------
;	0		%1000000		6	7
;	1		%1000001		6	7
;	..		..		..	..
;	$1f		%1011111		6	7
;	$20		%000000000	9	9
;	$21		%000000001	9	9
;	$22		%000000010	9	9
;	..		..		..	..
;	$11f		%011111111	9	9
;	$120		%11000000000000 	12	14
;	$121		%11000000000001 	12	14
;	$122		%11000000000010 	12	14
;	..		..		..	..
;	$111f		%11111111111111 	12	14
;----------------------------------------------------------------------

table3:	dc.w $0000,$0020,$0120
	dc.b $06,$06,$09,$08,$0C,$0D


make_offset_2:
;;; Ben: moved to caller
;;; cmpi.w	#$23f,d0		; Abstand zwischen Strings zu groû?
;;; bhi	ein_byte_ablegen	; ja: nichts packen
	move.w	d0,d1		; d0/d1 = Abstand zwischen Strings
	cmpi.w	#$3f,d1		; Abstand kleiner oder gleich $3f
	ble.s	offs_3f		; ja
	subi.w	#$40,d1		; Offset: zwischen 0 und $1ff
	moveq	#9,d4		; 10 Bits als Information ablegen
	bset	d4,d1		; d1: zwischen $200 und $3ff
	bra	put_bits		; Information ablegen
offs_3f:
	moveq	#6,d4		; 7 Bits Information ablegen
	bra.s	put_bits		; d1: zwischen 0 und $3f

;----------------------------------------------------------------------
;	Offset		stored		Åbertragen
;----------------------------------------------------------------------
;	0		%0000000		7
;	1		%0000001		7
;	2		%0000010		7
;	3		%0000011		7
;	..		..		..
;	$3f		%0111111		7
;	$40		%1000000000	10
;	$41		%1000000001	10
;	$42		%1000000010	10
;	..		..		..
;	$23e		%1111111110	10
;	$23f		%1111111111	10
;----------------------------------------------------------------------


make_stringlength:
	move.w	length(a2),d0 ; d0 = String length
	moveq	#4,d5		 ; d5 = Pointer in table2
.search:
	move.b	.ta(pc,d5.w),d4	; d4 = gelesenes Byte aus Tabelle
	ext.w	d4		; als Wort behandeln
	cmp.w	d4,d0		; Ist der String grîûer oder gleich?
	dbge	d5,.search 	; nein: In Tabelle weitersuchen
	sub.w	d4,d0		; d0 = um wieviel lagen wir daneben
	move.b	.tb(pc,d5.w),d4	; Shift Byte auslesen
	ext.w	d4		; als Wort behandeln
	moveq	#-1,d1		; d1: alle Bits auf 1 setzen
	lsl.w	d4,d1		; d1 nach links shiften
	or.w	d0,d1		; Bits in d1 setzen
	add.w	d5,d4		; d4 = Anzahl zu Åbertragene Bytes
				; mîgliche Ergebnisse: 1,2,4,6,14
	subq.w	#1,d4		; wegen dbf-Schleife
	bra.s	put_bits

	
;----------------------------------------------------------------------
;	String length	stored		shiften   Åbertragen
;----------------------------------------------------------------------
;	2		%0		1	1
;	3		%10		1	2
;	4		%1100		2	4
;	5		%1101		2	4
;	6		%111000		3	6
;	7		%111001		3	6
;	8		%111010		3	6
;	9		%111011		3	6
;	10		%11110000000000 	10	14
;	11		%11110000000001 	10	14
;	..		..		..	..
;	$409		%11111111111111 	10	14
;----------------------------------------------------------------------
.ta:	dc.b	$02,$03,$04,$06,$0a
.tb:	dc.b	$01,$01,$02,$03,$0a


;;; IN
put_bits:
	lsr.l	#1,d1		; insert less significant bit of d1
	roxr.b	#1,d7		; into d7 until it's full
	dbf	d6,.nextbit	; 
	move.b	d7,(a1)+	; store d7 and
	moveq	#0,d7		; start a fresh bit acumulator
	moveq	#7,d6		; 
.nextbit:
	dbf	d4,put_bits	; Do this d4+1 bits
	rts

;;; ----------------------------------------------------------------------
;;; Store d7.l into a1 (unaligned)
;;; 
;;; IN:  d7: longword value
;;;      a1: dst
;;; OUT: a1
longword_store:
	rol.l	#8,d7
	move.b	d7,(a1)+
	rol.l	#8,d7
	move.b	d7,(a1)+
	rol.l	#8,d7
	move.b	d7,(a1)+
	rol.l	#8,d7
	move.b	d7,(a1)+
	rts
