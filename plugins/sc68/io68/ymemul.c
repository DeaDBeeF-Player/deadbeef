/*
 *                         sc68 - YM-2149 emulator
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

#include <stdio.h>//$$$

#include <config68.h>
#include "io68/ymemul.h"

static u16 ymout[16*16*16] =
#include "io68/ym_fixed_vol.h"

#define YM_MASTER_FRQ  2000000  /**< YM master frequency in Atari ST:2Mhz */
#define YM_FRQ         250000   /**< Max frequency involved (for envelop) */

#define MAX_SPL_PER_PASS (YM_FRQ / REPLAY_RATE_MIN)+1
#define MAX_VOLUME_ENTRY 2048

#define INTMSB  ((sizeof(int)<<3)-1)

static s32 mixbuf[MAX_SPL_PER_PASS], * mixptr;
static int envbuf[MAX_SPL_PER_PASS], * envptr;
static int noibuf[MAX_SPL_PER_PASS], * noiptr;

static int fake_cycle;
static int filter_cnt, filter_acu;
static int output_level;

#ifdef _DEBUG
  static int noiseCnt;
  static int envCnt;
  static int mixCnt;
#endif

static unsigned int voiceCut = 0xFFF;

typedef struct
{
  int cycle;
  u8  reg,v;
} entry_t;

typedef struct
{
  int lc;                       /* Last cycle mixed */
  entry_t *c;                   /* Free entry */
  entry_t e[MAX_VOLUME_ENTRY];  /* Static register entry list */
} entrylist_t;

static entrylist_t envelopR, noiseR, mixerR;
static unsigned int ifrq;

static unsigned ifrq = 0;

ym2149_t ym;



static void reset_entry(entrylist_t *l)
{
  l->lc = 0;
  l->c = l->e;
}

static void add_entry(entrylist_t *l, u32 reg, u32 v, u32 cycle)
{
  entry_t *p = l->e+MAX_VOLUME_ENTRY;

  if (l->c != p) {
    p=l->c++;
  } else {
    p--;
#ifdef _DEBUG
    BREAKPOINT68;
#endif
  }
  p->cycle = cycle;
  p->reg = reg;
  p->v = v;
}

static unsigned int get_entry(entrylist_t *l, unsigned int reg, int cycle)
{
  entry_t *p;

  /* Found last minus cycle in write list */
  for (p=l->c-1; p>=l->e; p--) {
    if (p->reg==reg) return p->v;
  }

  /* Not found : Get local */
  return ym.data[reg];
}

static void subcycle_entry(entrylist_t *l, unsigned int subcycle)
{
  entry_t *p;
  l->lc -= subcycle;

#ifdef _DEBUG
  if ((int) l->lc) {
    BREAKPOINT68;
    l->lc = 0;
  }

  if ((int) l->lc < 0) {
    BREAKPOINT68;
    l->lc = 0;
  }
#endif

  for (p=l->e; p<l->c; p++) {
    p->cycle -= subcycle;
#ifdef _DEBUG
    if ( (int)p->cycle < 0) {
      BREAKPOINT68;
      p->cycle = 0;
    }
#endif
  }
}

/******************************************************
*                  Set replay frequency               *
******************************************************/
unsigned int YM_sampling_rate(unsigned int f)
{
  if (f) {
    ifrq =
      (f < SAMPLING_RATE_MIN)
      ? SAMPLING_RATE_MIN
      : ((f > SAMPLING_RATE_MAX)
	 ? SAMPLING_RATE_MAX
	 : f);
  }
  return ifrq;
}

/******************************************************
*                  Yamaha reset                       *
******************************************************/

/* shape format : CONTinue ATTack ALTernate HOLD */
static void change_env(int shape)
{
  /* Alternate mask start value depend on ATTack bit */
  ym.env_alt = (((shape << (INTMSB-2)) >> INTMSB) ^ 0x1F) & 0x1F;
  ym.env_bit = 0;
  ym.env_bitstp = 1;
  ym.env_cont = 0x1f;

  /* $$$ Not sure, that this counter is resetted, buit it seems 
         cleaner. */
/*   ym.env_ct = 0;  */

  // $$$ TEMP !
  ym.env_ct = 0x10000; 

  /* $$$ temp */
  ym.data[YM_ENVTYPE] = shape;

}

