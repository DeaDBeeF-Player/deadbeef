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

#include <stdlib.h>
#include <string.h>
#include "undo.h"
#include "undobuffer.h"
#include "undomanager.h"

struct undo_item_s;
typedef struct undo_item_s undo_item_t;

struct undo_item_s {
    char *action_name;
    undobuffer_t *undobuffer;
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
    undobuffer_free(item->undobuffer);
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

void
gtkui_undo_append_buffer (undobuffer_t *undobuffer, const char *action_name) {
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
        item->prev = _state.undo_tail;
        if (_state.undo_tail != NULL) {
            _state.undo_tail->next = item;
        }
        _state.undo_tail = item;

        if (_state.undo_head == NULL) {
            _state.undo_head = item;
        }
    }
    if (_state.type == undo) {
        // append to redo list
        item->prev = _state.redo_tail;
        if (_state.redo_tail != NULL) {
            _state.redo_tail->next = item;
        }
        _state.redo_tail = item;

        if (_state.redo_head == NULL) {
            _state.redo_head = item;
        }
    }

}

void
gtkui_perform_undo (void) {
    undo_item_t *item = _state.undo_tail;
    if (item == NULL) {
        return;
    }

    undomanager_t *undomanager = undomanager_shared ();
    undobuffer_t *new_buffer = undomanager_get_buffer (undomanager);

    // pop last undo item and execute in undo mode

    if (item->prev) {
        item->prev->next = NULL;
        _state.undo_tail = item->prev;
    }
    else {
        _state.undo_head = _state.undo_tail = NULL;
    }

    _state.type = undo;
    undobuffer_execute(item->undobuffer, new_buffer);
    undomanager_set_action_name(undomanager, item->action_name);
    undomanager_flush (undomanager);
    _free_item (item);
    _state.type = none;
}

void
gtkui_perform_redo (void) {
    undo_item_t *item = _state.redo_tail;
    if (item == NULL) {
        return;
    }

    undomanager_t *undomanager = undomanager_shared ();
    undobuffer_t *new_buffer = undomanager_get_buffer (undomanager);

    // pop last redo item and execute in redo mode

    if (item->prev) {
        item->prev->next = NULL;
        _state.redo_tail = item->prev;
    }
    else {
        _state.redo_head = _state.redo_tail = NULL;
    }

    _state.type = redo;
    undobuffer_execute(item->undobuffer, new_buffer);
    undomanager_set_action_name(undomanager, item->action_name);
    undomanager_flush (undomanager);
    _free_item (item);
    _state.type = none;
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
