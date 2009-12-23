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
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include "playlist.h"
#include "codec.h"
#include "streamer.h"
#include "messagepump.h"
#include "playback.h"
#include "plugins.h"
#include "junklib.h"
#include "vfs.h"
#include "conf.h"
#include "utf8.h"

// 1.0->1.1 changelog:
//    added sample-accurate seek positions for sub-tracks
#define PLAYLIST_MAJOR_VER 1
#define PLAYLIST_MINOR_VER 1

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define SKIP_BLANK_CUE_TRACKS 1

#define min(x,y) ((x)<(y)?(x):(y))

playItem_t *playlist_head[PL_MAX_ITERATORS];
playItem_t *playlist_tail[PL_MAX_ITERATORS];
int playlist_current_row[PL_MAX_ITERATORS];

playItem_t *playlist_current_ptr;
static int pl_count = 0;
static float pl_totaltime = 0;

#define PLAYQUEUE_SIZE 100
static playItem_t *playqueue[100];
static int playqueue_count = 0;

void
pl_free (void) {
    while (playlist_head[PL_MAIN]) {
        pl_remove (playlist_head[PL_MAIN]);
    }
}

static const uint8_t *
pl_str_skipspaces (const uint8_t *p, const uint8_t *end) {
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
pl_get_qvalue_from_cue (const uint8_t *p, int sz, char *out) {
    char *str = out;
    if (*p == 0) {
        *out = 0;
        return;
    }
    // seek "
    while (*p && *p != '"') {
        p++;
    }
    if (*p == 0) {
        *out = 0;
        return;
    }
    p++;
    p = pl_cue_skipspaces (p);
    while (*p && *p != '"' && sz > 1) {
        sz--;
        *out++ = *p++;
    }
    *out = 0;
    const char *charset = junk_detect_charset (str);
    if (!charset) {
        return;
    }
    // recode
    int l = strlen (str);
    char in[l+1];
    memcpy (in, str, l+1);
    junk_recode (in, l, str, sz, charset);
}

static void
pl_get_value_from_cue (const char *p, int sz, char *out) {
    while (*p >= ' ' && sz > 1) {
        sz--;
        *out++ = *p++;
    }
    *out = 0;
}

static float
pl_cue_parse_time (const char *p) {
    int len = strnlen(p, 9);
    // should be in 'mm:ss:ff' format, so only len = 8 is acceptable
    if (len != 8) {
        return -1;
    }
    if (p[2] != ':' || p[5] != ':') {
        return -1;
    }
    int mins = atoi (p);
    int sec = atoi (p + 3);
    int frm = atoi (p + 5);
    return mins * 60.f + sec + frm / 75.f;
}

static playItem_t *
pl_process_cue_track (playItem_t *after, const char *fname, playItem_t **prev, char *track, char *index00, char *index01, char *pregap, char *title, char *performer, char *albumtitle, struct DB_decoder_s *decoder, const char *ftype, int samplerate) {
    if (!track[0]) {
        return after;
    }
    if (!index00[0] && !index01[0]) {
        return after;
    }
#if SKIP_BLANK_CUE_TRACKS
    if (!title[0]) {
        return after;
    }
#endif
    // fix track number
    char *p = track;
    while (*p && isdigit (*p)) {
        p++;
    }
    *p = 0;
    // check that indexes have valid timestamps
    float f_index00 = index00[0] ? pl_cue_parse_time (index00) : 0;
    float f_index01 = index01[0] ? pl_cue_parse_time (index01) : 0;
    float f_pregap = pregap[0] ? pl_cue_parse_time (pregap) : 0;
    if (*prev) {
        float prevtime = 0;
        if (pregap[0] && index01[0]) {
            // PREGAP command
            prevtime = f_index01 - f_pregap;
        }
//        else if (index00[0] && index01[0]) {
//            // pregap in index 00
//            prevtime = f_index00;
//        }
        else if (index01[0]) {
            // no pregap
            prevtime = f_index01;
        }
        else {
            return after;
        }
        (*prev)->endsample = (prevtime * samplerate) - 1;
        pl_set_item_duration (*prev, (float)((*prev)->endsample - (*prev)->startsample + 1) / samplerate);
        if ((*prev)->_duration < 0) {
            // might be bad cuesheet file, try to fix
            trace ("cuesheet seems to be corrupted, trying workaround\n");
            //trace ("[bad:] calc endsample=%d, prevtime=%f, samplerate=%d, prev track duration=%f\n", (*prev)->endsample,  prevtime, samplerate, (*prev)->duration);
            prevtime = f_index01;
            (*prev)->endsample = (prevtime * samplerate) - 1;
            pl_set_item_duration (*prev, (float)((*prev)->endsample - (*prev)->startsample + 1) / samplerate);
            if ((*prev)->_duration > 0) {
                trace ("success :-D\n");
            }
            else {
                trace ("fail :-(\n");
            }
        }
        //trace ("calc endsample=%d, prevtime=%f, samplerate=%d, prev track duration=%f\n", (*prev)->endsample,  prevtime, samplerate, (*prev)->duration);
    }
    // non-compliant hack to handle tracks which only store pregap info
    if (!index01[0]) {
        *prev = NULL;
        return after;
    }
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->decoder = decoder;
    it->fname = strdup (fname);
    it->tracknum = atoi (track);
    it->startsample = index01[0] ? f_index01 * samplerate : 0;
    it->endsample = -1; // will be filled by next read, or by decoder
    it->filetype = ftype;
    after = pl_insert_item (after, it);
    pl_add_meta (it, "artist", performer);
    pl_add_meta (it, "album", albumtitle);
    pl_add_meta (it, "track", track);
    pl_add_meta (it, "title", title);
    *prev = it;
    return it;
}

playItem_t *
pl_insert_cue_from_buffer (playItem_t *after, const char *fname, const uint8_t *buffer, int buffersize, struct DB_decoder_s *decoder, const char *ftype, int numsamples, int samplerate) {
    trace ("pl_insert_cue_from_buffer numsamples=%d, samplerate=%d\n", numsamples, samplerate);
    char performer[256] = "";
    char albumtitle[256] = "";
    char track[256] = "";
    char title[256] = "";
    char pregap[256] = "";
    char index00[256] = "";
    char index01[256] = "";
    playItem_t *prev = NULL;
    while (buffersize > 0) {
        const uint8_t *p = buffer;
        // find end of line
        while (p - buffer < buffersize && *p >= 0x20) {
            p++;
        }
        // skip linebreak(s)
        while (p - buffer < buffersize && *p < 0x20) {
            *p++;
        }
        if (p-buffer > 2048) { // huge string, ignore
            buffer = p;
            buffersize -= p-buffer;
            continue;
        }
        char str[p-buffer+1];
        strncpy (str, buffer, p-buffer);
        str[p-buffer] = 0;
        buffersize -= p-buffer;
        buffer = p;
        p = pl_cue_skipspaces (str);
        if (!strncmp (p, "PERFORMER ", 10)) {
            pl_get_qvalue_from_cue (p + 10, sizeof (performer), performer);
//            printf ("got performer: %s\n", performer);
        }
        else if (!strncmp (p, "TITLE ", 6)) {
            if (str[0] > ' ') {
                pl_get_qvalue_from_cue (p + 6, sizeof (albumtitle), albumtitle);
//                printf ("got albumtitle: %s\n", albumtitle);
            }
            else {
                pl_get_qvalue_from_cue (p + 6, sizeof (title), title);
//                printf ("got title: %s\n", title);
            }
        }
        else if (!strncmp (p, "TRACK ", 6)) {
            // add previous track
            after = pl_process_cue_track (after, fname, &prev, track, index00, index01, pregap, title, performer, albumtitle, decoder, ftype, samplerate);
            track[0] = 0;
            title[0] = 0;
            pregap[0] = 0;
            index00[0] = 0;
            index01[0] = 0;
            pl_get_value_from_cue (p + 6, sizeof (track), track);
//            printf ("got track: %s\n", track);
        }
//        else if (!strncmp (p, "PERFORMER ", 10)) {
//            pl_get_qvalue_from_cue (p + 10, performer);
//        }

        else if (!strncmp (p, "PREGAP ", 7)) {
            pl_get_value_from_cue (p + 7, sizeof (pregap), pregap);
        }
        else if (!strncmp (p, "INDEX 00 ", 9)) {
            pl_get_value_from_cue (p + 9, sizeof (index00), index00);
        }
        else if (!strncmp (p, "INDEX 01 ", 9)) {
            pl_get_value_from_cue (p + 9, sizeof (index01), index01);
        }
        else {
//            fprintf (stderr, "got unknown line:\n%s\n", p);
        }
    }
    after = pl_process_cue_track (after, fname, &prev, track, index00, index01, pregap, title, performer, albumtitle, decoder, ftype, samplerate);
    if (after) {
        trace ("last track endsample: %d\n", numsamples-1);
        after->endsample = numsamples-1;
        pl_set_item_duration (after, (float)(after->endsample - after->startsample + 1) / samplerate);
    }
    return after;
}

playItem_t *
pl_insert_cue (playItem_t *after, const char *fname, struct DB_decoder_s *decoder, const char *ftype, int numsamples, int samplerate) {
    trace ("pl_insert_cue numsamples=%d, samplerate=%d\n", numsamples, samplerate);
    int len = strlen (fname);
    char cuename[len+5];
    strcpy (cuename, fname);
    strcpy (cuename+len, ".cue");
    FILE *fp = fopen (cuename, "rb");
    if (!fp) {
        char *ptr = cuename + len-1;
        while (ptr >= cuename && *ptr != '.') {
            ptr--;
        }
        strcpy (ptr+1, "cue");
        fp = fopen (cuename, "rb");
        if (!fp) {
            return NULL;
        }
    }
    fseek (fp, 0, SEEK_END);
    size_t sz = ftell (fp);
    if (sz == 0) {
        fclose (fp);
        return NULL;
    }
    rewind (fp);
    uint8_t buf[sz];
    if (fread (buf, 1, sz, fp) != sz) {
        fclose (fp);
        return NULL;
    }
    fclose (fp);
    return pl_insert_cue_from_buffer (after, fname, buf, sz, decoder, ftype, numsamples, samplerate);
}

playItem_t *
pl_insert_m3u (playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data) {
    // skip all empty lines and comments
    DB_FILE *fp = vfs_fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    int sz = vfs_fgetlength (fp);
    if (sz > 1024*1024) {
        vfs_fclose (fp);
        trace ("file %s is too large to be a playlist\n", fname);
        return NULL;
    }
    if (sz < 30) {
        vfs_fclose (fp);
        trace ("file %s is too small to be a playlist (%d)\n", fname, sz);
        return NULL;
    }
    trace ("loading m3u...\n");
    uint8_t buffer[sz];
    vfs_fread (buffer, 1, sz, fp);
    vfs_fclose (fp);
    const uint8_t *p = buffer;
    const uint8_t *end = buffer+sz;
    while (p < end) {
        p = pl_str_skipspaces (p, end);
        if (p >= end) {
            break;
        }
        if (*p == '#') {
            while (p < end && *p >= 0x20) {
                p++;
            }
            if (p >= end) {
                break;
            }
            continue;
        }
        const uint8_t *e = p;
        while (e < end && *e >= 0x20) {
            e++;
        }
        int n = e-p;
        uint8_t nm[n+1];
        memcpy (nm, p, n);
        nm[n] = 0;
        trace ("adding file %s\n", nm);
        playItem_t *it = pl_insert_file (after, nm, pabort, cb, user_data);
        if (it) {
            after = it;
        }
        if (pabort && *pabort) {
            return after;
        }
        p = e;
        if (p >= end) {
            break;
        }
    }
    return after;
}

// that has to be opened with vfs functions to allow loading from http, as
// referenced from M3U.
playItem_t *
pl_insert_pls (playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data) {
    DB_FILE *fp = vfs_fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    int sz = vfs_fgetlength (fp);
    if (sz > 1024*1024) {
        vfs_fclose (fp);
        trace ("file %s is too large to be a playlist\n", fname);
        return NULL;
    }
    if (sz < 30) {
        vfs_fclose (fp);
        trace ("file %s is too small to be a playlist (%d)\n", fname, sz);
        return NULL;
    }
    vfs_rewind (fp);
    uint8_t buffer[sz];
    vfs_fread (buffer, 1, sz, fp);
    vfs_fclose (fp);
    // 1st line must be "[playlist]"
    const uint8_t *p = buffer;
    const uint8_t *end = buffer+sz;
    if (strncasecmp (p, "[playlist]", 10)) {
        trace ("file %s doesn't begin with [playlist]\n", fname);
        return NULL;
    }
    p += 10;
    p = pl_str_skipspaces (p, end);
    if (p >= end) {
        trace ("file %s finished before numberofentries had been read\n", fname);
        return NULL;
    }
    if (strncasecmp (p, "numberofentries=", 16)) {
        trace ("can't get number of entries from %s\n", fname);
        return NULL;
    }
    p += 15;
    // ignore numentries - no real need for it here
    while (p < end && *p > 0x20) {
        p++;
    }
    p = pl_str_skipspaces (p, end);
    // fetch all tracks
    char url[1024] = "";
    char title[1024] = "";
    char length[20] = "";
    int nfile = 1;
    while (p < end) {
        if (p >= end) {
            break;
        }
        if (end-p < 6) {
            break;
        }
        if (strncasecmp (p, "file", 4)) {
            break;
        }
        p += 4;
        while (p < end && *p != '=') {
            p++;
        }
        p++;
        if (p >= end) {
            break;
        }
        const uint8_t *e = p;
        while (e < end && *e >= 0x20) {
            e++;
        }
        int n = e-p;
        n = min (n, sizeof (url)-1);
        memcpy (url, p, n);
        url[n] = 0;
        trace ("url: %s\n", url);
        p = ++e;
        p = pl_str_skipspaces (p, end);
        if (strncasecmp (p, "title", 5)) {
            break;
        }
        p += 5;
        while (p < end && *p != '=') {
            p++;
        }
        p++;
        if (p >= end) {
            break;
        }
        e = p;
        while (e < end && *e >= 0x20) {
            e++;
        }
        n = e-p;
        n = min (n, sizeof (title)-1);
        memcpy (title, p, n);
        title[n] = 0;
        trace ("title: %s\n", title);
        p = ++e;
        p = pl_str_skipspaces (p, end);
        if (strncasecmp (p, "length", 6)) {
            break;
        }
        p += 6;
        while (p < end && *p != '=') {
            p++;
        }
        p++;
        if (p >= end) {
            break;
        }
        e = p;
        while (e < end && *e >= 0x20) {
            e++;
        }
        n = e-p;
        n = min (n, sizeof (length)-1);
        memcpy (length, p, n);
        length[n] = 0;
        trace ("length: %s\n", length);
        // add track
        playItem_t *it = pl_insert_file (after, url, pabort, cb, user_data);
        if (it) {
            after = it;
            pl_set_item_duration (it, atoi (length));
            pl_add_meta (it, "title", title);
        }
        if (pabort && *pabort) {
            return after;
        }
    }
    return after;
}

playItem_t *
pl_insert_file (playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data) {
    if (!fname) {
        return NULL;
    }

    // detect decoder
    const char *eol = fname + strlen (fname) - 1;
    while (eol > fname && *eol != '.') {
        eol--;
    }
    eol++;

    // detect pls/m3u files
    // they must be handled before checking for http://,
    // so that remote playlist files referenced from other playlist files could
    // be loaded correctly
    if (!memcmp (eol, "m3u", 4)) {
        return pl_insert_m3u (after, fname, pabort, cb, user_data);
    }
    else if (!memcmp (eol, "pls", 4)) {

        return pl_insert_pls (after, fname, pabort, cb, user_data);
    }

    // add all posible streams as special-case:
    // set decoder to NULL, and filetype to "content"
    // streamer is responsible to determine content type on 1st access and
    // update decoder and filetype fields
    if (strncasecmp (fname, "file://", 7)) {
        const char *p = fname;
        int detect_on_access = 1;
        for (p = fname; *p; p++) {
            if (!strncmp (p, "://", 3)) {
                break;
            }
            if (!isalpha (*p)) {
                detect_on_access = 0;
                break;
            }
        }
        if (detect_on_access && *fname != ':') {
            playItem_t *it = pl_item_alloc ();
            it->decoder = NULL;
            it->fname = strdup (fname);
            it->filetype = "content";
            it->_duration = -1;
            pl_add_meta (it, "title", NULL);
            return pl_insert_item (after, it);
        }
    }
    else {
        fname += 7;
    }

    DB_decoder_t **decoders = plug_get_decoder_list ();
    // match by decoder
    for (int i = 0; decoders[i]; i++) {
        if (decoders[i]->exts && decoders[i]->insert) {
            const char **exts = decoders[i]->exts;
            for (int e = 0; exts[e]; e++) {
                if (!strcasecmp (exts[e], eol)) {
                    playItem_t *inserted = (playItem_t *)decoders[i]->insert (DB_PLAYITEM (after), fname);
                    if (inserted != NULL) {
                        if (cb && cb (inserted, user_data) < 0) {
                            *pabort = 1;
                        }
                        return inserted;
                    }
                }
            }
        }
    }
    trace ("no decoder found for %s\n", fname);
    return NULL;
}

playItem_t *
pl_insert_dir (playItem_t *after, const char *dirname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data) {
    if (!memcmp (dirname, "file://", 7)) {
        dirname += 7;
    }
    struct stat buf;
    lstat (dirname, &buf);
    if (S_ISLNK(buf.st_mode)) {
        return NULL;
    }
    struct dirent **namelist = NULL;
    int n;

    n = scandir (dirname, &namelist, NULL, alphasort);
    if (n < 0)
    {
        if (namelist)
            free (namelist);
        return NULL;	// not a dir or no read access
    }
    else
    {
        int i;
        for (i = 0; i < n; i++)
        {
            // no hidden files
            if (namelist[i]->d_name[0] != '.')
            {
                char fullname[PATH_MAX];
                snprintf (fullname, sizeof (fullname), "%s/%s", dirname, namelist[i]->d_name);
                playItem_t *inserted = pl_insert_dir (after, fullname, pabort, cb, user_data);
                if (!inserted) {
                    inserted = pl_insert_file (after, fullname, pabort, cb, user_data);
                }
                if (inserted) {
                    after = inserted;
                }
                if (*pabort) {
                    break;
                }
            }
            free (namelist[i]);
        }
        free (namelist);
    }
    return after;
}

int
pl_add_file (const char *fname, int (*cb)(playItem_t *it, void *data), void *user_data) {
    int abort = 0;
    if (pl_insert_file (playlist_tail[PL_MAIN], fname, &abort, cb, user_data)) {
        return 0;
    }
    return -1;
}

int
pl_add_dir (const char *dirname, int (*cb)(playItem_t *it, void *data), void *user_data) {
    int abort = 0;
    if (pl_insert_dir (playlist_tail[PL_MAIN], dirname, &abort, cb, user_data)) {
        return 0;
    }
    return -1;
}

int
pl_remove (playItem_t *it) {
    if (!it)
        return -1;
    streamer_song_removed_notify (it);
    pl_count--;
    if (playlist_current_ptr == it) {
        playlist_current_ptr = NULL;
    }
    pl_playqueue_remove (it);

    // remove from linear list
    if (it->prev[PL_MAIN]) {
        it->prev[PL_MAIN]->next[PL_MAIN] = it->next[PL_MAIN];
    }
    else {
        playlist_head[PL_MAIN] = it->next[PL_MAIN];
    }
    if (it->next[PL_MAIN]) {
        it->next[PL_MAIN]->prev[PL_MAIN] = it->prev[PL_MAIN];
    }
    else {
        playlist_tail[PL_MAIN] = it->prev[PL_MAIN];
    }
    // totaltime
    if (it->_duration > 0) {
        pl_totaltime -= it->_duration;
        if (pl_totaltime < 0) {
            pl_totaltime = 0;
        }
    }
    pl_item_free (it);
    free (it);
    return 0;
}

int
pl_getcount (void) {
    return pl_count;
}

int
pl_getselcount (void) {
    // FIXME: slow!
    int cnt = 0;
    for (playItem_t *it = playlist_head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        if (it->selected) {
            cnt++;
        }
    }
    return cnt;
}

playItem_t *
pl_get_for_idx_and_iter (int idx, int iter) {
    playItem_t *it = playlist_head[iter];
    while (idx--) {
        if (!it)
            return NULL;
        it = it->next[iter];
    }
    return it;
}

playItem_t *
pl_get_for_idx (int idx) {
    return pl_get_for_idx_and_iter (idx, PL_MAIN);
}

int
pl_get_idx_of (playItem_t *it) {
    playItem_t *c = playlist_head[PL_MAIN];
    int idx = 0;
    while (c && c != it) {
        c = c->next[PL_MAIN];
        idx++;
    }
    if (!c) {
        return -1;
    }
    return idx;
}

int
pl_append_item (playItem_t *it) {
    if (!playlist_tail[PL_MAIN]) {
        playlist_tail[PL_MAIN] = playlist_head[PL_MAIN] = it;
    }
    else {
        playlist_tail[PL_MAIN]->next[PL_MAIN] = it;
        it->prev[PL_MAIN] = playlist_tail[PL_MAIN];
        playlist_tail[PL_MAIN] = it;
    }
    pl_count++;
}

playItem_t *
pl_insert_item (playItem_t *after, playItem_t *it) {
    if (!after) {
        it->next[PL_MAIN] = playlist_head[PL_MAIN];
        it->prev[PL_MAIN] = NULL;
        if (playlist_head[PL_MAIN]) {
            playlist_head[PL_MAIN]->prev[PL_MAIN] = it;
        }
        else {
            playlist_tail[PL_MAIN] = it;
        }
        playlist_head[PL_MAIN] = it;
    }
    else {
        it->prev[PL_MAIN] = after;
        it->next[PL_MAIN] = after->next[PL_MAIN];
        if (after->next[PL_MAIN]) {
            after->next[PL_MAIN]->prev[PL_MAIN] = it;
        }
        after->next[PL_MAIN] = it;
        if (after == playlist_tail[PL_MAIN]) {
            playlist_tail[PL_MAIN] = it;
        }
    }
    it->in_playlist = 1;
    pl_count++;

    // shuffle
    it->shufflerating = rand ();
    it->played = 0;

    // totaltime
    if (it->_duration > 0) {
        pl_totaltime += it->_duration;
    }

    return it;
}

void
pl_item_copy (playItem_t *out, playItem_t *it) {
    out->fname = strdup (it->fname);
    out->decoder = it->decoder;
    out->tracknum = it->tracknum;
    out->startsample = it->startsample;
    out->endsample = it->endsample;
    pl_set_item_duration (out, it->_duration);
    out->shufflerating = it->shufflerating;
    out->filetype = it->filetype;
    out->replaygain_album_gain = it->replaygain_album_gain;
    out->replaygain_album_peak = it->replaygain_album_peak;
    out->replaygain_track_gain = it->replaygain_track_gain;
    out->replaygain_track_peak = it->replaygain_track_peak;
    out->started_timestamp = it->started_timestamp;
    out->next[PL_MAIN] = it->next[PL_MAIN];
    out->prev[PL_MAIN] = it->prev[PL_MAIN];
    out->next[PL_SEARCH] = it->next[PL_SEARCH];
    out->prev[PL_SEARCH] = it->prev[PL_SEARCH];
    // copy metainfo
    metaInfo_t *prev = NULL;
    metaInfo_t *meta = it->meta;
    while (meta) {
        metaInfo_t *m = malloc (sizeof (metaInfo_t));
        m->key = meta->key;
        m->value = strdup (meta->value);
        m->next = NULL;
        if (prev) {
            prev->next = m;
        }
        else {
            out->meta = m;
        }
        prev = m;
        meta = meta->next;
    }
}

playItem_t *
pl_item_alloc (void) {
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->replaygain_album_peak = 1;
    it->replaygain_track_peak = 1;
    return it;
}

void
pl_item_free (playItem_t *it) {
    if (it) {
        if (it->fname) {
            free (it->fname);
        }
        while (it->meta) {
            metaInfo_t *m = it->meta;
            it->meta = m->next;
            free (m->value);
            free (m);
        }
        memset (it, 0, sizeof (playItem_t));
    }
}

int
pl_prevsong (void) {
    pl_playqueue_clear ();
    if (!playlist_head[PL_MAIN]) {
        streamer_set_nextsong (-2, 1);
        return 0;
    }
    int pl_order = conf_get_int ("playback.order", 0);
    int pl_loop_mode = conf_get_int ("playback.loop", 0);
    if (pl_order == PLAYBACK_ORDER_SHUFFLE) { // shuffle
        if (!playlist_current_ptr) {
            return pl_nextsong (1);
        }
        else {
            playlist_current_ptr->played = 0;
            // find already played song with maximum shuffle rating below prev song
            int rating = playlist_current_ptr->shufflerating;
            playItem_t *pmax = NULL; // played maximum
            playItem_t *amax = NULL; // absolute maximum
            playItem_t *i = NULL;
            for (playItem_t *i = playlist_head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                if (i != playlist_current_ptr && i->played && (!amax || i->shufflerating > amax->shufflerating)) {
                    amax = i;
                }
                if (i == playlist_current_ptr || i->shufflerating > rating || !i->played) {
                    continue;
                }
                if (!pmax || i->shufflerating > pmax->shufflerating) {
                    pmax = i;
                }
            }
            playItem_t *it = pmax;
            if (!it) {
                // that means 1st in playlist, take amax
                if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                    if (!amax) {
                        pl_reshuffle (NULL, &amax);
                    }
                    it = amax;
                }
            }

            if (!it) {
                return -1;
            }
            int r = pl_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            return 0;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (playlist_current_ptr) {
            it = playlist_current_ptr->prev[PL_MAIN];
        }
        if (!it) {
            if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                it = playlist_tail[PL_MAIN];
            }
        }
        if (!it) {
            return -1;
        }
        int r = pl_get_idx_of (it);
        streamer_set_nextsong (r, 1);
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        pl_randomsong ();
    }
    return -1;
}

