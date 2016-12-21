
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

/********************************  Fraunhofer IIS  ***************************

   Author(s):   Arthur Tritthart
   Description: (ARM optimised) SBR domain coding

******************************************************************************/
#ifndef INCLUSION_GUARD_CALC_ENV_ARM
#define INCLUSION_GUARD_CALC_ENV_ARM


/*!
  \brief   Compute maximal value of a complex array (re/im) of a given width
           Negative values are temporarily logically or'ed with 0xFFFFFFFF
           instead of negating the value, if the sign bit is set.
  \param   maxVal  Preset maximal value
  \param   reTmp   real input signal
  \param   imTmp   imaginary input signal
  \return  new maximal value
*/

#ifdef FUNCTION_FDK_get_maxval
__asm FIXP_DBL FDK_get_maxval (FIXP_DBL maxVal, FIXP_DBL *reTmp, FIXP_DBL *imTmp, int width )
{

  /* Register map:
     r0    maxVal
     r1    reTmp
     r2    imTmp
     r3    width
     r4    real
     r5    imag
  */
    PUSH    {r4-r5}

    MOVS    r3, r3, ASR #1
    ADC     r3, r3, #0
    BCS     FDK_get_maxval_loop_2nd_part
    BEQ     FDK_get_maxval_loop_end

FDK_get_maxval_loop
    LDR     r4, [r1], #4
    LDR     r5, [r2], #4
    EOR     r4, r4, r4, ASR #31
    EOR     r5, r5, r5, ASR #31
    ORR     r0, r0, r4
    ORR     r0, r0, r5

FDK_get_maxval_loop_2nd_part
    LDR     r4, [r1], #4
    LDR     r5, [r2], #4
    EOR     r4, r4, r4, ASR #31
    EOR     r5, r5, r5, ASR #31
    ORR     r0, r0, r4
    ORR     r0, r0, r5

    SUBS    r3, r3, #1
    BNE     FDK_get_maxval_loop

FDK_get_maxval_loop_end
    POP     {r4-r5}
    BX      lr
}
#endif /* FUNCTION_FDK_get_maxval */

#endif /* INCLUSION_GUARD_CALC_ENV_ARM */
