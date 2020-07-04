/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2018 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software
that implements the MPEG Advanced Audio Coding ("AAC") encoding and decoding
scheme for digital audio. This FDK AAC Codec software is intended to be used on
a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient
general perceptual audio codecs. AAC-ELD is considered the best-performing
full-bandwidth communications codec by independent studies and is widely
deployed. AAC has been standardized by ISO and IEC as part of the MPEG
specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including
those of Fraunhofer) may be obtained through Via Licensing
(www.vialicensing.com) or through the respective patent owners individually for
the purpose of encoding or decoding bit streams in products that are compliant
with the ISO/IEC MPEG audio standards. Please note that most manufacturers of
Android devices already license these patent claims through Via Licensing or
directly from the patent owners, and therefore FDK AAC Codec software may
already be covered under those patent licenses when it is used for those
licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions
with enhanced sound quality, are also available from Fraunhofer. Users are
encouraged to check the Fraunhofer website for additional applications
information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK AAC Codec or your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK AAC Codec or
your modifications thereto in binary form. You must make available free of
charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this library without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK AAC Codec software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating
that you changed the software and the date of any change. For modified versions
of the FDK AAC Codec, the term "Fraunhofer FDK AAC Codec Library for Android"
must be replaced by the term "Third-Party Modified Version of the Fraunhofer FDK
AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software.

You may use this FDK AAC Codec software or modifications thereto only for
purposes that are authorized by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright
holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
including but not limited to the implied warranties of merchantability and
fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary,
or consequential damages, including but not limited to procurement of substitute
goods or services; loss of use, data, or profits, or business interruption,
however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of
this software, even if advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

/*********************** MPEG surround decoder library *************************

   Author(s):

   Description: SAC Dec tables

*******************************************************************************/

#include "sac_rom.h"
#include "sac_calcM1andM2.h"

#define SCALE_CPC(a) (FL2FXCONST_CFG(a / (float)(1 << SCALE_PARAM_M1)))
const FIXP_CFG dequantCPC__FDK[] = {
    SCALE_CPC(-2.0f), SCALE_CPC(-1.9f), SCALE_CPC(-1.8f), SCALE_CPC(-1.7f),
    SCALE_CPC(-1.6f), SCALE_CPC(-1.5f), SCALE_CPC(-1.4f), SCALE_CPC(-1.3f),
    SCALE_CPC(-1.2f), SCALE_CPC(-1.1f), SCALE_CPC(-1.0f), SCALE_CPC(-0.9f),
    SCALE_CPC(-0.8f), SCALE_CPC(-0.7f), SCALE_CPC(-0.6f), SCALE_CPC(-0.5f),
    SCALE_CPC(-0.4f), SCALE_CPC(-0.3f), SCALE_CPC(-0.2f), SCALE_CPC(-0.1f),
    SCALE_CPC(0.0f),  SCALE_CPC(0.1f),  SCALE_CPC(0.2f),  SCALE_CPC(0.3f),
    SCALE_CPC(0.4f),  SCALE_CPC(0.5f),  SCALE_CPC(0.6f),  SCALE_CPC(0.7f),
    SCALE_CPC(0.8f),  SCALE_CPC(0.9f),  SCALE_CPC(1.0f),  SCALE_CPC(1.1f),
    SCALE_CPC(1.2f),  SCALE_CPC(1.3f),  SCALE_CPC(1.4f),  SCALE_CPC(1.5f),
    SCALE_CPC(1.6f),  SCALE_CPC(1.7f),  SCALE_CPC(1.8f),  SCALE_CPC(1.9f),
    SCALE_CPC(2.0f),  SCALE_CPC(2.1f),  SCALE_CPC(2.2f),  SCALE_CPC(2.3f),
    SCALE_CPC(2.4f),  SCALE_CPC(2.5f),  SCALE_CPC(2.6f),  SCALE_CPC(2.7f),
    SCALE_CPC(2.8f),  SCALE_CPC(2.9f),  SCALE_CPC(3.0f)};

#define SCALE_ICC(a) (FL2FXCONST_CFG(a))
const FIXP_CFG dequantICC__FDK[8] = {
    /*SCALE_ICC(1.00000f)*/ FX_DBL2FX_CFG(MAXVAL_DBL),
    SCALE_ICC(0.9370f),
    SCALE_ICC(0.84118f),
    SCALE_ICC(0.60092f),
    SCALE_ICC(0.36764f),
    SCALE_ICC(0.0000f),
    SCALE_ICC(-0.58900f),
    SCALE_ICC(-0.9900f)};

