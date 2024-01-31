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

extern DB_functions_t *deadbeef;

struct undomanager_s {
    undobuffer_t *buffer;
    char *action_name;
};

static undomanager_t *_shared;

undomanager_t *
undomanager_alloc (void) {
    undomanager_t *undomanager = calloc (1, sizeof (undomanager_t));
    undomanager->buffer = undobuffer_alloc();
    return undomanager;
}

void
undomanager_free (undomanager_t *undomanager) {
    if (undomanager == _shared) {
        _shared = NULL;
    }
    free (undomanager->action_name);
    if (undomanager->buffer != NULL) {
        undobuffer_free(undomanager->buffer);
    }
    free (undomanager);
}

undomanager_t *
undomanager_shared (void) {
    return _shared;
}

void
undomanager_shared_init (void) {
    _shared = undomanager_alloc ();
}

undobuffer_t *
undomanager_get_buffer (undomanager_t *undomanager) {
    if (undomanager == NULL) {
        return NULL;
    }
    return undomanager->buffer;
}

undobuffer_t *
undomanager_consume_buffer (undomanager_t *undomanager) {
    undobuffer_t *buffer = undomanager->buffer;
    undomanager->buffer = undobuffer_alloc();
    return buffer;
}

static DB_plugin_t *
_plug_get_gui (void) {
    struct DB_plugin_s **plugs = deadbeef->plug_get_list ();
    for (int i = 0; plugs[i]; i++) {
        if (plugs[i]->type == DB_PLUGIN_GUI) {
            return plugs[i];
        }
    }
    return NULL;
}

void
undomanager_set_action_name (undomanager_t *undomanager, const char *name) {
    free (undomanager->action_name);
    undomanager->action_name = name ? strdup (name) : NULL;
}

const char *
undomanager_get_action_name (undomanager_t *undomanager) {
    return undomanager->action_name;
}

void
undomanager_flush(undomanager_t *undomanager) {
    DB_plugin_t *ui_plugin = _plug_get_gui ();
    undobuffer_t *undobuffer = undomanager_consume_buffer (undomanager);

    int has_ui_command = ui_plugin != NULL && ui_plugin->command != NULL;

    if (undobuffer_has_operations (undobuffer) && has_ui_command) {
        ui_plugin->command (111, undobuffer, undomanager_get_action_name (undomanager));
    }
    else {
        undobuffer_free (undobuffer);
    }

    undomanager_set_action_name (undomanager, NULL);
}
