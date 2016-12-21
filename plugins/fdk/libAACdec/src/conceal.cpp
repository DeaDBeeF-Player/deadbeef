
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

   Author(s):   Josef Hoepfl
   Description: independent channel concealment

******************************************************************************/

/*!
  \page concealment AAC core concealment

  This AAC core implementation includes a concealment function, which can be enabled
  using the several defines during compilation.

  There are various tests inside the core, starting with simple CRC tests and ending in
  a variety of plausibility checks. If such a check indicates an invalid bitstream, then
  concealment is applied.

  Concealment is also applied when the calling main program indicates a distorted or missing
  data frame using the frameOK flag. This is used for error detection on the transport layer.
  (See below)

  There are three concealment-modes:

  1) Muting: The spectral data is simply set to zero in case of an detected error.

  2) Noise substitution: In case of an detected error, concealment copies the last frame and adds
     attenuates the spectral data. For this mode you have to set the #CONCEAL_NOISE define.
     Noise substitution adds no additional delay.

  3) Interpolation: The interpolation routine swaps the spectral data from the previous and the
     current frame just before the final frequency to time conversion. In case a single frame is
     corrupted, concealmant interpolates between the last good and the first good frame to create
     the spectral data for the missing frame. If multiple frames are corrupted, concealment
     implements first a fade out based on slightly modified spectral values from the last good
     frame. As soon as good frames are available, concealmant fades in the new spectral data.
     For this mode you have to set the #CONCEAL_INTER define. Note that in this case, you also
     need to set #SBR_BS_DELAY_ENABLE, which basically adds approriate delay in the SBR decoder.
     Note that the Interpolating-Concealment increases the delay of your decoder by one frame
     and that it does require additional resources such as memory and computational complexity.

  <h2>How concealment can be used with errors on the transport layer</h2>

  Many errors can or have to be detected on the transport layer. For example in IP based systems
  packet loss can occur. The transport protocol used should indicate such packet loss by inserting
  an empty frame with frameOK=0.
*/

#include "conceal.h"

#include "aac_rom.h"
#include "genericStds.h"


/* PNS (of block) */
#include "aacdec_pns.h"
#include "block.h"

#include "FDK_tools_rom.h"

#define CONCEAL_DFLT_COMF_NOISE_LEVEL     ( 46 )  /* ~= -70 dB */


/* default settings */
#define CONCEAL_DFLT_FADEOUT_FRAMES       ( 5 )
#define CONCEAL_DFLT_FADEIN_FRAMES        ( 5 )
#define CONCEAL_DFLT_MUTE_RELEASE_FRAMES  ( 3 )

#define CONCEAL_DFLT_FADE_FACTOR          ( 0.707106781186548f )   /* 1/sqrt(2) */

/* some often used constants: */
#define FIXP_ZERO           FL2FXCONST_DBL(0.0f)
#define FIXP_ONE            FL2FXCONST_DBL(1.0f)
#define FIXP_FL_CORRECTION  FL2FXCONST_DBL(0.53333333333333333f)

/* For parameter conversion */
#define CONCEAL_PARAMETER_BITS              ( 8 )
#define CONCEAL_MAX_QUANT_FACTOR            ( (1<<CONCEAL_PARAMETER_BITS)-1 )
/*#define CONCEAL_MIN_ATTENUATION_FACTOR_025  ( FL2FXCONST_DBL(0.971627951577106174) )*/  /* -0.25 dB */
#define CONCEAL_MIN_ATTENUATION_FACTOR_025_LD  FL2FXCONST_DBL(-0.041524101186092029596853445212299)
/*#define CONCEAL_MIN_ATTENUATION_FACTOR_050  ( FL2FXCONST_DBL(0.944060876285923380) )*/  /* -0.50 dB */
#define CONCEAL_MIN_ATTENUATION_FACTOR_050_LD FL2FXCONST_DBL(-0.083048202372184059253597008145293)

typedef enum {
  CConcealment_NoExpand,
  CConcealment_Expand,
  CConcealment_Compress
}
CConcealmentExpandType;

static const FIXP_SGL facMod4Table[4] = {
  FL2FXCONST_SGL(0.500000000f),   /* FIXP_SGL(0x4000),  2^-(1-0,00) */
  FL2FXCONST_SGL(0.594603558f),   /* FIXP_SGL(0x4c1b),  2^-(1-0,25) */
  FL2FXCONST_SGL(0.707106781f),   /* FIXP_SGL(0x5a82),  2^-(1-0,50) */
  FL2FXCONST_SGL(0.840896415f)    /* FIXP_SGL(0x6ba2)   2^-(1-0,75) */
};




static void
  CConcealment_CalcBandEnergy (
    FIXP_DBL               *spectrum,
    const SamplingRateInfo *pSamplingRateInfo,
    const int               blockType,
    CConcealmentExpandType  ex,
    int                    *sfbEnergy
  );

static void
  CConcealment_InterpolateBuffer (
    FIXP_DBL    *spectrum,
    SHORT       *pSpecScalePrev,
    SHORT       *pSpecScaleAct,
    SHORT       *pSpecScaleOut,
    int         *enPrv,
    int         *enAct,
    int          sfbCnt,
    const SHORT *pSfbOffset
  );

static int
  CConcealment_ApplyInter (
    CConcealmentInfo       *pConcealmentInfo,
    CAacDecoderChannelInfo *pAacDecoderChannelInfo,
    const SamplingRateInfo *pSamplingRateInfo,
    const int  samplesPerFrame,
    const int  improveTonal,
    const int  frameOk
  );



static int
  CConcealment_ApplyNoise (
    CConcealmentInfo *pConcealmentInfo,
    CAacDecoderChannelInfo *pAacDecoderChannelInfo,
    CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
    const SamplingRateInfo *pSamplingRateInfo,
    const int    samplesPerFrame,
    const UINT flags
  );

static void
  CConcealment_UpdateState (
    CConcealmentInfo *pConcealmentInfo,
    int frameOk
  );

static void
  CConcealment_ApplyRandomSign (
    int        iRandomPhase,
    FIXP_DBL  *spec,
    int        samplesPerFrame
  );


static int CConcealment_GetWinSeq(int prevWinSeq)
{
  int newWinSeq = OnlyLongSequence;

  /* Try to have only long blocks */
  if ( prevWinSeq == LongStartSequence
    || prevWinSeq == EightShortSequence )
  {
    newWinSeq = LongStopSequence;
  }

  return (newWinSeq);
}


/*!
  \brief Init common concealment information data

  \pConcealCommonData Pointer to the concealment common data structure.

  \return  none
*/
void
  CConcealment_InitCommonData (CConcealParams *pConcealCommonData)
{
  if (pConcealCommonData != NULL)
  {
    int i;

    /* Set default error concealment technique */
    pConcealCommonData->method = ConcealMethodInter;

    pConcealCommonData->numFadeOutFrames     = CONCEAL_DFLT_FADEOUT_FRAMES;
    pConcealCommonData->numFadeInFrames      = CONCEAL_DFLT_FADEIN_FRAMES;
    pConcealCommonData->numMuteReleaseFrames = CONCEAL_DFLT_MUTE_RELEASE_FRAMES;

    pConcealCommonData->comfortNoiseLevel    = CONCEAL_DFLT_COMF_NOISE_LEVEL;

    /* Init fade factors (symetric) */
    pConcealCommonData->fadeOutFactor[0] = FL2FXCONST_SGL( CONCEAL_DFLT_FADE_FACTOR );
    pConcealCommonData->fadeInFactor[0]  = pConcealCommonData->fadeOutFactor[0];

    for (i = 1; i < CONCEAL_MAX_NUM_FADE_FACTORS; i++) {
      pConcealCommonData->fadeOutFactor[i] = FX_DBL2FX_SGL(fMult(pConcealCommonData->fadeOutFactor[i-1],FL2FXCONST_SGL(CONCEAL_DFLT_FADE_FACTOR)));
      pConcealCommonData->fadeInFactor[i]  = pConcealCommonData->fadeOutFactor[i];
    }
  }
}



