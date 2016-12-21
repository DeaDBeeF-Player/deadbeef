
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2013 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/***************************  Fraunhofer IIS FDK Tools  **********************

   Author(s):   Josef Hoepfl, Manuel Jander
   Description: MDCT routines

******************************************************************************/

#include "mdct.h"


#include "FDK_tools_rom.h"
#include "dct.h"
#include "fixpoint_math.h"


void mdct_init( H_MDCT hMdct,
                FIXP_DBL *overlap,
                INT overlapBufferSize )
{
  hMdct->overlap.freq = overlap;
  //FDKmemclear(overlap, overlapBufferSize*sizeof(FIXP_DBL));
  hMdct->prev_fr = 0;
  hMdct->prev_nr = 0;
  hMdct->prev_tl = 0;
  hMdct->ov_size = overlapBufferSize;
}


void imdct_gain(FIXP_DBL *pGain_m, int *pGain_e, int tl)
{
  FIXP_DBL gain_m = *pGain_m;
  int gain_e = *pGain_e;
  int log2_tl;

  log2_tl = DFRACT_BITS-1-fNormz((FIXP_DBL)tl);

  gain_e += -MDCT_OUTPUT_GAIN - log2_tl - MDCT_OUT_HEADROOM + 1;

  /* Detect non-radix 2 transform length and add amplitude compensation factor
     which cannot be included into the exponent above */
  switch ( (tl) >> (log2_tl - 2) ) {
    case 0x7: /* 10 ms, 1/tl = 1.0/(FDKpow(2.0, -log2_tl) * 0.53333333333333333333) */
      if (gain_m == (FIXP_DBL)0) {
        gain_m = FL2FXCONST_DBL(0.53333333333333333333f);
      } else {
        gain_m = fMult(gain_m, FL2FXCONST_DBL(0.53333333333333333333f));
      }
      break;
    case 0x6: /* 3/4 of radix 2, 1/tl = 1.0/(FDKpow(2.0, -log2_tl) * 2.0/3.0) */
      if (gain_m == (FIXP_DBL)0) {
        gain_m = FL2FXCONST_DBL(2.0/3.0f);
      } else {
        gain_m = fMult(gain_m, FL2FXCONST_DBL(2.0/3.0f));
      }
      break;
    case 0x4:
      /* radix 2, nothing to do. */
      break;
    default:
      /* unsupported */
      FDK_ASSERT(0);
      break;
  }

  *pGain_m = gain_m;
  *pGain_e = gain_e;
}

INT imdct_drain(
        H_MDCT hMdct,
        FIXP_DBL *output,
        INT nrSamplesRoom
        )
{
  int buffered_samples = 0;

  if (nrSamplesRoom > 0) {
    buffered_samples = hMdct->ov_offset;

    FDK_ASSERT(buffered_samples <= nrSamplesRoom);

    if (buffered_samples > 0)  {
      FDKmemcpy(output, hMdct->overlap.time, buffered_samples*sizeof(FIXP_DBL));
      hMdct->ov_offset = 0;
    }
  }
  return buffered_samples;
}

INT imdct_copy_ov_and_nr(
        H_MDCT hMdct,
        FIXP_DBL * pTimeData,
        INT nrSamples
        )
{
  FIXP_DBL *pOvl;
  int nt, nf, i;

  nt = fMin(hMdct->ov_offset, nrSamples);
  nrSamples -= nt;
  nf = fMin(hMdct->prev_nr, nrSamples);
  nrSamples -= nf;
  FDKmemcpy(pTimeData, hMdct->overlap.time, nt*sizeof(FIXP_DBL));
  pTimeData += nt;

  pOvl = hMdct->overlap.freq + hMdct->ov_size - 1;
  for (i=0; i<nf; i++) {
    FIXP_DBL x = - (*pOvl--);
    *pTimeData = IMDCT_SCALE_DBL(x);
    pTimeData ++;
  }

  return (nt+nf);
}

void imdct_adapt_parameters(H_MDCT hMdct, int *pfl, int *pnl, int tl, const FIXP_WTP *wls, int noOutSamples)
{
  int fl = *pfl, nl = *pnl;
  int window_diff, use_current = 0, use_previous = 0;
  if (hMdct->prev_tl == 0) {
    hMdct->prev_wrs    = wls;
    hMdct->prev_fr     = fl;
    hMdct->prev_nr     = (noOutSamples-fl)>>1;
    hMdct->prev_tl     = noOutSamples;
    hMdct->ov_offset   = 0;
    use_current = 1;
  }

  window_diff = (hMdct->prev_fr - fl)>>1;

  /* check if the previous window slope can be adjusted to match the current window slope */
  if (hMdct->prev_nr + window_diff > 0) {
    use_current = 1;
  }
  /* check if the current window slope can be adjusted to match the previous window slope */
  if (nl - window_diff > 0 ) {
    use_previous = 1;
  }

  /* if both is possible choose the larger of both window slope lengths */
  if (use_current && use_previous) {
    if (fl < hMdct->prev_fr) {
      use_current = 0;
    } else {
      use_previous = 0;
    }
  }
  /*
   * If the previous transform block is big enough, enlarge previous window overlap,
   * if not, then shrink current window overlap.
   */
  if (use_current) {
    hMdct->prev_nr += window_diff;
    hMdct->prev_fr = fl;
    hMdct->prev_wrs = wls;
  } else {
    nl -= window_diff;
    fl = hMdct->prev_fr;
  }

  *pfl = fl;
  *pnl = nl;
}

