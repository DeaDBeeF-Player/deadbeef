;*****************************************************************************
;*   JAM   TaoSid-Plugin     Version 1.00 (+FASTRAM)          rev 21.04.2000 *
;*****************************************************************************
; sc68 modified version by Ben(jamin) Gerard
; Time-stamp: <2011-09-12 15:23:26 ben>
; Init-stamp: <2001-12-30 05:07:00 ben>

DMA_VALUE     = 0 ;[0:50066 1:25033 2:12516.5 3:6258.25]
DMA_FREQUENCY = 50066>>DMA_VALUE

        ;; ORG $8000
        
        bra   BEGIN
        rts
        rts
        bra   play

play:
        ;bsr     SID_IRQ  ;For time calculation.
        ;rts


        move.l  SID_MIX_PTR(pc),a0  ; a0 = current SID mix read pointer
        move.w  SID_MIX_CNT(pc),d7  ; d7 = Bytes in SID buffer
        bne.s   .writeDMA
.loop:        
        ; Check for data in the SID mix buffer
        
        ; No data : Run the player, fill SID buffer
        bsr     SID_IRQ
        lea     SID_MIX_BUF(pc),a4
        bsr     INIT_FILL
        lea     SID_MIX_BUF(pc),a0
        sub.l   a0,a4
        move.l  a4,d7 ; number of bytes
        lsr     #2,d7 ; number of sample
        
.writeDMA:
        bsr     write_dma
        tst.w   d7
        beq     .loop
        lea     SID_MIX_CNT(pc),a1
        move.w  d7,(a1)   ; Save SID mix buffer status
        move.l  a0,SID_MIX_PTR-SID_MIX_CNT(a1)
        rts

start_dma:
        lea     $ffff8900.w,a0
        lea     DMA_BUF(pc),a1
        move.l  a1,d0
        movep.l d0,1(a0)        ; DMA OFF, Set start address
        lea     DMA_BUF_END(pc),a1
        move.l  a1,d0
        movep.l d0,$0d(a0)
        
        ; Set MicroWire to MW only
        ;         7654321076543210
        move.w  #%1111111111100000,$24(a0)
        move.w  #%1000000001000000,$22(a0)
        
        move.b  #$80+(3-DMA_VALUE),$21(a0)  ; mono-??khz
        move.b  #3,$01(a0)                  ; DMA on, loop mode
        rts
        
; in :
;   a0    SID mix ptr
;   d7    number of sample in SID buffer        
;
; out :
;   a0    new SID mix ptr
;   d7    number of sample not mixer
write_dma:
        tst.w   d7
        beq.s   .rts
        
        bsr     LockDMA ; [a5,d5] [a6,d6]
        
        move    d5,d4   ; try to mix all the pass !
        add     d6,d4
        cmp     d7,d4
        blt.s   .rts
        
        cmp.w   d7,d5
        ble.s   .ok1
        move    d7,d5
.ok1:
        sub     d5,d7
        
        move    d5,d0
        move.l  a5,a1
        bsr     CopySidToDMA
        
        cmp.w   d7,d6
        ble.s   .ok2
        move    d7,d6
.ok2:
        sub     d6,d7
        
        move.w  d6,d0
        beq.s   .noSecondBuffer
        move.l  a6,a1
        bsr     CopySidToDMA
.noSecondBuffer:
        lea     DMA_BUF_END(pc),a5
        cmp.l   a5,a1
        blt.s   .not_end
        lea     DMA_BUF(pc),a1
.not_end        
        move.l  a1,DMA_PTR-DMA_BUF_END(a5)
.rts:        
        rts

; 
; out:
;   a5, d5.w (1st locked buffer)
;   a6, d6.w (2nd locked buffer)
LockDMA:
    bsr     get_cur_dma             ; d0,d1=current DMA  pointer
    
    lea     DMA_BUF(pc),a5
    cmp.l   a5,d1
    bhs.s   .ok_start
    move.l  a5,d1
    bra.s   .ok_end
.ok_start:
    lea     DMA_BUF_END(pc),a6
    cmp.l   a6,d1
    blo.s   .ok_end
    move.l  a5,d1
.ok_end:    
    
    move.l  DMA_PTR(pc),a5    ; a5=next write pointer (1st buffer)
    moveq   #0,d6
    move.l  d1,d5
    sub.l   a5,d5
    bpl.s   .one_buffer
    move.w  #DMA_BUF_END-DMA_BUF,d6 ; d6=DMA buffer size
    add.w   d5,d6
    lea     DMA_BUF_END(pc),a6
    move.l  a6,d5
    sub.l   a5,d5
    sub.w   d5,d6
    lea     DMA_BUF(pc),a6
.one_buffer:
    rts

sid2DMA:  macro
{
	move.b	(a0)+,d2
	add.b	(a0)+,d2
	add.b	(a0)+,d2
	addq	#1,a0
	move.b	d2,(a1)+
  
;   move.b  (a0)+,d2
;   ext.w   d2
;   move.b  (a0)+,d3
;   ext     d3
;   add.w   d3,d2
;   move.b  (a0)+,d3
;   ext     d3
;   add.w   d3,d2
;   addq    #1,a0
;   asr.w   #2,d2
;   move.b  d2,(a1)+
}

; a0 : SID buffer
; a1 : DMA buffer
; d0 : number of sample to copy
CopySidToDMA:
  moveq #7,d1
  and.w d0,d1
  subq  #8,d1
  neg   d1
  muls  #(.jmp-.lp1)/8,d1
  lsr.w #3,d0
  ;neg.w d1
  ;add   d1,d1
  ;move  d1,d2 ; x2
  ;lsl   #2,d1 ; x8
  ;add   d2,d1 ; x2+x8 = x10
  jmp   .lp1(pc,d1.w)
.lp1:
  RPT 8
  {
    sid2DMA
  }
.jmp:  
  dbf d0,.lp1
  rts  

get_cur_dma:
        lea   $ffff8900.w,a6
        moveq #-1,d0
.lp:        
        move.l  d0,d1       ; previous value
        movep.l $07(a6),d0  ; current pointer
        and.l #$ffffff,d0   ; 24 bit only
        cmp.l d0,d1
        bne.s .lp           ; Read twice the same value to validate it !
        rts

MACHINE         = 0
TEST            = 0

;*----------------------------------------------------------------------------*
;                >PART 'Plugin Architechtur'
;*******************************************************************
;* Cream's JAM Plug In Architechture
;*
;* version 1.2
;*
;* candyman/cream 1999
;*******************************************************************

PLUGINEXTINFO   = 1
PLUGINEXTINIT   = 2
PLUGINEXTACTIVATE = 3
PLUGINEXTSONGSELECT = 4
PLUGINEXTSONGINFO = 5
PLUGINEXTSONGPLAY = 6
PLUGINEXTSONGSTOP = 7
PLUGINEXTDEACTIVATE = 8
PLUGINEXTDEINIT = 9
PLUGINEXTNEXTSONGHOOK = 10
PLUGINEXTNEXTLOGHOOK = 11
LAST_PLUGIN_COMMAND = 11
                ENDPART
;-----------------------------------------------------------------------
BEGIN:
PRG:
                lea MUSIC_ADR(pc),a1
                move.l a0,(a1)

                MOVE.W  #PLUGINEXTINFO,D1
                BSR     START

                MOVE.W  #PLUGINEXTINIT,D1 ;0=st 1=ste 2=tt 3=megaste 4=falcon
                MOVE.W  #MACHINE,D0

                BSR     START

                MOVE.W  #PLUGINEXTACTIVATE,D1
                BSR     START

                MOVE.W  #PLUGINEXTSONGSELECT,D1
                MOVE.W  #1,D0
                move.l  MUSIC_ADR(pc),A0
                BSR     START
                
                MOVE.W  #PLUGINEXTSONGPLAY,D1
                BSR     START
                rts
MUSIC_ADR:  dc.l  0                

;-----------------------------------------------------------------------
START:
; main handler for the plug in
; the plugin interface is message oriented that
; means jam sends some messages and the plug in
; must serve them..
; the data movement can be in both ways
; replay ------  msg -----> plug in
;        ---(ptr&value)--->
; or
;
; replay ------  msg -----> plug in
;        <---(ptr&value)---
;
; a0 = void* ( data );
; d0 = long  ( value )
; d1 = long  ( msg )

                moveq  #$00,D2
                moveq  #$00,D3
                moveq  #$00,D4
                moveq  #$00,D5
                moveq  #$00,D6
                moveq  #$00,D7
                
                cmp.w   #LAST_PLUGIN_COMMAND,d1
                bgt.s  .table
                lsl   #2,d1
                jmp   .table(pc,d1.w)
