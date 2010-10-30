/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
    deadbeef->plt_get_title (plt, buffer, len);
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

    event->configure.window = g_object_ref (widget->window);
    event->configure.send_event = TRUE;
    event->configure.x = widget->allocation.x;
    event->configure.y = widget->allocation.y;
    event->configure.width = widget->allocation.width;
    event->configure.height = widget->allocation.height;

    gtk_widget_event (widget, event);
    gdk_event_free (event);
}

static void
ddb_tabstrip_realize (GtkWidget *widget) {
    DdbTabStrip *darea = DDB_TABSTRIP (widget);
    GdkWindowAttr attributes;
    gint attributes_mask;

    if (GTK_WIDGET_FLAGS (widget)&GTK_NO_WINDOW/*GTK_WIDGET_NO_WINDOW (widget)*/)
    {
        GTK_WIDGET_CLASS (ddb_tabstrip_parent_class)->realize (widget);
    }
    else
    {
        GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

        attributes.window_type = GDK_WINDOW_CHILD;
        attributes.x = widget->allocation.x;
        attributes.y = widget->allocation.y;
        attributes.width = widget->allocation.width;
        attributes.height = widget->allocation.height;
        attributes.wclass = GDK_INPUT_OUTPUT;
        attributes.visual = gtk_widget_get_visual (widget);
        attributes.colormap = gtk_widget_get_colormap (widget);
        attributes.event_mask = gtk_widget_get_events (widget);
        attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK;

        attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

        widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
                &attributes, attributes_mask);
        gdk_window_set_user_data (widget->window, darea);

        widget->style = gtk_style_attach (widget->style, widget->window);
        gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
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

  widget->allocation = *allocation;

  if (GTK_WIDGET_FLAGS(widget)&GTK_REALIZED/*GTK_WIDGET_REALIZED (widget)*/)
    {
      if (!(GTK_WIDGET_FLAGS (widget)&GTK_NO_WINDOW)/*GTK_WIDGET_NO_WINDOW (widget)*/)
        gdk_window_move_resize (widget->window,
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);

      ddb_tabstrip_send_configure (DDB_TABSTRIP (widget));
    }
}

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
ddb_tabstrip_destroy(GtkObject *object)
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
  GtkObjectClass *object_class = (GtkObjectClass *) class;
  widget_class->realize = ddb_tabstrip_realize;
  widget_class->size_allocate = ddb_tabstrip_size_allocate;
  widget_class->expose_event = on_tabstrip_expose_event;
  widget_class->button_press_event = on_tabstrip_button_press_event;
  widget_class->button_release_event = on_tabstrip_button_release_event;
  widget_class->configure_event = on_tabstrip_configure_event;
  widget_class->motion_notify_event = on_tabstrip_motion_notify_event;
  widget_class->drag_motion = on_tabstrip_drag_motion_event;
  widget_class->drag_drop = on_tabstrip_drag_drop;
  widget_class->drag_end = on_tabstrip_drag_end;
  widget_class->drag_data_received = on_tabstrip_drag_data_received;
  widget_class->drag_leave = on_tabstrip_drag_leave;

  object_class->destroy = ddb_tabstrip_destroy;
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

    gchar *ptr=(char*)data->data;
    if (target_type == 0) { // uris
        // this happens when dropped from file manager
        char *mem = malloc (data->length+1);
        memcpy (mem, ptr, data->length);
        mem[data->length] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        ps->binding->external_drag_n_drop (NULL, mem, data->length);
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        int plt = *d;
        d++;
        int length = (data->length/4)-1;
        ps->binding->drag_n_drop (NULL, plt, d, length, drag_context->action == GDK_ACTION_COPY ? 1 : 0);
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

void
ddb_tabstrip_draw_tab (GtkWidget *widget, GdkDrawable *drawable, int selected, int x, int y, int w, int h) {
    GdkPoint points_filled[] = {
        { x+2, y + h },
        { x+2, y + 2 },
        { x + w - h + 1, y + 2 },
        { x + w - 1 + 1, y + h }
    };
    GdkPoint points_frame1[] = {
        { x, y + h-2 },
        { x, y + 1 },
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
        { x + 1, y + 1 },
        { x + w - h - 1, y + 1 },
        { x + w - h, y + 2 },
        { x + w - h + 1, y + 2 },
        { x + w-3, y + h - 2 },
        { x + w-2, y + h - 2 },
    };
    //gdk_draw_rectangle (widget->window, widget->style->black_gc, FALSE, x-1, y-1, w+2, h+2);
    GdkGC *bg = gdk_gc_new (drawable);
    GdkGC *outer_frame = gdk_gc_new (drawable);
    GdkGC *inner_frame = gdk_gc_new (drawable);
    GdkColor clr;
    if (selected) {
        gdk_gc_set_rgb_fg_color (bg, (gtkui_get_tabstrip_base_color (&clr), &clr));//&widget->style->bg[GTK_STATE_NORMAL]); // FIXME: need base color
        gdk_gc_set_rgb_fg_color (outer_frame, (gtkui_get_tabstrip_dark_color (&clr), &clr));
        gdk_gc_set_rgb_fg_color (inner_frame, (gtkui_get_tabstrip_light_color (&clr), &clr));
    }
    else {
        gdk_gc_set_rgb_fg_color (bg, (gtkui_get_tabstrip_mid_color (&clr), &clr));
        gdk_gc_set_rgb_fg_color (outer_frame, (gtkui_get_tabstrip_dark_color (&clr), &clr));
        gdk_gc_set_rgb_fg_color (inner_frame, (gtkui_get_tabstrip_mid_color (&clr), &clr));
    }
    gdk_draw_polygon (drawable, bg, TRUE, points_filled, 4);
    gdk_draw_lines (drawable, outer_frame, points_frame1, 9);
    gdk_draw_lines (drawable, inner_frame, points_frame2, 7);
    g_object_unref (bg);
    g_object_unref (outer_frame);
    g_object_unref (inner_frame);
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
    for (int idx = 0; idx < cnt; idx++) {
        w += ddb_tabstrip_get_tab_width (ts, idx) - tab_overlap_size;
        if (w >= widget->allocation.width) {
            return 1;
        }
    }
    w += tab_overlap_size + 3;
    if (w >= widget->allocation.width) {
        return 1;
    }
    return 0;
}

static void
tabstrip_scroll_to_tab_int (DdbTabStrip *ts, int tab, int redraw) {
    GtkWidget *widget = GTK_WIDGET (ts);
    int w = 0;
    int cnt = deadbeef->plt_get_count ();
    int boundary = widget->allocation.width - arrow_widget_width*2 + ts->hscrollpos;
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
            int w = 0;
            int cnt = deadbeef->plt_get_count ();
            for (int idx = 0; idx < cnt; idx++) {
                w += ddb_tabstrip_get_tab_width (ts, idx) - tab_overlap_size;
            }
            w += tab_overlap_size + 3;
            if (ts->hscrollpos > w - (widget->allocation.width - arrow_widget_width*2)) {
                ts->hscrollpos = w - (widget->allocation.width - arrow_widget_width*2);
                deadbeef->conf_set_int ("gtkui.tabscroll", ts->hscrollpos);
            }
            tabstrip_scroll_to_tab_int (ts, deadbeef->plt_get_curr (), 0);
        }
        else {
            ts->hscrollpos = 0;
            deadbeef->conf_set_int ("gtkui.tabscroll", ts->hscrollpos);
        }
    }
}

