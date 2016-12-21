
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

#if (QMF_NO_POLY==5)

#define FUNCTION_qmfForwardModulationLP_odd

#ifdef FUNCTION_qmfForwardModulationLP_odd
static void
qmfForwardModulationLP_odd( HANDLE_QMF_FILTER_BANK anaQmf, /*!< Handle of Qmf Analysis Bank  */
                            const FIXP_QMF *timeIn,        /*!< Time Signal */
                            FIXP_QMF *rSubband )           /*!< Real Output */
{
  int i;
  int L = anaQmf->no_channels;
  int M = L>>1;
  int shift = (anaQmf->no_channels>>6) + 1;
  int rSubband_e = 0;

  FIXP_QMF *rSubbandPtr0 = &rSubband[M+0];                /* runs with increment */
  FIXP_QMF *rSubbandPtr1 = &rSubband[M-1];                /* runs with decrement */
  FIXP_QMF *timeIn0 = (FIXP_DBL *) &timeIn[0];            /* runs with increment */
  FIXP_QMF *timeIn1 = (FIXP_DBL *) &timeIn[L];            /* runs with increment */
  FIXP_QMF *timeIn2 = (FIXP_DBL *) &timeIn[L-1];          /* runs with decrement */
  FIXP_QMF *timeIn3 = (FIXP_DBL *) &timeIn[2*L-1];        /* runs with decrement */

  for (i = 0; i < M; i++)
  {
    *rSubbandPtr0++ = (*timeIn2-- >> 1) - (*timeIn0++ >> shift);
    *rSubbandPtr1-- = (*timeIn1++ >> 1) + (*timeIn3-- >> shift);
  }

  dct_IV(rSubband,L, &rSubband_e);
}
#endif /* FUNCTION_qmfForwardModulationLP_odd */


/* NEON optimized QMF currently builts only with RVCT toolchain */

#if defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_5TE__)

#if (SAMPLE_BITS == 16)
#define FUNCTION_qmfAnaPrototypeFirSlot
#endif

#ifdef FUNCTION_qmfAnaPrototypeFirSlot

#if defined(__GNUC__)	/* cppp replaced: elif */

inline INT SMULBB (const SHORT a, const LONG b)
{
  INT result ;
  __asm__ ("smulbb %0, %1, %2"
     : "=r" (result)
     : "r" (a), "r" (b)) ;
  return result ;
}
inline INT SMULBT (const SHORT a, const LONG b)
{
  INT result ;
  __asm__ ("smulbt %0, %1, %2"
     : "=r" (result)
     : "r" (a), "r" (b)) ;
  return result ;
}

inline INT SMLABB(const LONG accu, const SHORT a, const LONG b)
{
  INT result ;
  __asm__ ("smlabb %0, %1, %2,%3"
     : "=r" (result)
     : "r" (a), "r" (b), "r" (accu)) ;
  return result;
}
inline INT SMLABT(const LONG accu, const SHORT a, const LONG b)
{
  INT result ;
  __asm__ ("smlabt %0, %1, %2,%3"
     : "=r" (result)
     : "r" (a), "r" (b), "r" (accu)) ;
  return result;
}
#endif /* compiler selection  */