int
pl_nextsong (int reason) {
    if (playqueue_count > 0) {
        playItem_t *it = playqueue[0];
        pl_playqueue_pop ();
        int r = pl_get_idx_of (it);
        streamer_set_nextsong (r, 1);
        return 0;
    }

    playItem_t *curr = streamer_get_streaming_track ();
    if (!playlist_head[PL_MAIN]) {
        streamer_set_nextsong (-2, 1);
        return 0;
    }
    int pl_order = conf_get_int ("playback.order", 0);
    int pl_loop_mode = conf_get_int ("playback.loop", 0);
    if (pl_order == PLAYBACK_ORDER_SHUFFLE) { // shuffle
        if (!curr) {
            // find minimal notplayed
            playItem_t *pmin = NULL; // notplayed minimum
            playItem_t *i = NULL;
            for (playItem_t *i = playlist_head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                if (i->played) {
                    continue;
                }
                if (!pmin || i->shufflerating < pmin->shufflerating) {
                    pmin = i;
                }
            }
            playItem_t *it = pmin;
            if (!it) {
                // all songs played, reshuffle and try again
                if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) { // loop
                    pl_reshuffle (&it, NULL);
                }
            }
            if (!it) {
                return -1;
            }
            int r = pl_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            return 0;
        }
        else {
            trace ("pl_next_song: reason=%d, loop=%d\n", reason, pl_loop_mode);
            if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // song finished, loop mode is "loop 1 track"
                int r = pl_get_idx_of (curr);
                streamer_set_nextsong (r, 1);
                return 0;
            }
            // find minimal notplayed above current
            int rating = curr->shufflerating;
            playItem_t *pmin = NULL; // notplayed minimum
            playItem_t *i = NULL;
            for (playItem_t *i = playlist_head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                if (i->played || i->shufflerating < rating) {
                    continue;
                }
                if (!pmin || i->shufflerating < pmin->shufflerating) {
                    pmin = i;
                }
            }
            playItem_t *it = pmin;
            if (!it) {
                trace ("all songs played! reshuffle\n");
                // all songs played, reshuffle and try again
                if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) { // loop
                    pl_reshuffle (&it, NULL);
                }
            }
            if (!it) {
                return -1;
            }
            int r = pl_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            return 0;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (curr) {
            if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // loop same track
                int r = pl_get_idx_of (curr);
                streamer_set_nextsong (r, 1);
                return 0;
            }
            it = curr->next[PL_MAIN];
        }
        if (!it) {
            trace ("pl_nextsong: was last track\n");
            if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                it = playlist_head[PL_MAIN];
            }
            else {
                streamer_set_nextsong (-2, 1);
                return 0;
            }
        }
        if (!it) {
            return -1;
        }
        int r = pl_get_idx_of (it);
        streamer_set_nextsong (r, 1);
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE && curr) {
            int r = pl_get_idx_of (curr);
            streamer_set_nextsong (r, 1);
            return 0;
        }
        return pl_randomsong ();
    }
    return -1;
}

