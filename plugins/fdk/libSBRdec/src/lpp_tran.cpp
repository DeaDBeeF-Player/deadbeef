
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

/*!
  \file
  \brief  Low Power Profile Transposer,  
  This module provides the transposer. The main entry point is lppTransposer(). The function generates
  high frequency content by copying data from the low band (provided by core codec) into the high band.
  This process is also referred to as "patching". The function also implements spectral whitening by means of
  inverse filtering based on LPC coefficients.

  Together with the QMF filterbank the transposer can be tested using a supplied test program. See main_audio.cpp for details.
  This module does use fractional arithmetic and the accuracy of the computations has an impact on the overall sound quality.
  The module also needs to take into account the different scaling of spectral data.

  \sa lppTransposer(), main_audio.cpp, sbr_scale.h, \ref documentationOverview
*/

#include "lpp_tran.h"

#include "sbr_ram.h"
#include "sbr_rom.h"

#include "genericStds.h"
#include "autocorr2nd.h"



#if defined(__arm__)
#include "arm/lpp_tran_arm.cpp"
#endif



#define LPC_SCALE_FACTOR  2


/*!
 *
 * \brief Get bandwidth expansion factor from filtering level
 *
 * Returns a filter parameter (bandwidth expansion factor) depending on
 * the desired filtering level signalled in the bitstream.
 * When switching the filtering level from LOW to OFF, an additional
 * level is being inserted to achieve a smooth transition.
 */

#ifndef FUNCTION_mapInvfMode
static FIXP_DBL
mapInvfMode (INVF_MODE mode,
             INVF_MODE prevMode,
             WHITENING_FACTORS whFactors)
{
  switch (mode) {
  case INVF_LOW_LEVEL:
    if(prevMode == INVF_OFF)
      return whFactors.transitionLevel;
    else
      return whFactors.lowLevel;

  case INVF_MID_LEVEL:
    return whFactors.midLevel;

  case INVF_HIGH_LEVEL:
    return whFactors.highLevel;

  default:
    if(prevMode == INVF_LOW_LEVEL)
      return whFactors.transitionLevel;
    else
      return whFactors.off;
  }
}
#endif /* #ifndef FUNCTION_mapInvfMode */

/*!
 *
 * \brief Perform inverse filtering level emphasis
 *
 * Retrieve bandwidth expansion factor and apply smoothing for each filter band
 *
 */

#ifndef FUNCTION_inverseFilteringLevelEmphasis
static void
inverseFilteringLevelEmphasis(HANDLE_SBR_LPP_TRANS hLppTrans,/*!< Handle of lpp transposer  */
                              UCHAR nInvfBands,              /*!< Number of bands for inverse filtering */
                              INVF_MODE *sbr_invf_mode,      /*!< Current inverse filtering modes */
                              INVF_MODE *sbr_invf_mode_prev, /*!< Previous inverse filtering modes */
                              FIXP_DBL * bwVector            /*!< Resulting filtering levels */
                              )
{
  for(int i = 0; i < nInvfBands; i++) {
    FIXP_DBL accu;
    FIXP_DBL bwTmp = mapInvfMode (sbr_invf_mode[i],
                                  sbr_invf_mode_prev[i],
                                  hLppTrans->pSettings->whFactors);

    if(bwTmp < hLppTrans->bwVectorOld[i]) {
      accu = fMultDiv2(FL2FXCONST_DBL(0.75f),bwTmp) +
             fMultDiv2(FL2FXCONST_DBL(0.25f),hLppTrans->bwVectorOld[i]);
    }
    else {
      accu = fMultDiv2(FL2FXCONST_DBL(0.90625f),bwTmp) +
             fMultDiv2(FL2FXCONST_DBL(0.09375f),hLppTrans->bwVectorOld[i]);
    }

    if (accu <  FL2FXCONST_DBL(0.015625f)>>1)
      bwVector[i] = FL2FXCONST_DBL(0.0f);
    else
      bwVector[i] = fixMin(accu<<1,FL2FXCONST_DBL(0.99609375f));
  }
}
#endif /* #ifndef FUNCTION_inverseFilteringLevelEmphasis */

/* Resulting autocorrelation determinant exponent */
#define ACDET_EXP (2*(DFRACT_BITS+sbrScaleFactor->lb_scale+10-ac.det_scale))
#define AC_EXP (-sbrScaleFactor->lb_scale+LPC_SCALE_FACTOR)
#define ALPHA_EXP (-sbrScaleFactor->lb_scale+LPC_SCALE_FACTOR+1)
/* Resulting transposed QMF values exponent 16 bit normalized samplebits assumed. */
#define QMFOUT_EXP ((SAMPLE_BITS-15)-sbrScaleFactor->lb_scale)

