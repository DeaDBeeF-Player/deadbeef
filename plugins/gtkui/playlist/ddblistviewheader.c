/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <math.h>
#include <string.h>
#include "../drawing.h"
#include "../gtkui.h"
#include "../support.h"
#include "ddblistview.h"
#include "ddblistviewheader.h"
#include "plcommon.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

struct _DdbListviewHeaderPrivate {
    GdkCursor *cursor_sz;
    GdkCursor *cursor_drag;
    drawctx_t hdrctx;
    int hscrollpos;
    int header_dragging;
    int header_sizing;
    int header_dragpt[2];
    gdouble prev_header_x;
    int header_prepare;
    int col_movepos;
};

#define DDB_LISTVIEW_HEADER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_LISTVIEW_HEADER_TYPE, DdbListviewHeaderPrivate))
G_DEFINE_TYPE (DdbListviewHeader, ddb_listview_header, GTK_TYPE_DRAWING_AREA);

static void ddb_listview_header_class_init(DdbListviewHeaderClass *klass);
static void ddb_listview_header_init(DdbListviewHeader *listview);
static void ddb_listview_header_destroy(GObject *object);

static void
ddb_listview_header_class_init(DdbListviewHeaderClass *class) {
    GObjectClass *object_class = (GObjectClass *) class;
    object_class->finalize = ddb_listview_header_destroy;
    g_type_class_add_private(class, sizeof(DdbListviewHeaderPrivate));
}

#pragma mark - Forward decls

static void
ddb_listview_header_render (DdbListviewHeader *header, cairo_t *cr, int x1, int x2);

#if GTK_CHECK_VERSION(3,0,0)
static gboolean
ddb_listview_header_draw                 (GtkWidget       *widget,
                                          cairo_t *cr,
                                          gpointer         user_data);
#else
static gboolean
ddb_listview_header_expose_event                 (GtkWidget       *widget,
                                                  GdkEventExpose  *event,
                                                  gpointer         user_data);
#endif

static void
ddb_listview_header_realize                      (GtkWidget       *widget,
                                                  gpointer         user_data);

static gboolean
ddb_listview_header_motion_notify_event          (GtkWidget       *widget,
                                                  GdkEventMotion  *event,
                                                  gpointer         user_data);

static gboolean
ddb_listview_header_button_press_event           (GtkWidget       *widget,
                                                  GdkEventButton  *event,
                                                  gpointer         user_data);

static gboolean
ddb_listview_header_button_release_event         (GtkWidget       *widget,
                                                  GdkEventButton  *event,
                                                  gpointer         user_data);

static gboolean
ddb_listview_header_enter (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);

static gboolean
header_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer p);

#pragma mark -

static void
ddb_listview_header_init(DdbListviewHeader *header) {
    gtk_widget_set_events (GTK_WIDGET(header), GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK);

#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect ((gpointer) header, "expose_event",
                      G_CALLBACK (ddb_listview_header_expose_event),
                      NULL);
#else
    g_signal_connect ((gpointer) header, "draw",
                      G_CALLBACK (ddb_listview_header_draw),
                      NULL);
#endif
    g_signal_connect ((gpointer) header, "realize",
                      G_CALLBACK (ddb_listview_header_realize),
                      NULL);
    g_signal_connect ((gpointer) header, "motion_notify_event",
                      G_CALLBACK (ddb_listview_header_motion_notify_event),
                      NULL);
    g_signal_connect_after ((gpointer) header, "button_press_event",
                            G_CALLBACK (ddb_listview_header_button_press_event),
                            NULL);
    g_signal_connect ((gpointer) header, "button_release_event",
                      G_CALLBACK (ddb_listview_header_button_release_event),
                      NULL);
    g_signal_connect ((gpointer) header, "enter-notify-event",
                      G_CALLBACK (ddb_listview_header_enter),
                      NULL);

    GValue value = {0, };
    g_value_init (&value, G_TYPE_BOOLEAN);
    g_value_set_boolean (&value, TRUE);
    g_object_set_property (G_OBJECT (header), "has-tooltip", &value);
    g_signal_connect (G_OBJECT (header), "query-tooltip", G_CALLBACK (header_tooltip_handler), NULL);

    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    memset (priv, 0, sizeof (DdbListviewHeaderPrivate));

    drawctx_init (&priv->hdrctx);

    priv->col_movepos = -1;
    priv->header_dragging = -1;
    priv->header_sizing = -1;
    priv->header_dragpt[0] = 0;
    priv->header_dragpt[1] = 0;
    priv->prev_header_x = -1;
    priv->header_prepare = 0;
    priv->cursor_sz = NULL;
    priv->cursor_drag = NULL;
}

