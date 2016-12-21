
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
   Description: long/short-block decoding

******************************************************************************/

#include "block.h"

#include "aac_rom.h"
#include "FDK_bitstream.h"
#include "FDK_tools_rom.h"




#include "aacdec_hcr.h"
#include "rvlc.h"


#if defined(__arm__)
#include "arm/block_arm.cpp"
#endif

/*!
  \brief Read escape sequence of codeword

  The function reads the escape sequence from the bitstream,
  if the absolute value of the quantized coefficient has the
  value 16.

  \return  quantized coefficient
*/
LONG CBlock_GetEscape(HANDLE_FDK_BITSTREAM bs, /*!< pointer to bitstream */
                     const LONG q)        /*!< quantized coefficient */
{
  LONG i, off, neg ;

  if (q < 0)
  {
    if (q != -16) return q;
    neg = 1;
  }
  else
  {
    if (q != +16) return q;
    neg = 0;
  }

  for (i=4; ; i++)
  {
    if (FDKreadBits(bs,1) == 0)
      break;
  }

  if (i > 16)
  {
    if (i - 16 > CACHE_BITS) { /* cannot read more than "CACHE_BITS" bits at once in the function FDKreadBits() */
      return (MAX_QUANTIZED_VALUE + 1); /* returning invalid value that will be captured later */
    }

    off = FDKreadBits(bs,i-16) << 16;
    off |= FDKreadBits(bs,16);
  }
  else
  {
    off = FDKreadBits(bs,i);
  }

  i = off + (1 << i);

  if (neg) i = -i;

  return i;
}

AAC_DECODER_ERROR CBlock_ReadScaleFactorData(
        CAacDecoderChannelInfo *pAacDecoderChannelInfo,
        HANDLE_FDK_BITSTREAM bs,
        UINT flags
        )
{
  int temp;
  int band;
  int group;
  int position = 0; /* accu for intensity delta coding */
  int factor = pAacDecoderChannelInfo->pDynData->RawDataInfo.GlobalGain; /* accu for scale factor delta coding */
  UCHAR *pCodeBook = pAacDecoderChannelInfo->pDynData->aCodeBook;
  SHORT *pScaleFactor = pAacDecoderChannelInfo->pDynData->aScaleFactor;
  const CodeBookDescription *hcb =&AACcodeBookDescriptionTable[BOOKSCL];

  int ScaleFactorBandsTransmitted = GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo->icsInfo);
  for (group=0; group < GetWindowGroups(&pAacDecoderChannelInfo->icsInfo); group++)
  {
    for (band=0; band < ScaleFactorBandsTransmitted; band++)
    {
      switch (pCodeBook[group*16+band]) {

      case ZERO_HCB: /* zero book */
        pScaleFactor[group*16+band] = 0;
        break;

      default: /* decode scale factor */
        {
          temp = CBlock_DecodeHuffmanWord(bs,hcb);
          factor += temp - 60; /* MIDFAC 1.5 dB */
        }
        pScaleFactor[group*16+band] = factor - 100;
        break;

      case INTENSITY_HCB: /* intensity steering */
      case INTENSITY_HCB2:
        temp = CBlock_DecodeHuffmanWord(bs,hcb);
        position += temp - 60;
        pScaleFactor[group*16+band] = position - 100;
        break;

      case NOISE_HCB: /* PNS */
        if (flags & (AC_MPS_RES|AC_USAC|AC_RSVD50)) {
          return AAC_DEC_PARSE_ERROR;
        }
        CPns_Read( &pAacDecoderChannelInfo->data.aac.PnsData, bs, hcb, pAacDecoderChannelInfo->pDynData->aScaleFactor, pAacDecoderChannelInfo->pDynData->RawDataInfo.GlobalGain, band, group);
        break;
      }
    }
  }

  return AAC_DEC_OK;
}