int
pl_randomsong (void) {
    if (!pl_getcount ()) {
        return -1;
    }
    int r = (float)rand ()/RAND_MAX * pl_getcount ();
    streamer_set_nextsong (r, 1);
    return 0;
}

void
pl_add_meta (playItem_t *it, const char *key, const char *value) {
    // check if it's already set
    metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            return;
        }
        m = m->next;
    }
    // add
    char str[256];
    if (!value || !*value) {
        if (!strcasecmp (key, "title")) {
            int len = 256;
            // cut filename without path and extension
            const char *pext = it->fname + strlen (it->fname) - 1;
            while (pext >= it->fname && *pext != '.') {
                pext--;
            }
            const char *pname = pext;
            while (pname >= it->fname && *pname != '/') {
                pname--;
            }
            if (*pname == '/') {
                pname++;
            }
            strncpy (str, pname, pext-pname);
            str[pext-pname] = 0;
            value = str;
        }
        else {
            return;
        }
    }
    m = malloc (sizeof (metaInfo_t));
    m->key = key;
    m->value = strdup (value);
    m->next = it->meta;
    it->meta = m;
}

void
pl_format_item_display_name (playItem_t *it, char *str, int len) {
    const char *artist = pl_find_meta (it, "artist");
    const char *title = pl_find_meta (it, "title");
    if (!artist) {
        artist = "Unknown artist";
    }
    if (!title) {
        title = "Unknown title";
    }
    snprintf (str, len, "%s - %s", artist, title);
}

