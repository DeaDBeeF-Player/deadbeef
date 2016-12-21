
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

/***************************  Fraunhofer IIS FDK Tools  **********************

   Author(s):   Josef Hoepfl, DSP Solutions
   Description: Fix point FFT

******************************************************************************/

#include "fft.h"

#include "fft_rad2.h"
#include "FDK_tools_rom.h"





#define F3C(x) STC(x)

#define     C31       (F3C(0x91261468))      /* FL2FXCONST_DBL(-0.86602540)   */

/* Performs the FFT of length 3 according to the algorithm after winograd.
   No scaling of the input vector because the scaling is already done in the rotation vector. */
static FORCEINLINE void fft3(FIXP_DBL *RESTRICT pDat)
{
  FIXP_DBL r1,r2;
  FIXP_DBL s1,s2;
  /* real part */
  r1      = pDat[2] + pDat[4];
  r2      = fMult((pDat[2] - pDat[4]), C31);
  pDat[0] = pDat[0] + r1;
  r1      = pDat[0] - r1 - (r1>>1);

  /* imaginary part */
  s1      = pDat[3] + pDat[5];
  s2      = fMult((pDat[3] - pDat[5]), C31);
  pDat[1] = pDat[1] + s1;
  s1      = pDat[1] - s1 - (s1>>1);

  /* combination */
  pDat[2] = r1 - s2;
  pDat[4] = r1 + s2;
  pDat[3] = s1 + r2;
  pDat[5] = s1 - r2;
}


#define F5C(x) STC(x)

#define     C51       (F5C(0x79bc3854))      /* FL2FXCONST_DBL( 0.95105652)   */
#define     C52       (F5C(0x9d839db0))      /* FL2FXCONST_DBL(-1.53884180/2) */
#define     C53       (F5C(0xd18053ce))      /* FL2FXCONST_DBL(-0.36327126)   */
#define     C54       (F5C(0x478dde64))      /* FL2FXCONST_DBL( 0.55901699)   */
#define     C55       (F5C(0xb0000001))      /* FL2FXCONST_DBL(-1.25/2)       */

/* performs the FFT of length 5 according to the algorithm after winograd */
static FORCEINLINE void fft5(FIXP_DBL *RESTRICT pDat)
{
  FIXP_DBL r1,r2,r3,r4;
  FIXP_DBL s1,s2,s3,s4;
  FIXP_DBL t;

  /* real part */
  r1      = pDat[2] + pDat[8];
  r4      = pDat[2] - pDat[8];
  r3      = pDat[4] + pDat[6];
  r2      = pDat[4] - pDat[6];
  t       = fMult((r1-r3), C54);
  r1      = r1 + r3;
  pDat[0] = pDat[0] + r1;
  /* Bit shift left because of the constant C55 which was scaled with the factor 0.5 because of the representation of
     the values as fracts */
  r1      = pDat[0] + (fMultDiv2(r1, C55) <<(2));
  r3      = r1 - t;
  r1      = r1 + t;
  t       = fMult((r4 + r2), C51);
  /* Bit shift left because of the constant C55 which was scaled with the factor 0.5 because of the representation of
     the values as fracts */
  r4      = t + (fMultDiv2(r4, C52) <<(2));
  r2      = t + fMult(r2, C53);

  /* imaginary part */
  s1      = pDat[3] + pDat[9];
  s4      = pDat[3] - pDat[9];
  s3      = pDat[5] + pDat[7];
  s2      = pDat[5] - pDat[7];
  t       = fMult((s1 - s3), C54);
  s1      = s1 + s3;
  pDat[1] = pDat[1] + s1;
  /* Bit shift left because of the constant C55 which was scaled with the factor 0.5 because of the representation of
     the values as fracts */
  s1      = pDat[1] + (fMultDiv2(s1, C55) <<(2));
  s3      = s1 - t;
  s1      = s1 + t;
  t       = fMult((s4 + s2), C51);
  /* Bit shift left because of the constant C55 which was scaled with the factor 0.5 because of the representation of
     the values as fracts */
  s4      = t + (fMultDiv2(s4, C52) <<(2));
  s2      = t + fMult(s2, C53);

  /* combination */
  pDat[2] = r1 + s2;
  pDat[8] = r1 - s2;
  pDat[4] = r3 - s4;
  pDat[6] = r3 + s4;

  pDat[3] = s1 - r2;
  pDat[9] = s1 + r2;
  pDat[5] = s3 + r4;
  pDat[7] = s3 - r4;
}




#define N3                    3
#define N5                    5
#define N6                    6
#define N15                   15

