/*
    Media Library plugin for DeaDBeeF Player
    Copyright (C) 2009-2020 Alexey Yakovenko

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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <jansson.h>
#include "../../deadbeef.h"
#include "medialib.h"

//#define FILTER_PERF 1 // measure / log file add filtering performance

static DB_functions_t *deadbeef;

static int filter_id;
static char *artist_album_bc;
static char *artist_album_id_bc;
static char *title_bc;

typedef struct ml_collection_item_s {
    DB_playItem_t *it;
    struct ml_collection_item_s *next;
} ml_collection_item_t;

typedef struct ml_string_s {
    const char *text;
    ml_collection_item_t *items;
    int items_count;
    ml_collection_item_t *items_tail;
    struct ml_string_s *bucket_next;
    struct ml_string_s *next;

    ddb_medialib_item_t *coll_item; // The item associated with collection string, used while building a list
    ddb_medialib_item_t *coll_item_tail; // Tail of the children list of coll_item, used while building a list
} ml_string_t;

typedef struct ml_entry_s {
    const char *file;
    const char *title;
    int subtrack;
    ml_string_t *artist;
    ml_string_t *album;
    ml_string_t *genre;
    ml_string_t *folder;
    ml_string_t *track_uri;
    struct ml_entry_s *next;
    struct ml_entry_s *bucket_next;
} ml_entry_t;

typedef struct ml_cached_string_s {
    const char *s;
    struct ml_cached_string_s *next;
} ml_cached_string_t;

#define ML_HASH_SIZE 4096

// a list of unique names in collection, as a list, and as a hash, with each item associated with list of tracks
typedef struct {
    ml_string_t *hash[ML_HASH_SIZE];
    ml_string_t *head;
    ml_string_t *tail;
    int count;
} ml_collection_t;

typedef struct ml_tree_node_s {
    const char *text;
    ml_collection_item_t *items;
    struct ml_tree_node_s *next;
    struct ml_tree_node_s *children;
} ml_tree_node_t;

typedef struct {
    // Plain list of all tracks in the entire collection
    // The purpose is to hold references to all metadata strings, used by the DB
    ml_entry_t *tracks;

    // hash formed by filename pointer
    // this hash purpose is to quickly check whether the filename is in the library already
    // NOTE: this hash doesn't contain all of the tracks from the `tracks` list, because of subtracks
    ml_entry_t *filename_hash[ML_HASH_SIZE];

    // plain lists for each index
    ml_collection_t albums;
    ml_collection_t artists;
    ml_collection_t genres;
    //collection_t folders;

    // for the folders, a tree structure is used
    ml_tree_node_t *folders_tree;

    // This hash is formed from track_uri ([%:TRACKNUM%#]%:URI%), and supposed to have all tracks from the `tracks` list
    // Main purpose is to find a library instance of a track for given track pointer
    ml_collection_t track_uris;

    // list of all strings which are not referenced by tracks
    ml_cached_string_t *cached_strings;
} ml_db_t;

static ddb_playlist_t *ml_playlist; // this playlist contains the actual data of the media library in plain list

typedef struct {
    ddb_playlist_t *plt;
} ml_filter_state_t;
static ml_filter_state_t ml_filter_state;

static ml_db_t db; // this is the index, which can be rebuilt from the playlist at any given time

static json_t *musicpaths_json;

static uintptr_t mutex;

#define MAX_LISTENERS 10
static ddb_medialib_listener_t ml_listeners[MAX_LISTENERS];
static void *ml_listeners_userdatas[MAX_LISTENERS];

static int _ml_state = DDB_MEDIALIB_STATE_IDLE;

static void
ml_free_list (ddb_medialib_item_t *list);

static uint32_t
hash_for_ptr (void *ptr) {
    // scrambling multiplier from http://vigna.di.unimi.it/ftp/papers/xorshift.pdf
    uint64_t scrambled = 1181783497276652981ULL * (uintptr_t)ptr;
    return (uint32_t)(scrambled & (ML_HASH_SIZE-1));
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

static ml_string_t *
hash_find (ml_string_t **hash, const char *val) {
    uint32_t h = hash_for_ptr ((void *)val);
    return hash_find_for_hashkey(hash, val, h);
}

/// When it is null, it's expected that the bucket will be added, without any associated tracks
static ml_string_t *
hash_add (ml_string_t **hash, const char *val, DB_playItem_t /* nullable */ *it) {
    uint32_t h = hash_for_ptr ((void *)val);
    ml_string_t *s = hash_find_for_hashkey(hash, val, h);
    ml_string_t *retval = NULL;
    if (!s) {
        deadbeef->metacache_add_string (val);
        s = calloc (1, sizeof (ml_string_t));
        s->bucket_next = hash[h];
        s->text = val;
        deadbeef->metacache_add_string (val);
        hash[h] = s;
        retval = s;
    }

    if (!it) {
        return retval;
    }

    ml_collection_item_t *item = calloc (1, sizeof (ml_collection_item_t));
    deadbeef->pl_item_ref (it);
    item->it = it;

    if (s->items_tail) {
        s->items_tail->next = item;
        s->items_tail = item;
    }
    else {
        s->items = s->items_tail = item;
    }

    s->items_count++;

    return retval;
}

