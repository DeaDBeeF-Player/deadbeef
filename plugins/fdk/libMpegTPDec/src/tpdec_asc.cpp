
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

   Author(s):   Daniel Homm
   Description:

******************************************************************************/

#include "tpdec_lib.h"
#include "tp_data.h"
#ifdef TP_PCE_ENABLE
#include "FDK_crc.h"
#endif


void CProgramConfig_Reset(CProgramConfig *pPce)
{
  pPce->elCounter = 0;
}

void CProgramConfig_Init(CProgramConfig *pPce)
{
  FDKmemclear(pPce, sizeof(CProgramConfig));
#ifdef TP_PCE_ENABLE
  pPce->SamplingFrequencyIndex = 0xf;
#endif
}

int  CProgramConfig_IsValid ( const CProgramConfig *pPce )
{
  return ( (pPce->isValid) ? 1 : 0);
}

#ifdef TP_PCE_ENABLE
#define PCE_HEIGHT_EXT_SYNC  ( 0xAC )

/*
 * Read the extension for height info.
 * return 0 if successfull or -1 if the CRC failed.
 */
static
int CProgramConfig_ReadHeightExt(
                                  CProgramConfig *pPce,
                                  HANDLE_FDK_BITSTREAM bs,
                                  int * const bytesAvailable,
                                  const UINT alignmentAnchor
                                )
{
  int err = 0;
  FDK_CRCINFO crcInfo;    /* CRC state info */
  INT crcReg;
  FDKcrcInit(&crcInfo, 0x07, 0xFF, 8);
  crcReg = FDKcrcStartReg(&crcInfo, bs, 0);
  UINT startAnchor = FDKgetValidBits(bs);

  FDK_ASSERT(pPce != NULL);
  FDK_ASSERT(bs != NULL);
  FDK_ASSERT(bytesAvailable != NULL);

  if ( (startAnchor >= 24) && (*bytesAvailable >= 3)
    && (FDKreadBits(bs,8) == PCE_HEIGHT_EXT_SYNC) )
  {
    int i;

    for (i=0; i < pPce->NumFrontChannelElements; i++)
    {
      pPce->FrontElementHeightInfo[i] = (UCHAR) FDKreadBits(bs,2);
    }
    for (i=0; i < pPce->NumSideChannelElements; i++)
    {
      pPce->SideElementHeightInfo[i] = (UCHAR) FDKreadBits(bs,2);
    }
    for (i=0; i < pPce->NumBackChannelElements; i++)
    {
      pPce->BackElementHeightInfo[i] = (UCHAR) FDKreadBits(bs,2);
    }
    FDKbyteAlign(bs, alignmentAnchor);

    FDKcrcEndReg(&crcInfo, bs, crcReg);
    if ((USHORT)FDKreadBits(bs,8) != FDKcrcGetCRC(&crcInfo)) {
      /* CRC failed */
      err = -1;
    }
  }
  else {
    /* No valid extension data found -> restore the initial bitbuffer state */
    FDKpushBack(bs, startAnchor - FDKgetValidBits(bs));
  }

  /* Always report the bytes read. */
  *bytesAvailable -= (startAnchor - FDKgetValidBits(bs)) >> 3;

  return (err);
}

void CProgramConfig_Read(
                          CProgramConfig *pPce,
                          HANDLE_FDK_BITSTREAM bs,
                          UINT alignmentAnchor
                        )
{
  int i, err = 0;
  int commentBytes;

  pPce->NumEffectiveChannels = 0;
  pPce->NumChannels = 0;
  pPce->ElementInstanceTag = (UCHAR) FDKreadBits(bs,4);
  pPce->Profile = (UCHAR) FDKreadBits(bs,2);
  pPce->SamplingFrequencyIndex = (UCHAR) FDKreadBits(bs,4);
  pPce->NumFrontChannelElements = (UCHAR) FDKreadBits(bs,4);
  pPce->NumSideChannelElements = (UCHAR) FDKreadBits(bs,4);
  pPce->NumBackChannelElements = (UCHAR) FDKreadBits(bs,4);
  pPce->NumLfeChannelElements = (UCHAR) FDKreadBits(bs,2);
  pPce->NumAssocDataElements = (UCHAR) FDKreadBits(bs,3);
  pPce->NumValidCcElements = (UCHAR) FDKreadBits(bs,4);

  if ((pPce->MonoMixdownPresent = (UCHAR) FDKreadBits(bs,1)) != 0)
  {
    pPce->MonoMixdownElementNumber = (UCHAR) FDKreadBits(bs,4);
  }

  if ((pPce->StereoMixdownPresent = (UCHAR) FDKreadBits(bs,1)) != 0)
  {
    pPce->StereoMixdownElementNumber = (UCHAR) FDKreadBits(bs,4);
  }

  if ((pPce->MatrixMixdownIndexPresent = (UCHAR) FDKreadBits(bs,1)) != 0)
  {
    pPce->MatrixMixdownIndex = (UCHAR) FDKreadBits(bs,2);
    pPce->PseudoSurroundEnable = (UCHAR) FDKreadBits(bs,1);
  }

  for (i=0; i < pPce->NumFrontChannelElements; i++)
  {
    pPce->FrontElementIsCpe[i] = (UCHAR) FDKreadBits(bs,1);
    pPce->FrontElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
    pPce->NumChannels += pPce->FrontElementIsCpe[i] ? 2 : 1;
  }

  for (i=0; i < pPce->NumSideChannelElements; i++)
  {
    pPce->SideElementIsCpe[i] = (UCHAR) FDKreadBits(bs,1);
    pPce->SideElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
    pPce->NumChannels += pPce->SideElementIsCpe[i] ? 2 : 1;
  }

  for (i=0; i < pPce->NumBackChannelElements; i++)
  {
    pPce->BackElementIsCpe[i] = (UCHAR) FDKreadBits(bs,1);
    pPce->BackElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
    pPce->NumChannels += pPce->BackElementIsCpe[i] ? 2 : 1;
  }

  pPce->NumEffectiveChannels = pPce->NumChannels;

  for (i=0; i < pPce->NumLfeChannelElements; i++)
  {
    pPce->LfeElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
    pPce->NumChannels += 1;
  }

  for (i=0; i < pPce->NumAssocDataElements; i++)
  {
    pPce->AssocDataElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
  }

  for (i=0; i < pPce->NumValidCcElements; i++)
  {
    pPce->CcElementIsIndSw[i] = (UCHAR) FDKreadBits(bs,1);
    pPce->ValidCcElementTagSelect[i] = (UCHAR) FDKreadBits(bs,4);
  }

  FDKbyteAlign(bs, alignmentAnchor);

  pPce->CommentFieldBytes = (UCHAR) FDKreadBits(bs,8);
  commentBytes = pPce->CommentFieldBytes;

  /* Search for height info extension and read it if available */
  err = CProgramConfig_ReadHeightExt( pPce, bs, &commentBytes, alignmentAnchor );

  for (i=0; i < commentBytes; i++)
  {
    UCHAR text;

    text = (UCHAR)FDKreadBits(bs,8);

    if (i < PC_COMMENTLENGTH)
    {
      pPce->Comment[i] = text;
    }
  }

  pPce->isValid = (err) ? 0 : 1;
}

