/*
 *                          sc68 - MFP 68901 emulator
 *           Copyright (C) 1999 Benjamin Gerard <ben@sashipa.com>
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

#include "io68/mfp_io.h"
#include "io68/mfpemul.h"

/* 0  GPIP   General purpose I/O */
static u8 mfpr_01(cycle68_t cycle) { cycle=cycle; return mfp[0x01]; }
/* 1  AER    Active edge register */
static u8 mfpr_03(cycle68_t cycle) { cycle=cycle; return mfp[0x03]; }
/* 2  DDR    Data direction register */
static u8 mfpr_05(cycle68_t cycle) { cycle=cycle; return mfp[0x05]; }      
/* 3  IERA   Interrupt enable register A */
static u8 mfpr_07(cycle68_t cycle) { cycle=cycle; return mfp[0x07]; }
/* 4  IERB   Interrupt enable register B */
static u8 mfpr_09(cycle68_t cycle) { cycle=cycle; return mfp[0x09]; }
/* 5  IPRA   Interrupt pending register A */
static u8 mfpr_0B(cycle68_t cycle) { cycle=cycle; return mfp[0x0B]; }
/* 6  IPRB   Interrupt pending register B */
static u8 mfpr_0D(cycle68_t cycle) { cycle=cycle; return mfp[0x0D]; }
/* 7  ISRA   Interrupt in-service register A */
static u8 mfpr_0F(cycle68_t cycle) { cycle=cycle; return mfp[0x0F]; }
/* 8  ISRB   Interrupt in-service register B */
static u8 mfpr_11(cycle68_t cycle) { cycle=cycle; return mfp[0x11]; }
/* 9  IMRA   Interrupt mask register A */
static u8 mfpr_13(cycle68_t cycle) { cycle=cycle; return mfp[0x13]; }
/* A  IMRB   Interrupt mask register B */
static u8 mfpr_15(cycle68_t cycle) { cycle=cycle; return mfp[0x15]; }
/* B  VR     Vector register */
static u8 mfpr_17(cycle68_t cycle) { cycle=cycle; return mfp[0x17]; }
/* C  TACR   Timer A control register */
static u8 mfpr_19(cycle68_t cycle) { cycle=cycle; return mfp[0x19]; }
/* D  TBCR   Timer B control register */
static u8 mfpr_1B(cycle68_t cycle) { cycle=cycle; return mfp[0x1B]; }
/* E  TCDCR  Timers C and D control registers */
static u8 mfpr_1D(cycle68_t cycle) { cycle=cycle; return mfp[0x1D]; }
/* F  TADR   Timer A data register */
static u8 mfpr_1F(cycle68_t cycle) { return MFP_getTDR(TIMER_A, cycle); }
/* 10 TBDR   Timer B data register */
static u8 mfpr_21(cycle68_t cycle) { return MFP_getTDR(TIMER_B, cycle); }
/* 11 TCDR   Timer C data register */
static u8 mfpr_23(cycle68_t cycle) { return MFP_getTDR(TIMER_C, cycle); }
/* 12 TDDR   Timer D data register */
static u8 mfpr_25(cycle68_t cycle) { return MFP_getTDR(TIMER_D, cycle); }
/* 13 SCR    Sync character register */
static u8 mfpr_27(cycle68_t cycle) { cycle=cycle; return mfp[0x27]; }
/* 14 UCR    USART control register */
static u8 mfpr_29(cycle68_t cycle) { cycle=cycle; return mfp[0x29]; }
/* 15 RSR    Receiver status register */
static u8 mfpr_2B(cycle68_t cycle) { cycle=cycle; return mfp[0x2B]; }
/* 16 TSR    Transmitter status register */
static u8 mfpr_2D(cycle68_t cycle) { cycle=cycle; return mfp[0x2D]; }
/* 17 UDR    USART data register */
static u8 mfpr_2F(cycle68_t cycle) { cycle=cycle; return mfp[0x2F]; }

static u8 mfpr_31(cycle68_t cycle) { cycle=cycle; return mfp[0x31]; }
static u8 mfpr_33(cycle68_t cycle) { cycle=cycle; return mfp[0x33]; }
static u8 mfpr_35(cycle68_t cycle) { cycle=cycle; return mfp[0x35]; }
static u8 mfpr_37(cycle68_t cycle) { cycle=cycle; return mfp[0x37]; }
static u8 mfpr_39(cycle68_t cycle) { cycle=cycle; return mfp[0x39]; }
static u8 mfpr_3B(cycle68_t cycle) { cycle=cycle; return mfp[0x3B]; }
static u8 mfpr_3D(cycle68_t cycle) { cycle=cycle; return mfp[0x3D]; }
static u8 mfpr_3F(cycle68_t cycle) { cycle=cycle; return mfp[0x3F]; }


