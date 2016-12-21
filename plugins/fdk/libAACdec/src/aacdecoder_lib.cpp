
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

/*****************************  MPEG-4 AAC Decoder  **************************

   Author(s):   Manuel Jander
   Description:

******************************************************************************/

#include "aacdecoder_lib.h"

#include "aac_ram.h"
#include "aacdecoder.h"
#include "tpdec_lib.h"
#include "FDK_core.h" /* FDK_tools version info */


 #include "sbrdecoder.h"




#include "conceal.h"

 #include "aacdec_drc.h"



/* Decoder library info */
#define AACDECODER_LIB_VL0 2
#define AACDECODER_LIB_VL1 5
#define AACDECODER_LIB_VL2 17
#define AACDECODER_LIB_TITLE "AAC Decoder Lib"
#ifdef __ANDROID__
#define AACDECODER_LIB_BUILD_DATE ""
#define AACDECODER_LIB_BUILD_TIME ""
#else
#define AACDECODER_LIB_BUILD_DATE __DATE__
#define AACDECODER_LIB_BUILD_TIME __TIME__
#endif

static AAC_DECODER_ERROR
setConcealMethod ( const HANDLE_AACDECODER  self,
                   const INT                method );


LINKSPEC_CPP AAC_DECODER_ERROR aacDecoder_GetFreeBytes ( const HANDLE_AACDECODER  self, UINT *pFreeBytes){

  /* reset free bytes */
  *pFreeBytes = 0;

  /* check handle */
  if(!self)
    return AAC_DEC_INVALID_HANDLE;

  /* return nr of free bytes */
  HANDLE_FDK_BITSTREAM hBs = transportDec_GetBitstream(self->hInput, 0);
  *pFreeBytes = FDKgetFreeBits(hBs) >> 3;

  /* success */
  return AAC_DEC_OK;
}

/**
 * Config Decoder using a CSAudioSpecificConfig struct.
 */
static
LINKSPEC_CPP AAC_DECODER_ERROR aacDecoder_Config(HANDLE_AACDECODER self, const CSAudioSpecificConfig *pAscStruct)
{
  AAC_DECODER_ERROR err;

  /* Initialize AAC core decoder, and update self->streaminfo */
  err = CAacDecoder_Init(self, pAscStruct);

  return err;
}

LINKSPEC_CPP AAC_DECODER_ERROR aacDecoder_ConfigRaw (
        HANDLE_AACDECODER self,
        UCHAR *conf[],
        const UINT length[] )
{
  AAC_DECODER_ERROR err = AAC_DEC_OK;
  TRANSPORTDEC_ERROR   errTp;
  UINT layer, nrOfLayers = self->nrOfLayers;

  for(layer = 0; layer < nrOfLayers; layer++){
    if(length[layer] > 0){
      errTp = transportDec_OutOfBandConfig(self->hInput, conf[layer], length[layer], layer);
      if (errTp != TRANSPORTDEC_OK) {
        switch (errTp) {
        case TRANSPORTDEC_NEED_TO_RESTART:
          err = AAC_DEC_NEED_TO_RESTART;
          break;
        case TRANSPORTDEC_UNSUPPORTED_FORMAT:
          err = AAC_DEC_UNSUPPORTED_FORMAT;
          break;
        default:
          err = AAC_DEC_UNKNOWN;
          break;
        }
        /* if baselayer is OK we continue decoding */
        if(layer >= 1){
          self->nrOfLayers = layer;
          err = AAC_DEC_OK;
        }
        break;
      }
    }
  }

  return err;
}



static INT aacDecoder_ConfigCallback(void *handle, const CSAudioSpecificConfig *pAscStruct)
{
  HANDLE_AACDECODER self = (HANDLE_AACDECODER)handle;
  AAC_DECODER_ERROR err = AAC_DEC_OK;
  TRANSPORTDEC_ERROR errTp;

  {
    {
      err = aacDecoder_Config(self, pAscStruct);
    }
  }
  if (err == AAC_DEC_OK) {
    if ( self->flags & (AC_USAC|AC_RSVD50|AC_LD|AC_ELD)
      && CConcealment_GetDelay(&self->concealCommonData) > 0 )
    {
      /* Revert to error concealment method Noise Substitution.
         Because interpolation is not implemented for USAC/RSVD50 or
         the additional delay is unwanted for low delay codecs. */
      setConcealMethod(self, 1);
#ifdef DEBUG
      FDKprintf("  Concealment method was reverted to 1 !\n");
#endif
    }
    errTp = TRANSPORTDEC_OK;
  } else {
    if (IS_INIT_ERROR(err)) {
      errTp = TRANSPORTDEC_UNSUPPORTED_FORMAT;
    } /* Fatal errors */
    else if (err == AAC_DEC_NEED_TO_RESTART) {
      errTp = TRANSPORTDEC_NEED_TO_RESTART;
    } else {
      errTp = TRANSPORTDEC_UNKOWN_ERROR;
    }
  }

  return errTp;
}