/* Performs the FFT of length 15. It is split into FFTs of length 3 and length 5. */
static inline void fft15(FIXP_DBL *pInput)
{
  FIXP_DBL  aDst[2*N15];
  FIXP_DBL  aDst1[2*N15];
  int    i,k,l;

  /* Sort input vector for fft's of length 3
  input3(0:2)   = [input(0) input(5) input(10)];
  input3(3:5)   = [input(3) input(8) input(13)];
  input3(6:8)   = [input(6) input(11) input(1)];
  input3(9:11)  = [input(9) input(14) input(4)];
  input3(12:14) = [input(12) input(2) input(7)]; */
  {
    const FIXP_DBL *pSrc = pInput;
    FIXP_DBL *RESTRICT pDst = aDst;
    /* Merge 3 loops into one, skip call of fft3 */
    for(i=0,l=0,k=0; i<N5; i++, k+=6)
    {
      pDst[k+0] = pSrc[l];
      pDst[k+1] = pSrc[l+1];
      l += 2*N5;
      if (l >= (2*N15))
          l -= (2*N15);

      pDst[k+2] = pSrc[l];
      pDst[k+3] = pSrc[l+1];
      l += 2*N5;
      if (l >= (2*N15))
          l -= (2*N15);
      pDst[k+4] = pSrc[l];
      pDst[k+5] = pSrc[l+1];
      l += (2*N5) + (2*N3);
      if (l >= (2*N15))
          l -= (2*N15);

      /* fft3 merged with shift right by 2 loop */
      FIXP_DBL r1,r2,r3;
      FIXP_DBL s1,s2;
      /* real part */
      r1      = pDst[k+2] + pDst[k+4];
      r2      = fMult((pDst[k+2] - pDst[k+4]), C31);
      s1      = pDst[k+0];
      pDst[k+0] = (s1 + r1)>>2;
      r1      = s1 - (r1>>1);

      /* imaginary part */
      s1      = pDst[k+3] + pDst[k+5];
      s2      = fMult((pDst[k+3] - pDst[k+5]), C31);
      r3      = pDst[k+1];
      pDst[k+1] = (r3 + s1)>>2;
      s1      = r3 - (s1>>1);

      /* combination */
      pDst[k+2] = (r1 - s2)>>2;
      pDst[k+4] = (r1 + s2)>>2;
      pDst[k+3] = (s1 + r2)>>2;
      pDst[k+5] = (s1 - r2)>>2;
    }
  }
  /* Sort input vector for fft's of length 5
  input5(0:4)   = [output3(0) output3(3) output3(6) output3(9) output3(12)];
  input5(5:9)   = [output3(1) output3(4) output3(7) output3(10) output3(13)];
  input5(10:14) = [output3(2) output3(5) output3(8) output3(11) output3(14)]; */
  /* Merge 2 loops into one, brings about 10% */
  {
    const FIXP_DBL *pSrc = aDst;
    FIXP_DBL *RESTRICT pDst = aDst1;
    for(i=0,l=0,k=0; i<N3; i++, k+=10)
    {
      l = 2*i;
      pDst[k+0] = pSrc[l+0];
      pDst[k+1] = pSrc[l+1];
      pDst[k+2] = pSrc[l+0+(2*N3)];
      pDst[k+3] = pSrc[l+1+(2*N3)];
      pDst[k+4] = pSrc[l+0+(4*N3)];
      pDst[k+5] = pSrc[l+1+(4*N3)];
      pDst[k+6] = pSrc[l+0+(6*N3)];
      pDst[k+7] = pSrc[l+1+(6*N3)];
      pDst[k+8] = pSrc[l+0+(8*N3)];
      pDst[k+9] = pSrc[l+1+(8*N3)];
      fft5(&pDst[k]);
    }
  }
  /* Sort output vector of length 15
  output = [out5(0)  out5(6)  out5(12) out5(3)  out5(9)
            out5(10) out5(1)  out5(7)  out5(13) out5(4)
            out5(5)  out5(11) out5(2)  out5(8)  out5(14)]; */
  /* optimize clumsy loop, brings about 5% */
  {
    const FIXP_DBL *pSrc = aDst1;
    FIXP_DBL *RESTRICT pDst = pInput;
    for(i=0,l=0,k=0; i<N3; i++, k+=10)
    {
      pDst[k+0] = pSrc[l];
      pDst[k+1] = pSrc[l+1];
      l += (2*N6);
      if (l >= (2*N15))
          l -= (2*N15);
      pDst[k+2] = pSrc[l];
      pDst[k+3] = pSrc[l+1];
      l += (2*N6);
      if (l >= (2*N15))
          l -= (2*N15);
      pDst[k+4] = pSrc[l];
      pDst[k+5] = pSrc[l+1];
      l += (2*N6);
      if (l >= (2*N15))
          l -= (2*N15);
      pDst[k+6] = pSrc[l];
      pDst[k+7] = pSrc[l+1];
      l += (2*N6);
      if (l >= (2*N15))
          l -= (2*N15);
      pDst[k+8] = pSrc[l];
      pDst[k+9] = pSrc[l+1];
      l += 2;    /* no modulo check needed, it cannot occur */
    }
  }
}

#define W_PiFOURTH STC(0x5a82799a)
#ifndef SUMDIFF_PIFOURTH
#define SUMDIFF_PIFOURTH(diff,sum,a,b) \
  { \
    FIXP_DBL wa, wb;\
    wa = fMultDiv2(a, W_PiFOURTH);\
    wb = fMultDiv2(b, W_PiFOURTH);\
    diff = wb - wa;\
    sum  = wb + wa;\
  }
#endif

/* This version is more overflow save, but less cycle optimal. */
#define SUMDIFF_EIGTH(x, y, ix, iy, vr, vi, ur, ui) \
  vr = (x[ 0 + ix]>>1) + (x[16 + ix]>>1);  /* Re A + Re B */ \
  vi = (x[ 8 + ix]>>1) + (x[24 + ix]>>1);     /* Re C + Re D */ \
  ur = (x[ 1 + ix]>>1) + (x[17 + ix]>>1);  /* Im A + Im B */ \
  ui = (x[ 9 + ix]>>1) + (x[25 + ix]>>1);     /* Im C + Im D */ \
  y[ 0 + iy] = vr + vi;     /* Re A' = ReA + ReB +ReC + ReD */    \
  y[ 4 + iy] = vr - vi;     /* Re C' = -(ReC+ReD) + (ReA+ReB) */  \
  y[ 1 + iy] = ur + ui;     /* Im A' = sum of imag values */      \
  y[ 5 + iy] = ur - ui;     /* Im C' = -Im C -Im D +Im A +Im B */ \
  vr -= x[16 + ix];              /* Re A - Re B */ \
  vi = vi - x[24 + ix];          /* Re C - Re D */ \
  ur -= x[17 + ix];              /* Im A - Im B */ \
  ui = ui - x[25 + ix];          /* Im C - Im D */ \
  y[ 2 + iy] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */ \
  y[ 6 + iy] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */ \
  y[ 3 + iy] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */  \
  y[ 7 + iy] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */

static const FIXP_STP fft16_w16[2] =  { STCP(0x7641af3d, 0x30fbc54d), STCP(0x30fbc54d, 0x7641af3d) };

