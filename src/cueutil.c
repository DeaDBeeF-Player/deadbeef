/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2017 Oleksiy Yakovenko and other contributors

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

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include "plugins.h"
#include <deadbeef/common.h>
#include "plmeta.h"
#include "junklib.h"
#include "vfs.h"
#include "playlist.h"
#include "cueutil.h"

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

#define CUE_FIELD_LEN 255

typedef struct {
    // initial values
    const char *fname; // cue of parent file name
    playItem_t *embedded_origin; // parent track of embedded cue
    int64_t embedded_numsamples;
    int embedded_samplerate;
    const char *cue_file_dir; // directory containing cue file or parent file (FIXME: looks like a dupe with `dirname`)
    const char *dirname; // directory path being loaded
    struct dirent **namelist;
    int n;
    int ncuefiles; // number of FILEs in cue
    int ncuetracks; // number of TRACKs in cue
    const char *cue_fname; // just the filename of cue file (or parent file)
    playlist_t *target_playlist; // for filtering
    playlist_t *temp_plt; // temporary playlist, for loading tracks before splitting

    // intermediate / output values
    char fullpath[PATH_MAX]; // current file name to be split

    char cuefields[CUE_MAX_FIELDS][CUE_FIELD_LEN];
    char extra_tags[MAX_EXTRA_TAGS_FROM_CUE][CUE_FIELD_LEN];
    int extra_tag_index;
    char extra_track_tags[MAX_EXTRA_TAGS_FROM_CUE][CUE_FIELD_LEN];
    int extra_track_tag_index;

    int64_t numsamples;
    int samplerate;
    playItem_t *origin; // current unsplit track, loaded from last FILE
    int first_track; // set to 1 immediately after loading a FILE, reset to 0 after processing first track
    const char *dec; // decoder of the origin
    const char *filetype; // filetype of the origin
    playItem_t *prev; // previous added track

    const uint8_t *p; // buffer ptr
    const char *charset; // detected charset
    int have_track; // whether track info has been found, before encountering the TRACK field
    playItem_t *cuetracks[MAX_CUE_TRACKS]; // all loaded cue tracks after splitting
    int ntracks; // count of cuetracks

    int64_t currsample;
    int last_round;
} cueparser_t;


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

static void
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
    int res = junk_recode (str, (int)l, recbuf, (int)(sizeof (recbuf)-1), charset);
    if (res >= 0) {
        strcpy (str, recbuf);
    }
    else
    {
        strcpy (str, "<UNRECOGNIZED CHARSET>");
    }
}

static void
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

static float
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

static void
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