/*!
 *
 * \brief Perform transposition by patching of subband samples.
 * This function serves as the main entry point into the module. The function determines the areas for the
 * patching process (these are the source range as well as the target range) and implements spectral whitening
 * by means of inverse filtering. The function autoCorrelation2nd() is an auxiliary function for calculating the
 * LPC coefficients for the filtering.  The actual calculation of the LPC coefficients and the implementation
 * of the filtering are done as part of lppTransposer().
 *
 * Note that the filtering is done on all available QMF subsamples, whereas the patching is only done on those QMF
 * subsamples that will be used in the next QMF synthesis. The filtering is also implemented before the patching
 * includes further dependencies on parameters from the SBR data.
 *
 */

void lppTransposer (HANDLE_SBR_LPP_TRANS hLppTrans,    /*!< Handle of lpp transposer  */
                    QMF_SCALE_FACTOR  *sbrScaleFactor, /*!< Scaling factors */
                    FIXP_DBL **qmfBufferReal,          /*!< Pointer to pointer to real part of subband samples (source) */

                    FIXP_DBL *degreeAlias,             /*!< Vector for results of aliasing estimation */
                    FIXP_DBL **qmfBufferImag,          /*!< Pointer to pointer to imaginary part of subband samples (source) */
                    const int useLP,
                    const int timeStep,                /*!< Time step of envelope */
                    const int firstSlotOffs,           /*!< Start position in time */
                    const int lastSlotOffs,            /*!< Number of overlap-slots into next frame */
                    const int nInvfBands,              /*!< Number of bands for inverse filtering */
                    INVF_MODE *sbr_invf_mode,          /*!< Current inverse filtering modes */
                    INVF_MODE *sbr_invf_mode_prev      /*!< Previous inverse filtering modes */
                    )
{
  INT    bwIndex[MAX_NUM_PATCHES];
  FIXP_DBL  bwVector[MAX_NUM_PATCHES];       /*!< pole moving factors */

  int    i;
  int    loBand, start, stop;
  TRANSPOSER_SETTINGS *pSettings = hLppTrans->pSettings;
  PATCH_PARAM *patchParam = pSettings->patchParam;
  int    patch;

  FIXP_SGL  alphar[LPC_ORDER], a0r, a1r;
  FIXP_SGL  alphai[LPC_ORDER], a0i=0, a1i=0;
  FIXP_SGL  bw = FL2FXCONST_SGL(0.0f);

  int    autoCorrLength;

  FIXP_DBL k1, k1_below=0, k1_below2=0;

  ACORR_COEFS ac;
  int    startSample;
  int    stopSample;
  int    stopSampleClear;

  int comLowBandScale;
  int ovLowBandShift;
  int lowBandShift;
/*  int ovHighBandShift;*/
  int targetStopBand;


  alphai[0] = FL2FXCONST_SGL(0.0f);
  alphai[1] = FL2FXCONST_SGL(0.0f);


  startSample = firstSlotOffs * timeStep;
  stopSample  = pSettings->nCols + lastSlotOffs * timeStep;


  inverseFilteringLevelEmphasis(hLppTrans, nInvfBands, sbr_invf_mode, sbr_invf_mode_prev, bwVector);

  stopSampleClear = stopSample;

  autoCorrLength = pSettings->nCols + pSettings->overlap;

  /* Set upper subbands to zero:
     This is required in case that the patches do not cover the complete highband
     (because the last patch would be too short).
     Possible optimization: Clearing bands up to usb would be sufficient here. */
  targetStopBand = patchParam[pSettings->noOfPatches-1].targetStartBand
                 + patchParam[pSettings->noOfPatches-1].numBandsInPatch;

  int memSize = ((64) - targetStopBand) * sizeof(FIXP_DBL);

  if (!useLP) {
    for (i = startSample; i < stopSampleClear; i++) {
      FDKmemclear(&qmfBufferReal[i][targetStopBand], memSize);
      FDKmemclear(&qmfBufferImag[i][targetStopBand], memSize);
    }
  } else
  for (i = startSample; i < stopSampleClear; i++) {
    FDKmemclear(&qmfBufferReal[i][targetStopBand], memSize);
  }

  /* init bwIndex for each patch */
  FDKmemclear(bwIndex, pSettings->noOfPatches*sizeof(INT));

  /*
    Calc common low band scale factor
  */
  comLowBandScale = fixMin(sbrScaleFactor->ov_lb_scale,sbrScaleFactor->lb_scale);

  ovLowBandShift =  sbrScaleFactor->ov_lb_scale - comLowBandScale;
  lowBandShift   =  sbrScaleFactor->lb_scale - comLowBandScale;
  /*  ovHighBandShift = firstSlotOffs == 0 ? ovLowBandShift:0;*/

  /* outer loop over bands to do analysis only once for each band */

  if (!useLP) {
    start = pSettings->lbStartPatching;
    stop = pSettings->lbStopPatching;
  } else
  {
    start = fixMax(1, pSettings->lbStartPatching - 2);
    stop = patchParam[0].targetStartBand;
  }


  for ( loBand = start; loBand <  stop; loBand++ ) {

    FIXP_DBL  lowBandReal[(((1024)/(32))+(6))+LPC_ORDER];
    FIXP_DBL *plowBandReal = lowBandReal;
    FIXP_DBL **pqmfBufferReal = qmfBufferReal;
    FIXP_DBL  lowBandImag[(((1024)/(32))+(6))+LPC_ORDER];
    FIXP_DBL *plowBandImag = lowBandImag;
    FIXP_DBL **pqmfBufferImag = qmfBufferImag;
    int resetLPCCoeffs=0;
    int dynamicScale = DFRACT_BITS-1-LPC_SCALE_FACTOR;
    int acDetScale = 0; /* scaling of autocorrelation determinant */

    for(i=0;i<LPC_ORDER;i++){
      *plowBandReal++ = hLppTrans->lpcFilterStatesReal[i][loBand];
      if (!useLP)
        *plowBandImag++ = hLppTrans->lpcFilterStatesImag[i][loBand];
    }

    /*
      Take old slope length qmf slot source values out of (overlap)qmf buffer
    */
    if (!useLP) {
      for(i=0;i<pSettings->nCols+pSettings->overlap;i++){
        *plowBandReal++ = (*pqmfBufferReal++)[loBand];
        *plowBandImag++ = (*pqmfBufferImag++)[loBand];
      }
    } else
    {
      /* pSettings->overlap is always even */
      FDK_ASSERT((pSettings->overlap & 1) == 0);

      for(i=0;i<((pSettings->overlap+pSettings->nCols)>>1);i++) {
        *plowBandReal++ = (*pqmfBufferReal++)[loBand];
        *plowBandReal++ = (*pqmfBufferReal++)[loBand];
      }
      if (pSettings->nCols & 1) {
        *plowBandReal++ = (*pqmfBufferReal++)[loBand];
      }
    }

    /*
      Determine dynamic scaling value.
     */
    dynamicScale = fixMin(dynamicScale, getScalefactor(lowBandReal, LPC_ORDER+pSettings->overlap) + ovLowBandShift);
    dynamicScale = fixMin(dynamicScale, getScalefactor(&lowBandReal[LPC_ORDER+pSettings->overlap], pSettings->nCols) + lowBandShift);
    if (!useLP) {
      dynamicScale = fixMin(dynamicScale, getScalefactor(lowBandImag, LPC_ORDER+pSettings->overlap) + ovLowBandShift);
      dynamicScale = fixMin(dynamicScale, getScalefactor(&lowBandImag[LPC_ORDER+pSettings->overlap], pSettings->nCols) + lowBandShift);
    }
    dynamicScale = fixMax(0, dynamicScale-1); /* one additional bit headroom to prevent -1.0 */

    /*
      Scale temporal QMF buffer.
     */
    scaleValues(&lowBandReal[0], LPC_ORDER+pSettings->overlap, dynamicScale-ovLowBandShift);
    scaleValues(&lowBandReal[LPC_ORDER+pSettings->overlap], pSettings->nCols, dynamicScale-lowBandShift);

    if (!useLP) {
      scaleValues(&lowBandImag[0], LPC_ORDER+pSettings->overlap, dynamicScale-ovLowBandShift);
      scaleValues(&lowBandImag[LPC_ORDER+pSettings->overlap], pSettings->nCols, dynamicScale-lowBandShift);
    }


      if (!useLP) {
        acDetScale += autoCorr2nd_cplx(&ac, lowBandReal+LPC_ORDER, lowBandImag+LPC_ORDER, autoCorrLength);
      }
      else
      {
        acDetScale += autoCorr2nd_real(&ac, lowBandReal+LPC_ORDER, autoCorrLength);
      }

      /* Examine dynamic of determinant in autocorrelation. */
      acDetScale += 2*(comLowBandScale + dynamicScale);
      acDetScale *= 2;              /* two times reflection coefficent scaling */
      acDetScale += ac.det_scale;   /* ac scaling of determinant */

      /* In case of determinant < 10^-38, resetLPCCoeffs=1 has to be enforced. */
      if (acDetScale>126 ) {
        resetLPCCoeffs = 1;
      }


    alphar[1] = FL2FXCONST_SGL(0.0f);
    if (!useLP)
      alphai[1] = FL2FXCONST_SGL(0.0f);

    if (ac.det != FL2FXCONST_DBL(0.0f)) {
      FIXP_DBL tmp,absTmp,absDet;

      absDet = fixp_abs(ac.det);

      if (!useLP) {
        tmp = ( fMultDiv2(ac.r01r,ac.r12r) >> (LPC_SCALE_FACTOR-1) ) -
              ( (fMultDiv2(ac.r01i,ac.r12i) + fMultDiv2(ac.r02r,ac.r11r)) >> (LPC_SCALE_FACTOR-1) );
      } else
      {
        tmp = ( fMultDiv2(ac.r01r,ac.r12r) >> (LPC_SCALE_FACTOR-1) ) -
              ( fMultDiv2(ac.r02r,ac.r11r) >> (LPC_SCALE_FACTOR-1) );
      }
      absTmp = fixp_abs(tmp);

      /*
        Quick check: is first filter coeff >= 1(4)
       */
      {
        INT scale;
        FIXP_DBL result = fDivNorm(absTmp, absDet, &scale);
        scale = scale+ac.det_scale;

        if ( (scale > 0) && (result >= (FIXP_DBL)MAXVAL_DBL>>scale) ) {
          resetLPCCoeffs = 1;
        }
        else {
          alphar[1] = FX_DBL2FX_SGL(scaleValue(result,scale));
          if((tmp<FL2FX_DBL(0.0f)) ^ (ac.det<FL2FX_DBL(0.0f))) {
            alphar[1] = -alphar[1];
          }
        }
      }

      if (!useLP)
      {
        tmp =  ( fMultDiv2(ac.r01i,ac.r12r) >> (LPC_SCALE_FACTOR-1) ) +
               ( (fMultDiv2(ac.r01r,ac.r12i) - (FIXP_DBL)fMultDiv2(ac.r02i,ac.r11r)) >> (LPC_SCALE_FACTOR-1) ) ;

        absTmp = fixp_abs(tmp);

        /*
        Quick check: is second filter coeff >= 1(4)
        */
        {
          INT scale;
          FIXP_DBL result = fDivNorm(absTmp, absDet, &scale);
          scale = scale+ac.det_scale;

          if ( (scale > 0) && (result >= /*FL2FXCONST_DBL(1.f)*/ (FIXP_DBL)MAXVAL_DBL>>scale) ) {
            resetLPCCoeffs = 1;
          }
          else {
            alphai[1] = FX_DBL2FX_SGL(scaleValue(result,scale));
            if((tmp<FL2FX_DBL(0.0f)) ^ (ac.det<FL2FX_DBL(0.0f))) {
              alphai[1] = -alphai[1];
            }
          }
        }
      }
    }

    alphar[0] =  FL2FXCONST_SGL(0.0f);
    if (!useLP)
      alphai[0] = FL2FXCONST_SGL(0.0f);

    if ( ac.r11r != FL2FXCONST_DBL(0.0f) ) {

      /* ac.r11r is always >=0 */
      FIXP_DBL tmp,absTmp;

      if (!useLP) {
        tmp = (ac.r01r>>(LPC_SCALE_FACTOR+1)) +
              (fMultDiv2(alphar[1],ac.r12r) + fMultDiv2(alphai[1],ac.r12i));
      } else
      {
        if(ac.r01r>=FL2FXCONST_DBL(0.0f))
          tmp = (ac.r01r>>(LPC_SCALE_FACTOR+1)) + fMultDiv2(alphar[1],ac.r12r);
        else
          tmp = -((-ac.r01r)>>(LPC_SCALE_FACTOR+1)) + fMultDiv2(alphar[1],ac.r12r);
      }

      absTmp = fixp_abs(tmp);

      /*
        Quick check: is first filter coeff >= 1(4)
      */

      if (absTmp >= (ac.r11r>>1)) {
        resetLPCCoeffs=1;
      }
      else {
        INT scale;
        FIXP_DBL result = fDivNorm(absTmp, fixp_abs(ac.r11r), &scale);
        alphar[0] =  FX_DBL2FX_SGL(scaleValue(result,scale+1));

        if((tmp>FL2FX_DBL(0.0f)) ^ (ac.r11r<FL2FX_DBL(0.0f)))
          alphar[0] = -alphar[0];
      }

      if (!useLP)
      {
        tmp = (ac.r01i>>(LPC_SCALE_FACTOR+1)) +
              (fMultDiv2(alphai[1],ac.r12r) - fMultDiv2(alphar[1],ac.r12i));

        absTmp = fixp_abs(tmp);

        /*
        Quick check: is second filter coeff >= 1(4)
        */
        if (absTmp >= (ac.r11r>>1)) {
          resetLPCCoeffs=1;
        }
        else {
          INT scale;
          FIXP_DBL result = fDivNorm(absTmp, fixp_abs(ac.r11r), &scale);
          alphai[0] = FX_DBL2FX_SGL(scaleValue(result,scale+1));
          if((tmp>FL2FX_DBL(0.0f)) ^ (ac.r11r<FL2FX_DBL(0.0f)))
            alphai[0] = -alphai[0];
        }
      }
    }


    if (!useLP)
    {
      /* Now check the quadratic criteria */
      if( (fMultDiv2(alphar[0],alphar[0]) + fMultDiv2(alphai[0],alphai[0])) >= FL2FXCONST_DBL(0.5f) )
        resetLPCCoeffs=1;
      if( (fMultDiv2(alphar[1],alphar[1]) + fMultDiv2(alphai[1],alphai[1])) >= FL2FXCONST_DBL(0.5f) )
        resetLPCCoeffs=1;
    }

    if(resetLPCCoeffs){
      alphar[0] = FL2FXCONST_SGL(0.0f);
      alphar[1] = FL2FXCONST_SGL(0.0f);
      if (!useLP)
      {
        alphai[0] = FL2FXCONST_SGL(0.0f);
        alphai[1] = FL2FXCONST_SGL(0.0f);
      }
    }

    if (useLP)
    {

      /* Aliasing detection */
      if(ac.r11r==FL2FXCONST_DBL(0.0f)) {
        k1 = FL2FXCONST_DBL(0.0f);
      }
      else {
        if ( fixp_abs(ac.r01r) >= fixp_abs(ac.r11r) ) {
          if ( fMultDiv2(ac.r01r,ac.r11r) < FL2FX_DBL(0.0f)) {
            k1 = (FIXP_DBL)MAXVAL_DBL /*FL2FXCONST_SGL(1.0f)*/;
          }else {
            /* Since this value is squared later, it must not ever become -1.0f. */
            k1 = (FIXP_DBL)(MINVAL_DBL+1) /*FL2FXCONST_SGL(-1.0f)*/;
          }
        }
        else {
          INT scale;
          FIXP_DBL result = fDivNorm(fixp_abs(ac.r01r), fixp_abs(ac.r11r), &scale);
          k1 = scaleValue(result,scale);

          if(!((ac.r01r<FL2FX_DBL(0.0f)) ^ (ac.r11r<FL2FX_DBL(0.0f)))) {
            k1 = -k1;
          }
        }
      }
      if(loBand > 1){
        /* Check if the gain should be locked */
        FIXP_DBL deg = /*FL2FXCONST_DBL(1.0f)*/ (FIXP_DBL)MAXVAL_DBL - fPow2(k1_below);
        degreeAlias[loBand] = FL2FXCONST_DBL(0.0f);
        if (((loBand & 1) == 0) && (k1 < FL2FXCONST_DBL(0.0f))){
          if (k1_below < FL2FXCONST_DBL(0.0f)) {         /* 2-Ch Aliasing Detection */
            degreeAlias[loBand] = (FIXP_DBL)MAXVAL_DBL /*FL2FXCONST_DBL(1.0f)*/;
            if ( k1_below2 > FL2FXCONST_DBL(0.0f) ) {    /* 3-Ch Aliasing Detection */
              degreeAlias[loBand-1] = deg;
            }
          }
          else if ( k1_below2 > FL2FXCONST_DBL(0.0f) ) { /* 3-Ch Aliasing Detection */
            degreeAlias[loBand]   = deg;
          }
        }
        if (((loBand & 1) == 1) && (k1 > FL2FXCONST_DBL(0.0f))){
          if (k1_below > FL2FXCONST_DBL(0.0f)) {         /* 2-CH Aliasing Detection */
            degreeAlias[loBand] = (FIXP_DBL)MAXVAL_DBL /*FL2FXCONST_DBL(1.0f)*/;
            if ( k1_below2 < FL2FXCONST_DBL(0.0f) ) {    /* 3-CH Aliasing Detection */
              degreeAlias[loBand-1] = deg;
            }
          }
          else if ( k1_below2 < FL2FXCONST_DBL(0.0f) ) { /* 3-CH Aliasing Detection */
            degreeAlias[loBand]   = deg;
          }
        }
      }
      /* remember k1 values of the 2 QMF channels below the current channel */
      k1_below2 = k1_below;
      k1_below = k1;
    }

    patch = 0;

    while ( patch < pSettings->noOfPatches ) { /* inner loop over every patch */

      int hiBand = loBand + patchParam[patch].targetBandOffs;

      if ( loBand < patchParam[patch].sourceStartBand
           || loBand >= patchParam[patch].sourceStopBand
           //|| hiBand >= hLppTrans->pSettings->noChannels
           ) {
        /* Lowband not in current patch - proceed */
        patch++;
        continue;
      }

      FDK_ASSERT( hiBand < (64) );

      /* bwIndex[patch] is already initialized with value from previous band inside this patch */
      while (hiBand >= pSettings->bwBorders[bwIndex[patch]])
        bwIndex[patch]++;


      /*
        Filter Step 2: add the left slope with the current filter to the buffer
                       pure source values are already in there
      */
      bw = FX_DBL2FX_SGL(bwVector[bwIndex[patch]]);

      a0r = FX_DBL2FX_SGL(fMult(bw,alphar[0])); /* Apply current bandwidth expansion factor */


      if (!useLP)
        a0i = FX_DBL2FX_SGL(fMult(bw,alphai[0]));
      bw =  FX_DBL2FX_SGL(fPow2(bw));
      a1r = FX_DBL2FX_SGL(fMult(bw,alphar[1]));
      if (!useLP)
        a1i = FX_DBL2FX_SGL(fMult(bw,alphai[1]));



      /*
        Filter Step 3: insert the middle part which won't be windowed
      */

      if ( bw <= FL2FXCONST_SGL(0.0f) ) {
        if (!useLP) {
          int descale = fixMin(DFRACT_BITS-1, (LPC_SCALE_FACTOR+dynamicScale));
          for(i = startSample; i < stopSample; i++ ) {
            qmfBufferReal[i][hiBand] = lowBandReal[LPC_ORDER+i]>>descale;
            qmfBufferImag[i][hiBand] = lowBandImag[LPC_ORDER+i]>>descale;
          }
        } else
        {
          int descale = fixMin(DFRACT_BITS-1, (LPC_SCALE_FACTOR+dynamicScale));
          for(i = startSample; i < stopSample; i++ ) {
            qmfBufferReal[i][hiBand] = lowBandReal[LPC_ORDER+i]>>descale;
          }
        }
      }
      else {  /* bw <= 0 */

        if (!useLP) {
          int descale = fixMin(DFRACT_BITS-1, (LPC_SCALE_FACTOR+dynamicScale));
#ifdef FUNCTION_LPPTRANSPOSER_func1
          lppTransposer_func1(lowBandReal+LPC_ORDER+startSample,lowBandImag+LPC_ORDER+startSample,
                              qmfBufferReal+startSample,qmfBufferImag+startSample,
                              stopSample-startSample, (int) hiBand,
                              dynamicScale,descale,
                              a0r, a0i, a1r, a1i);
#else
          for(i = startSample; i < stopSample; i++ ) {
            FIXP_DBL accu1, accu2;

            accu1 = (fMultDiv2(a0r,lowBandReal[LPC_ORDER+i-1]) - fMultDiv2(a0i,lowBandImag[LPC_ORDER+i-1]) +
                     fMultDiv2(a1r,lowBandReal[LPC_ORDER+i-2]) - fMultDiv2(a1i,lowBandImag[LPC_ORDER+i-2]))>>dynamicScale;
            accu2 = (fMultDiv2(a0i,lowBandReal[LPC_ORDER+i-1]) + fMultDiv2(a0r,lowBandImag[LPC_ORDER+i-1]) +
                     fMultDiv2(a1i,lowBandReal[LPC_ORDER+i-2]) + fMultDiv2(a1r,lowBandImag[LPC_ORDER+i-2]))>>dynamicScale;

            qmfBufferReal[i][hiBand] = (lowBandReal[LPC_ORDER+i]>>descale) + (accu1<<1);
            qmfBufferImag[i][hiBand] = (lowBandImag[LPC_ORDER+i]>>descale) + (accu2<<1);
          }
#endif
        } else
        {
          int descale = fixMin(DFRACT_BITS-1, (LPC_SCALE_FACTOR+dynamicScale));

          FDK_ASSERT(dynamicScale >= 0);
          for(i = startSample; i < stopSample; i++ ) {
            FIXP_DBL accu1;

            accu1 = (fMultDiv2(a0r,lowBandReal[LPC_ORDER+i-1]) + fMultDiv2(a1r,lowBandReal[LPC_ORDER+i-2]))>>dynamicScale;

            qmfBufferReal[i][hiBand] = (lowBandReal[LPC_ORDER+i]>>descale) + (accu1<<1);
          }
        }
      } /* bw <= 0 */

      patch++;

    }  /* inner loop over patches */

     /*
     * store the unmodified filter coefficients if there is
     * an overlapping envelope
     *****************************************************************/


  }  /* outer loop over bands (loBand) */

  if (useLP)
  {
    for ( loBand = pSettings->lbStartPatching; loBand <  pSettings->lbStopPatching; loBand++ ) {
      patch = 0;
      while ( patch < pSettings->noOfPatches ) {

        UCHAR hiBand = loBand + patchParam[patch].targetBandOffs;

        if ( loBand < patchParam[patch].sourceStartBand
          || loBand >= patchParam[patch].sourceStopBand
          || hiBand >= (64)              /* Highband out of range (biterror) */
          ) {
          /* Lowband not in current patch or highband out of range (might be caused by biterrors)- proceed */
          patch++;
          continue;
        }

        if(hiBand != patchParam[patch].targetStartBand)
          degreeAlias[hiBand] = degreeAlias[loBand];

        patch++;
      }
    }/* end  for loop */
  }

 for (i = 0; i < nInvfBands; i++ ) {
   hLppTrans->bwVectorOld[i] = bwVector[i];
 }

  /*
    set high band scale factor
  */
  sbrScaleFactor->hb_scale = comLowBandScale-(LPC_SCALE_FACTOR);

}