const char *
pl_find_meta (playItem_t *it, const char *key) {
    metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            return m->value;
        }
        m = m->next;
    }
    return NULL;
}

int
pl_delete_selected (void) {
    int i = 0;
    int ret = -1;
    playItem_t *next = NULL;
    for (playItem_t *it = playlist_head[PL_MAIN]; it; it = next, i++) {
        next = it->next[PL_MAIN];
        if (it->selected) {
            if (ret == -1) {
                ret = i;
            }
            pl_remove (it);
        }
    }
    return ret;
}

void
pl_crop_selected (void) {
    playItem_t *next = NULL;
    for (playItem_t *it = playlist_head[PL_MAIN]; it; it = next) {
        next = it->next[PL_MAIN];
        if (!it->selected) {
            pl_remove (it);
        }
    }
}

int
pl_save (const char *fname) {
    const char magic[] = "DBPL";
    uint8_t majorver = PLAYLIST_MAJOR_VER;
    uint8_t minorver = PLAYLIST_MINOR_VER;
    FILE *fp = fopen (fname, "w+b");
    if (!fp) {
        return -1;
    }
    if (fwrite (magic, 1, 4, fp) != 4) {
        goto save_fail;
    }
    if (fwrite (&majorver, 1, 1, fp) != 1) {
        goto save_fail;
    }
    if (fwrite (&minorver, 1, 1, fp) != 1) {
        goto save_fail;
    }
    uint32_t cnt = pl_count;
    if (fwrite (&cnt, 1, 4, fp) != 4) {
        goto save_fail;
    }
    for (playItem_t *it = playlist_head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        uint16_t l;
        uint8_t ll;
        l = strlen (it->fname);
        if (fwrite (&l, 1, 2, fp) != 2) {
            goto save_fail;
        }
        if (fwrite (it->fname, 1, l, fp) != l) {
            goto save_fail;
        }
        if (it->decoder) {
            ll = strlen (it->decoder->id);
            if (fwrite (&ll, 1, 1, fp) != 1) {
                goto save_fail;
            }
            if (fwrite (it->decoder->id, 1, ll, fp) != ll) {
                goto save_fail;
            }
        }
        else {
            ll = 0;
            if (fwrite (&ll, 1, 1, fp) != 1) {
                goto save_fail;
            }
        }
        l = it->tracknum;
        if (fwrite (&l, 1, 2, fp) != 2) {
            goto save_fail;
        }
        if (fwrite (&it->startsample, 1, 4, fp) != 4) {
            goto save_fail;
        }
        if (fwrite (&it->endsample, 1, 4, fp) != 4) {
            goto save_fail;
        }
        if (fwrite (&it->_duration, 1, 4, fp) != 4) {
            goto save_fail;
        }
        uint8_t ft = it->filetype ? strlen (it->filetype) : 0;
        if (fwrite (&ft, 1, 1, fp) != 1) {
            goto save_fail;
        }
        if (ft) {
            if (fwrite (it->filetype, 1, ft, fp) != ft) {
                goto save_fail;
            }
        }
        if (fwrite (&it->replaygain_album_gain, 1, 4, fp) != 4) {
            goto save_fail;
        }
        if (fwrite (&it->replaygain_album_peak, 1, 4, fp) != 4) {
            goto save_fail;
        }
        if (fwrite (&it->replaygain_track_gain, 1, 4, fp) != 4) {
            goto save_fail;
        }
        if (fwrite (&it->replaygain_track_peak, 1, 4, fp) != 4) {
            goto save_fail;
        }

        int16_t nm = 0;
        metaInfo_t *m;
        for (m = it->meta; m; m = m->next) {
            nm++;
        }
        if (fwrite (&nm, 1, 2, fp) != 2) {
            goto save_fail;
        }
        for (m = it->meta; m; m = m->next) {
            l = strlen (m->key);
            if (fwrite (&l, 1, 2, fp) != 2) {
                goto save_fail;
            }
            if (l) {
                if (fwrite (m->key, 1, l, fp) != l) {
                    goto save_fail;
                }
            }
            l = strlen (m->value);
            if (fwrite (&l, 1, 2, fp) != 2) {
                goto save_fail;
            }
            if (l) {
                if (fwrite (m->value, 1, l, fp) != l) {
                    goto save_fail;
                }
            }
        }
    }
    fclose (fp);
    return 0;
save_fail:
    fclose (fp);
    unlink (fname);
    return -1;
}

