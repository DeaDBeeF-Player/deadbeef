
/* -----------------------------------------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2015 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
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

/*!
  \file
  \brief  Sbr decoder  
  This module provides the actual decoder implementation. The SBR data (side information) is already
  decoded. Only three functions are provided:

  \li 1.) createSbrDec(): One time initialization
  \li 2.) resetSbrDec(): Called by sbr_Apply() when the information contained in an SBR_HEADER_ELEMENT requires a reset
  and recalculation of important SBR structures.
  \li 3.) sbr_dec(): The actual decoder. Calls the different tools such as filterbanks, lppTransposer(), and calculateSbrEnvelope()
  [the envelope adjuster].

  \sa sbr_dec(), \ref documentationOverview
*/

#include "sbr_dec.h"

#include "sbr_ram.h"
#include "env_extr.h"
#include "env_calc.h"
#include "scale.h"

#include "genericStds.h"

#include "sbrdec_drc.h"



static void assignLcTimeSlots( HANDLE_SBR_DEC hSbrDec,                     /*!< handle to Decoder channel */
                               FIXP_DBL  **QmfBufferReal,
                               int noCols )
{
  int slot, i;
  FIXP_DBL  *ptr;

  /* Number of QMF timeslots in the overlap buffer: */
  ptr = hSbrDec->pSbrOverlapBuffer;
  for(slot=0; slot<hSbrDec->LppTrans.pSettings->overlap; slot++) {
    QmfBufferReal[slot] = ptr; ptr += (64);
  }

  /* Assign timeslots to Workbuffer1 */
  ptr = hSbrDec->WorkBuffer1;
  for(i=0; i<noCols; i++) {
    QmfBufferReal[slot] = ptr; ptr += (64);
    slot++;
  }
}


static void assignHqTimeSlots( HANDLE_SBR_DEC hSbrDec,                     /*!< handle to Decoder channel */
                               FIXP_DBL  **QmfBufferReal,
                               FIXP_DBL  **QmfBufferImag,
                               int noCols )
{
  FIXP_DBL  *ptr;
  int slot;

  /* Number of QMF timeslots in one half of a frame (size of Workbuffer1 or 2): */
  int halflen = (noCols >> 1) + hSbrDec->LppTrans.pSettings->overlap;
  int totCols = noCols + hSbrDec->LppTrans.pSettings->overlap;

  /* Number of QMF timeslots in the overlap buffer: */
  ptr = hSbrDec->pSbrOverlapBuffer;
  for(slot=0; slot<hSbrDec->LppTrans.pSettings->overlap; slot++) {
    QmfBufferReal[slot] = ptr; ptr += (64);
    QmfBufferImag[slot] = ptr; ptr += (64);
  }

  /* Assign first half of timeslots to Workbuffer1 */
  ptr = hSbrDec->WorkBuffer1;
  for(; slot<halflen; slot++) {
    QmfBufferReal[slot] = ptr; ptr += (64);
    QmfBufferImag[slot] = ptr; ptr += (64);
  }

  /* Assign second half of timeslots to Workbuffer2 */
  ptr = hSbrDec->WorkBuffer2;
  for(; slot<totCols; slot++) {
    QmfBufferReal[slot] = ptr; ptr += (64);
    QmfBufferImag[slot] = ptr; ptr += (64);
  }
}


static void assignTimeSlots( HANDLE_SBR_DEC hSbrDec,                     /*!< handle to Decoder channel */
                             int noCols,
                             int useLP )
{
 /* assign qmf time slots */
  hSbrDec->useLP = useLP;
  if (useLP) {
    hSbrDec->SynthesisQMF.flags |= QMF_FLAG_LP;
    hSbrDec->AnalysiscQMF.flags |= QMF_FLAG_LP;
  } else {
    hSbrDec->SynthesisQMF.flags &= ~QMF_FLAG_LP;
    hSbrDec->AnalysiscQMF.flags &= ~QMF_FLAG_LP;
  }
  if (!useLP)
    assignHqTimeSlots( hSbrDec, hSbrDec->QmfBufferReal, hSbrDec->QmfBufferImag, noCols );
  else
  {
    assignLcTimeSlots( hSbrDec, hSbrDec->QmfBufferReal, noCols );
  }
}

