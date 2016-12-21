
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

/*****************************  MPEG-4 AAC Decoder  **************************

   Author(s):   Manuel Jander
   Description: Bitstream data provider for MP4 decoders

******************************************************************************/

#include "machine_type.h"
#include "FDK_audio.h"

#define MPFREAD_MP4FF_DISABLE

#ifndef MPFREAD_MP4FF_DISABLE
  /*!< If MPFREAD_MP4FF_ENABLE is set, include support for MPEG ISO fileformat.
       If not set, no .mp4, .m4a and .3gp files can be used for input. */
  #define MPFREAD_MP4FF_ENABLE
#endif

/* maximum number of layers which can be read        */
/* shall equal max number of layers read by iisisoff */
#define FILEREAD_MAX_LAYERS (2)

typedef struct STRUCT_FILEREAD *HANDLE_FILEREAD;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief           Open an MPEG audio file and try to detect its format.
 * \param filename  String of the filename to be opened.
 * \param fileFormat Skip file format detection and use given format if fileFormat != FF_UNKNOWN.
                    Else store detected format into *fileFmt.
 * \param transportType Skip transport type detection and use given format if transportType != TT_UNKNOWN.
                    Else store detected format into *fileFmt.
 * \param conf      Pointer to unsigned char to hold the AudioSpecificConfig of the input file, if
                    any (MPEG 4 file format). In case of RAW LATM it holds the StreamMuxConfig.
 * \param confSize  Pointer to an integer, where the length of the ASC or SMC (in case of RAW LATM)
                    is stored to.
 * \return          MPEG file read handle.
 */
HANDLE_FILEREAD mpegFileRead_Open( const char     *filename,
                                   FILE_FORMAT     fileFormat,
                                   TRANSPORT_TYPE  transportType,
                                   UCHAR          *conf[],
                                   UINT            confSize[],
                                   INT            *noOfLayers
                                 );

/**
 * \brief           Get the file format of the input file.
 * \param hDataSrc  MPEG file read handle.
 * \return          File format of the input file.
 */
FILE_FORMAT mpegFileRead_GetFileFormat(HANDLE_FILEREAD hDataSrc);

/**
 * \brief           Get the transport type of the input file.
 * \param hDataSrc  MPEG file read handle.
 * \return          Transport type of the input file.
 */
TRANSPORT_TYPE mpegFileRead_GetTransportType(HANDLE_FILEREAD hDataSrc);

/**
 * \brief Read data from MPEG file. In case of packet file, read one packet, in case
 *        of streaming file with embedded synchronisation layer (LOAS/ADTS...), just
 *        fill the buffer.
 *
 * \param hMpegFile   MPEG file read handle.
 * \param inBuffer    Pointer to input buffer.
 * \param bufferSize  Size of input buffer.
 * \param bytesValid  Number of bytes that were read.
 * \return            0 on success, -1 if unsupported file format or file read error.
 */
int mpegFileRead_Read( HANDLE_FILEREAD   hMpegFile,
                       UCHAR            *inBuffer[],
                       UINT              bufferSize,
                       UINT             *bytesValid
                     );

/**
 * \brief            Seek in file from origin by given offset in frames.
 * \param hMpegFile  MPEG file read handle.
 * \param origin     If 0, the origin is the file beginning (absolute seek).
 *                   If 1, the origin is the current position (relative seek).
 * \param offset     The amount of frames to seek from the given origin.
 * \return           0 on sucess, -1 if offset < 0 or file read error.
 */
int mpegFileRead_seek( HANDLE_FILEREAD   hMpegFile,
                       INT               origin,
                       INT               offset
                      );

/**
 * \brief            Get file position in percent.
 * \param hMpegFile  MPEG file read handle.
 * \return           File position in percent.
 */
int mpegFileRead_getPercent(HANDLE_FILEREAD hMpegFile);


/**
 * \brief           Close MPEG audio file.
 * \param hMpegFile Mpeg file read handle.
 * \return          0 on sucess.
 */
int mpegFileRead_Close(HANDLE_FILEREAD *hMpegFile);

#ifdef __cplusplus
}
#endif