#define SCALE_CLD2(a) (FL2FXCONST_CFG(a / (float)(1 << 8)))
const FIXP_CFG dequantCLD__FDK[31] = {
    SCALE_CLD2(-150.0f), SCALE_CLD2(-45.0f), SCALE_CLD2(-40.0f),
    SCALE_CLD2(-35.0f),  SCALE_CLD2(-30.0f), SCALE_CLD2(-25.0f),
    SCALE_CLD2(-22.0f),  SCALE_CLD2(-19.0f), SCALE_CLD2(-16.0f),
    SCALE_CLD2(-13.0f),  SCALE_CLD2(-10.0f), SCALE_CLD2(-8.0f),
    SCALE_CLD2(-6.0f),   SCALE_CLD2(-4.0f),  SCALE_CLD2(-2.0f),
    SCALE_CLD2(0.0f),    SCALE_CLD2(2.0f),   SCALE_CLD2(4.0f),
    SCALE_CLD2(6.0f),    SCALE_CLD2(8.0f),   SCALE_CLD2(10.0f),
    SCALE_CLD2(13.0f),   SCALE_CLD2(16.0f),  SCALE_CLD2(19.0f),
    SCALE_CLD2(22.0f),   SCALE_CLD2(25.0f),  SCALE_CLD2(30.0f),
    SCALE_CLD2(35.0f),   SCALE_CLD2(40.0f),  SCALE_CLD2(45.0f),
    SCALE_CLD2(150.0f)};

#define SCALE_IPD(a) (FL2FXCONST_CFG(a / (float)(1 << IPD_SCALE)))
const FIXP_CFG dequantIPD__FDK[16] = {
    /* SCALE_IPD(0.000000000f), SCALE_IPD(0.392699082f),
       SCALE_IPD(0.785398163f), SCALE_IPD(1.178097245f),
       SCALE_IPD(1.570796327f), SCALE_IPD(1.963495408f),
       SCALE_IPD(2.356194490f), SCALE_IPD(2.748893572f),
       SCALE_IPD(3.141592654f), SCALE_IPD(3.534291735f),
       SCALE_IPD(3.926990817f), SCALE_IPD(4.319689899f),
       SCALE_IPD(4.712388980f), SCALE_IPD(5.105088062f),
       SCALE_IPD(5.497787144f), SCALE_IPD(5.890486225f) */
    SCALE_IPD(0.00000000000000f), SCALE_IPD(0.392699082f),
    SCALE_IPD(0.78539816339745f), SCALE_IPD(1.178097245f),
    SCALE_IPD(1.57079632679490f), SCALE_IPD(1.963495408f),
    SCALE_IPD(2.35619449019234f), SCALE_IPD(2.748893572f),
    SCALE_IPD(3.14159265358979f), SCALE_IPD(3.534291735f),
    SCALE_IPD(3.92699081698724f), SCALE_IPD(4.319689899f),
    SCALE_IPD(4.71238898038469f), SCALE_IPD(5.105088062f),
    SCALE_IPD(5.49778714378214f), SCALE_IPD(5.890486225f)};

#define SCALE_SPLIT_ANGLE(a) (FL2FXCONST_CFG(a / (float)(1 << IPD_SCALE)))
/*
  Generate table dequantIPD_CLD_ICC_splitAngle__FDK[16][31][8]:

  #define ABS_THR                         ( 1e-9f * 32768 * 32768 )

  float dequantICC[] =
  {1.0000f,0.9370f,0.84118f,0.60092f,0.36764f,0.0f,-0.5890f,-0.9900f}; float
  dequantCLD[] =
  {-150.0,-45.0,-40.0,-35.0,-30.0,-25.0,-22.0,-19.0,-16.0,-13.0,-10.0, -8.0,
                          -6.0, -4.0, -2.0,  0.0,  2.0,  4.0,  6.0,  8.0,
  10.0, 13.0, 16.0, 19.0, 22.0, 25.0, 30.0, 35.0, 40.0, 45.0, 150.0 }; float
  dequantIPD[] =
  {0.f,0.392699082f,0.785398163f,1.178097245f,1.570796327f,1.963495408f,
                        2.35619449f,2.748893572f,3.141592654f,3.534291735f,3.926990817f,
                        4.319689899f,4.71238898f,5.105088062f,5.497787144f,5.890486225f};

  for (ipdIdx=0; ipdIdx<16; ipdIdx++)
    for (cldIdx=0; cldIdx<31; cldIdx++)
      for (iccIdx=0; iccIdx<8; iccIdx++) {
        ipd = dequantIPD[ipdIdx];
        cld = dequantCLD[cldIdx];
        icc = dequantICC[iccIdx];
        iidLin = (float) pow(10.0f, cld / 20.0f);
        iidLin2 = iidLin * iidLin;
        iidLin21 = iidLin2 + 1.0f;
        sinIpd = (float) sin(ipd);
        cosIpd = (float) cos(ipd);
        temp1 = 2.0f * icc * iidLin;
        temp1c = temp1 * cosIpd;
        ratio = (iidLin21 + temp1c) / (iidLin21 + temp1) + ABS_THR;
        w2 = (float) pow(ratio, 0.25f);
        w1 = 2.0f - w2;
        dequantIPD_CLD_ICC_splitAngle__FDK[ipdIdx][cldIdx][iccIdx] = (float)
  atan2(w2 * sinIpd, w1 * iidLin + w2 * cosIpd);
      }
*/

#define SCALE_CLD(a) (FL2FXCONST_CFG(a))