int YM_reset(void)
{
  int i;
  for (i=0; i < sizeof(ym.data); i++) {
    ym.data[i]=0;
  }
  ym.data[YM_MIXER] = 077;
  ym.data[YM_ENVTYPE] = 0x0a;
  ym.ctrl       = 0;
  ym.env_ct     = 0;
  ym.noise_gen  = 1;
  ym.noise_ct   = 0;
  ym.voice_ctA = 0;
  ym.voice_ctB = 0;
  ym.voice_ctC = 0;
  ym.sq = 0;
  /*voiceCut = 0xFFF;*/ /* Do not reset voice cut */
  reset_entry(&mixerR);
  reset_entry(&noiseR);
  reset_entry(&envelopR);
  change_env(ym.data[YM_ENVTYPE]);

  filter_cnt = 0;
  filter_acu = 0;

  fake_cycle = 0;

  return 0;
}

/******************************************************
*                  Yamaha init                        *
******************************************************/


static void init_output_level(unsigned int new_level)
{
  int i;
  unsigned int mult = (new_level<<14)/(ymout[0xfff]+1);

  output_level = new_level;

  for(i=0; i<4096; i++) {
    unsigned int v;
    v = ((unsigned int)ymout[i] * mult) >> 14;
    if (v > new_level) {
#ifdef _DEBUG
      BREAKPOINT68;
#endif
      v = new_level;
    }
    ymout[i] = v;
  }
}

int YM_init(void)
{
  init_output_level(0xD000);
  YM_reset();
  if (!ifrq) {
    YM_sampling_rate(SAMPLING_RATE_DEF);
  }
  return 0;
}

/******************************************************
*                  Noise generator                    *
******************************************************/

/*
 * Generate noise for ncycle ( 8Mhz )
 * - stp is step for this noise period
 * - msk is mixer mask for noise
 * - mix in noiptr & update it
 *
 * -> return # cycle not mixed
 *
 */

/** New version : one cycle emulation */
static int generate_noise(int ncycle)
{
  int rem_cycle;
  int v, noise_gen, ct, per, msk;
  int * b;

#ifdef _DEBUG
  if (ncycle < 0) {
    BREAKPOINT68;
    return 0;
  }
#endif

  rem_cycle = ncycle & 31;
  if(!(ncycle >>= 5)) return rem_cycle;

  /* All inits */
  ct        = ym.noise_ct;
  noise_gen = ym.noise_gen;
  per       = (ym.data[YM_NOISE] & 0x3F) << 1; //$$$ ???

  v = ym.data[YM_MIXER] >> 3;
  msk = -(v&1) & 0xF00;
  v >>= 1;
  msk |= -(v&1) & 0x0F0;
  v >>= 1;
  msk |= -(v&1) & 0x00F;
  b         = noiptr;

  v = -(noise_gen & 1) | msk;
  do
  {
    if (++ct >= per) {
      ct = 0;

      /* --- Based on MAME :) --- */
      /* The Random Number Generator of the 8910 is a 17-bit shift */
      /* register. The input to the shift register is bit0 XOR bit2 */
      /* (bit0 is the output). */

      /* bit 17 := bit 0 ^ bit 2 */
      noise_gen |= ((noise_gen^(noise_gen>>2)) & 1)<<17;
      noise_gen >>= 1;
/*       fprintf(stderr, "per:%d, no:%05x \n", per, noise_gen); */
      v = -(noise_gen & 1) | msk;
    }
    *b++ = v;
  } while(--ncycle);

#ifdef _DEBUG
  noiseCnt += b-noiptr;
#endif

  /* Save value for next pass */
  noiptr = b;
  ym.noise_gen = noise_gen;
  ym.noise_ct = ct;

  /* return not mixed cycle */
  return rem_cycle;
}

