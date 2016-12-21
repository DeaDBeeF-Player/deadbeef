
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

/********************************  Fraunhofer IIS  ***************************

   Author(s):   Markus Lohwasser, Josef Hoepfl, Manuel Jander
   Description: QMF filterbank

******************************************************************************/

/*!
  \file
  \brief  Complex qmf analysis/synthesis,  
  This module contains the qmf filterbank for analysis [ cplxAnalysisQmfFiltering() ] and
  synthesis [ cplxSynthesisQmfFiltering() ]. It is a polyphase implementation of a complex
  exponential modulated filter bank. The analysis part usually runs at half the sample rate
  than the synthesis part. (So called "dual-rate" mode.)

  The coefficients of the prototype filter are specified in #sbr_qmf_64_640 (in sbr_rom.cpp).
  Thus only a 64 channel version (32 on the analysis side) with a 640 tap prototype filter
  are used.

  \anchor PolyphaseFiltering <h2>About polyphase filtering</h2>
  The polyphase implementation of a filterbank requires filtering at the input and output.
  This is implemented as part of cplxAnalysisQmfFiltering() and cplxSynthesisQmfFiltering().
  The implementation requires the filter coefficients in a specific structure as described in
  #sbr_qmf_64_640_qmf (in sbr_rom.cpp).

  This module comprises the computationally most expensive functions of the SBR decoder. The accuracy of
  computations is also important and has a direct impact on the overall sound quality. Therefore a special
  test program is available which can be used to only test the filterbank: main_audio.cpp

  This modules also uses scaling of data to provide better SNR on fixed-point processors. See #QMF_SCALE_FACTOR (in sbr_scale.h) for details.
  An interesting note: The function getScalefactor() can constitute a significant amount of computational complexity - very much depending on the
  bitrate. Since it is a rather small function, effective assembler optimization might be possible.

*/

#include "qmf.h"


#include "fixpoint_math.h"
#include "dct.h"

#ifdef QMFSYN_STATES_16BIT
#define QSSCALE (7)
#define FX_DBL2FX_QSS(x) ((FIXP_QSS) ((x)>>(DFRACT_BITS-QSS_BITS-QSSCALE) ))
#define FX_QSS2FX_DBL(x) ((FIXP_DBL)((LONG)x)<<(DFRACT_BITS-QSS_BITS-QSSCALE))
#else
#define QSSCALE (0)
#define FX_DBL2FX_QSS(x) (x)
#define FX_QSS2FX_DBL(x) (x)
#endif


#if defined(__arm__)
#include "arm/qmf_arm.cpp"

#endif

/*!
 * \brief Algorithmic scaling in sbrForwardModulation()
 *
 * The scaling in sbrForwardModulation() is caused by:
 *
 *   \li 1 R_SHIFT in sbrForwardModulation()
 *   \li 5/6 R_SHIFT in dct3() if using 32/64 Bands
 *   \li 1 ommited gain of 2.0 in qmfForwardModulation()
 */
#define ALGORITHMIC_SCALING_IN_ANALYSIS_FILTERBANK 7

/*!
 * \brief Algorithmic scaling in cplxSynthesisQmfFiltering()
 *
 * The scaling in cplxSynthesisQmfFiltering() is caused by:
 *
 *   \li  5/6 R_SHIFT in dct2() if using 32/64 Bands
 *   \li  1 ommited gain of 2.0 in qmfInverseModulation()
 *   \li -6 division by 64 in synthesis filterbank
 *   \li x bits external influence
 */
#define ALGORITHMIC_SCALING_IN_SYNTHESIS_FILTERBANK 1


/*!
  \brief Perform Synthesis Prototype Filtering on a single slot of input data.

  The filter takes 2 * qmf->no_channels of input data and
  generates qmf->no_channels time domain output samples.
*/
static
#ifndef FUNCTION_qmfSynPrototypeFirSlot
void qmfSynPrototypeFirSlot(
#else
void qmfSynPrototypeFirSlot_fallback(
#endif
                             HANDLE_QMF_FILTER_BANK qmf,
                             FIXP_QMF *RESTRICT realSlot,            /*!< Input: Pointer to real Slot */
                             FIXP_QMF *RESTRICT imagSlot,            /*!< Input: Pointer to imag Slot */
                             INT_PCM  *RESTRICT timeOut,             /*!< Time domain data */
                             int       stride
                            )
{
  FIXP_QSS* FilterStates = (FIXP_QSS*)qmf->FilterStates;
  int       no_channels = qmf->no_channels;
  const FIXP_PFT *p_Filter = qmf->p_filter;
  int p_stride = qmf->p_stride;
  int j;
  FIXP_QSS *RESTRICT sta = FilterStates;
  const FIXP_PFT *RESTRICT p_flt, *RESTRICT p_fltm;
  int scale = ((DFRACT_BITS-SAMPLE_BITS)-1-qmf->outScalefactor);

  p_flt  = p_Filter+p_stride*QMF_NO_POLY;          /*                     5-ter von 330 */
  p_fltm = p_Filter+(qmf->FilterSize/2)-p_stride*QMF_NO_POLY;  /* 5 + (320 - 2*5) = 315-ter von 330 */

  FDK_ASSERT(SAMPLE_BITS-1-qmf->outScalefactor >= 0); //   (DFRACT_BITS-SAMPLE_BITS)-1-qmf->outScalefactor >= 0);

  for (j = no_channels-1; j >= 0; j--) {  /* ---- läuft ueber alle Linien eines Slots ---- */
    FIXP_QMF imag  =  imagSlot[j];  // no_channels-1 .. 0
    FIXP_QMF real  =  realSlot[j];  // ~~"~~
    {
      INT_PCM tmp;
      FIXP_DBL Are = FX_QSS2FX_DBL(sta[0]) + fMultDiv2( p_fltm[0] , real);

      if (qmf->outGain!=(FIXP_DBL)0x80000000) {
        Are = fMult(Are,qmf->outGain);
      }

  #if SAMPLE_BITS > 16
      tmp = (INT_PCM)(SATURATE_SHIFT(fAbs(Are), scale, SAMPLE_BITS));
  #else
      tmp = (INT_PCM)(SATURATE_RIGHT_SHIFT(fAbs(Are), scale, SAMPLE_BITS));
  #endif
      if (Are < (FIXP_QMF)0) {
        tmp = -tmp;
      }
      timeOut[ (j)*stride ] = tmp;
    }

    sta[0] = sta[1] + FX_DBL2FX_QSS(fMultDiv2( p_flt [4] , imag ));
    sta[1] = sta[2] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[1] , real ));
    sta[2] = sta[3] + FX_DBL2FX_QSS(fMultDiv2( p_flt [3] , imag ));
    sta[3] = sta[4] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[2] , real ));
    sta[4] = sta[5] + FX_DBL2FX_QSS(fMultDiv2( p_flt [2] , imag ));
    sta[5] = sta[6] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[3] , real ));
    sta[6] = sta[7] + FX_DBL2FX_QSS(fMultDiv2( p_flt [1] , imag ));
    sta[7] = sta[8] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[4] , real ));
    sta[8] =          FX_DBL2FX_QSS(fMultDiv2( p_flt [0] , imag ));

    p_flt  += (p_stride*QMF_NO_POLY);
    p_fltm -= (p_stride*QMF_NO_POLY);
    sta    += 9; // = (2*QMF_NO_POLY-1);
  }
}

