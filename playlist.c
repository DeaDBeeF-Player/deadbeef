/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
#include "playlist.h"
#include "codec.h"
#include "streamer.h"
#include "messagepump.h"
#include "messages.h"
#include "playback.h"

#include "cwav.h"
#include "cvorbis.h"
#include "cdumb.h"
#include "cmp3.h"
#include "cgme.h"
#include "cflac.h"
#include "csid.h"

codec_t *codecs[] = {
    &cdumb, &cvorbis, &cflac, &cgme, &cmp3, &csid, NULL
};

#define SKIP_BLANK_CUE_TRACKS 1

playItem_t *playlist_head[PL_MAX_ITERATORS];
playItem_t *playlist_tail[PL_MAX_ITERATORS];
playItem_t playlist_current;
playItem_t *playlist_current_ptr;
int pl_count = 0;
static int pl_order = 0; // 0 = linear, 1 = shuffle, 2 = random
static int pl_loop_mode = 0; // 0 = loop, 1 = don't loop, 2 = loop single

void
pl_free (void) {
    while (playlist_head[PL_MAIN]) {
        pl_remove (playlist_head[PL_MAIN]);
    }
}

static char *
pl_cue_skipspaces (char *p) {
    while (*p && *p <= ' ') {
        p++;
    }
    return p;
}

static void
pl_get_qvalue_from_cue (char *p, char *out) {
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
    while (*p && *p != '"') {
        *out++ = *p++;
    }
    *out = 0;
}

static void
pl_get_value_from_cue (char *p, char *out) {
    while (*p >= ' ') {
        *out++ = *p++;
    }
    *out = 0;
}

static float
pl_cue_parse_time (const char *p) {
    char tmp[3] = {0};
    const char *next = p;
    int s;
    while (*next && *next != ':') {
        next++;
    }
    if ((next - p) != 2) {
        return -1;
    }
    strncpy (tmp, p, 2);
    tmp[next-p] = 0;
    float mins = atoi (tmp);
    next++;
    p = next;
    while (*next && *next != ':') {
        next++;
    }
    if ((next - p) != 2) {
        return -1;
    }
    strncpy (tmp, p, 2);
    float sec = atoi (tmp);
    next++;
    p = next;
    while (*next && *next != ':') {
        next++;
    }
    if ((next - p) != 2) {
        return -1;
    }
    strncpy (tmp, p, 2);
    float frm = atoi (tmp);
    return mins * 60 + sec;
}

