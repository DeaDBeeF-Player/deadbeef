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
#include <gtk/gtk.h>
#include <string.h>
#include <assert.h>
#include <glib.h>
#include "ddbtabstrip.h"
#include "drawing.h"
#include "gtkui.h"
#include "interface.h"
#include "support.h"
#include "ddblistview.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    g_object_ref (G_OBJECT (widget)), (GDestroyNotify) g_object_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)


G_DEFINE_TYPE (DdbTabStrip, ddb_tabstrip, GTK_TYPE_WIDGET);

extern GtkWidget *theme_button;
#define arrow_sz 10
#define arrow_widget_width (arrow_sz+4)

void
plt_get_title_wrapper (int plt, char *buffer, int len) {
    if (plt == -1) {
        strcpy (buffer, "");
        return;
    }
    ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
    deadbeef->plt_get_title (p, buffer, len);
    deadbeef->plt_unref (p);
    char *end;
    if (!g_utf8_validate (buffer, -1, (const gchar **)&end)) {
        *end = 0;
    }
}

static void
ddb_tabstrip_send_configure (DdbTabStrip *darea)
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
ddb_tabstrip_realize (GtkWidget *widget) {
    DdbTabStrip *darea = DDB_TABSTRIP (widget);
    GdkWindowAttr attributes;
    gint attributes_mask;

    if (!gtk_widget_get_has_window (widget))
    {
        GTK_WIDGET_CLASS (ddb_tabstrip_parent_class)->realize (widget);
    }
    else
    {
        gtk_widget_set_realized (widget, TRUE);

        attributes.window_type = GDK_WINDOW_CHILD;
        GtkAllocation a;
        gtk_widget_get_allocation (widget, &a);
        attributes.x = a.x;
        attributes.y = a.y;
        attributes.width = a.width;
        attributes.height = a.height;
        attributes.wclass = GDK_INPUT_OUTPUT;
        attributes.visual = gtk_widget_get_visual (widget);
#if !GTK_CHECK_VERSION(3,0,0)
        attributes.colormap = gtk_widget_get_colormap (widget);
#endif
        attributes.event_mask = gtk_widget_get_events (widget);
        attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK;

        attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
#if !GTK_CHECK_VERSION(3,0,0)
        attributes_mask |= GDK_WA_COLORMAP;
#endif

        gtk_widget_set_window(widget, gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask));
        gdk_window_set_user_data (gtk_widget_get_window(widget), darea);

        gtk_widget_set_style (widget, gtk_style_attach (gtk_widget_get_style (widget), gtk_widget_get_window(widget)));
        gtk_style_set_background (gtk_widget_get_style (widget), gtk_widget_get_window(widget), GTK_STATE_NORMAL);
    }

    ddb_tabstrip_send_configure (DDB_TABSTRIP (widget));
    GtkTargetEntry entry = {
        .target = "STRING",
        .flags = GTK_TARGET_SAME_APP,
        TARGET_SAMEWIDGET
    };
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, &entry, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_drag_dest_add_uri_targets (widget);
    gtk_drag_dest_set_track_motion (widget, TRUE);
}

static void
ddb_tabstrip_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  g_return_if_fail (DDB_IS_TABSTRIP (widget));
  g_return_if_fail (allocation != NULL);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    {
      if (gtk_widget_get_has_window (widget))
        gdk_window_move_resize (gtk_widget_get_window(widget),
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);

      ddb_tabstrip_send_configure (DDB_TABSTRIP (widget));
    }
}


gboolean
on_tabstrip_scroll_event                 (GtkWidget      *widget,
                                        GdkEventScroll *event);

gboolean
on_tabstrip_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_tabstrip_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_tabstrip_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event);

gboolean
on_tabstrip_draw                 (GtkWidget       *widget,
                                        cairo_t *cr);


gboolean
on_tabstrip_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event);

gboolean
on_tabstrip_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event);

gboolean
on_tabstrip_drag_motion_event          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time);

gboolean
on_tabstrip_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time);

void
on_tabstrip_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            target_type,
                                        guint            time);

void
on_tabstrip_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time);

void
on_tabstrip_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context);

static int
get_tab_under_cursor (DdbTabStrip *ts, int x);

static void
ddb_tabstrip_destroy(GObject *object)
{
  DdbTabStrip *tabstrip;

  g_return_if_fail(object != NULL);
  g_return_if_fail(DDB_IS_TABSTRIP(object));

  tabstrip = DDB_TABSTRIP (object);
}

