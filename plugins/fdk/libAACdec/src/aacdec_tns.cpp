
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
   Description: temporal noise shaping tool

******************************************************************************/

#include "aacdec_tns.h"
#include "aac_rom.h"
#include "FDK_bitstream.h"
#include "channelinfo.h"



/*!
  \brief Reset tns data

  The function resets the tns data

  \return  none
*/
void CTns_Reset(CTnsData *pTnsData)
{
  /* Note: the following FDKmemclear should not be required. */
  FDKmemclear(pTnsData->Filter, TNS_MAX_WINDOWS*TNS_MAXIMUM_FILTERS*sizeof(CFilter));
  FDKmemclear(pTnsData->NumberOfFilters, TNS_MAX_WINDOWS*sizeof(UCHAR));
  pTnsData->DataPresent = 0;
  pTnsData->Active = 0;
}

void CTns_ReadDataPresentFlag(HANDLE_FDK_BITSTREAM bs,    /*!< pointer to bitstream */
                              CTnsData *pTnsData)         /*!< pointer to aac decoder channel info */
{
  pTnsData->DataPresent = (UCHAR) FDKreadBits(bs,1);
}

/*!
  \brief Read tns data from bitstream

  The function reads the elements for tns from
  the bitstream.

  \return  none
*/
AAC_DECODER_ERROR CTns_Read(HANDLE_FDK_BITSTREAM bs,
                            CTnsData *pTnsData,
                            const CIcsInfo *pIcsInfo,
                            const UINT flags)
{
  UCHAR n_filt,order;
  UCHAR length,coef_res,coef_compress;
  UCHAR window;
  UCHAR wins_per_frame = GetWindowsPerFrame(pIcsInfo);
  UCHAR isLongFlag = IsLongBlock(pIcsInfo);
  AAC_DECODER_ERROR ErrorStatus = AAC_DEC_OK;

  if (!pTnsData->DataPresent) {
    return ErrorStatus;
  }

  for (window = 0; window < wins_per_frame; window++)
  {
    pTnsData->NumberOfFilters[window] = n_filt = (UCHAR) FDKreadBits(bs, isLongFlag ? 2 : 1);

    if (pTnsData->NumberOfFilters[window] > TNS_MAXIMUM_FILTERS){
        pTnsData->NumberOfFilters[window] = n_filt = TNS_MAXIMUM_FILTERS;
    }

    if (n_filt)
    {
      int index;
      UCHAR nextstopband;

      coef_res = (UCHAR) FDKreadBits(bs,1);

      nextstopband = GetScaleFactorBandsTotal(pIcsInfo);

      for (index=0; index < n_filt; index++)
      {
        CFilter *filter = &pTnsData->Filter[window][index];

        length = (UCHAR)FDKreadBits(bs, isLongFlag ? 6 : 4);

        if (length > nextstopband){
          length = nextstopband;
        }

        filter->StartBand = nextstopband - length;
        filter->StopBand  = nextstopband;
        nextstopband = filter->StartBand;

        {
          filter->Order = order = (UCHAR) FDKreadBits(bs, isLongFlag ? 5 : 3);
        }

        if (filter->Order > TNS_MAXIMUM_ORDER){
          filter->Order = order = TNS_MAXIMUM_ORDER;
        }

        if (order)
        {
          UCHAR coef,s_mask;
          UCHAR i;
          SCHAR n_mask;
          static const UCHAR sgn_mask[] = {  0x2,  0x4,  0x8 };
          static const SCHAR neg_mask[] = { ~0x3, ~0x7, ~0xF };

          filter->Direction = FDKreadBits(bs,1) ? -1 : 1;

          coef_compress = (UCHAR) FDKreadBits(bs,1);

          filter->Resolution = coef_res + 3;

          s_mask = sgn_mask[coef_res + 1 - coef_compress];
          n_mask = neg_mask[coef_res + 1 - coef_compress];

          for (i=0; i < order; i++)
          {
            coef = (UCHAR) FDKreadBits(bs,filter->Resolution - coef_compress);
            filter->Coeff[i] = (coef & s_mask) ? (coef | n_mask) : coef;
          }
        }
      }
    }
  }

  pTnsData->Active = 1;

  return ErrorStatus;
}


