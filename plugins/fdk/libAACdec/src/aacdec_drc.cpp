
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
   Description: Dynamic range control (DRC) decoder tool for AAC

******************************************************************************/

#include "aacdec_drc.h"


#include "channelinfo.h"
#include "aac_rom.h"

 #include "sbrdecoder.h"

/*
 * Dynamic Range Control
 */

/* For parameter conversion */
#define DRC_PARAMETER_BITS        ( 7 )
#define DRC_MAX_QUANT_STEPS       ( 1<<DRC_PARAMETER_BITS )
#define DRC_MAX_QUANT_FACTOR      ( DRC_MAX_QUANT_STEPS-1 )
#define DRC_PARAM_QUANT_STEP      ( FL2FXCONST_DBL(1.0f/(float)DRC_MAX_QUANT_FACTOR) )
#define DRC_PARAM_SCALE           ( 1 )

#define MAX_REFERENCE_LEVEL       ( 127 )

 #define DVB_ANC_DATA_SYNC_BYTE   ( 0xBC )    /* DVB ancillary data sync byte. */

/*!
  \brief Initialize DRC information

  \self Handle of DRC info

  \return none
*/
void aacDecoder_drcInit (
    HANDLE_AAC_DRC self )
{
  CDrcParams *pParams;

  if (self == NULL) {
    return;
  }

  /* init control fields */
  self->enable = 0;
  self->numThreads = 0;

  /* init params */
  pParams = &self->params;
  pParams->bsDelayEnable = 0;
  pParams->cut      = FL2FXCONST_DBL(0.0f);
  pParams->usrCut   = FL2FXCONST_DBL(0.0f);
  pParams->boost    = FL2FXCONST_DBL(0.0f);
  pParams->usrBoost = FL2FXCONST_DBL(0.0f);
  pParams->targetRefLevel = -1;
  pParams->expiryFrame = AACDEC_DRC_DFLT_EXPIRY_FRAMES;
  pParams->applyDigitalNorm = 0;
  pParams->applyHeavyCompression = 0;

  /* initial program ref level = target ref level */
  self->progRefLevel = pParams->targetRefLevel;
  self->progRefLevelPresent = 0;
  self->presMode = -1;
}


/*!
  \brief Initialize DRC control data for one channel

  \self Handle of DRC info

  \return none
*/
void aacDecoder_drcInitChannelData (
    CDrcChannelData *pDrcChData )
{
  if (pDrcChData != NULL) {
    pDrcChData->expiryCount = 0;
    pDrcChData->numBands    = 1;
    pDrcChData->bandTop[0]  = (1024 >> 2) - 1;
    pDrcChData->drcValue[0] = 0;
    pDrcChData->drcInterpolationScheme = 0;
    pDrcChData->drcDataType = UNKNOWN_PAYLOAD;
  }
}


/*!
  \brief  Set one single DRC parameter

  \self   Handle of DRC info.
  \param  Parameter to be set.
  \value  Value to be set.

  \return an error code.
*/
AAC_DECODER_ERROR aacDecoder_drcSetParam (
    HANDLE_AAC_DRC    self,
    AACDEC_DRC_PARAM  param,
    INT               value )
{
  AAC_DECODER_ERROR ErrorStatus = AAC_DEC_OK;

  switch (param)
  {
  case DRC_CUT_SCALE:
    /* set attenuation scale factor */
    if ( (value < 0)
      || (value > DRC_MAX_QUANT_FACTOR) ) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    self->params.usrCut = (FIXP_DBL)((INT)(DRC_PARAM_QUANT_STEP>>DRC_PARAM_SCALE) * (INT)value);
    if (self->params.applyHeavyCompression == 0)
      self->params.cut = self->params.usrCut;
    break;
  case DRC_BOOST_SCALE:
    /* set boost factor */
    if ( (value < 0)
      || (value > DRC_MAX_QUANT_FACTOR) ) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    self->params.usrBoost = (FIXP_DBL)((INT)(DRC_PARAM_QUANT_STEP>>DRC_PARAM_SCALE) * (INT)value);
    if (self->params.applyHeavyCompression == 0)
      self->params.boost = self->params.usrBoost;
    break;
  case TARGET_REF_LEVEL:
    if ( value >  MAX_REFERENCE_LEVEL
      || value < -MAX_REFERENCE_LEVEL ) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    if (value < 0) {
      self->params.applyDigitalNorm = 0;
      self->params.targetRefLevel = -1;
    }
    else {
      /* ref_level must be between 0 and MAX_REFERENCE_LEVEL, inclusive */
      self->params.applyDigitalNorm = 1;
      if (self->params.targetRefLevel != (SCHAR)value) {
        self->params.targetRefLevel = (SCHAR)value;
        self->progRefLevel = (SCHAR)value;  /* Always set the program reference level equal to the
                                               target level according to 4.5.2.7.3 of ISO/IEC 14496-3. */
      }
    }
    break;
  case APPLY_NORMALIZATION:
    if (value < 0 || value > 1) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    /* Store new parameter value */
    self->params.applyDigitalNorm = (UCHAR)value;
    break;
  case APPLY_HEAVY_COMPRESSION:
    if (value < 0 || value > 1) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    if (self->params.applyHeavyCompression != (UCHAR)value) {
      if (value == 1) {
        /* Disable scaling of DRC values by setting the max values */
        self->params.boost = FL2FXCONST_DBL(1.0f/(float)(1<<DRC_PARAM_SCALE));
        self->params.cut   = FL2FXCONST_DBL(1.0f/(float)(1<<DRC_PARAM_SCALE));
      } else {
        /* Restore the user params */
        self->params.boost = self->params.usrBoost;
        self->params.cut   = self->params.usrCut;
      }
      /* Store new parameter value */
      self->params.applyHeavyCompression = (UCHAR)value;
    }
    break;
  case DRC_BS_DELAY:
    if (value < 0 || value > 1) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    self->params.bsDelayEnable = value;
    break;
  case DRC_DATA_EXPIRY_FRAME:
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    self->params.expiryFrame = (UINT)value;
    break;
  default:
    return AAC_DEC_SET_PARAM_FAIL;
  }  /* switch(param) */

  /* switch on/off processing */
  self->enable = ( (self->params.boost > (FIXP_DBL)0)
                || (self->params.cut   > (FIXP_DBL)0)
                || (self->params.applyHeavyCompression != 0)
                || (self->params.targetRefLevel >= 0) );


  return ErrorStatus;
}