LINKSPEC_CPP AAC_DECODER_ERROR
aacDecoder_AncDataInit ( HANDLE_AACDECODER self,
                         UCHAR *buffer,
                         int size )
{
  CAncData *ancData = &self->ancData;

  return CAacDecoder_AncDataInit(ancData, buffer, size);
}


LINKSPEC_CPP AAC_DECODER_ERROR
aacDecoder_AncDataGet ( HANDLE_AACDECODER self,
                        int     index,
                        UCHAR **ptr,
                        int    *size )
{
  CAncData *ancData = &self->ancData;

  return CAacDecoder_AncDataGet(ancData, index, ptr, size);
}


static AAC_DECODER_ERROR
setConcealMethod ( const HANDLE_AACDECODER  self,   /*!< Handle of the decoder instance */
                   const INT                method )
{
  AAC_DECODER_ERROR errorStatus = AAC_DEC_OK;
  CConcealParams  *pConcealData = NULL;
  HANDLE_SBRDECODER hSbrDec = NULL;
  HANDLE_AAC_DRC hDrcInfo = NULL;
  HANDLE_PCM_DOWNMIX hPcmDmx = NULL;
  CConcealmentMethod backupMethod = ConcealMethodNone;
  int backupDelay = 0;
  int bsDelay = 0;

  /* check decoder handle */
  if (self != NULL) {
    pConcealData = &self->concealCommonData;
    hSbrDec = self->hSbrDecoder;
    hDrcInfo = self->hDrcInfo;
    hPcmDmx = self->hPcmUtils;
  }


  /* Get current method/delay */
  backupMethod = CConcealment_GetMethod(pConcealData);
  backupDelay  = CConcealment_GetDelay(pConcealData);

  /* Be sure to set AAC and SBR concealment method simultaneously! */
  errorStatus =
    CConcealment_SetParams(
      pConcealData,
      (int)method,                         // concealMethod
      AACDEC_CONCEAL_PARAM_NOT_SPECIFIED,  // concealFadeOutSlope
      AACDEC_CONCEAL_PARAM_NOT_SPECIFIED,  // concealFadeInSlope
      AACDEC_CONCEAL_PARAM_NOT_SPECIFIED,  // concealMuteRelease
      AACDEC_CONCEAL_PARAM_NOT_SPECIFIED   // concealComfNoiseLevel
    );
  if ( (errorStatus != AAC_DEC_OK)
    && (errorStatus != AAC_DEC_INVALID_HANDLE) ) {
    goto bail;
  }

  /* Get new delay */
  bsDelay = CConcealment_GetDelay(pConcealData);

  {
    SBR_ERROR sbrErr = SBRDEC_OK;

    /* set SBR bitstream delay */
    sbrErr = sbrDecoder_SetParam (
      hSbrDec,
      SBR_SYSTEM_BITSTREAM_DELAY,
      bsDelay
    );

    switch (sbrErr) {
    case SBRDEC_OK:
    case SBRDEC_NOT_INITIALIZED:
      if (self != NULL) {
        /* save the param value and set later
           (when SBR has been initialized) */
        self->sbrParams.bsDelay = bsDelay;
      }
      break;
    default:
      errorStatus = AAC_DEC_SET_PARAM_FAIL;
      goto bail;
    }
  }

  errorStatus =
    aacDecoder_drcSetParam (
      hDrcInfo,
      DRC_BS_DELAY,
      bsDelay
    );
  if ( (errorStatus != AAC_DEC_OK)
    && (errorStatus != AAC_DEC_INVALID_HANDLE) ) {
    goto bail;
  }

  if (errorStatus == AAC_DEC_OK) {
    PCMDMX_ERROR err =
      pcmDmx_SetParam (
        hPcmDmx,
        DMX_BS_DATA_DELAY,
        bsDelay
      );
    switch (err) {
    case PCMDMX_INVALID_HANDLE:
      errorStatus = AAC_DEC_INVALID_HANDLE;
    case PCMDMX_OK:
      break;
    default:
      errorStatus = AAC_DEC_SET_PARAM_FAIL;
      goto bail;
    }
  }


bail:
  if ( (errorStatus != AAC_DEC_OK)
    && (errorStatus != AAC_DEC_INVALID_HANDLE) )
  {
    /* Revert to the initial state */
    CConcealment_SetParams (
        pConcealData,
        (int)backupMethod,
        AACDEC_CONCEAL_PARAM_NOT_SPECIFIED,
        AACDEC_CONCEAL_PARAM_NOT_SPECIFIED,
        AACDEC_CONCEAL_PARAM_NOT_SPECIFIED,
        AACDEC_CONCEAL_PARAM_NOT_SPECIFIED
      );
    /* Revert SBR bitstream delay */
    sbrDecoder_SetParam (
        hSbrDec,
        SBR_SYSTEM_BITSTREAM_DELAY,
        backupDelay
      );
    /* Revert DRC bitstream delay */
    aacDecoder_drcSetParam (
        hDrcInfo,
        DRC_BS_DELAY,
        backupDelay
      );
    /* Revert PCM mixdown bitstream delay */
    pcmDmx_SetParam (
        hPcmDmx,
        DMX_BS_DATA_DELAY,
        backupDelay
      );
  }

  return errorStatus;
}