static void
ddb_tabstrip_class_init(DdbTabStripClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
#if !GTK_CHECK_VERSION(3,0,0)
  widget_class->expose_event = on_tabstrip_expose_event;
#else
  widget_class->draw = on_tabstrip_draw;
#endif
  widget_class->realize = ddb_tabstrip_realize;
  widget_class->size_allocate = ddb_tabstrip_size_allocate;
  widget_class->button_press_event = on_tabstrip_button_press_event;
  widget_class->button_release_event = on_tabstrip_button_release_event;
  widget_class->configure_event = on_tabstrip_configure_event;
  widget_class->motion_notify_event = on_tabstrip_motion_notify_event;
  widget_class->scroll_event= on_tabstrip_scroll_event;
  widget_class->drag_motion = on_tabstrip_drag_motion_event;
  widget_class->drag_drop = on_tabstrip_drag_drop;
  widget_class->drag_end = on_tabstrip_drag_end;
  widget_class->drag_data_received = on_tabstrip_drag_data_received;
  widget_class->drag_leave = on_tabstrip_drag_leave;
}

gboolean
on_tabstrip_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time)
{
    return TRUE;
}

void
on_tabstrip_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            target_type,
                                        guint            time)
{
    DdbListview *ps = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));

    gchar *ptr=(char*)gtk_selection_data_get_data (data);
    int len = gtk_selection_data_get_length (data);
    if (target_type == 0) { // uris
        // this happens when dropped from file manager
        char *mem = malloc (len+1);
        memcpy (mem, ptr, len);
        mem[len] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        ps->binding->external_drag_n_drop (NULL, mem, len);
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        int plt = *d;
        d++;
        int length = (len/4)-1;
        ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
        if (p) {
            ps->binding->drag_n_drop (NULL, p, d, length, gdk_drag_context_get_selected_action (drag_context) == GDK_ACTION_COPY ? 1 : 0);
            deadbeef->plt_unref (p);
        }
    }
    gtk_drag_finish (drag_context, TRUE, FALSE, time);
}

void
on_tabstrip_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time)
{
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_list_drag_leave (pl->list, drag_context, time, NULL);
}

void
on_tabstrip_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context)
{
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_list_drag_end (pl->list, drag_context, NULL);
}

GtkWidget * ddb_tabstrip_new() {
    return g_object_new (DDB_TYPE_TABSTRIP, NULL);
}

static void
ddb_tabstrip_init(DdbTabStrip *tabstrip)
{
    tabstrip->hscrollpos = 0;
    tabstrip->dragging = -1;
    tabstrip->prepare = 0;
    tabstrip->dragpt[0] = 0;
    tabstrip->dragpt[1] = 0;
    tabstrip->prev_x = 0;
    tabstrip->movepos = 0;
}

static int tab_clicked = -1;
static int text_left_padding = 4;
static int text_right_padding = 0; // calculated from widget height
static int text_vert_offset = -2;
static int tab_overlap_size = 0; // widget_height/2
static int tabs_left_margin = 4;
static int min_tab_size = 80;

static int tab_moved = 0;

static void
cairo_draw_lines (cairo_t *cr, GdkPoint *pts, int cnt) {
    for (int i = 1; i < cnt; i++) {
        cairo_move_to (cr, pts[i-1].x+1, pts[i-1].y+1);
        cairo_line_to (cr, pts[i].x+1, pts[i].y+1);
    }
}

static void
cairo_draw_poly (cairo_t *cr, GdkPoint *pts, int cnt) {
    cairo_move_to (cr, pts[0].x, pts[0].y);
    for (int i = 1; i < cnt; i++) {
        cairo_line_to (cr, pts[i].x, pts[i].y);
    }
}



