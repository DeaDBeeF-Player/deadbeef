;;; sc68 wrapper for JAM-TSD (Tao's ms30)
;;;
;;; by Benjamin Gerard
;;; 
;;; Time-stamp: <2011-09-12 15:39:40 ben>
;;; 


OFFSET:     =$ac
SelectOut:  =replay+$528-OFFSET
SetSTE:     =replay+$d76-OFFSET
SetTimer:   =replay+$dc0-OFFSET

OFFSET2:    =$be
OFFSET3:    =$c2
OFFSET4:    =$172

start:
  bra   init
  bra   kill
  bra   replay+$108c-OFFSET2
;  bra     truereplay+8

kill:
  moveq   #0,d0
  bra     truereplay+8

pipotable:
  dcb.b 1,0
  dcb.b 2,1
  dcb.b 3,2
  dcb.b 5,3
  dcb.b 8,4
  dcb.b 13,5
  dcb.b 21,6
  dcb.b 44,7
  dcb.b 65,8
  dc.b  109,9
  dc.b  100,10
  
patch:
  move.l  a0,a6


patch2:
  lea     .rout(pc),a0
  lea     pipotable(pc),a1
  move.l  a1,2(a0)
  lea     replay+$15f8-OFFSET4(pc),a1
  move.w  #$960a-$95f8-1,d0
.lp:
  move.b  (a0)+,(a1)+
  dbf     d0,.lp
  rts

.rout:
  move.l  #$12345678,a0
  move.b  #8,$FFFF8800.w
;  eor.b   #$80,d0
  move.b  0(a0,d0),$FFFF8802.w
.here:  
  dcb.w   (($960a-$95f8)-(.here-.rout))/2,$4e71
 

oneVoiceIn:
  dc.w  $00,$70,$e5,$163
  dc.w  $248,$320,$4cc,$6a5
  dc.w  $9a8,$d4b,$13d0,$1b1d
  dc.w  $28ba,$39ab,$5cf8,$8fb4

oneVoiceOut:
  ds.w  16
  dc.w  $80

makeout:
  movem.l d0-a6,-(a7)

; SCALE TABLE  
  lea     oneVoiceIn(pc),a0
  lea     oneVoiceOut(pc),a1
  moveq   #15,d7
 .lp1:
  move    (a0)+,d0
  mulu    #$7FFFFFFF/$8fb4,d0
  swap    d0
  lsr.w   #8,d0
  move    d0,(a1)+
  dbf     d7,.lp1
  
  lea     oneVoiceOut(pc),a5
  lea   replay+$1588-OFFSET3(pc),a6
  lea   $80*16(a6),a6
  
  moveq #0,d0     ; current vol
  moveq #0,d7     ; check counter
.lp2:
  movem.w (a5),d1-d2
  addq    #2,a5
  sub     d1,d2
  add     d2,d7

.lp3:
  subq.w  #1,d2
  bmi.s   .ok
  move.l  #$08000000,(a6)
  move.l  #$09000000,4(a6)
  move.l  #$0A000000,8(a6)
  move.b  d0,2(a6)
  lea     16(a6),a6
  bra.s   .lp3
.ok:
  addq    #1,d0
  
  cmp.w   #$80,d7
  blt.s   .lp2
  movem.l (a7)+,d0-a6
  rts

_song:
  dc.l  0
init:
  lea     _song(pc),a6
  move.l  a0,(a6)

  lea     replay(pc),a0
  bsr.s   load_prg
;  lea     truereplay+$64(pc),a0
;  move.w  patch(pc),(a0)+
;  move.l  #$4e714e71,(a0)
  
  moveq   #1,d0   ;replay type ? (0:STF)
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
  
;  bsr     makeout
  bsr   patch2
  
  rts

  INCLUDE "lib/TosReloc.s"
replay:
truereplay: = *+$4b8
  INCBIN  "org/jam/tsd.jam"
  ds.b    128*1024
  print  replay-start
  