void qmfAnaPrototypeFirSlot( FIXP_QMF *analysisBuffer,
                             int       no_channels,             /*!< Number channels of analysis filter */
                             const FIXP_PFT *p_filter,
                             int       p_stride,                /*!< Stide of analysis filter    */
                             FIXP_QAS *RESTRICT pFilterStates
                            )
{
  LONG *p_flt = (LONG *) p_filter;
  LONG flt;
  FIXP_QMF *RESTRICT pData_0 = analysisBuffer + 2*no_channels - 1;
  FIXP_QMF *RESTRICT pData_1 = analysisBuffer;

  FIXP_QAS *RESTRICT sta_0 = (FIXP_QAS *)pFilterStates;
  FIXP_QAS *RESTRICT sta_1 = (FIXP_QAS *)pFilterStates + (2*QMF_NO_POLY*no_channels) - 1;

  FIXP_DBL accu0, accu1;
  FIXP_QAS sta0, sta1;

  int staStep1 =  no_channels<<1;
  int staStep2 = (no_channels<<3) - 1; /* Rewind one less */

  if (p_stride == 1)
  {
    /* FIR filter 0 */
    flt = *p_flt++;
    sta1 = *sta_1;  sta_1 -= staStep1;
    accu1 = SMULBB(        sta1, flt);
    sta1 = *sta_1;  sta_1 -= staStep1;
    accu1 = SMLABT( accu1, sta1, flt);

    flt = *p_flt++;
    sta1 = *sta_1;  sta_1 -= staStep1;
    accu1 = SMLABB( accu1, sta1, flt);
    sta1 = *sta_1;  sta_1 -= staStep1;
    accu1 = SMLABT( accu1, sta1, flt);

    flt = *p_flt++;
    sta1 = *sta_1;  sta_1 += staStep2;
    accu1 = SMLABB( accu1, sta1, flt);
    *pData_1++ = FX_DBL2FX_QMF(accu1<<1);

    /* FIR filters 1..63 127..65 or 1..31 63..33 */
    no_channels >>= 1;
    for (; --no_channels; )
    {
      sta0 = *sta_0; sta_0 += staStep1;  /* 1,3,5, ... 29/61 */
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMULBT(        sta0, flt);
      accu1 = SMULBT(        sta1, flt);

      flt = *p_flt++;
      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABB( accu0, sta0, flt);
      accu1 = SMLABB( accu1, sta1, flt);

      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABT( accu0, sta0, flt);
      accu1 = SMLABT( accu1, sta1, flt);

      flt = *p_flt++;
      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABB( accu0, sta0, flt);
      accu1 = SMLABB( accu1, sta1, flt);

      sta0 = *sta_0; sta_0 -= staStep2;
      sta1 = *sta_1; sta_1 += staStep2;
      accu0 = SMLABT( accu0, sta0, flt);
      accu1 = SMLABT( accu1, sta1, flt);

      *pData_0-- = FX_DBL2FX_QMF(accu0<<1);
      *pData_1++ = FX_DBL2FX_QMF(accu1<<1);

      /* Same sequence as above, but mix B=bottom with T=Top */

      flt = *p_flt++;
      sta0 = *sta_0; sta_0 += staStep1;  /* 2,4,6, ... 30/62 */
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMULBB(        sta0, flt);
      accu1 = SMULBB(        sta1, flt);

      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABT( accu0, sta0, flt);
      accu1 = SMLABT( accu1, sta1, flt);

      flt = *p_flt++;
      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABB( accu0, sta0, flt);
      accu1 = SMLABB( accu1, sta1, flt);

      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABT( accu0, sta0, flt);
      accu1 = SMLABT( accu1, sta1, flt);

      flt = *p_flt++;
      sta0 = *sta_0; sta_0 -= staStep2;
      sta1 = *sta_1; sta_1 += staStep2;
      accu0 = SMLABB( accu0, sta0, flt);
      accu1 = SMLABB( accu1, sta1, flt);

      *pData_0-- = FX_DBL2FX_QMF(accu0<<1);
      *pData_1++ = FX_DBL2FX_QMF(accu1<<1);
    }

    /* FIR filter 31/63 and 33/65 */
    sta0 = *sta_0; sta_0 += staStep1;
    sta1 = *sta_1; sta_1 -= staStep1;
    accu0 = SMULBT(        sta0, flt);
    accu1 = SMULBT(        sta1, flt);

    flt = *p_flt++;
    sta0 = *sta_0; sta_0 += staStep1;
    sta1 = *sta_1; sta_1 -= staStep1;
    accu0 = SMLABB( accu0, sta0, flt);
    accu1 = SMLABB( accu1, sta1, flt);

    sta0 = *sta_0; sta_0 += staStep1;
    sta1 = *sta_1; sta_1 -= staStep1;
    accu0 = SMLABT( accu0, sta0, flt);
    accu1 = SMLABT( accu1, sta1, flt);

    flt = *p_flt++;
    sta0 = *sta_0; sta_0 += staStep1;
    sta1 = *sta_1; sta_1 -= staStep1;
    accu0 = SMLABB( accu0, sta0, flt);
    accu1 = SMLABB( accu1, sta1, flt);

    sta0 = *sta_0; sta_0 -= staStep2;
    sta1 = *sta_1; sta_1 += staStep2;
    accu0 = SMLABT( accu0, sta0, flt);
    accu1 = SMLABT( accu1, sta1, flt);

    *pData_0-- = FX_DBL2FX_QMF(accu0<<1);
    *pData_1++ = FX_DBL2FX_QMF(accu1<<1);

    /* FIR filter 32/64 */
    flt = *p_flt++;
    sta0 = *sta_0; sta_0 += staStep1;
    sta1 = *sta_1; sta_1 -= staStep1;
    accu0 = SMULBB(        sta0, flt);
    accu1 = SMULBB(        sta1, flt);

    sta0 = *sta_0; sta_0 += staStep1;
    sta1 = *sta_1; sta_1 -= staStep1;
    accu0 = SMLABT( accu0, sta0, flt);
    accu1 = SMLABT( accu1, sta1, flt);

    flt = *p_flt++;
    sta0 = *sta_0; sta_0 += staStep1;
    sta1 = *sta_1; sta_1 -= staStep1;
    accu0 = SMLABB( accu0, sta0, flt);
    accu1 = SMLABB( accu1, sta1, flt);

    sta0 = *sta_0; sta_0 += staStep1;
    sta1 = *sta_1; sta_1 -= staStep1;
    accu0 = SMLABT( accu0, sta0, flt);
    accu1 = SMLABT( accu1, sta1, flt);

    flt = *p_flt;
    sta0 = *sta_0;
    sta1 = *sta_1;
    accu0 = SMLABB( accu0, sta0, flt);
    accu1 = SMLABB( accu1, sta1, flt);

    *pData_0-- = FX_DBL2FX_QMF(accu0<<1);
    *pData_1++ = FX_DBL2FX_QMF(accu1<<1);
  }
  else
  {
    int pfltStep = QMF_NO_POLY * (p_stride-1);

    flt = p_flt[0];
    sta1 = *sta_1;  sta_1 -= staStep1;
    accu1 = SMULBB(        sta1, flt);
    sta1 = *sta_1;  sta_1 -= staStep1;
    accu1 = SMLABT( accu1, sta1, flt);

    flt = p_flt[1];
    sta1 = *sta_1;  sta_1 -= staStep1;
    accu1 = SMLABB( accu1, sta1, flt);
    sta1 = *sta_1;  sta_1 -= staStep1;
    accu1 = SMLABT( accu1, sta1, flt);

    flt = p_flt[2]; p_flt += pfltStep;
    sta1 = *sta_1;  sta_1 += staStep2;
    accu1 = SMLABB( accu1, sta1, flt);
    *pData_1++ = FX_DBL2FX_QMF(accu1<<1);

    /* FIR filters 1..63 127..65 or 1..31 63..33 */
    for (; --no_channels; )
    {
      flt = p_flt[0];
      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMULBB(        sta0, flt);
      accu1 = SMULBB(        sta1, flt);

      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABT( accu0, sta0, flt);
      accu1 = SMLABT( accu1, sta1, flt);

      flt = p_flt[1];
      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABB( accu0, sta0, flt);
      accu1 = SMLABB( accu1, sta1, flt);

      sta0 = *sta_0; sta_0 += staStep1;
      sta1 = *sta_1; sta_1 -= staStep1;
      accu0 = SMLABT( accu0, sta0, flt);
      accu1 = SMLABT( accu1, sta1, flt);

      flt = p_flt[2]; p_flt += pfltStep;
      sta0 = *sta_0; sta_0 -= staStep2;
      sta1 = *sta_1; sta_1 += staStep2;
      accu0 = SMLABB( accu0, sta0, flt);
      accu1 = SMLABB( accu1, sta1, flt);

      *pData_0-- = FX_DBL2FX_QMF(accu0<<1);
      *pData_1++ = FX_DBL2FX_QMF(accu1<<1);
    }

    /* FIR filter 32/64 */
    flt = p_flt[0];
    sta0 = *sta_0; sta_0 += staStep1;
    accu0 = SMULBB(        sta0, flt);
    sta0 = *sta_0; sta_0 += staStep1;
    accu0 = SMLABT( accu0, sta0, flt);

    flt = p_flt[1];
    sta0 = *sta_0; sta_0 += staStep1;
    accu0 = SMLABB( accu0, sta0, flt);
    sta0 = *sta_0; sta_0 += staStep1;
    accu0 = SMLABT( accu0, sta0, flt);

    flt = p_flt[2];
    sta0 = *sta_0;
    accu0 = SMLABB( accu0, sta0, flt);
    *pData_0-- = FX_DBL2FX_QMF(accu0<<1);
  }
}
#endif /* FUNCTION_qmfAnaPrototypeFirSlot */
#endif /* #if defined(__CC_ARM) && defined(__ARM_ARCH_6__) */