LINKSPEC_CPP AAC_DECODER_ERROR
aacDecoder_SetParam ( const HANDLE_AACDECODER  self,   /*!< Handle of the decoder instance */
                      const AACDEC_PARAM       param,  /*!< Parameter to set               */
                      const INT                value)  /*!< Parameter valued               */
{
  AAC_DECODER_ERROR errorStatus = AAC_DEC_OK;
  CConcealParams  *pConcealData = NULL;
  HANDLE_AAC_DRC hDrcInfo = NULL;
  HANDLE_PCM_DOWNMIX hPcmDmx = NULL;
  TDLimiterPtr hPcmTdl = NULL;

  /* check decoder handle */
  if (self != NULL) {
    pConcealData = &self->concealCommonData;
    hDrcInfo = self->hDrcInfo;
    hPcmDmx = self->hPcmUtils;
    hPcmTdl = self->hLimiter;
  } else {
    errorStatus = AAC_DEC_INVALID_HANDLE;
  }

  /* configure the subsystems */
  switch (param)
  {
  case AAC_PCM_OUTPUT_INTERLEAVED:
    if (value < 0 || value > 1) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    self->outputInterleaved = value;
    break;

  case AAC_PCM_MIN_OUTPUT_CHANNELS:
    if (value < -1 || value > (8)) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    {
      PCMDMX_ERROR err;

      err = pcmDmx_SetParam (
              hPcmDmx,
              MIN_NUMBER_OF_OUTPUT_CHANNELS,
              value );

      switch (err) {
      case PCMDMX_OK:
        break;
      case PCMDMX_INVALID_HANDLE:
        return AAC_DEC_INVALID_HANDLE;
      default:
        return AAC_DEC_SET_PARAM_FAIL;
      }
    }
    break;

  case AAC_PCM_MAX_OUTPUT_CHANNELS:
    if (value < -1 || value > (8)) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    {
      PCMDMX_ERROR err;

      err = pcmDmx_SetParam (
              hPcmDmx,
              MAX_NUMBER_OF_OUTPUT_CHANNELS,
              value );

      switch (err) {
      case PCMDMX_OK:
        break;
      case PCMDMX_INVALID_HANDLE:
        return AAC_DEC_INVALID_HANDLE;
      default:
        return AAC_DEC_SET_PARAM_FAIL;
      }
    }
    break;

  case AAC_PCM_DUAL_CHANNEL_OUTPUT_MODE:
    {
      PCMDMX_ERROR err;

      err = pcmDmx_SetParam (
              hPcmDmx,
              DMX_DUAL_CHANNEL_MODE,
              value );

      switch (err) {
      case PCMDMX_OK:
        break;
      case PCMDMX_INVALID_HANDLE:
        return AAC_DEC_INVALID_HANDLE;
      default:
        return AAC_DEC_SET_PARAM_FAIL;
      }
    }
    break;


  case AAC_PCM_LIMITER_ENABLE:
    if (value < -1 || value > 1) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }
    self->limiterEnableUser = value;
    break;

  case AAC_PCM_LIMITER_ATTACK_TIME:
    if (value <= 0) {  /* module function converts value to unsigned */
      return AAC_DEC_SET_PARAM_FAIL;
    }
    switch (setLimiterAttack(hPcmTdl, value)) {
    case TDLIMIT_OK:
      break;
    case TDLIMIT_INVALID_HANDLE:
      return AAC_DEC_INVALID_HANDLE;
    case TDLIMIT_INVALID_PARAMETER:
    default:
      return AAC_DEC_SET_PARAM_FAIL;
    }
    break;

  case AAC_PCM_LIMITER_RELEAS_TIME:
    if (value <= 0) {  /* module function converts value to unsigned */
      return AAC_DEC_SET_PARAM_FAIL;
    }
    switch (setLimiterRelease(hPcmTdl, value)) {
    case TDLIMIT_OK:
      break;
    case TDLIMIT_INVALID_HANDLE:
      return AAC_DEC_INVALID_HANDLE;
    case TDLIMIT_INVALID_PARAMETER:
    default:
      return AAC_DEC_SET_PARAM_FAIL;
    }
    break;

  case AAC_PCM_OUTPUT_CHANNEL_MAPPING:
    switch (value) {
      case 0:
        if (self != NULL) {
          self->channelOutputMapping = channelMappingTablePassthrough;
        }
        break;
      case 1:
        if (self != NULL) {
          self->channelOutputMapping = channelMappingTableWAV;
        }
        break;
      default:
        errorStatus = AAC_DEC_SET_PARAM_FAIL;
        break;
    }
    break;


  case AAC_QMF_LOWPOWER:
    if (value < -1 || value > 1) {
      return AAC_DEC_SET_PARAM_FAIL;
    }
    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }

    /**
     * Set QMF mode (might be overriden)
     *  0:HQ (complex)
     *  1:LP (partially complex)
     */
    self->qmfModeUser = (QMF_MODE)value;
    break;


  case AAC_DRC_ATTENUATION_FACTOR:
    /* DRC compression factor (where 0 is no and 127 is max compression) */
    errorStatus =
      aacDecoder_drcSetParam (
        hDrcInfo,
        DRC_CUT_SCALE,
        value
      );
    break;

  case AAC_DRC_BOOST_FACTOR:
    /* DRC boost factor (where 0 is no and 127 is max boost) */
    errorStatus =
      aacDecoder_drcSetParam (
        hDrcInfo,
        DRC_BOOST_SCALE,
        value
      );
    break;

  case AAC_DRC_REFERENCE_LEVEL:
    /* DRC reference level quantized in 0.25dB steps using values [0..127] it is '-' for analog scaling */
    errorStatus =
      aacDecoder_drcSetParam (
        hDrcInfo,
        TARGET_REF_LEVEL,
        value
      );
    break;

  case AAC_DRC_HEAVY_COMPRESSION:
    /* Don't need to overwrite cut/boost values */
    errorStatus =
      aacDecoder_drcSetParam (
        hDrcInfo,
        APPLY_HEAVY_COMPRESSION,
        value
      );
    break;


  case AAC_TPDEC_CLEAR_BUFFER:
    transportDec_SetParam(self->hInput, TPDEC_PARAM_RESET, 1);
    self->streamInfo.numLostAccessUnits = 0;
    self->streamInfo.numBadBytes = 0;
    self->streamInfo.numTotalBytes = 0;
    /* aacDecoder_SignalInterruption(self); */
    break;

  case AAC_CONCEAL_METHOD:
    /* Changing the concealment method can introduce additional bitstream delay. And
       that in turn affects sub libraries and modules which makes the whole thing quite
       complex.  So the complete changing routine is packed into a helper function which
       keeps all modules and libs in a consistent state even in the case an error occures. */
    errorStatus = setConcealMethod ( self, value );
    break;

  default:
    return AAC_DEC_SET_PARAM_FAIL;
  }  /* switch(param) */

  return (errorStatus);
}