static ml_string_t *
ml_reg_col (ml_collection_t *coll, const char /* nonnull */ *c, DB_playItem_t *it) {
    int need_unref = 0;
    ml_string_t *s = hash_add (coll->hash, c, it);
    if (s) {
        if (coll->tail) {
            coll->tail->next = s;
            coll->tail = s;
        }
        else {
            coll->tail = coll->head = s;
        }
        coll->count++;
    }
    if (need_unref) {
        deadbeef->metacache_remove_string (c);
    }
    return s;
}

static void
ml_free_col (ml_collection_t *coll) {
    ml_string_t *s = coll->head;
    while (s) {
        ml_string_t *next = s->next;

        while (s->items) {
            ml_collection_item_t *next = s->items->next;
            deadbeef->pl_item_unref (s->items->it);
            free (s->items);
            s->items = next;
        }

        if (s->text) {
            deadbeef->metacache_remove_string (s->text);
        }
        free (s);
        s = next;
    }
    memset (coll->hash, 0, sizeof (coll->hash));
    coll->head = NULL;
    coll->tail = NULL;
}

// path is relative to root
static void
ml_reg_item_in_folder (ml_tree_node_t *node, const char *path, DB_playItem_t *it) {
    if (*path == 0) {
        // leaf -- add to the node
        ml_collection_item_t *item = calloc (1, sizeof (ml_collection_item_t));
        item->it = it;
        deadbeef->pl_item_ref (it);


        ml_collection_item_t *tail = NULL;
        for (tail = node->items; tail && tail->next; tail = tail->next);
        if (tail) {
            tail->next = item;
        }
        else {
            node->items = item;
        }
        return;
    }

    const char *slash = strchr (path, '/');
    if (!slash) {
        slash = path + strlen(path);
    }

    int len = (int)(slash - path);

    // node -- find existing child node with this name
    for (ml_tree_node_t *c = node->children; c; c = c->next) {
        if (!strncmp (c->text, path, len)) {
            // found, recurse
            path += len + 1;
            ml_reg_item_in_folder (c, path, it);
            return;
        }
    }

    // not found, start new branch
    ml_tree_node_t *n = calloc (1, sizeof (ml_tree_node_t));
    ml_tree_node_t *tail = NULL;
    for (tail = node->children; tail && tail->next; tail = tail->next);
    if (tail) {
        tail->next = n;
    }
    else {
        node->children = n;
    }

    char temp[len+1];
    memcpy (temp, path, len);
    temp[len] = 0;
    path += len + 1;

    n->text = deadbeef->metacache_add_string (temp);
    ml_reg_item_in_folder (n, path, it);
}

static void
ml_free_tree (ml_tree_node_t *node) {
    while (node->children) {
        ml_tree_node_t *next = node->children->next;
        ml_free_tree (node->children);
        node->children = next;
    }

    while (node->items) {
        ml_collection_item_t *next = node->items->next;
        deadbeef->pl_item_unref (node->items->it);
        free (node->items);
        node->items = next;
    }

    if (node->text) {
        deadbeef->metacache_remove_string (node->text);
    }
    free (node);
}

static uintptr_t tid;
static int scanner_terminate;
static int scanner_file_idx = 0;

static void
ml_notify_listeners (int event);

static void
ml_index (ddb_playlist_t *plt);

static void
ml_free_db (void) {
    fprintf (stderr, "clearing index...\n");

    deadbeef->mutex_lock (mutex);
    ml_free_col(&db.albums);
    ml_free_col(&db.artists);
    ml_free_col(&db.genres);
//    ml_free_col(&db.folders);
    ml_free_col(&db.track_uris);

    while (db.folders_tree) {
        ml_tree_node_t *next = db.folders_tree->next;
        ml_free_tree(db.folders_tree);
        db.folders_tree = next;
    }

    while (db.tracks) {
        ml_entry_t *next = db.tracks->next;
        if (db.tracks->title) {
            deadbeef->metacache_remove_string (db.tracks->title);
        }
        if (db.tracks->file) {
            deadbeef->metacache_remove_string (db.tracks->file);
        }
        free (db.tracks);
        db.tracks = next;
    }

    while (db.cached_strings) {
        ml_cached_string_t *next = db.cached_strings->next;
        deadbeef->metacache_remove_string (db.cached_strings->s);
        free (db.cached_strings);
        db.cached_strings = next;
    }

    deadbeef->mutex_unlock (mutex);

    memset (&db, 0, sizeof (db));
}

