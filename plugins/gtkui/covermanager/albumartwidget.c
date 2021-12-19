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
#include "covermanager.h"
#include <Block.h>

#define min(x,y) ((x)<(y)?(x):(y))

extern DB_functions_t *deadbeef;

typedef struct {
    ddb_gtkui_widget_t base;
    ddb_artwork_plugin_t *plugin;
    GtkWidget *drawing_area;
    GdkPixbuf *image;
    ddb_playItem_t *track;
    int64_t source_id;
    guint throttle_id;
    int64_t request_index;
} w_albumart_t;

static gboolean
_dispatch_on_main_wrapper (void *context) {
    void (^block)(void) = context;
    block ();
    Block_release(block);
    return FALSE;
}

static void
_dispatch_on_main(void (^block)(void)) {
    dispatch_block_t copy_block = Block_copy(block);
    g_idle_add(_dispatch_on_main_wrapper, copy_block);
}

static gboolean
_update (w_albumart_t *w) {
    w->throttle_id = 0;

    GtkAllocation frame;
    gtk_widget_get_allocation(GTK_WIDGET(w->drawing_area), &frame);

    if (frame.width == 0 || frame.height == 0) {
        return FALSE;
    }

    if (w->track != NULL) {
        deadbeef->pl_item_unref (w->track);
        w->track = NULL;
    }
    int cursor = deadbeef->pl_get_cursor(PL_MAIN);
    if (cursor == -1) {
        if (w->image != NULL) {
            g_object_unref(w->image);
            w->image = NULL;
        }
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
                        w->image = scaled_image;
                    }
                    else {
                        if (w->image != NULL) {
                            g_object_unref(w->image);
                            w->image = NULL;
                        }
                    }
                    gtk_widget_queue_draw(w->drawing_area);
                });

                if (image != NULL) {
                    GtkAllocation originalSize = {0};
                    originalSize.width = gdk_pixbuf_get_width(image);
                    originalSize.height = gdk_pixbuf_get_height(image);
                    GtkAllocation desired_size = covermanager_desired_size_for_image_size(cm, originalSize, album_art_space_width);

                    GdkPixbuf *scaled_image = covermanager_create_scaled_image(cm, image, desired_size);
                    w->image = scaled_image;
                    g_object_unref(image);
                }
            }

            deadbeef->plt_unref (plt);
        }
    }

    gtk_widget_queue_draw(w->drawing_area);

    return FALSE;
}

static void
_throttled_update (w_albumart_t *w) {
    if (w->throttle_id != 0) {
        g_source_remove(w->throttle_id);
    }
    w->throttle_id = g_timeout_add(10, G_SOURCE_FUNC(_update), w);

}

static gboolean
_size_did_change (GtkWidget* self, GdkEventConfigure *event, w_albumart_t *w) {
    _throttled_update(w);
    return FALSE;
}


static int
_message (ddb_gtkui_widget_t *base, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_albumart_t *w = (w_albumart_t *)base;
    switch (id) {
    case DB_EV_PLAYLISTCHANGED:
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
    w_albumart_t *w = (w_albumart_t *)base;
    if (w->track) {
        deadbeef->pl_item_unref (w->track);
        w->track = NULL;
    }
}

static gboolean
_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    w_albumart_t *w = user_data;

    GtkAllocation a;
    gtk_widget_get_allocation(widget, &a);

#if !GTK_CHECK_VERSION(3,0,0)
    gtk_paint_flat_box(gtk_widget_get_style(widget), gtk_widget_get_window(widget), GTK_STATE_NORMAL, GTK_SHADOW_NONE, &a, widget, NULL, 0, 0, a.width, a.height);
#else
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    gtk_render_background(context, cr, 0, 0, a.width, a.height);
#endif

    if (a.width < 8 || a.height < 8) {
        return TRUE;
    }

    if (w->image == NULL) {
        return TRUE;
    }

    const int pw = gdk_pixbuf_get_width(w->image);
    const int ph = gdk_pixbuf_get_height(w->image);
    cairo_rectangle(cr, 0, 0, a.width, a.height);
    if (pw > a.width || ph > a.height || (pw < a.width && ph < a.height)) {
        const double scale = min(a.width/(double)pw, a.height/(double)ph);
        cairo_translate(cr, (a.width - a.width*scale)/2., (a.height - a.height*scale)/2.);
        cairo_scale(cr, scale, scale);
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_FAST);
    }
    gdk_cairo_set_source_pixbuf(cr, w->image, (a.width - pw)/2., (a.height - ph)/2.);
    cairo_fill(cr);

    return TRUE;
}

#if !GTK_CHECK_VERSION(3,0,0)
static gboolean
_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = _draw_event (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}
#endif

ddb_gtkui_widget_t *
w_albumart_create (void) {
    w_albumart_t *w = malloc (sizeof (w_albumart_t));
    memset (w, 0, sizeof (w_albumart_t));

    w->base.widget = gtk_event_box_new ();
    w->base.message = _message;
    w->base.destroy = _destroy;
    w->drawing_area = gtk_drawing_area_new();
    gtk_widget_show (GTK_WIDGET(w->drawing_area));
    gtk_container_add (GTK_CONTAINER (w->base.widget), GTK_WIDGET(w->drawing_area));
    w_override_signals (w->base.widget, w);

    g_signal_connect(G_OBJECT(w->drawing_area), "configure-event", G_CALLBACK(_size_did_change), w);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawing_area, "expose_event", G_CALLBACK (_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawing_area, "draw", G_CALLBACK (_draw_event), w);
#endif
    w->plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id("artwork2");
    if (w->plugin != NULL) {
        w->source_id = w->plugin->allocate_source_id();
    }

    return (ddb_gtkui_widget_t *)w;
}


