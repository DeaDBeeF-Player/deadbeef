/***************************************************************************
                            spu.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
                       
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/
                           
//*************************************************************************//
// History of changes:
//
// 2005/08/29 - Pete
// - changed to 48Khz output
//
// 2004/12/25 - Pete
// - inc'd version for pcsx2-0.7
//
// 2004/04/18 - Pete
// - changed all kind of things in the plugin
//
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2003/04/07 - Eric
// - adjusted cubic interpolation algorithm
//
// 2003/03/16 - Eric
// - added cubic interpolation
//
// 2003/03/01 - linuzappz
// - libraryName changes using ALSA
//
// 2003/02/28 - Pete
// - added option for type of interpolation
// - adjusted spu irqs again (Thousant Arms, Valkyrie Profile)
// - added MONO support for MSWindows DirectSound
//
// 2003/02/20 - kode54
// - amended interpolation code, goto GOON could skip initialization of gpos and cause segfault
//
// 2003/02/19 - kode54
// - moved SPU IRQ handler and changed sample flag processing
//
// 2003/02/18 - kode54
// - moved ADSR calculation outside of the sample decode loop, somehow I doubt that
//   ADSR timing is relative to the frequency at which a sample is played... I guess
//   this remains to be seen, and I don't know whether ADSR is applied to noise channels...
//
// 2003/02/09 - kode54
// - one-shot samples now process the end block before stopping
// - in light of removing fmod hack, now processing ADSR on frequency channel as well
//
// 2003/02/08 - kode54
// - replaced easy interpolation with gaussian
// - removed fmod averaging hack
// - changed .sinc to be updated from .iRawPitch, no idea why it wasn't done this way already (<- Pete: because I sometimes fail to see the obvious, haharhar :)
//
// 2003/02/08 - linuzappz
// - small bugfix for one usleep that was 1 instead of 1000
// - added iDisStereo for no stereo (Linux)
//
// 2003/01/22 - Pete
// - added easy interpolation & small noise adjustments
//
// 2003/01/19 - Pete
// - added Neill's reverb
//
// 2003/01/12 - Pete
// - added recording window handlers
//
// 2003/01/06 - Pete
// - added Neill's ADSR timings
//
// 2002/12/28 - Pete
// - adjusted spu irq handling, fmod handling and loop handling
//
// 2002/08/14 - Pete
// - added extra reverb
//
// 2002/06/08 - linuzappz
// - SPUupdate changed for SPUasync
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_SPU

#include "../peops2/externals.h"
#include "../peops2/regs.h"
#include "../peops2/dma.h"
 
////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////

// psx buffer / addresses

unsigned short  regArea[32*1024];                        
unsigned short  spuMem[1*1024*1024];
unsigned char * spuMemC;
unsigned char * pSpuIrq[2];
unsigned char * pSpuBuffer;

// user settings          

int             iUseXA=0;
int             iVolume=3;
int             iXAPitch=1;
int             iUseTimer=2;
int             iSPUIRQWait=1;
int             iDebugMode=0;
int             iRecordMode=0;
int             iUseReverb=1;
int             iUseInterpolation=2;
                             
// MAIN infos struct for each channel

SPUCHAN         s_chan[MAXCHAN+1];                     // channel + 1 infos (1 is security for fmod handling)
REVERBInfo      rvb[2];

unsigned long   dwNoiseVal=1;                          // global noise generator

unsigned short  spuCtrl2[2];                           // some vars to store psx reg infos
unsigned short  spuStat2[2];
unsigned long   spuIrq2[2];             
unsigned long   spuAddr2[2];                           // address into spu mem
unsigned long   spuRvbAddr2[2];
unsigned long   spuRvbAEnd2[2];
int             bEndThread=0;                          // thread handlers
int             bThreadEnded=0;
int             bSpuInit=0;
int             bSPUIsOpen=0;

unsigned long dwNewChannel2[2];                        // flags for faster testing, if new channel starts
unsigned long dwEndChannel2[2];

// UNUSED IN PS2 YET
void (CALLBACK *irqCallback)(void)=0;                  // func of main emu, called on spu irq
void (CALLBACK *cddavCallback)(unsigned short,unsigned short)=0;