const FIXP_CFG dequantCLD_c_l[31] = {
    SCALE_CLD(0.0000000316f),
    SCALE_CLD(0.0056233243f),
    SCALE_CLD(0.0099994997f),
    SCALE_CLD(0.0177799836f),
    SCALE_CLD(0.0316069759f),
    SCALE_CLD(0.0561454296f),
    SCALE_CLD(0.0791834071f),
    SCALE_CLD(0.1115021780f),
    SCALE_CLD(0.1565355062f),
    SCALE_CLD(0.2184644639f),
    SCALE_CLD(0.3015113473f),
    SCALE_CLD(0.3698741496f),
    SCALE_CLD(0.4480624795f),
    SCALE_CLD(0.5336171389f),
    SCALE_CLD(0.6219832301f),
    SCALE_CLD(0.7071067691f),
    SCALE_CLD(0.7830305696f),
    SCALE_CLD(0.8457261920f),
    SCALE_CLD(0.8940021992f),
    SCALE_CLD(0.9290818572f),
    SCALE_CLD(0.9534626007f),
    SCALE_CLD(0.9758449197f),
    SCALE_CLD(0.9876723289f),
    SCALE_CLD(0.9937641621f),
    SCALE_CLD(0.9968600869f),
    SCALE_CLD(0.9984226227f),
    SCALE_CLD(0.9995003939f),
    SCALE_CLD(0.9998419285f),
    SCALE_CLD(0.9999499917f),
    SCALE_CLD(0.9999842048f),
    /*SCALE_CLD(1.0000000000f)*/ FX_DBL2FX_CFG(MAXVAL_DBL)};

#define SC_H(a) (FL2FXCONST_CFG(a))
#define DATA_TYPE_H FIXP_CFG

