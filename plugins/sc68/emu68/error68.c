/**
 * @ingroup   emu68_devel
 * @file      error68.c
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     Error message handler
 * @version   $Id: error68.c,v 2.0 2003/08/21 04:58:35 benjihan Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "emu68/error68.h"

#define MAXERROR    4
#define MAXERRORSTR 128

static char err[MAXERROR][MAXERRORSTR];
static int nerr = 0;

/*  Push error message
 */
int EMU68error_add(char *format, ... )
{
  va_list list;
  int     n;

  if(nerr<MAXERROR ) n = nerr++;
  else
  {
    memmove(err[0],err[1],MAXERRORSTR*(MAXERROR-1));
    n = MAXERROR;
  }
  va_start(list,format);
  vsprintf(err[n],format,list);
  va_end(list);

  return (int) ((0xDEAD<<(sizeof(int)*8-16)) | ( rand()&0xFFF ));
}

/*  Pop error message
 */
const char * EMU68error_get(void)
{
  if(nerr<=0)
  {
    nerr=0;
    return NULL;
  }
  return err[--nerr];
}