static void
pl_cue_set_track_field_values(playItem_t *it, cueparser_t *cue) {
    if (cue->cuefields[CUE_FIELD_PERFORMER][0]) {
        pl_add_meta (it, "artist", cue->cuefields[CUE_FIELD_PERFORMER]);
        if (cue->cuefields[CUE_FIELD_ALBUM_PERFORMER][0] && strcmp (cue->cuefields[CUE_FIELD_ALBUM_PERFORMER], cue->cuefields[CUE_FIELD_PERFORMER])) {
            pl_add_meta (it, "album artist", cue->cuefields[CUE_FIELD_ALBUM_PERFORMER]);
        }
    }
    else if (cue->cuefields[CUE_FIELD_ALBUM_PERFORMER][0]) {
        pl_add_meta (it, "artist", cue->cuefields[CUE_FIELD_ALBUM_PERFORMER]);
    }
    if (cue->cuefields[CUE_FIELD_SONGWRITER][0]) {
        pl_add_meta (it, "composer", cue->cuefields[CUE_FIELD_SONGWRITER]);
    }
    else if (cue->cuefields[CUE_FIELD_ALBUM_SONGWRITER][0]) {
        pl_add_meta (it, "composer", cue->cuefields[CUE_FIELD_ALBUM_SONGWRITER]);
    }
    if (cue->cuefields[CUE_FIELD_ALBUM_TITLE][0]) {
        pl_add_meta (it, "album", cue->cuefields[CUE_FIELD_ALBUM_TITLE]);
    }
    if (cue->cuefields[CUE_FIELD_TRACK][0]) {
        pl_add_meta (it, "track", cue->cuefields[CUE_FIELD_TRACK]);
    }
    if (cue->cuefields[CUE_FIELD_TITLE][0]) {
        pl_add_meta (it, "title", cue->cuefields[CUE_FIELD_TITLE]);
    }
    if (cue->cuefields[CUE_FIELD_ISRC]) {
        pl_add_meta (it, "ISRC", cue->cuefields[CUE_FIELD_ISRC]);
    }
    if (cue->cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN][0]) {
        pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, (float)atof (cue->cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN]));
    }
    if (cue->cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK][0]) {
        pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, (float)atof (cue->cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK]));
    }
    if (cue->cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN][0]) {
        pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, (float)atof (cue->cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN]));
    }
    if (cue->cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK][0]) {
        pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, (float)atof (cue->cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK]));
    }

    // add extra tags to tracks
    for (int y = 0; y < cue->extra_tag_index; y += 2) {
        if (cue->extra_tags[y+1]) {
            pl_add_meta (it, cue->extra_tags[y], cue->extra_tags[y+1]);
        }
    }
    for (int y = 0; y < cue->extra_track_tag_index; y += 2) {
        if (cue->extra_track_tags[y+1]) {
            pl_add_meta (it, cue->extra_track_tags[y], cue->extra_track_tags[y+1]);
        }
    }
    cue->extra_track_tag_index = 0;
    memset (cue->extra_track_tags, 0, sizeof (cue->extra_track_tags));

    // generated "total tracks" field
    pl_add_meta(it, "numtracks", cue->cuefields[CUE_FIELD_TOTALTRACKS]);

    pl_items_copy_junk (cue->origin, it, it);
}

