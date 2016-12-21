
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

/*!
  \file
  \brief  parametric stereo decoder  
*/

#include "psdec.h"



#include "FDK_bitbuffer.h"
#include "psdec_hybrid.h"

#include "sbr_rom.h"
#include "sbr_ram.h"

#include "FDK_tools_rom.h"

#include "genericStds.h"

#include "FDK_trigFcts.h"


/********************************************************************/
/*                       MLQUAL DEFINES                             */
/********************************************************************/

  #define FRACT_ZERO FRACT_BITS-1
/********************************************************************/

SBR_ERROR ResetPsDec( HANDLE_PS_DEC h_ps_d );

void ResetPsDeCor( HANDLE_PS_DEC h_ps_d );


/***** HELPERS *****/

static void assignTimeSlotsPS (FIXP_DBL *bufAdr, FIXP_DBL **bufPtr, const int numSlots, const int numChan);



/*******************/

#define DIV3 FL2FXCONST_DBL(1.f/3.f)     /* division 3.0 */
#define DIV1_5 FL2FXCONST_DBL(2.f/3.f)   /* division 1.5 */

/***************************************************************************/
/*!
  \brief  Creates one instance of the PS_DEC struct

  \return Error info

****************************************************************************/
int
CreatePsDec( HANDLE_PS_DEC *h_PS_DEC,   /*!< pointer to the module state */
             int aacSamplesPerFrame
           )
{
  SBR_ERROR errorInfo = SBRDEC_OK;
  HANDLE_PS_DEC  h_ps_d;
  int i;

  if (*h_PS_DEC == NULL) {
    /* Get ps dec ram */
    h_ps_d = GetRam_ps_dec();
    if (h_ps_d == NULL) {
      errorInfo = SBRDEC_MEM_ALLOC_FAILED;
      goto bail;
    }
  } else {
    /* Reset an open instance */
    h_ps_d = *h_PS_DEC;
  }

   /* initialisation */
  switch (aacSamplesPerFrame) {
  case 960:
    h_ps_d->noSubSamples = 30;              /* col */
    break;
  case 1024:
    h_ps_d->noSubSamples = 32;              /* col */
    break;
  default:
    h_ps_d->noSubSamples = -1;
    break;
  }

  if (h_ps_d->noSubSamples >  MAX_NUM_COL
   || h_ps_d->noSubSamples <= 0)
  {
    goto bail;
  }
  h_ps_d->noChannels   = NO_QMF_CHANNELS;   /* row */

  h_ps_d->psDecodedPrv   =  0;
  h_ps_d->procFrameBased = -1;
  for (i = 0; i < (1)+1; i++) {
    h_ps_d->bPsDataAvail[i]  =  ppt_none;
  }


  for (i = 0; i < (1)+1; i++) {
    FDKmemclear(&h_ps_d->bsData[i].mpeg, sizeof(MPEG_PS_BS_DATA));
  }

  errorInfo = ResetPsDec( h_ps_d );

  if ( errorInfo != SBRDEC_OK )
    goto bail;

  ResetPsDeCor( h_ps_d );

  *h_PS_DEC = h_ps_d;



  return 0;

bail:
  DeletePsDec(&h_ps_d);

  return -1;
} /*END CreatePsDec */

/***************************************************************************/
/*!
  \brief  Delete one instance of the PS_DEC struct

  \return Error info

****************************************************************************/
int
DeletePsDec( HANDLE_PS_DEC *h_PS_DEC)  /*!< pointer to the module state */
{
  if (*h_PS_DEC == NULL) {
    return -1;
  }


  FreeRam_ps_dec(h_PS_DEC);


  return 0;
} /*END DeletePsDec */

/***************************************************************************/
/*!
  \brief resets some values of the PS handle to default states

  \return

****************************************************************************/
SBR_ERROR ResetPsDec( HANDLE_PS_DEC h_ps_d )  /*!< pointer to the module state */
{
  SBR_ERROR errorInfo = SBRDEC_OK;
  INT i;

  const UCHAR noQmfBandsInHybrid20 = 3;
  /* const UCHAR noQmfBandsInHybrid34 = 5; */

  const UCHAR aHybridResolution20[] = { HYBRID_8_CPLX,
                                        HYBRID_2_REAL,
                                        HYBRID_2_REAL };

  h_ps_d->specificTo.mpeg.delayBufIndex   = 0;

  /* explicitly init state variables to safe values (until first ps header arrives) */

  h_ps_d->specificTo.mpeg.lastUsb        =  0;

  h_ps_d->specificTo.mpeg.scaleFactorPsDelayBuffer = -(DFRACT_BITS-1);

  FDKmemclear(h_ps_d->specificTo.mpeg.aDelayBufIndexDelayQmf, (NO_QMF_CHANNELS-FIRST_DELAY_SB)*sizeof(UCHAR));
  h_ps_d->specificTo.mpeg.noSampleDelay = delayIndexQmf[0];

  for (i=0 ; i < NO_SERIAL_ALLPASS_LINKS; i++) {
    h_ps_d->specificTo.mpeg.aDelayRBufIndexSer[i] = 0;
  }

  h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[0] = h_ps_d->specificTo.mpeg.aaQmfDelayBufReal;

  assignTimeSlotsPS ( h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[0] + (NO_QMF_CHANNELS-FIRST_DELAY_SB),
                     &h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[1],
                      h_ps_d->specificTo.mpeg.noSampleDelay-1,
                      (NO_DELAY_BUFFER_BANDS-FIRST_DELAY_SB));

  h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[0] = h_ps_d->specificTo.mpeg.aaQmfDelayBufImag;

  assignTimeSlotsPS ( h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[0] + (NO_QMF_CHANNELS-FIRST_DELAY_SB),
                     &h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[1],
                      h_ps_d->specificTo.mpeg.noSampleDelay-1,
                      (NO_DELAY_BUFFER_BANDS-FIRST_DELAY_SB));

  /* Hybrid Filter Bank 1 creation. */
  errorInfo = InitHybridFilterBank ( &h_ps_d->specificTo.mpeg.hybrid,
                                      h_ps_d->noSubSamples,
                                      noQmfBandsInHybrid20,
                                      aHybridResolution20 );

  for ( i = 0; i < NO_IID_GROUPS; i++ )
  {
    h_ps_d->specificTo.mpeg.h11rPrev[i] = FL2FXCONST_DBL(0.5f);
    h_ps_d->specificTo.mpeg.h12rPrev[i] = FL2FXCONST_DBL(0.5f);
  }

  FDKmemclear( h_ps_d->specificTo.mpeg.h21rPrev, sizeof( h_ps_d->specificTo.mpeg.h21rPrev ) );
  FDKmemclear( h_ps_d->specificTo.mpeg.h22rPrev, sizeof( h_ps_d->specificTo.mpeg.h22rPrev ) );

  return errorInfo;
}

/***************************************************************************/
/*!
  \brief  clear some buffers used in decorrelation process

  \return

****************************************************************************/
void ResetPsDeCor( HANDLE_PS_DEC h_ps_d )  /*!< pointer to the module state */
{
  INT i;

  FDKmemclear(h_ps_d->specificTo.mpeg.aPeakDecayFastBin, NO_MID_RES_BINS*sizeof(FIXP_DBL));
  FDKmemclear(h_ps_d->specificTo.mpeg.aPrevNrgBin, NO_MID_RES_BINS*sizeof(FIXP_DBL));
  FDKmemclear(h_ps_d->specificTo.mpeg.aPrevPeakDiffBin, NO_MID_RES_BINS*sizeof(FIXP_DBL));
  FDKmemclear(h_ps_d->specificTo.mpeg.aPowerPrevScal, NO_MID_RES_BINS*sizeof(SCHAR));

  for (i=0 ; i < FIRST_DELAY_SB ; i++) {
    FDKmemclear(h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerQmf[i], NO_DELAY_LENGTH_VECTORS*sizeof(FIXP_DBL));
    FDKmemclear(h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerQmf[i], NO_DELAY_LENGTH_VECTORS*sizeof(FIXP_DBL));
  }
  for (i=0 ; i < NO_SUB_QMF_CHANNELS ; i++) {
    FDKmemclear(h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerSubQmf[i], NO_DELAY_LENGTH_VECTORS*sizeof(FIXP_DBL));
    FDKmemclear(h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerSubQmf[i], NO_DELAY_LENGTH_VECTORS*sizeof(FIXP_DBL));
  }

}

