
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

/******************************** MPEG Audio Encoder **************************

   Initial author:
   contents/description: CRC calculation

******************************************************************************/

#include "FDK_crc.h"



/*---------------- constants -----------------------*/

/**
 * \brief  This table defines precalculated lookup tables for crc polynom  x^16 + x^15 + x^2 + x^0.
 */
static const USHORT crcLookup_16_15_2_0[256] =
{
  0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011,
  0x8033, 0x0036, 0x003c, 0x8039, 0x0028, 0x802d, 0x8027, 0x0022,
  0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
  0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041,
  0x80c3, 0x00c6, 0x00cc, 0x80c9, 0x00d8, 0x80dd, 0x80d7, 0x00d2,
  0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
  0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1,
  0x8093, 0x0096, 0x009c, 0x8099, 0x0088, 0x808d, 0x8087, 0x0082,
  0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
  0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1,
  0x01e0, 0x81e5, 0x81ef, 0x01ea, 0x81fb, 0x01fe, 0x01f4, 0x81f1,
  0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
  0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151,
  0x8173, 0x0176, 0x017c, 0x8179, 0x0168, 0x816d, 0x8167, 0x0162,
  0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
  0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101,
  0x8303, 0x0306, 0x030c, 0x8309, 0x0318, 0x831d, 0x8317, 0x0312,
  0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
  0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371,
  0x8353, 0x0356, 0x035c, 0x8359, 0x0348, 0x834d, 0x8347, 0x0342,
  0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
  0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2,
  0x83a3, 0x03a6, 0x03ac, 0x83a9, 0x03b8, 0x83bd, 0x83b7, 0x03b2,
  0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,
  0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291,
  0x82b3, 0x02b6, 0x02bc, 0x82b9, 0x02a8, 0x82ad, 0x82a7, 0x02a2,
  0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
  0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1,
  0x8243, 0x0246, 0x024c, 0x8249, 0x0258, 0x825d, 0x8257, 0x0252,
  0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
  0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231,
  0x8213, 0x0216, 0x021c, 0x8219, 0x0208, 0x820d, 0x8207, 0x0202
};

/**
 * \brief  This table defines precalculated lookup tables for crc polynom  x^16 + x^12 + x^5 + x^0.
 */
static const USHORT crcLookup_16_12_5_0[256] =
{
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
  0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
  0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
  0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
  0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
  0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
  0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
  0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
  0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
  0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
  0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
  0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
  0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
  0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
  0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
  0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
  0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
  0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
  0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
  0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
  0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
  0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};


/*--------------- function declarations --------------------*/

static inline INT calcCrc_Bits(
        USHORT * const                  pCrc,
        USHORT                          crcMask,
        USHORT                          crcPoly,
        HANDLE_FDK_BITSTREAM            hBs,
        INT                             nBits
        );

static inline INT calcCrc_Bytes(
        USHORT * const                  pCrc,
        const USHORT *                  pCrcLookup,
        HANDLE_FDK_BITSTREAM            hBs,
        INT                             nBytes
        );

static void crcCalc(
        HANDLE_FDK_CRCINFO              hCrcInfo,
        HANDLE_FDK_BITSTREAM            hBs,
        const INT                       reg
        );


/*------------- function definitions ----------------*/

void FDKcrcInit(
        HANDLE_FDK_CRCINFO              hCrcInfo,
        const UINT                      crcPoly,
        const UINT                      crcStartValue,
        const UINT                      crcLen
        )
{
  /* crc polynom example:
  x^16 + x^15 + x^2 + x^0        (1) 1000 0000 0000 0101 -> 0x8005
  x^16 + x^12 + x^5 + x^0        (1) 0001 0000 0010 0001 -> 0x1021
  x^8 + x^4 + x^3 + x^2 + x^0              (1) 0001 1101 -> 0x001d */

  hCrcInfo->crcLen     = crcLen;
  hCrcInfo->crcPoly    = crcPoly;
  hCrcInfo->startValue = crcStartValue;
  hCrcInfo->crcMask    = (crcLen) ? (1<<(crcLen-1)) : 0;

  FDKcrcReset(hCrcInfo);

  hCrcInfo->pCrcLookup = 0;

  if (hCrcInfo->crcLen==16) {
    switch ( crcPoly ) {
      case 0x8005:
        hCrcInfo->pCrcLookup = crcLookup_16_15_2_0;
        break;
      case 0x1021:
        hCrcInfo->pCrcLookup = crcLookup_16_12_5_0;
        break;
      case 0x001d:
      default:
        /* no lookup table */
        hCrcInfo->pCrcLookup = 0;
    }
  }


}