void
tabstrip_render (DdbTabStrip *ts) {
    GtkWidget *widget = GTK_WIDGET (ts);
    GdkDrawable *backbuf = gtk_widget_get_window (widget);

    tabstrip_adjust_hscroll (ts);

    int cnt = deadbeef->plt_get_count ();
    int hscroll = ts->hscrollpos;

    int need_arrows = tabstrip_need_arrows (ts);
    if (need_arrows) {
        hscroll -= arrow_widget_width;
    }

    int x = -hscroll;
    int w = 0;
    int h = draw_get_font_size ();
    h = widget->allocation.height;
    tab_overlap_size = (h-4)/2;
    text_right_padding = h - 3;

    const char *detail = "button";
    int tab_selected = deadbeef->plt_get_curr ();

    GdkGC *gc = gdk_gc_new (backbuf);

    // fill background
    GdkColor clr;
    gdk_gc_set_rgb_fg_color (gc, (gtkui_get_tabstrip_mid_color (&clr), &clr));
    gdk_draw_rectangle (backbuf, gc, TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
    gdk_gc_set_rgb_fg_color (gc, (gtkui_get_tabstrip_dark_color (&clr), &clr));
    gdk_draw_line (backbuf, gc, 0, 0, widget->allocation.width, 0);
    int y = 4;
    h = widget->allocation.height - 4;
    draw_begin ((uintptr_t)backbuf);
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
            ddb_tabstrip_draw_tab (widget, backbuf, idx == tab_selected, x, y, w, h);
            char tab_title[100];
            plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));
            GdkColor *color = &widget->style->text[GTK_STATE_NORMAL];
            float fg[3] = {(float)color->red/0xffff, (float)color->green/0xffff, (float)color->blue/0xffff};
            draw_set_fg_color (fg);
            draw_text (x + text_left_padding, y + h/2 - draw_get_font_size()/2 + text_vert_offset, w, 0, tab_title);
        }
        x += w - tab_overlap_size;
    }
    gdk_draw_line (backbuf, widget->style->dark_gc[GTK_STATE_NORMAL], 0, widget->allocation.height-2, widget->allocation.width, widget->allocation.height-2);
    gdk_draw_line (backbuf, widget->style->light_gc[GTK_STATE_NORMAL], 0, widget->allocation.height-1, widget->allocation.width, widget->allocation.height-1);
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
        ddb_tabstrip_draw_tab (widget, backbuf, 1, x, y, w, h);
        char tab_title[100];
        plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));
        GdkColor *color = &widget->style->text[GTK_STATE_NORMAL];
        float fg[3] = {(float)color->red/0xffff, (float)color->green/0xffff, (float)color->blue/0xffff};
        draw_set_fg_color (fg);
        draw_text (x + text_left_padding, y + h/2 - draw_get_font_size()/2 + text_vert_offset, w, 0, tab_title);
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
                if (x >= widget->allocation.width) {
                    break;
                }
                if (w > 0) {
                    ddb_tabstrip_draw_tab (widget, backbuf, 1, x, y, w, h);
                    char tab_title[100];
                    plt_get_title_wrapper (idx, tab_title, sizeof (tab_title));
                    GdkColor *color = &widget->style->text[GTK_STATE_NORMAL];
                    float fg[3] = {(float)color->red/0xffff, (float)color->green/0xffff, (float)color->blue/0xffff};
                    draw_set_fg_color (fg);
                    draw_text (x + text_left_padding, y + h/2 - draw_get_font_size()/2 + text_vert_offset, w, 0, tab_title);
                }
                break;
            }
            x += w - tab_overlap_size;
        }
    }

    if (need_arrows) {
        int sz = widget->allocation.height-3;
        GdkColor clr;
        gdk_gc_set_rgb_fg_color (gc, (gtkui_get_tabstrip_mid_color (&clr), &clr));
        gdk_draw_rectangle (backbuf, gc, TRUE, 0, 1, arrow_widget_width, sz);
        gtk_paint_arrow (widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_LEFT, TRUE, 2, sz/2-arrow_sz/2, arrow_sz, arrow_sz);
        gdk_draw_rectangle (backbuf, gc, TRUE, widget->allocation.width-arrow_widget_width, 1, arrow_widget_width, sz);
        gtk_paint_arrow (widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_RIGHT, TRUE, widget->allocation.width-arrow_sz-2, 1+sz/2-arrow_sz/2, arrow_sz, arrow_sz);
    }

    draw_end ();
    g_object_unref (gc);
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
    int tab_selected = deadbeef->plt_get_curr ();
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
    GtkWidget *dlg = create_editplaylistdlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Edit playlist"));
    GtkWidget *e = lookup_widget (dlg, "title");
    char t[100];
    plt_get_title_wrapper (tab_clicked, t, sizeof (t));
    gtk_entry_set_text (GTK_ENTRY (e), t);
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        const char *text = gtk_entry_get_text (GTK_ENTRY (e));
        deadbeef->plt_set_title (tab_clicked, text);
        extern GtkWidget *mainwin;
    }
    gtk_widget_destroy (dlg);
}


