
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

/*!
  \file
  \brief  Envelope extraction  
  The functions provided by this module are mostly called by applySBR(). After it is
  determined that there is valid SBR data, sbrGetHeaderData() might be called if the current
  SBR data contains an \ref SBR_HEADER_ELEMENT as opposed to a \ref SBR_STANDARD_ELEMENT. This function
  may return various error codes as defined in #SBR_HEADER_STATUS . Most importantly it returns HEADER_RESET when decoder
  settings need to be recalculated according to the SBR specifications. In that case applySBR()
  will initiatite the required re-configuration.

  The header data is stored in a #SBR_HEADER_DATA structure.

  The actual SBR data for the current frame is decoded into SBR_FRAME_DATA stuctures by sbrGetChannelPairElement()
  [for stereo streams] and sbrGetSingleChannelElement() [for mono streams]. There is no fractional arithmetic involved.

  Once the information is extracted, the data needs to be further prepared before the actual decoding process.
  This is done in decodeSbrData().

  \sa Description of buffer management in applySBR(). \ref documentationOverview

  <h1>About the SBR data format:</h1>

  Each frame includes SBR data (side chain information), and can be either the \ref SBR_HEADER_ELEMENT or the \ref SBR_STANDARD_ELEMENT.
  Parts of the data can be protected by a CRC checksum.

  \anchor SBR_HEADER_ELEMENT <h2>The SBR_HEADER_ELEMENT</h2>

  The SBR_HEADER_ELEMENT can be transmitted with every frame, however, it typically is send every second or so. It contains fundamental
  information such as SBR sampling frequency and frequency range as well as control signals that do not require frequent changes. It also
  includes the \ref SBR_STANDARD_ELEMENT.

  Depending on the changes between the information in a current SBR_HEADER_ELEMENT and the previous SBR_HEADER_ELEMENT, the SBR decoder might need
  to be reset and reconfigured (e.g. new tables need to be calculated).

  \anchor SBR_STANDARD_ELEMENT <h2>The SBR_STANDARD_ELEMENT</h2>

  This data can be subdivided into "side info" and "raw data", where side info is defined as signals needed to decode the raw data
  and some decoder tuning signals. Raw data is referred to as PCM and Huffman coded envelope and noise floor estimates. The side info also
  includes information about the time-frequency grid for the current frame.

  \sa \ref documentationOverview
*/

#include "env_extr.h"

#include "sbr_ram.h"
#include "sbr_rom.h"
#include "huff_dec.h"


#include "psbitdec.h"

#define DRM_PARAMETRIC_STEREO   0
#define EXTENSION_ID_PS_CODING  2


static int extractFrameInfo (HANDLE_FDK_BITSTREAM   hBs,
                             HANDLE_SBR_HEADER_DATA hHeaderData,
                             HANDLE_SBR_FRAME_DATA  h_frame_data,
                             const UINT             nrOfChannels,
                             const UINT             flags
                            );


static int sbrGetEnvelope (HANDLE_SBR_HEADER_DATA hHeaderData,
                           HANDLE_SBR_FRAME_DATA  h_frame_data,
                           HANDLE_FDK_BITSTREAM   hBs,
                           const UINT flags);

static void sbrGetDirectionControlData (HANDLE_SBR_FRAME_DATA hFrameData,
                                        HANDLE_FDK_BITSTREAM hBs);

static void sbrGetNoiseFloorData (HANDLE_SBR_HEADER_DATA hHeaderData,
                                  HANDLE_SBR_FRAME_DATA  h_frame_data,
                                  HANDLE_FDK_BITSTREAM   hBs);

static int checkFrameInfo (FRAME_INFO *pFrameInfo, int numberOfTimeSlots, int overlap, int timeStep);

SBR_ERROR
initHeaderData (
        HANDLE_SBR_HEADER_DATA  hHeaderData,
        const int               sampleRateIn,
        const int               sampleRateOut,
        const int               samplesPerFrame,
        const UINT              flags
        )
{
  HANDLE_FREQ_BAND_DATA hFreq = &hHeaderData->freqBandData;
  SBR_ERROR sbrError = SBRDEC_OK;
  int numAnalysisBands;

  if ( sampleRateIn == sampleRateOut ) {
    hHeaderData->sbrProcSmplRate = sampleRateOut<<1;
    numAnalysisBands = 32;
  } else {
    hHeaderData->sbrProcSmplRate = sampleRateOut;
    if ( (sampleRateOut>>1) == sampleRateIn) {
      /* 1:2 */
      numAnalysisBands = 32;
    } else if ( (sampleRateOut>>2) == sampleRateIn ) {
      /* 1:4 */
      numAnalysisBands = 32;
    } else if ( (sampleRateOut*3)>>3 == (sampleRateIn*8)>>3 ) {
      /* 3:8, 3/4 core frame length */
      numAnalysisBands = 24;
    } else {
      sbrError = SBRDEC_UNSUPPORTED_CONFIG;
      goto bail;
    }
  }

  /* Fill in default values first */
  hHeaderData->syncState          = SBR_NOT_INITIALIZED;
  hHeaderData->status             = 0;
  hHeaderData->frameErrorFlag     = 0;

  hHeaderData->bs_info.ampResolution     = 1;
  hHeaderData->bs_info.xover_band        = 0;
  hHeaderData->bs_info.sbr_preprocessing = 0;

  hHeaderData->bs_data.startFreq       = 5;
  hHeaderData->bs_data.stopFreq        = 0;
  hHeaderData->bs_data.freqScale       = 2;
  hHeaderData->bs_data.alterScale      = 1;
  hHeaderData->bs_data.noise_bands     = 2;
  hHeaderData->bs_data.limiterBands    = 2;
  hHeaderData->bs_data.limiterGains    = 2;
  hHeaderData->bs_data.interpolFreq    = 1;
  hHeaderData->bs_data.smoothingLength = 1;

  hHeaderData->timeStep = (flags & SBRDEC_ELD_GRID) ? 1 : 2;

  /* Setup pointers to frequency band tables */
  hFreq->freqBandTable[0]  = hFreq->freqBandTableLo;
  hFreq->freqBandTable[1] = hFreq->freqBandTableHi;

  /* Patch some entries */
  if (sampleRateOut > 24000) {    /* Trigger an error if SBR is going to be processed without     */
    hHeaderData->bs_data.startFreq = 7;   /*   having read these frequency values from bit stream before. */
    hHeaderData->bs_data.stopFreq  = 3;
  }

  /* One SBR timeslot corresponds to the amount of samples equal to the amount of analysis bands, divided by the timestep. */
  hHeaderData->numberTimeSlots = (samplesPerFrame/numAnalysisBands) >> (hHeaderData->timeStep - 1);
  if (hHeaderData->numberTimeSlots > (16)) {
    sbrError = SBRDEC_UNSUPPORTED_CONFIG;
  }

  hHeaderData->numberOfAnalysisBands = numAnalysisBands;

bail:
  return sbrError;
}


