
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

/**************************  Fraunhofer IIS FDK SysLib  **********************

   Author(s):   Eric Allamanche

******************************************************************************/

/** \file   wav_file.h
 *  \brief  Rudimentary WAVE file read/write support.
 *
 *  The WAVE file reader/writer is intented to be used in the codec's example
 *  framework for easily getting started with encoding/decoding. Therefore
 *  it serves mainly for helping quickly understand how a codec's API actually
 *  works.
 *  Being a WAVE file reader/writer with very basic functionality, it may not be
 *  able to read WAVE files that come with unusual configurations.
 *  Details on how to use the interface functions can be found in every
 *  (encoder/decoder) example framework.
 */

#ifndef __WAV_FILE_H__
#define __WAV_FILE_H__



#include "genericStds.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

/*!
 * RIFF WAVE file struct.
 * For details see WAVE file format documentation (for example at http://www.wotsit.org).
 */
typedef struct WAV_HEADER
{
  char   riffType[4];
  UINT   riffSize;
  char   waveType[4];
  char   formatType[4];
  UINT   formatSize;
  USHORT compressionCode;
  USHORT numChannels;
  UINT   sampleRate;
  UINT   bytesPerSecond;
  USHORT blockAlign;
  USHORT bitsPerSample;
  char   dataType[4];
  UINT   dataSize;
} WAV_HEADER;

struct WAV
{
  WAV_HEADER header;
  FDKFILE *fp;
  UINT channelMask;
};

typedef struct WAV *HANDLE_WAV;

/**
 * \brief  Open a WAV file handle for reading.
 *
 * \param pWav      Pointer to a memory location, where a WAV handle is returned.
 * \param filename  File name to be opened.
 *
 * \return  0 on success and non-zero on failure.
 */
INT WAV_InputOpen (HANDLE_WAV *pWav, const char *filename);

/**
 * \brief  Read samples from a WAVE file. The samples are automatically re-ordered to the
 *         native host endianess and scaled to full scale of the INT_PCM type, from
 *         whatever BPS the WAVE file had specified in its header data.
 *
 *  \param wav           Handle of WAV file.
 *  \param sampleBuffer  Pointer to store audio data.
 *  \param numSamples    Desired number of samples to read.
 *  \param nBufBits      Size in bit of each audio sample of sampleBuffer.
 *
 *  \return  Number of samples actually read.
 */
INT WAV_InputRead (HANDLE_WAV wav, void *sampleBuffer, UINT numSamples, int nBufBits);

/**
 * \brief       Close a WAV file reading handle.
 * \param pWav  Pointer to a WAV file reading handle.
 * \return      void
 */
void WAV_InputClose(HANDLE_WAV *pWav);

/**
 * \brief  Open WAV output/writer handle.
 *
 * \param pWav            Pointer to WAV handle to be returned.
 * \param outputFilename  File name of the file to be written to.
 * \param sampleRate      Desired samplerate of the resulting WAV file.
 * \param numChannels     Desired number of audio channels of the resulting WAV file.
 * \param bitsPerSample   Desired number of bits per audio sample of the resulting WAV file.
 *
 * \return  0: ok; -1: error
 */
INT WAV_OutputOpen(HANDLE_WAV *pWav, const char *outputFilename, INT sampleRate, INT numChannels, INT bitsPerSample);

/**
 * \brief  Write data to WAV file asociated to WAV handle.
 *
 * \param wav              Handle of WAV file
 * \param sampleBuffer     Pointer to audio samples, right justified integer values.
 * \param numberOfSamples  The number of individual audio sample valuesto be written.
 * \param nBufBits         Size in bits of each audio sample in sampleBuffer.
 * \param nSigBits         Amount of significant bits of each nBufBits in sampleBuffer.
 *
 * \return 0: ok; -1: error
 */
INT WAV_OutputWrite(HANDLE_WAV wav, void *sampleBuffer, UINT numberOfSamples, int nBufBits, int nSigBits);

/**
 * \brief       Close WAV output handle.
 * \param pWav  Pointer to WAV handle. *pWav is set to NULL.
 * \return      void
 */
void WAV_OutputClose(HANDLE_WAV *pWav);

#ifdef __cplusplus
}
#endif


#endif /* __WAV_FILE_H__ */