static json_t *
_ml_get_music_paths () {
    const char *paths = deadbeef->conf_get_str_fast ("medialib.paths", NULL);
    if (!paths) {
        return NULL;
    }
    json_error_t error;
    json_t *json = json_loads (paths, 0, &error);

    return json;
}

// This should be called only on pre-existing ml playlist.
// Subsequent indexing should be done on the fly, using fileadd listener.
static void
ml_index (ddb_playlist_t *plt) {
    ml_free_db();

    fprintf (stderr, "building index...\n");

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);

    ml_entry_t *tail = NULL;

    char folder[PATH_MAX];

    db.folders_tree = calloc (1, sizeof (ml_tree_node_t));
    db.folders_tree->text = deadbeef->metacache_add_string ("");

    int has_unknown_artist = 0;
    int has_unknown_album = 0;
    int has_unknown_genre = 0;

    const char *unknown_artist = deadbeef->metacache_add_string("Unknown Artist");
    const char *unknown_album = deadbeef->metacache_add_string("Unknown Album");
    const char *unknown_genre = deadbeef->metacache_add_string("Unknown Genre");

    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it && !scanner_terminate) {
        ml_entry_t *en = calloc (1, sizeof (ml_entry_t));

        const char *uri = deadbeef->pl_find_meta (it, ":URI");

        const char *title = deadbeef->pl_find_meta (it, "title");
        const char *artist = deadbeef->pl_find_meta (it, "artist");

        if (!artist) {
            artist = unknown_artist;
        }

        if (artist == unknown_artist) {
            has_unknown_artist = 1;
        }

        // find relative uri, or discard from library
        const char *reluri = NULL;
        for (int i = 0; i < json_array_size(musicpaths_json); i++) {
            json_t *data = json_array_get (musicpaths_json, i);
            if (!json_is_string (data)) {
                break;
            }
            const char *musicdir = json_string_value (data);
            if (!strncmp (musicdir, uri, strlen (musicdir))) {
                reluri = uri + strlen (musicdir);
                if (*reluri == '/') {
                    reluri++;
                }
                break;
            }
        }
        if (!reluri) {
            // uri doesn't match musicdir, remove from db
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->plt_remove_item (plt, it);
            deadbeef->pl_item_unref (it);
            it = next;
            continue;
        }
        // Get a combined cached artist/album string
        const char *album = deadbeef->pl_find_meta (it, "album");
        if (!album) {
            has_unknown_album = 1;
        }

        char artistalbum[1000] = "";
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
            .it = it,
        };

        deadbeef->tf_eval (&ctx, artist_album_id_bc, artistalbum, sizeof (artistalbum));
        album = deadbeef->metacache_add_string (artistalbum);

        const char *genre = deadbeef->pl_find_meta (it, "genre");

        if (!genre) {
            genre = unknown_genre;
        }

        if (genre == unknown_genre) {
            has_unknown_genre = 1;
        }

        ml_string_t *alb = ml_reg_col (&db.albums, album, it);

        deadbeef->metacache_remove_string (album);
        album = NULL;

        ml_string_t *art = ml_reg_col (&db.artists, artist, it);
        ml_string_t *gnr = ml_reg_col (&db.genres, genre, it);

        ml_cached_string_t *cs = calloc (1, sizeof (ml_cached_string_t));
        cs->s = deadbeef->metacache_add_string (uri);
        cs->next = db.cached_strings;

        ml_string_t *trkuri = ml_reg_col (&db.track_uris, cs->s, it);

        char *fn = strrchr (reluri, '/');
        ml_string_t *fld = NULL;
        if (fn) {
            memcpy (folder, reluri, fn-reluri);
            folder[fn-reluri] = 0;
            const char *s = deadbeef->metacache_add_string (folder);
            //fld = ml_reg_col (&db.folders, s, it);

            // add to tree
            ml_reg_item_in_folder (db.folders_tree, s, it);

            deadbeef->metacache_remove_string (s);
        }

        // uri and title are not indexed, only a part of track list,
        // that's why they have an extra ref for each entry
        deadbeef->metacache_add_string (uri);
        en->file = uri;
        if (title) {
            deadbeef->metacache_add_string (title);
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
        en->track_uri = trkuri;

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

    // Add unknown artist / album / genre, if necessary
    if (!has_unknown_artist) {
        ml_reg_col (&db.artists, unknown_artist, NULL);
    }
    if (!has_unknown_album) {
        ml_reg_col (&db.albums, unknown_album, NULL);
    }
    if (!has_unknown_genre) {
        ml_reg_col (&db.genres, unknown_genre, NULL);
    }

    deadbeef->metacache_remove_string (unknown_artist);
    deadbeef->metacache_remove_string (unknown_album);
    deadbeef->metacache_remove_string (unknown_genre);

    int nalb = 0;
    int nart = 0;
    int ngnr = 0;
    int nfld = 0;
    ml_string_t *s;
    for (s = db.albums.head; s; s = s->next, nalb++);
    for (s = db.artists.head; s; s = s->next, nart++);
    for (s = db.genres.head; s; s = s->next, ngnr++);
//    for (s = db.folders.head; s; s = s->next, nfld++);
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    fprintf (stderr, "index build time: %f seconds (%d albums, %d artists, %d genres, %d folders)\n", ms / 1000.f, nalb, nart, ngnr, nfld);
}

