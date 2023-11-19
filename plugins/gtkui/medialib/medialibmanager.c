//
//  medialibmanager.c
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 12/08/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#include <deadbeef/deadbeef.h>
#include "../../../gettext.h"
#include "../support.h"
#include "medialibmanager.h"
#include <stdlib.h>

extern DB_functions_t *deadbeef;

static DB_mediasource_t *_plugin;
static ddb_mediasource_source_t *_source;

ddb_mediasource_source_t *
gtkui_medialib_get_source (void) {
    if (_source != NULL) {
        return _source;
    }

    _plugin = (DB_mediasource_t *)deadbeef->plug_get_for_id ("medialib");
    if (_plugin == NULL) {
        return NULL;
    }

    _source = _plugin->create_source ("deadbeef");
    _plugin->refresh (_source);
    return _source;
}

void
gtkui_medialib_free (void) {
    if (_source != NULL) {
        _plugin->free_source (_source);
        _source = NULL;
    }
}

void
gtkui_medialib_preset_set (const char *preset) {
    deadbeef->conf_set_str ("medialib.preset", preset);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    // FIXME: notify
}

char *
gtkui_medialib_preset_get (void) {
    char *buffer = calloc (1, 100);
    deadbeef->conf_get_str ("medialib.preset", "", buffer, 100);
    return buffer;
}
