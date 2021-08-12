//
//  medialibmanager.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 12/08/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//


#include "../../../deadbeef.h"
#include "../../../gettext.h"
#include "../support.h"
#include "../../medialib/medialib.h"
#include "medialibmanager.h"

extern DB_functions_t *deadbeef;

static ddb_medialib_plugin_t *_plugin;
static ddb_mediasource_source_t _source;

ddb_mediasource_source_t
gtkui_medialib_get_source (void) {
    if (_source != NULL) {
        return _source;
    }

    _plugin = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id("medialib");
    if (_plugin == NULL) {
        return NULL;
    }

    _source = _plugin->plugin.create_source ("deadbeef");
    _plugin->plugin.refresh(_source);
    return _source;
}

void
gtkui_medialib_free (void) {
    if (_source != NULL) {
        _plugin->plugin.free_source (_source);
        _source = NULL;
    }
}

