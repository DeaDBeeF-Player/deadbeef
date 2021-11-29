/*
    Media Library plugin for DeaDBeeF Player
    Copyright (C) 2009-2021 Alexey Yakovenko

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

#include "../../deadbeef.h"
#include <dispatch/dispatch.h>
#include <jansson.h>
#include <limits.h>
#include "medialib.h"
#include "medialibstate.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#define trace(...) { deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); }

//#define FILTER_PERF 1 // measure / log file add filtering performance

static DB_functions_t *deadbeef;
static ddb_medialib_plugin_t plugin;

static char *artist_album_bc;
static char *artist_album_id_bc;
static char *title_bc;

typedef struct ml_collection_item_s {
    uint64_t row_id;

    DB_playItem_t *it;
    struct ml_collection_item_s *next; // next item in the same collection (albums, artists, ...)
} ml_collection_item_t;

typedef struct ml_medialib_tree_item_s {
    ddb_medialib_item_t item;
    uint64_t row_id; // a unique ID of the associated ml_string_t
} ml_tree_item_t;

typedef struct ml_string_s {
    uint64_t row_id; // a unique ID of the item, which is valid only during single session (will be different after deadbeef restarts)

    const char *text;
    ml_collection_item_t *items;
    int items_count;
    ml_collection_item_t *items_tail;
    struct ml_string_s *bucket_next;
    struct ml_string_s *next;

    ml_tree_item_t *coll_item; // The item associated with collection string, used while building a list
    ml_tree_item_t *coll_item_tail; // Tail of the children list of coll_item, used while building a list
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
    uint64_t row_id;
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

    /// Selected / expanded state
    ml_collection_state_t state;

    uint64_t row_id; // increment for each new ml_collection_item_t
} ml_db_t;

#define MAX_LISTENERS 10

typedef struct medialib_source_s {
    int scanner_terminate;
    dispatch_queue_t scanner_queue;
    dispatch_queue_t sync_queue;

    // The following properties should only be accessed / changed on the sync_queue
    int64_t scanner_current_index;
    int64_t scanner_cancel_index;
    json_t *musicpaths_json;
    int disable_file_operations;

    /// Whether the source is enabled.
    /// Disabled means that the scanner should never run, and that queries should return empty tree.
    /// Only access on sync_queue.
    int enabled;

    ddb_playlist_t *ml_playlist; // this playlist contains the actual data of the media library in plain list
    ml_db_t db; // this is the index, which can be rebuilt from the playlist at any given time
    ddb_medialib_listener_t ml_listeners[MAX_LISTENERS];
    void *ml_listeners_userdatas[MAX_LISTENERS];
    int _ml_state;
    char source_conf_prefix[100];
} medialib_source_t;

typedef struct {
    int64_t scanner_index; // can be compared with source.scanner_current_index and source.scanner_terminate_index
    char **medialib_paths;
    size_t medialib_paths_count;
}  ml_scanner_configuration_t;


typedef struct {
    medialib_source_t *source;
    ddb_playlist_t *plt; // The playlist which gets populated with new tracks during scan
    ddb_playItem_t **tracks; // The reused tracks from the current medialib playlist
    int track_count; // Current count of tracks
    int track_reserved_count; // Reserved / available space for tracks
    ml_db_t db; // The new db, with reused items transferred from source
} scanner_state_t;

typedef enum {
    SEL_ALBUMS = 1,
    SEL_ARTISTS = 2,
    SEL_GENRES = 3,
    SEL_FOLDERS = 4,
    SEL_FILLER = -1UL,
} medialibSelector_t;

static void
ml_free_list (ddb_mediasource_source_t source, ddb_medialib_item_t *list);

static int
ml_fileadd_filter (ddb_file_found_data_t *data, void *user_data);

static void
ml_notify_listeners (medialib_source_t *source, int event);

static void
ml_index (scanner_state_t *scanner, int can_terminate);


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

static ml_collection_item_t *
_collection_item_alloc (ml_db_t *db, uint64_t use_this_row_id) {
    ml_collection_item_t *item = calloc (1, sizeof (ml_collection_item_t));
    if (use_this_row_id != UINT64_MAX) {
        item->row_id = use_this_row_id;
    }
    else {
        item->row_id = ++db->row_id;
    }
    return item;
}

static void
_collection_item_free (ml_db_t *db, ml_collection_item_t *item) {
    ml_item_state_remove (&db->state, item->row_id);
    free (item);
}

static ml_string_t *
_ml_string_alloc (ml_db_t *db, uint64_t use_this_rowid) {
    ml_string_t *string = calloc (1, sizeof(ml_string_t));
    if (use_this_rowid != UINT64_MAX) {
        string->row_id = use_this_rowid;
    }
    else {
        string->row_id = ++db->row_id;
    }
    return string;
}

static void
_ml_string_free (ml_db_t *db, ml_string_t *s) {
    ml_item_state_remove (&db->state, s->row_id);
    free (s);
}


static ml_tree_node_t *
_tree_node_alloc (ml_db_t *db, uint64_t use_this_row_id) {
    ml_tree_node_t *node = calloc (1, sizeof(ml_tree_node_t));
    if (use_this_row_id != UINT64_MAX) {
        node->row_id = use_this_row_id;
    }
    else {
        node->row_id = ++db->row_id;
    }
    return node;
}

static void
_tree_node_free (ml_db_t *db, ml_tree_node_t *node) {
    ml_item_state_remove (&db->state, node->row_id);
    free (node);
}

#pragma mark -

/// When it is null, it's expected that the bucket will be added, without any associated tracks
static ml_string_t *
hash_add (ml_db_t *db, ml_string_t **hash, const char *val, DB_playItem_t /* nullable */ *it, uint64_t coll_row_id, uint64_t item_row_id) {
    uint32_t h = hash_for_ptr ((void *)val);
    ml_string_t *s = hash_find_for_hashkey(hash, val, h);
    ml_string_t *retval = NULL;
    if (!s) {
        deadbeef->metacache_add_string (val);
        s = _ml_string_alloc (db, coll_row_id);
        s->bucket_next = hash[h];
        s->text = val;
        deadbeef->metacache_add_string (val);
        hash[h] = s;
        retval = s;
    }

    if (!it) {
        return retval;
    }

    ml_collection_item_t *item = _collection_item_alloc (db, item_row_id);
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
ml_reg_col (ml_db_t *db, ml_collection_t *coll, const char /* nonnull */ *c, DB_playItem_t *it, uint64_t coll_row_id, uint64_t item_row_id) {
    int need_unref = 0;
    ml_string_t *s = hash_add (db, coll->hash, c, it, coll_row_id, item_row_id);
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
ml_free_col (ml_db_t *db, ml_collection_t *coll) {
    ml_string_t *s = coll->head;
    while (s) {
        ml_string_t *next = s->next;

        while (s->items) {
            ml_collection_item_t *next = s->items->next;
            deadbeef->pl_item_unref (s->items->it);
            _collection_item_free (db, s->items);
            s->items = next;
        }

        if (s->text) {
            deadbeef->metacache_remove_string (s->text);
        }


        _ml_string_free (db, s);
        s = next;
    }
    memset (coll->hash, 0, sizeof (coll->hash));
    coll->head = NULL;
    coll->tail = NULL;
}

// path is relative to root
static void
ml_reg_item_in_folder (ml_db_t *db, ml_tree_node_t *node, const char *path, DB_playItem_t *it, uint64_t use_this_row_id) {
    if (*path == 0) {
        // leaf -- add to the node
        ml_collection_item_t *item = _collection_item_alloc (db, use_this_row_id);
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
    if (len == 0 && !strcmp (path, "/")) {
        len = 1;
    }

    // node -- find existing child node with this name
    for (ml_tree_node_t *c = node->children; c; c = c->next) {
        if (!strncmp (c->text, path, len)) {
            // found, recurse
            path += len + 1;
            ml_reg_item_in_folder (db, c, path, it, use_this_row_id);
            return;
        }
    }

    // not found, start new branch
    ml_tree_node_t *n =  _tree_node_alloc (db, UINT64_MAX); // FIXME
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
    ml_reg_item_in_folder (db, n, path, it, use_this_row_id);
}

static void
ml_free_tree (ml_db_t *db, ml_tree_node_t *node) {
    while (node->children) {
        ml_tree_node_t *next = node->children->next;
        ml_free_tree (db, node->children);
        node->children = next;
    }

    while (node->items) {
        ml_collection_item_t *next = node->items->next;
        deadbeef->pl_item_unref (node->items->it);
        _collection_item_free (db, node->items);
        node->items = next;
    }

    if (node->text) {
        deadbeef->metacache_remove_string (node->text);
    }

    _tree_node_free (db, node);
}

static void
ml_db_free (ml_db_t *db) {
    fprintf (stderr, "clearing index...\n");

    // NOTE: Currently this is called from ml_index, which is executed on sync_queue
    ml_free_col(db, &db->albums);
    ml_free_col(db, &db->artists);
    ml_free_col(db, &db->genres);
    ml_free_col(db, &db->track_uris);

    while (db->folders_tree) {
        ml_tree_node_t *next = db->folders_tree->next;
        ml_free_tree (db, db->folders_tree);
        db->folders_tree = next;
    }

    while (db->tracks) {
        ml_entry_t *next = db->tracks->next;
        if (db->tracks->title) {
            deadbeef->metacache_remove_string (db->tracks->title);
        }
        if (db->tracks->file) {
            deadbeef->metacache_remove_string (db->tracks->file);
        }
        free (db->tracks);
        db->tracks = next;
    }

    while (db->cached_strings) {
        ml_cached_string_t *next = db->cached_strings->next;
        deadbeef->metacache_remove_string (db->cached_strings->s);
        free (db->cached_strings);
        db->cached_strings = next;
    }

    memset (db, 0, sizeof (ml_db_t));
}

static json_t *
_ml_get_music_paths (medialib_source_t *source) {
    char conf_name[200];
    snprintf (conf_name, sizeof (conf_name), "%spaths", source->source_conf_prefix);
    const char *paths = deadbeef->conf_get_str_fast (conf_name, NULL);
    if (!paths) {
        return json_array();
    }
    json_error_t error;
    json_t *json = json_loads (paths, 0, &error);

    return json;
}

static void
_copy_state (ml_collection_state_t *state, ml_collection_state_t *saved_state, uint64_t row_id) {
    ml_collection_item_state_t *item_state = ml_item_state_find(saved_state, row_id, NULL);
    if (item_state == NULL) {
        return;
    }

    ml_collection_item_state_t *prev = NULL;
    ml_collection_item_state_t *dest_state = ml_item_state_find(state, row_id, &prev);
    ml_item_state_update(state, row_id, dest_state, prev, item_state->selected, item_state->expanded);
}

static void
_copy_state_coll (ml_collection_state_t *state, ml_collection_state_t *saved_state, ml_string_t *saved) {
    if (saved == NULL) {
        return;
    }

    _copy_state(state, saved_state, saved->row_id);
}

static void
_copy_state_item (ml_collection_state_t *state, ml_collection_state_t *saved_state, ml_collection_item_t *saved) {
    if (saved == NULL) {
        return;
    }

    _copy_state(state, saved_state, saved->row_id);
}

static ml_collection_item_t *
_find_coll_item (ml_string_t *s, ddb_playItem_t *it) {
    if (s == NULL) {
        return NULL;
    }
    for (ml_collection_item_t *i = s->items; i; i = i->next) {
        if (i->it == it) {
            return i;
        }
    }
    return NULL;
}

static void
_reuse_row_ids (ml_collection_t *coll, const char *coll_name, ddb_playItem_t *item, ml_collection_state_t *state, ml_collection_state_t *saved_state, uint64_t *coll_rowid, uint64_t *item_rowid) {
    uint32_t h = hash_for_ptr ((void *)coll_name);
    ml_string_t *saved = hash_find_for_hashkey(coll->hash, coll_name, h);
    _copy_state_coll (state, saved_state, saved);

    *coll_rowid = saved ? saved->row_id : UINT64_MAX;


    if (item != NULL) {
        ml_collection_item_t *saved_it = _find_coll_item(saved, item);
        _copy_state_item (state, saved_state, saved_it);
        *item_rowid = saved_it ? saved_it->row_id : UINT64_MAX;
    }
    else {
        *item_rowid = UINT64_MAX;
    }
}

// This should be called only on pre-existing ml playlist.
// Subsequent indexing should be done on the fly, using fileadd listener.
static void
ml_index (scanner_state_t *scanner, int can_terminate) {
    fprintf (stderr, "building index...\n");

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);

    ml_entry_t *tail = NULL;

    char folder[PATH_MAX];

    scanner->db.folders_tree = _tree_node_alloc(&scanner->db, UINT64_MAX); // FIXME
    scanner->db.folders_tree->text = deadbeef->metacache_add_string ("");

    int has_unknown_artist = 0;
    int has_unknown_album = 0;
    int has_unknown_genre = 0;

    // NOTE: these are searched by content when creating item trees,
    // so the values must be the same, as the ones that actually get to the collections.
    const char *unknown_artist = deadbeef->metacache_add_string("<?>");
    const char *unknown_album = deadbeef->metacache_add_string("<?>");
    const char *unknown_genre = deadbeef->metacache_add_string("<?>");

    for (int i = 0; i < scanner->track_count && (!can_terminate || !scanner->source->scanner_terminate); i++) {
        ddb_playItem_t *it = scanner->tracks[i];
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
        for (int i = 0; i < json_array_size(scanner->source->musicpaths_json); i++) { // FIXME: these paths should be cached in the scanner state
            json_t *data = json_array_get (scanner->source->musicpaths_json, i);
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
            // uri doesn't match musicdir, skip
            free (en);
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

        uint64_t coll_row_id, item_row_id;
        _reuse_row_ids(&scanner->source->db.albums, album, it, &scanner->db.state, &scanner->source->db.state, &coll_row_id, &item_row_id);
        ml_string_t *alb = ml_reg_col (&scanner->db, &scanner->db.albums, album, it, coll_row_id, item_row_id);

        deadbeef->metacache_remove_string (album);
        album = NULL;

        _reuse_row_ids(&scanner->source->db.artists, artist, it, &scanner->db.state, &scanner->source->db.state, &coll_row_id, &item_row_id);
        ml_string_t *art = ml_reg_col (&scanner->db, &scanner->db.artists, artist, it, coll_row_id, item_row_id);

        _reuse_row_ids(&scanner->source->db.genres, genre, it, &scanner->db.state, &scanner->source->db.state, &coll_row_id, &item_row_id);
        ml_string_t *gnr = ml_reg_col (&scanner->db, &scanner->db.genres, genre, it, coll_row_id, item_row_id);

        ml_cached_string_t *cs = calloc (1, sizeof (ml_cached_string_t));
        cs->s = deadbeef->metacache_add_string (uri);
        cs->next = scanner->db.cached_strings;

        _reuse_row_ids(&scanner->source->db.track_uris, cs->s, it, &scanner->db.state, &scanner->source->db.state, &coll_row_id, &item_row_id);
        ml_string_t *trkuri = ml_reg_col (&scanner->db, &scanner->db.track_uris, cs->s, it, coll_row_id, item_row_id);
        free (cs);
        cs = NULL;

        char *fn = strrchr (reluri, '/');
        ml_string_t *fld = NULL;
        if (fn) {
            memcpy (folder, reluri, fn-reluri);
            folder[fn-reluri] = 0;
        }
        else {
            strcpy (folder, "/");
        }
        const char *s = deadbeef->metacache_add_string (folder);

        // add to tree
        ml_reg_item_in_folder (&scanner->db, scanner->db.folders_tree, s, it, UINT64_MAX); // FIXME

        deadbeef->metacache_remove_string (s);

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
            tail = scanner->db.tracks = en;
        }

        // add to the hash table
        // at this point, we only have unique pointers, and don't need a duplicate check
        uint32_t hash = hash_for_ptr ((void *)en->file);
        en->bucket_next = scanner->db.filename_hash[hash];
        scanner->db.filename_hash[hash] = en;

        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        it = next;
    }

    // Add unknown artist / album / genre, if necessary
    if (!has_unknown_artist) {
        uint64_t coll_row_id, item_row_id;
        _reuse_row_ids(&scanner->source->db.artists, unknown_artist, NULL, &scanner->db.state, &scanner->source->db.state, &coll_row_id, &item_row_id);
        ml_reg_col (&scanner->db, &scanner->db.artists, unknown_artist, NULL, coll_row_id, item_row_id);
    }
    if (!has_unknown_album) {
        uint64_t coll_row_id, item_row_id;
        _reuse_row_ids(&scanner->source->db.albums, unknown_album, NULL, &scanner->db.state, &scanner->source->db.state, &coll_row_id, &item_row_id);
        ml_reg_col (&scanner->db, &scanner->db.albums, unknown_album, NULL, coll_row_id, item_row_id);
    }
    if (!has_unknown_genre) {
        uint64_t coll_row_id, item_row_id;
        _reuse_row_ids(&scanner->source->db.genres, unknown_genre, NULL, &scanner->db.state, &scanner->source->db.state, &coll_row_id, &item_row_id);
        ml_reg_col (&scanner->db, &scanner->db.genres, unknown_genre, NULL, coll_row_id, item_row_id);
    }

    deadbeef->metacache_remove_string (unknown_artist);
    deadbeef->metacache_remove_string (unknown_album);
    deadbeef->metacache_remove_string (unknown_genre);

    int nalb = 0;
    int nart = 0;
    int ngnr = 0;
    int nfld = 0;
    ml_string_t *s;
    for (s = scanner->db.albums.head; s; s = s->next, nalb++);
    for (s = scanner->db.artists.head; s; s = s->next, nart++);
    for (s = scanner->db.genres.head; s; s = s->next, ngnr++);
//    for (s = db.folders.head; s; s = s->next, nfld++);
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    fprintf (stderr, "index build time: %f seconds (%d albums, %d artists, %d genres, %d folders)\n", ms / 1000.f, nalb, nart, ngnr, nfld);
}

static void
ml_notify_listeners (medialib_source_t *source, int event) {
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (source->ml_listeners[i]) {
            source->ml_listeners[i] (event, source->ml_listeners_userdatas[i]);
        }
    }
}

/// Load the current stored medialib playlist
static void
_ml_load_playlist (medialib_source_t *source, const char *plpath) {
    struct timeval tm1, tm2;

    source->_ml_state = DDB_MEDIASOURCE_STATE_LOADING;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);

    ddb_playlist_t *plt = deadbeef->plt_alloc ("medialib");

    gettimeofday (&tm1, NULL);
    if (!source->disable_file_operations) {
        deadbeef->plt_load2 (-1, plt, NULL, plpath, NULL, NULL, NULL);
    }
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "ml playlist load time: %f seconds\n", ms / 1000.f);

    // create scanner state
    __block scanner_state_t scanner;
    memset (&scanner, 0, sizeof (scanner));
    scanner.source = source;
    scanner.track_count = deadbeef->plt_get_item_count (plt, PL_MAIN);
    scanner.tracks = calloc (scanner.track_count, sizeof (ddb_playItem_t *));

    int idx = 0;
    ddb_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        scanner.tracks[idx++] = it;
        it = deadbeef->pl_get_next (it, PL_MAIN);
    }

    dispatch_sync(source->sync_queue, ^{
        ml_index (&scanner, 0);
    });

    // re-add all items (indexing may have removed some!)
    deadbeef->plt_clear(plt);
    ddb_playItem_t *after = NULL;
    for (int i = 0; i < scanner.track_count; i++) {
        if (scanner.tracks[i] != NULL) {
            after = deadbeef->plt_insert_item(plt, after, scanner.tracks[i]);
            deadbeef->pl_item_unref (scanner.tracks[i]);
        }
    }
    free (scanner.tracks);
    scanner.tracks = NULL;

    dispatch_sync(source->sync_queue, ^{
        source->ml_playlist = plt;
        memcpy (&source->db, &scanner.db, sizeof (ml_db_t));
    });

    source->_ml_state = DDB_MEDIASOURCE_STATE_IDLE;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE);
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);
}

