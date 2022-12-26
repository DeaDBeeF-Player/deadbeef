/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#include <assert.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "ddbtabstrip.h"
#include "drawing.h"
#include "gtkui.h"
#include "interface.h"
#include "playlist/mainplaylist.h"
#include "support.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    g_object_ref (G_OBJECT (widget)), (GDestroyNotify) g_object_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)


G_DEFINE_TYPE (DdbTabStrip, ddb_tabstrip, GTK_TYPE_WIDGET);

extern GtkWidget *theme_button;
#define arrow_sz (ts->calculated_arrow_width)
#define arrow_widget_width (arrow_sz+4)
#define add_playlist_btn_width arrow_widget_width*2

enum {
    INFO_TARGET_URIS, // gtk sets this to 0 by default
    INFO_TARGET_PLAYLIST_ITEM_INDEXES,
    INFO_TARGET_PLAYITEM_POINTERS,
};

static void
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
        gtk_widget_set_can_focus (widget, TRUE);

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
        attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_KEY_PRESS_MASK |
            GDK_LEAVE_NOTIFY_MASK;

        attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
#if !GTK_CHECK_VERSION(3,0,0)
        attributes_mask |= GDK_WA_COLORMAP;
#endif

        GdkWindow *window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
        gtk_widget_set_window(widget, window);
        gdk_window_set_user_data (gtk_widget_get_window(widget), darea);

#if !GTK_CHECK_VERSION(3,0,0)
        widget->style = gtk_style_attach (widget->style, widget->window);
        gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
#else
        gtk_style_context_set_background (gtk_widget_get_style_context (widget), window);
#endif
    }

    ddb_tabstrip_send_configure (DDB_TABSTRIP (widget));
    GtkTargetEntry entries[] = {
        {
            .target = TARGET_PLAYLIST_AND_ITEM_INDEXES,
            .flags = GTK_TARGET_SAME_APP,
            .info = INFO_TARGET_PLAYLIST_ITEM_INDEXES
        },
        {
            .target = TARGET_PLAYITEM_POINTERS,
            .flags = GTK_TARGET_SAME_APP,
            .info = INFO_TARGET_PLAYITEM_POINTERS
        },
    };
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, entries, 2, GDK_ACTION_COPY | GDK_ACTION_MOVE);
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

gboolean
on_tabstrip_key_press_event            (GtkWidget    *widget,
                                        GdkEventKey  *event);
static int
get_tab_under_cursor (DdbTabStrip *ts, int x);

static void
ddb_tabstrip_unrealize(GtkWidget *w)
{
  DdbTabStrip *tabstrip;

  g_return_if_fail(w != NULL);
  g_return_if_fail(DDB_IS_TABSTRIP(w));

  tabstrip = DDB_TABSTRIP (w);
  draw_free (&tabstrip->drawctx);
  GTK_WIDGET_CLASS (ddb_tabstrip_parent_class)->unrealize (w);
}

gboolean
on_tabstrip_leave_notify_event (GtkWidget *, GdkEventCrossing *);

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
  widget_class->unrealize = ddb_tabstrip_unrealize;
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
  widget_class->key_press_event = on_tabstrip_key_press_event;
  widget_class->leave_notify_event = on_tabstrip_leave_notify_event;
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
    gchar *ptr=(char*)gtk_selection_data_get_data (data);
    int len = gtk_selection_data_get_length (data);
    if (target_type == INFO_TARGET_URIS) { // uris
        // this happens when dropped from file manager
        char *mem = malloc (len+1);
        memcpy (mem, ptr, len);
        mem[len] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        gtkui_receive_fm_drop (NULL, mem, len);
    }
    else if (target_type == INFO_TARGET_PLAYLIST_ITEM_INDEXES) {
        uint32_t *d= (uint32_t *)ptr;
        int plt = *d;
        d++;
        int length = (len/4)-1;
        ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
        if (p) {
            main_drag_n_drop (NULL, p, d, length, gdk_drag_context_get_selected_action (drag_context) == GDK_ACTION_COPY ? 1 : 0);
            deadbeef->plt_unref (p);
        }
    }
    else if (target_type == INFO_TARGET_PLAYITEM_POINTERS) {
        // FIXME!
    }
    gtk_drag_finish (drag_context, TRUE, FALSE, time);
}

