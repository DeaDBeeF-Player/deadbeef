/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "deadbeef.h"

struct playItem_s;

int
junk_read_id3v1 (struct playItem_s *it, DB_FILE *fp);

int
junk_read_id3v2_full (struct playItem_s *it, DB_id3v2_tag_t *tag, DB_FILE *fp);

int
junk_id3v2_convert_24_to_23 (DB_id3v2_tag_t *tag24, DB_id3v2_tag_t *tag23);

int
junk_id3v2_convert_23_to_24 (DB_id3v2_tag_t *tag23, DB_id3v2_tag_t *tag24);

DB_id3v2_frame_t *
junk_id3v2_add_text_frame_23 (DB_id3v2_tag_t *tag, const char *frame_id, const char *value);

DB_id3v2_frame_t *
junk_id3v2_add_text_frame_24 (DB_id3v2_tag_t *tag, const char *frame_id, const char *value);

int
junk_id3v2_remove_frames (DB_id3v2_tag_t *tag, const char *frame_id);

int
junk_write_id3v2 (const char *fname, DB_id3v2_tag_t *tag);

void
junk_free_id3v2 (DB_id3v2_tag_t *tag);

int
junk_read_id3v2 (struct playItem_s *it, DB_FILE *fp);

int
junk_read_ape (struct playItem_s *it, DB_FILE *fp);

int
junk_get_leading_size_stdio (FILE *fp);

int
junk_get_leading_size (DB_FILE *fp);

const char *
junk_detect_charset (const char *s);

void
junk_recode (const char *in, int inlen, char *out, int outlen, const char *cs);

#endif // __JUNKLIB_H