static void changeQmfType( HANDLE_SBR_DEC hSbrDec,                     /*!< handle to Decoder channel */
                           int useLdTimeAlign )
{
  UINT synQmfFlags = hSbrDec->SynthesisQMF.flags;
  UINT anaQmfFlags = hSbrDec->AnalysiscQMF.flags;
  int  resetSynQmf = 0;
  int  resetAnaQmf = 0;

  /* assign qmf type */
  if (useLdTimeAlign) {
    if (synQmfFlags & QMF_FLAG_CLDFB) {
      /* change the type to MPSLD */
      synQmfFlags &= ~QMF_FLAG_CLDFB;
      synQmfFlags |=  QMF_FLAG_MPSLDFB;
      resetSynQmf = 1;
    }
    if (anaQmfFlags & QMF_FLAG_CLDFB) {
      /* change the type to MPSLD */
      anaQmfFlags &= ~QMF_FLAG_CLDFB;
      anaQmfFlags |=  QMF_FLAG_MPSLDFB;
      resetAnaQmf = 1;
    }
  } else {
    if (synQmfFlags & QMF_FLAG_MPSLDFB) {
      /* change the type to CLDFB */
      synQmfFlags &= ~QMF_FLAG_MPSLDFB;
      synQmfFlags |=  QMF_FLAG_CLDFB;
      resetSynQmf = 1;
    }
    if (anaQmfFlags & QMF_FLAG_MPSLDFB) {
      /* change the type to CLDFB */
      anaQmfFlags &= ~QMF_FLAG_MPSLDFB;
      anaQmfFlags |=  QMF_FLAG_CLDFB;
      resetAnaQmf = 1;
    }
  }

  if (resetAnaQmf) {
    QMF_FILTER_BANK prvAnaQmf;
    int  qmfErr;

    /* Store current configuration */
    FDKmemcpy(&prvAnaQmf, &hSbrDec->AnalysiscQMF, sizeof(QMF_FILTER_BANK));

    /* Reset analysis QMF */
    qmfErr = qmfInitAnalysisFilterBank (
           &hSbrDec->AnalysiscQMF,
            hSbrDec->anaQmfStates,
            hSbrDec->AnalysiscQMF.no_col,
            hSbrDec->AnalysiscQMF.lsb,
            hSbrDec->AnalysiscQMF.usb,
            hSbrDec->AnalysiscQMF.no_channels,
            anaQmfFlags | QMF_FLAG_KEEP_STATES
            );

    if (qmfErr != 0) {
      /* Restore old configuration of analysis QMF */
      FDKmemcpy(&hSbrDec->AnalysiscQMF, &prvAnaQmf, sizeof(QMF_FILTER_BANK));
    }
  }

  if (resetSynQmf) {
    QMF_FILTER_BANK prvSynQmf;
    int  qmfErr;

    /* Store current configuration */
    FDKmemcpy(&prvSynQmf, &hSbrDec->SynthesisQMF, sizeof(QMF_FILTER_BANK));

    /* Reset synthesis QMF */
    qmfErr = qmfInitSynthesisFilterBank (
           &hSbrDec->SynthesisQMF,
            hSbrDec->pSynQmfStates,
            hSbrDec->SynthesisQMF.no_col,
            hSbrDec->SynthesisQMF.lsb,
            hSbrDec->SynthesisQMF.usb,
            hSbrDec->SynthesisQMF.no_channels,
            synQmfFlags | QMF_FLAG_KEEP_STATES
            );

    if (qmfErr != 0) {
      /* Restore old configuration of synthesis QMF */
      FDKmemcpy(&hSbrDec->SynthesisQMF, &prvSynQmf, sizeof(QMF_FILTER_BANK));
    }
  }
}


/*!
  \brief      SBR decoder core function for one channel

  \image html  BufferMgmtDetailed-1632.png

  Besides the filter states of the QMF filter bank and the LPC-states of
  the LPP-Transposer, processing is mainly based on four buffers:
  #timeIn, #timeOut, #WorkBuffer2 and #OverlapBuffer. The #WorkBuffer2
  is reused for all channels and might be used by the core decoder, a
  static overlap buffer is required for each channel. Du to in-place
  processing, #timeIn and #timeOut point to identical locations.

  The spectral data is organized in so-called slots, each slot
  containing 64 bands of complex data. The number of slots per frame is
  dependend on the frame size. For mp3PRO, there are 18 slots per frame
  and 6 slots per #OverlapBuffer. It is not necessary to have the slots
  in located consecutive address ranges.

  To optimize memory usage and to minimize the number of memory
  accesses, the memory management is organized as follows (Slot numbers
  based on mp3PRO):

  1.) Input time domain signal is located in #timeIn, the last slots
  (0..5) of the spectral data of the previous frame are located in the
  #OverlapBuffer. In addition, #frameData of the current frame resides
  in the upper part of #timeIn.

  2.) During the cplxAnalysisQmfFiltering(), 32 samples from #timeIn are transformed
  into a slot of up to 32 complex spectral low band values at a
  time. The first spectral slot -- nr. 6 -- is written at slot number
  zero of #WorkBuffer2. #WorkBuffer2 will be completely filled with
  spectral data.

  3.) LPP-Transposition in lppTransposer() is processed on 24 slots. During the
  transposition, the high band part of the spectral data is replicated
  based on the low band data.

  Envelope Adjustment is processed on the high band part of the spectral
  data only by calculateSbrEnvelope().

  4.) The cplxSynthesisQmfFiltering() creates 64 time domain samples out
  of a slot of 64 complex spectral values at a time. The first 6 slots
  in #timeOut are filled from the results of spectral slots 0..5 in the
  #OverlapBuffer. The consecutive slots in timeOut are now filled with
  the results of spectral slots 6..17.

  5.) The preprocessed slots 18..23 have to be stored in the
  #OverlapBuffer.

*/

