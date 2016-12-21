
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
   Description:

******************************************************************************/

#include "channel.h"
#include "aacdecoder.h"
#include "block.h"
#include "aacdec_tns.h"
#include "FDK_bitstream.h"
#include "FDK_tools_rom.h"

#include "conceal.h"

#include "rvlc.h"

#include "aacdec_hcr.h"


static
void MapMidSideMaskToPnsCorrelation (CAacDecoderChannelInfo *pAacDecoderChannelInfo[2])
{
  int group;

  for (group = 0 ; group < pAacDecoderChannelInfo[L]->icsInfo.WindowGroups; group++) {
    UCHAR groupMask = 1 << group;

    for (UCHAR band = 0 ; band < pAacDecoderChannelInfo[L]->icsInfo.MaxSfBands; band++) {
      if (pAacDecoderChannelInfo[L]->pComData->jointStereoData.MsUsed[band] & groupMask) { /* channels are correlated */
        CPns_SetCorrelation(&pAacDecoderChannelInfo[L]->data.aac.PnsData, group, band, 0);

        if (CPns_IsPnsUsed(&pAacDecoderChannelInfo[L]->data.aac.PnsData, group, band) &&
            CPns_IsPnsUsed(&pAacDecoderChannelInfo[R]->data.aac.PnsData, group, band))
          pAacDecoderChannelInfo[L]->pComData->jointStereoData.MsUsed[band] ^= groupMask; /* clear the groupMask-bit */
      }
    }
  }
}

/*!
  \brief Decode channel pair element

  The function decodes a channel pair element.

  \return  none
*/
void CChannelElement_Decode( CAacDecoderChannelInfo *pAacDecoderChannelInfo[2], /*!< pointer to aac decoder channel info */
                             CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[2],
                             SamplingRateInfo *pSamplingRateInfo,
                             UINT  flags,
                             int el_channels)
{
  int ch, maybe_jstereo = 0;

  maybe_jstereo = (el_channels > 1);

  for (ch = 0; ch < el_channels; ch++) {
    if ( pAacDecoderChannelInfo[ch]->renderMode == AACDEC_RENDER_IMDCT
      || pAacDecoderChannelInfo[ch]->renderMode == AACDEC_RENDER_ELDFB )
    {
      CBlock_InverseQuantizeSpectralData(pAacDecoderChannelInfo[ch], pSamplingRateInfo);
    }
  }



  if (maybe_jstereo) {
    /* apply ms */
    if (pAacDecoderChannelInfo[L]->pDynData->RawDataInfo.CommonWindow) {
      int maxSfBandsL = GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo[L]->icsInfo);
      int maxSfBandsR = GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo[R]->icsInfo);
      if (pAacDecoderChannelInfo[L]->data.aac.PnsData.PnsActive || pAacDecoderChannelInfo[R]->data.aac.PnsData.PnsActive) {
        MapMidSideMaskToPnsCorrelation(pAacDecoderChannelInfo);
      }

      CJointStereo_ApplyMS(pAacDecoderChannelInfo,
                           GetScaleFactorBandOffsets(&pAacDecoderChannelInfo[L]->icsInfo, pSamplingRateInfo),
                           GetWindowGroupLengthTable(&pAacDecoderChannelInfo[L]->icsInfo),
                           GetWindowGroups(&pAacDecoderChannelInfo[L]->icsInfo),
                           maxSfBandsL,
                           maxSfBandsR);
    }

    /* apply intensity stereo */ /* modifies pAacDecoderChannelInfo[]->aSpecSfb */
    CJointStereo_ApplyIS(pAacDecoderChannelInfo,
                         GetScaleFactorBandOffsets(&pAacDecoderChannelInfo[L]->icsInfo, pSamplingRateInfo),
                         GetWindowGroupLengthTable(&pAacDecoderChannelInfo[L]->icsInfo),
                         GetWindowGroups(&pAacDecoderChannelInfo[L]->icsInfo),
                         GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo[L]->icsInfo),
                         pAacDecoderChannelInfo[L]->pDynData->RawDataInfo.CommonWindow ? 1 : 0);

  }

  for (ch = 0; ch < el_channels; ch++)
  {
    {
      /* write pAacDecoderChannelInfo[ch]->specScale */
      CBlock_ScaleSpectralData(pAacDecoderChannelInfo[ch], pSamplingRateInfo);

      ApplyTools (pAacDecoderChannelInfo, pSamplingRateInfo, flags, ch);
    }

  }

  CRvlc_ElementCheck(
          pAacDecoderChannelInfo,
          pAacDecoderStaticChannelInfo,
          flags,
          el_channels
          );
}