static void CTns_Filter (FIXP_DBL *spec, int size, int inc, FIXP_TCC coeff [], int order)
{
  // - Simple all-pole filter of order "order" defined by
  //   y(n) =  x(n) - a(2)*y(n-1) - ... - a(order+1)*y(n-order)
  //
  // - The state variables of the filter are initialized to zero every time
  //
  // - The output data is written over the input data ("in-place operation")
  //
  // - An input vector of "size" samples is processed and the index increment
  //   to the next data sample is given by "inc"

  int i,j,N;
  FIXP_DBL *pSpec;
  FIXP_DBL maxVal=FL2FXCONST_DBL(0.0);
  INT s;

  FDK_ASSERT(order <= TNS_MAXIMUM_ORDER);
  C_ALLOC_SCRATCH_START(state, FIXP_DBL, TNS_MAXIMUM_ORDER);
  FDKmemclear(state, order*sizeof(FIXP_DBL));

  for (i=0; i<size; i++) {
    maxVal = fixMax(maxVal,fixp_abs(spec[i]));
  }

  if ( maxVal > FL2FXCONST_DBL(0.03125*0.70710678118) )
    s = fixMax(CntLeadingZeros(maxVal)-6,0);
  else
    s = fixMax(CntLeadingZeros(maxVal)-5,0);

  s = fixMin(s,2);
  s = s-1;

  if (inc == -1)
    pSpec = &spec[size - 1];
  else
    pSpec = &spec[0];

  FIXP_TCC *pCoeff;

#define FIRST_PART_FLTR                                              \
    FIXP_DBL x, *pState = state;                                     \
    pCoeff = coeff;                                                  \
                                                                     \
    if (s < 0)                                                       \
      x = (pSpec [0]>>1) + fMultDiv2 (*pCoeff++, pState [0]) ;       \
    else                                                             \
      x = (pSpec [0]<<s) + fMultDiv2 (*pCoeff++, pState [0]) ;

#define INNER_FLTR_INLINE                                            \
      x = fMultAddDiv2 (x, *pCoeff, pState [1]);                     \
      pState [0] = pState [1] - (fMultDiv2 (*pCoeff++, x) <<2) ;     \
      pState++;

#define LAST_PART_FLTR                                               \
      if (s < 0)                                                     \
        *pSpec = x << 1;                                             \
      else                                                           \
        *pSpec = x >> s;                                             \
      *pState =(-x) << 1;                                            \
      pSpec   += inc ;


   if (order>8)
   {
      N = (order-1)&7;

      for (i = size ; i != 0 ; i--)
      {
        FIRST_PART_FLTR

        for (j = N; j > 0 ; j--) { INNER_FLTR_INLINE }

        INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE
        INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE

        LAST_PART_FLTR
      }

   } else if (order>4) {

      N = (order-1)&3;

      for (i = size ; i != 0 ; i--)
      {
        FIRST_PART_FLTR
        for (j = N; j > 0 ; j--) { INNER_FLTR_INLINE }

        INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE INNER_FLTR_INLINE

        LAST_PART_FLTR
      }

   } else {

      N = order-1;

      for (i = size ; i != 0 ; i--)
      {
        FIRST_PART_FLTR

        for (j = N; j > 0 ; j--) { INNER_FLTR_INLINE }

        LAST_PART_FLTR
      }
   }

   C_ALLOC_SCRATCH_END(state, FIXP_DBL, TNS_MAXIMUM_ORDER);
}

/*!
  \brief Apply tns to spectral lines

  The function applies the tns to the spectrum,

  \return  none
*/
void CTns_Apply (
        CTnsData *RESTRICT pTnsData, /*!< pointer to aac decoder info */
        const CIcsInfo *pIcsInfo,
        SPECTRAL_PTR pSpectralCoefficient,
        const SamplingRateInfo *pSamplingRateInfo,
        const INT granuleLength
        )
{
  int window,index,start,stop,size;


  if (pTnsData->Active)
  {
      C_AALLOC_SCRATCH_START(coeff, FIXP_TCC, TNS_MAXIMUM_ORDER);

      for (window=0; window < GetWindowsPerFrame(pIcsInfo); window++)
      {
        FIXP_DBL *pSpectrum = SPEC(pSpectralCoefficient, window, granuleLength);

        for (index=0; index < pTnsData->NumberOfFilters[window]; index++)
        {
          CFilter *RESTRICT filter = &pTnsData->Filter[window][index];

          if (filter->Order > 0)
          {
             FIXP_TCC *pCoeff;
             int tns_max_bands;

             pCoeff = &coeff[filter->Order-1];
             if (filter->Resolution == 3)
             {
               int i;
               for (i=0; i < filter->Order; i++)
                 *pCoeff-- = FDKaacDec_tnsCoeff3[filter->Coeff[i]+4];
             }
             else
             {
               int i;
               for (i=0; i < filter->Order; i++)
                 *pCoeff-- = FDKaacDec_tnsCoeff4[filter->Coeff[i]+8];
             }

             switch (granuleLength) {
               case 480:
                 tns_max_bands = tns_max_bands_tbl_480[pSamplingRateInfo->samplingRateIndex];
                 break;
               case 512:
                 tns_max_bands = tns_max_bands_tbl_512[pSamplingRateInfo->samplingRateIndex];
                 break;
               default:
                 tns_max_bands = GetMaximumTnsBands(pIcsInfo, pSamplingRateInfo->samplingRateIndex);
                 break;
             }

             start = fixMin( fixMin(filter->StartBand, tns_max_bands),
                             GetScaleFactorBandsTransmitted(pIcsInfo) );

             start = GetScaleFactorBandOffsets(pIcsInfo, pSamplingRateInfo)[start];

             stop = fixMin( fixMin(filter->StopBand, tns_max_bands),
                            GetScaleFactorBandsTransmitted(pIcsInfo) );

             stop = GetScaleFactorBandOffsets(pIcsInfo, pSamplingRateInfo)[stop];

             size = stop - start;

             if (size > 0) {
               CTns_Filter(&pSpectrum[start],
                            size,
                            filter->Direction,
                            coeff,
                            filter->Order );
             }
          }
        }
      }
      C_AALLOC_SCRATCH_END(coeff, FIXP_TCC, TNS_MAXIMUM_ORDER);
  }

}