/*******************************************************************************/

/* slot based funcion prototypes */

static void deCorrelateSlotBased( HANDLE_PS_DEC h_ps_d,

                                  FIXP_DBL    *mHybridRealLeft,
                                  FIXP_DBL    *mHybridImagLeft,
                                  SCHAR        sf_mHybridLeft,

                                  FIXP_DBL    *rIntBufferLeft,
                                  FIXP_DBL    *iIntBufferLeft,
                                  SCHAR        sf_IntBuffer,

                                  FIXP_DBL    *mHybridRealRight,
                                  FIXP_DBL    *mHybridImagRight,

                                  FIXP_DBL    *rIntBufferRight,
                                  FIXP_DBL    *iIntBufferRight );

static void applySlotBasedRotation( HANDLE_PS_DEC h_ps_d,

                                    FIXP_DBL  *mHybridRealLeft,
                                    FIXP_DBL  *mHybridImagLeft,

                                    FIXP_DBL  *QmfLeftReal,
                                    FIXP_DBL  *QmfLeftImag,

                                    FIXP_DBL  *mHybridRealRight,
                                    FIXP_DBL  *mHybridImagRight,

                                    FIXP_DBL  *QmfRightReal,
                                    FIXP_DBL  *QmfRightImag
                                  );


/***************************************************************************/
/*!
  \brief  Get scale factor for all ps delay buffer.

  \return

****************************************************************************/
static
int getScaleFactorPsStatesBuffer(HANDLE_PS_DEC   h_ps_d)
{
  INT i;
  int scale = DFRACT_BITS-1;

  for (i=0; i<NO_QMF_BANDS_HYBRID20; i++) {
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.hybrid.mQmfBufferRealSlot[i], NO_SUB_QMF_CHANNELS));
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.hybrid.mQmfBufferImagSlot[i], NO_SUB_QMF_CHANNELS));
  }

  for (i=0; i<NO_SAMPLE_DELAY_ALLPASS; i++) {
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.aaRealDelayBufferQmf[i], FIRST_DELAY_SB));
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.aaImagDelayBufferQmf[i], FIRST_DELAY_SB));
  }

  for (i=0; i<NO_SAMPLE_DELAY_ALLPASS; i++) {
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.aaRealDelayBufferSubQmf[i], NO_SUB_QMF_CHANNELS));
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.aaImagDelayBufferSubQmf[i], NO_SUB_QMF_CHANNELS));
  }

  for (i=0; i<FIRST_DELAY_SB; i++) {
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerQmf[i], NO_DELAY_LENGTH_VECTORS));
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerQmf[i], NO_DELAY_LENGTH_VECTORS));
  }

  for (i=0; i<NO_SUB_QMF_CHANNELS; i++) {
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerSubQmf[i], NO_DELAY_LENGTH_VECTORS));
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerSubQmf[i], NO_DELAY_LENGTH_VECTORS));
  }

  for (i=0; i<MAX_DELAY_BUFFER_SIZE; i++)
  {
    INT len;
    if (i==0)
      len = NO_QMF_CHANNELS-FIRST_DELAY_SB;
    else
      len = NO_DELAY_BUFFER_BANDS-FIRST_DELAY_SB;

    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[i], len));
    scale = fMin(scale, getScalefactor(h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[i], len));
  }

  return (scale);
}

/***************************************************************************/
/*!
  \brief  Rescale all ps delay buffer.

  \return

****************************************************************************/
static
void scalePsStatesBuffer(HANDLE_PS_DEC h_ps_d,
                         int           scale)
{
  INT i;

  if (scale < 0)
    scale = fixMax((INT)scale,(INT)-(DFRACT_BITS-1));
  else
    scale = fixMin((INT)scale,(INT)DFRACT_BITS-1);

  for (i=0; i<NO_QMF_BANDS_HYBRID20; i++) {
    scaleValues( h_ps_d->specificTo.mpeg.hybrid.mQmfBufferRealSlot[i], NO_SUB_QMF_CHANNELS, scale );
    scaleValues( h_ps_d->specificTo.mpeg.hybrid.mQmfBufferImagSlot[i], NO_SUB_QMF_CHANNELS, scale );
  }

  for (i=0; i<NO_SAMPLE_DELAY_ALLPASS; i++) {
    scaleValues( h_ps_d->specificTo.mpeg.aaRealDelayBufferQmf[i], FIRST_DELAY_SB, scale );
    scaleValues( h_ps_d->specificTo.mpeg.aaImagDelayBufferQmf[i], FIRST_DELAY_SB, scale );
  }

  for (i=0; i<NO_SAMPLE_DELAY_ALLPASS; i++) {
    scaleValues( h_ps_d->specificTo.mpeg.aaRealDelayBufferSubQmf[i], NO_SUB_QMF_CHANNELS, scale );
    scaleValues( h_ps_d->specificTo.mpeg.aaImagDelayBufferSubQmf[i], NO_SUB_QMF_CHANNELS, scale );
  }

  for (i=0; i<FIRST_DELAY_SB; i++) {
    scaleValues( h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerQmf[i], NO_DELAY_LENGTH_VECTORS, scale );
    scaleValues( h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerQmf[i], NO_DELAY_LENGTH_VECTORS, scale );
  }

  for (i=0; i<NO_SUB_QMF_CHANNELS; i++) {
    scaleValues( h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerSubQmf[i], NO_DELAY_LENGTH_VECTORS, scale );
    scaleValues( h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerSubQmf[i], NO_DELAY_LENGTH_VECTORS, scale );
  }

  for (i=0; i<MAX_DELAY_BUFFER_SIZE; i++) {
    INT len;
    if (i==0)
      len = NO_QMF_CHANNELS-FIRST_DELAY_SB;
    else
      len = NO_DELAY_BUFFER_BANDS-FIRST_DELAY_SB;

    scaleValues( h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[i], len, scale );
    scaleValues( h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[i], len, scale );
  }

  scale <<= 1;

  scaleValues( h_ps_d->specificTo.mpeg.aPeakDecayFastBin, NO_MID_RES_BINS, scale );
  scaleValues( h_ps_d->specificTo.mpeg.aPrevPeakDiffBin, NO_MID_RES_BINS, scale );
  scaleValues( h_ps_d->specificTo.mpeg.aPrevNrgBin, NO_MID_RES_BINS, scale );
}

/***************************************************************************/
/*!
  \brief  Scale input channel to the same scalefactor and rescale hybrid
          filterbank values

  \return

****************************************************************************/