static void
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
static int
pl_cue_get_field_value(cueparser_t *cue) {
    if (!strncasecmp (cue->p, "FILE ", 5)) {
        pl_get_qvalue_from_cue (cue->p + 5, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_FILE], cue->charset);
        const char *ext = strrchr (cue->cuefields[CUE_FIELD_FILE], '.');
        if (ext && !strcasecmp(ext, ".cue")) {
            trace_err("Cuesheet %s refers to another cuesheet, which is not allowed\n", cue->fname);
            return -1;
        }
        return CUE_FIELD_FILE;
    }
    else if (!strncasecmp (cue->p, "TRACK ", 6)) {
        // this requires some post-processing
        // and the previous value must not be lost..
        return CUE_FIELD_TRACK;
    }
    else if (!strncasecmp (cue->p, "PERFORMER ", 10)) {
        if (!cue->cuefields[CUE_FIELD_TRACK][0]) {
            pl_get_qvalue_from_cue (cue->p + 10, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_ALBUM_PERFORMER], cue->charset);
            return CUE_FIELD_ALBUM_PERFORMER;
        }
        else {
             pl_get_qvalue_from_cue (cue->p + 10, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_PERFORMER], cue->charset);
             return CUE_FIELD_PERFORMER;
        }
    }
    else if (!strncasecmp (cue->p, "SONGWRITER ", 11)) {
        if (!cue->cuefields[CUE_FIELD_TRACK][0]) {
            pl_get_qvalue_from_cue (cue->p + 11, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_ALBUM_SONGWRITER], cue->charset);
            return CUE_FIELD_ALBUM_SONGWRITER;
        }
        else {
            pl_get_qvalue_from_cue (cue->p + 11, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_SONGWRITER], cue->charset);
            return CUE_FIELD_SONGWRITER;
        }
    }
    else if (!strncasecmp (cue->p, "TITLE ", 6)) {
        if (!cue->have_track && !cue->cuefields[CUE_FIELD_ALBUM_TITLE][0]) {
            pl_get_qvalue_from_cue (cue->p + 6, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_ALBUM_TITLE], cue->charset);
            return CUE_FIELD_ALBUM_TITLE;
        }
        else {
            pl_get_qvalue_from_cue (cue->p + 6, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_TITLE], cue->charset);
            return CUE_FIELD_TITLE;
        }
    }
    else if (!strncasecmp (cue->p, "REM REPLAYGAIN_ALBUM_GAIN ", 26)) {
        pl_get_value_from_cue (cue->p + 26, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_GAIN]);
        return CUE_FIELD_REPLAYGAIN_ALBUM_GAIN;
    }
    else if (!strncasecmp (cue->p, "REM REPLAYGAIN_ALBUM_PEAK ", 26)) {
        pl_get_value_from_cue (cue->p + 26, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_REPLAYGAIN_ALBUM_PEAK]);
        return CUE_FIELD_REPLAYGAIN_ALBUM_PEAK;
    }
    else if (!strncasecmp (cue->p, "REM REPLAYGAIN_TRACK_GAIN ", 26)) {
        pl_get_value_from_cue (cue->p + 26, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_REPLAYGAIN_TRACK_GAIN]);
        return CUE_FIELD_REPLAYGAIN_TRACK_GAIN;
    }
    else if (!strncasecmp (cue->p, "REM REPLAYGAIN_TRACK_PEAK ", 26)) {
        pl_get_value_from_cue (cue->p + 26, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_REPLAYGAIN_TRACK_PEAK]);
        return CUE_FIELD_REPLAYGAIN_TRACK_PEAK;
    }
    else if (!strncasecmp (cue->p, "PREGAP ", 7)) {
        pl_get_value_from_cue (cue->p + 7, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_PREGAP]);
        return CUE_FIELD_PREGAP;
    }
    else if (!strncasecmp (cue->p, "INDEX 00 ", 9)) {
        pl_get_value_from_cue (cue->p + 9, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_INDEX00]);
        return CUE_FIELD_INDEX00;
    }
    else if (!strncasecmp (cue->p, "INDEX 01 ", 9)) {
        pl_get_value_from_cue (cue->p + 9, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_INDEX01]);
        return CUE_FIELD_INDEX01;
    }
    else if (!strncasecmp (cue->p, "INDEX ", 6)) {
        // INDEX 02, INDEX 03, INDEX 04, etc...
        // for practical purposes, store value of INDEX XX in fields[CUE_FIELD_INDEX01]
        pl_get_value_from_cue (cue->p + 9, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_INDEX01]);
        return CUE_FIELD_INDEX_X;
    }
    else if (!strncasecmp (cue->p, "ISRC ", 5)) {
        pl_get_value_from_cue (cue->p + 5, CUE_FIELD_LEN, cue->cuefields[CUE_FIELD_ISRC]);
        return CUE_FIELD_ISRC;
    }
    else {
        // determine and get extra tags
        for (int m = 0; cue_field_map[m]; m += 2) {
            if (!strncasecmp (cue->p, cue_field_map[m], strlen(cue_field_map[m]))) {
                char *key = NULL;
                char *value = NULL;
                if (!cue->have_track) {
                    if (cue->extra_tag_index >= MAX_EXTRA_TAGS_FROM_CUE) {
                        break;
                    }
                    key = cue->extra_tags[cue->extra_tag_index];
                    value = cue->extra_tags[cue->extra_tag_index+1];
                    cue->extra_tag_index = cue->extra_tag_index + 2;
                }
                else {
                    if (cue->extra_track_tag_index >= MAX_EXTRA_TAGS_FROM_CUE) {
                        break;
                    }
                    key = cue->extra_track_tags[cue->extra_track_tag_index];
                    value = cue->extra_track_tags[cue->extra_track_tag_index+1];
                    cue->extra_track_tag_index = cue->extra_track_tag_index + 2;
                }

                strcpy(key,cue_field_map[m+1]);
                pl_get_qvalue_from_cue (cue->p + strlen(cue_field_map[m]), CUE_FIELD_LEN, value, cue->charset);
                return CUE_MAX_FIELDS;
            }
        }

        // ignore unsupported fields
        while (*(cue->p) && *(cue->p) >= 0x20) {
            cue->p++;
        }
        return CUE_MAX_FIELDS;
    }
}

//========================================================================