/* not correlated  tables */
const DATA_TYPE_H H11_nc[31][8] = {
    {SC_H(0.0000000316f), SC_H(0.0000000296f), SC_H(0.0000000266f),
     SC_H(0.0000000190f), SC_H(0.0000000116f), SC_H(0.0000000000f),
     SC_H(-0.0000000186f), SC_H(-0.0000000313f)},
    {SC_H(0.0056233243f), SC_H(0.0052728835f), SC_H(0.0047394098f),
     SC_H(0.0033992692f), SC_H(0.0020946222f), SC_H(0.0000316215f),
     SC_H(-0.0032913829f), SC_H(-0.0055664564f)},
    {SC_H(0.0099994997f), SC_H(0.0093815643f), SC_H(0.0084402543f),
     SC_H(0.0060722125f), SC_H(0.0037622179f), SC_H(0.0000999898f),
     SC_H(-0.0058238208f), SC_H(-0.0098974844f)},
    {SC_H(0.0177799836f), SC_H(0.0166974831f), SC_H(0.0150465844f),
     SC_H(0.0108831404f), SC_H(0.0068073822f), SC_H(0.0003161267f),
     SC_H(-0.0102626514f), SC_H(-0.0175957214f)},
    {SC_H(0.0316069759f), SC_H(0.0297324844f), SC_H(0.0268681273f),
     SC_H(0.0196138974f), SC_H(0.0124691967f), SC_H(0.0009989988f),
     SC_H(-0.0179452803f), SC_H(-0.0312700421f)},
    {SC_H(0.0561454296f), SC_H(0.0529650487f), SC_H(0.0480896905f),
     SC_H(0.0356564634f), SC_H(0.0232860073f), SC_H(0.0031523081f),
     SC_H(-0.0309029408f), SC_H(-0.0555154830f)},
    {SC_H(0.0791834071f), SC_H(0.0748842582f), SC_H(0.0682762116f),
     SC_H(0.0513241664f), SC_H(0.0343080349f), SC_H(0.0062700072f),
     SC_H(-0.0422340371f), SC_H(-0.0782499388f)},
    {SC_H(0.1115021780f), SC_H(0.1057924852f), SC_H(0.0969873071f),
     SC_H(0.0742305145f), SC_H(0.0511277616f), SC_H(0.0124327289f),
     SC_H(-0.0566596612f), SC_H(-0.1100896299f)},
    {SC_H(0.1565355062f), SC_H(0.1491366178f), SC_H(0.1376826316f),
     SC_H(0.1078186408f), SC_H(0.0770794004f), SC_H(0.0245033558f),
     SC_H(-0.0735980421f), SC_H(-0.1543303132f)},
    {SC_H(0.2184644639f), SC_H(0.2091979682f), SC_H(0.1947948188f),
     SC_H(0.1568822265f), SC_H(0.1172478944f), SC_H(0.0477267131f),
     SC_H(-0.0899507254f), SC_H(-0.2148526460f)},
    {SC_H(0.3015113473f), SC_H(0.2904391289f), SC_H(0.2731673419f),
     SC_H(0.2273024023f), SC_H(0.1786239147f), SC_H(0.0909090787f),
     SC_H(-0.0964255333f), SC_H(-0.2951124907f)},
    {SC_H(0.3698741496f), SC_H(0.3578284085f), SC_H(0.3390066922f),
     SC_H(0.2888108492f), SC_H(0.2351117432f), SC_H(0.1368068755f),
     SC_H(-0.0850296095f), SC_H(-0.3597966135f)},
    {SC_H(0.4480624795f), SC_H(0.4354025424f), SC_H(0.4156077504f),
     SC_H(0.3627120256f), SC_H(0.3058823943f), SC_H(0.2007599771f),
     SC_H(-0.0484020934f), SC_H(-0.4304940701f)},
    {SC_H(0.5336171389f), SC_H(0.5208471417f), SC_H(0.5008935928f),
     SC_H(0.4476420581f), SC_H(0.3905044496f), SC_H(0.2847472429f),
     SC_H(0.0276676007f), SC_H(-0.4966579080f)},
    {SC_H(0.6219832301f), SC_H(0.6096963882f), SC_H(0.5905415416f),
     SC_H(0.5396950245f), SC_H(0.4856070578f), SC_H(0.3868631124f),
     SC_H(0.1531652957f), SC_H(-0.5045361519f)},
    {SC_H(0.7071067691f), SC_H(0.6958807111f), SC_H(0.6784504056f),
     SC_H(0.6326373219f), SC_H(0.5847306848f), SC_H(0.4999999702f),
     SC_H(0.3205464482f), SC_H(0.0500000045f)},
    {SC_H(0.7830305696f), SC_H(0.7733067870f), SC_H(0.7582961321f),
     SC_H(0.7194055915f), SC_H(0.6797705293f), SC_H(0.6131368876f),
     SC_H(0.4997332692f), SC_H(0.6934193969f)},
    {SC_H(0.8457261920f), SC_H(0.8377274871f), SC_H(0.8254694939f),
     SC_H(0.7942851782f), SC_H(0.7635439038f), SC_H(0.7152527571f),
     SC_H(0.6567122936f), SC_H(0.8229061961f)},
    {SC_H(0.8940021992f), SC_H(0.8877248168f), SC_H(0.8781855106f),
     SC_H(0.8544237614f), SC_H(0.8318918347f), SC_H(0.7992399335f),
     SC_H(0.7751275301f), SC_H(0.8853276968f)},
    {SC_H(0.9290818572f), SC_H(0.9243524075f), SC_H(0.9172304869f),
     SC_H(0.8998877406f), SC_H(0.8841174841f), SC_H(0.8631930947f),
     SC_H(0.8565139771f), SC_H(0.9251161218f)},
    {SC_H(0.9534626007f), SC_H(0.9500193000f), SC_H(0.9448821545f),
     SC_H(0.9326565266f), SC_H(0.9220023751f), SC_H(0.9090909362f),
     SC_H(0.9096591473f), SC_H(0.9514584541f)},
    {SC_H(0.9758449197f), SC_H(0.9738122821f), SC_H(0.9708200693f),
     SC_H(0.9639287591f), SC_H(0.9582763910f), SC_H(0.9522733092f),
     SC_H(0.9553207159f), SC_H(0.9750427008f)},
    {SC_H(0.9876723289f), SC_H(0.9865267277f), SC_H(0.9848603010f),
     SC_H(0.9811310172f), SC_H(0.9782302976f), SC_H(0.9754966497f),
     SC_H(0.9779621363f), SC_H(0.9873252511f)},
    {SC_H(0.9937641621f), SC_H(0.9931397438f), SC_H(0.9922404289f),
     SC_H(0.9902750254f), SC_H(0.9888116717f), SC_H(0.9875672460f),
     SC_H(0.9891131520f), SC_H(0.9936066866f)},
    {SC_H(0.9968600869f), SC_H(0.9965277910f), SC_H(0.9960530400f),
     SC_H(0.9950347543f), SC_H(0.9943022728f), SC_H(0.9937300086f),
     SC_H(0.9946073294f), SC_H(0.9967863560f)},
    {SC_H(0.9984226227f), SC_H(0.9982488155f), SC_H(0.9980020523f),
     SC_H(0.9974802136f), SC_H(0.9971146584f), SC_H(0.9968476892f),
     SC_H(0.9973216057f), SC_H(0.9983873963f)},
    {SC_H(0.9995003939f), SC_H(0.9994428754f), SC_H(0.9993617535f),
     SC_H(0.9991930723f), SC_H(0.9990783334f), SC_H(0.9990010262f),
     SC_H(0.9991616607f), SC_H(0.9994897842f)},
    {SC_H(0.9998419285f), SC_H(0.9998232722f), SC_H(0.9997970462f),
     SC_H(0.9997430444f), SC_H(0.9997069836f), SC_H(0.9996838570f),
     SC_H(0.9997364879f), SC_H(0.9998386502f)},
    {SC_H(0.9999499917f), SC_H(0.9999440312f), SC_H(0.9999356270f),
     SC_H(0.9999184012f), SC_H(0.9999070764f), SC_H(0.9998999834f),
     SC_H(0.9999169707f), SC_H(0.9999489784f)},
    {SC_H(0.9999842048f), SC_H(0.9999822974f), SC_H(0.9999796152f),
     SC_H(0.9999741912f), SC_H(0.9999706149f), SC_H(0.9999684095f),
     SC_H(0.9999738336f), SC_H(0.9999839067f)},
    /* { SC_H( 1.0000000000f), SC_H( 1.0000000000f), SC_H( 1.0000000000f),
       SC_H( 1.0000000000f), SC_H( 1.0000000000f), SC_H( 1.0000000000f),
       SC_H( 1.0000000000f), SC_H( 1.0000000000f)} */
    {FX_DBL2FX_CFG(MAXVAL_DBL), FX_DBL2FX_CFG(MAXVAL_DBL),
     FX_DBL2FX_CFG(MAXVAL_DBL), FX_DBL2FX_CFG(MAXVAL_DBL),
     FX_DBL2FX_CFG(MAXVAL_DBL), FX_DBL2FX_CFG(MAXVAL_DBL),
     FX_DBL2FX_CFG(MAXVAL_DBL), FX_DBL2FX_CFG(MAXVAL_DBL)}};
