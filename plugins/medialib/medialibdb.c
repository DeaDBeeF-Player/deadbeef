/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <stdlib.h>
#include <string.h>
#include "medialibdb.h"

static DB_functions_t *deadbeef;

uint32_t
hash_for_ptr (void *ptr) {
    // scrambling multiplier from http://vigna.di.unimi.it/ftp/papers/xorshift.pdf
    uint64_t scrambled = 1181783497276652981ULL * (uintptr_t)ptr;
    return (uint32_t)(scrambled & (ML_HASH_SIZE-1));
}

ml_collection_tree_node_t *
hash_find_for_hashkey (ml_collection_tree_node_t **hash, const char *val, uint32_t h) {
    ml_collection_tree_node_t *bucket = hash[h];
    while (bucket) {
        if (bucket->path != NULL) {
            if (bucket->path == val) {
                return bucket;
            }
        }
        else if (bucket->text == val) {
            return bucket;
        }
        bucket = bucket->bucket_next;
    }
    return NULL;
}

ml_collection_tree_node_t *
hash_find (ml_collection_tree_node_t **hash, const char *val) {
    uint32_t h = hash_for_ptr ((void *)val);
    return hash_find_for_hashkey(hash, val, h);
}

static ml_collection_track_ref_t *
_collection_item_alloc (ml_db_t *db, uint64_t use_this_row_id) {
    ml_collection_track_ref_t *item = calloc (1, sizeof (ml_collection_track_ref_t));
    if (use_this_row_id != UINT64_MAX) {
        item->row_id = use_this_row_id;
    }
    else {
        item->row_id = ++db->row_id;
    }
    return item;
}

static void
_collection_item_free (ml_db_t *db, ml_collection_track_ref_t *item) {
    ml_item_state_remove (&db->state, item->row_id);
    free (item);
}

static ml_collection_tree_node_t *
_ml_string_alloc (ml_db_t *db, uint64_t use_this_rowid) {
    ml_collection_tree_node_t *string = calloc (1, sizeof(ml_collection_tree_node_t));
    if (use_this_rowid != UINT64_MAX) {
        string->row_id = use_this_rowid;
    }
    else {
        string->row_id = ++db->row_id;
    }
    return string;
}

static void
_ml_string_free (ml_db_t *db, ml_collection_tree_node_t *s);

static void
_ml_string_deinit(ml_db_t *db, ml_collection_tree_node_t *s) {
    ml_collection_tree_node_t *c = s->children;
    while (c != NULL) {
        ml_collection_tree_node_t *next = c->next;
        _ml_string_free(db, c);
        c = next;
    }
    while (s->items) {
        ml_collection_track_ref_t *next = s->items->next;
        deadbeef->pl_item_unref (s->items->it);
        _collection_item_free (db, s->items);
        s->items = next;
    }

    if (s->text) {
        deadbeef->metacache_remove_string (s->text);
    }
    if (s->path) {
        deadbeef->metacache_remove_string (s->path);
    }
    ml_item_state_remove (&db->state, s->row_id);
    memset (s, 0, sizeof (ml_collection_tree_node_t));
}

static void
_ml_string_free (ml_db_t *db, ml_collection_tree_node_t *s) {
    _ml_string_deinit(db, s);
    free (s);
}

#pragma mark -