/*!
  \brief   Initialize the SBR_PREV_FRAME_DATA struct
*/
void
initSbrPrevFrameData (HANDLE_SBR_PREV_FRAME_DATA h_prev_data, /*!< handle to struct SBR_PREV_FRAME_DATA */
                      int timeSlots)                          /*!< Framelength in SBR-timeslots */
{
  int i;

  /* Set previous energy and noise levels to 0 for the case
     that decoding starts in the middle of a bitstream */
  for (i=0; i < MAX_FREQ_COEFFS; i++)
    h_prev_data->sfb_nrg_prev[i] = (FIXP_DBL)0;
  for (i=0; i < MAX_NOISE_COEFFS; i++)
    h_prev_data->prevNoiseLevel[i] = (FIXP_DBL)0;
  for (i=0; i < MAX_INVF_BANDS; i++)
    h_prev_data->sbr_invf_mode[i] = INVF_OFF;

  h_prev_data->stopPos = timeSlots;
  h_prev_data->coupling = COUPLING_OFF;
  h_prev_data->ampRes = 0;
}


/*!
  \brief   Read header data from bitstream

  \return  error status - 0 if ok
*/
SBR_HEADER_STATUS
sbrGetHeaderData (HANDLE_SBR_HEADER_DATA hHeaderData,
                  HANDLE_FDK_BITSTREAM   hBs,
                  const UINT             flags,
                  const int              fIsSbrData)
{
  SBR_HEADER_DATA_BS *pBsData;
  SBR_HEADER_DATA_BS lastHeader;
  SBR_HEADER_DATA_BS_INFO lastInfo;
  int headerExtra1=0, headerExtra2=0;

  /* Copy SBR bit stream header to temporary header */
  lastHeader = hHeaderData->bs_data;
  lastInfo   = hHeaderData->bs_info;

  /* Read new header from bitstream */
  {
    pBsData = &hHeaderData->bs_data;
  }

  {
    hHeaderData->bs_info.ampResolution = FDKreadBits (hBs, 1);
  }

  pBsData->startFreq = FDKreadBits (hBs, 4);
  pBsData->stopFreq = FDKreadBits (hBs, 4);

  {
    hHeaderData->bs_info.xover_band = FDKreadBits (hBs, 3);
    FDKreadBits (hBs, 2);
  }

  headerExtra1 = FDKreadBits (hBs, 1);
  headerExtra2 = FDKreadBits (hBs, 1);

  /* Handle extra header information */
  if( headerExtra1) 
  {
    pBsData->freqScale = FDKreadBits (hBs, 2);
    pBsData->alterScale = FDKreadBits (hBs, 1);
    pBsData->noise_bands = FDKreadBits (hBs, 2);
  }
  else {
    pBsData->freqScale   = 2;
    pBsData->alterScale  = 1;
    pBsData->noise_bands = 2;
  }

  if (headerExtra2) {
    pBsData->limiterBands = FDKreadBits (hBs, 2);
    pBsData->limiterGains = FDKreadBits (hBs, 2);
    pBsData->interpolFreq = FDKreadBits (hBs, 1);
    pBsData->smoothingLength = FDKreadBits (hBs, 1);
  }
  else {
    pBsData->limiterBands    = 2;
    pBsData->limiterGains    = 2;
    pBsData->interpolFreq    = 1;
    pBsData->smoothingLength = 1;
  }

  /* Look for new settings. IEC 14496-3, 4.6.18.3.1 */
  if(hHeaderData->syncState < SBR_HEADER ||
     lastHeader.startFreq   != pBsData->startFreq   ||
     lastHeader.stopFreq    != pBsData->stopFreq    ||
     lastHeader.freqScale   != pBsData->freqScale   ||
     lastHeader.alterScale  != pBsData->alterScale  ||
     lastHeader.noise_bands != pBsData->noise_bands ||
     lastInfo.xover_band    != hHeaderData->bs_info.xover_band) {
    return HEADER_RESET; /* New settings */
  }

  return HEADER_OK;
}

/*!
  \brief   Get missing harmonics parameters (only used for AAC+SBR)

  \return  error status - 0 if ok
*/
int
sbrGetSyntheticCodedData(HANDLE_SBR_HEADER_DATA hHeaderData,
                         HANDLE_SBR_FRAME_DATA  hFrameData,
                         HANDLE_FDK_BITSTREAM   hBs)
{
  int i, bitsRead = 0;

  int flag = FDKreadBits(hBs,1);
  bitsRead++;

  if(flag){
    for(i=0;i<hHeaderData->freqBandData.nSfb[1];i++){
      hFrameData->addHarmonics[i]  = FDKreadBits (hBs, 1 );
      bitsRead++;
    }
  }
  else {
    for(i=0; i<MAX_FREQ_COEFFS; i++)
      hFrameData->addHarmonics[i]  = 0;
  }
  return(bitsRead);
}