#ifndef FUNCTION_qmfSynPrototypeFirSlot_NonSymmetric
/*!
  \brief Perform Synthesis Prototype Filtering on a single slot of input data.

  The filter takes 2 * qmf->no_channels of input data and
  generates qmf->no_channels time domain output samples.
*/
static
void qmfSynPrototypeFirSlot_NonSymmetric(
                             HANDLE_QMF_FILTER_BANK qmf,
                             FIXP_QMF *RESTRICT realSlot,            /*!< Input: Pointer to real Slot */
                             FIXP_QMF *RESTRICT imagSlot,            /*!< Input: Pointer to imag Slot */
                             INT_PCM  *RESTRICT timeOut,             /*!< Time domain data */
                             int       stride
                            )
{
  FIXP_QSS* FilterStates = (FIXP_QSS*)qmf->FilterStates;
  int       no_channels = qmf->no_channels;
  const FIXP_PFT *p_Filter = qmf->p_filter;
  int p_stride = qmf->p_stride;
  int j;
  FIXP_QSS *RESTRICT sta = FilterStates;
  const FIXP_PFT *RESTRICT p_flt, *RESTRICT p_fltm;
  int scale = ((DFRACT_BITS-SAMPLE_BITS)-1-qmf->outScalefactor);

  p_flt  = p_Filter;                           /*!< Pointer to first half of filter coefficients */
  p_fltm = &p_flt[qmf->FilterSize/2];  /* at index 320, overall 640 coefficients */

  FDK_ASSERT(SAMPLE_BITS-1-qmf->outScalefactor >= 0); //   (DFRACT_BITS-SAMPLE_BITS)-1-qmf->outScalefactor >= 0);

  for (j = no_channels-1; j >= 0; j--) {  /* ---- läuft ueber alle Linien eines Slots ---- */

    FIXP_QMF imag  =  imagSlot[j];  // no_channels-1 .. 0
    FIXP_QMF real  =  realSlot[j];  // ~~"~~
    {
      INT_PCM tmp;
      FIXP_QMF Are = sta[0] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[4] , real ));

  #if SAMPLE_BITS > 16
      tmp = (INT_PCM)(SATURATE_SHIFT(fAbs(Are), scale, SAMPLE_BITS));
  #else
      tmp = (INT_PCM)(SATURATE_RIGHT_SHIFT(fAbs(Are), scale, SAMPLE_BITS));
  #endif
      if (Are < (FIXP_QMF)0) {
        tmp = -tmp;
      }
      timeOut[j*stride] = tmp;
    }

    sta[0] = sta[1] + FX_DBL2FX_QSS(fMultDiv2( p_flt [4] , imag ));
    sta[1] = sta[2] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[3] , real ));
    sta[2] = sta[3] + FX_DBL2FX_QSS(fMultDiv2( p_flt [3] , imag ));

    sta[3] = sta[4] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[2] , real ));
    sta[4] = sta[5] + FX_DBL2FX_QSS(fMultDiv2( p_flt [2] , imag ));
    sta[5] = sta[6] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[1] , real ));
    sta[6] = sta[7] + FX_DBL2FX_QSS(fMultDiv2( p_flt [1] , imag ));

    sta[7] = sta[8] + FX_DBL2FX_QSS(fMultDiv2( p_fltm[0] , real ));
    sta[8] =          FX_DBL2FX_QSS(fMultDiv2( p_flt [0] , imag ));

    p_flt  += (p_stride*QMF_NO_POLY);
    p_fltm += (p_stride*QMF_NO_POLY);
    sta    += 9; // = (2*QMF_NO_POLY-1);
  }

}
#endif /* FUNCTION_qmfSynPrototypeFirSlot_NonSymmetric */