GtkWidget *
ddb_listview_header_new(void) {
    return GTK_WIDGET(g_object_new(ddb_listview_header_get_type(), NULL));
}

static void
ddb_listview_header_destroy(GObject *object) {
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(object);
    draw_free (&priv->hdrctx);
    if (priv->cursor_sz) {
        gdk_cursor_unref (priv->cursor_sz);
        priv->cursor_sz = NULL;
    }
    if (priv->cursor_drag) {
        gdk_cursor_unref (priv->cursor_drag);
        priv->cursor_drag = NULL;
    }
}

static void
ddb_listview_header_expose (DdbListviewHeader *header, cairo_t *cr, int x, int y, int w, int h) {
    ddb_listview_header_render (header, cr, x, x+w);
}

#if GTK_CHECK_VERSION(3,0,0)
static gboolean
ddb_listview_header_draw                 (GtkWidget       *widget,
                                          cairo_t *cr,
                                          gpointer         user_data) {
    GdkRectangle clip;
    gdk_cairo_get_clip_rectangle(cr, &clip);
    ddb_listview_header_expose (DDB_LISTVIEW_HEADER(widget), cr, clip.x, clip.y, clip.width, clip.height);
    return TRUE;
}
#else
static gboolean
ddb_listview_header_expose_event                 (GtkWidget       *widget,
                                                  GdkEventExpose  *event,
                                                  gpointer         user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    ddb_listview_header_expose (DDB_LISTVIEW_HEADER(widget), cr, event->area.x, event->area.y, event->area.width, event->area.height);
    cairo_destroy (cr);
    return TRUE;
}
#endif

static void
set_tooltip (GtkTooltip *tooltip, const char *text, int x, int y, int width, int height) {
    GdkRectangle rect = {
        .x = x,
        .y = y,
        .width = width,
        .height = height
    };
    gtk_tooltip_set_tip_area (tooltip, &rect);
    gtk_tooltip_set_text (tooltip, text);
}

static gboolean
header_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer p) {
    DdbListviewHeader *header = DDB_LISTVIEW_HEADER (widget);
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    DdbListviewColumn *c;
    int col_x = -priv->hscrollpos;
    for (c = header->delegate->get_columns(header); c && col_x + c->width < x; col_x += c->width, c = c->next);
    if (c && c->show_tooltip && x < col_x + c->width - (c->sort_order != DdbListviewColumnSortOrderNone ? 14 : 4)) {
        GtkAllocation a;
        gtk_widget_get_allocation(GTK_WIDGET(header), &a);
        set_tooltip (tooltip, c->title, col_x, 0, c->width - 4, a.height);
        return TRUE;
    }
    return FALSE;
}

static void
ddb_listview_header_realize                      (GtkWidget       *widget,
                                                  gpointer         user_data) {
    // create cursor for sizing headers
    DdbListviewHeader *header = DDB_LISTVIEW_HEADER (widget);
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    priv->cursor_sz = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    priv->cursor_drag = gdk_cursor_new (GDK_FLEUR);
    ddb_listview_header_update_fonts(header);
}

static void
draw_cairo_rectangle (cairo_t *cr, GdkColor *color, int x, int y, int width, int height) {
    cairo_set_source_rgb(cr, color->red/65535., color->green/65535., color->blue/65535.);
    cairo_rectangle(cr, x, y, width, height);
    cairo_fill(cr);
}