void CBlock_ScaleSpectralData(CAacDecoderChannelInfo *pAacDecoderChannelInfo, SamplingRateInfo *pSamplingRateInfo)
{
  int band;
  int window;
  const SHORT * RESTRICT pSfbScale  = pAacDecoderChannelInfo->pDynData->aSfbScale;
  SHORT * RESTRICT pSpecScale = pAacDecoderChannelInfo->specScale;
  int groupwin,group;
  const SHORT * RESTRICT BandOffsets = GetScaleFactorBandOffsets(&pAacDecoderChannelInfo->icsInfo, pSamplingRateInfo);
  SPECTRAL_PTR RESTRICT pSpectralCoefficient = pAacDecoderChannelInfo->pSpectralCoefficient;


  FDKmemclear(pSpecScale, 8*sizeof(SHORT));

  int max_band = GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo->icsInfo);
  for (window=0, group=0; group < GetWindowGroups(&pAacDecoderChannelInfo->icsInfo); group++)
  {
    for (groupwin=0; groupwin < GetWindowGroupLength(&pAacDecoderChannelInfo->icsInfo,group); groupwin++, window++)
    {
      int SpecScale_window = pSpecScale[window];
      FIXP_DBL *pSpectrum = SPEC(pSpectralCoefficient, window,  pAacDecoderChannelInfo->granuleLength);

      /* find scaling for current window */
      for (band=0; band < max_band; band++)
      {
        SpecScale_window = fMax(SpecScale_window, (int)pSfbScale[window*16+band]);
      }

      if (pAacDecoderChannelInfo->pDynData->TnsData.Active) {
        SpecScale_window += TNS_SCALE;
      }

      /* store scaling of current window */
      pSpecScale[window] = SpecScale_window;

#ifdef FUNCTION_CBlock_ScaleSpectralData_func1

      CBlock_ScaleSpectralData_func1(pSpectrum, max_band, BandOffsets, SpecScale_window, pSfbScale, window);

#else /* FUNCTION_CBlock_ScaleSpectralData_func1 */
      for (band=0; band < max_band; band++)
      {
        int scale = SpecScale_window - pSfbScale[window*16+band];
        if (scale)
        {
          /* following relation can be used for optimizations: (BandOffsets[i]%4) == 0 for all i */
          int max_index = BandOffsets[band+1];
          for (int index = BandOffsets[band]; index < max_index; index++)
          {
            pSpectrum[index] >>= scale;
          }
        }
      }
#endif  /* FUNCTION_CBlock_ScaleSpectralData_func1 */
    }
  }

}

AAC_DECODER_ERROR CBlock_ReadSectionData(HANDLE_FDK_BITSTREAM bs,
                                         CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                                         const SamplingRateInfo *pSamplingRateInfo,
                                         const UINT  flags)
{
  int top, band;
  int sect_len, sect_len_incr;
  int group;
  UCHAR sect_cb;
  UCHAR *pCodeBook = pAacDecoderChannelInfo->pDynData->aCodeBook;
  /* HCR input (long) */
  SHORT *pNumLinesInSec    = pAacDecoderChannelInfo->pDynData->specificTo.aac.aNumLineInSec4Hcr;
  int    numLinesInSecIdx  = 0;
  UCHAR *pHcrCodeBook      = pAacDecoderChannelInfo->pDynData->specificTo.aac.aCodeBooks4Hcr;
  const SHORT *BandOffsets = GetScaleFactorBandOffsets(&pAacDecoderChannelInfo->icsInfo, pSamplingRateInfo);
  pAacDecoderChannelInfo->pDynData->specificTo.aac.numberSection = 0;
  AAC_DECODER_ERROR ErrorStatus = AAC_DEC_OK;

  FDKmemclear(pCodeBook, sizeof(UCHAR)*(8*16));

  const int nbits = (IsLongBlock(&pAacDecoderChannelInfo->icsInfo) == 1) ? 5 : 3;

  int sect_esc_val = (1 << nbits) - 1 ;

  UCHAR ScaleFactorBandsTransmitted = GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo->icsInfo);
  for (group=0; group<GetWindowGroups(&pAacDecoderChannelInfo->icsInfo); group++)
  {
    for (band=0; band < ScaleFactorBandsTransmitted; )
    {
      sect_len = 0;
      if ( flags & AC_ER_VCB11 )  {
        sect_cb = (UCHAR) FDKreadBits(bs,5);
      }
      else
        sect_cb = (UCHAR) FDKreadBits(bs,4);

      if ( ((flags & AC_ER_VCB11) == 0) || ( sect_cb < 11 ) || ((sect_cb > 11) && (sect_cb < 16)) ) {
        sect_len_incr = FDKreadBits(bs, nbits);
        while (sect_len_incr == sect_esc_val)
        {
          sect_len += sect_esc_val;
          sect_len_incr = FDKreadBits(bs, nbits);
        }
      }
      else {
        sect_len_incr = 1;
      }

      sect_len += sect_len_incr;


      top = band + sect_len;

      if (flags & AC_ER_HCR) {
        /* HCR input (long) -- collecting sideinfo (for HCR-_long_ only) */
        if (numLinesInSecIdx >= MAX_SFB_HCR) {
          return AAC_DEC_PARSE_ERROR;
        }
        pNumLinesInSec[numLinesInSecIdx] = BandOffsets[top] - BandOffsets[band];
        numLinesInSecIdx++;
        if (sect_cb == BOOKSCL)
        {
          return AAC_DEC_INVALID_CODE_BOOK;
        } else {
          *pHcrCodeBook++ = sect_cb;
        }
        pAacDecoderChannelInfo->pDynData->specificTo.aac.numberSection++;
      }

      /* Check spectral line limits */
      if (IsLongBlock( &(pAacDecoderChannelInfo->icsInfo) ))
      {
        if (top > 64) {
          return AAC_DEC_DECODE_FRAME_ERROR;
        }
      } else { /* short block */
        if (top + group*16 > (8 * 16)) {
          return AAC_DEC_DECODE_FRAME_ERROR;
        }
      }

      /* Check if decoded codebook index is feasible */
      if ( (sect_cb == BOOKSCL)
       || ( (sect_cb == INTENSITY_HCB || sect_cb == INTENSITY_HCB2) && pAacDecoderChannelInfo->pDynData->RawDataInfo.CommonWindow == 0)
         )
      {
        return AAC_DEC_INVALID_CODE_BOOK;
      }

      /* Store codebook index */
      for (; band < top; band++)
      {
        pCodeBook[group*16+band] = sect_cb;
      }
    }
  }


  return ErrorStatus;
}

