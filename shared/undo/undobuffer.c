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

#include <stdbool.h>
#include <stdlib.h>
#include "undobuffer.h"

struct _ddb_undobuffer_s {
    ddb_undo_operation_t *operations;
    int enabled;
    int grouping;
};

ddb_undobuffer_t *
ddb_undobuffer_alloc(void) {
    ddb_undobuffer_t *undobuffer = calloc (1, sizeof (ddb_undobuffer_t));
    undobuffer->enabled = 1;
    return undobuffer;
}

static void
_undobuffer_free_operations(ddb_undobuffer_t *undobuffer, ddb_undo_operation_t **operations) {
    for (ddb_undo_operation_t *op = *operations; op;) {
        ddb_undo_operation_t *next = op->next;

        if (op->deinit) {
            op->deinit (op);
        }

        free (op);

        op = next;
    }
    *operations = NULL;
}

void
ddb_undobuffer_free (ddb_undobuffer_t *undobuffer) {
    _undobuffer_free_operations(undobuffer, &undobuffer->operations);
    free (undobuffer);
}

void
ddb_undobuffer_set_enabled (ddb_undobuffer_t *undobuffer, int enabled) {
    undobuffer->enabled = enabled;
}

int
ddb_undobuffer_is_enabled (ddb_undobuffer_t *undobuffer) {
    return undobuffer->enabled;
}

static void
_undobuffer_append_operation(ddb_undobuffer_t *undobuffer, ddb_undo_operation_t *op, ddb_undo_operation_t **operations) {
    op->next = *operations;
    *operations = op;
}

void
ddb_undobuffer_append_operation (ddb_undobuffer_t *undobuffer, ddb_undo_operation_t *op) {
    _undobuffer_append_operation(undobuffer, op, &undobuffer->operations);
}

void
ddb_undobuffer_execute (ddb_undobuffer_t *undobuffer, ddb_undobuffer_t *current_undobuffer) {
    for (ddb_undo_operation_t *op = undobuffer->operations; op != NULL; op = op->next) {
        if (op->perform != NULL) {
            op->perform(current_undobuffer, op);
        }
    }
}

int
ddb_undobuffer_has_operations(ddb_undobuffer_t *undobuffer) {
    return undobuffer->operations != NULL;
}

void
ddb_undobuffer_group_begin (ddb_undobuffer_t *undobuffer) {
    undobuffer->grouping = 1;
}

void
ddb_undobuffer_group_end (ddb_undobuffer_t *undobuffer) {
    undobuffer->grouping = 0;
}

int
ddb_undobuffer_is_grouping (ddb_undobuffer_t *undobuffer) {
    return undobuffer->grouping;
}

ddb_undo_operation_t *
ddb_undobuffer_get_current_operation (ddb_undobuffer_t *undobuffer) {
    return undobuffer->grouping ? undobuffer->operations : NULL;
}
