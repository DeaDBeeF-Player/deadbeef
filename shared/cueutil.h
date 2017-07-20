/*
    DeaDBeeF -- the music player
    Copyright (C) 2017 Alexey Yakovenko and other contributors

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

#ifndef cueutil_h
#define cueutil_h

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#include "../deadbeef.h"

#define SKIP_BLANK_CUE_TRACKS 0
#define MAX_CUE_TRACKS 99

extern const char *cue_field_map[];
#define MAX_EXTRA_TAGS_FROM_CUE 16 //((sizeof(cue_field_map) / sizeof(cue_field_map[0])))

enum {
    CUE_FIELD_ALBUM_PERFORMER,
    CUE_FIELD_PERFORMER,
    CUE_FIELD_ALBUM_SONGWRITER,
    CUE_FIELD_SONGWRITER,
    CUE_FIELD_ALBUM_TITLE,
    CUE_FIELD_FILE,
    CUE_FIELD_TRACK,
    CUE_FIELD_TITLE,
    CUE_FIELD_PREGAP,
    CUE_FIELD_INDEX00,
    CUE_FIELD_INDEX01,
    CUE_FIELD_REPLAYGAIN_ALBUM_GAIN,
    CUE_FIELD_REPLAYGAIN_ALBUM_PEAK,
    CUE_FIELD_REPLAYGAIN_TRACK_GAIN,
    CUE_FIELD_REPLAYGAIN_TRACK_PEAK,
    CUE_FIELD_TOTALTRACKS,
    CUE_FIELD_ISRC,
    CUE_MAX_FIELDS,
};

#define CUE_FIELD_INDEX_X 100

const uint8_t *
skipspaces (const uint8_t *p, const uint8_t *end);

const uint8_t *
pl_cue_skipspaces (const uint8_t *p);

void
pl_get_qvalue_from_cue (const uint8_t *p, int sz, char *out, const char *charset);

void
pl_get_value_from_cue (const char *p, int sz, char *out);

float
pl_cue_parse_time (const char *p);

void
pl_cue_get_total_tracks_and_files(const uint8_t *buffer, const uint8_t *buffersize, int *ncuefiles, int *ncuetracks);

void
pl_cue_set_track_field_values(DB_playItem_t *it, char cuefields[CUE_MAX_FIELDS][255], char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255], int extra_tag_index);

void
pl_cue_reset_per_track_fields(char cuefields[CUE_MAX_FIELDS][255]);

int
pl_cue_get_field_value(const char *p, char cuefields[CUE_MAX_FIELDS][255], char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255], const char *charset, int have_track, int *extra_tag_index);

#endif /* cueutil_h */