static int parseExcludedChannels( UINT *excludedChnsMask,
                                  HANDLE_FDK_BITSTREAM bs )
{
  UINT excludeMask = 0;
  UINT i, j;
  int  bitCnt = 9;

  for (i = 0, j = 1; i < 7; i++, j<<=1) {
    if (FDKreadBits(bs,1)) {
      excludeMask |= j;
    }
  }

  /* additional_excluded_chns */
  while (FDKreadBits(bs,1)) {
    for (i = 0; i < 7; i++, j<<=1) {
      if (FDKreadBits(bs,1)) {
        excludeMask |= j;
      }
    }
    bitCnt += 9;
    FDK_ASSERT(j < (UINT)-1);
  }

  *excludedChnsMask = excludeMask;

  return (bitCnt);
}


/*!
  \brief Save DRC payload bitstream position

  \self Handle of DRC info
  \bs Handle of FDK bitstream

  \return The number of DRC payload bits
*/
int aacDecoder_drcMarkPayload (
    HANDLE_AAC_DRC self,
    HANDLE_FDK_BITSTREAM bs,
    AACDEC_DRC_PAYLOAD_TYPE type )
{
  UINT bsStartPos;
  int  i, numBands = 1, bitCnt = 0;

  if (self == NULL) {
    return 0;
  }

  bsStartPos = FDKgetValidBits(bs);

  switch (type) {
    case MPEG_DRC_EXT_DATA:
    {
      bitCnt = 4;

      if (FDKreadBits(bs,1)) {          /* pce_tag_present */
        FDKreadBits(bs,8);              /* pce_instance_tag + drc_tag_reserved_bits */
        bitCnt+=8;
      }

      if (FDKreadBits(bs,1)) {          /* excluded_chns_present */
        FDKreadBits(bs,7);              /* exclude mask [0..7] */
        bitCnt+=8;
        while (FDKreadBits(bs,1)) {     /* additional_excluded_chns */
          FDKreadBits(bs,7);            /* exclude mask [x..y] */
          bitCnt+=8;
        }
      }

      if (FDKreadBits(bs,1)) {          /* drc_bands_present */
        numBands += FDKreadBits(bs, 4); /* drc_band_incr */
        FDKreadBits(bs,4);              /* reserved */
        bitCnt+=8;
        for (i = 0; i < numBands; i++) {
          FDKreadBits(bs,8);            /* drc_band_top[i] */
          bitCnt+=8;
        }
      }

      if (FDKreadBits(bs,1)) {          /* prog_ref_level_present */
        FDKreadBits(bs,8);              /* prog_ref_level + prog_ref_level_reserved_bits */
        bitCnt+=8;
      }

      for (i = 0; i < numBands; i++) {
        FDKreadBits(bs,8);              /* dyn_rng_sgn[i] + dyn_rng_ctl[i] */
        bitCnt+=8;
      }

      if ( (self->numPayloads < MAX_DRC_THREADS)
        && ((INT)FDKgetValidBits(bs) >= 0) )
      {
        self->drcPayloadPosition[self->numPayloads++] = bsStartPos;
      }
    }
    break;

    case DVB_DRC_ANC_DATA:
      bitCnt += 8;
      /* check sync word */
      if (FDKreadBits(bs, 8) == DVB_ANC_DATA_SYNC_BYTE)
      {
        int dmxLevelsPresent, compressionPresent;
        int coarseGrainTcPresent, fineGrainTcPresent;

        /* bs_info field */ 
        FDKreadBits(bs, 8);                          /* mpeg_audio_type, dolby_surround_mode, presentation_mode */
        bitCnt+=8;

        /* Evaluate ancillary_data_status */
        FDKreadBits(bs, 3);                          /* reserved, set to 0 */
        dmxLevelsPresent = FDKreadBits(bs, 1);       /* downmixing_levels_MPEG4_status */
        FDKreadBits(bs, 1);                          /* reserved, set to 0 */
        compressionPresent   = FDKreadBits(bs, 1);   /* audio_coding_mode_and_compression status */
        coarseGrainTcPresent = FDKreadBits(bs, 1);   /* coarse_grain_timecode_status */
        fineGrainTcPresent   = FDKreadBits(bs, 1);   /* fine_grain_timecode_status */
        bitCnt+=8;

        /* MPEG4 downmixing levels */
        if (dmxLevelsPresent) {
          FDKreadBits(bs, 8);                        /* downmixing_levels_MPEG4 */
          bitCnt+=8;
        }
        /* audio coding mode and compression status */
        if (compressionPresent) {
          FDKreadBits(bs, 16);                        /* audio_coding_mode, Compression_value */
          bitCnt+=16;
        }
        /* coarse grain timecode */
        if (coarseGrainTcPresent) {
          FDKreadBits(bs, 16);                       /* coarse_grain_timecode */
          bitCnt+=16;
        }
        /* fine grain timecode */
        if (fineGrainTcPresent) {
          FDKreadBits(bs, 16);                       /* fine_grain_timecode */
          bitCnt+=16;
        }
        if ( !self->dvbAncDataAvailable
          && ((INT)FDKgetValidBits(bs) >= 0) )
        {
          self->dvbAncDataPosition  = bsStartPos;
          self->dvbAncDataAvailable = 1;
        }
      }
      break;

    default:
      break;
  }

  return (bitCnt);
}


