#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "playlist.h"
#include "codec.h"
#include "cwav.h"
#include "cvorbis.h"
#include "cdumb.h"
#include "cmp3.h"
#include "cgme.h"
#include "cflac.h"
#include "csid.h"
#include "streamer.h"
#include "messagepump.h"
#include "messages.h"

#define SKIP_BLANK_CUE_TRACKS 1

playItem_t *playlist_head;
playItem_t *playlist_tail;
playItem_t playlist_current;
playItem_t *playlist_current_ptr;
int ps_count = 0;

void
ps_free (void) {
    while (playlist_head) {
        ps_remove (playlist_head);
    }
}

static char *
ps_cue_skipspaces (char *p) {
    while (*p && *p <= ' ') {
        p++;
    }
    return p;
}

static void
ps_get_qvalue_from_cue (char *p, char *out) {
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
    p = ps_cue_skipspaces (p);
    while (*p && *p != '"') {
        *out++ = *p++;
    }
    *out = 0;
}

static void
ps_get_value_from_cue (char *p, char *out) {
    while (*p >= ' ') {
        *out++ = *p++;
    }
    *out = 0;
}

static float
ps_cue_parse_time (const char *p) {
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
ps_insert_cue (playItem_t *after, const char *cuename) {
    FILE *fp = fopen (cuename, "rt");
    if (!fp) {
        return NULL;
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
        char *p = ps_cue_skipspaces (str);
        if (!strncmp (p, "PERFORMER ", 10)) {
            ps_get_qvalue_from_cue (p + 10, performer);
//            printf ("got performer: %s\n", performer);
        }
        else if (!strncmp (p, "TITLE ", 6)) {
            if (str[0] > ' ') {
                ps_get_qvalue_from_cue (p + 6, albumtitle);
//                printf ("got albumtitle: %s\n", albumtitle);
            }
            else {
                ps_get_qvalue_from_cue (p + 6, title);
//                printf ("got title: %s\n", title);
            }
        }
        else if (!strncmp (p, "FILE ", 5)) {
            ps_get_qvalue_from_cue (p + 5, file);
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
            ps_get_value_from_cue (p + 6, track);
//            printf ("got track: %s\n", track);
        }
//        else if (!strncmp (p, "PERFORMER ", 10)) {
//            ps_get_qvalue_from_cue (p + 10, performer);
//        }
        else if (!strncmp (p, "INDEX 00 ", 9) || !strncmp (p, "INDEX 01 ", 9)) {
            if (!track[0]) {
                continue;
            }
#if SKIP_BLANK_CUE_TRACKS
            if (!title[0])
                continue;
#endif
            ps_get_value_from_cue (p + 9, start);
//            printf ("got index0: %s\n", start);
            char *p = track;
            while (*p && isdigit (*p)) {
                p++;
            }
            *p = 0;
            // check that indexes have valid timestamps
            float tstart = ps_cue_parse_time (start);
            if (tstart < 0) {
//                printf ("cue file %s has bad timestamp(s)\n", cuename);
                continue;
            }
            if (prev) {
                prev->timeend = tstart;
//                printf ("end time for prev track (%x): %f\n", prev, tstart);
            }
            // add this track
            char str[1024];
            snprintf (str, 1024, "%d. %s - %s", atoi (track), performer, title[0] ? title : "?", start, tstart);
//            printf ("adding %s\n", str);
            playItem_t *it = malloc (sizeof (playItem_t));
            memset (it, 0, sizeof (playItem_t));
            it->codec = &cflac;
            it->fname = strdup (file);
            it->tracknum = atoi (track);
            it->timestart = tstart;
            it->timeend = -1; // will be filled by next read, or by codec
            after = ps_insert_item (after, it);
//            ps_append_item (it);
//            printf ("added item %x\n", it);
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

int
ps_add_cue (const char *cuename) {
    playItem_t *it = ps_insert_cue (playlist_tail, cuename);
    if (!it) {
        return -1;
    }
    return 0;
// {{{ hide old cue code
#if 0
    FILE *fp = fopen (cuename, "rt");
    if (!fp) {
        return -1;
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
        char *p = ps_cue_skipspaces (str);
        if (!strncmp (p, "PERFORMER ", 10)) {
            ps_get_qvalue_from_cue (p + 10, performer);
//            printf ("got performer: %s\n", performer);
        }
        else if (!strncmp (p, "TITLE ", 6)) {
            if (str[0] > ' ') {
                ps_get_qvalue_from_cue (p + 6, albumtitle);
//                printf ("got albumtitle: %s\n", albumtitle);
            }
            else {
                ps_get_qvalue_from_cue (p + 6, title);
//                printf ("got title: %s\n", title);
            }
        }
        else if (!strncmp (p, "FILE ", 5)) {
            ps_get_qvalue_from_cue (p + 5, file);
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
                return -1;
            }
            strcpy (p, file);
            // copy full name in place of relative name
            strcpy (file, fname);
//            printf ("ended up as: %s\n", file);
        }
        else if (!strncmp (p, "TRACK ", 6)) {
            ps_get_value_from_cue (p + 6, track);
//            printf ("got track: %s\n", track);
        }
//        else if (!strncmp (p, "PERFORMER ", 10)) {
//            ps_get_qvalue_from_cue (p + 10, performer);
//        }
        else if (!strncmp (p, "INDEX 00 ", 9) || !strncmp (p, "INDEX 01 ", 9)) {
            if (!track[0]) {
                continue;
            }
#if SKIP_BLANK_CUE_TRACKS
            if (!title[0])
                continue;
#endif
            ps_get_value_from_cue (p + 9, start);
//            printf ("got index0: %s\n", start);
            char *p = track;
            while (*p && isdigit (*p)) {
                p++;
            }
            *p = 0;
            // check that indexes have valid timestamps
            float tstart = ps_cue_parse_time (start);
            if (tstart < 0) {
//                printf ("cue file %s has bad timestamp(s)\n", cuename);
                continue;
            }
            if (prev) {
                prev->timeend = tstart;
//                printf ("end time for prev track (%x): %f\n", prev, tstart);
            }
            // add this track
            char str[1024];
            snprintf (str, 1024, "%d. %s - %s", atoi (track), performer, title[0] ? title : "?", start, tstart);
//            printf ("adding %s\n", str);
            playItem_t *it = malloc (sizeof (playItem_t));
            memset (it, 0, sizeof (playItem_t));
            it->codec = &cflac;
            it->fname = strdup (file);
            it->tracknum = atoi (track);
            it->timestart = tstart;
            it->timeend = -1; // will be filled by next read, or by codec
            ps_append_item (it);
//            printf ("added item %x\n", it);
            prev = it;
            track[0] = 0;
        }
        else {
//            printf ("got unknown line:\n%s\n", p);
        }
    }
    fclose (fp);
    return 0;
#endif
// }}}
}

playItem_t *
ps_insert_file (playItem_t *after, const char *fname) {
}

playItem_t *
ps_insert_dir (playItem_t *after, const char *dirname) {
}

int
ps_add_file (const char *fname) {
    if (!fname) {
        return -1;
    }
    // detect codec
    codec_t *codec = NULL;
    const char *eol = fname + strlen (fname) - 1;
    while (eol > fname && *eol != '.') {
        eol--;
    }
    eol++;

    // match by codec
    codec_t *codecs[] = {
        &cdumb, &cvorbis, &cflac, &cgme, &cmp3, &csid, NULL
    };
    playItem_t *after = playlist_tail;
    for (int i = 0; codecs[i]; i++) {
        if (codecs[i]->getexts && codecs[i]->insert) {
            const char **exts = codecs[i]->getexts ();
            if (exts) {
                for (int e = 0; exts[e]; e++) {
                    if (!strcasecmp (exts[e], eol)) {
                        playItem_t *inserted = NULL;
                        if ((inserted = codecs[i]->insert (after, fname)) != NULL) {
                            return 0;
                        }
                    }
                }
            }
        }
    }

    return -1;
}

int
ps_add_dir (const char *dirname) {
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
                if (ps_add_dir (fullname)) {
                    ps_add_file (fullname);
                }
            }
            free (namelist[i]);
        }
        free (namelist);
    }
    return 0;
}

