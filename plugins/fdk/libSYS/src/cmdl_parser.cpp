
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



#define _CRT_SECURE_NO_WARNINGS

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "cmdl_parser.h"
#include "genericStds.h"



/************************ interface ************************/

static INT  GetArgFromString(INT argc, TEXTCHAR* argv[], TEXTCHAR* search_string, TEXTCHAR type, TEXTCHAR* found_string , INT* switches_used);
static void RemoveWhiteSpace(const TEXTCHAR* pReqArgs, TEXTCHAR* Removed);
static INT CheckArg(TEXTCHAR* arg,  TEXTCHAR* str, UINT numArgs, TEXTCHAR type, TEXTCHAR* current_str);
static INT CheckForUnusedSwitches(INT argc, /*TEXTCHAR* argv[],*/ INT* switches_used);
static INT ParseString(TEXTCHAR* str, INT*, TEXTCHAR*, TEXTCHAR*);
static void GetNumberOfArgs(TEXTCHAR* str, INT* nArgs);

static
void removeQuotes(char *str)
{
  if (str[0] == '"') {
    FDKstrcpy(str, str+1);
    str[FDKstrlen(str)-1] = 0;
  }
}


/********************** implementation *********************/

INT IIS_ScanCmdl(INT argc, TEXTCHAR* argv[], const TEXTCHAR* str, ...)
{
  INT i              = 0;
  INT found_and_set  = 0;
  INT nArgs          = 0;
  INT* switches_used = 0;
  INT*   b_str_opt   = 0;
  TEXTCHAR*  s_str       = 0;
  TEXTCHAR*  c_str_type  = 0;
  TEXTCHAR*  str_clean   = 0;

  va_list ap;

  if (argc == 0 || argc == 1)
  {
    FDKprintf("No command line arguments\n");
    goto bail;
  }

  str_clean  = (TEXTCHAR*)  FDKcalloc((unsigned int)_tcslen(str), sizeof(TEXTCHAR));
  if (str_clean == NULL) {
    FDKprintf("Error allocating memory line %d, file %s\n",  __LINE__, __FILE__);
    return 0;
  }

  RemoveWhiteSpace(str, str_clean );
  GetNumberOfArgs(str_clean, &nArgs);

  b_str_opt  = (INT*)   FDKcalloc(nArgs,    sizeof(INT));
  s_str      = (TEXTCHAR*)  FDKcalloc(nArgs*CMDL_MAX_ARGC, sizeof(TEXTCHAR) );
  c_str_type = (TEXTCHAR*)  FDKcalloc(nArgs,    sizeof(TEXTCHAR));
  switches_used = (INT*) FDKcalloc(argc, sizeof(INT));

  if (b_str_opt == NULL || s_str == NULL || c_str_type == NULL || switches_used == NULL) {
    FDKprintf("Error allocating memory line %d, file %s\n",  __LINE__, __FILE__);
    goto bail;
  }

  if ( ParseString( str_clean, b_str_opt, s_str, c_str_type )) {
    goto bail;
  }

  va_start(ap, str);

  for ( i = 0; i < nArgs; i++ )
    {
      TEXTCHAR arg[CMDL_MAX_STRLEN] = {L'\0'};
      TEXTCHAR* p_arg = arg;
      TEXTCHAR* current_str = &(s_str[i*CMDL_MAX_ARGC]);

      if (GetArgFromString(argc, argv, current_str, c_str_type[i], arg, switches_used )
          && !b_str_opt[i] )
        {
#ifdef _UNICODE
          _ftprintf(stderr, _TEXT("\n\nError: Parsing argument for required switch '%ls'.\n" ), current_str);
#else
          _ftprintf(stderr, _TEXT("\n\nError: Parsing argument for required switch '%s'.\n" ), current_str);
#endif
          found_and_set = 0;
          goto bail;
        }
      if (CheckArg(p_arg, s_str, nArgs, c_str_type[i], current_str))
        {
          goto bail;
        }

      switch (c_str_type[i] )
        {
        case 's':
          {
            TEXTCHAR* tmp;
            tmp = va_arg(ap, TEXTCHAR*);

            if ( arg[0] == '\0' )
              break;

            _tcsncpy( tmp, arg, CMDL_MAX_STRLEN );
            /* Remove quotes. Windows Mobile Workaround. */
            removeQuotes(tmp);
            found_and_set++;
            break;
          }
        case 'd':
          {
            INT* tmp = va_arg(ap, INT*);

            if ( arg[0] == '\0' )
              break;

            *tmp = _tcstol(arg, NULL, 0);
            found_and_set++;
            break;
          }
        case 'c':
          {
            char* tmp = va_arg(ap, char*);

            if ( arg[0] == '\0' )
              break;

            *tmp = *arg;
            found_and_set++;
            break;
          }
        case 'u':
          {
            UCHAR* tmp = va_arg(ap, UCHAR*);

            if ( arg[0] == '\0' )
              break;

            *tmp = _tstoi(arg);
            found_and_set++;
            break;
          }
        case 'f':
          {
            float* tmp = (float*) va_arg( ap,double*);

            if ( arg[0] == '\0' )
              break;

            *tmp = (float) _tstof(arg);
            found_and_set++;
            break;
          }
        case 'y': // support 'data type double'
          {
            double* tmp = (double*) va_arg( ap,double*);
            // use sscanf instead _tstof because of gcc
            //_tstof(arg,"%lf",tmp); // '%lf' reads as double
            *tmp = _tstof(arg); // '%lf' reads as double
            found_and_set++;
            break;
          }
        case '1':
          {

            INT* tmp = va_arg( ap, INT*);

            if ( arg[0] == '\0' )
              break;

            *tmp = 1;
            found_and_set++;
            break;
          }

        default:
          FDKprintfErr("Bug: unsupported data identifier \"%c\"\n", c_str_type[i]);
          break;

        }

    }

  va_end(ap);

  CheckForUnusedSwitches(argc, /*argv,*/ switches_used);

bail:
  if (b_str_opt)     FDKfree(b_str_opt);
  if (s_str)         FDKfree(s_str);
  if (c_str_type)    FDKfree(c_str_type);
  if (str_clean)     FDKfree(str_clean);
  if (switches_used) FDKfree(switches_used);

  return found_and_set;
}