LNK_SECTION_CODE_L1
inline void fft_16(FIXP_DBL *RESTRICT x)
{
  FIXP_DBL vr, vi, ur, ui;
  FIXP_DBL y[32];

  SUMDIFF_EIGTH(x, y, 0,  0, vr, vi, ur, ui);
  SUMDIFF_EIGTH(x, y, 4,  8, vr, vi, ur, ui);
  SUMDIFF_EIGTH(x, y, 2, 16, vr, vi, ur, ui);
  SUMDIFF_EIGTH(x, y, 6, 24, vr, vi, ur, ui);

// xt1 =  0
// xt2 =  8
  vr = y[ 8];
  vi = y[ 9];
  ur = y[ 0]>>1;
  ui = y[ 1]>>1;
  x[ 0] = ur + (vr>>1);
  x[ 1] = ui + (vi>>1);
  x[ 8] = ur - (vr>>1);
  x[ 9] = ui - (vi>>1);

// xt1 =  4
// xt2 = 12
  vr = y[13];
  vi = y[12];
  ur = y[ 4]>>1;
  ui = y[ 5]>>1;
  x[ 4] = ur + (vr>>1);
  x[ 5] = ui - (vi>>1);
  x[12] = ur - (vr>>1);
  x[13] = ui + (vi>>1);

// xt1 = 16
// xt2 = 24
  vr = y[24];
  vi = y[25];
  ur = y[16]>>1;
  ui = y[17]>>1;
  x[16] = ur + (vr>>1);
  x[17] = ui + (vi>>1);
  x[24] = ur - (vr>>1);
  x[25] = ui - (vi>>1);

// xt1 = 20
// xt2 = 28
  vr = y[29];
  vi = y[28];
  ur = y[20]>>1;
  ui = y[21]>>1;
  x[20] = ur + (vr>>1);
  x[21] = ui - (vi>>1);
  x[28] = ur - (vr>>1);
  x[29] = ui + (vi>>1);

  // xt1 =  2
// xt2 = 10
  SUMDIFF_PIFOURTH(vi, vr, y[10], y[11])
  //vr = fMultDiv2((y[11] + y[10]),W_PiFOURTH);
  //vi = fMultDiv2((y[11] - y[10]),W_PiFOURTH);
  ur = y[ 2];
  ui = y[ 3];
  x[ 2] = (ur>>1) + vr;
  x[ 3] = (ui>>1) + vi;
  x[10] = (ur>>1) - vr;
  x[11] = (ui>>1) - vi;

// xt1 =  6
// xt2 = 14
  SUMDIFF_PIFOURTH(vr, vi, y[14], y[15])
  ur = y[ 6];
  ui = y[ 7];
  x[ 6] = (ur>>1) + vr;
  x[ 7] = (ui>>1) - vi;
  x[14] = (ur>>1) - vr;
  x[15] = (ui>>1) + vi;

// xt1 = 18
// xt2 = 26
  SUMDIFF_PIFOURTH(vi, vr, y[26], y[27])
  ur = y[18];
  ui = y[19];
  x[18] = (ur>>1) + vr;
  x[19] = (ui>>1) + vi;
  x[26] = (ur>>1) - vr;
  x[27] = (ui>>1) - vi;

// xt1 = 22
// xt2 = 30
  SUMDIFF_PIFOURTH(vr, vi, y[30], y[31])
  ur = y[22];
  ui = y[23];
  x[22] = (ur>>1) + vr;
  x[23] = (ui>>1) - vi;
  x[30] = (ur>>1) - vr;
  x[31] = (ui>>1) + vi;

// xt1 =  0
// xt2 = 16
  vr = x[16];
  vi = x[17];
  ur = x[ 0]>>1;
  ui = x[ 1]>>1;
  x[ 0] = ur + (vr>>1);
  x[ 1] = ui + (vi>>1);
  x[16] = ur - (vr>>1);
  x[17] = ui - (vi>>1);

// xt1 =  8
// xt2 = 24
  vi = x[24];
  vr = x[25];
  ur = x[ 8]>>1;
  ui = x[ 9]>>1;
  x[ 8] = ur + (vr>>1);
  x[ 9] = ui - (vi>>1);
  x[24] = ur - (vr>>1);
  x[25] = ui + (vi>>1);

// xt1 =  2
// xt2 = 18
  cplxMultDiv2(&vi, &vr, x[19], x[18], fft16_w16[0]);
  ur = x[ 2];
  ui = x[ 3];
  x[ 2] = (ur>>1) + vr;
  x[ 3] = (ui>>1) + vi;
  x[18] = (ur>>1) - vr;
  x[19] = (ui>>1) - vi;

// xt1 = 10
// xt2 = 26
  cplxMultDiv2(&vr, &vi, x[27], x[26], fft16_w16[0]);
  ur = x[10];
  ui = x[11];
  x[10] = (ur>>1) + vr;
  x[11] = (ui>>1) - vi;
  x[26] = (ur>>1) - vr;
  x[27] = (ui>>1) + vi;

// xt1 =  4
// xt2 = 20
  SUMDIFF_PIFOURTH(vi, vr, x[20], x[21])
  ur = x[ 4];
  ui = x[ 5];
  x[ 4] = (ur>>1) + vr;
  x[ 5] = (ui>>1) + vi;
  x[20] = (ur>>1) - vr;
  x[21] = (ui>>1) - vi;

// xt1 = 12
// xt2 = 28
  SUMDIFF_PIFOURTH(vr, vi, x[28], x[29])
  ur = x[12];
  ui = x[13];
  x[12] = (ur>>1) + vr;
  x[13] = (ui>>1) - vi;
  x[28] = (ur>>1) - vr;
  x[29] = (ui>>1) + vi;

// xt1 =  6
// xt2 = 22
  cplxMultDiv2(&vi, &vr, x[23], x[22], fft16_w16[1]);
  ur = x[ 6];
  ui = x[ 7];
  x[ 6] = (ur>>1) + vr;
  x[ 7] = (ui>>1) + vi;
  x[22] = (ur>>1) - vr;
  x[23] = (ui>>1) - vi;

// xt1 = 14
// xt2 = 30
  cplxMultDiv2(&vr, &vi, x[31], x[30], fft16_w16[1]);
  ur = x[14];
  ui = x[15];
  x[14] = (ur>>1) + vr;
  x[15] = (ui>>1) - vi;
  x[30] = (ur>>1) - vr;
  x[31] = (ui>>1) + vi;
}

#ifndef FUNCTION_fft_32
static const FIXP_STP fft32_w32[6] =
{
  STCP (0x7641af3d, 0x30fbc54d), STCP(0x30fbc54d, 0x7641af3d), STCP(0x7d8a5f40, 0x18f8b83c),
  STCP (0x6a6d98a4, 0x471cece7), STCP(0x471cece7, 0x6a6d98a4), STCP(0x18f8b83c, 0x7d8a5f40)
};