// certain globals (were local before, but with the new timeproc I need em global)

const int f[5][2] = {   {    0,  0  },
                        {   60,  0  },
                        {  115, -52 },
                        {   98, -55 },
                        {  122, -60 } };
int SSumR[NSSIZE];
int SSumL[NSSIZE];
int iCycle=0;
short * pS;

static int lastch=-1;      // last channel processed on spu irq in timer mode
static int lastns=0;       // last ns pos
static int iSecureStart=0; // secure start counter

extern void ps2_update(unsigned char *samples, long lBytes);

////////////////////////////////////////////////////////////////////////
// CODE AREA
////////////////////////////////////////////////////////////////////////

// dirty inline func includes

#include "reverb2.c"        
#include "adsr2.c"

////////////////////////////////////////////////////////////////////////
// helpers for simple interpolation

//
// easy interpolation on upsampling, no special filter, just "Pete's common sense" tm
//
// instead of having n equal sample values in a row like:
//       ____
//           |____
//
// we compare the current delta change with the next delta change.
//
// if curr_delta is positive,
//
//  - and next delta is smaller (or changing direction):
//         \.
//          -__
//
//  - and next delta significant (at least twice) bigger:
//         --_
//            \.
//
//  - and next delta is nearly same:
//          \.
//           \.
//
//
// if curr_delta is negative,
//
//  - and next delta is smaller (or changing direction):
//          _--
//         /
//
//  - and next delta significant (at least twice) bigger:
//            /
//         __- 
//         
//  - and next delta is nearly same:
//           /
//          /
//     


INLINE void InterpolateUp(int ch)
{
 if(s_chan[ch].SB[32]==1)                              // flag == 1? calc step and set flag... and don't change the value in this pass
  {
   const int id1=s_chan[ch].SB[30]-s_chan[ch].SB[29];  // curr delta to next val
   const int id2=s_chan[ch].SB[31]-s_chan[ch].SB[30];  // and next delta to next-next val :)

   s_chan[ch].SB[32]=0;

   if(id1>0)                                           // curr delta positive
    {
     if(id2<id1)
      {s_chan[ch].SB[28]=id1;s_chan[ch].SB[32]=2;}
     else
     if(id2<(id1<<1))
      s_chan[ch].SB[28]=(id1*s_chan[ch].sinc)/0x10000L;
     else
      s_chan[ch].SB[28]=(id1*s_chan[ch].sinc)/0x20000L; 
    }
   else                                                // curr delta negative
    {
     if(id2>id1)
      {s_chan[ch].SB[28]=id1;s_chan[ch].SB[32]=2;}
     else
     if(id2>(id1<<1))
      s_chan[ch].SB[28]=(id1*s_chan[ch].sinc)/0x10000L;
     else
      s_chan[ch].SB[28]=(id1*s_chan[ch].sinc)/0x20000L; 
    }
  }
 else
 if(s_chan[ch].SB[32]==2)                              // flag 1: calc step and set flag... and don't change the value in this pass
  {
   s_chan[ch].SB[32]=0;

   s_chan[ch].SB[28]=(s_chan[ch].SB[28]*s_chan[ch].sinc)/0x20000L;
   if(s_chan[ch].sinc<=0x8000)
        s_chan[ch].SB[29]=s_chan[ch].SB[30]-(s_chan[ch].SB[28]*((0x10000/s_chan[ch].sinc)-1));
   else s_chan[ch].SB[29]+=s_chan[ch].SB[28];
  }
 else                                                  // no flags? add bigger val (if possible), calc smaller step, set flag1
  s_chan[ch].SB[29]+=s_chan[ch].SB[28];
}

//
// even easier interpolation on downsampling, also no special filter, again just "Pete's common sense" tm
//

INLINE void InterpolateDown(int ch)
{
 if(s_chan[ch].sinc>=0x20000L)                                 // we would skip at least one val?
  {
   s_chan[ch].SB[29]+=(s_chan[ch].SB[30]-s_chan[ch].SB[29])/2; // add easy weight
   if(s_chan[ch].sinc>=0x30000L)                               // we would skip even more vals?
    s_chan[ch].SB[29]+=(s_chan[ch].SB[31]-s_chan[ch].SB[30])/2;// add additional next weight
  }
}

