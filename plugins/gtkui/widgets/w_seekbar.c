/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../ddbseekbar.h"
#include "../drawing.h"
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_seekbar.h"

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *seekbar;
    gint timer;
    float last_songpos;
} w_seekbar_t;

// seekbar
static gboolean
redraw_seekbar_cb (gpointer data) {
    w_seekbar_t *w = data;
    int iconified = gdk_window_get_state (gtk_widget_get_window (mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    gtk_widget_queue_draw (w->seekbar);
    return FALSE;
}

static gboolean
seekbar_frameupdate (gpointer data) {
    w_seekbar_t *w = data;
    DB_output_t *output = deadbeef->get_output ();
    DB_playItem_t *track = deadbeef->streamer_get_playing_track_safe ();
    float songpos = w->last_songpos;
    float duration = track ? deadbeef->pl_get_item_duration (track) : -1;
    if (!output || (output->state () == DDB_PLAYBACK_STATE_STOPPED || !track)) {
        songpos = 0;
    }
    else {
        songpos = deadbeef->streamer_get_playpos ();
    }
    // translate pos to seekbar pixels
    songpos /= duration;
    GtkAllocation a;
    gtk_widget_get_allocation (w->seekbar, &a);
    songpos *= a.width;
    if (fabs (songpos - w->last_songpos) > 0.01) {
        gtk_widget_queue_draw (w->seekbar);
        w->last_songpos = songpos;
    }
    if (track) {
        deadbeef->pl_item_unref (track);
    }
    return TRUE;
}

static void
w_seekbar_init (ddb_gtkui_widget_t *base) {
    w_seekbar_t *w = (w_seekbar_t *)base;
    if (w->timer) {
        g_source_remove (w->timer);
        w->timer = 0;
    }

    w->timer = g_timeout_add (1000 / gtkui_get_gui_refresh_rate (), seekbar_frameupdate, w);
}

static int
w_seekbar_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        w_seekbar_init (w);
        if (ctx) {
            char *conf_str = (char *)ctx;
            if (gtkui_bar_override_conf (conf_str) || gtkui_bar_colors_conf (conf_str)) {
                g_idle_add (redraw_seekbar_cb, w);
            }
        }
        break;
    case DB_EV_SONGCHANGED:
        g_idle_add (redraw_seekbar_cb, w);
        break;
    }
    return 0;
}

static void
w_seekbar_destroy (ddb_gtkui_widget_t *wbase) {
    w_seekbar_t *w = (w_seekbar_t *)wbase;
    if (w->timer) {
        g_source_remove (w->timer);
        w->timer = 0;
    }
}

ddb_gtkui_widget_t *
w_seekbar_create (void) {
    w_seekbar_t *w = malloc (sizeof (w_seekbar_t));
    memset (w, 0, sizeof (w_seekbar_t));
    w->base.widget = gtk_event_box_new ();
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_add_events (GTK_WIDGET (w->base.widget), GDK_SCROLL_MASK);
#endif
    w->base.message = w_seekbar_message;
    w->base.destroy = w_seekbar_destroy;
    w->base.init = w_seekbar_init;
    w->seekbar = ddb_seekbar_new ();
    gtk_widget_set_size_request (w->base.widget, 20, 16);
    w->last_songpos = -1;
    ddb_seekbar_init_signals (DDB_SEEKBAR (w->seekbar), w->base.widget);
    gtk_widget_show (w->seekbar);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->seekbar);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
