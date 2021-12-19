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
#include <Block.h>

extern DB_functions_t *deadbeef;

#define CACHE_SIZE 50

typedef struct {
    ddb_artwork_plugin_t *plugin;
    gobj_cache_t *cache;
    dispatch_queue_t loader_queue;
    char *name_tf;
    char *default_cover_path;
    GdkPixbuf *default_cover;
}  covermanager_impl_t;

typedef struct {
    covermanager_impl_t *impl;
    dispatch_block_t completion_block;
} query_userdata_t;

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

static char *
_cache_key_for_track (covermanager_impl_t *impl, ddb_playItem_t *track)  {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
        .it = track,
    };

    char buffer[PATH_MAX];
    deadbeef->tf_eval (&ctx, impl->name_tf, buffer, sizeof (buffer));
    return strdup (buffer);
}

static void
_update_default_cover (covermanager_impl_t *impl) {
    if (impl->plugin == NULL) {
        return;
    }
    char path[PATH_MAX];
    impl->plugin->default_image_path(path, sizeof(path));

    if (strcmp (path, impl->default_cover_path)) {
        free (impl->default_cover_path);
        impl->default_cover_path = strdup (path);

        impl->default_cover = gdk_pixbuf_new_from_file(path, NULL);
        if (impl->default_cover == NULL) {
            uint32_t color = 0xffffffff;
            GBytes *bytes = g_bytes_new(&color, 4);
            impl->default_cover = gdk_pixbuf_new_from_bytes(bytes, GDK_COLORSPACE_RGB, FALSE, 32, 1, 1, 4);
            g_bytes_unref(bytes);
        }
    }
}

static void
_settings_did_change_for_track(covermanager_t manager, ddb_playItem_t *track) {
    covermanager_impl_t *impl = manager;
    if (track == NULL) {
        _update_default_cover (impl);
        gobj_cache_remove_all(impl->cache);
    }
    else {
        char *key = _cache_key_for_track(impl, track);
        gobj_cache_remove(impl->cache, key);
        free (key);
    }
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

static GdkPixbuf *
_load_image_from_cover(covermanager_impl_t *impl, ddb_cover_info_t *cover) {
    GdkPixbuf *img = NULL;

    if (cover && cover->blob) {
        GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
        gdk_pixbuf_loader_write (loader, (const guchar *)(cover->blob + cover->blob_image_offset), cover->blob_image_size, NULL);
        img = gdk_pixbuf_loader_get_pixbuf (loader);
        g_object_unref(loader);
    }
    if (!img && cover && cover->image_filename) {
        img = gdk_pixbuf_new_from_file(cover->image_filename, NULL);
    }
    if (!img) {
        img = impl->default_cover;
    }
    return img;
}

static void
_add_cover_for_track(covermanager_impl_t *impl, ddb_playItem_t *track, GdkPixbuf *img) {
    char *key = _cache_key_for_track(impl, track);
    gobj_cache_set(impl->cache, key, G_OBJECT(img));
    free (key);
}


static void
_cover_loaded_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    query_userdata_t *user_data = query->user_data;
    covermanager_impl_t *impl = user_data->impl;
    // Load the image on background queue
    dispatch_async(impl->loader_queue, ^{
        GdkPixbuf *img = NULL;

        if (!(query->flags & DDB_ARTWORK_FLAG_CANCELLED)) {
            img = _load_image_from_cover(impl, cover);
        }

        // Update the UI on main queue
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!(query->flags & DDB_ARTWORK_FLAG_CANCELLED)) {
                _add_cover_for_track(impl, query->track, img);
            }
            void (^completionBlock)(GdkPixbuf *) = (void (^)(GdkPixbuf *))user_data->completion_block;
            completionBlock(img);
            Block_release(user_data->completion_block);
            free (user_data);

            // Free the query -- it's fast, so it's OK to free it on main queue
            deadbeef->pl_item_unref (query->track);
            free (query);

            // Release the cover on background queue
            if (cover != NULL) {
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                    impl->plugin->cover_info_release (cover);
                });
            }

        });
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

    if (impl->plugin == NULL) {
        return impl;
    }

    impl->cache = gobj_cache_new(CACHE_SIZE);

    impl->name_tf = deadbeef->tf_compile ("%_path_raw%");

    impl->loader_queue = dispatch_queue_create("CoverManagerLoaderQueue", NULL);

    if (impl->plugin != NULL) {
        impl->plugin->add_listener(_artwork_listener, impl);
    }

    return impl;
}

void
covermanager_free (covermanager_t manager) {
    covermanager_impl_t *impl = manager;
    if (impl->plugin != NULL) {
        impl->plugin->remove_listener(_artwork_listener, impl);
        impl->plugin = NULL;
    }
    if (impl->name_tf != NULL) {
        deadbeef->tf_free (impl->name_tf);
        impl->name_tf = NULL;
    }
    if (impl->cache != NULL) {
        gobj_cache_free(impl->cache);
        impl->cache = NULL;
    }

    free (impl->default_cover_path);
    impl->default_cover_path = NULL;

    if (impl->default_cover) {
        g_object_unref(impl->default_cover);
    }

    free(impl);
}

GdkPixbuf *
covermanager_cover_for_track(covermanager_t manager, DB_playItem_t *track, int64_t source_id, covermanager_completion_block_t completion_block) {
    covermanager_impl_t *impl = manager;

    if (!impl->plugin) {
        completion_block(NULL);
        return NULL;
    }

    char *key = _cache_key_for_track(impl, track);
    GdkPixbuf *cover = GDK_PIXBUF(gobj_cache_get(impl->cache, key));
    free (key);
    key = NULL;

    if (cover != NULL) {
        if (cover == NULL) {
            completion_block(NULL);
        }
        // Callback is not executed if the image is non-nil, to avoid double drawing.
        // The caller must release user data if the returned image is not nil.
        return cover;
    }

    ddb_cover_query_t *query = calloc (sizeof (ddb_cover_query_t), 1);
    query->_size = sizeof (ddb_cover_query_t);
    query->track = track;
    deadbeef->pl_item_ref (track);
    query->source_id = source_id;

    query_userdata_t *data = calloc (1, sizeof (query_userdata_t));
    data->completion_block = (dispatch_block_t)Block_copy(completion_block);
    data->impl = impl;
    query->user_data = (void *)data;

    impl->plugin->cover_get (query, _cover_loaded_callback);

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
