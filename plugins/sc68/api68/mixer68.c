/*
 *                        sc68 - audio mixer
 *         Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
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

#include <config68.h>
#include "api68/mixer68.h"

/* ARM compliant version */
/* #define SWAP_16BITWORD(V) ((V^=V<<16), (V^=V>>16), (V^=V<<16)) */

#define SWAP_16BITWORD(V) (((u32)(V)>>16)+(V<<16))

/*  Mix 16-bit-stereo PCM into 16-bit-stereo with sign change.
 *
 *  PCM' = PCM ^ sign
 *
 *  sign=0          : Keep input sign, 
 *  sign=0x00008000 : Change left channel sign
 *  sign=0x80000000 : Change right channel sign
 *  sign=0x80008000 : Change both channel
 */
void SC68mixer_stereo_16_LR(u32 * dest, u32 * src, int nb,
			    const u32 sign)
{
  u32 *end;
  int rem;

  /* Optimize trivial case : same buffer, no sign change */
  if (!sign && dest == src) {
    return;
  }

  rem = nb&3;
  end = dest+(nb-rem);
  if(dest<end) {
    do {
      *dest++ = (*src++) ^ sign;
      *dest++ = (*src++) ^ sign;
      *dest++ = (*src++) ^ sign;
      *dest++ = (*src++) ^ sign;
    } while(dest<end);
  }

  if(rem&1) {
    *dest++ = (*src++) ^ sign;
  }
  if(rem&2) {
    *dest++ = (*src++) ^ sign;
    *dest++ = (*src++) ^ sign;
  }
}

/*  Mix 16-bit-stereo PCM into 16-bit-stereo PCM with channel swapping.
 */
void SC68mixer_stereo_16_RL(u32 * dest, u32 * src, int nb,
			    const u32 sign)
{
  u32 *end;
  int rem;

#undef  MIX_ONE
#define MIX_ONE \
 v = (*src++);\
 *dest++ = SWAP_16BITWORD(v) ^ sign

  rem = nb&3;
  end = dest+(nb-rem);
  if(dest<end) {
    do {
      u32 v;
      MIX_ONE;
      MIX_ONE;
      MIX_ONE;
      MIX_ONE;
    } while(dest<end);
  }

  if (rem&1) {
    u32 v;
    MIX_ONE;
  }

  if(rem&2) {
    u32 v;
    MIX_ONE;
    MIX_ONE;
  }
}

/*  Mix 16-bit-stereo PCM into 32-bit-stereo-float (-norm..norm)
 */
void SC68mixer_stereo_FL_LR (float * dest, u32 * src, int nb,
			     const u32 sign, const float norm)
{
  const float mult = norm / 32768.0f;
  int v;
  float *end;

  end = dest + (nb<<1);
  if (dest < end) {
    do {
      v = (int)(s32)(*src++ ^ sign);
      *dest++ = mult * (float)(s16)(v);
      *dest++ = mult * (float)(v>>16);
    } while (dest < end);
  }
}

/*  Duplicate left channel into right channel and change sign.
 *  PCM' = ( PCM-L | (PCM-L<<16) ) ^ sign
 */
void SC68mixer_dup_L_to_R(u32 *dest, u32 *src, int nb,
			  const u32 sign)
{
  u32 *end;
  int rem;

  rem = nb&3;
  end = dest+(nb-rem);
  if (dest < end) {
    do {
      u32 v;
      v = (u16)*src++; *dest++ = ((v<<16)|v) ^ sign;
      v = (u16)*src++; *dest++ = ((v<<16)|v) ^ sign;
      v = (u16)*src++; *dest++ = ((v<<16)|v) ^ sign;
      v = (u16)*src++; *dest++ = ((v<<16)|v) ^ sign;
    } while (dest < end);
  }

  if (rem&1) {
    u32 v;
    v = (u16)*src++; *dest++ = ((v<<16)|v) ^ sign;
  }

  if (rem&2) {
    u32 v;
    v = (u16)*src++; *dest++ = ((v<<16)|v) ^ sign;
    v = (u16)*src++; *dest++ = ((v<<16)|v) ^ sign;
  }
}

/*  Duplicate right channel into left channel and change sign.
 *  PCM = ( PCM-R | (PCM-R>>16) ) ^ sign
 */