// Get a copy of medialib folder paths
static char **
get_medialib_paths (medialib_source_t *source, size_t *medialib_paths_count) {
    if (!source->musicpaths_json) {
        source->musicpaths_json = _ml_get_music_paths(source);
    }

    char **medialib_paths = NULL;
    size_t count = 0;

    count = json_array_size(source->musicpaths_json);
    if (count == 0) {
        return NULL;
    }

    medialib_paths = calloc (sizeof (char *), count);

    for (int i = 0; i < count; i++) {
        json_t *data = json_array_get (source->musicpaths_json, i);
        if (!json_is_string (data)) {
            continue;
        }
        medialib_paths[i] = strdup (json_string_value (data));
    }

    *medialib_paths_count = count;

    return medialib_paths;
}

static void free_medialib_paths (char **medialib_paths, size_t medialib_paths_count) {
    if (medialib_paths) {
        for (int i = 0; i < medialib_paths_count; i++) {
            free (medialib_paths[i]);
        }
    }
    free (medialib_paths);
}

static int
_status_callback (ddb_insert_file_result_t result, const char *fname, void *user_data) {
    return 0;
}

static void
scanner_thread (medialib_source_t *source, ml_scanner_configuration_t conf) {
    struct timeval tm1, tm2;

    source->_ml_state = DDB_MEDIASOURCE_STATE_SCANNING;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);

    __block int reserve_tracks = 0;
    dispatch_sync(source->sync_queue, ^{
        if (source->ml_playlist != NULL) {
            reserve_tracks = deadbeef->plt_get_item_count (source->ml_playlist, PL_MAIN);
        }
    });

    if (reserve_tracks < 1000) {
        reserve_tracks = 1000;
    }

    scanner_state_t scanner = {0};
    scanner.source = source;
    scanner.plt = deadbeef->plt_alloc("medialib");
    scanner.tracks = calloc (reserve_tracks, sizeof (ddb_playItem_t *));
    scanner.track_count = 0;
    scanner.track_reserved_count = reserve_tracks;

    int filter_id = deadbeef->register_fileadd_filter (ml_fileadd_filter, &scanner);
    gettimeofday (&tm1, NULL);

    for (int i = 0; i < conf.medialib_paths_count; i++) {
        const char *musicdir = conf.medialib_paths[i];
        printf ("adding dir: %s\n", musicdir);
        // Create a new playlist, by looking back into the existing playlist.
        // The reusable tracks get moved to the new playlist.
        deadbeef->plt_insert_dir3 (-1, scanner.plt, NULL, musicdir, &source->scanner_terminate, _status_callback, NULL);
    }
    deadbeef->unregister_fileadd_filter (filter_id);

    if (source->scanner_terminate) {
        goto error;
    }

    // move from playlist to the track list
    int plt_track_count = deadbeef->plt_get_item_count (scanner.plt, PL_MAIN);
    if (scanner.track_count + plt_track_count > scanner.track_reserved_count) {
        scanner.track_reserved_count = scanner.track_count + plt_track_count;
        scanner.tracks = realloc(scanner.tracks, scanner.track_reserved_count * sizeof (ddb_playItem_t));
        if (scanner.tracks == NULL) {
            trace ("medialib: failed to allocate memory for tracks\n");
            goto error;
        }
    }

    time_t timestamp = time(NULL);
    char stimestamp[100];
    snprintf (stimestamp, sizeof (stimestamp), "%lld", (int64_t)timestamp);
    ddb_playItem_t *it = deadbeef->plt_get_head_item (scanner.plt, PL_MAIN);
    while (it) {
        deadbeef->pl_replace_meta (it, ":MEDIALIB_SCAN_TIME", stimestamp);
        scanner.tracks[scanner.track_count++] = it;
        it = deadbeef->pl_get_next (it, PL_MAIN);
    }
    deadbeef->plt_unref (scanner.plt);
    scanner.plt = NULL;

    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "scan time: %f seconds (%d tracks)\n", ms / 1000.f, deadbeef->plt_get_item_count (source->ml_playlist, PL_MAIN));

    source->_ml_state = DDB_MEDIASOURCE_STATE_INDEXING;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);

    ml_index(&scanner, 1);
    if (source->scanner_terminate) {
        goto error;
    }

    source->_ml_state = DDB_MEDIASOURCE_STATE_SAVING;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);

    // Create playlist from tracks
    ddb_playlist_t *new_plt = deadbeef->plt_alloc("Medialib Playlist");
    ddb_playItem_t *after = NULL;
    for (int i = 0; i < scanner.track_count; i++) {
        after = deadbeef->plt_insert_item(new_plt, after, scanner.tracks[i]);
        deadbeef->pl_item_unref(scanner.tracks[i]);
        scanner.tracks[i] = NULL;
    }
    free (scanner.tracks);
    scanner.tracks = NULL;

    if (!source->disable_file_operations) {
        char plpath[PATH_MAX];
        snprintf (plpath, sizeof (plpath), "%s/medialib.dbpl", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG));
        deadbeef->plt_save (new_plt, NULL, NULL, plpath, NULL, NULL, NULL);
    }

    dispatch_sync(source->sync_queue, ^{
        deadbeef->plt_unref (source->ml_playlist);
        source->ml_playlist = new_plt;
        ml_db_free(&source->db);
        memcpy (&source->db, &scanner.db, sizeof (ml_db_t));
    });

    free_medialib_paths (conf.medialib_paths, conf.medialib_paths_count);

    source->_ml_state = DDB_MEDIASOURCE_STATE_IDLE;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE);

    return;