/*!
  \brief      Reads extension data from the bitstream

  The bitstream format allows up to 4 kinds of extended data element.
  Extended data may contain several elements, each identified by a 2-bit-ID.
  So far, no extended data elements are defined hence the first 2 parameters
  are unused. The data should be skipped in order to update the number
  of read bits for the consistency check in applySBR().
*/
static int  extractExtendedData(
                                HANDLE_SBR_HEADER_DATA hHeaderData,    /*!< handle to SBR header */
                                HANDLE_FDK_BITSTREAM   hBs             /*!< Handle to the bit buffer */
                               ,HANDLE_PS_DEC hParametricStereoDec     /*!< Parametric Stereo Decoder */
                                ) {
  INT nBitsLeft;
  int extended_data;
  int i, frameOk = 1;


  extended_data = FDKreadBits(hBs, 1);

  if (extended_data) {
    int cnt;
    int bPsRead = 0;

    cnt = FDKreadBits(hBs, 4);
    if (cnt == (1<<4)-1)
      cnt += FDKreadBits(hBs, 8);


    nBitsLeft = 8 * cnt;

    /* sanity check for cnt */
    if (nBitsLeft > (INT)FDKgetValidBits(hBs)) {
      /* limit nBitsLeft */
      nBitsLeft = (INT)FDKgetValidBits(hBs);
      /* set frame error */
      frameOk = 0;
    }

    while (nBitsLeft > 7) {
      int extension_id = FDKreadBits(hBs, 2);
      nBitsLeft -= 2;

      switch(extension_id) {



        case EXTENSION_ID_PS_CODING:

        /* Read PS data from bitstream */

        if (hParametricStereoDec != NULL) {
          if(bPsRead && !hParametricStereoDec->bsData[hParametricStereoDec->bsReadSlot].mpeg.bPsHeaderValid) {
            cnt = nBitsLeft >> 3; /* number of remaining bytes */
            for (i=0; i<cnt; i++)
              FDKreadBits(hBs, 8);
            nBitsLeft -= cnt * 8;
          } else {
            nBitsLeft -= ReadPsData(hParametricStereoDec, hBs, nBitsLeft);
            bPsRead = 1;
          }
        }

          /* parametric stereo detected, could set channelMode accordingly here  */
          /*                                                                     */
          /* "The usage of this parametric stereo extension to HE-AAC is         */
          /* signalled implicitly in the bitstream. Hence, if an sbr_extension() */
          /* with bs_extension_id==EXTENSION_ID_PS is found in the SBR part of   */
          /* the bitstream, a decoder supporting the combination of SBR and PS   */
          /* shall operate the PS tool to generate a stereo output signal."      */
          /* source: ISO/IEC 14496-3:2001/FDAM 2:2004(E)                         */

        break;


      default:
        cnt = nBitsLeft >> 3; /* number of remaining bytes */
        for (i=0; i<cnt; i++)
          FDKreadBits(hBs, 8);
        nBitsLeft -= cnt * 8;
        break;
      }
    }

    if (nBitsLeft < 0) {
      frameOk = 0;
      goto bail;
    }
    else {
      /* Read fill bits for byte alignment */
      FDKreadBits(hBs, nBitsLeft);
    }
  }

bail:
  return (frameOk);
}


/*!
  \brief   Read bitstream elements of one channel

  \return  SbrFrameOK:  1=ok, 0=error
*/
int
sbrGetSingleChannelElement (HANDLE_SBR_HEADER_DATA hHeaderData,          /*!< Static control data */
                            HANDLE_SBR_FRAME_DATA  hFrameData,           /*!< Control data of current frame */
                            HANDLE_FDK_BITSTREAM   hBs,                  /*!< Handle to struct BIT_BUF */
                            HANDLE_PS_DEC          hParametricStereoDec, /*!< Handle to PS decoder */
                            const UINT             flags,
                            const int              overlap
                           )
{
  int i;


  hFrameData->coupling = COUPLING_OFF;

  {
    /* Reserved bits */
    if (FDKreadBits(hBs, 1)) {  /* bs_data_extra */
      FDKreadBits(hBs, 4);
      if (flags & SBRDEC_SYNTAX_SCAL) {
        FDKreadBits(hBs, 4);
      }
    }
  }

  if (flags & SBRDEC_SYNTAX_SCAL) {
    FDKreadBits (hBs, 1);     /* bs_coupling */
  }

  /*
    Grid control
  */
  if ( !extractFrameInfo ( hBs, hHeaderData, hFrameData, 1, flags) )
   return 0;

  if ( !checkFrameInfo (&hFrameData->frameInfo, hHeaderData->numberTimeSlots, overlap, hHeaderData->timeStep) )
    return 0;


  /*
    Fetch domain vectors (time or frequency direction for delta-coding)
  */
  sbrGetDirectionControlData (hFrameData, hBs);

  for (i=0; i<hHeaderData->freqBandData.nInvfBands; i++) {
    hFrameData->sbr_invf_mode[i] =
      (INVF_MODE) FDKreadBits (hBs, 2);
  }



  /* raw data */
  if ( !sbrGetEnvelope (hHeaderData, hFrameData, hBs, flags) )
    return 0;


  sbrGetNoiseFloorData (hHeaderData, hFrameData, hBs);

  sbrGetSyntheticCodedData(hHeaderData, hFrameData, hBs);

  {
    /* sbr extended data */
    if (! extractExtendedData(
                               hHeaderData,
                               hBs
                              ,hParametricStereoDec
                               )) {
      return 0;
    }
  }

  return 1;
}



