/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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
#include "undo_playlist.h"

#define UNDO_SELECTION_LIST_UNSELECTED 1

// This structure is reused for Insert, Remove and ChangeSelection operations.
// The list of items is required to be contiguous in the target playlist.
// ChangeSelection operations are registered automatically before registering Insert or Remove, only when undobuffer is empty.
typedef struct {
    ddb_undo_operation_t _super;
    playlist_t *plt;
    playItem_t **items;
    playItem_t *insert_position; // insert after this item
    size_t count;
    int current_row[PL_MAX_ITERATORS]; // current row (cursor)

    // UNDO_SELECTION_LIST_UNSELECTED: indicates that the items list represents unselected items for undo selection change operations.
    uint32_t flags;
} undo_operation_item_list_t;

void
undo_change_selection(ddb_undobuffer_t *undobuffer, playlist_t *plt);

static void
_undo_operation_item_list_deinit(undo_operation_item_list_t *op) {
    for (size_t i = 0; i < op->count; i++) {
        pl_item_unref (op->items[i]);
    }
    if (op->insert_position != NULL) {
        pl_item_unref(op->insert_position);
    }
    plt_unref(op->plt);
    free (op->items);
}

static undo_operation_item_list_t *
_undo_operation_item_list_new(ddb_undobuffer_t *undobuffer, playlist_t *plt, playItem_t **items, size_t count, int copy) {
    undo_operation_item_list_t *op = calloc (1, sizeof (undo_operation_item_list_t));
    op->count = count;
    plt_ref(plt);
    op->plt = plt;
    if (copy) {
        op->items = calloc (count, sizeof (playItem_t *));
        for (size_t i = 0; i < count; i++) {
            op->items[i] = items[i];
            pl_item_ref(items[i]);
        }
    }
    else {
        op->items = items;
    }
    op->_super.deinit = (ddb_undo_operation_deinit_fn)_undo_operation_item_list_deinit;
    return op;
}

static void
_undo_perform_remove_items(ddb_undobuffer_t *undobuffer, ddb_undo_operation_t *_op) {
    undo_operation_item_list_t *op = (undo_operation_item_list_t *)_op;
    ddb_undobuffer_group_begin (undobuffer);
    for (size_t i = 0; i < op->count; i++) {
        plt_remove_item(op->plt, op->items[i]);
    }
    ddb_undobuffer_group_end (undobuffer);
}

static void
_undo_perform_insert_items (ddb_undobuffer_t *undobuffer, ddb_undo_operation_t *_op) {
    undo_operation_item_list_t *op = (undo_operation_item_list_t *)_op;
    ddb_undobuffer_group_begin (undobuffer);
    playItem_t *after = op->insert_position;
    for (size_t i = 0; i < op->count; i++) {
        plt_insert_item(op->plt, after, op->items[i]);
        after = op->items[i];
    }
    ddb_undobuffer_group_end (undobuffer);
}

static void
_undo_perform_change_selection (ddb_undobuffer_t *undobuffer, ddb_undo_operation_t *_op) {
    undo_operation_item_list_t *op = (undo_operation_item_list_t *)_op;
    // Restore selection according to the flag.
    if (op->flags & UNDO_SELECTION_LIST_UNSELECTED) {
        plt_select_all(op->plt);
    }
    else {
        plt_deselect_all(op->plt);
    }

    for (size_t i = 0; i < op->count; i++) {
        pl_set_selected(op->items[i], !(op->flags & UNDO_SELECTION_LIST_UNSELECTED));
    }
    memcpy (op->plt->current_row, op->current_row, sizeof (op->current_row));
} 

static int
_undo_operation_prepare(ddb_undobuffer_t *undobuffer, playlist_t *plt) {
    if (!plt->undo_enabled) {
        return 1;
    }

    if (!ddb_undobuffer_has_operations(undobuffer)) {
        undo_change_selection(undobuffer, plt);
    }

    return 0;
}