void GetNumberOfArgs(TEXTCHAR* str, INT* nArgs)
{
  UINT i = 0;
  for ( i = 0; i < _tcslen(str); ++i )
    {
      if ( str[i] == '%')
        *nArgs+= 1;
    }

}

INT ParseString(TEXTCHAR* str, INT* b_str_opt, TEXTCHAR* s_str, TEXTCHAR* c_str_type )
{
    UINT i = 0;
    INT argCounter = 0;

    TEXTCHAR* str_start = 0;
    TEXTCHAR* str_stop = 0;


    str_start = str;
    str_stop = str_start;

    for ( i = 0; i < _tcslen(str) - 1; ++i )
      {
        if ( str[i] == '%' )  /* We have an Argument */
          {
            if ( argCounter )
              {
                if ( b_str_opt[argCounter-1] )
                  str_start = str_stop + 3;

                else
                  str_start = str_stop + 2;
              }

            /* Save Argument type */
            c_str_type[argCounter] = str[i+1];

            if ( *str_start == '(' ) /* Optional Argument */
              {
                b_str_opt[argCounter] = 1;
                str_start++;
              }

            /* Save Argument */
            str[i] = '\0';

            _tcsncpy(&(s_str[argCounter*CMDL_MAX_ARGC]), str_start, CMDL_MAX_ARGC );

            str[i] = '%';

            str_stop = &(str[i]);

            if ( b_str_opt[argCounter] )
              {
                if ( i+2 > ( _tcslen(str) -1 ))
                  {
                    _ftprintf(stderr,_TEXT("\n\nInternal Parser Error: Strlen Problem\n") );
                    return 1;
                  }
                if ( str[i+2] != ')' )
                  {
                    _ftprintf(stderr,_TEXT("\n\nInternal Parser Error: Missing bracket ')'\n") );
                    return 1;
                  }

              }


            argCounter++;
          }


      }

    return 0;
  }




void RemoveWhiteSpace(const TEXTCHAR* pReqArgs, TEXTCHAR* pRemoved)
{
  UINT i = 0;
  INT k = 0;
  UINT len = (UINT)_tcslen(pReqArgs);


  for ( i = 0; i < len; ++i )
    {

      if ( pReqArgs[i] != ' ' )
        {
          pRemoved[k] = pReqArgs[i];
          k++;
        }
    }
}


INT GetArgFromString(INT argc, TEXTCHAR* argv[], TEXTCHAR* search_string, TEXTCHAR type, TEXTCHAR* found_string, INT* sw_used )
{
  INT i = 0;

  for (i = 1; i < argc; ++i ) {
    if ( !_tcscmp(search_string, argv[i]) ) /* Strings are equal */
      {
        if ( type == '1' ) /* Switch without argument */
        {
          _tcsncpy( found_string, _TEXT("1"), 1);
          sw_used[i] = 1;
          return 0;

        }

        if ( i == (argc - 1))  /* We have %s or %d but are finished*/
          return 1;

        if ( _tcslen(argv[i+1]) > CMDL_MAX_STRLEN )
          {
#ifdef _UNICODE
            _ftprintf (stderr,_TEXT("Warning: Ignoring argument for switch '%ls'. "), search_string );
#else
            _ftprintf (stderr,_TEXT("Warning: Ignoring argument for switch '%s'. "), search_string );
#endif
            _ftprintf (stderr,_TEXT("Argument is too LONG.\n") );
            return 1;
          }
        else
        {
          _tcsncpy( found_string, argv[i+1], CMDL_MAX_STRLEN);
          sw_used[i] = 1;
          sw_used[i+1] = 1;
          return 0;
        }
      }
  }
  return 1;
}