/*!
  \brief Parse DRC parameters from bitstream

  \bs Handle of FDK bitstream (in)
  \pDrcBs Pointer to DRC payload data container (out)
  \payloadPosition Bitstream position of MPEG DRC data junk (in)

  \return Number of bits read (0 in case of a parse error)
*/
static int aacDecoder_drcParse (
    HANDLE_FDK_BITSTREAM  bs,
    CDrcPayload          *pDrcBs,
    UINT                  payloadPosition )
{
  int i, numBands, bitCnt = 4;

  /* Move to the beginning of the DRC payload field */
  FDKpushBiDirectional(bs, FDKgetValidBits(bs)-payloadPosition);

  /* pce_tag_present */
  if (FDKreadBits(bs,1))
  {
    pDrcBs->pceInstanceTag = FDKreadBits(bs, 4);  /* pce_instance_tag */
    /* only one program supported */
    FDKreadBits(bs, 4);  /* drc_tag_reserved_bits */
    bitCnt += 8;
  } else {
    pDrcBs->pceInstanceTag = -1;  /* not present */
  }

  if (FDKreadBits(bs,1)) {        /* excluded_chns_present */
    /* get excluded_chn_mask */
    bitCnt += parseExcludedChannels(&pDrcBs->excludedChnsMask, bs);
  } else {
    pDrcBs->excludedChnsMask = 0;
  }

  numBands = 1;
  if (FDKreadBits(bs,1))  /* drc_bands_present */
  {
    /* get band_incr */
    numBands += FDKreadBits(bs, 4);  /* drc_band_incr */
    pDrcBs->channelData.drcInterpolationScheme = FDKreadBits(bs, 4);  /* drc_interpolation_scheme */
    bitCnt += 8;
    /* band_top */
    for (i = 0; i < numBands; i++)
    {
      pDrcBs->channelData.bandTop[i] = FDKreadBits(bs, 8);  /* drc_band_top[i] */
      bitCnt += 8;
    }
  }
  else {
    pDrcBs->channelData.bandTop[0] = (1024 >> 2) - 1;  /* ... comprising the whole spectrum. */;
  }

  pDrcBs->channelData.numBands = numBands;

  if (FDKreadBits(bs,1))                        /* prog_ref_level_present */
  {
    pDrcBs->progRefLevel = FDKreadBits(bs, 7);  /* prog_ref_level */
    FDKreadBits(bs, 1);                         /* prog_ref_level_reserved_bits */
    bitCnt += 8;
  } else {
    pDrcBs->progRefLevel = -1;
  }

  for (i = 0; i < numBands; i++)
  {
    pDrcBs->channelData.drcValue[i]  = FDKreadBits(bs, 1) << 7;   /* dyn_rng_sgn[i] */
    pDrcBs->channelData.drcValue[i] |= FDKreadBits(bs, 7) & 0x7F; /* dyn_rng_ctl[i] */
    bitCnt += 8;
  }

  /* Set DRC payload type */
  pDrcBs->channelData.drcDataType = MPEG_DRC_EXT_DATA;

  return (bitCnt);
}


