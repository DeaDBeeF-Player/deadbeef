
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
   Description: individual channel stream info

******************************************************************************/

#ifndef CHANNELINFO_H
#define CHANNELINFO_H

#include "common_fix.h"

#include "aac_rom.h"
#include "aacdecoder_lib.h"
#include "FDK_bitstream.h"
#include "overlapadd.h"

#include "mdct.h"
#include "stereo.h"
#include "pulsedata.h"
#include "aacdec_tns.h"

#include "aacdec_pns.h"

#include "aacdec_hcr_types.h"
#include "rvlc_info.h"


#include "conceal_types.h"

 #include "aacdec_drc_types.h"

/* Output rendering mode */
typedef enum {
  AACDEC_RENDER_INVALID = 0,
  AACDEC_RENDER_IMDCT,
  AACDEC_RENDER_ELDFB,
  AACDEC_RENDER_LPD,
  AACDEC_RENDER_INTIMDCT
} AACDEC_RENDER_MODE;

enum {
  MAX_QUANTIZED_VALUE = 8191
};

enum
{
  OnlyLongSequence = 0,
  LongStartSequence,
  EightShortSequence,
  LongStopSequence
};


typedef struct
{
  const SHORT *ScaleFactorBands_Long;
  const SHORT *ScaleFactorBands_Short;
  UCHAR NumberOfScaleFactorBands_Long;
  UCHAR NumberOfScaleFactorBands_Short;
  UINT samplingRateIndex;
  UINT samplingRate;
} SamplingRateInfo;

typedef struct
{
  UCHAR CommonWindow;
  UCHAR GlobalGain;

} CRawDataInfo;

typedef struct
{
  UCHAR WindowGroupLength[8];
  UCHAR WindowGroups;
  UCHAR Valid;

  UCHAR WindowShape;    /* 0: sine window, 1: KBD, 2: low overlap */
  UCHAR WindowSequence; /* See enum above, 0: long, 1: start, 2: short, 3: stop */
  UCHAR MaxSfBands;
  UCHAR ScaleFactorGrouping;

  UCHAR TotalSfBands;

} CIcsInfo;


enum
{
  ZERO_HCB = 0,
  ESCBOOK  = 11,
  NSPECBOOKS = ESCBOOK + 1,
  BOOKSCL    = NSPECBOOKS,
  NOISE_HCB      = 13,
  INTENSITY_HCB2 = 14,
  INTENSITY_HCB  = 15,
  LAST_HCB
};

#define TNS_SCALE  3

/*
 * This struct must be allocated one for every channel and must be persistent.
 */
typedef struct
{
  FIXP_DBL *pOverlapBuffer;
  mdct_t IMdct;



  CDrcChannelData   drcData;
  CConcealmentInfo concealmentInfo;

} CAacDecoderStaticChannelInfo;


/*
 * This union must be allocated for every element (up to 2 channels).
 */
typedef struct {

  /* Common bit stream data */
  SHORT aScaleFactor[(8*16)];           /* Spectral scale factors for each sfb in each window. */
  SHORT aSfbScale[(8*16)];              /* could be free after ApplyTools() */
  UCHAR aCodeBook[(8*16)];              /* section data: codebook for each window and sfb. */
  CTnsData         TnsData;
  CRawDataInfo     RawDataInfo;

  shouldBeUnion {

    struct {
      CPulseData PulseData;
      SHORT aNumLineInSec4Hcr[MAX_SFB_HCR];          /* needed once for all channels except for Drm syntax */
      UCHAR aCodeBooks4Hcr[MAX_SFB_HCR];             /* needed once for all channels except for Drm syntax. Same as "aCodeBook" ? */
      SHORT lenOfReorderedSpectralData;
      SCHAR lenOfLongestCodeword;
      SCHAR numberSection;
      SCHAR rvlcCurrentScaleFactorOK;
      SCHAR rvlcIntensityUsed;
    } aac;
  } specificTo;

} CAacDecoderDynamicData;

