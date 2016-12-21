
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

   Author(s):   Josef Hoepfl
   Description:

******************************************************************************/


/*!
  \page default General Overview of the AAC Decoder Implementation

  The main entry point to decode a AAC frame is CAacDecoder_DecodeFrame(). It handles the different
  transport multiplexes and bitstream formats supported by this implementation. It extracts the
  AAC_raw_data_blocks from these bitstreams to further process then in the actual decoding stages.

  Note: Click on a function of file in the above image to see details about the function. Also note, that
  this is just an overview of the most important functions and not a complete call graph.

  <h2>1 Bitstream deformatter</h2>
  The basic bit stream parser function CChannelElement_Read() is called. It uses other subcalls in order
  to parse and unpack the bitstreams. Note, that this includes huffmann decoding of the coded spectral data.
  This operation can be computational significant specifically at higher bitrates. Optimization is likely in
  CBlock_ReadSpectralData().

  The bitstream deformatter also includes many bitfield operations. Profiling on the target will determine
  required optimizations.

  <h2>2 Actual decoding to retain the time domain output</h2>
  The basic bitstream deformatter function CChannelElement_Decode() for CPE elements and SCE elements are called.
  Except for the stereo processing (2.1) which is only used for CPE elements, the function calls for CPE or SCE
  are similar, except that CPE always processes to independent channels while SCE only processes one channel.

  Often there is the distinction between long blocks and short blocks. However, computational expensive functions
  that ususally require optimization are being shared by these two groups,

  <h3>2.1 Stereo processing for CPE elements</h3>
  CChannelPairElement_Decode() first calles the joint stereo  tools in stereo.cpp when required.

  <h3>2.2 Scaling of spectral data</h3>
  CBlock_ScaleSpectralData().

  <h3>2.3 Apply additional coding tools</h3>
  ApplyTools() calles the PNS tools in case of MPEG-4 bitstreams, and TNS filtering CTns_Apply() for MPEG-2 and MPEG-4 bitstreams.
  The function TnsFilterIIR() which is called by CTns_Apply() (2.3.1) might require some optimization.

  <h2>3 Frequency-To-Time conversion</h3>
  The filterbank is called using CBlock_FrequencyToTime() using the MDCT module from the FDK Tools

*/



#include "aacdecoder.h"

#include "aac_rom.h"
#include "aac_ram.h"
#include "channel.h"
#include "FDK_audio.h"

#include "FDK_tools_rom.h"

  #include "aacdec_pns.h"

  #include "sbrdecoder.h"




  #include "aacdec_hcr.h"
  #include "rvlc.h"


#include "tpdec_lib.h"

#include "conceal.h"

  #include "FDK_crc.h"


void CAacDecoder_SyncQmfMode(HANDLE_AACDECODER self)
{

  /* Assign user requested mode */
  self->qmfModeCurr = self->qmfModeUser;

  if ( self->qmfModeCurr == NOT_DEFINED )
  {
    if ( (IS_LOWDELAY(self->streamInfo.aot) && (self->flags & AC_MPS_PRESENT))
      || ( (self->streamInfo.aacNumChannels == 1)
        && ( (CAN_DO_PS(self->streamInfo.aot) && !(self->flags & AC_MPS_PRESENT))
          || (  IS_USAC(self->streamInfo.aot) &&  (self->flags & AC_MPS_PRESENT)) ) ) )
    {
      self->qmfModeCurr = MODE_HQ;
    } else {
      self->qmfModeCurr = MODE_LP;
    }
  }


  /* Set SBR to current QMF mode. Error does not matter. */
  sbrDecoder_SetParam(self->hSbrDecoder, SBR_QMF_MODE, (self->qmfModeCurr == MODE_LP));
  self->psPossible = ((CAN_DO_PS(self->streamInfo.aot) && self->streamInfo.aacNumChannels == 1 && ! (self->flags & AC_MPS_PRESENT))) && self->qmfModeCurr == MODE_HQ ;
  FDK_ASSERT( ! ( (self->flags & AC_MPS_PRESENT) && self->psPossible ) );
}

void CAacDecoder_SignalInterruption(HANDLE_AACDECODER self)
{
}

/*!
  \brief Reset ancillary data struct. Call before parsing a new frame.

  \ancData Pointer to ancillary data structure

  \return  Error code
*/
static AAC_DECODER_ERROR CAacDecoder_AncDataReset(CAncData *ancData)
{
  int i;
  for (i=0; i<8; i++)
  {
    ancData->offset[i] = 0;
  }
  ancData->nrElements = 0;

  return AAC_DEC_OK;
}

/*!
  \brief Initialize ancillary buffer

  \ancData Pointer to ancillary data structure
  \buffer Pointer to (external) anc data buffer
  \size Size of the buffer pointed on by buffer in bytes

  \return  Error code
*/
AAC_DECODER_ERROR CAacDecoder_AncDataInit(CAncData *ancData, unsigned char *buffer, int size)
{
  if (size >= 0) {
    ancData->buffer = buffer;
    ancData->bufferSize = size;

    CAacDecoder_AncDataReset(ancData);

    return AAC_DEC_OK;
  }

  return AAC_DEC_ANC_DATA_ERROR;
}

/*!
  \brief Get one ancillary data element

  \ancData Pointer to ancillary data structure
  \index Index of the anc data element to get
  \ptr Pointer to a buffer receiving a pointer to the requested anc data element
  \size Pointer to a buffer receiving the length of the requested anc data element in bytes

  \return  Error code
*/
AAC_DECODER_ERROR CAacDecoder_AncDataGet(CAncData *ancData, int index, unsigned char **ptr, int *size)
{
  AAC_DECODER_ERROR error = AAC_DEC_OK;

  *ptr  = NULL;
  *size = 0;

  if (index >= 0 && index < 8 && index < ancData->nrElements)
  {
    *ptr  = &ancData->buffer[ancData->offset[index]];
    *size = ancData->offset[index+1] - ancData->offset[index];
  }

  return error;
}


/*!
  \brief Parse ancillary data

  \ancData Pointer to ancillary data structure
  \hBs Handle to FDK bitstream
  \ancBytes Length of ancillary data to read from the bitstream

  \return  Error code
*/
static
AAC_DECODER_ERROR CAacDecoder_AncDataParse (
                                             CAncData *ancData,
                                             HANDLE_FDK_BITSTREAM hBs,
                                             const int ancBytes )
{
  AAC_DECODER_ERROR error = AAC_DEC_OK;
  int readBytes = 0;

  if (ancData->buffer != NULL)
  {
    if (ancBytes > 0) {
      /* write ancillary data to external buffer */
      int offset = ancData->offset[ancData->nrElements];

      if ((offset + ancBytes) > ancData->bufferSize)
      {
        error = AAC_DEC_TOO_SMALL_ANC_BUFFER;
      }
      else if (ancData->nrElements >= 8-1)
      {
        error = AAC_DEC_TOO_MANY_ANC_ELEMENTS;
      }
      else
      {
        int i;

        for (i = 0; i < ancBytes; i++) {
          ancData->buffer[i+offset] = FDKreadBits(hBs, 8);
          readBytes++;
        }

        ancData->nrElements++;
        ancData->offset[ancData->nrElements] = ancBytes + ancData->offset[ancData->nrElements-1];
      }
    }
  }

  readBytes = ancBytes - readBytes;

  if (readBytes > 0) {
    /* skip data */
    FDKpushFor(hBs, readBytes<<3);
  }

  return error;
}