void
on_tabstrip_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time)
{
    DdbTabStrip *ts = DDB_TABSTRIP(widget);
    if (ts->pick_drag_timer != 0) {
        g_source_remove(ts->pick_drag_timer);
    }
}

void
on_tabstrip_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context)
{
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
    drawctx_init (&tabstrip->drawctx);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_events (GTK_WIDGET (tabstrip), gtk_widget_get_events (GTK_WIDGET (tabstrip)) | GDK_SCROLL_MASK | GDK_KEY_PRESS_MASK);
#endif
}

static int tab_clicked = -1;
static int text_left_padding = 4;
static int text_right_padding = 0; // calculated from widget height
static int text_vert_offset = -2;
static int tab_overlap_size = 0; // widget_height/2
static int tabs_left_margin = 4;
static int min_tab_size = 80;
static int max_tab_size = 200;

static int tab_moved = 0;

typedef struct {
#if GTK_CHECK_VERSION(3,0,0)
    float x, y;
#else
    int x, y;
#endif
} coord_t;

#if GTK_CHECK_VERSION(3,0,0)
static void
cairo_draw_lines (cairo_t *cr, coord_t *pts, int cnt) {
    cairo_move_to (cr, pts[0].x+1, pts[0].y+1);
    for (int i = 1; i < cnt; i++) {
        cairo_line_to (cr, pts[i].x+1, pts[i].y+1);
    }
}

static void
cairo_draw_poly (cairo_t *cr, coord_t *pts, int cnt) {
    cairo_move_to (cr, pts[0].x, pts[0].y);
    for (int i = 1; i < cnt; i++) {
        cairo_line_to (cr, pts[i].x, pts[i].y);
    }
}
#endif

