//
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
#include "albumartwidget.h"
#include "coverart.h"

#define min(x,y) ((x)<(y)?(x):(y))

extern DB_functions_t *deadbeef;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
    int widget_height;
    int widget_width;
    guint load_timeout_id;
} w_coverart_t;

static gboolean
coverart_invalidate_cb (void *user_data) {
    gtk_widget_queue_draw(user_data);
    return FALSE;
}

static void
coverart_invalidate (void *user_data) {
    g_idle_add(coverart_invalidate_cb, user_data);
}

static gboolean
coverart_unref_cb (void *user_data) {
    g_object_unref(user_data);
    return FALSE;
}

static void
coverart_unref (void *user_data) {
    g_idle_add(coverart_unref_cb, user_data);
}

///// cover art display
static GdkPixbuf *
get_cover_art(const int width, const int height, void (*callback)(void *), void *user_data) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track();
    if (!it) {
        return NULL;
    }

    deadbeef->pl_lock();
    const char *uri = deadbeef->pl_find_meta(it, ":URI");
    const char *album = deadbeef->pl_find_meta(it, "album");
    const char *artist = deadbeef->pl_find_meta(it, "artist");
    if (!album || !*album) {
        album = deadbeef->pl_find_meta(it, "title");
    }
    GdkPixbuf *pixbuf = get_cover_art_primary_by_size(uri, artist, album, width, height, callback, user_data);
    deadbeef->pl_unlock();
    deadbeef->pl_item_unref(it);
    return pixbuf;
}

static gboolean
coverart_load (void *user_data) {
    w_coverart_t *w = user_data;
    w->load_timeout_id = 0;
    GdkPixbuf *pixbuf = get_cover_art(w->widget_width, w->widget_height, coverart_invalidate, w->drawarea);
    if (pixbuf) {
        coverart_invalidate(w->drawarea);
        g_object_unref(pixbuf);
    }
    return FALSE;
}

static void
coverart_draw_cairo (GdkPixbuf *pixbuf, GtkAllocation *a, cairo_t *cr, const int filter) {
    const int pw = gdk_pixbuf_get_width(pixbuf);
    const int ph = gdk_pixbuf_get_height(pixbuf);
    cairo_rectangle(cr, 0, 0, a->width, a->height);
    if (pw > a->width || ph > a->height || (pw < a->width && ph < a->height)) {
        const double scale = min(a->width/(double)pw, a->height/(double)ph);
        cairo_translate(cr, (a->width - a->width*scale)/2., (a->height - a->height*scale)/2.);
        cairo_scale(cr, scale, scale);
        cairo_pattern_set_filter(cairo_get_source(cr), filter);
    }
    gdk_cairo_set_source_pixbuf(cr, pixbuf, (a->width - pw)/2., (a->height - ph)/2.);
    cairo_fill(cr);
}

static void
coverart_draw_anything (GtkAllocation *a, cairo_t *cr) {
    GdkPixbuf *pixbuf = get_cover_art(-1, -1, NULL, NULL);
    if (pixbuf) {
        coverart_draw_cairo(pixbuf, a, cr, CAIRO_FILTER_FAST);
        g_object_unref(pixbuf);
    }
}

static void
coverart_draw_exact (GtkAllocation *a, cairo_t *cr, void *user_data) {
    w_coverart_t *w = user_data;
    GdkPixbuf *pixbuf = get_cover_art(a->width, a->height, coverart_invalidate, w->drawarea);
    if (pixbuf) {
        coverart_draw_cairo(pixbuf, a, cr, CAIRO_FILTER_BEST);
        g_object_unref(pixbuf);
    }
    else {
        coverart_draw_anything(a, cr);
    }
}

static gboolean
coverart_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkAllocation a;
    gtk_widget_get_allocation(widget, &a);
    if (a.width < 8 || a.height < 8) {
        return TRUE;
    }

    w_coverart_t *w = user_data;
    if (w->widget_height == a.height && w->widget_width == a.width) {
        coverart_draw_exact(&a, cr, user_data);
    }
    else {
        coverart_draw_anything(&a, cr);
        if (w->load_timeout_id) {
            g_source_remove(w->load_timeout_id);
        }
        w->load_timeout_id = g_timeout_add(w->widget_height == -1 ? 100 : 1000, coverart_load, user_data);
        w->widget_height = a.height;
        w->widget_width = a.width;
    }

    return TRUE;
}

#if !GTK_CHECK_VERSION(3,0,0)
static gboolean
coverart_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = coverart_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}
#endif

static int
coverart_message (ddb_gtkui_widget_t *base, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_coverart_t *w = (w_coverart_t *)base;
    switch (id) {
    case DB_EV_PLAYLIST_REFRESH:
    case DB_EV_SONGCHANGED:
        coverart_invalidate(w->drawarea);
        break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT) {
            coverart_invalidate(w->drawarea);
        }
        break;
    case DB_EV_TRACKINFOCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT) {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
            if (it == ev->track) {
                coverart_invalidate(w->drawarea);
            }
            if (it) {
                deadbeef->pl_item_unref (it);
            }
        }
        break;
    }
    return 0;
}

static gboolean
coverart_destroy_drawarea (GtkWidget *drawarea, gpointer user_data) {
    w_coverart_t *w = (w_coverart_t *)user_data;
    if (w->load_timeout_id) {
        g_source_remove(w->load_timeout_id);
    }
    g_object_ref(drawarea);
    queue_cover_callback(coverart_unref, drawarea);
    return FALSE;
}

ddb_gtkui_widget_t *
w_coverart_create (void) {
    w_coverart_t *w = malloc (sizeof (w_coverart_t));
    memset (w, 0, sizeof (w_coverart_t));

    w->base.widget = gtk_event_box_new ();
    w->base.message = coverart_message;
    w->drawarea = gtk_drawing_area_new ();
    w->widget_height = -1;
    w->widget_width = -1;
    w->load_timeout_id = 0;
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (coverart_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (coverart_draw), w);
#endif
    g_signal_connect ((gpointer) w->drawarea, "destroy", G_CALLBACK (coverart_destroy_drawarea), w);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}