INT CheckArg(TEXTCHAR* arg, TEXTCHAR* str, UINT numArgs, TEXTCHAR type, TEXTCHAR* cur_str)
{
  UINT i = 0;

  /* No argument given-> return */
  if (arg[0] == '\0')
    return 0;


  /* Check if arg is switch */
  for ( i = 0; i < numArgs; ++i )
    {
      if (!_tcscmp(arg, &(str[i*CMDL_MAX_ARGC])))
        {
#ifdef _UNICODE
          _ftprintf(stderr, _TEXT("\n\nError: Argument '%ls' for switch '%ls' is not valid \n" ), arg, cur_str );
#else
          _ftprintf(stderr, _TEXT("\n\nError: Argument '%s' for switch '%s' is not valid \n" ), arg, cur_str );
#endif
          return 1;
        }

    }
  /* Check if type is %d but a string is given */

  for ( i = 0; i < _tcslen(arg); ++i )
    {
      if ( (type == 'd') && !_istdigit(arg[i]) && arg[i] != 'x' && arg[i] != '-')
        {
#ifdef _UNICODE
          _ftprintf(stderr, _TEXT("\n\nError: Argument '%ls' for switch '%ls' is not a valid number.\n" ), arg, cur_str);
#else
          _ftprintf(stderr, _TEXT("\n\nError: Argument '%s' for switch '%s' is not a valid number.\n" ), arg, cur_str);
#endif
          return 1;
        }
    }


  return 0;
}


INT CheckForUnusedSwitches(INT argc, /*TEXTCHAR* argv[],*/ INT* switches_used)
{
  INT i = 0;

  for( i = 1; i < argc; ++i )
    {
      if ( !switches_used[i] )
        {
          ++i;
        }
    }

  return 0;
}



static char line[CMDL_MAX_STRLEN*CMDL_MAX_ARGC];
static char *argv_ptr[CMDL_MAX_ARGC];
#ifdef CMDFILE_PREFIX
static char tmp[256]; /* this array is used to store the prefix and the filepath/name */
#endif

int IIS_ProcessCmdlList(const char* param_filename, int (*pFunction)(int, TEXTCHAR**))
{
  /* static to reduce required stack size */

  FDKFILE *config_fp;
  int argc;
  char *line_ptr;

#ifdef CMDFILE_PREFIX
  FDKstrcpy(tmp, CMDFILE_PREFIX);
  FDKstrcpy(tmp+FDKstrlen(CMDFILE_PREFIX), param_filename);
  /* Open the file with command lines */
  config_fp = FDKfopen(tmp, "r");
#else
  /* Open the file with command lines */
  config_fp = FDKfopen(param_filename, "r");
#endif

  if(config_fp == NULL)
  {
#ifdef CMDFILE_PREFIX
    FDKprintf("\ncould not open config file %s", tmp);
#else
    FDKprintf("\ncould not open config file %s", param_filename);
#endif
    return 1;
  }

  /* Obtain a command line from config file */
  while (FDKfgets(line, CMDL_MAX_STRLEN*CMDL_MAX_ARGC, config_fp) != NULL)
  {
    argc = 1;

    /* Eat \n */
    line_ptr =  (char*)FDKstrchr(line, '\n');
    if (line_ptr != NULL)
      *line_ptr = ' ';

    line_ptr = line;

    /* Scan the line and put the command line params into argv */
    do {
      /* Skip consecutive blanks. */
      while (*line_ptr == ' ' && line_ptr < line+CMDL_MAX_STRLEN)
        line_ptr++;
      /* Assign argument. */
      argv_ptr[argc] = line_ptr;
      /* Get pointer to next blank. */
      line_ptr = (char*)FDKstrchr(line_ptr, ' ');
      /*  */
      if (line_ptr != NULL) {
        /* Null terminate */
        *line_ptr = 0;
        /* Skip former blank (now null character) */
        line_ptr++;
        /* Advance argument counter */
      }
      argc++;
    } while ( line_ptr != NULL && argc < CMDL_MAX_ARGC);

    /* call "would be main()" */
    if (argc > 2 && *argv_ptr[1] != '#' && FDKstrlen(argv_ptr[1])>1)
    {
      int retval;

      retval = (*pFunction)(argc, argv_ptr);

      FDKprintf("main returned %d\n", retval);
    }
  }

  FDKfclose(config_fp);
  return 0;
}