/*
 * Compare two program configurations.
 * Returns the result of the comparison:
 *  -1 - completely different
 *   0 - completely equal
 *   1 - different but same channel configuration
 *   2 - different channel configuration but same number of channels
 */
int CProgramConfig_Compare ( const CProgramConfig * const pPce1,
                             const CProgramConfig * const pPce2 )
{
  int result = 0;  /* Innocent until proven false. */

  if (FDKmemcmp(pPce1, pPce2, sizeof(CProgramConfig)) != 0)
  { /* Configurations are not completely different.
       So look into details and analyse the channel configurations: */
    result = -1;

    if (pPce1->NumChannels == pPce2->NumChannels)
    { /* Now the logic changes. We first assume to have the same channel configuration
         and then prove if this assumption is true. */
      result = 1;

      /* Front channels */
      if (pPce1->NumFrontChannelElements != pPce2->NumFrontChannelElements) {
        result = 2;  /* different number of front channel elements */
      } else {
        int el, numCh1 = 0, numCh2 = 0;
        for (el = 0; el < pPce1->NumFrontChannelElements; el += 1) {
          if (pPce1->FrontElementHeightInfo[el] != pPce2->FrontElementHeightInfo[el]) {
            result = 2; /* different height info */
            break;
          }
          numCh1 += pPce1->FrontElementIsCpe[el] ? 2 : 1;
          numCh2 += pPce2->FrontElementIsCpe[el] ? 2 : 1;
        }
        if (numCh1 != numCh2) {
          result = 2;  /* different number of front channels */
        }
      }
      /* Side channels */
      if (pPce1->NumSideChannelElements != pPce2->NumSideChannelElements) {
        result = 2;  /* different number of side channel elements */
      } else {
        int el, numCh1 = 0, numCh2 = 0;
        for (el = 0; el < pPce1->NumSideChannelElements; el += 1) {
          if (pPce1->SideElementHeightInfo[el] != pPce2->SideElementHeightInfo[el]) {
            result = 2; /* different height info */
            break;
          }
          numCh1 += pPce1->SideElementIsCpe[el] ? 2 : 1;
          numCh2 += pPce2->SideElementIsCpe[el] ? 2 : 1;
        }
        if (numCh1 != numCh2) {
          result = 2;  /* different number of side channels */
        }
      }
      /* Back channels */
      if (pPce1->NumBackChannelElements != pPce2->NumBackChannelElements) {
        result = 2;  /* different number of back channel elements */
      } else {
        int el, numCh1 = 0, numCh2 = 0;
        for (el = 0; el < pPce1->NumBackChannelElements; el += 1) {
          if (pPce1->BackElementHeightInfo[el] != pPce2->BackElementHeightInfo[el]) {
            result = 2; /* different height info */
            break;
          }
          numCh1 += pPce1->BackElementIsCpe[el] ? 2 : 1;
          numCh2 += pPce2->BackElementIsCpe[el] ? 2 : 1;
        }
        if (numCh1 != numCh2) {
          result = 2;  /* different number of back channels */
        }
      }
      /* LFE channels */
      if (pPce1->NumLfeChannelElements != pPce2->NumLfeChannelElements) {
        result = 2;  /* different number of lfe channels */
      }
      /* LFEs are always SCEs so we don't need to count the channels. */
    }
  }

  return result;
}