int
pl_load (const char *fname) {
    pl_free ();
    DB_decoder_t **decoders = plug_get_decoder_list ();
    uint8_t majorver;
    uint8_t minorver;
    playItem_t *it = NULL;
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        return -1;
    }
    char magic[4];
    if (fread (magic, 1, 4, fp) != 4) {
        goto load_fail;
    }
    if (strncmp (magic, "DBPL", 4)) {
        goto load_fail;
    }
    if (fread (&majorver, 1, 1, fp) != 1) {
        goto load_fail;
    }
    if (majorver != PLAYLIST_MAJOR_VER) {
        goto load_fail;
    }
    if (fread (&minorver, 1, 1, fp) != 1) {
        goto load_fail;
    }
    if (minorver != PLAYLIST_MINOR_VER) {
        goto load_fail;
    }
    uint32_t cnt;
    if (fread (&cnt, 1, 4, fp) != 4) {
        goto load_fail;
    }
    for (uint32_t i = 0; i < cnt; i++) {
        it = malloc (sizeof (playItem_t));
        if (!it) {
            goto load_fail;
        }
        memset (it, 0, sizeof (playItem_t));
        uint16_t l;
        // fname
        if (fread (&l, 1, 2, fp) != 2) {
            goto load_fail;
        }
        it->fname = malloc (l+1);
        if (fread (it->fname, 1, l, fp) != l) {
            goto load_fail;
        }
        it->fname[l] = 0;
        // decoder
        uint8_t ll;
        if (fread (&ll, 1, 1, fp) != 1) {
            goto load_fail;
        }
        if (ll >= 20) {
            goto load_fail;
        }
        if (ll) {
            char decoder[20];
            if (fread (decoder, 1, ll, fp) != ll) {
                goto load_fail;
            }
            decoder[ll] = 0;
            for (int c = 0; decoders[c]; c++) {
                if (!strcmp (decoder, decoders[c]->id)) {
                    it->decoder = decoders[c];
                }
            }
//            if (!it->decoder) {
//                goto load_fail;
//            }
        }
        else {
            it->decoder = NULL;
        }
        // tracknum
        if (fread (&l, 1, 2, fp) != 2) {
            goto load_fail;
        }
        it->tracknum = l;
        // startsample
        if (fread (&it->startsample, 1, 4, fp) != 4) {
            goto load_fail;
        }
        // endsample
        if (fread (&it->endsample, 1, 4, fp) != 4) {
            goto load_fail;
        }
        // duration
        float d;
        if (fread (&d, 1, 4, fp) != 4) {
            goto load_fail;
        }
        it->_duration = d;
        // get const filetype string from decoder
        uint8_t ft;
        if (fread (&ft, 1, 1, fp) != 1) {
            goto load_fail;
        }
        if (ft) {
            char ftype[ft+1];
            if (fread (ftype, 1, ft, fp) != ft) {
                goto load_fail;
            }
            ftype[ft] = 0;
            if (!strcmp (ftype, "content")) {
                it->filetype = "content";
            }
            else {
                if (it->decoder && it->decoder->filetypes) {
                    for (int i = 0; it->decoder->filetypes[i]; i++) {
                        if (!strcasecmp (it->decoder->filetypes[i], ftype)) {
                            it->filetype = it->decoder->filetypes[i];
                            break;
                        }
                    }
                }
            }
        }
        if (fread (&it->replaygain_album_gain, 1, 4, fp) != 4) {
            goto load_fail;
        }
        if (fread (&it->replaygain_album_peak, 1, 4, fp) != 4) {
            goto load_fail;
        }
        if (it->replaygain_album_peak == 0) {
            it->replaygain_album_peak = 1;
        }
        if (fread (&it->replaygain_track_gain, 1, 4, fp) != 4) {
            goto load_fail;
        }
        if (fread (&it->replaygain_track_peak, 1, 4, fp) != 4) {
            goto load_fail;
        }
        if (it->replaygain_track_peak == 0) {
            it->replaygain_track_peak = 1;
        }
        // printf ("loading file %s\n", it->fname);
        int16_t nm = 0;
        if (fread (&nm, 1, 2, fp) != 2) {
            goto load_fail;
        }
        for (int i = 0; i < nm; i++) {
            char key[1024];
            char value[1024];
            const char *valid_keys[] = {
                "title",
                "artist",
                "album",
                "vendor",
                "year",
                "genre",
                "comment",
                "track",
                "band",
                "cuesheet",
                NULL
            };

            if (fread (&l, 1, 2, fp) != 2) {
                goto load_fail;
            }
            if (!l || l >= 1024) {
                goto load_fail;
            }
            if (fread (key, 1, l, fp) != l) {
                goto load_fail;
            }
            key[l] = 0;
            if (fread (&l, 1, 2, fp) != 2) {
                goto load_fail;
            }
            if (!l || l >= 1024) {
                goto load_fail;
            }
            if (fread (value, 1, l, fp) != l) {
                goto load_fail;
            }
            value[l] = 0;
            //printf ("%s=%s\n", key, value);
            for (int n = 0; valid_keys[n]; n++) {
                if (!strcmp (valid_keys[n], key)) {
                    pl_add_meta (it, valid_keys[n], value);
                    break;
                }
            }
        }
        pl_insert_item (playlist_tail[PL_MAIN], it);
    }
    fclose (fp);
    return 0;