void SC68mixer_dup_R_to_L(u32 *dest, u32 *src, int nb,
			  const u32 sign)
{
  u32 *end;
  int rem;

  rem = nb&3;
  end = dest+(nb-rem);
  if (dest < end) {
    do {
      u32 v;
      v = *src++&0xFFFF0000; *dest++ = ((v>>16)|v) ^ sign;
      v = *src++&0xFFFF0000; *dest++ = ((v>>16)|v) ^ sign;
      v = *src++&0xFFFF0000; *dest++ = ((v>>16)|v) ^ sign;
      v = *src++&0xFFFF0000; *dest++ = ((v>>16)|v) ^ sign;
    } while (dest < end);
  }

  if (rem&1) {
    u32 v;
    v = *src++&0xFFFF0000; *dest++ = ((v>>16)|v) ^ sign;
  }

  if (rem&2) {
    u32 v;
    v = *src++&0xFFFF0000; *dest++ = ((v>>16)|v) ^ sign;
    v = *src++&0xFFFF0000; *dest++ = ((v>>16)|v) ^ sign;
  }
}

/*  Blend Left and right voice :
 *  factor [0..65536], 0:blend nothing, 65536:swap L/R
 */
void SC68mixer_blend_LR(u32 *dest, u32 *src, int nb,
			int factor,
			const u32 sign_r,const u32 sign_w)
{
  u32 *end;
  int rem;
  int oof;

  if (factor < 0) {
    factor = 0;
  } else if (factor > 65536) {
    factor = 65536;
  }

#undef  MIX_ONE
#define MIX_ONE \
 r = (int)(s32)(*src++ ^ sign_r);\
 l = r >> 16;\
 r = (int)(s16)r;\
 *dest++ = (\
  ((l*oof+r*factor)&0xFFFF0000)\
  |\
  ((u32)(r*oof+l*factor) >> 16)\
 ) ^ sign_w

  oof = 65536-factor;
  rem = nb&3;
  end = dest+(nb-rem);
  if (dest<end) {
    do {
      int l,r;

      MIX_ONE;
      MIX_ONE;
      MIX_ONE;
      MIX_ONE;
    } while(dest<end);
  }

  if (rem&1) {
    int l,r;
    MIX_ONE;
  }
  if(rem&2) {
    int l,r;
    MIX_ONE;
    MIX_ONE;
  }
}

/*  Multiply left/right (signed) channel by ml/mr factor [-65536..65536]
 */
void SC68mixer_mult_LR(u32 *dest, u32 *src, int nb,
		       const int ml, const int mr,
           const u32 sign_r, const u32 sign_w)
{
  u32 * end;
  int rem;

  /* Optimize some trivial case. */

  if (ml == 65536 && mr == 65536) {
    SC68mixer_stereo_16_LR(dest, src, nb, sign_r ^ sign_w);
    return;
  }

  if (ml==0 && mr==0) {
    SC68mixer_fill(dest, nb, sign_w);
    return;
  }


#undef  MIX_ONE
#define MIX_ONE \
 r = (int)(s32)(*src++ ^ sign_r);\
 l = (int)(s16)r;\
 r = r >> 16;\
 *dest++ = ((((u32)(l*ml))>>16) | ((r*mr)&0xFFFF0000)) ^ sign_w

  rem = nb&3;
  end = dest+(nb-rem);
  if (dest < end) {
    do {
    int l,r;
      MIX_ONE;
      MIX_ONE;
      MIX_ONE;
      MIX_ONE;
    } while (dest < end);
  }

  if (rem&1) {
    int l,r;
    MIX_ONE;
  }
  
  if (rem&2) {
    int l,r;
    MIX_ONE;
    MIX_ONE;
  }
}

/*  Fill buffer sign with value (RRRRLLLL)
 */
void SC68mixer_fill(u32 *dest, int nb, const u32 sign)
{
  u32 * end;
  int rem;

  rem = nb&3;
  end = dest+(nb-rem);
  if (dest < end) {
    do {
      *dest++ = sign;
      *dest++ = sign;
      *dest++ = sign;
      *dest++ = sign;
    } while (dest < end);
  }

  if (rem&1) {
    *dest++ = sign;
  }

  if (rem&2) {
    *dest++ = sign;
    *dest++ = sign;
  }
}
