/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2023 Oleksiy Yakovenko and other contributors

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

#include <deadbeef/deadbeef.h>
#include <stdlib.h>
#include <string.h>
#include "undostack.h"
#include "undointegration.h"

extern DB_functions_t *deadbeef;

struct undostack_item_s;
typedef struct undostack_item_s undostack_item_t;

struct undostack_item_s {
    char *action_name;
    struct ddb_undobuffer_s *undobuffer;
    undostack_item_t *prev;
    undostack_item_t *next;
};

typedef enum {
    undostack_operation_type_none,
    undostack_operation_type_undo,
    undostack_operation_type_redo,
} undostack_operation_type_t;

typedef struct {
    undostack_item_t *undo_head;
    undostack_item_t *undo_tail;

    undostack_item_t *redo_head;
    undostack_item_t *redo_tail;

    undostack_operation_type_t type;
} undostack_state_t;

static undostack_state_t _state;

static void
_free_item (undostack_item_t *item) {
    ddb_undo->free_buffer (item->undobuffer);
    free (item->action_name);
    free (item);
}

static void
_free_item_list (undostack_item_t *item) {
    while (item != NULL) {
        undostack_item_t *next = item->next;
        _free_item (item);
        item = next;
    }
}

static void
_append_item (undostack_item_t **head, undostack_item_t **tail, undostack_item_t *item) {
    item->prev = *tail;
    if (*tail != NULL) {
        (*tail)->next = item;
    }
    *tail = item;

    if (*head == NULL) {
        *head = item;
    }
}

static void
_pop_last (undostack_item_t **head, undostack_item_t **tail) {
    if ((*tail)->prev != NULL) {
        (*tail)->prev->next = NULL;
        (*tail) = (*tail)->prev;
    }
    else {
        *head = *tail = NULL;
    }
}

static void
_perform_undo_redo (undostack_operation_type_t type) {
    undostack_item_t **head;
    undostack_item_t **tail;

    if (type == undostack_operation_type_undo) {
        head = &_state.undo_head;
        tail = &_state.undo_tail;
    }
    else {
        head = &_state.redo_head;
        tail = &_state.redo_tail;
    }

    undostack_item_t *item = *tail;
    if (item == NULL) {
        return;
    }

    // pop last undo item and execute in undo mode
    _pop_last (head, tail);

    _state.type = type;
    ddb_undo->execute_buffer (item->undobuffer);
    ddb_undo->set_action_name (item->action_name);
    deadbeef->undo_process();
    _free_item (item);
    _state.type = undostack_operation_type_none;

    // FIXME: This would call undo_process again, which we don't want,
    // but we do need to call it here to refresh playlist.
    deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}

void
gtkui_undostack_deinit (void) {
    _free_item_list(_state.undo_head);
    _free_item_list(_state.redo_head);
    memset (&_state, 0, sizeof (_state));
}

void
gtkui_undostack_append_buffer (struct ddb_undobuffer_s *undobuffer, const char *action_name) {
    if (_state.type == undostack_operation_type_none) {
        // discard all redo operations
        _free_item_list (_state.redo_head);
        _state.redo_head = _state.redo_tail = NULL;
    }

    undostack_item_t *item = calloc (1, sizeof (undostack_item_t));
    item->action_name = action_name ? strdup (action_name) : NULL;
    item->undobuffer = undobuffer;

    if (_state.type == undostack_operation_type_none || _state.type == undostack_operation_type_redo) {
        // append to undo list
        _append_item (&_state.undo_head, &_state.undo_tail, item);
    }
    if (_state.type == undostack_operation_type_undo) {
        // append to redo list
        _append_item (&_state.redo_head, &_state.redo_tail, item);
    }

}

void
gtkui_undostack_perform_undo (void) {
    _perform_undo_redo (undostack_operation_type_undo);
}

void
gtkui_undostack_perform_redo (void) {
    _perform_undo_redo (undostack_operation_type_redo);
}

int
gtkui_undostack_has_undo (void) {
    return _state.undo_tail != NULL;
}

int
gtkui_undostack_has_redo (void) {
    return _state.redo_tail != NULL;
}

const char *
gtkui_undostack_get_undo_action_name (void) {
    return _state.undo_tail ? _state.undo_tail->action_name : NULL;
}

const char *
gtkui_undostack_get_redo_action_name (void) {
    return _state.redo_tail ? _state.redo_tail->action_name : NULL;
}
