/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <string.h>
#include <stdlib.h>
#include "../../deadbeef.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_functions_t *deadbeef;

static const uint8_t *
skipspaces (const uint8_t *p, const uint8_t *end) {
    while (p < end && *p <= ' ') {
        p++;
    }
    return p;
}

static DB_playItem_t *
load_m3u (DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    const char *slash = strrchr (fname, '/');
    trace ("enter pl_insert_m3u\n");
    // skip all empty lines and comments
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    int sz = deadbeef->fgetlength (fp);
    if (sz > 1024*1024) {
        deadbeef->fclose (fp);
        trace ("file %s is too large to be a playlist\n", fname);
        return NULL;
    }
    if (sz < 30) {
        deadbeef->fclose (fp);
        trace ("file %s is too small to be a playlist (%d)\n", fname, sz);
        return NULL;
    }
    trace ("loading m3u...\n");
    uint8_t buffer[sz];
    deadbeef->fread (buffer, 1, sz, fp);
    deadbeef->fclose (fp);
    const uint8_t *p = buffer;
    const uint8_t *end = buffer+sz;
    deadbeef->pl_lock ();
    while (p < end) {
        p = skipspaces (p, end);
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

        DB_playItem_t *it = NULL;
        if (strrchr (nm, '/')) {
            trace ("pl_insert_m3u: adding file %s\n", nm);
            it = deadbeef->pl_insert_file (after, nm, pabort, cb, user_data);
        }
        else {
            int l = strlen (nm);
            char fullpath[slash - fname + l + 2];
            memcpy (fullpath, fname, slash - fname + 1);
            strcpy (fullpath + (slash - fname + 1), nm);
            trace ("pl_insert_m3u: adding file %s\n", fullpath);
            it = deadbeef->pl_insert_file (after, fullpath, pabort, cb, user_data);
        }
        if (it) {
            after = it;
        }
        if (pabort && *pabort) {
            deadbeef->pl_unlock ();
            return after;
        }
        p = e;
        if (p >= end) {
            break;
        }
    }
    deadbeef->pl_unlock ();
    trace ("leave pl_insert_m3u\n");
    return after;
}

static DB_playItem_t *
pls_insert_file (DB_playItem_t *after, const char *fname, const char *uri, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    DB_playItem_t *it = NULL;
    const char *slash = NULL;

    if (strrchr (uri, '/')) {
        trace ("pls: adding file %s\n", uri);
        it = deadbeef->pl_insert_file (after, uri, pabort, cb, user_data);
    }
    else if (slash = strrchr (fname, '/')) {
        int l = strlen (uri);
        char fullpath[slash - fname + l + 2];
        memcpy (fullpath, fname, slash - fname + 1);
        strcpy (fullpath + (slash - fname + 1), uri);
        trace ("pl_insert_m3u: adding file %s\n", fullpath);
        it = deadbeef->pl_insert_file (after, fullpath, pabort, cb, user_data);
    }
    return it;
}

static DB_playItem_t *
load_pls (DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    const char *slash = strrchr (fname, '/');
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    int sz = deadbeef->fgetlength (fp);
    if (sz > 1024*1024) {
        deadbeef->fclose (fp);
        trace ("file %s is too large to be a playlist\n", fname);
        return NULL;
    }
    if (sz < 30) {
        deadbeef->fclose (fp);
        trace ("file %s is too small to be a playlist (%d)\n", fname, sz);
        return NULL;
    }
    deadbeef->rewind (fp);
    uint8_t buffer[sz];
    deadbeef->fread (buffer, 1, sz, fp);
    deadbeef->fclose (fp);
    // 1st line must be "[playlist]"
    const uint8_t *p = buffer;
    const uint8_t *end = buffer+sz;
    if (strncasecmp (p, "[playlist]", 10)) {
        trace ("file %s doesn't begin with [playlist]\n", fname);
        return NULL;
    }
    p += 10;
    p = skipspaces (p, end);
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
    p = skipspaces (p, end);
    // fetch all tracks
    char uri[1024] = "";
    char title[1024] = "";
    char length[20] = "";
    deadbeef->pl_lock ();
    while (p < end) {
        p = skipspaces (p, end);
        if (p >= end) {
            break;
        }
        if (end-p < 6) {
            break;
        }
        const uint8_t *e;
        int n;
        if (!strncasecmp (p, "file", 4)) {
            if (uri[0]) {
                DB_playItem_t *it = pls_insert_file (after, fname, uri, pabort, cb, user_data);
                if (it) {
                    after = it;
                    deadbeef->pl_set_item_duration (it, atoi (length));
                    if (title[0]) {
                        deadbeef->pl_add_meta (it, "title", title);
                    }
                }
                if (pabort && *pabort) {
                    deadbeef->pl_unlock ();
                    return after;
                }
                uri[0] = 0;
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
            n = min (n, sizeof (uri)-1);
            memcpy (uri, p, n);
            uri[n] = 0;
            trace ("uri: %s\n", uri);
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
    if (uri[0]) {
        DB_playItem_t *it = pls_insert_file (after, fname, uri, pabort, cb, user_data);
        if (it) {
            after = it;
            deadbeef->pl_set_item_duration (it, atoi (length));
            if (title[0]) {
                deadbeef->pl_add_meta (it, "title", title);
            }
        }
    }
    deadbeef->pl_unlock ();
    return after;
}

static DB_playItem_t *
m3uplug_load (DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    const char *ext = strrchr (fname, '.');
    if (!ext) {
        return NULL;
    }
    ext++;

    if (!strcasecmp (ext, "m3u")) {
        return load_m3u (after, fname, pabort, cb, user_data);
    }
    else if (!strcasecmp (ext, "pls")) {
        return load_pls (after, fname, pabort, cb, user_data);
    }

    return NULL;
}

static const char * exts[] = { "m3u", "pls", NULL };
DB_playlist_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_PLAYLIST,
    .plugin.id = "m3u",
    .plugin.name = "M3U and PLS playlist loader",
    .plugin.descr = "Imports playlists from M3U and PLS formats",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .load = m3uplug_load,
    .extensions = exts,
};

DB_plugin_t *
m3u_load (DB_functions_t *api) {
    deadbeef = api;
    return &plugin.plugin;
}