// The list of items must be contiguous in playlist.
// The list will be batched with the previous operation if possible.
void
undo_remove_items(ddb_undobuffer_t *undobuffer, playlist_t *plt, playItem_t **items, size_t count) {
    if (_undo_operation_prepare (undobuffer, plt) != 0) {
        return;
    }
    // optimization: batching
    undo_operation_item_list_t *op = NULL;
    if (ddb_undobuffer_is_grouping (undobuffer)) {
        ddb_undo_operation_t *baseop = ddb_undobuffer_get_current_operation(undobuffer);
        if (baseop != NULL && baseop->perform == (ddb_undo_operation_perform_fn)_undo_perform_insert_items) {
            op = (undo_operation_item_list_t *)baseop;
        }
    }

    playItem_t *after = items[0]->prev[PL_MAIN];

    if (op == NULL || after != op->insert_position) {
        op = _undo_operation_item_list_new(undobuffer, plt, items, count, 1);
        if (after != NULL) {
            pl_item_ref (after);
            op->insert_position = after;
        }
        op->_super.perform = (ddb_undo_operation_perform_fn)_undo_perform_insert_items;
        ddb_undobuffer_append_operation(undobuffer, &op->_super);
        return;
    }

    op->items = realloc (op->items, (op->count + count) * sizeof (playItem_t *));
    for (size_t i = 0; i < count; i++) {
        op->items[op->count + i] = items[i];
        pl_item_ref(items[i]);
    }
    op->count += count;
}

void
undo_insert_items(ddb_undobuffer_t *undobuffer, playlist_t *plt, playItem_t **items, size_t count) {
    if (_undo_operation_prepare (undobuffer, plt) != 0) {
        return;
    }

    // optimization: batching
    undo_operation_item_list_t *op = NULL;
    if (ddb_undobuffer_is_grouping (undobuffer)) {
        ddb_undo_operation_t *baseop = ddb_undobuffer_get_current_operation(undobuffer);
        if (baseop != NULL && baseop->perform == _undo_perform_remove_items) {
            op = (undo_operation_item_list_t *)baseop;
        }
    }

    if (op == NULL) {
        op = _undo_operation_item_list_new(undobuffer, plt, items, count, 1);
        op->_super.perform = _undo_perform_remove_items;
        ddb_undobuffer_append_operation(undobuffer, &op->_super);
        return;
    }
    op->items = realloc (op->items, (op->count + count) * sizeof (playItem_t *));
    for (size_t i = 0; i < count; i++) {
        op->items[op->count + i] = items[i];
        pl_item_ref(items[i]);
    }
    op->count += count;
}

void
undo_change_selection(ddb_undobuffer_t *undobuffer, playlist_t *plt) {
    if (!plt->undo_enabled) {
        return;
    }

    // Capture a list of all selected or unselected items, whichever is smaller,
    // and indicate which one is captured with a flag.

    const int selcount = plt_getselcount(plt);
    const int unselcount = plt_get_item_count(plt, PL_MAIN) - selcount;

    if (selcount == 0 && unselcount == 0) {
        return;
    }

    uint32_t flags = 0;
    playItem_t **items;
    size_t count = 0;
    if (selcount > unselcount) {
        flags = UNDO_SELECTION_LIST_UNSELECTED;
        count = unselcount;
        items = calloc (count, sizeof (playItem_t *));
    }
    else {
        count = selcount;
        items = calloc (count, sizeof (playItem_t *));
    }
    int index = 0;
    const int picking_selected = (flags & UNDO_SELECTION_LIST_UNSELECTED) == 0;
    for (playItem_t *item = plt->head[PL_MAIN]; item != NULL; item = item->next[PL_MAIN]) {
        if (item->selected == picking_selected) {
            items[index++] = item;
            pl_item_ref (item);
        }
    }

    undo_operation_item_list_t *op = _undo_operation_item_list_new(undobuffer, plt, items, count, 0);
    memcpy(op->current_row, plt->current_row, sizeof (plt->current_row));
    op->flags = flags;

    op->_super.perform = _undo_perform_change_selection;
    ddb_undobuffer_append_operation(undobuffer, &op->_super);
}
