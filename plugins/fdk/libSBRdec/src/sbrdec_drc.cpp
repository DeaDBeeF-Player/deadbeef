
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

/*****************************  MPEG-4 AAC Decoder  **************************

   Author(s):   Christian Griebel
   Description: Dynamic range control (DRC) decoder tool for SBR

******************************************************************************/

#include "sbrdec_drc.h"


/* DRC - Offset table for QMF interpolation. */
static const int offsetTab[2][16] =
{
  { 0, 4, 8, 12, 16, 20, 24, 28, 0, 0, 0, 0, 0, 0, 0, 0 },  /* 1024 framing */
  { 0, 4, 8, 12, 16, 19, 22, 26, 0, 0, 0, 0, 0, 0, 0, 0 }   /*  960 framing */
};

/*!
  \brief Initialize DRC QMF factors

  \hDrcData Handle to DRC channel data.

  \return none
*/
void sbrDecoder_drcInitChannel (
    HANDLE_SBR_DRC_CHANNEL  hDrcData )
{
  int band;

  if (hDrcData == NULL) {
    return;
  }

  for (band = 0; band < (64); band++) {
    hDrcData->prevFact_mag[band] = FL2FXCONST_DBL(0.5f);
  }

  for (band = 0; band < SBRDEC_MAX_DRC_BANDS; band++) {
    hDrcData->currFact_mag[band] = FL2FXCONST_DBL(0.5f);
    hDrcData->nextFact_mag[band] = FL2FXCONST_DBL(0.5f);
  }

  hDrcData->prevFact_exp = 1;
  hDrcData->currFact_exp = 1;
  hDrcData->nextFact_exp = 1;

  hDrcData->numBandsCurr = 1;
  hDrcData->numBandsNext = 1;

  hDrcData->winSequenceCurr = 0;
  hDrcData->winSequenceNext = 0;

  hDrcData->drcInterpolationSchemeCurr = 0;
  hDrcData->drcInterpolationSchemeNext = 0;

  hDrcData->enable = 0;
}


/*!
  \brief Swap DRC QMF scaling factors after they have been applied.

  \hDrcData Handle to DRC channel data.

  \return none
*/
void sbrDecoder_drcUpdateChannel (
    HANDLE_SBR_DRC_CHANNEL  hDrcData )
{
  if (hDrcData == NULL) {
    return;
  }
  if (hDrcData->enable != 1) {
    return;
  }

  /* swap previous data */
  FDKmemcpy( hDrcData->currFact_mag,
             hDrcData->nextFact_mag,
             SBRDEC_MAX_DRC_BANDS * sizeof(FIXP_DBL) );

  hDrcData->currFact_exp = hDrcData->nextFact_exp;

  hDrcData->numBandsCurr = hDrcData->numBandsNext;

  FDKmemcpy( hDrcData->bandTopCurr,
             hDrcData->bandTopNext,
             SBRDEC_MAX_DRC_BANDS * sizeof(USHORT) );

  hDrcData->drcInterpolationSchemeCurr = hDrcData->drcInterpolationSchemeNext;

  hDrcData->winSequenceCurr = hDrcData->winSequenceNext;
}


