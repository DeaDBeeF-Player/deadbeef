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
#include <stdlib.h>
#include <string.h>
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_logviewer.h"

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *textview;
    int scroll_bottomed;
} w_logviewer_t;

static int w_logviewer_instancecount;

typedef struct {
    w_logviewer_t *w;
    char *text_to_add;
} logviewer_addtexts_t;

////// Log viewer widget

static void
logviewer_logger_callback (struct DB_plugin_s *plugin, uint32_t layers, const char *text, void *ctx);

static void
w_logviewer_init (struct ddb_gtkui_widget_s *widget) {
    GtkTextBuffer *buffer;
    w_logviewer_t *w = (w_logviewer_t *)widget;
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w->textview));
    gtk_text_buffer_set_text (buffer, "Log\n", -1);
}

static void
w_logviewer_scroll_changed (GtkAdjustment *adjustment, gpointer user_data) {
    w_logviewer_t *w = (w_logviewer_t *)user_data;

    if (gtk_adjustment_get_value (adjustment) >=
        gtk_adjustment_get_upper (adjustment) -
            gtk_adjustment_get_page_size (adjustment) - 1e-12) {
        w->scroll_bottomed = 1;
    }
    else {
        w->scroll_bottomed = 0;
    }
}

static gboolean
logviewer_addtext_cb (gpointer data) {
    logviewer_addtexts_t *s = (logviewer_addtexts_t *)data;
    w_logviewer_t *w = s->w;

    GtkTextBuffer *buffer;
    GtkTextIter iter;
    size_t len;
    len = strlen (s->text_to_add);
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w->textview));
    gtk_text_buffer_get_end_iter (buffer, &iter);
    gtk_text_buffer_insert (buffer, &iter, s->text_to_add, (gint)len);
    // Make sure it ends on a newline
    if (s->text_to_add[len - 1] != '\n') {
        gtk_text_buffer_get_end_iter (buffer, &iter);
        gtk_text_buffer_insert (buffer, &iter, "\n", 1);
    }
    if (w->scroll_bottomed) {
        gtk_text_buffer_get_end_iter (buffer, &iter);
        GtkTextMark *mark = gtk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (w->textview), mark);
    }
    free (s->text_to_add);
    free (s);
    return FALSE;
}

static void
logviewer_logger_callback (struct DB_plugin_s *plugin, uint32_t layers, const char *text, void *ctx) {
    logviewer_addtexts_t *s = malloc (sizeof (logviewer_addtexts_t));
    s->w = (w_logviewer_t *)ctx;
    s->text_to_add = strdup (text);
    g_idle_add (logviewer_addtext_cb, (gpointer)s);
}

void
w_logviewer_destroy (ddb_gtkui_widget_t *w) {
    // This is only called if removing widget in design mode
    deadbeef->log_viewer_unregister (logviewer_logger_callback, w);
    if (w_logviewer_instancecount > 0) {
        w_logviewer_instancecount--;
    }
}

ddb_gtkui_widget_t *
w_logviewer_create (void) {
    w_logviewer_t *w = malloc (sizeof (w_logviewer_t));
    memset (w, 0, sizeof (w_logviewer_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_logviewer_init;
    w->base.destroy = w_logviewer_destroy;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_container_add (GTK_CONTAINER (w->base.widget), scroll);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
    w->textview = gtk_text_view_new ();
    gtk_text_view_set_editable (GTK_TEXT_VIEW (w->textview), FALSE);
    //gtk_widget_set_size_request(w->textview, 320, 240);
    gtk_widget_show (w->textview);
    gtk_container_add (GTK_CONTAINER (scroll), w->textview);

    w_override_signals (w->base.widget, w);

    GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scroll));
    w->scroll_bottomed = 1;
    g_signal_connect (adjustment, "value-changed", G_CALLBACK (w_logviewer_scroll_changed), w);

    deadbeef->log_viewer_register (logviewer_logger_callback, w);

    w_logviewer_instancecount++;

    return (ddb_gtkui_widget_t *)w;
}

gboolean
w_logviewer_is_present (void) {
    return w_logviewer_instancecount > 0;
}
