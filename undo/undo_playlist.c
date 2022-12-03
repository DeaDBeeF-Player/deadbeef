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
#include "undo_playlist.h"

typedef struct {
    undo_operation_t _super;
    playlist_t *plt;
    playItem_t **items;
    size_t count;
} undo_operation_append_remove_items_t;

static void
_undo_append_remove_items_deinit(undo_operation_append_remove_items_t *op) {
    for (size_t i = 0; i < op->count; i++) {
        pl_item_unref (op->items[i]);
    }
    plt_unref(op->plt);
    free (op->items);
}

static undo_operation_append_remove_items_t *
_undo_append_remove_items_new(undobuffer_t *undobuffer, playlist_t *plt, playItem_t **items, size_t count) {
    undo_operation_append_remove_items_t *op = calloc (1, sizeof (undo_operation_append_remove_items_t));
    op->items = calloc (count, sizeof (playItem_t *));
    plt_ref(plt);
    op->plt = plt;
    for (size_t i = 0; i < count; i++) {
        op->items[i] = items[i];
        pl_item_ref(items[i]);
    }
    op->_super.deinit = (undo_operation_deinit_fn)_undo_append_remove_items_deinit;
    return op;
}

static void
_undo_perform_append_items(undo_operation_append_remove_items_t *op) {
    for (size_t i = 0; i < op->count; i++) {
        plt_insert_item(op->plt, NULL, op->items[i]);
    }
}

static void
_undo_perform_remove_items(undo_operation_append_remove_items_t *op) {
    for (size_t i = 0; i < op->count; i++) {
        plt_remove_item(op->plt, op->items[i]);
    }
}

void
undo_remove_items(undobuffer_t *undobuffer, playlist_t *plt, playItem_t **items, size_t count) {
    undo_operation_append_remove_items_t *op = _undo_append_remove_items_new(undobuffer, plt, items, count);
    op->_super.perform = (undo_operation_perform_fn)_undo_perform_append_items;
    undobuffer_append_operation(undobuffer, &op->_super);
}

void
undo_append_items(undobuffer_t *undobuffer, playlist_t *plt, playItem_t **items, size_t count) {
    undo_operation_append_remove_items_t *op = _undo_append_remove_items_new(undobuffer, plt, items, count);
    op->_super.perform = (undo_operation_perform_fn)_undo_perform_remove_items;
    undobuffer_append_operation(undobuffer, &op->_super);
}