LNK_SECTION_CODE_L1
inline void fft_32(FIXP_DBL *x)
{

#define W_PiFOURTH STC(0x5a82799a)

  FIXP_DBL vr,vi,ur,ui;
  FIXP_DBL y[64];

  /*
   * 1+2 stage radix 4
   */

/////////////////////////////////////////////////////////////////////////////////////////

  // i = 0
  vr = (x[ 0] + x[32])>>1;  /* Re A + Re B */
  vi = (x[16] + x[48]);     /* Re C + Re D */
  ur = (x[ 1] + x[33])>>1;  /* Im A + Im B */
  ui = (x[17] + x[49]);     /* Im C + Im D */

  y[ 0] = vr + (vi>>1);     /* Re A' = ReA + ReB +ReC + ReD */
  y[ 4] = vr - (vi>>1);     /* Re C' = -(ReC+ReD) + (ReA+ReB) */
  y[ 1] = ur + (ui>>1);     /* Im A' = sum of imag values */
  y[ 5] = ur - (ui>>1);     /* Im C' = -Im C -Im D +Im A +Im B */

  vr -= x[32];              /* Re A - Re B */
  vi = (vi>>1) - x[48];     /* Re C - Re D */
  ur -= x[33];              /* Im A - Im B */
  ui = (ui>>1) - x[49];     /* Im C - Im D */

  y[ 2] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */
  y[ 6] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */
  y[ 3] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */
  y[ 7] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */

  //i=8
  vr = (x[ 8] + x[40])>>1;  /* Re A + Re B */
  vi = (x[24] + x[56]);     /* Re C + Re D */
  ur = (x[ 9] + x[41])>>1;  /* Im A + Im B */
  ui = (x[25] + x[57]);     /* Im C + Im D */

  y[ 8] = vr + (vi>>1);     /* Re A' = ReA + ReB +ReC + ReD */
  y[12] = vr - (vi>>1);     /* Re C' = -(ReC+ReD) + (ReA+ReB) */
  y[ 9] = ur + (ui>>1);     /* Im A' = sum of imag values */
  y[13] = ur - (ui>>1);     /* Im C' = -Im C -Im D +Im A +Im B */

  vr -= x[40];              /* Re A - Re B */
  vi = (vi>>1) - x[56];     /* Re C - Re D */
  ur -= x[41];              /* Im A - Im B */
  ui = (ui>>1) - x[57];     /* Im C - Im D */

  y[10] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */
  y[14] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */
  y[11] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */
  y[15] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */

  //i=16
  vr = (x[ 4] + x[36])>>1;  /* Re A + Re B */
  vi = (x[20] + x[52]);     /* Re C + Re D */
  ur = (x[ 5] + x[37])>>1;  /* Im A + Im B */
  ui = (x[21] + x[53]);     /* Im C + Im D */

  y[16] = vr + (vi>>1);     /* Re A' = ReA + ReB +ReC + ReD */
  y[20] = vr - (vi>>1);     /* Re C' = -(ReC+ReD) + (ReA+ReB) */
  y[17] = ur + (ui>>1);     /* Im A' = sum of imag values */
  y[21] = ur - (ui>>1);     /* Im C' = -Im C -Im D +Im A +Im B */

  vr -= x[36];              /* Re A - Re B */
  vi = (vi>>1) - x[52];     /* Re C - Re D */
  ur -= x[37];              /* Im A - Im B */
  ui = (ui>>1) - x[53];     /* Im C - Im D */

  y[18] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */
  y[22] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */
  y[19] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */
  y[23] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */

  //i=24
  vr = (x[12] + x[44])>>1;  /* Re A + Re B */
  vi = (x[28] + x[60]);     /* Re C + Re D */
  ur = (x[13] + x[45])>>1;  /* Im A + Im B */
  ui = (x[29] + x[61]);     /* Im C + Im D */

  y[24] = vr + (vi>>1);     /* Re A' = ReA + ReB +ReC + ReD */
  y[28] = vr - (vi>>1);     /* Re C' = -(ReC+ReD) + (ReA+ReB) */
  y[25] = ur + (ui>>1);     /* Im A' = sum of imag values */
  y[29] = ur - (ui>>1);     /* Im C' = -Im C -Im D +Im A +Im B */

  vr -= x[44];              /* Re A - Re B */
  vi = (vi>>1) - x[60];     /* Re C - Re D */
  ur -= x[45];              /* Im A - Im B */
  ui = (ui>>1) - x[61];     /* Im C - Im D */

  y[26] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */
  y[30] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */
  y[27] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */
  y[31] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */

  // i = 32
  vr = (x[ 2] + x[34])>>1;  /* Re A + Re B */
  vi = (x[18] + x[50]);     /* Re C + Re D */
  ur = (x[ 3] + x[35])>>1;  /* Im A + Im B */
  ui = (x[19] + x[51]);     /* Im C + Im D */

  y[32] = vr + (vi>>1);     /* Re A' = ReA + ReB +ReC + ReD */
  y[36] = vr - (vi>>1);     /* Re C' = -(ReC+ReD) + (ReA+ReB) */
  y[33] = ur + (ui>>1);     /* Im A' = sum of imag values */
  y[37] = ur - (ui>>1);     /* Im C' = -Im C -Im D +Im A +Im B */

  vr -= x[34];              /* Re A - Re B */
  vi = (vi>>1) - x[50];     /* Re C - Re D */
  ur -= x[35];              /* Im A - Im B */
  ui = (ui>>1) - x[51];     /* Im C - Im D */

  y[34] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */
  y[38] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */
  y[35] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */
  y[39] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */

  //i=40
  vr = (x[10] + x[42])>>1;  /* Re A + Re B */
  vi = (x[26] + x[58]);     /* Re C + Re D */
  ur = (x[11] + x[43])>>1;  /* Im A + Im B */
  ui = (x[27] + x[59]);     /* Im C + Im D */

  y[40] = vr + (vi>>1);     /* Re A' = ReA + ReB +ReC + ReD */
  y[44] = vr - (vi>>1);     /* Re C' = -(ReC+ReD) + (ReA+ReB) */
  y[41] = ur + (ui>>1);     /* Im A' = sum of imag values */
  y[45] = ur - (ui>>1);     /* Im C' = -Im C -Im D +Im A +Im B */

  vr -= x[42];              /* Re A - Re B */
  vi = (vi>>1) - x[58];     /* Re C - Re D */
  ur -= x[43];              /* Im A - Im B */
  ui = (ui>>1) - x[59];     /* Im C - Im D */

  y[42] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */
  y[46] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */
  y[43] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */
  y[47] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */

  //i=48
  vr = (x[ 6] + x[38])>>1;  /* Re A + Re B */
  vi = (x[22] + x[54]);     /* Re C + Re D */
  ur = (x[ 7] + x[39])>>1;  /* Im A + Im B */
  ui = (x[23] + x[55]);     /* Im C + Im D */

  y[48] = vr + (vi>>1);     /* Re A' = ReA + ReB +ReC + ReD */
  y[52] = vr - (vi>>1);     /* Re C' = -(ReC+ReD) + (ReA+ReB) */
  y[49] = ur + (ui>>1);     /* Im A' = sum of imag values */
  y[53] = ur - (ui>>1);     /* Im C' = -Im C -Im D +Im A +Im B */

  vr -= x[38];              /* Re A - Re B */
  vi = (vi>>1) - x[54];     /* Re C - Re D */
  ur -= x[39];              /* Im A - Im B */
  ui = (ui>>1) - x[55];     /* Im C - Im D */

  y[50] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */
  y[54] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */
  y[51] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */
  y[55] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */

  //i=56
  vr = (x[14] + x[46])>>1;  /* Re A + Re B */
  vi = (x[30] + x[62]);     /* Re C + Re D */
  ur = (x[15] + x[47])>>1;  /* Im A + Im B */
  ui = (x[31] + x[63]);     /* Im C + Im D */

  y[56] = vr + (vi>>1);     /* Re A' = ReA + ReB +ReC + ReD */
  y[60] = vr - (vi>>1);     /* Re C' = -(ReC+ReD) + (ReA+ReB) */
  y[57] = ur + (ui>>1);     /* Im A' = sum of imag values */
  y[61] = ur - (ui>>1);     /* Im C' = -Im C -Im D +Im A +Im B */

  vr -= x[46];              /* Re A - Re B */
  vi = (vi>>1) - x[62];     /* Re C - Re D */
  ur -= x[47];              /* Im A - Im B */
  ui = (ui>>1) - x[63];     /* Im C - Im D */

  y[58] = ui + vr;          /* Re B' = Im C - Im D  + Re A - Re B */
  y[62] = vr - ui;          /* Re D' = -Im C + Im D + Re A - Re B */
  y[59] = ur - vi;          /* Im B'= -Re C + Re D + Im A - Im B */
  y[63] = vi + ur;          /* Im D'= Re C - Re D + Im A - Im B */


  FIXP_DBL *xt = &x[0];
  FIXP_DBL *yt = &y[0];

  int j = 4;
  do
  {
    vr = yt[8];
    vi = yt[9];
    ur = yt[0]>>1;
    ui = yt[1]>>1;
    xt[ 0] = ur + (vr>>1);
    xt[ 1] = ui + (vi>>1);
    xt[ 8] = ur - (vr>>1);
    xt[ 9] = ui - (vi>>1);

    vr = yt[13];
    vi = yt[12];
    ur = yt[4]>>1;
    ui = yt[5]>>1;
    xt[ 4] = ur + (vr>>1);
    xt[ 5] = ui - (vi>>1);
    xt[12] = ur - (vr>>1);
    xt[13] = ui + (vi>>1);

    SUMDIFF_PIFOURTH(vi, vr, yt[10], yt[11])
    ur = yt[2];
    ui = yt[3];
    xt[ 2] = (ur>>1) + vr;
    xt[ 3] = (ui>>1) + vi;
    xt[10] = (ur>>1) - vr;
    xt[11] = (ui>>1) - vi;

    SUMDIFF_PIFOURTH(vr, vi, yt[14], yt[15])
    ur = yt[6];
    ui = yt[7];

    xt[ 6] = (ur>>1) + vr;
    xt[ 7] = (ui>>1) - vi;
    xt[14] = (ur>>1) - vr;
    xt[15] = (ui>>1) + vi;
    xt += 16;
    yt += 16;
  } while (--j != 0);

  vr = x[16];
  vi = x[17];
  ur = x[ 0]>>1;
  ui = x[ 1]>>1;
  x[ 0] = ur + (vr>>1);
  x[ 1] = ui + (vi>>1);
  x[16] = ur - (vr>>1);
  x[17] = ui - (vi>>1);

  vi = x[24];
  vr = x[25];
  ur = x[ 8]>>1;
  ui = x[ 9]>>1;
  x[ 8] = ur + (vr>>1);
  x[ 9] = ui - (vi>>1);
  x[24] = ur - (vr>>1);
  x[25] = ui + (vi>>1);

  vr = x[48];
  vi = x[49];
  ur = x[32]>>1;
  ui = x[33]>>1;
  x[32] = ur + (vr>>1);
  x[33] = ui + (vi>>1);
  x[48] = ur - (vr>>1);
  x[49] = ui - (vi>>1);

  vi = x[56];
  vr = x[57];
  ur = x[40]>>1;
  ui = x[41]>>1;
  x[40] = ur + (vr>>1);
  x[41] = ui - (vi>>1);
  x[56] = ur - (vr>>1);
  x[57] = ui + (vi>>1);

  cplxMultDiv2(&vi, &vr, x[19], x[18], fft32_w32[0]);
  ur = x[ 2];
  ui = x[ 3];
  x[ 2] = (ur>>1) + vr;
  x[ 3] = (ui>>1) + vi;
  x[18] = (ur>>1) - vr;
  x[19] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[27], x[26], fft32_w32[0]);
  ur = x[10];
  ui = x[11];
  x[10] = (ur>>1) + vr;
  x[11] = (ui>>1) - vi;
  x[26] = (ur>>1) - vr;
  x[27] = (ui>>1) + vi;

  cplxMultDiv2(&vi, &vr, x[51], x[50], fft32_w32[0]);
  ur = x[34];
  ui = x[35];
  x[34] = (ur>>1) + vr;
  x[35] = (ui>>1) + vi;
  x[50] = (ur>>1) - vr;
  x[51] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[59], x[58], fft32_w32[0]);
  ur = x[42];
  ui = x[43];
  x[42] = (ur>>1) + vr;
  x[43] = (ui>>1) - vi;
  x[58] = (ur>>1) - vr;
  x[59] = (ui>>1) + vi;

  SUMDIFF_PIFOURTH(vi, vr, x[20], x[21])
  ur = x[ 4];
  ui = x[ 5];
  x[ 4] = (ur>>1) + vr;
  x[ 5] = (ui>>1) + vi;
  x[20] = (ur>>1) - vr;
  x[21] = (ui>>1) - vi;

  SUMDIFF_PIFOURTH(vr, vi, x[28], x[29])
  ur = x[12];
  ui = x[13];
  x[12] = (ur>>1) + vr;
  x[13] = (ui>>1) - vi;
  x[28] = (ur>>1) - vr;
  x[29] = (ui>>1) + vi;

  SUMDIFF_PIFOURTH(vi, vr, x[52], x[53])
  ur = x[36];
  ui = x[37];
  x[36] = (ur>>1) + vr;
  x[37] = (ui>>1) + vi;
  x[52] = (ur>>1) - vr;
  x[53] = (ui>>1) - vi;

  SUMDIFF_PIFOURTH(vr, vi, x[60], x[61])
  ur = x[44];
  ui = x[45];
  x[44] = (ur>>1) + vr;
  x[45] = (ui>>1) - vi;
  x[60] = (ur>>1) - vr;
  x[61] = (ui>>1) + vi;


  cplxMultDiv2(&vi, &vr, x[23], x[22], fft32_w32[1]);
  ur = x[ 6];
  ui = x[ 7];
  x[ 6] = (ur>>1) + vr;
  x[ 7] = (ui>>1) + vi;
  x[22] = (ur>>1) - vr;
  x[23] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[31], x[30], fft32_w32[1]);
  ur = x[14];
  ui = x[15];
  x[14] = (ur>>1) + vr;
  x[15] = (ui>>1) - vi;
  x[30] = (ur>>1) - vr;
  x[31] = (ui>>1) + vi;

  cplxMultDiv2(&vi, &vr, x[55], x[54], fft32_w32[1]);
  ur = x[38];
  ui = x[39];
  x[38] = (ur>>1) + vr;
  x[39] = (ui>>1) + vi;
  x[54] = (ur>>1) - vr;
  x[55] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[63], x[62], fft32_w32[1]);
  ur = x[46];
  ui = x[47];

  x[46] = (ur>>1) + vr;
  x[47] = (ui>>1) - vi;
  x[62] = (ur>>1) - vr;
  x[63] = (ui>>1) + vi;

  vr = x[32];
  vi = x[33];
  ur = x[ 0]>>1;
  ui = x[ 1]>>1;
  x[ 0] = ur + (vr>>1);
  x[ 1] = ui + (vi>>1);
  x[32] = ur - (vr>>1);
  x[33] = ui - (vi>>1);

  vi = x[48];
  vr = x[49];
  ur = x[16]>>1;
  ui = x[17]>>1;
  x[16] = ur + (vr>>1);
  x[17] = ui - (vi>>1);
  x[48] = ur - (vr>>1);
  x[49] = ui + (vi>>1);

  cplxMultDiv2(&vi, &vr, x[35], x[34], fft32_w32[2]);
  ur = x[ 2];
  ui = x[ 3];
  x[ 2] = (ur>>1) + vr;
  x[ 3] = (ui>>1) + vi;
  x[34] = (ur>>1) - vr;
  x[35] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[51], x[50], fft32_w32[2]);
  ur = x[18];
  ui = x[19];
  x[18] = (ur>>1) + vr;
  x[19] = (ui>>1) - vi;
  x[50] = (ur>>1) - vr;
  x[51] = (ui>>1) + vi;

  cplxMultDiv2(&vi, &vr, x[37], x[36], fft32_w32[0]);
  ur = x[ 4];
  ui = x[ 5];
  x[ 4] = (ur>>1) + vr;
  x[ 5] = (ui>>1) + vi;
  x[36] = (ur>>1) - vr;
  x[37] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[53], x[52], fft32_w32[0]);
  ur = x[20];
  ui = x[21];
  x[20] = (ur>>1) + vr;
  x[21] = (ui>>1) - vi;
  x[52] = (ur>>1) - vr;
  x[53] = (ui>>1) + vi;

  cplxMultDiv2(&vi, &vr, x[39], x[38], fft32_w32[3]);
  ur = x[ 6];
  ui = x[ 7];
  x[ 6] = (ur>>1) + vr;
  x[ 7] = (ui>>1) + vi;
  x[38] = (ur>>1) - vr;
  x[39] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[55], x[54], fft32_w32[3]);
  ur = x[22];
  ui = x[23];
  x[22] = (ur>>1) + vr;
  x[23] = (ui>>1) - vi;
  x[54] = (ur>>1) - vr;
  x[55] = (ui>>1) + vi;

  SUMDIFF_PIFOURTH(vi, vr, x[40], x[41])
  ur = x[ 8];
  ui = x[ 9];
  x[ 8] = (ur>>1) + vr;
  x[ 9] = (ui>>1) + vi;
  x[40] = (ur>>1) - vr;
  x[41] = (ui>>1) - vi;

  SUMDIFF_PIFOURTH(vr, vi, x[56], x[57])
  ur = x[24];
  ui = x[25];
  x[24] = (ur>>1) + vr;
  x[25] = (ui>>1) - vi;
  x[56] = (ur>>1) - vr;
  x[57] = (ui>>1) + vi;

  cplxMultDiv2(&vi, &vr, x[43], x[42], fft32_w32[4]);
  ur = x[10];
  ui = x[11];

  x[10] = (ur>>1) + vr;
  x[11] = (ui>>1) + vi;
  x[42] = (ur>>1) - vr;
  x[43] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[59], x[58], fft32_w32[4]);
  ur = x[26];
  ui = x[27];
  x[26] = (ur>>1) + vr;
  x[27] = (ui>>1) - vi;
  x[58] = (ur>>1) - vr;
  x[59] = (ui>>1) + vi;

  cplxMultDiv2(&vi, &vr, x[45], x[44], fft32_w32[1]);
  ur = x[12];
  ui = x[13];
  x[12] = (ur>>1) + vr;
  x[13] = (ui>>1) + vi;
  x[44] = (ur>>1) - vr;
  x[45] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[61], x[60], fft32_w32[1]);
  ur = x[28];
  ui = x[29];
  x[28] = (ur>>1) + vr;
  x[29] = (ui>>1) - vi;
  x[60] = (ur>>1) - vr;
  x[61] = (ui>>1) + vi;

  cplxMultDiv2(&vi, &vr, x[47], x[46], fft32_w32[5]);
  ur = x[14];
  ui = x[15];
  x[14] = (ur>>1) + vr;
  x[15] = (ui>>1) + vi;
  x[46] = (ur>>1) - vr;
  x[47] = (ui>>1) - vi;

  cplxMultDiv2(&vr, &vi, x[63], x[62], fft32_w32[5]);
  ur = x[30];
  ui = x[31];
  x[30] = (ur>>1) + vr;
  x[31] = (ui>>1) - vi;
  x[62] = (ur>>1) - vr;
  x[63] = (ui>>1) + vi;
}
#endif /* #ifndef FUNCTION_fft_32 */