load_fail:
    trace ("playlist load fail!\n");
    fclose (fp);
    if (it) {
        pl_item_free (it);
    }
    pl_free ();
    return -1;
}

void
pl_select_all (void) {
    for (playItem_t *it = playlist_head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        it->selected = 1;
    }
}


void
pl_reshuffle (playItem_t **ppmin, playItem_t **ppmax) {
    playItem_t *pmin = NULL;
    playItem_t *pmax = NULL;
    for (playItem_t *it = playlist_head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        it->shufflerating = rand ();
        if (!pmin || it->shufflerating < pmin->shufflerating) {
            pmin = it;
        }
        if (!pmax || it->shufflerating > pmax->shufflerating) {
            pmax = it;
        }
        it->played = 0;
    }
    if (ppmin) {
        *ppmin = pmin;
    }
    if (ppmax) {
        *ppmax = pmax;
    }
}

void
pl_delete_all_meta (playItem_t *it) {
    while (it->meta) {
        metaInfo_t *m = it->meta;
        it->meta = m->next;
        free (m->value);
        free (m);
    }
    it->meta = NULL;
}

void
pl_set_item_duration (playItem_t *it, float duration) {
//    if (pl_get_idx_of (it) != -1) {
    if (it->in_playlist) {
        if (it->_duration > 0) {
            pl_totaltime -= it->_duration;
        }
        if (duration > 0) {
            pl_totaltime += duration;
        }
        if (pl_totaltime < 0) {
            pl_totaltime = 0;
        }
    }
    it->_duration = duration;
}