LINKSPEC_CPP HANDLE_AACDECODER aacDecoder_Open(TRANSPORT_TYPE transportFmt, UINT nrOfLayers)
{
  AAC_DECODER_INSTANCE *aacDec = NULL;
  HANDLE_TRANSPORTDEC pIn;
  int err = 0;

  /* Allocate transport layer struct. */
  pIn = transportDec_Open(transportFmt, TP_FLAG_MPEG4);
  if (pIn == NULL) {
    return NULL;
  }

  transportDec_SetParam(pIn, TPDEC_PARAM_IGNORE_BUFFERFULLNESS, 1);

  /* Allocate AAC decoder core struct. */
  aacDec = CAacDecoder_Open(transportFmt);

  if (aacDec == NULL) {
    transportDec_Close(&pIn);
    goto bail;
  }
  aacDec->hInput = pIn;

  aacDec->nrOfLayers = nrOfLayers;

  aacDec->channelOutputMapping = channelMappingTableWAV;

  /* Register Config Update callback. */
  transportDec_RegisterAscCallback(pIn, aacDecoder_ConfigCallback, (void*)aacDec);

  /* open SBR decoder */
  if ( SBRDEC_OK != sbrDecoder_Open ( &aacDec->hSbrDecoder )) {
    err = -1;
    goto bail;
  }
  aacDec->qmfModeUser = NOT_DEFINED;
  transportDec_RegisterSbrCallback(aacDec->hInput, (cbSbr_t)sbrDecoder_Header, (void*)aacDec->hSbrDecoder);


  pcmDmx_Open( &aacDec->hPcmUtils );
  if (aacDec->hPcmUtils == NULL) {
    err = -1;
    goto bail;
  }

  aacDec->hLimiter = createLimiter(TDL_ATTACK_DEFAULT_MS, TDL_RELEASE_DEFAULT_MS, SAMPLE_MAX, (8), 96000);
  if (NULL == aacDec->hLimiter) {
    err = -1;
    goto bail;
  }
  aacDec->limiterEnableUser = (UCHAR)-1;
  aacDec->limiterEnableCurr = 0;



  /* Assure that all modules have same delay */
  if ( setConcealMethod(aacDec, CConcealment_GetMethod(&aacDec->concealCommonData)) ) {
    err = -1;
    goto bail;
  }

bail:
  if (err == -1) {
    aacDecoder_Close(aacDec);
    aacDec = NULL;
  }
  return aacDec;
}