typedef shouldBeUnion {
  CAacDecoderDynamicData pAacDecoderDynamicData[2];

  /* Common signal data, can be used once the bit stream data from above is not used anymore. */
  FIXP_DBL mdctOutTemp[1024];
  FIXP_DBL sbrWorkBuffer[1024*2];

} CWorkBufferCore1;

/* Common data referenced by all channels */
typedef struct {

  CWorkBufferCore1 *workBufferCore1;
  FIXP_DBL* workBufferCore2;

  CPnsInterChannelData pnsInterChannelData;
  INT pnsCurrentSeed;
  INT pnsRandomSeed[(8*16)];

  CJointStereoData jointStereoData;              /* One for one element */

  shouldBeUnion {
    struct {
      CErHcrInfo erHcrInfo;
      CErRvlcInfo erRvlcInfo;
      SHORT aRvlcScfEsc[RVLC_MAX_SFB];               /* needed once for all channels */
      SHORT aRvlcScfFwd[RVLC_MAX_SFB];               /* needed once for all channels */
      SHORT aRvlcScfBwd[RVLC_MAX_SFB];               /* needed once for all channels */
    } aac;

  } overlay;

} CAacDecoderCommonData;


/*
 * This struct must be allocated one for every channels of every element and must be persistent.
 * Among its members, the following memory areas can be overwritten under the given conditions:
 *  - pSpectralCoefficient The memory pointed to can be overwritten after time signal rendering.
 *  - data can be overwritten after time signal rendering.
 *  - pDynData memory pointed to can be overwritten after each CChannelElement_Decode() call.
 *  - pComData->overlay memory pointed to can be overwritten after each CChannelElement_Decode() call..
 */
typedef struct
{
  SPECTRAL_PTR pSpectralCoefficient;             /* Spectral coefficients of each window */
  SHORT specScale[8];                  /* Scale shift values of each spectrum window */
  CIcsInfo icsInfo;
  INT granuleLength;                             /* Size of smallest spectrum piece */
  UCHAR ElementInstanceTag;

  AACDEC_RENDER_MODE renderMode;                 /* Output signal rendering mode */

  shouldBeUnion {
    struct {
      CPnsData PnsData; /* Not required for USAC */
    } aac;

    struct {
    } usac;
  } data;

  CAacDecoderDynamicData *pDynData; /* Data required for one element and discarded after decoding */
  CAacDecoderCommonData  *pComData; /* Data required for one channel at a time during decode */

} CAacDecoderChannelInfo;

/* channelinfo.cpp */

AAC_DECODER_ERROR getSamplingRateInfo(SamplingRateInfo *t, UINT samplesPerFrame, UINT samplingRateIndex, UINT samplingRate);

/**
 * \brief Read max SFB from bit stream and assign TotalSfBands according
 *        to the window sequence and sample rate.
 * \param hBs bit stream handle as data source
 * \param pIcsInfo IcsInfo structure to read the window sequence and store MaxSfBands and TotalSfBands
 * \param pSamplingRateInfo read only
 */
AAC_DECODER_ERROR IcsReadMaxSfb (
        HANDLE_FDK_BITSTREAM hBs,
        CIcsInfo *pIcsInfo,
        const SamplingRateInfo *pSamplingRateInfo
        );

AAC_DECODER_ERROR IcsRead(
        HANDLE_FDK_BITSTREAM bs,
        CIcsInfo *pIcsInfo,
        const SamplingRateInfo* SamplingRateInfoTable,
        const UINT flags
        );

/* stereo.cpp, only called from this file */

/*!
  \brief Applies MS stereo. 
  
  The function applies MS stereo.

  \param pAacDecoderChannelInfo aac channel info.
  \param pScaleFactorBandOffsets pointer to scalefactor band offsets.
  \param pWindowGroupLength pointer to window group length array.
  \param windowGroups number of window groups.
  \param scaleFactorBandsTransmittedL number of transmitted scalefactor bands in left channel.
  \param scaleFactorBandsTransmittedR number of transmitted scalefactor bands in right channel. 
                                      May differ from scaleFactorBandsTransmittedL only for USAC.
  \return  none
*/
void CJointStereo_ApplyMS(CAacDecoderChannelInfo *pAacDecoderChannelInfo[2],
                          const short *pScaleFactorBandOffsets,
                          const UCHAR *pWindowGroupLength,
                          const int windowGroups,
                          const int scaleFactorBandsTransmittedL,
                          const int scaleFactorBandsTransmittedR);