void CProgramConfig_GetDefault( CProgramConfig *pPce,
                                const UINT channelConfig )
{
  FDK_ASSERT(pPce != NULL);

  /* Init PCE */
  CProgramConfig_Init(pPce);
  pPce->Profile = 1;  /* Set AAC LC because it is the only supported object type. */

  switch (channelConfig) {
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  case 32: /* 7.1 side channel configuration as defined in FDK_audio.h */
    pPce->NumFrontChannelElements  = 2;
    pPce->FrontElementIsCpe[0]     = 0;
    pPce->FrontElementIsCpe[1]     = 1;
    pPce->NumSideChannelElements   = 1;
    pPce->SideElementIsCpe[0]      = 1;
    pPce->NumBackChannelElements   = 1;
    pPce->BackElementIsCpe[0]      = 1;
    pPce->NumLfeChannelElements    = 1;
    pPce->NumChannels              = 8;
    pPce->NumEffectiveChannels     = 7;
    pPce->isValid                  = 1;
    break;
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  case 12:  /* 3/0/4.1ch surround back */
    pPce->BackElementIsCpe[1]      = 1;
    pPce->NumChannels             += 1;
    pPce->NumEffectiveChannels    += 1;
  case 11:  /* 3/0/3.1ch */
    pPce->NumFrontChannelElements += 2;
    pPce->FrontElementIsCpe[0]     = 0;
    pPce->FrontElementIsCpe[1]     = 1;
    pPce->NumBackChannelElements  += 2;
    pPce->BackElementIsCpe[0]      = 1;
    pPce->BackElementIsCpe[1]     += 0;
    pPce->NumLfeChannelElements   += 1;
    pPce->NumChannels             += 7;
    pPce->NumEffectiveChannels    += 6;
    pPce->isValid                  = 1;
    break;
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  case 14:  /* 2/0/0-3/0/2-0.1ch front height */
    pPce->FrontElementHeightInfo[2] = 1;      /* Top speaker */
  case 7:   /* 5/0/2.1ch front */
    pPce->NumFrontChannelElements += 1;
    pPce->FrontElementIsCpe[2]     = 1;
    pPce->NumChannels             += 2;
    pPce->NumEffectiveChannels    += 2;
  case 6:   /* 3/0/2.1ch */
    pPce->NumLfeChannelElements   += 1;
    pPce->NumChannels             += 1;
  case 5:   /* 3/0/2.0ch */
  case 4:   /* 3/0/1.0ch */
    pPce->NumBackChannelElements  += 1;
    pPce->BackElementIsCpe[0]      = (channelConfig>4) ? 1 : 0;
    pPce->NumChannels             += (channelConfig>4) ? 2 : 1;
    pPce->NumEffectiveChannels    += (channelConfig>4) ? 2 : 1;
  case 3:   /* 3/0/0.0ch */
    pPce->NumFrontChannelElements += 1;
    pPce->FrontElementIsCpe[1]     = 1;
    pPce->NumChannels             += 2;
    pPce->NumEffectiveChannels    += 2;
  case 1:   /* 1/0/0.0ch */
    pPce->NumFrontChannelElements += 1;
    pPce->FrontElementIsCpe[0]     = 0;
    pPce->NumChannels             += 1;
    pPce->NumEffectiveChannels    += 1;
    pPce->isValid                  = 1;
    break;
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  case 2:   /* 2/0/0.ch */
    pPce->NumFrontChannelElements  = 1;
    pPce->FrontElementIsCpe[0]     = 1;
    pPce->NumChannels             += 2;
    pPce->NumEffectiveChannels    += 2;
    pPce->isValid                  = 1;
    break;
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  default:
    pPce->isValid                  = 0;   /* To be explicit! */
    break;
  }

  if (pPce->isValid) {
    /* Create valid element instance tags */
    int el, elTagSce = 0, elTagCpe = 0;

    for (el = 0; el < pPce->NumFrontChannelElements; el += 1) {
      pPce->FrontElementTagSelect[el] = (pPce->FrontElementIsCpe[el]) ? elTagCpe++ : elTagSce++;
    }
    for (el = 0; el < pPce->NumSideChannelElements; el += 1) {
      pPce->SideElementTagSelect[el] = (pPce->SideElementIsCpe[el]) ? elTagCpe++ : elTagSce++;
    }
    for (el = 0; el < pPce->NumBackChannelElements; el += 1) {
      pPce->BackElementTagSelect[el] = (pPce->BackElementIsCpe[el]) ? elTagCpe++ : elTagSce++;
    }
    elTagSce = 0;
    for (el = 0; el < pPce->NumLfeChannelElements; el += 1) {
      pPce->LfeElementTagSelect[el] = elTagSce++;
    }
  }
}
#endif /* TP_PCE_ENABLE */

/**
 * \brief get implicit audio channel type for given channelConfig and MPEG ordered channel index
 * \param channelConfig MPEG channelConfiguration from 1 upto 14
 * \param index MPEG channel order index
 * \return audio channel type.
 */
static
void getImplicitAudioChannelTypeAndIndex(
        AUDIO_CHANNEL_TYPE *chType,
        UCHAR *chIndex,
        UINT channelConfig,
        UINT index
        )
{
  if (index < 3) {
    *chType = ACT_FRONT;
    *chIndex = index;
  } else {
    switch (channelConfig) {
      case 4:  /* SCE, CPE, SCE */
      case 5:  /* SCE, CPE, CPE */
      case 6:  /* SCE, CPE, CPE, LFE */
        switch (index) {
          case 3:
          case 4:
            *chType = ACT_BACK;
            *chIndex = index - 3;
            break;
          case 5:
            *chType = ACT_LFE;
            *chIndex = 0;
            break;
        }
        break;
      case 7:  /* SCE,CPE,CPE,CPE,LFE */
        switch (index) {
          case 3:
          case 4:
            *chType = ACT_FRONT;
            *chIndex = index;
            break;
          case 5:
          case 6:
            *chType = ACT_BACK;
            *chIndex = index - 5;
            break;
          case 7:
            *chType = ACT_LFE;
            *chIndex = 0;
            break;
        }
        break;
      case 11:  /* SCE,CPE,CPE,SCE,LFE */
        if (index < 6) {
          *chType = ACT_BACK;
          *chIndex = index - 3;
        } else {
          *chType = ACT_LFE;
          *chIndex = 0;
        }
        break;
      case 12:  /* SCE,CPE,CPE,CPE,LFE */
        if (index < 7) {
          *chType = ACT_BACK;
          *chIndex = index - 3;
        } else {
          *chType = ACT_LFE;
          *chIndex = 0;
        }
        break;
      case 14:  /* SCE,CPE,CPE,LFE,CPE */
        switch (index) {
          case 3:
          case 4:
            *chType = ACT_BACK;
            *chIndex = index - 3;
            break;
          case 5:
            *chType = ACT_LFE;
            *chIndex = 0;
            break;
          case 6:
          case 7:
            *chType = ACT_FRONT_TOP;
            *chIndex = index - 6;  /* handle the top layer independently */
            break;
        }
        break;
      default:
        *chType = ACT_NONE;
        break;
    }
  }
}

