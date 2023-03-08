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

#ifndef medialibdb_h
#define medialibdb_h

#include <stdint.h>
#include <deadbeef/deadbeef.h>
#include "medialibstate.h"

// This struct represents a leaf node (track) in the tree of a collection.
// It's an optimization to reduce memory footprint -- these nodes are expected
// to represent the bulk of all nodes.
typedef struct ml_collection_track_ref_s {
    uint64_t row_id;

    ddb_playItem_t *it;
    struct ml_collection_track_ref_s *next; // next track in the same node
} ml_collection_track_ref_t;

// A string with associated "collection_items", stored as a hash and as a list.
// Basically, this is the "tree node".
typedef struct ml_collection_tree_node_s {
    uint64_t row_id; // a unique ID of the item, which is valid only during single session (will be different after deadbeef restarts)

    const char *text;
    const char *path;

    ml_collection_track_ref_t *items; // list of all leaf items (tracks) in this node (e.g. all tracks in an album)
    ml_collection_track_ref_t *items_tail; // tail, for fast append
    int items_count; // count of items

    struct ml_collection_tree_node_s *bucket_next; // next node in a hash bucket
    struct ml_collection_tree_node_s *next; // next node in the same parent node

    // to support tree hierarchy
    struct ml_collection_tree_node_s *children;
    struct ml_collection_tree_node_s *children_tail;

    // Note: this is "temporary state" stuff, used when processing a tree query
    struct ml_tree_item_s *coll_item; // The item associated with collection string
    struct ml_tree_item_s *coll_item_tail; // Tail of the children list of coll_item
} ml_collection_tree_node_t;

#define ML_HASH_SIZE 4096

// This is the collection "container" -- that is, item tree with a hash table.
// It's used to store a tree of items, based on certain arbitrary criteria -- e.g. by Albums, or by Folders.
typedef struct {
    ml_collection_tree_node_t *hash[ML_HASH_SIZE]; // hash table, for quick lookup by name (pointer-based hash)
    ml_collection_tree_node_t root;
} ml_collection_t;

typedef struct ml_entry_s {
    const char *file;
    struct ml_entry_s *bucket_next;
} ml_filename_hash_item_t;

typedef struct {
    // A hash formed by filename pointer.
    // This hash purpose is to quickly check whether the filename is in the library already.
    // Doesn't contain subtracks.
    ml_filename_hash_item_t *filename_hash[ML_HASH_SIZE];

    /// Collections (trees) for all supported hierarchies.
    /// Every time the library is updated, the following trees are updated as well.
    ml_collection_t albums;
    ml_collection_t artists;
    ml_collection_t genres;
    ml_collection_t folders;

    // This hash is formed from track_uri (filename),
    // and each node contains all tracks for the given filename.
    // It is used to update the already scanned tracks during rescans.
    ml_collection_t track_uris;

    /// Selected / expanded state.
    /// State is associated with IDs, therefore it survives updates/scans, as long as IDs are reused correctly.
    ml_collection_state_t state;

    /// Current row ID used by the above collections.
    /// Incremented for each new node.
    uint64_t row_id;
} ml_db_t;

uint32_t
ml_collection_hash_for_ptr (void *ptr);

ml_collection_tree_node_t *
ml_collection_hash_find_for_hashkey (ml_collection_tree_node_t **hash, const char *val, uint32_t h);

ml_collection_tree_node_t *
ml_collection_hash_find (ml_collection_tree_node_t **hash, const char *val);

void
ml_collection_reuse_row_ids (ml_collection_t *coll, const char *coll_name, ddb_playItem_t *item, ml_collection_state_t *state, ml_collection_state_t *saved_state, uint64_t *coll_rowid, uint64_t *item_rowid);

void
ml_collection_free (ml_db_t *db, ml_collection_t *coll);

/// Add an item to a node.
/// The parent node will be created if it doesn't exist.
/// Technically, this function implements a subset of the @c ml_collection_add_folder_tree,
/// which only supports hierarchy if depth=1
ml_collection_tree_node_t *
ml_collection_add_item (
                        ml_db_t *db,
                        ml_collection_t *coll,
                        const char /* nonnull */ *parent_node_text,
                        ddb_playItem_t *it,
                        uint64_t coll_row_id,
                        uint64_t item_row_id
                        );


/// Insert an item to node hierarchy.
/// The node hierarchy will be created if it doesn't exist.
/// This functions supports tree hieararchies of arbitrary depth.
void
ml_collection_add_tree_item (
                       ml_db_t *db,
                       ml_db_t *source_db,
                       ml_collection_tree_node_t *node,
                       const char *path,
                       int depth,
                       ddb_playItem_t *it,
                       ml_collection_state_t *state,
                       ml_collection_state_t *saved_state
                       );

void
ml_db_free (ml_db_t *db);

void
ml_db_init (DB_functions_t *_deadbeef);

#endif /* medialibdb_h */

