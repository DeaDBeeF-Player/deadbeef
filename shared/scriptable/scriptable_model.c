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

#include "scriptable_model.h"
#include <stdlib.h>
#include <string.h>

typedef struct listener_t {
    int64_t source;
    void (*listener) (scriptableModel_t *model, void *user_data);
    void *user_data;
    struct listener_t *next;
} listener_t;

struct scriptableModel_t {
    DB_functions_t *deadbeef;
    scriptableModelAPI_t api;

    char *config_name_active;

    int64_t next_listener_id;
    listener_t *listeners;
};

static char *
_get_active_name (scriptableModel_t *model);

static void
_set_active_name (scriptableModel_t *model, const char *active_name);

static int64_t
_add_listener (scriptableModel_t *model, void (*listener) (scriptableModel_t *model, void *user_data), void *user_data);

static void
_remove_listener (scriptableModel_t *model, int64_t listener);

static void
_notify_listeners (struct scriptableModel_t *model);

scriptableModel_t *
scriptableModelAlloc (void) {
    return calloc (1, sizeof (scriptableModel_t));
}

scriptableModel_t *
scriptableModelInit (scriptableModel_t *model, DB_functions_t *deadbeef, const char *config_name_active) {
    model->deadbeef = deadbeef;
    if (config_name_active != NULL) {
        model->config_name_active = strdup (config_name_active);
    }
    model->api.set_active_name = _set_active_name;
    model->api.get_active_name = _get_active_name;
    model->api.add_listener = _add_listener;
    model->api.remove_listener = _remove_listener;
    return model;
}

void
scriptableModelFree (scriptableModel_t *model) {
    free (model->config_name_active);
    free (model);
}

scriptableModelAPI_t *
scriptableModelGetAPI (scriptableModel_t *model) {
    return &model->api;
}

static char *
_get_active_name (struct scriptableModel_t *model) {
    if (model->config_name_active == NULL) {
        return NULL;
    }
    char *buffer = calloc (1, 100);
    model->deadbeef->conf_get_str (model->config_name_active, "", buffer, 100);
    return buffer;
}

static void
_set_active_name (struct scriptableModel_t *model, const char *active_name) {
    if (model->config_name_active != NULL) {
        model->deadbeef->conf_set_str (model->config_name_active, active_name);
        model->deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
        model->deadbeef->conf_save ();
    }
    _notify_listeners (model);
}

static int64_t
_add_listener (
    struct scriptableModel_t *model,
    void (*listener) (struct scriptableModel_t *model, void *user_data),
    void *user_data) {
    listener_t *l = calloc (1, sizeof (listener_t));
    l->source = ++model->next_listener_id;
    l->listener = listener;
    l->user_data = user_data;
    l->next = model->listeners;
    model->listeners = l;

    return l->source;
}

static void
_remove_listener (struct scriptableModel_t *model, int64_t listener) {
    listener_t *l = model->listeners;
    listener_t *p = NULL;
    while (l != NULL) {
        if (l->source == listener) {
            if (p != NULL) {
                p->next = l->next;
            }
            else {
                model->listeners = l->next;
            }
            free (l);
            return;
        }
        p = l;
        l = l->next;
    }
}

static void
_notify_listeners (struct scriptableModel_t *model) {
    // NOTE: will fail the listener is removed during while being notified
    listener_t *l = model->listeners;
    while (l != NULL) {
        l->listener (model, l->user_data);
        l = l->next;
    }
}