const DATA_TYPE_H H12_nc[31][8] = {
    {SC_H(0.0000000000f), SC_H(0.0000000110f), SC_H(0.0000000171f),
     SC_H(0.0000000253f), SC_H(0.0000000294f), SC_H(0.0000000316f),
     SC_H(0.0000000256f), SC_H(0.0000000045f)},
    {SC_H(0.0000000000f), SC_H(0.0019540924f), SC_H(0.0030265113f),
     SC_H(0.0044795922f), SC_H(0.0052186525f), SC_H(0.0056232354f),
     SC_H(0.0045594489f), SC_H(0.0007977085f)},
    {SC_H(0.0000000000f), SC_H(0.0034606720f), SC_H(0.0053620986f),
     SC_H(0.0079446984f), SC_H(0.0092647560f), SC_H(0.0099989995f),
     SC_H(0.0081285369f), SC_H(0.0014247064f)},
    {SC_H(0.0000000000f), SC_H(0.0061091618f), SC_H(0.0094724922f),
     SC_H(0.0140600521f), SC_H(0.0164252054f), SC_H(0.0177771728f),
     SC_H(0.0145191532f), SC_H(0.0025531140f)},
    {SC_H(0.0000000000f), SC_H(0.0107228858f), SC_H(0.0166464616f),
     SC_H(0.0247849934f), SC_H(0.0290434174f), SC_H(0.0315911844f),
     SC_H(0.0260186065f), SC_H(0.0046027615f)},
    {SC_H(0.0000000000f), SC_H(0.0186282862f), SC_H(0.0289774220f),
     SC_H(0.0433696397f), SC_H(0.0510888547f), SC_H(0.0560568646f),
     SC_H(0.0468755551f), SC_H(0.0083869267f)},
    {SC_H(0.0000000000f), SC_H(0.0257363543f), SC_H(0.0401044972f),
     SC_H(0.0602979437f), SC_H(0.0713650510f), SC_H(0.0789347738f),
     SC_H(0.0669798329f), SC_H(0.0121226767f)},
    {SC_H(0.0000000000f), SC_H(0.0352233723f), SC_H(0.0550108925f),
     SC_H(0.0832019597f), SC_H(0.0990892947f), SC_H(0.1108068749f),
     SC_H(0.0960334241f), SC_H(0.0176920593f)},
    {SC_H(0.0000000000f), SC_H(0.0475566536f), SC_H(0.0744772255f),
     SC_H(0.1134835035f), SC_H(0.1362429112f), SC_H(0.1546057910f),
     SC_H(0.1381545961f), SC_H(0.0261824392f)},
    {SC_H(0.0000000000f), SC_H(0.0629518181f), SC_H(0.0989024863f),
     SC_H(0.1520351619f), SC_H(0.1843357086f), SC_H(0.2131874412f),
     SC_H(0.1990868896f), SC_H(0.0395608991f)},
    {SC_H(0.0000000000f), SC_H(0.0809580907f), SC_H(0.1276271492f),
     SC_H(0.1980977356f), SC_H(0.2429044843f), SC_H(0.2874797881f),
     SC_H(0.2856767476f), SC_H(0.0617875643f)},
    {SC_H(0.0000000000f), SC_H(0.0936254337f), SC_H(0.1479234397f),
     SC_H(0.2310739607f), SC_H(0.2855334580f), SC_H(0.3436433673f),
     SC_H(0.3599678576f), SC_H(0.0857512727f)},
    {SC_H(0.0000000000f), SC_H(0.1057573780f), SC_H(0.1674221754f),
     SC_H(0.2630588412f), SC_H(0.3274079263f), SC_H(0.4005688727f),
     SC_H(0.4454404712f), SC_H(0.1242370531f)},
    {SC_H(0.0000000000f), SC_H(0.1160409302f), SC_H(0.1839915067f),
     SC_H(0.2904545665f), SC_H(0.3636667728f), SC_H(0.4512939751f),
     SC_H(0.5328993797f), SC_H(0.1951362640f)},
    {SC_H(0.0000000000f), SC_H(0.1230182052f), SC_H(0.1952532977f),
     SC_H(0.3091802597f), SC_H(0.3886501491f), SC_H(0.4870318770f),
     SC_H(0.6028295755f), SC_H(0.3637395203f)},
    {SC_H(0.0000000000f), SC_H(0.1254990250f), SC_H(0.1992611140f),
     SC_H(0.3158638775f), SC_H(0.3976053298f), SC_H(0.5000000000f),
     SC_H(0.6302776933f), SC_H(0.7053368092f)},
    {SC_H(0.0000000000f), SC_H(0.1230182052f), SC_H(0.1952533126f),
     SC_H(0.3091802597f), SC_H(0.3886501491f), SC_H(0.4870319068f),
     SC_H(0.6028295755f), SC_H(0.3637394905f)},
    {SC_H(0.0000000000f), SC_H(0.1160409302f), SC_H(0.1839915216f),
     SC_H(0.2904545665f), SC_H(0.3636668026f), SC_H(0.4512939751f),
     SC_H(0.5328993797f), SC_H(0.1951362044f)},
    {SC_H(0.0000000000f), SC_H(0.1057573855f), SC_H(0.1674221754f),
     SC_H(0.2630588710f), SC_H(0.3274079263f), SC_H(0.4005688727f),
     SC_H(0.4454405010f), SC_H(0.1242370382f)},
    {SC_H(0.0000000000f), SC_H(0.0936254337f), SC_H(0.1479234397f),
     SC_H(0.2310739607f), SC_H(0.2855334580f), SC_H(0.3436433673f),
     SC_H(0.3599678576f), SC_H(0.0857512653f)},
    {SC_H(0.0000000000f), SC_H(0.0809580907f), SC_H(0.1276271492f),
     SC_H(0.1980977207f), SC_H(0.2429044843f), SC_H(0.2874797881f),
     SC_H(0.2856767476f), SC_H(0.0617875606f)},
    {SC_H(0.0000000000f), SC_H(0.0629518107f), SC_H(0.0989024863f),
     SC_H(0.1520351619f), SC_H(0.1843357235f), SC_H(0.2131874412f),
     SC_H(0.1990868896f), SC_H(0.0395609401f)},
    {SC_H(0.0000000000f), SC_H(0.0475566462f), SC_H(0.0744772255f),
     SC_H(0.1134835184f), SC_H(0.1362429112f), SC_H(0.1546057761f),
     SC_H(0.1381545961f), SC_H(0.0261824802f)},
    {SC_H(0.0000000000f), SC_H(0.0352233797f), SC_H(0.0550108962f),
     SC_H(0.0832019448f), SC_H(0.0990892798f), SC_H(0.1108068526f),
     SC_H(0.0960334465f), SC_H(0.0176920686f)},
    {SC_H(0.0000000000f), SC_H(0.0257363524f), SC_H(0.0401044935f),
     SC_H(0.0602979474f), SC_H(0.0713650808f), SC_H(0.0789347589f),
     SC_H(0.0669797957f), SC_H(0.0121226516f)},
    {SC_H(0.0000000000f), SC_H(0.0186282881f), SC_H(0.0289774258f),
     SC_H(0.0433696248f), SC_H(0.0510888547f), SC_H(0.0560568906f),
     SC_H(0.0468755886f), SC_H(0.0083869714f)},
    {SC_H(0.0000000000f), SC_H(0.0107228830f), SC_H(0.0166464727f),
     SC_H(0.0247849822f), SC_H(0.0290434249f), SC_H(0.0315911621f),
     SC_H(0.0260186475f), SC_H(0.0046027377f)},
    {SC_H(0.0000000000f), SC_H(0.0061091576f), SC_H(0.0094724894f),
     SC_H(0.0140600465f), SC_H(0.0164251942f), SC_H(0.0177771524f),
     SC_H(0.0145191504f), SC_H(0.0025530567f)},
    {SC_H(0.0000000000f), SC_H(0.0034606743f), SC_H(0.0053620976f),
     SC_H(0.0079446994f), SC_H(0.0092647672f), SC_H(0.0099990256f),
     SC_H(0.0081285043f), SC_H(0.0014247177f)},
    {SC_H(0.0000000000f), SC_H(0.0019540912f), SC_H(0.0030265225f),
     SC_H(0.0044795908f), SC_H(0.0052186381f), SC_H(0.0056232223f),
     SC_H(0.0045594289f), SC_H(0.0007977359f)},
    {SC_H(0.0000000000f), SC_H(0.0000000149f), SC_H(0.0000000298f),
     SC_H(0.0000000298f), SC_H(0.0000000000f), SC_H(0.0000000596f),
     SC_H(0.0000000000f), SC_H(0.0000000000f)}};