int CProgramConfig_LookupElement(
        CProgramConfig *pPce,
        UINT            channelConfig,
        const UINT      tag,
        const UINT      channelIdx,
        UCHAR           chMapping[],
        AUDIO_CHANNEL_TYPE chType[],
        UCHAR           chIndex[],
        UCHAR          *elMapping,
        MP4_ELEMENT_ID  elList[],
        MP4_ELEMENT_ID  elType
       )
{
  if (channelConfig > 0)
  {
    /* Constant channel mapping must have
       been set during initialization. */
    if ( elType == ID_SCE
      || elType == ID_CPE
      || elType == ID_LFE )
    {
      *elMapping = pPce->elCounter;
      if (elList[pPce->elCounter] != elType) {
        /* Not in the list */
        if ( (channelConfig == 2) && (elType == ID_SCE) )
        { /* This scenario occurs with HE-AAC v2 streams of buggy encoders.
             Due to other decoder implementations decoding of these kind of streams is desired. */
          channelConfig = 1;
        } else {
          return 0;
        }
      }
      /* Assume all front channels */
      getImplicitAudioChannelTypeAndIndex(&chType[channelIdx], &chIndex[channelIdx], channelConfig, channelIdx);
      if (elType == ID_CPE) {
        chType[channelIdx+1] = chType[channelIdx];
        chIndex[channelIdx+1] = chIndex[channelIdx]+1;
      }
      pPce->elCounter++;
    }
    /* Accept all non-channel elements, too. */
    return 1;
  }
  else
  {
#ifdef TP_PCE_ENABLE
    if (!pPce->isValid)
#endif /* TP_PCE_ENABLE */
    {
      /* Implicit channel mapping. */
      if ( elType == ID_SCE
        || elType == ID_CPE
        || elType == ID_LFE )
      {
        /* Store all channel element IDs */
        elList[pPce->elCounter] = elType;
        *elMapping = pPce->elCounter++;
      }
    }
#ifdef  TP_PCE_ENABLE
    else {
      /* Accept the additional channel(s), only if the tag is in the lists */
      int isCpe = 0, i;
      /* Element counter */
      int ec[PC_NUM_HEIGHT_LAYER] = {0};
      /* Channel counters */
      int cc[PC_NUM_HEIGHT_LAYER] = {0};
      int fc[PC_NUM_HEIGHT_LAYER] = {0};
      int sc[PC_NUM_HEIGHT_LAYER] = {0};
      int bc[PC_NUM_HEIGHT_LAYER] = {0};
      int lc = 0;;

      /* General MPEG (PCE) composition rules:
         - Over all:
             <normal height channels><top height channels><bottom height channels>
         - Within each height layer:
             <front channels><side channels><back channels>
         - Exception:
             The LFE channels have no height info and thus they are arranged at the very
             end of the normal height layer channels.
       */

      switch (elType)
      {
      case ID_CPE:
        isCpe = 1;
      case ID_SCE:
        /* search in front channels */
        for (i = 0; i < pPce->NumFrontChannelElements; i++) {
          int heightLayer = pPce->FrontElementHeightInfo[i];
          if (isCpe == pPce->FrontElementIsCpe[i] && pPce->FrontElementTagSelect[i] == tag) {
            int h, elIdx = ec[heightLayer], chIdx = cc[heightLayer];
            AUDIO_CHANNEL_TYPE aChType = (AUDIO_CHANNEL_TYPE)((heightLayer<<4) | ACT_FRONT);
            for (h = heightLayer-1; h >= 0; h-=1) {
              int el;
              /* Count front channels/elements */
              for (el = 0; el < pPce->NumFrontChannelElements; el+=1) {
                if (pPce->FrontElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->FrontElementIsCpe[el]) ? 2 : 1;
                }
              }
              /* Count side channels/elements */
              for (el = 0; el < pPce->NumSideChannelElements; el+=1) {
                if (pPce->SideElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->SideElementIsCpe[el]) ? 2 : 1;
                }
              }
              /* Count back channels/elements */
              for (el = 0; el < pPce->NumBackChannelElements; el+=1) {
                if (pPce->BackElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->BackElementIsCpe[el]) ? 2 : 1;
                }
              }
              if (h == 0) {  /* normal height */
                elIdx += pPce->NumLfeChannelElements;
                chIdx += pPce->NumLfeChannelElements;
              }
            }
            chMapping[chIdx] = channelIdx;
            chType[chIdx] = aChType;
            chIndex[chIdx] = fc[heightLayer];
            if (isCpe) {
              chMapping[chIdx+1] = channelIdx+1;
              chType[chIdx+1] = aChType;
              chIndex[chIdx+1] = fc[heightLayer]+1;
            }
            *elMapping = elIdx;
            return 1;
          }
          ec[heightLayer] += 1;
          if (pPce->FrontElementIsCpe[i]) {
            cc[heightLayer] += 2;
            fc[heightLayer] += 2;
          } else {
            cc[heightLayer] += 1;
            fc[heightLayer] += 1;
          }
        }
        /* search in side channels */
        for (i = 0; i < pPce->NumSideChannelElements; i++) {
          int heightLayer = pPce->SideElementHeightInfo[i];
          if (isCpe == pPce->SideElementIsCpe[i] && pPce->SideElementTagSelect[i] == tag) {
            int h, elIdx = ec[heightLayer], chIdx = cc[heightLayer];
            AUDIO_CHANNEL_TYPE aChType = (AUDIO_CHANNEL_TYPE)((heightLayer<<4) | ACT_SIDE);
            for (h = heightLayer-1; h >= 0; h-=1) {
              int el;
              /* Count front channels/elements */
              for (el = 0; el < pPce->NumFrontChannelElements; el+=1) {
                if (pPce->FrontElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->FrontElementIsCpe[el]) ? 2 : 1;
                }
              }
              /* Count side channels/elements */
              for (el = 0; el < pPce->NumSideChannelElements; el+=1) {
                if (pPce->SideElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->SideElementIsCpe[el]) ? 2 : 1;
                }
              }
              /* Count back channels/elements */
              for (el = 0; el < pPce->NumBackChannelElements; el+=1) {
                if (pPce->BackElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->BackElementIsCpe[el]) ? 2 : 1;
                }
              }
              if (h == 0) {  /* LFE channels belong to the normal height layer */
                elIdx += pPce->NumLfeChannelElements;
                chIdx += pPce->NumLfeChannelElements;
              }
            }
            chMapping[chIdx] = channelIdx;
            chType[chIdx] = aChType;
            chIndex[chIdx] = sc[heightLayer];
            if (isCpe) {
              chMapping[chIdx+1] = channelIdx+1;
              chType[chIdx+1] = aChType;
              chIndex[chIdx+1] = sc[heightLayer]+1;
            }
            *elMapping = elIdx;
            return 1;
          }
          ec[heightLayer] += 1;
          if (pPce->SideElementIsCpe[i]) {
            cc[heightLayer] += 2;
            sc[heightLayer] += 2;
          } else {
            cc[heightLayer] += 1;
            sc[heightLayer] += 1;
          }
        }
        /* search in back channels */
        for (i = 0; i < pPce->NumBackChannelElements; i++) {
          int heightLayer = pPce->BackElementHeightInfo[i];
          if (isCpe == pPce->BackElementIsCpe[i] && pPce->BackElementTagSelect[i] == tag) {
            int h, elIdx = ec[heightLayer], chIdx = cc[heightLayer];
            AUDIO_CHANNEL_TYPE aChType = (AUDIO_CHANNEL_TYPE)((heightLayer<<4) | ACT_BACK);
            for (h = heightLayer-1; h >= 0; h-=1) {
              int el;
              /* Count front channels/elements */
              for (el = 0; el < pPce->NumFrontChannelElements; el+=1) {
                if (pPce->FrontElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->FrontElementIsCpe[el]) ? 2 : 1;
                }
              }
              /* Count side channels/elements */
              for (el = 0; el < pPce->NumSideChannelElements; el+=1) {
                if (pPce->SideElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->SideElementIsCpe[el]) ? 2 : 1;
                }
              }
              /* Count back channels/elements */
              for (el = 0; el < pPce->NumBackChannelElements; el+=1) {
                if (pPce->BackElementHeightInfo[el] == h) {
                  elIdx += 1;
                  chIdx += (pPce->BackElementIsCpe[el]) ? 2 : 1;
                }
              }
              if (h == 0) {  /* normal height */
                elIdx += pPce->NumLfeChannelElements;
                chIdx += pPce->NumLfeChannelElements;
              }
            }
            chMapping[chIdx] = channelIdx;
            chType[chIdx] = aChType;
            chIndex[chIdx] = bc[heightLayer];
            if (isCpe) {
              chMapping[chIdx+1] = channelIdx+1;
              chType[chIdx+1] = aChType;
              chIndex[chIdx+1] = bc[heightLayer]+1;
            }
            *elMapping = elIdx;
            return 1;
          }
          ec[heightLayer] += 1;
          if (pPce->BackElementIsCpe[i]) {
            cc[heightLayer] += 2;
            bc[heightLayer] += 2;
          } else {
            cc[heightLayer] += 1;
            bc[heightLayer] += 1;
          }
        }
        break;

      case ID_LFE:
      { /* Unfortunately we have to go through all normal height
           layer elements to get the position of the LFE channels.
           Start with counting the front channels/elements at normal height */
        for (i = 0; i < pPce->NumFrontChannelElements; i+=1) {
          int heightLayer = pPce->FrontElementHeightInfo[i];
          ec[heightLayer] += 1;
          cc[heightLayer] += (pPce->FrontElementIsCpe[i]) ? 2 : 1;
        }
        /* Count side channels/elements at normal height */
        for (i = 0; i < pPce->NumSideChannelElements; i+=1) {
          int heightLayer = pPce->SideElementHeightInfo[i];
          ec[heightLayer] += 1;
          cc[heightLayer] += (pPce->SideElementIsCpe[i]) ? 2 : 1;
        }
        /* Count back channels/elements at normal height */
        for (i = 0; i < pPce->NumBackChannelElements; i+=1) {
          int heightLayer = pPce->BackElementHeightInfo[i];
          ec[heightLayer] += 1;
          cc[heightLayer] += (pPce->BackElementIsCpe[i]) ? 2 : 1;
        }

        /* search in lfe channels */
        for (i = 0; i < pPce->NumLfeChannelElements; i++) {
          int elIdx = ec[0];  /* LFE channels belong to the normal height layer */
          int chIdx = cc[0];
          if ( pPce->LfeElementTagSelect[i] == tag ) {
            chMapping[chIdx] = channelIdx;
            *elMapping = elIdx;
            chType[chIdx] = ACT_LFE;
            chIndex[chIdx] = lc;
            return 1;
          }
          ec[0] += 1;
          cc[0] += 1;
          lc += 1;
        }
      } break;

      /* Non audio elements */
      case ID_CCE:
        /* search in cce channels */
        for (i = 0; i < pPce->NumValidCcElements; i++) {
          if (pPce->ValidCcElementTagSelect[i] == tag) {
            return 1;
          }
        }
        break;
      case ID_DSE:
        /* search associated data elements */
        for (i = 0; i < pPce->NumAssocDataElements; i++) {
          if (pPce->AssocDataElementTagSelect[i] == tag) {
            return 1;
          }
        }
        break;
      default:
        return 0;
      }
      return 0;  /* not found in any list */
    }
