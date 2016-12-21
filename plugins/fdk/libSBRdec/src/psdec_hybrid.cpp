
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

#include "psdec_hybrid.h"


#include "fft.h"
#include "sbr_ram.h"

#include "FDK_tools_rom.h"
#include "sbr_rom.h"

/*******************************************************************************
 Functionname:  InitHybridFilterBank
 *******************************************************************************

 Description:   Init one instance of HANDLE_HYBRID stuct

 Arguments:

 Return:        none

*******************************************************************************/


SBR_ERROR
InitHybridFilterBank ( HANDLE_HYBRID hs,          /*!< Handle to HYBRID struct. */
                       SCHAR frameSize,           /*!< Framesize (in Qmf súbband samples). */
                       SCHAR noBands,             /*!< Number of Qmf bands for hybrid filtering. */
                       const UCHAR *pResolution ) /*!< Resolution in Qmf bands (length noBands). */
{
  SCHAR i;
  UCHAR maxNoChannels = 0;

  for (i = 0; i < noBands; i++) {
    hs->pResolution[i] = pResolution[i];
    if(pResolution[i] > maxNoChannels)
      maxNoChannels = pResolution[i];
  }

  hs->nQmfBands     = noBands;
  hs->frameSize     = frameSize;
  hs->qmfBufferMove = HYBRID_FILTER_LENGTH - 1;

  hs->sf_mQmfBuffer = 0;

  return SBRDEC_OK;
}

/*******************************************************************************
 Functionname:  dualChannelFiltering
 *******************************************************************************

 Description:   fast 2-channel real-valued filtering with 6-tap delay.

 Arguments:

 Return:        none

*******************************************************************************/

/*!
2 channel filter
<pre>
   Filter Coefs:
   0.0,
   0.01899487526049,
   0.0,
   -0.07293139167538,
   0.0,
   0.30596630545168,
   0.5,
   0.30596630545168,
   0.0,
   -0.07293139167538,
   0.0,
   0.01899487526049,
   0.0


   Filter design:
   h[q,n] = g[n] * cos(2pi/2 * q * (n-6) );  n = 0..12,  q = 0,1;

   ->  h[0,n] = g[n] * 1;
   ->  h[1,n] = g[n] * pow(-1,n);
</pre>
*/

static void slotBasedDualChannelFiltering( const FIXP_DBL *pQmfReal,
                                           const FIXP_DBL *pQmfImag,

                                           FIXP_DBL       *mHybridReal,
                                           FIXP_DBL       *mHybridImag)
{

  FIXP_DBL  t1, t3, t5, t6;

  /* symmetric filter coefficients */

  /*  you don't have to shift the result after fMult because of p2_13_20 <= 0.5 */
  t1 = fMultDiv2(p2_13_20[1] , ( (pQmfReal[1] >> 1) + (pQmfReal[11] >> 1)));
  t3 = fMultDiv2(p2_13_20[3] , ( (pQmfReal[3] >> 1) + (pQmfReal[ 9] >> 1)));
  t5 = fMultDiv2(p2_13_20[5] , ( (pQmfReal[5] >> 1) + (pQmfReal[ 7] >> 1)));
  t6 = fMultDiv2(p2_13_20[6] ,   (pQmfReal[6] >> 1) );

  mHybridReal[0] = (t1 + t3 + t5 + t6) << 2;
  mHybridReal[1] = (- t1 - t3 - t5 + t6) << 2;

  t1 = fMultDiv2(p2_13_20[1] , ( (pQmfImag[1] >> 1) + (pQmfImag[11] >> 1)));
  t3 = fMultDiv2(p2_13_20[3] , ( (pQmfImag[3] >> 1) + (pQmfImag[ 9] >> 1)));
  t5 = fMultDiv2(p2_13_20[5] , ( (pQmfImag[5] >> 1) + (pQmfImag[ 7] >> 1)));
  t6 = fMultDiv2(p2_13_20[6] ,    pQmfImag[6] >> 1 );

  mHybridImag[0] = (t1 + t3 + t5 + t6) << 2;
  mHybridImag[1] = (- t1 - t3 - t5 + t6) << 2;
}


