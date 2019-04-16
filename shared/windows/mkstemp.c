/*
 * mkstemp.c
 *
 * $Id: mkstemp.c,v 1.1 2007-04-06 22:34:56 keithmarshall Exp $
 *
 * Provides a trivial replacement for the POSIX `mkstemp()' function,
 * suitable for use in MinGW (Win32) applications.
 *
 * 
 * This file is part of the MinGW32 package set.
 *
 * Contributed by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Last modification: 15-Dec-2006
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.1 $
 * $Author: keithmarshall $
 * $Date: 2007-04-06 22:34:56 $
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <share.h>

int mkstemp( char *template )
{
  int maxtry = 26, rtn = -1;

  while( maxtry-- && (rtn < 0) )
  {
    char *try = mktemp( template );
    if( try == NULL )
      return -1;
    rtn = sopen( try, O_RDWR | O_CREAT | O_EXCL | O_BINARY, SH_DENYRW, 0600 );
  }
  return rtn;
}

/* $RCSfile: mkstemp.c,v $Revision: 1.1 $: end of file */