void
#if !GTK_CHECK_VERSION(3,0,0)
ddb_tabstrip_draw_tab (GtkWidget *widget, GdkDrawable *drawable, int idx, int selected, int x, int y, int w, int h, GtkStyle *style) {
#else
ddb_tabstrip_draw_tab (GtkWidget *widget, cairo_t *cr, int idx, int selected, int x, int y, int w, int h, GtkStyle *style) {
#endif
    coord_t points_filled[] = {
        { x+2, y + h },
        { x+2, y + 2 },
        { x + w - h + 1, y + 2 },
        { x + w - 1 + 1, y + h }
    };
    coord_t points_frame1[] = {
        { x, y + h-2 },
#if GTK_CHECK_VERSION(3,0,0)
        { x, y + 0.5 },
        { x + 0.5, y },
#else
        { x, y + 1 },
        { x + 1, y },
#endif
        { x + w - h - 1, y },
        { x + w - h + 1, y + 1 },
        { x + w - 3, y + h - 3 },
        { x + w - 0, y + h - 2 },
    };
    coord_t points_frame2[] = {
        { x + 1, y + h -1 },
        { x + 1, y + 1 },
        { x + w - h - 1, y + 1 },
        { x + w - h + 1, y + 2 },
        { x + w - 3, y + h - 2 },
        { x + w - 0, y + h - 1 },
    };
#if !GTK_CHECK_VERSION(3,0,0)
    GdkGC *bg = gdk_gc_new (drawable);
    GdkGC *outer_frame = gdk_gc_new (drawable);
    GdkGC *inner_frame = gdk_gc_new (drawable);
#endif
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
    int theming = !gtkui_override_tabstrip_colors ();
    if (selected) {
        if (fallback) {
            if (theming) {
                clr_bg = style->bg[GTK_STATE_NORMAL];
            } else {
                gtkui_get_tabstrip_base_color (&clr_bg);
            }
        }
        if (theming) {
            clr_outer_frame = style->dark[GTK_STATE_NORMAL];
            clr_inner_frame = style->light[GTK_STATE_NORMAL];
        } else {
            gtkui_get_tabstrip_dark_color (&clr_outer_frame);
            gtkui_get_tabstrip_light_color (&clr_inner_frame);
        }
    }
    else {
        if (fallback) {
            if (theming) {
                clr_bg = style->mid[GTK_STATE_NORMAL];
            } else {
                gtkui_get_tabstrip_mid_color (&clr_bg);
            }
        }
        if (theming) {
            clr_outer_frame = style->dark[GTK_STATE_NORMAL];
            clr_inner_frame = style->mid[GTK_STATE_NORMAL];
        } else {
            gtkui_get_tabstrip_dark_color (&clr_outer_frame);
            gtkui_get_tabstrip_mid_color (&clr_inner_frame);
        }
    }
#if !GTK_CHECK_VERSION(3,0,0)
    gdk_gc_set_rgb_fg_color (bg, &clr_bg);
    gdk_gc_set_rgb_fg_color (outer_frame, &clr_outer_frame);
    gdk_gc_set_rgb_fg_color (inner_frame, &clr_inner_frame);

    gdk_draw_polygon (drawable, bg, TRUE, (GdkPoint*)points_filled, sizeof (points_filled)/sizeof(coord_t));
    gdk_draw_lines (drawable, outer_frame, (GdkPoint*)points_frame1, sizeof (points_frame1)/sizeof(coord_t));
    gdk_draw_lines (drawable, inner_frame, (GdkPoint*)points_frame2, sizeof (points_frame2)/sizeof(coord_t));
    g_object_unref (bg);
    g_object_unref (outer_frame);
    g_object_unref (inner_frame);
#else
    cairo_set_source_rgb (cr, clr_bg.red/65535.f, clr_bg.green/65535.f, clr_bg.blue/65535.0);
    cairo_new_path (cr);
    cairo_draw_poly (cr, points_filled, sizeof (points_filled)/sizeof(coord_t));
    cairo_close_path (cr);
    cairo_fill (cr);
    cairo_set_source_rgb (cr, clr_outer_frame.red/65535.f, clr_outer_frame.green/65535.f, clr_outer_frame.blue/65535.0);
    cairo_draw_lines (cr, points_frame1, sizeof (points_frame1)/sizeof(coord_t));
    cairo_stroke (cr);
    cairo_set_source_rgb (cr, clr_inner_frame.red/65535.f, clr_inner_frame.green/65535.f, clr_inner_frame.blue/65535.0);
    cairo_draw_lines (cr, points_frame2, sizeof (points_frame2)/sizeof(coord_t));
    cairo_stroke (cr);
#endif
}

int
ddb_tabstrip_get_tab_width (DdbTabStrip *ts, int tab) {
    int width;
    char title[1000];
    plt_get_title_wrapper (tab, title, sizeof (title));
    int h = 0;
    draw_get_text_extents (&ts->drawctx, title, (int)strlen (title), &width, &h);
    width += text_left_padding + text_right_padding;
    if (width < min_tab_size) {
        width = min_tab_size;
    }
    else if (width > max_tab_size) {
        width = max_tab_size;
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
        if (w >= a.width-add_playlist_btn_width-tab_overlap_size) {
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
    int tabarea_width = a.width-add_playlist_btn_width;
    int boundary = tabarea_width - arrow_widget_width*2 + ts->hscrollpos;
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
            int tabarea_width = a.width-add_playlist_btn_width;
            int cnt = deadbeef->plt_get_count ();
            for (int idx = 0; idx < cnt; idx++) {
                w += ddb_tabstrip_get_tab_width (ts, idx) - tab_overlap_size;
            }
            w += tab_overlap_size + 3;
            if (ts->hscrollpos > w - (tabarea_width - arrow_widget_width*2)) {
                ts->hscrollpos = w - (tabarea_width - arrow_widget_width*2);
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
set_tab_text_color (DdbTabStrip *ts, int idx, int selected, int playing, GtkStyle *style) {
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
            draw_set_fg_color (&ts->drawctx, fg);
        }
    }
    deadbeef->plt_unref (plt);
    if (fallback) {
        GdkColor color;
        if (gtkui_override_tabstrip_colors ()) {
            if (idx == selected) {
                gtkui_get_tabstrip_selected_text_color (&color);
            }
            else if (idx == playing) {
                gtkui_get_tabstrip_playing_text_color (&color);
            }
            else {
                gtkui_get_tabstrip_text_color (&color);
            }
        }
        else {
            color = style->text[GTK_STATE_NORMAL];
        }
        float fg[3] = {(float)color.red/0xffff, (float)color.green/0xffff, (float)color.blue/0xffff};
        draw_set_fg_color (&ts->drawctx, fg);
    }
    deadbeef->pl_unlock ();
}

void
tabstrip_render (DdbTabStrip *ts, cairo_t *cr) {
    GtkWidget *widget = GTK_WIDGET (ts);
#if !GTK_CHECK_VERSION(3,0,0)
    GdkDrawable *backbuf = gtk_widget_get_window (widget);
#endif
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    a.height = ts->calculated_height;

    tabstrip_adjust_hscroll (ts);
    cairo_set_line_width (cr, 1);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    int cnt = deadbeef->plt_get_count ();
    int hscroll = ts->hscrollpos;

    int theming = !gtkui_override_tabstrip_colors ();
    int need_arrows = tabstrip_need_arrows (ts);
    if (need_arrows) {
        hscroll -= arrow_widget_width;
    }

    int x = -hscroll;
    int w = 0;
    int h = draw_get_font_size (&ts->drawctx);
    h = a.height;
    tab_overlap_size = (h-4)/2;
    text_right_padding = h - 3;

    int tab_selected = deadbeef->plt_get_curr_idx ();
    if (tab_selected == -1) {
        return;
    }

    int tab_playing = -1;
    DB_playItem_t *playing = deadbeef->streamer_get_playing_track_safe ();
    if (playing) {
        ddb_playlist_t *plt = deadbeef->pl_get_playlist (playing);
        if (plt) {
            tab_playing = deadbeef->plt_get_idx (plt);
            deadbeef->plt_unref (plt);
        }
        deadbeef->pl_item_unref (playing);
    }

    GtkStyle *style = gtk_widget_get_style (widget);
    GdkColor clr_mid, clr_dark, clr_base;
    if (theming) {
        clr_mid = style->mid[GTK_STATE_NORMAL];
        clr_dark = style->dark[GTK_STATE_NORMAL];
        clr_base = style->base[GTK_STATE_NORMAL];
    } else {
        gtkui_get_tabstrip_mid_color (&clr_mid);
        gtkui_get_tabstrip_dark_color (&clr_dark);
        gtkui_get_tabstrip_base_color (&clr_base);
    }
#if !GTK_CHECK_VERSION(3,0,0)
    GdkGC *gc = gdk_gc_new (backbuf);
    // fill background
    gdk_gc_set_rgb_fg_color (gc, &clr_mid);
    gdk_draw_rectangle (backbuf, gc, TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
    gdk_gc_set_rgb_fg_color (gc, &clr_dark);
    gdk_draw_line (backbuf, gc, 0, 0, widget->allocation.width, 0);
#else

    // fill background
    cairo_set_source_rgb (cr, clr_mid.red/65535.f, clr_mid.green/65535.f, clr_mid.blue/65535.0);
    cairo_rectangle (cr, 0, 0, a.width, a.height);
    cairo_fill (cr);

    cairo_set_source_rgb (cr, clr_dark.red/65535.f, clr_dark.green/65535.f, clr_dark.blue/65535.0);
    cairo_move_to (cr, 0, 1);
    cairo_line_to (cr, a.width, 1);
    cairo_stroke (cr);
#endif

    int y = 4;
    h = a.height - 4;
    draw_begin (&ts->drawctx, cr);
    int need_draw_moving = 0;
    int idx;
    int widths[cnt];
    for (idx = 0; idx < cnt; idx++) {
        char title[1000];
        plt_get_title_wrapper (idx, title, sizeof (title));
        int h = 0;
        draw_get_text_extents (&ts->drawctx, title, (int)strlen (title), &widths[idx], &h);
        widths[idx] += text_left_padding + text_right_padding;
        if (widths[idx] < min_tab_size) {
            widths[idx] = min_tab_size;
        }
        else if (widths[idx] > max_tab_size) {
            widths[idx] = max_tab_size;
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
#if !GTK_CHECK_VERSION(3,0,0)
            ddb_tabstrip_draw_tab (widget, backbuf, idx, idx == tab_selected, x, y, w, h, style);
#else
            ddb_tabstrip_draw_tab (widget, cr, idx, idx == tab_selected, x, y, w, h, style);
#endif
            char tab_title[1000];
            plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));

            set_tab_text_color (ts, idx, tab_selected, tab_playing, style);
            int bold = 0;
            int italic = 0;
            if (!theming && idx == tab_playing) {
                italic = gtkui_tabstrip_italic_playing;
                bold = gtkui_tabstrip_embolden_playing;
            }
            draw_text_custom (&ts->drawctx, x + text_left_padding, y - text_vert_offset, w - (text_left_padding + text_right_padding - 1), 0, DDB_TABSTRIP_FONT, bold, italic, tab_title);
        }
        x += w - tab_overlap_size;
    }

#if !GTK_CHECK_VERSION(3,0,0)
    gdk_draw_line (backbuf, style->dark_gc[GTK_STATE_NORMAL], 0, widget->allocation.height-2, widget->allocation.width, widget->allocation.height-2);
    gdk_draw_line (backbuf, style->light_gc[GTK_STATE_NORMAL], 0, widget->allocation.height-1, widget->allocation.width, widget->allocation.height-1);
#else
    GdkColor *pclr = &style->dark[GTK_STATE_NORMAL];
    cairo_set_source_rgb (cr, pclr->red/65535.f, pclr->green/65535.f, pclr->blue/65535.0);
    cairo_move_to (cr, 0, a.height-1);
    cairo_line_to (cr, a.width, a.height-1);
    cairo_stroke (cr);
    pclr = &style->light[GTK_STATE_NORMAL];
    cairo_set_source_rgb (cr, pclr->red/65535.f, pclr->green/65535.f, pclr->blue/65535.0);
    cairo_move_to (cr, 0, a.height);
    cairo_line_to (cr, a.width, a.height);
    cairo_stroke (cr);
#endif
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
#if !GTK_CHECK_VERSION(3,0,0)
        ddb_tabstrip_draw_tab (widget, backbuf, idx, 1, x, y, w, h, style);
#else
        ddb_tabstrip_draw_tab (widget, cr, idx, 1, x, y, w, h, style);
#endif
        char tab_title[1000];
        plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));
        set_tab_text_color (ts, idx, tab_selected, -1, style);
        int bold = 0;
        int italic = 0;
        if (!theming) {
            bold = gtkui_tabstrip_embolden_selected;
            italic = gtkui_tabstrip_italic_selected;
        }
        if (gtk_widget_is_focus (GTK_WIDGET (ts))) {
#if GTK_CHECK_VERSION(3,0,0)
            gtk_render_focus (gtk_widget_get_style_context (widget), cr, x, y, w - ( text_right_padding - 1), ts->row_height);
#else
            GdkColor clr;
            gtkui_get_tabstrip_text_color (&clr);
            gdk_gc_set_rgb_fg_color (gc, &clr);
            gdk_gc_set_line_attributes (gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
            gdk_gc_set_dashes (gc, 0, (gint8[]) {1,1}, 2);
            gdk_draw_rectangle (backbuf, gc, FALSE, x, y , w - (text_right_padding - 1), ts->row_height-2);
#endif
        }
        draw_text_custom (&ts->drawctx, x + text_left_padding, y - text_vert_offset, w - (text_left_padding + text_right_padding - 1), 0, DDB_TABSTRIP_FONT, bold, italic, tab_title);
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
#if !GTK_CHECK_VERSION(3,0,0)
                    ddb_tabstrip_draw_tab (widget, backbuf, idx, 1, x, y, w, h, style);
#else
                    ddb_tabstrip_draw_tab (widget, cr, idx, 1, x, y, w, h, style);
#endif
                    char tab_title[1000];
                    plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));
                    set_tab_text_color (ts, idx, tab_selected, -1, style);
                    int bold = 0;
                    int italic = 0;
                    if (!theming) {
                        bold = gtkui_tabstrip_embolden_selected;
                        italic = gtkui_tabstrip_italic_selected;
                    }
                    draw_text_custom (&ts->drawctx, x + text_left_padding, y - text_vert_offset, w - (text_left_padding + text_right_padding - 1), 0, DDB_TABSTRIP_FONT, bold, italic, tab_title);
                }
                break;
            }
            x += w - tab_overlap_size;
        }
    }
    int tabarea_width = a.width - add_playlist_btn_width;