static void
ml_notify_listeners (int event) {
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (ml_listeners[i]) {
            ml_listeners[i] (event, ml_listeners_userdatas[i]);
        }
    }
}

/// Load and index the current stored medialib playlist
static void
_ml_load_playlist (const char *plpath) {
    struct timeval tm1, tm2;

    _ml_state = DDB_MEDIALIB_STATE_LOADING;
    ml_notify_listeners (DDB_MEDIALIB_EVENT_SCANNER);

    ddb_playlist_t *plt = deadbeef->plt_alloc ("medialib");

    gettimeofday (&tm1, NULL);
    deadbeef->plt_load2 (-1, plt, NULL, plpath, NULL, NULL, NULL);
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "ml playlist load time: %f seconds\n", ms / 1000.f);

    _ml_state = DDB_MEDIALIB_STATE_INDEXING;
    ml_notify_listeners (DDB_MEDIALIB_EVENT_SCANNER);

    deadbeef->mutex_lock (mutex);
    if (ml_playlist) {
        deadbeef->plt_free (ml_playlist);
    }
    ml_playlist = plt;
    if (ml_playlist) {
        ml_index (ml_playlist);
    }
    deadbeef->mutex_unlock (mutex);

    _ml_state = DDB_MEDIALIB_STATE_IDLE;
    ml_notify_listeners (DDB_MEDIALIB_EVENT_CHANGED);
    ml_notify_listeners (DDB_MEDIALIB_EVENT_SCANNER);
}

static void
scanner_thread (void *none) {
    scanner_file_idx = 0;

    char plpath[PATH_MAX];
    snprintf (plpath, sizeof (plpath), "%s/medialib.dbpl", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG));

    _ml_load_playlist(plpath);

//    return; // disable refresh on startup

    struct timeval tm1, tm2;

    _ml_state = DDB_MEDIALIB_STATE_SCANNING;
    ml_notify_listeners (DDB_MEDIALIB_EVENT_SCANNER);
    gettimeofday (&tm1, NULL);

    if (!musicpaths_json) {
        musicpaths_json = _ml_get_music_paths();
    }

    ddb_playlist_t *plt = deadbeef->plt_alloc("medialib");
    // doesn't need to be protected by mutex -- this is only supposed to be accessed on the scan thread
    ml_filter_state.plt = plt;

    for (int i = 0; i < json_array_size(musicpaths_json); i++) {
        json_t *data = json_array_get (musicpaths_json, i);
        if (!json_is_string (data)) {
            break;
        }
        const char *musicdir = json_string_value (data);

        printf ("adding dir: %s\n", musicdir);

        // update & index the cloned playlist
        deadbeef->plt_insert_dir (plt, NULL, musicdir, &scanner_terminate, NULL, NULL);
    }

    ml_filter_state.plt = NULL;

    _ml_state = DDB_MEDIALIB_STATE_INDEXING;
    ml_notify_listeners (DDB_MEDIALIB_EVENT_SCANNER);

    // set current time as timestamp
    time_t timestamp = time(NULL);
    char stimestamp[100];
    snprintf (stimestamp, sizeof (stimestamp), "%lld", (int64_t)timestamp);
    deadbeef->mutex_lock (mutex);
    ddb_playItem_t *it = deadbeef->plt_get_head_item (plt, PL_MAIN);
    while (it) {
        deadbeef->pl_replace_meta (it, ":MEDIALIB_SCAN_TIME", stimestamp);
        ddb_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->mutex_unlock (mutex);


    deadbeef->plt_save (plt, NULL, NULL, plpath, NULL, NULL, NULL);

    ml_notify_listeners (DDB_MEDIALIB_EVENT_CHANGED);

    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "scan time: %f seconds (%d tracks)\n", ms / 1000.f, deadbeef->plt_get_item_count (ml_playlist, PL_MAIN));

    _ml_state = DDB_MEDIALIB_STATE_SAVING;
    ml_notify_listeners (DDB_MEDIALIB_EVENT_SCANNER);

    // update the current ml playlist and index transactionally
    deadbeef->mutex_lock (mutex);
    if (ml_playlist) {
        deadbeef->plt_free (ml_playlist);
    }
    ml_playlist = plt;
    ml_index (ml_playlist);
    deadbeef->mutex_unlock (mutex);

    _ml_state = DDB_MEDIALIB_STATE_IDLE;
    ml_notify_listeners (DDB_MEDIALIB_EVENT_SCANNER);
}