/* mso: provides a faster way to i-quantize a whole band in one go */

/**
 * \brief inverse quantize one sfb. Each value of the sfb is processed according to the
 *        formula: spectrum[i] = Sign(spectrum[i]) * Matissa(spectrum[i])^(4/3) * 2^(lsb/4).
 * \param spectrum pointer to first line of the sfb to be inverse quantized.
 * \param noLines number of lines belonging to the sfb.
 * \param lsb last 2 bits of the scale factor of the sfb.
 * \param scale max allowed shift scale for the sfb.
 */
static
void InverseQuantizeBand( FIXP_DBL * RESTRICT spectrum,
                              INT noLines,
                              INT lsb,
                              INT scale )
{
    const FIXP_DBL * RESTRICT InverseQuantTabler=(FIXP_DBL *)InverseQuantTable;
    const FIXP_DBL * RESTRICT MantissaTabler=(FIXP_DBL *)MantissaTable[lsb];
    const SCHAR* RESTRICT ExponentTabler=(SCHAR*)ExponentTable[lsb];

    FIXP_DBL *ptr = spectrum;
    FIXP_DBL signedValue;

    FDK_ASSERT(noLines>2);
    for (INT i=noLines; i--; )
    {
        if ((signedValue = *ptr++) != FL2FXCONST_DBL(0))
        {
          FIXP_DBL value = fAbs(signedValue);
          UINT freeBits = CntLeadingZeros(value);
          UINT exponent = 32 - freeBits;

          UINT x = (UINT) (LONG)value << (INT) freeBits;
          x <<= 1;                                  /* shift out sign bit to avoid masking later on */
          UINT tableIndex = x >> 24;
          x = (x >> 20) &  0x0F;

          UINT r0=(UINT)(LONG)InverseQuantTabler[tableIndex+0];
          UINT r1=(UINT)(LONG)InverseQuantTabler[tableIndex+1];
          UINT temp= (r1 - r0)*x + (r0 << 4);

          value = fMultDiv2((FIXP_DBL)temp, MantissaTabler[exponent]);

          /* + 1 compensates fMultDiv2() */
          scaleValueInPlace(&value, scale + ExponentTabler[exponent] + 1);

          signedValue = (signedValue < (FIXP_DBL)0) ? -value : value;
          ptr[-1] = signedValue;
        }
    }
}

