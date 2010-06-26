/***************************************************************************
                         registers.c  -  description
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

/* ChangeLog 

 February 8, 2004	-	xodnizel
 - Fixed setting of reverb volume.  Just typecast val("u16") to s16.
   Also adjusted the normal channel volume to be one less than what it was before when the 
   "phase invert" bit is set.  I'm assuming it's just in two's complement.

 2003/02/09 - kode54
 - removed &0x3fff from reverb volume registers, fixes a few games,
   hopefully won't be breaking anything

 2003/01/19 - Pete
 - added Neill's reverb

 2003/01/06 - Pete
 - added Neill's ADSR timings

 2002/05/15 - Pete
 - generic cleanup for the Peops release

*/

#include "stdafx.h"

#define _IN_REGISTERS

#include "../peops/externals.h"
#include "../peops/registers.h"
#include "../peops/regs.h"

static void SoundOn(int start,int end,u16 val);
static void SoundOff(int start,int end,u16 val);
static void FModOn(int start,int end,u16 val);
static void NoiseOn(int start,int end,u16 val);
static void SetVolumeLR(int right, u8 ch,s16 vol);
static void SetPitch(int ch,u16 val);

////////////////////////////////////////////////////////////////////////
// WRITE REGISTERS: called by main emu
////////////////////////////////////////////////////////////////////////

