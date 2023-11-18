/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <string.h>
#include <stdlib.h>
#include <deadbeef/deadbeef.h>
#include "../../artwork/artwork.h"
#include "covermanager.h"
#include "gobjcache.h"
#include <Block.h>
#include "gtkui.h"

#define min(x, y) ((x) < (y) ? (x) : (y))
#define MAX_ALBUM_ART_FILE_SIZE (40 * 1024 * 1024)

extern DB_functions_t *deadbeef;

#define CACHE_SIZE 50

struct covermanager_s {
    ddb_artwork_plugin_t *plugin;
    gobj_cache_t *cache;
    dispatch_queue_t loader_queue;
    char *name_tf;
    char *default_cover_path;
    GdkPixbuf *default_cover;
    gboolean is_terminating;
    int image_size;
};

typedef struct {
    covermanager_t *impl;
    int want_default;
    dispatch_block_t completion_block;
} query_userdata_t;

static covermanager_t *_shared;

static char *
_cache_key_for_track (covermanager_t *impl, ddb_playItem_t *track) {
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
_update_default_cover (covermanager_t *impl) {
    if (impl->plugin == NULL) {
        return;
    }
    char path[PATH_MAX];
    impl->plugin->default_image_path (path, sizeof (path));

    if (impl->default_cover_path == NULL || strcmp (path, impl->default_cover_path)) {
        free (impl->default_cover_path);
        impl->default_cover_path = strdup (path);

        if (impl->default_cover != NULL) {
            gobj_unref (impl->default_cover);
        }

        impl->default_cover = gdk_pixbuf_new_from_file (path, NULL);
    }
}

static void
_settings_did_change_for_track (covermanager_t *manager, ddb_playItem_t *track) {
    covermanager_t *impl = manager;
    if (track == NULL) {
        impl->image_size = deadbeef->conf_get_int ("artwork.image_size", 256);
        _update_default_cover (impl);
        gobj_cache_remove_all (impl->cache);
    }
    else {
        char *key = _cache_key_for_track (impl, track);
        gobj_cache_remove (impl->cache, key);
        free (key);
    }
}

static void
_artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    covermanager_t *manager = user_data;

    gtkui_dispatch_on_main (^{
        if (event == DDB_ARTWORK_SETTINGS_DID_CHANGE) {
            _settings_did_change_for_track (manager, (ddb_playItem_t *)p1);
        }
    });
}

static char *
_buffer_from_file (const char *fname, long *psize) {
    char *buffer = NULL;
    FILE *fp = fopen (fname, "rb");
    if (fp == NULL) {
        return NULL;
    }
    if (fseek (fp, 0, SEEK_END) < 0) {
        goto error;
    }
    long size = ftell (fp);
    if (size <= 0 || size > MAX_ALBUM_ART_FILE_SIZE) {
        goto error; // we don't really want to load ultra-high-res images
    }
    rewind (fp);

    buffer = malloc (size);
    if (buffer == NULL) {
        goto error;
    }

    if (fread (buffer, 1, size, fp) != size) {
        goto error;
    }

    fclose (fp);

    *psize = size;
    return buffer;

error:
    if (fp != NULL) {
        fclose (fp);
    }
    free (buffer);
    return NULL;
}

static GdkPixbuf *
_load_image_from_cover (covermanager_t *impl, ddb_cover_info_t *cover, int want_default) {
    GdkPixbuf *img = NULL;

    if (!img && cover && cover->image_filename) {
        long size = 0;
        char *buf = _buffer_from_file (cover->image_filename, &size);
        if (buf != NULL) {
            GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
            gdk_pixbuf_loader_write (loader, (const guchar *)buf, size, NULL);
            gdk_pixbuf_loader_close (loader, NULL);
            img = gdk_pixbuf_loader_get_pixbuf (loader);
            free (buf);
        }
    }

    if (img) {
        const int max_image_size = impl->image_size;

        // downscale
        GtkAllocation size = {
            .width = gdk_pixbuf_get_width (img),
            .height = gdk_pixbuf_get_height (img),
        };

        if (size.width > max_image_size || size.height > max_image_size) {
            GtkAllocation new_size = {
                .width = max_image_size,
                .height = max_image_size,
            };
            new_size = covermanager_desired_size_for_image_size (impl, size, new_size);

            GdkPixbuf *scaled_img = covermanager_create_scaled_image (impl, img, new_size);
            gobj_unref (img);
            img = scaled_img;
        }
    }

    if (!img && want_default) {
        img = impl->default_cover;
        if (img != NULL) {
            gobj_ref (img);
        }
    }

    return img;
}

