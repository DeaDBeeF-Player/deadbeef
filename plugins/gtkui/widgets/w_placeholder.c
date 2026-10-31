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
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_placeholder.h"

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
} w_placeholder_t;

////// placeholder widget
gboolean
w_placeholder_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    if (!w_get_design_mode ()) {
        return FALSE;
    }
    cairo_set_source_rgb (cr, 255, 0, 0);
    cairo_surface_t *checker;
    cairo_t *cr2;

    checker = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 12, 12);
    cr2 = cairo_create (checker);

    cairo_set_source_rgb (cr2, 0.5, 0.5, 0.5);
    cairo_paint (cr2);
    cairo_set_source_rgb (cr2, 0, 0, 0);
    cairo_move_to (cr2, 0, 0);
    cairo_line_to (cr2, 12, 12);
    cairo_move_to (cr2, 1, 12);
    cairo_line_to (cr2, 12, 1);
    cairo_set_line_width (cr2, 1);
    cairo_set_antialias (cr2, CAIRO_ANTIALIAS_NONE);
    cairo_stroke (cr2);
    cairo_fill (cr2);
    cairo_destroy (cr2);

    cairo_set_source_surface (cr, checker, 0, 0);
    cairo_pattern_t *pt = cairo_get_source (cr);
    cairo_pattern_set_extend (pt, CAIRO_EXTEND_REPEAT);
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    cairo_rectangle (cr, 0, 0, a.width, a.height);
    cairo_paint (cr);
    cairo_surface_destroy (checker);
    return FALSE;
}

gboolean
w_placeholder_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = w_placeholder_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

ddb_gtkui_widget_t *
w_placeholder_create (void) {
    w_placeholder_t *w = malloc (sizeof (w_placeholder_t));
    memset (w, 0, sizeof (w_placeholder_t));

    w->base.widget = gtk_event_box_new ();
    w->drawarea = gtk_drawing_area_new ();
    gtk_widget_set_size_request (w->base.widget, 20, 20);
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);

#if !GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect_after ((gpointer)w->drawarea, "expose_event", G_CALLBACK (w_placeholder_expose_event), w);
#else
    g_signal_connect_after ((gpointer)w->drawarea, "draw", G_CALLBACK (w_placeholder_draw), w);
#endif
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