/*!
  \brief Parse heavy compression value transported in DSEs of DVB streams with MPEG-4 content.

  \bs Handle of FDK bitstream (in)
  \pDrcBs Pointer to DRC payload data container (out)
  \payloadPosition Bitstream position of DVB ancillary data junk

  \return Number of bits read (0 in case of a parse error)
*/
#define DVB_COMPRESSION_SCALE   ( 8 )       /* 48,164 dB */

static int aacDecoder_drcReadCompression (
    HANDLE_FDK_BITSTREAM  bs,
    CDrcPayload          *pDrcBs,
    UINT                  payloadPosition )
{
  int  bitCnt = 0;
  int  dmxLevelsPresent, extensionPresent, compressionPresent;
  int  coarseGrainTcPresent, fineGrainTcPresent;

  /* Move to the beginning of the DRC payload field */
  FDKpushBiDirectional(bs, FDKgetValidBits(bs)-payloadPosition);

  /* Sanity checks */
  if ( FDKgetValidBits(bs) < 24 ) {
    return 0;
  }

  /* Check sync word */
  if (FDKreadBits(bs, 8) != DVB_ANC_DATA_SYNC_BYTE) {
    return 0;
  }

  /* Evaluate bs_info field */ 
  if (FDKreadBits(bs, 2) != 3) {               /* mpeg_audio_type */
    /* No MPEG-4 audio data */
    return 0;
  }
  FDKreadBits(bs, 2);                          /* dolby_surround_mode */
  pDrcBs->presMode = FDKreadBits(bs, 2);       /* presentation_mode */
  FDKreadBits(bs, 1);                          /* stereo_downmix_mode */
  if (FDKreadBits(bs, 1) != 0) {               /* reserved, set to 0 */
    return 0;
  }

  /* Evaluate ancillary_data_status */
  if (FDKreadBits(bs, 3) != 0) {               /* reserved, set to 0 */
    return 0;
  }
  dmxLevelsPresent = FDKreadBits(bs, 1);       /* downmixing_levels_MPEG4_status */
  extensionPresent = FDKreadBits(bs, 1);       /* ancillary_data_extension_status; */
  compressionPresent   = FDKreadBits(bs, 1);   /* audio_coding_mode_and_compression status */
  coarseGrainTcPresent = FDKreadBits(bs, 1);   /* coarse_grain_timecode_status */
  fineGrainTcPresent   = FDKreadBits(bs, 1);   /* fine_grain_timecode_status */
  bitCnt += 24;

  if (dmxLevelsPresent) {
    FDKreadBits(bs, 8);                        /* downmixing_levels_MPEG4 */
    bitCnt += 8;
  }

  /* audio_coding_mode_and_compression_status */
  if (compressionPresent)
  {
    UCHAR compressionOn, compressionValue;

    /* audio_coding_mode */
    if ( FDKreadBits(bs, 7) != 0 ) {  /* The reserved bits shall be set to "0". */
      return 0;
    }
    compressionOn    = (UCHAR)FDKreadBits(bs, 1);  /* compression_on */
    compressionValue = (UCHAR)FDKreadBits(bs, 8);  /* Compression_value */
    bitCnt += 16;

    if ( compressionOn ) {
      /* A compression value is available so store the data just like MPEG DRC data */
      pDrcBs->channelData.numBands    =  1;                            /* One band ... */
      pDrcBs->channelData.drcValue[0] =  compressionValue;             /* ... with one value ... */
      pDrcBs->channelData.bandTop[0]  = (1024 >> 2) - 1;  /* ... comprising the whole spectrum. */
      pDrcBs->pceInstanceTag          = -1;                            /* Not present */
      pDrcBs->progRefLevel            = -1;                            /* Not present */
      pDrcBs->channelData.drcDataType =  DVB_DRC_ANC_DATA;             /* Set DRC payload type to DVB. */
    } else {
      /* No compression value available */
      /* CAUTION: It is not clearly defined by standard how to react in this situation. */
      /* Turn down the compression value to aprox. 0dB */
      pDrcBs->channelData.numBands    =  1;                            /* One band ... */
      pDrcBs->channelData.drcValue[0] =  0x80;                         /* ... with aprox. 0dB ... */
      pDrcBs->channelData.bandTop[0]  = (1024 >> 2) - 1;  /* ... comprising the whole spectrum. */
      pDrcBs->channelData.drcDataType =  DVB_DRC_ANC_DATA;             /* Set DRC payload type to DVB. */

      /* If compression_on field is set to "0" the compression_value field shall be "0000 0000". */
      if (compressionValue != 0) {
        return 0;
      }
    }
  }

  /* Read timecodes if available just to get the right amount of bits. */
  if (coarseGrainTcPresent) {
    FDKreadBits(bs, 16);      /* coarse_grain_timecode */
    bitCnt += 16;
  }
  if (fineGrainTcPresent) {
    FDKreadBits(bs, 16);      /* fine_grain_timecode */
    bitCnt += 16;
  }

  /* Read extension just to get the right amount of bits. */
  if (extensionPresent) {
    int  extBits = 8;

    FDKreadBits(bs, 1);                     /* reserved, set to 0 */
    if (FDKreadBits(bs, 1)) extBits += 8;   /* ext_downmixing_levels_status */
    if (FDKreadBits(bs, 1)) extBits += 16;  /* ext_downmixing_global_gains_status */
    if (FDKreadBits(bs, 1)) extBits += 8;   /* ext_downmixing_lfe_level_status */

    FDKpushFor(bs, extBits - 4);            /* skip the extension payload remainder. */
    bitCnt += extBits;
  }

  return (bitCnt);
}