void SPUwriteRegister(u32 reg, u16 val)
{
 const u32 r=reg&0xfff;
 regArea[(r-0xc00)>>1] = val;

// printf("SPUwrite: r %x val %x\n", r, val);

 if(r>=0x0c00 && r<0x0d80)                             // some channel info?
  {
   int ch=(r>>4)-0xc0;                                 // calc channel

   //if(ch==20) printf("%08x: %04x\n",reg,val);

   switch(r&0x0f)
    {
     //------------------------------------------------// r volume
     case 0:                                           
       SetVolumeLR(0,(u8)ch,val);
       break;
     //------------------------------------------------// l volume
     case 2:                                           
       SetVolumeLR(1,(u8)ch,val);
       break;
     //------------------------------------------------// pitch
     case 4:                                           
       SetPitch(ch,val);
       break;
     //------------------------------------------------// start
     case 6:      
       s_chan[ch].pStart=spuMemC+((u32) val<<3);
       break;
     //------------------------------------------------// level with pre-calcs
     case 8:
       {
        const u32 lval=val; // DEBUG CHECK
        //---------------------------------------------//
        s_chan[ch].ADSRX.AttackModeExp=(lval&0x8000)?1:0; 
        s_chan[ch].ADSRX.AttackRate=(lval>>8) & 0x007f;
        s_chan[ch].ADSRX.DecayRate=(lval>>4) & 0x000f;
        s_chan[ch].ADSRX.SustainLevel=lval & 0x000f;
        //---------------------------------------------//
      }
      break;
     //------------------------------------------------// adsr times with pre-calcs
     case 10:
      {
       const u32 lval=val; // DEBUG CHECK

       //----------------------------------------------//
       s_chan[ch].ADSRX.SustainModeExp = (lval&0x8000)?1:0;
       s_chan[ch].ADSRX.SustainIncrease= (lval&0x4000)?0:1;
       s_chan[ch].ADSRX.SustainRate = (lval>>6) & 0x007f;
       s_chan[ch].ADSRX.ReleaseModeExp = (lval&0x0020)?1:0;
       s_chan[ch].ADSRX.ReleaseRate = lval & 0x001f;
       //----------------------------------------------//
      }
     break;
     //------------------------------------------------// adsr volume... mmm have to investigate this
     //case 0xC:
     //  break;
     //------------------------------------------------//
     case 0xE:                                          // loop?
       s_chan[ch].pLoop=spuMemC+((u32) val<<3);
       s_chan[ch].bIgnoreLoop=1;
       break;
     //------------------------------------------------//
    }
   return;
  }

 switch(r)
   {
    //-------------------------------------------------//
    case H_SPUaddr:
      spuAddr = (u32) val<<3;
      break;
    //-------------------------------------------------//
    case H_SPUdata:
      spuMem[spuAddr>>1] = BFLIP16(val);
      spuAddr+=2;
      if(spuAddr>0x7ffff) spuAddr=0;
      break;
    //-------------------------------------------------//
    case H_SPUctrl:
      spuCtrl=val;
      break;
    //-------------------------------------------------//
    case H_SPUstat:
      spuStat=val & 0xf800;
      break;
    //-------------------------------------------------//
    case H_SPUReverbAddr:
      if(val==0xFFFF || val<=0x200)
       {rvb.StartAddr=rvb.CurrAddr=0;}
      else
       {
        const s32 iv=(u32)val<<2;
        if(rvb.StartAddr!=iv)
         {
          rvb.StartAddr=(u32)val<<2;
          rvb.CurrAddr=rvb.StartAddr;
         }
       }
      break;
    //-------------------------------------------------//
    case H_SPUirqAddr:
      spuIrq = val;
      pSpuIrq=spuMemC+((u32) val<<3);
      break;
    //-------------------------------------------------//
    /* Volume settings appear to be at least 15-bit unsigned in this case.  
       Definitely NOT 15-bit signed.  Probably 16-bit signed, so s16 type cast.
       Check out "Chrono Cross:  Shadow's End Forest"
    */
    case H_SPUrvolL:
      rvb.VolLeft=(s16)val;
      //printf("%d\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUrvolR:
      rvb.VolRight=(s16)val;
      //printf("%d\n",val);
      break;
    //-------------------------------------------------//

/*
    case H_ExtLeft:
     //auxprintf("EL %d\n",val);
      break;
    //-------------------------------------------------//
    case H_ExtRight:
     //auxprintf("ER %d\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUmvolL:
     //auxprintf("ML %d\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUmvolR:
     //auxprintf("MR %d\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUMute1:
     //printf("M0 %04x\n",val);
      break;
    //-------------------------------------------------//
    case H_SPUMute2:
    // printf("M1 %04x\n",val);
      break;
*/
    //-------------------------------------------------//
    case H_SPUon1:
      SoundOn(0,16,val);
      break;
    //-------------------------------------------------//
     case H_SPUon2:
	// printf("Boop: %08x: %04x\n",reg,val);
      SoundOn(16,24,val);
      break;
    //-------------------------------------------------//
    case H_SPUoff1:
      SoundOff(0,16,val);
      break;
    //-------------------------------------------------//
    case H_SPUoff2:
      SoundOff(16,24,val);
	// printf("Boop: %08x: %04x\n",reg,val);
      break;
    //-------------------------------------------------//
    case H_FMod1:
      FModOn(0,16,val);
      break;
    //-------------------------------------------------//
    case H_FMod2:
      FModOn(16,24,val);
      break;
    //-------------------------------------------------//
    case H_Noise1:
      NoiseOn(0,16,val);
      break;
    //-------------------------------------------------//
    case H_Noise2:
      NoiseOn(16,24,val);
      break;
    //-------------------------------------------------//
    case H_RVBon1:
      rvb.Enabled&=~0xFFFF;
      rvb.Enabled|=val;
      break;

    //-------------------------------------------------//
    case H_RVBon2:
      rvb.Enabled&=0xFFFF;
      rvb.Enabled|=val<<16;
      break;

    //-------------------------------------------------//
    case H_Reverb+0:
      rvb.FB_SRC_A=val;
      break;

    case H_Reverb+2   : rvb.FB_SRC_B=(s16)val;       break;
    case H_Reverb+4   : rvb.IIR_ALPHA=(s16)val;      break;
    case H_Reverb+6   : rvb.ACC_COEF_A=(s16)val;     break;
    case H_Reverb+8   : rvb.ACC_COEF_B=(s16)val;     break;
    case H_Reverb+10  : rvb.ACC_COEF_C=(s16)val;     break;
    case H_Reverb+12  : rvb.ACC_COEF_D=(s16)val;     break;
    case H_Reverb+14  : rvb.IIR_COEF=(s16)val;       break;
    case H_Reverb+16  : rvb.FB_ALPHA=(s16)val;       break;
    case H_Reverb+18  : rvb.FB_X=(s16)val;           break;
    case H_Reverb+20  : rvb.IIR_DEST_A0=(s16)val;    break;
    case H_Reverb+22  : rvb.IIR_DEST_A1=(s16)val;    break;
    case H_Reverb+24  : rvb.ACC_SRC_A0=(s16)val;     break;
    case H_Reverb+26  : rvb.ACC_SRC_A1=(s16)val;     break;
    case H_Reverb+28  : rvb.ACC_SRC_B0=(s16)val;     break;
    case H_Reverb+30  : rvb.ACC_SRC_B1=(s16)val;     break;
    case H_Reverb+32  : rvb.IIR_SRC_A0=(s16)val;     break;
    case H_Reverb+34  : rvb.IIR_SRC_A1=(s16)val;     break;
    case H_Reverb+36  : rvb.IIR_DEST_B0=(s16)val;    break;
    case H_Reverb+38  : rvb.IIR_DEST_B1=(s16)val;    break;
    case H_Reverb+40  : rvb.ACC_SRC_C0=(s16)val;     break;
    case H_Reverb+42  : rvb.ACC_SRC_C1=(s16)val;     break;
    case H_Reverb+44  : rvb.ACC_SRC_D0=(s16)val;     break;
    case H_Reverb+46  : rvb.ACC_SRC_D1=(s16)val;     break;
    case H_Reverb+48  : rvb.IIR_SRC_B1=(s16)val;     break;
    case H_Reverb+50  : rvb.IIR_SRC_B0=(s16)val;     break;
    case H_Reverb+52  : rvb.MIX_DEST_A0=(s16)val;    break;
    case H_Reverb+54  : rvb.MIX_DEST_A1=(s16)val;    break;
    case H_Reverb+56  : rvb.MIX_DEST_B0=(s16)val;    break;
    case H_Reverb+58  : rvb.MIX_DEST_B1=(s16)val;    break;
    case H_Reverb+60  : rvb.IN_COEF_L=(s16)val;      break;
    case H_Reverb+62  : rvb.IN_COEF_R=(s16)val;      break;
   }

}

