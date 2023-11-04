/*  convert.c - functions to convert little-endian data to endian of host
 *  Copyright (C) 2000-2007  Jason Jordan <shnutils@freeshell.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * $Id: convert.c,v 1.9 2007/03/23 05:49:48 jason Exp $
 */

#include <stdlib.h>
#include "shorten.h"

ulong shn_uchar_to_ulong_le(uchar *buf)
/* converts 4 bytes stored in little-endian format to a ulong */
{
	return (ulong)((buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0]);
}

slong shn_uchar_to_slong_le(uchar *buf)
/* converts 4 bytes stored in little-endian format to an slong */
{
	return (slong)shn_uchar_to_ulong_le(buf);
}

ushort shn_uchar_to_ushort_le(uchar *buf)
/* converts 4 bytes stored in little-endian format to a ushort */
{
	return (ushort)((buf[1] << 8) + buf[0]);
}