/*******************************************************************************
 Functionname:  eightChannelFiltering
 *******************************************************************************

 Description:   fast 8-channel complex-valued filtering with 6-tap delay.

 Arguments:

 Return:        none

*******************************************************************************/
/*!
   8 channel filter

   Implementation using a FFT of length 8
<pre>
   prototype filter coefficients:
   0.00746082949812   0.02270420949825   0.04546865930473   0.07266113929591   0.09885108575264   0.11793710567217
   0.125
   0.11793710567217   0.09885108575264   0.07266113929591   0.04546865930473   0.02270420949825   0.00746082949812

   Filter design:
   N = 13; Q = 8;
   h[q,n]       = g[n] * exp(j * 2 * pi / Q * (q + .5) * (n - 6));  n = 0..(N-1),  q = 0..(Q-1);

   Time Signal:   x[t];
   Filter Bank Output
   y[q,t] = conv(x[t],h[q,t]) = conv(h[q,t],x[t]) = sum(x[k] * h[q, t - k] ) = sum(h[q, k] * x[t - k] ); k = 0..(N-1);

   y[q,t] =   x[t - 12]*h[q, 12]  +  x[t - 11]*h[q, 11]  +  x[t - 10]*h[q, 10]  +  x[t -  9]*h[q,  9]
           +  x[t -  8]*h[q,  8]  +  x[t -  7]*h[q,  7]
           +  x[t -  6]*h[q,  6]
           +  x[t -  5]*h[q,  5]  +  x[t -  4]*h[q,  4]
           +  x[t -  3]*h[q,  3]  +  x[t -  2]*h[q,  2]  +  x[t -  1]*h[q,  1]  +  x[t -  0]*h[q,  0];

   h'[q, n] = h[q,(N-1)-n] = g[n] * exp(j * 2 * pi / Q * (q + .5) * (6 - n));  n = 0..(N-1),  q = 0..(Q-1);

   y[q,t] =   x[t - 12]*h'[q,  0]  +  x[t - 11]*h'[q,  1]  +  x[t - 10]*h'[q,  2]  +  x[t -  9]*h'[q,  3]
           +  x[t -  8]*h'[q,  4]  +  x[t -  7]*h'[q,  5]
           +  x[t -  6]*h'[q,  6]
           +  x[t -  5]*h'[q,  7]  +  x[t -  4]*h'[q,  8]
           +  x[t -  3]*h'[q,  9]  +  x[t -  2]*h'[q, 10]  +  x[t -  1]*h'[q, 11]  +  x[t -  0]*h'[q, 12];

   Try to split off FFT Modulation Term:
   FFT(x[t], q) = sum(x[t+k]*exp(-j*2*pi/N *q * k))
                                           c                                           m
   Step 1:  h'[q,n] = g[n] * ( exp(j * 2 * pi / 8 * .5 * (6 - n)) ) * ( exp (j * 2 * pi / 8 * q * (6 - n)) );

    h'[q,n] = g[n] *c[n] * m[q,n]; (see above)
    c[n]    = exp( j * 2 * pi / 8 * .5 * (6 - n) );
    m[q,n]  = exp( j * 2 * pi / 8 *  q * (6 - n) );

    y[q,t] = x[t -  0]*g[0]*c[0]*m[q,0]  +  x[t -  1]*g[1]*c[ 1]*m[q, 1]  + ...
             ...                         +  x[t - 12]*g[2]*c[12]*m[q,12];

                                                                              |
    n                   m                            *exp(-j*2*pi)            |   n'                   fft
-------------------------------------------------------------------------------------------------------------------------
    0       exp( j * 2 * pi / 8 * q * 6) ->  exp(-j * 2 * pi / 8 * q * 2)     |   2         exp(-j * 2 * pi / 8 * q * 0)
    1       exp( j * 2 * pi / 8 * q * 5) ->  exp(-j * 2 * pi / 8 * q * 3)     |   3         exp(-j * 2 * pi / 8 * q * 1)
    2       exp( j * 2 * pi / 8 * q * 4) ->  exp(-j * 2 * pi / 8 * q * 4)     |   4         exp(-j * 2 * pi / 8 * q * 2)
    3       exp( j * 2 * pi / 8 * q * 3) ->  exp(-j * 2 * pi / 8 * q * 5)     |   5         exp(-j * 2 * pi / 8 * q * 3)
    4       exp( j * 2 * pi / 8 * q * 2) ->  exp(-j * 2 * pi / 8 * q * 6)     |   6         exp(-j * 2 * pi / 8 * q * 4)
    5       exp( j * 2 * pi / 8 * q * 1) ->  exp(-j * 2 * pi / 8 * q * 7)     |   7         exp(-j * 2 * pi / 8 * q * 5)
    6       exp( j * 2 * pi / 8 * q * 0)                                      |   0         exp(-j * 2 * pi / 8 * q * 6)
    7       exp(-j * 2 * pi / 8 * q * 1)                                      |   1         exp(-j * 2 * pi / 8 * q * 7)
    8       exp(-j * 2 * pi / 8 * q * 2)                                      |   2
    9       exp(-j * 2 * pi / 8 * q * 3)                                      |   3
    10      exp(-j * 2 * pi / 8 * q * 4)                                      |   4
    11      exp(-j * 2 * pi / 8 * q * 5)                                      |   5
    12      exp(-j * 2 * pi / 8 * q * 6)                                      |   6


    now use fft modulation coefficients
    m[6]  =       = fft[0]
    m[7]  =       = fft[1]
    m[8]  = m[ 0] = fft[2]
    m[9]  = m[ 1] = fft[3]
    m[10] = m[ 2] = fft[4]
    m[11] = m[ 3] = fft[5]
    m[12] = m[ 4] = fft[6]
            m[ 5] = fft[7]

    y[q,t] = (                       x[t- 6]*g[ 6]*c[ 6] ) * fft[q,0]  +
             (                       x[t- 7]*g[ 7]*c[ 7] ) * fft[q,1]  +
             ( x[t- 0]*g[ 0]*c[ 0] + x[t- 8]*g[ 8]*c[ 8] ) * fft[q,2]  +
             ( x[t- 1]*g[ 1]*c[ 1] + x[t- 9]*g[ 9]*c[ 9] ) * fft[q,3]  +
             ( x[t- 2]*g[ 2]*c[ 2] + x[t-10]*g[10]*c[10] ) * fft[q,4]  +
             ( x[t- 3]*g[ 3]*c[ 3] + x[t-11]*g[11]*c[11] ) * fft[q,5]  +
             ( x[t- 4]*g[ 4]*c[ 4] + x[t-12]*g[12]*c[12] ) * fft[q,6]  +
             ( x[t- 5]*g[ 5]*c[ 5]                       ) * fft[q,7];

    pre twiddle factors c[n] = exp(j * 2 * pi / 8 * .5 * (6 - n));
    n                c]           |  n                c[n]         |  n                c[n]
---------------------------------------------------------------------------------------------------
    0       exp( j * 6 * pi / 8)  |  1       exp( j * 5 * pi / 8)  |  2       exp( j * 4 * pi / 8)
    3       exp( j * 3 * pi / 8)  |  4       exp( j * 2 * pi / 8)  |  5       exp( j * 1 * pi / 8)
    6       exp( j * 0 * pi / 8)  |  7       exp(-j * 1 * pi / 8)  |  8       exp(-j * 2 * pi / 8)
    9       exp(-j * 3 * pi / 8)  | 10       exp(-j * 4 * pi / 8)  | 11       exp(-j * 5 * pi / 8)
   12       exp(-j * 6 * pi / 8)  |                                |
</pre>
*/