/*!
  \brief      Read bitstream elements of a channel pair
  \return     SbrFrameOK
*/
int
sbrGetChannelPairElement (HANDLE_SBR_HEADER_DATA hHeaderData,    /*!< Static control data */
                          HANDLE_SBR_FRAME_DATA  hFrameDataLeft, /*!< Dynamic control data for first channel */
                          HANDLE_SBR_FRAME_DATA  hFrameDataRight,/*!< Dynamic control data for second channel */
                          HANDLE_FDK_BITSTREAM   hBs,            /*!< handle to struct BIT_BUF */
                          const UINT flags,
                          const int overlap )
{
  int i, bit;


  /* Reserved bits */
  if (FDKreadBits(hBs, 1)) {  /* bs_data_extra */
    FDKreadBits(hBs, 4);
    FDKreadBits(hBs, 4);
  }

  /* Read coupling flag */
  bit = FDKreadBits (hBs, 1);

  if (bit) {
    hFrameDataLeft->coupling = COUPLING_LEVEL;
    hFrameDataRight->coupling = COUPLING_BAL;
  }
  else {
    hFrameDataLeft->coupling = COUPLING_OFF;
    hFrameDataRight->coupling = COUPLING_OFF;
  }


  /*
    Grid control
  */
  if ( !extractFrameInfo (hBs, hHeaderData, hFrameDataLeft, 2, flags) )
    return 0;

  if ( !checkFrameInfo (&hFrameDataLeft->frameInfo, hHeaderData->numberTimeSlots, overlap, hHeaderData->timeStep) )
    return 0;

  if (hFrameDataLeft->coupling) {
    FDKmemcpy (&hFrameDataRight->frameInfo, &hFrameDataLeft->frameInfo, sizeof(FRAME_INFO));
    hFrameDataRight->ampResolutionCurrentFrame = hFrameDataLeft->ampResolutionCurrentFrame;
  }
  else {
    if ( !extractFrameInfo (hBs, hHeaderData, hFrameDataRight, 2, flags) )
      return 0;

    if ( !checkFrameInfo (&hFrameDataRight->frameInfo, hHeaderData->numberTimeSlots, overlap, hHeaderData->timeStep) )
      return 0;
  }

  /*
    Fetch domain vectors (time or frequency direction for delta-coding)
  */
  sbrGetDirectionControlData (hFrameDataLeft, hBs);
  sbrGetDirectionControlData (hFrameDataRight, hBs);

  for (i=0; i<hHeaderData->freqBandData.nInvfBands; i++) {
    hFrameDataLeft->sbr_invf_mode[i] = (INVF_MODE) FDKreadBits (hBs, 2);
  }

  if (hFrameDataLeft->coupling) {
    for (i=0; i<hHeaderData->freqBandData.nInvfBands; i++) {
      hFrameDataRight->sbr_invf_mode[i] = hFrameDataLeft->sbr_invf_mode[i];
    }


    if ( !sbrGetEnvelope (hHeaderData, hFrameDataLeft, hBs, flags) ) {
      return 0;
    }

    sbrGetNoiseFloorData (hHeaderData, hFrameDataLeft, hBs);

    if ( !sbrGetEnvelope (hHeaderData, hFrameDataRight, hBs, flags) ) {
      return 0;
    }
  }
  else {

    for (i=0; i<hHeaderData->freqBandData.nInvfBands; i++) {
      hFrameDataRight->sbr_invf_mode[i] = (INVF_MODE) FDKreadBits (hBs, 2);
    }



    if ( !sbrGetEnvelope (hHeaderData, hFrameDataLeft, hBs, flags) )
      return 0;

    if ( !sbrGetEnvelope (hHeaderData, hFrameDataRight, hBs, flags) )
      return 0;

    sbrGetNoiseFloorData (hHeaderData, hFrameDataLeft, hBs);

  }
  sbrGetNoiseFloorData (hHeaderData, hFrameDataRight, hBs);

  sbrGetSyntheticCodedData(hHeaderData, hFrameDataLeft, hBs);
  sbrGetSyntheticCodedData(hHeaderData, hFrameDataRight, hBs);

  {
    if (! extractExtendedData(
                               hHeaderData,
                               hBs
                              ,NULL
                             ) ) {
      return 0;
    }
  }

  return 1;
}




/*!
  \brief   Read direction control data from bitstream
*/
void
sbrGetDirectionControlData (HANDLE_SBR_FRAME_DATA h_frame_data, /*!< handle to struct SBR_FRAME_DATA */
                            HANDLE_FDK_BITSTREAM  hBs)          /*!< handle to struct BIT_BUF */
{
  int i;

  for (i = 0; i < h_frame_data->frameInfo.nEnvelopes; i++) {
    h_frame_data->domain_vec[i] = FDKreadBits (hBs, 1);
  }

  for (i = 0; i < h_frame_data->frameInfo.nNoiseEnvelopes; i++) {
    h_frame_data->domain_vec_noise[i] = FDKreadBits (hBs, 1);
  }
}



/*!
  \brief   Read noise-floor-level data from bitstream
*/
void
sbrGetNoiseFloorData (HANDLE_SBR_HEADER_DATA hHeaderData,  /*!< Static control data */
                      HANDLE_SBR_FRAME_DATA  h_frame_data, /*!< handle to struct SBR_FRAME_DATA */
                      HANDLE_FDK_BITSTREAM   hBs)          /*!< handle to struct BIT_BUF */
{
  int i,j;
  int delta;
  COUPLING_MODE coupling;
  int noNoiseBands = hHeaderData->freqBandData.nNfb;

  Huffman hcb_noiseF;
  Huffman hcb_noise;
  int envDataTableCompFactor;

  coupling = h_frame_data->coupling;


  /*
    Select huffman codebook depending on coupling mode
  */
  if (coupling == COUPLING_BAL) {
    hcb_noise = (Huffman)&FDK_sbrDecoder_sbr_huffBook_NoiseBalance11T;
    hcb_noiseF = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvBalance11F;  /* "sbr_huffBook_NoiseBalance11F" */
    envDataTableCompFactor = 1;
  }
  else {
    hcb_noise = (Huffman)&FDK_sbrDecoder_sbr_huffBook_NoiseLevel11T;
    hcb_noiseF = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvLevel11F;    /* "sbr_huffBook_NoiseLevel11F" */
    envDataTableCompFactor = 0;
  }

  /*
    Read raw noise-envelope data
  */
  for (i=0; i<h_frame_data->frameInfo.nNoiseEnvelopes; i++) {


    if (h_frame_data->domain_vec_noise[i] == 0) {
      if (coupling == COUPLING_BAL) {
        h_frame_data->sbrNoiseFloorLevel[i*noNoiseBands] =
          (FIXP_SGL) (((int)FDKreadBits (hBs, 5)) << envDataTableCompFactor);
      }
      else {
        h_frame_data->sbrNoiseFloorLevel[i*noNoiseBands] =
          (FIXP_SGL) (int)FDKreadBits (hBs, 5);
      }

      for (j = 1; j < noNoiseBands; j++) {
        delta = DecodeHuffmanCW(hcb_noiseF, hBs);
        h_frame_data->sbrNoiseFloorLevel[i*noNoiseBands+j] = (FIXP_SGL) (delta << envDataTableCompFactor);
      }
    }
    else {
      for (j = 0; j < noNoiseBands; j++) {
        delta = DecodeHuffmanCW(hcb_noise, hBs);
        h_frame_data->sbrNoiseFloorLevel[i*noNoiseBands+j] = (FIXP_SGL) (delta << envDataTableCompFactor);
      }
    }
  }
}