float
pl_get_item_duration (playItem_t *it) {
    return it->_duration;
}

int
pl_format_item_queue (playItem_t *it, char *s, int size) {
    *s = 0;
    if (!playqueue_count) {
        return 0;
    }
    int init = 1;
    int initsize = size;
    int len;
    for (int i = 0; i < playqueue_count; i++) {
        if (size <= 0) {
            break;
        }
        if (playqueue[i] == it) {
            len;
            if (init) {
                init = 0;
                s[0] = '(';
                s++;
                size--;
                len = snprintf (s, size, "%d", i+1);
            }
            else {
                len = snprintf (s, size, ",%d", i+1);
            }
            s += len;
            size -= len;
        }
    }
    if (size != initsize && size > 0) {
        len = snprintf (s, size, ")");
        s += len;
        size -= len;
    }
    return initsize-size;
}

static const char *
pl_get_meta_cached (playItem_t *it, const char *meta, const char *ret, const char *def) {
    if (!ret) {
        ret = pl_find_meta (it, meta);
        if (!ret) {
            ret = def;
        }
    }
    return ret;
}

static const char *
pl_format_duration (playItem_t *it, const char *ret, char dur[50]) {
    if (ret) {
        return ret;
    }
    if (it->_duration >= 0) {
        int hourdur = it->_duration / (60 * 60);
        int mindur = (it->_duration - hourdur * 60 * 60) / 60;
        int secdur = it->_duration - hourdur*60*60 - mindur * 60;

        if (hourdur) {
            snprintf (dur, sizeof (dur), "%d:%02d:%02d", hourdur, mindur, secdur);
        }
        else {
            snprintf (dur, sizeof (dur), "%d:%02d", mindur, secdur);
        }
    }
    else {
        strcpy (dur, "-:--");
    }
    return dur;
}

int
pl_format_title (playItem_t *it, char *s, int size, int id, const char *fmt) {
    char dur[50];
    const char *artist = NULL;
    const char *album = NULL;
    const char *track = NULL;
    const char *title = NULL;
    const char *duration = NULL;

    if (id != -1) {
        const char *text = NULL;
        switch (id) {
        case DB_COLUMN_PLAYING:
            return pl_format_item_queue (it, s, size);
        case DB_COLUMN_ARTIST_ALBUM:
            {
                char artistalbum[1024];
                artist = pl_get_meta_cached (it, "artist", artist, "?");
                album = pl_get_meta_cached (it, "album", album, "?");
                snprintf (artistalbum, sizeof (artistalbum), "%s - %s", artist, album);
                text = artistalbum;
            }
            break;
        case DB_COLUMN_ARTIST:
            text = (artist = pl_get_meta_cached (it, "artist", artist, "?"));
            break;
        case DB_COLUMN_ALBUM:
            text = (album = pl_get_meta_cached (it, "album", artist, "?"));
            break;
        case DB_COLUMN_TITLE:
            text = (title = pl_get_meta_cached (it, "title", artist, "?"));
            break;
        case DB_COLUMN_DURATION:
            text = (duration = pl_format_duration (it, duration, dur));
            break;
        case DB_COLUMN_TRACK:
            text = (track = pl_get_meta_cached (it, "track", track, ""));
            break;
        }
        if (text) {
            strncpy (s, text, size);
            return strlen (s);
        }
        else {
            s[0] = 0;
        }
        return 0;
    }
    int n = size-1;
    while (*fmt && n) {
        if (*fmt != '%') {
            *s++ = *fmt;
            n--;
        }
        else {
            fmt++;
            const char *meta = NULL;
            if (*fmt == 0) {
                break;
            }
            else if (*fmt == 'a') {
                meta = (artist = pl_get_meta_cached (it, "artist", artist, "?"));
            }
            else if (*fmt == 't') {
                meta = (title = pl_get_meta_cached (it, "title", title, "?"));
            }
            else if (*fmt == 'b') {
                meta = (album = pl_get_meta_cached (it, "album", album, "?"));
            }
            else if (*fmt == 'n') {
                meta = (track = pl_get_meta_cached (it, "track", track, ""));
            }
            else if (*fmt == 'l') {
                const char *value = (duration = pl_format_duration (it, duration, dur));
                while (n > 0 && *value) {
                    *s++ = *value++;
                    n--;
                }
            }
            else {
                *s++ = *fmt;
                n--;
            }

            if (meta) {
                const char *value = meta;
                while (n > 0 && *value) {
                    *s++ = *value++;
                    n--;
                }
            }
        }
        fmt++;
    }
    *s = 0;

    return size - n - 1;
}