void
on_remove_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (tab_clicked != -1) {
        deadbeef->plt_remove (tab_clicked);
        int playlist = deadbeef->plt_get_curr ();
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

#if 0
void
on_load_playlist1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save_playlist1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save_all_playlists1_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}
#endif

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
  gtk_widget_show (rename_playlist1);
  gtk_container_add (GTK_CONTAINER (plmenu), rename_playlist1);

  remove_playlist1 = gtk_menu_item_new_with_mnemonic (_("Remove Playlist"));
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


#if 0
  separator11 = gtk_separator_menu_item_new ();
  gtk_widget_show (separator11);
  gtk_container_add (GTK_CONTAINER (plmenu), separator11);
  gtk_widget_set_sensitive (separator11, FALSE);

  load_playlist1 = gtk_menu_item_new_with_mnemonic ("Load Playlist");
  gtk_widget_show (load_playlist1);
  gtk_container_add (GTK_CONTAINER (plmenu), load_playlist1);

  save_playlist1 = gtk_menu_item_new_with_mnemonic ("Save Playlist");
  gtk_widget_show (save_playlist1);
  gtk_container_add (GTK_CONTAINER (plmenu), save_playlist1);

  save_all_playlists1 = gtk_menu_item_new_with_mnemonic ("Save All Playlists");
  gtk_widget_show (save_all_playlists1);
  gtk_container_add (GTK_CONTAINER (plmenu), save_all_playlists1);

  g_signal_connect ((gpointer) load_playlist1, "activate",
                    G_CALLBACK (on_load_playlist1_activate),
                    NULL);
  g_signal_connect ((gpointer) save_playlist1, "activate",
                    G_CALLBACK (on_save_playlist1_activate),
                    NULL);
  g_signal_connect ((gpointer) save_all_playlists1, "activate",
                    G_CALLBACK (on_save_all_playlists1_activate),
                    NULL);