#if !GTK_CHECK_VERSION(3,0,0)
    if (need_arrows) {
        int sz = widget->allocation.height-3;
        gdk_gc_set_rgb_fg_color (gc, &clr_mid);
        gdk_draw_rectangle (backbuf, gc, TRUE, 0, 1, arrow_widget_width, sz);
        gtk_paint_arrow (style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_LEFT, TRUE, 2, sz/2-arrow_sz/2, arrow_sz, arrow_sz);
        gdk_draw_rectangle (backbuf, gc, TRUE, tabarea_width-arrow_widget_width, 1, arrow_widget_width, sz);
        gtk_paint_arrow (style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_RIGHT, TRUE, tabarea_width-arrow_sz-2, 1+sz/2-arrow_sz/2, arrow_sz, arrow_sz);
    }
#else
    if (need_arrows) {
        int sz = a.height-3;
        cairo_set_source_rgb (cr, clr_mid.red/65535.f, clr_mid.green/65535.f, clr_mid.blue/65535.0);
        cairo_rectangle (cr, 0, 1, arrow_widget_width, sz);
        cairo_fill (cr);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_paint_arrow (style, cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, widget, NULL, GTK_ARROW_LEFT, TRUE, 2, sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#else
        gtk_paint_arrow (style, gtk_widget_get_window(widget), GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_LEFT, TRUE, 2, sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#endif

        cairo_rectangle (cr, tabarea_width-arrow_widget_width, 1, arrow_widget_width, sz);
        cairo_fill (cr);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_paint_arrow (style, cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, widget, NULL, GTK_ARROW_RIGHT, TRUE, tabarea_width-arrow_sz-2, 1+sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#else
        gtk_paint_arrow (style, gtk_widget_get_window(widget), GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_RIGHT, TRUE, a.width-arrow_sz-2, 1+sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#endif
    }
#endif

    // Draw add playlist button
    {
        int playlist_button_border = 3;
        int add_playlist_btn_height = a.height-playlist_button_border;
        GdkColor textcolor, clr;
        if (ts->add_playlistbtn_hover) {
            clr = clr_base;
            if (theming) {
                textcolor = style->text[GTK_STATE_NORMAL];
            } else {
                gtkui_get_tabstrip_selected_text_color (&textcolor);
            }
        } else {
            clr = clr_mid;
            if (theming) {
                textcolor = style->text[GTK_STATE_NORMAL];
            } else {
                gtkui_get_tabstrip_text_color (&textcolor);
            }
        }
        float fg[3] = {(float)textcolor.red/0xffff, (float)textcolor.green/0xffff, (float)textcolor.blue/0xffff};
#if !GTK_CHECK_VERSION(3,0,0)
        gdk_gc_set_rgb_fg_color (gc, &clr);
        gdk_draw_rectangle (backbuf, gc, TRUE, tabarea_width, 1, add_playlist_btn_width, add_playlist_btn_height);

        draw_set_fg_color (&ts->drawctx, fg);
        draw_text_custom (&ts->drawctx, tabarea_width, playlist_button_border, add_playlist_btn_width, 2, DDB_TABSTRIP_FONT, TRUE, FALSE, "+");
#else
        cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.0);
        cairo_rectangle (cr, tabarea_width, 1, add_playlist_btn_width, add_playlist_btn_height);
        cairo_fill (cr);

        draw_set_fg_color (&ts->drawctx, fg);
        draw_text_custom (&ts->drawctx, tabarea_width, playlist_button_border, add_playlist_btn_width, 2, DDB_TABSTRIP_FONT, TRUE, FALSE, "+");
#endif
    }

    draw_end (&ts->drawctx);

#if !GTK_CHECK_VERSION(3,0,0)
    gdk_gc_unref (gc);
#endif
}

static int
get_tab_under_cursor (DdbTabStrip *ts, int x) {
    int hscroll = ts->hscrollpos;
    int need_arrows = tabstrip_need_arrows (ts);

    GtkAllocation a;
    int buttons_width = add_playlist_btn_width + (need_arrows ? arrow_widget_width : 0);
    gtk_widget_get_allocation (GTK_WIDGET(ts), &a);
    if (x > a.width - buttons_width) {
        return -1;
    }
    if (need_arrows && x < arrow_widget_width) {
        return -1;
    }

    if (need_arrows) {
        hscroll -= arrow_widget_width;
    }
    int idx;
    int cnt = deadbeef->plt_get_count ();
    int fw = tabs_left_margin - hscroll;
    for (idx = 0; idx < cnt; idx++) {
        char title[1000];
        plt_get_title_wrapper (idx, title, sizeof (title));
        int w = 0;
        int h = 0;
        draw_get_text_extents (&ts->drawctx, title, (int)strlen (title), &w, &h);
        w += text_left_padding + text_right_padding;
        if (w < min_tab_size) {
            w = min_tab_size;
        }
        else if (w > max_tab_size) {
            w = max_tab_size;
        }
        fw += w;
        fw -= tab_overlap_size;
        if (fw > x) {
            return idx;
        }
    }
    return -1;
}

static void
tabstrip_scroll_left (DdbTabStrip *ts) {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab > 0) {
        tab--;
        deadbeef->plt_set_curr_idx (tab);
    }
    tabstrip_scroll_to_tab (ts, tab);
}

static void
tabstrip_scroll_right (DdbTabStrip *ts) {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab < deadbeef->plt_get_count ()-1) {
        tab++;
        deadbeef->plt_set_curr_idx (tab);
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
    if (TEST_LEFT_CLICK(event)) {
        if (tab_clicked == deadbeef->plt_get_curr_idx ()) {
            gtk_widget_grab_focus (widget);
        }
        int need_arrows = tabstrip_need_arrows (ts);
        GtkAllocation a;
        gtk_widget_get_allocation (widget, &a);
        if (need_arrows) {
            if (event->x < arrow_widget_width) {
                if (event->type == GDK_BUTTON_PRESS) {
                    tabstrip_scroll_left (ts);
                    ts->scroll_direction = -1;
                    ts->scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                }
                return TRUE;
            }
            else if (event->x >= a.width - (arrow_widget_width + add_playlist_btn_width) && event->x < a.width - add_playlist_btn_width) {
                if (event->type == GDK_BUTTON_PRESS) {
                    tabstrip_scroll_right (ts);
                    ts->scroll_direction = 1;
                    ts->scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                }
                return TRUE;
            }
        }
        if (event->x > a.width - add_playlist_btn_width) {
            int playlist = gtkui_add_new_playlist ();
            if (playlist != -1) {
                deadbeef->plt_set_curr_idx (playlist);
            }
            return TRUE;
        }

        if (tab_clicked != -1) {
            deadbeef->plt_set_curr_idx (tab_clicked);

            if (event->type == GDK_2BUTTON_PRESS) {
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                int cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
                deadbeef->plt_unref (plt);

                if (cur == -1) {
                    cur = 0;
                }
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, cur, 0);
            }
        }
        else {
            if (event->type == GDK_2BUTTON_PRESS) {
                // new tab
                int playlist = gtkui_add_new_playlist ();
                if (playlist != -1) {
                    deadbeef->plt_set_curr_idx (playlist);
                }
                return TRUE;
            }
            return TRUE;
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
    else if (TEST_RIGHT_CLICK(event)) {
        ddb_playlist_t *plt = deadbeef->plt_get_for_idx (tab_clicked);
        GtkWidget *menu = gtkui_create_pltmenu (plt);
        if (plt != NULL) {
            deadbeef->plt_unref (plt);
        }
        gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
    }
    else if (event->button == 2) {
        if (tab_clicked == -1) {
            // new tab
            int playlist = gtkui_add_new_playlist ();
            if (playlist != -1) {
                deadbeef->plt_set_curr_idx (playlist);
            }
            return TRUE;
        }
        else if (deadbeef->conf_get_int ("gtkui.mmb_delete_playlist", 1)) {
            if (tab_clicked != -1) {
                ddb_playlist_t *plt = deadbeef->plt_get_for_idx(tab_clicked);
                if (plt != NULL) {
                    gtkui_remove_playlist(plt);
                    deadbeef->plt_unref (plt);
                }
            }
        }
    }
    return TRUE;
}


gboolean
on_tabstrip_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    if (event->button == 1) {
        if (ts->scroll_timer > 0) {
            ts->scroll_direction = 0;
            g_source_remove (ts->scroll_timer);
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
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    draw_init_font (&ts->drawctx, DDB_TABSTRIP_FONT, 1);
    tabstrip_adjust_hscroll (ts);
    ts->row_height = draw_get_listview_rowheight (&ts->drawctx);
    int height = ts->row_height + 4;
    ts->calculated_height = height;
    int w;
    draw_get_text_extents(&ts->drawctx, ">", 1, &w, NULL);
    ts->calculated_arrow_width = w;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    if (height != a.height) {
        gtk_widget_set_size_request (widget, -1, height);
    }
    return FALSE;
}

static void
tabstrip_update_font (DdbTabStrip *ts, GtkWidget *widget)
{
    draw_init_font (&ts->drawctx, DDB_TABSTRIP_FONT, 1);
    tabstrip_adjust_hscroll (ts);
    int height = draw_get_listview_rowheight (&ts->drawctx) + 4;
    ts->calculated_height = height;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    if (height != a.height) {
        gtk_widget_set_size_request (widget, -1, height);
    }
}

gboolean
on_tabstrip_draw (GtkWidget *widget, cairo_t *cr)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    tabstrip_update_font (ts, widget);
    tabstrip_render (ts, cr);
    return FALSE;
}

gboolean
on_tabstrip_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    tabstrip_update_font (ts, widget);

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
    int ev_x;
    GdkModifierType ev_state;
    ev_x = event->x;
    ev_state = event->state;
#if GTK_CHECK_VERSION(2,12,0)
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
            deadbeef->plt_move (ts->dragging, inspos);
            tab_moved = 1;
            ts->dragging = inspos;
            deadbeef->plt_set_curr_idx(ts->dragging);
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_POSITION, 0);
        }
        gtk_widget_queue_draw (widget);
    }
    else {
        int tab = get_tab_under_cursor (DDB_TABSTRIP (widget), event->x);
        if (tab >= 0) {
            char s[1000];
            plt_get_title_wrapper (tab, s, sizeof (s));

            int width;
            int height;
            draw_get_text_extents (&ts->drawctx, s, (int)strlen (s), &width, &height);
            width += text_left_padding + text_right_padding;
            if (width > max_tab_size) {
                gtk_widget_set_tooltip_text (widget, s);
                gtk_widget_set_has_tooltip (widget, TRUE);
            }
            else {
                gtk_widget_set_has_tooltip (widget, FALSE);
            }
        }
        else {
            gtk_widget_set_has_tooltip (widget, FALSE);
        }
        GtkAllocation a;
        gtk_widget_get_allocation(widget, &a);
        ts->add_playlistbtn_hover = (ev_x > a.width - add_playlist_btn_width) ? 1 : 0;
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}