/*!
  \brief Read Stream Data Element

  \bs Bitstream Handle

  \return  Error code
*/
static AAC_DECODER_ERROR CDataStreamElement_Read (
                                                  HANDLE_AACDECODER    self,
                                                  HANDLE_FDK_BITSTREAM bs,
                                                  UCHAR    *elementInstanceTag,
                                                  UINT      alignmentAnchor )
{
  HANDLE_TRANSPORTDEC  pTp;
  CAncData *ancData;
  AAC_DECODER_ERROR error = AAC_DEC_OK;
  UINT dataStart, dseBits;
  int dataByteAlignFlag, count;

  FDK_ASSERT(self != NULL);

  ancData = &self->ancData;
  pTp = self->hInput;

  int crcReg = transportDec_CrcStartReg(pTp, 0);

  /* Element Instance Tag */
  *elementInstanceTag = FDKreadBits(bs,4);
  /* Data Byte Align Flag */
  dataByteAlignFlag = FDKreadBits(bs,1);

  count = FDKreadBits(bs,8);

  if (count == 255) {
    count += FDKreadBits(bs,8); /* EscCount */
  }
  dseBits = count*8;

  if (dataByteAlignFlag) {
    FDKbyteAlign(bs, alignmentAnchor);
  }

  dataStart = FDKgetValidBits(bs);

  error = CAacDecoder_AncDataParse(ancData, bs, count);
  transportDec_CrcEndReg(pTp, crcReg);

  {
    /* Move to the beginning of the data junk */
    FDKpushBack(bs, dataStart-FDKgetValidBits(bs));

    /* Read Anc data if available */
    aacDecoder_drcMarkPayload( self->hDrcInfo, bs, DVB_DRC_ANC_DATA );
  }

  {
    PCMDMX_ERROR dmxErr = PCMDMX_OK;

    /* Move to the beginning of the data junk */
    FDKpushBack(bs, dataStart-FDKgetValidBits(bs));

    /* Read DMX meta-data */
    dmxErr = pcmDmx_Parse (
                     self->hPcmUtils,
                     bs,
                     dseBits,
                     0 /* not mpeg2 */ );
    }

  /* Move to the very end of the element. */
  FDKpushBiDirectional(bs, FDKgetValidBits(bs)-dataStart+dseBits);

  return error;
}

#ifdef TP_PCE_ENABLE
/*!
  \brief Read Program Config Element

  \bs Bitstream Handle
  \pTp Transport decoder handle for CRC handling
  \pce Pointer to PCE buffer
  \channelConfig Current channel configuration
  \alignAnchor Anchor for byte alignment

  \return  PCE status (-1: fail, 0: no new PCE, 1: PCE updated, 2: PCE updated need re-config).
*/
static int CProgramConfigElement_Read (
    HANDLE_FDK_BITSTREAM bs,
    HANDLE_TRANSPORTDEC  pTp,
    CProgramConfig      *pce,
    const UINT           channelConfig,
    const UINT           alignAnchor )
{
  int pceStatus = 0;
  int crcReg;

  /* read PCE to temporal buffer first */
  C_ALLOC_SCRATCH_START(tmpPce, CProgramConfig, 1);

  CProgramConfig_Init(tmpPce);
  CProgramConfig_Reset(tmpPce);

  crcReg = transportDec_CrcStartReg(pTp, 0);

  CProgramConfig_Read(tmpPce, bs, alignAnchor);

  transportDec_CrcEndReg(pTp, crcReg);

  if (  CProgramConfig_IsValid(tmpPce)
    && (tmpPce->Profile == 1) )
  {
    if ( !pce->isValid && (channelConfig > 0) ) {
      /* Create a standard channel config PCE to compare with */
      CProgramConfig_GetDefault( pce, channelConfig );
    }

    if (pce->isValid) {
      /* Compare the new and the old PCE (tags ignored) */
      switch ( CProgramConfig_Compare( pce, tmpPce ) )
      {
      case 1:  /* Channel configuration not changed. Just new metadata. */
        FDKmemcpy(pce, tmpPce, sizeof(CProgramConfig));    /* Store the complete PCE */
        pceStatus = 1;                                     /* New PCE but no change of config */
        break;
      case 2:  /* The number of channels are identical but not the config */
        if (channelConfig == 0) {
          FDKmemcpy(pce, tmpPce, sizeof(CProgramConfig));  /* Store the complete PCE */
          pceStatus = 2;                                   /* Decoder needs re-configuration */
        }
        break;
      case -1:  /* The channel configuration is completely different */
        pceStatus = -1;  /* Not supported! */
        break;
      case 0:  /* Nothing to do because PCE matches the old one exactly. */
      default:
        /* pceStatus = 0; */
        break;
      }
    }
  }

  C_ALLOC_SCRATCH_END(tmpPce, CProgramConfig, 1);

  return pceStatus;
}
#endif /* TP_PCE_ENABLE */