/*!
  \brief   Read envelope data from bitstream
*/
static int
sbrGetEnvelope (HANDLE_SBR_HEADER_DATA hHeaderData,  /*!< Static control data */
                HANDLE_SBR_FRAME_DATA  h_frame_data, /*!< handle to struct SBR_FRAME_DATA */
                HANDLE_FDK_BITSTREAM   hBs,          /*!< handle to struct BIT_BUF */
                const UINT             flags)
{
  int i, j;
  UCHAR no_band[MAX_ENVELOPES];
  int delta = 0;
  int offset = 0;
  COUPLING_MODE coupling = h_frame_data->coupling;
  int ampRes = hHeaderData->bs_info.ampResolution;
  int nEnvelopes = h_frame_data->frameInfo.nEnvelopes;
  int envDataTableCompFactor;
  int start_bits, start_bits_balance;
  Huffman hcb_t, hcb_f;

  h_frame_data->nScaleFactors = 0;

  if ( (h_frame_data->frameInfo.frameClass == 0) && (nEnvelopes == 1) ) {
    if (flags & SBRDEC_ELD_GRID)
      ampRes = h_frame_data->ampResolutionCurrentFrame;
    else
      ampRes = 0;
  }
  h_frame_data->ampResolutionCurrentFrame = ampRes;

  /*
    Set number of bits for first value depending on amplitude resolution
  */
  if(ampRes == 1)
  {
    start_bits = 6;
    start_bits_balance = 5;
  }
  else
  {
    start_bits = 7;
    start_bits_balance = 6;
  }

  /*
    Calculate number of values for each envelope and alltogether
  */
  for (i = 0; i < nEnvelopes; i++) {
    no_band[i] = hHeaderData->freqBandData.nSfb[h_frame_data->frameInfo.freqRes[i]];
    h_frame_data->nScaleFactors += no_band[i];
  }
  if (h_frame_data->nScaleFactors > MAX_NUM_ENVELOPE_VALUES)
    return 0;

  /*
    Select Huffman codebook depending on coupling mode and amplitude resolution
  */
  if (coupling == COUPLING_BAL) {
    envDataTableCompFactor = 1;
    if (ampRes == 0) {
      hcb_t = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvBalance10T;
      hcb_f = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvBalance10F;
    }
    else {
      hcb_t = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvBalance11T;
      hcb_f = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvBalance11F;
    }
  }
  else {
    envDataTableCompFactor = 0;
    if (ampRes == 0) {
      hcb_t = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvLevel10T;
      hcb_f = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvLevel10F;
    }
    else {
      hcb_t = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvLevel11T;
      hcb_f = (Huffman)&FDK_sbrDecoder_sbr_huffBook_EnvLevel11F;
    }
  }

  /*
    Now read raw envelope data
  */
  for (j = 0, offset = 0; j < nEnvelopes; j++) {


    if (h_frame_data->domain_vec[j] == 0) {
      if (coupling == COUPLING_BAL) {
        h_frame_data->iEnvelope[offset] =
          (FIXP_SGL) (( (int)FDKreadBits(hBs, start_bits_balance)) << envDataTableCompFactor);
      }
      else {
        h_frame_data->iEnvelope[offset] =
          (FIXP_SGL) (int)FDKreadBits (hBs, start_bits);
      }
    }

    for (i = (1 - h_frame_data->domain_vec[j]); i < no_band[j]; i++) {

      if (h_frame_data->domain_vec[j] == 0) {
        delta = DecodeHuffmanCW(hcb_f, hBs);
      }
      else {
        delta = DecodeHuffmanCW(hcb_t, hBs);
      }

      h_frame_data->iEnvelope[offset + i] = (FIXP_SGL) (delta << envDataTableCompFactor);
    }
    offset += no_band[j];
  }

#if ENV_EXP_FRACT
  /* Convert from int to scaled fract (ENV_EXP_FRACT bits for the fractional part) */
  for (i = 0; i < h_frame_data->nScaleFactors; i++) {
    h_frame_data->iEnvelope[i] <<= ENV_EXP_FRACT;
  }
#endif

  return 1;
}


//static const FRAME_INFO v_frame_info1_8 = { 0, 1, {0, 8}, {1}, -1, 1, {0, 8} };
static const FRAME_INFO v_frame_info2_8 = { 0, 2, {0, 4, 8}, {1, 1}, -1, 2, {0, 4, 8} };
static const FRAME_INFO v_frame_info4_8 = { 0, 4, {0, 2, 4, 6, 8}, {1, 1, 1, 1}, -1, 2, {0, 4, 8} };

