
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
   Description: joint stereo processing

******************************************************************************/

#include "stereo.h"


#include "aac_rom.h"
#include "FDK_bitstream.h"
#include "channelinfo.h"

enum
{
  L = 0,
  R = 1
};


int CJointStereo_Read(
        HANDLE_FDK_BITSTREAM bs,
        CJointStereoData *pJointStereoData,
        const int windowGroups,
        const int scaleFactorBandsTransmitted,
        const UINT flags
        )
{
  int group,band;

  pJointStereoData->MsMaskPresent = (UCHAR) FDKreadBits(bs,2);

  FDKmemclear(pJointStereoData->MsUsed, scaleFactorBandsTransmitted*sizeof(UCHAR));

  switch (pJointStereoData->MsMaskPresent)
  {
    case 0 : /* no M/S */
      /* all flags are already cleared */
      break ;

    case 1 : /* read ms_used */

      for (group=0; group<windowGroups; group++)
      {
        for (band=0; band<scaleFactorBandsTransmitted; band++)
        {
          pJointStereoData->MsUsed[band] |= (FDKreadBits(bs,1) << group);
        }
      }
      break ;

    case 2 : /* full spectrum M/S */

      for (band=0; band<scaleFactorBandsTransmitted; band++)
      {
        pJointStereoData->MsUsed[band] = 255 ;  /* set all flags to 1 */
      }
      break ;
  }

  return 0;
}

void CJointStereo_ApplyMS(
        CAacDecoderChannelInfo *pAacDecoderChannelInfo[2],
        const SHORT *pScaleFactorBandOffsets,
        const UCHAR *pWindowGroupLength,
        const int windowGroups,
        const int scaleFactorBandsTransmittedL,
        const int scaleFactorBandsTransmittedR
        )
{
  CJointStereoData *pJointStereoData = &pAacDecoderChannelInfo[L]->pComData->jointStereoData;
  int window, group, scaleFactorBandsTransmitted;

  FDK_ASSERT(scaleFactorBandsTransmittedL == scaleFactorBandsTransmittedR);
  scaleFactorBandsTransmitted = scaleFactorBandsTransmittedL;
  for (window = 0, group = 0; group < windowGroups; group++)
  {
    UCHAR groupMask = 1 << group;

    for (int groupwin=0; groupwin<pWindowGroupLength[group]; groupwin++, window++)
    {
      int band;
      FIXP_DBL *leftSpectrum, *rightSpectrum;
      SHORT *leftScale = &pAacDecoderChannelInfo[L]->pDynData->aSfbScale[window*16];
      SHORT *rightScale = &pAacDecoderChannelInfo[R]->pDynData->aSfbScale[window*16];

      leftSpectrum = SPEC(pAacDecoderChannelInfo[L]->pSpectralCoefficient, window, pAacDecoderChannelInfo[L]->granuleLength);
      rightSpectrum = SPEC(pAacDecoderChannelInfo[R]->pSpectralCoefficient, window, pAacDecoderChannelInfo[R]->granuleLength);

      for (band=0; band<scaleFactorBandsTransmitted; band++)
      {
        if (pJointStereoData->MsUsed[band] & groupMask)
        {
          int lScale=leftScale[band];
          int rScale=rightScale[band];
          int commonScale=lScale > rScale ? lScale:rScale;

          /* ISO/IEC 14496-3 Chapter 4.6.8.1.1 :
             M/S joint channel coding can only be used if common_window is ‘1’. */
          FDK_ASSERT(GetWindowSequence(&pAacDecoderChannelInfo[L]->icsInfo) == GetWindowSequence(&pAacDecoderChannelInfo[R]->icsInfo));
          FDK_ASSERT(GetWindowShape(&pAacDecoderChannelInfo[L]->icsInfo) == GetWindowShape(&pAacDecoderChannelInfo[R]->icsInfo));

          commonScale++;
          leftScale[band]=commonScale;
          rightScale[band]=commonScale;

          lScale = fMin(DFRACT_BITS-1, commonScale - lScale);
          rScale = fMin(DFRACT_BITS-1, commonScale - rScale);

          FDK_ASSERT(lScale >= 0 && rScale >= 0);

          for (int index=pScaleFactorBandOffsets[band]; index<pScaleFactorBandOffsets[band+1]; index++)
          {
            FIXP_DBL leftCoefficient  = leftSpectrum [index] ;
            FIXP_DBL rightCoefficient = rightSpectrum [index] ;

            leftCoefficient >>= lScale ;
            rightCoefficient >>= rScale ;

            leftSpectrum [index] = leftCoefficient + rightCoefficient ;
            rightSpectrum [index] = leftCoefficient - rightCoefficient ;
          }
        }
      }
    }
  }

  /* Reset MsUsed flags if no explicit signalling was transmitted. Necessary for intensity coding.
     PNS correlation signalling was mapped before calling CJointStereo_ApplyMS(). */
  if (pJointStereoData->MsMaskPresent == 2) {
    FDKmemclear(pJointStereoData->MsUsed, JointStereoMaximumBands * sizeof(UCHAR));
  }
}