static void cycle_noise_gen(int cycle)
{
  entry_t *p;
  unsigned int lastcycle;
  noiptr = noibuf;

#ifdef _DEBUG
  if (cycle < 0) {
    BREAKPOINT68;
    return;
  }
#endif

  if (!cycle) {
    return;
  }

  for (p=noiseR.e, lastcycle=noiseR.lc; p<noiseR.c; p++) {
    lastcycle = p->cycle - generate_noise(p->cycle-lastcycle);
    ym.data[p->reg] = p->v;
  }
  lastcycle = cycle - generate_noise(cycle-lastcycle);

#ifdef _DEBUG
  if (lastcycle != cycle) {
    BREAKPOINT68;
  }
#endif

  noiseR.lc = lastcycle;
  noiseR.c  = noiseR.e;
}

/******************************************************
*                Envelop generator                    *
******************************************************/

static int generate_env(int ncycle)
{
  int rem_cycle;
  int *b;
  int ct, per;
  unsigned int bit, bitstp, restp;
  unsigned int cont, recont;
  unsigned int alt, altalt;
  int shape;

#ifdef _DEBUG
  if (ncycle < 0) {
    BREAKPOINT68;
    return 0;
  }
#endif

  rem_cycle = ncycle & 31;
  if(!(ncycle >>= 5)) return rem_cycle;

  b       = envptr;

  /* period */
  ct      = ym.env_ct;
  per     = ym.data[YM_ENVL] | (ym.data[YM_ENVH]<<8);

  shape   = ym.data[YM_ENVTYPE];

  /* bit */
  bit     = ym.env_bit;
  bitstp  = ym.env_bitstp;
  restp   = (shape & 1) ^ 1;

  /* continue */
  cont    = ym.env_cont;
  recont  = (shape & 0x8) ? 0x1F : 0;

  /* alternate */
  alt     = ym.env_alt;
  altalt  = ((shape ^ (shape>>1)) & 0x1) ? 0x1f : 0;

  per |= !per;

  do {
    int n;

    n = per - ct;
    if (n <= 0) {
      int prev_bit;
      ct = 0;
      n = per;
      prev_bit = bit;
      bit += bitstp;
      if ((bit^prev_bit) & 32) {
        bitstp = restp;
        cont = recont;
        alt ^= altalt;
      }
    }

    /* $$$ Lost envelop less signifiant bit */
    {
      int v = ((bit ^ alt) & cont) >> 1;
      v |= v<<4;
      v |= v<<4;

      if (n > ncycle) {
        n = ncycle;
      }
      ncycle -= n;
      ct += n;

#ifdef _DEBUG
      if (n<=0) {
        BREAKPOINT68;
        break;
      }
#endif

      do {
        *b++ = v;
      } while(--n);
    }

  } while (ncycle);

  /* $$$  Must correct the env countr here */

#ifdef _DEBUG
  envCnt += b-envptr;
#endif

  /* Save value for next pass */
  envptr        = b;
  ym.env_ct     = ct;
  ym.env_bit    = bit;
  ym.env_bitstp = bitstp;
  ym.env_cont   = cont;
  ym.env_alt    = alt;

  return rem_cycle;
}

static void cycle_env_gen(int cycle)
{
  entry_t *p;
  int lastcycle;

#ifdef _DEBUG
  if (cycle < 0) {
    BREAKPOINT68;
    return;
  }
#endif

  if(!cycle) return;

  envptr = envbuf;
  lastcycle = envelopR.lc;

  p = envelopR.e;
  while (p < envelopR.c) {
    int curcycle;

    curcycle  = p->cycle;
#ifdef _DEBUG
    if (curcycle > cycle) {
      BREAKPOINT68;
    }
#endif

    lastcycle = curcycle - generate_env(curcycle-lastcycle);

    while (p < envelopR.c && p->cycle <= curcycle) {
      ym.data[p->reg] = p->v;
      if(p->reg==YM_ENVTYPE) {
        change_env(p->v);
      }
      ++p;
    }
  }

  lastcycle = cycle -
    generate_env(cycle-lastcycle);

#ifdef _DEBUG
  if (lastcycle != cycle) {
    BREAKPOINT68;
  }
#endif

  envelopR.lc = lastcycle;
  envelopR.c = envelopR.e;
}