static void
draw_cairo_line(cairo_t *cr, GdkColor *color, int x1, int y1, int x2, int y2) {
    cairo_set_source_rgb (cr, color->red/65535., color->green/65535., color->blue/65535.);
    cairo_move_to (cr, x1, y1);
    cairo_line_to (cr, x2, y2);
    cairo_stroke (cr);
}

static void
draw_header_fg(DdbListviewHeader *header, cairo_t *cr, DdbListviewColumn *c, GdkColor *clr, int x, int xx, int h) {
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    int text_width = xx - x - 10;
    if (c->sort_order != DdbListviewColumnSortOrderNone) {
        int arrow_sz = 10;
        text_width = max(0, text_width - arrow_sz);
#if GTK_CHECK_VERSION(3,0,0)
        gdouble angle = 0;
        if (c->sort_order == DdbListviewColumnSortOrderAscending) {
            angle = G_PI;
        }
        gtk_render_arrow(gtk_widget_get_style_context(theme_treeview), cr, angle, xx-arrow_sz-5, h/2-arrow_sz/2, arrow_sz);
#else
        int dir = c->sort_order == DdbListviewColumnSortOrderAscending ? GTK_ARROW_DOWN : GTK_ARROW_UP;
        gtk_paint_arrow(GTK_WIDGET(header)->style, GTK_WIDGET(header)->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, GTK_WIDGET(header), NULL, dir, TRUE, xx-arrow_sz-5, h/2-arrow_sz/2, arrow_sz, arrow_sz);
#endif
    }

    float fg[3] = {clr->red/65535., clr->green/65535., clr->blue/65535.};
    draw_set_fg_color(&priv->hdrctx, fg);
    cairo_save(cr);
    cairo_rectangle(cr, x+5, 0, text_width, h);
    cairo_clip(cr);
    draw_text_custom(&priv->hdrctx, x+5, 3, text_width, 0, DDB_COLUMN_FONT, 0, 0, c->title);
    c->show_tooltip = draw_is_ellipsized(&priv->hdrctx);
    cairo_restore(cr);
}

#if GTK_CHECK_VERSION(3,0,0)
static void
render_column_button (DdbListviewHeader *header, cairo_t *cr, GtkStateFlags state, int x, int y, int w, int h, GdkColor *clr) {
    GtkStyleContext *context = gtk_widget_get_style_context(theme_button);
    gtk_style_context_save(context);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_BUTTON);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_DEFAULT);
    gtk_style_context_set_state(context, state);
    //    gtk_style_context_add_region(context, GTK_STYLE_REGION_COLUMN_HEADER, 0);
    gtk_render_background(context, cr, x, y, w, h);
    gtk_render_frame(context, cr, x, y, w, h);
    if (clr) {
        GdkRGBA rgba;
        gtk_style_context_get_color(context, state, &rgba);
        clr->red = round(rgba.red * 65535);
        clr->green = round(rgba.green * 65535);
        clr->blue = round(rgba.blue * 65535);
    }
    gtk_style_context_restore(context);
}
#endif

static void
ddb_listview_header_render (DdbListviewHeader *header, cairo_t *cr, int x1, int x2) {
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    cairo_set_line_width (cr, 1);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET(header), &a);
    draw_begin(&priv->hdrctx, cr);
    int h = a.height;

    GtkStyle *style = gtk_widget_get_style(mainwin);
    // Paint the background for the whole header
    GdkColor gdkfg;
#if !GTK_HEADERS
    GdkColor clr_base, clr_dark, clr_light;
    if (gtkui_override_tabstrip_colors()) {
        gtkui_get_tabstrip_base_color(&clr_base);
        gtkui_get_tabstrip_dark_color(&clr_dark);
        gtkui_get_tabstrip_light_color (&clr_light);
        gtkui_get_listview_column_text_color(&gdkfg);
    } else {
        clr_base = style->bg[GTK_STATE_NORMAL];
        clr_dark = style->dark[GTK_STATE_NORMAL];
        clr_light = style->light[GTK_STATE_NORMAL];
        gdkfg = style->fg[GTK_STATE_NORMAL];
    }

    draw_cairo_rectangle(cr, &clr_base, 0, 0, a.width, h);
    draw_cairo_line(cr, &clr_dark, 0, h, a.width, h);
    draw_cairo_line(cr, &gtk_widget_get_style(GTK_WIDGET(header))->mid[GTK_STATE_NORMAL], 0, h, a.width, h);