int
ps_remove (playItem_t *it) {
    if (!it)
        return -1;
    ps_count--;
    if (it->prev) {
        it->prev->next = it->next;
    }
    else {
        playlist_head = it->next;
    }
    if (it->next) {
        it->next->prev = it->prev;
    }
    else {
        playlist_tail = it->prev;
    }
    ps_item_free (it);
    free (it);
    return 0;
}

int
ps_getcount (void) {
    return ps_count;
}

int
ps_getselcount (void) {
    // FIXME: slow!
    int cnt = 0;
    for (playItem_t *it = playlist_head; it; it = it->next) {
        if (it->selected) {
            cnt++;
        }
    }
    return cnt;
}

playItem_t *
ps_get_for_idx (int idx) {
    playItem_t *it = playlist_head;
    while (idx--) {
        if (!it)
            return NULL;
        it = it->next;
    }
    return it;
}

int
ps_get_idx_of (playItem_t *it) {
    playItem_t *c = playlist_head;
    int idx = 0;
    while (c && c != it) {
        c = c->next;
        idx++;
    }
    if (!c) {
        return -1;
    }
    return idx;
}

int
ps_append_item (playItem_t *it) {
    if (!playlist_tail) {
        playlist_tail = playlist_head = it;
    }
    else {
        playlist_tail->next = it;
        it->prev = playlist_tail;
        playlist_tail = it;
    }
    ps_count++;
}