/*!
  \brief Get current concealment method.

  \pConcealCommonData Pointer to common concealment data (for all channels)

  \return Concealment method.
*/
CConcealmentMethod
  CConcealment_GetMethod( CConcealParams *pConcealCommonData )
{
  CConcealmentMethod method = ConcealMethodNone;

  if (pConcealCommonData != NULL) {
    method = pConcealCommonData->method;
  }

  return (method);
}


/*!
  \brief Init concealment information for each channel

  The function initializes the concealment information. Two methods can be chosen:
             0 = interpolation method (adds delay)
             1 = noise substitution (no delay, low complexity)

  \return  none
*/
void
  CConcealment_InitChannelData (
    CConcealmentInfo *pConcealChannelInfo,
    CConcealParams   *pConcealCommonData,
    int samplesPerFrame )
{
  int i;

  pConcealChannelInfo->pConcealParams = pConcealCommonData;

  FDKmemclear(pConcealChannelInfo->spectralCoefficient, 1024 * sizeof(FIXP_CNCL));

  for (i = 0; i < 8; i++) {
    pConcealChannelInfo->specScale[i] = 0;
  }

  pConcealChannelInfo->iRandomPhase   = 0;

  pConcealChannelInfo->windowSequence = 0;
  pConcealChannelInfo->windowShape    = 0;

  pConcealChannelInfo->prevFrameOk[0] = 1;
  pConcealChannelInfo->prevFrameOk[1] = 1;

  pConcealChannelInfo->cntFadeFrames  = 0;
  pConcealChannelInfo->cntValidFrames = 0;

  pConcealChannelInfo->concealState   = ConcealState_Ok;

}


/*!
  \brief Set error concealment parameters

  \concealParams
  \method
  \fadeOutSlope
  \fadeInSlope
  \muteRelease
  \comfNoiseLevel

  \return  none
*/
AAC_DECODER_ERROR
  CConcealment_SetParams (
    CConcealParams *concealParams,
    int  method,
    int  fadeOutSlope,
    int  fadeInSlope,
    int  muteRelease,
    int  comfNoiseLevel )
{
  /* set concealment technique */
  if (method != AACDEC_CONCEAL_PARAM_NOT_SPECIFIED) {
    switch ((CConcealmentMethod)method)
    {
    case ConcealMethodMute:
    case ConcealMethodNoise:
    case ConcealMethodInter:
      /* Be sure to enable delay adjustment of SBR decoder! */
      if (concealParams == NULL) {
        return AAC_DEC_INVALID_HANDLE;
      } else {
        /* set param */
        concealParams->method = (CConcealmentMethod)method;
      }
      break;

    default:
      return AAC_DEC_SET_PARAM_FAIL;
    }
  }

  /* set number of frames for fade-out slope */
  if (fadeOutSlope != AACDEC_CONCEAL_PARAM_NOT_SPECIFIED) {
    if ( (fadeOutSlope < CONCEAL_MAX_NUM_FADE_FACTORS)
      && (fadeOutSlope >= 0) )
    {
      if (concealParams == NULL) {
        return AAC_DEC_INVALID_HANDLE;
      } else {
        /* set param */
        concealParams->numFadeOutFrames = fadeOutSlope;
      }
    } else {
      return AAC_DEC_SET_PARAM_FAIL;
    }
  }

  /* set number of frames for fade-in slope */
  if (fadeInSlope != AACDEC_CONCEAL_PARAM_NOT_SPECIFIED) {
    if ( (fadeInSlope < CONCEAL_MAX_NUM_FADE_FACTORS)
      && (fadeInSlope >= 1) )
    {
      if (concealParams == NULL) {
        return AAC_DEC_INVALID_HANDLE;
      } else {
        /* set param */
        concealParams->numFadeInFrames = fadeInSlope;
      }
    } else {
      return AAC_DEC_SET_PARAM_FAIL;
    }
  }

  /* set number of error-free frames after which the muting will be released */
  if (muteRelease != AACDEC_CONCEAL_PARAM_NOT_SPECIFIED) {
    if ( (muteRelease < (CONCEAL_MAX_NUM_FADE_FACTORS<<1))
      && (muteRelease >= 0) )
    {
      if (concealParams == NULL) {
        return AAC_DEC_INVALID_HANDLE;
      } else {
        /* set param */
        concealParams->numMuteReleaseFrames = muteRelease;
      }
    } else {
      return AAC_DEC_SET_PARAM_FAIL;
    }
  }

  /* set confort noise level which will be inserted while in state 'muting' */
  if (comfNoiseLevel != AACDEC_CONCEAL_PARAM_NOT_SPECIFIED) {
    if ( (comfNoiseLevel < -1)
      || (comfNoiseLevel > 127) ) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (concealParams == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    } else {
      concealParams->comfortNoiseLevel = comfNoiseLevel;
    }
  }

  return (AAC_DEC_OK);
}


/*!
  \brief Set fade-out/in attenuation factor vectors

  \concealParams
  \fadeOutAttenuationVector
  \fadeInAttenuationVector

  \return 0 if OK all other values indicate errors
*/
AAC_DECODER_ERROR
  CConcealment_SetAttenuation (
    CConcealParams *concealParams,
    SHORT *fadeOutAttenuationVector,
    SHORT *fadeInAttenuationVector )
{
  if ( (fadeOutAttenuationVector == NULL)
    && (fadeInAttenuationVector  == NULL) ) {
    return AAC_DEC_SET_PARAM_FAIL;
  }

  /* Fade-out factors */
  if (fadeOutAttenuationVector != NULL)
  {
    int i;

    /* check quantized factors first */
    for (i = 0; i < CONCEAL_MAX_NUM_FADE_FACTORS; i++) {
      if ((fadeOutAttenuationVector[i] < 0) || (fadeOutAttenuationVector[i] > CONCEAL_MAX_QUANT_FACTOR)) {
        return AAC_DEC_SET_PARAM_FAIL;
      }
    }
    if (concealParams == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }

    /* now dequantize factors */
    for (i = 0; i < CONCEAL_MAX_NUM_FADE_FACTORS; i++) 
    {
      concealParams->fadeOutFactor[i] =
        FX_DBL2FX_SGL( fLdPow(    CONCEAL_MIN_ATTENUATION_FACTOR_025_LD,
                                  0,
                                  (FIXP_DBL)((INT)(FL2FXCONST_DBL(1.0/2.0)>>(CONCEAL_PARAMETER_BITS-1)) * (INT)fadeOutAttenuationVector[i]),
                                  CONCEAL_PARAMETER_BITS
                                  )
                     );
    }
  }

  /* Fade-in factors */
  if (fadeInAttenuationVector != NULL)
  {
    int i;

    /* check quantized factors first */
    for (i = 0; i < CONCEAL_MAX_NUM_FADE_FACTORS; i++) {
      if ((fadeInAttenuationVector[i] < 0) || (fadeInAttenuationVector[i] > CONCEAL_MAX_QUANT_FACTOR)) {
        return AAC_DEC_SET_PARAM_FAIL;
      }
    }
    if (concealParams == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }

    /* now dequantize factors */
    for (i = 0; i < CONCEAL_MAX_NUM_FADE_FACTORS; i++)
    {
      concealParams->fadeInFactor[i] =
        FX_DBL2FX_SGL( fLdPow( CONCEAL_MIN_ATTENUATION_FACTOR_025_LD,
                               0,
                             (FIXP_DBL)((INT)(FIXP_ONE>>CONCEAL_PARAMETER_BITS) * (INT)fadeInAttenuationVector[i]),
                             CONCEAL_PARAMETER_BITS
                             )
                     );
    }
  }

  return (AAC_DEC_OK);
}


/*!
  \brief Get state of concealment module.

  \pConcealChannelInfo

  \return Concealment state.
*/
CConcealmentState
  CConcealment_GetState (
    CConcealmentInfo *pConcealChannelInfo
  )
{
  CConcealmentState state = ConcealState_Ok;

  if (pConcealChannelInfo != NULL) {
    state = pConcealChannelInfo->concealState;
  }

  return (state);
}