#endif /* TP_PCE_ENABLE */
  }

  return 1;
}

#ifdef  TP_PCE_ENABLE
int CProgramConfig_GetElementTable(
        const CProgramConfig *pPce,
        MP4_ELEMENT_ID  elList[],
        const INT elListSize,
        UCHAR *pChMapIdx
       )
{
  int i, el = 0;

  FDK_ASSERT(elList != NULL);
  FDK_ASSERT(pChMapIdx != NULL);

  *pChMapIdx = 0;

  if ( elListSize
    < pPce->NumFrontChannelElements + pPce->NumSideChannelElements + pPce->NumBackChannelElements + pPce->NumLfeChannelElements
    )
  {
    return 0;
  }

  for (i=0; i < pPce->NumFrontChannelElements; i++)
  {
    elList[el++] = (pPce->FrontElementIsCpe[i]) ?  ID_CPE : ID_SCE;
  }

  for (i=0; i < pPce->NumSideChannelElements; i++)
  {
    elList[el++] = (pPce->SideElementIsCpe[i]) ?  ID_CPE : ID_SCE;
  }

  for (i=0; i < pPce->NumBackChannelElements; i++)
  {
    elList[el++] = (pPce->BackElementIsCpe[i]) ?  ID_CPE : ID_SCE;
  }

  for (i=0; i < pPce->NumLfeChannelElements; i++)
  {
    elList[el++] = ID_LFE;
  }


  /* Find an corresponding channel configuration if possible */
  switch (pPce->NumChannels) {
  case 1: case 2: case 3: case 4: case 5: case 6:
    /* One and two channels have no alternatives. The other ones are mapped directly to the
       corresponding channel config. Because of legacy reasons or for lack of alternative mappings. */
    *pChMapIdx = pPce->NumChannels;
    break;
  case 7:
    {
      C_ALLOC_SCRATCH_START(tmpPce, CProgramConfig, 1);
      /* Create a PCE for the config to test ... */
      CProgramConfig_GetDefault(tmpPce, 11);
      /* ... and compare it with the given one. */
      *pChMapIdx = (!(CProgramConfig_Compare(pPce, tmpPce)&0xE)) ? 11 : 0;
      /* If compare result is 0 or 1 we can be sure that it is channel config 11. */
      C_ALLOC_SCRATCH_END(tmpPce, CProgramConfig, 1);
    }
    break;
  case 8:
    { /* Try the four possible 7.1ch configurations. One after the other. */
      UCHAR testCfg[4] = { 32, 14, 12, 7};
      C_ALLOC_SCRATCH_START(tmpPce, CProgramConfig, 1);
      for (i=0; i<4; i+=1) {
        /* Create a PCE for the config to test ... */
        CProgramConfig_GetDefault(tmpPce, testCfg[i]);
        /* ... and compare it with the given one. */
        if (!(CProgramConfig_Compare(pPce, tmpPce)&0xE)) {
          /* If the compare result is 0 or 1 than the two channel configurations match. */
          /* Explicit mapping of 7.1 side channel configuration to 7.1 rear channel mapping. */
          *pChMapIdx = (testCfg[i]==32) ? 12 : testCfg[i];
        }
      }
      C_ALLOC_SCRATCH_END(tmpPce, CProgramConfig, 1);
    }
    break;
  default:
    /* The PCE does not match any predefined channel configuration. */
    *pChMapIdx = 0;
    break;
  }

  return el;
}
#endif