playItem_t *
ps_insert_item (playItem_t *after, playItem_t *it) {
    if (!after) {
        it->next = playlist_head;
        it->prev = NULL;
        if (playlist_head) {
            playlist_head->prev = it;
        }
        else {
            playlist_tail = it;
        }
        playlist_head = it;
    }
    else {
        it->prev= after;
        it->next = after->next;
        if (after->next) {
            after->next->prev = it;
        }
        after->next = it;
        if (after == playlist_tail) {
            playlist_tail = it;
        }
    }
    ps_count++;
    return it;
}

void
ps_item_copy (playItem_t *out, playItem_t *it) {
    out->fname = strdup (it->fname);
    out->codec = it->codec;
    out->tracknum = it->tracknum;
    out->timestart = it->timestart;
    out->timeend = it->timeend;
    out->next = it->next;
    out->prev = it->prev;
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
ps_item_free (playItem_t *it) {
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
ps_set_current (playItem_t *it) {
    int ret = 0;
    int from = ps_get_idx_of (playlist_current_ptr);
    int to = ps_get_idx_of (it);
    if (it == playlist_current_ptr) {
        if (it && it->codec) {
            codec_lock ();
            ret = playlist_current_ptr->codec->seek (0);
            codec_unlock ();
        }
        return ret;
    }
    codec_lock ();
    if (playlist_current_ptr && playlist_current_ptr->codec) {
        playlist_current_ptr->codec->free ();
    }
    ps_item_free (&playlist_current);
    if (it) {
        ps_item_copy (&playlist_current, it);
    }
    playlist_current_ptr = it;
    if (it && it->codec) {
        // don't do anything on fail, streamer will take care
        ret = it->codec->init (it->fname, it->tracknum, it->timestart, it->timeend);
        if (ret < 0) {
            it->codec->info.samplesPerSecond = -1;
        }
    }
    if (playlist_current_ptr) {
        streamer_reset ();
    }
    codec_unlock ();
    messagepump_push (M_SONGCHANGED, 0, from, to);
    return ret;
}

int
ps_nextsong (void) {
    if (playlist_current_ptr && playlist_current_ptr->next) {
        return ps_set_current (playlist_current_ptr->next);
    }
    if (playlist_head) {
        return ps_set_current (playlist_head);
    }
    ps_set_current (NULL);
    return -1;
}

playItem_t *
ps_getnext (void) {
    if (playlist_current_ptr && playlist_current_ptr->next) {
        return playlist_current_ptr->next;
    }
    if (playlist_head) {
        return playlist_head;
    }
    return NULL;
}

void
ps_start_current (void) {
    codec_lock ();
    playItem_t *it = playlist_current_ptr;
    if (it && it->codec) {
        // don't do anything on fail, streamer will take care
        it->codec->free ();
        it->codec->init (it->fname, it->tracknum, it->timestart, it->timeend);
    }
    codec_unlock ();
}

void
ps_add_meta (playItem_t *it, const char *key, const char *value) {
    if (!value || !*value) {
        value = "?";
    }
    metaInfo_t *m = malloc (sizeof (metaInfo_t));
    m->key = key;
    m->value = strdup (value);
//    strncpy (m->value, value, META_FIELD_SIZE-1);
//    m->value[META_FIELD_SIZE-1] = 0;
    m->next = it->meta;
    it->meta = m;
}

void
ps_format_item_display_name (playItem_t *it, char *str, int len) {
    // artist - title
    const char *track = ps_find_meta (it, "track");
    const char *artist = ps_find_meta (it, "artist");
    const char *album = ps_find_meta (it, "album");
    const char *title = ps_find_meta (it, "title");
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
ps_find_meta (playItem_t *it, const char *key) {
    metaInfo_t *m = it->meta;
    while (m) {
        if (!strcmp (key, m->key)) {
            return m->value;
        }
        m = m->next;
    }
    return "?";
}

void
ps_delete_selected (void) {
    playItem_t *next = NULL;
    for (playItem_t *it = playlist_head; it; it = next) {
        next = it->next;
        if (it->selected) {
            if (it->prev) {
                it->prev->next = it->next;
            }
            if (it->next) {
                it->next->prev = it->prev;
            }
            if (playlist_head == it) {
                playlist_head = it->next;
            }
            if (playlist_tail == it) {
                playlist_tail = it->prev;
            }
            if (playlist_current_ptr == it) {
                playlist_current_ptr = NULL;
            }
            ps_item_free (it);
            free (it);
            ps_count--;
        }
    }
}

