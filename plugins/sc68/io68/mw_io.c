/*
 *                sc68 - MicroWire IO plugin (STE soundchip)
 *         Copyright (C) 1999 Benjamin Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "io68/mw_io.h"
#include "io68/mwemul.h"

static u32 mw_readB(u32 addr, cycle68_t cycle)
{
  cycle=cycle;
  addr &= 0x3F;

  /* Not Micro-Wire Ctrl/Data */
  if (addr < MW_DATA) {
    /* Even line */
    if (!(addr&1)) {
      return 0;
    }

    /* Sample counter */
    if(addr>=MW_CTH && addr<=MW_CTL) {
      //      BREAKPOINT68
      addr -= MW_CTH;           /* 0, 2, 4 */
      addr = 24 - (addr<<2);    /* 24, 16, 8 */
      return (u32)(u8)(mw_ct>>addr);
    }
  }

  /* Micro-Wire Ctrl/Data */
  return mw[addr];
}

static u32 mw_readW(u32 addr, cycle68_t cycle)
{
  return
    (mw_readB(addr+0,cycle)<<8)
    | mw_readB(addr+1,cycle);
}

static u32 mw_readL(u32 addr, cycle68_t cycle)
{
  return
    (mw_readB(addr+0,cycle) << 24)
    | (mw_readB(addr+1,cycle) << 16)
    | (mw_readB(addr+2,cycle+4) << 8)
    | mw_readB(addr+3,cycle+4);
}

#define TO_INTERNAL(N) ((mw[N]<<24) + (mw[(N)+2]<<16)+ (mw[(N)+4]<<8))

static void mw_command(int n)
{
  int c=n&0700;

  n-=c;
  switch(c) {
  case 0000:
    MW_set_LMC_mixer(n&3);
    /*      printf("Mixer = %d\n",n&3);*/
    break;
  case 0100:
    MW_set_LMC_low(n&15);
    /*      printf("Low = %d\n",n&15);*/
    break;
  case 0200:
    MW_set_LMC_high(n&15);
    /*      printf("High = %d\n",n&15);*/
    break;
  case 0300:
    MW_set_LMC_master(n&63);
    /*      printf("Master = %d\n",n&63);*/
    break;
  case 0400:
    MW_set_LMC_right(n&31);
    /*      printf("Right = %d\n",n&31);*/
    break;
  case 0500:
    MW_set_LMC_left(n&31);
    /*      printf("Left = %d\n",n&31);*/
    break;
  }
}

static void mw_writeB(u32 addr, u32 v, cycle68_t cycle)
{
  cycle=cycle;
  addr &= 0x3F;

  /* Not Micro-Wire Ctrl/Data */
  if(addr<MW_DATA) {
    /* Even line */
    if(!(addr&1)) {
      return;
    }

    /* ACTIV register */
    if (addr==MW_ACTI) {
      v &= 3;

      /* Reload internal counter */
      mw_ct  = TO_INTERNAL(MW_BASH);
      mw_end = TO_INTERNAL(MW_ENDH);

      /*printf("lauch: %x %x\n",mw_ct,mw_end);*/
      mw[MW_ACTI] = v;
      return;
    }

    /* Sample counter (RO) */
    if(addr>=MW_CTH && addr<=MW_CTL) {
      return;
    }

    mw[addr] = v;
    return;
  }

  /* Micro-Wire Data */
  if(addr<MW_CTRL) {
    u32 ctrl,data,adr;
    mw[addr] = v;

    /* Write in high : skip */
    if(addr == MW_DATA) {
      return;
    }

    /* Write in low : process */
    ctrl = (mw[MW_CTRL]<<8) + mw[MW_CTRL+1];
    data = (mw[MW_DATA]<<8) + mw[MW_DATA+1];

    /* Find first adress */
    for(; ctrl && (ctrl&0xC000)!=0xC000 ; ctrl<<=1, data<<=1)
      ;
    adr = (data>>14) & 3;
    if (adr != 2) {
      return;
    }

    for (ctrl<<=2, data<<=2; ctrl && (ctrl&0xFF80) != 0xFF80;
	 ctrl<<=1, data<<=1)
      ;
    if (ctrl) {
      mw_command((data>>7) & 0x1ff);
    }
    return;
  }

  /* Micro-Wire Ctrl */
  mw[addr] = v;
}

static void mw_writeW(u32 addr, u32 v, cycle68_t cycle)
{
  mw_writeB(addr+0,v>>8,cycle);
  mw_writeB(addr+1,v   ,cycle);
}

static void mw_writeL(u32 addr, u32 v, cycle68_t cycle)
{
  mw_writeB(addr+0,v>>24,cycle);
  mw_writeB(addr+1,v>>16,cycle);
  mw_writeB(addr+2,v>>8 ,cycle);
  mw_writeB(addr+3,v    ,cycle);
}

static int68_t *mw_int(cycle68_t cycle)
{
  cycle = cycle;
  return 0L;
}

static cycle68_t mw_nextint(cycle68_t cycle)
{
  cycle = cycle;
  return IO68_NO_INT;
}

static int mw_reset(void)
{
  MW_reset();
  return 0;
}

static void mw_subcycle(cycle68_t cycle)
{
  cycle=cycle;
}


io68_t mw_io =
  {
    0,
    "STE-MicroWire",
    0xFFFF8900, 0xFFFF8925,
    {mw_readB,  mw_readW,  mw_readL},
    {mw_writeB, mw_writeW, mw_writeL},
    mw_int, mw_nextint,
    mw_subcycle,
    mw_reset,
    0,0
  };
