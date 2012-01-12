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
#include <math.h> // for ceil
#include "../../deadbeef.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

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
load_m3u (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    const char *slash = strrchr (fname, '/');
    trace ("enter pl_insert_m3u\n");
    // skip all empty lines and comments
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    int sz = deadbeef->fgetlength (fp);
    trace ("loading m3u...\n");
    uint8_t *buffer = malloc (sz);
    if (!buffer) {
        deadbeef->fclose (fp);
        trace ("failed to allocate %d bytes to read the file %s\n", sz, fname);
        return NULL;
    }
    deadbeef->fread (buffer, 1, sz, fp);
    deadbeef->fclose (fp);

    int line = 0;
    int read_extm3u = 0;

    const uint8_t *p = buffer;
    const uint8_t *end = buffer+sz;
    const uint8_t *e;
    int length = -1;
    char title[1000] = "";
    char artist[1000] = "";
    while (p < end) {
        line++;
        p = skipspaces (p, end);
        if (p >= end) {
            break;
        }
        if (*p == '#') {
            if (line == 1) {
                if (end - p >= 7 && !strncmp (p, "#EXTM3U", 7)) {
                    read_extm3u = 1;
                }
            }
            else if (read_extm3u) {
                if (end - p >= 8 && !strncmp (p, "#EXTINF:", 8)) {
                    length = -1;
                    title[0] = 0;
                    artist[0] = 0;
                    p += 8;
                    e = p;
                    while (e < end && *e >= 0x20) {
                        e++;
                    }
                    int n = e-p;
                    uint8_t nm[n+1];
                    memcpy (nm, p, n);
                    nm[n] = 0;
                    length = atoi (nm);
                    char *c = nm;
                    while (*c && *c != ',') {
                        c++;
                    }
                    if (*c == ',') {
                        c++;
                        if (2 != sscanf (c, "%1000s - %1000s", artist, title)) {
                            strncpy (artist, c, sizeof (artist)-1);
                            artist[sizeof(artist)-1] = 0;
                        }
                    }
                }
            }
            while (p < end && *p >= 0x20) {
                p++;
            }
            if (p >= end) {
                break;
            }
            continue;
        }
        e = p;
        while (e < end && *e >= 0x20) {
            e++;
        }
        int n = e-p;
        uint8_t nm[n+1];
        memcpy (nm, p, n);
        nm[n] = 0;

        if (title[0]) {
            const char *cs = deadbeef->junk_detect_charset (title);
            if (cs) {
                char tmp[2048];
                if (deadbeef->junk_iconv (title, strlen (title), tmp, sizeof (tmp), cs, "utf-8") >= 0) {
                    strcpy (title, tmp);
                }
            }
        }
        if (artist[0]) {
            const char *cs = deadbeef->junk_detect_charset (artist);
            if (cs) {
                char tmp[2048];
                if (deadbeef->junk_iconv (artist, strlen (artist), tmp, sizeof (tmp), cs, "utf-8") >= 0) {
                    strcpy (artist, tmp);
                }
            }
        }

        DB_playItem_t *it = NULL;
        if (strrchr (nm, '/')) {
            trace ("pl_insert_m3u: adding file %s\n", nm);
            it = deadbeef->plt_insert_file (plt, after, nm, pabort, cb, user_data);
            if (it) {
                if (length >= 0) {
                    deadbeef->plt_set_item_duration (plt, it, length);
                }
                if (title[0]) {
                    deadbeef->pl_replace_meta (it, "title", title);
                }
                else if (artist[0]) {
                    deadbeef->pl_replace_meta (it, "title", " ");
                }
                if (artist[0]) {
                    deadbeef->pl_replace_meta (it, "artist", artist);
                }
            }
        }
        else {
            int l = strlen (nm);
            char fullpath[slash - fname + l + 2];
            memcpy (fullpath, fname, slash - fname + 1);
            strcpy (fullpath + (slash - fname + 1), nm);
            trace ("pl_insert_m3u: adding file %s\n", fullpath);
            it = deadbeef->plt_insert_file (plt, after, fullpath, pabort, cb, user_data);
        }
        if (it) {
            after = it;
        }
        if (pabort && *pabort) {
            if (after) {
                deadbeef->pl_item_ref (after);
            }
            free (buffer);
            return after;
        }
        p = e;
        if (p >= end) {
            break;
        }
    }
    if (after) {
        deadbeef->pl_item_ref (after);
    }
    trace ("leave pl_insert_m3u\n");
    free (buffer);
    return after;
}