void FDKcrcReset(HANDLE_FDK_CRCINFO hCrcInfo)
{
  int i;

  hCrcInfo->crcValue = hCrcInfo->startValue;

  for(i=0;i<MAX_CRC_REGS;i++) {
    hCrcInfo->crcRegData[i].isActive = 0;
  }
  hCrcInfo->regStart = 0;
  hCrcInfo->regStop  = 0;
}

INT FDKcrcStartReg(
        HANDLE_FDK_CRCINFO              hCrcInfo,
        const HANDLE_FDK_BITSTREAM      hBs,
        const INT                       mBits
        )
{
  int reg = hCrcInfo->regStart;

  FDK_ASSERT(hCrcInfo->crcRegData[reg].isActive==0);
  hCrcInfo->crcRegData[reg].isActive      = 1;
  hCrcInfo->crcRegData[reg].maxBits       = mBits;
  hCrcInfo->crcRegData[reg].validBits     = FDKgetValidBits(hBs) ;
  hCrcInfo->crcRegData[reg].bitBufCntBits = 0;

  hCrcInfo->regStart = (hCrcInfo->regStart+1)%MAX_CRC_REGS;

  return (reg);
}

INT FDKcrcEndReg(
        HANDLE_FDK_CRCINFO              hCrcInfo,
        const HANDLE_FDK_BITSTREAM      hBs,
        const INT                       reg
        )
{
  FDK_ASSERT((reg==(INT)hCrcInfo->regStop)&&(hCrcInfo->crcRegData[reg].isActive==1));

  if (hBs->ConfigCache==BS_WRITER) {
    hCrcInfo->crcRegData[reg].bitBufCntBits = FDKgetValidBits(hBs) - hCrcInfo->crcRegData[reg].validBits;
  }
  else {
    hCrcInfo->crcRegData[reg].bitBufCntBits = hCrcInfo->crcRegData[reg].validBits - FDKgetValidBits(hBs);
  }

  if (hCrcInfo->crcRegData[reg].maxBits == 0) {
    hCrcInfo->crcRegData[reg].maxBits = hCrcInfo->crcRegData[reg].bitBufCntBits;
  }

  crcCalc( hCrcInfo, hBs, reg);

  hCrcInfo->crcRegData[reg].isActive = 0;
  hCrcInfo->regStop = (hCrcInfo->regStop+1)%MAX_CRC_REGS;

  return 0;
}

USHORT FDKcrcGetCRC(
        const HANDLE_FDK_CRCINFO        hCrcInfo
        )
{
  return ( hCrcInfo->crcValue & (((hCrcInfo->crcMask-1)<<1)+1) );
}

/**
 * \brief  Calculate crc bits.
 *
 * Calculate crc starting at current bitstream postion over nBits.
 *
 * \param pCrc                  Pointer to an outlying allocated crc info structure.
 * \param crcMask               CrcMask in use.
 * \param crcPoly               Crc polynom in use.
 * \param hBs                   Handle to current bit buffer structure.
 * \param nBits                 Number of processing bits.
 *
 * \return  Number of processed bits.
 */
static inline INT calcCrc_Bits(
        USHORT * const                  pCrc,
        USHORT                          crcMask,
        USHORT                          crcPoly,
        HANDLE_FDK_BITSTREAM            hBs,
        INT                             nBits
        )
{
  int i;
  USHORT crc = *pCrc; /* get crc value */

  if (hBs!=NULL) {
    for (i = 0; (i < nBits); i++) {
      USHORT tmp = FDKreadBits(hBs,1);     // process single bit
      tmp ^= ( (crc & crcMask) ? 1 : 0 );
      tmp *= crcPoly;
      crc <<= 1;
      crc ^= tmp;
    }
  }
  else {
    for (i = 0; (i < nBits); i++) {
      USHORT tmp = 0;     // process single bit
      tmp ^= ( (crc & crcMask) ? 1 : 0 );
      tmp *= crcPoly;
      crc <<= 1;
      crc ^= tmp;
    }
  }
  *pCrc = crc; /* update crc value */

  return nBits;
}

