
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

   Author(s):   Daniel Homm
   Description:

******************************************************************************/

#include "tpdec_latm.h"


#include "FDK_bitstream.h"


#define TPDEC_TRACKINDEX(p,l) (2*(p) + (l))

static
UINT CLatmDemux_GetValue(HANDLE_FDK_BITSTREAM bs)
{
  UCHAR bytesForValue = 0, tmp = 0;
  int value = 0;

  bytesForValue = (UCHAR) FDKreadBits(bs,2);

  for (UINT i=0; i<=bytesForValue; i++) {
    value <<= 8;
    tmp = (UCHAR) FDKreadBits(bs,8);
    value += tmp;
  }

  return value;
}


static
TRANSPORTDEC_ERROR CLatmDemux_ReadAudioMuxElement(
        HANDLE_FDK_BITSTREAM bs,
        CLatmDemux *pLatmDemux,
        int m_muxConfigPresent,
        CSTpCallBacks *pTpDecCallbacks,
        CSAudioSpecificConfig *pAsc,
        int *pfConfigFound
        )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  if (m_muxConfigPresent) {
    pLatmDemux->m_useSameStreamMux = FDKreadBits(bs,1);

    if (!pLatmDemux->m_useSameStreamMux) {
      if ((ErrorStatus = CLatmDemux_ReadStreamMuxConfig(bs, pLatmDemux, pTpDecCallbacks, pAsc, pfConfigFound))) {
        return (ErrorStatus);
      }
    }
  }

  /* If there was no configuration read, its not possible to parse PayloadLengthInfo below. */
  if (! *pfConfigFound) {
    return TRANSPORTDEC_SYNC_ERROR;
  }

  if (pLatmDemux->m_AudioMuxVersionA == 0) {
    /* Do only once per call, because parsing and decoding is done in-line. */
    if ((ErrorStatus = CLatmDemux_ReadPayloadLengthInfo(bs,pLatmDemux))) {
      return (ErrorStatus);
    }
  } else {
    /* audioMuxVersionA > 0 is reserved for future extensions */
    ErrorStatus = TRANSPORTDEC_UNSUPPORTED_FORMAT;
  }

  return (ErrorStatus);
}

TRANSPORTDEC_ERROR CLatmDemux_Read(
        HANDLE_FDK_BITSTREAM bs,
        CLatmDemux *pLatmDemux,
        TRANSPORT_TYPE tt,
        CSTpCallBacks *pTpDecCallbacks,
        CSAudioSpecificConfig *pAsc,
        int *pfConfigFound,
        const INT ignoreBufferFullness
        )
{
  UINT cntBits;
  UINT cmpBufferFullness;
  UINT audioMuxLengthBytesLast = 0;
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  cntBits = FDKgetValidBits(bs);

  if ((INT)cntBits < MIN_LATM_HEADERLENGTH) {
    return TRANSPORTDEC_NOT_ENOUGH_BITS;
  }

  if ((ErrorStatus = CLatmDemux_ReadAudioMuxElement(bs, pLatmDemux, (tt != TT_MP4_LATM_MCP0), pTpDecCallbacks, pAsc, pfConfigFound)))
    return (ErrorStatus);

  if (!ignoreBufferFullness)
  {
    cmpBufferFullness =   24+audioMuxLengthBytesLast*8
                        + pLatmDemux->m_linfo[0][0].m_bufferFullness* pAsc[TPDEC_TRACKINDEX(0,0)].m_channelConfiguration*32;

    /* evaluate buffer fullness */

    if (pLatmDemux->m_linfo[0][0].m_bufferFullness != 0xFF)
    {
      if (!pLatmDemux->BufferFullnessAchieved)
      {
        if (cntBits < cmpBufferFullness)
        {
          /* condition for start of decoding is not fulfilled */

          /* the current frame will not be decoded */
          return TRANSPORTDEC_NOT_ENOUGH_BITS;
        }
        else
        {
          pLatmDemux->BufferFullnessAchieved = 1;
        }
      }
    }
  }

  return (ErrorStatus);
}