.table:
                rts
                rts                                   ;0
                bra pluginextinfo_rout                ;1
                bra pluginextinit_rout                ;2
                bra pluginextactivate_rout            ;3
                bra pluginextsongselect_rout          ;4
                bra pluginextsonginfo_rout            ;5
                bra pluginextsongplay_rout            ;6
                bra pluginextsongstop_rout            ;7
                bra pluginextdeactivate_rout          ;8
                bra pluginextdeinit_rout              ;9
                bra pluginextnextsonghook_rout        ;10
                bra pluginextnextloghook_rout         ;11
                
;-----------------------------------------------------------------------
; msg = plugInExtInfo:
; in  = -
; out = a0 ptr to the plugininfo header
pluginextinfo_rout:
                LEA     PLUGININFO(pc),A0
                rts
;-----------------------------------------------------------------------
; msg = plugInExtInit:
; in  = - d0    0=st 1=ste 2=tt 3=megaste 4=falcon
; out = -
pluginextinit_rout:
                bsr     PREPARE_TSD
                rts
COMPTAB:        DC.W 0,1,2,1,3

;-----------------------------------------------------------------------
; msg = plugInExtActivate
; in  = -
; out = -
pluginextactivate_rout:
                lea     DMA_BUF(pc),a0
                lea     DMA_BUF_SZ/2(a0),a1
                move.l  a1,DMA_PTR-DMA_BUF(a0)
                MOVE.W  #DMA_BUF_SZ/2-1,D0
                MOVEQ   #0,D1
.clr:           MOVE.W  D1,(A0)+
                DBRA    D0,.clr
                rts
;-----------------------------------------------------------------------
; msg = plugInExtSongSelect
; in  = a0 pointer to the music data
;       IMPORTANT: u have to save this pointer !!!!
;       d0 which song
; out = -
pluginextsongselect_rout:
                lea     MUSIC_ADR(pc),a1
                MOVE.L  A0,(a1)
                BSR     INIT
                bsr     start_dma
                rts

;-----------------------------------------------------------------------
; msg = plugInExtSongInfo
; jam sends this message to get special informations about
; the current song. the message is send directly after
; a plugInExtSongSelect.
; u have to fill up the structure by using the given
; adress. if u can't give informations about something
; don't touch it... the fields are already filled
; with informations from the inf-file.
; ALL strings are 0 terminated !!!!
;
; in = a0 this is a internal structure pointer of jam so be careful
;      what u are doing ! because if u overwrite something
;      jam will crash !...
;      u can use the rs equs for this
;      to access the fields.do it like this
;      move.w           #1,SI_SONGCOUNT(a0) etc.
; out = -
pluginextsonginfo_rout:
                MOVE.W  #1,SI_SONGCOUNT(A0)
                MOVE.W  #1,SI_SONGPRESELECT(A0)

                LEA     SI_TITLE(A0),A2

                MOVEA.L MUSIC_ADR(pc),A1
                MOVE.W  #31,D0
COPY_SONGTITLE: MOVE.B  (A1)+,(A2)+
                DBRA    D0,COPY_SONGTITLE
                CLR.B   (A2)+

                LEA     SI_COMPOSER(A0),A2
                MOVEA.L MUSIC_ADR(pc),A1
                ADDA.L  #$20,A1
                MOVE.W  #31,D0
COPY_COMPOSER:  MOVE.B  (A1)+,(A2)+
                DBRA    D0,COPY_COMPOSER
                CLR.B   (A2)+

                rts

;-----------------------------------------------------------------------
; msg = plugInExtSongPlay
; should be clear i guess..but one important thing
; u have to do a irq-rout for your own that means
; jam don't handle irq stuff...!!!!
; please do not use vbl or vbl query...there are some
; guys which are using vga ;) btw and to use a normal
; vbl vector is a bit unclean....so use a timer !!!
; in  = -
; out = -
pluginextsongplay_rout:
                BSR     INIT_SID
                rts

;-----------------------------------------------------------------------
; msg = plugInExtSongStop
; should be clear too...u have to remove your
; entire timer stuff when this function is called...
; and plz reset the matrix and switch off volumes etc.
; in  = -
; out = -
pluginextsongstop_rout:
                bsr     END_TSD
                
;-----------------------------------------------------------------------
; msg = plugInExtSongDeactivate
; in  = -
; out = -
pluginextdeactivate_rout:
;-----------------------------------------------------------------------
; msg = plugInExtDeinit
; in  = -
; out = -
pluginextdeinit_rout:
;-----------------------------------------------------------------------
; msg = plugInExtNextSongHook
; this is needed to implement the playlist.
; if the plugin recognizes the song reaches the
; end it should do a call to the given funktion
; to tell jam to select the next song
; from the playlist.
; in  = a0 hookfunktion...do in your plugin a jsr
;       to is if the song is finished and jam
;       will select the next song
; out = -
pluginextnextsonghook_rout:

;-----------------------------------------------------------------------
; msg = plugInExtNextLogHook
; in  = a0 logfunktion...
; out = -
pluginextnextloghook_rout:
                RTS
;-----------------------------------------------------------------------
;be careful with the lengthes of the strings
;for example turboass don't show an error if the
;string is too long...it only shows in
;the status line something like "turns to negative" or so

PLUGININFO:
INTERFACEVERSION:DC.W $0100
VERSION:        DC.W $0100              ; of the plug in

DATE:           DC.B "21.04.2000"
DATEE:
                DS.B 12-(DATEE-DATE)

FILEEXT:        DC.B ".TSD"
FILEEXTE:
                DS.B 6-(FILEEXTE-FILEEXT)

FILEEXT2:       DC.B " "
FILEEXTE2:
                DS.B 6-(FILEEXTE2-FILEEXT2)

FILEEXT3:       DC.B " "
FILEEXTE3:
                DS.B 6-(FILEEXTE3-FILEEXT3)

FILEEXT4:       DC.B " "
FILEEXTE4:
                DS.B 6-(FILEEXTE4-FILEEXT4)

FILEEXT5:       DC.B " "
FILEEXTE5:
                DS.B 6-(FILEEXTE5-FILEEXT5)

FILEEXT6:       DC.B " "
FILEEXTE6:
                DS.B 6-(FILEEXTE6-FILEEXT6)

FILEEXT7:       DC.B " "
FILEEXTE7:
                DS.B 6-(FILEEXTE7-FILEEXT7)

FILEEXT8:       DC.B " "
FILEEXTE8:
                DS.B 6-(FILEEXTE8-FILEEXT8)

NAME:           DC.B "MS 3.0 (TSD)"
NAMEE:
                DS.B 128-(NAMEE-NAME)

CODER:          DC.B "Tao"
CODERE:
                DS.B 128-(CODERE-CODER)

EMAIL:          DC.B "mail@creamhq.de"
EMAILE:
                DS.B 128-(EMAILE-EMAIL)

WWW:            DC.B "www.creamhq.de"
WWWE:
                DS.B 128-(WWWE-WWW)

COMMENT:        DC.B " "
COMMENTE:
                DS.B 128-(COMMENTE-COMMENT)
;********************************
;1 = plugin uses dsp
ISDSP:          DC.W 0
;********************************
;tells the jam for which
;machines is the plugin
;this is a bit field
;0 = st
;1 = ste
;2 = tt030
;3 = mega ste
;4 = falcon
SUPPORT:        DC.W %11110
;********************************
; this is the last check before jam
; starts to play the song.
; most of the songs have a 4 byte ascii identification
; mark.
; if it don't match the replay kicks the song
; 0 mean no identification mark...
DATASTART:      DC.L 0
;********************************
; this is to tell the jam that
; the plugin handles the end of the song
; otherwise jam has to handle it
; via replaytime.
; 1 = true
; 0 = false
SUPPORTSNEXTSONGHOOK:DC.W 0
;********************************
; this is to tell the replayer that
; the plugin handles the songinfo for the name of the song
; otherwise the replayer has to handle it
; via inf-file.
; 1 = true
; 0 = false
SUPPORTSNAME:   DC.W 0
;********************************
; this is to tell the replayer that
; the plugin handles the songinfo for the composer of the song
; otherwise the replayer has to handle it
; via inf-file.
; 1 = true
; 0 = false
SUPPORTSCOMPOSER:DC.W 0
;********************************
;* song count support