#if ( defined(__ARM_ARCH_5TE__) && (SAMPLE_BITS == 16) ) && !defined(QMF_TABLE_FULL)

#define FUNCTION_qmfSynPrototypeFirSlot

#if defined(FUNCTION_qmfSynPrototypeFirSlot)

#if defined(__GNUC__)	/* cppp replaced: elif */

inline INT SMULWB (const LONG a, const LONG b)
{
  INT result ;
  __asm__ ("smulwb %0, %1, %2"
    : "=r" (result)
    : "r" (a), "r" (b)) ;

  return result ;
}
inline INT SMULWT (const LONG a, const LONG b)
{
  INT result ;
  __asm__ ("smulwt %0, %1, %2"
    : "=r" (result)
    : "r" (a), "r" (b)) ;

  return result ;
}

inline INT SMLAWB(const LONG accu, const LONG a, const LONG b)
{
  INT result;
  asm("smlawb %0, %1, %2, %3 "
        : "=r" (result)
        : "r" (a), "r" (b), "r" (accu) );
  return result ;
}

inline INT SMLAWT(const LONG accu, const LONG a, const LONG b)
{
  INT result;
  asm("smlawt %0, %1, %2, %3 "
        : "=r" (result)
        : "r" (a), "r" (b), "r" (accu) );
  return result ;
}