error:
    // scanning or indexing has was cancelled, cleanup
    for (int i = 0; i < scanner.track_count; i++) {
        if (scanner.tracks[i] != NULL) {
            deadbeef->pl_item_unref(scanner.tracks[i]);
        }
    }
    free (scanner.tracks);
    scanner.tracks = NULL;

    ml_db_free (&scanner.db);
    memset (&scanner.db, 0, sizeof (ml_db_t));

    if (scanner.plt) {
        deadbeef->plt_unref (scanner.plt);
        scanner.plt = NULL;
    }
    source->_ml_state = DDB_MEDIASOURCE_STATE_IDLE;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);
}

// NOTE: make sure to run on sync_queue
/// Returns 1 for the files which need to be included in the scan, based on their timestamp and metadata
static int
ml_filter_int (ddb_file_found_data_t *data, time_t mtime, scanner_state_t *state) {
    int res = 0;

    const char *s = deadbeef->metacache_get_string (data->filename);
    if (!s) {
        return 0;
    }

    uint32_t hash = hash_for_ptr((void *)s);

    if (!state->source->db.filename_hash[hash]) {
        deadbeef->metacache_remove_string (s);
        return 0;
    }

    ml_entry_t *en = state->source->db.filename_hash[hash];
    while (en) {
        if (en->file == s) {
            res = -1;

            // Copy from medialib playlist into scanner state
            ml_string_t *str = hash_find (state->source->db.track_uris.hash, s);
            if (str) {
                for (ml_collection_item_t *item = str->items; item; item = item->next) {
                    const char *stimestamp = deadbeef->pl_find_meta (item->it, ":MEDIALIB_SCAN_TIME");
                    if (!stimestamp) {
                        // no scan time
                        return 0;
                    }
                    int64_t timestamp;
                    if (sscanf (stimestamp, "%lld", &timestamp) != 1) {
                        // parse error
                        return 0;
                    }
                    if (timestamp < mtime) {
                        return 0;
                    }
                }

                for (ml_collection_item_t *item = str->items; item; item = item->next) {
                    // Because of cuesheets, the same track may get added multiple times,
                    // since all items reference the same filename.
                    // Check if this track is still in ml_playlist
                    int track_found = 0;
                    for (int i = state->track_count-1; i >= 0; i--) {
                        if (state->tracks[i] == item->it) {
                            track_found = 1;
                            break;
                        }
                    }

                    if (track_found) {
                        continue;
                    }

                    deadbeef->pl_item_ref (item->it);

                    // Allocated space precisely matches playlist count, so no check is necessary
                    state->tracks[state->track_count++] = item->it;
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
    return res;
}

// intention is to skip the files which are already indexed
// how to speed this up:
// first check if a folder exists (early out?)
static int
ml_fileadd_filter (ddb_file_found_data_t *data, void *user_data) {
    __block int res = 0;

    scanner_state_t *state = user_data;

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

    medialib_source_t *source = state->source;

    dispatch_sync(source->sync_queue, ^{
        res = ml_filter_int(data, mtime, state);
    });

    return res;
}

static int
ml_connect (void) {
    return 0;
}

static int
ml_start (void) {
    artist_album_bc = deadbeef->tf_compile ("[%album artist% - ]%album%");
    title_bc = deadbeef->tf_compile ("[%tracknumber%. ]%title%");
    artist_album_id_bc = deadbeef->tf_compile ("artist=$if2(%album artist%,Unknown Artist);album=$if2(%album%,Unknown Album)");
    return 0;
}

static int
ml_stop (void) {
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
    printf ("medialib cleanup done\n");

    return 0;
}

static int
ml_add_listener (ddb_mediasource_source_t _source, ddb_medialib_listener_t listener, void *user_data) {
    medialib_source_t *source = (medialib_source_t *)_source;

    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (!source->ml_listeners[i]) {
            source->ml_listeners[i] = listener;
            source->ml_listeners_userdatas[i] = user_data;
            return i;
        }
    }
    return -1;
}

static void
ml_remove_listener (ddb_mediasource_source_t _source, int listener_id) {
    medialib_source_t *source = (medialib_source_t *)_source;

    source->ml_listeners[listener_id] = NULL;
    source->ml_listeners_userdatas[listener_id] = NULL;
}

static int _is_blank_text (const char *track_field) {
    if (!track_field) {
        return 1;
    }
    for (int i = 0; track_field[i]; i++) {
        if (track_field[i] < 0 || track_field[i] > 0x20) {
            return 0;
        }
    }
    return 1;
}

static ml_tree_item_t *
_tree_item_alloc (uint64_t row_id) {
    ml_tree_item_t *item = calloc (1, sizeof (ml_tree_item_t));
    item->row_id = row_id;
    return item;
}

static void
get_albums_for_collection_group_by_field (medialib_source_t *source, ml_tree_item_t *root, ml_collection_t *coll, const char *field, int field_tf, const char /* nonnull */ *default_field_value, int selected) {

    default_field_value = deadbeef->metacache_add_string (default_field_value);

    ml_string_t *album = source->db.albums.head;
    char text[1024];

    char *tf = NULL;
    if (field_tf) {
        tf = deadbeef->tf_compile (field);
    }

    ml_tree_item_t *root_tail = NULL;

    for (int i = 0; i < source->db.albums.count; i++, album = album->next) {
        if (!album->items_count) {
            continue;
        }

        ml_tree_item_t *album_item = NULL;
        ml_tree_item_t *album_tail = NULL;


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

        if (_is_blank_text (track_field)) {
            track_field = default_field_value;
        }

        // Find the bucket of this album - e.g. a genre or an artist
        // NOTE: multiple albums may belong to the same bucket
        ml_string_t *s = NULL;
        for (s = coll->head; s; s = s->next) {
            if (track_field == s->text) {
                break;
            }
        }

        if (s == NULL) {
            if (mc_str_for_track_field) {
                deadbeef->metacache_remove_string (mc_str_for_track_field);
            }
            continue;
        }

        // Add all of the album's tracks into that bucket
        ml_collection_item_t *album_coll_item = album->items;
        for (int j = 0; j < album->items_count; j++, album_coll_item = album_coll_item->next) {
            if (selected && !deadbeef->pl_is_selected (album_coll_item->it)) {
                continue;
            }
            int append = 0;

            if (!s->coll_item) {
                s->coll_item = _tree_item_alloc (s->row_id);
                s->coll_item->item.text = deadbeef->metacache_add_string (s->text);
                append = 1;
            }

            DB_playItem_t *it = album_coll_item->it;

            ml_tree_item_t *libitem = s->coll_item;

            if (!album_item && s->coll_item) {
                album_item = _tree_item_alloc (s->row_id);
                if (s->coll_item_tail) {
                    s->coll_item_tail->item.next = &album_item->item;
                    s->coll_item_tail = album_item;
                }
                else {
                    s->coll_item_tail = album_item;
                    libitem->item.children = &album_item->item;
                }

                ddb_tf_context_t ctx = {
                    ._size = sizeof (ddb_tf_context_t),
                    .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                    .it = it,
                };

                deadbeef->tf_eval (&ctx, artist_album_bc, text, sizeof (text));

                album_item->item.text = deadbeef->metacache_add_string (text);
                libitem->item.num_children++;
            }

            ml_tree_item_t *track_item = calloc(1, sizeof (ml_tree_item_t));

            if (album_tail) {
                album_tail->item.next = &track_item->item;
                album_tail = track_item;
            }
            else {
                album_tail = track_item;
                album_item->item.children = &track_item->item;
            }
            album_item->item.num_children++;

            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = it,
            };

            deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

            track_item->item.text = deadbeef->metacache_add_string (text);
            deadbeef->pl_item_ref (it);
            track_item->item.track = it;

            if (!libitem->item.children) {
                ml_free_list (source, &libitem->item);
                s->coll_item = NULL;
                s->coll_item_tail = NULL;
                continue;
            }

            if (append) {
                if (root_tail) {
                    root_tail->item.next = &libitem->item;
                    root_tail = libitem;
                }
                else {
                    root_tail = libitem;
                    root->item.children = &libitem->item;
                }
                root->item.num_children++;
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
get_list_of_tracks_for_album (ml_tree_item_t *libitem, ml_string_t *album, int selected) {
    char text[1024];

    ml_tree_item_t *album_item = NULL;
    ml_tree_item_t *album_tail = NULL;

    ml_collection_item_t *album_coll_item = album->items;
    for (int j = 0; j < album->items_count; j++, album_coll_item = album_coll_item->next) {
        DB_playItem_t *it = album_coll_item->it;
        if (selected && !deadbeef->pl_is_selected(it)) {
            continue;
        }
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
            .it = it,
        };

        if (!album_item) {
            album_item = libitem;
            deadbeef->tf_eval (&ctx, artist_album_bc, text, sizeof (text));

            if (!_is_blank_text(text)) {
                album_item->item.text = deadbeef->metacache_add_string (text);
            }
            else {
                album_item->item.text = deadbeef->metacache_add_string ("<?>");
            }
        }

        ml_tree_item_t *track_item = _tree_item_alloc(album_coll_item->row_id);

        if (album_tail) {
            album_tail->item.next = &track_item->item;
            album_tail = track_item;
        }
        else {
            album_tail = track_item;
            album_item->item.children = &track_item->item;
        }
        album_item->item.num_children++;

        deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

        track_item->item.text = deadbeef->metacache_add_string (text);
        deadbeef->pl_item_ref (it);
        track_item->item.track = it;
    }
}

static void
get_subfolders_for_folder (ml_tree_item_t *folderitem, ml_tree_node_t *folder, int selected) {
    if (!folderitem->item.text) {
        folderitem->item.text = deadbeef->metacache_add_string (folder->text);
    }

    ml_tree_item_t *tail = NULL;
    if (folder->children) {
        for (ml_tree_node_t *c = folder->children; c; c = c->next) {
            ml_tree_item_t *subfolder = _tree_item_alloc(folder->row_id);
            get_subfolders_for_folder (subfolder, c, selected);
            if (subfolder->item.num_children > 0) {
                if (tail) {
                    tail->item.next = &subfolder->item;
                    tail = subfolder;
                }
                else {
                    folderitem->item.children = &subfolder->item;
                    tail = subfolder;
                }
                folderitem->item.num_children++;
            }
            else {
                ml_free_list (NULL, &subfolder->item);
            }
        }
    }
    if (folder->items) {
        for (ml_collection_item_t *i = folder->items; i; i = i->next) {
            if (selected && !deadbeef->pl_is_selected(i->it)) {
                continue;
            }
            ml_tree_item_t *trackitem = _tree_item_alloc(i->row_id);
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK,
                .it = i->it,
            };
            char text[1000];
            deadbeef->tf_eval (&ctx, title_bc, text, sizeof (text));

            trackitem->item.text = deadbeef->metacache_add_string (text);
            trackitem->item.track = i->it;
            deadbeef->pl_item_ref (i->it);

            if (tail) {
                tail->item.next = &trackitem->item;
                tail = trackitem;
            }
            else {
                folderitem->item.children = &trackitem->item;
                tail = trackitem;
            }
            folderitem->item.num_children++;
        }
    }
}

static ml_tree_item_t *
_create_item_tree_from_collection(ml_collection_t *coll, const char *filter, medialibSelector_t index, medialib_source_t *source) {
    int selected = 0;
    if (filter && source->ml_playlist) {
        deadbeef->plt_search_reset (source->ml_playlist);
        deadbeef->plt_search_process2 (source->ml_playlist, filter, 1);
        selected = 1;
    }

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);

    ml_tree_item_t *root = _tree_item_alloc(0);
    root->item.text = deadbeef->metacache_add_string ("All Music");

    // make sure no dangling pointers from the previous run
    if (coll) {
        for (ml_string_t *s = coll->head; s; s = s->next) {
            s->coll_item = NULL;
            s->coll_item_tail = NULL;
        }
    }

    if (index == SEL_FOLDERS) {
        get_subfolders_for_folder(root, source->db.folders_tree, selected);
    }
    else if (index == SEL_ARTISTS) {
        // list of albums for artist
        get_albums_for_collection_group_by_field (source, root, coll, "artist", 0, "<?>", selected);
    }
    else if (index == SEL_GENRES) {
        // list of albums for genre
        get_albums_for_collection_group_by_field (source, root, coll, "genre", 0, "<?>", selected);
    }
    else if (index == SEL_ALBUMS) {
        // list of tracks for album
        ml_tree_item_t *tail = NULL;
        ml_tree_item_t *parent = root;
        for (ml_string_t *s = coll->head; s; s = s->next) {
            ml_tree_item_t *item = _tree_item_alloc (s->row_id);

            get_list_of_tracks_for_album (item, s, selected);

            if (!item->item.children) {
                ml_free_list (source, &item->item);
                continue;
            }

            if (tail) {
                tail->item.next = &item->item;
                tail = item;
            }
            else {
                tail = item;
                parent->item.children = &item->item;
            }
            parent->item.num_children++;
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
    return root;
}

static ddb_medialib_item_t *
ml_create_item_tree (ddb_mediasource_source_t _source, ddb_mediasource_list_selector_t selector, const char *filter) {
    medialib_source_t *source = (medialib_source_t *)_source;

    __block ml_tree_item_t *root = NULL;

    dispatch_sync(source->sync_queue, ^{
        if (!source->enabled) {
            return;
        }

        ml_collection_t *coll = NULL;

        medialibSelector_t index = (medialibSelector_t)selector;

        switch (index) {
        case SEL_ALBUMS:
            coll = &source->db.albums;
            break;
        case SEL_ARTISTS:
            coll = &source->db.artists;
            break;
        case SEL_GENRES:
            coll = &source->db.genres;
            break;
        case SEL_FOLDERS:
            break;
        default:
            return;
        }

        root = _create_item_tree_from_collection(coll, filter, index, source);
    });

    return &root->item;
}

#pragma mark - Select / Expand

static int
ml_is_tree_item_selected (ddb_mediasource_source_t _source, ddb_medialib_item_t *_item) {
    medialib_source_t *source = (medialib_source_t *)_source;
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    uint64_t row_id = item->row_id;
    __block ml_collection_item_state_t state;
    dispatch_sync(source->sync_queue, ^{
        state = ml_item_state_get (&source->db.state, row_id);
    });
    return state.selected;
}

static void
ml_set_tree_item_selected (ddb_mediasource_source_t _source, ddb_medialib_item_t *_item, int selected) {
    medialib_source_t *source = (medialib_source_t *)_source;
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    uint64_t row_id = item->row_id;
    dispatch_sync(source->sync_queue, ^{
        ml_collection_item_state_t *prev = NULL;
        ml_collection_item_state_t *state = ml_item_state_find (&source->db.state, row_id, &prev);
        int expanded = 0;
        if (state != NULL) {
            expanded = state->expanded;
        }
        ml_item_state_update (&source->db.state, row_id, state, prev, selected, expanded);
    });
}

static int
ml_is_tree_item_expanded (ddb_mediasource_source_t _source, ddb_medialib_item_t *_item) {
    medialib_source_t *source = (medialib_source_t *)_source;
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    uint64_t row_id = item->row_id;
    __block ml_collection_item_state_t state;
    dispatch_sync(source->sync_queue, ^{
        state = ml_item_state_get (&source->db.state, row_id);
    });
    return state.expanded;
}

static void
ml_set_tree_item_expanded (ddb_mediasource_source_t _source, ddb_medialib_item_t *_item, int expanded) {
    medialib_source_t *source = (medialib_source_t *)_source;
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    uint64_t row_id = item->row_id;
    dispatch_sync(source->sync_queue, ^{
        ml_collection_item_state_t *prev = NULL;
        ml_collection_item_state_t *state = ml_item_state_find (&source->db.state, row_id, &prev);
        int selected = 0;
        if (state != NULL) {
            selected = state->selected;
        }
        ml_item_state_update (&source->db.state, row_id, state, prev, selected, expanded);
    });
}

#pragma mark -

static void
ml_free_list (ddb_mediasource_source_t source, ddb_medialib_item_t *list) {
    while (list) {
        ddb_medialib_item_t *next = list->next;
        if (list->children) {
            ml_free_list(source, list->children);
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

#if 0
static DB_playItem_t *
ml_find_track (medialib_source_t *source, DB_playItem_t *it) {
    char track_uri[PATH_MAX];
    const char *uri = deadbeef->pl_find_meta (it, ":URI");

    const char *subsong = NULL;
    if (deadbeef->pl_get_item_flags (it) & DDB_IS_SUBTRACK) {
        subsong = deadbeef->pl_find_meta (it, ":TRACKNUM");
    }
    printf ("find lib track for key: %s\n", track_uri);
    const char *key = deadbeef->metacache_add_string (track_uri);

    ml_string_t *s = hash_find (source->db.track_uris.hash, key);

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
#endif

static ddb_mediasource_state_t ml_scanner_state (ddb_mediasource_source_t _source) {
    medialib_source_t *source = (medialib_source_t *)_source;
    return source->_ml_state;
}

static int
ml_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
}

#pragma mark - folder access

static void
ml_enable_saving(ddb_mediasource_source_t _source, int enable) {
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        source->disable_file_operations = !enable;
    });
}

static unsigned
ml_folder_count (ddb_mediasource_source_t _source) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block unsigned res = 0;
    dispatch_sync(source->sync_queue, ^{
        res = (unsigned)json_array_size(source->musicpaths_json);
    });
    return res;
}

static void
ml_folder_at_index (ddb_mediasource_source_t _source, int index, char *folder, size_t size) {
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        json_t *data = json_array_get (source->musicpaths_json, index);
        *folder = 0;
        if (json_is_string (data)) {
            const char *musicdir = json_string_value (data);
            strncat(folder, musicdir, size);
        }
    });
}

static void
_save_folders_config (medialib_source_t *source) {
    char *dump = json_dumps(source->musicpaths_json, JSON_COMPACT);
    if (dump) {
        char conf_name[200];
        snprintf (conf_name, sizeof (conf_name), "%spaths", source->source_conf_prefix);
        deadbeef->conf_set_str (conf_name, dump);
        free (dump);
        dump = NULL;
        deadbeef->conf_save();
    }
}

static void
ml_set_folders (ddb_mediasource_source_t _source, const char **folders, size_t count) {
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        if (!source->musicpaths_json) {
            source->musicpaths_json = json_array();
        }

        json_array_clear(source->musicpaths_json);
        for (int i = 0; i < count; i++) {
            json_t *value = json_string(folders[i]);
            json_array_append(source->musicpaths_json, value);
            json_decref(value);
        }

        _save_folders_config(source);
    });
}

static char **
ml_get_folders (ddb_mediasource_source_t _source, /* out */ size_t *_count) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block char **folders = NULL;
    __block size_t count = 0;
    dispatch_sync(source->sync_queue, ^{
        count = json_array_size(source->musicpaths_json);
        folders = calloc (count, sizeof (char *));
        for (int i = 0; i < count; i++) {
            json_t *data = json_array_get (source->musicpaths_json, i);
            if (json_is_string (data)) {
                folders[i] = strdup (json_string_value (data));
            }
        }
    });

    *_count = count;
    return folders;
}