AAC_DECODER_ERROR CBlock_InverseQuantizeSpectralData(CAacDecoderChannelInfo *pAacDecoderChannelInfo, SamplingRateInfo *pSamplingRateInfo)
{
  int window, group, groupwin, band;
  int ScaleFactorBandsTransmitted = GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo->icsInfo);
  UCHAR *RESTRICT pCodeBook = pAacDecoderChannelInfo->pDynData->aCodeBook;
  SHORT *RESTRICT pSfbScale = pAacDecoderChannelInfo->pDynData->aSfbScale;
  SHORT *RESTRICT pScaleFactor = pAacDecoderChannelInfo->pDynData->aScaleFactor;
  const SHORT *RESTRICT BandOffsets = GetScaleFactorBandOffsets(&pAacDecoderChannelInfo->icsInfo, pSamplingRateInfo);

  FDKmemclear(pAacDecoderChannelInfo->pDynData->aSfbScale, (8*16)*sizeof(SHORT));

  for (window=0, group=0; group < GetWindowGroups(&pAacDecoderChannelInfo->icsInfo); group++)
  {
    for (groupwin=0; groupwin < GetWindowGroupLength(&pAacDecoderChannelInfo->icsInfo,group); groupwin++, window++)
    {
      /* inverse quantization */
      for (band=0; band < ScaleFactorBandsTransmitted; band++)
      {
        FIXP_DBL *pSpectralCoefficient = SPEC(pAacDecoderChannelInfo->pSpectralCoefficient, window, pAacDecoderChannelInfo->granuleLength) + BandOffsets[band];

        int noLines = BandOffsets[band+1] - BandOffsets[band];
        int bnds = group*16+band;
        int i;

        if ((pCodeBook[bnds] == ZERO_HCB)
         || (pCodeBook[bnds] == INTENSITY_HCB)
         || (pCodeBook[bnds] == INTENSITY_HCB2)
           )
          continue;

        if (pCodeBook[bnds] == NOISE_HCB)
        {
          /* Leave headroom for PNS values. + 1 because ceil(log2(2^(0.25*3))) = 1,
             worst case of additional headroom required because of the scalefactor. */
          pSfbScale[window*16+band] = (pScaleFactor [bnds] >> 2) + 1 ;
          continue;
        }

        /* Find max spectral line value of the current sfb */
        FIXP_DBL locMax = (FIXP_DBL)0;

        for (i = noLines; i-- ; ) {
          /* Expensive memory access */
          locMax = fMax(fixp_abs(pSpectralCoefficient[i]), locMax);
        }

        /* Cheap robustness improvement - Do not remove!!! */
        if (fixp_abs(locMax) > (FIXP_DBL)MAX_QUANTIZED_VALUE) {
          return AAC_DEC_DECODE_FRAME_ERROR;
        }

        /*
           The inverse quantized spectral lines are defined by:
        pSpectralCoefficient[i] = Sign(pSpectralCoefficient[i]) * 2^(0.25*pScaleFactor[bnds]) * pSpectralCoefficient[i]^(4/3)
           This is equivalent to:
        pSpectralCoefficient[i]    = Sign(pSpectralCoefficient[i]) * (2^(pScaleFactor[bnds] % 4) * pSpectralCoefficient[i]^(4/3))
        pSpectralCoefficient_e[i] += pScaleFactor[bnds]/4
        */
        {
          int msb = pScaleFactor [bnds] >> 2 ;
          int lsb = pScaleFactor [bnds] & 0x03 ;

          int scale = GetScaleFromValue(locMax, lsb);

          pSfbScale[window*16+band] = msb - scale;
          InverseQuantizeBand(pSpectralCoefficient, noLines, lsb, scale);
        }
      }
    }
  }


  return AAC_DEC_OK;
}