/*!
  \brief Apply DRC factors slot based.

  \hDrcData Handle to DRC channel data.
  \qmfRealSlot Pointer to real valued QMF data of one time slot.
  \qmfImagSlot Pointer to the imaginary QMF data of one time slot.
  \col Number of the time slot.
  \numQmfSubSamples Total number of time slots for one frame.
  \scaleFactor Pointer to the out scale factor of the time slot.

  \return None.
*/
void sbrDecoder_drcApplySlot (
    HANDLE_SBR_DRC_CHANNEL  hDrcData,
    FIXP_DBL   *qmfRealSlot,
    FIXP_DBL   *qmfImagSlot,
    int  col,
    int  numQmfSubSamples,
    int  maxShift
  )
{
  const int *offset;

  int band, bottomMdct, topMdct, bin, useLP;
  int indx = numQmfSubSamples - (numQmfSubSamples >> 1) - 10;   /* l_border */
  int frameLenFlag = (numQmfSubSamples == 30) ? 1 : 0;

  const FIXP_DBL *fact_mag = NULL;
  INT fact_exp = 0;
  UINT numBands = 0;
  USHORT *bandTop = NULL;
  int shortDrc = 0;

  FIXP_DBL alphaValue = FL2FXCONST_DBL(0.0f);

  if (hDrcData == NULL) {
    return;
  }
  if (hDrcData->enable != 1) {
    return;
  }

  offset = offsetTab[frameLenFlag];

  useLP = (qmfImagSlot == NULL) ? 1 : 0;

  col += indx;
  bottomMdct = 0;
  bin = 0;

  /* get respective data and calc interpolation factor */
  if (col < (numQmfSubSamples>>1)) {  /* first half of current frame */
    if (hDrcData->winSequenceCurr != 2) { /* long window */
      int j = col + (numQmfSubSamples>>1);

      if (hDrcData->drcInterpolationSchemeCurr == 0) {
        INT k = (frameLenFlag) ? 0x4444444 : 0x4000000;

        alphaValue = (FIXP_DBL)(j * k);
      }
      else {
        if (j >= offset[hDrcData->drcInterpolationSchemeCurr - 1]) {
          alphaValue = (FIXP_DBL)MAXVAL_DBL;
        }
      }
    }
    else {  /* short windows */
      shortDrc = 1;
    }

    fact_mag = hDrcData->currFact_mag;
    fact_exp = hDrcData->currFact_exp;
    numBands = hDrcData->numBandsCurr;
    bandTop = hDrcData->bandTopCurr;
  }
  else if (col < numQmfSubSamples) {  /* second half of current frame */
    if (hDrcData->winSequenceNext != 2) { /* next: long window */
      int j = col - (numQmfSubSamples>>1);

      if (hDrcData->drcInterpolationSchemeNext == 0) {
        INT k = (frameLenFlag) ? 0x4444444 : 0x4000000;

        alphaValue = (FIXP_DBL)(j * k);
      }
      else {
        if (j >= offset[hDrcData->drcInterpolationSchemeNext - 1]) {
          alphaValue = (FIXP_DBL)MAXVAL_DBL;
        }
      }

      fact_mag = hDrcData->nextFact_mag;
      fact_exp = hDrcData->nextFact_exp;
      numBands = hDrcData->numBandsNext;
      bandTop = hDrcData->bandTopNext;
    }
    else {  /* next: short windows */
      if (hDrcData->winSequenceCurr != 2) {  /* current: long window */
        alphaValue = (FIXP_DBL)0;

        fact_mag = hDrcData->nextFact_mag;
        fact_exp = hDrcData->nextFact_exp;
        numBands = hDrcData->numBandsNext;
        bandTop = hDrcData->bandTopNext;
      }
      else {  /* current: short windows */
        shortDrc = 1;

        fact_mag = hDrcData->currFact_mag;
        fact_exp = hDrcData->currFact_exp;
        numBands = hDrcData->numBandsCurr;
        bandTop = hDrcData->bandTopCurr;
      }
    }
  }
  else {  /* first half of next frame */
    if (hDrcData->winSequenceNext != 2) { /* long window */
      int j = col - (numQmfSubSamples>>1);

      if (hDrcData->drcInterpolationSchemeNext == 0) {
        INT k = (frameLenFlag) ? 0x4444444 : 0x4000000;

        alphaValue = (FIXP_DBL)(j * k);
      }
      else {
        if (j >= offset[hDrcData->drcInterpolationSchemeNext - 1]) {
          alphaValue = (FIXP_DBL)MAXVAL_DBL;
        }
      }
    }
    else {  /* short windows */
      shortDrc = 1;
    }

    fact_mag = hDrcData->nextFact_mag;
    fact_exp = hDrcData->nextFact_exp;
    numBands = hDrcData->numBandsNext;
    bandTop = hDrcData->bandTopNext;

    col -= numQmfSubSamples;
  }


  /* process bands */
  for (band = 0; band < (int)numBands; band++) {
    int bottomQmf, topQmf;

    FIXP_DBL drcFact_mag = (FIXP_DBL)MAXVAL_DBL;

    topMdct = (bandTop[band]+1) << 2;

    if (!shortDrc) {  /* long window */
      if (frameLenFlag) {
        /* 960 framing */
        bottomMdct = 30 * (bottomMdct / 30);
        topMdct    = 30 * (topMdct / 30);

        bottomQmf = fMultIfloor((FIXP_DBL)0x4444444, bottomMdct);
        topQmf    = fMultIfloor((FIXP_DBL)0x4444444, topMdct);
      }
      else {
        /* 1024 framing */
        bottomMdct &= ~0x1f;
        topMdct    &= ~0x1f;

        bottomQmf = bottomMdct >> 5;
        topQmf    = topMdct >> 5;
      }

      if (band == ((int)numBands-1)) {
        topQmf = (64);
      }
    
      for (bin = bottomQmf; bin < topQmf; bin++) {
        FIXP_DBL drcFact1_mag = hDrcData->prevFact_mag[bin];
        FIXP_DBL drcFact2_mag = fact_mag[band];

        /* normalize scale factors */
        if (hDrcData->prevFact_exp < maxShift) {
          drcFact1_mag >>= maxShift - hDrcData->prevFact_exp;
        }
        if (fact_exp < maxShift) {
          drcFact2_mag >>= maxShift - fact_exp;
        }

        /* interpolate */
        if (alphaValue == (FIXP_DBL)0) {
          drcFact_mag = drcFact1_mag;
        } else if (alphaValue == (FIXP_DBL)MAXVAL_DBL) {
          drcFact_mag = drcFact2_mag;
        } else {
          drcFact_mag = fMult(alphaValue, drcFact2_mag) + fMult(((FIXP_DBL)MAXVAL_DBL - alphaValue), drcFact1_mag);
        }

        /* apply scaling */
        qmfRealSlot[bin] = fMult(qmfRealSlot[bin], drcFact_mag);
        if (!useLP) {
          qmfImagSlot[bin] = fMult(qmfImagSlot[bin], drcFact_mag);
        }

        /* save previous factors */
        if (col == (numQmfSubSamples>>1)-1) {
          hDrcData->prevFact_mag[bin] = fact_mag[band];
        }
      }
    }
    else {  /* short windows */
      int startSample, stopSample;
      FIXP_DBL invFrameSizeDiv8 = (frameLenFlag) ? (FIXP_DBL)0x1111111 : (FIXP_DBL)0x1000000;

      if (frameLenFlag) {
        /*  960 framing */
        bottomMdct = 30/8 * (bottomMdct*8/30);
        topMdct    = 30/8 * (topMdct*8/30);
      }
      else {
        /* 1024 framing */
        bottomMdct &= ~0x03;
        topMdct    &= ~0x03;
      }

      /* startSample is truncated to the nearest corresponding start subsample in
         the QMF of the short window bottom is present in:*/
      startSample  = ((fMultIfloor( invFrameSizeDiv8, bottomMdct ) & 0x7) * numQmfSubSamples) >> 3;

      /* stopSample is rounded upwards to the nearest corresponding stop subsample
         in the QMF of the short window top is present in. */
      stopSample  = ((fMultIceil( invFrameSizeDiv8, topMdct ) & 0xf) * numQmfSubSamples) >> 3;

      bottomQmf = fMultIfloor( invFrameSizeDiv8, ((bottomMdct%(numQmfSubSamples<<2)) << 5) );
      topQmf    = fMultIfloor( invFrameSizeDiv8, ((topMdct%(numQmfSubSamples<<2)) << 5) );

      /* extend last band */
      if (band == ((int)numBands-1)) {
        topQmf = (64);
        stopSample = numQmfSubSamples;
      }

      if (topQmf == 0) {
        topQmf = (64);
      }

      /* save previous factors */
      if (stopSample == numQmfSubSamples) {
        int tmpBottom = bottomQmf;

        if (((numQmfSubSamples-1) & ~0x03) > startSample) {
            tmpBottom = 0;    /* band starts in previous short window */
        }

        for (bin = tmpBottom; bin < topQmf; bin++) {
          hDrcData->prevFact_mag[bin] = fact_mag[band];
        }
      }

      /* apply */
      if ((col >= startSample) && (col < stopSample)) {
        if ((col & ~0x03) > startSample) {
            bottomQmf = 0;    /* band starts in previous short window */
        }
        if (col < ((stopSample-1) & ~0x03)) {
            topQmf = (64);   /* band ends in next short window */
        }

        drcFact_mag = fact_mag[band];

        /* normalize scale factor */
        if (fact_exp < maxShift) {
          drcFact_mag >>= maxShift - fact_exp;
        }

        /* apply scaling */
        for (bin = bottomQmf; bin < topQmf; bin++) {
          qmfRealSlot[bin] = fMult(qmfRealSlot[bin], drcFact_mag);
          if (!useLP) {
            qmfImagSlot[bin] = fMult(qmfImagSlot[bin], drcFact_mag);
          }
        }
      }
    }

    bottomMdct = topMdct;
  }   /* end of bands loop */

  if (col == (numQmfSubSamples>>1)-1) {
    hDrcData->prevFact_exp = fact_exp;
  }
}