void scalFilterBankValues( HANDLE_PS_DEC   h_ps_d,
                           FIXP_DBL      **fixpQmfReal,
                           FIXP_DBL      **fixpQmfImag,
                           int             lsb,
                           int             scaleFactorLowBandSplitLow,
                           int             scaleFactorLowBandSplitHigh,
                           SCHAR          *scaleFactorLowBand_lb,
                           SCHAR          *scaleFactorLowBand_hb,
                           int             scaleFactorHighBands,
                           INT            *scaleFactorHighBand,
                           INT             noCols
                         )
{
  INT maxScal;

  INT i;

  scaleFactorHighBands        =  -scaleFactorHighBands;
  scaleFactorLowBandSplitLow  =  -scaleFactorLowBandSplitLow;
  scaleFactorLowBandSplitHigh =  -scaleFactorLowBandSplitHigh;

  /* get max scale factor */
  maxScal = fixMax(scaleFactorHighBands,fixMax(scaleFactorLowBandSplitLow, scaleFactorLowBandSplitHigh ));

  {
    int headroom  = getScaleFactorPsStatesBuffer(h_ps_d);
    maxScal   = fixMax(maxScal,(INT)(h_ps_d->specificTo.mpeg.scaleFactorPsDelayBuffer-headroom));
    maxScal  += 1;
  }

  /* scale whole left channel to the same scale factor */

  /* low band ( overlap buffer ) */
  if ( maxScal != scaleFactorLowBandSplitLow ) {
    INT scale = scaleFactorLowBandSplitLow - maxScal;
    for ( i=0; i<(6); i++ ) {
      scaleValues( fixpQmfReal[i], lsb, scale );
      scaleValues( fixpQmfImag[i], lsb, scale );
    }
  }
  /* low band ( current frame ) */
  if ( maxScal != scaleFactorLowBandSplitHigh ) {
    INT scale = scaleFactorLowBandSplitHigh - maxScal;
    /* for ( i=(6); i<(6)+MAX_NUM_COL; i++ ) { */
    for ( i=(6); i<(6)+noCols; i++ ) {
      scaleValues( fixpQmfReal[i], lsb, scale );
      scaleValues( fixpQmfImag[i], lsb, scale );
    }
  }
  /* high band */
  if ( maxScal != scaleFactorHighBands ) {
    INT scale = scaleFactorHighBands - maxScal;
    /* for ( i=0; i<MAX_NUM_COL; i++ ) { */
    for ( i=0; i<noCols; i++ ) {
      scaleValues( &fixpQmfReal[i][lsb], (64)-lsb, scale );
      scaleValues( &fixpQmfImag[i][lsb], (64)-lsb, scale );
    }
  }

  if ( maxScal != h_ps_d->specificTo.mpeg.scaleFactorPsDelayBuffer )
    scalePsStatesBuffer(h_ps_d,(h_ps_d->specificTo.mpeg.scaleFactorPsDelayBuffer-maxScal));

  h_ps_d->specificTo.mpeg.hybrid.sf_mQmfBuffer = maxScal;
  h_ps_d->specificTo.mpeg.scaleFactorPsDelayBuffer = maxScal;

  *scaleFactorHighBand += maxScal - scaleFactorHighBands;

  h_ps_d->rescal = maxScal - scaleFactorLowBandSplitHigh;
  h_ps_d->sf_IntBuffer = maxScal;

  *scaleFactorLowBand_lb += maxScal - scaleFactorLowBandSplitLow;
  *scaleFactorLowBand_hb += maxScal - scaleFactorLowBandSplitHigh;
}

void rescalFilterBankValues( HANDLE_PS_DEC   h_ps_d,                      /* parametric stereo decoder handle     */
                             FIXP_DBL      **QmfBufferReal,               /* qmf filterbank values                */
                             FIXP_DBL      **QmfBufferImag,               /* qmf filterbank values                */
                             int             lsb,                         /* sbr start subband                    */
                             INT             noCols)
{
  int i;
  /* scale back 6 timeslots look ahead for hybrid filterbank to original value */
  for ( i=noCols; i<noCols + (6); i++ ) {
    scaleValues( QmfBufferReal[i], lsb, h_ps_d->rescal );
    scaleValues( QmfBufferImag[i], lsb, h_ps_d->rescal );
  }
}