/* 
 * Prepare DRC processing
 */
static int aacDecoder_drcExtractAndMap (
        HANDLE_AAC_DRC  self,
        HANDLE_FDK_BITSTREAM hBs,
        CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[],
        UCHAR  pceInstanceTag,
        UCHAR  channelMapping[], /* Channel mapping translating drcChannel index to canonical channel index */
        int    validChannels )
{
  CDrcPayload  threadBs[MAX_DRC_THREADS];
  CDrcPayload *validThreadBs[MAX_DRC_THREADS];
  CDrcParams  *pParams;
  UINT backupBsPosition;
  int  i, thread, validThreads = 0;
  int  numExcludedChns[MAX_DRC_THREADS];

  FDK_ASSERT(self != NULL);
  FDK_ASSERT(hBs != NULL);
  FDK_ASSERT(pAacDecoderStaticChannelInfo != NULL);

  pParams = &self->params;

  self->numThreads = 0;
  backupBsPosition = FDKgetValidBits(hBs);

  for (i = 0; i < self->numPayloads && self->numThreads < MAX_DRC_THREADS; i++) {
    int bitsParsed;

    /* Init payload data chunk. The memclear is very important because it initializes
       the most values. Without it the module wouldn't work properly or crash. */
    FDKmemclear(&threadBs[self->numThreads], sizeof(CDrcPayload));
    threadBs[self->numThreads].channelData.bandTop[0]  = (1024 >> 2) - 1;

    /* Extract payload */
    bitsParsed = aacDecoder_drcParse( hBs,
                                     &threadBs[self->numThreads],
                                      self->drcPayloadPosition[i] );
    if (bitsParsed > 0) {
      self->numThreads++;
    }
  }
  self->numPayloads = 0;

  if (self->dvbAncDataAvailable && self->numThreads < MAX_DRC_THREADS)
  { /* Append a DVB heavy compression payload thread if available. */
    int bitsParsed;

    /* Init payload data chunk. The memclear is very important because it initializes
       the most values. Without it the module wouldn't work properly or crash. */
    FDKmemclear(&threadBs[self->numThreads], sizeof(CDrcPayload));
    threadBs[self->numThreads].channelData.bandTop[0]  = (1024 >> 2) - 1;

    /* Extract payload */
    bitsParsed = aacDecoder_drcReadCompression( hBs,
                                               &threadBs[self->numThreads],
                                                self->dvbAncDataPosition );
    if (bitsParsed > 0) {
      self->numThreads++;
    }
  }
  self->dvbAncDataAvailable = 0;

  /* Reset the bitbufffer */
  FDKpushBiDirectional(hBs, FDKgetValidBits(hBs) - backupBsPosition);

  /* calculate number of valid bits in excl_chn_mask */

  /* coupling channels not supported */

  /* check for valid threads */
  for (thread = 0; thread < self->numThreads; thread++) {
    CDrcPayload *pThreadBs = &threadBs[thread];
    int numExclChns = 0;

    switch ((AACDEC_DRC_PAYLOAD_TYPE)pThreadBs->channelData.drcDataType) {
      default:
        continue;
      case MPEG_DRC_EXT_DATA:
      case DVB_DRC_ANC_DATA:
        break;
    }

    if (pThreadBs->pceInstanceTag >= 0) {  /* if PCE tag present */
      if (pThreadBs->pceInstanceTag != pceInstanceTag) {
        continue;  /* don't accept */
      }
    }

    /* calculate number of excluded channels */
    if (pThreadBs->excludedChnsMask > 0) {
      INT exclMask = pThreadBs->excludedChnsMask;
      int ch;
      for (ch = 0; ch < validChannels; ch++) {
        numExclChns += exclMask & 0x1;
        exclMask >>= 1;
      }
    }
    if (numExclChns < validChannels) {
      validThreadBs[validThreads]   = pThreadBs;
      numExcludedChns[validThreads] = numExclChns;
      validThreads++;
    }
  }

  if (validThreads > 1) {
    int ch;

    /* check consistency of excl_chn_mask amongst valid DRC threads */
    for (ch = 0; ch < validChannels; ch++) {
      int present = 0;

      for (thread = 0; thread < validThreads; thread++) {
        CDrcPayload *pThreadBs = validThreadBs[thread];


        /* thread applies to this channel */
        if ( (pThreadBs->channelData.drcDataType == MPEG_DRC_EXT_DATA)
          && ( (numExcludedChns[thread] == 0)
            || (!(pThreadBs->excludedChnsMask & (1<<ch))) ) ) {
          present++;
        }
      }


      if (present > 1) {
        return -1;
      }
    }
  }

  /* map DRC bitstream information onto DRC channel information */
  for (thread = 0; thread < validThreads; thread++)
  {
    CDrcPayload *pThreadBs = validThreadBs[thread];
    INT exclMask = pThreadBs->excludedChnsMask;
    AACDEC_DRC_PAYLOAD_TYPE drcPayloadType = (AACDEC_DRC_PAYLOAD_TYPE)pThreadBs->channelData.drcDataType;
    int ch;

    /* last progRefLevel transmitted is the one that is used
     * (but it should really only be transmitted once per block!)
     */
    if (pThreadBs->progRefLevel >= 0) {
      self->progRefLevel = pThreadBs->progRefLevel;
      self->progRefLevelPresent = 1;
      self->prlExpiryCount = 0;  /* Got a new value -> Reset counter */
    }

    if (drcPayloadType == DVB_DRC_ANC_DATA) {
      /* Announce the presentation mode of this valid thread. */
      self->presMode = pThreadBs->presMode;
    }

    /* SCE, CPE and LFE */
    for (ch = 0; ch < validChannels; ch++) {
      int mapedChannel = channelMapping[ch];

      if ( ((exclMask & (1<<mapedChannel)) == 0)
        && ( (drcPayloadType == MPEG_DRC_EXT_DATA)
          || ((drcPayloadType == DVB_DRC_ANC_DATA) && self->params.applyHeavyCompression)
         ) ) {
        /* copy thread to channel */
        pAacDecoderStaticChannelInfo[ch]->drcData = pThreadBs->channelData;
      }
    }
    /* CCEs not supported by now */
  }

  /* Increment and check expiry counter for the program reference level: */
  if ( (pParams->expiryFrame > 0)
    && (self->prlExpiryCount++ > pParams->expiryFrame) )
  { /* The program reference level is too old, so set it back to the target level. */
    self->progRefLevelPresent = 0;
    self->progRefLevel = pParams->targetRefLevel;
    self->prlExpiryCount = 0;
  }

  return 0;
}