static void
_add_cover_for_track (covermanager_t *impl, ddb_playItem_t *track, GdkPixbuf *img) {
    char *key = _cache_key_for_track (impl, track);
    gobj_cache_set (impl->cache, key, G_OBJECT (img));
    free (key);
}

static void
_cleanup_query (ddb_cover_query_t *query) {
    query_userdata_t *user_data = query->user_data;
    void (^completionBlock) (GdkPixbuf *) = (void (^) (GdkPixbuf *))user_data->completion_block;
    Block_release (completionBlock);
    free (user_data);

    // Free the query -- it's fast, so it's OK to free it on main queue
    deadbeef->pl_item_unref (query->track);
    free (query);
}

static void
_callback_and_cleanup (ddb_cover_query_t *query, ddb_cover_info_t *cover, GdkPixbuf *img) {
    query_userdata_t *user_data = query->user_data;
    covermanager_t *impl = user_data->impl;

    if (impl->is_terminating) {
        _cleanup_query (query);
        return;
    }

    if (!(query->flags & DDB_ARTWORK_FLAG_CANCELLED)) {
        _add_cover_for_track (impl, query->track, img);
    }
    void (^completionBlock) (GdkPixbuf *) = (void (^) (GdkPixbuf *))user_data->completion_block;
    completionBlock (img);
    if (img != NULL) {
        gobj_unref (img);
        img = NULL;
    }
    Block_release (completionBlock);
    free (user_data);

    // Free the query -- it's fast, so it's OK to free it on main queue
    deadbeef->pl_item_unref (query->track);
    free (query);

    // Release the cover on background queue
    if (cover != NULL) {
        dispatch_async (dispatch_get_global_queue (DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            impl->plugin->cover_info_release (cover);
        });
    }
}

static void
_cover_loaded_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    query_userdata_t *user_data = query->user_data;
    covermanager_t *impl = user_data->impl;

    if (impl->is_terminating) {
        _cleanup_query (query);
        return;
    }

    gtkui_dispatch_on_main (^{
        // Prevent spurious loading of the same image. The load is already scheduled, so we should just wait for it.
        char *key = _cache_key_for_track (impl, query->track);
        gboolean should_wait = gobj_cache_get_should_wait (impl->cache, key) || (gobj_cache_get (impl->cache, key) != NULL);

        if (should_wait) {
            // append to the end of loader queue
            dispatch_async (impl->loader_queue, ^{
                gtkui_dispatch_on_main (^{
                    GdkPixbuf *img = GDK_PIXBUF (gobj_cache_get (impl->cache, key));
                    _callback_and_cleanup (query, cover, img);
                    free (key);
                });
            });
            return;
        }
        else {
            gobj_cache_set_should_wait (impl->cache, key, TRUE);
            free (key);
        }

        // Load the image on background queue
        dispatch_async (impl->loader_queue, ^{
            if (impl->is_terminating) {
                _cleanup_query (query);
                return;
            }

            __block GdkPixbuf *img = NULL;

            if (!(query->flags & DDB_ARTWORK_FLAG_CANCELLED)) {
                query_userdata_t *user_data = query->user_data;
                img = _load_image_from_cover (impl, cover, user_data->want_default);
            }

            // Update the UI on main queue
            gtkui_dispatch_on_main (^{
                _callback_and_cleanup (query, cover, img);
            });
        });
    });
}

covermanager_t *
covermanager_shared (void) {
    // FIXME: this is not thread-safe
    if (_shared == NULL) {
        _shared = covermanager_new ();
    }
    return _shared;
}

void
covermanager_shared_free (void) {
    if (_shared != NULL) {
        covermanager_free (_shared);
        _shared = NULL;
    }
}

covermanager_t *
covermanager_new (void) {
    covermanager_t *impl = calloc (1, sizeof (covermanager_t));

    impl->plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");

    if (impl->plugin == NULL) {
        return impl;
    }

    impl->cache = gobj_cache_new (CACHE_SIZE);

    impl->image_size = deadbeef->conf_get_int ("artwork.image_size", 256);

    impl->name_tf = deadbeef->tf_compile ("%_path_raw%");

    impl->loader_queue = dispatch_queue_create ("CoverManagerLoaderQueue", NULL);

    if (impl->plugin != NULL) {
        impl->plugin->add_listener (_artwork_listener, impl);
    }

    _update_default_cover (impl);

    return impl;
}

