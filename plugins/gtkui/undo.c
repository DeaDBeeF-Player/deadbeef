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
#include "undo.h"
#include "undo/undobuffer.h"
#include "undo/undomanager.h"

extern DB_functions_t *deadbeef;

struct undo_item_s;
typedef struct undo_item_s undo_item_t;

struct undo_item_s {
    char *action_name;
    ddb_undobuffer_t *undobuffer;
    undo_item_t *prev;
    undo_item_t *next;
};

typedef enum {
    none,
    undo,
    redo,
} undo_type_t;

typedef struct {
    undo_item_t *undo_head;
    undo_item_t *undo_tail;

    undo_item_t *redo_head;
    undo_item_t *redo_tail;

    undo_type_t type;
} undo_state_t;

static undo_state_t _state;

static void
_free_item (undo_item_t *item) {
    ddb_undobuffer_free(item->undobuffer);
    free (item->action_name);
    free (item);
}

static void
_free_item_list (undo_item_t *item) {
    while (item != NULL) {
        undo_item_t *next = item->next;
        _free_item (item);
        item = next;
    }
}

void
gtkui_undo_deinit (void) {
    _free_item_list(_state.undo_head);
    _free_item_list(_state.redo_head);
    memset (&_state, 0, sizeof (_state));
}

static void
_append_item (undo_item_t **head, undo_item_t **tail, undo_item_t *item) {
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
_pop_last (undo_item_t **head, undo_item_t **tail) {
    if ((*tail)->prev != NULL) {
        (*tail)->prev->next = NULL;
        (*tail) = (*tail)->prev;
    }
    else {
        *head = *tail = NULL;
    }
}

void
gtkui_undo_append_buffer (ddb_undobuffer_t *undobuffer, const char *action_name) {
    if (_state.type == none) {
        // discard all redo operations
        _free_item_list (_state.redo_head);
        _state.redo_head = _state.redo_tail = NULL;
    }

    undo_item_t *item = calloc (1, sizeof (undo_item_t));
    item->action_name = action_name ? strdup (action_name) : NULL;
    item->undobuffer = undobuffer;

    if (_state.type == none || _state.type == redo) {
        // append to undo list
        _append_item (&_state.undo_head, &_state.undo_tail, item);
    }
    if (_state.type == undo) {
        // append to redo list
        _append_item (&_state.redo_head, &_state.redo_tail, item);
    }

}

static void
_perform_undo_redo (undo_type_t type) {
    undo_item_t **head;
    undo_item_t **tail;

    if (type == undo) {
        head = &_state.undo_head;
        tail = &_state.undo_tail;
    }
    else {
        head = &_state.redo_head;
        tail = &_state.redo_tail;
    }

    undo_item_t *item = *tail;
    if (item == NULL) {
        return;
    }

    ddb_undomanager_t *undomanager = ddb_undomanager_shared ();
    ddb_undobuffer_t *new_buffer = ddb_undomanager_get_buffer (undomanager);

    // pop last undo item and execute in undo mode
    _pop_last (head, tail);

    _state.type = type;
    ddb_undobuffer_execute(item->undobuffer, new_buffer);
    ddb_undomanager_set_action_name(undomanager, item->action_name);
    deadbeef->undo_process();
    _free_item (item);
    _state.type = none;

    deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}

void
gtkui_perform_undo (void) {
    _perform_undo_redo (undo);
}

void
gtkui_perform_redo (void) {
    _perform_undo_redo (redo);
}

int
gtkui_has_undo (void) {
    return _state.undo_tail != NULL;
}

int
gtkui_has_redo (void) {
    return _state.redo_tail != NULL;
}

const char *
gtkui_get_undo_action_name (void) {
    return _state.undo_tail ? _state.undo_tail->action_name : NULL;
}

const char *
gtkui_get_redo_action_name (void) {
    return _state.redo_tail ? _state.redo_tail->action_name : NULL;
}