void
sbr_dec ( HANDLE_SBR_DEC hSbrDec,            /*!< handle to Decoder channel */
          INT_PCM *timeIn,                   /*!< pointer to input time signal */
          INT_PCM *timeOut,                  /*!< pointer to output time signal */
          HANDLE_SBR_DEC hSbrDecRight,       /*!< handle to Decoder channel right */
          INT_PCM *timeOutRight,             /*!< pointer to output time signal */
          const int strideIn,                /*!< Time data traversal strideIn */
          const int strideOut,               /*!< Time data traversal strideOut */
          HANDLE_SBR_HEADER_DATA hHeaderData,/*!< Static control data */
          HANDLE_SBR_FRAME_DATA hFrameData,  /*!< Control data of current frame */
          HANDLE_SBR_PREV_FRAME_DATA hPrevFrameData,  /*!< Some control data of last frame */
          const int applyProcessing,         /*!< Flag for SBR operation */
          HANDLE_PS_DEC h_ps_d,
          const UINT flags,
          const int codecFrameSize
         )
{
  int i, slot, reserve;
  int saveLbScale;
  int ov_len;
  int lastSlotOffs;
  FIXP_DBL maxVal;

  /* 1+1/3 frames of spectral data: */
  FIXP_DBL **QmfBufferReal = hSbrDec->QmfBufferReal;
  FIXP_DBL **QmfBufferImag = hSbrDec->QmfBufferImag;

 /* Number of QMF timeslots in the overlap buffer: */
 ov_len = hSbrDec->LppTrans.pSettings->overlap;

 /* Number of QMF slots per frame */
  int noCols = hHeaderData->numberTimeSlots * hHeaderData->timeStep;

 /* assign qmf time slots */
  if ( ((flags & SBRDEC_LOW_POWER ) ? 1 : 0) != ((hSbrDec->SynthesisQMF.flags & QMF_FLAG_LP) ? 1 : 0) ) {
    assignTimeSlots( hSbrDec, hHeaderData->numberTimeSlots * hHeaderData->timeStep, flags & SBRDEC_LOW_POWER);
  }

  if (flags & SBRDEC_ELD_GRID) {
    /* Choose the right low delay filter bank */
    changeQmfType( hSbrDec, (flags & SBRDEC_LD_MPS_QMF) ? 1 : 0 );

    /* If the LD-MPS QMF is not available delay the signal by (96-48*ldSbrSamplingRate)
     * samples according to ISO/IEC 14496-3:2009/FDAM 2:2010(E) chapter 4.5.2.13. */
    if ( (flags & SBRDEC_LD_MPS_QMF)
      && (hSbrDec->AnalysiscQMF.flags & QMF_FLAG_CLDFB) )
    {
      INT_PCM *pDlyBuf = hSbrDec->coreDelayBuf;  /* DLYBUF */
      int smpl, delay = 96 >> (!(flags & SBRDEC_DOWNSAMPLE) ? 1 : 0);
      /* Create TMPBUF */
      C_AALLOC_SCRATCH_START(pcmTemp, INT_PCM, (96));
      /* Copy delay samples from INBUF to TMPBUF */
      for (smpl = 0; smpl < delay; smpl += 1) {
        pcmTemp[smpl] = timeIn[(codecFrameSize-delay+smpl)*strideIn];
      }
      /* Move input signal remainder to the very end of INBUF */
      for (smpl = (codecFrameSize-delay-1)*strideIn; smpl >= 0; smpl -= strideIn) {
        timeIn[smpl+delay] = timeIn[smpl];
      }
      /* Copy delayed samples from last frame from DLYBUF to the very beginning of INBUF */
      for (smpl = 0; smpl < delay; smpl += 1) {
        timeIn[smpl*strideIn] = pDlyBuf[smpl];
      }
      /* Copy TMPBUF to DLYBUF */
      FDKmemcpy(pDlyBuf, pcmTemp, delay*sizeof(INT_PCM));
      /* Destory TMPBUF */
      C_AALLOC_SCRATCH_END(pcmTemp, INT_PCM, (96));
    }
  }

  /*
    low band codec signal subband filtering
   */

  {
    C_AALLOC_SCRATCH_START(qmfTemp, FIXP_DBL, 2*(64));

    qmfAnalysisFiltering( &hSbrDec->AnalysiscQMF,
                           QmfBufferReal + ov_len,
                           QmfBufferImag + ov_len,
                          &hSbrDec->sbrScaleFactor,
                           timeIn,
                           strideIn,
                           qmfTemp
                         );

    C_AALLOC_SCRATCH_END(qmfTemp, FIXP_DBL, 2*(64));
  }

  /*
    Clear upper half of spectrum
  */
  {
    int nAnalysisBands = hHeaderData->numberOfAnalysisBands;

    if (! (flags & SBRDEC_LOW_POWER)) {
      for (slot = ov_len; slot < noCols+ov_len; slot++) {
        FDKmemclear(&QmfBufferReal[slot][nAnalysisBands],((64)-nAnalysisBands)*sizeof(FIXP_DBL));
        FDKmemclear(&QmfBufferImag[slot][nAnalysisBands],((64)-nAnalysisBands)*sizeof(FIXP_DBL));
      }
    } else
    for (slot = ov_len; slot < noCols+ov_len; slot++) {
      FDKmemclear(&QmfBufferReal[slot][nAnalysisBands],((64)-nAnalysisBands)*sizeof(FIXP_DBL));
    }
  }



  /*
    Shift spectral data left to gain accuracy in transposer and adjustor
  */
  maxVal = maxSubbandSample( QmfBufferReal,
                            (flags & SBRDEC_LOW_POWER) ? NULL : QmfBufferImag,
                             0,
                             hSbrDec->AnalysiscQMF.lsb,
                             ov_len,
                             noCols+ov_len );

  reserve = fixMax(0,CntLeadingZeros(maxVal)-1) ;
  reserve = fixMin(reserve,DFRACT_BITS-1-hSbrDec->sbrScaleFactor.lb_scale);

  /* If all data is zero, lb_scale could become too large */
  rescaleSubbandSamples( QmfBufferReal,
                         (flags & SBRDEC_LOW_POWER) ? NULL : QmfBufferImag,
                         0,
                         hSbrDec->AnalysiscQMF.lsb,
                         ov_len,
                         noCols+ov_len,
                         reserve);

  hSbrDec->sbrScaleFactor.lb_scale += reserve;

  /*
    save low band scale, wavecoding or parametric stereo may modify it
  */
  saveLbScale = hSbrDec->sbrScaleFactor.lb_scale;


  if (applyProcessing)
  {
    UCHAR * borders = hFrameData->frameInfo.borders;
    lastSlotOffs =  borders[hFrameData->frameInfo.nEnvelopes] - hHeaderData->numberTimeSlots;

    FIXP_DBL degreeAlias[(64)];

    /* The transposer will override most values in degreeAlias[].
       The array needs to be cleared at least from lowSubband to highSubband before. */
    if (flags & SBRDEC_LOW_POWER)
      FDKmemclear(&degreeAlias[hHeaderData->freqBandData.lowSubband], (hHeaderData->freqBandData.highSubband-hHeaderData->freqBandData.lowSubband)*sizeof(FIXP_DBL));

    /*
      Inverse filtering of lowband and transposition into the SBR-frequency range
    */

    lppTransposer ( &hSbrDec->LppTrans,
                    &hSbrDec->sbrScaleFactor,
                    QmfBufferReal,
                    degreeAlias,                  // only used if useLP = 1
                    QmfBufferImag,
                    flags & SBRDEC_LOW_POWER,
                    hHeaderData->timeStep,
                    borders[0],
                    lastSlotOffs,
                    hHeaderData->freqBandData.nInvfBands,
                    hFrameData->sbr_invf_mode,
                    hPrevFrameData->sbr_invf_mode );





    /*
      Adjust envelope of current frame.
    */

    calculateSbrEnvelope (&hSbrDec->sbrScaleFactor,
                          &hSbrDec->SbrCalculateEnvelope,
                          hHeaderData,
                          hFrameData,
                          QmfBufferReal,
                          QmfBufferImag,
                          flags & SBRDEC_LOW_POWER,

                          degreeAlias,
                          flags,
                          (hHeaderData->frameErrorFlag || hPrevFrameData->frameErrorFlag));


    /*
      Update hPrevFrameData (to be used in the next frame)
    */
    for (i=0; i<hHeaderData->freqBandData.nInvfBands; i++) {
      hPrevFrameData->sbr_invf_mode[i] = hFrameData->sbr_invf_mode[i];
    }
    hPrevFrameData->coupling = hFrameData->coupling;
    hPrevFrameData->stopPos = borders[hFrameData->frameInfo.nEnvelopes];
    hPrevFrameData->ampRes = hFrameData->ampResolutionCurrentFrame;
  }
  else {
    /* Reset hb_scale if no highband is present, because hb_scale is considered in the QMF-synthesis */
    hSbrDec->sbrScaleFactor.hb_scale = saveLbScale;
  }


  for (i=0; i<LPC_ORDER; i++){
    /*
      Store the unmodified qmf Slots values (required for LPC filtering)
    */
    if (! (flags & SBRDEC_LOW_POWER)) {
      FDKmemcpy(hSbrDec->LppTrans.lpcFilterStatesReal[i], QmfBufferReal[noCols-LPC_ORDER+i], hSbrDec->AnalysiscQMF.lsb*sizeof(FIXP_DBL));
      FDKmemcpy(hSbrDec->LppTrans.lpcFilterStatesImag[i], QmfBufferImag[noCols-LPC_ORDER+i], hSbrDec->AnalysiscQMF.lsb*sizeof(FIXP_DBL));
    } else
    FDKmemcpy(hSbrDec->LppTrans.lpcFilterStatesReal[i], QmfBufferReal[noCols-LPC_ORDER+i], hSbrDec->AnalysiscQMF.lsb*sizeof(FIXP_DBL));
  }

  /*
    Synthesis subband filtering.
  */

  if ( ! (flags & SBRDEC_PS_DECODED) ) {

    {
      int outScalefactor = 0;

      if (h_ps_d != NULL) {
        h_ps_d->procFrameBased = 1;  /* we here do frame based processing */
      }


      sbrDecoder_drcApply(&hSbrDec->sbrDrcChannel,
                           QmfBufferReal,
                           (flags & SBRDEC_LOW_POWER) ? NULL : QmfBufferImag,
                           hSbrDec->SynthesisQMF.no_col,
                          &outScalefactor
                          );



      qmfChangeOutScalefactor(&hSbrDec->SynthesisQMF, outScalefactor );

      {
        C_AALLOC_SCRATCH_START(qmfTemp, FIXP_DBL, 2*(64));

        qmfSynthesisFiltering( &hSbrDec->SynthesisQMF,
                                QmfBufferReal,
                                (flags & SBRDEC_LOW_POWER) ? NULL : QmfBufferImag,
                               &hSbrDec->sbrScaleFactor,
                                hSbrDec->LppTrans.pSettings->overlap,
                                timeOut,
                                strideOut,
                                qmfTemp);

        C_AALLOC_SCRATCH_END(qmfTemp, FIXP_DBL, 2*(64));
      }

    }

  } else { /* (flags & SBRDEC_PS_DECODED) */
    INT i, sdiff, outScalefactor, scaleFactorLowBand, scaleFactorHighBand;
    SCHAR scaleFactorLowBand_ov, scaleFactorLowBand_no_ov;

    HANDLE_QMF_FILTER_BANK synQmf      = &hSbrDec->SynthesisQMF;
    HANDLE_QMF_FILTER_BANK synQmfRight = &hSbrDecRight->SynthesisQMF;

    /* adapt scaling */
    sdiff = hSbrDec->sbrScaleFactor.lb_scale - reserve;                  /* Scaling difference         */
    scaleFactorHighBand   = sdiff - hSbrDec->sbrScaleFactor.hb_scale;    /* Scale of current high band */
    scaleFactorLowBand_ov = sdiff - hSbrDec->sbrScaleFactor.ov_lb_scale; /* Scale of low band overlapping QMF data */
    scaleFactorLowBand_no_ov = sdiff - hSbrDec->sbrScaleFactor.lb_scale; /* Scale of low band current QMF data     */
    outScalefactor  = 0;                                                 /* Initial output scale */

    if (h_ps_d->procFrameBased == 1)    /* If we have switched from frame to slot based processing copy filter states */
    { /* procFrameBased will be unset later */
      /* copy filter states from left to right */
      FDKmemcpy(synQmfRight->FilterStates, synQmf->FilterStates, ((640)-(64))*sizeof(FIXP_QSS));
    }

    /* scale ALL qmf vales ( real and imag ) of mono / left channel to the
       same scale factor ( ov_lb_sf, lb_sf and hq_sf )                      */
    scalFilterBankValues( h_ps_d,                             /* parametric stereo decoder handle     */
                          QmfBufferReal,                      /* qmf filterbank values                */
                          QmfBufferImag,                      /* qmf filterbank values                */
                          synQmf->lsb,                        /* sbr start subband                    */
                          hSbrDec->sbrScaleFactor.ov_lb_scale,
                          hSbrDec->sbrScaleFactor.lb_scale,
                         &scaleFactorLowBand_ov,              /* adapt scaling values */
                         &scaleFactorLowBand_no_ov,           /* adapt scaling values */
                          hSbrDec->sbrScaleFactor.hb_scale,   /* current frame ( highband ) */
                         &scaleFactorHighBand,
                          synQmf->no_col);

    /* use the same synthese qmf values for left and right channel */
    synQmfRight->no_col = synQmf->no_col;
    synQmfRight->lsb    = synQmf->lsb;
    synQmfRight->usb    = synQmf->usb;

    int env=0;

      outScalefactor += (SCAL_HEADROOM+1); /* psDiffScale! */

    {
      C_AALLOC_SCRATCH_START(pWorkBuffer, FIXP_DBL, 2*(64));

      int maxShift = 0;

      if (hSbrDec->sbrDrcChannel.enable != 0) {
        if (hSbrDec->sbrDrcChannel.prevFact_exp > maxShift) {
          maxShift = hSbrDec->sbrDrcChannel.prevFact_exp;
        }
        if (hSbrDec->sbrDrcChannel.currFact_exp > maxShift) {
          maxShift = hSbrDec->sbrDrcChannel.currFact_exp;
        }
        if (hSbrDec->sbrDrcChannel.nextFact_exp > maxShift) {
          maxShift = hSbrDec->sbrDrcChannel.nextFact_exp;
        }
      }

      /* copy DRC data to right channel (with PS both channels use the same DRC gains) */
      FDKmemcpy(&hSbrDecRight->sbrDrcChannel, &hSbrDec->sbrDrcChannel, sizeof(SBRDEC_DRC_CHANNEL));

      for (i = 0; i < synQmf->no_col; i++) {  /* ----- no_col loop ----- */

        INT outScalefactorR, outScalefactorL;
        outScalefactorR = outScalefactorL = outScalefactor;

        /* qmf timeslot of right channel */
        FIXP_DBL* rQmfReal = pWorkBuffer;
        FIXP_DBL* rQmfImag = pWorkBuffer + 64;


        {
          if ( i == h_ps_d->bsData[h_ps_d->processSlot].mpeg.aEnvStartStop[env] ) {
            initSlotBasedRotation( h_ps_d, env, hHeaderData->freqBandData.highSubband );
            env++;
          }

          ApplyPsSlot( h_ps_d,                   /* parametric stereo decoder handle  */
                      (QmfBufferReal + i),       /* one timeslot of left/mono channel */
                      (QmfBufferImag + i),       /* one timeslot of left/mono channel */
                       rQmfReal,                 /* one timeslot or right channel     */
                       rQmfImag);                /* one timeslot or right channel     */
        }


        scaleFactorLowBand = (i<(6)) ? scaleFactorLowBand_ov : scaleFactorLowBand_no_ov;


        sbrDecoder_drcApplySlot ( /* right channel */
                                 &hSbrDecRight->sbrDrcChannel,
                                  rQmfReal,
                                  rQmfImag,
                                  i,
                                  synQmfRight->no_col,
                                  maxShift
                                );

        outScalefactorR += maxShift;

        sbrDecoder_drcApplySlot ( /* left channel */
                                 &hSbrDec->sbrDrcChannel,
                                 *(QmfBufferReal + i),
                                 *(QmfBufferImag + i),
                                  i,
                                  synQmf->no_col,
                                  maxShift
                                );

        outScalefactorL += maxShift;


        /* scale filter states for left and right channel */
        qmfChangeOutScalefactor( synQmf, outScalefactorL );
        qmfChangeOutScalefactor( synQmfRight, outScalefactorR );

        {

          qmfSynthesisFilteringSlot( synQmfRight,
                                     rQmfReal,                /* QMF real buffer */
                                     rQmfImag,                /* QMF imag buffer */
                                     scaleFactorLowBand,
                                     scaleFactorHighBand,
                                     timeOutRight+(i*synQmf->no_channels*strideOut),
                                     strideOut,
                                     pWorkBuffer);

          qmfSynthesisFilteringSlot( synQmf,
                                   *(QmfBufferReal + i),      /* QMF real buffer */
                                   *(QmfBufferImag + i),      /* QMF imag buffer */
                                     scaleFactorLowBand,
                                     scaleFactorHighBand,
                                     timeOut+(i*synQmf->no_channels*strideOut),
                                     strideOut,
                                     pWorkBuffer);

        }
      } /* no_col loop  i  */

      /* scale back (6) timeslots look ahead for hybrid filterbank to original value */
      rescalFilterBankValues( h_ps_d,
                              QmfBufferReal,
                              QmfBufferImag,
                              synQmf->lsb,
                              synQmf->no_col );

      C_AALLOC_SCRATCH_END(pWorkBuffer, FIXP_DBL, 2*(64));
    }
  }

  sbrDecoder_drcUpdateChannel( &hSbrDec->sbrDrcChannel );


  /*
    Update overlap buffer
    Even bands above usb are copied to avoid outdated spectral data in case
    the stop frequency raises.
  */

  if (hSbrDec->LppTrans.pSettings->overlap > 0)
  {
    if (! (flags & SBRDEC_LOW_POWER)) {
      for ( i=0; i<hSbrDec->LppTrans.pSettings->overlap; i++ ) {
        FDKmemcpy(QmfBufferReal[i], QmfBufferReal[i+noCols], (64)*sizeof(FIXP_DBL));
        FDKmemcpy(QmfBufferImag[i], QmfBufferImag[i+noCols], (64)*sizeof(FIXP_DBL));
      }
    } else
      for ( i=0; i<hSbrDec->LppTrans.pSettings->overlap; i++ ) {
        FDKmemcpy(QmfBufferReal[i], QmfBufferReal[i+noCols], (64)*sizeof(FIXP_DBL));
      }
  }

  hSbrDec->sbrScaleFactor.ov_lb_scale = saveLbScale;

  /* Save current frame status */
  hPrevFrameData->frameErrorFlag = hHeaderData->frameErrorFlag;

} // sbr_dec()