SUPPORTSSONGCOUNT:DC.W 1
;********************************
;* song pre select support
SUPPORTSPRESELECT:DC.W 1
;********************************
;* song comment support
SUPPORTSCOMMENT:DC.W 0
;********************************
;* tells jam if the plugin is fast
;* ram compatible... 1 = true 0 = false
FASTRAM:        DC.W 1
;-----------------------------------------------------------------------
;songinfo structure
                RSRESET
SI_TITLE:       RS.B 256
SI_COMPOSER:    RS.B 256
SI_RIPPER:      RS.B 256
SI_EMAIL:       RS.B 256
SI_WWW:         RS.B 256
SI_COMMENTS:    RS.B 256
SI_SONGHZ:      RS.W 1
SI_SONGCOUNT:   RS.W 1
SI_SONGPRESELECT:RS.W 1
SI_ISYMSONG:    RS.W 1
SI_DMAHZ:       RS.L 1
SI_FILENAME:    RS.W 256
SI_PLAYTIME_MIN:RS.W 99
SI_PLAYTIME_SEC:RS.W 99

;*----------------------------------------------------------------------------*
;COMPUTER:       DC.W 1
;0=ST 1=STE 3=Falcon

PREPARE_TSD:
                BSR     PREPARE_WAVES
                move.l  #$0700ff00,$ffff8800.w
                move.l  #$0800000,$ffff8800.w
                move.l  #$0900000,$ffff8800.w
                move.l  #$0A00000,$ffff8800.w
                rts

END_TSD:
                CLR.B   $FFFF8901.w
                rts
                
;******************************************************************************
;*               Magic Synth 3.00 replay routine, written by TAO of CREAM     *
;******************************************************************************

BASE:           BRA     INIT
                BRA     MUSICIRQ        ;IRQ OHNE REGISTER-RETTEN
                BRA     MUSICIRQ        ;IRQ OHNE REGISTER-RETTEN
                DC.W 0
                DC.W 0                  ;FADESTATUS: $0001 FADEOUT FINISHED

                DC.B "** MS 3.00 *****"
                DC.B " Tao's  DMA SID "
                DC.B "****************"
                EVEN

INIT:
                MOVEM.L D0-A6,-(A7)
                LEA     BASE(PC),A4
                TST.L   D0
                BNE.S   INITTUNE
                MOVE.W  #$01,STOPF-BASE(A4)
                CLR.B   STIMME1+$0E-BASE(A4)
                CLR.B   STIMME2+$0E-BASE(A4)
                CLR.B   STIMME3+$0E-BASE(A4)
                MOVEM.L (A7)+,D0-A6
                RTS

INITTUNE:
                MOVEA.L MUSIC_ADR(pc),A6
                ADDA.L  #64,A6
                MOVEA.L A6,A0

                BSR     CONVERT_MUSIC

                MOVE.W  $0C(A6),SPEED-BASE(A4)
                MOVE.L  $04(A6),INSBASE-BASE(A4)

                LEA     STIMME1(PC),A0
                MOVEQ   #2,D0
                CLR.L   D2
INITLOOP:
                MOVEA.L A0,A3
                MOVE.W  #$4A/4-1,D1
CLR_FIELD:      CLR.L   (A3)+
                DBRA    D1,CLR_FIELD

                MOVE.W  #$0101,$2C(A0)
                MOVE.B  15(A6),$32(A0)
                MOVE.B  15(A6),$33(A0)

                LEA     W0(PC),A3
                MOVE.L  A3,$08(A0)
                LEA     ARP0(PC),A3
                MOVE.L  A3,$14(A0)
                MOVE.L  A3,$18(A0)

                MOVEA.L A6,A5
                ADDA.L  D2,A5
                MOVEA.L $20(A5),A3      ;Get Pointertable
                MOVE.L  (A3),$10(A0)    ;actual Pattern
                MOVE.B  $05(A3),$0F(A0) ;Transpose
                ADDQ.L  #6,A3
                MOVE.L  A3,(A0)         ;Pointertable

                ADDQ.L  #$04,D2
                ADDA.L  #$4A,A0
                DBRA    D0,INITLOOP

                MOVE.W  16(A6),D1   ; player irq freq (MFP parm ?) 4->200hz
                                    ; 
                MOVE.L  #(2000<<1)>>DMA_VALUE,D0    ; ????
                DIVU    D1,D0
                AND.L   #$FFFF,D0
                LSL.L   #2,D0
                LSR.L   #4,D0
                SUBQ.L  #1,D0
                MOVE.L  D0,PUFFER_LEN-BASE(A4)
                CLR.W   STOPF-BASE(A4)

ENDINIT:        MOVEM.L (A7)+,D0-A6
                RTS

;-----------------------------------------------------------------------
MUSICIRQ:
                LEA     BASE(PC),A4
                TST.W   STOPF-BASE(A4)
                BNE.S   ALLOFF

                SUBQ.B  #$01,SPEED-BASE(A4)
                BNE.S   PATTSPD
                MOVE.B  SPEED+1-BASE(A4),SPEED-BASE(A4)

                LEA     STIMME1(PC),A0
                BSR.S   WORK_PATTERN
                LEA     STIMME2(PC),A0
                BSR.S   WORK_PATTERN
                LEA     STIMME3(PC),A0
                BSR.S   WORK_PATTERN

PATTSPD:        LEA     FREQTAB(PC),A5
                MOVEQ   #$00,D7
                LEA     STIMME1(PC),A0
                BSR     EFFECTS
                LEA     STIMME2(PC),A0
                BSR     EFFECTS
                LEA     STIMME3(PC),A0
                BSR     EFFECTS
                RTS

ALLOFF:         MOVE.W  #1,$10(A4)
ENDARB:         RTS

;-----------------------------------------------------------------------
WORK_PATTERN:

                SUBQ.B  #$01,$2C(A0)
                BNE     NOINST
                MOVE.B  $2D(A0),$2C(A0)

                MOVE.B  D7,$35(A0)

PATT2:          MOVEA.L $10(A0),A1      ;PATTERN AUSLESEN
                MOVE.B  (A1)+,D1
                MOVE.B  (A1)+,D0
                MOVE.L  A1,$10(A0)

                MOVE.W  -2(A1),D3
                BEQ     NOINST
                BPL.S   TSTINST

;-----------------------------------------------
NOEND:          AND.W   #$07,D1         ;FX-NUMBER
                ADD.W   D1,D1
                JMP     EFFJUMP(PC,D1.w)

EFFJUMP:        BRA.S   EFF0
                BRA.S   EFF1
                BRA.S   EFF2
                BRA.S   EFF3
                BRA.S   PATBRK
                
EFF5:           MOVE.B  D0,$2B(A0)      ;05 -> Buzzer Finetuning
                BRA.S   PATT2
;------------------------------------------------
PATBRK:         MOVEA.L (A0),A1         ;04 -> PATTERN-BREAK
                MOVE.L  (A1),$10(A0)
                BGE.S   NO_PAT_END
                MOVEA.L $04(A1),A1
                MOVE.L  (A1),$10(A0)

NO_PAT_END:     MOVE.B  $05(A1),$0F(A0)
                LEA     $06(A1),A1
                MOVE.L  A1,(A0)
                BRA.S   PATT2
;------------------------------------------------
EFF3:           MOVE.B  D0,SPEED-BASE(A4) ;03 -> SPEED
                MOVE.B  D0,SPEED+1-BASE(A4)
                BRA.S   PATT2
;------------------------------------------------
EFF2:           MOVE.B  D0,$34(A0)      ;02   -> PORTAMENTO
                MOVE.B  #$01,$35(A0)
                BRA.S   PATT2
;------------------------------------------------
EFF1:           MOVEQ   #$0F,D2         ;01   -> VOLUME
                SUB.B   D0,D2
                MOVE.B  D2,$0D(A0)
                BRA.S   PATT2
;------------------------------------------------
EFF0:           MOVE.B  D0,$2C(A0)      ;00   -> KEY LENGTH
                MOVE.B  D0,$2D(A0)
                BRA.S   PATT2
;------------------------------------------------
TSTINST:
                MOVE.L  D7,$34(A0)

                TST.B   D1              ;Note = 0 ?
                BNE.S   OK_INS
                LEA     W0(PC),A2
                MOVE.L  A2,$08(A0)
                RTS

