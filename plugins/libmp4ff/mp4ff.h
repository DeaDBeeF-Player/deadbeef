/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: mp4ff.h,v 1.27 2009/01/29 00:41:08 menno Exp $
**/

#ifndef MP4FF_H
#define MP4FF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include "mp4ff_int_types.h"
#endif
#include "mp4ffint.h"

/* API */

mp4ff_t *mp4ff_open_read(mp4ff_callback_t *f);
mp4ff_t *mp4ff_open_read_metaonly(mp4ff_callback_t *f);
mp4ff_t *mp4ff_open_read_coveronly(mp4ff_callback_t *f);
void mp4ff_close(mp4ff_t *f);
int32_t mp4ff_get_sample_duration(const mp4ff_t *f, const int32_t track, const int32_t sample);
int32_t mp4ff_get_num_sample_byte_sizes (const mp4ff_t *f, const int32_t track);
int32_t mp4ff_get_sample_info(const mp4ff_t *f, const int32_t track, const int32_t samplenum, uint32_t *sample_duration, uint32_t *sample_byte_size);
int32_t mp4ff_get_sample_duration_use_offsets(const mp4ff_t *f, const int32_t track, const int32_t sample);
int64_t mp4ff_get_sample_position(const mp4ff_t *f, const int32_t track, const int32_t sample);
int32_t mp4ff_get_sample_offset(const mp4ff_t *f, const int32_t track, const int32_t sample);
int32_t mp4ff_find_sample(const mp4ff_t *f, const int32_t track, const int64_t offset,int32_t * toskip);
int32_t mp4ff_find_sample_use_offsets(const mp4ff_t *f, const int32_t track, const int64_t offset,int32_t * toskip);

int32_t mp4ff_read_sample(mp4ff_t *f, const int track, const int sample,
                          unsigned char **audio_buffer,  unsigned int *bytes);

int32_t mp4ff_read_sample_v2(mp4ff_t *f, const int track, const int sample,unsigned char *buffer);//returns 0 on error, number of bytes read on success, use mp4ff_read_sample_getsize() to check buffer size needed
int32_t mp4ff_read_sample_getsize(mp4ff_t *f, const int track, const int sample);//returns 0 on error, buffer size needed for mp4ff_read_sample_v2() on success



int32_t mp4ff_get_decoder_config(const mp4ff_t *f, const int track,
                             unsigned char** ppBuf, unsigned int* pBufSize);
int32_t mp4ff_get_track_type(const mp4ff_t *f, const int track);
int32_t mp4ff_get_track_id(const mp4ff_t *f, const int track);
int32_t mp4ff_get_track_fmt_cat(const mp4ff_t *f, const int track);
int32_t mp4ff_get_track_fmt_codec(const mp4ff_t *f, const int track);
int32_t mp4ff_total_tracks(const mp4ff_t *f);
int32_t mp4ff_num_samples(const mp4ff_t *f, const int track);
int32_t mp4ff_time_scale(const mp4ff_t *f, const int track);

uint32_t mp4ff_get_avg_bitrate(const mp4ff_t *f, const int32_t track);
uint32_t mp4ff_get_max_bitrate(const mp4ff_t *f, const int32_t track);
int64_t mp4ff_get_track_duration(const mp4ff_t *f, const int32_t track); //returns (-1) if unknown
int64_t mp4ff_get_track_duration_use_offsets(const mp4ff_t *f, const int32_t track); //returns (-1) if unknown
uint32_t mp4ff_get_sample_rate(const mp4ff_t *f, const int32_t track);
uint32_t mp4ff_get_channel_count(const mp4ff_t * f,const int32_t track);
uint32_t mp4ff_get_audio_type(const mp4ff_t * f,const int32_t track);


/* metadata */
int mp4ff_meta_get_num_items(const mp4ff_t *f);
int mp4ff_meta_get_by_index(const mp4ff_t *f, unsigned int index,
                            char **item, char **value);
int mp4ff_meta_find_by_name(const mp4ff_t *f, const char *item, char **value);
int mp4ff_meta_get_title(const mp4ff_t *f, char **value);
int mp4ff_meta_get_artist(const mp4ff_t *f, char **value);
int mp4ff_meta_get_writer(const mp4ff_t *f, char **value);
int mp4ff_meta_get_album(const mp4ff_t *f, char **value);
int mp4ff_meta_get_date(const mp4ff_t *f, char **value);
int mp4ff_meta_get_tool(const mp4ff_t *f, char **value);
int mp4ff_meta_get_comment(const mp4ff_t *f, char **value);
int mp4ff_meta_get_genre(const mp4ff_t *f, char **value);
int mp4ff_meta_get_track(const mp4ff_t *f, char **value);
int mp4ff_meta_get_disc(const mp4ff_t *f, char **value);
int mp4ff_meta_get_totaltracks(const mp4ff_t *f, char **value);
int mp4ff_meta_get_totaldiscs(const mp4ff_t *f, char **value);
int mp4ff_meta_get_compilation(const mp4ff_t *f, char **value);
int mp4ff_meta_get_tempo(const mp4ff_t *f, char **value);

// The mp4ff_meta_get_coverart doesn't function correctly, and should be avoided
// Please use mp4ff_cover_get instead
int32_t mp4ff_meta_get_coverart(const mp4ff_t *f, char **value);
#ifdef USE_TAGGING

int32_t mp4ff_chapters_get_num_items (mp4ff_t *f);
const char *mp4ff_chapters_get_item (mp4ff_t *f, int i);
int32_t mp4ff_chap_get_num_tracks (mp4ff_t *f);
int32_t mp4ff_chap_get_track_id (mp4ff_t *f, int t);
#if 0
int64_t mp4ff_get_track_dts (mp4ff_t *f, int t, int s);
int64_t mp4ff_get_track_pts_delta(mp4ff_t *f, int t, int s);
int mp4ff_get_track_sample_size(mp4ff_t *f, int t, int s);
#endif

int32_t mp4ff_tag_delete(mp4ff_metadata_t *tags);
int32_t mp4ff_meta_update(mp4ff_callback_t *f,const mp4ff_metadata_t * data);
int32_t mp4ff_tag_add_field(mp4ff_metadata_t *tags, const char *item, const char *value);
int32_t mp4ff_tag_set_field(mp4ff_metadata_t *tags, const char *item, const char *value);
int32_t mp4ff_set_metadata_name(mp4ff_t *f, const uint8_t atom_type, char **name);
int32_t mp4ff_parse_tag(mp4ff_t *f, const uint8_t parent_atom_type, const int32_t size);

void mp4ff_cover_delete (mp4ff_cover_art_t *cover);
void mp4ff_cover_append_item (mp4ff_t *f, char *data, uint32_t datasize);

// returns a linked list of all available cover art images
mp4ff_cover_art_t *mp4ff_cover_get (mp4ff_t *f);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
