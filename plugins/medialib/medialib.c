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
#include <sys/time.h>
#include <string.h>
#include "../../deadbeef.h"

DB_functions_t *deadbeef;

typedef struct ml_string_s {
    const char *text;
    struct ml_string_s *next;
} ml_string_t;

typedef struct ml_entry_s {
    const char *file;
    const char *title;
    int subtrack;
    ml_string_t *artist;
    ml_string_t *album;
    ml_string_t *genre;
    ml_string_t *folder;
    struct ml_entry_s *next;
} ml_entry_t;

typedef struct {
    ml_entry_t *tracks;

    // index tables
    ml_string_t *album;
    ml_string_t *artist;
    ml_string_t *genre;
    ml_string_t *folder;
} ml_db_t;

#define REG_COL(col)\
ml_string_t *\
ml_reg_##col (ml_db_t *db, const char *col) {\
    ml_string_t *s;\
    int release = 0;\
    if (!col) {\
        col = deadbeef->metacache_add_string ("Unknown");\
        release = 1;\
    }\
    for (s = db->col; s; s = s->next) {\
        if (s->text == col) {\
            if (release) {\
                deadbeef->metacache_unref (col);\
            }\
            return s;\
        }\
    }\
    if (!release) {\
        deadbeef->metacache_ref (col);\
    }\
    s = malloc (sizeof (ml_string_t));\
    memset (s, 0, sizeof (ml_string_t));\
    s->text = col;\
    s->next = db->col;\
    db->col = s;\
    return s;\
}

REG_COL(album);
REG_COL(artist);
REG_COL(genre);
REG_COL(folder);

DB_playItem_t *(*plt_insert_dir) (ddb_playlist_t *plt, DB_playItem_t *after, const char *dirname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

uintptr_t tid;
int scanner_terminate;

static int
add_file_info_cb (DB_playItem_t *it, void *data) {
//    fprintf (stderr, "added %s                                 \r", deadbeef->pl_find_meta (it, ":URI"));
    return 0;
}

static void
scanner_thread (void *none) {
    return;
    // create invisible playlist
    ddb_playlist_t *plt = deadbeef->plt_alloc ("scanner");

    struct timeval tm1;
    gettimeofday (&tm1, NULL);

    plt_insert_dir (plt, NULL, "/backup/mus/en", &scanner_terminate, add_file_info_cb, NULL);

    struct timeval tm2;
    gettimeofday (&tm2, NULL);
    int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "initial scan time: %f seconds (%d tracks)\n", ms / 1000.f, deadbeef->plt_get_item_count (plt, PL_MAIN));

    fprintf (stderr, "building index...\n");
    gettimeofday (&tm1, NULL);
    ml_db_t db;
    memset (&db, 0, sizeof (db));

    ml_entry_t *tail = NULL;

    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {

        ml_entry_t *en = malloc (sizeof (ml_entry_t));
        memset (en, 0, sizeof (ml_entry_t));

        const char *uri = deadbeef->pl_find_meta (it, ":URI");
        const char *title = deadbeef->pl_find_meta (it, "title");
        const char *artist = deadbeef->pl_find_meta (it, "artist");
        const char *album = deadbeef->pl_find_meta (it, "album");
        const char *genre = deadbeef->pl_find_meta (it, "genre");

        ml_string_t *alb = ml_reg_album (&db, album);
        ml_string_t *art = ml_reg_artist (&db, artist);
        ml_string_t *gnr = ml_reg_genre (&db, genre);

        char *fn = strrchr (uri, '/');
        ml_string_t *fld = NULL;
        if (fn) {
            char folder[fn-uri+1];
            memcpy (folder, uri, fn-uri);
            folder[fn-uri] = 0;
            const char *s = deadbeef->metacache_add_string (folder);
            fld = ml_reg_folder (&db, s);
            deadbeef->metacache_unref (s); // there should be at least 1 ref left, so it's safe
        }

        deadbeef->metacache_ref (uri);
        en->file = uri;
        deadbeef->metacache_ref (title);
        if (deadbeef->pl_get_item_flags (it) & DDB_IS_SUBTRACK) {
            en->subtrack = deadbeef->pl_find_meta_int (it, ":TRACKNUM", -1);
        }
        else {
            en->subtrack = -1;
        }
        en->title = title;
        en->artist = art;
        en->album = alb;
        en->genre = gnr;
        en->folder = fld;

        if (tail) {
            tail->next = en;
            tail = en;
        }
        else {
            tail = db.tracks = en;
        }

        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    int nalb = 0;
    int nart = 0;
    int ngnr = 0;
    int nfld = 0;
    ml_string_t *s;
    for (s = db.album; s; s = s->next, nalb++);
    for (s = db.artist; s; s = s->next, nart++);
    for (s = db.genre; s; s = s->next, ngnr++);
    for (s = db.folder; s; s = s->next, nfld++);

    fprintf (stderr, "index build time: %f seconds (%d albums, %d artists, %d genres, %d folders)\n", ms / 1000.f, nalb, nart, ngnr, nfld);

    deadbeef->plt_free (plt);
}

static int
ml_connect (void) {
    tid = deadbeef->thread_start_low_priority (scanner_thread, NULL);
    return 0;
}

static int
ml_stop (void) {
    if (tid) {
        scanner_terminate = 1;
        deadbeef->thread_join (tid);
        tid = 0;
    }

    return 0;
}

static int
ml_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
}

typedef struct {
    DB_misc_t plugin;
} ddb_medialib_plugin_t;

// define plugin interface
static ddb_medialib_plugin_t plugin = {
    .plugin.plugin.api_vmajor = 1,
    .plugin.plugin.api_vminor = 0,
    .plugin.plugin.version_major = 0,
    .plugin.plugin.version_minor = 1,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "medialib",
    .plugin.plugin.name = "Media Library",
    .plugin.plugin.descr = "Scans disk for music files and manages them as database",
    .plugin.plugin.copyright = 
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
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.connect = ml_connect,
    .plugin.plugin.stop = ml_stop,
//    .plugin.plugin.configdialog = settings_dlg,
    .plugin.plugin.message = ml_message,
};

DB_plugin_t *
medialib_load (DB_functions_t *api) {
    deadbeef = api;

    // hack: we need original function without overrides
    plt_insert_dir = deadbeef->plt_insert_dir;
    return DB_PLUGIN (&plugin);
}
