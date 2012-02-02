/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <string.h>
#include "ddbvolumebar.h"
#include "drawing.h"
#include "gtkui.h"
#include "interface.h"
#include "support.h"

#define min(x,y) ((x)<(y)?(x):(y))

G_DEFINE_TYPE (DdbVolumeBar, ddb_volumebar, GTK_TYPE_WIDGET);

static void
ddb_volumebar_send_configure (DdbVolumeBar *darea)
{
  GtkWidget *widget;
  GdkEvent *event = gdk_event_new (GDK_CONFIGURE);

  widget = GTK_WIDGET (darea);

  event->configure.window = g_object_ref (gtk_widget_get_window(widget));
  event->configure.send_event = TRUE;
  GtkAllocation a;
  gtk_widget_get_allocation (widget, &a);
  event->configure.x = a.x;
  event->configure.y = a.y;
  event->configure.width = a.width;
  event->configure.height = a.height;
  
  gtk_widget_event (widget, event);
  gdk_event_free (event);
}

static void
ddb_volumebar_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  g_return_if_fail (DDB_IS_VOLUMEBAR (widget));
  g_return_if_fail (allocation != NULL);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    {
      if (gtk_widget_get_has_window (widget))
        gdk_window_move_resize (gtk_widget_get_window(widget),
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);

      ddb_volumebar_send_configure (DDB_VOLUMEBAR (widget));
    }
}

gboolean
on_volumebar_draw (GtkWidget    *widget, cairo_t *cr);

gboolean
on_volumebar_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event);

gboolean
on_volumebar_motion_notify_event       (GtkWidget       *widget,
                                        GdkEventMotion  *event);

gboolean
on_volumebar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_volumebar_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event);

gboolean
on_volumebar_configure_event (GtkWidget *widget, GdkEventConfigure *event);

static void
ddb_volumebar_class_init(DdbVolumeBarClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  widget_class->size_allocate = ddb_volumebar_size_allocate;
#if GTK_CHECK_VERSION(3,0,0)
  widget_class->draw = on_volumebar_draw;
#else
  widget_class->expose_event = on_volumebar_expose_event;
#endif
  widget_class->button_press_event = on_volumebar_button_press_event;
  widget_class->button_release_event = on_volumebar_button_release_event;
  widget_class->motion_notify_event = on_volumebar_motion_notify_event;
  widget_class->scroll_event = on_volumebar_scroll_event;
  widget_class->configure_event = on_volumebar_configure_event;
}

GtkWidget * ddb_volumebar_new() {
    return g_object_new (DDB_TYPE_VOLUMEBAR, NULL);
}

static void
ddb_volumebar_init(DdbVolumeBar *volumebar)
{
    char s[100];
    int db = deadbeef->volume_get_db ();
    snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    gtk_widget_set_tooltip_text (GTK_WIDGET (volumebar), s);
    gtk_widget_set_has_window (GTK_WIDGET (volumebar), FALSE);
}

void
volumebar_draw (GtkWidget *widget, cairo_t *cr) {
    if (!widget) {
        return;
    }

#if GTK_CHECK_VERSION(3,0,0)
    GtkAllocation allocation;
    gtk_widget_get_allocation (widget, &allocation);
    cairo_translate (cr, -allocation.x, -allocation.y);
#endif

    float range = -deadbeef->volume_get_min_db ();
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    int n = a.width / 4;
    float vol = (range + deadbeef->volume_get_db ()) / range * n;
    float h = 17;

    GdkColor clr_fg;
    GdkColor clr_bg;
    gtkui_get_bar_foreground_color (&clr_fg);
    gtkui_get_bar_background_color (&clr_bg);

    for (int i = 0; i < n; i++) {
        float iy = (float)i + 3;
        int _x = i * 4;
        int _h = h * iy / n;
        int _y = a.height/2-h/2;
        _y += (h - _h);
        int _w = 3;
        if (i < vol) {
            cairo_set_source_rgb (cr, clr_fg.red/65535.f, clr_fg.green/65535.f, clr_fg.blue/65535.f);
            cairo_rectangle (cr, _x + a.x, _y + a.y, _w, _h);
            cairo_fill (cr);
        }
        else {
            cairo_set_source_rgb (cr, clr_bg.red/65535.f, clr_bg.green/65535.f, clr_bg.blue/65535.f);
            cairo_rectangle (cr, _x + a.x, _y + a.y, _w, _h);
            cairo_fill (cr);
        }
    }
#if 0
    if (DDB_VOLUMEBAR (widget)->show_dbs) {
        draw_begin ((uintptr_t)gtk_widget_get_window(widget));
        draw_init_font (widget->style);
        char s[100];
        int db = deadbeef->volume_get_db ();
        snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
        draw_text (widget->allocation.x, widget->allocation.y, widget->allocation.width, 0, s);
        gtk_widget_set_tooltip_text (widget, s);
        gtk_widget_trigger_tooltip_query (widget);
        draw_end ();
    }
#endif
}

