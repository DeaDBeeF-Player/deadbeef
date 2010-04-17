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
#ifndef __linux__
#define _POSIX_C_SOURCE
#endif
#include <limits.h>
#include "playlist.h"
#include "streamer.h"
#include "messagepump.h"
#include "playback.h"
#include "plugins.h"
#include "junklib.h"
#include "vfs.h"
#include "conf.h"
#include "utf8.h"
#include "common.h"
#include "threading.h"
#include "metacache.h"

#define DISABLE_LOCKING 0
#define DEBUG_LOCKING 0

// file format revision history
// 1.1->1.2 changelog:
//    added flags field
// 1.0->1.1 changelog:
//    added sample-accurate seek positions for sub-tracks
#define PLAYLIST_MAJOR_VER 1
#define PLAYLIST_MINOR_VER 2

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define SKIP_BLANK_CUE_TRACKS 1

#define min(x,y) ((x)<(y)?(x):(y))

#define PLAYQUEUE_SIZE 100
static playItem_t *playqueue[100];
static int playqueue_count = 0;

static int playlists_count = 0;
static playlist_t *playlists_head = NULL;
static playlist_t *playlist = NULL; // current playlist
static int plt_loading = 0; // disable sending event about playlist switch, config regen, etc

#if !DISABLE_LOCKING
static uintptr_t mutex;
static uintptr_t mutex_plt;
#endif

#define LOCK {pl_lock();}
#define UNLOCK {pl_unlock();}

#define PLT_LOCK {plt_lock();}
#define PLT_UNLOCK {plt_unlock();}

#define GLOBAL_LOCK {pl_global_lock();}
#define GLOBAL_UNLOCK {pl_global_unlock();}

int
pl_init (void) {
#if !DISABLE_LOCKING
    mutex = mutex_create ();
    mutex_plt = mutex_create ();
#endif
}

void
pl_free (void) {
#if !DISABLE_LOCKING
    if (mutex) {
        mutex_free (mutex);
        mutex = 0;
    }
    if (mutex_plt) {
        mutex_free (mutex_plt);
        mutex_plt = 0;
    }
#endif
}

#if DEBUG_LOCKING
int plt_lock_cnt = 0;
#endif
void
plt_lock (void) {
#if !DISABLE_LOCKING
    mutex_lock (mutex_plt);
#if DEBUG_LOCKING
    plt_lock_cnt++;
    printf ("cnt: %d\n", plt_lock_cnt);
#endif
#endif
}

void
plt_unlock (void) {
#if !DISABLE_LOCKING
    mutex_unlock (mutex_plt);
#if DEBUG_LOCKING
    plt_lock_cnt--;
    printf ("cnt: %d\n", plt_lock_cnt);
#endif
#endif
}

#if DEBUG_LOCKING
int pl_lock_cnt = 0;
#endif
void
pl_lock (void) {
#if !DISABLE_LOCKING
    mutex_lock (mutex);
#if DEBUG_LOCKING
    pl_lock_cnt++;
    printf ("pcnt: %d\n", pl_lock_cnt);
#endif
#endif
}

void
pl_unlock (void) {
#if !DISABLE_LOCKING
    mutex_unlock (mutex);
#if DEBUG_LOCKING
    pl_lock_cnt--;
    printf ("pcnt: %d\n", pl_lock_cnt);
#endif
#endif
}

void
pl_global_lock (void) {
    PLT_LOCK;
    LOCK;
}

void
pl_global_unlock (void) {
    UNLOCK;
    PLT_UNLOCK;
}

static void
pl_item_free (playItem_t *it);

static void
plt_gen_conf (void) {
    if (plt_loading) {
        return;
    }
    PLT_LOCK;
    int cnt = plt_get_count ();
    int i;
    conf_remove_items ("playlist.tab.");

    playlist_t *p = playlists_head;
    for (i = 0; i < cnt; i++, p = p->next) {
        char s[100];
        snprintf (s, sizeof (s), "playlist.tab.%d", i);
        conf_set_str (s, p->title);
    }
    PLT_UNLOCK;
}

playlist_t *
plt_get_curr_ptr (void) {
    return playlist;
}

int
plt_get_count (void) {
    return playlists_count;
}

playItem_t *
plt_get_head (int plt) {
    playlist_t *p = playlists_head;
    for (int i = 0; p && i <= plt; i++, p = p->next) {
        if (i == plt) {
            if (p->head[PL_MAIN]) {
                pl_item_ref (p->head[PL_MAIN]);
            }
            return p->head[PL_MAIN];
        }
    }
    return NULL;
}

int
plt_get_sel_count (int plt) {
    playlist_t *p = playlists_head;
    for (int i = 0; p && i <= plt; i++, p = p->next) {
        if (i == plt) {
            int cnt = 0;
            LOCK;
            for (playItem_t *it = p->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
                if (it->selected) {
                    cnt++;
                }
            }
            UNLOCK;
            return cnt;
        }
    }
    return 0;
}

int
plt_add (int before, const char *title) {
    trace ("plt_add\n");
    playlist_t *plt = malloc (sizeof (playlist_t));
    memset (plt, 0, sizeof (playlist_t));
    plt->title = strdup (title);

    trace ("locking\n");
    PLT_LOCK;
    trace ("locked\n");
    playlist_t *p_before = NULL;
    playlist_t *p_after = playlists_head;

    int i;
    for (i = 0; i < before; i++) {
        if (i >= before+1) {
            break;
        }
        p_before = p_after;
        if (p_after) {
            p_after = p_after->next;
        }
        else {
            p_after = playlists_head;
        }
    }
    
    if (p_before) {
        p_before->next = plt;
    }
    else {
        playlists_head = plt;
    }
    plt->next = p_after;
    playlists_count++;

    if (!playlist) {
        playlist = plt;
        if (!plt_loading) {
            // need to delete old playlist file if exists
            char path[PATH_MAX];
            if (snprintf (path, sizeof (path), "%s/playlists/%d.dbpl", dbconfdir, playlists_count-1) <= sizeof (path)) {
                unlink (path);
            }
            pl_save_current ();
        }
    }
    PLT_UNLOCK;

    plt_gen_conf ();
    if (!plt_loading) {
        plug_trigger_event (DB_EV_PLAYLISTSWITCH, 0);
    }
    return playlists_count-1;
}

