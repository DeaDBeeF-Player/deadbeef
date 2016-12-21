
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

/************************  FDK PCM postprocessor module  *********************

   Author(s):   Matthias Neusinger
   Description: Hard limiter for clipping prevention

*******************************************************************************/

#ifndef _LIMITER_H_
#define _LIMITER_H_


#include "common_fix.h"

#define TDL_ATTACK_DEFAULT_MS      (15)              /* default attack  time in ms */
#define TDL_RELEASE_DEFAULT_MS     (50)              /* default release time in ms */

#define TDL_GAIN_SCALING           (15)              /* scaling of gain value. */


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
  TDLIMIT_OK = 0,

  __error_codes_start = -100,

  TDLIMIT_INVALID_HANDLE,
  TDLIMIT_INVALID_PARAMETER,

  __error_codes_end
} TDLIMITER_ERROR;

struct TDLimiter;
typedef struct TDLimiter* TDLimiterPtr;

/******************************************************************************
* createLimiter                                                               *
* maxAttackMs:   maximum and initial attack/lookahead time in milliseconds    *
* releaseMs:     release time in milliseconds (90% time constant)             *
* threshold:     limiting threshold                                           *
* maxChannels:   maximum and initial number of channels                       *
* maxSampleRate: maximum and initial sampling rate in Hz                      *
* returns:       limiter handle                                               *
******************************************************************************/
TDLimiterPtr createLimiter(unsigned int  maxAttackMs,
                           unsigned int  releaseMs,
                           INT_PCM       threshold,
                           unsigned int  maxChannels,
                           unsigned int  maxSampleRate);


/******************************************************************************
* resetLimiter                                                                *
* limiter: limiter handle                                                     *
* returns: error code                                                         *
******************************************************************************/
TDLIMITER_ERROR resetLimiter(TDLimiterPtr limiter);


/******************************************************************************
* destroyLimiter                                                              *
* limiter: limiter handle                                                     *
* returns: error code                                                         *
******************************************************************************/
TDLIMITER_ERROR destroyLimiter(TDLimiterPtr limiter);

/******************************************************************************
* applyLimiter                                                                *
* limiter:    limiter handle                                                  *
* pGain :     pointer to gains to be applied to the signal before limiting,   *
*             which are downscaled by TDL_GAIN_SCALING bit.                   *
*             These gains are delayed by gain_delay, and smoothed.            *
*             Smoothing is done by a butterworth lowpass filter with a cutoff *
*             frequency which is fixed with respect to the sampling rate.     *
*             It is a substitute for the smoothing due to windowing and       *
*             overlap/add, if a gain is applied in frequency domain.          *
* gain_scale: pointer to scaling exponents to be applied to the signal before *
*             limiting, without delay and without smoothing                   *
* gain_size:  number of elements in pGain, currently restricted to 1          *
* gain_delay: delay [samples] with which the gains in pGain shall be applied  *
*             gain_delay <= nSamples                                          *
* samples:    input/output buffer containing interleaved samples              *
*             precision of output will be DFRACT_BITS-TDL_GAIN_SCALING bits   *
* nSamples:   number of samples per channel                                   *
* returns:    error code                                                      *
******************************************************************************/
TDLIMITER_ERROR applyLimiter(TDLimiterPtr limiter,
                 INT_PCM*    samples,
                 FIXP_DBL*    pGain,
                 const INT*   gain_scale,
                 const UINT   gain_size,
                 const UINT   gain_delay,
                 const UINT   nSamples);

/******************************************************************************
* getLimiterDelay                                                             *
* limiter: limiter handle                                                     *
* returns: exact delay caused by the limiter in samples                       *
******************************************************************************/
unsigned int getLimiterDelay(TDLimiterPtr limiter);

/******************************************************************************
* setLimiterNChannels                                                         *
* limiter:   limiter handle                                                   *
* nChannels: number of channels ( <= maxChannels specified on create)         *
* returns:   error code                                                       *
******************************************************************************/
TDLIMITER_ERROR setLimiterNChannels(TDLimiterPtr limiter, unsigned int nChannels);

/******************************************************************************
* setLimiterSampleRate                                                        *
* limiter:    limiter handle                                                  *
* sampleRate: sampling rate in Hz ( <= maxSampleRate specified on create)     *
* returns:    error code                                                      *
******************************************************************************/
TDLIMITER_ERROR setLimiterSampleRate(TDLimiterPtr limiter, unsigned int sampleRate);

/******************************************************************************
* setLimiterAttack                                                            *
* limiter:    limiter handle                                                  *
* attackMs:   attack time in ms ( <= maxAttackMs specified on create)         *
* returns:    error code                                                      *
******************************************************************************/
TDLIMITER_ERROR setLimiterAttack(TDLimiterPtr limiter, unsigned int attackMs);

/******************************************************************************
* setLimiterRelease                                                           *
* limiter:    limiter handle                                                  *
* releaseMs:  release time in ms                                              *
* returns:    error code                                                      *
******************************************************************************/
TDLIMITER_ERROR setLimiterRelease(TDLimiterPtr limiter, unsigned int releaseMs);

/******************************************************************************
* setLimiterThreshold                                                         *
* limiter:    limiter handle                                                  *
* threshold:  limiter threshold                                               *
* returns:    error code                                                      *
******************************************************************************/
TDLIMITER_ERROR setLimiterThreshold(TDLimiterPtr limiter, INT_PCM threshold);

#ifdef __cplusplus
}
#endif


#endif  //#ifndef _LIMITER_H_
