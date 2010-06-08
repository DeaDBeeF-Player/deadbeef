/****************************************************************************

  emu76489.c -- SN76489 emulator by [OK] 2001-2004

  2001 08-13 : Version 1.00
  2001 10-03 : Version 1.01 -- Added SNG_set_quality().
  2004 05-23 : Version 1.10 -- Implemented GG stereo mode by RuRuRu

  References: 
    SN76489 data sheet   
    sn76489.c            -- in MAME

*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emu76489.h"

static e_uint32 voltbl[16] = {
  0xff, 0xcb, 0xa1, 0x80, 0x65, 0x50, 0x40, 0x33, 0x28, 0x20, 0x19, 0x14, 0x10, 0x0c, 0x0a, 0x00
};

#define GETA_BITS 24

static void
internal_refresh (SNG * sng)
{
  if (sng->quality)
  {
    sng->base_incr = 1 << GETA_BITS;
    sng->realstep = (e_uint32) ((1 << 31) / sng->rate);
    sng->sngstep = (e_uint32) ((1 << 31) / (sng->clk / 16));
    sng->sngtime = 0;
  }
  else
  {
    sng->base_incr = (e_uint32) ((double) sng->clk * (1 << GETA_BITS) / (16 * sng->rate));
  }
}

EMU76489_API void
SNG_set_rate (SNG * sng, e_uint32 r)
{
  sng->rate = r ? r : 44100;
  internal_refresh (sng);
}

EMU76489_API void
SNG_set_quality (SNG * sng, e_uint32 q)
{
  sng->quality = q;
  internal_refresh (sng);
}

EMU76489_API SNG *
SNG_new (e_uint32 c, e_uint32 r)
{
  SNG *sng;

  sng = (SNG *) malloc (sizeof (SNG));
  if (sng == NULL)
    return NULL;

  sng->clk = c;
  sng->rate = r ? r : 44100;
  SNG_set_quality (sng, 0);

  return sng;
}

EMU76489_API void
SNG_reset (SNG * sng)
{
  int i;

  sng->base_count = 0;

  for (i = 0; i < 3; i++)
  {
    sng->count[i] = 0;
    sng->freq[i] = 0;
    sng->edge[i] = 0;
    sng->volume[i] = 0x0f;
    sng->mute[i] = 0;
  }

  sng->adr = 0;

  sng->noise_seed = 0xffff;
  sng->noise_count = 0;
  sng->noise_freq = 0;
  sng->noise_volume = 0x0f;
  sng->noise_mode = 0;

  sng->out = 0;

  sng->out2 = 0;
  sng->stereo = 0xFF;
}

EMU76489_API void
SNG_delete (SNG * sng)
{
  free (sng);
}

EMU76489_API void
SNG_writeIO (SNG * sng, e_uint32 val)
{
  if (val & 0x80)
  {
    sng->adr = (val & 0x70) >> 4;
    switch (sng->adr)
    {
    case 0:
    case 2:
    case 4:
      sng->freq[sng->adr >> 1] = (sng->freq[sng->adr >> 1] & 0x3F0) | (val & 0x0F);
      break;

    case 1:
    case 3:
    case 5:
      sng->volume[(sng->adr - 1) >> 1] = val & 0xF;
      break;

    case 6:
      sng->noise_mode = (val & 4) >> 2;

      if ((val & 0x03) == 0x03)
        sng->noise_freq = sng->freq[2];
      else
        sng->noise_freq = 32 << (val & 0x03);

      if (sng->noise_freq == 0)
        sng->noise_freq = 1;

      sng->noise_seed = 0xf35;
      break;

    case 7:
      sng->noise_volume = val & 0x0f;
      break;
    }
  }
  else
  {
    sng->freq[sng->adr >> 1] = ((val & 0x3F) << 4) | (sng->freq[sng->adr >> 1] & 0x0F);
  }
}

INLINE static e_int16
calc (SNG * sng)
{

  int i;
  e_uint32 incr;
  e_int32 mix = 0;

  sng->base_count += sng->base_incr;
  incr = (sng->base_count >> GETA_BITS);
  sng->base_count &= (1 << GETA_BITS) - 1;

  /* Noise */
  sng->noise_count += incr;
  if (sng->noise_count & 0x100)
  {
    if (sng->noise_seed & 1)
    {
      if (sng->noise_mode)
        sng->noise_seed ^= 0x12000;     /* White */
      else
        sng->noise_seed ^= 0x08000;     /* Periodic */
    }
    sng->noise_seed >>= 1;
    sng->noise_count -= sng->noise_freq;
  }

  if (sng->noise_seed & 1)
    mix = voltbl[sng->noise_volume];

  /* Tone */
  for (i = 0; i < 3; i++)
  {
    sng->count[i] += incr;
    if (sng->count[i] & 0x400)
    {
      if (sng->freq[i] > 1)
      {
        sng->edge[i] = !sng->edge[i];
        sng->count[i] -= sng->freq[i];
      }
      else
      {
        sng->edge[i] = 1;
      }
    }

    if (sng->edge[i] && !sng->mute[i])
    {
      mix += voltbl[sng->volume[i]];
    }
  }

  return (e_int16) mix;

}