static void CConcealment_fakePnsData (
   CPnsData *pPnsData,
   CIcsInfo *pIcsInfo,
   const SamplingRateInfo *pSamplingRateInfo,
   SHORT *pSpecScale,
   SHORT *pScaleFactor,
   const int level )
{
  CPnsInterChannelData *pInterChannelData = pPnsData->pPnsInterChannelData;

  int  pnsBand, band, group, win;
  //int  delta = 0;
  int  windowsPerFrame = GetWindowsPerFrame(pIcsInfo);
  int  refLevel = (windowsPerFrame > 1) ? 82 : 91;

  FDK_ASSERT(level >= 0 && level <= 127);

  for (win = 0; win < windowsPerFrame; win++) {
    pSpecScale[win] = 31;
  }

  /* fake ICS info if necessary */
  if (!IsValid(pIcsInfo)) {
    pIcsInfo->WindowGroups = 1;
    if (IsLongBlock(pIcsInfo)) {
      pIcsInfo->TotalSfBands = pSamplingRateInfo->NumberOfScaleFactorBands_Long;
      pIcsInfo->WindowGroupLength[0] = 1;
    }
    else {
      pIcsInfo->TotalSfBands = pSamplingRateInfo->NumberOfScaleFactorBands_Short;
      pIcsInfo->WindowGroupLength[0] = 8;
    }
    pIcsInfo->MaxSfBands = pIcsInfo->TotalSfBands;
  }

  /* global activate PNS */
  pPnsData->PnsActive = 1;
  /* set energy level */
  pPnsData->CurrentEnergy = fixMax( 0, refLevel - level );

  /*
    value: | Avg. RMS power | Avg. RMS power |
           | specScale = 22 | specScale = 31 |
    -------+----------------+----------------+
        5  |                |  -99.0 dB
       15  |                |  -90.0 dB
       25  |                |  -89.7 dB 
       35  |                |  -85.3 dB
      ...  |    ...         |   ...
       45  |  -69.9 dB      |  -70.0 dB
       50  |  -62.2 dB      |  
       55  |  -55.6 dB      |  -54.6 dB
       60  |  -47.0 dB      |
       65  |  -39.5 dB      |  -39.5 dB
       70  |  -31.9 dB      |  
       75  |  -24.4 dB      |  -24.4 dB
       80  |  -16.9 dB      |  
       85  |   -9.4 dB (c)  |   -9.4 dB
       90  |   -3.9 dB (c)  |  
       95  |                |   -2.1 dB
      100  |                |   -1.6 dB
      105  |                |   -1.4 dB
  */

  for (group=0; group < GetWindowGroups(pIcsInfo); group++)
  {
    for (band=0; band < GetScaleFactorBandsTransmitted(pIcsInfo); band++)
    {
      pnsBand = group * 16 + band;

      if (pnsBand >= NO_OFBANDS) {
        return;
      }
      //pPnsData->CurrentEnergy += delta ;
      pScaleFactor[pnsBand] = pPnsData->CurrentEnergy;
      pInterChannelData->correlated[pnsBand] = 0;
      pPnsData->pnsUsed[pnsBand] = 1;
    }
  }
}


/*!
  \brief Store data for concealment techniques applied later

  Interface function to store data for different concealment strategies

   \return  none
 */
void
  CConcealment_Store (
    CConcealmentInfo *hConcealmentInfo,
    CAacDecoderChannelInfo *pAacDecoderChannelInfo,
    CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo )
{
  if ( !(pAacDecoderChannelInfo->renderMode == AACDEC_RENDER_LPD
      ) )
  {
    FIXP_DBL *pSpectralCoefficient =  SPEC_LONG(pAacDecoderChannelInfo->pSpectralCoefficient);
    SHORT    *pSpecScale           =  pAacDecoderChannelInfo->specScale;
    CIcsInfo *pIcsInfo             = &pAacDecoderChannelInfo->icsInfo;

    SHORT  tSpecScale[8];
    UCHAR  tWindowShape, tWindowSequence;

    /* store old window infos for swapping */
    tWindowSequence = hConcealmentInfo->windowSequence;
    tWindowShape    = hConcealmentInfo->windowShape;

    /* store old scale factors for swapping */
    FDKmemcpy(tSpecScale, hConcealmentInfo->specScale, 8*sizeof(SHORT));

    /* store new window infos */
    hConcealmentInfo->windowSequence = GetWindowSequence(pIcsInfo);
    hConcealmentInfo->windowShape    = GetWindowShape(pIcsInfo);
    hConcealmentInfo->lastWinGrpLen  = *(GetWindowGroupLengthTable(pIcsInfo)+GetWindowGroups(pIcsInfo)-1);

    /* store new scale factors */
    FDKmemcpy(hConcealmentInfo->specScale, pSpecScale, 8*sizeof(SHORT));

    if (CConcealment_GetDelay(hConcealmentInfo->pConcealParams) == 0)
    {
      /* store new spectral bins */
#if (CNCL_FRACT_BITS == DFRACT_BITS)
      FDKmemcpy(hConcealmentInfo->spectralCoefficient, pSpectralCoefficient, 1024 * sizeof(FIXP_CNCL));
#else
      FIXP_CNCL *RESTRICT pCncl = &hConcealmentInfo->spectralCoefficient[1024-1];
      FIXP_DBL  *RESTRICT pSpec = &pSpectralCoefficient[1024-1];
      int i;

      for (i = 1024; i != 0; i--) {
        *pCncl-- = FX_DBL2FX_CNCL(*pSpec--);
      }
#endif
    }
    else
    {
      FIXP_CNCL *RESTRICT pCncl = &hConcealmentInfo->spectralCoefficient[1024-1];
      FIXP_DBL  *RESTRICT pSpec = &pSpectralCoefficient[1024-1];
      int i;

      /* swap spectral data */
      for (i = 1024; i != 0; i--) {
        FIXP_DBL tSpec = *pSpec;
        *pSpec-- = FX_CNCL2FX_DBL(*pCncl);
        *pCncl-- = FX_DBL2FX_CNCL( tSpec);
      }

      /* complete swapping of window infos */
      pIcsInfo->WindowSequence = tWindowSequence;
      pIcsInfo->WindowShape    = tWindowShape;

      /* complete swapping of scale factors */
      FDKmemcpy(pSpecScale, tSpecScale, 8*sizeof(SHORT));
    }
  }
  
}


/*!
  \brief Apply concealment

  Interface function to different concealment strategies

   \return  none
 */
int
  CConcealment_Apply (
    CConcealmentInfo *hConcealmentInfo,
    CAacDecoderChannelInfo *pAacDecoderChannelInfo,
    CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
    const SamplingRateInfo *pSamplingRateInfo,
    const int samplesPerFrame,
    const UCHAR lastLpdMode,
    const int frameOk,
    const UINT flags)
{
  int appliedProcessing = 0;

  if ( (frameOk == 0)
    && (pAacDecoderChannelInfo->renderMode != (AACDEC_RENDER_MODE)hConcealmentInfo->lastRenderMode) ) {
    /* restore the last render mode to stay in the same domain which allows to do a proper concealment */
    pAacDecoderChannelInfo->renderMode = (AACDEC_RENDER_MODE)hConcealmentInfo->lastRenderMode;
  } else {
    /* otherwise store the current mode */
    hConcealmentInfo->lastRenderMode = (SCHAR)pAacDecoderChannelInfo->renderMode;
  }

  if ( frameOk )
  {
    /* Rescue current data for concealment in future frames */
    CConcealment_Store ( hConcealmentInfo,
                         pAacDecoderChannelInfo,
                         pAacDecoderStaticChannelInfo );
    /* Reset index to random sign vector to make sign calculation frame agnostic 
       (only depends on number of subsequently concealed spectral blocks) */
        hConcealmentInfo->iRandomPhase = 0;
  }

  /* hand current frame status to the state machine */
  CConcealment_UpdateState( hConcealmentInfo,
                            frameOk );

  {
    /* Create data for signal rendering according to the selected concealment method and decoder operating mode. */


    if ( !(pAacDecoderChannelInfo->renderMode == AACDEC_RENDER_LPD
        )
        )
    {
      switch (hConcealmentInfo->pConcealParams->method)
      {
      default:
      case ConcealMethodMute:
        if (!frameOk) {
          /* Mute spectral data in case of errors */
          FDKmemclear(pAacDecoderChannelInfo->pSpectralCoefficient, samplesPerFrame * sizeof(FIXP_DBL));
          /* Set last window shape */
          pAacDecoderChannelInfo->icsInfo.WindowShape = hConcealmentInfo->windowShape;
          appliedProcessing = 1;
        }
        break;

      case ConcealMethodNoise:
        /* Noise substitution error concealment technique */
        appliedProcessing =
          CConcealment_ApplyNoise (hConcealmentInfo,
                                   pAacDecoderChannelInfo,
                                   pAacDecoderStaticChannelInfo,
                                   pSamplingRateInfo,
                                   samplesPerFrame,
                                   flags);
        break;

      case ConcealMethodInter:
        /* Energy interpolation concealment based on 3GPP */
        appliedProcessing =
          CConcealment_ApplyInter (hConcealmentInfo,
                                   pAacDecoderChannelInfo,
                                   pSamplingRateInfo,
                                   samplesPerFrame,
                                   0,  /* don't use tonal improvement */
                                   frameOk);
        break;

      }
    }
  }
  /* update history */
  hConcealmentInfo->prevFrameOk[0] = hConcealmentInfo->prevFrameOk[1];
  hConcealmentInfo->prevFrameOk[1] = frameOk;

  return appliedProcessing;
}