static void
ml_free_folders (ddb_mediasource_source_t source, char **folders, size_t count) {
    for (int i = 0; i < count; i++) {
        free (folders[i]);
    }
    free (folders);
}

static void
ml_insert_folder_at_index (ddb_mediasource_source_t _source, const char *folder, int index) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block int notify = 0;
    dispatch_sync(source->sync_queue, ^{
        json_t *value = json_string(folder);
        if (-1 != json_array_insert(source->musicpaths_json, index, value)) {
            notify = 1;
        }
        json_decref(value);
        _save_folders_config(source);
    });
    if (notify) {
        ml_notify_listeners (source, DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE);
    }
}

static void
ml_remove_folder_at_index (ddb_mediasource_source_t _source, int index) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block int notify = 0;
    dispatch_sync(source->sync_queue, ^{
        if (-1 != json_array_remove(source->musicpaths_json, index)) {
            notify = 1;
        }
        _save_folders_config(source);
    });
    if (notify) {
        ml_notify_listeners (source, DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE);
    }
}

static void
ml_append_folder (ddb_mediasource_source_t _source, const char *folder) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block int notify = 0;
    dispatch_sync(source->sync_queue, ^{
        json_t *value = json_string(folder);
        if (-1 != json_array_append(source->musicpaths_json, value)) {
            notify = 1;
        }
        json_decref(value);
        _save_folders_config(source);
    });
    if (notify) {
        ml_notify_listeners (source, DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE);
    }
}