/*!
  \brief Parse Extension Payload

  \self Handle of AAC decoder
  \count Pointer to bit counter.
  \previous_element ID of previous element (required by some extension payloads)

  \return  Error code
*/
static
AAC_DECODER_ERROR CAacDecoder_ExtPayloadParse (HANDLE_AACDECODER self,
                                               HANDLE_FDK_BITSTREAM hBs,
                                               int *count,
                                               MP4_ELEMENT_ID previous_element,
                                               int elIndex,
                                               int fIsFillElement)
{
  AAC_DECODER_ERROR error = AAC_DEC_OK;
  EXT_PAYLOAD_TYPE extension_type;
  int bytes = (*count) >> 3;
  int crcFlag = 0;

  if (*count < 4) {
    return AAC_DEC_PARSE_ERROR;
  } else if ((INT)FDKgetValidBits(hBs) < *count) {
    return AAC_DEC_DECODE_FRAME_ERROR;
  }

  extension_type = (EXT_PAYLOAD_TYPE) FDKreadBits(hBs, 4);    /* bs_extension_type */
  *count -= 4;

  switch (extension_type)
  {
  case EXT_DYNAMIC_RANGE:
    {
      INT readBits = aacDecoder_drcMarkPayload( self->hDrcInfo, hBs, MPEG_DRC_EXT_DATA );

      if (readBits > *count)
      { /* Read too much. Something went wrong! */
        error = AAC_DEC_PARSE_ERROR;
      }
      *count -= readBits;
    }
    break;


  case EXT_SBR_DATA_CRC:
    crcFlag = 1;
  case EXT_SBR_DATA:
    if (IS_CHANNEL_ELEMENT(previous_element)) {
      SBR_ERROR sbrError;

      CAacDecoder_SyncQmfMode(self);

      sbrError = sbrDecoder_InitElement(
              self->hSbrDecoder,
              self->streamInfo.aacSampleRate,
              self->streamInfo.extSamplingRate,
              self->streamInfo.aacSamplesPerFrame,
              self->streamInfo.aot,
              previous_element,
              elIndex
              );

      if (sbrError == SBRDEC_OK) {
        sbrError = sbrDecoder_Parse (
                self->hSbrDecoder,
                hBs,
                count,
               *count,
                crcFlag,
                previous_element,
                elIndex,
                self->flags & AC_INDEP );
        /* Enable SBR for implicit SBR signalling but only if no severe error happend. */
        if ( (sbrError == SBRDEC_OK)
          || (sbrError == SBRDEC_PARSE_ERROR) ) {
          self->sbrEnabled = 1;
        }
      } else {
        /* Do not try to apply SBR because initializing the element failed. */
        self->sbrEnabled = 0;
      }
      /* Citation from ISO/IEC 14496-3 chapter 4.5.2.1.5.2
      Fill elements containing an extension_payload() with an extension_type of EXT_SBR_DATA
      or EXT_SBR_DATA_CRC shall not contain any other extension_payload of any other extension_type.
      */
      if (fIsFillElement) {
        FDKpushBiDirectional(hBs, *count);
        *count = 0;
      } else {
        /* If this is not a fill element with a known length, we are screwed and further parsing makes no sense. */
        if (sbrError != SBRDEC_OK) {
          self->frameOK = 0;
        }
      }
    } else {
      error = AAC_DEC_PARSE_ERROR;
    }
    break;

  case EXT_FILL_DATA:
    {
      int temp;

      temp = FDKreadBits(hBs,4);
      bytes--;
      if (temp != 0) {
        error = AAC_DEC_PARSE_ERROR;
        break;
      }
      while (bytes > 0) {
        temp = FDKreadBits(hBs,8);
        bytes--;
        if (temp != 0xa5) {
          error = AAC_DEC_PARSE_ERROR;
          break;
        }
      }
      *count = bytes<<3;
    }
    break;

  case EXT_DATA_ELEMENT:
    {
      int dataElementVersion;

      dataElementVersion = FDKreadBits(hBs,4);
      *count -= 4;
      if (dataElementVersion == 0) /* ANC_DATA */
      {
        int temp, dataElementLength = 0;
        do {
          temp = FDKreadBits(hBs,8);
          *count -= 8;
          dataElementLength += temp;
        } while (temp == 255 );

        CAacDecoder_AncDataParse(&self->ancData, hBs, dataElementLength);
        *count -= (dataElementLength<<3);
      } else {
        /* align = 0 */
        error = AAC_DEC_PARSE_ERROR;
        goto bail;
      }
    }
    break;

  case EXT_DATA_LENGTH:
    if ( !fIsFillElement          /* Makes no sens to have an additional length in a fill ...   */
      && (self->flags & AC_ER) )  /* ... element because this extension payload type was ...    */
    {                             /* ... created to circumvent the missing length in ER-Syntax. */
      int bitCnt, len = FDKreadBits(hBs, 4);
      *count -= 4;

      if (len == 15) {
        int add_len = FDKreadBits(hBs, 8);
        *count -= 8;
        len += add_len;

        if (add_len == 255) {
          len += FDKreadBits(hBs, 16);
          *count -= 16;
        }
      }
      len <<= 3;
      bitCnt = len;

      if ( (EXT_PAYLOAD_TYPE)FDKreadBits(hBs, 4) == EXT_DATA_LENGTH ) {
        /* Check NOTE 2: The extension_payload() included here must
                         not have extension_type == EXT_DATA_LENGTH. */
        error = AAC_DEC_PARSE_ERROR;
      } else {
        /* rewind and call myself again. */
        FDKpushBack(hBs, 4);

        error =
          CAacDecoder_ExtPayloadParse (
                  self,
                  hBs,
                 &bitCnt,
                  previous_element,
                  elIndex,
                  0 );

        *count -= len - bitCnt;
      }
      /* Note: the fall through in case the if statement above is not taken is intentional. */
      break;
    }

  case EXT_FIL:

  default:
    /* align = 4 */
    FDKpushFor(hBs, *count);
    *count = 0;
    break;
  }

bail:
  if ( (error != AAC_DEC_OK)
    && fIsFillElement )
  { /* Skip the remaining extension bytes */
    FDKpushBiDirectional(hBs, *count);
    *count = 0;
    /* Patch error code because decoding can go on. */
    error = AAC_DEC_OK;
    /* Be sure that parsing errors have been stored. */
  }
  return error;
}

/*  Stream Configuration and Information.

    This class holds configuration and information data for a stream to be decoded. It
    provides the calling application as well as the decoder with substantial information,
    e.g. profile, sampling rate, number of channels found in the bitstream etc.
*/
static
void CStreamInfoInit(CStreamInfo *pStreamInfo)
{
  pStreamInfo->aacSampleRate = 0;
  pStreamInfo->profile = -1;
  pStreamInfo->aot = AOT_NONE;

  pStreamInfo->channelConfig = -1;
  pStreamInfo->bitRate = 0;
  pStreamInfo->aacSamplesPerFrame = 0;

  pStreamInfo->extAot  = AOT_NONE;
  pStreamInfo->extSamplingRate = 0;

  pStreamInfo->flags = 0;

  pStreamInfo->epConfig = -1;   /* default is no ER */

  pStreamInfo->numChannels = 0;
  pStreamInfo->sampleRate = 0;
  pStreamInfo->frameSize = 0;

  pStreamInfo->outputDelay = 0;

  /* DRC */
  pStreamInfo->drcProgRefLev = -1;                           /* set program reference level to not indicated */
  pStreamInfo->drcPresMode = -1;                             /* default: presentation mode not indicated */
}

/*!
  \brief Initialization of AacDecoderChannelInfo

  The function initializes the pointers to AacDecoderChannelInfo for each channel,
  set the start values for window shape and window sequence of overlap&add to zero,
  set the overlap buffer to zero and initializes the pointers to the window coefficients.
  \param bsFormat is the format of the AAC bitstream

  \return  AACDECODER instance
*/
LINKSPEC_CPP HANDLE_AACDECODER CAacDecoder_Open(TRANSPORT_TYPE bsFormat)    /*!< bitstream format (adif,adts,loas,...). */
{
  HANDLE_AACDECODER self;

  self = GetAacDecoder();
  if (self == NULL) {
    goto bail;
  }

  /* Assign channel mapping info arrays (doing so removes dependency of settings header in API header). */
  self->streamInfo.pChannelIndices = self->channelIndices;
  self->streamInfo.pChannelType = self->channelType;

  /* set default output mode */
  self->outputInterleaved = 1;  /* interleaved */

  /* initialize anc data */
  CAacDecoder_AncDataInit(&self->ancData, NULL, 0);

  /* initialize stream info */
  CStreamInfoInit(&self->streamInfo);

  /* initialize error concealment common data */
  CConcealment_InitCommonData(&self->concealCommonData);

  self->hDrcInfo = GetDrcInfo();
  if (self->hDrcInfo == NULL) {
    goto bail;
  }
  /* Init common DRC structure */
  aacDecoder_drcInit( self->hDrcInfo );
  /* Set default frame delay */
  aacDecoder_drcSetParam (
          self->hDrcInfo,
          DRC_BS_DELAY,
          CConcealment_GetDelay(&self->concealCommonData)
        );


  self->aacCommonData.workBufferCore1 = GetWorkBufferCore1();
  self->aacCommonData.workBufferCore2 = GetWorkBufferCore2();
  if (self->aacCommonData.workBufferCore1 == NULL
    ||self->aacCommonData.workBufferCore2 == NULL )
    goto bail;

  return self;

bail:
  CAacDecoder_Close( self );

  return NULL;
}

/* Destroy aac decoder */
LINKSPEC_CPP void CAacDecoder_Close(HANDLE_AACDECODER self)
{
  int ch;

  if (self == NULL)
    return;

  for (ch=0; ch<(8); ch++) {
    if (self->pAacDecoderStaticChannelInfo[ch] != NULL) {
      if (self->pAacDecoderStaticChannelInfo[ch]->pOverlapBuffer != NULL) {
        FreeOverlapBuffer (&self->pAacDecoderStaticChannelInfo[ch]->pOverlapBuffer);
      }
      if (self->pAacDecoderStaticChannelInfo[ch] != NULL) {
        FreeAacDecoderStaticChannelInfo (&self->pAacDecoderStaticChannelInfo[ch]);
      }
    }
    if (self->pAacDecoderChannelInfo[ch] != NULL) {
      FreeAacDecoderChannelInfo (&self->pAacDecoderChannelInfo[ch]);
    }
  }

  self->aacChannels = 0;

  if (self->hDrcInfo) {
    FreeDrcInfo(&self->hDrcInfo);
  }

  if (self->aacCommonData.workBufferCore1 != NULL) {
    FreeWorkBufferCore1 (&self->aacCommonData.workBufferCore1);
  }
  if (self->aacCommonData.workBufferCore2 != NULL) {
    FreeWorkBufferCore2 (&self->aacCommonData.workBufferCore2);
  }

  FreeAacDecoder ( &self);
}