void
covermanager_free (covermanager_t *impl) {
    if (impl->plugin != NULL) {
        impl->plugin->remove_listener (_artwork_listener, impl);
        impl->plugin = NULL;
    }
    if (impl->name_tf != NULL) {
        deadbeef->tf_free (impl->name_tf);
        impl->name_tf = NULL;
    }
    if (impl->cache != NULL) {
        gobj_cache_free (impl->cache);
        impl->cache = NULL;
    }

    free (impl->default_cover_path);
    impl->default_cover_path = NULL;

    if (impl->default_cover) {
        gobj_unref (impl->default_cover);
    }

    free (impl);
}

static GdkPixbuf *
_cover_for_track (covermanager_t *impl, int want_default, DB_playItem_t *track, int64_t source_id, covermanager_completion_block_t completion_block) {
    if (!impl->plugin) {
        completion_block (NULL);
        return NULL;
    }

    char *key = _cache_key_for_track (impl, track);
    GdkPixbuf *cover = GDK_PIXBUF (gobj_cache_get (impl->cache, key));
    free (key);
    key = NULL;

    // FIXME: need to check whether the cache has NULL object for the key
    if (cover != NULL) {
        // completion_block is not executed if the image is non-nil, to avoid double drawing.
        // The caller must release user data if the returned image is not nil.
        return cover;
    }

    if (gobj_cache_get_should_wait (impl->cache, key)) {
        return NULL;
    }

    ddb_cover_query_t *query = calloc (1, sizeof (ddb_cover_query_t));
    query->_size = sizeof (ddb_cover_query_t);
    query->track = track;
    deadbeef->pl_item_ref (track);
    query->source_id = source_id;

    query_userdata_t *data = calloc (1, sizeof (query_userdata_t));
    data->completion_block = (dispatch_block_t)Block_copy (completion_block);
    data->want_default = want_default;
    data->impl = impl;
    query->user_data = (void *)data;

    impl->plugin->cover_get (query, _cover_loaded_callback);

    return NULL;
}

GdkPixbuf *
covermanager_cover_for_track_no_default (covermanager_t *impl, DB_playItem_t *track, int64_t source_id, covermanager_completion_block_t completion_block) {
    return _cover_for_track (impl, 0, track, source_id, completion_block);
}

GdkPixbuf *
covermanager_cover_for_track (covermanager_t *impl, DB_playItem_t *track, int64_t source_id, covermanager_completion_block_t completion_block) {
    return _cover_for_track (impl, 1, track, source_id, completion_block);
}

GdkPixbuf *
covermanager_create_scaled_image (covermanager_t *manager, GdkPixbuf *image, GtkAllocation size) {
    int originalWidth = gdk_pixbuf_get_width (image);
    int originalHeight = gdk_pixbuf_get_height (image);

    if (originalWidth <= size.width && originalHeight <= size.height) {
        gobj_ref (image);
        return image;
    }

    gboolean has_alpha = gdk_pixbuf_get_has_alpha (image);
    int bits_per_sample = gdk_pixbuf_get_bits_per_sample (image);

    GdkPixbuf *scaled_image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, bits_per_sample, size.width, size.height);

    double scale_x = (double)size.width / (double)originalWidth;
    double scale_y = (double)size.height / (double)originalHeight;

    gdk_pixbuf_scale (image, scaled_image, 0, 0, size.width, size.height, 0, 0, scale_x, scale_y, GDK_INTERP_BILINEAR);

    return scaled_image;
}

GtkAllocation
covermanager_desired_size_for_image_size (covermanager_t *manager, GtkAllocation image_size, GtkAllocation availableSize) {
    double scale = min ((double)availableSize.width / (double)image_size.width, (double)availableSize.height / (double)image_size.height);

    GtkAllocation a = { 0 };
    a.width = image_size.width * scale;
    a.height = image_size.height * scale;

    return a;
}

void
covermanager_terminate (covermanager_t *manager) {
    manager->is_terminating = TRUE;
}