/*!
\brief Apply concealment noise substitution

  In case of frame lost this function produces a noisy frame with respect to the
  energies values of past frame.

\return  none
 */
static int
  CConcealment_ApplyNoise (CConcealmentInfo *pConcealmentInfo,
                           CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                           CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
                           const SamplingRateInfo *pSamplingRateInfo,
                           const int samplesPerFrame,
                           const UINT flags)
{
  CConcealParams *pConcealCommonData = pConcealmentInfo->pConcealParams;

  FIXP_DBL *pSpectralCoefficient =  SPEC_LONG(pAacDecoderChannelInfo->pSpectralCoefficient);
  SHORT    *pSpecScale           =  pAacDecoderChannelInfo->specScale;
  CIcsInfo *pIcsInfo             = &pAacDecoderChannelInfo->icsInfo;

  int appliedProcessing = 0;

  FDK_ASSERT((samplesPerFrame>=480) && (samplesPerFrame<=1024));
  FDK_ASSERT((samplesPerFrame&0x1F) == 0);

  switch (pConcealmentInfo->concealState)
  {
  case ConcealState_Ok:
    /* Nothing to do here! */
    break;

  case ConcealState_Single:
  case ConcealState_FadeOut:
    {
      /* restore frequency coefficients from buffer with a specific muting */
      FIXP_SGL  fac;
      int win, numWindows = 1;
      int windowLen = samplesPerFrame;
      int tFadeFrames, lastWindow = 0;
      int win_idx_stride = 1;

      FDK_ASSERT(pConcealmentInfo != NULL);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames >= 0);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames < CONCEAL_MAX_NUM_FADE_FACTORS);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames <= pConcealCommonData->numFadeOutFrames);

      /* get attenuation factor */
      tFadeFrames = pConcealmentInfo->cntFadeFrames;
      fac = pConcealCommonData->fadeOutFactor[tFadeFrames];

      /* set old window parameters */
      {
        pIcsInfo->WindowShape    = pConcealmentInfo->windowShape;
        pIcsInfo->WindowSequence = pConcealmentInfo->windowSequence;

        if (pConcealmentInfo->windowSequence == 2) {
          /* short block handling */
          numWindows = 8;
          windowLen  = samplesPerFrame >> 3;
          lastWindow = numWindows - pConcealmentInfo->lastWinGrpLen;
        }
      }

      for (win = 0; win < numWindows; win++) {
        FIXP_CNCL *pCncl = pConcealmentInfo->spectralCoefficient + (lastWindow * windowLen);
        FIXP_DBL  *pOut  = pSpectralCoefficient + (win * windowLen);
        int i;

        FDK_ASSERT((lastWindow * windowLen + windowLen) <= samplesPerFrame);

        /* restore frequency coefficients from buffer with a specific attenuation */
        for (i = 0; i < windowLen; i++) {
          pOut[i] = fMult(pCncl[i], fac);
        }

        /* apply random change of sign for spectral coefficients */
        CConcealment_ApplyRandomSign(pConcealmentInfo->iRandomPhase,
                                            pOut,
                                            windowLen );

        /* Increment random phase index to avoid repetition artifacts. */
        pConcealmentInfo->iRandomPhase = (pConcealmentInfo->iRandomPhase + 1) & (AAC_NF_NO_RANDOM_VAL - 1);

        /* set old scale factors */
        pSpecScale[win*win_idx_stride] = pConcealmentInfo->specScale[win_idx_stride*lastWindow++];

        if ( (lastWindow >= numWindows)
          && (numWindows >  1) )
        {
          /* end of sequence -> rewind */
          lastWindow = numWindows - pConcealmentInfo->lastWinGrpLen;
          /* update the attenuation factor to get a faster fade-out */
          tFadeFrames += 1;
          if (tFadeFrames < pConcealCommonData->numFadeOutFrames) {
            fac = pConcealCommonData->fadeOutFactor[tFadeFrames];
          } else {
            fac = (FIXP_SGL)0;
          }
        }
      }

      /* store temp vars */
      pConcealmentInfo->cntFadeFrames = tFadeFrames;
      appliedProcessing = 1;
    }
    break;

  case ConcealState_Mute:
    {
      /* set dummy window parameters */
      pIcsInfo->Valid          = 0;                                /* Trigger the generation of a consitent IcsInfo */
      pIcsInfo->WindowShape    = pConcealmentInfo->windowShape;    /* Prevent an invalid WindowShape (required for F/T transform) */
      pIcsInfo->WindowSequence = CConcealment_GetWinSeq(pConcealmentInfo->windowSequence);
      pConcealmentInfo->windowSequence = pIcsInfo->WindowSequence; /* Store for next frame (spectrum in concealment buffer can't be used at all) */

      /* mute spectral data */
      FDKmemclear(pSpectralCoefficient, samplesPerFrame * sizeof(FIXP_DBL));

      if ( !(flags & (AC_USAC|AC_RSVD50)) 
           && pConcealCommonData->comfortNoiseLevel >= 0
           && pConcealCommonData->comfortNoiseLevel <= 61 /* -90dB */)
        {
        /* insert comfort noise using PNS */
        CConcealment_fakePnsData (
         &pAacDecoderChannelInfo->data.aac.PnsData,
          pIcsInfo,
          pSamplingRateInfo,
          pAacDecoderChannelInfo->pDynData->aSfbScale,
          pAacDecoderChannelInfo->pDynData->aScaleFactor,
          pConcealCommonData->comfortNoiseLevel
        );

        CPns_Apply (
               &pAacDecoderChannelInfo->data.aac.PnsData,
                pIcsInfo,
                pAacDecoderChannelInfo->pSpectralCoefficient,
                pAacDecoderChannelInfo->specScale,
                pAacDecoderChannelInfo->pDynData->aScaleFactor,
                pSamplingRateInfo,
                pAacDecoderChannelInfo->granuleLength,
                0  /* always apply to first channel */
              );
      }
      appliedProcessing = 1;
    }
    break;

  case ConcealState_FadeIn:
    {
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames >= 0);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames < CONCEAL_MAX_NUM_FADE_FACTORS);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames < pConcealCommonData->numFadeInFrames);

      /* attenuate signal to get a smooth fade-in */
      FIXP_DBL *RESTRICT pOut = &pSpectralCoefficient[samplesPerFrame-1];
      FIXP_SGL fac = pConcealCommonData->fadeInFactor[pConcealmentInfo->cntFadeFrames];
      int i;

      for (i = samplesPerFrame; i != 0; i--) {
        *pOut = fMult(*pOut, fac);
        pOut--;
      }
      appliedProcessing = 1;
    }
    break;

  default:
    /* we shouldn't come here anyway */
    FDK_ASSERT(0);
    break;
  }

  return appliedProcessing;
}