#ifndef FUNCTION_qmfAnaPrototypeFirSlot
/*!
  \brief Perform Analysis Prototype Filtering on a single slot of input data.
*/
static
void qmfAnaPrototypeFirSlot( FIXP_QMF *analysisBuffer,
                             int       no_channels,             /*!< Number channels of analysis filter */
                             const FIXP_PFT *p_filter,
                             int       p_stride,                /*!< Stide of analysis filter    */
                             FIXP_QAS *RESTRICT pFilterStates
                            )
{
    int k;

    FIXP_DBL accu;
    const FIXP_PFT *RESTRICT p_flt = p_filter;
    FIXP_QMF *RESTRICT pData_0 = analysisBuffer + 2*no_channels - 1;
    FIXP_QMF *RESTRICT pData_1 = analysisBuffer;

    FIXP_QAS *RESTRICT sta_0 = (FIXP_QAS *)pFilterStates;
    FIXP_QAS *RESTRICT sta_1 = (FIXP_QAS *)pFilterStates + (2*QMF_NO_POLY*no_channels) - 1;
    int pfltStep = QMF_NO_POLY * (p_stride);
    int staStep1 = no_channels<<1;
    int staStep2 = (no_channels<<3) - 1; /* Rewind one less */

    /* FIR filter 0 */
    accu =   fMultDiv2( p_flt[0], *sta_1);  sta_1 -= staStep1;
    accu +=  fMultDiv2( p_flt[1], *sta_1);  sta_1 -= staStep1;
    accu +=  fMultDiv2( p_flt[2], *sta_1);  sta_1 -= staStep1;
    accu +=  fMultDiv2( p_flt[3], *sta_1);  sta_1 -= staStep1;
    accu +=  fMultDiv2( p_flt[4], *sta_1);
    *pData_1++ = FX_DBL2FX_QMF(accu<<1);
    sta_1 += staStep2;

    p_flt += pfltStep;

    /* FIR filters 1..63 127..65 */
    for (k=0; k<no_channels-1; k++)
    {
      accu =  fMultDiv2( p_flt[0], *sta_0);  sta_0 += staStep1;
      accu += fMultDiv2( p_flt[1], *sta_0);  sta_0 += staStep1;
      accu += fMultDiv2( p_flt[2], *sta_0);  sta_0 += staStep1;
      accu += fMultDiv2( p_flt[3], *sta_0);  sta_0 += staStep1;
      accu += fMultDiv2( p_flt[4], *sta_0);
      *pData_0-- = FX_DBL2FX_QMF(accu<<1);
      sta_0 -= staStep2;

      accu =   fMultDiv2( p_flt[0], *sta_1);  sta_1 -= staStep1;
      accu +=  fMultDiv2( p_flt[1], *sta_1);  sta_1 -= staStep1;
      accu +=  fMultDiv2( p_flt[2], *sta_1);  sta_1 -= staStep1;
      accu +=  fMultDiv2( p_flt[3], *sta_1);  sta_1 -= staStep1;
      accu +=  fMultDiv2( p_flt[4], *sta_1);
      *pData_1++ = FX_DBL2FX_QMF(accu<<1);
      sta_1 += staStep2;

      p_flt += pfltStep;
    }

    /* FIR filter 64 */
    accu =  fMultDiv2( p_flt[0], *sta_0);  sta_0 += staStep1;
    accu += fMultDiv2( p_flt[1], *sta_0);  sta_0 += staStep1;
    accu += fMultDiv2( p_flt[2], *sta_0);  sta_0 += staStep1;
    accu += fMultDiv2( p_flt[3], *sta_0);  sta_0 += staStep1;
    accu += fMultDiv2( p_flt[4], *sta_0);
    *pData_0-- = FX_DBL2FX_QMF(accu<<1);
    sta_0 -= staStep2;
}
#endif /* !defined(FUNCTION_qmfAnaPrototypeFirSlot) */


#ifndef FUNCTION_qmfAnaPrototypeFirSlot_NonSymmetric
/*!
  \brief Perform Analysis Prototype Filtering on a single slot of input data.
*/
static
void qmfAnaPrototypeFirSlot_NonSymmetric(
                                        FIXP_QMF *analysisBuffer,
                                        int       no_channels,             /*!< Number channels of analysis filter */
                                        const FIXP_PFT *p_filter,
                                        int       p_stride,                /*!< Stide of analysis filter    */
                                        FIXP_QAS *RESTRICT pFilterStates
                                       )
{
  const FIXP_PFT *RESTRICT p_flt = p_filter;
  int  p, k;

  for (k = 0; k < 2*no_channels; k++)
  {
    FIXP_DBL accu = (FIXP_DBL)0;

    p_flt += QMF_NO_POLY * (p_stride - 1);

    /*
      Perform FIR-Filter
    */
    for (p = 0; p < QMF_NO_POLY; p++) {
      accu +=  fMultDiv2(*p_flt++, pFilterStates[2*no_channels * p]);
    }
    analysisBuffer[2*no_channels - 1 - k] = FX_DBL2FX_QMF(accu<<1);
    pFilterStates++;
  }
}
#endif /* FUNCTION_qmfAnaPrototypeFirSlot_NonSymmetric */

/*!
 *
 * \brief Perform real-valued forward modulation of the time domain
 *        data of timeIn and stores the real part of the subband
 *        samples in rSubband
 *
 */
static void
qmfForwardModulationLP_even( HANDLE_QMF_FILTER_BANK anaQmf, /*!< Handle of Qmf Analysis Bank  */
                             FIXP_QMF *timeIn,              /*!< Time Signal */
                             FIXP_QMF *rSubband )           /*!< Real Output */
{
  int i;
  int L = anaQmf->no_channels;
  int M = L>>1;
  int scale;
  FIXP_QMF accu;

  const FIXP_QMF *timeInTmp1 = (FIXP_QMF *) &timeIn[3 * M];
  const FIXP_QMF *timeInTmp2 = timeInTmp1;
  FIXP_QMF *rSubbandTmp = rSubband;

  rSubband[0] = timeIn[3 * M] >> 1;

  for (i = M-1; i != 0; i--) {
    accu = ((*--timeInTmp1) >> 1) + ((*++timeInTmp2) >> 1);
    *++rSubbandTmp = accu;
  }

  timeInTmp1 = &timeIn[2 * M];
  timeInTmp2 = &timeIn[0];
  rSubbandTmp = &rSubband[M];

  for (i = L-M; i != 0; i--) {
    accu = ((*timeInTmp1--) >> 1) - ((*timeInTmp2++) >> 1);
    *rSubbandTmp++ = accu;
  }

  dct_III(rSubband, timeIn, L, &scale);
}

#if !defined(FUNCTION_qmfForwardModulationLP_odd)
static void
qmfForwardModulationLP_odd( HANDLE_QMF_FILTER_BANK anaQmf, /*!< Handle of Qmf Analysis Bank  */
                            const FIXP_QMF *timeIn,        /*!< Time Signal */
                            FIXP_QMF *rSubband )           /*!< Real Output */
{
  int i;
  int L = anaQmf->no_channels;
  int M = L>>1;
  int shift = (anaQmf->no_channels>>6) + 1;

  for (i = 0; i < M; i++) {
    rSubband[M + i]     = (timeIn[L - 1 - i]>>1) - (timeIn[i]>>shift);
    rSubband[M - 1 - i] = (timeIn[L + i]>>1)     + (timeIn[2 * L - 1 - i]>>shift);
  }

  dct_IV(rSubband, L, &shift);
}
#endif /* !defined(FUNCTION_qmfForwardModulationLP_odd) */