INT  imdct_block(
        H_MDCT hMdct,
        FIXP_DBL *output,
        FIXP_DBL *spectrum,
        const SHORT scalefactor[],
        const INT nSpec,
        const INT noOutSamples,
        const INT tl,
        const FIXP_WTP *wls,
        INT fl,
        const FIXP_WTP *wrs,
        const INT fr,
        FIXP_DBL gain
        )
{
  FIXP_DBL *pOvl;
  FIXP_DBL *pOut0 = output, *pOut1;
  INT nl, nr;
  int w, i, nrSamples = 0, specShiftScale, transform_gain_e = 0;

  /* Derive NR and NL */
  nr = (tl - fr)>>1;
  nl = (tl - fl)>>1;

  /* Include 2/N IMDCT gain into gain factor and exponent. */
  imdct_gain(&gain, &transform_gain_e, tl);

  /* Detect FRprevious / FL mismatches and override parameters accordingly */
  if (hMdct->prev_fr != fl) {
    imdct_adapt_parameters(hMdct, &fl, &nl, tl, wls, noOutSamples);
  }

  pOvl = hMdct->overlap.freq + hMdct->ov_size - 1;

  if ( noOutSamples > nrSamples ) {
    /* Purge buffered output. */
    for (i=0; i<hMdct->ov_offset; i++) {
      *pOut0 = hMdct->overlap.time[i];
      pOut0 ++;
    }
    nrSamples = hMdct->ov_offset;
    hMdct->ov_offset = 0;
  }

  for (w=0; w<nSpec; w++)
  {
    FIXP_DBL *pSpec, *pCurr;
    const FIXP_WTP *pWindow;

    specShiftScale = transform_gain_e;

    /* Setup window pointers */
    pWindow = hMdct->prev_wrs;

    /* Current spectrum */
    pSpec = spectrum+w*tl;

    /* DCT IV of current spectrum. */
    dct_IV(pSpec, tl, &specShiftScale);

    /* Optional scaling of time domain - no yet windowed - of current spectrum */
    /* and de-scale current spectrum signal (time domain, no yet windowed) */	
    if (gain != (FIXP_DBL)0) {
      scaleValuesWithFactor(pSpec, gain, tl, scalefactor[w] + specShiftScale);
    } else {
      scaleValues(pSpec, tl, scalefactor[w] + specShiftScale);
    }

    if ( noOutSamples <= nrSamples ) {
      /* Divert output first half to overlap buffer if we already got enough output samples. */
      pOut0 = hMdct->overlap.time + hMdct->ov_offset;
      hMdct->ov_offset += hMdct->prev_nr + fl/2;
    } else {
      /* Account output samples */
      nrSamples += hMdct->prev_nr + fl/2;
    }

    /* NR output samples 0 .. NR. -overlap[TL/2..TL/2-NR] */
    for (i=0; i<hMdct->prev_nr; i++) {
      FIXP_DBL x = - (*pOvl--);
      *pOut0 = IMDCT_SCALE_DBL(x);
      pOut0 ++;
    }

    if ( noOutSamples <= nrSamples ) {
      /* Divert output second half to overlap buffer if we already got enough output samples. */
      pOut1 = hMdct->overlap.time + hMdct->ov_offset + fl/2 - 1;
      hMdct->ov_offset += fl/2 + nl;
    } else {
      pOut1 = pOut0 + (fl - 1);
      nrSamples += fl/2 + nl;
    }

    /* output samples before window crossing point NR .. TL/2. -overlap[TL/2-NR..TL/2-NR-FL/2] + current[NR..TL/2] */
    /* output samples after window crossing point TL/2 .. TL/2+FL/2. -overlap[0..FL/2] - current[TL/2..FL/2] */
    pCurr = pSpec + tl - fl/2;
    for (i=0; i<fl/2; i++) {
      FIXP_DBL x0, x1;

      cplxMult(&x1, &x0, *pCurr++, - *pOvl--, pWindow[i]);
      *pOut0 = IMDCT_SCALE_DBL(x0);
      *pOut1 = IMDCT_SCALE_DBL(-x1);
      pOut0 ++;
      pOut1 --;
    }
    pOut0 += (fl/2);

    /* NL output samples TL/2+FL/2..TL. - current[FL/2..0] */
    pOut1 += (fl/2) + 1;
    pCurr = pSpec + tl - fl/2 - 1;
    for (i=0; i<nl; i++) {
      FIXP_DBL x = - (*pCurr--);
      *pOut1 = IMDCT_SCALE_DBL(x);
      pOut1 ++;
    }

    /* Set overlap source pointer for next window pOvl = pSpec + tl/2 - 1; */
    pOvl = pSpec + tl/2 - 1;

    /* Previous window values. */
    hMdct->prev_nr = nr;
    hMdct->prev_fr = fr;
    hMdct->prev_tl = tl;
    hMdct->prev_wrs = wrs;
  }

  /* Save overlap */
  
  pOvl = hMdct->overlap.freq + hMdct->ov_size - tl/2;
  FDK_ASSERT(pOvl >= hMdct->overlap.time + hMdct->ov_offset);
  FDK_ASSERT(tl/2 <= hMdct->ov_size);
  for (i=0; i<tl/2; i++) {
    pOvl[i] = spectrum[i+(nSpec-1)*tl];
  }

  return nrSamples;
}