/*!
  \brief Initialization of decoder instance

  The function initializes the decoder.

  \return  error status: 0 for success, <>0 for unsupported configurations
*/
LINKSPEC_CPP AAC_DECODER_ERROR CAacDecoder_Init(HANDLE_AACDECODER self, const CSAudioSpecificConfig *asc)
{
  AAC_DECODER_ERROR err = AAC_DEC_OK;
  INT ascChannels, ch, ascChanged = 0;

  if (!self)
    return AAC_DEC_INVALID_HANDLE;

  // set profile and check for supported aot
  // leave profile on default (=-1) for all other supported MPEG-4 aot's except aot=2 (=AAC-LC)
  switch (asc->m_aot) {
  case AOT_AAC_LC:
    self->streamInfo.profile = 1;

  case AOT_ER_AAC_SCAL:
    if (asc->m_sc.m_gaSpecificConfig.m_layer > 0) {
      /* aac_scalable_extension_element() currently not supported. */
      return AAC_DEC_UNSUPPORTED_FORMAT;
    }

  case AOT_SBR:
  case AOT_PS:
  case AOT_ER_AAC_LD:
  case AOT_ER_AAC_ELD:
  case AOT_DRM_AAC:
    break;

  default:
    return AAC_DEC_UNSUPPORTED_AOT;
  }

  CProgramConfig_Init(&self->pce);

  /* set channels */
  switch (asc->m_channelConfiguration) {
  case 0:
#ifdef TP_PCE_ENABLE
    /* get channels from program config (ASC) */
    if (CProgramConfig_IsValid(&asc->m_progrConfigElement)) {
      ascChannels = asc->m_progrConfigElement.NumChannels;
      if (ascChannels > 0) {
        int el;
        /* valid number of channels -> copy program config element (PCE) from ASC */
        FDKmemcpy(&self->pce, &asc->m_progrConfigElement, sizeof(CProgramConfig));
        /* Built element table */
        el = CProgramConfig_GetElementTable(&asc->m_progrConfigElement, self->elements, (8), &self->chMapIndex);
        for (; el<(8); el++) {
          self->elements[el] = ID_NONE;
        }
      } else {
        return AAC_DEC_UNSUPPORTED_CHANNELCONFIG;
      }
    } else {
      self->chMapIndex = 0;
      if (transportDec_GetFormat(self->hInput) == TT_MP4_ADTS) {
        /* set default max_channels for memory allocation because in implicit channel mapping mode
           we don't know the actual number of channels until we processed at least one raw_data_block(). */
        ascChannels = (8);
      } else {
        return AAC_DEC_UNSUPPORTED_CHANNELCONFIG;
      }
    }
#else /* TP_PCE_ENABLE */
    return AAC_DEC_UNSUPPORTED_CHANNELCONFIG;
#endif /* TP_PCE_ENABLE */
    break;
  case 1: case 2: case 3: case 4: case 5: case 6:
    ascChannels = asc->m_channelConfiguration;
    break;
  case 11:
    ascChannels = 7;
    break;
  case 7: case 12: case 14:
    ascChannels = 8;
    break;
  default:
    return AAC_DEC_UNSUPPORTED_CHANNELCONFIG;
  }

  if (ascChannels > (8)) {
    return AAC_DEC_UNSUPPORTED_CHANNELCONFIG;
  }

  /* Initialize constant mappings for channel config 1-7 */
  if (asc->m_channelConfiguration > 0) {
    int el;
    FDKmemcpy(self->elements, elementsTab[asc->m_channelConfiguration-1], sizeof(MP4_ELEMENT_ID)*FDKmin(7,(8)));
    for (el=7; el<(8); el++) {
      self->elements[el] = ID_NONE;
    }
    for (ch=0; ch<ascChannels; ch++) {
      self->chMapping[ch] = ch;
    }
    for (; ch<(8); ch++) {
      self->chMapping[ch] = 255;
    }
    self->chMapIndex = asc->m_channelConfiguration;
  }
 #ifdef TP_PCE_ENABLE
  else {
    if (CProgramConfig_IsValid(&asc->m_progrConfigElement)) {
      /* Set matrix mixdown infos if available from PCE. */
      pcmDmx_SetMatrixMixdownFromPce ( self->hPcmUtils,
                                       asc->m_progrConfigElement.MatrixMixdownIndexPresent,
                                       asc->m_progrConfigElement.MatrixMixdownIndex,
                                       asc->m_progrConfigElement.PseudoSurroundEnable );
    }
  }
 #endif

  self->streamInfo.channelConfig = asc->m_channelConfiguration;

  if (self->streamInfo.aot != asc->m_aot) {
    self->streamInfo.aot = asc->m_aot;
    ascChanged = 1;
  }

  if (self->streamInfo.aacSamplesPerFrame != (INT)asc->m_samplesPerFrame) {
    self->streamInfo.aacSamplesPerFrame = asc->m_samplesPerFrame;
    ascChanged = 1;
  }

  self->streamInfo.bitRate            = 0;

  /* Set syntax flags */
  self->flags = 0;

  self->streamInfo.extAot               = asc->m_extensionAudioObjectType;
  self->streamInfo.extSamplingRate      = asc->m_extensionSamplingFrequency;
  self->flags |= (asc->m_sbrPresentFlag) ? AC_SBR_PRESENT : 0;
  self->flags |= (asc->m_psPresentFlag) ? AC_PS_PRESENT : 0;
  self->sbrEnabled = 0;

  /* --------- vcb11 ------------ */
  self->flags |= (asc->m_vcb11Flag) ? AC_ER_VCB11 : 0;

  /* ---------- rvlc ------------ */
  self->flags |= (asc->m_rvlcFlag) ? AC_ER_RVLC : 0;

  /* ----------- hcr ------------ */
  self->flags |= (asc->m_hcrFlag) ? AC_ER_HCR : 0;

  if (asc->m_aot == AOT_ER_AAC_ELD) {
    self->flags |=  AC_ELD;
    self->flags |= (asc->m_sbrPresentFlag) ? AC_SBR_PRESENT : 0;  /* Need to set the SBR flag for backward-compatibility
                                                                     reasons. Even if SBR is not supported. */
    self->flags |= (asc->m_sc.m_eldSpecificConfig.m_sbrCrcFlag) ? AC_SBRCRC : 0;
    self->flags |= (asc->m_sc.m_eldSpecificConfig.m_useLdQmfTimeAlign) ? AC_LD_MPS : 0;
  }
  self->flags |= (asc->m_aot == AOT_ER_AAC_LD) ? AC_LD : 0;
  self->flags |= (asc->m_epConfig >= 0) ? AC_ER : 0;
  if ( asc->m_aot == AOT_DRM_AAC ) {
    self->flags |= AC_DRM|AC_SBRCRC|AC_SCALABLE;
  }
  if ( (asc->m_aot == AOT_AAC_SCAL)
    || (asc->m_aot == AOT_ER_AAC_SCAL) ) {
    self->flags |= AC_SCALABLE;
  }


  if (asc->m_sbrPresentFlag) {
    self->sbrEnabled = 1;
    self->sbrEnabledPrev = 1;
  }
  if (asc->m_psPresentFlag) {
    self->flags |= AC_PS_PRESENT;
  }

  if ( (asc->m_epConfig >= 0)
    && (asc->m_channelConfiguration <= 0) ) {
    /* we have to know the number of channels otherwise no decoding is possible */
    return AAC_DEC_UNSUPPORTED_ER_FORMAT;
  }

  self->streamInfo.epConfig = asc->m_epConfig;
  /* self->hInput->asc.m_epConfig = asc->m_epConfig; */

  if (asc->m_epConfig > 1)
    return AAC_DEC_UNSUPPORTED_ER_FORMAT;

  /* Check if samplerate changed. */
  if (self->streamInfo.aacSampleRate != (INT)asc->m_samplingFrequency) {
    AAC_DECODER_ERROR error;

    ascChanged = 1;

    /* Update samplerate info. */
    error = getSamplingRateInfo(&self->samplingRateInfo, asc->m_samplesPerFrame, asc->m_samplingFrequencyIndex, asc->m_samplingFrequency);
    if (error != AAC_DEC_OK) {
      return error;
    }
    self->streamInfo.aacSampleRate = self->samplingRateInfo.samplingRate;
  }

  /* Check if amount of channels has changed. */
  if (self->ascChannels != ascChannels)
  {
     ascChanged = 1;

     /* Allocate all memory structures for each channel */
     {
       for (ch = 0; ch < ascChannels; ch++) {
         CAacDecoderDynamicData *aacDecoderDynamicData = &self->aacCommonData.workBufferCore1->pAacDecoderDynamicData[ch%2];

         /* initialize pointer to CAacDecoderChannelInfo */
         if (self->pAacDecoderChannelInfo[ch] == NULL) {
           self->pAacDecoderChannelInfo[ch] = GetAacDecoderChannelInfo(ch);
           /* This is temporary until the DynamicData is split into two or more regions!
              The memory could be reused after completed core decoding. */
           if (self->pAacDecoderChannelInfo[ch] == NULL) {
             goto bail;
           }
           /* Hook shared work memory into channel data structure */
           self->pAacDecoderChannelInfo[ch]->pDynData =  aacDecoderDynamicData;
           self->pAacDecoderChannelInfo[ch]->pComData = &self->aacCommonData;
         }

         /* Allocate persistent channel memory */
         if (self->pAacDecoderStaticChannelInfo[ch] == NULL) {
           self->pAacDecoderStaticChannelInfo[ch] = GetAacDecoderStaticChannelInfo(ch);
           if (self->pAacDecoderStaticChannelInfo[ch] == NULL) {
             goto bail;
           }
           self->pAacDecoderStaticChannelInfo[ch]->pOverlapBuffer = GetOverlapBuffer(ch); /* This area size depends on the AOT */
           if (self->pAacDecoderStaticChannelInfo[ch]->pOverlapBuffer == NULL) {
             goto bail;
           }
           self->pAacDecoderChannelInfo[ch]->pSpectralCoefficient = (SPECTRAL_PTR) &self->aacCommonData.workBufferCore2[ch*1024];

         }
         CPns_InitPns(&self->pAacDecoderChannelInfo[ch]->data.aac.PnsData, &self->aacCommonData.pnsInterChannelData, &self->aacCommonData.pnsCurrentSeed, self->aacCommonData.pnsRandomSeed);
       }

       if (ascChannels > self->aacChannels)
       {
         /* Make allocated channel count persistent in decoder context. */
         self->aacChannels = ascChannels;
       }

       HcrInitRom(&self->aacCommonData.overlay.aac.erHcrInfo);
       setHcrType(&self->aacCommonData.overlay.aac.erHcrInfo, ID_SCE);
    }

    /* Make amount of signalled channels persistent in decoder context. */
    self->ascChannels = ascChannels;
  }

  /* Update structures */
  if (ascChanged) {

     /* Things to be done for each channel, which do not involve allocating memory.
        Doing these things only on the channels needed for the current configuration
        (ascChannels) could lead to memory access violation later (error concealment). */
     for (ch = 0; ch < self->aacChannels; ch++) {
       switch (self->streamInfo.aot) {
         case AOT_ER_AAC_ELD:
         case AOT_ER_AAC_LD:
           self->pAacDecoderChannelInfo[ch]->granuleLength = self->streamInfo.aacSamplesPerFrame;
           break;
         default:
           self->pAacDecoderChannelInfo[ch]->granuleLength = self->streamInfo.aacSamplesPerFrame / 8;
           break;
       }
       mdct_init( &self->pAacDecoderStaticChannelInfo[ch]->IMdct,
                   self->pAacDecoderStaticChannelInfo[ch]->pOverlapBuffer,
                   OverlapBufferSize );


        /* Reset DRC control data for this channel */
        aacDecoder_drcInitChannelData ( &self->pAacDecoderStaticChannelInfo[ch]->drcData );

       /* Reset concealment only if ASC changed. Otherwise it will be done with any config callback.
          E.g. every time the LATM SMC is present. */
       CConcealment_InitChannelData(&self->pAacDecoderStaticChannelInfo[ch]->concealmentInfo,
                                    &self->concealCommonData,
                                     self->streamInfo.aacSamplesPerFrame );
     }
  }

  /* Update externally visible copy of flags */
  self->streamInfo.flags = self->flags;

  return err;

bail:
  aacDecoder_Close( self );
  return AAC_DEC_OUT_OF_MEMORY;
}