/* defining rotation factors for *ChannelFiltering */

#define cos0Pi    FL2FXCONST_DBL( 1.f)
#define sin0Pi    FL2FXCONST_DBL( 0.f)

#define cos1Pi    FL2FXCONST_DBL(-1.f)
#define sin1Pi    FL2FXCONST_DBL( 0.f)

#define cos1Pi_2  FL2FXCONST_DBL( 0.f)
#define sin1Pi_2  FL2FXCONST_DBL( 1.f)

#define cos1Pi_3  FL2FXCONST_DBL( 0.5f)
#define sin1Pi_3  FL2FXCONST_DBL( 0.86602540378444f)

#define cos0Pi_4  cos0Pi
#define cos1Pi_4  FL2FXCONST_DBL(0.70710678118655f)
#define cos2Pi_4  cos1Pi_2
#define cos3Pi_4  (-cos1Pi_4)
#define cos4Pi_4  (-cos0Pi_4)
#define cos5Pi_4  cos3Pi_4
#define cos6Pi_4  cos2Pi_4

#define sin0Pi_4  sin0Pi
#define sin1Pi_4  FL2FXCONST_DBL(0.70710678118655f)
#define sin2Pi_4  sin1Pi_2
#define sin3Pi_4  sin1Pi_4
#define sin4Pi_4  sin0Pi_4
#define sin5Pi_4  (-sin3Pi_4)
#define sin6Pi_4  (-sin2Pi_4)