void
plt_remove (int plt) {
    int i;
    PLT_LOCK;

    // find playlist and notify streamer
    playlist_t *p = playlists_head;
    playlist_t *prev = NULL;
    for (i = 0; p && i < plt; i++) {
        prev = p;
        p = p->next;
    }
    streamer_notify_playlist_deleted (p);

    if (!plt_loading && (playlists_head && !playlists_head->next)) {
        trace ("warning: deleting last playlist\n");
        pl_clear ();
        free (playlist->title);
        playlist->title = strdup ("Default");
        PLT_UNLOCK;
        plt_gen_conf ();
        plug_trigger_event (DB_EV_PLAYLISTSWITCH, 0);
        return;
    }
    if (i != plt) {
        trace ("plt_remove %d failed\n", i);
    }
    if (p) {
        if (!prev) {
            playlists_head = p->next;
        }
        else {
            prev->next = p->next;
        }
    }
    playlist_t *old = playlist;
    playlist = p;
    pl_clear ();
    playlist = old;
    if (p == playlist) {
        playlist = prev ? prev : playlists_head;
    }
    free (p->title);
    free (p);
    playlists_count--;
    PLT_UNLOCK;

    plt_gen_conf ();
    if (!plt_loading) {
        plug_trigger_event (DB_EV_PLAYLISTSWITCH, 0);
    }
}

void
plt_set_curr (int plt) {
    int i;
    PLT_LOCK;
    playlist_t *p = playlists_head;
    for (i = 0; p && p->next && i < plt; i++) {
        p = p->next;
    }
#if 0
    if (i != plt) {
        PLT_UNLOCK;
        trace ("plt_set_curr %d failed\n", plt);
        return;
    }
#endif
    if (p != playlist) {
        playlist = p;
        if (!plt_loading) {
            plug_trigger_event (DB_EV_PLAYLISTSWITCH, 0);
            conf_set_int ("playlist.current", plt_get_curr ());
            conf_save ();
        }
    }
    PLT_UNLOCK;
}

int
plt_get_curr (void) {
    int i;
    PLT_LOCK;
    playlist_t *p = playlists_head;
    for (i = 0; p && i < playlists_count; i++) {
        if (p == playlist) {
            PLT_UNLOCK;
            return i;
        }
        p = p->next;
    }
    PLT_UNLOCK;
    return -1;
}

int
plt_get_idx_of (playlist_t *plt) {
    int i;
    PLT_LOCK;
    playlist_t *p = playlists_head;
    for (i = 0; p && i < playlists_count; i++) {
        if (p == plt) {
            PLT_UNLOCK;
            return i;
        }
        p = p->next;
    }
    PLT_UNLOCK;
    return -1;
}

int
plt_get_title (int plt, char *buffer, int bufsize) {
    int i;
    PLT_LOCK;
    playlist_t *p = playlists_head;
    for (i = 0; p && i <= plt; i++) {
        if (i == plt) {
            strncpy (buffer, p->title, bufsize);
            buffer[bufsize-1] = 0;
            PLT_UNLOCK;
            return 0;
        }
        p = p->next;
    }
    PLT_UNLOCK;
    buffer[0] = 0;
    return -1;
}

int
plt_set_title (int plt, const char *title) {
    int i;
    PLT_LOCK;
    playlist_t *p = playlists_head;
    for (i = 0; p && i <= plt; i++) {
        if (i == plt) {
            free (p->title);
            p->title = strdup (title);
            break;
        }
        p = p->next;
    }
    PLT_UNLOCK;
    plt_gen_conf ();
    if (!plt_loading) {
        plug_trigger_event (DB_EV_PLAYLISTSWITCH, 0);
    }
    return i == plt ? 0 : -1;
}

void
plt_free (void) {
    trace ("plt_free\n");
    PLT_LOCK;
    plt_loading = 1;
    while (playlists_head) {

        for (playItem_t *it = playlists_head->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
            if (it->_refc > 1) {
                fprintf (stderr, "\033[0;31mWARNING: playitem %p %s has refc=%d at delete time\033[37;0m\n", it, it->fname, it->_refc);
            }
        }

        plt_remove (0);
    }
    plt_loading = 0;
    PLT_UNLOCK;
}

void
plt_move (int from, int before) {
    if (from == before) {
        return;
    }
    int i;
    PLT_LOCK;
    playlist_t *p = playlists_head;

    playlist_t *pfrom = NULL;
    playlist_t *prev = NULL;
    playlist_t *ins = NULL;

    for (i = 0; p && i <= from; i++) {
        if (i == from) {
            pfrom = p;
            if (prev) {
                prev->next = p->next;
            }
            else {
                playlists_head = p->next;
            }
            break;
        }
        prev = p;
        p = p->next;
    }
//    if (before > from) {
//        before--;
//    }

    if (before == 0) {
        pfrom->next = playlists_head;
        playlists_head = pfrom;
    }
    else {
        p = playlists_head;
        for (i = 0; p && i < before; i++) {
            if (i == before-1) {
                playlist_t *next = p->next;
                p->next = pfrom;
                pfrom->next = next;
                break;
            }
            prev = p;
            p = p->next;
        }
    }

    PLT_UNLOCK;
    plt_gen_conf ();
}