// intention is to skip the files which are already indexed
// how to speed this up:
// first check if a folder exists (early out?)
static int
ml_fileadd_filter (ddb_file_found_data_t *data, void *user_data) {
    int res = 0;

    ml_filter_state_t *state = user_data;

    if (!user_data || data->plt != state->plt || data->is_dir) {
        return 0;
    }

#if FILTER_PERF
    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);
#endif

    time_t mtime = 0;
    struct stat st = {0};
    if (stat (data->filename, &st) == 0) {
        mtime = st.st_mtime;
    }

    deadbeef->mutex_lock (mutex);

    const char *s = deadbeef->metacache_get_string (data->filename);
    if (!s) {
        deadbeef->mutex_unlock (mutex);

        return 0;
    }

    uint32_t hash = hash_for_ptr((void *)s);

    if (!db.filename_hash[hash]) {
        deadbeef->metacache_remove_string (s);
        deadbeef->mutex_unlock (mutex);

        return 0;
    }

    ml_entry_t *en = db.filename_hash[hash];
    while (en) {
        if (en->file == s) {
            res = -1;

            // move from current to new playlist
            ml_string_t *str = hash_find (db.track_uris.hash, s);
            if (str) {
                for (ml_collection_item_t *item = str->items; item; item = item->next) {
                    const char *stimestamp = deadbeef->pl_find_meta (item->it, ":MEDIALIB_SCAN_TIME");
                    if (!stimestamp) {
                        // no scan time
                        deadbeef->mutex_unlock (mutex);
                        return 0;
                    }
                    int64_t timestamp;
                    if (sscanf (stimestamp, "%lld", &timestamp) != 1) {
                        // parse error
                        deadbeef->mutex_unlock (mutex);
                        return 0;
                    }
                    if (timestamp < mtime) {
                        deadbeef->mutex_unlock (mutex);
                        return 0;
                    }
                }

                for (ml_collection_item_t *item = str->items; item; item = item->next) {
                    // Because of cuesheets, the same track may get added multiple times,
                    // since all items reference the same filename.
                    // Check if this track is still in ml_playlist
                    if (deadbeef->plt_get_item_idx (ml_playlist, item->it, PL_MAIN) == -1) {
                        continue;
                    }
                    deadbeef->pl_item_ref (item->it);
                    deadbeef->plt_remove_item (ml_playlist, item->it);

                    ddb_playItem_t *tail = deadbeef->plt_get_tail_item (data->plt, PL_MAIN);

                    deadbeef->plt_insert_item (data->plt, tail, item->it);
                    deadbeef->pl_item_unref (item->it);

                    if (tail) {
                        deadbeef->pl_item_unref (tail);
                    }
                }
            }
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

    deadbeef->metacache_remove_string (s);
    deadbeef->mutex_unlock (mutex);


    return res;
}

static int
ml_connect (void) {
    tid = deadbeef->thread_start_low_priority (scanner_thread, NULL);

#if 0
    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);
    scanner_thread(NULL);
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "whole ml init time: %f seconds\n", ms / 1000.f);
//    exit (0);
#endif
    return 0;
}

static int
ml_start (void) {
    musicpaths_json = _ml_get_music_paths();

    artist_album_bc = deadbeef->tf_compile ("[%album artist% - ]%album%");
    title_bc = deadbeef->tf_compile ("[%tracknumber%. ]%title%");
    artist_album_id_bc = deadbeef->tf_compile ("artist=$if2(%album artist%,Unknown Artist);album=$if2(%album%,Unknown Album)");

    mutex = deadbeef->mutex_create ();
    filter_id = deadbeef->register_fileadd_filter (ml_fileadd_filter, &ml_filter_state);
    return 0;
}

