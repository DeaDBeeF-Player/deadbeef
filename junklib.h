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
junk_id3v1_read (struct playItem_s *it, DB_FILE *fp);

int
junk_id3v1_find (DB_FILE *fp);

int
junk_id3v1_write (FILE *fp, struct playItem_s *it);

int
junk_id3v2_find (DB_FILE *fp, int *psize);

int
junk_id3v2_read_full (struct playItem_s *it, DB_id3v2_tag_t *tag, DB_FILE *fp);

int
junk_id3v2_convert_24_to_23 (DB_id3v2_tag_t *tag24, DB_id3v2_tag_t *tag23);

int
junk_id3v2_convert_23_to_24 (DB_id3v2_tag_t *tag23, DB_id3v2_tag_t *tag24);

int
junk_id3v2_convert_22_to_24 (DB_id3v2_tag_t *tag22, DB_id3v2_tag_t *tag24);

int
junk_id3v2_convert_apev2_to_24 (DB_apev2_tag_t *ape, DB_id3v2_tag_t *tag24);

DB_id3v2_frame_t *
junk_id3v2_add_text_frame_23 (DB_id3v2_tag_t *tag, const char *frame_id, const char *value);

DB_id3v2_frame_t *
junk_id3v2_add_text_frame_24 (DB_id3v2_tag_t *tag, const char *frame_id, const char *value);

int
junk_id3v2_remove_frames (DB_id3v2_tag_t *tag, const char *frame_id);

int
junk_id3v2_write (FILE *file, DB_id3v2_tag_t *tag);

void
junk_id3v2_free (DB_id3v2_tag_t *tag);

int
junk_id3v2_read (struct playItem_s *it, DB_FILE *fp);

int
junk_apev2_read_full (struct playItem_s *it, DB_apev2_tag_t *tag_store, DB_FILE *fp);

int
junk_apev2_read (struct playItem_s *it, DB_FILE *fp);

int
junk_apev2_find (DB_FILE *fp, int32_t *psize, uint32_t *pflags, uint32_t *pnumitems);

DB_apev2_frame_t *
junk_apev2_add_text_frame (DB_apev2_tag_t *tag, const char *frame_id, const char *value);

int
junk_apev2_remove_frames (DB_apev2_tag_t *tag, const char *frame_id);

void
junk_apev2_free (DB_apev2_tag_t *tag);

int
junk_apev2_write (FILE *fp, DB_apev2_tag_t *tag, int write_header, int write_footer);

int
junk_get_leading_size_stdio (FILE *fp);

int
junk_get_leading_size (DB_FILE *fp);

const char *
junk_detect_charset (const char *s);

int
junk_iconv (const char *in, int inlen, char *out, int outlen, const char *cs_in, const char *cs_out);

int
junk_recode (const char *in, int inlen, char *out, int outlen, const char *cs);

int
junk_rewrite_tags (struct playItem_s *it, uint32_t junk_flags, int id3v2_version, const char *id3v1_encoding);

#endif // __JUNKLIB_H