void
pl_clear (void) {
    LOCK;
    while (playlist->head[PL_MAIN]) {
        pl_remove_item (playlist->head[PL_MAIN]);
    }
    playlist->current_row[PL_MAIN] = -1;
    playlist->current_row[PL_SEARCH] = -1;
    UNLOCK;
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
    char *endptr;
    long mins = strtol(p, &endptr, 10);
    if (endptr - p < 2 || *endptr != ':') {
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

static playItem_t *
pl_process_cue_track (playItem_t *after, const char *fname, playItem_t **prev, char *track, char *index00, char *index01, char *pregap, char *title, char *performer, char *albumtitle, char *genre, char *date, const char *decoder_id, const char *ftype, int samplerate) {
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
    //float f_index00 = index00[0] ? pl_cue_parse_time (index00) : 0;
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
    playItem_t *it = pl_item_alloc ();
    it->decoder_id = plug_get_decoder_id (decoder_id);
    it->fname = strdup (fname);
    it->tracknum = atoi (track);
    it->startsample = index01[0] ? f_index01 * samplerate : 0;
    it->endsample = -1; // will be filled by next read, or by decoder
    it->filetype = ftype;
    if (performer[0]) {
        pl_add_meta (it, "artist", performer);
    }
    if (albumtitle[0]) {
        pl_add_meta (it, "album", albumtitle);
    }
    if (track[0]) {
        pl_add_meta (it, "track", track);
    }
    if (title[0]) {
        pl_add_meta (it, "title", title);
    }
    if (genre[0]) {
        pl_add_meta (it, "genre", genre);
    }
    if (date[0]) {
        pl_add_meta (it, "year", date);
    }
    it->_flags |= DDB_IS_SUBTRACK;
    after = pl_insert_item (after, it);
    pl_item_unref (it);
    *prev = it;
    return it;
}

playItem_t *
pl_insert_cue_from_buffer (playItem_t *after, playItem_t *origin, const uint8_t *buffer, int buffersize, int numsamples, int samplerate) {
    LOCK;
    playItem_t *ins = after;
    trace ("pl_insert_cue_from_buffer numsamples=%d, samplerate=%d\n", numsamples, samplerate);
    char performer[256] = "";
    char albumtitle[256] = "";
    char genre[256] = "";
    char date[256] = "";
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
        else if (!strncmp (p, "REM GENRE ", 10)) {
            pl_get_qvalue_from_cue (p + 10, sizeof (genre), genre);
        }
        else if (!strncmp (p, "REM DATE ", 9)) {
            pl_get_value_from_cue (p + 9, sizeof (date), date);
        }
        else if (!strncmp (p, "TRACK ", 6)) {
            // add previous track
            after = pl_process_cue_track (after, origin->fname, &prev, track, index00, index01, pregap, title, performer, albumtitle, genre, date, origin->decoder_id, origin->filetype, samplerate);

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
    after = pl_process_cue_track (after, origin->fname, &prev, track, index00, index01, pregap, title, performer, albumtitle, genre, date, origin->decoder_id, origin->filetype, samplerate);
    if (after) {
        trace ("last track endsample: %d\n", numsamples-1);
        after->endsample = numsamples-1;
        pl_set_item_duration (after, (float)(after->endsample - after->startsample + 1) / samplerate);
    }
    // add caller ref
    if (after && after != ins) {
        pl_item_ref (after);
    }
    // copy metadata from embedded tags
    playItem_t *first = ins ? ins->next[PL_MAIN] : playlist->head[PL_MAIN];
    uint32_t f = pl_get_item_flags (origin);
    f |= DDB_TAG_CUESHEET;
    if (pl_find_meta (origin, "cuesheet")) {
        f |= DDB_HAS_EMBEDDED_CUESHEET;
    }
    pl_set_item_flags (origin, f);
    pl_items_copy_junk (origin, first, after);
    UNLOCK;
    return after;
}

playItem_t *
pl_insert_cue (playItem_t *after, playItem_t *origin, int numsamples, int samplerate) {
    trace ("pl_insert_cue numsamples=%d, samplerate=%d\n", numsamples, samplerate);
    int len = strlen (origin->fname);
    char cuename[len+5];
    strcpy (cuename, origin->fname);
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
    return pl_insert_cue_from_buffer (after, origin, buf, sz, numsamples, samplerate);
}

playItem_t *
pl_insert_m3u (playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data) {
    trace ("enter pl_insert_m3u\n");
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
    LOCK;
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
        trace ("pl_insert_m3u: adding file %s\n", nm);
        playItem_t *it = pl_insert_file (after, nm, pabort, cb, user_data);
        if (it) {
            after = it;
        }
        if (pabort && *pabort) {
            UNLOCK;
            return after;
        }
        p = e;
        if (p >= end) {
            break;
        }
    }
    UNLOCK;
    trace ("leave pl_insert_m3u\n");
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
    LOCK;
    while (p < end) {
        p = pl_str_skipspaces (p, end);
        if (p >= end) {
            break;
        }
        if (end-p < 6) {
            break;
        }
        const uint8_t *e;
        int n;
        if (!strncasecmp (p, "file", 4)) {
            if (url[0]) {
                // add track
                playItem_t *it = pl_insert_file (after, url, pabort, cb, user_data);
                if (it) {
                    after = it;
                    pl_set_item_duration (it, atoi (length));
                    if (title[0]) {
                        pl_delete_all_meta (it);
                        pl_add_meta (it, "title", title);
                    }
                }
                if (pabort && *pabort) {
                    UNLOCK;
                    return after;
                }
                url[0] = 0;
                title[0] = 0;
                length[0] = 0;
            }
            p += 4;
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
            n = min (n, sizeof (url)-1);
            memcpy (url, p, n);
            url[n] = 0;
            trace ("url: %s\n", url);
            p = ++e;
        }
        else if (!strncasecmp (p, "title", 5)) {
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
        }
        else if (!strncasecmp (p, "length", 6)) {
            p += 6;
            // skip =
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
            break;
        }
        else {
            trace ("invalid entry in pls file: %s\n", p);
            break;
        }
        while (e < end && *e < 0x20) {
            e++;
        }
        p = e;
    }
    if (url[0]) {
        playItem_t *it = pl_insert_file (after, url, pabort, cb, user_data);
        if (it) {
            after = it;
            pl_set_item_duration (it, atoi (length));
            if (title[0]) {
                pl_delete_all_meta (it);
                pl_add_meta (it, "title", title);
            }
        }
    }
    UNLOCK;
    return after;
}

playItem_t *
pl_insert_file (playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data) {
    trace ("count: %d\n", playlist->count[PL_MAIN]);
    trace ("pl_insert_file %s\n", fname);
    if (!fname || !(*fname)) {
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

        // check if it's URI
        for (; *p; p++) {
            if (!strncmp (p, "://", 3)) {
                break;
            }
            if (!isalpha (*p)) {
                detect_on_access = 0;
                break;
            }
        }

        if (detect_on_access && *p == ':') {
            // check for wrong chars like CR/LF, TAB, etc
            // they are not allowed and might lead to corrupt display in GUI
            int32_t i = 0;
            while (fname[i]) {
                uint32_t c = u8_nextchar (fname, &i);
                if (c < 0x20) {
                    return NULL;
                }
            }
            
            playItem_t *it = pl_item_alloc ();
            it->decoder_id = NULL;
            it->fname = strdup (fname);
            it->filetype = "content";
            it->_duration = -1;
            pl_add_meta (it, "title", NULL);
            after = pl_insert_item (after, it);
            pl_item_unref (it);
            return after;
        }
    }
    else {
        fname += 7;
    }

    DB_decoder_t **decoders = plug_get_decoder_list ();
    // match by decoder
    for (int i = 0; decoders[i]; i++) {
        trace ("matching decoder %d(%s)...\n", i, decoders[i]->plugin.id);
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
    playItem_t *it = pl_insert_file (playlist->tail[PL_MAIN], fname, &abort, cb, user_data);
    if (it) {
        // pl_insert_file doesn't hold reference, don't unref here
        return 0;
    }
    return -1;
}

int
pl_add_dir (const char *dirname, int (*cb)(playItem_t *it, void *data), void *user_data) {
    int abort = 0;
    playItem_t *it = pl_insert_dir (playlist->tail[PL_MAIN], dirname, &abort, cb, user_data);
    if (it) {
        // pl_insert_file doesn't hold reference, don't unref here
        return 0;
    }
    return -1;
}

int
pl_remove_item (playItem_t *it) {
    if (!it)
        return -1;
    streamer_song_removed_notify (it);
    pl_playqueue_remove (it);

    // remove from both lists
    LOCK;
    for (int iter = PL_MAIN; iter <= PL_SEARCH; iter++) {
        if (it->prev[iter] || it->next[iter] || playlist->head[iter] == it || playlist->tail[iter] == it) {
            playlist->count[iter]--;
        }
        if (it->prev[iter]) {
            it->prev[iter]->next[iter] = it->next[iter];
        }
        else {
            playlist->head[iter] = it->next[iter];
        }
        if (it->next[iter]) {
            it->next[iter]->prev[iter] = it->prev[iter];
        }
        else {
            playlist->tail[iter] = it->prev[iter];
        }
    }

    // totaltime
    if (it->_duration > 0) {
        playlist->totaltime -= it->_duration;
        if (playlist->totaltime < 0) {
            playlist->totaltime = 0;
        }
    }
    UNLOCK;
    pl_item_unref (it);
    return 0;
}

int
pl_getcount (int iter) {
    if (!playlist) {
        return 0;
    }
    return playlist->count[iter];
}

int
pl_getselcount (void) {
    // FIXME: slow!
    int cnt = 0;
    LOCK;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        if (it->selected) {
            cnt++;
        }
    }
    UNLOCK;
    return cnt;
}

playItem_t *
pl_get_for_idx_and_iter (int idx, int iter) {
    LOCK;
    playItem_t *it = playlist->head[iter];
    while (idx--) {
        if (!it) {
            UNLOCK;
            return NULL;
        }
        it = it->next[iter];
    }
    if (it) {
        pl_item_ref (it);
    }
    UNLOCK;
    return it;
}

playItem_t *
pl_get_for_idx (int idx) {
    return pl_get_for_idx_and_iter (idx, PL_MAIN);
}

int
pl_get_idx_of (playItem_t *it) {
    LOCK;
    playItem_t *c = playlist->head[PL_MAIN];
    int idx = 0;
    while (c && c != it) {
        c = c->next[PL_MAIN];
        idx++;
    }
    if (!c) {
        UNLOCK;
        return -1;
    }
    UNLOCK;
    return idx;
}

playItem_t *
pl_insert_item (playItem_t *after, playItem_t *it) {
    GLOBAL_LOCK;
    pl_item_ref (it);
    if (!after) {
        it->next[PL_MAIN] = playlist->head[PL_MAIN];
        it->prev[PL_MAIN] = NULL;
        if (playlist->head[PL_MAIN]) {
            playlist->head[PL_MAIN]->prev[PL_MAIN] = it;
        }
        else {
            playlist->tail[PL_MAIN] = it;
        }
        playlist->head[PL_MAIN] = it;
    }
    else {
        it->prev[PL_MAIN] = after;
        it->next[PL_MAIN] = after->next[PL_MAIN];
        if (after->next[PL_MAIN]) {
            after->next[PL_MAIN]->prev[PL_MAIN] = it;
        }
        after->next[PL_MAIN] = it;
        if (after == playlist->tail[PL_MAIN]) {
            playlist->tail[PL_MAIN] = it;
        }
    }
    it->in_playlist = 1;

    playlist->count[PL_MAIN]++;

    // shuffle
    it->shufflerating = rand ();
    it->played = 0;

    // totaltime
    if (it->_duration > 0) {
        playlist->totaltime += it->_duration;
    }

    GLOBAL_UNLOCK;
    return it;
}

void
pl_item_copy (playItem_t *out, playItem_t *it) {
    LOCK;
    out->fname = strdup (it->fname);
    out->decoder_id = it->decoder_id;
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
    out->_refc = 1;
    // copy metainfo
    DB_metaInfo_t *prev = NULL;
    DB_metaInfo_t *meta = it->meta;
    while (meta) {
        DB_metaInfo_t *m = malloc (sizeof (DB_metaInfo_t));
        m->key = metacache_add_string (meta->key);
        m->value = metacache_add_string (meta->value);
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
    UNLOCK;
}

playItem_t *
pl_item_alloc (void) {
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->replaygain_album_peak = 1;
    it->replaygain_track_peak = 1;
    it->_refc = 1;
    return it;
}

void
pl_item_ref (playItem_t *it) {
    LOCK;
    it->_refc++;
    //trace ("\033[0;34m+it %p: refc=%d: %s\033[37;0m\n", it, it->_refc, it->fname);
    UNLOCK;
}

static void
pl_item_free (playItem_t *it) {
    LOCK;
    if (it) {
        if (it->fname) {
            free (it->fname);
        }
        while (it->meta) {
            DB_metaInfo_t *m = it->meta;
            it->meta = m->next;
            metacache_remove_string (m->key);
            metacache_remove_string (m->value);
            free (m);
        }
        free (it);
    }
    UNLOCK;
}

void
pl_item_unref (playItem_t *it) {
    LOCK;
    it->_refc--;
    //trace ("\033[0;31m-it %p: refc=%d: %s\033[37;0m\n", it, it->_refc, it->fname);
    if (it->_refc < 0) {
        trace ("\033[0;31mplaylist: bad refcount on item %p\033[37;0m\n", it);
    }
    if (it->_refc <= 0) {
        //printf ("\033[0;31mdeleted %s\033[37;0m\n", it->fname);
        pl_item_free (it);
    }
    UNLOCK;
}

void
pl_add_meta (playItem_t *it, const char *key, const char *value) {
    LOCK;
    // check if it's already set
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            // duplicate key
            UNLOCK;
            return;
        }
        m = m->next;
    }
    // add
    char str[256];
    if (!value || !*value) {
        if (!strcasecmp (key, "title")) {
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
            UNLOCK;
            return;
        }
    }
    m = malloc (sizeof (DB_metaInfo_t));
    m->key = metacache_add_string (key); //key;
    m->value = metacache_add_string (value); //strdup (value);
    m->next = it->meta;
    it->meta = m;
    UNLOCK;
}

void
pl_append_meta (playItem_t *it, const char *key, const char *value) {
    const char *old = pl_find_meta (it, key);
    if (!old) {
        pl_add_meta (it, key, value);
    }
    else {
        // check for duplicate data
        const char *str = old;
        int len;
        while (str) {
            char *next = strchr (str, '\n');

            if (next) {
                len = next - str;
                next++;
            }
            else {
                len = strlen (str);
            }

            if (!strncmp (str, value, len)) {
                return;
            }

            str = next;
        }
        int sz = strlen (old) + strlen (value) + 2;
        char out[sz];
        snprintf (out, sz, "%s\n%s", old, value);
        pl_replace_meta (it, key, out);
    }
}

void
pl_replace_meta (playItem_t *it, const char *key, const char *value) {
    LOCK;
    // check if it's already set
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            break;
        }
        m = m->next;
    }
    if (m) {
        metacache_remove_string (m->value);//free (m->value);
        m->value = metacache_add_string (value);//strdup (value);
        UNLOCK;
        return;
    }
    else {
        pl_add_meta (it, key, value);
    }
    UNLOCK;
}

