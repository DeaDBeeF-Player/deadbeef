/*
    M3U and PLS playlist plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko

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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h> // for ceil

#include <deadbeef/deadbeef.h>

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

DB_playItem_t *
load_m3u_from_buffer(DB_playItem_t *after, const char *buffer, int64_t sz, int (*cb)(DB_playItem_t *, void *), const char *fname, int *pabort, ddb_playlist_t *plt, void *user_data) {
    const char *slash = strrchr (fname, '/');
    if (sz >= 3 && (uint8_t)buffer[0] == 0xef && (uint8_t)buffer[1] == 0xbb && (uint8_t)buffer[2] == 0xbf) {
        buffer += 3;
        sz -= 3;
    }
    int line = 0;
    int read_extm3u = 0;

    const char *p = buffer;
    const char *end = buffer+sz;
    const char *e;
    int length = -1;
    char title[1000] = "";
    char artist[1000] = "";
    while (p < end) {
        line++;
        p = (const char *)skipspaces ((const uint8_t *)p, (const uint8_t *)end);
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
                    memset (title, 0, sizeof (title));
                    memset (artist, 0, sizeof (artist));
                    p += 8;
                    e = p;
                    while (e < end && ((uint8_t)(*e)) >= 0x20) {
                        e++;
                    }
                    long n = e-p;
                    char nm[n+1];
                    memcpy (nm, p, n);
                    nm[n] = 0;
                    length = atoi (nm);
                    char *c = nm;
                    while (*c && *c != ',') {
                        c++;
                    }
                    if (*c == ',') {
                        c++;
                        while (*c && ((uint8_t)*c) <= 0x20) {
                            c++;
                        }
                        const char *dash = NULL;
                        const char *newdash = strstr (c, " - ");

                        while (newdash) {
                            dash = newdash;
                            newdash = strstr (newdash+3, " - ");
                        }

                        if (dash) {
                            strncpy (title, dash+3, sizeof (title)-1);
                            title[sizeof(title)-1] = 0;
                            long l = dash - c;
                            strncpy (artist, c, min(l, sizeof (artist)));
                            artist[sizeof(artist)-1] = 0;
                        }
                        else {
                            strncpy (title, c, sizeof (title)-1);
                            title[sizeof(title)-1] = 0;
                        }
                        trace ("title: %s, artist: %s\n", title, artist);
                    }
                }
            }
            while (p < end && ((uint8_t)(*p)) >= 0x20) {
                p++;
            }
            if (p >= end) {
                break;
            }
            continue;
        }
        e = p;
        while (e < end && ((uint8_t)(*e)) >= 0x20) {
            e++;
        }
        long n = e-p;
        char nm[n+1];
        memcpy (nm, p, n);
        nm[n] = 0;

        if (title[0]) {
            const char *cs = deadbeef->junk_detect_charset (title);
            if (cs) {
                char tmp[1000];
                if (deadbeef->junk_iconv (title, (int)strlen (title), tmp, sizeof (tmp), cs, "utf-8") >= 0) {
                    strcpy (title, tmp);
                }
            }
        }
        if (artist[0]) {
            const char *cs = deadbeef->junk_detect_charset (artist);
            if (cs) {
                char tmp[1000];
                if (deadbeef->junk_iconv (artist, (int)strlen (artist), tmp, sizeof (tmp), cs, "utf-8") >= 0) {
                    strcpy (artist, tmp);
                }
            }
        }

        DB_playItem_t *it = NULL;
        int is_fullpath = 0;
        if (nm[0] == '/') {
            is_fullpath = 1;
        }
        else {
            char *p = strstr (nm, "://");
            if (p) {
                p--;
                while (p >= nm) {
                    if (*p < 'a' && *p > 'z') {
                        break;
                    }
                    p--;
                }
                if (p < nm) {
                    is_fullpath = 1;
                }
            }
        }
        if (is_fullpath) { // full path
            trace ("pl_insert_m3u: adding file %s\n", nm);
            it = deadbeef->plt_insert_file2 (0, plt, after, nm, pabort, cb, user_data);
            if (it) {
                if (length >= 0 && deadbeef->pl_get_item_duration (it) < 0) {
                    deadbeef->plt_set_item_duration (plt, it, length);
                }
                if (title[0]) {
                    deadbeef->pl_add_meta (it, "title", title);
                }
                if (artist[0]) {
                    deadbeef->pl_add_meta (it, "artist", artist);
                }
            }
            // reset title/artist, to avoid them from being reused in the next track
            memset (title, 0, sizeof (title));
            memset (artist, 0, sizeof (artist));
        }
        else {
            size_t l = strlen (nm);
            char fullpath[slash - fname + l + 2];
            memcpy (fullpath, fname, slash - fname + 1);
            strcpy (fullpath + (slash - fname + 1), nm);
            trace ("pl_insert_m3u: adding file %s\n", fullpath);
            it = deadbeef->plt_insert_file2 (0, plt, after, fullpath, pabort, cb, user_data);
        }
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

static DB_playItem_t *
load_m3u (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    trace ("enter pl_insert_m3u\n");
    // skip all empty lines and comments
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    int64_t sz = deadbeef->fgetlength (fp);
    trace ("loading m3u...\n");
    char *membuffer = malloc (sz);
    if (!membuffer) {
        deadbeef->fclose (fp);
        trace ("failed to allocate %d bytes to read the file %s\n", sz, fname);
        return NULL;
    }
    char *buffer = membuffer;
    deadbeef->fread (buffer, 1, sz, fp);
    deadbeef->fclose (fp);

    after = load_m3u_from_buffer(after, buffer, sz, cb, fname, pabort, plt, user_data);
    trace ("leave pl_insert_m3u\n");
    free (membuffer);
    return after;
}

static DB_playItem_t *
pls_insert_file (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, const char *uri, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data, const char *title, const char *length) {
    trace ("pls_insert_file uri: %s\n", uri);
    trace ("pls_insert_file fname: %s\n", fname);
    DB_playItem_t *it = NULL;
    const char *slash = NULL;

    if (strrchr (uri, '/')) {
        trace ("pls: inserting from uri: %s\n", uri);
        it = deadbeef->plt_insert_file2 (0, plt, after, uri, pabort, cb, user_data);
    }

    if (!it) {
        slash = strrchr (fname, '/');
    }
    if (slash) {
        size_t l = strlen (uri);
        char fullpath[slash - fname + l + 2];
        memcpy (fullpath, fname, slash - fname + 1);
        strcpy (fullpath + (slash - fname + 1), uri);
        trace ("pls: inserting from calculated relative path: %s\n", fullpath);
        it = deadbeef->plt_insert_file2 (0, plt, after, fullpath, pabort, cb, user_data);
    }
    if (it) {
        if (length[0]) {
            deadbeef->plt_set_item_duration (plt, it, atoi (length));
        }
        if (title[0]) {
            deadbeef->pl_add_meta (it, "title", title);
        }
    }
    return it;
}

static DB_playItem_t *
load_pls (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    trace ("load_pls %s\n", fname);
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    int64_t sz = deadbeef->fgetlength (fp);
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
    const char *p = (const char *)buffer;
    const char *end = (const char *)buffer+sz;
    if (strncasecmp (p, "[playlist]", 10)) {
        trace ("file %s doesn't begin with [playlist]\n", fname);
        free (buffer);
        return NULL;
    }
    p += 10;
    p = (const char *)skipspaces ((const uint8_t *)p, (const uint8_t *)end);
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
        p = (const char *)skipspaces ((const uint8_t *)p, (const uint8_t *)end);
        if (p >= end) {
            break;
        }
        if (end-p < 6) {
            break;
        }
        const char *e;
        long n;
        if (!strncasecmp (p, "file", 4)) {
            int idx = atoi (p + 4);
            if (uri[0] && idx != lastidx && lastidx != -1) {
                DB_playItem_t *it = pls_insert_file (plt, after, fname, uri, pabort, cb, user_data, title, length);
                if (it) {
                    after = it;
                }
                if (pabort && *pabort) {
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
            while (p < end && ((uint8_t)(*p)) <= 0x20) {
                p++;
            }
            if (p >= end) {
                break;
            }
            e = p;
            while (e < end && ((uint8_t)(*e)) >= 0x20) {
                e++;
            }
            n = e-p;
            n = min (n, sizeof (uri)-1);
            memcpy (uri, p, n);
            uri[n] = 0;
            trace ("uri: %s\n", uri);
            trace ("uri%d=%s\n", idx, uri);
            e++;
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
            while (p < end && ((uint8_t)(*p)) <= 0x20) {
                p++;
            }
            if (p >= end) {
                break;
            }
            e = p;
            while (e < end && ((uint8_t)(*e)) >= 0x20) {
                e++;
            }
            n = e-p;
            n = min (n, sizeof (title)-1);
            memcpy (title, p, n);
            title[n] = 0;
            trace ("title%d=%s\n", idx, title);
            e++;
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
            while (e < end && ((uint8_t)(*e)) >= 0x20) {
                e++;
            }
            n = e-p;
            n = min (n, sizeof (length)-1);
            memcpy (length, p, n);
            trace ("length%d=%s\n", idx, length);
        }
        else {
            trace ("pls: skipping unrecognized entry in pls file: %s\n", p);
            e = p;
            while (e < end && ((uint8_t)(*e)) >= 0x20) {
                e++;
            }
        }
        while (e < end && ((uint8_t)(*e)) < 0x20) {
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
    free (buffer);
    return after;
}

static DB_playItem_t *
m3uplug_load (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    char resolved_fname[PATH_MAX];
    char *res = realpath (fname, resolved_fname);
    if (res) {
        fname = resolved_fname;
    }

    const char *ext = strrchr (fname, '.');
    if (ext) {
        ext++;
    }

    DB_playItem_t *ret = NULL;

    int tried_pls = 0;

    if (ext && !strcasecmp (ext, "pls")) {
        tried_pls = 1;
        ret = load_pls (plt, after, fname, pabort, cb, user_data);
    }
    
    if (!ret) {
        ret = load_m3u (plt, after, fname, pabort, cb, user_data);
    }

    if (!ret && !tried_pls) {
        ret = load_pls (plt, after, fname, pabort, cb, user_data);
    }

    return ret;
}

int
m3uplug_save_m3u (const char *fname, DB_playItem_t *first, DB_playItem_t *last) {
    FILE *fp = fopen (fname, "w+t");
    if (!fp) {
        return -1;
    }

    char *tf = deadbeef->tf_compile ("[%artist% - ]%title%");
    char s[1000];

    DB_playItem_t *it = first;
    deadbeef->pl_item_ref (it);
    fprintf (fp, "#EXTM3U\n");
    while (it) {
        // skip subtracks, pls and m3u formats don't support that
        uint32_t flags = deadbeef->pl_get_item_flags (it);
        if (flags & DDB_IS_SUBTRACK) {
            if (deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0)) {
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
                continue;
            }
        }
        int dur = (int)floor(deadbeef->pl_get_item_duration (it));
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .it = it,
        };
        deadbeef->tf_eval (&ctx, tf, s, sizeof (s));
        fprintf (fp, "#EXTINF:%d,%s\n", dur, s);

        deadbeef->pl_lock ();
        {
            const char *fname = deadbeef->pl_find_meta (it, ":URI");
            fprintf (fp, "%s\n", fname);
        }
        deadbeef->pl_unlock ();

        if (it == last) {
            break;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    fclose (fp);

    deadbeef->tf_free (tf);
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
        // skip subtracks, pls and m3u formats don't support that
        uint32_t flags = deadbeef->pl_get_item_flags (it);
        if (flags & DDB_IS_SUBTRACK) {
            if (deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0)) {
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
                continue;
            }
        }
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
        // skip subtracks, pls and m3u formats don't support that
        uint32_t flags = deadbeef->pl_get_item_flags (it);
        if (flags & DDB_IS_SUBTRACK) {
            if (deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0)) {
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
                continue;
            }
        }
        deadbeef->pl_lock ();
        {
            const char *fname = deadbeef->pl_find_meta (it, ":URI");
            fprintf (fp, "File%d=%s\n", i, fname);
        }
        deadbeef->pl_unlock ();

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
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_PLAYLIST,
    .plugin.id = "m3u",
    .plugin.name = "M3U and PLS support",
    .plugin.descr = "Importing and exporting M3U and PLS formats\nRecognizes .pls, .m3u and .m3u8 file types\n\nNOTE: only utf8 file names are currently supported",
    .plugin.copyright = 
        "M3U and PLS playlist plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
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
