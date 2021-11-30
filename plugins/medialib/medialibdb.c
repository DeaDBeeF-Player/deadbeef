/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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

ml_string_t *
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


ml_tree_node_t *
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

void
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

ml_string_t *
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

void
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
void
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

void
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

void
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

void
ml_db_init (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
}