#pragma mark -

static ddb_mediasource_source_t
ml_create_source (const char *source_path) {
    medialib_source_t *source = calloc (1, sizeof (medialib_source_t));
    snprintf (source->source_conf_prefix, sizeof (source->source_conf_prefix), "medialib.%s.", source_path);

    source->musicpaths_json = _ml_get_music_paths(source);

    source->sync_queue = dispatch_queue_create("MediaLibSyncQueue", NULL);
    source->scanner_queue = dispatch_queue_create("MediaLibScanQueue", NULL);

    char conf_name[200];
    snprintf (conf_name, sizeof (conf_name), "%senabled", source->source_conf_prefix);

    source->enabled = deadbeef->conf_get_int (conf_name, 1);

    // load and index the stored playlist
    dispatch_async(source->scanner_queue, ^{
        char plpath[PATH_MAX];
        snprintf (plpath, sizeof (plpath), "%s/medialib.dbpl", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG));
        _ml_load_playlist(source, plpath);
    });

    return (ddb_mediasource_source_t)source;
}

static void
ml_free_source (ddb_mediasource_source_t _source) {
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        source->scanner_terminate = 1;
    });

    printf ("waiting for scanner queue to finish\n");
    dispatch_sync(source->scanner_queue, ^{
    });
    printf ("scanner queue finished\n");

    dispatch_release(source->scanner_queue);
    dispatch_release(source->sync_queue);

    if (source->ml_playlist) {
        printf ("free medialib database\n");
        deadbeef->plt_free (source->ml_playlist);
    }

    if (source->musicpaths_json) {
        json_decref(source->musicpaths_json);
        source->musicpaths_json = NULL;
    }
}