#endif

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (plmenu, plmenu, "plmenu");
  GLADE_HOOKUP_OBJECT (plmenu, rename_playlist1, "rename_playlist1");
  GLADE_HOOKUP_OBJECT (plmenu, remove_playlist1, "remove_playlist1");
  GLADE_HOOKUP_OBJECT (plmenu, add_new_playlist1, "add_new_playlist1");
//  GLADE_HOOKUP_OBJECT (plmenu, separator11, "separator11");
//  GLADE_HOOKUP_OBJECT (plmenu, load_playlist1, "load_playlist1");
//  GLADE_HOOKUP_OBJECT (plmenu, save_playlist1, "save_playlist1");
//  GLADE_HOOKUP_OBJECT (plmenu, save_all_playlists1, "save_all_playlists1");

  return plmenu;
}

static void
tabstrip_scroll_left (DdbTabStrip *ts) {
#if 0
    // scroll to leftmost border-spanning tab
    int scrollsize = 0;
    int w = 0;
    int cnt = deadbeef->plt_get_count ();
    for (int idx = 0; idx < cnt; idx++) {
        int tab_w = ddb_tabstrip_get_tab_width (ts, idx);
        if (w < ts->hscrollpos && w + tab_w >= ts->hscrollpos) {
            scrollsize = ts->hscrollpos - w;
            break;
        }
        w += tab_w - tab_overlap_size;
    }
    w += tab_overlap_size + 3;

    ts->hscrollpos -= scrollsize;
    if (ts->hscrollpos < 0) {
        ts->hscrollpos = 0;
    }
    deadbeef->conf_set_int ("gtkui.tabscroll", ts->hscrollpos);
    gtk_widget_queue_draw (GTK_WIDGET (ts));
#endif
    int tab = deadbeef->plt_get_curr ();
    if (tab > 0) {
        tab--;
        gtkui_playlist_set_curr (tab);
    }
    tabstrip_scroll_to_tab (ts, tab);
}