/***************************************************************************/
/*!
  \brief    Generates frame info for FIXFIXonly frame class used for low delay version

  \return   nothing
 ****************************************************************************/
 static void generateFixFixOnly ( FRAME_INFO *hSbrFrameInfo,
                                  int tranPosInternal,
                                  int numberTimeSlots
                                )
{
    int nEnv, i, tranIdx;
    const int *pTable;

    switch (numberTimeSlots) {
        case 8:
            pTable = FDK_sbrDecoder_envelopeTable_8[tranPosInternal];
            break;
        case 15:
            pTable = FDK_sbrDecoder_envelopeTable_15[tranPosInternal];
            break;
        case 16:
            pTable = FDK_sbrDecoder_envelopeTable_16[tranPosInternal];
            break;
        default:
            FDK_ASSERT(0);
            /* in case assertion checks are disabled, force a definite memory fault at first access */
            pTable = NULL;
            break;
    }

    /* look number of envelopes in table */
    nEnv = pTable[0];
    /* look up envelope distribution in table */
    for (i=1; i<nEnv; i++)
        hSbrFrameInfo->borders[i] = pTable[i+2];
    /* open and close frame border */
    hSbrFrameInfo->borders[0]    = 0;
    hSbrFrameInfo->borders[nEnv] = numberTimeSlots;
    hSbrFrameInfo->nEnvelopes = nEnv;

   /* transient idx */
    tranIdx = hSbrFrameInfo->tranEnv = pTable[1];

    /* add noise floors */
    hSbrFrameInfo->bordersNoise[0] = 0;
    hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[tranIdx?tranIdx:1];
    hSbrFrameInfo->bordersNoise[2] = numberTimeSlots;
    /* nEnv is always > 1, so nNoiseEnvelopes is always 2 (IEC 14496-3 4.6.19.3.2) */
    hSbrFrameInfo->nNoiseEnvelopes = 2;
}

/*!
  \brief  Extracts LowDelaySBR control data from the bitstream.

  \return zero for bitstream error, one for correct.
*/
static int
extractLowDelayGrid (HANDLE_FDK_BITSTREAM hBitBuf,          /*!< bitbuffer handle */
                     HANDLE_SBR_HEADER_DATA hHeaderData,
                     HANDLE_SBR_FRAME_DATA h_frame_data, /*!< contains the FRAME_INFO struct to be filled */
                     int timeSlots
                    )
{
  FRAME_INFO * pFrameInfo = &h_frame_data->frameInfo;
  INT numberTimeSlots = hHeaderData->numberTimeSlots;
  INT temp = 0, k;

      /* FIXFIXonly framing case */
      h_frame_data->frameInfo.frameClass = 0;

      /* get the transient position from the bitstream */
      switch (timeSlots){
        case 8:
          /* 3bit transient position (temp={0;..;7}) */
          temp = FDKreadBits( hBitBuf, 3);
          break;

        case 16:
        case 15:
          /* 4bit transient position (temp={0;..;15}) */
          temp = FDKreadBits( hBitBuf, 4);
          break;

        default:
          return 0;
      }

      /* calculate borders according to the transient position */
      generateFixFixOnly ( pFrameInfo,
                           temp,
                           numberTimeSlots
                         );

      /* decode freq res: */
      for (k = 0; k < pFrameInfo->nEnvelopes; k++) {
          pFrameInfo->freqRes[k] = (UCHAR) FDKreadBits (hBitBuf, 1); /* f = F [1 bits] */          
      }


  return 1;
}