static int
_file_exists (const char *fname) {
    if (!plug_is_local_file(fname)) {
        return 0;
    }
    DB_FILE *fp = vfs_fopen(fname);
    if (!fp) {
        return 0;
    }
    vfs_fclose (fp);
    return 1;
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
    char resolved_fname[PATH_MAX];

    uint8_t *membuffer = NULL;

    char *res = realpath (fname, resolved_fname);
    if (res) {
        fname = resolved_fname;
    }

    DB_FILE *fp = vfs_fopen (fname);
    if (!fp) {
        goto error;
    }

    int sz = (int)vfs_fgetlength (fp);
    membuffer = malloc (sz + 1);
    if (!membuffer) {
        vfs_fclose (fp);
        trace ("failed to allocate %d bytes to read the file %s\n", sz, fname);
        goto error;
    }
    uint8_t *buffer = membuffer;
    size_t rb = vfs_fread (buffer, sz, 1, fp);
    buffer[sz] = 0;
    vfs_fclose (fp);
    fp = NULL;
    if (rb != 1) {
        goto error;
    }

    after = plt_load_cuesheet_from_buffer (plt, after, fname, NULL, 0, 0, buffer, sz, dirname, namelist, n);
error:
    if (fp) {
        vfs_fclose (fp);
    }
    if (membuffer) {
        free (membuffer);
    }
    return after;
}

static int
_file_present_in_namelist (const char *fullpath, cueparser_t *cue) {
    int i = 0;
    for (i = 0; i < cue->n; i++) {
        if (cue->namelist[i]->d_name[0]) {
            char path[PATH_MAX];
            snprintf (path, sizeof (path), "%s/%s", cue->dirname, cue->namelist[i]->d_name);
            if (!strcmp (path, cue->fullpath)) {
                // file present
                break;
            }
            // poor's man vfs detection -- directory ends with ':'
            snprintf (path, sizeof (path), "%s%s", cue->dirname, cue->namelist[i]->d_name);
            if (!strcmp (path, cue->fullpath)) {
                // file present
                break;
            }
        }
    }
    if (i == cue->n) {
        return 0;
    }
    return 1;
}


static int
cue_addfile_filter (cueparser_t *cue) {
    ddb_file_found_data_t dt;
    dt.filename = cue->fullpath;
    dt.plt = (ddb_playlist_t *)cue->target_playlist;
    dt.is_dir = 0;
    int res = fileadd_filter_test (&dt);
    return res;
}