/*
  for (i=0; i<31; i++) {
    cld = dequantCLD[i];
    val = (float)(FDKexp(cld/dbe)/(1+FDKexp(cld/dbe)));
    val = (float)(dbe*FDKlog(val));
  }
*/
#define SCALE_CLD_C1C2(a) (FL2FXCONST_DBL(a / (float)(1 << SF_CLD_C1C2)))
const FIXP_DBL dequantCLD_c1[31] = {SCALE_CLD_C1C2(-1.5000000000000000e+002f),
                                    SCALE_CLD_C1C2(-4.5000137329101563e+001f),
                                    SCALE_CLD_C1C2(-4.0000434875488281e+001f),
                                    SCALE_CLD_C1C2(-3.5001373291015625e+001f),
                                    SCALE_CLD_C1C2(-3.0004341125488281e+001f),
                                    SCALE_CLD_C1C2(-2.5013711929321289e+001f),
                                    SCALE_CLD_C1C2(-2.2027315139770508e+001f),
                                    SCALE_CLD_C1C2(-1.9054332733154297e+001f),
                                    SCALE_CLD_C1C2(-1.6107742309570313e+001f),
                                    SCALE_CLD_C1C2(-1.3212384223937988e+001f),
                                    SCALE_CLD_C1C2(-1.0413927078247070e+001f),
                                    SCALE_CLD_C1C2(-8.6389207839965820e+000f),
                                    SCALE_CLD_C1C2(-6.9732279777526855e+000f),
                                    SCALE_CLD_C1C2(-5.4554042816162109e+000f),
                                    SCALE_CLD_C1C2(-4.1244258880615234e+000f),
                                    SCALE_CLD_C1C2(-3.0102999210357666e+000f),
                                    SCALE_CLD_C1C2(-2.1244258880615234e+000f),
                                    SCALE_CLD_C1C2(-1.4554045200347900e+000f),
                                    SCALE_CLD_C1C2(-9.7322785854339600e-001f),
                                    SCALE_CLD_C1C2(-6.3892036676406860e-001f),
                                    SCALE_CLD_C1C2(-4.1392669081687927e-001f),
                                    SCALE_CLD_C1C2(-2.1238386631011963e-001f),
                                    SCALE_CLD_C1C2(-1.0774217545986176e-001f),
                                    SCALE_CLD_C1C2(-5.4333221167325974e-002f),
                                    SCALE_CLD_C1C2(-2.7315950021147728e-002f),
                                    SCALE_CLD_C1C2(-1.3711934909224510e-002f),
                                    SCALE_CLD_C1C2(-4.3406565673649311e-003f),
                                    SCALE_CLD_C1C2(-1.3732088264077902e-003f),
                                    SCALE_CLD_C1C2(-4.3438826105557382e-004f),
                                    SCALE_CLD_C1C2(-1.3745666365139186e-004f),
                                    SCALE_CLD_C1C2(0.0000000000000000e+000f)};

