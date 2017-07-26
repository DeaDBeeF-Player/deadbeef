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

/* this is basically part of playlist.c */

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include "plugins.h"
#include "common.h"
#include "playlist.h"
#include "junklib.h"
#include "vfs.h"

#include "cueutil.h"

extern int conf_cue_subindexes_as_tracks;

#define SKIP_BLANK_CUE_TRACKS 0
#define MAX_CUE_TRACKS 99

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
#define MAX_EXTRA_TAGS_FROM_CUE ((sizeof(cue_field_map) / sizeof(cue_field_map[0])))


static const uint8_t *
skipspaces (const uint8_t *p, const uint8_t *end) {
    while (p < end && *p <= ' ') {
        p++;
    }
    return p;
}

static const uint8_t *
pl_cue_skipspaces (const uint8_t *p) {
    while (*p && *p <= ' ') {
        p++;
    }
    return p;
}

void
static pl_get_qvalue_from_cue (const uint8_t *p, int sz, char *out, const char *charset) {
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
    int res = junk_recode (str, (int)l, recbuf, (int)(sizeof (recbuf)-1), charset);
    if (res >= 0) {
        strcpy (str, recbuf);
    }
    else
    {
        strcpy (str, "<UNRECOGNIZED CHARSET>");
    }
}

void
static pl_get_value_from_cue (const char *p, int sz, char *out) {
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
static pl_cue_parse_time (const char *p) {
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
static pl_cue_get_total_tracks_and_files(const uint8_t *buffer, const uint8_t *buffer_end, int *ncuefiles, int *ncuetracks) {
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
static pl_cue_set_track_field_values(playItem_t *it, char cuefields[CUE_MAX_FIELDS][255], char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255], int extra_tag_index) {
    if (cuefields[CUE_FIELD_PERFORMER][0]) {
        pl_add_meta (it, "artist", cuefields[CUE_FIELD_PERFORMER]);
        if (cuefields[CUE_FIELD_ALBUM_PERFORMER][0] && strcmp (cuefields[CUE_FIELD_ALBUM_PERFORMER], cuefields[CUE_FIELD_PERFORMER])) {
            pl_add_meta (it, "album artist", cuefields[CUE_FIELD_ALBUM_PERFORMER]);
        }
    }
    else if (cuefields[CUE_FIELD_ALBUM_PERFORMER][0]) {
        pl_add_meta (it, "artist", cuefields[CUE_FIELD_ALBUM_PERFORMER]);
    }
    if (cuefields[CUE_FIELD_SONGWRITER][0]) {
        pl_add_meta (it, "SONGWRITER", cuefields[CUE_FIELD_SONGWRITER]);
    }
    else if (cuefields[CUE_FIELD_SONGWRITER][0]) {
        pl_add_meta (it, "SONGWRITER", cuefields[CUE_FIELD_SONGWRITER]);
    }
    if (cuefields[CUE_FIELD_ALBUM_TITLE][0]) {
        pl_add_meta (it, "album", cuefields[CUE_FIELD_ALBUM_TITLE]);
    }
    if (cuefields[CUE_FIELD_TRACK][0]) {
        pl_add_meta (it, "track", cuefields[CUE_FIELD_TRACK]);
    }
    if (cuefields[CUE_FIELD_TITLE][0]) {
        pl_add_meta (it, "title", cuefields[CUE_FIELD_TITLE]);
    }
    if (cuefields[CUE_FIELD_ISRC]) {
        pl_add_meta (it, "ISRC", cuefields[CUE_FIELD_ISRC]);
    }
    if (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN][0]) {
        pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, atof (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN]));
    }
    if (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK][0]) {
        pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, atof (cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK]));
    }
    if (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN][0]) {
        pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, atof (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN]));
    }
    if (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK][0]) {
        pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, atof (cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK]));
    }
    // add extra tags to tracks
    for (int y = 0; y < extra_tag_index; y += 2) {
        if (extra_tags[y+1]) {
            pl_add_meta (it, extra_tags[y], extra_tags[y+1]);
        }
    }
    // generated "total tracks" field
    pl_add_meta(it, "numtracks", cuefields[CUE_FIELD_TOTALTRACKS]);
}

