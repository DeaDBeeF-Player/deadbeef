;;; sc68 wrapper for jam-hpn player (big demo)
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 14:55:13 ben>
;;; 

	
OFFSET:     =$be

start:
  bra   init
  bra   kill
  lea   replay+$578-OFFSET(pc),a0
  move.w  #$4e75,(a0)
  movem.l d0-a6,-(a7)
  bra   replay+$542-OFFSET+$16

kill:
  moveq   #0,d0
  ;bra     truereplay+8
  rts
  
patch:
  move.l  a0,a6

_song:
  dc.l  0
init:
  lea     _song(pc),a6
  move.l  a0,(a6)

  lea     replay(pc),a0
  bsr.s   load_prg
  
  moveq   #0,d0   ;replay type ? (0:STF)
  moveq   #2,d1
  bsr     replay
  
  moveq   #3,d1
  bsr     replay  ;pre-init
  
  move.l  _song(pc),a0
  moveq   #1,d0   ; song number
  moveq   #4,d1
  bsr     replay

;  moveq   #1,d1
;  bsr     replay  ; =>a0:get TSD struct
  
;  moveq   #5,d1
;  bsr     replay  ; <=a0: Get info
 
  moveq   #6,d1
  bsr     replay  ; Init STE and Timer
 
;  bsr     replay+$53c-OFFSET2 ; Init table and sample ?
;  bsr     SetSTE
;  bsr     SetTimer  ; ? modify sound table ?
  
;  bsr     truereplay
  
  rts

  include "lib/TosReloc.s"
replay:
truereplay: = *+$4b8
	INCBIN  "org/hpn.jam"	
  print  replay-start
