/*
 *                         sc68 - debug message
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

/* Copyright (C) 1998-2003 Benjamin Gerard */

#include <config68.h>
#include "file68/debugmsg68.h"

static debugmsg68_t debug = 0;
static void * debug_cookie = 0;

/** Set handler. */
debugmsg68_t debugmsg68_set_handler(debugmsg68_t handler)
{
  debugmsg68_t old = debug;
  debug = handler;
  return old;
}

/** Set cookie. */
void * debugmsg68_set_cookie(void * cookie)
{
  void * old = debug_cookie;
  debug_cookie = cookie;
  return old;
}

/** Print debug message. */
void debugmsg68(const char * fmt, ...)
{
  if (debug) {
    va_list list;
    va_start(list,fmt);
    debug(debug_cookie,fmt,list);
    va_end(list);
  }
}

/** Print debug message (variable argument). */
void vdebugmsg68(const char * fmt, va_list list)
{
  if (debug) {
    debug(debug_cookie, fmt, list);
  }
}

/** Wrapper for debugmsg68(). */
void SC68os_pdebug(const char *fmt, ...)
{
  if (debug) {
    va_list list;
    va_start(list,fmt);
    debug(debug_cookie,fmt,list);
    va_end(list);
  }
}
