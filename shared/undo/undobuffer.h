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



#ifndef ddb_undobuffer_h
#define ddb_undobuffer_h

struct ddb_undobuffer_s;

typedef struct ddb_undobuffer_s ddb_undobuffer_t;

struct ddb_undo_operation_s;
typedef void (*ddb_undo_operation_perform_fn)(ddb_undobuffer_t *undobuffer, struct ddb_undo_operation_s *op);
typedef void (*ddb_undo_operation_deinit_fn)(struct ddb_undo_operation_s *op);

typedef struct ddb_undo_operation_s {
    struct ddb_undo_operation_s *next;
    ddb_undo_operation_perform_fn perform;
    ddb_undo_operation_deinit_fn deinit;
} ddb_undo_operation_t;

ddb_undobuffer_t *
ddb_undobuffer_alloc(void);

void
ddb_undobuffer_free (ddb_undobuffer_t *undobuffer);

void
ddb_undobuffer_set_enabled (ddb_undobuffer_t *undobuffer, int enabled);

int
ddb_undobuffer_is_enabled (ddb_undobuffer_t *undobuffer);

void
ddb_undobuffer_group_begin (ddb_undobuffer_t *undobuffer);

void
ddb_undobuffer_group_end (ddb_undobuffer_t *undobuffer);

int
ddb_undobuffer_is_grouping (ddb_undobuffer_t *undobuffer);

ddb_undo_operation_t *
ddb_undobuffer_get_current_operation (ddb_undobuffer_t *undobuffer);

void
ddb_undobuffer_append_operation (ddb_undobuffer_t *undobuffer, ddb_undo_operation_t *op);

/// @param undobuffer the buffer to execute
/// @param current_undobuffer the current buffer to register the resulting undo operations
void
ddb_undobuffer_execute (ddb_undobuffer_t *undobuffer, ddb_undobuffer_t *current_undobuffer);

int
ddb_undobuffer_has_operations(ddb_undobuffer_t *undobuffer);

#endif /* ddb_undobuffer_h */