/**
 * \brief Apply rotation vectors to a data buffer.
 * \param cl length of each row of input data.
 * \param l total length of input data.
 * \param pVecRe real part of rotation ceofficient vector.
 * \param pVecIm imaginary part of rotation ceofficient vector.
 */
static inline void fft_apply_rot_vector(FIXP_DBL *RESTRICT pData, const int cl, const int l, const FIXP_STB *pVecRe, const FIXP_STB *pVecIm)
{
  FIXP_DBL re, im;
  FIXP_STB vre, vim;

  int i, c;

  for(i=0; i<cl; i++) {
    re  = pData[2*i];
    im  = pData[2*i+1];

    pData[2*i]   = re>>2; /* * 0.25 */
    pData[2*i+1] = im>>2; /* * 0.25 */
  }
  for(; i<l; i+=cl)
  {
    re  = pData[2*i];
    im  = pData[2*i+1];

    pData[2*i]   = re>>2; /* * 0.25 */
    pData[2*i+1] = im>>2; /* * 0.25 */

    for (c=i+1; c<i+cl; c++)
    {
      re  = pData[2*c]>>1;
      im  = pData[2*c+1]>>1;
      vre = *pVecRe++;
      vim = *pVecIm++;

      cplxMultDiv2(&pData[2*c+1], &pData[2*c], im, re, vre, vim);
    }
  }
}