AAC_DECODER_ERROR  CBlock_ReadSpectralData(HANDLE_FDK_BITSTREAM bs,
                                           CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                                           const SamplingRateInfo *pSamplingRateInfo,
                                           const UINT  flags)
{
  int i,index;
  int window,group,groupwin,groupoffset,band;
  UCHAR *RESTRICT pCodeBook = pAacDecoderChannelInfo->pDynData->aCodeBook;
  const SHORT *RESTRICT BandOffsets = GetScaleFactorBandOffsets(&pAacDecoderChannelInfo->icsInfo, pSamplingRateInfo);

  SPECTRAL_PTR pSpectralCoefficient = pAacDecoderChannelInfo->pSpectralCoefficient;
  FIXP_DBL locMax;

  int ScaleFactorBandsTransmitted = GetScaleFactorBandsTransmitted(&pAacDecoderChannelInfo->icsInfo);

  FDK_ASSERT(BandOffsets != NULL);

  FDKmemclear(pSpectralCoefficient, sizeof(SPECTRUM));

  if ( (flags & AC_ER_HCR) == 0 )
  {
    groupoffset = 0;

    /* plain huffman decoder  short */
    for (group=0; group < GetWindowGroups(&pAacDecoderChannelInfo->icsInfo); group++)
    {
      for (band=0; band < ScaleFactorBandsTransmitted; band++)
      {
        int bnds = group*16+band;
        UCHAR currentCB = pCodeBook[bnds];

        /* patch to run plain-huffman-decoder with vcb11 input codebooks (LAV-checking might be possible below using the virtual cb and a LAV-table) */
        if ((currentCB >= 16) && (currentCB <= 31)) {
          pCodeBook[bnds] = currentCB = 11;
        }
        if ( !((currentCB == ZERO_HCB)
            || (currentCB == NOISE_HCB)
            || (currentCB == INTENSITY_HCB)
            || (currentCB == INTENSITY_HCB2)) )
        {
          const CodeBookDescription *hcb = &AACcodeBookDescriptionTable[currentCB];
          int step = hcb->Dimension;
          int offset = hcb->Offset;
          int bits = hcb->numBits;
          int mask = (1<<bits)-1;

          for (groupwin=0; groupwin < GetWindowGroupLength(&pAacDecoderChannelInfo->icsInfo,group); groupwin++)
          {
            window = groupoffset + groupwin;

            FIXP_DBL *mdctSpectrum = SPEC(pSpectralCoefficient, window, pAacDecoderChannelInfo->granuleLength);

            locMax = (FIXP_DBL)0 ;

            for (index=BandOffsets[band]; index < BandOffsets[band+1]; index+=step)
            {
              int idx = CBlock_DecodeHuffmanWord(bs,hcb);

              for (i=0; i<step; i++) {
                FIXP_DBL tmp;

                tmp = (FIXP_DBL)((idx & mask)-offset);
                idx >>= bits;

                if (offset == 0) {
                  if (tmp != FIXP_DBL(0))
                    tmp = (FDKreadBits(bs,1))? -tmp : tmp;
                }
                mdctSpectrum[index+i] = tmp;
              }

              if (currentCB == ESCBOOK)
              {
                mdctSpectrum[index+0] = (FIXP_DBL)CBlock_GetEscape(bs, (LONG)mdctSpectrum[index+0]);
                mdctSpectrum[index+1] = (FIXP_DBL)CBlock_GetEscape(bs, (LONG)mdctSpectrum[index+1]);

              }
            }
          }
        }
      }
      groupoffset += GetWindowGroupLength(&pAacDecoderChannelInfo->icsInfo,group);
    }
    /* plain huffman decoding (short) finished */
  }
  /* HCR - Huffman Codeword Reordering  short */
  else  /* if ( flags & AC_ER_HCR ) */
  {
    H_HCR_INFO hHcr = &pAacDecoderChannelInfo->pComData->overlay.aac.erHcrInfo;
    int hcrStatus = 0;

    /* advanced Huffman decoding starts here (HCR decoding :) */
    if ( pAacDecoderChannelInfo->pDynData->specificTo.aac.lenOfReorderedSpectralData != 0 ) {

      /* HCR initialization short */
      hcrStatus = HcrInit(hHcr, pAacDecoderChannelInfo, pSamplingRateInfo, bs);

      if (hcrStatus != 0) {
        return AAC_DEC_DECODE_FRAME_ERROR;
      }

      /* HCR decoding short */
      hcrStatus = HcrDecoder(hHcr, pAacDecoderChannelInfo, pSamplingRateInfo, bs);

      if (hcrStatus != 0) {
#if HCR_ERROR_CONCEALMENT
        HcrMuteErroneousLines(hHcr);
#else
        return AAC_DEC_DECODE_FRAME_ERROR;
#endif /* HCR_ERROR_CONCEALMENT */
      }

      FDKpushFor (bs, pAacDecoderChannelInfo->pDynData->specificTo.aac.lenOfReorderedSpectralData);
    }
  }
  /* HCR - Huffman Codeword Reordering short finished */



  if ( IsLongBlock(&pAacDecoderChannelInfo->icsInfo) && !(flags & (AC_ELD|AC_SCALABLE)) )
  {
    /* apply pulse data */
    CPulseData_Apply(&pAacDecoderChannelInfo->pDynData->specificTo.aac.PulseData,
                      GetScaleFactorBandOffsets(&pAacDecoderChannelInfo->icsInfo, pSamplingRateInfo),
                      SPEC_LONG(pSpectralCoefficient));
  }


  return AAC_DEC_OK;
}