TRANSPORTDEC_ERROR CLatmDemux_ReadStreamMuxConfig(
        HANDLE_FDK_BITSTREAM bs,
        CLatmDemux *pLatmDemux,
        CSTpCallBacks *pTpDecCallbacks,
        CSAudioSpecificConfig *pAsc,
        int * pfConfigFound
        )
{
  LATM_LAYER_INFO *p_linfo = NULL;
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  pLatmDemux->m_AudioMuxVersion = FDKreadBits(bs,1);

  if (pLatmDemux->m_AudioMuxVersion == 0) {
    pLatmDemux->m_AudioMuxVersionA = 0;
  } else {
    pLatmDemux->m_AudioMuxVersionA = FDKreadBits(bs,1);
  }

  if (pLatmDemux->m_AudioMuxVersionA == 0) {
    if (pLatmDemux->m_AudioMuxVersion == 1) {
      pLatmDemux->m_taraBufferFullness = CLatmDemux_GetValue(bs);
    }
    pLatmDemux->m_allStreamsSameTimeFraming = FDKreadBits(bs,1);
    pLatmDemux->m_noSubFrames = FDKreadBits(bs,6) + 1;
    pLatmDemux->m_numProgram  = FDKreadBits(bs,4) + 1;

    if (pLatmDemux->m_numProgram > 1) {
      return TRANSPORTDEC_UNSUPPORTED_FORMAT;
    }

    int idCnt = 0;
    for (UINT prog = 0; prog < pLatmDemux->m_numProgram; prog++) {
      pLatmDemux->m_numLayer = FDKreadBits(bs,3) + 1;
      if (pLatmDemux->m_numLayer > 2) {
        return TRANSPORTDEC_UNSUPPORTED_FORMAT;
      }

      for (UINT lay = 0; lay < pLatmDemux->m_numLayer; lay++) {
        p_linfo = &pLatmDemux->m_linfo[prog][lay];

        p_linfo->m_streamID = idCnt++;
        p_linfo->m_frameLengthInBits = 0;

        if( (prog == 0) && (lay == 0) ) {
          pLatmDemux->m_useSameConfig = 0;
        } else {
          pLatmDemux->m_useSameConfig = FDKreadBits(bs,1);
        }

        if (pLatmDemux->m_useSameConfig) {
          if (lay > 1) {
            FDKmemcpy(&pAsc[TPDEC_TRACKINDEX(prog,lay)], &pAsc[TPDEC_TRACKINDEX(prog,lay-1)], sizeof(CSAudioSpecificConfig));
          } else {
            return TRANSPORTDEC_PARSE_ERROR;
          }
        } else {
          if (pLatmDemux->m_AudioMuxVersion == 1)
          {
            FDK_BITSTREAM tmpBs;
            UINT ascStartPos, ascLen=0;

            ascLen = CLatmDemux_GetValue(bs);
            ascStartPos = FDKgetValidBits(bs);
            tmpBs = *bs;
            FDKsyncCache(&tmpBs);
            tmpBs.hBitBuf.ValidBits = ascLen;
            
            /* Read ASC */
            if ((ErrorStatus = AudioSpecificConfig_Parse(&pAsc[TPDEC_TRACKINDEX(prog,lay)], &tmpBs, 1, pTpDecCallbacks))) {
              return (ErrorStatus);
            }
            *pfConfigFound = 1;

            /* The field p_linfo->m_ascLen could be wrong, so check if */
            if ( 0 > (INT)FDKgetValidBits(&tmpBs)) {
              return TRANSPORTDEC_PARSE_ERROR;
            }
            FDKpushFor(bs, ascLen); /* position bitstream after ASC */
          }
          else {
            /* Read ASC */
            if ((ErrorStatus = AudioSpecificConfig_Parse(&pAsc[TPDEC_TRACKINDEX(prog,lay)], bs, 0, pTpDecCallbacks))) {
              return (ErrorStatus);
            }
          }
          {
            int cbError;

            cbError = pTpDecCallbacks->cbUpdateConfig(pTpDecCallbacks->cbUpdateConfigData, &pAsc[TPDEC_TRACKINDEX(prog,lay)]);
            if (cbError != 0) {
              return TRANSPORTDEC_UNKOWN_ERROR;
            }
            *pfConfigFound = 1;
          }
        }

        p_linfo->m_frameLengthType = FDKreadBits(bs,3);
        switch( p_linfo->m_frameLengthType ) {
        case 0:
          p_linfo->m_bufferFullness = FDKreadBits(bs,8);

          if (!pLatmDemux->m_allStreamsSameTimeFraming) {
            if ((lay > 0) && (pAsc[TPDEC_TRACKINDEX(prog,lay)].m_aot == AOT_AAC_SCAL || pAsc[TPDEC_TRACKINDEX(prog,lay)].m_aot == AOT_ER_AAC_SCAL)) {
              return TRANSPORTDEC_UNSUPPORTED_FORMAT;
            }
          }
          break;
        case 1:
          /* frameLength = FDKreadBits(bs,9); */
        case 3:
        case 4:
        case 5:
          /* CELP */
        case 6:
        case 7:
          /* HVXC */
        default:
          return TRANSPORTDEC_PARSE_ERROR; //_LATM_INVALIDFRAMELENGTHTYPE;

        }  /* switch framelengthtype*/

      }  /* layer loop */
    }  /* prog loop */

    pLatmDemux->m_otherDataPresent = FDKreadBits(bs,1);
    pLatmDemux->m_otherDataLength  = 0;

    if (pLatmDemux->m_otherDataPresent) {
      int otherDataLenEsc  = 0;
      do {
        pLatmDemux->m_otherDataLength <<= 8;   // *= 256
        otherDataLenEsc = FDKreadBits(bs,1);
        pLatmDemux->m_otherDataLength += FDKreadBits(bs,8);
      } while (otherDataLenEsc);
    }

    pLatmDemux->m_crcCheckPresent = FDKreadBits(bs,1);
    pLatmDemux->m_crcCheckSum     = 0;

    if (pLatmDemux->m_crcCheckPresent) {
      pLatmDemux->m_crcCheckSum = FDKreadBits(bs,8);
    }

  }
  else {
    /* audioMuxVersionA > 0 is reserved for future extensions */
    ErrorStatus = TRANSPORTDEC_UNSUPPORTED_FORMAT;
  }
  return (ErrorStatus);
}