#define FFT_TWO_STAGE_MACRO_ENABLE


#ifdef FFT_TWO_STAGE_MACRO_ENABLE

#define fftN2(pInput, length, dim1, dim2, fft_func1, fft_func2, RotVectorReal, RotVectorImag) \
{ \
  int       i, j; \
 \
  C_ALLOC_SCRATCH_START(aDst, FIXP_DBL, length*2); \
  C_ALLOC_SCRATCH_START(aDst2, FIXP_DBL, dim2*2); \
 \
  FDK_ASSERT(length == dim1*dim2); \
 \
  /* Perform dim2 times the fft of length dim1. The input samples are at the address of pSrc and the \
  output samples are at the address of pDst. The input vector for the fft of length dim1 is built \
  of the interleaved samples in pSrc, the output samples are stored consecutively. \
  */ \
  { \
    const FIXP_DBL* pSrc = pInput; \
    FIXP_DBL  *RESTRICT pDst = aDst; \
    \
    for(i=0; i<dim2; i++) \
    { \
      for(j=0; j<dim1; j++) \
      { \
        pDst[2*j]   = pSrc[2*j*dim2]; \
        pDst[2*j+1] = pSrc[2*j*dim2+1]; \
      } \
      \
      fft_func1(pDst); \
      pSrc += 2; \
      pDst = pDst + 2*dim1; \
    } \
  } \
  \
  /* Perform the modulation of the output of the fft of length dim1 */ \
  fft_apply_rot_vector(aDst, dim1, length, RotVectorReal, RotVectorImag); \
  \
  /* Perform dim1 times the fft of length dim2. The input samples are at the address of aDst and the \
  output samples are at the address of pInput. The input vector for the fft of length dim2 is built \
  of the interleaved samples in aDst, the output samples are stored consecutively at the address \
  of pInput. \
  */ \
  { \
    const FIXP_DBL* pSrc       = aDst; \
    FIXP_DBL *RESTRICT pDst    = aDst2; \
    FIXP_DBL *RESTRICT pDstOut = pInput; \
    \
    for(i=0; i<dim1; i++) \
    { \
      for(j=0; j<dim2; j++) \
      { \
        pDst[2*j]   = pSrc[2*j*dim1]; \
        pDst[2*j+1] = pSrc[2*j*dim1+1]; \
      } \
      \
      fft_func2(pDst); \
      \
      for(j=0; j<dim2; j++) \
      { \
        pDstOut[2*j*dim1]   = pDst[2*j]; \
        pDstOut[2*j*dim1+1] = pDst[2*j+1]; \
      } \
      pSrc += 2; \
      pDstOut += 2; \
    } \
  } \
  \
  C_ALLOC_SCRATCH_END(aDst2, FIXP_DBL, dim2*2); \
  C_ALLOC_SCRATCH_END(aDst, FIXP_DBL, length*2); \
} \