/******************************************************
*                Sound generator                      *
******************************************************/

static int generate_mixer(int ncycle,
			  /*const*/ int smsk,
			  /*const*/ int emsk,
			  /*const*/ int vols,
			  const int center)
{
  int ctA,  ctB,  ctC;
  int perA, perB, perC;
  s32 * b;
  int * e, * o;
  int rem_cycle;
  int levels;

#ifdef _DEBUG
  if (ncycle < 0) {
    BREAKPOINT68;
    return 0;
  }
#endif

/*   SC68os_pdebug("GENERATE : %03X\n", emsk|vols);  */

  /* Not enought cycle !!! */
  rem_cycle = ncycle & 31;
  ncycle >>= 5;
  if(!ncycle) {
    return rem_cycle;
  }

  /* init buffer address */
  o = noiptr;
  e = envptr;
  b = mixptr;


  {
    int v;
    /* 3 voice sound mask */

    emsk = vols = 0;
    v = ym.data[YM_MIXER];
    smsk = -(v & 1) & 0xF00;
    v >>= 1;
    smsk |= -(v & 1) & 0x0F0;
    v >>= 1;
    smsk |= -(v & 1) & 0x00F;

    /* 3 voices buzz or lvl mask */
    v = ym.data[YM_VOL(0)] & 0x1F;
    if(v&0x10) emsk |= 0xF00;
    else       vols |= v<<8;

    v = ym.data[YM_VOL(1)]&0x1F;
    if(v&0x10) emsk |= 0x0F0;
    else       vols |= v<<4;

    v = ym.data[YM_VOL(2)]&0x1F;
    if(v&0x10) emsk |= 0x00F;
    else       vols |= v;
  }

  /* Mixer steps & couters */
  ctA = ym.voice_ctA;
  ctB = ym.voice_ctB;
  ctC = ym.voice_ctC;

  perA = ym.data[YM_PERL(0)] | ((ym.data[YM_PERH(0)]&0xF)<<8);
  perB = ym.data[YM_PERL(1)] | ((ym.data[YM_PERH(1)]&0xF)<<8);
  perC = ym.data[YM_PERL(2)] | ((ym.data[YM_PERH(2)]&0xF)<<8);
/*   perA <<= 1; */
/*   perB <<= 1; */
/*   perC <<= 1; */
  perA |= !perA;
  perB |= !perB;
  perC |= !perC;

  levels = ym.sq;

#if 1

  do {
    int sq;
    
    if (ctA >= perA) {
      ctA = 0;
      levels = levels ^ 0xF00;
    }

    if (ctB >= perB) {
      ctB = 0;
      levels = levels ^ 0x0F0;
    }
    
    if (ctC >= perC) {
      ctC = 0;
      levels = levels ^ 0x00F;
    }
    
    sq = levels;
    sq |= smsk;
    sq &= *o++;
    sq &= ((*e++)&emsk) | vols;
    
#ifdef _DEBUG
    if (sq < 0 || sq > 0xFFF) {
      BREAKPOINT68;
    }
#endif
    
    sq &= voiceCut;
    sq = (int)ymout[sq];
    sq -= center;
    
#ifdef _DEBUG
    if (sq < -0x8000 || sq > 0x7fff) {
      BREAKPOINT68;
      sq = (sq < 0) ? -0x8000 : 0x7fff;
    }
#endif
    *b++ = sq;
  
    ctA++;
    ctB++;
    ctC++;
  } while (--ncycle);

#endif


  /* $$$ Add error correction here  */

#ifdef _DEBUG
  mixCnt += b-mixptr;
#endif

  /* Save value for next pass */
  ym.voice_ctA = ctA;
  ym.voice_ctB = ctB;
  ym.voice_ctC = ctC;
  ym.sq        = levels;
  noiptr = o;
  envptr = e;
  mixptr = b;

  return rem_cycle;
}