gboolean
on_volumebar_draw (GtkWidget    *widget, cairo_t *cr) {
    volumebar_draw (widget, cr);
    return FALSE;
}

#if !GTK_CHECK_VERSION(3,0,0)
gboolean
on_volumebar_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event)
{
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    on_volumebar_draw (widget, cr);
    cairo_destroy (cr);
    return FALSE;
}
#endif

gboolean
on_volumebar_motion_notify_event       (GtkWidget       *widget,
                                        GdkEventMotion  *event)
{
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    if (event->state & GDK_BUTTON1_MASK) {
        float range = -deadbeef->volume_get_min_db ();
        float volume = (event->x - a.x) / a.width * range - range;
        if (volume > 0) {
            volume = 0;
        }
        if (volume < -range) {
            volume = -range;
        }
        deadbeef->volume_set_db (volume);
        char s[100];
        int db = volume;
        snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
        gtk_widget_set_tooltip_text (widget, s);
        gtk_widget_trigger_tooltip_query (widget);
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}

gboolean
on_volumebar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    if (event->button == 1) {
        float range = -deadbeef->volume_get_min_db ();
        float volume = (event->x - a.x)/ a.width * range - range;
        if (volume < -range) {
            volume = -range;
        }
        if (volume > 0) {
            volume = 0;
        }
        deadbeef->volume_set_db (volume);
        char s[100];
        int db = volume;
        snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
        gtk_widget_set_tooltip_text (widget, s);
        gtk_widget_trigger_tooltip_query (widget);
//        DDB_VOLUMEBAR (widget)->show_dbs = 1;
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}


gboolean
on_volumebar_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    if (event->button == 1) {
//        DDB_VOLUMEBAR (widget)->show_dbs = 0;
        gtk_widget_queue_draw (widget);
    }
  return FALSE;
}

void
volumebar_notify_changed (void) {
    GtkWidget *widget = lookup_widget (mainwin, "volumebar");
    gtk_widget_queue_draw (widget);
    char s[100];
    int db = deadbeef->volume_get_db ();
    snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    gtk_widget_set_tooltip_text (widget, s);
    gtk_widget_trigger_tooltip_query (widget);
}

gboolean
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event)
{
    float range = -deadbeef->volume_get_min_db ();
    float vol = deadbeef->volume_get_db ();
    if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
        vol += 1;
    }
    else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
        vol -= 1;
    }
    if (vol > 0) {
        vol = 0;
    }
    else if (vol < -range) {
        vol = -range;
    }
    deadbeef->volume_set_db (vol);
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    gtk_widget_queue_draw (widget);
    char s[100];
    int db = deadbeef->volume_get_db ();
    snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
    gtk_widget_set_tooltip_text (widget, s);
    gtk_widget_trigger_tooltip_query (widget);
    return FALSE;
}

gboolean
on_volumebar_configure_event (GtkWidget *widget, GdkEventConfigure *event) {
    gtkui_init_theme_colors ();
    return FALSE;
}