#define cos0Pi_8  cos0Pi
#define cos1Pi_8  FL2FXCONST_DBL(0.92387953251129f)
#define cos2Pi_8  cos1Pi_4
#define cos3Pi_8  FL2FXCONST_DBL(0.38268343236509f)
#define cos4Pi_8  cos2Pi_4
#define cos5Pi_8  (-cos3Pi_8)
#define cos6Pi_8  (-cos2Pi_8)

#define sin0Pi_8  sin0Pi
#define sin1Pi_8  cos3Pi_8
#define sin2Pi_8  sin1Pi_4
#define sin3Pi_8  cos1Pi_8
#define sin4Pi_8  sin2Pi_4
#define sin5Pi_8  sin3Pi_8
#define sin6Pi_8  sin1Pi_4

#if defined(ARCH_PREFER_MULT_32x16)
  #define FIXP_HYB FIXP_SGL
  #define FIXP_CAST FX_DBL2FX_SGL
#else
  #define FIXP_HYB FIXP_DBL
  #define FIXP_CAST
#endif

static const FIXP_HYB  cr[13] =
{
   FIXP_CAST(cos6Pi_8), FIXP_CAST(cos5Pi_8), FIXP_CAST(cos4Pi_8),
   FIXP_CAST(cos3Pi_8), FIXP_CAST(cos2Pi_8), FIXP_CAST(cos1Pi_8),
   FIXP_CAST(cos0Pi_8),
   FIXP_CAST(cos1Pi_8), FIXP_CAST(cos2Pi_8), FIXP_CAST(cos3Pi_8),
   FIXP_CAST(cos4Pi_8), FIXP_CAST(cos5Pi_8), FIXP_CAST(cos6Pi_8)
};

static const FIXP_HYB  ci[13] =
{
   FIXP_CAST( sin6Pi_8), FIXP_CAST( sin5Pi_8), FIXP_CAST( sin4Pi_8),
   FIXP_CAST( sin3Pi_8), FIXP_CAST( sin2Pi_8), FIXP_CAST( sin1Pi_8),
   FIXP_CAST( sin0Pi_8) ,
   FIXP_CAST(-sin1Pi_8), FIXP_CAST(-sin2Pi_8), FIXP_CAST(-sin3Pi_8),
   FIXP_CAST(-sin4Pi_8), FIXP_CAST(-sin5Pi_8), FIXP_CAST(-sin6Pi_8)
};

