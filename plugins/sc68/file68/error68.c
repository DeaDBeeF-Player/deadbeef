/*
 *                      sc68 - Error message handling
 *         Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#define ERROR68_MAXERROR    8
#define ERROR68_MAXERRORSTR 256

#include <config68.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "file68/error68.h"

static char err[ERROR68_MAXERROR][ERROR68_MAXERRORSTR];
static int nerr = 0;

/* Push error message (printf like format) */
int SC68error_add(const char *format, ...)
{
  va_list list;
  int n;

  if (nerr < ERROR68_MAXERROR) {
    n = nerr++;
  } else {
    memmove(err[0], err[1], ERROR68_MAXERRORSTR * (ERROR68_MAXERROR - 1));
    n = ERROR68_MAXERROR - 1;
  }
  va_start(list, format);
  vsnprintf(err[n], ERROR68_MAXERRORSTR, format, list);
  err[n][ERROR68_MAXERRORSTR-1] = 0;
  va_end(list);

  return -1;
}

/* Pop error message. */
const char * SC68error_get(void)
{
  if (nerr <= 0) {
    nerr = 0;
    return 0;
  }
  return err[--nerr];
}