OK_INS:         MOVE.B  $0F(A0),$0C(A0)
                ADD.B   D1,$0C(A0)
                MOVE.W  $1C(A0),$40(A0)

;-------------- Initialize Instrument -------------

                AND.L   #$3F,D0
                BEQ.S   NOINST

                LSL.W   #5,D0
                MOVEA.L INSBASE(PC),A2
                ADDA.L  D0,A2

REPLAY_INS:     MOVE.L  D7,$38(A0)      ;38/39/3a/(3b)
                MOVE.L  (A2)+,$08(A0)   ;ADSR-BASIS
                MOVE.L  (A2),$14(A0)    ;ARPEGGIO-FX-BASIS
                MOVE.L  (A2)+,$18(A0)
                MOVE.L  (A2)+,$20(A0)   ;20/21/22/23
                MOVE.L  (A2)+,$24(A0)   ;24/25/26/27
                MOVE.L  (A2)+,$2E(A0)   ;2E/2F/30/31
                MOVE.W  (A2)+,$44(A0)   ;44/45
NOINST:         RTS

SETNIC:         MOVE.W  #$01,STOPF-BASE(A4)
                RTS

;-----------------------------------------------------------------------
EFFECTS:
                CLR.W   D0
                CLR.W   D3

;-------------- Volume ---------------------------

                MOVEA.L $08(A0),A1
                MOVE.B  (A1)+,D0
                BPL.S   OK_ADSR

                SUBQ.L  #2,A1
                MOVE.B  (A1),D0

OK_ADSR:        MOVE.L  A1,$08(A0)
                MOVE.B  D0,D1
                AND.W   #$0F,D0
                AND.W   #$F0,D1
                LSR.W   #4,D1
                MOVE.B  D1,$28(A0)

                SUB.B   $0D(A0),D0
                BPL.S   VOLOK
                SUB.W   D0,D0
VOLOK:          MOVE.B  D0,$0E(A0)

;-------------- FX - Arpeggio ------------------------

                MOVEA.L $14(A0),A2
FX_ROUT:        MOVE.B  (A2)+,D1
                MOVE.B  (A2)+,D0

                CMP.B   #$0E,D1
                BNE.S   NO_FX_LOOP

                MOVEA.L $18(A0),A2      ; 0E Jump
                ADD.W   D0,D0
                AND.L   #$FFFF,D0
                ADDA.L  D0,A2
                BRA.S   FX_ROUT

NO_FX_LOOP:     CMP.B   #$0F,D1
                BNE.S   END_FX

FX_ENDE:        SUBQ.L  #$04,A2         ; 0F End
                BRA.S   FX_ROUT

END_FX:         MOVE.L  A2,$14(A0)
                LSL.W   #4,D1
                AND.W   #$F0,D1
                OR.B    $28(A0),D1
                MOVE.B  D1,$28(A0)
                BTST    D7,D1
                BNE.S   NO_TSP
                ADD.B   $0C(A0),D0
NO_TSP:

;-------------- Auslesen der Frequenz -------------                                                                                              der frequenz --------

READFREQ:       AND.W   #$7F,D0
                MOVE.W  D0,D3
                ADD.W   D0,D0
                MOVE.W  0(A5,D0.w),D0

;-------------- Vibrato ---------------------------

                SUBQ.B  #1,$32(A0)
                BEQ.S   VIBRATO

                MOVE.W  $36(A0),D2
                LSR.W   #2,D2
                ADD.W   D2,D0
                ADD.W   $2A(A0),D0
                MOVE.W  D0,$1C(A0)
                RTS

VIBRATO:        MOVE.B  $33(A0),$32(A0)

                SUBQ.B  #$01,$22(A0)
                BPL.S   NODELVIB
                MOVE.B  $23(A0),$20(A0)
                MOVE.B  $24(A0),$21(A0)
                CLR.B   $22(A0)
                MOVE.B  #$01,$3A(A0)

NODELVIB:       SUB.W   D1,D1
                MOVE.B  $21(A0),D2
                MOVE.B  $38(A0),D1

                TST.B   $39(A0)
                BEQ.S   HOCHVIB

                SUB.B   $20(A0),D1
                BCC.S   WRITEB
                CLR.B   $39(A0)
                SUB.W   D1,D1
                BRA.S   WRITEB

HOCHVIB:        ADD.B   $20(A0),D1
                CMP.B   D2,D1
                BCS.S   WRITEB
                MOVE.B  #$01,$39(A0)
                MOVE.B  D2,D1

WRITEB:         MOVE.B  D1,$38(A0)

SECONDT:        LSR.B   #1,D2
                SUB.B   D2,D1
                BCC.S   ADDIERE
                SUB.W   #$0100,D1
ADDIERE:        LSR.W   #1,D1
                MOVE.W  D1,$2A(A0)
                ADD.W   D1,D0

;-------------- Portamento ----------------------

                MOVE.B  $34(A0),D1
                BEQ.S   SAVE_PERIOD
                MOVE.W  $36(A0),D2
                EXT.W   D1
                SUB.W   D1,D2
                MOVE.W  D2,$36(A0)
                LSR.W   #2,D2
                ADD.W   D2,D0
SAVE_PERIOD:    MOVE.W  D0,$1C(A0)

;-------------- Pulse Vibration ------------------

                TST.B   $25(A0)
                BMI.S   DO_PULSE
                BEQ.S   INIT_PULSE
                SUBQ.B  #$01,$25(A0)
                RTS
INIT_PULSE:
                MOVE.B  $45(A0),$26(A0)
                MOVE.B  #$FF,$25(A0)
DO_PULSE:       SUB.W   D1,D1
                SUB.W   D2,D2
                MOVE.B  $30(A0),D1      ;start (44)
                MOVE.B  $31(A0),D2      ;bound (45)
                CMP.W   D1,D2
                BEQ.S   FADE_ROUT

                MOVE.W  $2E(A0),D0      ; speed

                TST.B   $44(A0)
                BEQ.S   COUNT_UP

                SUB.W   D0,$26(A0)
                MOVE.B  $26(A0),D3
                AND.W   #$FF,D3
                CMP.W   D1,D3
                BGE.S   FADE_ROUT
URGENT:         CLR.B   $44(A0)
                MOVE.B  D1,$26(A0)
                MOVE.B  D7,$27(A0)
                BRA.S   FADE_ROUT

COUNT_UP:       ADD.W   D0,$26(A0)
                MOVE.B  $26(A0),D3
                AND.W   #$FF,D3
                CMP.W   D2,D3
                BLE.S   FADE_ROUT
                MOVE.B  #1,$44(A0)
                MOVE.B  D2,$26(A0)
                CLR.B   $27(A0)
;--------------------------------------------
FADE_ROUT:      RTS

;-----------------------------------------------------------------------
CONVERT_MUSIC:
                CMPI.L  #"MS30",(A6)
                BEQ.S   CONVERT
                RTS

CONVERT:        MOVE.L  #"DONE",(A6)
                MOVE.L  A6,D0

                ADD.L   D0,$04(A6)
                ADD.L   D0,$20(A6)
                ADD.L   D0,$24(A6)
                ADD.L   D0,$28(A6)

                MOVEA.L $20(A6),A1

                MOVE.W  #2,D1
CON_LOOP_1:     CMPI.L  #$FFFFFFFF,(A1)
                BEQ.S   END_POINT_1
                ADD.L   D0,(A1)
                ADDQ.L  #6,A1
                BRA.S   CON_LOOP_1

END_POINT_1:    ADDQ.L  #4,A1
                ADD.L   D0,(A1)+
                DBRA    D1,CON_LOOP_1

                MOVEA.L $04(A6),A1
                ADDA.L  #$20,A1
                MOVE.L  $08(A6),D1
CON_INS_LOOP:   ADD.L   D0,(A1)
                ADD.L   D0,$04(A1)
                ADDA.L  #$20,A1
                DBRA    D1,CON_INS_LOOP
                RTS