////////////////////////////////////////////////////////////////////////
// READ REGISTER: called by main emu
////////////////////////////////////////////////////////////////////////

u16 SPUreadRegister(u32 reg)
{
 const u32 r=reg&0xfff;

 if(r>=0x0c00 && r<0x0d80)
  {
   switch(r&0x0f)
    {
     case 0xC:                                          // get adsr vol
      {
       const int ch=(r>>4)-0xc0;
       if(s_chan[ch].bNew) return 1;                   // we are started, but not processed? return 1
       if(s_chan[ch].ADSRX.lVolume &&                  // same here... we haven't decoded one sample yet, so no envelope yet. return 1 as well
          !s_chan[ch].ADSRX.EnvelopeVol)                   
        return 1;
       return (u16)(s_chan[ch].ADSRX.EnvelopeVol>>16);
      }

     case 0xE:                                          // get loop address
      {
       const int ch=(r>>4)-0xc0;
       if(s_chan[ch].pLoop==NULL) return 0;
       return (u16)((s_chan[ch].pLoop-spuMemC)>>3);
      }
    }
  }

 switch(r)
  {
    case H_SPUctrl:
     return spuCtrl;

    case H_SPUstat:
     return spuStat;
        
    case H_SPUaddr:
     return (u16)(spuAddr>>3);

    case H_SPUdata:
     {
      u16 s=BFLIP16(spuMem[spuAddr>>1]);
      spuAddr+=2;
      if(spuAddr>0x7ffff) spuAddr=0;
      return s;
     }

    case H_SPUirqAddr:
     return spuIrq;

    //case H_SPUIsOn1:
    // return IsSoundOn(0,16);

    //case H_SPUIsOn2:
    // return IsSoundOn(16,24);
 
  }

 return regArea[(r-0xc00)>>1];
}
 
