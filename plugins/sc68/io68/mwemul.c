/*
 *                 sc68 - MicroWire - STE soundchip emulator
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

/** @todo $$$
 *
 * - Stereo !!
 * - Check overflow in mix routine.
 * - Verify STE / YM volume ratio
 * - Special case for not mixing in Db_alone.
 * - And in the YM emul, skip emulation !!! 
 *
 */


#include <config_option68.h>

#define MW_CALCUL_TABLE 0

#if MW_CALCUL_TABLE
# include <math.h>   /* $$$ Calcul DB table */
# include <stdio.h>  /* $$$ For display table */
#endif


#include "io68/mwemul.h"

#define LN_10_OVER_10 0.230258509299
#define MW_N_DECIBEL 121

#define MW_MIX_FIX 10

#define MW_STE_MULT ((1<<MW_MIX_FIX)/4)
#define MW_YM_MULT  ((1<<MW_MIX_FIX)-MW_STE_MULT)

#define TO_INTERNAL(N) ((mw[N]<<24) + (mw[(N)+2]<<16)+ (mw[(N)+4]<<8))
#define TOINTERNAL(N) TO_INTERNAL(N)

static unsigned int replay_frq;
u8 mw[0x40];
struct
{
  int master, left, right, lr, high, low, mixer;
} lmc;

u32 mw_ct, mw_end;

#if MW_CALCUL_TABLE

static int Db_alone[MW_N_DECIBEL];
static int Db_mix[MW_N_DECIBEL];
static int Db_mix12[MW_N_DECIBEL];

#else

static const int Db_alone[MW_N_DECIBEL] = {
  0x40000,0x32d64,0x28619,0x20137,0x197a9,0x143d1,0x10137,0xcc50,
  0xa24b,0x80e9,0x6666,0x5156,0x409c,0x3352,0x28c4,0x2061,
  0x19b8,0x146e,0x103a,0xce4,0xa3d,0x822,0x676,0x521,
  0x413,0x33c,0x292,0x20b,0x19f,0x14a,0x106,0xd0,
  0xa5,0x83,0x68,0x52,0x41,0x34,0x29,0x21,
  0x1a,0x14,0x10,0xd,0xa,0x8,0x6,0x5,
  0x4,0x3,0x2,0x2,0x1,0x1,0x1,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0 
};

static const int Db_mix[MW_N_DECIBEL] = {
  0x10000,0xcb59,0xa186,0x804d,0x65ea,0x50f4,0x404d,0x3314,
  0x2892,0x203a,0x1999,0x1455,0x1027,0xcd4,0xa31,0x818,
  0x66e,0x51b,0x40e,0x339,0x28f,0x208,0x19d,0x148,
  0x104,0xcf,0xa4,0x82,0x67,0x52,0x41,0x34,
  0x29,0x20,0x1a,0x14,0x10,0xd,0xa,0x8,
  0x6,0x5,0x4,0x3,0x2,0x2,0x1,0x1,
  0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,
};

static const int Db_mix12[MW_N_DECIBEL] = {
  0x1027,0xcd4,0xa31,0x818,0x66e,0x51b,0x40e,0x339,
  0x28f,0x208,0x19d,0x148,0x104,0xcf,0xa4,0x82,
  0x67,0x52,0x41,0x34,0x29,0x20,0x1a,0x14,
  0x10,0xd,0xa,0x8,0x6,0x5,0x4,0x3,
  0x2,0x2,0x1,0x1,0x1,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0
};
#endif

static const int * Db_activ = Db_mix;

/******************************************************
*                  Set replay frequency               *
******************************************************/
unsigned int MW_sampling_rate(unsigned int f)
{
  if (f) {
    replay_frq = f;
  }
  return replay_frq;
}

/***********************************************************
*                                                          *
* Set master volume   0=-80 Db 40=0 Db                     *
* Set left volume     0=-40 Db 40=0 Db                     *
* Set right volume    0=-40 Db 40=0 Db                     *
* Set high frequency  0=-12 Db 40=12 Db                    *
* Set low  frequency  0=-12 Db 40=12 Db                    *
* Set mixer type : 0=-12 Db 1=YM+STE 2=STE only 3=reserved *
*                                                          *
***********************************************************/

void MW_set_LMC_mixer(unsigned int n)
{
  static const u32 *table[] = { Db_mix12, Db_mix, Db_alone };
  n&=3;
  lmc.mixer = n;
  if (n!=3) {
    Db_activ = table[n];
  }
}

void MW_set_LMC_master(unsigned int n)
{
  if (n>40u) n=40;
  lmc.master = -80+2*n;
}

void MW_set_LMC_left(unsigned int n)
{
  if (n>20u) n=20;
  lmc.left = -40+2*n;
  lmc.lr = (lmc.left+lmc.right)>>1;
}

void MW_set_LMC_right(unsigned int n)
{
  if (n>20u) n=20;
  lmc.right = -40+2*n;
  lmc.lr = (lmc.left+lmc.right)>>1;
}

void MW_set_LMC_high(unsigned int n)
{
  if (n>12u) n=12;
  lmc.high = -12+2*n;
}

void MW_set_LMC_low(unsigned int n)
{
  if (n>12u) n=12;
  lmc.low = -12+2*n;
}


/*

 A,B signal intensity
 1.Db = 10*LOG( A/B ) = 10*LN(A/B)/LN(10)
 => A = B * EXP( Decibel*LN(10)/10 )
 => A = B * R
    with R=EXP( Decibel*LN(10)/10 )

 R1,R2 rate of 2 signal for D1,D2 in decibel
 A = B*R1*R2  <=> B*R3
 with R3 = rate for (D1+D2) decibel

*/