static AUDIO_OBJECT_TYPE getAOT(HANDLE_FDK_BITSTREAM bs)
{
  int tmp = 0;

  tmp = FDKreadBits(bs,5);
  if (tmp == AOT_ESCAPE) {
    int tmp2 = FDKreadBits(bs,6);
    tmp = 32 + tmp2;
  }

  return (AUDIO_OBJECT_TYPE)tmp;
}

static INT getSampleRate(HANDLE_FDK_BITSTREAM bs, UCHAR *index, int nBits)
{
  INT sampleRate;
  int idx;

  idx = FDKreadBits(bs, nBits);
  if( idx == (1<<nBits)-1 ) {
    if(FDKgetValidBits(bs) < 24) {
      return 0;
    }
    sampleRate = FDKreadBits(bs,24);
  } else {
    sampleRate = SamplingRateTable[idx];
  }

  *index = idx;

  return sampleRate;
}

#ifdef TP_GA_ENABLE
static
TRANSPORTDEC_ERROR GaSpecificConfig_Parse( CSGaSpecificConfig    *self,
                                           CSAudioSpecificConfig *asc,
                                           HANDLE_FDK_BITSTREAM   bs,
                                           UINT                   ascStartAnchor )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  self->m_frameLengthFlag = FDKreadBits(bs,1);

  self->m_dependsOnCoreCoder = FDKreadBits(bs,1);

  if( self->m_dependsOnCoreCoder )
    self->m_coreCoderDelay = FDKreadBits(bs,14);

  self->m_extensionFlag = FDKreadBits(bs,1);

  if( asc->m_channelConfiguration == 0 ) {
    CProgramConfig_Read(&asc->m_progrConfigElement, bs, ascStartAnchor);
  }

  if ((asc->m_aot == AOT_AAC_SCAL) || (asc->m_aot == AOT_ER_AAC_SCAL)) {
    self->m_layer = FDKreadBits(bs,3);
  }

  if (self->m_extensionFlag) {
    if (asc->m_aot == AOT_ER_BSAC) {
      self->m_numOfSubFrame = FDKreadBits(bs,5);
      self->m_layerLength   = FDKreadBits(bs,11);
    }

    if ((asc->m_aot == AOT_ER_AAC_LC)   || (asc->m_aot == AOT_ER_AAC_LTP)  ||
        (asc->m_aot == AOT_ER_AAC_SCAL) || (asc->m_aot == AOT_ER_AAC_LD))
    {
      asc->m_vcb11Flag = FDKreadBits(bs,1); /* aacSectionDataResilienceFlag */
      asc->m_rvlcFlag  = FDKreadBits(bs,1); /* aacScalefactorDataResilienceFlag */
      asc->m_hcrFlag   = FDKreadBits(bs,1); /* aacSpectralDataResilienceFlag */
    }

    self->m_extensionFlag3 = FDKreadBits(bs,1);

  }
  return (ErrorStatus);
}
#endif /* TP_GA_ENABLE */





#ifdef TP_ELD_ENABLE

static INT ld_sbr_header( const CSAudioSpecificConfig *asc,
                           HANDLE_FDK_BITSTREAM hBs,
                           CSTpCallBacks *cb )
{
  const int channelConfiguration = asc->m_channelConfiguration;
  int i = 0;
  INT error = 0;

  if (channelConfiguration == 2) {
    error = cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
  } else {
    error = cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_SCE, i++);
  }

  switch ( channelConfiguration ) {
    case 14:
    case 12:
    case 7:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
    case 6:
    case 5:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
    case 3:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
      break;

    case 11:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
    case 4:
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_CPE, i++);
      error |= cb->cbSbr(cb->cbSbrData, hBs, asc->m_samplingFrequency, asc->m_extensionSamplingFrequency, asc->m_samplesPerFrame, AOT_ER_AAC_ELD, ID_SCE, i++);
      break;
  }

  return error;
}

static
TRANSPORTDEC_ERROR EldSpecificConfig_Parse(
        CSAudioSpecificConfig *asc,
        HANDLE_FDK_BITSTREAM hBs,
        CSTpCallBacks *cb
        )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;
  CSEldSpecificConfig *esc = &asc->m_sc.m_eldSpecificConfig;
  ASC_ELD_EXT_TYPE eldExtType;
  int eldExtLen, len, cnt;

  FDKmemclear(esc, sizeof(CSEldSpecificConfig));

  esc->m_frameLengthFlag = FDKreadBits(hBs, 1 );
  if (esc->m_frameLengthFlag) {
    asc->m_samplesPerFrame = 480;
  } else {
    asc->m_samplesPerFrame = 512;
  }

  asc->m_vcb11Flag = FDKreadBits(hBs, 1 );
  asc->m_rvlcFlag  = FDKreadBits(hBs, 1 );
  asc->m_hcrFlag   = FDKreadBits(hBs, 1 );

  esc->m_sbrPresentFlag     = FDKreadBits(hBs, 1 );

  if (esc->m_sbrPresentFlag == 1) {
    esc->m_sbrSamplingRate    = FDKreadBits(hBs, 1 ); /* 0: single rate, 1: dual rate */
    esc->m_sbrCrcFlag         = FDKreadBits(hBs, 1 );

    asc->m_extensionSamplingFrequency = asc->m_samplingFrequency << esc->m_sbrSamplingRate;

    if (cb->cbSbr != NULL){
      if ( 0 != ld_sbr_header(asc, hBs, cb) ) {
        return TRANSPORTDEC_PARSE_ERROR;
      }
    } else {
      return TRANSPORTDEC_UNSUPPORTED_FORMAT;
    }
  }
  esc->m_useLdQmfTimeAlign = 0;

  /* new ELD syntax */
  /* parse ExtTypeConfigData */
  while ((eldExtType = (ASC_ELD_EXT_TYPE)FDKreadBits(hBs, 4 )) != ELDEXT_TERM) {
    eldExtLen = len = FDKreadBits(hBs, 4 );
    if ( len == 0xf ) {
      len = FDKreadBits(hBs, 8 );
      eldExtLen += len;

      if ( len == 0xff ) {
        len = FDKreadBits(hBs, 16 );
        eldExtLen += len;
      }
    }

    switch (eldExtType) {
      default:
        for(cnt=0; cnt<eldExtLen; cnt++) {
          FDKreadBits(hBs, 8 );
        }
        break;
      /* add future eld extension configs here */
    }
  }