playItem_t *
pl_insert_cue (playItem_t *after, const char *fname, codec_t *codec, const char *ftype) {
    int len = strlen (fname);
    char cuename[len+5];
    strcpy (cuename, fname);
    strcpy (cuename+len, ".cue");
    FILE *fp = fopen (cuename, "rt");
    if (!fp) {
        char *ptr = cuename + len;
        while (ptr >= cuename && *ptr != '.') {
            ptr--;
        }
        strcpy (ptr+1, "cue");
        fp = fopen (cuename, "rt");
        if (!fp) {
            return NULL;
        }
    }
    char performer[1024];
    char albumtitle[1024];
    char file[1024];
    char track[1024];
    char title[1024];
    char start[1024];
    playItem_t *prev = NULL;
    for (;;) {
        char str[1024];
        if (!fgets (str, 1024, fp)) {
            break; // eof
        }
        char *p = pl_cue_skipspaces (str);
        if (!strncmp (p, "PERFORMER ", 10)) {
            pl_get_qvalue_from_cue (p + 10, performer);
//            printf ("got performer: %s\n", performer);
        }
        else if (!strncmp (p, "TITLE ", 6)) {
            if (str[0] > ' ') {
                pl_get_qvalue_from_cue (p + 6, albumtitle);
//                printf ("got albumtitle: %s\n", albumtitle);
            }
            else {
                pl_get_qvalue_from_cue (p + 6, title);
//                printf ("got title: %s\n", title);
            }
        }
        else if (!strncmp (p, "FILE ", 5)) {
            pl_get_qvalue_from_cue (p + 5, file);
//            printf ("got filename: %s\n", file);
            // copy directory name
            char fname[1024];
            int len = strlen (cuename);
            memcpy (fname, cuename, len+1);
            char *p = fname + len;
            while (*p != '/') {
                p--;
                len--;
            }
            p++;
            len++;
            // add file name
            int flen = strlen (file);
            // ensure fullname fills in buffer
            if (flen + len >= 1024) {
//                printf ("cue file name is too long");
                return NULL;
            }
            strcpy (p, file);
            // copy full name in place of relative name
            strcpy (file, fname);
//            printf ("ended up as: %s\n", file);
        }
        else if (!strncmp (p, "TRACK ", 6)) {
            pl_get_value_from_cue (p + 6, track);
//            printf ("got track: %s\n", track);
        }
//        else if (!strncmp (p, "PERFORMER ", 10)) {
//            pl_get_qvalue_from_cue (p + 10, performer);
//        }
        else if (!strncmp (p, "INDEX 00 ", 9) || !strncmp (p, "INDEX 01 ", 9)) {
            if (!track[0]) {
                continue;
            }
#if SKIP_BLANK_CUE_TRACKS
            if (!title[0])
                continue;
#endif
            pl_get_value_from_cue (p + 9, start);
//            printf ("got index0: %s\n", start);
            char *p = track;
            while (*p && isdigit (*p)) {
                p++;
            }
            *p = 0;
            // check that indexes have valid timestamps
            float tstart = pl_cue_parse_time (start);
            if (tstart < 0) {
//                printf ("cue file %s has bad timestamp(s)\n", cuename);
                continue;
            }
            if (prev) {
                prev->timeend = tstart;
                prev->duration = prev->timeend - prev->timestart;
//                printf ("end time for prev track (%x): %f\n", prev, tstart);
            }
            // add this track
            char str[1024];
            snprintf (str, 1024, "%d. %s - %s", atoi (track), performer, title[0] ? title : "?", start, tstart);
//            printf ("adding %s\n", str);
            playItem_t *it = malloc (sizeof (playItem_t));
            memset (it, 0, sizeof (playItem_t));
            it->codec = codec;
            it->fname = strdup (file);
            it->tracknum = atoi (track);
            it->timestart = tstart;
            it->timeend = -1; // will be filled by next read, or by codec
            it->filetype = ftype;
            after = pl_insert_item (after, it);
            pl_add_meta (it, "artist", performer);
            pl_add_meta (it, "album", albumtitle);
            pl_add_meta (it, "track", track);
            pl_add_meta (it, "title", title);
            prev = it;
            track[0] = 0;
        }
        else {
//            printf ("got unknown line:\n%s\n", p);
        }
    }
    fclose (fp);
    return after;
}