static void cycle_mix_gen(int cycle)
{
  entry_t *p;
  unsigned int lastcycle,smsk,emsk,vols;
  int perA, perB, perC;
  int center = 0;
  static int oldcenter = 0x8000;

  mixptr = mixbuf;
  envptr = envbuf;
  noiptr = noibuf;

  perA = ym.data[YM_PERL(0)] | ((ym.data[YM_PERH(0)]&0xF)<<8);
  perB = ym.data[YM_PERL(1)] | ((ym.data[YM_PERH(1)]&0xF)<<8);
  perC = ym.data[YM_PERL(2)] | ((ym.data[YM_PERH(2)]&0xF)<<8);

  emsk = vols = 0;
  {
    int v;

    /* 3 voice sound mask */
    v = ym.data[YM_MIXER];
    smsk =
      ((v<<(INTMSB-0)>>INTMSB) & 0xf00 ) |
      ((v<<(INTMSB-1)>>INTMSB) & 0x0f0 ) |
      ((v<<(INTMSB-2)>>INTMSB) & 0x00f );

    /* 3 voices buzz or lvl mask */
    v = ym.data[YM_VOL(0)] & 0x1F;
    if (v&0x10) emsk |= 0xF00;
    else        vols |= v<<8;

    v = ym.data[YM_VOL(1)] & 0x1F;
    if (v&0x10) emsk |= 0x0F0;
    else        vols |= v<<4;

    v = ym.data[YM_VOL(2)] & 0x1F;
    if (v&0x10) emsk |= 0x00F;
    else        vols |= v;
  }

  /* Calculate center */
  if (1) {
    /* Scan volume register change to get the maximum output value
       that may happen. It will be used to recenter the signal.
    */
    int vvv = (vols | emsk) & voiceCut;
    center = (int)ymout[vvv];

    for (p=mixerR.e; p<mixerR.c; ) {
      const cycle68_t current_cycle = p->cycle;
      int newcenter = 0;

      do {
	int i = p->reg;

	switch (i) {
	case YM_VOL(0):
	case YM_VOL(1):
	case YM_VOL(2):
	  {
	    const int shift = 8 - ((i - YM_VOL(0)) << 2); /* [8 4 0] */
	    const int mask  = (0xF << shift);
	    const int v = p->v & 0x1F;

	    vvv &= ~mask;
	    if (v & 0x10) {
	      vvv |= mask;
	    } else {
	      vvv |= v << shift;
	    }
	  } break;
	}
	++p;
      } while ( p < mixerR.c && p->cycle == current_cycle);
      vvv &= voiceCut;
      newcenter = (int)ymout[vvv];
      if (newcenter > center) {
	center = newcenter;
      }
    }

    /* At this point generated value should be between [0 .. center].
     *
     * Following code tries to adjust centering with a smoothed value.
     * But sometimes it may produce saturation probleme. This is fixed,
     * but it could be better with an marging on the maximum output level
     * which allow higher centering value before saturation...
     */
    if (0) {
      const int clip_max =  0x7F00;
      int smoothed = (3*oldcenter + center) >> 2;
      int half = (smoothed+1) >> 1;;
      if (center - half > clip_max) {
	half = center - clip_max;
	smoothed = half << 1;
      }
      oldcenter = smoothed;
      center = half;
    } else {
      /* Do not use smoothing :) */
      center = (center+1) >> 1;
    }
  } else {
    center = 0x8000;
  }

  for (p=mixerR.e, lastcycle=mixerR.lc; p<mixerR.c; p++) {
    int v;
    int ncycle = p->cycle-lastcycle;
    
    if (ncycle) {
      lastcycle =
	p->cycle -
	generate_mixer(ncycle,smsk, emsk, vols, center);
    }

    v = ym.data[p->reg] = p->v;
    switch(p->reg)
    {
      case YM_VOL(0):
        v    &= 0x1F;
        vols &= 0x0FF;
        if (v&0x10) {
          emsk |= 0xF00;
        } else {
          emsk &= 0x0FF;
          vols |= v<<8;
        }
        break;

      case YM_VOL(1):
        v    &= 0x1F;
        vols &= 0xF0F;
        if (v&0x10) {
          emsk |= 0x0F0;
        } else {
          emsk &= 0xF0F;
          vols |= v<<4;
        }
        break;

      case YM_VOL(2):
        v    &= 0x1F;
        vols &= 0xFF0;
        if (v&0x10) {
          emsk |= 0x00F;
	} else {
          emsk &= 0xFF0;
          vols |= v;
        }
        break;

      case YM_MIXER:
        smsk =
          ((v<<(INTMSB-0)>>INTMSB) & 0xf00 ) |
          ((v<<(INTMSB-1)>>INTMSB) & 0x0f0 ) |
          ((v<<(INTMSB-2)>>INTMSB) & 0x00f );
        break;

      case YM_PERL(0): case YM_PERH(0):
        perA = ym.data[YM_PERL(0)] | ((ym.data[YM_PERH(0)]&0xF)<<8);
#ifdef HFLIMIT
        if(perA<hflimit)  hfmsk |= 0xF00;
        else              hfmsk &= 0x0FF;
#endif
        break;

      case YM_PERL(1): case YM_PERH(1):
        perB = ym.data[YM_PERL(1)] | ((ym.data[YM_PERH(1)]&0xF)<<8);
#ifdef HFLIMIT
        if(perB<hflimit)  hfmsk |= 0x0F0;
        else              hfmsk &= 0xF0F;
#endif
        break;

      case YM_PERL(2): case YM_PERH(2):
        perC = ym.data[YM_PERL(2)] | ((ym.data[YM_PERH(2)]&0xF)<<8);
#ifdef HFLIMIT
        if(perC<hflimit)  hfmsk |= 0x00F;
        else              hfmsk &= 0xFF0;
#endif
        break;
    }
  }
  lastcycle =
    cycle
    - generate_mixer(cycle-lastcycle, smsk, emsk, vols, center);

#ifdef _DEBUG
  if (lastcycle != cycle) {
    BREAKPOINT68;
  }
#endif

  mixerR.lc = lastcycle;
  mixerR.c = mixerR.e;
}