/*!
  \brief Apply concealment interpolation

  The function swaps the data from the current and the previous frame. If an
  error has occured, frame interpolation is performed to restore the missing
  frame. In case of multiple faulty frames, fade-in and fade-out is applied.

  \return  none
*/
static int
  CConcealment_ApplyInter (
    CConcealmentInfo       *pConcealmentInfo,
    CAacDecoderChannelInfo *pAacDecoderChannelInfo,
    const SamplingRateInfo *pSamplingRateInfo,
    const int  samplesPerFrame,
    const int  improveTonal,
    const int  frameOk )
{
  CConcealParams   *pConcealCommonData    =  pConcealmentInfo->pConcealParams;

  FIXP_DBL         *pSpectralCoefficient  =  SPEC_LONG(pAacDecoderChannelInfo->pSpectralCoefficient);
  CIcsInfo         *pIcsInfo              = &pAacDecoderChannelInfo->icsInfo;
  SHORT            *pSpecScale            =  pAacDecoderChannelInfo->specScale;


  int sfbEnergyPrev[64];
  int sfbEnergyAct [64];

  int i, appliedProcessing = 0;

  /* clear/init */
  FDKmemclear(sfbEnergyPrev, 64 * sizeof(int));
  FDKmemclear(sfbEnergyAct,  64 * sizeof(int));


  if (!frameOk)
  {
    /* Restore last frame from concealment buffer */
    pIcsInfo->WindowShape    = pConcealmentInfo->windowShape;
    pIcsInfo->WindowSequence = pConcealmentInfo->windowSequence;

    /* Restore spectral data */
    for (i = 0; i < samplesPerFrame; i++) {
      pSpectralCoefficient[i] = FX_CNCL2FX_DBL(pConcealmentInfo->spectralCoefficient[i]);
    }

    /* Restore scale factors */
    FDKmemcpy(pSpecScale, pConcealmentInfo->specScale, 8*sizeof(SHORT));
  }

  /* if previous frame was not ok */
  if (!pConcealmentInfo->prevFrameOk[1]) {

    /* if current frame (f_n) is ok and the last but one frame (f_(n-2))
       was ok, too, then interpolate both frames in order to generate
       the current output frame (f_(n-1)). Otherwise, use the last stored
       frame (f_(n-2) or f_(n-3) or ...). */
    if (frameOk && pConcealmentInfo->prevFrameOk[0])
    {
      appliedProcessing = 1;


      /* Interpolate both frames in order to generate the current output frame (f_(n-1)). */
      if (pIcsInfo->WindowSequence == EightShortSequence) {
        /* f_(n-2) == EightShortSequence */
        /* short--??????--short, short--??????--long interpolation */
        /* short--short---short, short---long---long interpolation */

        int wnd;

        if (pConcealmentInfo->windowSequence == EightShortSequence) { /* f_n == EightShortSequence */
          /* short--short---short interpolation */

          int scaleFactorBandsTotal = pSamplingRateInfo->NumberOfScaleFactorBands_Short;
          const SHORT *pSfbOffset   = pSamplingRateInfo->ScaleFactorBands_Short;
          pIcsInfo->WindowShape = 1;
          pIcsInfo->WindowSequence = EightShortSequence;

          for (wnd = 0; wnd < 8; wnd++)
          {
            CConcealment_CalcBandEnergy(
              &pSpectralCoefficient[wnd * (samplesPerFrame / 8)], /* spec_(n-2) */
               pSamplingRateInfo,
               EightShortSequence,
               CConcealment_NoExpand,
               sfbEnergyPrev);

            CConcealment_CalcBandEnergy(
              &pConcealmentInfo->spectralCoefficient[wnd * (samplesPerFrame / 8)], /* spec_n */
               pSamplingRateInfo,
               EightShortSequence,
               CConcealment_NoExpand,
               sfbEnergyAct);

            CConcealment_InterpolateBuffer(
              &pSpectralCoefficient[wnd * (samplesPerFrame / 8)], /* spec_(n-1) */
              &pSpecScale[wnd],
              &pConcealmentInfo->specScale[wnd],
              &pSpecScale[wnd],
               sfbEnergyPrev,
               sfbEnergyAct,
               scaleFactorBandsTotal,
               pSfbOffset);

          }
        } else { /* f_n != EightShortSequence */
          /* short---long---long interpolation */

          int scaleFactorBandsTotal = pSamplingRateInfo->NumberOfScaleFactorBands_Long;
          const SHORT *pSfbOffset   = pSamplingRateInfo->ScaleFactorBands_Long;
          SHORT specScaleOut;

          CConcealment_CalcBandEnergy(&pSpectralCoefficient[samplesPerFrame - (samplesPerFrame / 8)], /* [wnd] spec_(n-2) */
                                      pSamplingRateInfo,
                                      EightShortSequence,
                                      CConcealment_Expand,
                                      sfbEnergyAct);

          CConcealment_CalcBandEnergy(pConcealmentInfo->spectralCoefficient, /* spec_n */
                                      pSamplingRateInfo,
                                      OnlyLongSequence,
                                      CConcealment_NoExpand,
                                      sfbEnergyPrev);

          pIcsInfo->WindowShape = 0;
          pIcsInfo->WindowSequence = LongStopSequence;

          for (i = 0; i < samplesPerFrame ; i++) {
            pSpectralCoefficient[i] = pConcealmentInfo->spectralCoefficient[i]; /* spec_n */
          }

          for (i = 0; i < 8; i++) { /* search for max(specScale) */
            if (pSpecScale[i] > pSpecScale[0]) {
              pSpecScale[0] = pSpecScale[i];
            }
          }

          CConcealment_InterpolateBuffer(
            pSpectralCoefficient, /* spec_(n-1) */
           &pConcealmentInfo->specScale[0],
           &pSpecScale[0],
           &specScaleOut,
            sfbEnergyPrev,
            sfbEnergyAct,
            scaleFactorBandsTotal,
            pSfbOffset);

          pSpecScale[0] = specScaleOut;
        }
      } else {
        /* long--??????--short, long--??????--long interpolation */
        /* long---long---short, long---long---long interpolation */

        int scaleFactorBandsTotal = pSamplingRateInfo->NumberOfScaleFactorBands_Long;
        const SHORT *pSfbOffset   = pSamplingRateInfo->ScaleFactorBands_Long;
        SHORT specScaleAct        = pConcealmentInfo->specScale[0];

        CConcealment_CalcBandEnergy(pSpectralCoefficient,  /* spec_(n-2) */
                                    pSamplingRateInfo,
                                    OnlyLongSequence,
                                    CConcealment_NoExpand,
                                    sfbEnergyPrev);

        if (pConcealmentInfo->windowSequence == EightShortSequence) {  /* f_n == EightShortSequence */
          /* long---long---short interpolation */

          pIcsInfo->WindowShape = 1;
          pIcsInfo->WindowSequence = LongStartSequence;

          for (i = 1; i < 8; i++) { /* search for max(specScale) */
            if (pConcealmentInfo->specScale[i] > specScaleAct) {
              specScaleAct = pConcealmentInfo->specScale[i];
            }
          }

          /* Expand first short spectrum */
          CConcealment_CalcBandEnergy(pConcealmentInfo->spectralCoefficient,  /* spec_n */
                                      pSamplingRateInfo,
                                      EightShortSequence,
                                      CConcealment_Expand,  /* !!! */
                                      sfbEnergyAct);
        } else {
          /* long---long---long interpolation */

          pIcsInfo->WindowShape = 0;
          pIcsInfo->WindowSequence = OnlyLongSequence;

          CConcealment_CalcBandEnergy(pConcealmentInfo->spectralCoefficient,  /* spec_n */
                                      pSamplingRateInfo,
                                      OnlyLongSequence,
                                      CConcealment_NoExpand,
                                      sfbEnergyAct);
        }

          CConcealment_InterpolateBuffer(
            pSpectralCoefficient,  /* spec_(n-1) */
           &pSpecScale[0],
           &specScaleAct,
           &pSpecScale[0],
            sfbEnergyPrev,
            sfbEnergyAct,
            scaleFactorBandsTotal,
            pSfbOffset);

      }
    }

      /* Noise substitution of sign of the output spectral coefficients */
      CConcealment_ApplyRandomSign (pConcealmentInfo->iRandomPhase,
                                    pSpectralCoefficient,
                                    samplesPerFrame);
      /* Increment random phase index to avoid repetition artifacts. */
      pConcealmentInfo->iRandomPhase = (pConcealmentInfo->iRandomPhase + 1) & (AAC_NF_NO_RANDOM_VAL - 1);
  }

  /* scale spectrum according to concealment state */
  switch (pConcealmentInfo->concealState)
  {
  case ConcealState_Single:
    appliedProcessing = 1;
    break;

  case ConcealState_FadeOut:
    {
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames >= 0);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames < CONCEAL_MAX_NUM_FADE_FACTORS);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames < pConcealCommonData->numFadeOutFrames);

      /* restore frequency coefficients from buffer with a specific muting */
      FIXP_DBL *RESTRICT pOut = &pSpectralCoefficient[samplesPerFrame-1];
      FIXP_SGL fac = pConcealCommonData->fadeOutFactor[pConcealmentInfo->cntFadeFrames];

      for (i = samplesPerFrame; i != 0; i--) {
        *pOut = fMult(*pOut, fac);
        pOut--;
      }
      appliedProcessing = 1;
    }
    break;

  case ConcealState_FadeIn:
    {
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames >= 0);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames < CONCEAL_MAX_NUM_FADE_FACTORS);
      FDK_ASSERT(pConcealmentInfo->cntFadeFrames < pConcealCommonData->numFadeInFrames);

      /* attenuate signal to get a smooth fade-in */
      FIXP_DBL *RESTRICT pOut = &pSpectralCoefficient[samplesPerFrame-1];
      FIXP_SGL fac = pConcealCommonData->fadeInFactor[pConcealmentInfo->cntFadeFrames];

      for (i = samplesPerFrame; i != 0; i--) {
        *pOut = fMult(*pOut, fac);
        pOut--;
      }
      appliedProcessing = 1;
    }
    break;

  case ConcealState_Mute:
    {
      int fac = pConcealCommonData->comfortNoiseLevel;

      /* set dummy window parameters */
      pIcsInfo->Valid          = 0;                                /* Trigger the generation of a consitent IcsInfo */
      pIcsInfo->WindowShape    = pConcealmentInfo->windowShape;    /* Prevent an invalid WindowShape (required for F/T transform) */
      pIcsInfo->WindowSequence = CConcealment_GetWinSeq(pConcealmentInfo->windowSequence);
      pConcealmentInfo->windowSequence = pIcsInfo->WindowSequence; /* Store for next frame (spectrum in concealment buffer can't be used at all) */

      /* mute spectral data */
      FDKmemclear(pSpectralCoefficient, samplesPerFrame * sizeof(FIXP_DBL));

      if (fac >= 0 && fac <= 61) {
        /* insert comfort noise using PNS */
        CConcealment_fakePnsData (
         &pAacDecoderChannelInfo->data.aac.PnsData,
          pIcsInfo,
          pSamplingRateInfo,
          pAacDecoderChannelInfo->specScale,
          pAacDecoderChannelInfo->pDynData->aScaleFactor,
          fac
        );

        CPns_Apply (
               &pAacDecoderChannelInfo->data.aac.PnsData,
                pIcsInfo,
                pAacDecoderChannelInfo->pSpectralCoefficient,
                pAacDecoderChannelInfo->specScale,
                pAacDecoderChannelInfo->pDynData->aScaleFactor,
                pSamplingRateInfo,
                pAacDecoderChannelInfo->granuleLength,
                0  /* always apply to first channel */
              );
      }
      appliedProcessing = 1;
    }
    break;

  default:
    /* nothing to do here */
    break;
  }

  return appliedProcessing;
}


