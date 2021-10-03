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

#ifndef medialibstate_h
#define medialibstate_h

// Medialibrary owns the following items with `row_id` in the same namespace:
//   ml_collection_item_t: leaf item, pointing to a track
//   ml_tree_item_t: a folder node when using Folders selector
//   ml_string_t: a string identifying a unique Album, Artist or Genre

// How row_id works:
// A basic autoincrement.
// The IDs should never be reused.
// The IDs are not persisted on disk.
// TODO: when wrap-around occurs, need to clear the state, and reassign row_ids to the whole library.
// TODO: The client also needs to be notified that the existing row_ids in the ddb_medialib_item_t are invalid.

/// This structure owns the selected/expanded state of an item identified by a row_id.
/// When item is unselected / unexpanded -- the state object is deleted
/// When an item with row_id is destroyed -- the state object is deleted
typedef struct ml_collection_item_state_s {
    uint64_t row_id; // a unique ID of the associated ml_collection_item_t
    unsigned selected: 1;
    unsigned expanded: 1;
    struct ml_collection_item_state_s *next;
} ml_collection_item_state_t;

#define ML_COLLECTION_STATE_HASH_SIZE 1024

typedef struct ml_collection_state_s {
    ml_collection_item_state_t *hash[ML_COLLECTION_STATE_HASH_SIZE];
} ml_collection_state_t;

/// Get item state by value.
/// Default is unselected/unexpanded.
ml_collection_item_state_t
ml_item_state_get (ml_collection_state_t *coll_state, uint64_t row_id);

/// Remove item state, accelerated by previously known prev pointer
void
ml_item_state_remove_with_prev (ml_collection_state_t *coll_state, ml_collection_item_state_t *prev, ml_collection_item_state_t *state);

/// Find item state pointer and it's previous neighbour in the list.
ml_collection_item_state_t *
ml_item_state_find (ml_collection_state_t *coll_state, uint64_t row_id, ml_collection_item_state_t **pprev);

/// Remove item state with specified @c row_id
void
ml_item_state_remove(ml_collection_state_t *coll_state, uint64_t row_id);

/// Update item state.
/// Will create or delete the state as necessary.
void
ml_item_state_update (ml_collection_state_t *coll_state, uint64_t row_id, ml_collection_item_state_t *state, ml_collection_item_state_t *prev, int selected, int expanded);

#endif /* medialibstate_h */