#else
#if GTK_CHECK_VERSION(3,0,0)
    render_column_button(listview, cr, GTK_STATE_FLAG_NORMAL, 0, -1, a.width, h+2, &gdkfg);
#else
    gtk_paint_box(gtk_widget_get_style(theme_button), gtk_widget_get_window(priv->header), GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, priv->header, "button", -2, -2, a.width+4, h+4);
    gdkfg = gtk_widget_get_style(theme_button)->fg[GTK_STATE_NORMAL];
    draw_cairo_line(cr, &gtk_widget_get_style(priv->header)->mid[GTK_STATE_NORMAL], 0, h, a.width, h);
#endif
#endif
    int x = -priv->hscrollpos;
    int idx = 0;
    // Add a column header pseudo-button for each configured treeview column, by drawing lines across the background
    for (DdbListviewColumn *c = header->delegate->get_columns(header); c && x < x2; c = c->next, idx++) {
        int xx = x + c->width;

        // Only render for columns within the clip region, and not any column which is being dragged
        if (idx != priv->header_dragging && xx >= x1) {
            // Paint the button text
            draw_header_fg(header, cr, c, &gdkfg, x, xx, h);

            // Add a vertical line near the right side of the column width, but not right next to an empty slot
            if (c->width > 0 && priv->header_dragging != idx + 1) {
                draw_cairo_line(cr, &clr_dark, xx-2, 2, xx-2, h-4);
                draw_cairo_line(cr, &clr_light, xx-1, 2, xx-1, h-4);
            }
        }
        x = xx;
    }

    // Do special drawing when a column is being dragged
    if (priv->header_dragging != -1) {
        x = -priv->hscrollpos;
        idx = 0;
        DdbListviewColumn *c = header->delegate->get_columns(header);
        while (c && idx++ != priv->header_dragging) {
            x += c->width;
            c = c->next;
        }

        if (!c) {
            draw_end (&priv->hdrctx);
            return;
        }

        // Mark the position where the dragged column used to be with an indented/active/dark position
        int xx = x - 2; // Where the divider line is
        int w = c->width + 2;
        if (xx < x2) {
#if GTK_CHECK_VERSION(3,0,0)
            render_column_button(header, cr, GTK_STATE_FLAG_ACTIVE, xx, 0, w, h, NULL);
#else
            gtk_paint_box(gtk_widget_get_style(theme_button), gtk_widget_get_window(GTK_WIDGET(header)), GTK_STATE_ACTIVE, GTK_SHADOW_ETCHED_IN, NULL, GTK_WIDGET(header), "button", xx, 0, w, h);
#endif
        }

        // Draw a highlighted/selected "button" wherever the dragged column is currently positioned
        xx = priv->col_movepos - priv->hscrollpos - 2;
        if (w > 0 && xx < x2) {
#if GTK_CHECK_VERSION(3,0,0)
            render_column_button(header, cr, GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_FOCUSED, xx, 0, w, h, &gdkfg);
#else
            gtk_paint_box(gtk_widget_get_style(theme_button), gtk_widget_get_window(GTK_WIDGET(header)), GTK_STATE_SELECTED, GTK_SHADOW_OUT, NULL, GTK_WIDGET(header), "button", xx, 0, w, h);
            gdkfg = gtk_widget_get_style(theme_button)->fg[GTK_STATE_SELECTED];
#endif
            if (gtkui_override_listview_colors()) {
                gtkui_get_listview_selected_text_color(&gdkfg);
            }
            draw_header_fg(header, cr, c, &gdkfg, xx, xx+w, h);
        }
    }

    draw_end (&priv->hdrctx);
}