////////////////////////////////////////////////////////////////////////
// SOUND ON register write
////////////////////////////////////////////////////////////////////////

static void SoundOn(int start,int end,u16 val)     // SOUND ON PSX COMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if((val&1) && s_chan[ch].pStart)                    // mmm... start has to be set before key on !?!
    {
     s_chan[ch].bIgnoreLoop=0;
     s_chan[ch].bNew=1;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// SOUND OFF register write
////////////////////////////////////////////////////////////////////////

static void SoundOff(int start,int end,u16 val)    // SOUND OFF PSX COMMAND
{
 int ch;
 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // && s_chan[i].bOn)  mmm...
    {
     s_chan[ch].bStop=1;
    }                                                  
  }
}

////////////////////////////////////////////////////////////////////////
// FMOD register write
////////////////////////////////////////////////////////////////////////

static void FModOn(int start,int end,u16 val)      // FMOD ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> fmod on/off
    {
     if(ch>0) 
      {
       s_chan[ch].bFMod=1;                             // --> sound channel
       s_chan[ch-1].bFMod=2;                           // --> freq channel
      }
    }
   else
    {
     s_chan[ch].bFMod=0;                               // --> turn off fmod
    }
  }
}

////////////////////////////////////////////////////////////////////////
// NOISE register write
////////////////////////////////////////////////////////////////////////

static void NoiseOn(int start,int end,u16 val)     // NOISE ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> noise on/off
    {
     s_chan[ch].bNoise=1;
    }
   else 
    {
     s_chan[ch].bNoise=0;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// LEFT VOLUME register write
////////////////////////////////////////////////////////////////////////

// please note: sweep is wrong.

static void SetVolumeLR(int right, u8 ch,s16 vol)            // LEFT VOLUME
{
 //if(vol&0xc000)
 //printf("%d %08x\n",right,vol);
 if(right)
  s_chan[ch].iRightVolRaw=vol;
 else
  s_chan[ch].iLeftVolRaw=vol;

 if(vol&0x8000)                                        // sweep?
  {
   s16 sInc=1;                                       // -> sweep up?
   if(vol&0x2000) sInc=-1;                             // -> or down?
   if(vol&0x1000) vol^=0xffff;                         // -> mmm... phase inverted? have to investigate this
   vol=((vol&0x7f)+1)/2;                               // -> sweep: 0..127 -> 0..64
   vol+=vol/(2*sInc);                                  // -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
   vol*=128;
   vol&=0x3fff;
   //puts("Sweep");
  }
 else                                                  // no sweep:
  {
   if(vol&0x4000)
    vol=(vol&0x3FFF)-0x4000;
   else
    vol&=0x3FFF;

   //if(vol&0x4000)                                      // -> mmm... phase inverted? have to investigate this
   // vol=0-(0x3fff-(vol&0x3fff));
   //else
   // vol&=0x3fff;
  }
 if(right)
  s_chan[ch].iRightVolume=vol;
 else
  s_chan[ch].iLeftVolume=vol;                           // store volume
}

////////////////////////////////////////////////////////////////////////
// PITCH register write
////////////////////////////////////////////////////////////////////////

static void SetPitch(int ch,u16 val)               // SET PITCH
{
 int NP;
 if(val>0x3fff) NP=0x3fff;                             // get pitch val
 else           NP=val;

 s_chan[ch].iRawPitch=NP;

 NP=(44100L*NP)/4096L;                                 // calc frequency
 if(NP<1) NP=1;                                        // some security
 s_chan[ch].iActFreq=NP;                               // store frequency
}