////////////////////////////////////////////////////////////////////////
// helpers for gauss interpolation

#define gval0 (((short*)(&s_chan[ch].SB[29]))[gpos])
#define gval(x) (((short*)(&s_chan[ch].SB[29]))[(gpos+x)&3])

#include "gauss_i.h"

////////////////////////////////////////////////////////////////////////

//#include "xa.c"

////////////////////////////////////////////////////////////////////////
// START SOUND... called by main thread to setup a new sound on a channel
////////////////////////////////////////////////////////////////////////

INLINE void StartSound(int ch)
{
 dwNewChannel2[ch/24]&=~(1<<(ch%24));                  // clear new channel bit
 dwEndChannel2[ch/24]&=~(1<<(ch%24));                  // clear end channel bit

 StartADSR(ch);
 StartREVERB(ch);      
                          
 s_chan[ch].pCurr=s_chan[ch].pStart;                   // set sample start
                         
 s_chan[ch].s_1=0;                                     // init mixing vars
 s_chan[ch].s_2=0;
 s_chan[ch].iSBPos=28;

 s_chan[ch].bNew=0;                                    // init channel flags
 s_chan[ch].bStop=0;                                   
 s_chan[ch].bOn=1;

 s_chan[ch].SB[29]=0;                                  // init our interpolation helpers
 s_chan[ch].SB[30]=0;

 if(iUseInterpolation>=2)                              // gauss interpolation?
      {s_chan[ch].spos=0x30000L;s_chan[ch].SB[28]=0;}  // -> start with more decoding
 else {s_chan[ch].spos=0x10000L;s_chan[ch].SB[31]=0;}  // -> no/simple interpolation starts with one 44100 decoding
}

////////////////////////////////////////////////////////////////////////
// MAIN SPU FUNCTION
// here is the main job handler... thread, timer or direct func call
// basically the whole sound processing is done in this fat func!
////////////////////////////////////////////////////////////////////////

static u32 sampcount;
static u32 decaybegin;
static u32 decayend;

// Counting to 65536 results in full volume offage.
void setlength2(s32 stop, s32 fade)
{
 if(stop==~0)
 {
  decaybegin=~0;
 }
 else
 {
  stop=(stop*441)/10;
  fade=(fade*441)/10;

  decaybegin=stop;
  decayend=stop+fade;
 }
}
// 5 ms waiting phase, if buffer is full and no new sound has to get started
// .. can be made smaller (smallest val: 1 ms), but bigger waits give
// better performance

#define PAUSE_W 5
#define PAUSE_L 5000

////////////////////////////////////////////////////////////////////////

int iSpuAsyncWait=0;