bail:
  return (ErrorStatus);
}
#endif /* TP_ELD_ENABLE */


static
TRANSPORTDEC_ERROR AudioSpecificConfig_ExtensionParse(CSAudioSpecificConfig *self, HANDLE_FDK_BITSTREAM bs, CSTpCallBacks *cb)
{
  TP_ASC_EXTENSION_ID  lastAscExt, ascExtId = ASCEXT_UNKOWN;
  INT  bitsAvailable = (INT)FDKgetValidBits(bs);

  while (bitsAvailable >= 11)
  {
    lastAscExt = ascExtId;
    ascExtId   = (TP_ASC_EXTENSION_ID)FDKreadBits(bs, 11);
    bitsAvailable -= 11;

    switch (ascExtId) {
    case ASCEXT_SBR:    /* 0x2b7 */
      if ( (self->m_extensionAudioObjectType != AOT_SBR) && (bitsAvailable >= 5) ) {
        self->m_extensionAudioObjectType = getAOT(bs);

        if ( (self->m_extensionAudioObjectType == AOT_SBR)
          || (self->m_extensionAudioObjectType == AOT_ER_BSAC) )
        { /* Get SBR extension configuration */
          self->m_sbrPresentFlag = FDKreadBits(bs, 1);
          bitsAvailable -= 1;

          if ( self->m_sbrPresentFlag == 1 ) {
            self->m_extensionSamplingFrequency = getSampleRate(bs, &self->m_extensionSamplingFrequencyIndex, 4);

            if ((INT)self->m_extensionSamplingFrequency <= 0) {
              return TRANSPORTDEC_PARSE_ERROR;
            }
          }
          if ( self->m_extensionAudioObjectType == AOT_ER_BSAC ) {
            self->m_extensionChannelConfiguration = FDKreadBits(bs, 4);
            bitsAvailable -= 4;
          }
        }
        /* Update counter because of variable length fields (AOT and sampling rate) */
        bitsAvailable = (INT)FDKgetValidBits(bs);
      }
      break;
    case ASCEXT_PS:     /* 0x548 */
      if ( (lastAscExt == ASCEXT_SBR)
        && (self->m_extensionAudioObjectType == AOT_SBR)
        && (bitsAvailable > 0) )
      { /* Get PS extension configuration */
        self->m_psPresentFlag = FDKreadBits(bs, 1);
        bitsAvailable -= 1;
      }
      break;
    default:
      /* Just ignore anything. */
      return TRANSPORTDEC_OK;
    }
  }

  return TRANSPORTDEC_OK;
}

/*
 * API Functions
 */

void AudioSpecificConfig_Init(CSAudioSpecificConfig *asc)
{
  FDKmemclear(asc, sizeof(CSAudioSpecificConfig));

  /* Init all values that should not be zero. */
  asc->m_aot                    = AOT_NONE;
  asc->m_samplingFrequencyIndex = 0xf;
  asc->m_epConfig               = -1;
  asc->m_extensionAudioObjectType        = AOT_NULL_OBJECT;
#ifdef TP_PCE_ENABLE
  CProgramConfig_Init(&asc->m_progrConfigElement);
#endif
}

TRANSPORTDEC_ERROR AudioSpecificConfig_Parse(
        CSAudioSpecificConfig *self,
        HANDLE_FDK_BITSTREAM   bs,
        int                    fExplicitBackwardCompatible,
        CSTpCallBacks      *cb
        )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;
  UINT ascStartAnchor = FDKgetValidBits(bs);
  int frameLengthFlag = -1;

  AudioSpecificConfig_Init(self);

  self->m_aot = getAOT(bs);
  self->m_samplingFrequency = getSampleRate(bs, &self->m_samplingFrequencyIndex, 4);
  if (self->m_samplingFrequency <= 0) {
    return TRANSPORTDEC_PARSE_ERROR;
  }

  self->m_channelConfiguration = FDKreadBits(bs,4);

  /* SBR extension ( explicit non-backwards compatible mode ) */
  self->m_sbrPresentFlag = 0;
  self->m_psPresentFlag  = 0;

  if ( self->m_aot == AOT_SBR || self->m_aot == AOT_PS ) {
    self->m_extensionAudioObjectType = AOT_SBR;

    self->m_sbrPresentFlag = 1;
    if ( self->m_aot == AOT_PS ) {
      self->m_psPresentFlag = 1;
    }

    self->m_extensionSamplingFrequency = getSampleRate(bs, &self->m_extensionSamplingFrequencyIndex, 4);
    self->m_aot = getAOT(bs);

  } else {
    self->m_extensionAudioObjectType = AOT_NULL_OBJECT;
  }

  /* Parse whatever specific configs */
  switch (self->m_aot)
  {
#ifdef TP_GA_ENABLE
    case AOT_AAC_LC:
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LD:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_BSAC:
      if ((ErrorStatus = GaSpecificConfig_Parse(&self->m_sc.m_gaSpecificConfig, self, bs, ascStartAnchor)) != TRANSPORTDEC_OK ) {
        return (ErrorStatus);
      }
      frameLengthFlag = self->m_sc.m_gaSpecificConfig.m_frameLengthFlag;
      break;
#endif /* TP_GA_ENABLE */
    case AOT_MPEGS:
      if (cb->cbSsc != NULL) {
        cb->cbSsc(
                cb->cbSscData,
                bs,
                self->m_aot,
                self->m_samplingFrequency,
                1,
                0  /* don't know the length */
                );
      } else {
        return TRANSPORTDEC_UNSUPPORTED_FORMAT;
      }
      break;
#ifdef TP_ELD_ENABLE
    case AOT_ER_AAC_ELD:
      if ((ErrorStatus = EldSpecificConfig_Parse(self, bs, cb)) != TRANSPORTDEC_OK ) {
        return (ErrorStatus);
      }
      frameLengthFlag = self->m_sc.m_eldSpecificConfig.m_frameLengthFlag;
      self->m_sbrPresentFlag = self->m_sc.m_eldSpecificConfig.m_sbrPresentFlag;
      self->m_extensionSamplingFrequency = (self->m_sc.m_eldSpecificConfig.m_sbrSamplingRate+1) * self->m_samplingFrequency;
      break;
#endif /* TP_ELD_ENABLE */

    default:
      return TRANSPORTDEC_UNSUPPORTED_FORMAT;
      break;
  }

  /* Frame length */
  switch (self->m_aot)
  {
#if defined(TP_GA_ENABLE) || defined(TP_USAC_ENABLE)
    case AOT_AAC_LC:
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_BSAC:
    /*case AOT_USAC:*/
      if (!frameLengthFlag)
        self->m_samplesPerFrame = 1024;
      else
        self->m_samplesPerFrame = 960;
      break;
#endif /* TP_GA_ENABLE */
#if defined(TP_GA_ENABLE)
    case AOT_ER_AAC_LD:
      if (!frameLengthFlag)
        self->m_samplesPerFrame = 512;
      else
        self->m_samplesPerFrame = 480;
      break;
#endif /* defined(TP_GA_ENABLE) */
    default:
      break;
  }

  switch (self->m_aot)
  {
    case AOT_ER_AAC_LC:
    case AOT_ER_AAC_LD:
    case AOT_ER_AAC_ELD:
    case AOT_ER_AAC_SCAL:
    case AOT_ER_CELP:
    case AOT_ER_HVXC:
    case AOT_ER_BSAC:
      self->m_epConfig = FDKreadBits(bs,2);

      if (self->m_epConfig > 1) {
        return TRANSPORTDEC_UNSUPPORTED_FORMAT; // EPCONFIG;
      }
      break;
    default:
      break;
  }

  if (fExplicitBackwardCompatible) {
    ErrorStatus = AudioSpecificConfig_ExtensionParse(self, bs, cb);
  }

  return (ErrorStatus);
}

