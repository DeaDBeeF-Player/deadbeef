/*
 *                 sc68 - Paula emulator (Amiga soundchip)
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

#include <config_option68.h>

#define PAULA_CALCUL_TABLE 0

#if PAULA_CALCUL_TABLE
# include <math.h>
# include <stdio.h>
#endif

#include "io68/paulaemul.h"

#define CT_FIX       PAULA_CT_FIX
#define PL_VOL_FIX   16
#define PL_VOL_MUL   (1<<PL_VOL_FIX)
#define PL_MIX_FIX   (PL_VOL_FIX+1+8-16) /*(PL_VOL_FIX+2+8-16) */

#if PAULA_CALCUL_TABLE
# define LN_10_OVER_10 0.230258509299
# define PL_N_DECIBEL 65

static const s16 Tvol[] = {
  0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 15, 16, 18, 20, 21, 23,
  25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 48, 50, 52, 55, 58,
  60, 63, 66, 69, 72, 75, 78, 82, 85, 89, 93, 97, 101, 105, 110,
  115, 120, 126, 132, 138, 145, 153, 161, 170, 181, 192, 206,
  221, 241, 266, 301, 361, 450
};
static unsigned int volume[65];

#else

static const unsigned int volume[65] = {
  0x00000,0x006ec,0x00c9e,0x011e8,0x016fe,0x01c15,0x020a0,0x02588,
  0x029e5,0x02ec4,0x0332b,0x0376e,0x03c0c,0x04067,0x04462,0x0489d,
  0x04d1b,0x0510f,0x05537,0x05995,0x05d3d,0x0610b,0x06501,0x06920,
  0x06d6b,0x070c0,0x0755a,0x078ed,0x07c9b,0x08067,0x08450,0x08857,
  0x08c7e,0x08f55,0x093b2,0x09832,0x09b45,0x09e68,0x0a33b,0x0a687,
  0x0a9e4,0x0ad53,0x0b0d3,0x0b466,0x0b80b,0x0bbc3,0x0bf8e,0x0c36c,
  0x0c75f,0x0cb66,0x0cf82,0x0d198,0x0d5d4,0x0da26,0x0dc57,0x0e0ca,
  0x0e30d,0x0e7a3,0x0e9f7,0x0eeb1,0x0f117,0x0f5f6,0x0f86f,0x0fd73,
  0x10000,
};

#endif

static int msw_first = 0; /* big/little endian compliance */
static unsigned int replay_frq, dividand;

u8 paula[256];
paulav_t paulav[4];
int paula_dmacon;   /* Shadow DMACON */
int paula_intena;   /* Shadow INTENA */
int paula_intreq;   /* Shadow INTREQ */
int paula_adkcon;   /* Shadow ADKCON */


/******************************************************
*                  Set replay frequency               *
******************************************************/
unsigned int PL_sampling_rate(unsigned int f)
{
  if (f) {
    u64 tmp;

    replay_frq = f;
    tmp = (u64) PAULA_FRQ;
    tmp <<= CT_FIX;
    tmp /= replay_frq;
    dividand = (unsigned int) tmp;
  }
  return replay_frq;
}

#if PAULA_CALCUL_TABLE
static unsigned int calc_volume(unsigned int vol, unsigned int mult)
{
  double r;

  if (!vol) {
    return 0;
  }
  r = (double) mult / exp((double) Tvol[64 - vol] / 100.0);
  return (unsigned int) r;
}

static void init_volume(void)
{
  int i;

  for (i = 0; i < 65; i++) {
    volume[i] = calc_volume(i, PL_VOL_MUL);
    fprintf(stderr, "XXX:%08x\n", volume[i]);
  }
}
#endif

/******************************************************
*                     PAULA reset                     *
******************************************************/

int PL_reset(void)
{
  int i;

  for (i = 0; i < sizeof(paula); i++) {
    paula[i] = 0;
  }
  for (i = 0; i < 4; i++) {
    paulav[i].adr = 0;
    paulav[i].start = 0;
    paulav[i].end = 2;
  }

  /* Reset DMACON and INTENA/REQ to something that
   * seems acceptable to me.
   */
  paula_dmacon = 1 << 9;  /* DMA general activate, DMA audio desactivate. */
  paula_intreq = 0;       /* No interrupt request.                        */
  paula_intena = 1 << 14; /* Master interrupt enable, audio int disable.  */
  paula_adkcon = 0;       /* No modulation.                               */
  return 0;
}

/******************************************************
*                      PAULA init                     *
******************************************************/

int PL_init(void)
{
  u32 tmp = 0x1234;

  /* Setup little/big endian swap */
  msw_first = !(*(u16 *)&tmp);

  /* Reset hardware. */
  PL_reset();

  /* Set default replay rate if it neccessary. */
  if (!replay_frq) {
    PL_sampling_rate(SAMPLING_RATE_DEF);
  }

#if PAULA_CALCUL_TABLE
  init_volume();
#endif

  return 0;
}