static void slotBasedEightChannelFiltering( const FIXP_DBL *pQmfReal,
                                            const FIXP_DBL *pQmfImag,

                                            FIXP_DBL  *mHybridReal,
                                            FIXP_DBL  *mHybridImag)
{

  int bin;
  FIXP_DBL _fft[128 + ALIGNMENT_DEFAULT - 1];
  FIXP_DBL *fft = (FIXP_DBL *)ALIGN_PTR(_fft);

#if defined(ARCH_PREFER_MULT_32x16)
  const FIXP_SGL *p = p8_13_20; /* BASELINE_PS */
#else
  const FIXP_DBL *p = p8_13_20; /* BASELINE_PS */
#endif

  /* pre twiddeling */

  /*   x*(a*b + c*d) = fMultDiv2(x, fMultAddDiv2(fMultDiv2(a, b), c, d)) */
  /*   x*(a*b - c*d) = fMultDiv2(x, fMultSubDiv2(fMultDiv2(a, b), c, d)) */
  FIXP_DBL accu1, accu2, accu3, accu4;

  #define TWIDDLE_1(n_0,n_1,n_2)                                                        \
         cplxMultDiv2(&accu1, &accu2, pQmfReal[n_0], pQmfImag[n_0], cr[n_0], ci[n_0]);  \
         accu1 = fMultDiv2(p[n_0], accu1);                                              \
         accu2 = fMultDiv2(p[n_0], accu2);                                              \
         cplxMultDiv2(&accu3, &accu4, pQmfReal[n_1], pQmfImag[n_1], cr[n_1], ci[n_1]);  \
         accu3 = fMultDiv2(p[n_1], accu3);                                              \
         accu4 = fMultDiv2(p[n_1], accu4);                                              \
         fft[FIXP_FFT_IDX_R(n_2)] = accu1 + accu3;                                      \
         fft[FIXP_FFT_IDX_I(n_2)] = accu2 + accu4;

  #define TWIDDLE_0(n_0,n_1)                                                            \
         cplxMultDiv2(&accu1, &accu2, pQmfReal[n_0], pQmfImag[n_0], cr[n_0], ci[n_0]);  \
         fft[FIXP_FFT_IDX_R(n_1)] = fMultDiv2(p[n_0], accu1);                           \
         fft[FIXP_FFT_IDX_I(n_1)] = fMultDiv2(p[n_0], accu2);

  TWIDDLE_0( 6, 0)
  TWIDDLE_0( 7, 1)

  TWIDDLE_1( 0, 8, 2)
  TWIDDLE_1( 1, 9, 3)
  TWIDDLE_1( 2,10, 4)
  TWIDDLE_1( 3,11, 5)
  TWIDDLE_1( 4,12, 6)

  TWIDDLE_0( 5, 7)

  fft_8 (fft);

  /* resort fft data into output array*/
  for(bin=0; bin<8;bin++ ) {
    mHybridReal[bin] = fft[FIXP_FFT_IDX_R(bin)] << 4;
    mHybridImag[bin] = fft[FIXP_FFT_IDX_I(bin)] << 4;
  }
}


/*******************************************************************************
 Functionname:  fillHybridDelayLine
 *******************************************************************************

 Description:   The delay line of the hybrid filter is filled and copied from
                left to right.

 Return:        none

*******************************************************************************/

void
fillHybridDelayLine( FIXP_DBL **fixpQmfReal,          /*!< Qmf real Values    */
                     FIXP_DBL **fixpQmfImag,          /*!< Qmf imag Values    */
                     FIXP_DBL   fixpHybridLeftR[12],  /*!< Hybrid real Values left channel  */
                     FIXP_DBL   fixpHybridLeftI[12],  /*!< Hybrid imag Values left channel  */
                     FIXP_DBL   fixpHybridRightR[12], /*!< Hybrid real Values right channel */
                     FIXP_DBL   fixpHybridRightI[12], /*!< Hybrid imag Values right channel */
                     HANDLE_HYBRID hHybrid )
{
  int i;

  for (i = 0; i < HYBRID_FILTER_DELAY; i++) {
    slotBasedHybridAnalysis ( fixpQmfReal[i],
                              fixpQmfReal[i],
                              fixpHybridLeftR,
                              fixpHybridLeftI,
                              hHybrid );
  }

  FDKmemcpy(fixpHybridRightR, fixpHybridLeftR, sizeof(FIXP_DBL)*NO_SUB_QMF_CHANNELS);
  FDKmemcpy(fixpHybridRightI, fixpHybridLeftI, sizeof(FIXP_DBL)*NO_SUB_QMF_CHANNELS);
}