/*!
 *
 * \brief Perform complex-valued forward modulation of the time domain
 *        data of timeIn and stores the real part of the subband
 *        samples in rSubband, and the imaginary part in iSubband
 *
 *        Only the lower bands are obtained (upto anaQmf->lsb). For
 *        a full bandwidth analysis it is required to set both anaQmf->lsb
 *        and anaQmf->usb to the amount of QMF bands.
 *
 */
static void
qmfForwardModulationHQ( HANDLE_QMF_FILTER_BANK anaQmf,     /*!< Handle of Qmf Analysis Bank  */
                        const FIXP_QMF *RESTRICT timeIn,   /*!< Time Signal */
                        FIXP_QMF *RESTRICT rSubband,       /*!< Real Output */
                        FIXP_QMF *RESTRICT iSubband        /*!< Imaginary Output */
                       )
{
  int i;
  int L = anaQmf->no_channels;
  int L2 = L<<1;
  int shift = 0;

  for (i = 0; i < L; i+=2) {
    FIXP_QMF x0, x1, y0, y1;

    x0 = timeIn[i] >> 1;
    x1 = timeIn[i+1] >> 1;
    y0 = timeIn[L2 - 1 - i] >> 1;
    y1 = timeIn[L2 - 2 - i] >> 1;

    rSubband[i] = x0 - y0;
    rSubband[i+1] = x1 - y1;
    iSubband[i] = x0 + y0;
    iSubband[i+1] = x1 + y1;
  }

  dct_IV(rSubband, L, &shift);
  dst_IV(iSubband, L, &shift);

  {
    {
      const FIXP_QTW *RESTRICT sbr_t_cos;
      const FIXP_QTW *RESTRICT sbr_t_sin;
      sbr_t_cos = anaQmf->t_cos;
      sbr_t_sin = anaQmf->t_sin;

      for (i = 0; i < anaQmf->lsb; i++) {
        cplxMult(&iSubband[i], &rSubband[i], iSubband[i], rSubband[i], sbr_t_cos[i], sbr_t_sin[i]);
      }
    }
  }
}

/*
 * \brief Perform one QMF slot analysis of the time domain data of timeIn
 *        with specified stride and stores the real part of the subband
 *        samples in rSubband, and the imaginary part in iSubband
 *
 *        Only the lower bands are obtained (upto anaQmf->lsb). For
 *        a full bandwidth analysis it is required to set both anaQmf->lsb
 *        and anaQmf->usb to the amount of QMF bands.
 */
void
qmfAnalysisFilteringSlot( HANDLE_QMF_FILTER_BANK anaQmf,  /*!< Handle of Qmf Synthesis Bank  */
                          FIXP_QMF      *qmfReal,         /*!< Low and High band, real */
                          FIXP_QMF      *qmfImag,         /*!< Low and High band, imag */
                          const INT_PCM *RESTRICT timeIn, /*!< Pointer to input */
                          const int      stride,          /*!< stride factor of input */
                          FIXP_QMF      *pWorkBuffer      /*!< pointer to temporal working buffer */
                         )
{
    int i;
    int offset = anaQmf->no_channels*(QMF_NO_POLY*2-1);
    /*
      Feed time signal into oldest anaQmf->no_channels states
    */
    {
      FIXP_QAS *RESTRICT FilterStatesAnaTmp = ((FIXP_QAS*)anaQmf->FilterStates)+offset;

      /* Feed and scale actual time in slot */
      for(i=anaQmf->no_channels>>1; i!=0; i--) {
        /* Place INT_PCM value left aligned in scaledTimeIn */
#if (QAS_BITS==SAMPLE_BITS)
        *FilterStatesAnaTmp++ = (FIXP_QAS)*timeIn; timeIn += stride;
        *FilterStatesAnaTmp++ = (FIXP_QAS)*timeIn; timeIn += stride;
#elif (QAS_BITS>SAMPLE_BITS)
        *FilterStatesAnaTmp++ = (FIXP_QAS)((*timeIn)<<(QAS_BITS-SAMPLE_BITS)); timeIn += stride;
        *FilterStatesAnaTmp++ = (FIXP_QAS)((*timeIn)<<(QAS_BITS-SAMPLE_BITS)); timeIn += stride;
#else
        *FilterStatesAnaTmp++ = (FIXP_QAS)((*timeIn)>>(SAMPLE_BITS-QAS_BITS)); timeIn += stride;
        *FilterStatesAnaTmp++ = (FIXP_QAS)((*timeIn)>>(SAMPLE_BITS-QAS_BITS)); timeIn += stride;
#endif
      }
    }

    if (anaQmf->flags & QMF_FLAG_NONSYMMETRIC) {
      qmfAnaPrototypeFirSlot_NonSymmetric(
                              pWorkBuffer,
                              anaQmf->no_channels,
                              anaQmf->p_filter,
                              anaQmf->p_stride,
                              (FIXP_QAS*)anaQmf->FilterStates
                            );
    } else {
      qmfAnaPrototypeFirSlot( pWorkBuffer,
                              anaQmf->no_channels,
                              anaQmf->p_filter,
                              anaQmf->p_stride,
                              (FIXP_QAS*)anaQmf->FilterStates
                            );
    }

    if (anaQmf->flags & QMF_FLAG_LP) {
      if (anaQmf->flags & QMF_FLAG_CLDFB)
        qmfForwardModulationLP_odd( anaQmf,
                                    pWorkBuffer,
                                    qmfReal );
      else
        qmfForwardModulationLP_even( anaQmf,
                                     pWorkBuffer,
                                     qmfReal );

    } else {
      qmfForwardModulationHQ( anaQmf,
                              pWorkBuffer,
                              qmfReal,
                              qmfImag
                             );
    }
    /*
      Shift filter states

      Should be realized with modulo adressing on a DSP instead of a true buffer shift
    */
    FDKmemmove ((FIXP_QAS*)anaQmf->FilterStates, (FIXP_QAS*)anaQmf->FilterStates+anaQmf->no_channels, offset*sizeof(FIXP_QAS));
}