const char *
pl_find_meta (playItem_t *it, const char *key) {
    DB_metaInfo_t *m = it->meta;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            return m->value;
        }
        m = m->next;
    }
    return NULL;
}

DB_metaInfo_t *
pl_get_metadata (playItem_t *it) {
    return it->meta;
}

int
pl_delete_selected (void) {
    GLOBAL_LOCK;
    int i = 0;
    int ret = -1;
    playItem_t *next = NULL;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = next, i++) {
        next = it->next[PL_MAIN];
        if (it->selected) {
            if (ret == -1) {
                ret = i;
            }
            pl_remove_item (it);
        }
    }
    if (playlist->current_row[PL_MAIN] >= playlist->count[PL_MAIN]) {
        playlist->current_row[PL_MAIN] = playlist->count[PL_MAIN] - 1;
    }
    if (playlist->current_row[PL_SEARCH] >= playlist->count[PL_SEARCH]) {
        playlist->current_row[PL_SEARCH] = playlist->count[PL_SEARCH] - 1;
    }
    GLOBAL_UNLOCK;
    return ret;
}

void
pl_crop_selected (void) {
    GLOBAL_LOCK;
    playItem_t *next = NULL;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = next) {
        next = it->next[PL_MAIN];
        if (!it->selected) {
            pl_remove_item (it);
        }
    }
    GLOBAL_UNLOCK;
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
    GLOBAL_LOCK;
    if (fwrite (magic, 1, 4, fp) != 4) {
        goto save_fail;
    }
    if (fwrite (&majorver, 1, 1, fp) != 1) {
        goto save_fail;
    }
    if (fwrite (&minorver, 1, 1, fp) != 1) {
        goto save_fail;
    }
    uint32_t cnt = playlist->count[PL_MAIN];
    if (fwrite (&cnt, 1, 4, fp) != 4) {
        goto save_fail;
    }
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        uint16_t l;
        uint8_t ll;
        l = strlen (it->fname);
        if (fwrite (&l, 1, 2, fp) != 2) {
            goto save_fail;
        }
        if (fwrite (it->fname, 1, l, fp) != l) {
            goto save_fail;
        }
        if (it->decoder_id) {
            ll = strlen (it->decoder_id);
            if (fwrite (&ll, 1, 1, fp) != 1) {
                goto save_fail;
            }
            if (fwrite (it->decoder_id, 1, ll, fp) != ll) {
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
        if (fwrite (&it->_flags, 1, 4, fp) != 4) {
            goto save_fail;
        }

        int16_t nm = 0;
        DB_metaInfo_t *m;
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
    GLOBAL_UNLOCK;
    fclose (fp);
    return 0;
save_fail:
    GLOBAL_UNLOCK;
    fclose (fp);
    unlink (fname);
    return -1;
}

int
pl_save_current (void) {
    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/playlists", dbconfdir) > sizeof (path)) {
        fprintf (stderr, "error: failed to make path string for playlists folder\n");
        return -1;
    }
    // make folder
    mkdir (path, 0755);

    PLT_LOCK;
    int curr = plt_get_curr ();
    int err = 0;

    plt_loading = 1;
    if (snprintf (path, sizeof (path), "%s/playlists/%d.dbpl", dbconfdir, curr) > sizeof (path)) {
        fprintf (stderr, "error: failed to make path string for playlist file\n");
        PLT_UNLOCK;
        return -1;
    }
    err = pl_save (path);
    plt_loading = 0;
    PLT_UNLOCK;
    return err;
}