static DB_playItem_t *
pls_insert_file (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, const char *uri, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data, const char *title, const char *length) {
    trace ("pls_insert_file uri: %s\n", uri);
    DB_playItem_t *it = NULL;
    const char *slash = NULL;

    if (strrchr (uri, '/')) {
        it = deadbeef->plt_insert_file (plt, after, uri, pabort, cb, user_data);
    }
    else if (slash = strrchr (fname, '/')) {
        int l = strlen (uri);
        char fullpath[slash - fname + l + 2];
        memcpy (fullpath, fname, slash - fname + 1);
        strcpy (fullpath + (slash - fname + 1), uri);
        trace ("pls_insert_file: adding file %s\n", fullpath);
        it = deadbeef->plt_insert_file (plt, after, fullpath, pabort, cb, user_data);
    }
    if (it) {
        if (length[0]) {
            deadbeef->plt_set_item_duration (plt, it, atoi (length));
        }
        if (title[0]) {
            deadbeef->pl_replace_meta (it, "title", title);
        }
    }
    return it;
}

static DB_playItem_t *
load_pls (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    const char *slash = strrchr (fname, '/');
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    int sz = deadbeef->fgetlength (fp);
    deadbeef->rewind (fp);
    uint8_t *buffer = malloc (sz);
    if (!buffer) {
        deadbeef->fclose (fp);
        trace ("failed to allocate %d bytes to read the file %s\n", sz, fname);
        return NULL;
    }
    deadbeef->fread (buffer, 1, sz, fp);
    deadbeef->fclose (fp);
    // 1st line must be "[playlist]"
    const uint8_t *p = buffer;
    const uint8_t *end = buffer+sz;
    if (strncasecmp (p, "[playlist]", 10)) {
        trace ("file %s doesn't begin with [playlist]\n", fname);
        free (buffer);
        return NULL;
    }
    p += 10;
    p = skipspaces (p, end);
    if (p >= end) {
        trace ("file %s finished before numberofentries had been read\n", fname);
        free (buffer);
        return NULL;
    }
    // fetch all tracks
    char uri[1024] = "";
    char title[1024] = "";
    char length[20] = "";
    int lastidx = -1;
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
        if (!strncasecmp (p, "numberofentries=", 16) || !strncasecmp (p, "version=", 8)) {
            while (p < end && *p >= 0x20) {
                p++;
            }
            continue;
        }
        else if (!strncasecmp (p, "file", 4)) {
            int idx = atoi (p + 4);
            if (uri[0] && idx != lastidx && lastidx != -1) {
                trace ("uri%d\n", idx);
                DB_playItem_t *it = pls_insert_file (plt, after, fname, uri, pabort, cb, user_data, title, length);
                if (it) {
                    after = it;
                }
                if (pabort && *pabort) {
                    if (after) {
                        deadbeef->pl_item_ref (after);
                    }
                    free (buffer);
                    return after;
                }
                uri[0] = 0;
                title[0] = 0;
                length[0] = 0;
            }
            lastidx = idx;
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
            trace ("uri%d=%s\n", idx, uri);
            p = ++e;
        }
        else if (!strncasecmp (p, "title", 5)) {
            int idx = atoi (p + 5);
            if (uri[0] && idx != lastidx && lastidx != -1) {
                trace ("title%d\n", idx);
                DB_playItem_t *it = pls_insert_file (plt, after, fname, uri, pabort, cb, user_data, title, length);
                if (it) {
                    after = it;
                }
                if (pabort && *pabort) {
                    if (after) {
                        deadbeef->pl_item_ref (after);
                    }
                    free (buffer);
                    return after;
                }
                uri[0] = 0;
                title[0] = 0;
                length[0] = 0;
            }
            lastidx = idx;
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
            trace ("title%d=%s\n", idx, title);
            p = ++e;
        }
        else if (!strncasecmp (p, "length", 6)) {
            int idx = atoi (p + 6);
            if (uri[0] && idx != lastidx && lastidx != -1) {
                trace ("length%d\n", idx);
                DB_playItem_t *it = pls_insert_file (plt, after, fname, uri, pabort, cb, user_data, title, length);
                if (it) {
                    after = it;
                }
                if (pabort && *pabort) {
                    if (after) {
                        deadbeef->pl_item_ref (after);
                    }
                    free (buffer);
                    return after;
                }
                uri[0] = 0;
                title[0] = 0;
                length[0] = 0;
            }
            lastidx = idx;
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
            trace ("length%d=%s\n", idx, length);
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
        DB_playItem_t *it = pls_insert_file (plt, after, fname, uri, pabort, cb, user_data, title, length);
        if (it) {
            after = it;
        }
    }
    if (after) {
        deadbeef->pl_item_ref (after);
    }
    free (buffer);
    return after;
}