/*!
 *
 * \brief Perform complex-valued subband filtering of the time domain
 *        data of timeIn and stores the real part of the subband
 *        samples in rAnalysis, and the imaginary part in iAnalysis
 * The qmf coefficient table is symmetric. The symmetry is expoited by
 * shrinking the coefficient table to half the size. The addressing mode
 * takes care of the symmetries.
 *
 * Only the lower bands are obtained (upto anaQmf->lsb). For
 * a full bandwidth analysis it is required to set both anaQmf->lsb
 * and anaQmf->usb to the amount of QMF bands.
 *
 * \sa PolyphaseFiltering
 */

void
qmfAnalysisFiltering( HANDLE_QMF_FILTER_BANK anaQmf,    /*!< Handle of Qmf Analysis Bank */
                      FIXP_QMF **qmfReal,               /*!< Pointer to real subband slots */
                      FIXP_QMF **qmfImag,               /*!< Pointer to imag subband slots */
                      QMF_SCALE_FACTOR *scaleFactor,
                      const INT_PCM *timeIn,            /*!< Time signal */
                      const int  stride,
                      FIXP_QMF  *pWorkBuffer            /*!< pointer to temporal working buffer */
                      )
{
  int i;
  int no_channels = anaQmf->no_channels;

  scaleFactor->lb_scale = -ALGORITHMIC_SCALING_IN_ANALYSIS_FILTERBANK;
  scaleFactor->lb_scale -= anaQmf->filterScale;

  for (i = 0; i < anaQmf->no_col; i++)
  {
      FIXP_QMF *qmfImagSlot = NULL;

      if (!(anaQmf->flags & QMF_FLAG_LP)) {
        qmfImagSlot = qmfImag[i];
      }

      qmfAnalysisFilteringSlot( anaQmf, qmfReal[i], qmfImagSlot, timeIn , stride, pWorkBuffer );

      timeIn += no_channels*stride;

  } /* no_col loop  i  */
}

/*!
 *
 * \brief Perform low power inverse modulation of the subband
 *        samples stored in rSubband (real part) and iSubband (imaginary
 *        part) and stores the result in pWorkBuffer.
 *
 */
inline
static void
qmfInverseModulationLP_even( HANDLE_QMF_FILTER_BANK synQmf,   /*!< Handle of Qmf Synthesis Bank  */
                             const FIXP_QMF *qmfReal,         /*!< Pointer to qmf real subband slot (input) */
                             const int   scaleFactorLowBand,  /*!< Scalefactor for Low band */
                             const int   scaleFactorHighBand, /*!< Scalefactor for High band */
                             FIXP_QMF *pTimeOut               /*!< Pointer to qmf subband slot (output)*/
                           )
{
  int i;
  int L = synQmf->no_channels;
  int M = L>>1;
  int scale;
  FIXP_QMF tmp;
  FIXP_QMF *RESTRICT tReal = pTimeOut;
  FIXP_QMF *RESTRICT tImag = pTimeOut + L;

  /* Move input to output vector with offset */
  scaleValues(&tReal[0],             &qmfReal[0],             synQmf->lsb,             scaleFactorLowBand);
  scaleValues(&tReal[0+synQmf->lsb], &qmfReal[0+synQmf->lsb], synQmf->usb-synQmf->lsb, scaleFactorHighBand);
  FDKmemclear(&tReal[0+synQmf->usb], (L-synQmf->usb)*sizeof(FIXP_QMF));

  /* Dct type-2 transform */
  dct_II(tReal, tImag, L, &scale);

  /* Expand output and replace inplace the output buffers */
  tImag[0] = tReal[M];
  tImag[M] = (FIXP_QMF)0;
  tmp = tReal [0];
  tReal [0] = tReal[M];
  tReal [M] = tmp;

  for (i = 1; i < M/2; i++) {
    /* Imag */
    tmp = tReal[L - i];
    tImag[M - i] =  tmp;
    tImag[i + M] = -tmp;

    tmp = tReal[M + i];
    tImag[i] =  tmp;
    tImag[L - i] = -tmp;

    /* Real */
    tReal [M + i] = tReal[i];
    tReal [L - i] = tReal[M - i];
    tmp = tReal[i];
    tReal[i] = tReal [M - i];
    tReal [M - i] = tmp;

  }
  /* Remaining odd terms */
  tmp = tReal[M + M/2];
  tImag[M/2]     =  tmp;
  tImag[M/2 + M] = -tmp;

  tReal [M + M/2] = tReal[M/2];
}

inline
static void
qmfInverseModulationLP_odd( HANDLE_QMF_FILTER_BANK synQmf,   /*!< Handle of Qmf Synthesis Bank  */
                            const FIXP_QMF *qmfReal,         /*!< Pointer to qmf real subband slot (input) */
                            const int scaleFactorLowBand,    /*!< Scalefactor for Low band */
                            const int scaleFactorHighBand,   /*!< Scalefactor for High band */
                            FIXP_QMF *pTimeOut               /*!< Pointer to qmf subband slot (output)*/
                          )
{
  int i;
  int L = synQmf->no_channels;
  int M = L>>1;
  int shift = 0;

  /* Move input to output vector with offset */
  scaleValues(pTimeOut+M,              qmfReal,             synQmf->lsb,             scaleFactorLowBand);
  scaleValues(pTimeOut+M+synQmf->lsb,  qmfReal+synQmf->lsb, synQmf->usb-synQmf->lsb, scaleFactorHighBand);
  FDKmemclear(pTimeOut+M+synQmf->usb, (L-synQmf->usb)*sizeof(FIXP_QMF));

  dct_IV(pTimeOut+M, L, &shift);
  for (i = 0; i < M; i++) {
    pTimeOut[i]             =  pTimeOut[L - 1 - i];
    pTimeOut[2 * L - 1 - i] = -pTimeOut[L + i];
  }
}