int
pl_save_all (void) {
    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/playlists", dbconfdir) > sizeof (path)) {
        fprintf (stderr, "error: failed to make path string for playlists folder\n");
        return -1;
    }
    // make folder
    mkdir (path, 0755);

    PLT_LOCK;
    playlist_t *p = playlists_head;
    int i;
    int cnt = plt_get_count ();
    int curr = plt_get_curr ();
    int err = 0;

    plt_loading = 1;
    for (i = 0; i < cnt; i++, p = p->next) {
        plt_set_curr (i);
        if (snprintf (path, sizeof (path), "%s/playlists/%d.dbpl", dbconfdir, i) > sizeof (path)) {
            fprintf (stderr, "error: failed to make path string for playlist file\n");
            err = -1;
            break;
        }
        err = pl_save (path);
        if (err < 0) {
            break;
        }
    }
    plt_set_curr (curr);
    plt_loading = 0;
    PLT_UNLOCK;
    return err;
}

int
pl_load (const char *fname) {
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        return -1;
    }
    GLOBAL_LOCK;
    pl_clear ();
    uint8_t majorver;
    uint8_t minorver;
    playItem_t *it = NULL;
    char magic[4];
    if (fread (magic, 1, 4, fp) != 4) {
        goto load_fail;
    }
    if (strncmp (magic, "DBPL", 4)) {
        trace ("bad signature\n");
        goto load_fail;
    }
    if (fread (&majorver, 1, 1, fp) != 1) {
        goto load_fail;
    }
    if (majorver != PLAYLIST_MAJOR_VER) {
        trace ("bad majorver=%d\n", majorver);
        goto load_fail;
    }
    if (fread (&minorver, 1, 1, fp) != 1) {
        goto load_fail;
    }
    if (minorver < 1/*PLAYLIST_MINOR_VER*/) {
        trace ("bad minorver=%d\n", minorver);
        goto load_fail;
    }
    trace ("playlist version=%d.%d\n", majorver, minorver);
    uint32_t cnt;
    if (fread (&cnt, 1, 4, fp) != 4) {
        goto load_fail;
    }
    for (uint32_t i = 0; i < cnt; i++) {
        it = pl_item_alloc ();
        if (!it) {
            goto load_fail;
        }
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
            it->decoder_id = plug_get_decoder_id (decoder);
//            if (!it->decoder) {
//                goto load_fail;
//            }
        }
        else {
            it->decoder_id = NULL;
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
            else if (it->decoder_id) {
                DB_decoder_t *dec = plug_get_decoder_for_id (it->decoder_id);
                if (dec && dec->filetypes) {
                    for (int i = 0; dec->filetypes[i]; i++) {
                        if (!strcasecmp (dec->filetypes[i], ftype)) {
                            it->filetype = dec->filetypes[i];
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
        if (minorver >= 2) {
            if (fread (&it->_flags, 1, 4, fp) != 4) {
                goto load_fail;
            }
        }
        else {
            if (it->tracknum != 0) {
                it->_flags |= DDB_IS_READONLY; // to prevent editing metadata in subsongs
            }
        }

        int16_t nm = 0;
        if (fread (&nm, 1, 2, fp) != 2) {
            goto load_fail;
        }
        for (int i = 0; i < nm; i++) {
            char key[1024];
            char value[1024];
#if 0
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
                "copyright",
                NULL
            };
#endif

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
                // skip
                fseek (fp, l, SEEK_CUR);
            }
            else {
                if (fread (value, 1, l, fp) != l) {
                    goto load_fail;
                }
                value[l] = 0;
                //printf ("%s=%s\n", key, value);
                pl_add_meta (it, key, value);
#if 0
                for (int n = 0; valid_keys[n]; n++) {
                    if (!strcmp (valid_keys[n], key)) {
                        pl_add_meta (it, valid_keys[n], value);
                        break;
                    }
                }
#endif
            }
        }
        pl_insert_item (playlist->tail[PL_MAIN], it);
        pl_item_unref (it);
        trace ("last playlist item refc: %d\n", it->_refc);
        it = NULL;
    }
    GLOBAL_UNLOCK;
    if (fp) {
        fclose (fp);
    }
    return 0;
load_fail:
    fprintf (stderr, "playlist load fail (%s)!\n", fname);
    if (it) {
        pl_item_unref (it);
    }
    pl_clear ();
    GLOBAL_UNLOCK;
    if (fp) {
        fclose (fp);
    }
    return -1;
}

int
pl_load_all (void) {
    int i = 0;
    int err = 0;
    char path[1024];
    DB_conf_item_t *it = conf_find ("playlist.tab.", NULL);
    if (!it) {
        fprintf (stderr, "INFO: loading legacy default playlist\n");
        // legacy (0.3.3 and earlier)
        char defpl[1024]; // $HOME/.config/deadbeef/default.dbpl
        if (snprintf (defpl, sizeof (defpl), "%s/deadbeef/default.dbpl", confdir) > sizeof (defpl)) {
            fprintf (stderr, "error: cannot make string with default playlist path\n");
            return -1;
        }
        plt_add (plt_get_count (), "Default");
        return pl_load (defpl);
    }
    trace ("pl_load_all started\n");
    GLOBAL_LOCK;
    trace ("locked\n");
    plt_loading = 1;
    while (it) {
        fprintf (stderr, "INFO: loading playlist %s\n", it->value);
        if (!err) {
            plt_add (plt_get_count (), it->value);
            plt_set_curr (plt_get_count () - 1);
        }
        err = 0;
        if (snprintf (path, sizeof (path), "%s/playlists/%d.dbpl", dbconfdir, i) > sizeof (path)) {
            fprintf (stderr, "error: failed to make path string for playlist filename\n");
            err = -1;
        }
        else {
            fprintf (stderr, "INFO: from file %s\n", path);
            err = pl_load (path);
        }
        it = conf_find ("playlist.tab.", it);
        fprintf (stderr, "conf_find returned %p\n", it);
        i++;
    }
    plt_set_curr (0);
    plt_loading = 0;
    plt_gen_conf ();
    plug_trigger_event (DB_EV_PLAYLISTSWITCH, 0);
    GLOBAL_UNLOCK;
    trace ("pl_load_all finished\n");
    return err;
}

void
pl_select_all (void) {
    GLOBAL_LOCK;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        it->selected = 1;
    }
    GLOBAL_UNLOCK;
}