void
ddb_tabstrip_draw_tab (GtkWidget *widget, cairo_t *cr, int idx, int selected, int x, int y, int w, int h) {
    GdkPoint points_filled[] = {
        { x+2, y + h },
        { x+2, y + 2 },
        { x + w - h + 1, y + 2 },
        { x + w - 1 + 1, y + h }
    };
    GdkPoint points_frame1[] = {
        { x, y + h-2 },
        { x, y + 0 },
        { x + 1, y + 0 },
        { x + w - h - 1, y + 0 },
        { x + w - h, y + 1 },
        { x + w - h + 1, y + 1 },
        { x + w - 2, y + h - 2 },
        { x + w - 1, y + h - 2 },
        { x + w-2, y + h - 3 }
    };
    GdkPoint points_frame2[] = {
        { x + 1, y + h + 1 },
        { x + 1, y + 0 },
        { x + w - h - 1, y + 1 },
        { x + w - h, y + 2 },
        { x + w - h + 1, y + 2 },
        { x + w-3, y + h - 2 },
        { x + w-2, y + h - 2 },
    };
    GdkColor clr_bg;
    GdkColor clr_outer_frame;
    GdkColor clr_inner_frame;
    int fallback = 1;
    deadbeef->pl_lock ();
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (idx);
    const char *bgclr = deadbeef->plt_find_meta (plt, "gui.bgcolor");
    deadbeef->plt_unref (plt);
    if (bgclr) {
        int r, g, b;
        if (3 == sscanf (bgclr, "%02x%02x%02x", &r, &g, &b)) {
            fallback = 0;
            clr_bg.red = r * 0x101;
            clr_bg.green = g * 0x101;
            clr_bg.blue = b * 0x101;
        }
    }
    deadbeef->pl_unlock ();
    if (selected) {
        if (fallback) {
            gtkui_get_tabstrip_base_color (&clr_bg);
        }
        gtkui_get_tabstrip_dark_color (&clr_outer_frame);
        gtkui_get_tabstrip_light_color (&clr_inner_frame);
    }
    else {
        if (fallback) {
            gtkui_get_tabstrip_mid_color (&clr_bg);
        }

        gtkui_get_tabstrip_dark_color (&clr_outer_frame);
        gtkui_get_tabstrip_mid_color (&clr_inner_frame);
    }
    cairo_set_source_rgb (cr, clr_bg.red/65535.f, clr_bg.green/65535.f, clr_bg.blue/65535.0);
    cairo_new_path (cr);
    cairo_draw_poly (cr, points_filled, 4);
    cairo_close_path (cr);
    cairo_fill (cr);
    cairo_set_source_rgb (cr, clr_outer_frame.red/65535.f, clr_outer_frame.green/65535.f, clr_outer_frame.blue/65535.0);
    cairo_draw_lines (cr, points_frame1, 9);
    cairo_stroke (cr);
    cairo_set_source_rgb (cr, clr_inner_frame.red/65535.f, clr_inner_frame.green/65535.f, clr_inner_frame.blue/65535.0);
    cairo_draw_lines (cr, points_frame2, 7);
    cairo_stroke (cr);
}

int
ddb_tabstrip_get_tab_width (DdbTabStrip *ts, int tab) {
    int width;
    char title[100];
    plt_get_title_wrapper (tab, title, sizeof (title));
    int h = 0;
    draw_get_text_extents (title, strlen (title), &width, &h);
    width += text_left_padding + text_right_padding;
    if (width < min_tab_size) {
        width = min_tab_size;
    }
    return width;
}

int
tabstrip_need_arrows (DdbTabStrip *ts) {
    GtkWidget *widget = GTK_WIDGET (ts);
    int cnt = deadbeef->plt_get_count ();
    int w = 0;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    for (int idx = 0; idx < cnt; idx++) {
        w += ddb_tabstrip_get_tab_width (ts, idx) - tab_overlap_size;
        if (w >= a.width) {
            return 1;
        }
    }
    w += tab_overlap_size + 3;
    if (w >= a.width) {
        return 1;
    }
    return 0;
}

static void
tabstrip_scroll_to_tab_int (DdbTabStrip *ts, int tab, int redraw) {
    GtkWidget *widget = GTK_WIDGET (ts);
    int w = 0;
    int cnt = deadbeef->plt_get_count ();
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    int boundary = a.width - arrow_widget_width*2 + ts->hscrollpos;
    for (int idx = 0; idx < cnt; idx++) {
        int tab_w = ddb_tabstrip_get_tab_width (ts, idx);
        if (idx == cnt-1) {
            tab_w += 3;
        }
        if (idx == tab) {
            if (w < ts->hscrollpos) {
                ts->hscrollpos = w;
                deadbeef->conf_set_int ("gtkui.tabscroll", ts->hscrollpos);
                if (redraw) {
                    gtk_widget_queue_draw (widget);
                }
            }
            else if (w + tab_w >= boundary) {
                ts->hscrollpos += (w+tab_w) - boundary;
                deadbeef->conf_set_int ("gtkui.tabscroll", ts->hscrollpos);
                if (redraw) {
                    gtk_widget_queue_draw (widget);
                }
            }
            break;
        }
        w += tab_w - tab_overlap_size;
    }
}