EMU76489_API e_int16
SNG_calc (SNG * sng)
{
  if (!sng->quality)
    return (e_int16) (calc (sng) << 4);
  /* Simple rate converter */
  while (sng->realstep > sng->sngtime)
  {
    sng->sngtime += sng->sngstep;
    sng->out += calc (sng);
    sng->out >>= 1;
  }

  sng->sngtime = sng->sngtime - sng->realstep;

  return (e_int16) (sng->out << 4);
}

INLINE static void
calc_stereo (SNG * sng, e_int32 out[2])
{

  int i;
  e_uint32 incr;
  e_int32 lmix =0, rmix = 0;

  sng->base_count += sng->base_incr;
  incr = (sng->base_count >> GETA_BITS);
  sng->base_count &= (1 << GETA_BITS) - 1;

  /* Noise */
  sng->noise_count += incr;
  if (sng->noise_count & 0x100)
  {
    if (sng->noise_seed & 1)
    {
      if (sng->noise_mode)
        sng->noise_seed ^= 0x12000;     /* White */
      else
        sng->noise_seed ^= 0x08000;     /* Periodic */
    }
    sng->noise_seed >>= 1;
    sng->noise_count -= sng->noise_freq;
  }

  if (sng->noise_seed & 1) {
    if (sng->stereo >>4 & 0x08) {
      lmix = voltbl[sng->noise_volume];
	}
    if (sng->stereo & 0x08) {
      rmix = voltbl[sng->noise_volume];
    }
  }

  /* Tone */
  for (i = 0; i < 3; i++)
  {
    sng->count[i] += incr;
    if (sng->count[i] & 0x400)
    {
      if (sng->freq[i] > 1)
      {
        sng->edge[i] = !sng->edge[i];
        sng->count[i] -= sng->freq[i];
      }
      else
      {
        sng->edge[i] = 1;
      }
    }

    if (sng->edge[i] && !sng->mute[i])
    {
      if (sng->stereo >> (i+4) & 0x01) {
        lmix += voltbl[sng->volume[i]];
	  }
      if (sng->stereo >> i & 0x01) {
        rmix += voltbl[sng->volume[i]];
      }
    }
  }

  out[0] = lmix;
  out[1] = rmix;
  return;

}

EMU76489_API void
SNG_calc_stereo (SNG *sng, e_int32 out[2])
{
  if (!sng->quality) {
    calc_stereo (sng, out);
    out[0] = out[0] << 4;
    out[1] = out[1] << 4;
    return;
  }

  while (sng->realstep > sng->sngtime)
  {
    sng->sngtime += sng->sngstep;
    calc_stereo(sng, out);
    sng->out  += out[0];
    sng->out2 += out[1];
    sng->out  >>= 1;
    sng->out2 >>= 1;
  }

  sng->sngtime = sng->sngtime - sng->realstep;
  out[0] = (e_int16) (sng->out  << 4);
  out[1] = (e_int16) (sng->out2 << 4);

  return;
}

EMU76489_API void
SNG_writeGGIO(SNG *sng, e_uint32 val)
{
  sng->stereo = val;
}