/*!
 *
 * \brief Perform complex-valued inverse modulation of the subband
 *        samples stored in rSubband (real part) and iSubband (imaginary
 *        part) and stores the result in pWorkBuffer.
 *
 */
inline
static void
qmfInverseModulationHQ( HANDLE_QMF_FILTER_BANK synQmf,  /*!< Handle of Qmf Synthesis Bank     */
                        const FIXP_QMF *qmfReal,        /*!< Pointer to qmf real subband slot */
                        const FIXP_QMF *qmfImag,        /*!< Pointer to qmf imag subband slot */
                        const int   scaleFactorLowBand, /*!< Scalefactor for Low band         */
                        const int   scaleFactorHighBand,/*!< Scalefactor for High band        */
                        FIXP_QMF  *pWorkBuffer          /*!< WorkBuffer (output)              */
                      )
{
  int i;
  int L = synQmf->no_channels;
  int M = L>>1;
  int shift = 0;
  FIXP_QMF *RESTRICT tReal = pWorkBuffer;
  FIXP_QMF *RESTRICT tImag = pWorkBuffer+L;

  if (synQmf->flags & QMF_FLAG_CLDFB){
    for (i = 0; i < synQmf->lsb; i++) {
      cplxMult(&tImag[i], &tReal[i],
                scaleValue(qmfImag[i],scaleFactorLowBand), scaleValue(qmfReal[i],scaleFactorLowBand),
                synQmf->t_cos[i], synQmf->t_sin[i]);
    }
    for (; i < synQmf->usb; i++) {
      cplxMult(&tImag[i], &tReal[i],
                scaleValue(qmfImag[i],scaleFactorHighBand), scaleValue(qmfReal[i],scaleFactorHighBand),
                synQmf->t_cos[i], synQmf->t_sin[i]);
    }
  }

  if ( (synQmf->flags & QMF_FLAG_CLDFB) == 0) {
    scaleValues(&tReal[0],             &qmfReal[0],             synQmf->lsb,             scaleFactorLowBand);
    scaleValues(&tReal[0+synQmf->lsb], &qmfReal[0+synQmf->lsb], synQmf->usb-synQmf->lsb, scaleFactorHighBand);
    scaleValues(&tImag[0],             &qmfImag[0],             synQmf->lsb,             scaleFactorLowBand);
    scaleValues(&tImag[0+synQmf->lsb], &qmfImag[0+synQmf->lsb], synQmf->usb-synQmf->lsb, scaleFactorHighBand);
  }

  FDKmemclear(&tReal[synQmf->usb], (synQmf->no_channels-synQmf->usb)*sizeof(FIXP_QMF));
  FDKmemclear(&tImag[synQmf->usb], (synQmf->no_channels-synQmf->usb)*sizeof(FIXP_QMF));

  dct_IV(tReal, L, &shift);
  dst_IV(tImag, L, &shift);

  if (synQmf->flags & QMF_FLAG_CLDFB){
    for (i = 0; i < M; i++) {
      FIXP_QMF r1, i1, r2, i2;
      r1 = tReal[i];
      i2 = tImag[L - 1 - i];
      r2 = tReal[L - i - 1];
      i1 = tImag[i];

      tReal[i] = (r1 - i1)>>1;
      tImag[L - 1 - i] = -(r1 + i1)>>1;
      tReal[L - i - 1] =  (r2 - i2)>>1;
      tImag[i] = -(r2 + i2)>>1;
    }
  } else
  {
    /* The array accesses are negative to compensate the missing minus sign in the low and hi band gain. */
    /* 26 cycles on ARM926 */
    for (i = 0; i < M; i++) {
      FIXP_QMF r1, i1, r2, i2;
      r1 = -tReal[i];
      i2 = -tImag[L - 1 - i];
      r2 = -tReal[L - i - 1];
      i1 = -tImag[i];

      tReal[i] = (r1 - i1)>>1;
      tImag[L - 1 - i] = -(r1 + i1)>>1;
      tReal[L - i - 1] =  (r2 - i2)>>1;
      tImag[i] = -(r2 + i2)>>1;
    }
  }
}

void qmfSynthesisFilteringSlot( HANDLE_QMF_FILTER_BANK  synQmf,
                                const FIXP_QMF  *realSlot,
                                const FIXP_QMF  *imagSlot,
                                const int        scaleFactorLowBand,
                                const int        scaleFactorHighBand,
                                INT_PCM         *timeOut,
                                const int        stride,
                                FIXP_QMF        *pWorkBuffer)
{
    if (!(synQmf->flags & QMF_FLAG_LP))
      qmfInverseModulationHQ ( synQmf,
                               realSlot,
                               imagSlot,
                               scaleFactorLowBand,
                               scaleFactorHighBand,
                               pWorkBuffer
                             );
    else
    {
      if (synQmf->flags & QMF_FLAG_CLDFB) {
        qmfInverseModulationLP_odd ( synQmf,
                                 realSlot,
                                 scaleFactorLowBand,
                                 scaleFactorHighBand,
                                 pWorkBuffer
                               );
      } else {
        qmfInverseModulationLP_even ( synQmf,
                                 realSlot,
                                 scaleFactorLowBand,
                                 scaleFactorHighBand,
                                 pWorkBuffer
                               );
      }
    }

    if (synQmf->flags & QMF_FLAG_NONSYMMETRIC) {
        qmfSynPrototypeFirSlot_NonSymmetric (
                                 synQmf,
                                 pWorkBuffer,
                                 pWorkBuffer+synQmf->no_channels,
                                 timeOut,
                                 stride
                               );
    } else {
        qmfSynPrototypeFirSlot ( synQmf,
                                 pWorkBuffer,
                                 pWorkBuffer+synQmf->no_channels,
                                 timeOut,
                                 stride
                               );
    }
}