/***************************************************************************/
/*!
  \brief  Generate decorrelated side channel using allpass/delay

  \return

****************************************************************************/
static void
deCorrelateSlotBased( HANDLE_PS_DEC h_ps_d,            /*!< pointer to the module state */

                      FIXP_DBL    *mHybridRealLeft,    /*!< left (mono) hybrid values real */
                      FIXP_DBL    *mHybridImagLeft,    /*!< left (mono) hybrid values imag */
                      SCHAR        sf_mHybridLeft,     /*!< scalefactor for left (mono) hybrid bands */

                      FIXP_DBL    *rIntBufferLeft,     /*!< real qmf bands left (mono) (38x64) */
                      FIXP_DBL    *iIntBufferLeft,     /*!< real qmf bands left (mono) (38x64) */
                      SCHAR        sf_IntBuffer,       /*!< scalefactor for all left and right qmf bands   */

                      FIXP_DBL    *mHybridRealRight,   /*!< right (decorrelated) hybrid values real */
                      FIXP_DBL    *mHybridImagRight,   /*!< right (decorrelated) hybrid values imag */

                      FIXP_DBL    *rIntBufferRight,    /*!< real qmf bands right (decorrelated) (38x64) */
                      FIXP_DBL    *iIntBufferRight )   /*!< real qmf bands right (decorrelated) (38x64) */
{

  INT  i, m, sb, gr, bin;

  FIXP_DBL peakDiff, nrg, transRatio;

  FIXP_DBL *RESTRICT aaLeftReal;
  FIXP_DBL *RESTRICT aaLeftImag;

  FIXP_DBL *RESTRICT aaRightReal;
  FIXP_DBL *RESTRICT aaRightImag;

  FIXP_DBL *RESTRICT pRealDelayBuffer;
  FIXP_DBL *RESTRICT pImagDelayBuffer;

  C_ALLOC_SCRATCH_START(aaPowerSlot, FIXP_DBL, NO_MID_RES_BINS);
  C_ALLOC_SCRATCH_START(aaTransRatioSlot, FIXP_DBL, NO_MID_RES_BINS);

/*!
<pre>
   parameter index       qmf bands             hybrid bands
  ----------------------------------------------------------------------------
         0                   0                      0,7
         1                   0                      1,6
         2                   0                      2
         3                   0                      3           HYBRID BANDS
         4                   1                      9
         5                   1                      8
         6                   2                     10
         7                   2                     11
  ----------------------------------------------------------------------------
         8                   3
         9                   4
        10                   5
        11                   6
        12                   7
        13                   8
        14                   9,10      (2 )                      QMF BANDS
        15                   11 - 13   (3 )
        16                   14 - 17   (4 )
        17                   18 - 22   (5 )
        18                   23 - 34   (12)
        19                   35 - 63   (29)
  ----------------------------------------------------------------------------
</pre>
*/

  #define FLTR_SCALE 3

  /* hybrid bands (parameter index 0 - 7) */
  aaLeftReal  = mHybridRealLeft;
  aaLeftImag  = mHybridImagLeft;

  aaPowerSlot[0] = ( fMultAddDiv2( fMultDiv2(aaLeftReal[0],  aaLeftReal[0]),  aaLeftImag[0],  aaLeftImag[0] ) >> FLTR_SCALE ) +
                   ( fMultAddDiv2( fMultDiv2(aaLeftReal[7],  aaLeftReal[7]),  aaLeftImag[7],  aaLeftImag[7] ) >> FLTR_SCALE );

  aaPowerSlot[1] = ( fMultAddDiv2( fMultDiv2(aaLeftReal[1],  aaLeftReal[1]),  aaLeftImag[1],  aaLeftImag[1] ) >> FLTR_SCALE ) +
                   ( fMultAddDiv2( fMultDiv2(aaLeftReal[6],  aaLeftReal[6]),  aaLeftImag[6],  aaLeftImag[6] ) >> FLTR_SCALE );

  aaPowerSlot[2] =   fMultAddDiv2( fMultDiv2(aaLeftReal[2],  aaLeftReal[2]),  aaLeftImag[2],  aaLeftImag[2] ) >> FLTR_SCALE;
  aaPowerSlot[3] =   fMultAddDiv2( fMultDiv2(aaLeftReal[3],  aaLeftReal[3]),  aaLeftImag[3],  aaLeftImag[3] ) >> FLTR_SCALE;

  aaPowerSlot[4] =   fMultAddDiv2( fMultDiv2(aaLeftReal[9],  aaLeftReal[9]),  aaLeftImag[9],  aaLeftImag[9] ) >> FLTR_SCALE;
  aaPowerSlot[5] =   fMultAddDiv2( fMultDiv2(aaLeftReal[8],  aaLeftReal[8]),  aaLeftImag[8],  aaLeftImag[8] ) >> FLTR_SCALE;

  aaPowerSlot[6] =   fMultAddDiv2( fMultDiv2(aaLeftReal[10], aaLeftReal[10]), aaLeftImag[10], aaLeftImag[10] ) >> FLTR_SCALE;
  aaPowerSlot[7] =   fMultAddDiv2( fMultDiv2(aaLeftReal[11], aaLeftReal[11]), aaLeftImag[11], aaLeftImag[11] ) >> FLTR_SCALE;

  /* qmf bands (parameter index 8 - 19) */
  for ( bin = 8; bin < NO_MID_RES_BINS; bin++ ) {
    FIXP_DBL slotNrg = FL2FXCONST_DBL(0.f);

    for ( i = groupBorders20[bin+2]; i < groupBorders20[bin+3]; i++ ) {  /* max loops: 29 */
      slotNrg += fMultAddDiv2 ( fMultDiv2(rIntBufferLeft[i], rIntBufferLeft[i]), iIntBufferLeft[i], iIntBufferLeft[i]) >> FLTR_SCALE;
    }
    aaPowerSlot[bin] = slotNrg;

  }


  /* calculation of transient ratio */
  for (bin=0; bin < NO_MID_RES_BINS; bin++) {   /* noBins = 20 ( BASELINE_PS ) */

    h_ps_d->specificTo.mpeg.aPeakDecayFastBin[bin] = fMult( h_ps_d->specificTo.mpeg.aPeakDecayFastBin[bin], PEAK_DECAY_FACTOR );

    if (h_ps_d->specificTo.mpeg.aPeakDecayFastBin[bin] < aaPowerSlot[bin]) {
      h_ps_d->specificTo.mpeg.aPeakDecayFastBin[bin] = aaPowerSlot[bin];
    }

    /* calculate PSmoothPeakDecayDiffNrg */
    peakDiff = fMultAdd ( (h_ps_d->specificTo.mpeg.aPrevPeakDiffBin[bin]>>1),
                 INT_FILTER_COEFF, h_ps_d->specificTo.mpeg.aPeakDecayFastBin[bin] - aaPowerSlot[bin] - h_ps_d->specificTo.mpeg.aPrevPeakDiffBin[bin]);

    /* save peakDiff for the next frame */
    h_ps_d->specificTo.mpeg.aPrevPeakDiffBin[bin] = peakDiff;

    nrg = h_ps_d->specificTo.mpeg.aPrevNrgBin[bin] + fMult( INT_FILTER_COEFF, aaPowerSlot[bin] - h_ps_d->specificTo.mpeg.aPrevNrgBin[bin] );

    /* Negative energies don't exist. But sometimes they appear due to rounding. */

    nrg = fixMax(nrg,FL2FXCONST_DBL(0.f));

    /* save nrg for the next frame */
    h_ps_d->specificTo.mpeg.aPrevNrgBin[bin] = nrg;

    nrg = fMult( nrg, TRANSIENT_IMPACT_FACTOR );

    /* save transient impact factor */
    if ( peakDiff <= nrg || peakDiff == FL2FXCONST_DBL(0.0) ) {
      aaTransRatioSlot[bin] = (FIXP_DBL)MAXVAL_DBL /* FL2FXCONST_DBL(1.0f)*/;
    }
    else if ( nrg <= FL2FXCONST_DBL(0.0f) ) {
        aaTransRatioSlot[bin] = FL2FXCONST_DBL(0.f);
      }
    else {
      /* scale to denominator */
      INT scale_left = fixMax(0, CntLeadingZeros(peakDiff) - 1);
      aaTransRatioSlot[bin] = schur_div( nrg<<scale_left, peakDiff<<scale_left, 16);
    }
  } /* bin */




  #define DELAY_GROUP_OFFSET    20
  #define NR_OF_DELAY_GROUPS     2

  FIXP_DBL rTmp, iTmp, rTmp0, iTmp0, rR0, iR0;

  INT TempDelay     = h_ps_d->specificTo.mpeg.delayBufIndex;  /* set delay indices */

  pRealDelayBuffer = h_ps_d->specificTo.mpeg.aaRealDelayBufferSubQmf[TempDelay];
  pImagDelayBuffer = h_ps_d->specificTo.mpeg.aaImagDelayBufferSubQmf[TempDelay];

  aaLeftReal  = mHybridRealLeft;
  aaLeftImag  = mHybridImagLeft;
  aaRightReal = mHybridRealRight;
  aaRightImag = mHybridImagRight;

  /************************/
  /* ICC groups :  0 -  9 */
  /************************/

  /* gr = ICC groups */
  for (gr=0; gr < SUBQMF_GROUPS; gr++) {

    transRatio = aaTransRatioSlot[bins2groupMap20[gr]];

    /* sb = subQMF/QMF subband */
    sb = groupBorders20[gr];

    /* Update delay buffers, sample delay allpass = 2 */
    rTmp0 = pRealDelayBuffer[sb];
    iTmp0 = pImagDelayBuffer[sb];

    pRealDelayBuffer[sb] = aaLeftReal[sb];
    pImagDelayBuffer[sb] = aaLeftImag[sb];

    /* delay by fraction */
    cplxMultDiv2(&rR0, &iR0, rTmp0, iTmp0, aaFractDelayPhaseFactorReSubQmf20[sb], aaFractDelayPhaseFactorImSubQmf20[sb]);
    rR0<<=1;
    iR0<<=1;

    FIXP_DBL *pAaaRealDelayRBufferSerSubQmf = h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerSubQmf[sb];
    FIXP_DBL *pAaaImagDelayRBufferSerSubQmf = h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerSubQmf[sb];

    for (m=0; m<NO_SERIAL_ALLPASS_LINKS ; m++) {

      INT tmpDelayRSer = h_ps_d->specificTo.mpeg.aDelayRBufIndexSer[m];

      /* get delayed values from according buffer : m(0)=3; m(1)=4; m(2)=5; */
      rTmp0 = pAaaRealDelayRBufferSerSubQmf[tmpDelayRSer];
      iTmp0 = pAaaImagDelayRBufferSerSubQmf[tmpDelayRSer];

      /* delay by fraction */
      cplxMultDiv2(&rTmp, &iTmp, rTmp0, iTmp0, aaFractDelayPhaseFactorSerReSubQmf20[sb][m], aaFractDelayPhaseFactorSerImSubQmf20[sb][m]);

      rTmp = (rTmp - fMultDiv2(aAllpassLinkDecaySer[m], rR0)) << 1;
      iTmp = (iTmp - fMultDiv2(aAllpassLinkDecaySer[m], iR0)) << 1;

      pAaaRealDelayRBufferSerSubQmf[tmpDelayRSer] = rR0 + fMult(aAllpassLinkDecaySer[m], rTmp);
      pAaaImagDelayRBufferSerSubQmf[tmpDelayRSer] = iR0 + fMult(aAllpassLinkDecaySer[m], iTmp);

      rR0 = rTmp;
      iR0 = iTmp;

      pAaaRealDelayRBufferSerSubQmf += aAllpassLinkDelaySer[m];
      pAaaImagDelayRBufferSerSubQmf += aAllpassLinkDelaySer[m];

    } /* m */

    /* duck if a past transient is found */
    aaRightReal[sb] = fMult(transRatio, rR0);
    aaRightImag[sb] = fMult(transRatio, iR0);

  } /* gr */


  scaleValues( mHybridRealLeft,  NO_SUB_QMF_CHANNELS, -SCAL_HEADROOM );
  scaleValues( mHybridImagLeft,  NO_SUB_QMF_CHANNELS, -SCAL_HEADROOM );
  scaleValues( mHybridRealRight, NO_SUB_QMF_CHANNELS, -SCAL_HEADROOM );
  scaleValues( mHybridImagRight, NO_SUB_QMF_CHANNELS, -SCAL_HEADROOM );


  /************************/

  aaLeftReal  = rIntBufferLeft;
  aaLeftImag  = iIntBufferLeft;
  aaRightReal = rIntBufferRight;
  aaRightImag = iIntBufferRight;

  pRealDelayBuffer = h_ps_d->specificTo.mpeg.aaRealDelayBufferQmf[TempDelay];
  pImagDelayBuffer = h_ps_d->specificTo.mpeg.aaImagDelayBufferQmf[TempDelay];

  /************************/
  /* ICC groups : 10 - 19 */
  /************************/


  /* gr = ICC groups */
  for (gr=SUBQMF_GROUPS; gr < NO_IID_GROUPS - NR_OF_DELAY_GROUPS; gr++) {

    transRatio = aaTransRatioSlot[bins2groupMap20[gr]];

    /* sb = subQMF/QMF subband */
    for (sb = groupBorders20[gr]; sb < groupBorders20[gr+1]; sb++) {
      FIXP_DBL resR, resI;

      /* decayScaleFactor = 1.0f + decay_cutoff * DECAY_SLOPE - DECAY_SLOPE * sb; DECAY_SLOPE = 0.05 */
      FIXP_DBL decayScaleFactor = decayScaleFactTable[sb];

      /* Update delay buffers, sample delay allpass = 2 */
      rTmp0 = pRealDelayBuffer[sb];
      iTmp0 = pImagDelayBuffer[sb];

      pRealDelayBuffer[sb] = aaLeftReal[sb];
      pImagDelayBuffer[sb] = aaLeftImag[sb];

      /* delay by fraction */
      cplxMultDiv2(&rR0, &iR0, rTmp0, iTmp0, aaFractDelayPhaseFactorReQmf[sb], aaFractDelayPhaseFactorImQmf[sb]);
      rR0<<=1;
      iR0<<=1;

      resR = fMult(decayScaleFactor, rR0);
      resI = fMult(decayScaleFactor, iR0);

      FIXP_DBL *pAaaRealDelayRBufferSerQmf = h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerQmf[sb];
      FIXP_DBL *pAaaImagDelayRBufferSerQmf = h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerQmf[sb];

      for (m=0; m<NO_SERIAL_ALLPASS_LINKS ; m++) {

        INT tmpDelayRSer = h_ps_d->specificTo.mpeg.aDelayRBufIndexSer[m];

        /* get delayed values from according buffer : m(0)=3; m(1)=4; m(2)=5; */
        rTmp0 = pAaaRealDelayRBufferSerQmf[tmpDelayRSer];
        iTmp0 = pAaaImagDelayRBufferSerQmf[tmpDelayRSer];

        /* delay by fraction */
        cplxMultDiv2(&rTmp, &iTmp, rTmp0, iTmp0, aaFractDelayPhaseFactorSerReQmf[sb][m], aaFractDelayPhaseFactorSerImQmf[sb][m]);

        rTmp = (rTmp - fMultDiv2(aAllpassLinkDecaySer[m], resR))<<1;
        iTmp = (iTmp - fMultDiv2(aAllpassLinkDecaySer[m], resI))<<1;

        resR = fMult(decayScaleFactor, rTmp);
        resI = fMult(decayScaleFactor, iTmp);

        pAaaRealDelayRBufferSerQmf[tmpDelayRSer] = rR0 + fMult(aAllpassLinkDecaySer[m], resR);
        pAaaImagDelayRBufferSerQmf[tmpDelayRSer] = iR0 + fMult(aAllpassLinkDecaySer[m], resI);

        rR0 = rTmp;
        iR0 = iTmp;

        pAaaRealDelayRBufferSerQmf += aAllpassLinkDelaySer[m];
        pAaaImagDelayRBufferSerQmf += aAllpassLinkDelaySer[m];

      } /* m */

      /* duck if a past transient is found */
      aaRightReal[sb] = fMult(transRatio, rR0);
      aaRightImag[sb] = fMult(transRatio, iR0);

    } /* sb */
  } /* gr */

  /************************/
  /* ICC groups : 20,  21 */
  /************************/


  /* gr = ICC groups */
  for (gr=DELAY_GROUP_OFFSET; gr < NO_IID_GROUPS; gr++) {

    INT sbStart = groupBorders20[gr];
    INT sbStop  = groupBorders20[gr+1];

    UCHAR *pDelayBufIdx = &h_ps_d->specificTo.mpeg.aDelayBufIndexDelayQmf[sbStart-FIRST_DELAY_SB];

    transRatio = aaTransRatioSlot[bins2groupMap20[gr]];

    /* sb = subQMF/QMF subband */
    for (sb = sbStart; sb < sbStop; sb++) {

      /* Update delay buffers */
      rR0 = h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[*pDelayBufIdx][sb-FIRST_DELAY_SB];
      iR0 = h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[*pDelayBufIdx][sb-FIRST_DELAY_SB];

      h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[*pDelayBufIdx][sb-FIRST_DELAY_SB] = aaLeftReal[sb];
      h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[*pDelayBufIdx][sb-FIRST_DELAY_SB] = aaLeftImag[sb];

      /* duck if a past transient is found */
      aaRightReal[sb] = fMult(transRatio, rR0);
      aaRightImag[sb] = fMult(transRatio, iR0);

      if (++(*pDelayBufIdx) >= delayIndexQmf[sb]) {
        *pDelayBufIdx = 0;
      }
      pDelayBufIdx++;

    } /* sb */
  } /* gr */


  /* Update delay buffer index */
  if (++h_ps_d->specificTo.mpeg.delayBufIndex >= NO_SAMPLE_DELAY_ALLPASS)
    h_ps_d->specificTo.mpeg.delayBufIndex = 0;

  for (m=0; m<NO_SERIAL_ALLPASS_LINKS ; m++) {
    if (++h_ps_d->specificTo.mpeg.aDelayRBufIndexSer[m] >= aAllpassLinkDelaySer[m])
      h_ps_d->specificTo.mpeg.aDelayRBufIndexSer[m] = 0;
  }


  scaleValues( &rIntBufferLeft[NO_QMF_BANDS_HYBRID20],  NO_QMF_CHANNELS-NO_QMF_BANDS_HYBRID20, -SCAL_HEADROOM );
  scaleValues( &iIntBufferLeft[NO_QMF_BANDS_HYBRID20],  NO_QMF_CHANNELS-NO_QMF_BANDS_HYBRID20, -SCAL_HEADROOM );
  scaleValues( &rIntBufferRight[NO_QMF_BANDS_HYBRID20], NO_QMF_CHANNELS-NO_QMF_BANDS_HYBRID20, -SCAL_HEADROOM );
  scaleValues( &iIntBufferRight[NO_QMF_BANDS_HYBRID20], NO_QMF_CHANNELS-NO_QMF_BANDS_HYBRID20, -SCAL_HEADROOM );

  /* free memory on scratch */
  C_ALLOC_SCRATCH_END(aaTransRatioSlot, FIXP_DBL, NO_MID_RES_BINS);
  C_ALLOC_SCRATCH_END(aaPowerSlot, FIXP_DBL, NO_MID_RES_BINS);
}