/*!
  \brief Applies intensity stereo

  The function applies intensity stereo.

  \param pAacDecoderChannelInfo aac channel info.
  \param pScaleFactorBandOffsets pointer to scalefactor band offsets.
  \param pWindowGroupLength pointer to window group length array.
  \param windowGroups number of window groups.
  \param scaleFactorBandsTransmitted number of transmitted scalefactor bands.
  \param CommonWindow common window bit.
  \return  none
*/
void CJointStereo_ApplyIS(CAacDecoderChannelInfo *pAacDecoderChannelInfo[2],
                          const short *pScaleFactorBandOffsets,
                          const UCHAR *pWindowGroupLength,
                          const int windowGroups,
                          const int scaleFactorBandsTransmitted,
                          const UINT CommonWindow);


/* aacdec_pns.cpp */
int CPns_IsPnsUsed (const CPnsData *pPnsData,
                    const int group,
                    const int band);

void CPns_SetCorrelation(CPnsData *pPnsData,
                         const int group,
                         const int band,
                         const int outofphase);

/****************** inline functions ******************/

inline UCHAR IsValid(const CIcsInfo *pIcsInfo)
{
  return pIcsInfo->Valid;
}

inline UCHAR IsLongBlock(const CIcsInfo *pIcsInfo)
{
  return (pIcsInfo->WindowSequence != EightShortSequence);
}

inline UCHAR GetWindowShape(const CIcsInfo *pIcsInfo)
{
  return pIcsInfo->WindowShape;
}

inline UCHAR GetWindowSequence(const CIcsInfo *pIcsInfo)
{
  return pIcsInfo->WindowSequence;
}

inline const SHORT *GetScaleFactorBandOffsets(const CIcsInfo *pIcsInfo, const SamplingRateInfo* samplingRateInfo)
{
  if (IsLongBlock(pIcsInfo))
  {
    return samplingRateInfo->ScaleFactorBands_Long;
  }
  else
  {
    return samplingRateInfo->ScaleFactorBands_Short;
  }
}

inline int GetWindowsPerFrame(const CIcsInfo *pIcsInfo)
{
  return (pIcsInfo->WindowSequence == EightShortSequence) ? 8 : 1;
}

inline UCHAR GetWindowGroups(const CIcsInfo *pIcsInfo)
{
  return pIcsInfo->WindowGroups;
}

inline UCHAR GetWindowGroupLength(const CIcsInfo *pIcsInfo, const INT index)
{
  return pIcsInfo->WindowGroupLength[index];
}

inline const UCHAR *GetWindowGroupLengthTable(const CIcsInfo *pIcsInfo)
{
  return pIcsInfo->WindowGroupLength;
}

inline UCHAR GetScaleFactorBandsTransmitted(const CIcsInfo *pIcsInfo)
{
  return pIcsInfo->MaxSfBands;
}

inline UCHAR GetScaleMaxFactorBandsTransmitted(const CIcsInfo *pIcsInfo0, const CIcsInfo *pIcsInfo1)
{
  return fMax(pIcsInfo0->MaxSfBands, pIcsInfo1->MaxSfBands);
}

inline UCHAR GetScaleFactorBandsTotal(const CIcsInfo *pIcsInfo)
{
  return pIcsInfo->TotalSfBands;
}

/* Note: This function applies to AAC-LC only ! */
inline UCHAR GetMaximumTnsBands(const CIcsInfo *pIcsInfo, const int samplingRateIndex)
{
  return tns_max_bands_tbl[samplingRateIndex][!IsLongBlock(pIcsInfo)];
}

#endif /* #ifndef CHANNELINFO_H */

