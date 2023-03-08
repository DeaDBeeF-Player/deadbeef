/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  DeaDBeeF Ogg Edit library internal headers

  Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>

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

*/

#ifndef __OGGEDIT_INT_H
#define __OGGEDIT_INT_H

#include <stdbool.h>
#include <ogg/ogg.h>
#include <limits.h>
#include <deadbeef/deadbeef.h>
#include "oggedit.h"

#define OPUSNAME "Opus"
#define VORBISNAME "Vorbis"
#define FLACNAME "Flac"

#define CHUNKSIZE 4096
#define MAXPAGE 65536
#define MAXPAYLOAD 65025

void _oggpack_chars(oggpack_buffer *opb, const char *s, size_t length);
void _oggpack_string(oggpack_buffer *opb, const char *s);
void _oggpackB_string(oggpack_buffer *opb, const char *s);
int open_temp_file(const char *fname, char *tempname, FILE **out);
FILE *open_new_file(const char *outname);
off_t file_size(const char *fname);
void cleanup(DB_FILE *in, FILE *out, ogg_sync_state *oy, void *buffer);
int64_t copy_up_to_codec(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og, const off_t start_offset, const off_t link_offset, const char *codec);
int64_t copy_up_to_header(DB_FILE *in, FILE *out, ogg_sync_state *oy, ogg_page *og, const int64_t codec_serial);
int64_t flush_stream(FILE *out, ogg_stream_state *os);
char *codec_names(DB_FILE *in, ogg_sync_state *oy, const off_t link_offset);
off_t codec_stream_size(DB_FILE *in, ogg_sync_state *oy, const off_t start_offset, const off_t end_offset, const char *codec);
char *parse_vendor(const ogg_packet *op, const size_t magic_length);
int64_t init_read_stream(DB_FILE *in, ogg_sync_state *oy, ogg_stream_state *os, ogg_page *og, const off_t offset, const char *codec);
int64_t read_packet(DB_FILE *in, ogg_sync_state *oy, ogg_stream_state *os, ogg_page *og, ogg_packet *header, int64_t pages);
ogg_packet *fill_vc_packet(const char *magic, const size_t magic_length, const char *vendor, const size_t num_tags, char **tags, const bool framing, const size_t padding, ogg_packet *op);
size_t vc_size(const char *vendor, size_t num_tags, char **tags);
int64_t copy_remaining_pages(DB_FILE *in, FILE *out, ogg_sync_state *oy, const int64_t codec_serial, uint32_t pageno);
int64_t write_all_streams(DB_FILE *in, FILE *out, ogg_sync_state *oy, const off_t offset);
int64_t write_one_stream(DB_FILE *in, FILE *out, ogg_sync_state *oy, const off_t offset, const char *codec);

#endif /* __OGGEDIT_INT_H */