void aacDecoder_drcApply (
        HANDLE_AAC_DRC          self,
        void                   *pSbrDec,
        CAacDecoderChannelInfo *pAacDecoderChannelInfo,
        CDrcChannelData        *pDrcChData,
        FIXP_DBL               *extGain,
        int  ch,   /* needed only for SBR */
        int  aacFrameSize,
        int  bSbrPresent )
{
  int band, top, bin, numBands;
  int bottom = 0;
  int modifyBins = 0;

  FIXP_DBL max_mantissa;
  INT max_exponent;

  FIXP_DBL norm_mantissa = FL2FXCONST_DBL(0.5f);
  INT  norm_exponent = 1;

  FIXP_DBL fact_mantissa[MAX_DRC_BANDS];
  INT  fact_exponent[MAX_DRC_BANDS];

  CDrcParams  *pParams = &self->params;

  FIXP_DBL    *pSpectralCoefficient  =  SPEC_LONG(pAacDecoderChannelInfo->pSpectralCoefficient);
  CIcsInfo    *pIcsInfo              = &pAacDecoderChannelInfo->icsInfo;
  SHORT       *pSpecScale            =  pAacDecoderChannelInfo->specScale;

  int winSeq = pIcsInfo->WindowSequence;

  /* Increment and check expiry counter */
  if ( (pParams->expiryFrame > 0)
    && (++pDrcChData->expiryCount > pParams->expiryFrame) )
  { /* The DRC data is too old, so delete it. */
    aacDecoder_drcInitChannelData( pDrcChData );
  }

  if (!self->enable) {
    sbrDecoder_drcDisable( (HANDLE_SBRDECODER)pSbrDec, ch );
    if (extGain != NULL) {
      INT gainScale = (INT)*extGain;
      /* The gain scaling must be passed to the function in the buffer pointed on by extGain. */
      if (gainScale >= 0 && gainScale <= DFRACT_BITS) {
        *extGain = scaleValue(norm_mantissa, norm_exponent-gainScale);
      } else {
        FDK_ASSERT(0);
      }
    }
    return;
  }

  numBands = pDrcChData->numBands;
  top = FDKmax(0, numBands-1);

  pDrcChData->bandTop[0] = fixMin(pDrcChData->bandTop[0], (aacFrameSize >> 2) - 1);

  /* If program reference normalization is done in the digital domain,
  modify factor to perform normalization.  prog_ref_level can
  alternatively be passed to the system for modification of the level in
  the analog domain.  Analog level modification avoids problems with
  reduced DAC SNR (if signal is attenuated) or clipping (if signal is
  boosted) */

  if (pParams->targetRefLevel >= 0)
  {
    /* 0.5^((targetRefLevel - progRefLevel)/24) */
    norm_mantissa = fLdPow(
            FL2FXCONST_DBL(-1.0), /* log2(0.5) */
            0,
            (FIXP_DBL)((INT)(FL2FXCONST_DBL(1.0f/24.0)>>3) * (INT)(pParams->targetRefLevel-self->progRefLevel)),
            3,
           &norm_exponent );
  }
  /* Always export the normalization gain (if possible). */
  if (extGain != NULL) {
    INT gainScale = (INT)*extGain;
    /* The gain scaling must be passed to the function in the buffer pointed on by extGain. */
    if (gainScale >= 0 && gainScale <= DFRACT_BITS) {
      *extGain = scaleValue(norm_mantissa, norm_exponent-gainScale);
    } else {
      FDK_ASSERT(0);
    }
  }
  if (self->params.applyDigitalNorm == 0) {
    /* Reset normalization gain since this module must not apply it */
    norm_mantissa = FL2FXCONST_DBL(0.5f);
    norm_exponent = 1;
  }


  /* calc scale factors */
  for (band = 0; band < numBands; band++)
  {
    UCHAR drcVal = pDrcChData->drcValue[band];
    top = fixMin((int)( (pDrcChData->bandTop[band]+1)<<2 ), aacFrameSize);

    fact_mantissa[band] = FL2FXCONST_DBL(0.5f);
    fact_exponent[band] = 1;

    if (  pParams->applyHeavyCompression
      && ((AACDEC_DRC_PAYLOAD_TYPE)pDrcChData->drcDataType == DVB_DRC_ANC_DATA) )
    {
      INT compressionFactorVal_e;
      int valX, valY;

      valX = drcVal >> 4;
      valY = drcVal & 0x0F;

      /* calculate the unscaled heavy compression factor.
         compressionFactor = 48.164 - 6.0206*valX - 0.4014*valY dB
         range: -48.166 dB to 48.164 dB */
      if ( drcVal != 0x7F ) {
        fact_mantissa[band] =
          fPowInt( FL2FXCONST_DBL(0.95483867181), /* -0.4014dB = 0.95483867181 */
                   0,
                   valY,
                  &compressionFactorVal_e );

        /* -0.0008dB (48.164 - 6.0206*8 = -0.0008) */
        fact_mantissa[band] = fMult(FL2FXCONST_DBL(0.99990790084), fact_mantissa[band]);

        fact_exponent[band] = DVB_COMPRESSION_SCALE - valX + compressionFactorVal_e;
      }
    } else
    if ((AACDEC_DRC_PAYLOAD_TYPE)pDrcChData->drcDataType == MPEG_DRC_EXT_DATA)
    {
    /* apply the scaled dynamic range control words to factor.
     * if scaling drc_cut (or drc_boost), or control word drc_mantissa is 0
     * then there is no dynamic range compression
     *
     * if pDrcChData->drcSgn[band] is 
     *  1 then gain is < 1 :  factor = 2^(-self->cut   * pDrcChData->drcMag[band] / 24)
     *  0 then gain is > 1 :  factor = 2^( self->boost * pDrcChData->drcMag[band] / 24)
     */

    if ((drcVal&0x7F) > 0) {
      FIXP_DBL tParamVal = (drcVal & 0x80) ? -pParams->cut : pParams->boost;

      fact_mantissa[band] =
        f2Pow( (FIXP_DBL)((INT)fMult(FL2FXCONST_DBL(1.0f/192.0f), tParamVal) * (drcVal&0x7F)),
                 3+DRC_PARAM_SCALE,
                &fact_exponent[band] );
    }
    }

    fact_mantissa[band]  = fMult(fact_mantissa[band], norm_mantissa);
    fact_exponent[band] += norm_exponent;


    bottom = top;

  }  /* end loop over bands */


  /* normalizations */
  {
    int res;

    max_mantissa = FL2FXCONST_DBL(0.0f);
    max_exponent = 0;
    for (band = 0; band < numBands; band++) {
      max_mantissa = fixMax(max_mantissa, fact_mantissa[band]);
      max_exponent = fixMax(max_exponent, fact_exponent[band]);
    }

    /* left shift factors to gain accurancy */
    res = CntLeadingZeros(max_mantissa) - 1;

    /* above topmost DRC band gain factor is 1 */
    if (((pDrcChData->bandTop[numBands-1]+1)<<2) < aacFrameSize) res = 0;

    if (res > 0) {
      res = fixMin(res, max_exponent);
      max_exponent -= res;

      for (band = 0; band < numBands; band++) {
        fact_mantissa[band] <<= res;
        fact_exponent[band]  -= res;
      }
    }

    /* normalize magnitudes to one scale factor */
    for (band = 0; band < numBands; band++) {
      if (fact_exponent[band] < max_exponent) {
        fact_mantissa[band] >>= max_exponent - fact_exponent[band];
      }
      if (fact_mantissa[band] != FL2FXCONST_DBL(0.5f)) {
        modifyBins = 1;
      }
    }
    if (max_exponent != 1) {
      modifyBins = 1;
    }
  }

  /*  apply factor to spectral lines
   *  short blocks must take care that bands fall on 
   *  block boundaries!
   */
  if (!bSbrPresent)
  {
    bottom = 0;

    if (!modifyBins) {
      /* We don't have to modify the spectral bins because the fractional part of all factors is 0.5.
         In order to keep accurancy we don't apply the factor but decrease the exponent instead. */
      max_exponent -= 1;
    } else
    {
      for (band = 0; band < numBands; band++)
      {
        top = fixMin((int)( (pDrcChData->bandTop[band]+1)<<2 ), aacFrameSize);   /* ... * DRC_BAND_MULT; */

        for (bin = bottom; bin < top; bin++) {
          pSpectralCoefficient[bin] = fMult(pSpectralCoefficient[bin], fact_mantissa[band]);
        }

        bottom = top;
      }
    }

    /* above topmost DRC band gain factor is 1 */
    if (max_exponent > 0) {
      for (bin = bottom; bin < aacFrameSize; bin+=1) {
        pSpectralCoefficient[bin] >>= max_exponent;
      }
    }

    /* adjust scaling */
    pSpecScale[0] += max_exponent;

    if (winSeq == EightShortSequence) {
      int win;
      for (win = 1; win < 8; win++) {
        pSpecScale[win] += max_exponent;
      }
    }
  }
  else {
    HANDLE_SBRDECODER hSbrDecoder = (HANDLE_SBRDECODER)pSbrDec;
    UINT numBands = pDrcChData->numBands;

    /* feed factors into SBR decoder for application in QMF domain. */
    sbrDecoder_drcFeedChannel (
            hSbrDecoder,
            ch,
            numBands,
            fact_mantissa,
            max_exponent,
            pDrcChData->drcInterpolationScheme,
            winSeq,
            pDrcChData->bandTop
          );
  }

  return;
}