static void
tabstrip_scroll_to_tab (DdbTabStrip *ts, int tab) {
    tabstrip_scroll_to_tab_int (ts, tab, 1);
}

void
tabstrip_adjust_hscroll (DdbTabStrip *ts) {
    GtkWidget *widget = GTK_WIDGET (ts);
    ts->hscrollpos = deadbeef->conf_get_int ("gtkui.tabscroll", 0);
    if (deadbeef->plt_get_count () > 0) {
        int need_arrows = tabstrip_need_arrows (ts);
        if (need_arrows) {
            GtkAllocation a;
            gtk_widget_get_allocation (widget, &a);
            int w = 0;
            int cnt = deadbeef->plt_get_count ();
            for (int idx = 0; idx < cnt; idx++) {
                w += ddb_tabstrip_get_tab_width (ts, idx) - tab_overlap_size;
            }
            w += tab_overlap_size + 3;
            if (ts->hscrollpos > w - (a.width - arrow_widget_width*2)) {
                ts->hscrollpos = w - (a.width - arrow_widget_width*2);
                deadbeef->conf_set_int ("gtkui.tabscroll", ts->hscrollpos);
            }
            tabstrip_scroll_to_tab_int (ts, deadbeef->plt_get_curr_idx (), 0);
        }
        else {
            ts->hscrollpos = 0;
            deadbeef->conf_set_int ("gtkui.tabscroll", ts->hscrollpos);
        }
    }
}

void
set_tab_text_color (int idx, int selected) {
    if (idx == -1) {
        return;
    }
    deadbeef->pl_lock ();
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (idx);
    int fallback = 1;
    const char *clr = deadbeef->plt_find_meta (plt, "gui.color");
    if (clr) {
        int r, g, b;
        if (3 == sscanf (clr, "%02x%02x%02x", &r, &g, &b)) {
            fallback = 0;
            float fg[3] = {(float)r/0xff, (float)g/0xff, (float)b/0xff};
            draw_set_fg_color (fg);
        }
    }
    deadbeef->plt_unref (plt);
    if (fallback) {
        GdkColor color;
        gtkui_get_tabstrip_text_color (&color);
        float fg[3] = {(float)color.red/0xffff, (float)color.green/0xffff, (float)color.blue/0xffff};
        draw_set_fg_color (fg);
    }
    deadbeef->pl_unlock ();
}