/*!
  \brief Calculate the spectral energy

  The function calculates band-wise the spectral energy. This is used for
  frame interpolation.

  \return  none
*/
static void
  CConcealment_CalcBandEnergy (
    FIXP_DBL               *spectrum,
    const SamplingRateInfo *pSamplingRateInfo,
    const int               blockType,
    CConcealmentExpandType  expandType,
    int                    *sfbEnergy )
{
  const SHORT *pSfbOffset;
  int line, sfb, scaleFactorBandsTotal = 0;
  
  /* In the following calculations, enAccu is initialized with LSB-value in order to avoid zero energy-level */

  line = 0;

  switch(blockType) {

  case OnlyLongSequence:
  case LongStartSequence:
  case LongStopSequence:

    if (expandType == CConcealment_NoExpand) {
      /* standard long calculation */
      scaleFactorBandsTotal = pSamplingRateInfo->NumberOfScaleFactorBands_Long;
      pSfbOffset            = pSamplingRateInfo->ScaleFactorBands_Long;

      for (sfb = 0; sfb < scaleFactorBandsTotal; sfb++) {
        FIXP_DBL enAccu = (FIXP_DBL)(LONG)1;
        int sfbScale = (sizeof(LONG)<<3) - CntLeadingZeros(pSfbOffset[sfb+1] - pSfbOffset[sfb]) - 1;
        /* scaling depends on sfb width. */
        for ( ; line < pSfbOffset[sfb+1]; line++) {
          enAccu += fPow2Div2(*(spectrum + line)) >> sfbScale;
        }
        *(sfbEnergy + sfb) = CntLeadingZeros(enAccu) - 1;
      }
    }
    else {
      /* compress long to short */
      scaleFactorBandsTotal = pSamplingRateInfo->NumberOfScaleFactorBands_Short;
      pSfbOffset            = pSamplingRateInfo->ScaleFactorBands_Short;

      for (sfb = 0; sfb < scaleFactorBandsTotal; sfb++) {
        FIXP_DBL enAccu = (FIXP_DBL)(LONG)1;
        int sfbScale = (sizeof(LONG)<<3) - CntLeadingZeros(pSfbOffset[sfb+1] - pSfbOffset[sfb]) - 1;
        /* scaling depends on sfb width. */
        for (; line < pSfbOffset[sfb+1] << 3; line++) {
          enAccu += (enAccu + (fPow2Div2(*(spectrum + line)) >> sfbScale)) >> 3;
        }
        *(sfbEnergy + sfb) = CntLeadingZeros(enAccu) - 1;
      }
    }
    break;

  case EightShortSequence:

    if (expandType == CConcealment_NoExpand) {
      /*   standard short calculation */
      scaleFactorBandsTotal = pSamplingRateInfo->NumberOfScaleFactorBands_Short;
      pSfbOffset            = pSamplingRateInfo->ScaleFactorBands_Short;

      for (sfb = 0; sfb < scaleFactorBandsTotal; sfb++) {
        FIXP_DBL enAccu = (FIXP_DBL)(LONG)1;
        int sfbScale = (sizeof(LONG)<<3) - CntLeadingZeros(pSfbOffset[sfb+1] - pSfbOffset[sfb]) - 1;
        /* scaling depends on sfb width. */
        for ( ; line < pSfbOffset[sfb+1]; line++) {
          enAccu += fPow2Div2(*(spectrum + line)) >> sfbScale;
        }
        *(sfbEnergy + sfb) = CntLeadingZeros(enAccu) - 1;
      }
    }
    else {
      /*  expand short to long spectrum */
      scaleFactorBandsTotal = pSamplingRateInfo->NumberOfScaleFactorBands_Long;
      pSfbOffset            = pSamplingRateInfo->ScaleFactorBands_Long;

      for (sfb = 0; sfb < scaleFactorBandsTotal; sfb++) {
        FIXP_DBL enAccu = (FIXP_DBL)(LONG)1;
        int sfbScale = (sizeof(LONG)<<3) - CntLeadingZeros(pSfbOffset[sfb+1] - pSfbOffset[sfb]) - 1;
        /* scaling depends on sfb width. */
        for ( ; line < pSfbOffset[sfb+1]; line++) {
          enAccu += fPow2Div2(*(spectrum + (line >> 3))) >> sfbScale;
        }
        *(sfbEnergy + sfb) = CntLeadingZeros(enAccu) - 1;
      }
    }
    break;
  }
}