LINKSPEC_CPP AAC_DECODER_ERROR aacDecoder_Fill(
        HANDLE_AACDECODER   self,
        UCHAR              *pBuffer[],
        const UINT          bufferSize[],
        UINT               *pBytesValid
        )
{
  TRANSPORTDEC_ERROR tpErr;
  /* loop counter for layers; if not TT_MP4_RAWPACKETS used as index for only 
     available layer                                                           */
  INT layer      = 0;
  INT nrOfLayers = self->nrOfLayers;

  {
    for (layer = 0; layer < nrOfLayers; layer++){
      {
        tpErr = transportDec_FillData( self->hInput, pBuffer[layer], bufferSize[layer], &pBytesValid[layer], layer );
        if (tpErr != TRANSPORTDEC_OK) {
          return AAC_DEC_UNKNOWN;  /* Must be an internal error */
        }
      }
    }
  }

  return AAC_DEC_OK;
}


static void aacDecoder_SignalInterruption(HANDLE_AACDECODER self)
{
  CAacDecoder_SignalInterruption(self);

  if ( self->hSbrDecoder != NULL ) {
    sbrDecoder_SetParam(self->hSbrDecoder, SBR_BS_INTERRUPTION, 0);
  }
}

static void aacDecoder_UpdateBitStreamCounters(CStreamInfo *pSi, HANDLE_FDK_BITSTREAM hBs, int nBits, AAC_DECODER_ERROR ErrorStatus)
{
  /* calculate bit difference (amount of bits moved forward) */
  nBits = nBits - FDKgetValidBits(hBs);

  /* Note: The amount of bits consumed might become negative when parsing a
     bit stream with several sub frames, and we find out at the last sub frame
     that the total frame length does not match the sum of sub frame length. 
     If this happens, the transport decoder might want to rewind to the supposed
     ending of the transport frame, and this position might be before the last
     access unit beginning. */

  /* Calc bitrate. */
  if (pSi->frameSize > 0) {
    pSi->bitRate = (nBits * pSi->sampleRate)/pSi->frameSize;
  }

  /* bit/byte counters */
  {
    int nBytes;

    nBytes = nBits>>3;
    pSi->numTotalBytes += nBytes;
    if (IS_OUTPUT_VALID(ErrorStatus)) {
      pSi->numTotalAccessUnits++;
    }
    if (IS_DECODE_ERROR(ErrorStatus)) {
      pSi->numBadBytes += nBytes;
      pSi->numBadAccessUnits++;
    }
  }
}

static INT aacDecoder_EstimateNumberOfLostFrames(HANDLE_AACDECODER self)
{
  INT n;

  transportDec_GetMissingAccessUnitCount( &n, self->hInput);

  return n;
}