TRANSPORTDEC_ERROR DrmRawSdcAudioConfig_Parse(
        CSAudioSpecificConfig *self,
        HANDLE_FDK_BITSTREAM   bs
        )
{
  TRANSPORTDEC_ERROR ErrorStatus = TRANSPORTDEC_OK;

  AudioSpecificConfig_Init(self);

  if ((INT)FDKgetValidBits(bs) < 20) {
    ErrorStatus = TRANSPORTDEC_PARSE_ERROR;
    goto bail;
  }
  else {
    /* DRM - Audio information data entity - type 9
       - Short Id            2 bits
       - Stream Id           2 bits
       - audio coding        2 bits
       - SBR flag            1 bit
       - audio mode          2 bits
       - audio sampling rate 3 bits
       - text flag           1 bit
       - enhancement flag    1 bit
       - coder field         5 bits
       - rfa                 1 bit  */

    int audioCoding, audioMode, cSamplingFreq, coderField, sfIdx, sbrFlag;

    /* Read the SDC field */
    FDKreadBits(bs,4);   /* Short and Stream Id */

    audioCoding   = FDKreadBits(bs, 2);
    sbrFlag       = FDKreadBits(bs, 1);
    audioMode     = FDKreadBits(bs, 2);
    cSamplingFreq = FDKreadBits(bs, 3);    /* audio sampling rate */

    FDKreadBits(bs, 2);  /* Text and enhancement flag */
    coderField   = FDKreadBits(bs, 5);
    FDKreadBits(bs, 1);  /* rfa */

    /* Evaluate configuration and fill the ASC */
    switch (cSamplingFreq) {
    case 0: /*  8 kHz */
      sfIdx = 11;
      break;
    case 1: /* 12 kHz */
      sfIdx = 9;
      break;
    case 2: /* 16 kHz */
      sfIdx = 8;
      break;
    case 3: /* 24 kHz */
      sfIdx = 6;
      break;
    case 5: /* 48 kHz */
      sfIdx = 3;
      break;
    case 4: /* reserved */
    case 6: /* reserved */
    case 7: /* reserved */
    default:
      ErrorStatus = TRANSPORTDEC_PARSE_ERROR;
      goto bail;
    }

    self->m_samplingFrequencyIndex = sfIdx;
    self->m_samplingFrequency = SamplingRateTable[sfIdx];

    if ( sbrFlag ) {
      UINT i;
      int tmp = -1;
      self->m_sbrPresentFlag = 1;
      self->m_extensionAudioObjectType = AOT_SBR;
      self->m_extensionSamplingFrequency = self->m_samplingFrequency << 1;
      for (i=0; i<(sizeof(SamplingRateTable)/sizeof(SamplingRateTable[0])); i++){
        if (SamplingRateTable[i] == self->m_extensionSamplingFrequency){
          tmp = i;
          break;
        }
      }
      self->m_extensionSamplingFrequencyIndex = tmp;
    }

    switch (audioCoding) {
      case 0: /* AAC */
          self->m_aot = AOT_DRM_AAC     ;  /* Set pseudo AOT for Drm AAC */

        switch (audioMode) {
        case 1: /* parametric stereo */
          self->m_psPresentFlag = 1;
        case 0: /* mono */
          self->m_channelConfiguration = 1;
          break;
        case 2: /* stereo */
          self->m_channelConfiguration = 2;
          break;
        default:
          ErrorStatus = TRANSPORTDEC_PARSE_ERROR;
          goto bail;
        }
        self->m_vcb11Flag = 1;
        self->m_hcrFlag = 1;
        self->m_samplesPerFrame = 960;
        self->m_epConfig = 1;
        break;
      case 1: /* CELP */
        self->m_aot = AOT_ER_CELP;
        self->m_channelConfiguration = 1;
        break;
      case 2: /* HVXC */
        self->m_aot = AOT_ER_HVXC;
        self->m_channelConfiguration = 1;
        break;
      case 3: /* reserved */
      default:
        ErrorStatus = TRANSPORTDEC_PARSE_ERROR;
        self->m_aot = AOT_NONE;
        break;
    }

    if (self->m_psPresentFlag && !self->m_sbrPresentFlag) {
      ErrorStatus = TRANSPORTDEC_PARSE_ERROR;
      goto bail;
    }
  }

bail:
  return (ErrorStatus);
}