#endif /* ARM compiler selector */


static void qmfSynPrototypeFirSlot1_filter(FIXP_QMF *RESTRICT realSlot, 
                                           FIXP_QMF *RESTRICT imagSlot, 
                                           const FIXP_DBL *RESTRICT p_flt, 
                                           FIXP_QSS *RESTRICT sta,
                                           FIXP_DBL *pMyTimeOut, 
                                           int no_channels)
{
  /* This code was the base for the above listed assembler sequence */
  /* It can be used for debugging purpose or further optimizations  */
  const FIXP_DBL *RESTRICT p_fltm = p_flt + 155;

  do
  {
     FIXP_DBL result;
     FIXP_DBL A, B, real, imag, sta0;

     real = *--realSlot;
     imag = *--imagSlot;
     B = p_flt[4];                        /* Bottom=[8] Top=[9]     */
     A = p_fltm[3];                       /* Bottom=[316] Top=[317] */
     sta0 = sta[0];                       /* save state[0]          */
     sta[0] = SMLAWT( sta[1], imag, B ); sta++; /* index=9...........319  */
     sta[0] = SMLAWB( sta[1], real, A ); sta++; /* index=316...........6  */
     sta[0] = SMLAWB( sta[1], imag, B ); sta++; /* index=8,18,    ...318  */
     B = p_flt[3];                        /* Bottom=[6] Top=[7]     */
     sta[0] = SMLAWT( sta[1], real, A ); sta++; /* index=317...........7  */
     A = p_fltm[4];                       /* Bottom=[318] Top=[319] */
     sta[0] = SMLAWT( sta[1], imag, B ); sta++; /* index=7...........317  */
     sta[0] = SMLAWB( sta[1], real, A ); sta++; /* index=318...........8  */
     sta[0] = SMLAWB( sta[1], imag, B ); sta++; /* index=6...........316  */
     B = p_flt[2];                        /* Bottom=[X] Top=[5]     */
     sta[0] = SMLAWT( sta[1], real, A ); sta++; /* index=9...........319  */
     A = p_fltm[2];                       /* Bottom=[X] Top=[315]   */
     sta[0] =         SMULWT( imag, B ); sta++; /* index=5,15, ...   315  */
     result = SMLAWT( sta0,   real, A );  /* index=315...........5  */

     pMyTimeOut[0] = result; pMyTimeOut++;

     real = *--realSlot;
     imag = *--imagSlot;
     A = p_fltm[0];                       /* Bottom=[310] Top=[311] */
     B = p_flt[7];                        /* Bottom=[14]  Top=[15]  */
     result = SMLAWB( sta[0], real, A );  /* index=310...........0  */
     sta[0] = SMLAWB( sta[1], imag, B ); sta++; /* index=14..........324  */
     pMyTimeOut[0] = result; pMyTimeOut++;
     B = p_flt[6];                        /* Bottom=[12]  Top=[13]  */
     sta[0] = SMLAWT( sta[1], real, A ); sta++; /* index=311...........1  */
     A = p_fltm[1];                       /* Bottom=[312] Top=[313] */
     sta[0] = SMLAWT( sta[1], imag, B ); sta++; /* index=13..........323  */
     sta[0] = SMLAWB( sta[1], real, A ); sta++; /* index=312...........2  */
     sta[0] = SMLAWB( sta[1], imag, B ); sta++; /* index=12..........322  */
     sta[0] = SMLAWT( sta[1], real, A ); sta++; /* index=313...........3  */
     A = p_fltm[2];                       /* Bottom=[314] Top=[315] */
     B = p_flt[5];                        /* Bottom=[10]  Top=[11]  */
     sta[0] = SMLAWT( sta[1], imag, B ); sta++; /* index=11..........321  */
     sta[0] = SMLAWB( sta[1], real, A ); sta++; /* index=314...........4  */
     sta[0] =         SMULWB( imag, B ); sta++; /* index=10..........320  */


     p_flt    += 5;
     p_fltm   -= 5;
  } 
  while ((--no_channels) != 0);

}



