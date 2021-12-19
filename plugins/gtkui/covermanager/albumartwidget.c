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

#include <gtk/gtk.h>
#include "../../../deadbeef.h"
#include "../../artwork/artwork.h"
#include "albumartwidget.h"
#include "coverart.h"
#include "covermanager.h"

#define min(x,y) ((x)<(y)?(x):(y))

extern DB_functions_t *deadbeef;

typedef struct {
    ddb_gtkui_widget_t base;
    ddb_artwork_plugin_t *plugin;
    GtkImage *image_widget;
    ddb_playItem_t *track;
    int64_t source_id;
    guint throttle_id;
    int64_t request_index;
} w_coverart_t;

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

static gboolean
_update (w_coverart_t *w) {
    w->throttle_id = 0;

    GtkAllocation frame;
    gtk_widget_get_allocation(GTK_WIDGET(w->image_widget), &frame);

    if (frame.width == 0 || frame.height == 0) {
        return FALSE;
    }

    if (w->track != NULL) {
        deadbeef->pl_item_unref (w->track);
        w->track = NULL;
    }
    int cursor = deadbeef->pl_get_cursor(PL_MAIN);
    if (cursor == -1) {
        gtk_image_clear(w->image_widget);
    }
    else {
        ddb_playlist_t *plt = deadbeef->plt_get_curr();
        if (plt) {
            ddb_playItem_t *it = deadbeef->plt_get_item_for_idx(plt, cursor, PL_MAIN);

            if (it) {
                w->track = it;

                w->plugin->cancel_queries_with_source_id(w->source_id);

                int album_art_space_width = frame.width;

                int64_t currentIndex = w->request_index++;

                covermanager_t *cm = covermanager_shared();

                GdkPixbuf *image = covermanager_cover_for_track(cm, w->track, w->source_id, ^(GdkPixbuf *img) {
                    if (currentIndex != w->request_index-1) {
                        return;
                    }
                    if (img != NULL) {
                        GtkAllocation originalSize = {0};
                        originalSize.width = gdk_pixbuf_get_width(img);
                        originalSize.height = gdk_pixbuf_get_height(img);
                        GtkAllocation desired_size = covermanager_desired_size_for_image_size(cm, originalSize, album_art_space_width);
                        GdkPixbuf *scaled_image = covermanager_create_scaled_image(cm, img, desired_size);
                        if (scaled_image != NULL) {
                            gtk_image_set_from_pixbuf(w->image_widget, scaled_image);
                        }
                        else {
                            gtk_image_clear(w->image_widget); // FIXME: check what gtk_image_set_from_pixbuf does with NULL arg
                        }
                    }
                    else {
                        gtk_image_clear(w->image_widget);
                    }
                });

                if (image != NULL) {
                    GtkAllocation originalSize = {0};
                    originalSize.width = gdk_pixbuf_get_width(image);
                    originalSize.height = gdk_pixbuf_get_height(image);
                    GtkAllocation desired_size = covermanager_desired_size_for_image_size(cm, originalSize, album_art_space_width);

                    GdkPixbuf *scaled_image = covermanager_create_scaled_image(cm, image, desired_size);
                    if (scaled_image != NULL) {
                        gtk_image_set_from_pixbuf(w->image_widget, scaled_image);
                    }
                    else {
                        gtk_image_clear(w->image_widget);
                    }
                }
            }

            deadbeef->plt_unref (plt);
        }
    }

    return FALSE;
}

static void
_throttled_update (w_coverart_t *w) {
    if (w->throttle_id != 0) {
        g_source_remove(w->throttle_id);
    }
    w->throttle_id = g_timeout_add(10, G_SOURCE_FUNC(_update), w);

}

static gboolean
_size_did_change (GtkWidget* self, GdkEventConfigure event, w_coverart_t *w) {
    _throttled_update(w);
    return FALSE;
}


static int
coverart_message (ddb_gtkui_widget_t *base, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_coverart_t *w = (w_coverart_t *)base;
    switch (id) {
    case DB_EV_PLAYLISTSWITCHED:
    case DB_EV_CURSOR_MOVED: {
        
        _dispatch_on_main(^{
            _throttled_update(w);
        });
    }
        break;
    }
    return 0;
}

static void
_destroy (ddb_gtkui_widget_t *base) {
    w_coverart_t *w = (w_coverart_t *)base;
    if (w->track) {
        deadbeef->pl_item_unref (w->track);
        w->track = NULL;
    }
}

ddb_gtkui_widget_t *
w_coverart_create (void) {
    w_coverart_t *w = malloc (sizeof (w_coverart_t));
    memset (w, 0, sizeof (w_coverart_t));

    w->base.widget = gtk_event_box_new ();
    w->base.message = coverart_message;
    w->base.destroy = _destroy;
    w->image_widget = GTK_IMAGE(gtk_image_new());
    gtk_widget_show (GTK_WIDGET(w->image_widget));
    gtk_container_add (GTK_CONTAINER (w->base.widget), GTK_WIDGET(w->image_widget));
    w_override_signals (w->base.widget, w);

    g_signal_connect(G_OBJECT(w->image_widget), "configure-event", G_CALLBACK(_size_did_change), w);

    w->plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id("artwork2");
    if (w->plugin != NULL) {
        w->source_id = w->plugin->allocate_source_id();
    }

    return (ddb_gtkui_widget_t *)w;
}


