/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2017 Alexey Yakovenko and other contributors

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

#include "cueutil.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

extern DB_functions_t *deadbeef;

const char *cue_field_map[] = {
    "CATALOG ", "CATALOG",
    "REM DATE ", "year",
    "REM GENRE ", "genre",
    "REM COMMENT ", "comment",
    "REM COMPOSER ", "composer",
    "REM DISCNUMBER ", "disc",
    "REM TOTALDISCS ", "numdiscs",
    "REM DISCID ", "DISCID",
    NULL, NULL,
};
//#define MAX_EXTRA_TAGS_FROM_CUE ((sizeof(cue_field_map) / sizeof(cue_field_map[0])))

const uint8_t *
skipspaces (const uint8_t *p, const uint8_t *end) {
    while (p < end && *p <= ' ') {
        p++;
    }
    return p;
}

const uint8_t *
pl_cue_skipspaces (const uint8_t *p) {
    while (*p && *p <= ' ') {
        p++;
    }
    return p;
}

void
pl_get_qvalue_from_cue (const uint8_t *p, int sz, char *out, const char *charset) {
    char *str = out;
    if (*p == 0) {
        *out = 0;
        return;
    }
    p = pl_cue_skipspaces (p);
    if (*p == 0) {
        *out = 0;
        return;
    }

    if (*p == '"') {
        p++;
        p = pl_cue_skipspaces (p);
        while (*p && *p != '"' && sz > 1) {
            sz--;
            *out++ = *p++;
        }
        *out = 0;
    }
    else {
        while (*p && *p >= 0x20) {
            sz--;
            *out++ = *p++;
        }
        out--;
        while (out > str && *out == 0x20) {
            out--;
        }
        out++;
        *out = 0;
    }

    if (!charset) {
        return;
    }

    // recode
    size_t l = strlen (str);
    if (l == 0) {
        return;
    }

    char recbuf[l*10];
    int res = deadbeef->junk_recode (str, (int)l, recbuf, (int)(sizeof (recbuf)-1), charset);
    if (res >= 0) {
        strcpy (str, recbuf);
    }
    else
    {
        strcpy (str, "<UNRECOGNIZED CHARSET>");
    }
}

void
pl_get_value_from_cue (const char *p, int sz, char *out) {
    while (*p >= ' ' && sz > 1) {
        sz--;
        *out++ = *p++;
    }
    while (out > p && (*(out-1) == 0x20 || *(out-1) == 0x8)) {
        out--;
    }
    *out = 0;
}

float
pl_cue_parse_time (const char *p) {
    char *endptr;
    long mins = strtol(p, &endptr, 10);
    if (endptr - p < 1 || *endptr != ':') {
        return -1;
    }
    p = endptr + 1;
    long sec = strtol(p, &endptr, 10);
    if (endptr - p != 2 || *endptr != ':') {
        return -1;
    }
    p = endptr + 1;
    long frm = strtol(p, &endptr, 10);
    if (endptr - p != 2 || *endptr != '\0') {
        return -1;
    }
    return mins * 60.f + sec + frm / 75.f;
}

void
pl_cue_get_total_tracks_and_files(const uint8_t *buffer, const uint8_t *buffer_end, int *ncuefiles, int *ncuetracks) {
    const uint8_t *p = buffer;
    *ncuetracks = 0;
    *ncuefiles = 0;
    while (p < buffer_end) {
        p = skipspaces (p, buffer_end);
        if (p >= buffer_end) {
            break;
        }
        if (!strncasecmp (p, "FILE ", 5)) {
            *ncuefiles = *ncuefiles + 1;
        }
        if (!strncasecmp (p, "TRACK ", 6)) {
            *ncuetracks = *ncuetracks + 1;
        }
        // move pointer to the next line
        while (p < buffer_end && *p >= 0x20) {
           p++;
        }
    }
}