static void
tabstrip_scroll_right (DdbTabStrip *ts) {
#if 0
    // scroll to rightmost border-spanning tab
    GtkWidget *widget = GTK_WIDGET (ts);
    int scrollsize = 0;
    int w = 0;
    int cnt = deadbeef->plt_get_count ();
    int boundary = widget->allocation.width - arrow_widget_width*2 + ts->hscrollpos;
    for (int idx = 0; idx < cnt; idx++) {
        int tab_w = ddb_tabstrip_get_tab_width (ts, idx);

        if (scrollsize == 0 && w < boundary && w + tab_w >= boundary) {
            scrollsize = (w + tab_w) - boundary;
        }
        w += tab_w - tab_overlap_size;
    }
    w += tab_overlap_size + 3;
    ts->hscrollpos += scrollsize;
    if (ts->hscrollpos > w - (widget->allocation.width - arrow_widget_width*2)) {
        ts->hscrollpos = w - (widget->allocation.width - arrow_widget_width*2);
    }
    deadbeef->conf_set_int ("gtkui.tabscroll", ts->hscrollpos);
    gtk_widget_queue_draw (widget);
#endif
    int tab = deadbeef->plt_get_curr ();
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
on_tabstrip_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    tab_clicked = get_tab_under_cursor (ts, event->x);
    if (event->button == 1)
    {
        int need_arrows = tabstrip_need_arrows (ts);
        if (need_arrows) {
            if (event->x < arrow_widget_width) {
                if (event->type == GDK_BUTTON_PRESS) {
                    tabstrip_scroll_left (ts);
                    ts->scroll_direction = -1;
                    ts->scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                }
                return FALSE;
            }
            else if (event->x >= widget->allocation.width - arrow_widget_width) {
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
                int playlist = deadbeef->plt_get_curr ();
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
    draw_init_font (widget->style);
    DdbTabStrip *ts = DDB_TABSTRIP (widget);
    tabstrip_adjust_hscroll (ts);
    int height = draw_get_font_size () + 13;
    if (height != widget->allocation.height) {
        gtk_widget_set_size_request (widget, -1, height);
    }
    return FALSE;
}


gboolean
on_tabstrip_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event)
{
    tabstrip_render (DDB_TABSTRIP (widget));
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
    gdk_event_request_motions (event);
    if ((ev_state & GDK_BUTTON1_MASK) && ts->prepare) {
        if (gtk_drag_check_threshold (widget, ev_x, ts->prev_x, 0, 0)) {
            ts->prepare = 0;
        }
    }
    if (!ts->prepare && ts->dragging >= 0) {
//        gdk_window_set_cursor (widget->window, cursor_drag);
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
    int prev = deadbeef->plt_get_curr ();
    if (tab != -1 && tab != prev) {
        gtkui_playlist_set_curr (tab);
    }

    GtkWidget *pl = lookup_widget (mainwin, "playlist");

    int cnt = g_list_length (drag_context->targets);
    int i;
    for (i = 0; i < cnt; i++) {
        GdkAtom a = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets, i));
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