static void mfpw_01(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x01]=v; }
static void mfpw_03(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x03]=v; }
static void mfpw_05(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x05]=v; }
static void mfpw_07(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x07]=v; }
static void mfpw_09(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x09]=v; }
static void mfpw_0B(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x0B]=v; }
static void mfpw_0D(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x0D]=v; }
static void mfpw_0F(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x0F]=v; }
static void mfpw_11(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x11]=v; }
static void mfpw_13(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x13]=v; }
static void mfpw_15(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x15]=v; }
static void mfpw_17(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x17]=v; }
static void mfpw_19(u8 v, cycle68_t cycle) { MFP_putTCR(TIMER_A, v, cycle); }
static void mfpw_1B(u8 v, cycle68_t cycle) { MFP_putTCR(TIMER_B, v, cycle); }
static void mfpw_1D(u8 v, cycle68_t cycle) { MFP_putTCR(TIMER_C, v, cycle); }
static void mfpw_1F(u8 v, cycle68_t cycle) { MFP_putTDR(TIMER_A, v, cycle); }
static void mfpw_21(u8 v, cycle68_t cycle) { MFP_putTDR(TIMER_B, v, cycle); }
static void mfpw_23(u8 v, cycle68_t cycle) { MFP_putTDR(TIMER_C, v, cycle); }
static void mfpw_25(u8 v, cycle68_t cycle) { MFP_putTDR(TIMER_D, v, cycle); }
static void mfpw_27(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x27]=v; }
static void mfpw_29(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x29]=v; }
static void mfpw_2B(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x2B]=v; }
static void mfpw_2D(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x2D]=v; }
static void mfpw_2F(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x2F]=v; }
static void mfpw_31(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x31]=v; }
static void mfpw_33(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x33]=v; }
static void mfpw_35(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x35]=v; }
static void mfpw_37(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x37]=v; }
static void mfpw_39(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x39]=v; }
static void mfpw_3B(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x3B]=v; }
static void mfpw_3D(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x3D]=v; }
static void mfpw_3F(u8 v, cycle68_t cycle) { cycle=cycle; mfp[0x3F]=v; }

/* MFP read jump table */
static u8 (*mfpr_func[32])(cycle68_t) =
{
  mfpr_01,mfpr_03,mfpr_05,mfpr_07,
  mfpr_09,mfpr_0B,mfpr_0D,mfpr_0F,
  mfpr_11,mfpr_13,mfpr_15,mfpr_17,
  mfpr_19,mfpr_1B,mfpr_1D,mfpr_1F,
  mfpr_21,mfpr_23,mfpr_25,mfpr_27,
  mfpr_29,mfpr_2B,mfpr_2D,mfpr_2F,
  mfpr_31,mfpr_33,mfpr_35,mfpr_37,
  mfpr_39,mfpr_3B,mfpr_3D,mfpr_3F,
};

/* MFP write jump table */
static void (*mfpw_func[32])(u8, cycle68_t) =
{
  mfpw_01,mfpw_03,mfpw_05,mfpw_07,
  mfpw_09,mfpw_0B,mfpw_0D,mfpw_0F,
  mfpw_11,mfpw_13,mfpw_15,mfpw_17,
  mfpw_19,mfpw_1B,mfpw_1D,mfpw_1F,
  mfpw_21,mfpw_23,mfpw_25,mfpw_27,
  mfpw_29,mfpw_2B,mfpw_2D,mfpw_2F,
  mfpw_31,mfpw_33,mfpw_35,mfpw_37,
  mfpw_39,mfpw_3B,mfpw_3D,mfpw_3F,
};

static u32 mfp_readB(u32 addr, cycle68_t cycle)
{
  cycle=cycle;
  if (!(addr&1)) {
    return 0;
  }
  addr &= 0x3F;
  addr>>=1;
  return (*mfpr_func[addr])(cycle);
}

static u32 mfp_readW(u32 addr, cycle68_t cycle)
{
  /* Expected addr is EVEN becoz of 16 bit access */
  return mfp_readB(addr+1, cycle);
}

static u32 mfp_readL(u32 addr, cycle68_t cycle)
{
  /* Expected addr is EVEN becoz of 32 bit access */
  return (mfp_readB(addr+1, cycle)<<16) | mfp_readB(addr+3, cycle+4);
}

static void mfp_writeB(u32 addr, u32 v, cycle68_t cycle)
{
  addr=addr; cycle=cycle;
  if (!(addr&1)) {
    return;
  }
  addr &= 0x3F;
  addr>>=1;
  (*mfpw_func[addr])((u8)v,cycle);
}

static void mfp_writeW(u32 addr, u32 v, cycle68_t cycle)
{
  /* Expected addr is EVEN becoz of 16 bit access */
  mfp_writeB(addr+1, v, cycle);
}

static void mfp_writeL(u32 addr, u32 v, cycle68_t cycle)
{
  /* Expected addr is EVEN becoz of 32 bit access */
  mfp_writeB(addr+1, v>>16, cycle);
  mfp_writeB(addr+3, v, cycle+4);
}

static int mfp_reset(void)
{
  MFP_reset();
  return 0;
}

io68_t mfp_io =
{
  0,
  "MFP-68901",
  0xFFFFFA00, 0xFFFFFA2F,
  {mfp_readB,mfp_readW,mfp_readL},
  {mfp_writeB,mfp_writeW,mfp_writeL},
  MFP_interrupt,MFP_nextinterrupt,
  MFP_subcycle,
  mfp_reset,
  0,0
};