/* Transform 250000Hz buffer to current sampling rate.
 *
 * In order to emulate envelop tone half level trick, 
 * the function works by block block of 4 PCM which are
 * averaged. Since number of PCM in source buffer may not be
 * a multiple of 4, the funcion use a filter accumulator.
 */
static void filter(void)
{
  int n;
  s32 * src, * dst;

  /* Get number of PCM. */
  n = mixptr - mixbuf;
  if (n <= 0) {
    return;
  }
  src = dst = mixbuf;

  
  /* Is there PCM in accumulator ? */
  if (filter_cnt) {
    int v = filter_acu;
    int m = 4-filter_cnt;
    if (m > n) {
      m = n;
    }
    n -= m;

    if (m&1) {
      v += *src++;
    }
    if (m&2) {
      v += *src++;
      v += *src++;
    }

    filter_cnt += m;
    if (filter_cnt < 4) {
      /* filter acu not fully filled: no more sample */
#if _DEBUG
      if (filter_cnt != 4) {
	BREAKPOINT68;
      }
#endif
      return;
    }
    *dst++ = v>>2;
  }

  filter_cnt = n & 3; /* Get remainder */
  n >>= 2;            /* Number of block */

  if (n > 0) {
    unsigned int stp, ct, end;

    ct = 0;
    stp = (YM_FRQ << 14) / ifrq;
    end = n << 16;

    do {
      int v, i;
      /* Get block of 4 samples. */
      i = (int)(ct>>(16-2))&-4;
      v = (src[i] + src[i+1] + src[i+2] + src[i+3]) >> 2;
      *dst++ = v;
    } while ((ct += stp) < end);
    src += (ct >> (16-2)) & -4;
  }
  mixptr = dst;


  /* Fill filter accumulator with remaining PCM for next pass. */
  filter_acu = 0;
  if (filter_cnt & 1) {
    filter_acu += *src++;
  }
  if (filter_cnt & 2) {
    filter_acu += *src++;
    filter_acu += *src++;
  }

}

