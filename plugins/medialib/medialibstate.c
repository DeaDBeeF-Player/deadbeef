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

#include <stdint.h>
#include <stdlib.h>
#include "medialibstate.h"

static DB_functions_t *deadbeef;

#pragma mark - ml_collection_item_state_t

static uint32_t
_hash_for_ptr (const void *ptr) {
    // scrambling multiplier from http://vigna.di.unimi.it/ftp/papers/xorshift.pdf
    uint64_t scrambled = 1181783497276652981ULL * (uintptr_t)ptr;
    return (uint32_t)(scrambled & (ML_COLLECTION_STATE_HASH_SIZE-1));
}

ml_collection_item_state_t
ml_item_state_get (ml_collection_state_t *coll_state, const char *path) {
    __block ml_collection_item_state_t result = {0};
    uint32_t hash = _hash_for_ptr (path);
    for (ml_collection_item_state_t *state = coll_state->hash[hash]; state; state = state->next) {
        if (state->path == path) {
            result = *state;
            break;
        }
    }
    return result;
}

void
ml_item_state_remove_with_prev (ml_collection_state_t *coll_state, ml_collection_item_state_t *prev, ml_collection_item_state_t *state) {
    if (prev == NULL) {
        const char *path = state->path;
        uint32_t hash = _hash_for_ptr (path);
        coll_state->hash[hash] = state->next;
    }
    else {
        prev->next = state->next;
    }
    deadbeef->metacache_remove_string(state->path);
    free (state);
}

ml_collection_item_state_t *
ml_item_state_find (ml_collection_state_t *coll_state, const char *path, ml_collection_item_state_t **pprev) {
    ml_collection_item_state_t *prev = NULL;
    uint32_t hash = _hash_for_ptr (path);
    for (ml_collection_item_state_t *state = coll_state->hash[hash]; state; prev = state, state = state->next) {
        if (state->path == path) {
            if (pprev != NULL) {
                *pprev = prev;
            }
            return state;
        }
    }
    return NULL;
}

void
ml_item_state_remove(ml_collection_state_t *coll_state, const char *path) {
    ml_collection_item_state_t *prev = NULL;
    ml_collection_item_state_t *state = ml_item_state_find(coll_state, path, &prev);
    if (state != NULL) {
        ml_item_state_remove_with_prev (coll_state, prev, state);
    }
}

void
ml_item_state_update (ml_collection_state_t *coll_state, const char *path, ml_collection_item_state_t *state, ml_collection_item_state_t *prev, int selected, int expanded) {
    if (path == NULL) {
        return;
    }
    if (state != NULL) {
        if (!selected && !expanded) {
            ml_item_state_remove_with_prev (coll_state, prev, state);
            return;
        }
        state->selected = selected;
        state->expanded = expanded;
    }
    else if (selected || expanded) {
        state = calloc (1, sizeof (ml_collection_item_state_t));
        state->selected = selected;
        state->expanded = expanded;
        state->path = deadbeef->metacache_add_string(path);

        uint32_t hash = _hash_for_ptr (path);
        state->next = coll_state->hash[hash];
        coll_state->hash[hash] = state;
    }
}

void
ml_item_state_free (ml_collection_state_t *coll_state) {
    for (int i = 0; i < ML_COLLECTION_STATE_HASH_SIZE; i++) {
        for (ml_collection_item_state_t *s = coll_state->hash[i]; s; ) {
            ml_collection_item_state_t *next = s->next;
            deadbeef->metacache_remove_string(s->path);
            free (s);
            s = next;
        }
    }
}

void
ml_item_state_init(DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
}
