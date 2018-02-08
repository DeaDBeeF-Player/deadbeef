/*
    Media Library plugin for DeaDBeeF Player
    Copyright (C) 2009-2016 Alexey Yakovenko

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

#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "../../deadbeef.h"

DB_functions_t *deadbeef;

static int filter_id;

typedef struct ml_string_s {
    const char *text;
    struct ml_string_s *bucket_next;
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
    struct ml_entry_s *bucket_next;
} ml_entry_t;

#define ML_HASH_SIZE 4096

typedef struct {
    // plain list of all tracks in the entire collection
    ml_entry_t *tracks;

    // hash formed by filename pointer
    // this hash purpose is to quickly check whether the filename is in the library already
    // NOTE: this hash doesn't contain all of the tracks from the `tracks` list, because of subtracks
    ml_entry_t *filename_hash[ML_HASH_SIZE];

    // hash tables for each index
    ml_string_t *hash_album[ML_HASH_SIZE];
    ml_string_t *hash_artist[ML_HASH_SIZE];
    ml_string_t *hash_genre[ML_HASH_SIZE];
    ml_string_t *hash_folder[ML_HASH_SIZE];
} ml_db_t;

static uint32_t
hash_for_ptr (void *ptr) {
    return (((uint32_t)(ptr))>>1) & (ML_HASH_SIZE-1);
}

static ml_string_t *
hash_find_for_hashkey (ml_string_t **hash, const char *val, uint32_t h) {
    ml_string_t *bucket = hash[h];
    while (bucket) {
        if (bucket->text == val) {
            return bucket;
        }
        bucket = bucket->bucket_next;
    }
    return NULL;
}

ml_string_t *
hash_find (ml_string_t **hash, const char *val) {
    uint32_t h = hash_for_ptr ((void *)val) & (ML_HASH_SIZE-1);
    return hash_find_for_hashkey(hash, val, h);
}

static ml_string_t *
hash_add (ml_string_t **hash, const char *val) {
    uint32_t h = hash_for_ptr ((void *)val) & (ML_HASH_SIZE-1);
    if (!hash_find_for_hashkey(hash, val, h)) {
        deadbeef->metacache_ref (val);
        ml_string_t *s = calloc (sizeof (ml_string_t), 1);
        s->bucket_next = hash[h];
        s->text = val;
        deadbeef->metacache_ref (val);
        hash[h] = s;
        return s;
    }
    return NULL;
}

static ddb_playlist_t *ml_playlist; // this playlist contains the actual data of the media library in plain list

static ml_db_t db; // this is the index, which can be rebuilt from the playlist at any given time

#define REG_COL_DEF(col)\
ml_string_t *\
ml_reg_##col (ml_db_t *db, const char *c) {\
    if (!c) {\
        return NULL;\
    }\
    return hash_add (db->hash_##col, c);\
}

REG_COL_DEF(album);
REG_COL_DEF(artist);
REG_COL_DEF(genre);
REG_COL_DEF(folder);

DB_playItem_t *(*plt_insert_dir) (ddb_playlist_t *plt, DB_playItem_t *after, const char *dirname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

uintptr_t tid;
int scanner_terminate;

static int
add_file_info_cb (DB_playItem_t *it, void *data) {
//    fprintf (stderr, "added %s                                 \r", deadbeef->pl_find_meta (it, ":URI"));
    return 0;
}

#define FREE_COL(col)\
    for (int idx_##col = 0; idx_##col < ML_HASH_SIZE; idx_##col++) {\
        ml_string_t *s = db.hash_##col[idx_##col];\
        while (s) {\
            ml_string_t *next = s->bucket_next;\
            if (s->text) {\
                deadbeef->metacache_unref (s->text);\
            }\
            free (s);\
            s = next;\
        }\
        db.hash_##col[idx_##col] = NULL;\
    }

static void
ml_free_db (void) {
    fprintf (stderr, "clearing index...\n");

    FREE_COL(album);
    FREE_COL(artist);
    FREE_COL(genre);
    FREE_COL(folder);

    while (db.tracks) {
        ml_entry_t *next = db.tracks->next;
        if (db.tracks->title) {
            deadbeef->metacache_unref (db.tracks->title);
        }
        if (db.tracks->file) {
            deadbeef->metacache_unref (db.tracks->file);
        }
        free (db.tracks);
        db.tracks = next;
    }

    memset (&db, 0, sizeof (db));
}

// This should be called only on pre-existing ml playlist.
// Subsequent indexing should be done on the fly, using fileadd listener.
static void
ml_index (void) {
    ml_free_db();

    fprintf (stderr, "building index...\n");

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);

    ml_entry_t *tail = NULL;

    char folder[PATH_MAX];

    DB_playItem_t *it = deadbeef->plt_get_first (ml_playlist, PL_MAIN);
    while (it) {
        ml_entry_t *en = calloc (sizeof (ml_entry_t), 1);

        const char *uri = deadbeef->pl_find_meta (it, ":URI");
        const char *title = deadbeef->pl_find_meta (it, "title");
        const char *artist = deadbeef->pl_find_meta (it, "artist");

        // FIXME: album needs to be a combination of album + artist for indexing / library
        const char *album = deadbeef->pl_find_meta (it, "album");
        const char *genre = deadbeef->pl_find_meta (it, "genre");
        ml_string_t *alb = ml_reg_album (&db, album);
        ml_string_t *art = ml_reg_artist (&db, artist);
        ml_string_t *gnr = ml_reg_genre (&db, genre);

        char *fn = strrchr (uri, '/');
        ml_string_t *fld = NULL;
        if (fn) {
            memcpy (folder, uri, fn-uri);
            folder[fn-uri] = 0;
            const char *s = deadbeef->metacache_add_string (folder);
            fld = ml_reg_folder (&db, s);
            deadbeef->metacache_unref (s);
        }

        // uri and title are not indexed, only a part of track list,
        // that's why they have an extra ref for each entry
        deadbeef->metacache_ref (uri);
        en->file = uri;
        if (title) {
            deadbeef->metacache_ref (title);
        }
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

        // add to the hash table
        // at this point, we only have unique pointers, and don't need a duplicate check
        uint32_t hash = hash_for_ptr ((void *)en->file);
        en->bucket_next = db.filename_hash[hash];
        db.filename_hash[hash] = en;

        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    int nalb = 0;
    int nart = 0;
    int ngnr = 0;
    int nfld = 0;
    ml_string_t *s;
    for (int i = 0; i < ML_HASH_SIZE; i++) {
        for (s = db.hash_album[i]; s; s = s->bucket_next, nalb++);
        for (s = db.hash_artist[i]; s; s = s->bucket_next, nart++);
        for (s = db.hash_genre[i]; s; s = s->bucket_next, ngnr++);
        for (s = db.hash_folder[i]; s; s = s->bucket_next, nfld++);
    }
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    fprintf (stderr, "index build time: %f seconds (%d albums, %d artists, %d genres, %d folders)\n", ms / 1000.f, nalb, nart, ngnr, nfld);
}

static void
scanner_thread (void *none) {
    char plpath[PATH_MAX];
    snprintf (plpath, sizeof (plpath), "%s/medialib.dbpl", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG));

    struct timeval tm1, tm2;

    if (!ml_playlist) {
        ml_playlist = deadbeef->plt_alloc ("medialib");

        printf ("loading %s\n", plpath);
        gettimeofday (&tm1, NULL);
        DB_playItem_t *plt_head = deadbeef->plt_load2 (-1, ml_playlist, NULL, plpath, NULL, NULL, NULL);
        gettimeofday (&tm2, NULL);
        long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
        fprintf (stderr, "ml playlist load time: %f seconds\n", ms / 1000.f);

        if (plt_head) {
//            for (int i = 0; i < 100; i++) {
                ml_index ();
//            }
        }
    }

    gettimeofday (&tm1, NULL);

    const char *musicdir = deadbeef->conf_get_str_fast ("medialib.path", NULL);
    if (!musicdir) {
        return;
    }

    printf ("adding dir: %s\n", musicdir);
    plt_insert_dir (ml_playlist, NULL, musicdir, &scanner_terminate, add_file_info_cb, NULL);

    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "scan time: %f seconds (%d tracks)\n", ms / 1000.f, deadbeef->plt_get_item_count (ml_playlist, PL_MAIN));

    deadbeef->plt_save (ml_playlist, NULL, NULL, plpath, NULL, NULL, NULL);
}

//#define FILTER_PERF

// intention is to skip the files which are already indexed
// how to speed this up:
// first check if a folder exists (early out?)
static int
ml_fileadd_filter (ddb_file_found_data_t *data, void *user_data) {
    int res = 0;

    if (data->plt != ml_playlist || data->is_dir) {
        return 0;
    }

#if FILTER_PERF
    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);
#endif

    const char *s = deadbeef->metacache_get_string (data->filename);
    if (!s) {
        return 0;
    }

    uint32_t hash = (((uint32_t)(s))>>1) & (ML_HASH_SIZE-1);

    if (!db.filename_hash[hash]) {
        return 0;
    }

    ml_entry_t *en = db.filename_hash[hash];
    while (en) {
        if (en->file == s) {
            res = -1;
            break;
        }
        en = en->bucket_next;
    }

#if FILTER_PERF
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    if (!res) {
        fprintf (stderr, "ADD %s: file presence check took %f sec\n", s, ms / 1000.f);
    }
    else {
        fprintf (stderr, "SKIP %s: file presence check took %f sec\n", s, ms / 1000.f);
    }
#endif

    deadbeef->metacache_unref (s);

    return res;
}

static int
ml_connect (void) {
#if 0
    //tid = deadbeef->thread_start_low_priority (scanner_thread, NULL);

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);
    scanner_thread(NULL);
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "whole ml init time: %f seconds\n", ms / 1000.f);
    exit (0);
#endif
    return 0;
}

static int
ml_start (void) {
    filter_id = deadbeef->register_fileadd_filter (ml_fileadd_filter, NULL);
    return 0;
}

static int
ml_stop (void) {
    if (tid) {
        scanner_terminate = 1;
        deadbeef->thread_join (tid);
        tid = 0;
    }
    if (filter_id) {
        deadbeef->unregister_fileadd_filter (filter_id);
        filter_id = 0;
    }

    if (ml_playlist) {
        deadbeef->plt_free (ml_playlist);
    }

    return 0;
}

static int
ml_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
}

typedef struct ddb_medialib_plugin_s {
    DB_misc_t plugin;
} ddb_medialib_plugin_t;

// define plugin interface
static ddb_medialib_plugin_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.plugin.version_major = 0,
    .plugin.plugin.version_minor = 1,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "medialib",
    .plugin.plugin.name = "Media Library",
    .plugin.plugin.descr = "Scans disk for music files and manages them as database",
    .plugin.plugin.copyright = 
        "Media Library plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Alexey Yakovenko\n"
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
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.connect = ml_connect,
    .plugin.plugin.start = ml_start,
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