;-----------------------------------------------------------------------
TABS:
FREQTAB:
                DC.W $0EEE*2,$0E17*2,$0D4D*2,$0C8E*2,$0BD9*2,$0B2F*2,$0A8E*2,$09F7*2
                DC.W $0967*2,$08E0*2,$0861*2,$07E8*2
                DC.W $0EEE,$0E17,$0D4D,$0C8E,$0BD9,$0B2F,$0A8E,$09F7
                DC.W $0967,$08E0,$0861,$07E8,$0777,$070B,$06A6,$0647
                DC.W $05EC,$0597,$0547,$04FB,$04B3,$0470,$0430,$03F4
                DC.W $03BB,$0385,$0353,$0323,$02F6,$02CB,$02A3,$027D
                DC.W $0259,$0238,$0218,$01FA,$01DD,$01C2,$01A9,$0191
                DC.W $017B,$0165,$0151,$013E,$012C,$011C,$010C,$FD
                DC.W $EE,$E1,$D4,$C8,$BD,$B2,$A8,$9F
                DC.W $96,$8E,$86,$7E,$77,$70,$6A,$64
                DC.W $5E,$59,$54,$4F,$4B,$47,$43,$3F
                DC.W $3B,$38,$35,$32,$2F,$2C,$2A,$27
                DC.W $25,$23,$21,$1F,$1D,$1C,$1A,$19
                DC.W $17,$16,$15,$13,$12,$11,$10,$0F
                DC.W $0E,$0E,$0D,$0C,$0B,$0B,$0A,$09
                DC.W $09,$08,$08,$07,$07,$07,$06,$06
                DC.W $05,$05,$05,$04,$04,$04,$04,$03
                DC.W $03,$03,$03,$03,$02,$02,$02,$02
                DC.W $02,$02,$02,$01,$01,$01,$01,$01
                DC.W $01,$01,$01,$01,$01,$01,$01,$00
                EVEN

SPEED:          DC.W 0
STOPF:           DC.W 1
INSBASE:        DC.L 0                  ;128 samples
;------------------------------------------------------------------------------------------
STIMME1:
                DC.L 0                  ; $00 Patterntable Basis
                DC.L 0                  ; $04 Patterntable Offset
                DC.L 0                  ; $08 Volume-Table Basis
                DC.B 0                  ; $0c Actual Note
                DC.B 0                  ; $0d Volume of Instrument
                DC.B 0                  ; $0e Volume
                DC.B 0                  ; $0f Transpose
                DC.L 0                  ; $10 Patternposition
                DC.L 0                  ; $14 Arpeggio-FX-Position
                DC.L 0                  ; $18 Arpeggio-FX-Basis
                DC.B 0                  ; $1c YM-Period Hi
                DC.B 0                  ; $1d YM-Period Low
                DC.W 0                  ; $1e Vibrato-Offset
                DC.B 0                  ; $20 Vibrato Depth
                DC.B 0                  ; $21 Vobrato Speed
                DC.B 0                  ; $22 Vibrato Delay
                DC.B 0                  ; $23 Vibrato Depth 2
                DC.B 0                  ; $24 Vibrato Speed 2
                DC.B 0                  ; $25 pulse delay
                DC.B 0                  ; $26 pulse width hi
                DC.B 0                  ; $27 pilse width lo
                DC.B 0                  ; $28 SID Status (0-4)
                DC.B 0                  ; $29 Channel-Status
                DC.B 0                  ; $2a period save
                DC.B 0                  ; $2b period save
                DC.B 0                  ; $2c note length
                DC.B 0                  ; $2d note length
                DC.B 0                  ; $2e pulse speed
                DC.B 0                  ; $2f pulse speed
                DC.B 0                  ; $30 pulse start
                DC.B 0                  ; $31 pulse start
                DC.B 0                  ; $32 t_verz
                DC.B 0                  ; $33 t_verz
                DC.B 0                  ; $34 Portamento Speed
                DC.B 0                  ; $35 Portamento Flag
                DC.W 0                  ; $36 Portamento Offset
                DC.B 0                  ; $38 Vib data
                DC.B 0                  ; $39 Vib data
                DC.B 0                  ; $3a Vib data
                DC.B 0                  ; $3b
                DC.B 0                  ; $3c Fade Speed
                DC.B 0                  ; $3d Fade Speed
                DC.B 0                  ; $3e Fade Position
                DC.B 0                  ; $3f Fade Status
                DC.W 0                  ; $40 Last Frequency
                DC.B 0                  ; $42 Pattern Count
                DC.B 0                  ; $43
                DC.B 0                  ; $44 pulse direction
                DC.B 0                  ; $45 pulse hold
                DC.B 0                  ; $46
                DC.B 0                  ; $47
                DC.B 0                  ; $48
                DC.B 0                  ; $49
                EVEN

STIMME2:        DS.B $4A
STIMME3:        DS.B $4A

;------------------------------------

W0:             DC.B $00,$FF
ARP0:           DC.W $00,$0F00
                EVEN

;-----------------------------------------------------------------------
PREPARE_WAVES:
                LEA     _W_SAWTOOTH(PC),A1
                add.l   (a1),a1
                LEA     $0F00(A1),A0
                MOVE.W  #255,D3
                BSR     CALC_WAVES

                LEA     _W_TRIANGLE(PC),A1
                add.l   (a1),a1
                LEA     $0F00(A1),A0
                MOVE.W  #255,D3
                BSR     CALC_WAVES

                LEA     _W_PULSE(PC),A1
                add.l   (a1),a1
                LEA     $1E00(A1),A0
                MOVE.W  #511,D3
                BSR     CALC_WAVES

                LEA     _W_NOISE(PC),A1
                add.l   (a1),a1
                move.l  a1,a0
                add.l   #$F000,A0
                MOVE.W  #4095,D3
                BSR     CALC_WAVES

                LEA     _W_PULSE(PC),A0
                add.l   (a0),a0
                LEA     _W_TRIANGLE(PC),A1
                add.l   (a1),a1
                LEA     _PULSE_TRIANGLE(PC),A2
                add.l   (a2),a2
                BSR.S   MIX_PULSE

                LEA     _W_PULSE(PC),A0
                add.l   (a0),a0
                LEA     _W_SAWTOOTH(PC),A1
                add.l   (a1),a1
                LEA     _PULSE_SAWTOOTH(PC),A2
                add.l   (a2),a2
                BSR.S   MIX_PULSE

                LEA     _W_SAWTOOTH(PC),A0
                add.l   (a0),a0
                LEA     _W_TRIANGLE(PC),A1
                add.l   (a1),a1
                LEA     _TRIANGLE_SAWTOOTH(PC),A2
                add.l   (a2),a2
                BSR.S   MIX_NORMAL

                RTS

MIX_NORMAL:     MOVE.W  #15,D3
PT_LOOP4:       MOVE.W  #255,D2
PT_LOOP3:
                MOVE.B  (A0)+,D0
                OR.B    (A1)+,D0
                MOVE.B  D0,(A2)+

                DBRA    D2,PT_LOOP3
                DBRA    D3,PT_LOOP4
                RTS


MIX_PULSE:      MOVE.W  #15,D3
PT_LOOP2:       MOVE.W  #255,D2
PT_LOOP:
                MOVE.B  (A0),D0
                OR.B    (A1),D0
                MOVE.B  D0,(A2)

                MOVE.B  $0100(A0),D0
                OR.B    (A1),D0
                MOVE.B  D0,$0100(A2)

                LEA     $01(A0),A0
                LEA     $01(A1),A1
                LEA     $01(A2),A2
                DBRA    D2,PT_LOOP
                ADDA.L  #$0100,A0
                ADDA.L  #$0100,A2
                DBRA    D3,PT_LOOP2
                RTS


CALC_WAVES:     MOVEQ   #0,D0           ;volume

                MOVE.W  #15,D2  ;$$$
WAVELOOP2:
                MOVEA.L A0,A2
                MOVE.W  D3,D4

WAVELOOP:       MOVE.B  (A2)+,D1; Read sample
                EXT.W   D1	; Get it signed
                MULS    D0,D1	; Multiply by volume factor (0...15)
                divs    #16*3,d1; Divide by 3 for fast mixing
                MOVE.B  D1,(A1)+
                DBRA    D4,WAVELOOP

                ADDQ.W  #1,D0
                DBRA    D2,WAVELOOP2
                RTS

	if 0
	{
	
CONVERT_TABLES:
;LEA     FREQUENCIES,A0
;MOVE.L  #$0FFF,D2
;CMPI.W  #2,COMPUTER
;BGE.S   F_CON
CONVERT_FRQS:
;MOVE.L  (A0),D0
;LSL.L   #1,D0
;MOVE.L  D0,(A0)+
;DBRA    D2,CONVERT_FRQS
;BRA.S   GO_ON_CON

F_CON:
;MOVE.L  (A0),D0
;LSR.L   #1,D0
;MOVE.L  D0,(A0)+
;DBRA    D2,F_CON


GO_ON_CON:      LEA     _NOISE(PC),A0
                MOVE.W  #4095,D0
                bsr.s   CON_ANY

                LEA     _SAWTOOTH(PC),A0
                MOVE.W  #255,D0
                bsr.s   CON_ANY

                LEA     _TRIANGLE(PC),A0
                MOVE.W  #255,D0
                bsr.s   CON_ANY

                LEA     _PULSE(PC),A0
                MOVE.W  #511,D0
                bsr.s   CON_ANY

                LEA     _PULSE_TRIANGLE(PC),A0
                MOVE.W  #511,D0
                bsr.s   CON_ANY

                RTS

CON_ANY:
                add.l   (a0),a0
.lp:                
                MOVE.B  (A0),D1
		asr.b   #1,d1
                MOVE.B  D1,(A0)+
                dbf     d0,.lp
                rts
	}
	