INT qmfSynPrototypeFirSlot2(
                             HANDLE_QMF_FILTER_BANK qmf,
                             FIXP_QMF *RESTRICT realSlot,            /*!< Input: Pointer to real Slot */
                             FIXP_QMF *RESTRICT imagSlot,            /*!< Input: Pointer to imag Slot */
                             INT_PCM  *RESTRICT timeOut,             /*!< Time domain data */
                             INT       stride                        /*!< Time output buffer stride factor*/
                            )
{
  FIXP_QSS *RESTRICT sta = (FIXP_QSS*)qmf->FilterStates;
  int no_channels = qmf->no_channels;
  int scale = ((DFRACT_BITS-SAMPLE_BITS)-1-qmf->outScalefactor);

  /* We map an arry of 16-bit values upon an array of 2*16-bit values to read 2 values in one shot */
  const FIXP_DBL *RESTRICT p_flt  = (FIXP_DBL *) qmf->p_filter;           /* low=[0],   high=[1]   */
  const FIXP_DBL *RESTRICT p_fltm = (FIXP_DBL *) qmf->p_filter + 155;     /* low=[310], high=[311] */

  FDK_ASSERT(SAMPLE_BITS-1-qmf->outScalefactor >= 0); //   (DFRACT_BITS-SAMPLE_BITS)-1-qmf->outScalefactor >= 0);
  FDK_ASSERT(qmf->p_stride==2 && qmf->no_channels == 32);

  FDK_ASSERT((no_channels&3) == 0);  /* should be a multiple of 4 */

  realSlot += no_channels-1;    // ~~"~~
  imagSlot += no_channels-1;    // no_channels-1 .. 0

  FIXP_DBL MyTimeOut[32];
  FIXP_DBL *pMyTimeOut = &MyTimeOut[0];

  for (no_channels = no_channels; no_channels--;)
  {
     FIXP_DBL result;
     FIXP_DBL A, B, real, imag;

     real = *realSlot--;
     imag = *imagSlot--;
     A = p_fltm[0];                       /* Bottom=[310] Top=[311] */
     B = p_flt[7];                        /* Bottom=[14]  Top=[15]  */
     result = SMLAWB( sta[0], real, A );  /* index=310...........0  */
     sta[0] = SMLAWB( sta[1], imag, B ); sta++; /* index=14..........324  */
     B = p_flt[6];                        /* Bottom=[12]  Top=[13]  */
     sta[0] = SMLAWT( sta[1], real, A ); sta++; /* index=311...........1  */
     A = p_fltm[1];                       /* Bottom=[312] Top=[313] */
     sta[0] = SMLAWT( sta[1], imag, B ); sta++; /* index=13..........323  */
     sta[0] = SMLAWB( sta[1], real, A ); sta++; /* index=312...........2  */
     sta[0] = SMLAWB( sta[1], imag, B ); sta++; /* index=12..........322  */
     sta[0] = SMLAWT( sta[1], real, A ); sta++; /* index=313...........3  */
     A = p_fltm[2];                       /* Bottom=[314] Top=[315] */
     B = p_flt[5];                        /* Bottom=[10]  Top=[11]  */
     sta[0] = SMLAWT( sta[1], imag, B ); sta++; /* index=11..........321  */
     sta[0] = SMLAWB( sta[1], real, A ); sta++; /* index=314...........4  */
     sta[0] =         SMULWB( imag, B ); sta++; /* index=10..........320  */

     pMyTimeOut[0] = result; pMyTimeOut++;

     p_fltm   -= 5;
     p_flt    += 5;
  }

  pMyTimeOut = &MyTimeOut[0];
#if (SAMPLE_BITS == 16)      
  const FIXP_DBL max_pos = (FIXP_DBL) 0x00007FFF << scale;
  const FIXP_DBL max_neg = (FIXP_DBL) 0xFFFF8001 << scale;
#else
  scale = -scale;
  const FIXP_DBL max_pos = (FIXP_DBL) 0x7FFFFFFF >> scale;
  const FIXP_DBL max_neg = (FIXP_DBL) 0x80000001 >> scale;  
#endif
  const FIXP_DBL add_neg = (1 << scale) - 1;

  no_channels = qmf->no_channels;

  timeOut += no_channels*stride;

  FDK_ASSERT(scale >= 0);

  if (qmf->outGain != 0x80000000)
  {
    FIXP_DBL gain = qmf->outGain;
    for (no_channels>>=2; no_channels--;)
    {
      FIXP_DBL result1, result2;

      result1 = pMyTimeOut[0]; pMyTimeOut++;
      result2 = pMyTimeOut[0]; pMyTimeOut++;

      result1 = fMult(result1,gain);
      timeOut -= stride;
      if (result1 < 0)        result1 += add_neg;
      if (result1 < max_neg)  result1 = max_neg;
      if (result1 > max_pos)  result1 = max_pos;
#if (SAMPLE_BITS == 16)
      timeOut[0] = result1 >> scale;
#else
      timeOut[0] = result1 << scale;
#endif

      result2 = fMult(result2,gain);
      timeOut -= stride;
      if (result2 < 0)        result2 += add_neg;
      if (result2 < max_neg)  result2 = max_neg;
      if (result2 > max_pos)  result2 = max_pos;
#if (SAMPLE_BITS == 16)
      timeOut[0] = result2 >> scale;
#else
      timeOut[0] = result2 << scale;
#endif

      result1 = pMyTimeOut[0]; pMyTimeOut++;
      result2 = pMyTimeOut[0]; pMyTimeOut++;

      result1 = fMult(result1,gain);
      timeOut -= stride;
      if (result1 < 0)        result1 += add_neg;
      if (result1 < max_neg)  result1 = max_neg;
      if (result1 > max_pos)  result1 = max_pos;
#if (SAMPLE_BITS == 16)
      timeOut[0] = result1 >> scale;
#else
      timeOut[0] = result1 << scale;
#endif

      result2 = fMult(result2,gain);
      timeOut -= stride;
      if (result2 < 0)        result2 += add_neg;
      if (result2 < max_neg)  result2 = max_neg;
      if (result2 > max_pos)  result2 = max_pos;
#if (SAMPLE_BITS == 16)
      timeOut[0] = result2 >> scale;
#else
      timeOut[0] = result2 << scale;
#endif
    }
  }
  else
  {
    for (no_channels>>=2; no_channels--;)
    {
      FIXP_DBL result1, result2;
      result1 = pMyTimeOut[0]; pMyTimeOut++;
      result2 = pMyTimeOut[0]; pMyTimeOut++;
      timeOut -= stride;
      if (result1 < 0)        result1 += add_neg;
      if (result1 < max_neg)  result1 = max_neg;
      if (result1 > max_pos)  result1 = max_pos;
#if (SAMPLE_BITS == 16)
      timeOut[0] = result1 >> scale;
#else
      timeOut[0] = result1 << scale;
#endif
      
      timeOut -= stride;
      if (result2 < 0)        result2 += add_neg;
      if (result2 < max_neg)  result2 = max_neg;
      if (result2 > max_pos)  result2 = max_pos;
#if (SAMPLE_BITS == 16)
      timeOut[0] = result2 >> scale;
#else
      timeOut[0] = result2 << scale;
#endif
      
      result1 = pMyTimeOut[0]; pMyTimeOut++;
      result2 = pMyTimeOut[0]; pMyTimeOut++;
      timeOut -= stride;
      if (result1 < 0)        result1 += add_neg;
      if (result1 < max_neg)  result1 = max_neg;
      if (result1 > max_pos)  result1 = max_pos;
#if (SAMPLE_BITS == 16)
      timeOut[0] = result1 >> scale;
#else
      timeOut[0] = result1 << scale;
#endif
      
      timeOut -= stride;
      if (result2 < 0)        result2 += add_neg;
      if (result2 < max_neg)  result2 = max_neg;
      if (result2 > max_pos)  result2 = max_pos;
#if (SAMPLE_BITS == 16)
      timeOut[0] = result2 >> scale;
#else
      timeOut[0] = result2 << scale;
#endif
    }
  }
  return 0;
}