void
pl_cue_set_track_field_values(DB_playItem_t *it, char cuefields[CUE_MAX_FIELDS][255], char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255], int extra_tag_index) {
    if (cuefields[CUE_FIELD_PERFORMER][0]) {
        deadbeef->pl_add_meta (it, "artist", cuefields[CUE_FIELD_PERFORMER]);
        if (cuefields[CUE_FIELD_ALBUM_PERFORMER][0] && strcmp (cuefields[CUE_FIELD_ALBUM_PERFORMER], cuefields[CUE_FIELD_PERFORMER])) {
            deadbeef->pl_add_meta (it, "album artist", cuefields[CUE_FIELD_ALBUM_PERFORMER]);
        }
    }
    else if (cuefields[CUE_FIELD_ALBUM_PERFORMER][0]) {
        deadbeef->pl_add_meta (it, "artist", cuefields[CUE_FIELD_ALBUM_PERFORMER]);
    }
    if (cuefields[CUE_FIELD_SONGWRITER][0]) {
        deadbeef->pl_add_meta (it, "SONGWRITER", cuefields[CUE_FIELD_SONGWRITER]);
    }
    else if (cuefields[CUE_FIELD_SONGWRITER][0]) {
        deadbeef->pl_add_meta (it, "SONGWRITER", cuefields[CUE_FIELD_SONGWRITER]);
    }
    if (cuefields[CUE_FIELD_ALBUM_TITLE][0]) {
        deadbeef->pl_add_meta (it, "album", cuefields[CUE_FIELD_ALBUM_TITLE]);
    }
    if (cuefields[CUE_FIELD_TRACK][0]) {
        deadbeef->pl_add_meta (it, "track", cuefields[CUE_FIELD_TRACK]);
    }
    if (cuefields[CUE_FIELD_TITLE][0]) {
        deadbeef->pl_add_meta (it, "title", cuefields[CUE_FIELD_TITLE]);
    }
    if (cuefields[CUE_FIELD_ISRC]) {
        deadbeef->pl_add_meta (it, "ISRC", cuefields[CUE_FIELD_ISRC]);
    }
    if (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN][0]) {
        deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, atof (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN]));
    }
    if (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK][0]) {
        deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, atof (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK]));
    }
    if (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN][0]) {
        deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, atof (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN]));
    }
    if (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK][0]) {
        deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, atof (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK]));
    }
    // add extra tags to tracks
    for (int y = 0; y < extra_tag_index; y += 2) {
        if (extra_tags[y+1]) {
            deadbeef->pl_add_meta (it, extra_tags[y], extra_tags[y+1]);
        }
    }
    // generated "total tracks" field
    deadbeef->pl_add_meta(it, "numtracks", cuefields[CUE_FIELD_TOTALTRACKS]);
}

void
pl_cue_reset_per_track_fields(char cuefields[CUE_MAX_FIELDS][255]) {
    //cuefields[CUE_FIELD_TRACK][0] = 0;
    cuefields[CUE_FIELD_TITLE][0] = 0;
    cuefields[CUE_FIELD_PREGAP][0] = 0;
    cuefields[CUE_FIELD_INDEX00][0] = 0;
    cuefields[CUE_FIELD_INDEX01][0] = 0;
    cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN][0] = 0;
    cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK][0] = 0;
    cuefields[CUE_FIELD_PERFORMER][0] = 0;
    cuefields[CUE_FIELD_SONGWRITER][0] = 0;
    cuefields[CUE_FIELD_ISRC][0] = 0;
}

