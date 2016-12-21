
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

   Author(s):
   Description: command line parser

******************************************************************************/

/** \file   cmdl_parser.h
 *  \brief  Command line parser.
 *
 *  The command line parser can extract certain data fields out of a character
 *  string and assign values to variables. It has 2 main functions. One to parse
 *  a command line in the form of standard C runtime "argc" and "argv" parameters,
 *  and the other to assemble these parameters reading text lines from a file in
 *  case the C runtime does not provide them.
 */

#ifndef  __PARSER_H
#define __PARSER_H



#include "machine_type.h"

#define CMDL_MAX_STRLEN 255
#define CMDL_MAX_ARGC    30

/* \cond */
/* Type definition for text */


#ifdef WIN32
  #include <tchar.h>
  #ifndef _tstof  /* For Visual Studio 6 */
    #ifdef _UNICODE
      #include <wchar.h>
      #define _tstof(x)   (float) wcstod(x, NULL)  /* For Visual Studio 6 */
    #else
      #define _tstof      atof
    #endif
  #endif

  #ifndef _tstol  /* For Visual Studio 6 */
    #ifdef _UNICODE
      #define _tstol      _wtol
    #else
      #define _tstol      atol
    #endif
  #endif

  #ifndef _tstoi  /* For Visual Studio 6 */
    #ifdef _UNICODE
      #define _tstoi      _wtoi
    #else
      #define _tstoi      atoi
    #endif
  #endif

  #ifndef TEXTCHAR
    #define TEXTCHAR char
  #endif

  #ifndef _TEXT
    #define _TEXT
  #endif

#else /* WIN32 */

    #define TEXTCHAR char
    #define _tcslen(a)  FDKstrlen(a)
    #define _tcscpy     strcpy
    #define _tcscmp     FDKstrcmp
    #define _tcsncmp    FDKstrncmp
    #define _tscanf     scanf
    #define _TEXT(x)    x
    #define _tfopen     fopen
    #define _ftprintf   fprintf
    #define _tcsncpy    FDKstrncpy
    #define _tstof      FDKatof
    #define _tstol      FDKatol
    #define _tstoi      FDKatoi
    #define _tcstol     strtol
    #define _istdigit   isdigit
#endif /* WIN32 */

/* \endcond */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 *  Scans argc, argv and a scanf style format string for parameters and stores the
 *  values in the variable number of pointers passed to the function.

 For example:
   \code
   #define ARG_PARAM  "(-a %d) (-v %1)"
   #define ARG_VALUE &config->aot, &verbose
   int nFoundArgs = IIS_ScanCmdl(argc, argv, ARG_PARAM, ARG_VALUE);
   \endcode
   wheras the wild-cards (\%d, \%1, ..) define the data type of the argument:
	- \%1 boolean (e. g. -x)
	- \%d integer (e. g. -x 23)
	- \%f float (e. g. -x 3.4)
	- \%y double (e. g. -x 31415926535897932384626433832795028841971693993751)
	- \%s string (e. g. -x "file.dat")
	- \%u unsigned character (e. g. -x 3)
	- \%c signed character (e. g. -x -3)
    More examples on how to use it are located in every (encoder/decoder) example framework.

 * \param argc      Number of arguments.
 * \param argv      Complete character string of the command line arguments.
 * \param pReqArgs  A list of parameters and a corresponding list of memory addresses to
 *                  assign each parameter to.
 *
 * \return  Number of found arguments.
 */
INT IIS_ScanCmdl(INT argc, TEXTCHAR* argv[], const TEXTCHAR* pReqArgs, ...);

#ifdef __cplusplus
}
#endif

/**
 *  Reads a text file, assembles argc and argv parameters for each text line
 *  and calls the given function for each set of argc, argv parameters.
 *
 * \param param_filename  Name of text file that should be parsed.
 * \param pFunction       Pointer to function that should be called for every text line found.
 *
 * \return  0 on success, 1 on failure.
 */
INT IIS_ProcessCmdlList(const TEXTCHAR* param_filename, int (*pFunction)(int, TEXTCHAR**));


#endif /* __PARSER_H */