playItem_t *
pl_insert_file (playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data) {
    if (!fname) {
        return NULL;
    }
    // detect codec
    codec_t *codec = NULL;
    const char *eol = fname + strlen (fname) - 1;
    while (eol > fname && *eol != '.') {
        eol--;
    }
    eol++;

    // match by codec
    for (int i = 0; codecs[i]; i++) {
        if (codecs[i]->getexts && codecs[i]->insert) {
            const char **exts = codecs[i]->getexts ();
            if (exts) {
                for (int e = 0; exts[e]; e++) {
                    if (!strcasecmp (exts[e], eol)) {
                        playItem_t *inserted = NULL;
                        if ((inserted = codecs[i]->insert (after, fname)) != NULL) {
                            if (cb) {
                                if (cb (inserted, user_data) < 0) {
                                    *pabort = 1;
                                }
                            }
                            return inserted;
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

playItem_t *
pl_insert_dir (playItem_t *after, const char *dirname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data) {
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
                char fullname[1024];
                strcpy (fullname, dirname);
                strncat (fullname, "/", 1024);
                strncat (fullname, namelist[i]->d_name, 1024);
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
// {{{ original pl_add_dir code
#if 0
    struct stat buf;
    lstat (dirname, &buf);
    if (S_ISLNK(buf.st_mode)) {
        return -1;
    }
    struct dirent **namelist = NULL;
    int n;

    n = scandir (dirname, &namelist, NULL, alphasort);
    if (n < 0)
    {
        if (namelist)
            free (namelist);
        return -1;	// not a dir or no read access
    }
    else
    {
        int i;
        for (i = 0; i < n; i++)
        {
            // no hidden files
            if (namelist[i]->d_name[0] != '.')
            {
                char fullname[1024];
                strcpy (fullname, dirname);
                strncat (fullname, "/", 1024);
                strncat (fullname, namelist[i]->d_name, 1024);
                if (pl_add_dir (fullname)) {
                    pl_add_file (fullname);
                }
            }
            free (namelist[i]);
        }
        free (namelist);
    }
    return 0;
#endif
// }}}
}

int
pl_remove (playItem_t *it) {
    if (!it)
        return -1;
    pl_count--;
    if (playlist_current_ptr == it) {
        playlist_current_ptr = NULL;
    }

    // remove from shuffle list
    {
        playItem_t *prev = it->prev[PL_SHUFFLE];
        playItem_t *next = it->next[PL_SHUFFLE];
        if (prev) {
            prev->next[PL_SHUFFLE] = next;
        }
        else {
            playlist_head[PL_SHUFFLE] = next;
        }
        if (next) {
            next->prev[PL_SHUFFLE] = prev;
        }
        else {
            playlist_tail[PL_SHUFFLE] = prev;
        }
    }

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
pl_get_for_idx (int idx) {
    playItem_t *it = playlist_head[PL_MAIN];
    while (idx--) {
        if (!it)
            return NULL;
        it = it->next[PL_MAIN];
    }
    return it;
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
    pl_count++;
    int idx = (float)rand ()/RAND_MAX * pl_count;

#if 0
    // shuffle
    playItem_t *prev = NULL;
    if (!playlist_head[PL_SHUFFLE]) {
        playlist_head[PL_SHUFFLE] = playlist_tail[PL_SHUFFLE] = it;
    }
    else if (idx == pl_count-1) {
        // append to end
        assert (playlist_tail[PL_SHUFFLE]);
        playlist_tail[PL_SHUFFLE]->next[PL_SHUFFLE] = it;
        playlist_tail[PL_SHUFFLE] = it;
    }
    else {
        for (playItem_t *sh = playlist_head[PL_SHUFFLE]; sh; sh = sh->next[PL_SHUFFLE], idx--) {
            if (!idx) {
                if (prev) {
                    prev->next[PL_SHUFFLE] = it;
                }
                else {
                    playlist_head[PL_SHUFFLE] = it;
                }
                it->next[PL_SHUFFLE] = sh;
                it->prev[PL_SHUFFLE] = sh->prev[PL_SHUFFLE];
                sh->prev[PL_SHUFFLE] = it;
                break;
            }
            prev = sh;
        }
    }
#endif

    return it;
}

void
pl_item_copy (playItem_t *out, playItem_t *it) {
    out->fname = strdup (it->fname);
    out->codec = it->codec;
    out->tracknum = it->tracknum;
    out->timestart = it->timestart;
    out->timeend = it->timeend;
    out->duration = it->duration;
    out->filetype = it->filetype;
    out->next[PL_MAIN] = it->next[PL_MAIN];
    out->prev[PL_MAIN] = it->prev[PL_MAIN];
    out->next[PL_SHUFFLE] = it->next[PL_SHUFFLE];
    out->prev[PL_SHUFFLE] = it->prev[PL_SHUFFLE];
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
pl_set_current (playItem_t *it) {
    int ret = 0;
    int from = pl_get_idx_of (playlist_current_ptr);
    int to = pl_get_idx_of (it);
#if 0
    // this produces some kind of bug in the beginning of track
    if (it == playlist_current_ptr) {
        if (it && it->codec) {
            codec_lock ();
            ret = playlist_current_ptr->codec->seek (0);
            codec_unlock ();
        }
        return ret;
    }
#endif
    codec_lock ();
    if (playlist_current_ptr && playlist_current_ptr->codec) {
        playlist_current_ptr->codec->free ();
    }
    pl_item_free (&playlist_current);
    playlist_current_ptr = it;
    if (it && it->codec) {
        // don't do anything on fail, streamer will take care
        ret = it->codec->init (it);
        if (ret < 0) {
//            pl_item_free (&playlist_current);
//            playlist_current_ptr = NULL;
//            return ret;
////            it->codec->info.samplesPerSecond = -1;
        }
    }
    if (playlist_current_ptr) {
        streamer_reset (0);
    }
    if (it) {
        pl_item_copy (&playlist_current, it);
    }
    codec_unlock ();
    messagepump_push (M_SONGCHANGED, 0, from, to);
    return ret;
}

int
pl_prevsong (void) {
    if (pl_order == 1) { // shuffle
        if (!playlist_current_ptr) {
            return pl_nextsong (1);
        }
        else {
            playItem_t *it = playlist_current_ptr->prev[PL_SHUFFLE];
            if (!it) {
                if (pl_loop_mode == 0) {
                    it = playlist_tail[PL_SHUFFLE];
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
    else if (pl_order == 0) { // linear
        playItem_t *it = NULL;
        if (playlist_current_ptr) {
            it = playlist_current_ptr->prev[PL_MAIN];
        }
        if (!it) {
            if (pl_loop_mode == 0) {
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
    else if (pl_order == 2) { // random
        pl_randomsong ();
    }
    return -1;
}

int
pl_nextsong (int reason) {
    if (pl_order == 1) { // shuffle
        if (!playlist_current_ptr) {
            playItem_t *it = playlist_head[PL_SHUFFLE];
            if (!it) {
                return -1;
            }
            int r = pl_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            return 0;
        }
        else {
            if (reason == 0 && pl_loop_mode == 2) {
                int r = pl_get_idx_of (playlist_current_ptr);
                streamer_set_nextsong (r, 1);
                return 0;
            }
            playItem_t *it = playlist_current_ptr->next[PL_SHUFFLE];
            if (!it) {
                if (pl_loop_mode == 0) { // loop
                    it = playlist_head[PL_SHUFFLE];
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
    }
    else if (pl_order == 0) { // linear
        playItem_t *it = NULL;
        if (playlist_current_ptr) {
            if (reason == 0 && pl_loop_mode == 2) {
                int r = pl_get_idx_of (playlist_current_ptr);
                streamer_set_nextsong (r, 1);
                return 0;
            }
            it = playlist_current_ptr->next[PL_MAIN];
        }
        if (!it) {
            if (pl_loop_mode == 0) {
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
    else if (pl_order == 2) { // random
        if (reason == 0 && pl_loop_mode == 2 && playlist_current_ptr) {
            int r = pl_get_idx_of (playlist_current_ptr);
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
pl_start_current (void) {
    codec_lock ();
    playItem_t *it = playlist_current_ptr;
    if (it && it->codec) {
        // don't do anything on fail, streamer will take care
        it->codec->free ();
        it->codec->init (it);
    }
    codec_unlock ();
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
            value = "?";
        }
    }
    m = malloc (sizeof (metaInfo_t));
    m->key = key;
    m->value = strdup (value);
//    strncpy (m->value, value, META_FIELD_SIZE-1);
//    m->value[META_FIELD_SIZE-1] = 0;
    m->next = it->meta;
    it->meta = m;
}

void
pl_format_item_display_name (playItem_t *it, char *str, int len) {
    // artist - title
    const char *track = pl_find_meta (it, "track");
    const char *artist = pl_find_meta (it, "artist");
    const char *album = pl_find_meta (it, "album");
    const char *title = pl_find_meta (it, "title");
    if (*track == '?' && *album == '?' && *artist != '?' && *title != '?') {
        snprintf (str, len, "%s - %s", artist, title);
    }
    else if (*artist != '?' && *track != '?' && *title != '?') {
        snprintf (str, len, "%s. %s - %s", track, artist, title);
    }
    else if (*artist == '?' && *track != '?' && *album != '?') {
        snprintf (str, len, "%s. %s", track, album);
    }
    else if (*artist != '?' && *track != '?' && *album != '?') {
        snprintf (str, len, "%s. %s - %s", track, artist, album);
    }
    else if (*artist != '?' && *title != '?') {
        snprintf (str, len, "%s - %s", artist, title);
    }
    else if (*artist != '?') {
        snprintf (str, len, "%s", artist);
    }
    else if (*title != '?') {
        snprintf (str, len, "%s", title);
    }
    else {
        // cut filename without path and extension
        char *pext = it->fname + strlen (it->fname) - 1;
        while (pext >= it->fname && *pext != '.') {
            pext--;
        }
        char *pname = pext;
        while (pname >= it->fname && *pname != '/') {
            pname--;
        }
        if (*pname == '/') {
            pname++;
        }
        strncpy (str, pname, pext-pname);
        str[pext-pname] = 0;
    }
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
    return "?";
}

void
pl_delete_selected (void) {
    playItem_t *next = NULL;
    for (playItem_t *it = playlist_head[PL_MAIN]; it; it = next) {
        next = it->next[PL_MAIN];
        if (it->selected) {
            pl_remove (it);
        }
    }
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

void
pl_set_order (int order) {
    pl_order = order;
}

void
pl_set_loop_mode (int mode) {
    pl_loop_mode = mode;
}

int
pl_save (const char *fname) {
    const char magic[] = "DBPL";
    uint8_t majorver = 1;
    uint8_t minorver = 0;
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
        ll = strlen (it->codec->id);
        if (fwrite (&ll, 1, 1, fp) != 1) {
            goto save_fail;
        }
        if (fwrite (it->codec->id, 1, ll, fp) != ll) {
            goto save_fail;
        }
        l = it->tracknum;
        if (fwrite (&l, 1, 2, fp) != 2) {
            goto save_fail;
        }
        if (fwrite (&it->timestart, 1, 4, fp) != 4) {
            goto save_fail;
        }
        if (fwrite (&it->timeend, 1, 4, fp) != 4) {
            goto save_fail;
        }
        if (fwrite (&it->duration, 1, 4, fp) != 4) {
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
    uint8_t majorver = 1;
    uint8_t minorver = 0;
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
    if (majorver != 1) {
        goto load_fail;
    }
    if (fread (&minorver, 1, 1, fp) != 1) {
        goto load_fail;
    }
    if (minorver != 0) {
        goto load_fail;
    }
    uint32_t cnt;
    if (fread (&cnt, 1, 4, fp) != 4) {
        goto load_fail;
    }
    playItem_t *it = NULL;
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
        // codec
        uint8_t ll;
        if (fread (&ll, 1, 1, fp) != 1) {
            goto load_fail;
        }
        if (ll >= 20) {
            goto load_fail;
        }
        char codec[20];
        if (fread (codec, 1, ll, fp) != ll) {
            goto load_fail;
        }
        codec[ll] = 0;
        for (int c = 0; codecs[c]; c++) {
            if (!strcmp (codec, codecs[c]->id)) {
                it->codec = codecs[c];
            }
        }
        if (!it->codec) {
            goto load_fail;
        }
        // tracknum
        if (fread (&l, 1, 2, fp) != 2) {
            goto load_fail;
        }
        it->tracknum = l;
        // timestart
        if (fread (&it->timestart, 1, 4, fp) != 4) {
            goto load_fail;
        }
        // timeend
        if (fread (&it->timeend, 1, 4, fp) != 4) {
            goto load_fail;
        }
        // duration
        if (fread (&it->duration, 1, 4, fp) != 4) {
            goto load_fail;
        }
        // get const filetype string from codec
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
            if (it->codec && it->codec->filetypes) {
                for (int i = 0; it->codec->filetypes[i]; i++) {
                    if (!strcasecmp (it->codec->filetypes[i], ftype)) {
                        it->filetype = it->codec->filetypes[i];
                        break;
                    }
                }
            }
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
