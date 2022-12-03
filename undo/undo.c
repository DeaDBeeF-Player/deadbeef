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
#include "undo.h"

struct _undobuffer_s {
    undo_operation_t *operations;
};

undobuffer_t *
undobuffer_alloc(void) {
    return calloc (1, sizeof (undobuffer_t));
}

void
undobuffer_init(undobuffer_t *undobuffer) {
}

static void
_undobuffer_free_operations(undobuffer_t *undobuffer, undo_operation_t **operations) {
    for (undo_operation_t *op = *operations; op;) {
        undo_operation_t *next = op->next;

        if (op->deinit) {
            op->deinit (op);
        }

        free (op);

        op = next;
    }
    *operations = NULL;
}

void
undobuffer_free (undobuffer_t *undobuffer) {
    _undobuffer_free_operations(undobuffer, &undobuffer->operations);
    free (undobuffer);
}

static void
_undobuffer_append_operation(undobuffer_t *undobuffer, undo_operation_t *op, undo_operation_t **operations) {
    op->next = *operations;
    *operations = op;
}

void
undobuffer_append_operation (undobuffer_t *undobuffer, undo_operation_t *op) {
    _undobuffer_append_operation(undobuffer, op, &undobuffer->operations);
}

void
undobuffer_execute (undobuffer_t *undobuffer) {
    for (undo_operation_t *op = undobuffer->operations; op != NULL; op = op->next) {
        if (op->perform != NULL) {
            op->perform(undobuffer, op);
        }
    }
}