/*!
  \brief     Creates sbr decoder structure
  \return    errorCode, 0 if successful
*/
SBR_ERROR
createSbrDec (SBR_CHANNEL * hSbrChannel,
              HANDLE_SBR_HEADER_DATA hHeaderData, /*!< Static control data */
              TRANSPOSER_SETTINGS *pSettings,
              const int     downsampleFac,        /*!< Downsampling factor */
              const UINT    qmfFlags,             /*!< flags -> 1: HQ/LP selector, 2: CLDFB */
              const UINT    flags,
              const int     overlap, 
              int           chan)                 /*!< Channel for which to assign buffers etc. */

{
  SBR_ERROR err = SBRDEC_OK;
  int timeSlots = hHeaderData->numberTimeSlots;   /* Number of SBR slots per frame */
  int noCols = timeSlots * hHeaderData->timeStep; /* Number of QMF slots per frame */
  HANDLE_SBR_DEC hs = &(hSbrChannel->SbrDec);

  /* Initialize scale factors */
  hs->sbrScaleFactor.ov_lb_scale  = 0;
  hs->sbrScaleFactor.ov_hb_scale  = 0;
  hs->sbrScaleFactor.hb_scale     = 0;


  /*
    create envelope calculator
  */
  err = createSbrEnvelopeCalc (&hs->SbrCalculateEnvelope,
                               hHeaderData,
                               chan,
                               flags);
  if (err != SBRDEC_OK) {
    return err;
  }

  /*
    create QMF filter banks
  */
  {
    int qmfErr;
    /* Adapted QMF analysis post-twiddles for down-sampled HQ SBR */
    const UINT downSampledFlag = (flags & SBRDEC_DOWNSAMPLE) ? QMF_FLAG_DOWNSAMPLED : 0;

    qmfErr = qmfInitAnalysisFilterBank (
                    &hs->AnalysiscQMF,
                     hs->anaQmfStates,
                     noCols,
                     hHeaderData->freqBandData.lowSubband,
                     hHeaderData->freqBandData.highSubband,
                     hHeaderData->numberOfAnalysisBands,
                     (qmfFlags & (~QMF_FLAG_KEEP_STATES)) | downSampledFlag
                     );
    if (qmfErr != 0) {
      return SBRDEC_UNSUPPORTED_CONFIG;
    }
  }
  if (hs->pSynQmfStates == NULL) {
    hs->pSynQmfStates = GetRam_sbr_QmfStatesSynthesis(chan);
    if (hs->pSynQmfStates == NULL)
      return SBRDEC_MEM_ALLOC_FAILED;
  }

  {
    int qmfErr;

    qmfErr = qmfInitSynthesisFilterBank (
           &hs->SynthesisQMF,
            hs->pSynQmfStates,
            noCols,
            hHeaderData->freqBandData.lowSubband,
            hHeaderData->freqBandData.highSubband,
            (64) / downsampleFac,
            qmfFlags & (~QMF_FLAG_KEEP_STATES)
            );

    if (qmfErr != 0) {
      return SBRDEC_UNSUPPORTED_CONFIG;
    }
  }
  initSbrPrevFrameData (&hSbrChannel->prevFrameData, timeSlots);

  /*
    create transposer
  */
  err = createLppTransposer (&hs->LppTrans,
                             pSettings,
                             hHeaderData->freqBandData.lowSubband,
                             hHeaderData->freqBandData.v_k_master,
                             hHeaderData->freqBandData.numMaster,
                             hs->SynthesisQMF.usb,
                             timeSlots,
                             hs->AnalysiscQMF.no_col,
                             hHeaderData->freqBandData.freqBandTableNoise,
                             hHeaderData->freqBandData.nNfb,
                             hHeaderData->sbrProcSmplRate,
                             chan,
                             overlap );
  if (err != SBRDEC_OK) {
    return err;
  }

  /* The CLDFB does not have overlap */
  if ((qmfFlags & QMF_FLAG_CLDFB) == 0) {
    if (hs->pSbrOverlapBuffer == NULL) {
      hs->pSbrOverlapBuffer = GetRam_sbr_OverlapBuffer(chan);
      if (hs->pSbrOverlapBuffer == NULL)  {
        return SBRDEC_MEM_ALLOC_FAILED;
      }
    } else {
      /* Clear overlap buffer */
      FDKmemclear( hs->pSbrOverlapBuffer,
                   sizeof(FIXP_DBL) * 2 * (6) * (64)
                 );
    }
  }

  /* Clear input delay line */
  FDKmemclear(hs->coreDelayBuf, (96)*sizeof(INT_PCM));

  /* assign qmf time slots */
  assignTimeSlots( &hSbrChannel->SbrDec, hHeaderData->numberTimeSlots * hHeaderData->timeStep, qmfFlags & QMF_FLAG_LP);

  return err;
}

