/*
 * AdPlug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2020 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * strnlen.h - Provide strnlen() if not available in the system library,
 *             by Alexander Miller <alex.miller@gmx.de>.
 */
#ifndef ADPLUG_STRNLEN_H
#define ADPLUG_STRNLEN_H

/*
strnlen() is a useful function, available in GNU libc and on newer
Windows versions, but it's not part of any standard and many systems
don't support it. The C11 standard defines strnlen_s() with the
same functionality, but as with all bounds-checked functions, support
is still optional.

This header provides strnlen() for all systems such that the first
available implementation from the following list is used:
1. the system's strlen() (defined as a macro or function),
2. the system's strlen_s() (macro or function),
3. function template from this header.
*/

#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>

#ifndef strnlen
#ifndef strnlen_s
template <class Char>
size_t strnlen_s(const Char *s, size_t size)
{
	size_t i = 0;
	while (i < size && s[i]) i++;
	return i;
}
#endif // strnlen_s

template <class Char>
static inline size_t strnlen(const Char *s, size_t size)
{
	return strnlen_s(s, size);
}
#endif // strnlen

#endif // ADPLUG_STRNLEN_H