static void
set_header_cursor (DdbListviewHeader *header, gdouble mousex) {
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    int x = -priv->hscrollpos;
    for (DdbListviewColumn *c = header->delegate->get_columns(header); c; c = c->next) {
        if (mousex >= x + c->width - 4 && mousex <= x + c->width) {
            gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(header)), priv->cursor_sz);
            return;
        }
        x += c->width;
    }

    gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(header)), NULL);
}

static gboolean
ddb_listview_header_motion_notify_event          (GtkWidget       *widget,
                                                  GdkEventMotion  *event,
                                                  gpointer         user_data) {
    DdbListviewHeader *header = DDB_LISTVIEW_HEADER(widget);
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(widget);
#if GTK_CHECK_VERSION(2,12,0)
    gdk_event_request_motions (event);
#endif

    if (priv->header_prepare) {
        if (priv->header_dragging != -1 && gtk_drag_check_threshold(widget, round(priv->prev_header_x), 0, round(event->x), 0)) {
            priv->header_prepare = 0;
        }
        else {
            return FALSE;
        }
    }

    if (priv->header_dragging >= 0) {
        gdk_window_set_cursor (gtk_widget_get_window (widget), priv->cursor_drag);
        DdbListviewColumn *c = header->delegate->get_columns(header);
        for (int i = 0; c && i < priv->header_dragging; c = c->next, i++);
        if (c) {
            int left = event->x - priv->header_dragpt[0] + priv->hscrollpos;
            int right = left + c->width;
            DdbListviewColumn *cc = header->delegate->get_columns(header);
            for (int xx = 0, ii = 0; cc; xx += cc->width, cc = cc->next, ii++) {
                if ((priv->header_dragging > ii && left < xx + cc->width/2) || (priv->header_dragging < ii && right > xx + cc->width/2)) {
                    header->delegate->move_column(header, c, ii);
                    priv->header_dragging = ii;
                    break;
                }
            }
            priv->col_movepos = left;
            gtk_widget_queue_draw (GTK_WIDGET(header));
        }
    }
    else if (priv->header_sizing >= 0) {
        int x = -priv->hscrollpos;
        DdbListviewColumn *c = header->delegate->get_columns(header);
        for (int i = 0; i < priv->header_sizing; i++, c = c->next) {
            x += c->width;
        }
        header->delegate->set_column_width(header, c, max(DDB_LISTVIEW_MIN_COLUMN_WIDTH, round (event->x) - priv->header_dragpt[0] - x));
        gtk_widget_queue_draw(GTK_WIDGET(header));
    }
    else {
        set_header_cursor(header, event->x);
    }
    return FALSE;
}

static int
ddb_listview_header_get_column_idx_for_coord (DdbListviewHeader *header, int click_x) {
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    int x = -priv->hscrollpos;
    int idx = 0;
    for (DdbListviewColumn *c = header->delegate->get_columns(header); c; c = c->next, idx++) {
        int w = c->width;
        if (click_x >= x && click_x < x + w) {
            return idx;
        }
        x += w;
    }
    return -1;
}

static gboolean
ddb_listview_header_button_press_event           (GtkWidget       *widget,
                                                  GdkEventButton  *event,
                                                  gpointer         user_data) {
    DdbListviewHeader *header = DDB_LISTVIEW_HEADER(widget);
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    priv->prev_header_x = -1;
    if (TEST_LEFT_CLICK (event)) {
        header->delegate->update_scroll_ref_point(header);
        int x = -priv->hscrollpos;
        int i = 0;
        DdbListviewColumn *c = header->delegate->get_columns(header);
        while (c && event->x > x + c->width) {
            i++;
            x += c->width;
            c = c->next;

        }
        priv->header_dragpt[0] = round(event->x);
        priv->header_dragpt[1] = round(event->y);
        priv->prev_header_x = event->x;
        if (!c) {
            priv->header_prepare = 1;
        }
        else if (event->x < x + c->width - 4) {
            priv->header_prepare = 1;
            priv->header_dragging = i;
            priv->header_dragpt[0] -= x;
        }
        else {
            priv->header_sizing = i;
            priv->header_dragpt[0] -= (x + c->width);
        }
        return TRUE;
    }
    else if (TEST_RIGHT_CLICK (event)) {
        if (priv->header_dragging != -1) {
            gtk_widget_queue_draw (GTK_WIDGET(header));
            priv->header_dragging = -1;
        }
        priv->header_sizing = -1;
        priv->header_prepare = 0;
        int idx = ddb_listview_header_get_column_idx_for_coord (header, event->x);
        header->delegate->context_menu (header, idx);
        return TRUE;
    }
    return FALSE;
}