static DB_playItem_t *
m3uplug_load (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    const char *ext = strrchr (fname, '.');
    if (ext) {
        ext++;
    }

    DB_playItem_t *ret = NULL;
    if (ext && !strcasecmp (ext, "pls")) {
        ret = load_pls (plt, after, fname, pabort, cb, user_data);
    }
    
    if (!ret) {
        ret = load_m3u (plt, after, fname, pabort, cb, user_data);
    }

    return ret;
}

int
m3uplug_save_m3u (const char *fname, DB_playItem_t *first, DB_playItem_t *last) {
    FILE *fp = fopen (fname, "w+t");
    if (!fp) {
        return -1;
    }
    DB_playItem_t *it = first;
    deadbeef->pl_item_ref (it);
    fprintf (fp, "#M3UEXT\n");
    while (it) {
        int dur = (int)ceil(deadbeef->pl_get_item_duration (it));
        char s[1000];
        deadbeef->pl_format_title (it, -1, s, sizeof (s), -1, "%a - %t");
        const char *fname = deadbeef->pl_find_meta (it, ":URI");
        fprintf (fp, "#EXTINF:%d,%s\n", dur, s);
        fprintf (fp, "%s\n", fname);

        if (it == last) {
            break;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    fclose (fp);
    return 0;
}

int
m3uplug_save_pls (const char *fname, DB_playItem_t *first, DB_playItem_t *last) {
    FILE *fp = fopen (fname, "w+t");
    if (!fp) {
        return -1;
    }

    int n = 0;
    DB_playItem_t *it = first;
    deadbeef->pl_item_ref (it);
    while (it) {
        n++;
        if (it == last) {
            break;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    fprintf (fp, "[playlist]\n");
    fprintf (fp, "NumberOfEntries=%d\n", n);

    it = first;
    deadbeef->pl_item_ref (it);
    int i = 1;
    while (it) {
        const char *fname = deadbeef->pl_find_meta (it, ":URI");
        fprintf (fp, "File%d=%s\n", i, fname);

        if (it == last) {
            break;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
        i++;
    }
    fclose (fp);
    return 0;
}

int
m3uplug_save (ddb_playlist_t *plt, const char *fname, DB_playItem_t *first, DB_playItem_t *last) {
    const char *e = strrchr (fname, '.');
    if (!e) {
        return -1;
    }
    if (!strcasecmp (e, ".m3u") || !strcasecmp (e, ".m3u8")) {
        return m3uplug_save_m3u (fname, first, last);
    }
    else if (!strcasecmp (e, ".pls")) {
        return m3uplug_save_pls (fname, first, last);
    }
    return -1;
}

static const char * exts[] = { "m3u", "m3u8", "pls", NULL };
DB_playlist_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_PLAYLIST,
    .plugin.id = "m3u",
    .plugin.name = "M3U and PLS support",
    .plugin.descr = "Importing and exporting M3U and PLS formats\nRecognizes .pls, .m3u and .m3u8 file types\n\nNOTE: only utf8 file names are currently supported",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .load = m3uplug_load,
    .save = m3uplug_save,
    .extensions = exts,
};

DB_plugin_t *
m3u_load (DB_functions_t *api) {
    deadbeef = api;
    return &plugin.plugin;
}
