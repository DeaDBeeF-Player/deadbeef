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
#include "undomanager.h"

struct ddb_undomanager_s {
    ddb_undobuffer_t *buffer;
    char *action_name;
};

static ddb_undomanager_t *_shared;

ddb_undomanager_t *
ddb_undomanager_alloc (void) {
    ddb_undomanager_t *undomanager = calloc (1, sizeof (ddb_undomanager_t));
    undomanager->buffer = ddb_undobuffer_alloc();
    return undomanager;
}

void
ddb_undomanager_free (ddb_undomanager_t *undomanager) {
    if (undomanager == _shared) {
        _shared = NULL;
    }
    free (undomanager->action_name);
    if (undomanager->buffer != NULL) {
        ddb_undobuffer_free(undomanager->buffer);
    }
    free (undomanager);
}

ddb_undomanager_t *
ddb_undomanager_shared (void) {
    return _shared;
}

void
ddb_undomanager_shared_init (ddb_undomanager_t *undomanager) {
    if (undomanager == NULL) {
        _shared = ddb_undomanager_alloc ();
    }
    else {
        _shared = undomanager;
    }
}

ddb_undobuffer_t *
ddb_undomanager_get_buffer (ddb_undomanager_t *undomanager) {
    if (undomanager == NULL) {
        return NULL;
    }
    return undomanager->buffer;
}

ddb_undobuffer_t *
ddb_undomanager_consume_buffer (ddb_undomanager_t *undomanager) {
    ddb_undobuffer_t *buffer = undomanager->buffer;
    undomanager->buffer = ddb_undobuffer_alloc();
    return buffer;
}

void
ddb_undomanager_set_action_name (ddb_undomanager_t *undomanager, const char *name) {
    free (undomanager->action_name);
    undomanager->action_name = name ? strdup (name) : NULL;
}

const char *
ddb_undomanager_get_action_name (ddb_undomanager_t *undomanager) {
    return undomanager->action_name;
}