#if MW_CALCUL_TABLE

static u32 calc_volume(s32 decibel, u32 mult)
{
  double r;
  r = exp( (double)decibel*LN_10_OVER_10 );
  r *= (double)mult;
  return (u32)r;
}

static void init_volume(void)
{
  int i;

  fprintf(stderr, "\n");
  for(i=0; i<MW_N_DECIBEL; i++) {
    Db_alone[i] = calc_volume(-i,256<<MW_MIX_FIX);
    Db_mix[i] = calc_volume(-i,MW_STE_MULT*256);
    Db_mix12[i] = calc_volume(-i-12,MW_STE_MULT*256);
    fprintf(stderr, "AAA:%x\n", Db_alone[i]);
    fprintf(stderr, "BBB:%x\n", Db_mix[i]);
    fprintf(stderr, "CCC:%x\n", Db_mix12[i]);
  }
}
#endif


/******************************************************
*                  Micro Wire reset                   *
******************************************************/

static void LMC_reset(void)
{
  MW_set_LMC_mixer(1);
  MW_set_LMC_master(40);
  MW_set_LMC_left(20);
  MW_set_LMC_right(20);
  MW_set_LMC_high(0);
  MW_set_LMC_low(0);
}

int MW_reset(void)
{
  int i;

  for(i=0; i<0x26; i++) {
    mw[i]=0;
  }
  mw_ct = mw_end = 0;
  Db_activ = Db_mix;
  LMC_reset();
  return 0;
}

/******************************************************
*                  Micro Wire init                    *
******************************************************/

int MW_init(void)
{
  MW_reset();
  if (!replay_frq) {
    MW_sampling_rate(SAMPLING_RATE_DEF);
  }
#if MW_CALCUL_TABLE
  init_volume();
#endif
  return 0;
}

/* --- Rescale n sample of b with r ( << MW_MIX_FIX ) --- */
static void no_mix_ste(s32 *b, int n)
{
  const int ym_mult = (Db_activ == Db_alone) ? 0 : MW_YM_MULT;

  /* Just rescale and change sign */
  do {
    int v;
    v = ((*b) * ym_mult) >> MW_MIX_FIX;
    *b++ = (u16)v + (v<<16);
  } while (--n);
}

static void mix_ste(s32 *b, int n, const s8 * spl)
{
  u32 base, end2;
  u32 ct, end, stp;
  const int vl = Db_activ[0/*lmc.master+lmc.left*/];
  const int vr = Db_activ[0/*lmc.master+lmc.right*/];
  const int loop = mw[MW_ACTI] & 2;
  const int mono = (mw[MW_MODE]>>7) & 1;
  const unsigned int frq = 50066u >> ((mw[MW_MODE]&3)^3);
  const int ym_mult = (Db_activ == Db_alone) ? 0 : MW_YM_MULT;

  /* Get internal register for sample base and sample end
   * $$$ ??? what if base > end2 ??? 
   */
  base = TOINTERNAL(MW_BASH);
  end2 = TOINTERNAL(MW_ENDH);

  /* Get current sample counter and end */
  ct = mw_ct;
  end = mw_end;

  if (ct >= end) {
    if (!loop) {
      goto out;
    } else {
      ct  = base - end + ct;
      end = end2;
      /* $$$ overflow may occurs if sample length is small or minus */
    }
  }
  /* Calculate sample step.
   * Stereo trick : Advance 2 times faster, take care of word alignment later.
   */
  stp = (frq << (9-mono)) / replay_frq;

  if (mono) {
    /* mix mono */
    do {
      int v, ym;

      ym = (*b) * ym_mult;
      v = spl[ct>>8];
      *b++ =
	(
	 (u16)((v*vl + ym) >> MW_MIX_FIX)
	 +
	 (((v*vr + ym)>>MW_MIX_FIX)<<16)
	 );

      ct += stp;
      if (ct >= end) {
	if (!loop) {
	  --n;
	  break;
	}
	ct  = base - end + ct;
	end = end2;
      }
    } while(--n);

  } else {
    /* mix stereo */
    do {
      int l,r,ym;
      int addr;

      ym = (*b) * ym_mult;
      addr = (ct >> 8) & ~1;
      l = spl[addr];
      r = spl[addr+1];
      *b++ =
	(
	 (u16)((l*vl + ym)>>MW_MIX_FIX)
	 +
	 (((r*vr + ym)>>MW_MIX_FIX)<<16)
	 );

      ct += stp;
      if (ct >= end) {
	if (!loop) {
	  --n;
	  break;
	}
	ct  = base - end + ct;
	end = end2;
      }
    } while(--n);
  }

 out:
  if (!loop && ct >= end) {
    mw[MW_ACTI] = 0;
    ct  = base;
    end = end2;
  }
  mw_ct  = ct;
  mw_end = end;

  /* Finish the buffer */
  if (n) {
    no_mix_ste(b,n);
  }
}

/******************************************************
*                  Micro Wire process                 *
******************************************************/

void MW_mix(u32 *b, const u8 * mem68, int n)
{
  if (n <= 0) {
    return;
  }

  if (!(mw[MW_ACTI]&1)) {
    /* Micro wire desactivated */
    no_mix_ste(b,n);
  } else {
    /* Micro wire activated */
    mix_ste(b,n,mem68);
  }
}