static void *MAINThread(int samp2run)
{
 int s_1,s_2,fa,voldiv=iVolume;
 unsigned char * start;unsigned int nSample;
 int ch,predict_nr,shift_factor,flags,d,d2,s;
 int gpos,bIRQReturn=0;

// while(!bEndThread)                                    // until we are shutting down
  {
   //--------------------------------------------------//
   // ok, at the beginning we are looking if there is
   // enuff free place in the dsound/oss buffer to
   // fill in new data, or if there is a new channel to start.
   // if not, we wait (thread) or return (timer/spuasync)
   // until enuff free place is available/a new channel gets
   // started

   if(dwNewChannel2[0] || dwNewChannel2[1])            // new channel should start immedately?
    {                                                  // (at least one bit 0 ... MAXCHANNEL is set?)
     iSecureStart++;                                   // -> set iSecure
     if(iSecureStart>5) iSecureStart=0;                //    (if it is set 5 times - that means on 5 tries a new samples has been started - in a row, we will reset it, to give the sound update a chance)
    }
   else iSecureStart=0;                                // 0: no new channel should start

/*	if (!iSecureStart)
	{
	     iSecureStart=0;                                   // reset secure
	     return;
	}*/

#if 0
   while(!iSecureStart && !bEndThread) // &&               // no new start? no thread end?
//         (SoundGetBytesBuffered()>TESTSIZE))           // and still enuff data in sound buffer?
    {
     iSecureStart=0;                                   // reset secure

     if(iUseTimer) return 0;                           // linux no-thread mode? bye

     if(dwNewChannel2[0] || dwNewChannel2[1]) 
      iSecureStart=1;                                  // if a new channel kicks in (or, of course, sound buffer runs low), we will leave the loop
    }
#endif

   //--------------------------------------------------// continue from irq handling in timer mode? 

   if(lastch>=0)                                       // will be -1 if no continue is pending
    {
     ch=lastch; lastch=-1;                  // -> setup all kind of vars to continue
     goto GOON;                                        // -> directly jump to the continue point
    }

   //--------------------------------------------------//
   //- main channel loop                              -// 
   //--------------------------------------------------//
    {
     for(ch=0;ch<MAXCHAN;ch++)                         // loop em all... we will collect 1 ms of sound of each playing channel
      {
       if(s_chan[ch].bNew) StartSound(ch);             // start new sound
       if(!s_chan[ch].bOn) continue;                   // channel not playing? next

       if(s_chan[ch].iActFreq!=s_chan[ch].iUsedFreq)   // new psx frequency?
        {
         s_chan[ch].iUsedFreq=s_chan[ch].iActFreq;     // -> take it and calc steps
         s_chan[ch].sinc=s_chan[ch].iRawPitch<<4;
         if(!s_chan[ch].sinc) s_chan[ch].sinc=1;
         if(iUseInterpolation==1) s_chan[ch].SB[32]=1; // -> freq change in simle imterpolation mode: set flag
        }
//       ns=0;
//       while(ns<NSSIZE)                                // loop until 1 ms of data is reached
        {
         while(s_chan[ch].spos>=0x10000L)
          {
           if(s_chan[ch].iSBPos==28)                   // 28 reached?
            {
             start=s_chan[ch].pCurr;                   // set up the current pos

             if (start == (unsigned char*)-1)          // special "stop" sign
              {
               s_chan[ch].bOn=0;                       // -> turn everything off
               s_chan[ch].ADSRX.lVolume=0;
               s_chan[ch].ADSRX.EnvelopeVol=0;
               goto ENDX;                              // -> and done for this channel
              }

             s_chan[ch].iSBPos=0;

             //////////////////////////////////////////// spu irq handler here? mmm... do it later

             s_1=s_chan[ch].s_1;
             s_2=s_chan[ch].s_2;

             predict_nr=(int)*start;start++;
             shift_factor=predict_nr&0xf;
             predict_nr >>= 4;
             flags=(int)*start;start++;

             // -------------------------------------- // 

             for (nSample=0;nSample<28;start++)      
              {
               d=(int)*start;
               s=((d&0xf)<<12);
               if(s&0x8000) s|=0xffff0000;

               fa=(s >> shift_factor);
               fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
               s_2=s_1;s_1=fa;
               s=((d & 0xf0) << 8);

               s_chan[ch].SB[nSample++]=fa;

               if(s&0x8000) s|=0xffff0000;
               fa=(s>>shift_factor);              
               fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
               s_2=s_1;s_1=fa;

               s_chan[ch].SB[nSample++]=fa;
              }     

             //////////////////////////////////////////// irq check

             if(spuCtrl2[ch/24]&0x40)                  // some irq active?
              {
               if((pSpuIrq[ch/24] >  start-16 &&       // irq address reached?
                   pSpuIrq[ch/24] <= start) ||
                  ((flags&1) &&                        // special: irq on looping addr, when stop/loop flag is set 
                   (pSpuIrq[ch/24] >  s_chan[ch].pLoop-16 &&
                    pSpuIrq[ch/24] <= s_chan[ch].pLoop)))
                {
                 s_chan[ch].iIrqDone=1;                // -> debug flag

                 if(irqCallback) irqCallback();        // -> call main emu (not supported in SPU2 right now)
                 else
                  {
                   if(ch<24) InterruptDMA4();            // -> let's see what is happening if we call our irqs instead ;)
                   else      InterruptDMA7();
                  }
                  
                 if(iSPUIRQWait)                       // -> option: wait after irq for main emu
                  {
                   iSpuAsyncWait=1;
                   bIRQReturn=1;
                  }
                }
              }
      
             //////////////////////////////////////////// flag handler

             if((flags&4) && (!s_chan[ch].bIgnoreLoop))
              s_chan[ch].pLoop=start-16;               // loop adress

             if(flags&1)                               // 1: stop/loop
              {
               dwEndChannel2[ch/24]|=(1<<(ch%24));
              
               // We play this block out first...
               //if(!(flags&2)|| s_chan[ch].pLoop==NULL)   
                                                       // 1+2: do loop... otherwise: stop
               if(flags!=3 || s_chan[ch].pLoop==NULL)  // PETE: if we don't check exactly for 3, loop hang ups will happen (DQ4, for example)
                {                                      // and checking if pLoop is set avoids crashes, yeah
                 start = (unsigned char*)-1;
                }
               else
                {
                 start = s_chan[ch].pLoop;
                }
              }

             s_chan[ch].pCurr=start;                   // store values for next cycle
             s_chan[ch].s_1=s_1;
             s_chan[ch].s_2=s_2;      

             ////////////////////////////////////////////

             if(bIRQReturn)                            // special return for "spu irq - wait for cpu action"
              {
               bIRQReturn=0;
                {
                 lastch=ch; 
//                 lastns=ns;	// changemeback

                 return 0;
                }
              }

             ////////////////////////////////////////////

GOON: ;

            }

           fa=s_chan[ch].SB[s_chan[ch].iSBPos++];      // get sample data

//           if((spuCtrl2[ch/24]&0x4000)==0) fa=0;       // muted?
//           else                                        // else adjust
            {
             if(fa>32767L)  fa=32767L;
             if(fa<-32767L) fa=-32767L;              
            }

           if(iUseInterpolation>=2)                    // gauss/cubic interpolation
            {
             gpos = s_chan[ch].SB[28];
             gval0 = fa;          
             gpos = (gpos+1) & 3;
             s_chan[ch].SB[28] = gpos;
            }
           else
           if(iUseInterpolation==1)                    // simple interpolation
            {
             s_chan[ch].SB[28] = 0;                    
             s_chan[ch].SB[29] = s_chan[ch].SB[30];    // -> helpers for simple linear interpolation: delay real val for two slots, and calc the two deltas, for a 'look at the future behaviour'
             s_chan[ch].SB[30] = s_chan[ch].SB[31];
             s_chan[ch].SB[31] = fa;
             s_chan[ch].SB[32] = 1;                    // -> flag: calc new interolation
            }
           else s_chan[ch].SB[29]=fa;                  // no interpolation

           s_chan[ch].spos -= 0x10000L;
          }

         ////////////////////////////////////////////////
         // noise handler... just produces some noise data
         // surely wrong... and no noise frequency (spuCtrl&0x3f00) will be used...
         // and sometimes the noise will be used as fmod modulation... pfff

         if(s_chan[ch].bNoise)
          {
           if((dwNoiseVal<<=1)&0x80000000L)
            {
             dwNoiseVal^=0x0040001L;
             fa=((dwNoiseVal>>2)&0x7fff);
             fa=-fa;
            }
           else fa=(dwNoiseVal>>2)&0x7fff;

           // mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
           fa=s_chan[ch].iOldNoise+((fa-s_chan[ch].iOldNoise)/((0x001f-((spuCtrl2[ch/24]&0x3f00)>>9))+1));
           if(fa>32767L)  fa=32767L;
           if(fa<-32767L) fa=-32767L;              
           s_chan[ch].iOldNoise=fa;

           if(iUseInterpolation<2)                     // no gauss/cubic interpolation?
            s_chan[ch].SB[29] = fa;                    // -> store noise val in "current sample" slot
          }                                            //----------------------------------------
         else                                          // NO NOISE (NORMAL SAMPLE DATA) HERE 
          {//------------------------------------------//
           if(iUseInterpolation==3)                    // cubic interpolation
            {
             long xd;  
             xd = ((s_chan[ch].spos) >> 1)+1;
             gpos = s_chan[ch].SB[28];

             fa  = gval(3) - 3*gval(2) + 3*gval(1) - gval0;
             fa *= (xd - (2<<15)) / 6;
             fa >>= 15;
             fa += gval(2) - gval(1) - gval(1) + gval0;
             fa *= (xd - (1<<15)) >> 1;
             fa >>= 15;
             fa += gval(1) - gval0;
             fa *= xd;
             fa >>= 15;
             fa = fa + gval0;
            }
           //------------------------------------------//
           else
           if(iUseInterpolation==2)                    // gauss interpolation
            {
             int vl, vr;
             vl = (s_chan[ch].spos >> 6) & ~3;
             gpos = s_chan[ch].SB[28];
             vr=(gauss[vl]*gval0)&~2047;
             vr+=(gauss[vl+1]*gval(1))&~2047;
             vr+=(gauss[vl+2]*gval(2))&~2047;
             vr+=(gauss[vl+3]*gval(3))&~2047;
             fa = vr>>11;
/*
             vr=(gauss[vl]*gval0)>>9;
             vr+=(gauss[vl+1]*gval(1))>>9;
             vr+=(gauss[vl+2]*gval(2))>>9;
             vr+=(gauss[vl+3]*gval(3))>>9;
             fa = vr>>2;
*/
            }
           //------------------------------------------//
           else
           if(iUseInterpolation==1)                    // simple interpolation
            {
             if(s_chan[ch].sinc<0x10000L)              // -> upsampling?
                  InterpolateUp(ch);                   // --> interpolate up
             else InterpolateDown(ch);                 // --> else down
             fa=s_chan[ch].SB[29];
            }
           //------------------------------------------//
           else fa=s_chan[ch].SB[29];                  // no interpolation
          }

         s_chan[ch].sval = (MixADSR(ch) * fa) / 1023;  // add adsr

         if(s_chan[ch].bFMod==2)                       // fmod freq channel
          {
           int NP=s_chan[ch+1].iRawPitch;
           double intr;

           NP=((32768L+s_chan[ch].sval)*NP)/32768L;    // mmm... I still need to adjust that to 1/48 khz... we will wait for the first game/demo using it to decide how to do it :)

           if(NP>0x3fff) NP=0x3fff;
           if(NP<0x1)    NP=0x1;
           
	   intr = (double)48000.0f / (double)44100.0f * (double)NP;
           NP = (UINT32)intr;

           NP=(44100L*NP)/(4096L);                     // calc frequency

           s_chan[ch+1].iActFreq=NP;
           s_chan[ch+1].iUsedFreq=NP;
           s_chan[ch+1].sinc=(((NP/10)<<16)/4410);
           if(!s_chan[ch+1].sinc) s_chan[ch+1].sinc=1;
           if(iUseInterpolation==1)                    // freq change in sipmle interpolation mode
            s_chan[ch+1].SB[32]=1;

// mmmm... set up freq decoding positions?
//           s_chan[ch+1].iSBPos=28;
//           s_chan[ch+1].spos=0x10000L;
          }                    
         else                   
          {                                          
           //////////////////////////////////////////////
           // ok, left/right sound volume (psx volume goes from 0 ... 0x3fff)

           if(s_chan[ch].iMute) 
            s_chan[ch].sval=0;                         // debug mute
           else
            {
             if(s_chan[ch].bVolumeL)
              SSumL[0]+=(s_chan[ch].sval*s_chan[ch].iLeftVolume)/0x4000L;
             if(s_chan[ch].bVolumeR)
              SSumR[0]+=(s_chan[ch].sval*s_chan[ch].iRightVolume)/0x4000L;
            }
        
           //////////////////////////////////////////////
           // now let us store sound data for reverb    
                                                          
           if(s_chan[ch].bRVBActive) StoreREVERB(ch,0);
          }

         ////////////////////////////////////////////////
         // ok, go on until 1 ms data of this channel is collected
                                                       
         s_chan[ch].spos += s_chan[ch].sinc;             
                                                              
        }        
ENDX:   ;                                                      
      }
    }                                                         
   
  //---------------------------------------------------//
  //- here we have another 1 ms of sound data
  //---------------------------------------------------//

  ///////////////////////////////////////////////////////
  // mix all channels (including reverb) into one buffer

    SSumL[0]+=MixREVERBLeft(0,0);
    SSumL[0]+=MixREVERBLeft(0,1);
    SSumR[0]+=MixREVERBRight(0);
    SSumR[0]+=MixREVERBRight(1);
                                              
    d=SSumL[0]/voldiv;SSumL[0]=0;
    d2=SSumR[0]/voldiv;SSumR[0]=0;

    if(d<-32767) d=-32767;if(d>32767) d=32767;
    if(d2<-32767) d2=-32767;if(d2>32767) d2=32767;

    if(sampcount>=decaybegin)
    {
	s32 dmul;
	if(decaybegin!=~0) // Is anyone REALLY going to be playing a song
		      // for 13 hours?
    	{
		if(sampcount>=decayend) 
		{
//		       	ao_song_done = 1;
		        return(0);
		}

		dmul=256-(256*(sampcount-decaybegin)/(decayend-decaybegin));
		d=(d*dmul)>>8;
		d2=(d2*dmul)>>8;
	}
    }
    sampcount++;

    *pS++=d;
    *pS++=d2;

    InitREVERB();

  //////////////////////////////////////////////////////                   
  // feed the sound
  // wanna have around 1/60 sec (16.666 ms) updates
	if ((((unsigned char *)pS)-((unsigned char *)pSpuBuffer)) == (735*4))
	{
	    	ps2_update((u8*)pSpuBuffer,(u8*)pS-(u8*)pSpuBuffer);
	        pS=(short *)pSpuBuffer;					  
	}
 }

 // end of big main loop...

 bThreadEnded=1;

 return 0;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// SPU ASYNC... even newer epsxe func
//  1 time every 'cycle' cycles... harhar
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2async(unsigned long cycle)
{
 if(iSpuAsyncWait)
  {
   iSpuAsyncWait++;
   if(iSpuAsyncWait<=64) return;
   iSpuAsyncWait=0;
  }

   MAINThread(0);                                      // -> linux high-compat mode
}

////////////////////////////////////////////////////////////////////////
// INIT/EXIT STUFF
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// SPUINIT: this func will be called first by the main emu
////////////////////////////////////////////////////////////////////////

              
EXPORT_GCC long CALLBACK SPU2init(void)
{
 spuMemC=(unsigned char *)spuMem;                      // just small setup
 memset((void *)s_chan,0,MAXCHAN*sizeof(SPUCHAN));
 memset(rvb,0,2*sizeof(REVERBInfo));

 sampcount = 0;

 InitADSR();

 return 0;
}

////////////////////////////////////////////////////////////////////////
// SETUPTIMER: init of certain buffers and threads/timers
////////////////////////////////////////////////////////////////////////

static void SetupTimer(void)
{
 memset(SSumR,0,NSSIZE*sizeof(int));                   // init some mixing buffers
 memset(SSumL,0,NSSIZE*sizeof(int));
 pS=(short *)pSpuBuffer;                               // setup soundbuffer pointer

 bEndThread=0;                                         // init thread vars
 bThreadEnded=0; 
 bSpuInit=1;                                           // flag: we are inited
}

////////////////////////////////////////////////////////////////////////
// REMOVETIMER: kill threads/timers
////////////////////////////////////////////////////////////////////////

static void RemoveTimer(void)
{
 bEndThread=1;                                         // raise flag to end thread
 bThreadEnded=0;                                       // no more spu is running
 bSpuInit=0;
}

////////////////////////////////////////////////////////////////////////
// SETUPSTREAMS: init most of the spu buffers
////////////////////////////////////////////////////////////////////////

static void SetupStreams(void)
{ 
 int i;

 pSpuBuffer=(unsigned char *)malloc(32768);            // alloc mixing buffer

 i=NSSIZE*2;

 sRVBStart[0] = (int *)malloc(i*4);                    // alloc reverb buffer
 memset(sRVBStart[0],0,i*4);
 sRVBEnd[0]  = sRVBStart[0] + i;
 sRVBPlay[0] = sRVBStart[0];
 sRVBStart[1] = (int *)malloc(i*4);                    // alloc reverb buffer
 memset(sRVBStart[1],0,i*4);
 sRVBEnd[1]  = sRVBStart[1] + i;
 sRVBPlay[1] = sRVBStart[1];

 for(i=0;i<MAXCHAN;i++)                                // loop sound channels
  {
// we don't use mutex sync... not needed, would only 
// slow us down:
//   s_chan[i].hMutex=CreateMutex(NULL,FALSE,NULL);
   s_chan[i].ADSRX.SustainLevel = 1024;                // -> init sustain
   s_chan[i].iMute=0;
   s_chan[i].iIrqDone=0;
   s_chan[i].pLoop=spuMemC;
   s_chan[i].pStart=spuMemC;
   s_chan[i].pCurr=spuMemC;
  }
}

////////////////////////////////////////////////////////////////////////
// REMOVESTREAMS: free most buffer
////////////////////////////////////////////////////////////////////////

static void RemoveStreams(void)
{ 
 free(pSpuBuffer);                                     // free mixing buffer
 pSpuBuffer=NULL;
 free(sRVBStart[0]);                                   // free reverb buffer
 sRVBStart[0]=0;
 free(sRVBStart[1]);                                   // free reverb buffer
 sRVBStart[1]=0;

/*
 int i;
 for(i=0;i<MAXCHAN;i++)
  {
   WaitForSingleObject(s_chan[i].hMutex,2000);
   ReleaseMutex(s_chan[i].hMutex);
   if(s_chan[i].hMutex)    
    {CloseHandle(s_chan[i].hMutex);s_chan[i].hMutex=0;}
  }
*/
}


////////////////////////////////////////////////////////////////////////
// SPUOPEN: called by main emu after init
////////////////////////////////////////////////////////////////////////
   
EXPORT_GCC long CALLBACK SPU2open(void *pDsp)                          
{
 if(bSPUIsOpen) return 0;                              // security for some stupid main emus

 iUseXA=0;                                             // just small setup
 iVolume=3;
 bEndThread=0;
 bThreadEnded=0;
 spuMemC=(unsigned char *)spuMem;      
 memset((void *)s_chan,0,(MAXCHAN+1)*sizeof(SPUCHAN));
 pSpuIrq[0]=0;
 pSpuIrq[1]=0;
 iSPUIRQWait=1;
 dwNewChannel2[0]=0;
 dwNewChannel2[1]=0;
 dwEndChannel2[0]=0;
 dwEndChannel2[1]=0;
 spuCtrl2[0]=0;              
 spuCtrl2[1]=0;              
 spuStat2[0]=0;
 spuStat2[1]=0;
 spuIrq2[0]=0;             
 spuIrq2[1]=0;             
 spuAddr2[0]=0xffffffff;   
 spuAddr2[1]=0xffffffff;   
 spuRvbAddr2[0]=0;
 spuRvbAddr2[1]=0;
 spuRvbAEnd2[0]=0;
 spuRvbAEnd2[1]=0;
 
// ReadConfig();                                         // read user stuff
 
// SetupSound();                                         // setup midas (before init!)

 SetupStreams();                                       // prepare streaming

 SetupTimer();                                         // timer for feeding data

 bSPUIsOpen=1;

 return 0;        
}

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// SPUCLOSE: called before shutdown
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2close(void)
{
 if(!bSPUIsOpen) return;                               // some security

 bSPUIsOpen=0;                                         // no more open

 RemoveTimer();                                        // no more feeding

// RemoveSound();                                        // no more sound handling

 RemoveStreams();                                      // no more streaming
}

////////////////////////////////////////////////////////////////////////
// SPUSHUTDOWN: called by main emu on final exit
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2shutdown(void)
{
 return;
}

////////////////////////////////////////////////////////////////////////
// SPUTEST: we don't test, we are always fine ;)
////////////////////////////////////////////////////////////////////////

EXPORT_GCC long CALLBACK SPU2test(void)
{
 return 0;
}

////////////////////////////////////////////////////////////////////////
// SETUP CALLBACKS
// this functions will be called once, 
// passes a callback that should be called on SPU-IRQ/cdda volume change
////////////////////////////////////////////////////////////////////////

// not used yet
EXPORT_GCC void CALLBACK SPU2irqCallback(void (CALLBACK *callback)(void))
{
 irqCallback = callback;
}

// not used yet
EXPORT_GCC void CALLBACK SPU2registerCallback(void (CALLBACK *callback)(void))
{
 irqCallback = callback;
}

// not used yet
EXPORT_GCC void CALLBACK SPU2registerCDDAVolume(void (CALLBACK *CDDAVcallback)(unsigned short,unsigned short))
{
 cddavCallback = CDDAVcallback;
}