void initSlotBasedRotation( HANDLE_PS_DEC h_ps_d, /*!< pointer to the module state */
                            int env,
                            int usb
                            ) {

  INT     group = 0;
  INT     bin =  0;
  INT     noIidSteps;

/*  const UCHAR *pQuantizedIIDs;*/

  FIXP_SGL  invL;
  FIXP_DBL  ScaleL, ScaleR;
  FIXP_DBL  Alpha, Beta;
  FIXP_DBL  h11r, h12r, h21r, h22r;

  const FIXP_DBL  *PScaleFactors;

  /* Overwrite old values in delay buffers when upper subband is higher than in last frame */
  if (env == 0) {

    if ((usb > h_ps_d->specificTo.mpeg.lastUsb) && h_ps_d->specificTo.mpeg.lastUsb) {

      INT i,k,length;

      for (i=h_ps_d->specificTo.mpeg.lastUsb ; i < FIRST_DELAY_SB; i++) {
        FDKmemclear(h_ps_d->specificTo.mpeg.aaaRealDelayRBufferSerQmf[i], NO_DELAY_LENGTH_VECTORS*sizeof(FIXP_DBL));
        FDKmemclear(h_ps_d->specificTo.mpeg.aaaImagDelayRBufferSerQmf[i], NO_DELAY_LENGTH_VECTORS*sizeof(FIXP_DBL));
      }

      for (k=0 ; k<NO_SAMPLE_DELAY_ALLPASS; k++) {
        FDKmemclear(h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[k], FIRST_DELAY_SB*sizeof(FIXP_DBL));
      }
      length = (usb-FIRST_DELAY_SB)*sizeof(FIXP_DBL);
      if(length>0) {
        FDKmemclear(h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[0], length);
        FDKmemclear(h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[0], length);
      }
      length = (fixMin(NO_DELAY_BUFFER_BANDS,(INT)usb)-FIRST_DELAY_SB)*sizeof(FIXP_DBL);
      if(length>0) {
        for (k=1 ; k < h_ps_d->specificTo.mpeg.noSampleDelay; k++) {
          FDKmemclear(h_ps_d->specificTo.mpeg.pAaRealDelayBufferQmf[k], length);
          FDKmemclear(h_ps_d->specificTo.mpeg.pAaImagDelayBufferQmf[k], length);
        }
      }
    }
    h_ps_d->specificTo.mpeg.lastUsb = usb;
  } /* env == 0 */

  if (h_ps_d->bsData[h_ps_d->processSlot].mpeg.bFineIidQ)
  {
    PScaleFactors = ScaleFactorsFine; /* values are shiftet right by one */
    noIidSteps = NO_IID_STEPS_FINE;
    /*pQuantizedIIDs = quantizedIIDsFine;*/
  }

  else
  {
    PScaleFactors = ScaleFactors; /* values are shiftet right by one */
    noIidSteps = NO_IID_STEPS;
    /*pQuantizedIIDs = quantizedIIDs;*/
  }


  /* dequantize and decode */
  for ( group = 0; group < NO_IID_GROUPS; group++ ) {

    bin = bins2groupMap20[group];

    /*!
    <h3> type 'A' rotation </h3>
    mixing procedure R_a, used in baseline version<br>

     Scale-factor vectors c1 and c2 are precalculated in initPsTables () and stored in
     scaleFactors[] and scaleFactorsFine[] = pScaleFactors [].
     From the linearized IID parameters (intensity differences), two scale factors are
     calculated. They are used to obtain the coefficients h11... h22.
    */

    /* ScaleR and ScaleL are scaled by 1 shift right */

    ScaleR = PScaleFactors[noIidSteps + h_ps_d->specificTo.mpeg.coef.aaIidIndexMapped[env][bin]];
    ScaleL = PScaleFactors[noIidSteps - h_ps_d->specificTo.mpeg.coef.aaIidIndexMapped[env][bin]];

    Beta   = fMult (fMult( Alphas[h_ps_d->specificTo.mpeg.coef.aaIccIndexMapped[env][bin]], ( ScaleR - ScaleL )), FIXP_SQRT05);
    Alpha  = Alphas[h_ps_d->specificTo.mpeg.coef.aaIccIndexMapped[env][bin]]>>1;

    /* Alpha and Beta are now both scaled by 2 shifts right */

    /* calculate the coefficients h11... h22 from scale-factors and ICC parameters */

    /* h values are scaled by 1 shift right */
    {
      FIXP_DBL trigData[4];

      inline_fixp_cos_sin(Beta + Alpha, Beta - Alpha, 2, trigData);
      h11r = fMult( ScaleL, trigData[0]);
      h12r = fMult( ScaleR, trigData[2]);
      h21r = fMult( ScaleL, trigData[1]);
      h22r = fMult( ScaleR, trigData[3]);
    }
    /*****************************************************************************************/
    /* Interpolation of the matrices H11... H22:                                             */
    /*                                                                                       */
    /* H11(k,n) = H11(k,n[e]) + (n-n[e]) * (H11(k,n[e+1] - H11(k,n[e])) / (n[e+1] - n[e])    */
    /* ...                                                                                   */
    /*****************************************************************************************/

    /* invL = 1/(length of envelope) */
    invL = FX_DBL2FX_SGL(GetInvInt(h_ps_d->bsData[h_ps_d->processSlot].mpeg.aEnvStartStop[env + 1] - h_ps_d->bsData[h_ps_d->processSlot].mpeg.aEnvStartStop[env]));

    h_ps_d->specificTo.mpeg.coef.H11r[group]  = h_ps_d->specificTo.mpeg.h11rPrev[group];
    h_ps_d->specificTo.mpeg.coef.H12r[group]  = h_ps_d->specificTo.mpeg.h12rPrev[group];
    h_ps_d->specificTo.mpeg.coef.H21r[group]  = h_ps_d->specificTo.mpeg.h21rPrev[group];
    h_ps_d->specificTo.mpeg.coef.H22r[group]  = h_ps_d->specificTo.mpeg.h22rPrev[group];

    h_ps_d->specificTo.mpeg.coef.DeltaH11r[group]  = fMult ( h11r - h_ps_d->specificTo.mpeg.coef.H11r[group], invL );
    h_ps_d->specificTo.mpeg.coef.DeltaH12r[group]  = fMult ( h12r - h_ps_d->specificTo.mpeg.coef.H12r[group], invL );
    h_ps_d->specificTo.mpeg.coef.DeltaH21r[group]  = fMult ( h21r - h_ps_d->specificTo.mpeg.coef.H21r[group], invL );
    h_ps_d->specificTo.mpeg.coef.DeltaH22r[group]  = fMult ( h22r - h_ps_d->specificTo.mpeg.coef.H22r[group], invL );

    /* update prev coefficients for interpolation in next envelope */

    h_ps_d->specificTo.mpeg.h11rPrev[group] = h11r;
    h_ps_d->specificTo.mpeg.h12rPrev[group] = h12r;
    h_ps_d->specificTo.mpeg.h21rPrev[group] = h21r;
    h_ps_d->specificTo.mpeg.h22rPrev[group] = h22r;

  } /* group loop */
}