static ddb_mediasource_list_selector_t *
ml_get_selectors (ddb_mediasource_source_t source) {
    static ddb_mediasource_list_selector_t selectors[] = {
        (ddb_mediasource_list_selector_t)SEL_ALBUMS,
        (ddb_mediasource_list_selector_t)SEL_ARTISTS,
        (ddb_mediasource_list_selector_t)SEL_GENRES,
        (ddb_mediasource_list_selector_t)SEL_FOLDERS,
        0
    };
    return selectors;
}

static void ml_free_selectors (ddb_mediasource_source_t source, ddb_mediasource_list_selector_t *selectors) {
    // the list is predefined, nothing to free
}

static const char *
ml_get_name_for_selector (ddb_mediasource_source_t source, ddb_mediasource_list_selector_t selector) {
    medialibSelector_t index = (medialibSelector_t)selector;
    switch (index) {
    case SEL_ALBUMS:
        return "Albums";
    case SEL_ARTISTS:
        return "Artists";
    case SEL_GENRES:
        return "Genres";
    case SEL_FOLDERS:
        return "Folders";
    default:
        break;
    }
    return NULL;
}

static void
ml_set_source_enabled (ddb_mediasource_source_t _source, int enabled) {
    __block int notify = 0;
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        if (source->enabled != enabled) {
            source->enabled = enabled;
            if (!enabled) {
                source->scanner_terminate = 1;
            }
            char conf_name[200];
            snprintf (conf_name, sizeof (conf_name), "%senabled", source->source_conf_prefix);
            deadbeef->conf_set_int(conf_name, enabled);
            deadbeef->conf_save();
            notify = 1;
        }
    });
    if (notify) {
        ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_ENABLED_DID_CHANGE);
        ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE);
    }
}