/*!
  \brief   Extract the frame information (structure FRAME_INFO) from the bitstream
  \return  Zero for bitstream error, one for correct.
*/
int
extractFrameInfo ( HANDLE_FDK_BITSTREAM   hBs,          /*!< bitbuffer handle */
                   HANDLE_SBR_HEADER_DATA hHeaderData,  /*!< Static control data */
                   HANDLE_SBR_FRAME_DATA  h_frame_data, /*!< pointer to memory where the frame-info will be stored */
                   const UINT             nrOfChannels,
                   const UINT             flags
                 )
{
  FRAME_INFO * pFrameInfo = &h_frame_data->frameInfo;
  int numberTimeSlots = hHeaderData->numberTimeSlots;
  int pointer_bits = 0, nEnv = 0, b = 0, border, i, n = 0,
    k, p, aL, aR, nL, nR,
    temp = 0, staticFreqRes;
  UCHAR frameClass;

  if (flags & SBRDEC_ELD_GRID) {
      /* CODEC_AACLD (LD+SBR) only uses the normal 0 Grid for non-transient Frames and the LowDelayGrid for transient Frames */
      frameClass = FDKreadBits (hBs, 1); /* frameClass = [1 bit] */
      if ( frameClass == 1 ) {
        /* if frameClass == 1, extract LowDelaySbrGrid, otherwise extract normal SBR-Grid for FIXIFX */
        /* extract the AACLD-Sbr-Grid */
        pFrameInfo->frameClass = frameClass;
        extractLowDelayGrid (hBs, hHeaderData, h_frame_data, numberTimeSlots);
        return 1;
      }
  } else
  {
    frameClass = FDKreadBits (hBs, 2); /* frameClass = C [2 bits] */
  }


  switch (frameClass) {
  case 0:
    temp = FDKreadBits (hBs, 2);     /* E [2 bits ] */
    nEnv = (int) (1 << temp);    /* E -> e */

    if ((flags & SBRDEC_ELD_GRID) && (nEnv == 1))
      h_frame_data->ampResolutionCurrentFrame = FDKreadBits( hBs, 1); /* new ELD Syntax 07-11-09 */

    staticFreqRes = FDKreadBits (hBs, 1);

    {
      if (nEnv > MAX_ENVELOPES_HEAAC)
        return 0;
    }

    b = nEnv + 1;
    switch (nEnv) {
    case 1:
      switch (numberTimeSlots) {
        case 15:
          FDKmemcpy (pFrameInfo, &FDK_sbrDecoder_sbr_frame_info1_15, sizeof(FRAME_INFO));
          break;
        case 16:
          FDKmemcpy (pFrameInfo, &FDK_sbrDecoder_sbr_frame_info1_16, sizeof(FRAME_INFO));
          break;
        default:
          FDK_ASSERT(0);
      }
      break;
    case 2:
      switch (numberTimeSlots) {
        case 15:
          FDKmemcpy (pFrameInfo, &FDK_sbrDecoder_sbr_frame_info2_15, sizeof(FRAME_INFO));
          break;
        case 16:
          FDKmemcpy (pFrameInfo, &FDK_sbrDecoder_sbr_frame_info2_16, sizeof(FRAME_INFO));
          break;
        default:
          FDK_ASSERT(0);
      }
      break;
    case 4:
      switch (numberTimeSlots) {
        case 15:
          FDKmemcpy (pFrameInfo, &FDK_sbrDecoder_sbr_frame_info4_15, sizeof(FRAME_INFO));
          break;
        case 16:
          FDKmemcpy (pFrameInfo, &FDK_sbrDecoder_sbr_frame_info4_16, sizeof(FRAME_INFO));
          break;
        default:
          FDK_ASSERT(0);
      }
      break;
    case 8:
#if (MAX_ENVELOPES >= 8)
      switch (numberTimeSlots) {
        case 15:
          FDKmemcpy (pFrameInfo, &FDK_sbrDecoder_sbr_frame_info8_15, sizeof(FRAME_INFO));
          break;
        case 16:
          FDKmemcpy (pFrameInfo, &FDK_sbrDecoder_sbr_frame_info8_16, sizeof(FRAME_INFO));
          break;
        default:
          FDK_ASSERT(0);
      }
      break;
#else
      return 0;
#endif
    }
    /* Apply correct freqRes (High is default) */
    if (!staticFreqRes) {
      for (i = 0; i < nEnv ; i++)
        pFrameInfo->freqRes[i] = 0;
    }

    break;
  case 1:
  case 2:
    temp = FDKreadBits (hBs, 2);  /* A [2 bits] */

    n    = FDKreadBits (hBs, 2);  /* n = N [2 bits] */

    nEnv = n + 1;                             /* # envelopes */
    b = nEnv + 1;                             /* # borders   */

    break;
  }

  switch (frameClass) {
  case 1:
    /* Decode borders: */
    pFrameInfo->borders[0] = 0;               /* first border          */
    border = temp + numberTimeSlots;          /* A -> aR               */
    i = b-1;                                  /* frame info index for last border */
    pFrameInfo->borders[i] = border;          /* last border                      */

    for (k = 0; k < n; k++) {
      temp = FDKreadBits (hBs, 2);/* R [2 bits] */
      border -= (2 * temp + 2);               /* R -> r                */
      pFrameInfo->borders[--i] = border;
    }


    /* Decode pointer: */
    pointer_bits = DFRACT_BITS - 1 - CountLeadingBits((FIXP_DBL)(n+1));
    p = FDKreadBits (hBs, pointer_bits);     /* p = P [pointer_bits bits] */

    if (p > n+1)
      return 0;

    pFrameInfo->tranEnv = p ? n + 2 - p : -1;


    /* Decode freq res: */
    for (k = n; k >= 0; k--) {
      pFrameInfo->freqRes[k] = FDKreadBits (hBs, 1); /* f = F [1 bits] */
    }


    /* Calculate noise floor middle border: */
    if (p == 0 || p == 1)
      pFrameInfo->bordersNoise[1] = pFrameInfo->borders[n];
    else
      pFrameInfo->bordersNoise[1] = pFrameInfo->borders[pFrameInfo->tranEnv];

    break;

  case 2:
    /* Decode borders: */
    border = temp;                            /* A -> aL */
    pFrameInfo->borders[0] = border;          /* first border */

    for (k = 1; k <= n; k++) {
      temp = FDKreadBits (hBs, 2);/* R [2 bits] */
      border += (2 * temp + 2);               /* R -> r                */
      pFrameInfo->borders[k] = border;
    }
    pFrameInfo->borders[k] = numberTimeSlots; /* last border */


    /* Decode pointer: */
    pointer_bits = DFRACT_BITS - 1 - CountLeadingBits((FIXP_DBL)(n+1));
    p = FDKreadBits (hBs, pointer_bits);     /* p = P [pointer_bits bits] */
    if (p > n+1)
      return 0;

    if (p == 0 || p == 1)
      pFrameInfo->tranEnv = -1;
    else
      pFrameInfo->tranEnv = p - 1;



    /* Decode freq res: */
    for (k = 0; k <= n; k++) {
      pFrameInfo->freqRes[k] = FDKreadBits(hBs, 1); /* f = F [1 bits] */
    }



    /* Calculate noise floor middle border: */
    switch (p) {
    case 0:
      pFrameInfo->bordersNoise[1] = pFrameInfo->borders[1];
      break;
    case 1:
      pFrameInfo->bordersNoise[1] = pFrameInfo->borders[n];
      break;
    default:
      pFrameInfo->bordersNoise[1] = pFrameInfo->borders[pFrameInfo->tranEnv];
      break;
    }

    break;

  case 3:
    /* v_ctrlSignal = [frameClass,aL,aR,nL,nR,v_rL,v_rR,p,v_fLR]; */

    aL = FDKreadBits (hBs, 2);       /* AL [2 bits], AL -> aL */

    aR = FDKreadBits (hBs, 2) + numberTimeSlots;     /* AR [2 bits], AR -> aR */

    nL = FDKreadBits (hBs, 2);       /* nL = NL [2 bits] */

    nR = FDKreadBits (hBs, 2);       /* nR = NR [2 bits] */



    /*-------------------------------------------------------------------------
      Calculate help variables
      --------------------------------------------------------------------------*/

    /* general: */
    nEnv = nL + nR + 1;            /* # envelopes */
    if (nEnv > MAX_ENVELOPES)
      return 0;
    b = nEnv + 1;                  /* # borders   */



    /*-------------------------------------------------------------------------
      Decode envelopes
      --------------------------------------------------------------------------*/


    /* L-borders:   */
    border            = aL;                   /* first border */
    pFrameInfo->borders[0] = border;

    for (k = 1; k <= nL; k++) {
      temp = FDKreadBits (hBs, 2);/* R [2 bits] */
      border += (2 * temp + 2);               /* R -> r                */
      pFrameInfo->borders[k] = border;
    }


    /* R-borders:  */
    border = aR;                              /* last border */
    i      = nEnv;

    pFrameInfo->borders[i] = border;

    for (k = 0; k < nR; k++) {
      temp = FDKreadBits (hBs, 2);/* R [2 bits] */
      border -= (2 * temp + 2);               /* R -> r                */
      pFrameInfo->borders[--i] = border;
    }


    /* decode pointer: */
    pointer_bits = DFRACT_BITS - 1 - CountLeadingBits((FIXP_DBL)(nL+nR+1));
    p = FDKreadBits (hBs, pointer_bits);     /* p = P [pointer_bits bits] */

    if (p > nL+nR+1)
      return 0;

    pFrameInfo->tranEnv = p ? b - p : -1;



    /* decode freq res: */
    for (k = 0; k < nEnv; k++) {
      pFrameInfo->freqRes[k] = FDKreadBits(hBs, 1); /* f = F [1 bits] */
    }



    /*-------------------------------------------------------------------------
      Decode noise floors
      --------------------------------------------------------------------------*/
    pFrameInfo->bordersNoise[0] = aL;

    if (nEnv == 1) {
      /* 1 noise floor envelope: */
      pFrameInfo->bordersNoise[1] = aR;
    }
    else {
      /* 2 noise floor envelopes */
      if (p == 0 || p == 1)
        pFrameInfo->bordersNoise[1] = pFrameInfo->borders[nEnv - 1];
      else
        pFrameInfo->bordersNoise[1] = pFrameInfo->borders[pFrameInfo->tranEnv];
      pFrameInfo->bordersNoise[2] = aR;
    }
    break;
  }


  /*
    Store number of envelopes, noise floor envelopes and frame class
  */
  pFrameInfo->nEnvelopes = nEnv;

  if (nEnv == 1)
    pFrameInfo->nNoiseEnvelopes = 1;
  else
    pFrameInfo->nNoiseEnvelopes = 2;

  pFrameInfo->frameClass = frameClass;

  if (pFrameInfo->frameClass == 2 || pFrameInfo->frameClass == 1) {
    /* calculate noise floor first and last borders: */
    pFrameInfo->bordersNoise[0] = pFrameInfo->borders[0];
    pFrameInfo->bordersNoise[pFrameInfo->nNoiseEnvelopes] = pFrameInfo->borders[nEnv];
  }


  return 1;
}