LINKSPEC_CPP AAC_DECODER_ERROR CAacDecoder_DecodeFrame(
        HANDLE_AACDECODER self,
        const UINT flags,
        INT_PCM *pTimeData,
        const INT  timeDataSize,
        const INT interleaved
        )
{
  AAC_DECODER_ERROR ErrorStatus = AAC_DEC_OK;

  CProgramConfig *pce;
  HANDLE_FDK_BITSTREAM bs = transportDec_GetBitstream(self->hInput, 0);

  MP4_ELEMENT_ID type = ID_NONE;            /* Current element type */
  INT aacChannels=0;                        /* Channel counter for channels found in the bitstream */
  int chOutMapIdx;                          /* Output channel mapping index (see comment below) */

  INT auStartAnchor = (INT)FDKgetValidBits(bs);  /* AU start bit buffer position for AU byte alignment */

  self->frameOK = 1;

  /* Any supported base layer valid AU will require more than 16 bits. */
  if ( (transportDec_GetAuBitsRemaining(self->hInput, 0) < 15) && (flags & (AACDEC_CONCEAL|AACDEC_FLUSH)) == 0) {
    self->frameOK = 0;
    ErrorStatus = AAC_DEC_DECODE_FRAME_ERROR;
  }


  /* Reset Program Config structure */
  pce = &self->pce;
  CProgramConfig_Reset(pce);

  CAacDecoder_AncDataReset(&self->ancData);

  {
    int ch;

    if (self->streamInfo.channelConfig == 0) {
      /* Init Channel/Element mapping table */
      for (ch=0; ch<(8); ch++) {
        self->chMapping[ch] = 255;
      }
      if (!CProgramConfig_IsValid(pce)) {
        int el;
        for (el=0; el<(8); el++) {
          self->elements[el] = ID_NONE;
        }
      }
    }
  }

  /* Check sampling frequency  */
  switch ( self->streamInfo.aacSampleRate ) {
    case 96000:
    case 88200:
    case 64000:
    case 16000:
    case 12000:
   case 11025:
   case  8000:
    case  7350:
    case 48000:
    case 44100:
    case 32000:
    case 24000:
    case 22050:
      break;
    default:
      if ( ! (self->flags & (AC_USAC|AC_RSVD50)) ) {
        return AAC_DEC_UNSUPPORTED_SAMPLINGRATE;
      }
      break;
  }


  if ( flags & AACDEC_CLRHIST )
  {
    int ch;
    /* Clear history */
    for (ch = 0; ch < self->aacChannels; ch++) {
      /* Reset concealment */
      CConcealment_InitChannelData(&self->pAacDecoderStaticChannelInfo[ch]->concealmentInfo,
                                   &self->concealCommonData,
                                    self->streamInfo.aacSamplesPerFrame );
      /* Clear overlap-add buffers to avoid clicks. */
      FDKmemclear(self->pAacDecoderStaticChannelInfo[ch]->pOverlapBuffer, OverlapBufferSize*sizeof(FIXP_DBL));
     }
  }



#ifdef TP_PCE_ENABLE
  int pceRead = 0;                          /* Flag indicating a PCE in the current raw_data_block() */
#endif


  INT hdaacDecoded = 0;
  MP4_ELEMENT_ID previous_element = ID_END; /* Last element ID (required for extension payload mapping */
  UCHAR previous_element_index = 0;         /* Canonical index of last element */
  int element_count = 0;                    /* Element counter for elements found in the bitstream */
  int el_cnt[ID_LAST] = { 0 };              /* element counter ( robustness ) */

  while ( (type != ID_END) && (! (flags & (AACDEC_CONCEAL | AACDEC_FLUSH))) && self->frameOK )
  {
    int el_channels;

    if (! (self->flags & (AC_USAC|AC_RSVD50|AC_ELD|AC_SCALABLE|AC_ER)))
      type = (MP4_ELEMENT_ID) FDKreadBits(bs,3);
    else 
      type = self->elements[element_count];

    setHcrType(&self->aacCommonData.overlay.aac.erHcrInfo, type);


    if ((INT)FDKgetValidBits(bs) < 0)
      self->frameOK = 0;

    switch (type)
    {
      case ID_SCE:
      case ID_CPE:
      case ID_LFE:
        /*
          Consistency check
        */

        if (type == ID_CPE) {
          el_channels = 2;
        } else {
          el_channels = 1;
        }

        if ( (el_cnt[type] >= (self->ascChannels>>(el_channels-1))) || (aacChannels > (self->ascChannels-el_channels)) ) {
          ErrorStatus = AAC_DEC_DECODE_FRAME_ERROR;
          self->frameOK = 0;
          break;
        }

        if ( !(self->flags & (AC_USAC|AC_RSVD50)) ) {
          int ch;
          for (ch=0; ch < el_channels; ch+=1) {
            CPns_ResetData(&self->pAacDecoderChannelInfo[aacChannels+ch]->data.aac.PnsData,
                           &self->pAacDecoderChannelInfo[aacChannels+ch]->pComData->pnsInterChannelData);
          }
        }

        if(self->frameOK) {
          ErrorStatus = CChannelElement_Read( bs,
                                             &self->pAacDecoderChannelInfo[aacChannels],
                                             &self->pAacDecoderStaticChannelInfo[aacChannels],
                                              self->streamInfo.aot,
                                             &self->samplingRateInfo,
                                              self->flags,
                                              self->streamInfo.aacSamplesPerFrame,
                                              el_channels,
                                              self->streamInfo.epConfig,
                                              self->hInput
                                              );
          if (ErrorStatus) {
            self->frameOK = 0;
          }
        }


        if ( self->frameOK) {
          /* Lookup the element and decode it only if it belongs to the current program */
          if ( CProgramConfig_LookupElement(
                  pce,
                  self->streamInfo.channelConfig,
                  self->pAacDecoderChannelInfo[aacChannels]->ElementInstanceTag,
                  aacChannels,
                  self->chMapping,
                  self->channelType,
                  self->channelIndices,
                 &previous_element_index,
                  self->elements,
                  type) )
          {
            if ( !hdaacDecoded ) {
              CChannelElement_Decode(
                     &self->pAacDecoderChannelInfo[aacChannels],
                     &self->pAacDecoderStaticChannelInfo[aacChannels],
                     &self->samplingRateInfo,
                      self->flags,
                      el_channels
                      );
            }
            aacChannels += 1;
            if (type == ID_CPE) {
              aacChannels += 1;
            }
          }
          else {
            self->frameOK = 0;
          }
          /* Create SBR element for SBR for upsampling for LFE elements,
             and if SBR was explicitly signaled, because the first frame(s)
             may not contain SBR payload (broken encoder, bit errors). */
          if ( (self->flags & AC_SBR_PRESENT) || (self->sbrEnabled == 1) )
          {
            SBR_ERROR sbrError;

            sbrError = sbrDecoder_InitElement(
                    self->hSbrDecoder,
                    self->streamInfo.aacSampleRate,
                    self->streamInfo.extSamplingRate,
                    self->streamInfo.aacSamplesPerFrame,
                    self->streamInfo.aot,
                    type,
                    previous_element_index
                    );
            if (sbrError != SBRDEC_OK) {
              /* Do not try to apply SBR because initializing the element failed. */
              self->sbrEnabled = 0;
            }
          }
        }

        el_cnt[type]++;
        break;

      case ID_CCE:
        /*
          Consistency check
        */
        if ( el_cnt[type] > self->ascChannels ) {
          ErrorStatus = AAC_DEC_DECODE_FRAME_ERROR;
          self->frameOK = 0;
          break;
        }

        if (self->frameOK)
        {
          /* memory for spectral lines temporal on scratch */
          C_ALLOC_SCRATCH_START(mdctSpec, FIXP_DBL, 1024);

          /* create dummy channel for CCE parsing on stack */
          CAacDecoderChannelInfo  tmpAacDecoderChannelInfo, *pTmpAacDecoderChannelInfo;

          FDKmemclear(mdctSpec, 1024*sizeof(FIXP_DBL));

          tmpAacDecoderChannelInfo.pDynData =   self->aacCommonData.workBufferCore1->pAacDecoderDynamicData;
          tmpAacDecoderChannelInfo.pComData =  &self->aacCommonData;
          tmpAacDecoderChannelInfo.pSpectralCoefficient  = (SPECTRAL_PTR)mdctSpec;
          /* Assume AAC-LC */
          tmpAacDecoderChannelInfo.granuleLength = self->streamInfo.aacSamplesPerFrame / 8;

          /* Reset PNS data. */
          CPns_ResetData(&tmpAacDecoderChannelInfo.data.aac.PnsData, &tmpAacDecoderChannelInfo.pComData->pnsInterChannelData);

          pTmpAacDecoderChannelInfo = &tmpAacDecoderChannelInfo;
          /* do CCE parsing */
          ErrorStatus = CChannelElement_Read( bs,
                                             &pTmpAacDecoderChannelInfo,
                                              NULL,
                                              self->streamInfo.aot,
                                             &self->samplingRateInfo,
                                              self->flags,
                                              self->streamInfo.aacSamplesPerFrame,
                                              1,
                                              self->streamInfo.epConfig,
                                              self->hInput
                                             );

          C_ALLOC_SCRATCH_END(mdctSpec, FIXP_DBL, 1024);

          if (ErrorStatus) {
            self->frameOK = 0;
          }

          if (self->frameOK) {
            /* Lookup the element and decode it only if it belongs to the current program */
            if (CProgramConfig_LookupElement(
                    pce,
                    self->streamInfo.channelConfig,
                    pTmpAacDecoderChannelInfo->ElementInstanceTag,
                    0,
                    self->chMapping,
                    self->channelType,
                    self->channelIndices,
                   &previous_element_index,
                    self->elements,
                    type) )
            {
              /* decoding of CCE not supported */
            }
            else {
              self->frameOK = 0;
            }
          }
        }
        el_cnt[type]++;
        break;

      case ID_DSE:
        {
          UCHAR element_instance_tag;

          CDataStreamElement_Read( self,
                                   bs,
                                  &element_instance_tag,
                                   auStartAnchor );

          if (!CProgramConfig_LookupElement(
                   pce,
                   self->streamInfo.channelConfig,
                   element_instance_tag,
                   0,
                   self->chMapping,
                   self->channelType,
                   self->channelIndices,
                  &previous_element_index,
                   self->elements,
                   type) )
          {
            /* most likely an error in bitstream occured */
            //self->frameOK = 0;
          }
        }
        break;

#ifdef TP_PCE_ENABLE
      case ID_PCE:
        {
          int result = CProgramConfigElement_Read(
                                    bs,
                                    self->hInput,
                                    pce,
                                    self->streamInfo.channelConfig,
                                    auStartAnchor );
          if ( result < 0 ) {
            /* Something went wrong */
            ErrorStatus = AAC_DEC_PARSE_ERROR;
            self->frameOK = 0;
          }
          else if ( result > 1 ) {
            /* Built element table */
            int elIdx = CProgramConfig_GetElementTable(pce, self->elements, (8), &self->chMapIndex);
            /* Reset the remaining tabs */
            for ( ; elIdx<(8); elIdx++) {
              self->elements[elIdx] = ID_NONE;
            }
            /* Make new number of channel persistant */
            self->ascChannels = pce->NumChannels;
            /* If PCE is not first element conceal this frame to avoid inconsistencies */
            if ( element_count != 0 ) {
              self->frameOK = 0;
            }
          }
          pceRead = (result>=0) ? 1 : 0;
        }
        break;
#endif /* TP_PCE_ENABLE */

      case ID_FIL:
        {
          int bitCnt = FDKreadBits(bs,4);           /* bs_count */

          if (bitCnt == 15)
          {
            int esc_count = FDKreadBits(bs,8);     /* bs_esc_count */
            bitCnt =  esc_count + 14;
          }

          /* Convert to bits */
          bitCnt <<= 3;

          while (bitCnt > 0) {
            ErrorStatus = CAacDecoder_ExtPayloadParse(self, bs, &bitCnt, previous_element, previous_element_index, 1);
            if (ErrorStatus != AAC_DEC_OK) {
              self->frameOK = 0;
              break;
            }
          }
        }
        break;

      case ID_EXT:
        {
          INT bitCnt = 0;

          /* get the remaining bits of this frame */
          bitCnt = transportDec_GetAuBitsRemaining(self->hInput, 0);

          if ( (bitCnt > 0) && (self->flags & AC_SBR_PRESENT) && (self->flags & (AC_USAC|AC_RSVD50|AC_ELD|AC_DRM)) )
          {
            SBR_ERROR err = SBRDEC_OK;
            int  elIdx, numChElements = el_cnt[ID_SCE] + el_cnt[ID_CPE];

            for (elIdx = 0; elIdx < numChElements; elIdx += 1)
            {
              err = sbrDecoder_Parse (
                    self->hSbrDecoder,
                    bs,
                   &bitCnt,
                    -1,
                    self->flags & AC_SBRCRC,
                    self->elements[elIdx],
                    elIdx,
                    self->flags & AC_INDEP );

              if (err != SBRDEC_OK) {
                break;
              }
            }
            switch (err) {
            case SBRDEC_PARSE_ERROR:
              /* Can not go on parsing because we do not
                 know the length of the SBR extension data. */
              FDKpushFor(bs, bitCnt);
              bitCnt = 0;
              break;
            case SBRDEC_OK:
              self->sbrEnabled = 1;
              break;
            default:
              self->frameOK = 0;
              break;
            }
          }


          if (self->flags & AC_DRM)
          {
            if ((bitCnt = (INT)FDKgetValidBits(bs)) != 0) {
              FDKpushBiDirectional(bs, bitCnt);
            }
          }

          if ( ! (self->flags & (AC_USAC|AC_RSVD50|AC_DRM)) )
          {
            while ( bitCnt > 7 ) {
              ErrorStatus = CAacDecoder_ExtPayloadParse(self, bs, &bitCnt, previous_element, previous_element_index, 0);
              if (ErrorStatus != AAC_DEC_OK) {
                self->frameOK = 0;
                ErrorStatus = AAC_DEC_PARSE_ERROR;
                break;
              }
            }
          }
        }
        break;

      case ID_END:
        break;

      default:
        ErrorStatus = AAC_DEC_DECODE_FRAME_ERROR;
        self->frameOK = 0;
        break;
    }

    previous_element = type;
    element_count++;

  }   /* while ( (type != ID_END) ... ) */

  if ( !(flags & (AACDEC_CONCEAL|AACDEC_FLUSH)) )
  {
    /* Byte alignment with respect to the first bit of the raw_data_block(). */
    {
      FDKbyteAlign(bs, auStartAnchor);
    }

    /* Check if all bits of the raw_data_block() have been read. */
    if ( transportDec_GetAuBitsTotal(self->hInput, 0) > 0 ) {
      INT unreadBits = transportDec_GetAuBitsRemaining(self->hInput, 0);
      if ( unreadBits != 0 ) {

        self->frameOK = 0;
        /* Do not overwrite current error */
        if (ErrorStatus == AAC_DEC_OK && self->frameOK == 0) {
          ErrorStatus = AAC_DEC_PARSE_ERROR;
        }
        /* Always put the bitbuffer at the right position after the current Access Unit. */
        FDKpushBiDirectional(bs, unreadBits);
      }
    }

    /* Check the last element. The terminator (ID_END) has to be the last one (even if ER syntax is used). */
    if ( self->frameOK && type != ID_END ) {
      /* Do not overwrite current error */
      if (ErrorStatus == AAC_DEC_OK) {
        ErrorStatus = AAC_DEC_PARSE_ERROR;
      }
      self->frameOK = 0;
    }
  }

  /* More AAC channels than specified by the ASC not allowed. */
  if ( (aacChannels == 0 || aacChannels > self->aacChannels) && !(flags & (AACDEC_CONCEAL|AACDEC_FLUSH)) ) {
    {
      /* Do not overwrite current error */
      if (ErrorStatus == AAC_DEC_OK) {
        ErrorStatus = AAC_DEC_DECODE_FRAME_ERROR;
      }
      self->frameOK = 0;
    }
    aacChannels = 0;
  }
  else if ( aacChannels > self->ascChannels ) {
    /* Do not overwrite current error */
    if (ErrorStatus == AAC_DEC_OK) {
      ErrorStatus = AAC_DEC_UNSUPPORTED_FORMAT;
    }
    self->frameOK = 0;
    aacChannels = 0;
  }

  if ( TRANSPORTDEC_OK != transportDec_CrcCheck(self->hInput) )
  {
    self->frameOK=0;
  }

  /* store or restore the number of channels and the corresponding info */
  if ( self->frameOK && !(flags &(AACDEC_CONCEAL|AACDEC_FLUSH)) ) {
    self->aacChannelsPrev = aacChannels;  /* store */
    FDKmemcpy(self->channelTypePrev, self->channelType, (8)*sizeof(AUDIO_CHANNEL_TYPE));  /* store */
    FDKmemcpy(self->channelIndicesPrev, self->channelIndices, (8)*sizeof(UCHAR));         /* store */
    self->sbrEnabledPrev = self->sbrEnabled;
  } else {
    if (self->aacChannels > 0) {
      aacChannels = self->aacChannelsPrev;  /* restore */
      FDKmemcpy(self->channelType, self->channelTypePrev, (8)*sizeof(AUDIO_CHANNEL_TYPE));  /* restore */
      FDKmemcpy(self->channelIndices, self->channelIndicesPrev, (8)*sizeof(UCHAR));         /* restore */
      self->sbrEnabled = self->sbrEnabledPrev;
     }
  }

  /* Update number of output channels */
  self->streamInfo.aacNumChannels = aacChannels;

 #ifdef TP_PCE_ENABLE
  if (pceRead == 1 && CProgramConfig_IsValid(pce)) {
    /* Set matrix mixdown infos if available from PCE. */
    pcmDmx_SetMatrixMixdownFromPce ( self->hPcmUtils,
                                     pce->MatrixMixdownIndexPresent,
                                     pce->MatrixMixdownIndex,
                                     pce->PseudoSurroundEnable );
  }
 #endif

  /* If there is no valid data to transfrom into time domain, return. */
  if ( ! IS_OUTPUT_VALID(ErrorStatus) ) {
    return ErrorStatus;
  }

  /* Setup the output channel mapping. The table below shows the four possibilities:
   *   # | chCfg | PCE | cChCfg | chOutMapIdx
   *  ---+-------+-----+--------+------------------
   *   1 |  > 0  |  no |    -   | chCfg
   *   2 |   0   | yes |  > 0   | cChCfg
   *   3 |   0   | yes |    0   | aacChannels || 0
   *   4 |   0   |  no |    -   | aacChannels || 0
   *  ---+-------+-----+--------+------------------
   *  Where chCfg is the channel configuration index from ASC and cChCfg is a corresponding chCfg
   *  derived from a given PCE. The variable aacChannels represents the number of channel found
   *  during bitstream decoding. Due to the structure of the mapping table it can only be used for
   *  mapping if its value is smaller than 7. Otherwise we use the fallback (0) which is a simple
   *  pass-through. The possibility #4 should appear only with MPEG-2 (ADTS) streams. This is
   *  mode is called "implicit channel mapping".
   */
  chOutMapIdx = ((self->chMapIndex==0) && (aacChannels<7)) ? aacChannels : self->chMapIndex;

  /*
    Inverse transform
  */
  {
    int stride, offset, c;

    /* Turn on/off DRC modules level normalization in digital domain depending on the limiter status. */
    aacDecoder_drcSetParam( self->hDrcInfo, APPLY_NORMALIZATION, (self->limiterEnableCurr) ? 0 : 1 );
    /* Extract DRC control data and map it to channels (without bitstream delay) */
    aacDecoder_drcProlog (
            self->hDrcInfo,
            bs,
            self->pAacDecoderStaticChannelInfo,
            self->pce.ElementInstanceTag,
            self->chMapping,
            aacChannels
          );

    /* "c" iterates in canonical MPEG channel order */
    for (c=0; c < aacChannels; c++)
    {
      CAacDecoderChannelInfo *pAacDecoderChannelInfo;

      /* Select correct pAacDecoderChannelInfo for current channel */
      if (self->chMapping[c] >= aacChannels) {
        pAacDecoderChannelInfo = self->pAacDecoderChannelInfo[c];
      } else {
        pAacDecoderChannelInfo = self->pAacDecoderChannelInfo[self->chMapping[c]];
      }

      /* Setup offset and stride for time buffer traversal. */
      if (interleaved) {
        stride = aacChannels;
        offset = self->channelOutputMapping[chOutMapIdx][c];
      } else {
        stride = 1;
        offset = self->channelOutputMapping[chOutMapIdx][c] * self->streamInfo.aacSamplesPerFrame;
      }


      if ( flags&AACDEC_FLUSH ) {
        /* Clear pAacDecoderChannelInfo->pSpectralCoefficient because with AACDEC_FLUSH set it contains undefined data. */
        FDKmemclear(pAacDecoderChannelInfo->pSpectralCoefficient, sizeof(FIXP_DBL)*self->streamInfo.aacSamplesPerFrame);
      }

      /*
        Conceal defective spectral data
      */
      CConcealment_Apply(&self->pAacDecoderStaticChannelInfo[c]->concealmentInfo,
                          pAacDecoderChannelInfo,
                          self->pAacDecoderStaticChannelInfo[c],
                         &self->samplingRateInfo,
                          self->streamInfo.aacSamplesPerFrame,
                          0,
                          (self->frameOK && !(flags&AACDEC_CONCEAL)),
                          self->flags
                        );


      if (flags & (AACDEC_INTR|AACDEC_CLRHIST)) {
        /* Reset DRC control data for this channel */
        aacDecoder_drcInitChannelData ( &self->pAacDecoderStaticChannelInfo[c]->drcData );
      }
      /* The DRC module demands to be called with the gain field holding the gain scale. */
      self->extGain[0] = (FIXP_DBL)TDL_GAIN_SCALING;
      /* DRC processing */
      aacDecoder_drcApply (
              self->hDrcInfo,
              self->hSbrDecoder,
              pAacDecoderChannelInfo,
             &self->pAacDecoderStaticChannelInfo[c]->drcData,
              self->extGain,
              c,
              self->streamInfo.aacSamplesPerFrame,
              self->sbrEnabled
            );

      switch (pAacDecoderChannelInfo->renderMode)
      {
        case AACDEC_RENDER_IMDCT:
          CBlock_FrequencyToTime(
                  self->pAacDecoderStaticChannelInfo[c],
                  pAacDecoderChannelInfo,
                  pTimeData + offset,
                  self->streamInfo.aacSamplesPerFrame,
                  stride,
                  (self->frameOK && !(flags&AACDEC_CONCEAL)),
                  self->aacCommonData.workBufferCore1->mdctOutTemp
                  );
          self->extGainDelay = self->streamInfo.aacSamplesPerFrame;
          break;
        case AACDEC_RENDER_ELDFB:
          CBlock_FrequencyToTimeLowDelay(
                  self->pAacDecoderStaticChannelInfo[c],
                  pAacDecoderChannelInfo,
                  pTimeData + offset,
                  self->streamInfo.aacSamplesPerFrame,
                  stride
                  );
          self->extGainDelay = (self->streamInfo.aacSamplesPerFrame*2 -  self->streamInfo.aacSamplesPerFrame/2 - 1)/2;
          break;
        default:
          ErrorStatus = AAC_DEC_UNKNOWN;
          break;
      }
      if ( flags&AACDEC_FLUSH ) {
          FDKmemclear(pAacDecoderChannelInfo->pSpectralCoefficient, sizeof(FIXP_DBL)*self->streamInfo.aacSamplesPerFrame);
        FDKmemclear(self->pAacDecoderStaticChannelInfo[c]->pOverlapBuffer, OverlapBufferSize*sizeof(FIXP_DBL));
      }
    }


    /* Extract DRC control data and map it to channels (with bitstream delay) */
    aacDecoder_drcEpilog (
            self->hDrcInfo,
            bs,
            self->pAacDecoderStaticChannelInfo,
            self->pce.ElementInstanceTag,
            self->chMapping,
            aacChannels
          );
  }

  /* Add additional concealment delay */
  self->streamInfo.outputDelay += CConcealment_GetDelay(&self->concealCommonData) * self->streamInfo.aacSamplesPerFrame;

  /* Map DRC data to StreamInfo structure */
  aacDecoder_drcGetInfo (
            self->hDrcInfo,
           &self->streamInfo.drcPresMode,
           &self->streamInfo.drcProgRefLev
          );

  /* Reorder channel type information tables.  */
  {
    AUDIO_CHANNEL_TYPE types[(8)];
    UCHAR idx[(8)];
    int c;

    FDK_ASSERT(sizeof(self->channelType) == sizeof(types));
    FDK_ASSERT(sizeof(self->channelIndices) == sizeof(idx));

    FDKmemcpy(types, self->channelType, sizeof(types));
    FDKmemcpy(idx, self->channelIndices, sizeof(idx));

    for (c=0; c<aacChannels; c++) {
      self->channelType[self->channelOutputMapping[chOutMapIdx][c]] = types[c];
      self->channelIndices[self->channelOutputMapping[chOutMapIdx][c]] = idx[c];
    }
  }

  self->blockNumber++;

  return ErrorStatus;
}

/*!
  \brief returns the streaminfo pointer

  The function hands back a pointer to the streaminfo structure

  \return pointer to the struct
*/
LINKSPEC_CPP CStreamInfo* CAacDecoder_GetStreamInfo ( HANDLE_AACDECODER self )
{
  if (!self) {
    return NULL;
  }
  return &self->streamInfo;
}