/*!
 *
 * \brief Initialize one low power transposer instance
 *
 *
 */
SBR_ERROR
createLppTransposer (HANDLE_SBR_LPP_TRANS hs, /*!< Handle of low power transposer  */
                     TRANSPOSER_SETTINGS *pSettings, /*!< Pointer to settings */
                     const int  highBandStartSb, /*!< ? */
                     UCHAR *v_k_master,         /*!< Master table */
                     const int numMaster,       /*!< Valid entries in master table */
                     const int usb,             /*!< Highband area stop subband */
                     const int timeSlots,       /*!< Number of time slots */
                     const int nCols,           /*!< Number of colums (codec qmf bank) */
                     UCHAR *noiseBandTable,     /*!< Mapping of SBR noise bands to QMF bands */
                     const int  noNoiseBands,   /*!< Number of noise bands */
                     UINT   fs,                 /*!< Sample Frequency */
                     const int chan,            /*!< Channel number */
                     const int overlap
                     )
{
  /* FB inverse filtering settings */
  hs->pSettings = pSettings;

  pSettings->nCols = nCols;
  pSettings->overlap = overlap;

  switch (timeSlots) {

  case 15:
  case 16:
    break;

  default:
    return SBRDEC_UNSUPPORTED_CONFIG; /* Unimplemented */
  }

  if (chan==0) {
    /* Init common data only once */
    hs->pSettings->nCols = nCols;

    return resetLppTransposer (hs,
                               highBandStartSb,
                               v_k_master,
                               numMaster,
                               noiseBandTable,
                               noNoiseBands,
                               usb,
                               fs);
  }
  return SBRDEC_OK;
}