void ApplyTools ( CAacDecoderChannelInfo *pAacDecoderChannelInfo[],
                  const SamplingRateInfo *pSamplingRateInfo,
                  const UINT flags,
                  const int channel )
{

  if ( !(flags & (AC_USAC|AC_RSVD50|AC_MPS_RES)) ) {
    CPns_Apply(
           &pAacDecoderChannelInfo[channel]->data.aac.PnsData,
           &pAacDecoderChannelInfo[channel]->icsInfo,
            pAacDecoderChannelInfo[channel]->pSpectralCoefficient,
            pAacDecoderChannelInfo[channel]->specScale,
            pAacDecoderChannelInfo[channel]->pDynData->aScaleFactor,
            pSamplingRateInfo,
            pAacDecoderChannelInfo[channel]->granuleLength,
            channel
            );
  }

  CTns_Apply (
         &pAacDecoderChannelInfo[channel]->pDynData->TnsData,
         &pAacDecoderChannelInfo[channel]->icsInfo,
          pAacDecoderChannelInfo[channel]->pSpectralCoefficient,
          pSamplingRateInfo,
          pAacDecoderChannelInfo[channel]->granuleLength
          );
}

static
int getWindow2Nr(int length, int shape)
{
  int nr = 0;

  if (shape == 2) {
    /* Low Overlap, 3/4 zeroed */
    nr = (length * 3)>>2;
  }

  return nr;
}

void CBlock_FrequencyToTime(CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
                            CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                            INT_PCM outSamples[],
                            const SHORT frameLen,
                            const int stride,
                            const int frameOk,
                            FIXP_DBL *pWorkBuffer1 )
{
  int fr, fl, tl, nSamples, nSpec;

  /* Determine left slope length (fl), right slope length (fr) and transform length (tl).
     USAC: The slope length may mismatch with the previous frame in case of LPD / FD
           transitions. The adjustment is handled by the imdct implementation.
  */
  tl = frameLen;
  nSpec = 1;

  switch( pAacDecoderChannelInfo->icsInfo.WindowSequence ) {
    default:
    case OnlyLongSequence:
      fl = frameLen;
      fr = frameLen - getWindow2Nr(frameLen, GetWindowShape(&pAacDecoderChannelInfo->icsInfo));
      break;
    case LongStopSequence:
      fl = frameLen >> 3;
      fr = frameLen;
      break;
    case LongStartSequence: /* or StopStartSequence */
      fl = frameLen;
      fr = frameLen >> 3;
      break;
    case EightShortSequence:
      fl = fr = frameLen >> 3;
      tl >>= 3;
      nSpec = 8;
      break;
  }

  {
    int i;

    {
      FIXP_DBL *tmp = pAacDecoderChannelInfo->pComData->workBufferCore1->mdctOutTemp;

      nSamples = imdct_block(
             &pAacDecoderStaticChannelInfo->IMdct,
              tmp,
              SPEC_LONG(pAacDecoderChannelInfo->pSpectralCoefficient),
              pAacDecoderChannelInfo->specScale,
              nSpec,
              frameLen,
              tl,
              FDKgetWindowSlope(fl, GetWindowShape(&pAacDecoderChannelInfo->icsInfo)),
              fl,
              FDKgetWindowSlope(fr, GetWindowShape(&pAacDecoderChannelInfo->icsInfo)),
              fr,
              (FIXP_DBL)0 );

      for (i=0; i<frameLen; i++) {
        outSamples[i*stride] = IMDCT_SCALE(tmp[i]);
      }
    }
  }

  FDK_ASSERT(nSamples == frameLen);

}

#include "ldfiltbank.h"
void CBlock_FrequencyToTimeLowDelay( CAacDecoderStaticChannelInfo *pAacDecoderStaticChannelInfo,
                                     CAacDecoderChannelInfo *pAacDecoderChannelInfo,
                                     INT_PCM outSamples[],
                                     const short frameLen,
                                     const char stride )
{
  InvMdctTransformLowDelay_fdk (
          SPEC_LONG(pAacDecoderChannelInfo->pSpectralCoefficient),
          pAacDecoderChannelInfo->specScale[0],
          outSamples,
          pAacDecoderStaticChannelInfo->pOverlapBuffer,
          stride,
          frameLen
          );
}