/* sac_stp */
/* none scaled */
const FIXP_CFG BP__FDK[] = {FL2FXCONST_CFG(0.73919999599457),
                            FL2FXCONST_CFG(0.97909998893738),
                            FL2FXCONST_CFG(0.99930000305176)};

/* scaled with 26 bits */
const FIXP_CFG BP_GF__FDK[] = {
    FL2FXCONST_CFG(0.00000000643330), FL2FXCONST_CFG(0.00004396850232),
    FL2FXCONST_CFG(0.00087456948552), FL2FXCONST_CFG(0.00474648220243),
    FL2FXCONST_CFG(0.01717987244800), FL2FXCONST_CFG(0.04906742491073),
    FL2FXCONST_CFG(0.10569518656729), FL2FXCONST_CFG(0.21165767592653),
    FL2FXCONST_CFG(0.36036762478024), FL2FXCONST_CFG(0.59894182766948),
    FL2FXCONST_CFG(0.81641678929129), FL2FXCONST_CFG(0.97418481133397),
    FL2FXCONST_CFG(0.99575411610845), FL2FXCONST_CFG(0.88842666281361),
    FL2FXCONST_CFG(0.79222317063736), FL2FXCONST_CFG(0.70828604318604),
    FL2FXCONST_CFG(0.66395054816338), FL2FXCONST_CFG(0.64633739516952),
    FL2FXCONST_CFG(0.66098278185255)};

/* sac_bitdec */
const INT samplingFreqTable[16] = {96000, 88200, 64000, 48000, 44100, 32000,
                                   24000, 22050, 16000, 12000, 11025, 8000,
                                   7350,  0,     0,     0};

const UCHAR freqResTable[] = {0, 28, 20, 14, 10, 7, 5, 4};

const UCHAR freqResTable_LD[] = {0, 23, 15, 12, 9, 7, 5, 4};

const UCHAR tempShapeChanTable[][8] = {{5, 5, 4, 6, 6, 4, 4, 2},
                                       {5, 5, 5, 7, 7, 4, 4, 2}};

const TREEPROPERTIES treePropertyTable[] = {
    {1, 6, 5, 0, {0, 0, 0, 0, 1}}, {1, 6, 5, 0, {0, 0, 1, 0, 0}},
    {2, 6, 3, 1, {1, 0, 0, 0, 0}}, {2, 8, 5, 1, {1, 0, 0, 0, 0}},
    {2, 8, 5, 1, {1, 0, 0, 0, 0}}, {6, 8, 2, 0, {0, 0, 0, 0, 0}},
    {6, 8, 2, 0, {0, 0, 0, 0, 0}}, {1, 2, 1, 0, {0, 0, 0, 0, 0}}};

const SCHAR kernels_4_to_71[MAX_HYBRID_BANDS] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