void
pl_reshuffle (playItem_t **ppmin, playItem_t **ppmax) {
    GLOBAL_LOCK;
    playItem_t *pmin = NULL;
    playItem_t *pmax = NULL;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
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
    GLOBAL_UNLOCK;
}

void
pl_delete_all_meta (playItem_t *it) {
    LOCK;
    while (it->meta) {
        DB_metaInfo_t *m = it->meta;
        it->meta = m->next;
        metacache_remove_string (m->key);
        metacache_remove_string (m->value);
        free (m);
    }
    it->meta = NULL;
    UNLOCK;
}

void
pl_set_item_duration (playItem_t *it, float duration) {
    GLOBAL_LOCK;
    if (it->in_playlist) {
        if (it->_duration > 0) {
            playlist->totaltime -= it->_duration;
        }
        if (duration > 0) {
            playlist->totaltime += duration;
        }
        if (playlist->totaltime < 0) {
            playlist->totaltime = 0;
        }
    }
    it->_duration = duration;
    GLOBAL_UNLOCK;
}

float
pl_get_item_duration (playItem_t *it) {
    LOCK;
    float d = it->_duration;
    UNLOCK;
    return d;
}

int
pl_format_item_queue (playItem_t *it, char *s, int size) {
    *s = 0;
    if (!playqueue_count) {
        return 0;
    }
    LOCK;
    int init = 1;
    int initsize = size;
    int len;
    for (int i = 0; i < playqueue_count; i++) {
        if (size <= 0) {
            break;
        }
        if (playqueue[i] == it) {
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
    UNLOCK;
    return initsize-size;
}

static void
pl_format_time (float t, char *dur, int size) {
    if (t >= 0) {
        int hourdur = t / (60 * 60);
        int mindur = (t - hourdur * 60 * 60) / 60;
        int secdur = t - hourdur*60*60 - mindur * 60;

        if (hourdur) {
            snprintf (dur, size, "%d:%02d:%02d", hourdur, mindur, secdur);
        }
        else {
            snprintf (dur, size, "%d:%02d", mindur, secdur);
        }
    }
    else {
        strcpy (dur, "-:--");
    }
}

// following helpers must be called from within pl_lock/unlock block
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
pl_format_duration (playItem_t *it, const char *ret, char *dur, int size) {
    if (ret) {
        return ret;
    }
    pl_format_time (it->_duration, dur, size);
    return dur;
}

static const char *
pl_format_elapsed (const char *ret, char *elapsed, int size) {
    if (ret) {
        return ret;
    }
    float playpos = streamer_get_playpos ();
    pl_format_time (playpos, elapsed, size);
    return elapsed;
}

int
pl_format_title (playItem_t *it, int idx, char *s, int size, int id, const char *fmt) {
    char dur[50];
    char elp[50];
    char fno[50];
    char tags[200];
    const char *artist = NULL;
    const char *album = NULL;
    const char *track = NULL;
    const char *title = NULL;
    const char *duration = NULL;
    const char *elapsed = NULL;
    const char *year = NULL;
    const char *genre = NULL;
    const char *comment = NULL;
    const char *copyright = NULL;
    const char *filename = NULL;

    char *ss = s;

    LOCK;
    if (id != -1) {
        const char *text = NULL;
        switch (id) {
        case DB_COLUMN_FILENUMBER:
            if (idx == -1) {
                idx = pl_get_idx_of (it);
            }
            snprintf (fno, sizeof (fno), "%d", idx+1);
            text = fno;
            break;
        case DB_COLUMN_PLAYING:
            UNLOCK;
            return pl_format_item_queue (it, s, size);
        case DB_COLUMN_ARTIST_ALBUM:
            {
                char artistalbum[1024];
                artist = pl_get_meta_cached (it, "artist", artist, "Unknown artist");
                album = pl_get_meta_cached (it, "album", album, "Unknown album");
                snprintf (artistalbum, sizeof (artistalbum), "%s - %s", artist, album);
                text = artistalbum;
            }
            break;
        case DB_COLUMN_ARTIST:
            text = (artist = pl_get_meta_cached (it, "artist", artist, "Unknown artist"));
            break;
        case DB_COLUMN_ALBUM:
            text = (album = pl_get_meta_cached (it, "album", artist, "Unknown album"));
            break;
        case DB_COLUMN_TITLE:
            text = (title = pl_get_meta_cached (it, "title", artist, "?"));
            break;
        case DB_COLUMN_DURATION:
            text = (duration = pl_format_duration (it, duration, dur, sizeof (dur)));
            break;
        case DB_COLUMN_TRACK:
            track = pl_get_meta_cached (it, "track", track, "");
            text = track;
#if 0 // kept for tracing purposes
            char ftrk[50];
            if (isdigit (track[0])) {
                snprintf (ftrk, sizeof (ftrk), "%03d (%s)", atoi (track), track);
                text = ftrk;
            }
            else {
                snprintf (ftrk, sizeof (ftrk), "-1 (%s)", track);
                text = ftrk;
            }
#endif
            break;
        }
        if (text) {
            strncpy (s, text, size);
            UNLOCK;
            for (ss = s; *ss; ss++) {
                if (*ss == '\n') {
                    *ss = ';';
                }
            }
            return strlen (s);
        }
        else {
            s[0] = 0;
        }
        UNLOCK;
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
                meta = (artist = pl_get_meta_cached (it, "artist", artist, "Unknown artist"));
            }
            else if (*fmt == 't') {
                meta = (title = pl_get_meta_cached (it, "title", title, "?"));
            }
            else if (*fmt == 'b') {
                meta = (album = pl_get_meta_cached (it, "album", album, "Unknown album"));
            }
            else if (*fmt == 'n') {
                meta = (track = pl_get_meta_cached (it, "track", track, ""));
            }
            else if (*fmt == 'y') {
                meta = (year = pl_get_meta_cached (it, "year", year, ""));
            }
            else if (*fmt == 'g') {
                meta = (genre = pl_get_meta_cached (it, "genre", genre, ""));
            }
            else if (*fmt == 'c') {
                meta = (comment = pl_get_meta_cached (it, "comment", comment, ""));
            }
            else if (*fmt == 'r') {
                meta = (copyright = pl_get_meta_cached (it, "copyright", copyright, ""));
            }
            else if (*fmt == 'l') {
                const char *value = (duration = pl_format_duration (it, duration, dur, sizeof (dur)));
                while (n > 0 && *value) {
                    *s++ = *value++;
                    n--;
                }
            }
            else if (*fmt == 'e') {
                // what a hack..
                const char *value = (elapsed = pl_format_elapsed (elapsed, elp, sizeof (elp)));
                while (n > 0 && *value) {
                    *s++ = *value++;
                    n--;
                }
            }
            else if (*fmt == 'f') {
                filename = it->fname + strlen (it->fname) - 1;
                while (filename > it->fname && (*filename) != '/') {
                    filename--;
                }
                if (*filename == '/') {
                    filename++;
                }
                meta = filename;
            }
            else if (*fmt == 'T') {
//                meta = pl_find_meta (it, "tags");
                char *t = tags;
                char *e = tags + sizeof (tags);
                int c;
                *t = 0;

                if (it->_flags & DDB_TAG_ID3V1) {
                    c = snprintf (t, e-t, "ID3v1 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_ID3V22) {
                    c = snprintf (t, e-t, "ID3v2.2 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_ID3V23) {
                    c = snprintf (t, e-t, "ID3v2.3 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_ID3V24) {
                    c = snprintf (t, e-t, "ID3v2.4 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_APEV2) {
                    c = snprintf (t, e-t, "APEv2 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_VORBISCOMMENTS) {
                    c = snprintf (t, e-t, "VorbisComments | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_CUESHEET) {
                    c = snprintf (t, e-t, "CueSheet | ");
                    t += c;
                }
                if (t != tags) {
                    *(t - 3) = 0;
                }
                meta = tags;
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

    UNLOCK;

    // replace all \n with ;
    while (*ss) {
        if (*ss == '\n') {
            *ss = ';';
        }
        ss++;
    }

    return size - n - 1;
}

void
pl_sort (int iter, int id, const char *format, int ascending) {
    if (id == DB_COLUMN_FILENUMBER) {
        return;
    }
    GLOBAL_LOCK;
    int sorted = 0;
    do {
        sorted = 1;
        playItem_t *it;
        for (it = playlist->head[iter]; it; it = it->next[iter]) {
            playItem_t *next = it->next[iter];
            if (!next) {
                break;
            }
            int cmp;
            if (id == DB_COLUMN_DURATION) {
                cmp = ascending ? next->_duration > it->_duration : it->_duration > next->_duration;
            }
            else if (id == DB_COLUMN_TRACK) {
                const char *t;
                t = pl_find_meta (it, "track");
                int a = t ? atoi (t) : -1;
                t = pl_find_meta (next, "track");
                int b = t ? atoi (t) : -1;
                cmp = ascending ? b > a : a > b;
            }
            else {
                char title1[1024];
                char title2[1024];
                pl_format_title (it, -1, title1, sizeof (title1), id, format);
                pl_format_title (next, -1, title2, sizeof (title2), id, format);
                cmp = ascending ? strcmp (title1, title2) < 0 : strcmp (title1, title2) > 0;
            }
            if (cmp) {
//                printf ("%p %p swapping %s and %s\n", it, next, meta1, meta2);
                sorted = 0;
                // swap them
                if (it->prev[iter]) {
                    it->prev[iter]->next[iter] = next;
//                    printf ("it->prev->next = it->next\n");
                }
                else {
                    playlist->head[iter] = next;
                    next->prev[iter] = NULL;
//                    printf ("head = it->next\n");
                }
                if (next->next[iter]) {
                    next->next[iter]->prev[iter] = it;
//                    printf ("it->next->next->prev = it\n");
                }
                else {
                    playlist->tail[iter] = it;
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
        }
    } while (!sorted);
    GLOBAL_UNLOCK;
}

void
pl_reset_cursor (void) {
    int i;
    PLT_LOCK;
    for (i = 0; i < PL_MAX_ITERATORS; i++) {
        playlist->current_row[i] = -1;
    }
    PLT_UNLOCK;
}

float
pl_get_totaltime (void) {
    return playlist->totaltime;
}

void
pl_set_selected (playItem_t *it, int sel) {
    LOCK;
    it->selected = sel;
    UNLOCK;
}

int
pl_is_selected (playItem_t *it) {
    return it->selected;
}

playItem_t *
pl_get_first (int iter) {
    playItem_t *p = playlist->head[iter];
    if (p) {
        pl_item_ref (p);
    }
    return p;
}

playItem_t *
pl_get_last (int iter) {
    playItem_t *p = playlist->tail[iter];
    if (p) {
        pl_item_ref (p);
    }
    return p;
}

playItem_t *
pl_get_next (playItem_t *it, int iter) {
    playItem_t *next = it ? it->next[iter] : NULL;
    if (next) {
        pl_item_ref (next);
    }
    return next;
}

playItem_t *
pl_get_prev (playItem_t *it, int iter) {
    playItem_t *prev = it ? it->prev[iter] : NULL;
    if (prev) {
        pl_item_ref (prev);
    }
    return prev;
}

int
pl_get_cursor (int iter) {
    if (!playlist) {
        return -1;
    }
    return playlist->current_row[iter];
}

void
pl_set_cursor (int iter, int cursor) {
    PLT_LOCK;
    playlist->current_row[iter] = cursor;
    PLT_UNLOCK;
}

// this function must move items in playlist
// list of items is indexes[count]
// drop_before is insertion point
void
pl_move_items (int iter, playItem_t *drop_before, uint32_t *indexes, int count) {
    // unlink items from playlist, and link together
    GLOBAL_LOCK;
    playItem_t *head = NULL;
    playItem_t *tail = NULL;
    int processed = 0;
    int idx = 0;
    playItem_t *next = NULL;
    for (playItem_t *it = playlist->head[iter]; it && processed < count; it = next, idx++) {
        next = it->next[iter];
        if (idx == indexes[processed]) {
            if (it->prev[iter]) {
                it->prev[iter]->next[iter] = it->next[iter];
            }
            else {
                playlist->head[iter] = it->next[iter];
            }
            if (it->next[iter]) {
                it->next[iter]->prev[iter] = it->prev[iter];
            }
            else {
                playlist->tail[iter] = it->prev[iter];
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
        drop_after = playlist->tail[iter];
    }
    // insert in between
    head->prev[iter] = drop_after;
    if (drop_after) {
        drop_after->next[iter] = head;
    }
    else {
        playlist->head[iter] = head;
    }
    tail->next[iter] = drop_before;
    if (drop_before) {
        drop_before->prev[iter] = tail;
    }
    else {
        playlist->tail[iter] = tail;
    }
    GLOBAL_UNLOCK;
}

void
pl_copy_items (int iter, int plt_from, playItem_t *before, uint32_t *indices, int cnt) {
    pl_lock ();
    playlist_t *from = playlists_head;
    playlist_t *to = plt_get_curr_ptr ();

    int i;
    for (i = 0; i < plt_from; i++) {
        from = from->next;
    }

    if (!from || !to) {
        pl_unlock ();
        return;
    }

    for (i = 0; i < cnt; i++) {
        playItem_t *it = from->head[iter];
        int idx = 0;
        while (it && idx < indices[i]) {
            it = it->next[iter];
            idx++;
        }
        if (!it) {
            trace ("pl_copy_items: warning: item %d not found in source playlist\n", indices[i]);
            continue;
        }
        playItem_t *it_new = pl_item_alloc ();
        pl_item_copy (it_new, it);

        playItem_t *after = before ? before->prev[iter] : to->tail[iter];
        pl_insert_item (after, it_new);
        pl_item_unref (it_new);

    }
    pl_unlock ();
}

void
pl_search_reset (void) {
    GLOBAL_LOCK;
    while (playlist->head[PL_SEARCH]) {
        playItem_t *next = playlist->head[PL_SEARCH]->next[PL_SEARCH];
        playlist->head[PL_SEARCH]->selected = 0;
        playlist->head[PL_SEARCH]->next[PL_SEARCH] = NULL;
        playlist->head[PL_SEARCH]->prev[PL_SEARCH] = NULL;
        playlist->head[PL_SEARCH] = next;
    }
    playlist->tail[PL_SEARCH] = NULL;
    playlist->count[PL_SEARCH] = 0;
    GLOBAL_UNLOCK;
}

void
pl_search_process (const char *text) {
    pl_search_reset ();
    GLOBAL_LOCK;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        it->selected = 0;
        if (*text) {
            for (DB_metaInfo_t *m = it->meta; m; m = m->next) {
                if (strcasecmp (m->key, "cuesheet") && utfcasestr (m->value, text)) {
                    //fprintf (stderr, "%s -> %s match (%s.%s)\n", text, m->value, it->fname, m->key);
                    // add to list
                    it->next[PL_SEARCH] = NULL;
                    if (playlist->tail[PL_SEARCH]) {
                        playlist->tail[PL_SEARCH]->next[PL_SEARCH] = it;
                        playlist->tail[PL_SEARCH] = it;
                    }
                    else {
                        playlist->head[PL_SEARCH] = playlist->tail[PL_SEARCH] = it;
                    }
                    it->selected = 1;
                    playlist->count[PL_SEARCH]++;
                    break;
                }
            }
        }
    }
    GLOBAL_UNLOCK;
}

int
pl_playqueue_push (playItem_t *it) {
    if (playqueue_count == PLAYQUEUE_SIZE) {
        trace ("playqueue is full\n");
        return -1;
    }
    LOCK;
    playqueue[playqueue_count++] = it;
    UNLOCK;
    return 0;
}

void
pl_playqueue_clear (void) {
    LOCK;
    playqueue_count = 0;
    UNLOCK;
}

void
pl_playqueue_pop (void) {
    if (!playqueue_count) {
        return;
    }
    LOCK;
    if (playqueue_count == 1) {
        playqueue_count = 0;
        UNLOCK;
        return;
    }
    memmove (&playqueue[0], &playqueue[1], (playqueue_count-1) * sizeof (playItem_t*));
    playqueue_count--;
    UNLOCK;
}

void
pl_playqueue_remove (playItem_t *it) {
    LOCK;
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
    UNLOCK;
}

int
pl_playqueue_test (playItem_t *it) {
    LOCK;
    for (int i = 0; i < playqueue_count; i++) {
        if (playqueue[i] == it) {
            UNLOCK;
            return i;
        }
    }
    UNLOCK;
    return -1;
}

playItem_t *
pl_playqueue_getnext (void) {
    LOCK;
    if (playqueue_count > 0) {
        playItem_t *val = playqueue[0];
        pl_item_ref (val);
        UNLOCK;
        return val;
    }
    UNLOCK;
    return NULL;
}

int
pl_playqueue_getcount (void) {
    return playqueue_count;
}

void
pl_items_copy_junk (playItem_t *from, playItem_t *first, playItem_t *last) {
    LOCK;
    const char *metainfo[] = {
        "year", "genre", "copyright", "vendor", "comment", "tags", "numtracks", "band", "performer", "composer", "disc", NULL
    };
    for (int m = 0; metainfo[m]; m++) {
        const char *data = pl_find_meta (from, metainfo[m]);
        if (data) {
            playItem_t *i;
            for (i = first; ; i = i->next[PL_MAIN]) {
                pl_add_meta (i, metainfo[m], data);
                if (i == last) {
                    break;
                }
            }
        }
    }
    UNLOCK;
}

uint32_t
pl_get_item_flags (playItem_t *it) {
    LOCK;
    uint32_t flags = it->_flags;
    UNLOCK;
    return flags;
}

void
pl_set_item_flags (playItem_t *it, uint32_t flags) {
    LOCK;
    it->_flags = flags;
    UNLOCK;
}