/*!
  \brief Interpolate buffer

  The function creates the interpolated spectral data according to the
  energy of the last good frame and the current (good) frame.

  \return  none
*/
static void
  CConcealment_InterpolateBuffer (
    FIXP_DBL    *spectrum,
    SHORT       *pSpecScalePrv,
    SHORT       *pSpecScaleAct,
    SHORT       *pSpecScaleOut,
    int         *enPrv,
    int         *enAct,
    int          sfbCnt,
    const SHORT *pSfbOffset )
{
  int    sfb, line = 0;
  int    fac_shift;
  int    fac_mod;
  FIXP_DBL accu;

  for (sfb = 0; sfb < sfbCnt; sfb++) {

    fac_shift = enPrv[sfb] - enAct[sfb] + ((*pSpecScaleAct - *pSpecScalePrv) << 1);
    fac_mod   = fac_shift & 3;
    fac_shift = (fac_shift >> 2) + 1;
    fac_shift += *pSpecScalePrv - fixMax(*pSpecScalePrv, *pSpecScaleAct);

    for (; line < pSfbOffset[sfb+1]; line++) {
      accu = fMult(*(spectrum+line), facMod4Table[fac_mod]);
      if (fac_shift < 0) {
        accu >>= -fac_shift;
      } else {
        accu <<= fac_shift;
      }
      *(spectrum+line) = accu;
    }
  }
  *pSpecScaleOut = fixMax(*pSpecScalePrv, *pSpecScaleAct);
}




static INT findEquiFadeFrame (
    CConcealParams *pConcealCommonData,
    INT actFadeIndex,
    int direction )
{
  FIXP_SGL *pFactor;
  FIXP_SGL  referenceVal;
  FIXP_SGL  minDiff = (FIXP_SGL)MAXVAL_SGL;

  INT  numFrames = 0;
  INT  nextFadeIndex = 0;

  int  i;

  /* init depending on direction */
  if (direction == 0) {  /* FADE-OUT => FADE-IN */
    numFrames = pConcealCommonData->numFadeInFrames;
    referenceVal = pConcealCommonData->fadeOutFactor[actFadeIndex] >> 1;
    pFactor = pConcealCommonData->fadeInFactor;
  }
  else {  /* FADE-IN => FADE-OUT */
    numFrames = pConcealCommonData->numFadeOutFrames;
    referenceVal = pConcealCommonData->fadeInFactor[actFadeIndex] >> 1;
    pFactor = pConcealCommonData->fadeOutFactor;
  }

  /* search for minimum difference */
  for (i = 0; i < numFrames; i++) {
    FIXP_SGL diff = fixp_abs((pFactor[i]>>1) - referenceVal);
    if (diff < minDiff) {
      minDiff = diff;
      nextFadeIndex = i;
    }
  }

  /* check and adjust depending on direction */
  if (direction == 0) {  /* FADE-OUT => FADE-IN */
    if (((pFactor[nextFadeIndex]>>1) <= referenceVal) && (nextFadeIndex > 0)) {
      nextFadeIndex -= 1;
    }
  }
  else {  /* FADE-IN => FADE-OUT */
    if (((pFactor[nextFadeIndex]>>1) >= referenceVal) && (nextFadeIndex < numFrames-1)) {
      nextFadeIndex += 1;
    }
  }

  return (nextFadeIndex);
}


/*!
  \brief Update the concealment state

  The function updates the state of the concealment state-machine. The
  states are: mute, fade-in, fade-out, interpolate and frame-ok.

  \return  none
*/
static void
  CConcealment_UpdateState (
    CConcealmentInfo *pConcealmentInfo,
    int frameOk )
{
  CConcealParams *pConcealCommonData = pConcealmentInfo->pConcealParams;

  switch (pConcealCommonData->method)
  {
  case ConcealMethodNoise:
    {
      if (pConcealmentInfo->concealState != ConcealState_Ok) {
        /* count the valid frames during concealment process */
        if (frameOk) {
          pConcealmentInfo->cntValidFrames += 1;
        } else {
          pConcealmentInfo->cntValidFrames  = 0;
        }
      }

      /* -- STATE MACHINE for Noise Substitution -- */
      switch (pConcealmentInfo->concealState)
      {
      case ConcealState_Ok:
        if (!frameOk) {
          if (pConcealCommonData->numFadeOutFrames > 0) {
            /* change to state SINGLE-FRAME-LOSS */
            pConcealmentInfo->concealState   = ConcealState_Single;
          } else {
            /* change to state MUTE */
            pConcealmentInfo->concealState = ConcealState_Mute;
          }
          pConcealmentInfo->cntFadeFrames  = 0;
          pConcealmentInfo->cntValidFrames = 0;
        }
        break;

      case ConcealState_Single:  /* Just a pre-stage before fade-out begins. Stay here only one frame! */
        pConcealmentInfo->cntFadeFrames += 1;
        if (frameOk) {
          if (pConcealmentInfo->cntValidFrames > pConcealCommonData->numMuteReleaseFrames) {
            /* change to state FADE-IN */
            pConcealmentInfo->concealState  = ConcealState_FadeIn;
            pConcealmentInfo->cntFadeFrames = findEquiFadeFrame( pConcealCommonData,
                                                                 pConcealmentInfo->cntFadeFrames-1,
                                                                 0 /* FadeOut -> FadeIn */);
          } else {
            /* change to state OK */
            pConcealmentInfo->concealState = ConcealState_Ok;
          }
        } else {
          if (pConcealmentInfo->cntFadeFrames >= pConcealCommonData->numFadeOutFrames) {
            /* change to state MUTE */
            pConcealmentInfo->concealState = ConcealState_Mute;
          } else {
            /* change to state FADE-OUT */
            pConcealmentInfo->concealState = ConcealState_FadeOut;
          }
        }
        break;

      case ConcealState_FadeOut:
        pConcealmentInfo->cntFadeFrames += 1;  /* used to address the fade-out factors */
        if (pConcealmentInfo->cntValidFrames > pConcealCommonData->numMuteReleaseFrames) {
          if (pConcealCommonData->numFadeInFrames > 0) {
            /* change to state FADE-IN */
            pConcealmentInfo->concealState  = ConcealState_FadeIn;
            pConcealmentInfo->cntFadeFrames = findEquiFadeFrame( pConcealCommonData,
                                                                 pConcealmentInfo->cntFadeFrames-1,
                                                                 0 /* FadeOut -> FadeIn */);
          } else {
            /* change to state OK */
            pConcealmentInfo->concealState = ConcealState_Ok;
          }
        } else {
          if (pConcealmentInfo->cntFadeFrames >= pConcealCommonData->numFadeOutFrames) {
            /* change to state MUTE */
            pConcealmentInfo->concealState = ConcealState_Mute;
          }
        }
        break;

      case ConcealState_Mute:
        if (pConcealmentInfo->cntValidFrames > pConcealCommonData->numMuteReleaseFrames) {
          if (pConcealCommonData->numFadeInFrames > 0) {
            /* change to state FADE-IN */
            pConcealmentInfo->concealState = ConcealState_FadeIn;
            pConcealmentInfo->cntFadeFrames = pConcealCommonData->numFadeInFrames - 1;
          } else {
            /* change to state OK */
            pConcealmentInfo->concealState = ConcealState_Ok;
          }
        }
        break;

      case ConcealState_FadeIn:
        pConcealmentInfo->cntFadeFrames -= 1;  /* used to address the fade-in factors */
        if (frameOk) {
          if (pConcealmentInfo->cntFadeFrames < 0) {
            /* change to state OK */
            pConcealmentInfo->concealState = ConcealState_Ok;
          }
        } else {
          if (pConcealCommonData->numFadeOutFrames > 0) {
            /* change to state FADE-OUT */
            pConcealmentInfo->concealState  = ConcealState_FadeOut;
            pConcealmentInfo->cntFadeFrames = findEquiFadeFrame( pConcealCommonData,
                                                                 pConcealmentInfo->cntFadeFrames+1,
                                                                 1 /* FadeIn -> FadeOut */);
          } else {
            /* change to state MUTE */
            pConcealmentInfo->concealState = ConcealState_Mute;
          }
        }
        break;

      default:
        FDK_ASSERT(0);
        break;
      }
    }
    break;

  case ConcealMethodInter:
  case ConcealMethodTonal:
    {
      if (pConcealmentInfo->concealState != ConcealState_Ok) {
        /* count the valid frames during concealment process */
        if ( pConcealmentInfo->prevFrameOk[1] ||
            (pConcealmentInfo->prevFrameOk[0] && !pConcealmentInfo->prevFrameOk[1] && frameOk) ) {
          /* The frame is OK even if it can be estimated by the energy interpolation algorithm */
          pConcealmentInfo->cntValidFrames += 1;
        } else {
          pConcealmentInfo->cntValidFrames  = 0;
        }
      }

      /* -- STATE MACHINE for energy interpolation -- */
      switch (pConcealmentInfo->concealState)
      {
      case ConcealState_Ok:
        if (!(pConcealmentInfo->prevFrameOk[1] ||
             (pConcealmentInfo->prevFrameOk[0] && !pConcealmentInfo->prevFrameOk[1] && frameOk))) {
          if (pConcealCommonData->numFadeOutFrames > 0) {
            /* Fade out only if the energy interpolation algorithm can not be applied! */
            pConcealmentInfo->concealState   = ConcealState_FadeOut;
          } else {
            /* change to state MUTE */
            pConcealmentInfo->concealState = ConcealState_Mute;
          }
          pConcealmentInfo->cntFadeFrames  = 0;
          pConcealmentInfo->cntValidFrames = 0;
        }
        break;

      case ConcealState_Single:
        pConcealmentInfo->concealState = ConcealState_Ok;
        break;

      case ConcealState_FadeOut:
        pConcealmentInfo->cntFadeFrames += 1;

        if (pConcealmentInfo->cntValidFrames > pConcealCommonData->numMuteReleaseFrames) {
          if (pConcealCommonData->numFadeInFrames > 0) {
            /* change to state FADE-IN */
            pConcealmentInfo->concealState  = ConcealState_FadeIn;
            pConcealmentInfo->cntFadeFrames = findEquiFadeFrame( pConcealCommonData,
                                                                 pConcealmentInfo->cntFadeFrames-1,
                                                                 0 /* FadeOut -> FadeIn */);
          } else {
            /* change to state OK */
            pConcealmentInfo->concealState = ConcealState_Ok;
          }
        } else {
          if (pConcealmentInfo->cntFadeFrames >= pConcealCommonData->numFadeOutFrames) {
            /* change to state MUTE */
            pConcealmentInfo->concealState = ConcealState_Mute;
          }
        }
        break;

      case ConcealState_Mute:
        if (pConcealmentInfo->cntValidFrames > pConcealCommonData->numMuteReleaseFrames) {
          if (pConcealCommonData->numFadeInFrames > 0) {
            /* change to state FADE-IN */
            pConcealmentInfo->concealState = ConcealState_FadeIn;
            pConcealmentInfo->cntFadeFrames = pConcealCommonData->numFadeInFrames - 1;
          } else {
            /* change to state OK */
            pConcealmentInfo->concealState = ConcealState_Ok;
          }
        }
        break;

      case ConcealState_FadeIn:
        pConcealmentInfo->cntFadeFrames -= 1;  /* used to address the fade-in factors */

        if (frameOk || pConcealmentInfo->prevFrameOk[1]) {
          if (pConcealmentInfo->cntFadeFrames < 0) {
            /* change to state OK */
            pConcealmentInfo->concealState = ConcealState_Ok;
          }
        } else {
          if (pConcealCommonData->numFadeOutFrames > 0) {
            /* change to state FADE-OUT */
            pConcealmentInfo->concealState  = ConcealState_FadeOut;
            pConcealmentInfo->cntFadeFrames = findEquiFadeFrame( pConcealCommonData,
                                                                 pConcealmentInfo->cntFadeFrames+1,
                                                                 1 /* FadeIn -> FadeOut */);
          } else {
            /* change to state MUTE */
            pConcealmentInfo->concealState = ConcealState_Mute;
          }
        }
        break;
      } /* End switch(pConcealmentInfo->concealState) */
    }
    break;

  default:
    /* Don't need a state machine for other concealment methods. */
    break;
  }

}