static int
ml_get_source_enabled (ddb_mediasource_source_t _source) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block int enabled = 0;
    dispatch_sync(source->sync_queue, ^{
        enabled = source->enabled;
    });
    return enabled;
}

static void
ml_refresh (ddb_mediasource_source_t _source) {
    medialib_source_t *source = (medialib_source_t *)_source;

    __block int64_t scanner_current_index = -1;
    dispatch_sync(source->sync_queue, ^{
        // interrupt plt_insert_dir
        source->scanner_terminate = 1;
        // interrupt all queued scanners
        source->scanner_cancel_index = source->scanner_current_index;
        source->scanner_current_index += 1;
        scanner_current_index = source->scanner_current_index;
    });

    dispatch_async(source->scanner_queue, ^{
        __block int enabled = 0;
        __block int cancel = 0;
        dispatch_sync(source->sync_queue, ^{
            if (source->scanner_cancel_index >= scanner_current_index) {
                cancel = 1;
                return;
            }
            source->scanner_terminate = 0;
        });

        if (cancel) {
            return;
        }

        __block ml_scanner_configuration_t conf = {0};
        dispatch_sync(source->sync_queue, ^{
            conf.medialib_paths = get_medialib_paths (source, &conf.medialib_paths_count);
            enabled = source->enabled;
            if (!conf.medialib_paths || !source->enabled) {
                // no paths: early out
                // empty playlist + empty index
                if (!source->ml_playlist) {
                    source->ml_playlist = deadbeef->plt_alloc("medialib");
                }
                deadbeef->plt_clear (source->ml_playlist);
                ml_db_free(&source->db); // TODO: was ml_index -- needs a test
                free_medialib_paths (conf.medialib_paths, conf.medialib_paths_count);
                return;
            }
        });

        if (conf.medialib_paths == NULL || !enabled) {
            // content became empty
            ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE);
            return;
        }

        scanner_thread(source, conf);
    });
}