/*!
  \brief   Check if the frameInfo vector has reasonable values.
  \return  Zero for error, one for correct
*/
static int
checkFrameInfo (FRAME_INFO * pFrameInfo, /*!< pointer to frameInfo */
                int numberOfTimeSlots,   /*!< QMF time slots per frame */
                int overlap,             /*!< Amount of overlap QMF time slots */
                int timeStep)            /*!< QMF slots to SBR slots step factor */
{
  int maxPos,i,j;
  int startPos;
  int stopPos;
  int tranEnv;
  int startPosNoise;
  int stopPosNoise;
  int nEnvelopes = pFrameInfo->nEnvelopes;
  int nNoiseEnvelopes = pFrameInfo->nNoiseEnvelopes;

  if(nEnvelopes < 1 || nEnvelopes > MAX_ENVELOPES)
    return 0;

  if(nNoiseEnvelopes > MAX_NOISE_ENVELOPES)
    return 0;

  startPos        = pFrameInfo->borders[0];
  stopPos         = pFrameInfo->borders[nEnvelopes];
  tranEnv         = pFrameInfo->tranEnv;
  startPosNoise   = pFrameInfo->bordersNoise[0];
  stopPosNoise    = pFrameInfo->bordersNoise[nNoiseEnvelopes];

  if (overlap < 0 || overlap > (6)) {
    return 0;
  }
  if (timeStep < 1 || timeStep > 2) {
    return 0;
  }
  maxPos = numberOfTimeSlots + (overlap/timeStep);

  /* Check that the start and stop positions of the frame are reasonable values. */
  if( (startPos < 0) || (startPos >= stopPos) )
    return 0;
  if( startPos > maxPos-numberOfTimeSlots ) /* First env. must start in or directly after the overlap buffer */
    return 0;
  if( stopPos < numberOfTimeSlots ) /* One complete frame must be ready for output after processing */
    return 0;
  if(stopPos > maxPos)
    return 0;

  /* Check that the  start border for every envelope is strictly later in time */
  for(i=0;i<nEnvelopes;i++) {
    if(pFrameInfo->borders[i] >= pFrameInfo->borders[i+1])
      return 0;
  }

  /* Check that the envelope to be shortened is actually among the envelopes */
  if(tranEnv>nEnvelopes)
    return 0;


  /* Check the noise borders */
  if(nEnvelopes==1 && nNoiseEnvelopes>1)
    return 0;

  if(startPos != startPosNoise || stopPos != stopPosNoise)
    return 0;


  /* Check that the  start border for every noise-envelope is strictly later in time*/
  for(i=0; i<nNoiseEnvelopes; i++) {
    if(pFrameInfo->bordersNoise[i] >= pFrameInfo->bordersNoise[i+1])
      return 0;
  }

  /* Check that every noise border is the same as an envelope border*/
  for(i=0; i<nNoiseEnvelopes; i++) {
    startPosNoise = pFrameInfo->bordersNoise[i];

    for(j=0; j<nEnvelopes; j++) {
      if(pFrameInfo->borders[j] == startPosNoise)
        break;
    }
    if(j==nEnvelopes)
      return 0;
  }

  return 1;
}