static void applySlotBasedRotation( HANDLE_PS_DEC h_ps_d,        /*!< pointer to the module state */

                                    FIXP_DBL  *mHybridRealLeft,  /*!< hybrid values real left  */
                                    FIXP_DBL  *mHybridImagLeft,  /*!< hybrid values imag left  */

                                    FIXP_DBL  *QmfLeftReal,      /*!< real bands left qmf channel */
                                    FIXP_DBL  *QmfLeftImag,      /*!< imag bands left qmf channel */

                                    FIXP_DBL  *mHybridRealRight, /*!< hybrid values real right  */
                                    FIXP_DBL  *mHybridImagRight, /*!< hybrid values imag right  */

                                    FIXP_DBL  *QmfRightReal,     /*!< real bands right qmf channel */
                                    FIXP_DBL  *QmfRightImag      /*!< imag bands right qmf channel */
                                   )
{
  INT     group;
  INT     subband;

  FIXP_DBL *RESTRICT HybrLeftReal;
  FIXP_DBL *RESTRICT HybrLeftImag;
  FIXP_DBL *RESTRICT HybrRightReal;
  FIXP_DBL *RESTRICT HybrRightImag;

  FIXP_DBL tmpLeft, tmpRight;


  /**********************************************************************************************/
  /*!
  <h2> Mapping </h2>

  The number of stereo bands that is actually used depends on the number of availble
  parameters for IID and ICC:
  <pre>
   nr. of IID para.| nr. of ICC para. | nr. of Stereo bands
   ----------------|------------------|-------------------
     10,20         |     10,20        |        20
     10,20         |     34           |        34
     34            |     10,20        |        34
     34            |     34           |        34
  </pre>
  In the case the number of parameters for IIS and ICC differs from the number of stereo
  bands, a mapping from the lower number to the higher number of parameters is applied.
  Index mapping of IID and ICC parameters is already done in psbitdec.cpp. Further mapping is
  not needed here in baseline version.
  **********************************************************************************************/

  /************************************************************************************************/
  /*!
  <h2> Mixing </h2>

  To generate the QMF subband signals for the subband samples n = n[e]+1 ,,, n_[e+1] the
  parameters at position n[e] and n[e+1] are required as well as the subband domain signals
  s_k(n) and d_k(n) for n = n[e]+1... n_[e+1]. n[e] represents the start position for
  envelope e. The border positions n[e] are handled in DecodePS().

  The stereo sub subband signals are constructed as:
  <pre>
  l_k(n) = H11(k,n) s_k(n) + H21(k,n) d_k(n)
  r_k(n) = H21(k,n) s_k(n) + H22(k,n) d_k(n)
  </pre>
  In order to obtain the matrices H11(k,n)... H22 (k,n), the vectors h11(b)... h22(b) need to
  be calculated first (b: parameter index). Depending on ICC mode either mixing procedure R_a
  or R_b is used for that. For both procedures, the parameters for parameter position n[e+1]
  is used.
  ************************************************************************************************/


  /************************************************************************************************/
  /*!
  <h2>Phase parameters </h2>
  With disabled phase parameters (which is the case in baseline version), the H-matrices are
  just calculated by:

  <pre>
  H11(k,n[e+1] = h11(b(k))
  (...)
  b(k): parameter index according to mapping table
  </pre>

  <h2>Processing of the samples in the sub subbands </h2>
  this loop includes the interpolation of the coefficients Hxx
  ************************************************************************************************/


  /* loop thru all groups ... */
  HybrLeftReal  = mHybridRealLeft;
  HybrLeftImag  = mHybridImagLeft;
  HybrRightReal = mHybridRealRight;
  HybrRightImag = mHybridImagRight;

  /******************************************************/
  /* construct stereo sub subband signals according to: */
  /*                                                    */
  /* l_k(n) = H11(k,n) s_k(n) + H21(k,n) d_k(n)         */
  /* r_k(n) = H12(k,n) s_k(n) + H22(k,n) d_k(n)         */
  /******************************************************/
  for ( group = 0; group < SUBQMF_GROUPS; group++ ) {

    h_ps_d->specificTo.mpeg.coef.H11r[group] += h_ps_d->specificTo.mpeg.coef.DeltaH11r[group];
    h_ps_d->specificTo.mpeg.coef.H12r[group] += h_ps_d->specificTo.mpeg.coef.DeltaH12r[group];
    h_ps_d->specificTo.mpeg.coef.H21r[group] += h_ps_d->specificTo.mpeg.coef.DeltaH21r[group];
    h_ps_d->specificTo.mpeg.coef.H22r[group] += h_ps_d->specificTo.mpeg.coef.DeltaH22r[group];

    subband = groupBorders20[group];

    tmpLeft  = fMultAddDiv2( fMultDiv2(h_ps_d->specificTo.mpeg.coef.H11r[group], HybrLeftReal[subband]), h_ps_d->specificTo.mpeg.coef.H21r[group], HybrRightReal[subband]);
    tmpRight = fMultAddDiv2( fMultDiv2(h_ps_d->specificTo.mpeg.coef.H12r[group], HybrLeftReal[subband]), h_ps_d->specificTo.mpeg.coef.H22r[group], HybrRightReal[subband]);
    HybrLeftReal [subband] = tmpLeft<<1;
    HybrRightReal[subband] = tmpRight<<1;

    tmpLeft  = fMultAdd( fMultDiv2(h_ps_d->specificTo.mpeg.coef.H11r[group], HybrLeftImag[subband]), h_ps_d->specificTo.mpeg.coef.H21r[group], HybrRightImag[subband]);
    tmpRight = fMultAdd( fMultDiv2(h_ps_d->specificTo.mpeg.coef.H12r[group], HybrLeftImag[subband]), h_ps_d->specificTo.mpeg.coef.H22r[group], HybrRightImag[subband]);
    HybrLeftImag [subband] = tmpLeft;
    HybrRightImag[subband] = tmpRight;
  }

  /* continue in the qmf buffers */
  HybrLeftReal  = QmfLeftReal;
  HybrLeftImag  = QmfLeftImag;
  HybrRightReal = QmfRightReal;
  HybrRightImag = QmfRightImag;

  for (; group < NO_IID_GROUPS; group++ ) {

    h_ps_d->specificTo.mpeg.coef.H11r[group] += h_ps_d->specificTo.mpeg.coef.DeltaH11r[group];
    h_ps_d->specificTo.mpeg.coef.H12r[group] += h_ps_d->specificTo.mpeg.coef.DeltaH12r[group];
    h_ps_d->specificTo.mpeg.coef.H21r[group] += h_ps_d->specificTo.mpeg.coef.DeltaH21r[group];
    h_ps_d->specificTo.mpeg.coef.H22r[group] += h_ps_d->specificTo.mpeg.coef.DeltaH22r[group];

    for ( subband = groupBorders20[group]; subband < groupBorders20[group + 1]; subband++ )
    {
      tmpLeft  = fMultAdd( fMultDiv2(h_ps_d->specificTo.mpeg.coef.H11r[group], HybrLeftReal[subband]), h_ps_d->specificTo.mpeg.coef.H21r[group], HybrRightReal[subband]);
      tmpRight = fMultAdd( fMultDiv2(h_ps_d->specificTo.mpeg.coef.H12r[group], HybrLeftReal[subband]), h_ps_d->specificTo.mpeg.coef.H22r[group], HybrRightReal[subband]);
      HybrLeftReal [subband] = tmpLeft;
      HybrRightReal[subband] = tmpRight;

      tmpLeft  = fMultAdd( fMultDiv2(h_ps_d->specificTo.mpeg.coef.H11r[group], HybrLeftImag[subband]), h_ps_d->specificTo.mpeg.coef.H21r[group], HybrRightImag[subband]);
      tmpRight = fMultAdd( fMultDiv2(h_ps_d->specificTo.mpeg.coef.H12r[group], HybrLeftImag[subband]), h_ps_d->specificTo.mpeg.coef.H22r[group], HybrRightImag[subband]);
      HybrLeftImag [subband] = tmpLeft;
      HybrRightImag[subband] = tmpRight;

    } /* subband */
  }
}