/******************************************************
*                  Yamaha process                     *
*               return # mixed sample                 *
******************************************************/
unsigned int YM_mix(cycle68_t cycle2mix)
{
  /* $$$ Must check buffer overflow here */

#ifdef _DEBUG
  if (cycle2mix&31) {
    BREAKPOINT68;
  }
#endif
  cycle2mix += fake_cycle;
  fake_cycle = cycle2mix&31;
  cycle2mix -= fake_cycle;
  

#ifdef _DEBUG
  mixCnt = noiseCnt = envCnt = 0;
#endif

#ifdef _DEBUG
  if (envelopR.lc != noiseR.lc || envelopR.lc != mixerR.lc) {
    BREAKPOINT68;
  }
#endif
  
  cycle_noise_gen(cycle2mix);
  cycle_env_gen(cycle2mix);
  cycle_mix_gen(cycle2mix);

#ifdef _DEBUG
  /* $$$ Should not happen */
  if (mixCnt != noiseCnt || mixCnt != envCnt) {
    BREAKPOINT68;
  }
#endif

  mixerR.lc = envelopR.lc = noiseR.lc = cycle2mix + fake_cycle;

  filter();
  return mixptr - mixbuf;
}

/*************************************
*         Write in YM register       *
*************************************/
void YM_writereg(u8 reg, u8 v, cycle68_t cycle)
{
  switch(reg)
  {
    case YM_PERL(0): case YM_PERH(0):
    case YM_PERL(1): case YM_PERH(1):
    case YM_PERL(2): case YM_PERH(2):
    case YM_VOL(0): case YM_VOL(1): case YM_VOL(2):
      add_entry( &mixerR, reg, v,  cycle );
      break;

    case YM_ENVL: case YM_ENVH: case YM_ENVTYPE:
      add_entry( &envelopR, reg, v, cycle );
      break;

    case YM_MIXER: /* Reg 7 modify both noise gen */
                   /* & mixer */
      add_entry( &mixerR, reg, v, cycle );
    case YM_NOISE:
      add_entry( &noiseR, reg, v, cycle );
      break;

    default:
      break;
  }
}

/********************************************
*         Read from YM register             *
********************************************/
u8 YM_readreg(u8 reg, cycle68_t cycle)
{
    /*return ym.data[reg];*/

  switch(reg)
  {
    case YM_PERL(0): case YM_PERH(0):
    case YM_PERL(1): case YM_PERH(1):
    case YM_PERL(2): case YM_PERH(2):
    case YM_VOL(0): case YM_VOL(1): case YM_VOL(2):
      return get_entry( &mixerR, reg, cycle );

    case YM_ENVL: case YM_ENVH: case YM_ENVTYPE:
      return get_entry( &envelopR, reg, cycle );

    case YM_NOISE:
    case YM_MIXER:
      /* Reg 7 is used in 2 lists */
      /* Better choice is noise   */
      return get_entry( &noiseR, reg, cycle );

    default:
      break;
  }
  return 0;
}

/****************************************** *
*         Change YM cycle count base        *
********************************************/
void YM_subcycle(cycle68_t subcycle)
{
  subcycle_entry(&mixerR,   subcycle);
  subcycle_entry(&envelopR, subcycle);
  subcycle_entry(&noiseR,   subcycle);
}

/******************************************************
*                  Yamaha get buffer                  *
******************************************************/
u32 *YM_get_buffer(void)
{
  return mixbuf;
}

/******************************************************
*                 Yamaha get activated voices         *
******************************************************/
int YM_get_activevoices(void)
{
  return ((voiceCut>>8)&1) | ((voiceCut>>4)&2) | (voiceCut&4);
}

/******************************************************
*                 Yamaha set activated voices         *
******************************************************/
void YM_set_activeVoices(int v)
{
  int cut = 0;
  if (v&1) cut |= 0xF00;
  if (v&2) cut |= 0x0F0;
  if (v&4) cut |= 0x00F;
  voiceCut = cut;
}