void
pl_sort (int iter, int id, const char *format, int ascending) {
    int sorted = 0;
    do {
        sorted = 1;
        playItem_t *it;
        playItem_t *next = NULL;
        for (it = playlist_head[iter]; it; it = it->next[iter]) {
            playItem_t *next = it->next[iter];
            if (!next) {
                break;
            }
            char title1[1024];
            char title2[1024];
            pl_format_title (it, title1, sizeof (title1), id, format);
            pl_format_title (next, title2, sizeof (title2), id, format);
//            const char *meta1 = pl_find_meta (it, meta);
//            const char *meta2 = pl_find_meta (next, meta);
            int cmp = ascending ? strcmp (title1, title2) < 0 : strcmp (title1, title2) > 0;
            if (cmp) {
//                printf ("%p %p swapping %s and %s\n", it, next, meta1, meta2);
                sorted = 0;
                // swap them
                if (it->prev[iter]) {
                    it->prev[iter]->next[iter] = next;
//                    printf ("it->prev->next = it->next\n");
                }
                else {
                    playlist_head[iter] = next;
                    next->prev[iter] = NULL;
//                    printf ("head = it->next\n");
                }
                if (next->next[iter]) {
                    next->next[iter]->prev[iter] = it;
//                    printf ("it->next->next->prev = it\n");
                }
                else {
                    playlist_tail[iter] = it;
                    it->next[iter] = NULL;
//                    printf ("tail = it\n");
                }
                playItem_t *it_prev = it->prev[iter];
                it->next[iter] = next->next[iter];
                it->prev[iter] = next;
                next->next[iter] = it;
                next->prev[iter] = it_prev;
                it = next;
            }
#if 0
            else {
                printf ("%p %p NOT swapping %s and %s\n", it, next, meta1, meta2);
            }
#endif
#if 0
            // print list
            int k = 0;
            playItem_t *p = NULL;
            for (playItem_t *i = playlist_head[iter]; i; p = i, i = i->next[iter], k++) {
                printf ("%p ", i);
                if (i->prev[iter] != p) {
                    printf ("\n\033[0;33mbroken link, i=%p, i->prev=%p, prev=%p\033[37;0m\n", i, i->prev[iter], p);
                }
                if (k > 20) {
                    printf ("\033[0;31mlist corrupted\033[37;0m\n");
                    return;
                }
            }
            printf ("\n");
#endif
        }
    } while (!sorted);
}

void
pl_reset_cursor (void) {
    int i;
    for (i = 0; i < PL_MAX_ITERATORS; i++) {
        playlist_current_row[i] = -1;
    }
}

float
pl_get_totaltime (void) {
    return pl_totaltime;
}

playItem_t *
pl_getcurrent (void) {
    return playlist_current_ptr;
}

void
pl_set_selected (playItem_t *it, int sel) {
    it->selected = sel;
}

int
pl_is_selected (playItem_t *it) {
    return it->selected;
}

playItem_t *
pl_get_first (int iter) {
    return playlist_head[iter];
}

playItem_t *
pl_get_last (int iter) {
    return playlist_tail[iter];
}

playItem_t *
pl_get_next (playItem_t *it, int iter) {
    return it ? it->next[iter] : NULL;
}

playItem_t *
pl_get_prev (playItem_t *it, int iter) {
    return it ? it->prev[iter] : NULL;
}

int
pl_get_cursor (int iter) {
    return playlist_current_row[iter];
}

void
pl_set_cursor (int iter, int cursor) {
    playlist_current_row[iter] = cursor;
}

// this function must move items in playlist
// list of items is indexes[count]
// drop_before is insertion point
void
pl_move_items (int iter, playItem_t *drop_before, uint32_t *indexes, int count) {
    // unlink items from playlist, and link together
    playItem_t *head = NULL;
    playItem_t *tail = NULL;
    int processed = 0;
    int idx = 0;
    playItem_t *next = NULL;
    for (playItem_t *it = playlist_head[iter]; it && processed < count; it = next, idx++) {
        next = it->next[iter];
        if (idx == indexes[processed]) {
            if (it->prev[iter]) {
                it->prev[iter]->next[iter] = it->next[iter];
            }
            else {
                playlist_head[iter] = it->next[iter];
            }
            if (it->next[iter]) {
                it->next[iter]->prev[iter] = it->prev[iter];
            }
            else {
                playlist_tail[iter] = it->prev[iter];
            }
            if (tail) {
                tail->next[iter] = it;
                it->prev[iter] = tail;
                tail = it;
            }
            else {
                head = tail = it;
                it->prev[iter] = it->next[iter] = NULL;
            }
            processed++;
        }
    }
    // find insertion point
    playItem_t *drop_after = NULL;
    if (drop_before) {
        drop_after = drop_before->prev[iter];
    }
    else {
        drop_after = playlist_tail[iter];
    }
    // insert in between
    head->prev[iter] = drop_after;
    if (drop_after) {
        drop_after->next[iter] = head;
    }
    else {
        playlist_head[iter] = head;
    }
    tail->next[iter] = drop_before;
    if (drop_before) {
        drop_before->prev[iter] = tail;
    }
    else {
        playlist_tail[iter] = tail;
    }
}

int
pl_process_search (const char *text) {
    playlist_head[PL_SEARCH] = NULL;
    playlist_tail[PL_SEARCH] = NULL;
    int search_count = 0;
    if (*text) {
        for (playItem_t *it = playlist_head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
            it->selected = 0;
            for (metaInfo_t *m = it->meta; m; m = m->next) {
//                if (strcasestr (m->value, text)) {
                if (utfcasestr (m->value, text)) {
                    // add to list
                    it->next[PL_SEARCH] = NULL;
                    if (playlist_tail[PL_SEARCH]) {
                        playlist_tail[PL_SEARCH]->next[PL_SEARCH] = it;
                        playlist_tail[PL_SEARCH] = it;
                    }
                    else {
                        playlist_head[PL_SEARCH] = playlist_tail[PL_SEARCH] = it;
                    }
                    it->selected = 1;
                    search_count++;
                    break;
                }
            }
        }
    }
    return search_count;
}

int
pl_playqueue_push (playItem_t *it) {
    if (playqueue_count == PLAYQUEUE_SIZE) {
        trace ("playqueue is full\n");
        return -1;
    }
    playqueue[playqueue_count++] = it;
    return 0;
}

void
pl_playqueue_clear (void) {
    playqueue_count = 0;
}

void
pl_playqueue_pop (void) {
    if (!playqueue_count) {
        return;
    }
    if (playqueue_count == 1) {
        playqueue_count = 0;
        return;
    }
    memmove (&playqueue[0], &playqueue[1], (playqueue_count-1) * sizeof (playItem_t*));
    playqueue_count--;
}

void
pl_playqueue_remove (playItem_t *it) {
    for (;;) {
        int i;
        for (i = 0; i < playqueue_count; i++) {
            if (playqueue[i] == it) {
                if (i < playqueue_count-1) {
                    memmove (&playqueue[i], &playqueue[i+1], (playqueue_count-i) * sizeof (playItem_t*));
                }
                playqueue_count--;
                break;
            }
        }
        if (i == playqueue_count) {
            break;
        }
    }
}
int
pl_playqueue_test (playItem_t *it) {
    for (int i = 0; i < playqueue_count; i++) {
        if (playqueue[i] == it) {
            return i;
        }
    }
    return -1;
}