;****************************************************************************                                                                          *
;*               3 Channel-SID-Emulator                                     *
;*               Written by TAO of CREAM                   ST Rev. 07.01.98 *
;****************************************************************************                                                                          *

INIT_SID:       ;>PART 'Initialize SID'
;*---------------------------------------------------------------------------*

INIT_A:         LEA     NO_NOISE(PC),A0
                LEA     SAMPLE_1(PC),A1
                MOVE.L  A0,(A1)
                LEA     SAMPLE_2(PC),A1
                MOVE.L  A0,(A1)
                LEA     SAMPLE_3(PC),A1
                MOVE.L  A0,(A1)
                
                lea     SID_MIX_CNT(PC),A1
                clr.w   (a1)

SID_IRQ:
                BSR     MUSICIRQ

VOICE_1:        LEA     STIMME1(PC),A0
                MOVE.B  $0E(A0),D0      ;05
                MOVE.W  D0,D1
                LEA     NOISE_LEN1(PC),A4
                BTST    #3,$28(A0)      ;04
                BEQ.S   DO_VOICE1
                CLR.W   D1
DO_VOICE1:      BSR.S   GET_WAVE
                MOVE.L  A1,SAMPLE_1-STIMME1(A0)
;----------------------------------------------------
VOICE_2:        LEA     STIMME2(PC),A0
                MOVE.B  $0E(A0),D0
                MOVE.W  D0,D1
                LEA     NOISE_LEN2(PC),A4
                BTST    #3,$28(A0)
                BEQ.S   DO_VOICE2
                CLR.W   D1
DO_VOICE2:      BSR.S   GET_WAVE
                MOVE.L  A1,SAMPLE_2-STIMME2(A0)
;----------------------------------------------------
VOICE_3:        LEA     STIMME3(PC),A0
                MOVE.B  $0E(A0),D0
                MOVE.W  D0,D1
                LEA     NOISE_LEN3(PC),A4
                BTST    #3,$28(A0)
                BEQ.S   DO_VOICE3
                CLR.W   D1
DO_VOICE3:      BSR.S   GET_WAVE
                MOVE.L  A1,SAMPLE_3-STIMME3(A0)
                RTS
;----------------------------------------------------

GET_WAVE:
                MOVE.W  #$FF,(A4)
                MOVE.B  $28(A0),D0      ;04
                BTST    #$02,D0
                BEQ.S   NO_MODULATION
                BTST    #$04,D0
                BNE.S   MODULATION
                BCLR    #$02,$28(A0)    ;04
                BRA.S   NO_MODULATION

MODULATION:     LEA     _W_TRIANGLE(PC),A1
                add.l   (a1),a1
                MOVE.W  D1,D0
                ANDI.L  #$0F,D0
                LSL.L   #8,D0
                ADDA.L  D0,A1
                RTS

NO_MODULATION:  LSR.W   #2,D0
                AND.W   #$3C,D0
                lea     WAVES(PC),a1
                add.L   0(a1,D0.w),A1
                TST.W   D0
                BEQ.S   NOPULS2
                BTST    #$06,$28(A0)    ;04
                BEQ.S   MAKE_VOLUME
                MOVE.B  $26(A0),D0      ;02
                ANDI.L  #$FF,D0
                ADDA.L  D0,A1
MAKE_VOLUME:
                MOVE.W  D1,D0
                ANDI.L  #$0F,D0
                LSL.L   #8,D0
                BTST    #6,$28(A0)      ;04
                BEQ.S   NOPULSE1
                ADD.L   D0,D0
                ADDA.L  D0,A1
                RTS

NOPULSE1:       BTST    #7,$28(A0)      ;04
                BEQ.S   NOPULS1
                LSL.L   #4,D0
                MOVE.W  #$0FFF,(A4)
NOPULS1:        ADDA.L  D0,A1
NOPULS2:        RTS

WAVES:          DC.L NO_NOISE-WAVES,W_TRIANGLE-WAVES,W_SAWTOOTH-WAVES,TRIANGLE_SAWTOOTH-WAVES
                DC.L W_PULSE-WAVES,PULSE_TRIANGLE-WAVES,PULSE_SAWTOOTH-WAVES,W_PULSE-WAVES
                DC.L W_NOISE-WAVES,W_NOISE-WAVES,W_NOISE-WAVES,W_NOISE-WAVES,W_NOISE-WAVES,W_NOISE-WAVES
                DC.L W_NOISE-WAVES,W_NOISE-WAVES

; In:
;   a4    destination buffer
;
INIT_FILL:
;*---------------------------------------------------------------------------*
                LEA     BASE(PC),A6

                MOVEA.L SAMPLE_1(PC),A0
                MOVEA.L SAMPLE_2(PC),A1
                MOVEA.L SAMPLE_3(PC),A2

                MOVEQ   #$00,D0
                MOVE.W  STIMME1+$1C(pc),D0
                BSR     CALC_FREQ
                MOVE.L  D0,D6
                
                MOVEQ   #$00,D0
                MOVE.W  STIMME2+$1C(pc),D0
                BSR     CALC_FREQ
                MOVE.L  D0,D4
                
                MOVEQ   #$00,D0
                MOVE.W  STIMME3+$1C(pc),D0
                BSR     CALC_FREQ
                MOVE.L  D0,D5

                BTST    #7,STIMME1+$28-BASE(A6) ;04
                BEQ.S   NO_NOI_1
                LSR.L   #2,D6
NO_NOI_1:       BTST    #7,STIMME2+$28-BASE(A6) ;0B
                BEQ.S   NO_NOI_2
                LSR.L   #2,D4
NO_NOI_2:       BTST    #7,STIMME3+$28-BASE(A6) ;12
                BEQ.S   NO_NOI_3
                LSR.L   #2,D5
NO_NOI_3:
                SWAP    D4
                SWAP    D5
                SWAP    D6
                MOVE.W  D6,D7
                MOVE.W  D5,D6
                MOVE.W  D4,D5
                MOVE.W  D7,D4

                MOVE.L  SMP_COUNT_1(PC),D0
                MOVE.L  SMP_COUNT_2(PC),D1
                MOVE.L  SMP_COUNT_3(PC),D2
                MOVE.L  PUFFER_LEN(PC),D7

                BSR     PREPARE_50
                CLR.L   D3
                BSR.S   FILL_ROUT

                MOVE.L  D0,SMP_COUNT_1-BASE(A6)
                MOVE.L  D1,SMP_COUNT_2-BASE(A6)
                MOVE.L  D2,SMP_COUNT_3-BASE(A6)
                RTS

;*---------------------------------------------------------------------------*
CALC_FREQ:      ;>PART
                MOVEM.L D1-D4,-(A7)     ;<-d0:period,  ->d0:frq
                MOVEQ   #0,D1
                AND.W   #$1FFF,D0
                BEQ.S   ZEROFREQ
                MOVE.L  #125000*$0100,D1
                BSR.S   DIV
                MOVE.L  #DMA_FREQUENCY/256,D0
                BSR.S   DIV
                lsl.L   #8,D1
ZEROFREQ:       MOVE.L  D1,D0
                MOVEM.L (A7)+,D1-D4
                RTS

DIV:            MOVE.L  D1,D3           ;div dividiert d1.l durch d2.w nach d1.l
                SUB.W   D3,D3
                SWAP    D3
                DIVU    D0,D3
                MOVE.W  D3,D4
                MOVE.W  D1,D3
                DIVU    D0,D3
                SWAP    D4
                MOVE.W  D3,D4