/*******************************************************************************
 Functionname:  slotBasedHybridAnalysis
 *******************************************************************************

 Description:   The lower QMF subbands are further split to provide better
                frequency resolution for PS processing.

  Return:        none

*******************************************************************************/


void
slotBasedHybridAnalysis ( FIXP_DBL *fixpQmfReal,      /*!< Qmf real Values */
                          FIXP_DBL *fixpQmfImag,      /*!< Qmf imag Values */

                          FIXP_DBL  fixpHybridReal[12],   /*!< Hybrid real Values */
                          FIXP_DBL  fixpHybridImag[12],   /*!< Hybrid imag Values */

                          HANDLE_HYBRID hHybrid)
{
  int  k, band;
  HYBRID_RES hybridRes;
  int  chOffset = 0;

  C_ALLOC_SCRATCH_START(pTempRealSlot, FIXP_DBL, 4*HYBRID_FILTER_LENGTH);

  FIXP_DBL *pTempImagSlot = pTempRealSlot + HYBRID_FILTER_LENGTH;
  FIXP_DBL *pWorkRealSlot = pTempImagSlot + HYBRID_FILTER_LENGTH;
  FIXP_DBL *pWorkImagSlot = pWorkRealSlot + HYBRID_FILTER_LENGTH;

  /*!
  Hybrid filtering is applied to the first hHybrid->nQmfBands QMF bands (3 when 10 or 20 stereo bands
  are used, 5 when 34 stereo bands are used). For the remaining QMF bands a delay would be necessary.
  But there is no need to implement a delay because there is a look-ahead of HYBRID_FILTER_DELAY = 6
  QMF samples in the low-band buffer.
  */

  for(band = 0; band < hHybrid->nQmfBands; band++) {

    /*  get hybrid resolution per qmf band                */
    /*  in case of baseline ps 10/20 band stereo mode :   */
    /*                                                    */
    /*             qmfBand[0] : 8 ( HYBRID_8_CPLX )       */
    /*             qmfBand[1] : 2 ( HYBRID_2_REAL )       */
    /*             qmfBand[2] : 2 ( HYBRID_2_REAL )       */
    /*                                                    */
    /*  (split the 3 lower qmf band to 12 hybrid bands)   */

    hybridRes = (HYBRID_RES)hHybrid->pResolution[band];

    FDKmemcpy(pWorkRealSlot, hHybrid->mQmfBufferRealSlot[band], hHybrid->qmfBufferMove * sizeof(FIXP_DBL));
    FDKmemcpy(pWorkImagSlot, hHybrid->mQmfBufferImagSlot[band], hHybrid->qmfBufferMove * sizeof(FIXP_DBL));

    pWorkRealSlot[hHybrid->qmfBufferMove] = fixpQmfReal[band];
    pWorkImagSlot[hHybrid->qmfBufferMove] = fixpQmfImag[band];

    FDKmemcpy(hHybrid->mQmfBufferRealSlot[band], pWorkRealSlot + 1, hHybrid->qmfBufferMove * sizeof(FIXP_DBL));
    FDKmemcpy(hHybrid->mQmfBufferImagSlot[band], pWorkImagSlot + 1, hHybrid->qmfBufferMove * sizeof(FIXP_DBL));

    if (fixpQmfReal) {

      /* actual filtering only if output signal requested */
      switch( hybridRes ) {

      /* HYBRID_2_REAL & HYBRID_8_CPLX are only needful for baseline ps */
      case HYBRID_2_REAL:

        slotBasedDualChannelFiltering( pWorkRealSlot,
                                       pWorkImagSlot,
                                       pTempRealSlot,
                                       pTempImagSlot);
        break;

      case HYBRID_8_CPLX:

        slotBasedEightChannelFiltering( pWorkRealSlot,
                                        pWorkImagSlot,
                                        pTempRealSlot,
                                        pTempImagSlot);
        break;

      default:
        FDK_ASSERT(0);
      }

      for(k = 0; k < (SCHAR)hybridRes; k++) {
        fixpHybridReal [chOffset + k] = pTempRealSlot[k];
        fixpHybridImag [chOffset + k] = pTempImagSlot[k];
      }
      chOffset += hybridRes;
    } /* if (mHybridReal) */
  }

  /* group hybrid channels 3+4 -> 3 and 2+5 -> 2 */
  fixpHybridReal[3] += fixpHybridReal[4];
  fixpHybridImag[3] += fixpHybridImag[4];
  fixpHybridReal[4] = (FIXP_DBL)0;
  fixpHybridImag[4] = (FIXP_DBL)0;

  fixpHybridReal[2] += fixpHybridReal[5];
  fixpHybridImag[2] += fixpHybridImag[5];
  fixpHybridReal[5] = (FIXP_DBL)0;
  fixpHybridImag[5] = (FIXP_DBL)0;

  /* free memory on scratch */
  C_ALLOC_SCRATCH_END(pTempRealSlot, FIXP_DBL, 4*HYBRID_FILTER_LENGTH);

}