LINKSPEC_CPP AAC_DECODER_ERROR aacDecoder_DecodeFrame(
        HANDLE_AACDECODER  self,
        INT_PCM           *pTimeData_extern,
        const INT          timeDataSize_extern,
        const UINT         flags)
{
    AAC_DECODER_ERROR ErrorStatus;
    INT layer;
    INT nBits;
    INT interleaved = self->outputInterleaved;
    HANDLE_FDK_BITSTREAM hBs;
    int fTpInterruption = 0;  /* Transport originated interruption detection. */
    int fTpConceal = 0;       /* Transport originated concealment. */
    INT_PCM *pTimeData = NULL;
    INT timeDataSize = 0;


    if (self == NULL) {
      return AAC_DEC_INVALID_HANDLE;
    }

    pTimeData = self->pcmOutputBuffer;
    timeDataSize = sizeof(self->pcmOutputBuffer)/sizeof(*self->pcmOutputBuffer);

    if (flags & AACDEC_INTR) {
      self->streamInfo.numLostAccessUnits = 0;
    }

    hBs = transportDec_GetBitstream(self->hInput, 0);

    /* Get current bits position for bitrate calculation. */
    nBits = FDKgetValidBits(hBs);
    if (! (flags & (AACDEC_CONCEAL | AACDEC_FLUSH) ) )
    {
      TRANSPORTDEC_ERROR err;

      for(layer = 0; layer < self->nrOfLayers; layer++)
      {
        err = transportDec_ReadAccessUnit(self->hInput, layer);
        if (err != TRANSPORTDEC_OK) {
          switch (err) {
          case TRANSPORTDEC_NOT_ENOUGH_BITS:
            ErrorStatus = AAC_DEC_NOT_ENOUGH_BITS;
            goto bail;
          case TRANSPORTDEC_SYNC_ERROR:
            self->streamInfo.numLostAccessUnits = aacDecoder_EstimateNumberOfLostFrames(self);
            fTpInterruption = 1;
            break;
          case TRANSPORTDEC_NEED_TO_RESTART:
            ErrorStatus = AAC_DEC_NEED_TO_RESTART;
            goto bail;
          case TRANSPORTDEC_CRC_ERROR:
            fTpConceal = 1;
            break;
          default:
            ErrorStatus = AAC_DEC_UNKNOWN;
            goto bail;
          }
        }
      }
    } else {
      if (self->streamInfo.numLostAccessUnits > 0) {
        self->streamInfo.numLostAccessUnits--;
      }
    }

    /* Signal bit stream interruption to other modules if required. */
    if ( fTpInterruption || (flags & (AACDEC_INTR|AACDEC_CLRHIST)) )
    {
      sbrDecoder_SetParam(self->hSbrDecoder, SBR_CLEAR_HISTORY, (flags&AACDEC_CLRHIST));
      aacDecoder_SignalInterruption(self);
      if ( ! (flags & AACDEC_INTR) ) {
        ErrorStatus = AAC_DEC_TRANSPORT_SYNC_ERROR;
        goto bail;
      }
    }

    /* Empty bit buffer in case of flush request. */
    if (flags & AACDEC_FLUSH)
    {
      transportDec_SetParam(self->hInput, TPDEC_PARAM_RESET, 1);
      self->streamInfo.numLostAccessUnits = 0;
      self->streamInfo.numBadBytes = 0;
      self->streamInfo.numTotalBytes = 0;
    }
    /* Reset the output delay field. The modules will add their figures one after another. */
    self->streamInfo.outputDelay = 0;

    if (self->limiterEnableUser==(UCHAR)-1) {
      /* Enbale limiter for all non-lowdelay AOT's. */
      self->limiterEnableCurr = ( self->flags & (AC_LD|AC_ELD) ) ? 0 : 1;
    }
    else {
      /* Use limiter configuration as requested. */
      self->limiterEnableCurr = self->limiterEnableUser;
    }
    /* reset limiter gain on a per frame basis */
    self->extGain[0] = FL2FXCONST_DBL(1.0f/(float)(1<<TDL_GAIN_SCALING));


    ErrorStatus = CAacDecoder_DecodeFrame(self,
                                          flags | (fTpConceal ? AACDEC_CONCEAL : 0),
                                          pTimeData,
                                          timeDataSize,
                                          interleaved);

    if (!(flags & (AACDEC_CONCEAL|AACDEC_FLUSH))) {
      TRANSPORTDEC_ERROR tpErr;
      tpErr = transportDec_EndAccessUnit(self->hInput);
      if (tpErr != TRANSPORTDEC_OK) {
        self->frameOK = 0;
      }
    }

    /* If the current pTimeData does not contain a valid signal, there nothing else we can do, so bail. */
    if ( ! IS_OUTPUT_VALID(ErrorStatus) ) {
      goto bail;
    }

    {
      /* Export data into streaminfo structure */
      self->streamInfo.sampleRate = self->streamInfo.aacSampleRate;
      self->streamInfo.frameSize  = self->streamInfo.aacSamplesPerFrame;
    }
    self->streamInfo.numChannels = self->streamInfo.aacNumChannels;



    CAacDecoder_SyncQmfMode(self);

/* sbr decoder */

    if (ErrorStatus || (flags & AACDEC_CONCEAL) || self->pAacDecoderStaticChannelInfo[0]->concealmentInfo.concealState > ConcealState_FadeIn)
    {
      self->frameOK = 0;  /* if an error has occured do concealment in the SBR decoder too */
    }

    if (self->sbrEnabled)
    {
      SBR_ERROR sbrError = SBRDEC_OK;
      int chIdx, numCoreChannel = self->streamInfo.numChannels;
      int chOutMapIdx = ((self->chMapIndex==0) && (numCoreChannel<7)) ? numCoreChannel : self->chMapIndex;

      /* set params */
      sbrDecoder_SetParam ( self->hSbrDecoder,
                            SBR_SYSTEM_BITSTREAM_DELAY,
                            self->sbrParams.bsDelay);
      sbrDecoder_SetParam ( self->hSbrDecoder,
                            SBR_FLUSH_DATA,
                            (flags & AACDEC_FLUSH) );

      if ( self->streamInfo.aot == AOT_ER_AAC_ELD ) {
        /* Configure QMF */
        sbrDecoder_SetParam ( self->hSbrDecoder,
                              SBR_LD_QMF_TIME_ALIGN,
                              (self->flags & AC_LD_MPS) ? 1 : 0 );
      }

      {
        PCMDMX_ERROR dmxErr;
        INT  maxOutCh = 0;

        dmxErr = pcmDmx_GetParam(self->hPcmUtils, MAX_NUMBER_OF_OUTPUT_CHANNELS, &maxOutCh);
        if ( (dmxErr == PCMDMX_OK) && (maxOutCh == 1) ) {
          /* Disable PS processing if we have to create a mono output signal. */
          self->psPossible = 0;
        }
      }


      /* apply SBR processing */
      sbrError = sbrDecoder_Apply ( self->hSbrDecoder,
                                    pTimeData,
                                   &self->streamInfo.numChannels,
                                   &self->streamInfo.sampleRate,
                                    self->channelOutputMapping[chOutMapIdx],
                                    interleaved,
                                    self->frameOK,
                                   &self->psPossible);


     if (sbrError == SBRDEC_OK) {
       #define UPS_SCALE  2  /* Maximum upsampling factor is 4 (CELP+SBR) */
       FIXP_DBL  upsampleFactor = FL2FXCONST_DBL(1.0f/(1<<UPS_SCALE));

       /* Update data in streaminfo structure. Assume that the SBR upsampling factor is either 1 or 2 */
       self->flags |= AC_SBR_PRESENT;
       if (self->streamInfo.aacSampleRate != self->streamInfo.sampleRate) {
         if (self->streamInfo.frameSize == 768) {
           upsampleFactor = FL2FXCONST_DBL(8.0f/(3<<UPS_SCALE));
         } else {
           upsampleFactor = FL2FXCONST_DBL(2.0f/(1<<UPS_SCALE));
         }
       }
       /* Apply upsampling factor to both the core frame length and the core delay */
       self->streamInfo.frameSize    =       (INT)fMult((FIXP_DBL)self->streamInfo.aacSamplesPerFrame<<UPS_SCALE, upsampleFactor);
       self->streamInfo.outputDelay  = (UINT)(INT)fMult((FIXP_DBL)self->streamInfo.outputDelay<<UPS_SCALE, upsampleFactor);
       self->streamInfo.outputDelay += sbrDecoder_GetDelay( self->hSbrDecoder );

       if (self->psPossible) {
         self->flags |= AC_PS_PRESENT;
       }
       for (chIdx = numCoreChannel; chIdx < self->streamInfo.numChannels; chIdx+=1) {
         self->channelType[chIdx] = ACT_FRONT;
         self->channelIndices[chIdx] = chIdx;
       }
     }
   }


    {
    INT pcmLimiterScale = 0;
    PCMDMX_ERROR dmxErr = PCMDMX_OK;
    if ( flags & (AACDEC_INTR | AACDEC_CLRHIST) ) {
      /* delete data from the past (e.g. mixdown coeficients) */
      pcmDmx_Reset( self->hPcmUtils, PCMDMX_RESET_BS_DATA );
    }
    /* do PCM post processing */
    dmxErr = pcmDmx_ApplyFrame (
            self->hPcmUtils,
            pTimeData,
            self->streamInfo.frameSize,
           &self->streamInfo.numChannels,
            interleaved,
            self->channelType,
            self->channelIndices,
            self->channelOutputMapping,
            (self->limiterEnableCurr) ? &pcmLimiterScale : NULL
      );
    if ( (ErrorStatus == AAC_DEC_OK)
      && (dmxErr == PCMDMX_INVALID_MODE) ) {
      /* Announce the framework that the current combination of channel configuration and downmix
       * settings are not know to produce a predictable behavior and thus maybe produce strange output. */
      ErrorStatus = AAC_DEC_DECODE_FRAME_ERROR;
    }

    if ( flags & AACDEC_CLRHIST ) {
      /* Delete the delayed signal. */
      resetLimiter(self->hLimiter);
    }
    if (self->limiterEnableCurr)
    {
      /* Set actual signal parameters */
      setLimiterNChannels(self->hLimiter, self->streamInfo.numChannels);
      setLimiterSampleRate(self->hLimiter, self->streamInfo.sampleRate);

      applyLimiter(
              self->hLimiter,
              pTimeData,
              self->extGain,
             &pcmLimiterScale,
              1,
              self->extGainDelay,
              self->streamInfo.frameSize
              );

      /* Announce the additional limiter output delay */
      self->streamInfo.outputDelay += getLimiterDelay(self->hLimiter);
    }
    }


    /* Signal interruption to take effect in next frame. */
    if ( flags & AACDEC_FLUSH ) {
      aacDecoder_SignalInterruption(self);
    }

    /* Update externally visible copy of flags */
    self->streamInfo.flags = self->flags;

bail:

    /* Update Statistics */
    aacDecoder_UpdateBitStreamCounters(&self->streamInfo, hBs, nBits, ErrorStatus);

    /* Check whether external output buffer is large enough. */
    if (timeDataSize_extern < self->streamInfo.numChannels*self->streamInfo.frameSize) {
      ErrorStatus = AAC_DEC_OUTPUT_BUFFER_TOO_SMALL;
    }

    /* Update external output buffer. */
    if ( IS_OUTPUT_VALID(ErrorStatus) ) {
      FDKmemcpy(pTimeData_extern, pTimeData, self->streamInfo.numChannels*self->streamInfo.frameSize*sizeof(*pTimeData));
    }
    else {
      FDKmemclear(pTimeData_extern, timeDataSize_extern*sizeof(*pTimeData_extern));
    }

    return ErrorStatus;
}

