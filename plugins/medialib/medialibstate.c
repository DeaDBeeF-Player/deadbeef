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

#include <stdint.h>
#include <stdlib.h>
#include "medialibstate.h"

#pragma mark - ml_collection_item_state_t

ml_collection_item_state_t
ml_item_state_get (ml_collection_state_t *coll_state, uint64_t row_id) {
    __block ml_collection_item_state_t result = {0};
    for (ml_collection_item_state_t *state = coll_state->hash[row_id&(ML_COLLECTION_STATE_HASH_SIZE-1)]; state; state = state->next) {
        if (state->row_id == row_id) {
            result = *state;
            break;
        }
    }
    return result;
}

void
ml_item_state_remove_with_prev (ml_collection_state_t *coll_state, ml_collection_item_state_t *prev, ml_collection_item_state_t *state) {
    uint64_t row_id = state->row_id;
    if (prev == NULL) {
        coll_state->hash[row_id&(ML_COLLECTION_STATE_HASH_SIZE-1)] = state->next;
    }
    else {
        prev->next = state->next;
    }
    free (state);
}

ml_collection_item_state_t *
ml_item_state_find (ml_collection_state_t *coll_state, uint64_t row_id, ml_collection_item_state_t **pprev) {
    ml_collection_item_state_t *prev = NULL;
    for (ml_collection_item_state_t *state = coll_state->hash[row_id&(ML_COLLECTION_STATE_HASH_SIZE-1)]; state; prev = state, state = state->next) {
        if (state->row_id == row_id) {
            *pprev = prev;
            return state;
        }
    }
    return NULL;
}

void
ml_item_state_remove(ml_collection_state_t *coll_state, uint64_t row_id) {
    ml_collection_item_state_t *prev = NULL;
    ml_collection_item_state_t *state = ml_item_state_find(coll_state, row_id, &prev);
    if (state != NULL) {
        ml_item_state_remove_with_prev (coll_state, prev, state);
    }
}

void
ml_item_state_update (ml_collection_state_t *coll_state, uint64_t row_id, ml_collection_item_state_t *state, ml_collection_item_state_t *prev, int selected, int expanded) {
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
        state->row_id = row_id;

        state->next = coll_state->hash[row_id&(ML_COLLECTION_STATE_HASH_SIZE-1)];
        coll_state->hash[row_id&(ML_COLLECTION_STATE_HASH_SIZE-1)] = state;
    }
}