/***************************************************************************/
/*!
  \brief  Applies IID, ICC, IPD and OPD parameters to the current frame.

  \return none

****************************************************************************/
void
ApplyPsSlot( HANDLE_PS_DEC h_ps_d,         /*!< handle PS_DEC*/
             FIXP_DBL  **rIntBufferLeft,   /*!< real bands left qmf channel (38x64)  */
             FIXP_DBL  **iIntBufferLeft,   /*!< imag bands left qmf channel (38x64)  */
             FIXP_DBL  *rIntBufferRight,   /*!< real bands right qmf channel (38x64) */
             FIXP_DBL  *iIntBufferRight    /*!< imag bands right qmf channel (38x64) */
           )
{

  /*!
  The 64-band QMF representation of the monaural signal generated by the SBR tool
  is used as input of the PS tool. After the PS processing, the outputs of the left
  and right hybrid synthesis filterbanks are used to generate the stereo output
  signal.

  <pre>

             -------------            ----------            -------------
            | Hybrid      | M_n[k,m] |          | L_n[k,m] | Hybrid      | l[n]
   m[n] --->| analysis    |--------->|          |--------->| synthesis   |----->
            | filter bank |          |          |          | filter bank |
             -------------           | Stereo   |           -------------
                   |                 | recon-   |
                   |                 | stuction |
                  \|/                |          |
             -------------           |          |
            | De-         | D_n[k,m] |          |
            | correlation |--------->|          |
             -------------           |          |           -------------
                                     |          | R_n[k,m] | Hybrid      | r[n]
                                     |          |--------->| synthesis   |----->
   IID, ICC ------------------------>|          |          | filter bank |
  (IPD, OPD)                          ----------            -------------

  m[n]:      QMF represantation of the mono input
  M_n[k,m]:  (sub-)sub-band domain signals of the mono input
  D_n[k,m]:  decorrelated (sub-)sub-band domain signals
  L_n[k,m]:  (sub-)sub-band domain signals of the left output
  R_n[k,m]:  (sub-)sub-band domain signals of the right output
  l[n],r[n]: left/right output signals

  </pre>
  */

  /* get temporary hybrid qmf values of one timeslot */
  C_ALLOC_SCRATCH_START(hybridRealLeft, FIXP_DBL, NO_SUB_QMF_CHANNELS);
  C_ALLOC_SCRATCH_START(hybridImagLeft, FIXP_DBL, NO_SUB_QMF_CHANNELS);
  C_ALLOC_SCRATCH_START(hybridRealRight, FIXP_DBL, NO_SUB_QMF_CHANNELS);
  C_ALLOC_SCRATCH_START(hybridImagRight, FIXP_DBL, NO_SUB_QMF_CHANNELS);

  SCHAR sf_IntBuffer     = h_ps_d->sf_IntBuffer;

  /* clear workbuffer */
  FDKmemclear(hybridRealLeft,  NO_SUB_QMF_CHANNELS*sizeof(FIXP_DBL));
  FDKmemclear(hybridImagLeft,  NO_SUB_QMF_CHANNELS*sizeof(FIXP_DBL));
  FDKmemclear(hybridRealRight, NO_SUB_QMF_CHANNELS*sizeof(FIXP_DBL));
  FDKmemclear(hybridImagRight, NO_SUB_QMF_CHANNELS*sizeof(FIXP_DBL));


  /*!
  Hybrid analysis filterbank:
  The lower 3 (5) of the 64 QMF subbands are further split to provide better frequency resolution.
  for PS processing.
  For the 10 and 20 stereo bands configuration, the QMF band H_0(w) is split
  up into 8 (sub-) sub-bands and the QMF bands H_1(w) and H_2(w) are spit into 2 (sub-)
  4th. (See figures 8.20 and 8.22 of ISO/IEC 14496-3:2001/FDAM 2:2004(E) )
  */


  if (h_ps_d->procFrameBased == 1)    /* If we have switched from frame to slot based processing  */
  {                                   /* fill hybrid delay buffer.                                */
    h_ps_d->procFrameBased = 0;

    fillHybridDelayLine( rIntBufferLeft,
                         iIntBufferLeft,
                         hybridRealLeft,
                         hybridImagLeft,
                         hybridRealRight,
                         hybridImagRight,
                        &h_ps_d->specificTo.mpeg.hybrid );
  }

  slotBasedHybridAnalysis ( rIntBufferLeft[HYBRID_FILTER_DELAY], /* qmf filterbank values                         */
                            iIntBufferLeft[HYBRID_FILTER_DELAY], /* qmf filterbank values                         */
                            hybridRealLeft,                      /* hybrid filterbank values                      */
                            hybridImagLeft,                      /* hybrid filterbank values                      */
                           &h_ps_d->specificTo.mpeg.hybrid);          /* hybrid filterbank handle                      */


  SCHAR hybridScal = h_ps_d->specificTo.mpeg.hybrid.sf_mQmfBuffer;


  /*!
  Decorrelation:
  By means of all-pass filtering and delaying, the (sub-)sub-band samples s_k(n) are
  converted into de-correlated (sub-)sub-band samples d_k(n).
  - k: frequency in hybrid spectrum
  - n: time index
  */

  deCorrelateSlotBased( h_ps_d,              /* parametric stereo decoder handle       */
                        hybridRealLeft,      /* left hybrid time slot                  */
                        hybridImagLeft,
                        hybridScal,      /* scale factor of left hybrid time slot  */
                        rIntBufferLeft[0],   /* left qmf time slot                     */
                        iIntBufferLeft[0],
                        sf_IntBuffer,        /* scale factor of left and right qmf time slot */
                        hybridRealRight,     /* right hybrid time slot                 */
                        hybridImagRight,
                        rIntBufferRight,     /* right qmf time slot                    */
                        iIntBufferRight );



  /*!
  Stereo Processing:
  The sets of (sub-)sub-band samples s_k(n) and d_k(n) are processed according to
  the stereo cues which are defined per stereo band.
  */


  applySlotBasedRotation( h_ps_d,            /* parametric stereo decoder handle       */
                          hybridRealLeft,    /* left hybrid time slot                  */
                          hybridImagLeft,
                          rIntBufferLeft[0], /* left qmf time slot                     */
                          iIntBufferLeft[0],
                          hybridRealRight,   /* right hybrid time slot                 */
                          hybridImagRight,
                          rIntBufferRight,   /* right qmf time slot                    */
                          iIntBufferRight );




  /*!
  Hybrid synthesis filterbank:
  The stereo processed hybrid subband signals l_k(n) and r_k(n) are fed into the hybrid synthesis
  filterbanks which are identical to the 64 complex synthesis filterbank of the SBR tool. The
  input to the filterbank are slots of 64 QMF samples. For each slot the filterbank outputs one
  block of 64 samples of one reconstructed stereo channel. The hybrid synthesis filterbank is
  computed seperatly for the left and right channel.
  */


  /* left channel */
  slotBasedHybridSynthesis ( hybridRealLeft,         /* one timeslot of hybrid filterbank values */
                             hybridImagLeft,
                             rIntBufferLeft[0],      /* one timeslot of qmf filterbank values    */
                             iIntBufferLeft[0],
                            &h_ps_d->specificTo.mpeg.hybrid );      /* hybrid filterbank handle                 */

  /* right channel */
  slotBasedHybridSynthesis ( hybridRealRight,        /* one timeslot of hybrid filterbank values */
                             hybridImagRight,
                             rIntBufferRight,        /* one timeslot of qmf filterbank values    */
                             iIntBufferRight,
                            &h_ps_d->specificTo.mpeg.hybrid );      /* hybrid filterbank handle                 */







  /* free temporary hybrid qmf values of one timeslot */
  C_ALLOC_SCRATCH_END(hybridImagRight, FIXP_DBL, NO_SUB_QMF_CHANNELS);
  C_ALLOC_SCRATCH_END(hybridRealRight, FIXP_DBL, NO_SUB_QMF_CHANNELS);
  C_ALLOC_SCRATCH_END(hybridImagLeft, FIXP_DBL, NO_SUB_QMF_CHANNELS);
  C_ALLOC_SCRATCH_END(hybridRealLeft, FIXP_DBL, NO_SUB_QMF_CHANNELS);

}/* END ApplyPsSlot */


/***************************************************************************/
/*!

  \brief  assigns timeslots to an array

  \return

****************************************************************************/

static void assignTimeSlotsPS (FIXP_DBL *bufAdr,
                               FIXP_DBL **bufPtr,
                               const int numSlots,
                               const int numChan)
{
  FIXP_DBL  *ptr;
  int slot;
  ptr = bufAdr;
  for(slot=0; slot < numSlots; slot++) {
   bufPtr [slot] = ptr;
    ptr += numChan;
  }
}