static gboolean
_tabstrip_drag_pick (void *ctx) {
    GtkWidget *widget = ctx;
    DdbTabStrip *ts = DDB_TABSTRIP(widget);
    gint x, y;
    gtk_widget_get_pointer(widget, &x, &y);
    int tab = get_tab_under_cursor (DDB_TABSTRIP (widget), x);
    int prev = deadbeef->plt_get_curr_idx ();
    if (tab != -1 && tab != prev) {
        deadbeef->plt_set_curr_idx (tab);
    }
    ts->pick_drag_timer = 0;
    return FALSE;
}

gboolean
on_tabstrip_drag_motion_event          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time)
{
    DdbTabStrip *ts = DDB_TABSTRIP(widget);
    if (ts->pick_drag_timer != 0) {
        g_source_remove(ts->pick_drag_timer);
    }
    ts->pick_drag_timer = g_timeout_add(500, _tabstrip_drag_pick, widget);

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

gboolean
on_tabstrip_key_press_event            (GtkWidget    *widget,
                                        GdkEventKey  *event)
{
    switch (event->keyval) {
    case GDK_Left:
        tabstrip_scroll_left(DDB_TABSTRIP(widget));
        return TRUE;
    case GDK_Right:
        tabstrip_scroll_right(DDB_TABSTRIP(widget));
        return TRUE;
    case GDK_F2:
        {
            int idx = deadbeef->plt_get_curr_idx ();
            if (idx != -1) {
                gtkui_rename_playlist_at_index(idx);
            }
        }
        break;
    }
    return FALSE;
}

gboolean
on_tabstrip_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event_crossing)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);

    ts->add_playlistbtn_hover = 0;
    gtk_widget_queue_draw(widget);

    return FALSE;
}