;SWAP    D3
                MOVE.L  D4,D1
                RTS

FILL_ROUT:      DS.B 100

PREPARE_50:     ;>PART

                MOVEM.L D0-A4,-(A7)

                LEA     BASE(PC),A3
                LEA     FILL_ROUT(PC),A1

SV1:            BTST    #2,STIMME1+$28-BASE(A3) ;04
                BEQ.S   SDO_NO_RI1
                LEA     RMOD1(PC),A0
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.W  (A0)+,(A1)+
                BRA.S   SV22
SDO_NO_RI1:     MOVE.L  #$18F00000,(A1)+ ;move.b 0(a0,d0.w),(a4)+

SV22:           BTST    #2,STIMME2+$28-BASE(A3) ;0B
                BEQ.S   SDO_NO_RI2
                LEA     RMOD2(PC),A0
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.W  (A0)+,(A1)+
                BRA.S   SV33
SDO_NO_RI2:     MOVE.L  #$18F11000,(A1)+ ;move.b 0(a1,d1.w),(a4)+

SV33:           BTST    #2,STIMME3+$28-BASE(A3) ;12
                BEQ.S   SDO_NO_RI3
                LEA     RMOD3(PC),A0
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.W  (A0)+,(A1)+
                BRA.S   SDO_SYNC_12
SDO_NO_RI3:     MOVE.L  #$18F22000,(A1)+ ;move.b 0(a2,d2.w),(a4)+


SDO_SYNC_12:    MOVE.W  #$421C,(A1)+    ;clr.b (a4)+

                MOVE.L  #$D686D184,(A1)+ ;ADD.l, ADDX.l
                MOVE.L  #$D385D546,(A1)+ ;ADDX.l ADDX.w

                BTST    #$01,STIMME1+$28-BASE(A3) ;04
                BEQ.S   SDO_NO_SYNC12
                MOVE.L  #$B47C00FF,(A1)+ ;cmp.w #$ff,d3
                MOVE.L  #$6F024240,(A1)+ ;ble.s +2, clr.w d0
SDO_NO_SYNC12:
                BTST    #$01,STIMME2+$28-BASE(A3) ;0B
                BEQ.S   SDO_NO_SYNC22
                MOVE.L  #$B07C00FF,(A1)+ ;cmp.w #$ff,d0
                MOVE.L  #$6F024241,(A1)+ ;ble +2, clr.w d1
SDO_NO_SYNC22:
                BTST    #$01,STIMME3+$28-BASE(A3) ;12
                BEQ.S   SDO_NO_SYNC32
                MOVE.L  #$B27C00FF,(A1)+ ;cmp.w #$ff,d1
                MOVE.L  #$6F024242,(A1)+ ;ble +2, clr.w d2
SDO_NO_SYNC32:
                MOVE.W  #$0240,(A1)+
                MOVE.W  NOISE_LEN1(PC),(A1)+ ;cmp.w #xxx,d0
                MOVE.W  #$0241,(A1)+
                MOVE.W  NOISE_LEN2(PC),(A1)+ ;cmp.w #xxx,d1
                MOVE.W  #$0242,(A1)+
                MOVE.W  NOISE_LEN3(PC),(A1)+ ;cmp.w #xxx,d2

                MOVE.W  #$51CF,(A1)+    ;DBRA d7,XX

                LEA     FILL_ROUT(PC),A2
                SUBA.L  A1,A2
                MOVE.W  A2,(A1)+        ;OFFSET fr DBRA

                MOVE.W  #$4E75,(A1)+    ;RTS

                MOVEM.L (A7)+,D0-A4
                RTS

RMOD1:          MOVE.B  0(A0,D0.w),D3
                TST.B   0(A0,D2.w)
                BPL.S   NOR1
                NEG.B   D3
NOR1:           MOVE.B  D3,(A4)+

RMOD2:          MOVE.B  0(A1,D1.w),D3
                TST.B   0(A1,D0.w)
                BPL.S   NOR2
                NEG.B   D3
NOR2:           MOVE.B  D3,(A4)+

RMOD3:          MOVE.B  0(A2,D2.w),D3
                TST.B   0(A2,D1.w)
                BPL.S   NOR3
                NEG.B   D3
NOR3:           MOVE.B  D3,(A4)+

PREPARE_FILL:
                MOVEM.L D0-A4,-(A7)
                LEA     FILL_ROUT(PC),A1
                LEA     BASE(PC),A3

                BTST    #2,STIMME1+$28-BASE(A3)
                BEQ.S   SDO_NO_RI1F
                LEA     RMOD1F(PC),A0
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                BTST    #2,STIMME2+$28-BASE(A3)
                BEQ.S   SV22F
                MOVE.W  (A0)+,(A1)+
                BRA.S   SV22F

SDO_NO_RI1F:    BTST    #2,STIMME2+$28-BASE(A3)
                BNE.S   RINGZ2F
                MOVE.L  #$16300000,(A1)+ ;move.b 0(a0,d0.w),d3
                BRA     SV22F
RINGZ2F:        MOVE.L  #$18B00000,(A1)+ ;move.b 0(a0,d0.w),(a4)


SV22F:          BTST    #2,STIMME2+$28-BASE(A3)
                BEQ.S   SDO_NO_RI2F
                LEA     RMOD2F(PC),A0
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.W  (A0)+,(A1)+
                BRA.S   SV33F
SDO_NO_RI2F:    MOVE.L  #$D6311000,(A1)+ ;add.b 0(a1,d1.w),d3
                MOVE.W  #$18C3,(A1)+    ; move.b d3,(a4)+

;*-------------------------------------------------------------------*

SV33F:          BTST    #2,STIMME3+$28-BASE(A3)
                BEQ.S   SDO_NO_RI3F
                LEA     RMOD3F(PC),A0
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.L  (A0)+,(A1)+
                MOVE.W  (A0)+,(A1)+
                BRA.S   SDO_SYNC_12F
SDO_NO_RI3F:    MOVE.L  #$18F22000,(A1)+ ;move.b 0(a2,d2.w),(a4)+

SDO_SYNC_12F:                           ;MOVE.W  #$421C,(A1)+    ;clr.b (a4)+

                MOVE.L  #$D686D184,(A1)+ ;ADD.l, ADDX.l
                MOVE.L  #$D385D546,(A1)+ ;ADDX.l ADDw.l

                BTST    #$01,STIMME1+$28-BASE(A3)
                BEQ.S   SDO_NO_SYNC12F
                MOVE.L  #$B47C00FF,(A1)+ ;cmp.w #$ff,d3
                MOVE.L  #$6F024240,(A1)+ ;ble.s +2, clr.w d0
SDO_NO_SYNC12F:
                BTST    #$01,STIMME2+$28-BASE(A3)
                BEQ.S   SDO_NO_SYNC22F
                MOVE.L  #$B07C00FF,(A1)+ ;cmp.w #$ff,d0
                MOVE.L  #$6F024241,(A1)+ ;ble +2, clr.w d1
SDO_NO_SYNC22F:
                BTST    #$01,STIMME3+$28-BASE(A3)
                BEQ.S   SDO_NO_SYNC32F
                MOVE.L  #$B27C00FF,(A1)+ ;cmp.w #$ff,d1
                MOVE.L  #$6F024242,(A1)+ ;ble +2, clr.w d2
SDO_NO_SYNC32F:
                MOVE.W  #$0240,(A1)+
                MOVE.W  NOISE_LEN1(PC),(A1)+ ;cmp.w #xxx,d0
                MOVE.W  #$0241,(A1)+
                MOVE.W  NOISE_LEN2(PC),(A1)+ ;cmp.w #xxx,d1
                MOVE.W  #$0242,(A1)+
                MOVE.W  NOISE_LEN3(PC),(A1)+ ;cmp.w #xxx,d2

                MOVE.W  #$51CF,(A1)+    ;DBRA d7,XX

                LEA     FILL_ROUT(PC),A2
                SUBA.L  A1,A2
                MOVE.W  A2,(A1)+        ;OFFSET fr DBRA

                MOVE.W  #$4E75,(A1)+    ;RTS

                MOVEM.L (A7)+,D0-A4
                RTS


RMOD1F:         MOVE.B  0(A0,D0.w),D3
                TST.B   0(A0,D2.w)
                BPL.S   NOR1F
                NEG.B   D3
NOR1F:          MOVE.B  D3,(A4)