/*!
  \brief     Delete sbr decoder structure
  \return    errorCode, 0 if successful
*/
int
deleteSbrDec (SBR_CHANNEL * hSbrChannel)
{
  HANDLE_SBR_DEC hs = &hSbrChannel->SbrDec;

  deleteSbrEnvelopeCalc (&hs->SbrCalculateEnvelope);

  /* delete QMF filter states */
  if (hs->pSynQmfStates != NULL) {
    FreeRam_sbr_QmfStatesSynthesis(&hs->pSynQmfStates);
  }


  if (hs->pSbrOverlapBuffer != NULL) {
    FreeRam_sbr_OverlapBuffer(&hs->pSbrOverlapBuffer);
  }

  return 0;
}


/*!
  \brief     resets sbr decoder structure
  \return    errorCode, 0 if successful
*/
SBR_ERROR
resetSbrDec (HANDLE_SBR_DEC hSbrDec,
             HANDLE_SBR_HEADER_DATA hHeaderData,
             HANDLE_SBR_PREV_FRAME_DATA hPrevFrameData,
             const int useLP,
             const int downsampleFac
             )
{
  SBR_ERROR sbrError = SBRDEC_OK;
  
  int old_lsb = hSbrDec->SynthesisQMF.lsb;
  int new_lsb = hHeaderData->freqBandData.lowSubband;
  int l, startBand, stopBand, startSlot, size;

  int source_scale, target_scale, delta_scale, target_lsb, target_usb, reserve;
  FIXP_DBL maxVal;

  /* overlapBuffer point to first (6) slots */
  FIXP_DBL  **OverlapBufferReal = hSbrDec->QmfBufferReal;
  FIXP_DBL  **OverlapBufferImag = hSbrDec->QmfBufferImag;

  /* assign qmf time slots */
  assignTimeSlots( hSbrDec, hHeaderData->numberTimeSlots * hHeaderData->timeStep, useLP);



  resetSbrEnvelopeCalc (&hSbrDec->SbrCalculateEnvelope);

  hSbrDec->SynthesisQMF.lsb = hHeaderData->freqBandData.lowSubband;
  hSbrDec->SynthesisQMF.usb = fixMin((INT)hSbrDec->SynthesisQMF.no_channels, (INT)hHeaderData->freqBandData.highSubband);

  hSbrDec->AnalysiscQMF.lsb = hSbrDec->SynthesisQMF.lsb;
  hSbrDec->AnalysiscQMF.usb = hSbrDec->SynthesisQMF.usb;


  /*
    The following initialization of spectral data in the overlap buffer
    is required for dynamic x-over or a change of the start-freq for 2 reasons:

    1. If the lowband gets _wider_, unadjusted data would remain

    2. If the lowband becomes _smaller_, the highest bands of the old lowband
       must be cleared because the whitening would be affected
  */
  startBand = old_lsb;
  stopBand  = new_lsb;
  startSlot = hHeaderData->timeStep * (hPrevFrameData->stopPos - hHeaderData->numberTimeSlots);
  size      = fixMax(0,stopBand-startBand);

  /* keep already adjusted data in the x-over-area */
  if (!useLP) {
    for (l=startSlot; l<hSbrDec->LppTrans.pSettings->overlap; l++) {
      FDKmemclear(&OverlapBufferReal[l][startBand], size*sizeof(FIXP_DBL));
      FDKmemclear(&OverlapBufferImag[l][startBand], size*sizeof(FIXP_DBL));
    }
  } else
  for (l=startSlot; l<hSbrDec->LppTrans.pSettings->overlap ; l++) {
    FDKmemclear(&OverlapBufferReal[l][startBand], size*sizeof(FIXP_DBL));
  }


  /*
    reset LPC filter states
  */
  startBand = fixMin(old_lsb,new_lsb);
  stopBand  = fixMax(old_lsb,new_lsb);
  size      = fixMax(0,stopBand-startBand);

  FDKmemclear(&hSbrDec->LppTrans.lpcFilterStatesReal[0][startBand], size*sizeof(FIXP_DBL));
  FDKmemclear(&hSbrDec->LppTrans.lpcFilterStatesReal[1][startBand], size*sizeof(FIXP_DBL));
  if (!useLP) {
    FDKmemclear(&hSbrDec->LppTrans.lpcFilterStatesImag[0][startBand], size*sizeof(FIXP_DBL));
    FDKmemclear(&hSbrDec->LppTrans.lpcFilterStatesImag[1][startBand], size*sizeof(FIXP_DBL));
  }


  /*
    Rescale already processed spectral data between old and new x-over frequency.
    This must be done because of the separate scalefactors for lowband and highband.
  */
  startBand = fixMin(old_lsb,new_lsb);
  stopBand =  fixMax(old_lsb,new_lsb);

  if (new_lsb > old_lsb) {
    /* The x-over-area was part of the highband before and will now belong to the lowband */
    source_scale = hSbrDec->sbrScaleFactor.ov_hb_scale;
    target_scale = hSbrDec->sbrScaleFactor.ov_lb_scale;
    target_lsb   = 0;
    target_usb   = old_lsb;
  }
  else {
    /* The x-over-area was part of the lowband before and will now belong to the highband */
    source_scale = hSbrDec->sbrScaleFactor.ov_lb_scale;
    target_scale = hSbrDec->sbrScaleFactor.ov_hb_scale;
    /* jdr: The values old_lsb and old_usb might be wrong because the previous frame might have been "upsamling". */
    target_lsb   = hSbrDec->SynthesisQMF.lsb;
    target_usb   = hSbrDec->SynthesisQMF.usb;
  }

  /* Shift left all samples of the x-over-area as much as possible
     An unnecessary coarse scale could cause ov_lb_scale or ov_hb_scale to be
     adapted and the accuracy in the next frame would seriously suffer! */

  maxVal = maxSubbandSample( OverlapBufferReal,
                             (useLP) ? NULL : OverlapBufferImag,
                             startBand,
                             stopBand,
                             0,
                             startSlot);

  reserve = CntLeadingZeros(maxVal)-1;
  reserve = fixMin(reserve,DFRACT_BITS-1-source_scale);

  rescaleSubbandSamples( OverlapBufferReal,
                         (useLP) ? NULL : OverlapBufferImag,
                         startBand,
                         stopBand,
                         0,
                         startSlot,
                         reserve);
  source_scale += reserve;

  delta_scale = target_scale - source_scale;

  if (delta_scale > 0) { /* x-over-area is dominant */
    delta_scale = -delta_scale;
    startBand = target_lsb;
    stopBand = target_usb;

    if (new_lsb > old_lsb) {
      /* The lowband has to be rescaled */
      hSbrDec->sbrScaleFactor.ov_lb_scale = source_scale;
    }
    else {
      /* The highband has be be rescaled */
      hSbrDec->sbrScaleFactor.ov_hb_scale = source_scale;
    }
  }

  FDK_ASSERT(startBand <= stopBand);

  if (!useLP) {
    for (l=0; l<startSlot; l++) {
      scaleValues( OverlapBufferReal[l] + startBand, stopBand-startBand, delta_scale );
      scaleValues( OverlapBufferImag[l] + startBand, stopBand-startBand, delta_scale );
    }
  } else
  for (l=0; l<startSlot; l++) {
    scaleValues( OverlapBufferReal[l] + startBand, stopBand-startBand, delta_scale );
  }


  /*
    Initialize transposer and limiter
  */
  sbrError = resetLppTransposer (&hSbrDec->LppTrans,
                                 hHeaderData->freqBandData.lowSubband,
                                 hHeaderData->freqBandData.v_k_master,
                                 hHeaderData->freqBandData.numMaster,
                                 hHeaderData->freqBandData.freqBandTableNoise,
                                 hHeaderData->freqBandData.nNfb,
                                 hHeaderData->freqBandData.highSubband,
                                 hHeaderData->sbrProcSmplRate);
  if (sbrError != SBRDEC_OK)
    return sbrError;

  sbrError = ResetLimiterBands ( hHeaderData->freqBandData.limiterBandTable,
                                 &hHeaderData->freqBandData.noLimiterBands,
                                 hHeaderData->freqBandData.freqBandTable[0],
                                 hHeaderData->freqBandData.nSfb[0],
                                 hSbrDec->LppTrans.pSettings->patchParam,
                                 hSbrDec->LppTrans.pSettings->noOfPatches,
                                 hHeaderData->bs_data.limiterBands);


  return sbrError;
}