static int
ml_stop (void) {
    if (tid) {
        scanner_terminate = 1;
        printf ("waiting for scanner thread to finish\n");
        deadbeef->thread_join (tid);
        printf ("scanner thread finished\n");
        tid = 0;
    }
    if (filter_id) {
        deadbeef->unregister_fileadd_filter (filter_id);
        filter_id = 0;
    }

    if (ml_playlist) {
        printf ("free medialib database\n");
        deadbeef->plt_free (ml_playlist);
    }

    if (artist_album_bc) {
        deadbeef->tf_free (artist_album_bc);
        artist_album_bc = NULL;
    }

    if (artist_album_id_bc) {
        deadbeef->tf_free (artist_album_id_bc);
        artist_album_id_bc = NULL;
    }

    if (title_bc) {
        deadbeef->tf_free (title_bc);
        title_bc = NULL;
    }

    if (mutex) {
        deadbeef->mutex_free (mutex);
        mutex = 0;
    }

    if (musicpaths_json) {
        json_decref(musicpaths_json);
        musicpaths_json = NULL;
    }

    printf ("medialib cleanup done\n");

    return 0;
}

static int
ml_add_listener (ddb_medialib_listener_t listener, void *user_data) {
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (!ml_listeners[i]) {
            ml_listeners[i] = listener;
            ml_listeners_userdatas[i] = user_data;
            return i;
        }
    }
    return -1;
}

static void
ml_remove_listener (int listener_id) {
    ml_listeners[listener_id] = NULL;
    ml_listeners_userdatas[listener_id] = NULL;
}

static void
get_albums_for_collection_group_by_field (ddb_medialib_item_t *root, ml_collection_t *coll, const char *field, int field_tf, const char /* nonnull */ *default_field_value) {

    default_field_value = deadbeef->metacache_add_string (default_field_value);

    ml_string_t *album = db.albums.head;
    char text[1024];

    char *tf = NULL;
    if (field_tf) {
        tf = deadbeef->tf_compile (field);
    }

    ddb_medialib_item_t *root_tail = NULL;

    for (int i = 0; i < db.albums.count; i++, album = album->next) {
        if (!album->items_count) {
            continue;
        }

        ddb_medialib_item_t *album_item = NULL;
        ddb_medialib_item_t *album_tail = NULL;


        // find the bucket -- a genre or artist
        const char *mc_str_for_track_field = NULL;
        const char *track_field = NULL;
        if (!tf) {
            track_field = deadbeef->pl_find_meta (album->items->it, field);
        }
        else {
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = album->items->it,
            };

            deadbeef->tf_eval (&ctx, tf, text, sizeof (text));
            track_field = mc_str_for_track_field = deadbeef->metacache_add_string (text);
        }

        if (!track_field) {
            track_field = default_field_value;
        }

        // Find the "bucket of this album - e.g. a genre or an artist
        // NOTE: multiple albums may belong to the same bucket
        ml_string_t *s = NULL;
        for (s = coll->head; s; s = s->next) {
            if (track_field == s->text) {
                break;
            }
        }

        // Add all of the album's tracks into that bucket
        ml_collection_item_t *album_coll_item = album->items;
        for (int j = 0; j < album->items_count; j++, album_coll_item = album_coll_item->next) {
            int append = 0;
            if (!s->coll_item) {
                s->coll_item = calloc (1, sizeof (ddb_medialib_item_t));
                s->coll_item->text = deadbeef->metacache_add_string (s->text);
                append = 1;
            }

            DB_playItem_t *it = album_coll_item->it;

            ddb_medialib_item_t *libitem = s->coll_item;

            if (!album_item) {
                album_item = calloc (1, sizeof (ddb_medialib_item_t));
                if (s->coll_item_tail) {
                    s->coll_item_tail->next = album_item;
                    s->coll_item_tail = album_item;
                }
                else {
                    s->coll_item_tail = libitem->children = album_item;
                }

                ddb_tf_context_t ctx = {
                    ._size = sizeof (ddb_tf_context_t),
                    .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                    .it = it,
                };

                deadbeef->tf_eval (&ctx, artist_album_bc, text, sizeof (text));

                album_item->text = deadbeef->metacache_add_string (text);
                libitem->num_children++;
            }

            ddb_medialib_item_t *track_item = calloc(1, sizeof (ddb_medialib_item_t));

            if (album_tail) {
                album_tail->next = track_item;
                album_tail = track_item;
            }
            else {
                album_tail = album_item->children = track_item;
            }
            album_item->num_children++;

            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = it,
            };

            deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

            track_item->text = deadbeef->metacache_add_string (text);
            deadbeef->pl_item_ref (it);
            track_item->track = it;

            if (!libitem->children) {
                ml_free_list (libitem);
                s->coll_item = NULL;
                s->coll_item_tail = NULL;
                continue;
            }

            if (append) {
                if (root_tail) {
                    root_tail->next = libitem;
                    root_tail = libitem;
                }
                else {
                    root_tail = root->children = libitem;
                }
                root->num_children++;
            }
        }

        if (mc_str_for_track_field) {
            deadbeef->metacache_remove_string (mc_str_for_track_field);
        }
    }

    deadbeef->metacache_remove_string (default_field_value);

    if (tf) {
        deadbeef->tf_free (tf);
    }
}