void
static pl_cue_reset_per_track_fields(char cuefields[CUE_MAX_FIELDS][255]) {
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
static pl_cue_get_field_value(const char *p, char cuefields[CUE_MAX_FIELDS][255], char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255], const char *charset, int have_track, int *extra_tag_index) {
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
    }
}

//========================================================================

static playItem_t *
plt_process_cue_track (playlist_t *playlist, const char *fname, const int64_t startsample, playItem_t **prev, const char *decoder_id, const char *ftype, int samplerate, char cuefields[CUE_MAX_FIELDS][255], char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255], int extra_tag_index) {
    if (!cuefields[CUE_FIELD_TRACK][0]) {
        trace ("pl_process_cue_track: invalid track (file=%s, title=%s)\n", fname, cuefields[CUE_FIELD_TITLE]);
        return NULL;
    }
    // index00 is irrelevant
    if (!cuefields[CUE_FIELD_INDEX01][0]) {
        trace ("pl_process_cue_track: invalid index (file=%s, title=%s, track=%s)\n", fname, cuefields[CUE_FIELD_TITLE], cuefields[CUE_FIELD_TRACK]);
        return NULL;
    }
#if SKIP_BLANK_CUE_TRACKS
    if (!title[0]) {
        trace ("pl_process_cue_track: invalid title (file=%s, title=%s, track=%s)\n", fname, cuefields[CUE_FIELD_TITLE], cuefields[CUE_FIELD_TRACK]);
        return NULL;
    }
#endif
    // fix track number
    char *p = cuefields[CUE_FIELD_TRACK];
    while (*p && isdigit (*p)) {
        p++;
    }
    *p = 0;
    // check that indexes have valid timestamps
    //float f_index00 = cuefields[CUE_FIELD_INDEX00][0] ? pl_cue_parse_time (cuefields[CUE_FIELD_INDEX00]) : 0;
    float f_index01 = cuefields[CUE_FIELD_INDEX01][0] ? pl_cue_parse_time (cuefields[CUE_FIELD_INDEX01]) : 0;
    float f_pregap = cuefields[CUE_FIELD_PREGAP][0] ? pl_cue_parse_time (cuefields[CUE_FIELD_PREGAP]) : 0;
    if (*prev) {
        float prevtime = 0;
        if (cuefields[CUE_FIELD_PREGAP][0] && cuefields[CUE_FIELD_INDEX01][0]) {
            // PREGAP command
            prevtime = f_index01 - f_pregap;
        }
//        else if (cuefields[CUE_FIELD_INDEX00][0] && cuefields[CUE_FIELD_INDEX01][0]) {
//            // pregap in index 00
//            prevtime = f_cuefields[CUE_FIELD_INDEX00];
//        }
        else if (cuefields[CUE_FIELD_INDEX01][0]) {
            // no pregap
            prevtime = f_index01;
        }
        else {
            trace ("pl_process_cue_track: invalid pregap or index01 (pregap=%s, index01=%s)\n", cuefields[CUE_FIELD_PREGAP], cuefields[CUE_FIELD_INDEX01]);
            return NULL;
        }
        // knowing the startsample of the current track,
        // now it's possible to calculate startsample and duration of the previous one.
        pl_item_set_endsample (*prev, startsample + (prevtime * samplerate) - 1);
        plt_set_item_duration (playlist, *prev, (float)(pl_item_get_endsample (*prev) - pl_item_get_startsample (*prev) + 1) / samplerate);

        if (pl_get_item_duration (*prev) < 0) {
            // might be bad cuesheet file, try to fix
            trace ("cuesheet seems to be corrupted, trying workaround\n");
            prevtime = f_index01;
            pl_item_set_endsample (*prev, startsample + (prevtime * samplerate) - 1);
            float dur = (float)(pl_item_get_endsample (*prev) - pl_item_get_startsample (*prev) + 1) / samplerate;
            plt_set_item_duration (playlist, *prev, dur);
        }
    }
    // non-compliant hack to handle tracks which only store pregap info
    if (!cuefields[CUE_FIELD_INDEX01][0]) {
        *prev = NULL;
        trace ("pl_process_cue_track: invalid index01 (pregap=%s, index01=%s)\n", cuefields[CUE_FIELD_PREGAP], cuefields[CUE_FIELD_INDEX01]);
        return NULL;
    }
    playItem_t *it = pl_item_alloc_init (fname, decoder_id);
    pl_set_meta_int (it, ":TRACKNUM", atoi (cuefields[CUE_FIELD_TRACK]));
    pl_item_set_startsample (it, cuefields[CUE_FIELD_INDEX01][0] ? startsample + f_index01 * samplerate : startsample);
    pl_item_set_endsample (it, -1); // will be filled by next read, or by decoder
    pl_replace_meta (it, ":FILETYPE", ftype);
    it->_flags |= DDB_IS_SUBTRACK | DDB_TAG_CUESHEET;

    pl_cue_set_track_field_values(it, cuefields, extra_tags, extra_tag_index);

    *prev = it;
    return it;
}

