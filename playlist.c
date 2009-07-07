#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <ctype.h>
#include "playlist.h"
#include "codec.h"
#include "cwav.h"
#include "cvorbis.h"
#include "cmod.h"
#include "cmp3.h"
#include "cgme.h"
#include "cflac.h"

playItem_t *playlist_head;
playItem_t *playlist_tail;
playItem_t *playlist_current;
static int ps_count = 0;

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

int
ps_add_cue (const char *cuename) {
    printf ("trying to load %s\n", cuename);
    FILE *fp = fopen (cuename, "rt");
    if (!fp) {
        printf ("failed\n");
        return -1;
    }
    printf ("found!\n");
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
            printf ("got performer: %s\n", performer);
        }
        else if (!strncmp (p, "TITLE ", 6)) {
            if (str[0] > ' ') {
                ps_get_qvalue_from_cue (p + 6, albumtitle);
                printf ("got albumtitle: %s\n", albumtitle);
            }
            else {
                ps_get_qvalue_from_cue (p + 6, title);
                printf ("got title: %s\n", title);
            }
        }
        else if (!strncmp (p, "FILE ", 5)) {
            ps_get_qvalue_from_cue (p + 5, file);
            printf ("got filename: %s\n", file);
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
                printf ("cue file name is too long");
                return -1;
            }
            strcpy (p, file);
            // copy full name in place of relative name
            strcpy (file, fname);
            printf ("ended up as: %s\n", file);
        }
        else if (!strncmp (p, "TRACK ", 6)) {
            ps_get_value_from_cue (p + 6, track);
            printf ("got track: %s\n", track);
        }
//        else if (!strncmp (p, "PERFORMER ", 10)) {
//            ps_get_qvalue_from_cue (p + 10, performer);
//        }
        else if (!strncmp (p, "INDEX 00 ", 9)) {
            ps_get_value_from_cue (p + 9, start);
            printf ("got index0: %s\n", start);
            char *p = track;
            while (*p && isdigit (*p)) {
                p++;
            }
            *p = 0;
            // check that indexes have valid timestamps
            float tstart = ps_cue_parse_time (start);
            if (tstart < 0) {
                printf ("cue file %s has bad timestamp(s)\n", cuename);
                continue;
            }
            if (prev) {
                prev->timeend = tstart;
                printf ("end time for prev track (%x): %f\n", prev, tstart);
            }
            // add this track
            char str[1024];
            snprintf (str, 1024, "%d. %s - %s", atoi (track), performer, title[0] ? title : "?", start, tstart);
            printf ("adding %s\n", str);
            playItem_t *it = malloc (sizeof (playItem_t));
            memset (it, 0, sizeof (playItem_t));
            it->codec = &cflac;
            it->fname = strdup (file);
            it->tracknum = atoi (track);
            it->timestart = tstart;
            it->timeend = -1; // will be filled by next read, or by codec
            it->displayname = strdup (str);
            ps_append_item (it);
            printf ("added item %x\n", it);
            prev = it;
        }
        else {
            printf ("got unknown line:\n%s\n", p);
        }
    }
    fclose (fp);
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
    if (!strcasecmp (eol, "ogg")) {
        codec = &cvorbis;
    }
    else if (!strcasecmp (eol, "wav")) {
        codec = &cwav;
    }
    else if (!strcasecmp (eol, "mod")) {
        codec = &cmod;
    }
    else if (!strcasecmp (eol, "mp3")) {
        codec = &cmp3;
    }
    else if (!strcasecmp (eol, "flac")) {
        codec = &cflac;
        return codec->add (fname);
    }
    else if (!strcasecmp (eol, "nsf")) {
        codec = &cgme;
        return codec->add (fname);
    }
//    else if (!strcasecmp (eol, "cue")) {
//        ps_add_cue (fname);
//        return -1;
//    }
    else {
        return -1;
    }
    // copy string
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->codec = codec;
    it->fname = strdup (fname);
    // find 1st slash from end
    while (eol > fname && *eol != '/') {
        eol--;
    }
    if (*eol=='/') {
        eol++;
    }

    it->displayname = strdup (eol);
    it->timestart = -1;
    it->timeend = -1;

    ps_append_item (it);
}

int
ps_add_dir (const char *dirname) {
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
    if (it->fname) {
        free (it->fname);
    }
    if (it->displayname) {
        free (it->displayname);
    }
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
    free (it);
    return 0;
}

int
ps_getcount (void) {
    return ps_count;
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