static void
get_list_of_tracks_for_album (ddb_medialib_item_t *libitem, ml_string_t *album) {
    char text[1024];

    ddb_medialib_item_t *album_item = NULL;
    ddb_medialib_item_t *album_tail = NULL;

    ml_collection_item_t *album_coll_item = album->items;
    for (int j = 0; j < album->items_count; j++, album_coll_item = album_coll_item->next) {
        DB_playItem_t *it = album_coll_item->it;
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
            .it = it,
        };

        if (!album_item) {
            album_item = libitem;
            deadbeef->tf_eval (&ctx, artist_album_bc, text, sizeof (text));

            album_item->text = deadbeef->metacache_add_string (text);
        }

        ddb_medialib_item_t *track_item = calloc(1, sizeof (ddb_medialib_item_t));

        if (album_tail) {
            album_tail->next = track_item;
            album_tail = track_item;
        }
        else {
            album_tail = album_item->children = track_item;
        }
        album_item->num_children++;

        deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

        track_item->text = deadbeef->metacache_add_string (text);
        track_item->track = it;
    }
}

static void
get_subfolders_for_folder (ddb_medialib_item_t *folderitem, ml_tree_node_t *folder) {
    if (!folderitem->text) {
        folderitem->text = deadbeef->metacache_add_string (folder->text);
    }

    ddb_medialib_item_t *tail = NULL;
    if (folder->children) {
        for (ml_tree_node_t *c = folder->children; c; c = c->next) {
            ddb_medialib_item_t *subfolder = calloc (1, sizeof (ddb_medialib_item_t));
            get_subfolders_for_folder (subfolder, c);
            if (tail) {
                tail->next = subfolder;
                tail = subfolder;
            }
            else {
                folderitem->children = tail = subfolder;
            }
            folderitem->num_children++;
        }
    }
    if (folder->items) {
        for (ml_collection_item_t *i = folder->items; i; i = i->next) {
            ddb_medialib_item_t *trackitem = calloc (1, sizeof (ddb_medialib_item_t));
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = i->it,
            };
            char text[1000];
            deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

            trackitem->text = deadbeef->metacache_add_string (text);
            trackitem->track = i->it;

            if (tail) {
                tail->next = trackitem;
                tail = trackitem;
            }
            else {
                folderitem->children = tail = trackitem;
            }
            folderitem->num_children++;
        }
    }
}

static ddb_medialib_item_t *
ml_get_list (const char *index) {
    ml_collection_t *coll = NULL;

    enum {album, artist, genre, folder};

    int type = -1;

    if (!strcmp (index, "album")) {
        coll = &db.albums;
        type = album;
    }
    else if (!strcmp (index, "artist")) {
        coll = &db.artists;
        type = artist;
    }
    else if (!strcmp (index, "genre")) {
        coll = &db.genres;
        type = genre;
    }
    else if (!strcmp (index, "folder")) {
        type = folder;
    }
    else {
        return NULL;
    }

    deadbeef->mutex_lock (mutex);

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);

    ddb_medialib_item_t *root = calloc (1, sizeof (ddb_medialib_item_t));
    root->text = deadbeef->metacache_add_string ("All Music");

    // make sure no dangling pointers from previos run
    if (coll) {
        for (ml_string_t *s = coll->head; s; s = s->next) {
            s->coll_item = NULL;
            s->coll_item_tail = NULL;
        }
    }

    if (type == folder) {
        get_subfolders_for_folder(root, db.folders_tree);
    }
    else if (type == artist) {
        // list of albums for artist
        get_albums_for_collection_group_by_field (root, coll, "artist", 0, "Unknown Artist");
    }
    else if (type == genre) {
        // list of albums for genre
        get_albums_for_collection_group_by_field (root, coll, "genre", 0, "Unknown Genre");
    }
    else if (type == album) {
        // list of tracks for album
        ddb_medialib_item_t *tail = NULL;
        ddb_medialib_item_t *parent = root;
        for (ml_string_t *s = coll->head; s; s = s->next) {
            ddb_medialib_item_t *item = calloc (1, sizeof (ddb_medialib_item_t));

            get_list_of_tracks_for_album (item, s);

            if (!item->children) {
                ml_free_list (item);
                continue;
            }

            if (tail) {
                tail->next = item;
                tail = item;
            }
            else {
                tail = parent->children = item;
            }
            parent->num_children++;
        }
    }

    // cleanup
    if (coll) {
        for (ml_string_t *s = coll->head; s; s = s->next) {
            s->coll_item = NULL;
            s->coll_item_tail = NULL;
        }
    }

    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    fprintf (stderr, "tree build time: %f seconds\n", ms / 1000.f);
    deadbeef->mutex_unlock (mutex);

    return root;
}