void
tabstrip_render (DdbTabStrip *ts, cairo_t *cr) {
    GtkWidget *widget = GTK_WIDGET (ts);
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    tabstrip_adjust_hscroll (ts);
    cairo_set_line_width (cr, 1);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

    int cnt = deadbeef->plt_get_count ();
    int hscroll = ts->hscrollpos;

    int need_arrows = tabstrip_need_arrows (ts);
    if (need_arrows) {
        hscroll -= arrow_widget_width;
    }

    int x = -hscroll;
    int w = 0;
    int h = draw_get_font_size ();
    h = a.height;
    tab_overlap_size = (h-4)/2;
    text_right_padding = h - 3;

    const char *detail = "button";
    int tab_selected = deadbeef->plt_get_curr_idx ();
    if (tab_selected == -1) {
        return;
    }

    // fill background
    GdkColor clr;
    gtkui_get_tabstrip_mid_color (&clr);
    cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.0);
    cairo_rectangle (cr, 0, 0, a.width, a.height);
    cairo_fill (cr);

    gtkui_get_tabstrip_dark_color (&clr);
    cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.0);
    cairo_move_to (cr, 0, 1);
    cairo_line_to (cr, a.width, 1);
    cairo_stroke (cr);

    int y = 4;
    h = a.height - 4;
    draw_begin (cr);
    int need_draw_moving = 0;
    int idx;
    int widths[cnt];
    for (idx = 0; idx < cnt; idx++) {
        char title[100];
        plt_get_title_wrapper (idx, title, sizeof (title));
        int h = 0;
        draw_get_text_extents (title, strlen (title), &widths[idx], &h);
        widths[idx] += text_left_padding + text_right_padding;
        if (widths[idx] < min_tab_size) {
            widths[idx] = min_tab_size;
        }
    }

    x = -hscroll + tabs_left_margin;

    for (idx = 0; idx < cnt; idx++) {
        w = widths[idx];
        GdkRectangle area;
        area.x = x;
        area.y = 0;
        area.width = w;
        area.height = 24;
        if (idx != tab_selected) {
            ddb_tabstrip_draw_tab (widget, cr, idx, idx == tab_selected, x, y, w, h);
            char tab_title[100];
            plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));

            set_tab_text_color (idx, tab_selected);
            draw_text (x + text_left_padding, y - text_vert_offset, w, 0, tab_title);
        }
        x += w - tab_overlap_size;
    }
    GdkColor *pclr = &gtk_widget_get_style (widget)->dark[GTK_STATE_NORMAL];
    cairo_set_source_rgb (cr, pclr->red/65535.f, pclr->green/65535.f, pclr->blue/65535.0);
    cairo_move_to (cr, 0, a.height-1);
    cairo_line_to (cr, a.width, a.height-1);
    cairo_stroke (cr);
    pclr = &gtk_widget_get_style (widget)->light[GTK_STATE_NORMAL];
    cairo_set_source_rgb (cr, pclr->red/65535.f, pclr->green/65535.f, pclr->blue/65535.0);
    cairo_move_to (cr, 0, a.height);
    cairo_line_to (cr, a.width, a.height);
    cairo_stroke (cr);
    // calc position for drawin selected tab
    x = -hscroll;
    for (idx = 0; idx < tab_selected; idx++) {
        x += widths[idx] - tab_overlap_size;
    }
    x += tabs_left_margin;
    // draw selected
    if (ts->dragging < 0 || ts->prepare || tab_selected != ts->dragging) {
        idx = tab_selected;
        w = widths[tab_selected];
        GdkRectangle area;
        area.x = x;
        area.y = 0;
        area.width = w;
        area.height = 24;
        ddb_tabstrip_draw_tab (widget, cr, idx, 1, x, y, w, h);
        char tab_title[100];
        plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));
        set_tab_text_color (idx, tab_selected);
        draw_text (x + text_left_padding, y - text_vert_offset, w, 0, tab_title);
    }
    else {
        need_draw_moving = 1;
    }
    if (need_draw_moving) {
        x = -hscroll + tabs_left_margin;
        for (idx = 0; idx < cnt; idx++) {
            w = widths[idx];
            if (idx == ts->dragging) {
                x = ts->movepos;
                if (x >= a.width) {
                    break;
                }
                if (w > 0) {
                    ddb_tabstrip_draw_tab (widget, cr, idx, 1, x, y, w, h);
                    char tab_title[100];
                    plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));
                    set_tab_text_color (idx, tab_selected);
                    draw_text (x + text_left_padding, y - text_vert_offset, w, 0, tab_title);
                }
                break;
            }
            x += w - tab_overlap_size;
        }
    }
    if (need_arrows) {
        int sz = a.height-3;
        gtkui_get_tabstrip_mid_color (&clr);
        cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.0);
        cairo_rectangle (cr, 0, 1, arrow_widget_width, sz);
        cairo_fill (cr);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_paint_arrow (gtk_widget_get_style (widget), cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, widget, NULL, GTK_ARROW_LEFT, TRUE, 2, sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#else
        gtk_paint_arrow (gtk_widget_get_style (widget), gtk_widget_get_window(widget), GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_LEFT, TRUE, 2, sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#endif

        cairo_rectangle (cr, a.width-arrow_widget_width, 1, arrow_widget_width, sz);
        cairo_fill (cr);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_paint_arrow (gtk_widget_get_style (widget), cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, widget, NULL, GTK_ARROW_RIGHT, TRUE, a.width-arrow_sz-2, 1+sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#else
        gtk_paint_arrow (gtk_widget_get_style (widget), gtk_widget_get_window(widget), GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_RIGHT, TRUE, a.width-arrow_sz-2, 1+sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#endif
    }

    draw_end ();
}

static int
get_tab_under_cursor (DdbTabStrip *ts, int x) {
    int hscroll = ts->hscrollpos;
    int need_arrows = tabstrip_need_arrows (ts);
    if (need_arrows) {
        hscroll -= arrow_widget_width;
    }
    int idx;
    int cnt = deadbeef->plt_get_count ();
    int fw = tabs_left_margin - hscroll;
    int tab_selected = deadbeef->plt_get_curr_idx ();
    for (idx = 0; idx < cnt; idx++) {
        char title[100];
        plt_get_title_wrapper (idx, title, sizeof (title));
        int w = 0;
        int h = 0;
        draw_get_text_extents (title, strlen (title), &w, &h);
        w += text_left_padding + text_right_padding;
        if (w < min_tab_size) {
            w = min_tab_size;
        }
        fw += w;
        fw -= tab_overlap_size;
        if (fw > x) {
            return idx;
        }
    }
    return -1;
}

void
on_rename_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_entrydialog ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Edit playlist"));
    GtkWidget *e;
    e = lookup_widget (dlg, "title_label");
    gtk_label_set_text (GTK_LABEL(e), _("Title:"));
    e = lookup_widget (dlg, "title");
    char t[100];
    plt_get_title_wrapper (tab_clicked, t, sizeof (t));
    gtk_entry_set_text (GTK_ENTRY (e), t);
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        const char *text = gtk_entry_get_text (GTK_ENTRY (e));
        deadbeef->pl_lock ();
        ddb_playlist_t *p = deadbeef->plt_get_for_idx (tab_clicked);
        deadbeef->plt_set_title (p, text);
        deadbeef->plt_unref (p);
        deadbeef->pl_unlock ();
    }
    gtk_widget_destroy (dlg);
}