static gboolean
ddb_listview_header_button_release_event         (GtkWidget       *widget,
                                                  GdkEventButton  *event,
                                                  gpointer         user_data) {
    DdbListviewHeader *header = DDB_LISTVIEW_HEADER(widget);
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    if (event->button == 1) {
        if (priv->header_sizing != -1) {
            header->delegate->columns_changed (header);
            priv->header_sizing = -1;
        }
        else if (priv->header_dragging != -1) {
            if (priv->header_prepare) {
                if (event->y >= 0 && event->y <= header->delegate->get_list_height(header)) {
                    // sort
                    int x = -priv->hscrollpos;
                    int i = 0;
                    DdbListviewColumn *c = header->delegate->get_columns(header);
                    while (c && event->x > x + c->width) {
                        i++;
                        x += c->width;
                        c = c->next;

                    }
                    if (c && event->x > x + 1 && event->x < x + c->width - 5) {
                        for (DdbListviewColumn *cc = header->delegate->get_columns(header); cc; cc = cc->next) {
                            if (cc != c) {
                                cc->sort_order = DdbListviewColumnSortOrderNone;
                            }
                        }
                        c->sort_order = c->sort_order == DdbListviewColumnSortOrderDescending
                            ? DdbListviewColumnSortOrderAscending
                            : DdbListviewColumnSortOrderDescending;
                        header->delegate->col_sort (header, c->sort_order, c->user_data);
                        gtk_widget_queue_draw (GTK_WIDGET(header));
                    }
                }
            }
            else {
                gtk_widget_queue_draw (GTK_WIDGET(header));
            }
            priv->header_dragging = -1;
        }
        priv->header_prepare = 0;
        set_header_cursor(header, event->x);
    }
    return FALSE;
}

static gboolean
ddb_listview_header_enter (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    DdbListviewHeader *header = DDB_LISTVIEW_HEADER(widget);
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    if (!priv->header_prepare && priv->header_dragging == -1 && priv->header_sizing == -1) {
        int x = event->x;
#if GTK_CHECK_VERSION(3,0,0)
        if (event->send_event) {
            GdkWindow *win = gtk_widget_get_window(widget);
            GdkDeviceManager *device_manager = gdk_display_get_device_manager(gdk_window_get_display(win));
            gdk_window_get_device_position(win, gdk_device_manager_get_client_pointer(device_manager), &x, NULL, NULL);
        }
#endif
        set_header_cursor(header, x);
    }
    return FALSE;
}

void
ddb_listview_header_set_hscrollpos(DdbListviewHeader *header, int hscrollpos) {
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);

    int diff = priv->hscrollpos - hscrollpos;
    gdk_window_scroll(gtk_widget_get_window(GTK_WIDGET(header)), diff, 0);
    priv->hscrollpos = hscrollpos;
}

gboolean
ddb_listview_header_is_sizing (DdbListviewHeader *header) {
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    return priv->header_sizing != -1;
}

void
ddb_listview_header_update_fonts (DdbListviewHeader *header) {
    DdbListviewHeaderPrivate *priv = DDB_LISTVIEW_HEADER_GET_PRIVATE(header);
    draw_init_font (&priv->hdrctx, DDB_COLUMN_FONT, 1);
    int height = draw_get_listview_rowheight (&priv->hdrctx);
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET(header), &a);
    if (height != a.height) {
        gtk_widget_set_size_request (GTK_WIDGET(header), -1, height);
    }
}