/*!
 *
 *
 * \brief Perform complex-valued subband synthesis of the
 *        low band and the high band and store the
 *        time domain data in timeOut
 *
 * First step: Calculate the proper scaling factor of current
 * spectral data in qmfReal/qmfImag, old spectral data in the overlap
 * range and filter states.
 *
 * Second step: Perform Frequency-to-Time mapping with inverse
 * Modulation slot-wise.
 *
 * Third step: Perform FIR-filter slot-wise. To save space for filter
 * states, the MAC operations are executed directly on the filter states
 * instead of accumulating several products in the accumulator. The
 * buffer shift at the end of the function should be replaced by a
 * modulo operation, which is available on some DSPs.
 *
 * Last step: Copy the upper part of the spectral data to the overlap buffer.
 *
 * The qmf coefficient table is symmetric. The symmetry is exploited by
 * shrinking the coefficient table to half the size. The addressing mode
 * takes care of the symmetries.  If the #define #QMFTABLE_FULL is set,
 * coefficient addressing works on the full table size. The code will be
 * slightly faster and slightly more compact.
 *
 * Workbuffer requirement: 2 x sizeof(**QmfBufferReal) * synQmf->no_channels
 */
void
qmfSynthesisFiltering( HANDLE_QMF_FILTER_BANK synQmf,       /*!< Handle of Qmf Synthesis Bank  */
                       FIXP_QMF  **QmfBufferReal,           /*!< Low and High band, real */
                       FIXP_QMF  **QmfBufferImag,           /*!< Low and High band, imag */
                       const QMF_SCALE_FACTOR *scaleFactor,
                       const INT   ov_len,                  /*!< split Slot of overlap and actual slots */
                       INT_PCM    *timeOut,                 /*!< Pointer to output */
                       const INT   stride,                  /*!< stride factor of output */
                       FIXP_QMF   *pWorkBuffer              /*!< pointer to temporal working buffer */
                      )
{
  int i;
  int L = synQmf->no_channels;
  SCHAR scaleFactorHighBand;
  SCHAR scaleFactorLowBand_ov, scaleFactorLowBand_no_ov;

  /* adapt scaling */
  scaleFactorHighBand = -ALGORITHMIC_SCALING_IN_ANALYSIS_FILTERBANK - scaleFactor->hb_scale;
  scaleFactorLowBand_ov = - ALGORITHMIC_SCALING_IN_ANALYSIS_FILTERBANK - scaleFactor->ov_lb_scale;
  scaleFactorLowBand_no_ov = - ALGORITHMIC_SCALING_IN_ANALYSIS_FILTERBANK - scaleFactor->lb_scale;

  for (i = 0; i < synQmf->no_col; i++)  /* ----- no_col loop ----- */
  {
    const FIXP_DBL *QmfBufferImagSlot = NULL;

    SCHAR scaleFactorLowBand = (i<ov_len) ? scaleFactorLowBand_ov : scaleFactorLowBand_no_ov;

    if (!(synQmf->flags & QMF_FLAG_LP))
        QmfBufferImagSlot = QmfBufferImag[i];

    qmfSynthesisFilteringSlot(  synQmf,
                                QmfBufferReal[i],
                                QmfBufferImagSlot,
                                scaleFactorLowBand,
                                scaleFactorHighBand,
                                timeOut+(i*L*stride),
                                stride,
                                pWorkBuffer);
  } /* no_col loop  i  */

}


/*!
 *
 * \brief Create QMF filter bank instance
 *
 * \return 0 if successful
 *
 */
static int
qmfInitFilterBank (HANDLE_QMF_FILTER_BANK h_Qmf,     /*!< Handle to return */
                   void *pFilterStates,              /*!< Handle to filter states */
                   int noCols,                       /*!< Number of timeslots per frame */
                   int lsb,                          /*!< Lower end of QMF frequency range */
                   int usb,                          /*!< Upper end of QMF frequency range */
                   int no_channels,                  /*!< Number of channels (bands) */
                   UINT flags)                       /*!< flags */
{
  FDKmemclear(h_Qmf,sizeof(QMF_FILTER_BANK));

  if (flags & QMF_FLAG_MPSLDFB)
  {
    return -1;
  }

  if ( !(flags & QMF_FLAG_MPSLDFB) && (flags & QMF_FLAG_CLDFB) )
  {
    flags |= QMF_FLAG_NONSYMMETRIC;
    h_Qmf->filterScale = QMF_CLDFB_PFT_SCALE;

    h_Qmf->p_stride = 1;
    switch (no_channels) {
      case 64:
        h_Qmf->t_cos = qmf_phaseshift_cos64_cldfb;
        h_Qmf->t_sin = qmf_phaseshift_sin64_cldfb;
        h_Qmf->p_filter = qmf_cldfb_640;
        h_Qmf->FilterSize = 640;
        break;
      case 32:
        h_Qmf->t_cos = qmf_phaseshift_cos32_cldfb;
        h_Qmf->t_sin = qmf_phaseshift_sin32_cldfb;
        h_Qmf->p_filter = qmf_cldfb_320;
        h_Qmf->FilterSize = 320;
        break;
      default:
        return -1;
    }
  }

  if ( !(flags & QMF_FLAG_MPSLDFB) && ((flags & QMF_FLAG_CLDFB) == 0) )
  {
    switch (no_channels) {
      case 64:
        h_Qmf->p_filter = qmf_64;
        h_Qmf->t_cos = qmf_phaseshift_cos64;
        h_Qmf->t_sin = qmf_phaseshift_sin64;
        h_Qmf->p_stride = 1;
        h_Qmf->FilterSize = 640;
        h_Qmf->filterScale = 0;
        break;
      case 32:
        h_Qmf->p_filter = qmf_64;
        if (flags & QMF_FLAG_DOWNSAMPLED) {
          h_Qmf->t_cos = qmf_phaseshift_cos_downsamp32;
          h_Qmf->t_sin = qmf_phaseshift_sin_downsamp32;
        }
        else {
        h_Qmf->t_cos = qmf_phaseshift_cos32;
        h_Qmf->t_sin = qmf_phaseshift_sin32;
        }
        h_Qmf->p_stride = 2;
        h_Qmf->FilterSize = 640;
        h_Qmf->filterScale = 0;
        break;
      default:
        return -1;
    }
  }

  h_Qmf->flags = flags;

  h_Qmf->no_channels = no_channels;
  h_Qmf->no_col = noCols;

  h_Qmf->lsb = lsb;
  h_Qmf->usb = fMin(usb, h_Qmf->no_channels);

  h_Qmf->FilterStates = (void*)pFilterStates;

  h_Qmf->outScalefactor = ALGORITHMIC_SCALING_IN_ANALYSIS_FILTERBANK + ALGORITHMIC_SCALING_IN_SYNTHESIS_FILTERBANK + h_Qmf->filterScale;

  if ( (h_Qmf->p_stride == 2)
    || ((flags & QMF_FLAG_CLDFB) && (no_channels == 32)) ) {
    h_Qmf->outScalefactor -= 1;
  }
  h_Qmf->outGain = (FIXP_DBL)0x80000000; /* default init value will be not applied */

  return (0);
}