static int findClosestEntry(UCHAR goalSb, UCHAR *v_k_master, UCHAR numMaster, UCHAR direction)
{
  int index;

  if( goalSb <= v_k_master[0] )
    return v_k_master[0];

  if( goalSb >= v_k_master[numMaster] )
    return v_k_master[numMaster];

  if(direction) {
    index = 0;
    while( v_k_master[index] < goalSb ) {
      index++;
    }
  } else {
    index = numMaster;
    while( v_k_master[index] > goalSb ) {
      index--;
    }
  }

  return v_k_master[index];
}


/*!
 *
 * \brief Reset memory for one lpp transposer instance
 *
 * \return SBRDEC_OK on success, SBRDEC_UNSUPPORTED_CONFIG on error
 */
SBR_ERROR
resetLppTransposer (HANDLE_SBR_LPP_TRANS hLppTrans,  /*!< Handle of lpp transposer  */
                    UCHAR  highBandStartSb,          /*!< High band area: start subband */
                    UCHAR *v_k_master,               /*!< Master table */
                    UCHAR  numMaster,                /*!< Valid entries in master table */
                    UCHAR *noiseBandTable,           /*!< Mapping of SBR noise bands to QMF bands */
                    UCHAR  noNoiseBands,             /*!< Number of noise bands */
                    UCHAR  usb,                      /*!< High band area: stop subband */
                    UINT   fs                        /*!< SBR output sampling frequency */
                    )
{
  TRANSPOSER_SETTINGS *pSettings = hLppTrans->pSettings;
  PATCH_PARAM  *patchParam = pSettings->patchParam;

  int i, patch;
  int targetStopBand;
  int sourceStartBand;
  int patchDistance;
  int numBandsInPatch;

  int lsb = v_k_master[0];                 /* Start subband expressed in "non-critical" sampling terms*/
  int xoverOffset = highBandStartSb - lsb; /* Calculate distance in QMF bands between k0 and kx */
  int startFreqHz;

  int desiredBorder;

  usb = fixMin(usb, v_k_master[numMaster]); /* Avoid endless loops (compare with float code). */

  /*
   * Plausibility check
   */

  if ( lsb - SHIFT_START_SB < 4 ) {
    return SBRDEC_UNSUPPORTED_CONFIG;
  }


  /*
   * Initialize the patching parameter
   */
  /* ISO/IEC 14496-3 (Figure 4.48): goalSb = round( 2.048e6 / fs ) */
  desiredBorder    = (((2048000*2) / fs) + 1) >> 1;

  desiredBorder = findClosestEntry(desiredBorder, v_k_master, numMaster, 1); /* Adapt region to master-table */

  /* First patch */
  sourceStartBand = SHIFT_START_SB + xoverOffset;
  targetStopBand = lsb + xoverOffset; /* upperBand */

  /* Even (odd) numbered channel must be patched to even (odd) numbered channel */
  patch = 0;
  while(targetStopBand < usb) {

    /* Too many patches?
       Allow MAX_NUM_PATCHES+1 patches here.
       we need to check later again, since patch might be the highest patch
       AND contain less than 3 bands => actual number of patches will be reduced by 1.
    */
    if (patch > MAX_NUM_PATCHES) {
      return SBRDEC_UNSUPPORTED_CONFIG;
    }

    patchParam[patch].guardStartBand = targetStopBand;
    patchParam[patch].targetStartBand = targetStopBand;

    numBandsInPatch = desiredBorder - targetStopBand;                   /* Get the desired range of the patch */

    if ( numBandsInPatch >= lsb - sourceStartBand ) {
      /* Desired number bands are not available -> patch whole source range */
      patchDistance   = targetStopBand - sourceStartBand;        /* Get the targetOffset */
      patchDistance   = patchDistance & ~1;                      /* Rounding off odd numbers and make all even */
      numBandsInPatch = lsb - (targetStopBand - patchDistance);  /* Update number of bands to be patched */
      numBandsInPatch = findClosestEntry(targetStopBand + numBandsInPatch, v_k_master, numMaster, 0) -
                        targetStopBand;  /* Adapt region to master-table */
    }

    /* Desired number bands are available -> get the minimal even patching distance */
    patchDistance   = numBandsInPatch + targetStopBand - lsb;  /* Get minimal distance */
    patchDistance   = (patchDistance + 1) & ~1;                /* Rounding up odd numbers and make all even */

    if (numBandsInPatch > 0) {
      patchParam[patch].sourceStartBand = targetStopBand - patchDistance;
      patchParam[patch].targetBandOffs  = patchDistance;
      patchParam[patch].numBandsInPatch = numBandsInPatch;
      patchParam[patch].sourceStopBand  = patchParam[patch].sourceStartBand + numBandsInPatch;

      targetStopBand += patchParam[patch].numBandsInPatch;
      patch++;
    }

    /* All patches but first */
    sourceStartBand = SHIFT_START_SB;

    /* Check if we are close to desiredBorder */
    if( desiredBorder - targetStopBand < 3)  /* MPEG doc */
    {
      desiredBorder = usb;
    }

  }

  patch--;

  /* If highest patch contains less than three subband: skip it */
  if ( (patch>0) && (patchParam[patch].numBandsInPatch < 3) ) {
    patch--;
    targetStopBand = patchParam[patch].targetStartBand + patchParam[patch].numBandsInPatch;
  }

  /* now check if we don't have one too many */
  if (patch >= MAX_NUM_PATCHES) {
    return SBRDEC_UNSUPPORTED_CONFIG;
  }

  pSettings->noOfPatches = patch + 1;

  /* Check lowest and highest source subband */
  pSettings->lbStartPatching = targetStopBand;
  pSettings->lbStopPatching  = 0;
  for ( patch = 0; patch < pSettings->noOfPatches; patch++ ) {
    pSettings->lbStartPatching = fixMin( pSettings->lbStartPatching, patchParam[patch].sourceStartBand );
    pSettings->lbStopPatching  = fixMax( pSettings->lbStopPatching, patchParam[patch].sourceStopBand );
  }

  for(i = 0 ; i < noNoiseBands; i++){
    pSettings->bwBorders[i] = noiseBandTable[i+1];
  }

  /*
   * Choose whitening factors
   */

  startFreqHz = ( (lsb + xoverOffset)*fs ) >> 7;  /* Shift does a division by 2*(64) */

  for( i = 1; i < NUM_WHFACTOR_TABLE_ENTRIES; i++ )
  {
    if( startFreqHz < FDK_sbrDecoder_sbr_whFactorsIndex[i])
      break;
  }
  i--;

  pSettings->whFactors.off = FDK_sbrDecoder_sbr_whFactorsTable[i][0];
  pSettings->whFactors.transitionLevel = FDK_sbrDecoder_sbr_whFactorsTable[i][1];
  pSettings->whFactors.lowLevel = FDK_sbrDecoder_sbr_whFactorsTable[i][2];
  pSettings->whFactors.midLevel = FDK_sbrDecoder_sbr_whFactorsTable[i][3];
  pSettings->whFactors.highLevel = FDK_sbrDecoder_sbr_whFactorsTable[i][4];

  return SBRDEC_OK;
}