void
on_remove_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (tab_clicked != -1) {
        deadbeef->plt_remove (tab_clicked);
        DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
        ddb_listview_refresh (pl, DDB_LIST_CHANGED | DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
        search_refresh ();
        int playlist = deadbeef->plt_get_curr_idx ();
        deadbeef->conf_set_int ("playlist.current", playlist);
    }
}

void
on_add_new_playlist1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int playlist = gtkui_add_new_playlist ();
    if (playlist != -1) {
        gtkui_playlist_set_curr (playlist);
    }
}

GtkWidget*
create_plmenu (void)
{
  GtkWidget *plmenu;
  GtkWidget *rename_playlist1;
  GtkWidget *remove_playlist1;
  GtkWidget *add_new_playlist1;
  GtkWidget *separator11;
  GtkWidget *load_playlist1;
  GtkWidget *save_playlist1;
  GtkWidget *save_all_playlists1;

  plmenu = gtk_menu_new ();

  rename_playlist1 = gtk_menu_item_new_with_mnemonic (_("Rename Playlist"));
  if (tab_clicked == -1) {
      gtk_widget_set_sensitive (rename_playlist1, FALSE);
  }
  gtk_widget_show (rename_playlist1);
  gtk_container_add (GTK_CONTAINER (plmenu), rename_playlist1);

  remove_playlist1 = gtk_menu_item_new_with_mnemonic (_("Remove Playlist"));
  if (tab_clicked == -1) {
      gtk_widget_set_sensitive (remove_playlist1, FALSE);
  }
  gtk_widget_show (remove_playlist1);
  gtk_container_add (GTK_CONTAINER (plmenu), remove_playlist1);

  add_new_playlist1 = gtk_menu_item_new_with_mnemonic (_("Add New Playlist"));
  gtk_widget_show (add_new_playlist1);
  gtk_container_add (GTK_CONTAINER (plmenu), add_new_playlist1);

  g_signal_connect ((gpointer) rename_playlist1, "activate",
                    G_CALLBACK (on_rename_playlist1_activate),
                    NULL);
  g_signal_connect ((gpointer) remove_playlist1, "activate",
                    G_CALLBACK (on_remove_playlist1_activate),
                    NULL);
  g_signal_connect ((gpointer) add_new_playlist1, "activate",
                    G_CALLBACK (on_add_new_playlist1_activate),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (plmenu, plmenu, "plmenu");
  GLADE_HOOKUP_OBJECT (plmenu, rename_playlist1, "rename_playlist1");
  GLADE_HOOKUP_OBJECT (plmenu, remove_playlist1, "remove_playlist1");
  GLADE_HOOKUP_OBJECT (plmenu, add_new_playlist1, "add_new_playlist1");

  return plmenu;
}

static void
tabstrip_scroll_left (DdbTabStrip *ts) {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab > 0) {
        tab--;
        gtkui_playlist_set_curr (tab);
    }
    tabstrip_scroll_to_tab (ts, tab);
}

static void
tabstrip_scroll_right (DdbTabStrip *ts) {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab < deadbeef->plt_get_count ()-1) {
        tab++;
        gtkui_playlist_set_curr (tab);
    }
    tabstrip_scroll_to_tab (ts, tab);
}

gboolean
tabstrip_scroll_cb (gpointer data) {
    DdbTabStrip *ts = DDB_TABSTRIP (data);
    if (ts->scroll_direction < 0) {
        tabstrip_scroll_left (ts);
    }
    else if (ts->scroll_direction > 0) {
        tabstrip_scroll_right (ts);
    }
    else {
        return FALSE;
    }
    return TRUE;
}