/// When it is null, it's expected that the bucket will be added, without any associated tracks
static ml_collection_tree_node_t *
hash_add (ml_db_t *db, ml_collection_tree_node_t **hash, const char *val, ddb_playItem_t /* nullable */ *it, uint64_t coll_row_id, uint64_t item_row_id) {
    uint32_t h = hash_for_ptr ((void *)val);
    ml_collection_tree_node_t *s = hash_find_for_hashkey(hash, val, h);
    ml_collection_tree_node_t *retval = NULL;
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

    ml_collection_track_ref_t *item = _collection_item_alloc (db, item_row_id);
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

ml_collection_tree_node_t *
ml_reg_col (ml_db_t *db, ml_collection_t *coll, const char /* nonnull */ *c, ddb_playItem_t *it, uint64_t coll_row_id, uint64_t item_row_id) {
    int need_unref = 0;
    ml_collection_tree_node_t *s = hash_add (db, coll->hash, c, it, coll_row_id, item_row_id);

    if (s) {
        if (coll->root.children_tail) {
            coll->root.children_tail->next = s;
            coll->root.children_tail = s;
        }
        else {
            coll->root.children_tail = coll->root.children = s;
        }
    }
    if (need_unref) {
        deadbeef->metacache_remove_string (c);
    }
    return s;
}

void
ml_free_col (ml_db_t *db, ml_collection_t *coll) {
    _ml_string_deinit(db, &coll->root);
    memset (coll->hash, 0, sizeof (coll->hash));
}

static const char *
_get_path_component(const char *path, int index, const char **endptr) {
    const char *slash = NULL;
    const char *ptr = path;

    index += 1;
    int curr = 0;

    for (;;) {
        slash = strchr (ptr, '/');
        if (slash == NULL) {
            slash = ptr + strlen(ptr);
        }

        curr += 1;
        if (curr == index || *ptr == 0) {
            break;
        }
        ptr = slash + 1;
    }

    *endptr = slash;
    return ptr;
}

// path is relative to root
void
ml_reg_item_in_folder (
                       ml_db_t *db,
                       ml_db_t *source_db,
                       ml_collection_tree_node_t *node,
                       const char *path,
                       int depth,
                       ddb_playItem_t *it,
                       ml_collection_state_t *state,
                       ml_collection_state_t *saved_state
                       ) {

    const char *end;
    const char *ptr = _get_path_component(path, depth, &end);

    if (*ptr == 0) { // EOL: create leaf
        uint64_t coll_row_id, item_row_id;
        _reuse_row_ids(&source_db->folders, node->path, it, state, saved_state, &coll_row_id, &item_row_id);

        ml_collection_track_ref_t *item = _collection_item_alloc (db, item_row_id);
        item->it = it;
        deadbeef->pl_item_ref (it);

        ml_collection_track_ref_t *tail = NULL;
        for (tail = node->items; tail && tail->next; tail = tail->next);
        if (tail) {
            tail->next = item;
        }
        else {
            node->items = item;
        }
        return;
    }

    // create node path
    char *node_path = malloc (end - path + 1);
    memcpy (node_path, path, end - path);
    node_path[end - path] = 0;
    const char *cached_node_path = deadbeef->metacache_add_string(node_path);
    free (node_path);
    node_path = NULL;

    // check if the node exists
    uint32_t h = hash_for_ptr ((void *)cached_node_path);
    ml_collection_tree_node_t *c = hash_find_for_hashkey(db->folders.hash, cached_node_path, h);
    if (c != NULL) {
        // found, recurse
        ml_reg_item_in_folder (db, source_db, c, path, depth + 1, it, state, saved_state);
        deadbeef->metacache_remove_string(cached_node_path);
        return;
    }

    // create node title (last component in node path)
    char *node_title = malloc (end - ptr + 1);
    memcpy (node_title, ptr, end - ptr);
    node_title[end - ptr] = 0;
    const char *cached_node_title = deadbeef->metacache_add_string(node_title);
    free (node_title);
    node_title = NULL;

    // not found, start new branch

    uint64_t coll_row_id, item_row_id;
    _reuse_row_ids(&source_db->folders, cached_node_path, NULL, state, saved_state, &coll_row_id, &item_row_id);

    ml_collection_tree_node_t *n = _ml_string_alloc(db, coll_row_id);
    ml_collection_tree_node_t *tail = node->children_tail;
    if (tail) {
        tail->next = n;
        node->children_tail = n;
    }
    else {
        node->children = n;
        node->children_tail = n;
    }

    n->text = cached_node_title;
    n->path = cached_node_path;

    n->bucket_next = db->folders.hash[h];
    db->folders.hash[h] = n;

    // recurse
    ml_reg_item_in_folder (db, source_db, n, path, depth + 1, it, state, saved_state);
}

void
ml_db_free (ml_db_t *db) {
    fprintf (stderr, "clearing index...\n");

    // NOTE: Currently this is called from ml_index, which is executed on sync_queue
    ml_free_col(db, &db->albums);
    ml_free_col(db, &db->artists);
    ml_free_col(db, &db->genres);
    ml_free_col(db, &db->track_uris);
    ml_free_col(db, &db->folders);

    for (int i = 0; i < ML_HASH_SIZE; i++) {
        ml_filename_hash_item_t *en = db->filename_hash[i];
        while (en) {
            ml_filename_hash_item_t *next = en->bucket_next;
            if (en->file) {
                deadbeef->metacache_remove_string (en->file);
            }
            free (en);
            en = next;
        }
        db->filename_hash[i] = NULL;
    }

    memset (db, 0, sizeof (ml_db_t));
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
_copy_state_coll (ml_collection_state_t *state, ml_collection_state_t *saved_state, ml_collection_tree_node_t *saved) {
    if (saved == NULL) {
        return;
    }

    _copy_state(state, saved_state, saved->row_id);
}

static void
_copy_state_item (ml_collection_state_t *state, ml_collection_state_t *saved_state, ml_collection_track_ref_t *saved) {
    if (saved == NULL) {
        return;
    }

    _copy_state(state, saved_state, saved->row_id);
}

static ml_collection_track_ref_t *
_find_coll_item (ml_collection_tree_node_t *s, ddb_playItem_t *it) {
    if (s == NULL) {
        return NULL;
    }
    for (ml_collection_track_ref_t *i = s->items; i; i = i->next) {
        if (i->it == it) {
            return i;
        }
    }
    return NULL;
}

void
_reuse_row_ids (ml_collection_t *coll, const char *coll_name, ddb_playItem_t *item, ml_collection_state_t *state, ml_collection_state_t *saved_state, uint64_t *coll_rowid, uint64_t *item_rowid) {
    uint32_t h = hash_for_ptr ((void *)coll_name);
    ml_collection_tree_node_t *saved = hash_find_for_hashkey(coll->hash, coll_name, h);
    _copy_state_coll (state, saved_state, saved);

    *coll_rowid = saved ? saved->row_id : UINT64_MAX;


    if (item != NULL) {
        ml_collection_track_ref_t *saved_it = _find_coll_item(saved, item);
        _copy_state_item (state, saved_state, saved_it);
        *item_rowid = saved_it ? saved_it->row_id : UINT64_MAX;
    }
    else {
        *item_rowid = UINT64_MAX;
    }
}

void
ml_db_init (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
}