//========================================================================

static int
_file_exists (const char *fname) {
    struct stat s;
    memset (&s, 0, sizeof (s));
    if (!stat (fname, &s) && (s.st_mode & S_IFREG)) {
        return 1;
    }
    return 0;
}

static int
_set_last_item_region (playlist_t *plt, playItem_t *it, playItem_t *origin, int64_t totalsamples, int samplerate) {
    pl_item_set_endsample (it, pl_item_get_startsample (origin) + totalsamples - 1);
    if ((pl_item_get_endsample (it) - pl_item_get_startsample (origin)) >= totalsamples || (pl_item_get_startsample (it)-pl_item_get_startsample (origin)) >= totalsamples) {
        return -1;
    }
    plt_set_item_duration (plt, it, (float)(pl_item_get_endsample (it) - pl_item_get_startsample (it) + 1) / samplerate);
    return 0;
}

playItem_t *
plt_load_cue_file (playlist_t *plt, playItem_t *after, const char *fname, const char *dirname, struct dirent **namelist, int n) {
    playItem_t *ins = after;
    char resolved_fname[PATH_MAX];
    playItem_t *cuetracks[MAX_CUE_TRACKS];
    int ntracks = 0;

    uint8_t *membuffer = NULL;
    playlist_t *temp_plt = calloc (1, sizeof (playlist_t));
    temp_plt->loading_cue = 1;

    char *res = realpath (fname, resolved_fname);
    if (res) {
        fname = resolved_fname;
    }

    const char *slash = strrchr (fname, '/');
    char cue_file_dir[strlen(fname)]; // can't be longer than fname
    cue_file_dir[0] = 0;
    const char *cue_fname = NULL;
    if (slash && slash > fname) {
        strncat (cue_file_dir, fname, slash - fname);
        cue_fname = slash+1;
    }

    DB_FILE *fp = vfs_fopen (fname);
    if (!fp) {
        goto error;
    }

    int sz = (int)vfs_fgetlength (fp);
    membuffer = malloc (sz);
    if (!membuffer) {
        vfs_fclose (fp);
        trace ("failed to allocate %d bytes to read the file %s\n", sz, fname);
        goto error;
    }
    uint8_t *buffer = membuffer;
    vfs_fread (buffer, 1, sz, fp);
    vfs_fclose (fp);

    if (sz >= 3 && buffer[0] == 0xef && buffer[1] == 0xbb && buffer[2] == 0xbf) {
        buffer += 3;
        sz -= 3;
    }

    const char *charset = junk_detect_charset (buffer);

    const uint8_t *p = buffer;
    const uint8_t *end = buffer+sz;

    // determine total tracks/files
    int ncuetracks, ncuefiles;
    pl_cue_get_total_tracks_and_files(p, end, &ncuefiles, &ncuetracks);
    if (!ncuefiles || !ncuetracks) {
        trace_err("invalid cuesheet (%s)\n", fname);
        goto error;
    }


    pl_lock();

    char fullpath[PATH_MAX];

    char cuefields[CUE_MAX_FIELDS][255];
    memset(cuefields, 0, sizeof(cuefields));
    snprintf(cuefields[CUE_FIELD_TOTALTRACKS], sizeof(cuefields[CUE_FIELD_TOTALTRACKS]), "%d", ncuetracks);

    char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255];
    memset (extra_tags, 0, sizeof (extra_tags));
    int extra_tag_index = 0;

    int have_track = 0;

    playItem_t *origin = NULL;
    playItem_t *prev = NULL;
    const char *dec = NULL;
    const char *filetype = NULL;

    ntracks = 0;

    p = buffer;
    while (p < end) {
        p = skipspaces (p, end);
        if (p >= end) {
            break;
        }

        int field = pl_cue_get_field_value(p, cuefields, extra_tags, charset, have_track, &extra_tag_index);

        if (field == CUE_FIELD_FILE) {
            char *audio_file = cuefields[CUE_FIELD_FILE];
            if (origin) {
                // last track of current file
                if (have_track) {
                    playItem_t *it = plt_process_cue_track (plt, fullpath, pl_item_get_startsample (origin), &prev, dec, filetype, temp_plt->cue_samplerate, cuefields, extra_tags, extra_tag_index);
                    if (it) {
                        cuetracks[ntracks++] = it;
                        if (_set_last_item_region (plt, it, origin, temp_plt->cue_numsamples, temp_plt->cue_samplerate)) {
                            break;
                        }
                    }
                }
                origin = NULL;
            }

            pl_cue_reset_per_track_fields(cuefields);
            prev = NULL;
            have_track = 0;

            // full path in CUE entry
            if (audio_file[0] == '/' && _file_exists (audio_file)) {
                strcpy (fullpath, audio_file);
            }
            else {
                // try relative path
                snprintf(fullpath, sizeof(fullpath), "%s/%s", cue_file_dir, audio_file);

                if (!_file_exists (fullpath)) {
                    fullpath[0] = 0;
                    if (namelist) {
                        // for image+cue, try guessing the audio filename from cuesheet filename
                        if (ncuefiles == 1) {
                            size_t l = strlen (cue_fname);
                            for (int i = 0; i < n; i++) {
                                const char *ext = strrchr (namelist[i]->d_name, '.');
                                if (!ext || !strcasecmp (ext, ".cue")) {
                                    continue;
                                }

                                if (!strncasecmp (cue_fname, namelist[i]->d_name, l-4)) {
                                    // have to try loading each of these files
                                    snprintf (fullpath, sizeof (fullpath), "%s/%s", dirname, namelist[i]->d_name);
                                    origin = plt_insert_file2 (-1, temp_plt, after, fullpath, NULL, NULL, NULL);
                                    if (origin) {
                                        break;
                                    }
                                }
                            }
                        }
                        else {
                            // for tracks+cue, try guessing the extension of the FILE value
                            char *ext = strrchr (audio_file, '.');
                            if (ext) {
                                *ext = 0;
                            }
                            for (int i = 0; i < n; i++) {
                                const char *cueext = strrchr (namelist[i]->d_name, '.');
                                if (!cueext || !strcasecmp (cueext, ".cue")) {
                                    continue;
                                }

                                if (!strncasecmp (audio_file, namelist[i]->d_name, ext-audio_file)
                                    && namelist[i]->d_name[ext-audio_file] == '.') {
                                    // have to try loading each of these files
                                    snprintf (fullpath, sizeof (fullpath), "%s/%s", dirname, namelist[i]->d_name);
                                    origin = plt_insert_file2 (-1, temp_plt, after, fullpath, NULL, NULL, NULL);
                                    if (origin) {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (fullpath[0] && !origin) {
                origin = plt_insert_file2 (-1, temp_plt, after, fullpath, NULL, NULL, NULL);
            }

            if (!origin) {
                trace_err("Invalid FILE entry %s in cuesheet %s, and could not guess any suitable file name.\n", audio_file, fname);
                dec = filetype = NULL;
            }
            else {
                // now we got the image + totalsamples + samplerate,
                // process each track until next file
                dec = pl_find_meta_raw (origin, ":DECODER");
                filetype = pl_find_meta_raw (origin, ":FILETYPE");;
            }
        }
        else if (field == CUE_FIELD_TRACK) {
            if (!origin) {
                // this is an error with loading cuesheet, but it should have been already covered by previous message
            }
            else if (have_track) {
                playItem_t *it = plt_process_cue_track (plt, fullpath, pl_item_get_startsample (origin), &prev, dec, filetype, temp_plt->cue_samplerate, cuefields, extra_tags, extra_tag_index);
                if (it) {
                    if ((pl_item_get_startsample (it)-pl_item_get_startsample (origin)) >= temp_plt->cue_numsamples || (pl_item_get_endsample (it)-pl_item_get_startsample (origin)) >= temp_plt->cue_numsamples) {
                        goto error;
                    }
                    cuetracks[ntracks++] = it;
                }
            }
            pl_cue_reset_per_track_fields(cuefields);
            pl_get_value_from_cue (p + 6, sizeof (cuefields[CUE_FIELD_TRACK]), cuefields[CUE_FIELD_TRACK]);
            have_track = 1;
        }

        // move pointer to the next line
        while (p < end && *p >= 0x20) {
           p++;
        }
    }
    if (origin) {
        // last track of the cuesheet
        playItem_t *it = plt_process_cue_track (plt, fullpath, pl_item_get_startsample (origin), &prev, dec, filetype, temp_plt->cue_samplerate, cuefields, extra_tags, extra_tag_index);
        if (it) {
            _set_last_item_region (plt, it, origin, temp_plt->cue_numsamples, temp_plt->cue_samplerate);
            cuetracks[ntracks++] = it;
        }
    }

    for (int i = 0; i < ntracks; i++) {
        after = plt_insert_item (plt, after, cuetracks[i]);
        pl_item_unref (cuetracks[i]);
    }
    ntracks = 0;
    playItem_t *first = ins ? ins->next[PL_MAIN] : plt->head[PL_MAIN];
    if (!first) {
        after = NULL;
    }
    pl_unlock();

error:
    for (int i = 0; i < ntracks; i++) {
        pl_item_unref (cuetracks[i]);
    }
    if (membuffer) {
        free (membuffer);
    }
    if (temp_plt) {
        plt_free (temp_plt);
    }
    return after;
}

playItem_t *
plt_load_cuesheet_from_buffer (playlist_t *playlist, playItem_t *after, playItem_t *origin, const uint8_t *buffer, int buffersize, uint64_t numsamples, int samplerate) {
    if (buffersize >= 3 && buffer[0] == 0xef && buffer[1] == 0xbb && buffer[2] == 0xbf) {
        buffer += 3;
        buffersize -= 3;
    }

    // go through the file, and verify that it's not for multiple tracks
    uint8_t *p = (uint8_t *)buffer;
    uint8_t *end = (uint8_t *)(buffer + buffersize);
    int ncuetracks, ncuefiles;
    pl_cue_get_total_tracks_and_files(p, end, &ncuefiles, &ncuetracks);
    if (ncuefiles > 1 || !ncuefiles || !ncuetracks) {
        trace("Not loading cuesheet from buffer\n");
        return NULL;
    }

    const char *charset = junk_detect_charset_len (buffer, buffersize);

    pl_lock();
    playItem_t *ins = after;

    const char *uri = pl_find_meta_raw (origin, ":URI");
    const char *dec = pl_find_meta_raw (origin, ":DECODER");
    const char *filetype = pl_find_meta_raw (origin, ":FILETYPE");

    char cuefields[CUE_MAX_FIELDS][255];
    memset(cuefields, 0, sizeof(cuefields));
    snprintf(cuefields[CUE_FIELD_TOTALTRACKS], sizeof(cuefields[CUE_FIELD_TOTALTRACKS]), "%d", ncuetracks);

    char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][255];
    memset (extra_tags, 0, sizeof (extra_tags));
    int extra_tag_index = 0;

    int have_track = 0;
    int track_subindexes = 0;

    playItem_t *cuetracks[MAX_CUE_TRACKS];
    ncuetracks = 0;

    playItem_t *prev = NULL;
    while (buffersize > 0 && ncuetracks < MAX_CUE_TRACKS) {
        const uint8_t *p = buffer;
        // find end of line
        while (p - buffer < buffersize && *p >= 0x20) {
            p++;
        }
        // skip linebreak(s)
        while (p - buffer < buffersize && *p < 0x20) {
            p++;
        }
        if (p-buffer > 2048) { // huge string, ignore
            buffersize -= p-buffer;
            buffer = p;
            continue;
        }
        char str[p-buffer+1];
        strncpy (str, buffer, p-buffer);
        str[p-buffer] = 0;
        buffersize -= p-buffer;
        buffer = p;
        p = pl_cue_skipspaces (str);

        int field = pl_cue_get_field_value(p, cuefields, extra_tags, charset, have_track, &extra_tag_index);

        if (!conf_cue_subindexes_as_tracks) {
            /* normal operation */
            if (field == CUE_FIELD_TRACK) {
                if (have_track) {
                    // add previous track
                    playItem_t *it = plt_process_cue_track (playlist, uri, pl_item_get_startsample (origin), &prev, dec, filetype, samplerate, cuefields, extra_tags, extra_tag_index);
                    if (it) {
                        if ((pl_item_get_startsample (it)-pl_item_get_startsample (origin)) >= numsamples || (pl_item_get_endsample (it)-pl_item_get_startsample (origin)) >= numsamples) {
                            goto error;
                        }
                        cuetracks[ncuetracks++] = it;
                    }
                }
                pl_cue_reset_per_track_fields(cuefields);
                pl_get_value_from_cue (p + 6, sizeof (cuefields[CUE_FIELD_TRACK]), cuefields[CUE_FIELD_TRACK]);
                have_track = 1;
            }
        }
        else {
            /* subindexes as tracks */
            if (field == CUE_FIELD_TRACK) {
                if (have_track) {
                    //trace("track %s has %d subindexes\n", cuefields[CUE_FIELD_TRACK], track_subindexes);
                }
                track_subindexes = 0;
                pl_cue_reset_per_track_fields(cuefields);
                pl_get_value_from_cue (p + 6, sizeof (cuefields[CUE_FIELD_TRACK]), cuefields[CUE_FIELD_TRACK]);
                have_track = 1;
            }
            else if (field == CUE_FIELD_INDEX01 || field == CUE_FIELD_INDEX_X) {
                char indexnumber[10];
                if (field != CUE_FIELD_INDEX01) { // INDEX 02, INDEX 03, INDEX 04
                    // get " 02", " 03", etc
                    pl_get_value_from_cue (p + 5, sizeof (indexnumber), indexnumber);
                    if ( strlen(indexnumber) > 3 ) {
                        indexnumber[0] = '_';
                        indexnumber[3] = 0;
                    }
                    // append _02, _03, etc to track title)
                    if ( (strlen(cuefields[CUE_FIELD_TITLE]) + 3) < 255) {
                        strncat(cuefields[CUE_FIELD_TITLE], indexnumber, 3);
                    }
                    //trace("title: %s\n", cuefields[CUE_FIELD_TITLE]);
                }
                // insert SUBINDEX as TRACK
                playItem_t *it = plt_process_cue_track (playlist, uri, pl_item_get_startsample (origin), &prev, dec, filetype, samplerate, cuefields, extra_tags, extra_tag_index);
                if (it) {
                    if ((pl_item_get_startsample (it)-pl_item_get_startsample (origin)) >= numsamples || (pl_item_get_endsample (it)-pl_item_get_startsample (origin)) >= numsamples) {
                        goto error;
                    }
                    cuetracks[ncuetracks++] = it;
                }
                if (field != CUE_FIELD_INDEX01 && strlen(indexnumber) == 3) {
                    // INDEX XX: have to restore the original title
                    size_t a = strlen(cuefields[CUE_FIELD_TITLE]);
                    cuefields[CUE_FIELD_TITLE][a-3] = 0;
                }
                track_subindexes++;
            }
        }
    } /* end of while loop */

    if (!conf_cue_subindexes_as_tracks) {
        /* normal operation */
        if (have_track) {
            // handle last track
            playItem_t *last_track = plt_process_cue_track (playlist, uri, pl_item_get_startsample (origin), &prev, dec, filetype, samplerate, cuefields, extra_tags, extra_tag_index);
            if (last_track) {
                pl_item_set_endsample (last_track, pl_item_get_startsample (origin) + numsamples - 1);
                if ((pl_item_get_endsample (last_track)-pl_item_get_startsample (origin)) >= numsamples || (pl_item_get_startsample (last_track)-pl_item_get_startsample (origin)) >= numsamples) {
                    goto error;
                }
                plt_set_item_duration (playlist, last_track, (float)(pl_item_get_endsample (last_track) - pl_item_get_startsample (last_track) + 1) / samplerate);
                cuetracks[ncuetracks++] = last_track;
            }
        }
        pl_item_ref(cuetracks[ncuetracks-1]);
    }
    else {
        /* subindexes as tracks */
        if (have_track || track_subindexes > 1) {
            playItem_t *last_track = cuetracks[ncuetracks-1];
            pl_item_set_endsample (last_track, pl_item_get_startsample (origin) + numsamples - 1);
            if ((pl_item_get_endsample (last_track)-pl_item_get_startsample (origin)) >= numsamples || (pl_item_get_startsample (last_track)-pl_item_get_startsample (origin)) >= numsamples) {
                goto error;
            }
            plt_set_item_duration (playlist, last_track, (float)(pl_item_get_endsample (last_track) - pl_item_get_startsample (last_track) + 1) / samplerate);
            pl_item_ref(last_track);
        }
    }

    for (int i = 0; i < ncuetracks; i++) {
        after = plt_insert_item (playlist, after, cuetracks[i]);
        pl_item_unref (cuetracks[i]);
    }
    playItem_t *first = ins ? ins->next[PL_MAIN] : playlist->head[PL_MAIN];
    if (!first) {
        pl_unlock();
        return NULL;
    }
    // copy metadata from embedded tags
    uint32_t f = pl_get_item_flags (origin);
    f |= DDB_TAG_CUESHEET | DDB_IS_SUBTRACK;
    if (pl_find_meta_raw (origin, "cuesheet")) {
        f |= DDB_HAS_EMBEDDED_CUESHEET;
    }
    pl_set_item_flags (origin, f);
    pl_items_copy_junk (origin, first, after);
    pl_unlock();
    return after;
error:
    trace ("cue parsing error occured\n");
    for (int i = 0; i < ncuetracks; i++) {
        pl_item_unref (cuetracks[i]);
    }
    pl_unlock();
    return NULL;
}