LINKSPEC_CPP void aacDecoder_Close ( HANDLE_AACDECODER self )
{
  if (self == NULL)
    return;


  if (self->hLimiter != NULL) {
    destroyLimiter(self->hLimiter);
  }

  if (self->hPcmUtils != NULL) {
    pcmDmx_Close( &self->hPcmUtils );
  }



  if (self->hSbrDecoder != NULL) {
    sbrDecoder_Close(&self->hSbrDecoder);
  }

  if (self->hInput != NULL) {
    transportDec_Close(&self->hInput);
  }

  CAacDecoder_Close(self);
}


LINKSPEC_CPP CStreamInfo* aacDecoder_GetStreamInfo ( HANDLE_AACDECODER self )
{
  return CAacDecoder_GetStreamInfo(self);
}

LINKSPEC_CPP INT aacDecoder_GetLibInfo ( LIB_INFO *info )
{
  int i;

  if (info == NULL) {
    return -1;
  }

  sbrDecoder_GetLibInfo( info );
  transportDec_GetLibInfo( info );
  FDK_toolsGetLibInfo( info );
  pcmDmx_GetLibInfo( info );

  /* search for next free tab */
  for (i = 0; i < FDK_MODULE_LAST; i++) {
    if (info[i].module_id == FDK_NONE) break;
  }
  if (i == FDK_MODULE_LAST) {
    return -1;
  }
  info += i;

  info->module_id = FDK_AACDEC;
  /* build own library info */
  info->version = LIB_VERSION(AACDECODER_LIB_VL0, AACDECODER_LIB_VL1, AACDECODER_LIB_VL2);
  LIB_VERSION_STRING(info);
  info->build_date = AACDECODER_LIB_BUILD_DATE;
  info->build_time = AACDECODER_LIB_BUILD_TIME;
  info->title = AACDECODER_LIB_TITLE;

  /* Set flags */
  info->flags = 0
      | CAPF_AAC_LC
      | CAPF_ER_AAC_SCAL
      | CAPF_AAC_VCB11
      | CAPF_AAC_HCR
      | CAPF_AAC_RVLC
      | CAPF_ER_AAC_LD
      | CAPF_ER_AAC_ELD
      | CAPF_AAC_CONCEALMENT
      | CAPF_AAC_DRC

      | CAPF_AAC_MPEG4

      | CAPF_AAC_DRM_BSFORMAT

      | CAPF_AAC_1024
      | CAPF_AAC_960

      | CAPF_AAC_512

      | CAPF_AAC_480

      ;
  /* End of flags */

  return 0;
}