/*!
\brief Randomizes the sign of the spectral data

  The function toggles the sign of the spectral data randomly. This is
  useful to ensure the quality of the concealed frames.

\return  none
 */
static
void CConcealment_ApplyRandomSign (int randomPhase,
                                   FIXP_DBL *spec,
                                   int samplesPerFrame
                                               )
{
  int i;
  USHORT packedSign=0;

  /* random table 512x16bit has been reduced to 512 packed sign bits = 32x16 bit */

  /* read current packed sign word */
  packedSign = randomSign[randomPhase>>4];
  packedSign >>= (randomPhase&0xf);

  for (i = 0; i < samplesPerFrame ; i++) {
    if ((randomPhase & 0xf) == 0) {
      packedSign = randomSign[randomPhase>>4];
    }

    if (packedSign & 0x1) {
      spec[i] = -spec[i];
    }
    packedSign >>= 1;

    randomPhase = (randomPhase + 1) & (AAC_NF_NO_RANDOM_VAL - 1);
  }
}


/*!
  \brief Get fadeing factor for current concealment state.

  The function returns the factor used for fading that belongs to the current internal state.

  \return Fade factor
 */
FIXP_DBL
  CConcealment_GetFadeFactor (
      CConcealmentInfo *hConcealmentInfo,
      const int fPreviousFactor
  )
{
  FIXP_DBL fac = (FIXP_DBL)0;

  CConcealParams *pConcealCommonData = hConcealmentInfo->pConcealParams;

  if (hConcealmentInfo->pConcealParams->method > ConcealMethodMute) {
    switch (hConcealmentInfo->concealState) {
      default:
      case ConcealState_Mute:
        /* Nothing to do here */
        break;
      case ConcealState_Ok:
        fac = (FIXP_DBL)MAXVAL_DBL;
        break;
      case ConcealState_Single:
      case ConcealState_FadeOut:
        {
          int idx = hConcealmentInfo->cntFadeFrames - ((fPreviousFactor != 0) ? 1 : 0);
          fac = (idx < 0) ? (FIXP_DBL)MAXVAL_DBL : FX_SGL2FX_DBL(pConcealCommonData->fadeOutFactor[idx]);
        }
        break;
      case ConcealState_FadeIn:
        {
          int idx = hConcealmentInfo->cntFadeFrames + ((fPreviousFactor != 0) ? 1 : 0);
          fac = (idx >= hConcealmentInfo->pConcealParams->numFadeInFrames) ? (FIXP_DBL)0 : FX_SGL2FX_DBL(pConcealCommonData->fadeInFactor[idx]);
        }
        break;
    }
  }

  return (fac);
}


/*!
  \brief Get fadeing factor for current concealment state.

  The function returns the state (ok or not) of the previous frame.
  If called before the function CConcealment_Apply() set the fBeforeApply
  flag to get the correct value.

  \return Frame OK flag of previous frame.
 */
int
  CConcealment_GetLastFrameOk (
      CConcealmentInfo *hConcealmentInfo,
      const int fBeforeApply
  )
{
  int prevFrameOk = 1;

  if (hConcealmentInfo != NULL) {
    prevFrameOk = hConcealmentInfo->prevFrameOk[fBeforeApply & 0x1];
  }

  return prevFrameOk;
}

/*!
  \brief Get the number of delay frames introduced by concealment technique. 

  \return Number of delay frames.
 */
UINT
  CConcealment_GetDelay (
      CConcealParams *pConcealCommonData
  )
{
  UINT frameDelay = 0;

  if (pConcealCommonData != NULL) {
    switch (pConcealCommonData->method) {
    case ConcealMethodTonal:
    case ConcealMethodInter:
      frameDelay = 1;
      break;
    default:
      break;
    }
  }

  return frameDelay;
}