gboolean
on_tabstrip_scroll_event(GtkWidget       *widget,
                         GdkEventScroll  *event)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);

    if(event->direction == GDK_SCROLL_UP)
    {
        tabstrip_scroll_left(ts);
    }
    else if (event->direction == GDK_SCROLL_DOWN)
    {
        tabstrip_scroll_right(ts);
    }

    return TRUE;
}

gboolean
on_tabstrip_button_press_event(GtkWidget      *widget,
                               GdkEventButton *event)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    tab_clicked = get_tab_under_cursor (ts, event->x);
    if (event->button == 1)
    {
        int need_arrows = tabstrip_need_arrows (ts);
        if (need_arrows) {
            GtkAllocation a;
            gtk_widget_get_allocation (widget, &a);
            if (event->x < arrow_widget_width) {
                if (event->type == GDK_BUTTON_PRESS) {
                    tabstrip_scroll_left (ts);
                    ts->scroll_direction = -1;
                    ts->scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                }
                return FALSE;
            }
            else if (event->x >= a.width - arrow_widget_width) {
                if (event->type == GDK_BUTTON_PRESS) {
                    tabstrip_scroll_right (ts);
                    ts->scroll_direction = 1;
                    ts->scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                }
                return FALSE;
            }
        }
        if (tab_clicked != -1) {
            gtkui_playlist_set_curr (tab_clicked);
        }
        else {
            if (event->type == GDK_2BUTTON_PRESS) {
                // new tab
                int playlist = gtkui_add_new_playlist ();
                if (playlist != -1) {
                    gtkui_playlist_set_curr (playlist);
                }
                return FALSE;
            }
            return FALSE;
        }

        // adjust scroll if clicked tab spans border
        if (need_arrows) {
            tabstrip_scroll_to_tab (ts, tab_clicked);
        }

        int hscroll = ts->hscrollpos;
        if (need_arrows) {
            hscroll -= arrow_widget_width;
        }
        int x = -hscroll + tabs_left_margin;
        int idx;
        for (idx = 0; idx < tab_clicked; idx++) {
            int width = ddb_tabstrip_get_tab_width (ts, idx);
            x += width - tab_overlap_size;
        }
        ts->dragpt[0] = event->x - x;
        ts->dragpt[1] = event->y;
        ts->prepare = 1;
        ts->dragging = tab_clicked;
        ts->prev_x = event->x;
        tab_moved = 0;
    }
    else if (event->button == 3) {
        GtkWidget *menu = create_plmenu ();
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, widget, 0, gtk_get_current_event_time());
    }
    else if (event->button == 2) {
        if (tab_clicked == -1) {
            // new tab
            int playlist = gtkui_add_new_playlist ();
            if (playlist != -1) {
                gtkui_playlist_set_curr (playlist);
            }
            return FALSE;
        }
        else if (deadbeef->conf_get_int ("gtkui.mmb_delete_playlist", 1)) {
            if (tab_clicked != -1) {
                deadbeef->plt_remove (tab_clicked);
                // force invalidation of playlist cache
                DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
                ddb_listview_refresh (pl, DDB_LIST_CHANGED | DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
                search_refresh ();
                int playlist = deadbeef->plt_get_curr_idx ();
                deadbeef->conf_set_int ("playlist.current", playlist);
            }
        }
    }
    return FALSE;
}


gboolean
on_tabstrip_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    if (event->button == 1) {
        if (ts->scroll_timer > 0) {
            ts->scroll_direction = 0;
            ts->scroll_timer = 0;
        }
        if (ts->prepare || ts->dragging >= 0) {
            ts->dragging = -1;
            ts->prepare = 0;
            gtk_widget_queue_draw (widget);
        }
    }
    return FALSE;
}

gboolean
on_tabstrip_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event)
{
    draw_init_font (gtk_widget_get_style (widget));
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    tabstrip_adjust_hscroll (ts);
    int height = draw_get_listview_rowheight () + 4;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    if (height != a.height) {
        gtk_widget_set_size_request (widget, -1, height);
    }
    return FALSE;
}

gboolean
on_tabstrip_draw (GtkWidget *widget, cairo_t *cr) {
    tabstrip_render (DDB_TABSTRIP (widget), cr);
    return FALSE;
}

