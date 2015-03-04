/*
 *                 sc68 - Miscellaneous utility functions
 *         Copyright (C) 2001 Ben(jamin) Gerard <ben@sashipa.com>
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

#include "file68/endian68.h"

int SC68byte_order(void)
{
  static int endian = 0;

  if (!endian) {
    char * s;
    int i;
    for (i=0, s = (char *)&endian; i<sizeof(int); ++i) {
      s[i] = i;
    }
  }
  return endian;
}

int SC68little_endian(void)
{
  return !(SC68byte_order() & 255);
}

int SC68big_endian(void)
{
  return !SC68little_endian();
}