static
void qmfSynPrototypeFirSlot_fallback( HANDLE_QMF_FILTER_BANK qmf,
                             FIXP_DBL *realSlot,      /*!< Input: Pointer to real Slot */
                             FIXP_DBL *imagSlot,      /*!< Input: Pointer to imag Slot */
                             INT_PCM  *timeOut,             /*!< Time domain data */
                             const int       stride
                            );

/*!
  \brief Perform Synthesis Prototype Filtering on a single slot of input data.

  The filter takes 2 * #MAX_SYNTHESIS_CHANNELS of input data and
  generates #MAX_SYNTHESIS_CHANNELS time domain output samples.
*/

static
void qmfSynPrototypeFirSlot( HANDLE_QMF_FILTER_BANK qmf,
                             FIXP_DBL *realSlot,      /*!< Input: Pointer to real Slot */
                             FIXP_DBL *imagSlot,      /*!< Input: Pointer to imag Slot */
                             INT_PCM  *timeOut,             /*!< Time domain data */
                             const int       stride
                            )
{
    INT err = -1;

    switch (qmf->p_stride) {
    case 2:
      err = qmfSynPrototypeFirSlot2(qmf, realSlot, imagSlot, timeOut, stride);
      break;
    default:
      err = -1;
    }

    /* fallback if configuration not available or failed */
    if(err!=0) {
        qmfSynPrototypeFirSlot_fallback(qmf, realSlot, imagSlot, timeOut, stride);
    }
}
#endif /* FUNCTION_qmfSynPrototypeFirSlot */

#endif  /*  ( defined(__CC_ARM) && defined(__ARM_ARCH_5TE__) && (SAMPLE_BITS == 16) ) && !defined(QMF_TABLE_FULL) */



/* #####################################################################################*/



#endif  /* (QMF_NO_POLY==5) */