gboolean
on_tabstrip_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event)
{
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    on_tabstrip_draw (widget, cr);
    cairo_destroy (cr);
    return FALSE;
}

gboolean
on_tabstrip_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    int ev_x, ev_y;
    GdkModifierType ev_state;
    ev_x = event->x;
    ev_y = event->y;
    ev_state = event->state;
#if GTK_CHECK_VERSION(2,12,0) && !defined(ULTRA_COMPATIBLE)
    gdk_event_request_motions (event);
#endif
    if ((ev_state & GDK_BUTTON1_MASK) && ts->prepare) {
        if (gtk_drag_check_threshold (widget, ev_x, ts->prev_x, 0, 0)) {
            ts->prepare = 0;
        }
    }
    if (!ts->prepare && ts->dragging >= 0) {
//        gdk_window_set_cursor (gtk_widget_get_window(widget), cursor_drag);
        ts->movepos = ev_x - ts->dragpt[0];

        // find closest tab to the left
        int idx;
        int hscroll = ts->hscrollpos;
        int need_arrows = tabstrip_need_arrows (ts);
        if (need_arrows) {
            hscroll -= arrow_widget_width;
        }
        int x = -hscroll + tabs_left_margin;
        int inspos = -1;
        int cnt = deadbeef->plt_get_count ();
        for (idx = 0; idx < cnt; idx++) {
            int width = ddb_tabstrip_get_tab_width (ts, idx);
            if (idx != ts->dragging && x <= ts->movepos && x + width/2 - tab_overlap_size  > ts->movepos) {
                inspos = idx;
                break;
            }
            x += width - tab_overlap_size;
        }
        if (inspos >= 0 && inspos != ts->dragging) {
            char str1[100];
            char str2[100];
            char strcursor1[100];
            char strcursor2[100];
            int pos1;
            int pos2;
            int cursor1;
            int cursor2;
            snprintf (str1, sizeof (str1), "playlist.scroll.%d", ts->dragging);
            pos1 = deadbeef->conf_get_int (str1, 0);
            snprintf (str2, sizeof (str2), "playlist.scroll.%d", inspos);
            pos2 = deadbeef->conf_get_int (str2, 0);

            snprintf (strcursor1, sizeof (strcursor1), "playlist.cursor.%d", ts->dragging);
            cursor1 = deadbeef->conf_get_int (strcursor1, 0);
            snprintf (strcursor2, sizeof (strcursor2), "playlist.cursor.%d", inspos);
            cursor2 = deadbeef->conf_get_int (strcursor2, 0);

            deadbeef->plt_move (ts->dragging, inspos);
            tab_moved = 1;
            deadbeef->conf_set_int (str1, pos2);
            deadbeef->conf_set_int (str2, pos1);
            deadbeef->conf_set_int (strcursor1, cursor2);
            deadbeef->conf_set_int (strcursor2, cursor1);
            ts->dragging = inspos;
            deadbeef->conf_set_int ("playlist.current", ts->dragging);
        }
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}

gboolean
on_tabstrip_drag_motion_event          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time)
{
    int tab = get_tab_under_cursor (DDB_TABSTRIP (widget), x);
    int prev = deadbeef->plt_get_curr_idx ();
    if (tab != -1 && tab != prev) {
        gtkui_playlist_set_curr (tab);
    }

    GtkWidget *pl = lookup_widget (mainwin, "playlist");
    GList *targets = gdk_drag_context_list_targets (drag_context);
    int cnt = g_list_length (targets);
    int i;
    for (i = 0; i < cnt; i++) {
        GdkAtom a = GDK_POINTER_TO_ATOM (g_list_nth_data (targets, i));
        gchar *nm = gdk_atom_name (a);
        if (!strcmp (nm, "text/uri-list")) {
            g_free (nm);
            break;
        }
        g_free (nm);
    }
    if (i != cnt) {
        gdk_drag_status (drag_context, GDK_ACTION_COPY, time);
    }
    else {
        GdkModifierType mask;

        gdk_window_get_pointer (gtk_widget_get_window (widget),
                NULL, NULL, &mask);
        if (mask & GDK_CONTROL_MASK) {
            gdk_drag_status (drag_context, GDK_ACTION_COPY, time);
        }
        else {
            gdk_drag_status (drag_context, GDK_ACTION_MOVE, time);
        }
    }
    return FALSE;
}
void
ddb_tabstrip_refresh (DdbTabStrip *ts) {
    gtk_widget_queue_draw (GTK_WIDGET (ts));
}