static void
ml_free_list (ddb_medialib_item_t *list) {
    while (list) {
        ddb_medialib_item_t *next = list->next;
        if (list->children) {
            ml_free_list(list->children);
            list->children = NULL;
        }
        if (list->track) {
            deadbeef->pl_item_unref (list->track);
        }
        if (list->text) {
            deadbeef->metacache_remove_string (list->text);
        }
        free (list);
        list = next;
    }
}

static DB_playItem_t *
ml_find_track (DB_playItem_t *it) {
    char track_uri[PATH_MAX];
    const char *uri = deadbeef->pl_find_meta (it, ":URI");

    const char *subsong = NULL;
    if (deadbeef->pl_get_item_flags (it) & DDB_IS_SUBTRACK) {
        subsong = deadbeef->pl_find_meta (it, ":TRACKNUM");
    }
    printf ("find lib track for key: %s\n", track_uri);
    const char *key = deadbeef->metacache_add_string (track_uri);

    ml_string_t *s = hash_find (db.track_uris.hash, key);

    ml_collection_item_t *item = NULL;
    if (s) {
        // find the one with correct subsong
        item = s->items;
        while (item) {
            if (!subsong) {
                break;
            }
            const char *item_subsong = deadbeef->pl_find_meta (it, ":TRACKNUM");
            if (item_subsong == subsong) {
                break;
            }
            item = item->next;
        }
    }
    if (item) {
        uri = deadbeef->pl_find_meta (item->it, ":URI");
        printf ("Found lib track %p (%s) for input track %p\n", item->it, uri, it);
    }
    else {
        printf ("Track not found in lib: %s\n", key);
    }
    deadbeef->metacache_remove_string (key);

    if (s) {
        deadbeef->pl_item_ref (s->items->it);
        return s->items->it;
    }

    return NULL;
}

static int ml_scanner_state (void) {
    return _ml_state;
}

static ddb_playlist_t *ml_get_playlist (void) {
    deadbeef->mutex_lock (mutex);

    if (ml_playlist) {
        deadbeef->plt_ref (ml_playlist);
    }

    ddb_playlist_t *res = ml_playlist;

    deadbeef->mutex_unlock (mutex);
    return res;
}

static int
ml_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
}

static unsigned
ml_folder_count (void) {
    deadbeef->mutex_lock (mutex);
    unsigned res = (unsigned)json_array_size(musicpaths_json);
    deadbeef->mutex_unlock (mutex);
    return res;
}

static void
ml_folder_for_index (int index, char *folder, size_t size) {
    deadbeef->mutex_lock (mutex);
    json_t *data = json_array_get (musicpaths_json, index);
    *folder = 0;
    if (json_is_string (data)) {
        const char *musicdir = json_string_value (data);
        strncat(folder, musicdir, size);
    }

    deadbeef->mutex_unlock (mutex);
}


// define plugin interface
static ddb_medialib_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.version_major = DDB_MEDIALIB_VERSION_MAJOR,
    .plugin.plugin.version_minor = DDB_MEDIALIB_VERSION_MINOR,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "medialib",
    .plugin.plugin.name = "Media Library",
    .plugin.plugin.descr = "Scans disk for music files and manages them as database",
    .plugin.plugin.copyright = 
        "Media Library plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2020 Alexey Yakovenko\n"
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
    .add_listener = ml_add_listener,
    .remove_listener = ml_remove_listener,
    .get_list = ml_get_list,
    .free_list = ml_free_list,
    .find_track = ml_find_track,
    .scanner_state = ml_scanner_state,
    .playlist = ml_get_playlist,
    .folder_count = ml_folder_count,
    .folder_for_index = ml_folder_for_index,
};

DB_plugin_t *
medialib_load (DB_functions_t *api) {
    deadbeef = api;

    return DB_PLUGIN (&plugin);
}
