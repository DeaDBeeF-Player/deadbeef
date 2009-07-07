#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
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

int
ps_add_file (const char *fname) {
    if (!fname) {
        return -1;
    }
    // detect codec
    codec_t *codec = NULL;
    const char *eol = fname + strlen (fname) - 1;
    while (*eol != '.') {
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
    }
    else if (!strcasecmp (eol, "nsf")) {
        codec = &cgme;
        return codec->add (fname);
    }
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