/*!
  \brief Apply DRC factors frame based.

  \hDrcData Handle to DRC channel data.
  \qmfRealSlot Pointer to real valued QMF data of the whole frame.
  \qmfImagSlot Pointer to the imaginary QMF data of the whole frame.
  \numQmfSubSamples Total number of time slots for one frame.
  \scaleFactor Pointer to the out scale factor of the frame.

  \return None.
*/
void sbrDecoder_drcApply (
    HANDLE_SBR_DRC_CHANNEL  hDrcData,
    FIXP_DBL **QmfBufferReal,
    FIXP_DBL **QmfBufferImag,
    int  numQmfSubSamples,
    int *scaleFactor
  )
{
  int col;
  int maxShift = 0;

  if (hDrcData == NULL) {
    return;
  }
  if (hDrcData->enable == 0) {
    return;  /* Avoid changing the scaleFactor even though the processing is disabled. */
  }

  /* get max scale factor */
  if (hDrcData->prevFact_exp > maxShift) {
    maxShift = hDrcData->prevFact_exp;
  }
  if (hDrcData->currFact_exp > maxShift) {
    maxShift = hDrcData->currFact_exp;
  }
  if (hDrcData->nextFact_exp > maxShift) {
    maxShift = hDrcData->nextFact_exp;
  }

  for (col = 0; col < numQmfSubSamples; col++)
  {
    FIXP_DBL *qmfSlotReal = QmfBufferReal[col];
    FIXP_DBL *qmfSlotImag = (QmfBufferImag == NULL) ? NULL : QmfBufferImag[col];

    sbrDecoder_drcApplySlot (
      hDrcData,
      qmfSlotReal,
      qmfSlotImag,
      col,
      numQmfSubSamples,
      maxShift
    );
  }

  *scaleFactor += maxShift;
}