/*!
 *
 * \brief Adjust synthesis qmf filter states
 *
 * \return void
 *
 */
static inline void
qmfAdaptFilterStates (HANDLE_QMF_FILTER_BANK synQmf,     /*!< Handle of Qmf Filter Bank */
                      int scaleFactorDiff)               /*!< Scale factor difference to be applied */
{
  if (synQmf == NULL || synQmf->FilterStates == NULL) {
    return;
  }
  scaleValues((FIXP_QSS*)synQmf->FilterStates, synQmf->no_channels*(QMF_NO_POLY*2 - 1), scaleFactorDiff);
}

/*!
 *
 * \brief Create QMF filter bank instance
 *
 * Only the lower bands are obtained (upto anaQmf->lsb). For
 * a full bandwidth analysis it is required to set both anaQmf->lsb
 * and anaQmf->usb to the amount of QMF bands.
 *
 * \return 0 if succesful
 *
 */
int
qmfInitAnalysisFilterBank (HANDLE_QMF_FILTER_BANK h_Qmf,   /*!< Returns handle */
                           FIXP_QAS *pFilterStates,        /*!< Handle to filter states */
                           int noCols,                     /*!< Number of timeslots per frame */
                           int lsb,                        /*!< lower end of QMF */
                           int usb,                        /*!< upper end of QMF */
                           int no_channels,                /*!< Number of channels (bands) */
                           int flags)                      /*!< Low Power flag */
{
  int err = qmfInitFilterBank(h_Qmf, pFilterStates, noCols, lsb, usb, no_channels, flags);
  if ( !(flags & QMF_FLAG_KEEP_STATES) && (h_Qmf->FilterStates != NULL) ) {
    FDKmemclear(h_Qmf->FilterStates, (2*QMF_NO_POLY-1)*h_Qmf->no_channels*sizeof(FIXP_QAS));
  }

  return err;
}

/*!
 *
 * \brief Create QMF filter bank instance
 *
 * Only the lower bands are obtained (upto anaQmf->lsb). For
 * a full bandwidth analysis it is required to set both anaQmf->lsb
 * and anaQmf->usb to the amount of QMF bands.
 *
 * \return 0 if succesful
 *
 */
int
qmfInitSynthesisFilterBank (HANDLE_QMF_FILTER_BANK h_Qmf,   /*!< Returns handle */
                            FIXP_QSS *pFilterStates,        /*!< Handle to filter states */
                            int noCols,                     /*!< Number of timeslots per frame */
                            int lsb,                        /*!< lower end of QMF */
                            int usb,                        /*!< upper end of QMF */
                            int no_channels,                /*!< Number of channels (bands) */
                            int flags)                      /*!< Low Power flag */
{
  int oldOutScale = h_Qmf->outScalefactor;
  int err = qmfInitFilterBank(h_Qmf, pFilterStates, noCols, lsb, usb, no_channels, flags);
  if ( h_Qmf->FilterStates != NULL ) {
    if ( !(flags & QMF_FLAG_KEEP_STATES) ) {
      FDKmemclear(h_Qmf->FilterStates, (2*QMF_NO_POLY-1)*h_Qmf->no_channels*sizeof(FIXP_QSS));
    } else {
      qmfAdaptFilterStates(h_Qmf, oldOutScale-h_Qmf->outScalefactor);
    }
  }
  return err;
}




/*!
 *
 * \brief Change scale factor for output data and adjust qmf filter states
 *
 * \return void
 *
 */
void
qmfChangeOutScalefactor (HANDLE_QMF_FILTER_BANK synQmf,     /*!< Handle of Qmf Synthesis Bank */
                         int outScalefactor                 /*!< New scaling factor for output data */
                        )
{
  if (synQmf == NULL || synQmf->FilterStates == NULL) {
    return;
  }

  /* Add internal filterbank scale */
  outScalefactor += ALGORITHMIC_SCALING_IN_ANALYSIS_FILTERBANK + ALGORITHMIC_SCALING_IN_SYNTHESIS_FILTERBANK + synQmf->filterScale;

  if ( (synQmf->p_stride == 2)
    || ((synQmf->flags & QMF_FLAG_CLDFB) && (synQmf->no_channels == 32)) ) {
    outScalefactor -= 1;
  }

  /* adjust filter states when scale factor has been changed */
  if (synQmf->outScalefactor != outScalefactor)
  {
    int diff;

    if (outScalefactor > (SAMPLE_BITS - 1)) {
      outScalefactor = SAMPLE_BITS - 1;
    } else if (outScalefactor < (1 - SAMPLE_BITS)) {
      outScalefactor = 1 - SAMPLE_BITS;
    }

    diff = synQmf->outScalefactor - outScalefactor;

    qmfAdaptFilterStates(synQmf, diff);

    /* save new scale factor */
    synQmf->outScalefactor = outScalefactor;
  }
}

/*!
 *
 * \brief Change gain for output data
 *
 * \return void
 *
 */
void
qmfChangeOutGain (HANDLE_QMF_FILTER_BANK synQmf,     /*!< Handle of Qmf Synthesis Bank */
                  FIXP_DBL outputGain                /*!< New gain for output data */
                 )
{
  synQmf->outGain = outputGain;
}

