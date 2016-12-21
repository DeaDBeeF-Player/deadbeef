
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

/***************************  Fraunhofer IIS FDK Tools  ***********************

   Author(s):   Oliver Moser
   Description: ROM tables used by FDK tools

******************************************************************************/

#ifndef __FDK_TOOLS_ROM_H__
#define __FDK_TOOLS_ROM_H__

#include "common_fix.h"
#include "FDK_audio.h"


/* None radix2 rotation vectors */
extern const FIXP_STB RotVectorReal60[60];
extern const FIXP_STB RotVectorImag60[60];
extern const FIXP_STB RotVectorReal240[240];
extern const FIXP_STB RotVectorImag240[240];
extern const FIXP_STB RotVectorReal480[480];
extern const FIXP_STB RotVectorImag480[480];


/* Regular sine tables */
extern const FIXP_STP SineTable512[];
extern const FIXP_STP SineTable480[];

/* AAC-LC windows */
extern const FIXP_WTP SineWindow1024[];
extern const FIXP_WTP KBDWindow1024[];
extern const FIXP_WTP SineWindow128[];
extern const FIXP_WTP KBDWindow128[];

extern const FIXP_WTP SineWindow960[];
extern const FIXP_WTP KBDWindow960[];
extern const FIXP_WTP SineWindow120[];
extern const FIXP_WTP KBDWindow120[];

/* AAC-LD windows */
extern const FIXP_WTP SineWindow512[];
#define LowOverlapWindow512 SineWindow128
extern const FIXP_WTP SineWindow480[];
#define LowOverlapWindow480 SineWindow120



extern const FIXP_WTP SineWindow64[];
extern const FIXP_WTP SineWindow32[];

/**
 * \brief Helper table for window slope mapping. You should prefer the usage of the
 * function FDKgetWindowSlope(), this table is only made public for some optimized
 * access inside dct.cpp.
 */
extern const FIXP_WTP *const windowSlopes[2][3][9];

/**
 * \brief Window slope access helper. Obtain a window of given length and shape.
 * \param length Length of the window slope.
 * \param shape Shape index of the window slope. 0: sine window, 1: Kaiser-Bessel. Any other
 *              value is applied a mask of 1 to, mapping it to either 0 or 1.
 * \param Pointer to window slope or NULL if the requested window slope is not available.
 */
const FIXP_WTP * FDKgetWindowSlope(int length, int shape);

extern const FIXP_WTP sin_twiddle_L64[];

/*
 * Filter coefficient type definition
 */

#if defined(ARCH_PREFER_MULT_16x16) || defined(ARCH_PREFER_MULT_32x16)
  #define QMF_COEFF_16BIT
#endif

#define QMF_FILTER_PROTOTYPE_SIZE    640
#define QMF_NO_POLY                  5

#ifdef QMF_COEFF_16BIT
  #define FIXP_PFT FIXP_SGL
  #define FIXP_QTW FIXP_SGL
#else
  #define FIXP_PFT FIXP_DBL
  #define FIXP_QTW FIXP_DBL
#endif

#define QMF640_PFT_TABLE_SIZE       (640/2 + QMF_NO_POLY)

extern const FIXP_QTW qmf_phaseshift_cos32[32];
extern const FIXP_QTW qmf_phaseshift_sin32[32];
/* Adapted analysis post-twiddles for down-sampled HQ SBR */
extern const FIXP_QTW qmf_phaseshift_cos_downsamp32[32];
extern const FIXP_QTW qmf_phaseshift_sin_downsamp32[32];
extern const FIXP_QTW qmf_phaseshift_cos64[64];
extern const FIXP_QTW qmf_phaseshift_sin64[64];

extern const FIXP_PFT qmf_64[QMF640_PFT_TABLE_SIZE+QMF_NO_POLY];





#define QMF640_CLDFB_PFT_TABLE_SIZE (640)
#define QMF320_CLDFB_PFT_TABLE_SIZE (320)
#define QMF_CLDFB_PFT_SCALE 1

extern const FIXP_QTW qmf_phaseshift_cos32_cldfb[32];
extern const FIXP_QTW qmf_phaseshift_sin32_cldfb[32];
extern const FIXP_QTW qmf_phaseshift_cos64_cldfb[64];
extern const FIXP_QTW qmf_phaseshift_sin64_cldfb[64];

extern const FIXP_PFT qmf_cldfb_640[QMF640_CLDFB_PFT_TABLE_SIZE];
extern const FIXP_PFT qmf_cldfb_320[QMF320_CLDFB_PFT_TABLE_SIZE];





/*
 * Raw Data Block list stuff.
 */
typedef enum {
  element_instance_tag,
  common_window,
  global_gain,
  ics_info, /* ics_reserved_bit, window_sequence, window_shape, max_sfb, scale_factor_grouping, predictor_data_present, ltp_data_present, ltp_data */
  max_sfb,
  ms, /* ms_mask_present, ms_used */
  /*predictor_data_present,*/ /* part of ics_info */
  ltp_data_present,
  ltp_data,
  section_data,
  scale_factor_data,
  pulse, /* pulse_data_present, pulse_data  */
  tns_data_present,
  tns_data,
  gain_control_data_present,
  gain_control_data,
  esc1_hcr,
  esc2_rvlc,
  spectral_data,

  scale_factor_data_usac,
  core_mode,
  common_tw,
  lpd_channel_stream,
  tw_data,
  noise,
  ac_spectral_data,
  fac_data,
  tns_active, /* introduced in MPEG-D usac CD */
  tns_data_present_usac,
  common_max_sfb,


  /* Non data list items */
  adtscrc_start_reg1,
  adtscrc_start_reg2,
  adtscrc_end_reg1,
  adtscrc_end_reg2,
  drmcrc_start_reg,
  drmcrc_end_reg,
  next_channel,
  next_channel_loop,
  link_sequence,
  end_of_sequence
} rbd_id_t;

struct element_list {
  const rbd_id_t *id;
  const struct element_list *next[2];
};

typedef struct element_list element_list_t;
/**
 * \brief get elementary stream pieces list for given parameters.
 * \param aot audio object type
 * \param epConfig the epConfig value from the current Audio Specific Config
 * \param nChannels amount of channels contained in the current element.
 * \param layer the layer of the current element.
 * \return element_list_t parser guidance structure.
 */
const element_list_t * getBitstreamElementList(AUDIO_OBJECT_TYPE aot, SCHAR epConfig, UCHAR nChannels, UCHAR layer);


#endif