/*******************************************************************************
 Functionname:  slotBasedHybridSynthesis
 *******************************************************************************

 Description:  The coefficients offering higher resolution for the lower QMF
               channel are simply added prior to the synthesis with the 54
               subbands QMF.

 Arguments:

 Return:        none

*******************************************************************************/

/*! <pre>
      l,r0(n) ---\
      l,r1(n) ---- + --\
      l,r2(n) ---/      \
                         + --> F0(w)
      l,r3(n) ---\      /
      l,r4(n) ---- + --/
      l,r5(n) ---/


      l,r6(n) ---\
                  + ---------> F1(w)
      l,r7(n) ---/


      l,r8(n) ---\
                  + ---------> F2(w)
      l,r9(n) ---/

    </pre>
      Hybrid QMF synthesis filterbank for the 10 and 20 stereo-bands configurations. The
      coefficients offering higher resolution for the lower QMF channel are simply added
      prior to the synthesis with the 54 subbands QMF.

      [see ISO/IEC 14496-3:2001/FDAM 2:2004(E) - Page 52]
*/


void
slotBasedHybridSynthesis ( FIXP_DBL  *fixpHybridReal,  /*!< Hybrid real Values */
                           FIXP_DBL  *fixpHybridImag,  /*!< Hybrid imag Values */
                           FIXP_DBL  *fixpQmfReal,     /*!< Qmf real Values */
                           FIXP_DBL  *fixpQmfImag,     /*!< Qmf imag Values */
                           HANDLE_HYBRID hHybrid )     /*!< Handle to HYBRID struct. */
{
  int  k, band;

  HYBRID_RES hybridRes;
  int  chOffset = 0;

  for(band = 0; band < hHybrid->nQmfBands; band++) {

    FIXP_DBL qmfReal = FL2FXCONST_DBL(0.f);
    FIXP_DBL qmfImag = FL2FXCONST_DBL(0.f);
    hybridRes = (HYBRID_RES)hHybrid->pResolution[band];

    for(k = 0; k < (SCHAR)hybridRes; k++) {
      qmfReal += fixpHybridReal[chOffset + k];
      qmfImag += fixpHybridImag[chOffset + k];
    }

    fixpQmfReal[band] = qmfReal;
    fixpQmfImag[band] = qmfImag;

    chOffset += hybridRes;
  }
}