RMOD2F:         MOVE.B  0(A1,D1.w),D3
                TST.B   0(A1,D0.w)
                BPL.S   NOR2F
                NEG.B   D3
NOR2F:          ADD.B   D3,(A4)+

RMOD3F:         MOVE.B  0(A2,D2.w),D3
                TST.B   0(A2,D1.w)
                BPL.S   NOR3F
                NEG.B   D3
NOR3F:          MOVE.B  D3,(A4)+

;*---------------------  Datafield  ----------------------------------------*
DATAS:          ;>PART 'Datas'

SAMPLE_1:       DC.L 0
SMP_COUNT_1:    DC.L 0
SAMPLE_2:       DC.L 0
SMP_COUNT_2:    DC.L 0
SAMPLE_3:       DC.L 0
SMP_COUNT_3:    DC.L 0

NOISE_LEN1:     DC.W $FF
NOISE_LEN2:     DC.W $FF
NOISE_LEN3:     DC.W $FF

NO_NOISE:       DS.B 256

PUFFER_LEN:     DC.L 0

SID_MIX_PTR:    dc.l  0
SID_MIX_CNT:    dc.w  0
SID_MIX_BUF:    ds.w  1024  ; DMA frequency / Replay Frequency

DMA_PTR:  	dc.l  0;; DMA_BUF
DMA_BUF:        dcb.b 512
DMA_BUF_END:
DMA_BUF_SZ: = DMA_BUF_END-DMA_BUF

_W_NOISE:       dc.l  W_NOISE-_W_NOISE
_NOISE:         dc.l  NOISE-_NOISE

_W_SAWTOOTH:    dc.l  W_SAWTOOTH-_W_SAWTOOTH
_SAWTOOTH:      dc.l  SAWTOOTH-_SAWTOOTH

_W_TRIANGLE:    dc.l  W_TRIANGLE-_W_TRIANGLE
_TRIANGLE:      dc.l  TRIANGLE-_TRIANGLE

_W_PULSE:           dc.l  W_PULSE-_W_PULSE
_PULSE:             dc.l  PULSE-_PULSE
_PULSE_TRIANGLE:    dc.l  PULSE_TRIANGLE-_PULSE_TRIANGLE
_PULSE_SAWTOOTH:    dc.l  PULSE_SAWTOOTH-_PULSE_SAWTOOTH
_TRIANGLE_SAWTOOTH: dc.l  TRIANGLE_SAWTOOTH-_TRIANGLE_SAWTOOTH

;*---------------------  Waves  --------------------------------------------*

W_NOISE:        DS.W 30720              ; 4096*15
NOISE:          INCBIN "org/tsd_noise.dat"
                EVEN

W_SAWTOOTH:     DS.B 256*15
SAWTOOTH:
                DC.B $00,$01,$02,$03,$04,$05,$06,$07
                DC.B $08,$09,$0A,$0B,$0C,$0D,$0E,$0F
                DC.B $10,$11,$12,$13,$14,$15,$16,$17
                DC.B $18,$19,$1A,$1B,$1C,$1D,$1E,$1F
                DC.B $20,$21,$22,$23,$24,$25,$26,$27
                DC.B $28,$29,$2A,$2B,$2C,$2D,$2E,$2F
                DC.B $30,$31,$32,$33,$34,$35,$36,$37
                DC.B $38,$39,$3A,$3B,$3C,$3D,$3E,$3F
                DC.B $40,$41,$42,$43,$44,$45,$46,$47
                DC.B $48,$49,$4A,$4B,$4C,$4D,$4E,$4F
                DC.B $50,$51,$52,$53,$54,$55,$56,$57
                DC.B $58,$59,$5A,$5B,$5C,$5D,$5E,$5F
                DC.B $60,$61,$62,$63,$64,$65,$66,$67
                DC.B $68,$69,$6A,$6B,$6C,$6D,$6E,$6F
                DC.B $70,$71,$72,$73,$74,$75,$76,$77
                DC.B $78,$79,$7A,$7B,$7C,$7D,$7E,$7F

                DC.B $80,$81,$82,$83,$84,$85,$86,$87
                DC.B $88,$89,$8A,$8B,$8C,$8D,$8E,$8F
                DC.B $90,$91,$92,$93,$94,$95,$96,$97
                DC.B $98,$99,$9A,$9B,$9C,$9D,$9E,$9F
                DC.B $A0,$A1,$A2,$A3,$A4,$A5,$A6,$A7
                DC.B $A8,$A9,$AA,$AB,$AC,$AD,$AE,$AF
                DC.B $B0,$B1,$B2,$B3,$B4,$B5,$B6,$B7
                DC.B $B8,$B9,$BA,$BB,$BC,$BD,$BE,$BF
                DC.B $C0,$C1,$C2,$C3,$C4,$C5,$C6,$C7
                DC.B $C8,$C9,$CA,$CB,$CC,$CD,$CE,$CF
                DC.B $D0,$D1,$D2,$D3,$D4,$D5,$D6,$D7
                DC.B $D8,$D9,$DA,$DB,$DC,$DD,$DE,$DF
                DC.B $E0,$E1,$E2,$E3,$E4,$E5,$E6,$E7
                DC.B $E8,$E9,$EA,$EB,$EC,$ED,$EE,$EF
                DC.B $F0,$F1,$F2,$F3,$F4,$F5,$F6,$F7
                DC.B $F8,$F9,$FA,$FB,$FC,$FD,$FE,$FF


W_TRIANGLE:     DS.B 256*15
TRIANGLE:
                DC.B $00,$02,$04,$06,$08,$0A,$0C,$0F
                DC.B $10,$12,$14,$16,$18,$1A,$1C,$1F
                DC.B $20,$22,$24,$26,$28,$2A,$2C,$2F
                DC.B $30,$32,$34,$36,$38,$3A,$3C,$3F
                DC.B $40,$42,$44,$46,$48,$4A,$4C,$4F
                DC.B $50,$52,$54,$56,$58,$5A,$5C,$5F
                DC.B $60,$62,$64,$66,$68,$6A,$6C,$6F
                DC.B $70,$72,$74,$76,$78,$7A,$7C,$7F

                DC.B $7F,$7C,$7A,$78,$76,$74,$72,$70
                DC.B $6E,$6C,$6A,$68,$66,$64,$62,$60
                DC.B $5E,$5C,$5A,$58,$56,$54,$52,$50
                DC.B $4E,$4C,$4A,$48,$46,$44,$42,$40
                DC.B $3E,$3C,$3A,$38,$36,$34,$32,$30
                DC.B $2E,$2C,$2A,$28,$26,$24,$22,$20
                DC.B $1E,$1C,$1A,$18,$16,$14,$12,$10
                DC.B $0E,$0C,$0A,$08,$06,$04,$02,$00

                DC.B $FE,$FC,$FA,$F8,$F6,$F4,$F2,$F0
                DC.B $EE,$EC,$EA,$E8,$E6,$E4,$E2,$E0
                DC.B $DE,$DC,$DA,$D8,$D6,$D4,$D2,$D0
                DC.B $CE,$CC,$CA,$C8,$C6,$C4,$C2,$C0
                DC.B $BE,$BC,$BA,$B8,$B6,$B3,$B2,$B0
                DC.B $AE,$AC,$AA,$A8,$A6,$A4,$A2,$A0
                DC.B $9E,$9C,$9A,$98,$96,$94,$92,$90
                DC.B $8E,$8C,$8A,$88,$86,$84,$82,$80

                DC.B $80,$82,$84,$86,$88,$8A,$8C,$8E
                DC.B $90,$92,$94,$96,$98,$9A,$9C,$9E
                DC.B $A0,$A2,$A4,$A6,$A8,$AA,$AC,$AE
                DC.B $B0,$B2,$B4,$B6,$B8,$BA,$BC,$BE
                DC.B $C0,$C2,$C4,$C6,$C8,$CA,$CC,$CE
                DC.B $D0,$D2,$D4,$D6,$D8,$DA,$DC,$DE
                DC.B $E0,$E2,$E4,$E6,$E8,$EA,$EC,$EE
                DC.B $F0,$F2,$F4,$F6,$F8,$FA,$FC,$FE

W_PULSE:        DS.B 512*15
PULSE:          dcb.b 256,$7C
                dcb.b 256,$83

PULSE_TRIANGLE: DS.B 512*16
PULSE_SAWTOOTH: DS.B 512*16
TRIANGLE_SAWTOOTH:DS.B 256*16

;*--------------------------------------------------------------------------*