// define plugin interface
static ddb_medialib_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.version_major = DDB_MEDIALIB_VERSION_MAJOR,
    .plugin.plugin.version_minor = DDB_MEDIALIB_VERSION_MINOR,
    .plugin.plugin.type = DB_PLUGIN_MEDIASOURCE,
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
    .plugin.plugin.message = ml_message,
    .plugin.create_source = ml_create_source,
    .plugin.free_source = ml_free_source,
    .plugin.set_source_enabled = ml_set_source_enabled,
    .plugin.get_source_enabled = ml_get_source_enabled,
    .plugin.refresh = ml_refresh,
    .plugin.get_selectors_list = ml_get_selectors,
    .plugin.free_selectors_list = ml_free_selectors,
    .plugin.selector_name = ml_get_name_for_selector,
    .plugin.add_listener = ml_add_listener,
    .plugin.remove_listener = ml_remove_listener,
    .plugin.create_item_tree = ml_create_item_tree,
    .plugin.is_tree_item_selected = ml_is_tree_item_selected,
    .plugin.set_tree_item_selected = ml_set_tree_item_selected,
    .plugin.is_tree_item_expanded = ml_is_tree_item_expanded,
    .plugin.set_tree_item_expanded = ml_set_tree_item_expanded,
    .plugin.free_item_tree = ml_free_list,
    .plugin.scanner_state = ml_scanner_state,
    .enable_file_operations = ml_enable_saving,
    .folder_count = ml_folder_count,
    .folder_at_index = ml_folder_at_index,
    .set_folders = ml_set_folders,
    .get_folders = ml_get_folders,
    .free_folders = ml_free_folders,
    .insert_folder_at_index = ml_insert_folder_at_index,
    .remove_folder_at_index = ml_remove_folder_at_index,
    .append_folder = ml_append_folder,
};

DB_plugin_t *
medialib_load (DB_functions_t *api) {
    deadbeef = api;

    return DB_PLUGIN (&plugin);
}
