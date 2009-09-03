/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __JUNKLIB_H
#define __JUNKLIB_H

#include <stdio.h>

struct playItem_s;

int
junk_read_id3v1 (struct playItem_s *it, FILE *fp);

int
junk_read_id3v2 (struct playItem_s *it, FILE *fp);

int
junk_read_ape (struct playItem_s *it, FILE *fp);

int
junk_get_leading_size (FILE *fp);

#endif // __JUNKLIB_H