void CChannel_CodebookTableInit(CAacDecoderChannelInfo *pAacDecoderChannelInfo)
{
  int b, w, maxBands, maxWindows;
  int maxSfb = GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo->icsInfo);
  UCHAR *pCodeBook = pAacDecoderChannelInfo->pDynData->aCodeBook;

  if ( IsLongBlock(&pAacDecoderChannelInfo->icsInfo) ) {
    maxBands = 64;
    maxWindows = 1;
  } else {
    maxBands = 16;
    maxWindows = 8;
  }

  for (w = 0; w<maxWindows; w++) {
    for (b = 0; b < maxSfb; b++) {
      pCodeBook[b] = ESCBOOK;
    }
    for (; b<maxBands; b++) {
      pCodeBook[b] = ZERO_HCB;
    }
    pCodeBook += maxBands;
  }
}


/*
 * Arbitrary order bitstream parser
 */

AAC_DECODER_ERROR CChannelElement_Read(HANDLE_FDK_BITSTREAM hBs,
                                       CAacDecoderChannelInfo *pAacDecoderChannelInfo[],
                                       CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo[],
                                       const AUDIO_OBJECT_TYPE aot,
                                       const SamplingRateInfo *pSamplingRateInfo,
                                       const UINT  flags,
                                       const UINT  frame_length,
                                       const UCHAR numberOfChannels,
                                       const SCHAR epConfig,
                                       HANDLE_TRANSPORTDEC pTpDec
                                       )
{
  AAC_DECODER_ERROR error = AAC_DEC_OK;
  const element_list_t *list;
  int i, ch, decision_bit;
  int crcReg1 = -1, crcReg2 = -1;

  FDK_ASSERT( (numberOfChannels == 1) || (numberOfChannels == 2) );

  /* Get channel element sequence table */
  list = getBitstreamElementList(aot, epConfig, numberOfChannels, 0);
  if (list == NULL) {
    error = AAC_DEC_UNSUPPORTED_FORMAT;
    goto bail;
  }

  CTns_Reset(&pAacDecoderChannelInfo[0]->pDynData->TnsData);
  if (numberOfChannels == 2) {
    CTns_Reset(&pAacDecoderChannelInfo[1]->pDynData->TnsData);
  }

  if (flags & (AC_ELD|AC_SCALABLE)) {
    pAacDecoderChannelInfo[0]->pDynData->RawDataInfo.CommonWindow = 1;
    if (numberOfChannels == 2) {
      pAacDecoderChannelInfo[1]->pDynData->RawDataInfo.CommonWindow = pAacDecoderChannelInfo[0]->pDynData->RawDataInfo.CommonWindow;
    }
    if (numberOfChannels == 2) {
      pAacDecoderChannelInfo[1]->pDynData->RawDataInfo.CommonWindow = pAacDecoderChannelInfo[0]->pDynData->RawDataInfo.CommonWindow;
    }
  }

  /* Iterate through sequence table */
  i = 0;
  ch = 0;
  decision_bit = 0;
  do {
    switch (list->id[i]) {
    case element_instance_tag:
      pAacDecoderChannelInfo[0]->ElementInstanceTag = FDKreadBits(hBs, 4);
      if (numberOfChannels == 2) {
        pAacDecoderChannelInfo[1]->ElementInstanceTag = pAacDecoderChannelInfo[0]->ElementInstanceTag;
      }
      break;
    case common_window:
      decision_bit = pAacDecoderChannelInfo[ch]->pDynData->RawDataInfo.CommonWindow = FDKreadBits(hBs, 1);
      if (numberOfChannels == 2) {
        pAacDecoderChannelInfo[1]->pDynData->RawDataInfo.CommonWindow = pAacDecoderChannelInfo[0]->pDynData->RawDataInfo.CommonWindow;
      }
      break;
    case ics_info:
      /* Read individual channel info */
      error = IcsRead( hBs,
                      &pAacDecoderChannelInfo[ch]->icsInfo,
                       pSamplingRateInfo,
                       flags );

      if (numberOfChannels == 2 && pAacDecoderChannelInfo[0]->pDynData->RawDataInfo.CommonWindow) {
        pAacDecoderChannelInfo[1]->icsInfo = pAacDecoderChannelInfo[0]->icsInfo;
      }
      break;


    case ltp_data_present:
      if (FDKreadBits(hBs, 1) != 0) {
        error = AAC_DEC_UNSUPPORTED_PREDICTION;
      }
      break;

    case ms:
      if ( CJointStereo_Read(
              hBs,
             &pAacDecoderChannelInfo[0]->pComData->jointStereoData, 
              GetWindowGroups(&pAacDecoderChannelInfo[0]->icsInfo),
              GetScaleMaxFactorBandsTransmitted(&pAacDecoderChannelInfo[0]->icsInfo,
                                                &pAacDecoderChannelInfo[1]->icsInfo),
              flags) )
      {
        error = AAC_DEC_PARSE_ERROR;
      }
      break;

    case global_gain:
      pAacDecoderChannelInfo[ch]->pDynData->RawDataInfo.GlobalGain = (UCHAR) FDKreadBits(hBs,8);
      break;

    case section_data:
      error = CBlock_ReadSectionData( hBs,
                                      pAacDecoderChannelInfo[ch],
                                      pSamplingRateInfo,
                                      flags );
      break;


    case scale_factor_data:
      if (flags & AC_ER_RVLC) {
        /* read RVLC data from bitstream (error sens. cat. 1) */ 
        CRvlc_Read(pAacDecoderChannelInfo[ch], hBs);
      }
      else
      {
        error = CBlock_ReadScaleFactorData(pAacDecoderChannelInfo[ch], hBs, flags);
      }
      break;

    case pulse:
      if ( CPulseData_Read( hBs,
                           &pAacDecoderChannelInfo[ch]->pDynData->specificTo.aac.PulseData,
                            pSamplingRateInfo->ScaleFactorBands_Long, /* pulse data is only allowed to be present in long blocks! */
                            (void*)&pAacDecoderChannelInfo[ch]->icsInfo,
                            frame_length
                          ) != 0 )
      {
        error = AAC_DEC_DECODE_FRAME_ERROR;
      }
      break;
    case tns_data_present:
      CTns_ReadDataPresentFlag(hBs, &pAacDecoderChannelInfo[ch]->pDynData->TnsData);
      break;
    case tns_data:
      /* tns_data_present is checked inside CTns_Read(). */
      error = CTns_Read(hBs, &pAacDecoderChannelInfo[ch]->pDynData->TnsData, &pAacDecoderChannelInfo[ch]->icsInfo, flags);
      break;

    case gain_control_data:
      break;
    
    case gain_control_data_present:
      if (FDKreadBits(hBs, 1)) {
        error = AAC_DEC_UNSUPPORTED_GAIN_CONTROL_DATA;
      }
      break;

    case esc2_rvlc:
      if (flags & AC_ER_RVLC) {
        CRvlc_Decode(
                pAacDecoderChannelInfo[ch],
                pAacDecoderStaticChannelInfo[ch],
                hBs
                );
      }
      break;

    case esc1_hcr:
      if (flags & AC_ER_HCR) {
        CHcr_Read(hBs, pAacDecoderChannelInfo[ch] );
      }
      break;

    case spectral_data:
      error = CBlock_ReadSpectralData( hBs,
                                       pAacDecoderChannelInfo[ch],
                                       pSamplingRateInfo,
                                       flags );
      if (flags & AC_ELD) {
        pAacDecoderChannelInfo[ch]->renderMode = AACDEC_RENDER_ELDFB;
      } else {
        pAacDecoderChannelInfo[ch]->renderMode = AACDEC_RENDER_IMDCT;
      }
      break;


      /* CRC handling */
    case adtscrc_start_reg1:
      if (pTpDec != NULL) {
        crcReg1 = transportDec_CrcStartReg(pTpDec, 192);
      }
      break;
    case adtscrc_start_reg2:
      if (pTpDec != NULL) {
        crcReg2 = transportDec_CrcStartReg(pTpDec, 128);
      }
      break;
    case adtscrc_end_reg1:
    case drmcrc_end_reg:
      if (pTpDec != NULL) {
        transportDec_CrcEndReg(pTpDec, crcReg1);
      }
      break;
    case adtscrc_end_reg2:
      if (pTpDec != NULL) {
        transportDec_CrcEndReg(pTpDec, crcReg2);
      }
      break;
    case drmcrc_start_reg:
      if (pTpDec != NULL) {
        crcReg1 = transportDec_CrcStartReg(pTpDec, 0);
      }
      break;

      /* Non data cases */
    case next_channel:
      ch = (ch + 1) % numberOfChannels;
      break;
    case link_sequence:
      list = list->next[decision_bit];
      i=-1;
      break;

    default:
      error = AAC_DEC_UNSUPPORTED_FORMAT;
      break;
    }

    if (error != AAC_DEC_OK) {
      goto bail;
    }

    i++;

  } while (list->id[i] != end_of_sequence);

bail:
  return error;
}
