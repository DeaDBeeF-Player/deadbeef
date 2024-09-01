/* junklib -- library for reading tags from various audio files for deadbeef player
  http://deadbeef.sourceforge.net

  Copyright (C) 2009-2013 Oleksiy Yakovenko

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#ifndef __JUNKLIB_H
#define __JUNKLIB_H

#include <stdio.h>
#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct playItem_s;

extern const char *ddb_internal_rg_keys[];

int
junk_id3v1_read (struct playItem_s *it, DB_FILE *fp);

int
junk_id3v1_find (DB_FILE *fp);

int
junk_id3v1_write (FILE *fp, struct playItem_s *it, const char *enc);

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
junk_id3v2_add_text_frame (DB_id3v2_tag_t *tag, const char *frame_id, const char *value);

DB_id3v2_frame_t *
junk_id3v2_add_text_frame2 (DB_id3v2_tag_t *tag, const char *frame_id, const char *value, size_t value_size);

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
junk_apev2_read_full_mem (struct playItem_s *it, DB_apev2_tag_t *tag_store, char *mem, int memsize);

int
junk_apev2_read (struct playItem_s *it, DB_FILE *fp);

int
junk_apev2_read_mem (struct playItem_s *it, char *mem, int size);

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

void
junk_enable_cp1251_detection (int enable);

void
junk_configchanged (void);

void
junk_enable_cp936_detection (int enable);

const char *
junk_detect_charset_len (const char *s, int len);

int
junk_get_tail_size (DB_FILE *fp);

void
junk_get_tag_offsets (DB_FILE *fp, uint32_t *head, uint32_t *tail);

unsigned
junk_stars_from_popm_rating (uint8_t rating);

uint8_t
junk_popm_rating_from_stars (unsigned stars);

void
junk_make_tdrc_string (char *tdrc, size_t tdrc_size, int year, int month, int day, int hour, int minute);

#ifdef __cplusplus
}
#endif

#endif // __JUNKLIB_H