/* 
 * Prepare DRC processing
 */
int aacDecoder_drcProlog (
        HANDLE_AAC_DRC  self,
        HANDLE_FDK_BITSTREAM hBs,
        CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[],
        UCHAR  pceInstanceTag,
        UCHAR  channelMapping[], /* Channel mapping translating drcChannel index to canonical channel index */
        int    validChannels )
{
  int err = 0;

  if (self == NULL) {
    return -1;
  }

  if (!self->params.bsDelayEnable)
  {
    err = aacDecoder_drcExtractAndMap (
            self,
            hBs,
            pAacDecoderStaticChannelInfo,
            pceInstanceTag,
            channelMapping,
            validChannels );
  }

  return err;
}


/* 
 * Finalize DRC processing
 */
int aacDecoder_drcEpilog (
        HANDLE_AAC_DRC  self,
        HANDLE_FDK_BITSTREAM hBs,
        CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[],
        UCHAR  pceInstanceTag,
        UCHAR  channelMapping[], /* Channel mapping translating drcChannel index to canonical channel index */
        int    validChannels )
{
  int err = 0;

  if (self == NULL) {
    return -1;
  }

  if (self->params.bsDelayEnable)
  {
    err = aacDecoder_drcExtractAndMap (
            self,
            hBs,
            pAacDecoderStaticChannelInfo,
            pceInstanceTag,
            channelMapping,
            validChannels );
  }

  return err;
}

/*
 * Export relevant metadata info from bitstream payload.
 */
void aacDecoder_drcGetInfo (
        HANDLE_AAC_DRC  self,
        SCHAR          *pPresMode,
        SCHAR          *pProgRefLevel )
{
  if (self != NULL) {
    if (pPresMode != NULL) {
      *pPresMode = self->presMode;
    }
    if (pProgRefLevel != NULL) {
      if (self->progRefLevelPresent) {
        *pProgRefLevel = self->progRefLevel;
      } else {
        *pProgRefLevel = -1;
      }
    }
  }
}