static int
_load_nextfile (cueparser_t *cue) {
    cue->origin = NULL;
    char *audio_file = cue->cuefields[CUE_FIELD_FILE];
    if (cue->embedded_origin) {
        // embedded cuesheet
        strcpy (cue->fullpath, cue->fname);
        cue->origin = cue->embedded_origin;
        cue->numsamples = cue->embedded_numsamples;
        cue->samplerate = cue->embedded_samplerate;
    }
    // full path in CUE entry
    else if (audio_file[0] == '/' && _file_exists (audio_file)) {
        strcpy (cue->fullpath, audio_file);
    }
    else {
        // try relative path
        snprintf(cue->fullpath, sizeof(cue->fullpath), "%s/%s", cue->cue_file_dir, audio_file);

        if (!_file_exists (cue->fullpath)) {
            cue->fullpath[0] = 0;
            if (cue->namelist) {
                // for image+cue, try guessing the audio filename from cuesheet filename
                int image_found = 0;
                if (cue->ncuefiles == 1) {
                    size_t l = strlen (cue->cue_fname);
                    for (int i = 0; i < cue->n; i++) {
                        const char *ext = strrchr (cue->namelist[i]->d_name, '.');
                        if (!ext || !strcasecmp (ext, ".cue")) {
                            continue;
                        }

                        if (!strncasecmp (cue->cue_fname, cue->namelist[i]->d_name, l-4)) {
                            // have to try loading each of these files
                            snprintf (cue->fullpath, sizeof (cue->fullpath), "%s/%s", cue->dirname, cue->namelist[i]->d_name);
                            int res = cue_addfile_filter(cue);
                            if (res >= 0) {
                                cue->origin = plt_insert_file2 (-1, cue->temp_plt, NULL, cue->fullpath, NULL, NULL, NULL);
                            }
                            if (cue->origin) {
                                image_found = 1;
                                cue->namelist[i]->d_name[0] = 0;
                                break;
                            }
                        }
                    }
                }
                if (!image_found) {
                    // for tracks+cue, try guessing the extension of the FILE value
                    char *ext = strrchr (audio_file, '.');
                    if (ext) {
                        *ext = 0;
                    }
                    for (int i = 0; i < cue->n; i++) {
                        const char *cueext = strrchr (cue->namelist[i]->d_name, '.');
                        if (!cueext || !strcasecmp (cueext, ".cue")) {
                            continue;
                        }

                        if (!strncasecmp (audio_file, cue->namelist[i]->d_name, ext-audio_file)
                            && cue->namelist[i]->d_name[ext-audio_file] == '.') {
                            // have to try loading each of these files
                            snprintf (cue->fullpath, sizeof (cue->fullpath), "%s/%s", cue->dirname, cue->namelist[i]->d_name);

                            // adding to temp playlist will fail file add filters, so need to call this manually here
                            int res = cue_addfile_filter(cue);
                            if (res >= 0) {
                                cue->origin = plt_insert_file2 (-1, cue->temp_plt, NULL, cue->fullpath, NULL, NULL, NULL);
                            }
                            if (cue->origin) {
                                cue->namelist[i]->d_name[0] = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (cue->fullpath[0] && !cue->origin) {
        // if we have namelist - means we're loading cue as part of the folder
        // need to check if the fullpath file is present in the list, to avoid double-loading

        if (cue->namelist && !_file_present_in_namelist (cue->fullpath, cue)) {
            cue->fullpath[0] = 0;
        }
    }

    if (cue->fullpath[0] && !cue->origin) {
        // adding to temp playlist will fail file add filters, so need to call this manually here
        int res = cue_addfile_filter (cue);
        if (res >= 0) {
            cue->origin = plt_insert_file2 (-1, cue->temp_plt, NULL, cue->fullpath, NULL, NULL, NULL);
        }
        if (cue->origin) {
            // mark the file as used
            if (cue->namelist) {
                const char *fn_vfs = NULL;
                const char *fn_nonvfs = NULL;

                const char *fn_slash = strrchr (cue->fullpath, '/');
                const char *fn_col = strrchr (cue->fullpath, ':');
                const char *fn_fslash = strchr (cue->fullpath, '/');

                // this is for files inside of VFS containers, e.g. zip://file.zip:path/to/the/file
                if (fn_col) {
                    fn_vfs = fn_col + 1;
                }
                else if (fn_slash) {
                    fn_vfs = fn_slash + 1;
                }
                else {
                    fn_vfs = cue->fullpath;
                }

                // this is for local FS paths which contain colons, like gvfs mounts
                if (fn_col && (!fn_fslash || fn_fslash > fn_col)) {
                    fn_nonvfs = fn_col + 1;
                }
                else if (fn_slash) {
                    fn_nonvfs = fn_slash + 1;
                }
                else {
                    fn_nonvfs = cue->fullpath;
                }

                for (int i = 0; i < cue->n; i++) {
                    if (!strcmp (fn_vfs, cue->namelist[i]->d_name) || !strcmp (fn_nonvfs, cue->namelist[i]->d_name)) {
                        cue->namelist[i]->d_name[0] = 0;
                        break;
                    }
                }
            }
        }
    }

    if (!cue->origin) {
        if (!cue->namelist) {
            // only display error if adding individual file;
            // this is to prevent bogus errors when auto-scanning for cuesheets in folders.
            trace_err("Invalid FILE entry %s in cuesheet %s, and could not guess any suitable file name.\n", audio_file, cue->fname);
        }
        cue->dec = cue->filetype = NULL;
        return -1;
    }
    else {
        // now we got the image + totalsamples + samplerate,
        // process each track until next file
        if (!cue->prev) {
            cue->dec = pl_find_meta_raw (cue->origin, ":DECODER");
            cue->filetype = pl_find_meta_raw (cue->origin, ":FILETYPE");
            if (!cue->embedded_origin) {
                cue->numsamples = cue->temp_plt->cue_numsamples;
                cue->samplerate = cue->temp_plt->cue_samplerate;
            }
        }
    }
    cue->first_track = 1;

    return 0;
}

static int
plt_process_cue_track (playlist_t *plt, cueparser_t *cue) {
    // fix track number
    char *p = cue->cuefields[CUE_FIELD_TRACK];
    while (*p && isdigit (*p)) {
        p++;
    }
    *p = 0;

    float f_pregap = cue->cuefields[CUE_FIELD_PREGAP][0] ? pl_cue_parse_time (cue->cuefields[CUE_FIELD_PREGAP]) : 0;

    if (cue->prev) {
        // knowing the startsample of the current track,
        // now it's possible to calculate startsample and duration of the previous one.
        if (cue->currsample >= cue->numsamples) {
            return -1;
        }
        pl_item_set_endsample (cue->prev, cue->currsample - 1);
        plt_set_item_duration (plt, cue->prev, (float)(cue->currsample - pl_item_get_startsample (cue->prev)) / cue->samplerate);
    }

    playItem_t *it = pl_item_alloc_init (cue->fullpath, cue->dec);
    pl_set_meta_int (it, ":TRACKNUM", atoi (cue->cuefields[CUE_FIELD_TRACK]));
    pl_item_set_startsample (it, cue->currsample + (int64_t)(f_pregap * cue->samplerate));
    pl_item_set_endsample (it, -1); // will be filled by next read, or by decoder
    pl_replace_meta (it, ":FILETYPE", cue->filetype);

    pl_cue_set_track_field_values(it, cue);

    if (cue->last_round) {
        int res = _set_last_item_region (plt, it, cue->origin, cue->numsamples, cue->samplerate);
        if (res < 0) {
            return res;
        }
    }

    uint32_t f = pl_get_item_flags (cue->origin);
    f |= DDB_TAG_CUESHEET;
    if (!cue->first_track) {
        // First track in a FILE is not a subtrack, but subsequent tracks are.
        // That's necessary to be able to write tags to tracks+cue albums.
        f |= DDB_IS_SUBTRACK;
    }
    cue->first_track = 0;
    if (cue->embedded_origin) {
        f |= DDB_HAS_EMBEDDED_CUESHEET;
    }
    pl_set_item_flags (it, f);

    cue->cuetracks[cue->ntracks++] = it;

    cue->prev = it;
    return 0;
}

static int
_is_audio_track (const char *track) {
    return !strcmp (track + strlen (track) - 6, " AUDIO");
}

playItem_t *
plt_load_cuesheet_from_buffer (playlist_t *plt, playItem_t *after, const char *fname, playItem_t *embedded_origin, int64_t embedded_numsamples, int embedded_samplerate, const uint8_t *buffer, int sz, const char *dirname, struct dirent **namelist, int n) {
    playItem_t *result = NULL;
    cueparser_t cue;
    memset (&cue, 0, sizeof (cue));

    cue.fname = fname;
    cue.embedded_origin = embedded_origin;
    cue.embedded_numsamples = embedded_numsamples;
    cue.embedded_samplerate = embedded_samplerate;

    playItem_t *ins = after;
    char cue_file_dir[strlen(fname)]; // can't be longer than fname
    cue.cue_file_dir = cue_file_dir;

    cue_file_dir[0] = 0;
    const char *slash = strrchr (fname, '/');

    if (slash && slash > fname) {
        strncat (cue_file_dir, fname, slash - fname);
        cue.cue_fname = slash+1;
    }


    cue.dirname = dirname;
    cue.namelist = namelist;
    cue.n = n;

    cue.target_playlist = plt;

    cue.temp_plt = calloc (1, sizeof (playlist_t));
    cue.temp_plt->loading_cue = 1;

    if (sz >= 3 && !memcmp (buffer, "\xef\xbb\xbf", 3)) {
        buffer += 3;
        sz -= 3;
    }

    cue.charset = junk_detect_charset (buffer);
    cue.p = buffer;

    const uint8_t *end = buffer+sz;

    // determine total tracks/files
    pl_cue_get_total_tracks_and_files(cue.p, end, &cue.ncuefiles, &cue.ncuetracks);
    if (!cue.ncuefiles) {
        trace_err("Cuesheet has zero FILE entries (%s)\n", fname);
        goto error;
    }

    if (!cue.ncuetracks) {
        trace_err("Cuesheet has zero TRACK entries (%s)\n", fname);
        goto error;
    }

    if (cue.ncuefiles > 1 && embedded_origin) {
        trace_err("Can't load embedded cuesheet referencing multiple audio files (%s)\n", fname);
        goto error;
    }


    snprintf(cue.cuefields[CUE_FIELD_TOTALTRACKS], CUE_FIELD_LEN, "%d", cue.ncuetracks);

    int filefield = 0;

    while (!cue.last_round) {
        cue.p = skipspaces (cue.p, end);
        int field;
        if (cue.p >= end) {
            field = CUE_FIELD_TRACK;
            cue.last_round = 1;
        }
        else {
            field = pl_cue_get_field_value(&cue);
            if (field < 0) {
                goto error;
            }
        }

        // Next field immediately after FILE, indicates whether current TRACK belongs to previous or next FILE
        if (filefield) {
            filefield = 0;
            // If FILE is immediately followed by TRACK, that next TRACK is from the new FILE
            if (field == CUE_FIELD_TRACK
                && _is_audio_track(cue.cuefields[CUE_FIELD_TRACK])) {
                if (plt_process_cue_track (plt, &cue) < 0) {
                    goto error;
                }
            }
            if (cue.prev) {
                _set_last_item_region (plt, cue.prev, cue.origin, cue.numsamples, cue.samplerate);
                cue.prev = NULL;
            }
            if (_load_nextfile (&cue)) {
                break;
            }
            if (field == CUE_FIELD_TRACK) {
                cue.have_track = 0;
            }
            cue.currsample = pl_item_get_startsample (cue.origin);
        }

        if (field == CUE_FIELD_INDEX01) {
            float sec = cue.cuefields[CUE_FIELD_INDEX01][0] ? pl_cue_parse_time (cue.cuefields[CUE_FIELD_INDEX01]) : 0;
            // that's the startsample of the current track, and endsample-1 of the next one.
            // relative to the beginning of previous file

            if (cue.origin == NULL) {
                trace_err ("CUE: ignoring malformed cuesheet in file %s", fname);
                goto error;
            }

            int64_t val = pl_item_get_startsample (cue.origin) + (int64_t)(sec * cue.samplerate);
            if (val > cue.currsample) {
                cue.currsample = val;
            }
        }
        else if (field == CUE_FIELD_FILE) {
            if (!cue.have_track) {
                if (_load_nextfile (&cue)) {
                    break;
                }
            }
            else {
                filefield = 1;
            }
        }
        else if (field == CUE_FIELD_TRACK) {
            if (cue.origin && cue.have_track) {
                if (_is_audio_track(cue.cuefields[CUE_FIELD_TRACK])) {
                    if (plt_process_cue_track (plt, &cue) < 0) {
                        goto error;
                    }
                }
                else if (cue.prev && cue.last_round) {
                    // set duration for last item
                    _set_last_item_region (plt, cue.prev, cue.origin, cue.numsamples, cue.samplerate);
                }
            }

            if (cue.last_round) {
                break;
            }

            pl_cue_reset_per_track_fields(cue.cuefields);
            pl_get_value_from_cue (cue.p + 6, CUE_FIELD_LEN, cue.cuefields[CUE_FIELD_TRACK]);
            cue.have_track = 1;
        }

        // move pointer to the next line
        while (cue.p < end && *cue.p >= 0x20) {
            cue.p++;
        }
    }
    for (int i = 0; i < cue.ntracks; i++) {
        after = plt_insert_item (plt, after, cue.cuetracks[i]);
        pl_item_unref (cue.cuetracks[i]);
    }
    cue.ntracks = 0;
    playItem_t *first = ins ? ins->next[PL_MAIN] : plt->head[PL_MAIN];
    if (!first) {
        after = NULL;
    }

    result = after;

error:
    for (int i = 0; i < cue.ntracks; i++) {
        pl_item_unref (cue.cuetracks[i]);
    }
    if (cue.temp_plt) {
        plt_free (cue.temp_plt);
    }
    return result;
}