#else /* FFT_TWO_STAGE_MACRO_ENABLE */

/* select either switch case of function pointer. */
//#define FFT_TWO_STAGE_SWITCH_CASE

static inline void fftN2(
        FIXP_DBL *pInput,
        const int length,
        const int dim1,
        const int dim2,
        void (* const fft1)(FIXP_DBL *),
        void (* const fft2)(FIXP_DBL *),
        const FIXP_STB *RotVectorReal,
        const FIXP_STB *RotVectorImag
        )
{
  /* The real part of the input samples are at the addresses with even indices and the imaginary
  part of the input samples are at the addresses with odd indices. The output samples are stored
  at the address of pInput
  */
  FIXP_DBL  *pSrc, *pDst, *pDstOut;
  int       i, j;

  C_ALLOC_SCRATCH_START(aDst, FIXP_DBL, length*2);
  C_ALLOC_SCRATCH_START(aDst2, FIXP_DBL, dim2*2);
     
  FDK_ASSERT(length == dim1*dim2);

  /* Perform dim2 times the fft of length dim1. The input samples are at the address of pSrc and the
  output samples are at the address of pDst. The input vector for the fft of length dim1 is built
  of the interleaved samples in pSrc, the output samples are stored consecutively.
  */
  pSrc = pInput;
  pDst = aDst;
  for(i=0; i<length/dim1; i++)
  {
    for(j=0; j<length/dim2; j++)
    {
      pDst[2*j]   = pSrc[2*j*dim2];
      pDst[2*j+1] = pSrc[2*j*dim2+1];
    }

    /* fft of size dim1 */
#ifndef FFT_TWO_STAGE_SWITCH_CASE
    fft1(pDst);
#else
    switch (dim1) {
      case 3: fft3(pDst); break;
      case 4: fft_4(pDst); break;
      case 5: fft5(pDst); break;
      case 8: fft_8(pDst); break;
      case 15: fft15(pDst); break;
      case 16: fft_16(pDst); break;
      case 32: fft_32(pDst); break;
      /*case 64: fft_64(pDst); break;*/
      case 128: fft_128(pDst); break;
    }
#endif
    pSrc += 2;
    pDst = pDst + 2*length/dim2;
  }

  /* Perform the modulation of the output of the fft of length dim1 */
  pSrc=aDst;
  fft_apply_rot_vector(pSrc, length/dim2, length, RotVectorReal, RotVectorImag);

  /* Perform dim1 times the fft of length dim2. The input samples are at the address of aDst and the
  output samples are at the address of pInput. The input vector for the fft of length dim2 is built
  of the interleaved samples in aDst, the output samples are stored consecutively at the address
  of pInput.
  */
  pSrc    = aDst;
  pDst    = aDst2;
  pDstOut = pInput;
  for(i=0; i<length/dim2; i++)
  {
    for(j=0; j<length/dim1; j++)
    {
      pDst[2*j]   = pSrc[2*j*dim1];
      pDst[2*j+1] = pSrc[2*j*dim1+1];
    }

#ifndef FFT_TWO_STAGE_SWITCH_CASE
    fft2(pDst);
#else
    switch (dim2) {
      case 3: fft3(pDst); break;
      case 4: fft_4(pDst); break;
      case 5: fft5(pDst); break;
      case 8: fft_8(pDst); break;
      case 15: fft15(pDst); break;
      case 16: fft_16(pDst); break;
      case 32: fft_32(pDst); break;
      /*case 64: fft_64(pDst); break;*/
      case 128: fft_128(pDst); break;
    }
#endif

    for(j=0; j<length/dim1; j++)
    {
      pDstOut[2*j*dim1]   = pDst[2*j];
      pDstOut[2*j*dim1+1] = pDst[2*j+1];
    }
    pSrc += 2;
    pDstOut += 2;
  }

  C_ALLOC_SCRATCH_END(aDst2, FIXP_DBL, dim2*2);
  C_ALLOC_SCRATCH_END(aDst, FIXP_DBL, length*2);
}