// success:
//    CUE_FIELD_*
// error:
//    -1
int
pl_cue_get_field_value(const char *p, char cuefields[CUE_MAX_FIELDS][255], char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255], const char *charset, int have_track, int *extra_tag_index) {
    if (!strncasecmp (p, "FILE ", 5)) {
        pl_get_qvalue_from_cue (p + 5, sizeof (cuefields[CUE_FIELD_FILE]), cuefields[CUE_FIELD_FILE], charset);
        return CUE_FIELD_FILE;
    }
    else if (!strncasecmp (p, "TRACK ", 6)) {
        // this requires some post-processing
        // and the previous value must not be lost..
        //pl_get_value_from_cue (p + 6, sizeof (cuefields[CUE_FIELD_TRACK]), cuefields[CUE_FIELD_TRACK]);
        return CUE_FIELD_TRACK;
    }
    else if (!strncasecmp (p, "PERFORMER ", 10)) {
        if (!cuefields[CUE_FIELD_TRACK][0]) {
            pl_get_qvalue_from_cue (p + 10, sizeof (cuefields[CUE_FIELD_ALBUM_PERFORMER]), cuefields[CUE_FIELD_ALBUM_PERFORMER], charset);
            return CUE_FIELD_ALBUM_PERFORMER;
        }
        else {
             pl_get_qvalue_from_cue (p + 10, sizeof (cuefields[CUE_FIELD_PERFORMER]), cuefields[CUE_FIELD_PERFORMER], charset);
             return CUE_FIELD_PERFORMER;
        }
    }
    else if (!strncasecmp (p, "SONGWRITER ", 11)) {
        if (!cuefields[CUE_FIELD_TRACK][0]) {
            pl_get_qvalue_from_cue (p + 11, sizeof (cuefields[CUE_FIELD_ALBUM_SONGWRITER]), cuefields[CUE_FIELD_ALBUM_SONGWRITER], charset);
            return CUE_FIELD_ALBUM_SONGWRITER;
        }
        else {
            pl_get_qvalue_from_cue (p + 11, sizeof (cuefields[CUE_FIELD_SONGWRITER]), cuefields[CUE_FIELD_SONGWRITER], charset);
            return CUE_FIELD_SONGWRITER;
        }
    }
    else if (!strncasecmp (p, "TITLE ", 6)) {
        if (!have_track && !cuefields[CUE_FIELD_ALBUM_TITLE][0]) {
            pl_get_qvalue_from_cue (p + 6, sizeof (cuefields[CUE_FIELD_ALBUM_TITLE]), cuefields[CUE_FIELD_ALBUM_TITLE], charset);
            return CUE_FIELD_ALBUM_TITLE;
        }
        else {
            pl_get_qvalue_from_cue (p + 6, sizeof (cuefields[CUE_FIELD_TITLE]), cuefields[CUE_FIELD_TITLE], charset);
            return CUE_FIELD_TITLE;
        }
    }
    else if (!strncasecmp (p, "REM REPLAYGAIN_ALBUM_GAIN ", 26)) {
        pl_get_value_from_cue (p + 26, sizeof (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN]), cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN]);
        return CUE_FIELD_REPLAYGAIN_ALBUM_GAIN;
    }
    else if (!strncasecmp (p, "REM REPLAYGAIN_ALBUM_PEAK ", 26)) {
        pl_get_value_from_cue (p + 26, sizeof (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK]), cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK]);
        return CUE_FIELD_REPLAYGAIN_ALBUM_PEAK;
    }
    else if (!strncasecmp (p, "REM REPLAYGAIN_TRACK_GAIN ", 26)) {
        pl_get_value_from_cue (p + 26, sizeof (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN]), cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN]);
        return CUE_FIELD_REPLAYGAIN_TRACK_GAIN;
    }
    else if (!strncasecmp (p, "REM REPLAYGAIN_TRACK_PEAK ", 26)) {
        pl_get_value_from_cue (p + 26, sizeof (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK]), cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK]);
        return CUE_FIELD_REPLAYGAIN_TRACK_PEAK;
    }
    else if (!strncasecmp (p, "PREGAP ", 7)) {
        pl_get_value_from_cue (p + 7, sizeof (cuefields[CUE_FIELD_PREGAP]), cuefields[CUE_FIELD_PREGAP]);
        return CUE_FIELD_PREGAP;
    }
    else if (!strncasecmp (p, "INDEX 00 ", 9)) {
        pl_get_value_from_cue (p + 9, sizeof (cuefields[CUE_FIELD_INDEX00]), cuefields[CUE_FIELD_INDEX00]);
        return CUE_FIELD_INDEX00;
    }
    else if (!strncasecmp (p, "INDEX 01 ", 9)) {
        pl_get_value_from_cue (p + 9, sizeof (cuefields[CUE_FIELD_INDEX01]), cuefields[CUE_FIELD_INDEX01]);
        return CUE_FIELD_INDEX01;
    }
    else if (!strncasecmp (p, "INDEX ", 6)) {
        // INDEX 02, INDEX 03, INDEX 04, etc...
        // for practical purposes, store value of INDEX XX in fields[CUE_FIELD_INDEX01]
        // ( see playlist.c -> plt_process_cue_track()
        pl_get_value_from_cue (p + 9, sizeof (cuefields[CUE_FIELD_INDEX01]), cuefields[CUE_FIELD_INDEX01]);
        return CUE_FIELD_INDEX_X;
    }
    else if (!strncasecmp (p, "ISRC ", 5)) {
        pl_get_value_from_cue (p + 5, sizeof (cuefields[CUE_FIELD_ISRC]), cuefields[CUE_FIELD_ISRC]);
        return CUE_FIELD_ISRC;
    }
    else {
        // determine and get extra tags
        if (!have_track) {
            for (int m = 0; cue_field_map[m]; m += 2) {
                if (!strncasecmp (p, cue_field_map[m], strlen(cue_field_map[m]))) {
                    strcpy(extra_tags[*extra_tag_index],cue_field_map[m+1]);
                    pl_get_qvalue_from_cue (p + strlen(cue_field_map[m]), sizeof(extra_tags[*extra_tag_index+1]), extra_tags[*extra_tag_index+1], charset);
                    *extra_tag_index = *extra_tag_index + 2;
                    return CUE_MAX_FIELDS;
                }
            }
        }
        return -1;
//      fprintf (stderr, "got unknown line:\n%s\n", p);
    }
}