void CJointStereo_ApplyIS(
        CAacDecoderChannelInfo *pAacDecoderChannelInfo[2],
        const SHORT *pScaleFactorBandOffsets,
        const UCHAR *pWindowGroupLength,
        const int windowGroups,
        const int scaleFactorBandsTransmitted,
        const UINT CommonWindow
        )
{
  CJointStereoData *pJointStereoData = &pAacDecoderChannelInfo[L]->pComData->jointStereoData;

  for (int window=0,group=0; group<windowGroups; group++)
  {
    UCHAR *CodeBook;
    SHORT *ScaleFactor;
    UCHAR groupMask = 1 << group;

    CodeBook = &pAacDecoderChannelInfo[R]->pDynData->aCodeBook[group*16];
    ScaleFactor = &pAacDecoderChannelInfo[R]->pDynData->aScaleFactor[group*16];

    for (int groupwin=0; groupwin<pWindowGroupLength[group]; groupwin++, window++)
    {
      FIXP_DBL *leftSpectrum, *rightSpectrum;
      SHORT *leftScale = &pAacDecoderChannelInfo[L]->pDynData->aSfbScale[window*16];
      SHORT *rightScale = &pAacDecoderChannelInfo[R]->pDynData->aSfbScale[window*16];
      int band;

      leftSpectrum = SPEC(pAacDecoderChannelInfo[L]->pSpectralCoefficient, window, pAacDecoderChannelInfo[L]->granuleLength);
      rightSpectrum = SPEC(pAacDecoderChannelInfo[R]->pSpectralCoefficient, window, pAacDecoderChannelInfo[R]->granuleLength);

      for (band=0; band<scaleFactorBandsTransmitted; band++)
      {
        if ((CodeBook [band] == INTENSITY_HCB) ||
            (CodeBook [band] == INTENSITY_HCB2))
        {
          int bandScale = -(ScaleFactor [band] + 100) ;

          int msb = bandScale >> 2 ;
          int lsb = bandScale & 0x03 ;

          /* exponent of MantissaTable[lsb][0] is 1, thus msb+1 below. */
          FIXP_DBL scale = MantissaTable[lsb][0];

          /* ISO/IEC 14496-3 Chapter 4.6.8.2.3 :
             The use of intensity stereo coding is signaled by the use of the pseudo codebooks
             INTENSITY_HCB and INTENSITY_HCB2 (15 and 14) only in the right channel of a
             channel_pair_element() having a common ics_info() (common_window == 1). */
          FDK_ASSERT(GetWindowSequence(&pAacDecoderChannelInfo[L]->icsInfo) == GetWindowSequence(&pAacDecoderChannelInfo[R]->icsInfo));
          FDK_ASSERT(GetWindowShape(&pAacDecoderChannelInfo[L]->icsInfo) == GetWindowShape(&pAacDecoderChannelInfo[R]->icsInfo));

          rightScale[band] = leftScale[band]+msb+1;

          if (CommonWindow && (pJointStereoData->MsUsed[band] & groupMask))
          {

            if (CodeBook[band] == INTENSITY_HCB) /* _NOT_ in-phase */
            {
              scale = -scale ;
            }
          }
          else
          {
            if (CodeBook[band] == INTENSITY_HCB2) /* out-of-phase */
            {
              scale = -scale ;
            }
          }

          for (int index=pScaleFactorBandOffsets[band]; index<pScaleFactorBandOffsets[band+1]; index++)
          {
            rightSpectrum[index] = fMult(leftSpectrum[index],scale);
          }
        }
      }
    }
  }
}