const SCHAR kernels_5_to_71[MAX_HYBRID_BANDS] = {
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

const SCHAR kernels_7_to_71[MAX_HYBRID_BANDS] = {
    0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

const SCHAR kernels_10_to_71[MAX_HYBRID_BANDS] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7, 7, 7, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

const SCHAR kernels_14_to_71[MAX_HYBRID_BANDS] = {
    0,  0,  0,  0,  1,  1,  2,  3,  4,  4,  5,  6,  6,  7,  7,  8,  8,
    8,  9,  9,  9,  10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};

const SCHAR kernels_20_to_71[MAX_HYBRID_BANDS] = {
    0,  0,  1,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    14, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 17, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19};

const SCHAR kernels_28_to_71[MAX_HYBRID_BANDS] = {
    0,  0,  1,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22, 22, 23,
    23, 23, 23, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 26, 26, 26,
    26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27};

const SCHAR kernels_4_to_64[MAX_HYBRID_BANDS] = {
    0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

const SCHAR kernels_5_to_64[MAX_HYBRID_BANDS] = {
    0, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

const SCHAR kernels_7_to_64[MAX_HYBRID_BANDS] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

const SCHAR kernels_9_to_64[MAX_HYBRID_BANDS] = {
    0, 1, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

const SCHAR kernels_12_to_64[MAX_HYBRID_BANDS] = {
    0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  8,  8,  9,
    9,  9,  9,  9,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11};

const SCHAR kernels_15_to_64[MAX_HYBRID_BANDS] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  9,  10, 10, 10, 11, 11, 11, 11, 12,
    12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14};

const SCHAR kernels_23_to_64[MAX_HYBRID_BANDS] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 12, 13, 13, 14, 14, 15,
    15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 19, 20, 20, 20,
    20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22};

const UCHAR mapping_15_to_23[MAX_PARAMETER_BANDS_LD] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  9, 10,
    10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 14};

const UCHAR mapping_12_to_23[MAX_PARAMETER_BANDS_LD] = {
    0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 10, 11, 11, 11};

const UCHAR mapping_9_to_23[MAX_PARAMETER_BANDS_LD] = {
    0, 1, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8};

const UCHAR mapping_7_to_23[MAX_PARAMETER_BANDS_LD] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6};

const UCHAR mapping_5_to_23[MAX_PARAMETER_BANDS_LD] = {
    0, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4};

const UCHAR mapping_4_to_23[MAX_PARAMETER_BANDS_LD] = {
    0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3};

const FIXP_CFG clipGainTable__FDK[] = {
    /*CLIP_PROTECT_GAIN_0(1.000000f)*/ FX_DBL2FX_CFG(MAXVAL_DBL),
    CLIP_PROTECT_GAIN_1(1.189207f),
    CLIP_PROTECT_GAIN_1(1.414213f),
    CLIP_PROTECT_GAIN_1(1.681792f),
    /*CLIP_PROTECT_GAIN_1(2.000000f)*/ FX_DBL2FX_CFG(MAXVAL_DBL),
    CLIP_PROTECT_GAIN_2(2.378414f),
    CLIP_PROTECT_GAIN_2(2.828427f),
    /*CLIP_PROTECT_GAIN_2(4.000000f)*/ FX_DBL2FX_CFG(MAXVAL_DBL)};

const UCHAR clipGainSFTable__FDK[] = {0, 1, 1, 1, 1, 2, 2, 2};

const UCHAR pbStrideTable[] = {1, 2, 5, 28};

const int smgTimeTable[] = {64, 128, 256, 512};

/* table is scaled by factor 0.5 */
const FIXP_CFG envShapeDataTable__FDK[5][2] = {
    {FL2FXCONST_CFG(0.25000000000000f), FL2FXCONST_CFG(0.25000000000000f)},
    {FL2FXCONST_CFG(0.35355339059327f), FL2FXCONST_CFG(0.31498026247372f)},
    {FL2FXCONST_CFG(0.50000000000000f), FL2FXCONST_CFG(0.39685026299205f)},
    {FL2FXCONST_CFG(0.70710678118655f), FL2FXCONST_CFG(0.50000000000000f)},
    {/*FL2FXCONST_CFG( 1.00000000000000f)*/ FX_DBL2FX_CFG(MAXVAL_DBL),
     FL2FXCONST_CFG(0.62996052494744f)}};

/* sac_calcM1andM2 */
const SCHAR row2channelSTP[][MAX_M2_INPUT] = {{0, 1}, {0, 3}, {0, 2},  {0, 4},
                                              {0, 4}, {0, 2}, {-1, 2}, {0, 1}};

const SCHAR row2channelGES[][MAX_M2_INPUT] = {{0, 1}, {0, 3}, {0, 3},  {0, 5},
                                              {0, 5}, {0, 2}, {-1, 2}, {0, 1}};

const SCHAR row2residual[][MAX_M2_INPUT] = {{-1, 0},  {-1, 0},  {-1, -1},
                                            {-1, -1}, {-1, -1}, {-1, -1},
                                            {-1, -1}, {-1, 0}};

/*******************************************************************************
 Functionname: sac_getCLDValues
 *******************************************************************************

 Description: Get CLD values from table index.

 Arguments:
   index: Table index
   *cu, *cl : Pointer to locations where resulting values will be written to.

 Return: nothing

*******************************************************************************/
void SpatialDequantGetCLDValues(int index, FIXP_DBL* cu, FIXP_DBL* cl) {
  *cu = FX_CFG2FX_DBL(dequantCLD_c_l[index]);
  *cl = FX_CFG2FX_DBL(dequantCLD_c_l[31 - 1 - index]);
}

void SpatialDequantGetCLD2Values(int idx, FIXP_DBL* x) {
  *x = FX_CFG2FX_DBL(dequantCLD__FDK[idx]);
}