TRANSPORTDEC_ERROR CLatmDemux_ReadPayloadLengthInfo(HANDLE_FDK_BITSTREAM bs, CLatmDemux *pLatmDemux)
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;
  int totalPayloadBits = 0;

  if( pLatmDemux->m_allStreamsSameTimeFraming == 1 ) {
    for (UINT prog=0; prog<pLatmDemux->m_numProgram; prog++ ) {
      for (UINT lay=0; lay<pLatmDemux->m_numLayer; lay++ ) {
        LATM_LAYER_INFO *p_linfo = &pLatmDemux->m_linfo[prog][lay];

        switch (p_linfo->m_frameLengthType ) {
        case 0:
          p_linfo->m_frameLengthInBits = CLatmDemux_ReadAuChunkLengthInfo(bs);
          totalPayloadBits += p_linfo->m_frameLengthInBits;
          break;
        case 3:
        case 5:
        case 7:
        default:
          return TRANSPORTDEC_PARSE_ERROR; //AAC_DEC_LATM_INVALIDFRAMELENGTHTYPE;
        }
      }
    }
  }
  else {
    ErrorStatus = TRANSPORTDEC_PARSE_ERROR; //AAC_DEC_LATM_TIMEFRAMING;
  }
  if (pLatmDemux->m_audioMuxLengthBytes > (UINT)0 && totalPayloadBits > (int)pLatmDemux->m_audioMuxLengthBytes*8) {
    return TRANSPORTDEC_PARSE_ERROR;
  }
  return (ErrorStatus);
}

int CLatmDemux_ReadAuChunkLengthInfo(HANDLE_FDK_BITSTREAM bs)
{
  UCHAR endFlag;
  int len = 0;

  do {
    UCHAR tmp = (UCHAR) FDKreadBits(bs,8);
    endFlag = (tmp < 255);

    len += tmp;

  } while( endFlag == 0 );

  len <<= 3;  /* convert from bytes to bits */

  return len;
}

int CLatmDemux_GetFrameLengthInBits(CLatmDemux *pLatmDemux)
{
  return pLatmDemux->m_linfo[0][0].m_frameLengthInBits;
}

int CLatmDemux_GetOtherDataPresentFlag(CLatmDemux *pLatmDemux)
{
  return pLatmDemux->m_otherDataPresent ? 1 : 0;
}

int CLatmDemux_GetOtherDataLength(CLatmDemux *pLatmDemux)
{
  return pLatmDemux->m_otherDataLength;
}

UINT CLatmDemux_GetNrOfSubFrames(CLatmDemux *pLatmDemux)
{
  return pLatmDemux->m_noSubFrames;
}