/**
 * \brief  Calculate crc bytes.
 *
 * Calculate crc starting at current bitstream postion over nBytes.
 *
 * \param pCrc                  Pointer to an outlying allocated crc info structure.
 * \param pCrcLookup            Pointer to lookup table used for fast crc calculation.
 * \param hBs                   Handle to current bit buffer structure.
 * \param nBits                 Number of processing bytes.
 *
 * \return  Number of processed bits.
 */
static inline INT calcCrc_Bytes(
        USHORT * const                  pCrc,
        const USHORT *                  pCrcLookup,
        HANDLE_FDK_BITSTREAM            hBs,
        INT                             nBytes
        )
{
  int i;
  USHORT crc = *pCrc; /* get crc value */

  if (hBs!=NULL) {
    for (i=0; i<nBytes; i++) {
      crc = (crc<<8)^pCrcLookup[((crc>>8)^((UCHAR)FDKreadBits(hBs,8)))&0xFF];
    }
  }
  else {
    for (i=0; i<nBytes; i++) {
      crc = (crc<<8)^pCrcLookup[((crc>>8)^((UCHAR)0))&0xFF];
    }
  }

  *pCrc = crc; /* update crc value */

  return (i);
}

/**
 * \brief  Calculate crc.
 *
 * Calculate crc. Lenght depends on mBits parameter in FDKcrcStartReg() configuration.
 *
 * \param hCrcInfo              Pointer to an outlying allocated crc info structure.
 * \param hBs                   Pointer to current bit buffer structure.
 * \param reg                   Crc region ID.
 *
 * \return  Number of processed bits.
 */
static void crcCalc(
        HANDLE_FDK_CRCINFO              hCrcInfo,
        HANDLE_FDK_BITSTREAM            hBs,
        const INT                       reg
        )
{
  USHORT crc = hCrcInfo->crcValue;
  CCrcRegData *rD = &hCrcInfo->crcRegData[reg];
  FDK_BITSTREAM bsReader;

  if (hBs->ConfigCache==BS_READER) {
    bsReader = *hBs;
    FDKpushBiDirectional(&bsReader, -(INT)(rD->validBits-FDKgetValidBits(&bsReader)));
  }
  else {
    FDKinitBitStream(&bsReader, hBs->hBitBuf.Buffer, hBs->hBitBuf.bufSize, hBs->hBitBuf.ValidBits, BS_READER);
    FDKpushBiDirectional(&bsReader, rD->validBits);
  }

  int bits, rBits;
  rBits = (rD->maxBits>=0) ? rD->maxBits : -rD->maxBits; /* ramaining bits */
  if ((rD->maxBits>0) && (((INT)rD->bitBufCntBits>>3<<3)<rBits) ) {
    bits = rD->bitBufCntBits;
  }
  else {
    bits = rBits;
  }

  int words = bits >> 3;  /* processing bytes */
  int mBits = bits & 0x7; /* modulo bits */

  if(hCrcInfo->pCrcLookup) {
    rBits -= (calcCrc_Bytes(&crc, hCrcInfo->pCrcLookup, &bsReader, words)<<3);
  }
  else {
    rBits -= calcCrc_Bits(&crc, hCrcInfo->crcMask, hCrcInfo->crcPoly, &bsReader, words<<3 );
  }

  /* remaining valid bits*/
  if(mBits!=0) {
    rBits -= calcCrc_Bits(&crc, hCrcInfo->crcMask, hCrcInfo->crcPoly, &bsReader, mBits );
  }

  if (rBits!=0) {
    /* zero bytes */
    if ( (hCrcInfo->pCrcLookup) && (rBits>8) ) {
      rBits -= (calcCrc_Bytes(&crc, hCrcInfo->pCrcLookup, NULL, rBits>>3)<<3);
    }
    /* remaining zero bits */
    if (rBits!=0) {
      rBits -= calcCrc_Bits(&crc, hCrcInfo->crcMask, hCrcInfo->crcPoly, NULL, rBits );
    }
  }

  hCrcInfo->crcValue = crc;
}