static void poll_irq(unsigned int N)
{
  u8 *p = paula + PAULA_VOICE(N);
  paulav_t * w = paulav + N;

  /* Reload internal when interrupt is DENIED */
  if (
      (paula_intreq
       |
       ~((paula_intena << (8 * sizeof(int) - 1 - 14) >> (8 * sizeof(int) - 1))
         & paula_intena)) & (1 << (N + 7))) {
    unsigned int a,l;

    /* Get sample pointer. */
    a = (((p[1] << 16) | (p[2] << 8) | p[3]) & 0x7FFFE) << PAULA_CT_FIX;
    w->adr = w->start = a;

    /* Get length */
    l = (p[4] << 8) | p[5];
    l |= (!l) << 16;
    l <<= (1 + PAULA_CT_FIX);
    w->end = a + l;
  }
  paula_intreq |= 1 << (N + 7);
}

/* Mix with laudio channel data (1 char instead of 2) */
static void mix_zero(u32 * b, unsigned n, unsigned N, int right)
{
  /* Add +0 => nothing to do :) */
#if 0
  int o;
  u8 *p;
  paulav_t *w;
  u16 *b2;

  p = paula + PAULA_VOICE(N);
  w = paulav + N;

  b2 = (u16 *) b;
  b2 += right;

  do {
    *b2 += 0;
    b2 += 2;
  } while (--n);
#endif
}

static void mix_one(u32 * b,
		    int n,
		    int N,
		    u8 * mem,
		    const int shift)
{
  paulav_t *w;
  u8 * p;
  int last;
  unsigned int stp, vol, per, readr, reend, end, hasloop;
  u32 adr;
  s16 *b2;

  hasloop = 0;

  /* volume & period */
  p = paula + PAULA_VOICE(N);
  w = paulav + N;

  vol = p[9] & 127;
  if (vol >= 64) {
    vol = 64;
  }
  vol = volume[vol];

  per = ((p[6] << 8) + p[7]);
  if (per == 0)
    per = 1;
  stp = dividand / per;

// /* audio int enable for this voice */
// if(paula_intena & ((paula_intena&~paula_intreq)<<(14-7-N)) & (1<<14) )
// {
//  /* Audio interrupt enable for this voice : No internal reload */
//  readr = v->start;
//  reend = v->end;
// }
// else
  {
    /* Audio irq disable for this voice :
     * Internal will be reload at end of block
     */
    readr = (((p[1] << 16) | (p[2] << 8) | p[3]) & 0x7FFFE) << PAULA_CT_FIX;
    reend = ((p[4] << 8) | p[5]) << (1 + PAULA_CT_FIX);
    reend = (reend == 0) ? (1 << (17 + PAULA_CT_FIX)) : reend;
    reend += readr;
  }

  if (reend < readr) {
/*		printf("voice %d reend<readr !!!!!!!!!!!\n");*/
    return;
  }

  adr = w->adr;
  end = w->end;

/*	printf("V(%d) %03u:%03u -> %03u:%03u\n",N,adr>>PAULA_CT_FIX,end>>PAULA_CT_FIX,readr>>PAULA_CT_FIX,reend>>PAULA_CT_FIX);*/

  if (end < adr) {
/*		printf("V(%d) adr>end %03u<%03u!!!!!!!!!!!\n",N,adr>>PAULA_CT_FIX,end>>PAULA_CT_FIX);*/
    return;
  }

  b2 = (u16 *) b;
  if (shift) {
    ++b2;
  }

  /* mix stereo */
  do {
    int v, ov;
    int low;

    low = adr & ((1 << CT_FIX) - 1);

    if (adr >= end) {
      unsigned int relen = reend - readr;
      hasloop = 1;
      adr = readr + adr - end;
      end = reend;
      while (adr >= end) {
        adr -= relen;
      }
    }

    {
      int i = (int)(adr >> CT_FIX);
      last = (int)(s8)mem[i++];
      v = last;

      if (( (u32)i << CT_FIX) >= end) {
        i = (int)(readr >> CT_FIX);
      }
      ov = (int)(s8)mem[i];
    }

    /* Linear interpol */
    v = (ov * low + v * ((1 << CT_FIX) - low)) >> CT_FIX;
    v *= vol;
    v >>= PL_MIX_FIX;
    *b2 += v;
    b2 += 2;
    adr += stp;
  } while (--n);

  if (adr >= end) {
    unsigned int relen = reend - readr;

    hasloop = 1;
    adr = readr + adr - end;
    end = reend;
    while (adr >= end)
      adr -= relen;
  }

  last &= 0xFF;
  p[0xA] = last + (last << 8);
  w->adr = adr;
  if (hasloop) {
    w->start = readr;
    w->end = end;
  }
}

/******************************************************
*                  PAULA process                      *
******************************************************/

static void clear_buffer(u32 * b, int n)
{
  const u32 v = 0;
  int rem = n & 3;
  n >>= 2;
  if (n) {
    do {
      *b++ = v;
      *b++ = v;
      *b++ = v;
      *b++ = v;
    } while (--n);
  }
  if (rem & 1) {
    *b++ = v;
  }
  if (rem & 2) {
    *b++ = v;
    *b++ = v;
  }
}

void PL_mix(u32 * b, u8 * mem68, int n)
{
  int i;

  if (n <= 0) {
    return;
  }

  clear_buffer(b, n);

  for (i = 0; i < 4; i++) {
    const int right = (i ^ msw_first) & 1;
    if ((paula_dmacon >> 9) & (paula_dmacon >> i) & 1) {
      mix_one(b, n, i, mem68, right);
    } else {
      mix_zero(b, n, i, right);
    }
  }

  /* Assume next mix is next frame : reset beam V/H position. */
  paula[PAULA_VHPOSR] = 0;
}