#endif /* FFT_TWO_STAGE_MACRO_ENABLE */












#define SCALEFACTOR60         5
/**
The function performs the fft of length 60. It is splittet into fft's of length 4 and fft's of
length 15. Between the fft's a modolation is calculated.
*/
static inline void fft60(FIXP_DBL *pInput, INT *pScalefactor)
{
  fftN2(
          pInput, 60, 4, 15, 
          fft_4, fft15,
          RotVectorReal60, RotVectorImag60
          );
  *pScalefactor += SCALEFACTOR60;
}



/* Fallback implementation in case of no better implementation available. */

#define SCALEFACTOR240        7

/**
The function performs the fft of length 240. It is splittet into fft's of length 16 and fft's of
length 15. Between the fft's a modulation is calculated.
*/
static inline void fft240(FIXP_DBL *pInput, INT *pScalefactor)
{
  fftN2(
          pInput, 240, 16, 15, 
          fft_16, fft15,
          RotVectorReal240, RotVectorImag240
          );
  *pScalefactor += SCALEFACTOR240;
}


#define SCALEFACTOR480        8
#define N32                   32
#define TABLE_SIZE_16        (32/2)

/**
The function performs the fft of length 480. It is splittet into fft's of length 32 and fft's of
length 15. Between the fft's a modulation is calculated.
*/
static inline void fft480(FIXP_DBL *pInput, INT *pScalefactor)
{
  fftN2(
          pInput, 480, 32, 15, 
          fft_32, fft15,
          RotVectorReal480, RotVectorImag480
          );
  *pScalefactor += SCALEFACTOR480;
}

void fft(int length, FIXP_DBL *pInput, INT *pScalefactor)
{
  if (length == 32)
  {
      fft_32(pInput);
      *pScalefactor += SCALEFACTOR32;
  }
  else
  {
  
  switch (length) {
    case 16:
      fft_16(pInput);
      *pScalefactor += SCALEFACTOR16;
      break;
    case 8:
      fft_8(pInput);
      *pScalefactor += SCALEFACTOR8;
      break;
    case 3:
      fft3(pInput);
      break;
    case 4:
      fft_4(pInput);
      *pScalefactor += SCALEFACTOR4;
      break;
    case 5:
      fft5(pInput);
      break;
    case 15:
      fft15(pInput);
      *pScalefactor += 2;
      break;
    case 60:
      fft60(pInput, pScalefactor);
      break;
    case 64:
      dit_fft(pInput, 6, SineTable512, 512);
      *pScalefactor += SCALEFACTOR64;
      break;
    case 240:
      fft240(pInput, pScalefactor);
      break;
    case 256:
      dit_fft(pInput, 8, SineTable512, 512);
      *pScalefactor += SCALEFACTOR256;
      break;
    case 480:
      fft480(pInput, pScalefactor);
      break;
    case 512:
      dit_fft(pInput, 9, SineTable512, 512);
      *pScalefactor += SCALEFACTOR512;
      break;
    default:
      FDK_ASSERT(0); /* FFT length not supported! */
      break;
  }
  }
}


void ifft(int length, FIXP_DBL *pInput, INT *scalefactor)
{
  switch (length) {
    default:
      FDK_ASSERT(0); /* IFFT length not supported! */
      break;
  }
}


