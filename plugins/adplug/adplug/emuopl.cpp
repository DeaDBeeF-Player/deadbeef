/*
 * AdPlug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * emuopl.cpp - Emulated OPL, by Simon Peter <dn.tlp@gmx.net>
 */

#include "emuopl.h"

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

CEmuopl::CEmuopl(int rate, bool bit16, bool usestereo)
  : use16bit(bit16), stereo(usestereo), mixbufSamples(0)
{
  opl[0] = OPLCreate(OPL_TYPE_YM3812, 3579545, rate);
  opl[1] = OPLCreate(OPL_TYPE_YM3812, 3579545, rate);
  mixbuf0 = 0;
  mixbuf1 = 0;

  currType = TYPE_DUAL_OPL2;

  init();
}

CEmuopl::~CEmuopl()
{
  OPLDestroy(opl[0]); OPLDestroy(opl[1]);

  delete [] mixbuf0;
  delete [] mixbuf1;
}

void CEmuopl::update(short *buf, int samples)
{
  int i;

  //ensure that our mix buffers are adequately sized
  if(unlikely (mixbufSamples < samples)) {

      if (mixbuf0) {
          delete[] mixbuf0;
          mixbuf0 = 0;
      }
      if (mixbuf1) {
          delete[] mixbuf1;
          mixbuf1 = 0;
      }
    //*2 = make room for stereo, if we need it
    mixbuf0 = new short[samples*2]; 
    mixbuf1 = new short[samples*2];
  }

  mixbufSamples = samples;
  //data should be rendered to outbuf
  //tempbuf should be used as a temporary buffer
  //if we are supposed to generate 16bit output,
  //then outbuf may point directly to the actual waveform output "buf"
  //if we are supposed to generate 8bit output,
  //then outbuf cannot point to "buf" (because there will not be enough room)
  //and so it must point to a mixbuf instead--
  //it will be reduced to 8bit and put in "buf" later
  short *outbuf;
  if (likely (use16bit)) {
      outbuf = buf;
  }
  else {
      outbuf = mixbuf1;
  }
  //...there is a potentially confusing situation where mixbuf1 can be aliased.
  //beware. it is a little loony.

  //all of the following rendering code produces 16bit output

  switch(currType) {
  case TYPE_OPL2:
    //for opl2 mode:
    //render chip0 to the output buffer
    YM3812UpdateOne(opl[0],outbuf,samples);

    //if we are supposed to output stereo,
    //then we need to dup the mono channel
    if(unlikely (stereo)) {
        for(i=samples-1;i>=0;i--) {
            outbuf[i*2] = outbuf[i];
            outbuf[i*2+1] = outbuf[i];
        }
    }
    break;

  case TYPE_OPL3:	// unsupported
    break;

  case TYPE_DUAL_OPL2:
    //for dual opl2 mode:
    //render each chip to a different tempbuffer
    YM3812UpdateOne(opl[0],mixbuf1,samples);
    YM3812UpdateOne(opl[1],mixbuf0,samples);

    //output stereo:
    //then we need to interleave the two buffers
    if(unlikely (stereo)){
        //first, spread tempbuf's samples across left channel
        for(i=0;i<samples;i++)
            outbuf[i*2] = mixbuf1[i];
        //next, insert the samples from tempbuf2 into right channel
        for(i=0;i<samples;i++)
            outbuf[i*2+1] = mixbuf0[i];
    } else {
        //output mono:
        //then we need to mix the two buffers into buf
        for(i=0;i<samples;i++) {
            int sample = (int)mixbuf0[i] + (int)mixbuf1[i];
            if (unlikely (sample > 32767)) {
                sample = 32767;
            }
            else if (unlikely (sample < -32768)) {
                sample = -32768;
            }
            outbuf[i] = (short)sample;
        }
    }
    break;
  }

  //now reduce to 8bit if we need to
  if(unlikely (!use16bit)) {
      int smp = stereo ? samples*2 : samples;
      for(i=0;i<smp;i++) {
          ((char *)buf)[i] = (outbuf[i] >> 8) ^ 0x80;
      }
  }
}

void CEmuopl::write(int reg, int val)
{
  switch(currType){
  case TYPE_DUAL_OPL2:
    OPLWrite(opl[1], 0, reg);
    OPLWrite(opl[1], 1, val);
  case TYPE_OPL2:
    OPLWrite(opl[0], 0, reg);
    OPLWrite(opl[0], 1, val);
    break;
  case TYPE_OPL3:	// unsupported
    break;
  }
}

void CEmuopl::init()
{
  OPLResetChip(opl[0]); OPLResetChip(opl[1]);
  currChip = 0;
}

void CEmuopl::settype(ChipType type)
{
  currType = type;
}
