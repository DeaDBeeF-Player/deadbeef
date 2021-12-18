/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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

#include "../../artwork/artwork.h"
#include "covermanager.h"
#include "gobjcache.h"
#include <dispatch/dispatch.h>

extern DB_functions_t *deadbeef;

#define CACHE_SIZE 50

typedef struct {
    ddb_artwork_plugin_t *plugin;
    gobj_cache_t *cache;
}  covermanager_impl_t;

static covermanager_t *_shared;

static gboolean
_dispatch_on_main_wrapper (void *context) {
    void (^block)(void) = context;
    block ();
    return FALSE;
}

static void
_dispatch_on_main(void (^block)(void)) {
    g_idle_add(_dispatch_on_main_wrapper, block);
}

static void
_settings_did_change_for_track(covermanager_t manager, ddb_playItem_t *track) {
    // FIXME:
//    if (track == NULL) {
//        [self updateDefaultCover];
//        [self resetCache];
//    }
//    else {
//        NSString *hash = [self hashForTrack:track];
//        [self.cachedCovers removeObjectForKey:hash];
//    }
}

static void
_artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    covermanager_t manager = user_data;

    _dispatch_on_main(^{
        if (event == DDB_ARTWORK_SETTINGS_DID_CHANGE) {
            _settings_did_change_for_track (manager, (ddb_playItem_t *)p1);
        }
    });
}


covermanager_t
covermanager_shared(void) {
    if (_shared == NULL) {
        _shared = covermanager_new ();
    }
    return _shared;
}

covermanager_t
covermanager_new(void) {
    covermanager_impl_t *impl = calloc (1, sizeof (covermanager_impl_t));
    impl->plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id("artwork2");
    impl->cache = gobj_cache_new(CACHE_SIZE);

    impl->plugin->add_listener(_artwork_listener, impl);

    return impl;
}

void
covermanager_free (covermanager_t manager) {
    covermanager_impl_t *impl = manager;
    impl->plugin->remove_listener(_artwork_listener, impl);
    gobj_cache_free(impl->cache);
    impl->cache = NULL;
    free(impl);
}

GdkPixbuf *
covermanager_cover_for_track(covermanager_t manager, DB_playItem_t *track, int64_t source_id, covermanager_completion_func_t completion_func, void *user_data) {
    return NULL;
}

GdkPixbuf *
covermanager_create_scaled_image (covermanager_t manager, GdkPixbuf *image, GtkAllocation size) {
    return NULL;
}

GtkAllocation
covermanager_desired_size_for_image_size (covermanager_t manager, GtkAllocation image_size, int album_art_space_width) {
    GtkAllocation a = {0};
    return a;
}
