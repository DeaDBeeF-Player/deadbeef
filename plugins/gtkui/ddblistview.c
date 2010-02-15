/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif
#if HAVE_NOTIFY
#include <libnotify/notify.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <sys/time.h>
#include "ddblistview.h"
#include "drawing.h"
//#include "callbacks.h"
//#include "interface.h"
//#include "support.h"
//#include "search.h"
//#include "progress.h"
//#include "../../session.h"
//#include "parser.h"
//#include "gtkui.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

//#define PL_HEAD(iter) (deadbeef->pl_get_first(iter))
//#define PL_TAIL(iter) (deadbeef->pl_get_last(iter))
#define PL_NEXT(it) (ps->binding->next (it))
#define PL_PREV(it) (ps->binding->prev (it))
//#define ps->binding->is_selected(it) (deadbeef->pl_is_selected(it))
//#define ps->binding->select(it, sel) (deadbeef->pl_set_selected(it,sel))
//#define Vps->binding->select(it, sel) {deadbeef->pl_set_selected(it,sel);gtk_pl_redraw_item_everywhere (it);}
#define REF(it) {if (it) ps->binding->ref (it);}
#define UNREF(it) {if (it) ps->binding->unref(it);}

// HACK!!
extern GtkWidget *theme_treeview;

G_DEFINE_TYPE (DdbListview, ddb_listview, GTK_TYPE_TABLE);

static void ddb_listview_class_init(DdbListviewClass *klass);
static void ddb_listview_init(DdbListview *listview);
static void ddb_listview_size_request(GtkWidget *widget,
        GtkRequisition *requisition);
static void ddb_listview_size_allocate(GtkWidget *widget,
        GtkAllocation *allocation);
static void ddb_listview_realize(GtkWidget *widget);
static gboolean ddb_listview_expose(GtkWidget *widget,
        GdkEventExpose *event);
static void ddb_listview_paint(GtkWidget *widget);
static void ddb_listview_destroy(GtkObject *object);

// fwd decls
static inline void
draw_drawable (GdkDrawable *window, GdkGC *gc, GdkDrawable *drawable, int x1, int y1, int x2, int y2, int w, int h);

////// list functions ////
void
ddb_listview_list_render (DdbListview *ps, int x, int y, int w, int h);
void
ddb_listview_list_expose (DdbListview *ps, int x, int y, int w, int h);
void
ddb_listview_list_render_row_background (DdbListview *ps, int row, DdbListviewIter it);
void
ddb_listview_list_render_row_foreground (DdbListview *ps, int row, DdbListviewIter it);
void
ddb_listview_list_render_row (DdbListview *ps, int row, DdbListviewIter it);
void
ddb_listview_list_track_dragdrop (DdbListview *ps, int y);
void
ddb_listview_list_mousemove (DdbListview *ps, GdkEventMotion *event);
void
ddb_listview_list_setup_vscroll (DdbListview *ps);
void
ddb_listview_list_setup_hscroll (DdbListview *ps);
void
ddb_listview_list_set_hscroll (DdbListview *ps, int newscroll);
void
ddb_listview_set_cursor (DdbListview *pl, int cursor);

////// header functions ////
void
ddb_listview_header_render (DdbListview *ps);
void
ddb_listview_header_expose (DdbListview *ps, int x, int y, int w, int h);


// signal handlers
void
ddb_listview_vscroll_value_changed            (GtkRange        *widget,
                                        gpointer         user_data);
void
ddb_listview_hscroll_value_changed           (GtkRange        *widget,
                                        gpointer         user_data);

void
ddb_listview_list_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

gboolean
ddb_listview_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

void
ddb_listview_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
ddb_listview_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_list_configure_event            (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
ddb_listview_list_expose_event               (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

void
ddb_listview_list_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
ddb_listview_list_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_list_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

gboolean
ddb_listview_list_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

void
ddb_listview_list_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
ddb_listview_list_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

gboolean
ddb_listview_list_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data);

void
ddb_listview_list_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data);

gboolean
ddb_listview_list_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);
gboolean
ddb_listview_list_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_vscroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
ddb_listview_list_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);


static void
ddb_listview_class_init(DdbListviewClass *class)
{
  GtkTableClass *widget_class;
  widget_class = (GtkTableClass *) class;
}

static void
ddb_listview_init(DdbListview *listview)
{
    // init instance - create all subwidgets, and insert into table

    listview->rowheight = draw_get_font_size () + 12;

    listview->col_movepos = -1;
    listview->drag_motion_y = -1;

    listview->scroll_mode = 0;
    listview->scroll_pointer_y = -1;
    listview->scroll_direction = 0;
    listview->scroll_active = 0;
    memset (&listview->tm_prevscroll, 0, sizeof (listview->tm_prevscroll));
    listview->scroll_sleep_time = 0;

    listview->areaselect = 0;
    listview->areaselect_x = -1;
    listview->areaselect_y = -1;
    listview->areaselect_dx = -1;
    listview->areaselect_dy = -1;
    listview->dragwait = 0;
    listview->shift_sel_anchor = -1;

    listview->header_dragging = -1;
    listview->header_sizing = -1;
    listview->header_dragpt[0] = 0;
    listview->header_dragpt[1] = 0;
    listview->last_header_motion_ev = -1; //is it subject to remove?
    listview->prev_header_x = -1;
    listview->header_prepare = 0;

    GtkWidget *hbox;
    GtkWidget *vbox;

    gtk_table_resize (GTK_TABLE (listview), 2, 2);
    listview->scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 1, 1, 0, 0)));
    gtk_widget_show (listview->scrollbar);
    gtk_table_attach (GTK_TABLE (listview), listview->scrollbar, 1, 2, 0, 1,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (GTK_FILL), 0, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_table_attach (GTK_TABLE (listview), hbox, 0, 1, 0, 1,
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

    listview->header = gtk_drawing_area_new ();
    gtk_widget_show (listview->header);
    gtk_box_pack_start (GTK_BOX (vbox), listview->header, FALSE, TRUE, 0);
    gtk_widget_set_size_request (listview->header, -1, 24);
    gtk_widget_set_events (listview->header, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    listview->list = gtk_drawing_area_new ();
    gtk_widget_show (listview->list);
    gtk_box_pack_start (GTK_BOX (vbox), listview->list, TRUE, TRUE, 0);
    gtk_widget_set_events (listview->list, GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

    listview->hscrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 0, 0, 0)));
    gtk_widget_show (listview->hscrollbar);
    gtk_table_attach (GTK_TABLE (listview), listview->hscrollbar, 0, 1, 1, 2,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (GTK_FILL), 0, 0);


    gtk_object_set_data (GTK_OBJECT (listview->list), "owner", listview);
    gtk_object_set_data (GTK_OBJECT (listview->header), "owner", listview);
    gtk_object_set_data (GTK_OBJECT (listview->scrollbar), "owner", listview);
    gtk_object_set_data (GTK_OBJECT (listview->hscrollbar), "owner", listview);

    g_signal_connect ((gpointer) listview->list, "configure_event",
            G_CALLBACK (ddb_listview_list_configure_event),
            NULL);

    g_signal_connect ((gpointer) listview->scrollbar, "value_changed",
            G_CALLBACK (ddb_listview_vscroll_value_changed),
            NULL);
    g_signal_connect ((gpointer) listview->header, "expose_event",
            G_CALLBACK (ddb_listview_header_expose_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "configure_event",
            G_CALLBACK (ddb_listview_header_configure_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "realize",
            G_CALLBACK (ddb_listview_header_realize),
            NULL);
    g_signal_connect ((gpointer) listview->header, "motion_notify_event",
            G_CALLBACK (ddb_listview_header_motion_notify_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "button_press_event",
            G_CALLBACK (ddb_listview_header_button_press_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "button_release_event",
            G_CALLBACK (ddb_listview_header_button_release_event),
            NULL);
    g_signal_connect ((gpointer) listview->list, "expose_event",
            G_CALLBACK (ddb_listview_list_expose_event),
            NULL);
    g_signal_connect ((gpointer) listview->list, "realize",
            G_CALLBACK (ddb_listview_list_realize),
            NULL);
    g_signal_connect ((gpointer) listview->list, "button_press_event",
            G_CALLBACK (ddb_listview_list_button_press_event),
            NULL);
    g_signal_connect ((gpointer) listview->list, "scroll_event",
            G_CALLBACK (ddb_listview_vscroll_event),
            NULL);
//    g_signal_connect ((gpointer) listview->list, "drag_begin",
//            G_CALLBACK (on_playlist_drag_begin),
//            NULL);
    g_signal_connect ((gpointer) listview->list, "drag_motion",
            G_CALLBACK (ddb_listview_list_drag_motion),
            NULL);
    g_signal_connect ((gpointer) listview->list, "drag_drop",
            G_CALLBACK (ddb_listview_list_drag_drop),
            NULL);
    g_signal_connect ((gpointer) listview->list, "drag_data_get",
            G_CALLBACK (ddb_listview_list_drag_data_get),
            NULL);
    g_signal_connect ((gpointer) listview->list, "drag_end",
            G_CALLBACK (ddb_listview_list_drag_end),
            NULL);
    g_signal_connect ((gpointer) listview->list, "drag_failed",
            G_CALLBACK (ddb_listview_list_drag_failed),
            NULL);
    g_signal_connect ((gpointer) listview->list, "drag_leave",
            G_CALLBACK (ddb_listview_list_drag_leave),
            NULL);
    g_signal_connect ((gpointer) listview->list, "button_release_event",
            G_CALLBACK (ddb_listview_list_button_release_event),
            NULL);
    g_signal_connect ((gpointer) listview->list, "motion_notify_event",
            G_CALLBACK (ddb_listview_motion_notify_event),
            NULL);
    g_signal_connect ((gpointer) listview->list, "drag_data_received",
            G_CALLBACK (ddb_listview_list_drag_data_received),
            NULL);
    g_signal_connect ((gpointer) listview->hscrollbar, "value_changed",
            G_CALLBACK (ddb_listview_hscroll_value_changed),
            NULL);
}

GtkWidget * ddb_listview_new()
{
   return GTK_WIDGET(gtk_type_new(ddb_listview_get_type()));
}

static void
ddb_listview_destroy(GtkObject *object)
{
  DdbListview *listview;
  DdbListviewClass *class;

  g_return_if_fail(object != NULL);
  g_return_if_fail(DDB_IS_LISTVIEW(object));

  listview = DDB_LISTVIEW(object);

  class = gtk_type_class(gtk_widget_get_type());

  if (GTK_OBJECT_CLASS(class)->destroy) {
     (* GTK_OBJECT_CLASS(class)->destroy) (object);
  }
}

void
ddb_listview_refresh (DdbListview *listview, uint32_t flags) {
    if (flags & DDB_REFRESH_VSCROLL) {
        ddb_listview_list_setup_vscroll (listview);
    }
    if (flags & DDB_REFRESH_HSCROLL) {
        ddb_listview_list_setup_hscroll (listview);
    }
    if (flags & DDB_REFRESH_COLUMNS) {
        ddb_listview_header_render (listview);
    }
    if (flags & DDB_REFRESH_LIST) {
        ddb_listview_list_render (listview, 0, 0, listview->list->allocation.width, listview->list->allocation.height);
    }
    if (flags & DDB_EXPOSE_COLUMNS) {
        ddb_listview_header_expose (listview, 0, 0, listview->header->allocation.width, listview->header->allocation.height);
    }
    if (flags & DDB_EXPOSE_LIST) {
        ddb_listview_list_expose (listview, 0, 0, listview->list->allocation.width, listview->list->allocation.height);
    }
}

void
ddb_listview_list_realize                    (GtkWidget       *widget,
        gpointer         user_data)
{
    GtkTargetEntry entry = {
        .target = "STRING",
        .flags = GTK_TARGET_SAME_WIDGET/* | GTK_TARGET_OTHER_APP*/,
        TARGET_SAMEWIDGET
    };
    // setup drag-drop source
//    gtk_drag_source_set (widget, GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_MOVE);
    // setup drag-drop target
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, &entry, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_drag_dest_add_uri_targets (widget);
//    gtk_drag_dest_set_track_motion (widget, TRUE);
}

gboolean
ddb_listview_list_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    ddb_listview_list_setup_vscroll (ps);
    ddb_listview_list_setup_hscroll (ps);
    widget = ps->list;
    if (ps->backbuf) {
        g_object_unref (ps->backbuf);
        ps->backbuf = NULL;
    }
    ps->nvisiblerows = ceil (widget->allocation.height / (float)ps->rowheight);
    ps->nvisiblefullrows = floor (widget->allocation.height / (float)ps->rowheight);
    ps->backbuf = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);

    ddb_listview_list_render (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}

void
ddb_listview_list_render (DdbListview *ps, int x, int y, int w, int h) {
    if (!ps->backbuf) {
        return;
    }
    draw_begin ((uintptr_t)ps->backbuf);
	int row;
	int row1;
	int row2;
	int row2_full;
	row1 = max (0, y / ps->rowheight + ps->scrollpos);
	int cnt = ps->binding->count ();
	row2 = min (cnt, (y+h) / ps->rowheight + ps->scrollpos + 1);
	row2_full = (y+h) / ps->rowheight + ps->scrollpos + 1;
	// draw background
	DdbListviewIter it = ps->binding->get_for_idx (ps->scrollpos);
	DdbListviewIter it_copy = it;
	if (it_copy) {
        REF (it_copy);
    }
	for (row = row1; row < row2_full; row++) {
		ddb_listview_list_render_row_background (ps, row, it);
        if (it) {
            UNREF (it);
            it = PL_NEXT (it);
        }
	}
    UNREF (it);
	it = it_copy;
	int idx = 0;
	for (row = row1; row < row2; row++, idx++) {
        if (!it) {
            break;
        }
        ddb_listview_list_render_row_foreground (ps, row, it);
        DdbListviewIter next = PL_NEXT (it);
        UNREF (it);
        it = next;
	}
    UNREF (it);

    draw_end ();
}

gboolean
ddb_listview_list_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    // draw visible area of playlist
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    ddb_listview_list_expose (ps, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}

void
ddb_listview_list_expose (DdbListview *ps, int x, int y, int w, int h) {
    GtkWidget *widget = ps->list;
    if (widget->window) {
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, x, y, x, y, w, h);
    }
}

gboolean
ddb_listview_vscroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
	GdkEventScroll *ev = (GdkEventScroll*)event;
    GtkWidget *range = ps->scrollbar;;
    GtkWidget *playlist = ps->list;
    int h = playlist->allocation.height / ps->rowheight;
    int size = ps->binding->count ();
    if (h >= size) {
        size = 0;
    }
    if (size == 0) {
        return FALSE;
    }
    // pass event to scrollbar
    int newscroll = gtk_range_get_value (GTK_RANGE (range));
    if (ev->direction == GDK_SCROLL_UP) {
        newscroll -= 2;
    }
    else if (ev->direction == GDK_SCROLL_DOWN) {
        newscroll += 2;
    }
    gtk_range_set_value (GTK_RANGE (range), newscroll);

    return FALSE;
}

void
ddb_listview_vscroll_value_changed            (GtkRange        *widget,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    if (newscroll != ps->scrollpos) {
        GtkWidget *widget = ps->list;
        int di = newscroll - ps->scrollpos;
        int d = abs (di);
        if (d < ps->nvisiblerows) {
            if (di > 0) {
                draw_drawable (ps->backbuf, widget->style->black_gc, ps->backbuf, 0, d * ps->rowheight, 0, 0, widget->allocation.width, widget->allocation.height-d * ps->rowheight);
                int i;
                ps->scrollpos = newscroll;
                int start = ps->nvisiblerows-d-1;
                start = max (0, ps->nvisiblerows-d-1);
                for (i = start; i <= ps->nvisiblerows; i++) {
                    DdbListviewIter it = ps->binding->get_for_idx (i + ps->scrollpos);
                    ddb_listview_list_render_row (ps, i+ps->scrollpos, it);
                    UNREF (it);
                }
            }
            else {
                draw_drawable (ps->backbuf, widget->style->black_gc, ps->backbuf, 0, 0, 0, d*ps->rowheight, widget->allocation.width, widget->allocation.height);
                ps->scrollpos = newscroll;
                int i;
                for (i = 0; i <= d+1; i++) {
                    DdbListviewIter it = ps->binding->get_for_idx (i+ps->scrollpos);
                    ddb_listview_list_render_row (ps, i+ps->scrollpos, it);
                    UNREF (it);
                }
            }
        }
        else {
            ps->scrollpos = newscroll;
            ddb_listview_list_render (ps, 0, 0, widget->allocation.width, widget->allocation.height);
        }
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
    }
}

void
ddb_listview_hscroll_value_changed           (GtkRange        *widget,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    ddb_listview_list_set_hscroll (pl, newscroll);
}

gboolean
ddb_listview_list_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    ddb_listview_list_track_dragdrop (pl, y);
    return FALSE;
}


gboolean
ddb_listview_list_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    if (drag_context->targets) {
        GdkAtom target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets, TARGET_SAMEWIDGET));
        if (!target_type) {
            return FALSE;
        }
        gtk_drag_get_data (widget, drag_context, target_type, time);
        return TRUE;
    }
    return FALSE;
}


void
ddb_listview_list_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *selection_data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    switch (target_type) {
    case TARGET_SAMEWIDGET:
        {
            // format as "STRING" consisting of array of pointers
            int nsel = ps->binding->sel_count ();
            if (!nsel) {
                break; // something wrong happened
            }
            uint32_t *ptr = malloc (nsel * sizeof (uint32_t));
            int idx = 0;
            int i = 0;
            DdbListviewIter it = ps->binding->head ();
            for (; it; idx++) {
                if (ps->binding->is_selected (it)) {
                    ptr[i] = idx;
                    i++;
                }
                DdbListviewIter next = ps->binding->next (it);
                ps->binding->unref (it);
                it = next;
            }
            gtk_selection_data_set (selection_data, selection_data->target, sizeof (uint32_t) * 8, (gchar *)ptr, nsel * sizeof (uint32_t));
            free (ptr);
        }
        break;
    default:
        g_assert_not_reached ();
    }
}


void
ddb_listview_list_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    gchar *ptr=(char*)data->data;
    if (target_type == 0) { // uris
        // this happens when dropped from file manager
        char *mem = malloc (data->length+1);
        memcpy (mem, ptr, data->length);
        mem[data->length] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        ps->binding->external_drag_n_drop (mem, data->length, y);
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        int length = data->length/4;
        int drop_row = y / ps->rowheight + ps->scrollpos;
        DdbListviewIter drop_before = ps->binding->get_for_idx (drop_row);
        while (drop_before && ps->binding->is_selected (drop_before)) {
            DdbListviewIter next = PL_NEXT(drop_before);
            UNREF (drop_before);
            drop_before = next;
        }
        ps->binding->drag_n_drop (drop_before, d, length);
    }
    gtk_drag_finish (drag_context, TRUE, FALSE, time);
}

gboolean
ddb_listview_list_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data)
{
    return TRUE;
}


void
ddb_listview_list_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    ddb_listview_list_track_dragdrop (pl, -1);
}

// debug function for gdk_draw_drawable
static inline void
draw_drawable (GdkDrawable *window, GdkGC *gc, GdkDrawable *drawable, int x1, int y1, int x2, int y2, int w, int h) {
//    printf ("dd: %p %p %p %d %d %d %d %d %d\n", window, gc, drawable, x1, y1, x2, y2, w, h);
    gdk_draw_drawable (window, gc, drawable, x1, y1, x2, y2, w, h);
}

int
ddb_listview_get_vscroll_pos (DdbListview *listview) {
    return listview->scrollpos;
}

int
ddb_listview_get_hscroll_pos (DdbListview *listview) {
    return listview->hscrollpos;
}

#define MIN_COLUMN_WIDTH 16

static GdkCursor* cursor_sz;
static GdkCursor* cursor_drag;
#define COLHDR_ANIM_TIME 0.2f

#if 0
typedef struct {
    int c1;
    int c2;
    int x1, x2;
    int dx1, dx2;
    // animated values
    int ax1, ax2;
    timeline_t *timeline;
    int anim_active;
    DdbListview *pl;
} colhdr_animator_t;

static colhdr_animator_t colhdr_anim;

static gboolean
redraw_header (void *data) {
    colhdr_animator_t *anim = (colhdr_animator_t *)data;
    ddb_listview_header_render (anim->pl);
    ddb_listview_header_expose (anim->pl, 0, 0, anim->pl->header->allocation.width, anim->pl->header->allocation.height);
    return FALSE;
}

static int
colhdr_anim_cb (float _progress, int _last, void *_ctx) {
    colhdr_animator_t *anim = (colhdr_animator_t *)_ctx;
    anim->ax1 = anim->x1 + (float)(anim->dx1 - anim->x1) * _progress;
    anim->ax2 = anim->x2 + (float)(anim->dx2 - anim->x2) * _progress;
//    printf ("%f %d %d\n", _progress, anim->ax1, anim->ax2);
    g_idle_add (redraw_header, anim);
    if (_last) {
        anim->anim_active = 0;
    }
    return 0;
}

static void
colhdr_anim_swap (DdbListview *pl, int c1, int c2, int x1, int x2) {
    // interrupt previous anim
    if (!colhdr_anim.timeline) {
        colhdr_anim.timeline = timeline_create ();
    }
    colhdr_anim.pl = pl;

    colhdr_anim.c1 = c1;
    colhdr_anim.c2 = c2;

    // find c1 and c2 in column list and setup coords
    // note: columns are already swapped, so their coords must be reversed,
    // as if before swap
    DdbListviewColIter c;
    int idx = 0;
    int x = 0;
    for (c = pl->columns; c; c = c->next, idx++) {
        if (idx == c1) {
            colhdr_anim.x1 = x1;
            colhdr_anim.dx2 = x;
        }
        else if (idx == c2) {
            colhdr_anim.x2 = x2;
            colhdr_anim.dx1 = x;
        }
        x += c->width;
    }
    colhdr_anim.anim_active = 1;
    timeline_stop (colhdr_anim.timeline, 0);
    timeline_init (colhdr_anim.timeline, COLHDR_ANIM_TIME, 100, colhdr_anim_cb, &colhdr_anim);
    timeline_start (colhdr_anim.timeline);
}
#endif

void
ddb_listview_list_setup_vscroll (DdbListview *ps) {
    GtkWidget *playlist = ps->list;
    int h = playlist->allocation.height / ps->rowheight;
    int cnt = ps->binding->count ();
    int size = cnt;
    if (h >= size) {
        size = 0;
    }
    GtkWidget *scroll = ps->scrollbar;
    int row = ps->binding->cursor ();
    if (row >= cnt) {
        row = cnt - 1;
    }
    if (ps->scrollpos > cnt-ps->nvisiblerows+1) {
        int n = cnt - ps->nvisiblerows + 1;
        ps->scrollpos = max (0, n);
        gtk_range_set_value (GTK_RANGE (scroll), ps->scrollpos);
    }
    if (size == 0) {
        gtk_widget_hide (scroll);
    }
    else {
        GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, size, 1, h, h);
        gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
        gtk_widget_show (scroll);
    }
}

void
ddb_listview_list_setup_hscroll (DdbListview *ps) {
    GtkWidget *playlist = ps->list;
    int w = playlist->allocation.width;
    int size = 0;
    DdbListviewColIter c;
    for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c)) {
        size += ps->binding->col_get_width (c);
    }
    ps->totalwidth = size;
    if (ps->totalwidth < ps->list->allocation.width) {
        ps->totalwidth = ps->list->allocation.width;
    }
    if (w >= size) {
        size = 0;
    }
    GtkWidget *scroll = ps->hscrollbar;
    if (ps->hscrollpos >= size-w) {
        int n = size-w-1;
        ps->hscrollpos = max (0, n);
        gtk_range_set_value (GTK_RANGE (scroll), ps->hscrollpos);
    }
    if (size == 0) {
        gtk_widget_hide (scroll);
    }
    else {
        GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, size, 1, w, w);
        gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
        gtk_widget_show (scroll);
    }
}

void
ddb_listview_list_render_row (DdbListview *ps, int row, DdbListviewIter it) {
    draw_begin ((uintptr_t)ps->backbuf);
    ddb_listview_list_render_row_background (ps, row, it);
	if (it) {
        ddb_listview_list_render_row_foreground (ps, row, it);
    }
    draw_end ();
}

void
ddb_listview_list_expose_row (DdbListview *ps, int row, DdbListviewIter it) {
    int x, y, w, h;
    GtkWidget *widget = ps->list;
    x = 0;
    y = (row  - ps->scrollpos) * ps->rowheight;
    w = widget->allocation.width;
    h = ps->rowheight;
	draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, x, y, x, y, w, h);
}

void
ddb_listview_draw_row (DdbListview *ps, int row, DdbListviewIter it) {
    if (row < ps->scrollpos || row >= ps->scrollpos+ps->nvisiblerows) {
        return;
    }
    ddb_listview_list_render_row (ps, row, it);
    ddb_listview_list_expose_row (ps, row, it);
}

void
ddb_listview_list_render_row_background (DdbListview *ps, int row, DdbListviewIter it) {
	// draw background
	GtkWidget *treeview = theme_treeview;
	if (treeview->style->depth == -1) {
        return; // drawing was called too early
    }
    GTK_OBJECT_FLAGS (treeview) |= GTK_HAS_FOCUS;
    int x = -ps->hscrollpos;
    int w = ps->totalwidth;
    // clear area -- workaround for New Wave theme
    if (ps->list->style->bg_gc[GTK_STATE_NORMAL]) {
        gdk_draw_rectangle (ps->backbuf, ps->list->style->bg_gc[GTK_STATE_NORMAL], TRUE, 0, row * ps->rowheight - ps->scrollpos * ps->rowheight, ps->list->allocation.width, ps->rowheight);
    }
    gtk_paint_flat_box (treeview->style, ps->backbuf, (it && ps->binding->is_selected(it)) ? GTK_STATE_SELECTED : GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, treeview, (row & 1) ? "cell_even_ruled" : "cell_odd_ruled", x, row * ps->rowheight - ps->scrollpos * ps->rowheight, w, ps->rowheight);
	if (row == ps->binding->cursor ()) {
        // not all gtk engines/themes render focus rectangle in treeviews
        // but we want it anyway
        gdk_draw_rectangle (ps->backbuf, treeview->style->fg_gc[GTK_STATE_NORMAL], FALSE, x, row * ps->rowheight - ps->scrollpos * ps->rowheight, w-1, ps->rowheight-1);
        // gtkstyle focus drawing, for reference
//        gtk_paint_focus (treeview->style, ps->backbuf, (it && ps->binding->is_selected(it)) ? GTK_STATE_SELECTED : GTK_STATE_NORMAL, NULL, treeview, "treeview", x, row * ps->rowheight - ps->scrollpos * ps->rowheight, w, ps->rowheight);
    }
}

void
ddb_listview_list_render_row_foreground (DdbListview *ps, int row, DdbListviewIter it) {
    if (row-ps->scrollpos >= ps->nvisiblerows || row-ps->scrollpos < 0) {
//        fprintf (stderr, "WARNING: attempt to draw row outside of screen bounds (%d)\n", row-ps->scrollpos);
        return;
    }
	int width, height;
	draw_get_canvas_size ((uintptr_t)ps->backbuf, &width, &height);
	if (it && ps->binding->is_selected (it)) {
        GdkColor *clr = &theme_treeview->style->fg[GTK_STATE_SELECTED];
        float rgb[3] = { clr->red/65535.f, clr->green/65535.f, clr->blue/65535.f };
        draw_set_fg_color (rgb);
    }
    else {
        GdkColor *clr = &theme_treeview->style->fg[GTK_STATE_NORMAL];
        float rgb[3] = { clr->red/65535.f, clr->green/65535.f, clr->blue/65535.f };
        draw_set_fg_color (rgb);
    }
    int x = -ps->hscrollpos;
    DdbListviewColIter c;
    for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c)) {
        int cw = ps->binding->col_get_width (c);
        if (ps->binding->draw_column_data) {
            ps->binding->draw_column_data (ps->backbuf, it, row, c, x - ps->hscrollpos, row * ps->rowheight - ps->scrollpos * ps->rowheight, cw, ps->rowheight);
        }
        x += cw;
    }
}


void
ddb_listview_header_expose (DdbListview *ps, int x, int y, int w, int h) {
    GtkWidget *widget = ps->header;
	draw_drawable (widget->window, widget->style->black_gc, ps->backbuf_header, x, y, x, y, w, h);
}

void
ddb_listview_select_single (DdbListview *ps, int sel) {
    int idx=0;
    DdbListviewIter it = ps->binding->head ();
    for (; it; idx++) {
        if (idx == sel) {
            if (!ps->binding->is_selected (it)) {
                ps->binding->select (it, 1);
                ddb_listview_draw_row (ps, idx, it);
                ps->binding->selection_changed (it, idx);
            }
        }
        else if (ps->binding->is_selected (it)) {
            ps->binding->select (it, 0);
            ddb_listview_draw_row (ps, idx, it);
            ps->binding->selection_changed (it, idx);
        }
        DdbListviewIter next = PL_NEXT (it);
        UNREF (it);
        it = next;
    }
    UNREF (it);
}

// {{{ expected behaviour for mouse1 without modifiers:
//   {{{ [+] if clicked unselected item:
//       unselect all
//       select clicked item
//       deadbeef->pl_get_cursor (ps->iterator) = clicked
//       redraw
//       start 'area selection' mode
//   }}}
//   {{{ [+] if clicked selected item:
//       deadbeef->pl_get_cursor (ps->iterator) = clicked
//       redraw
//       wait until next release or motion event, whichever is 1st
//       if release is 1st:
//           unselect all except clicked, redraw
//       else if motion is 1st:
//           enter drag-drop mode
//   }}}
// }}}
void
ddb_listview_list_mouse1_pressed (DdbListview *ps, int state, int ex, int ey, double time) {
    // cursor must be set here, but selection must be handled in keyrelease
    int cnt = ps->binding->count ();
    if (cnt == 0) {
        return;
    }
    // remember mouse coords for doubleclick detection
    ps->lastpos[0] = ex;
    ps->lastpos[1] = ey;
    // select item
    int y = ey/ps->rowheight + ps->scrollpos;
    if (y < 0 || y >= ps->binding->count ()) {
        y = -1;
    }

    int cursor = ps->binding->cursor ();
    if (time - ps->clicktime < 0.5
            && fabs(ps->lastpos[0] - ex) < 3
            && fabs(ps->lastpos[1] - ey) < 3) {
        // doubleclick - play this item
        if (y != -1 && cursor != -1) {
            int idx = cursor;
            DdbListviewIter it = ps->binding->get_for_idx (idx);
            if (ps->binding->handle_doubleclick && it) {
                ps->binding->handle_doubleclick (ps, it, idx);
            }
            if (it) {
                ps->binding->unref (it);
            }
            return;
        }

        // prevent next click to trigger doubleclick
        ps->clicktime = time-1;
    }
    else {
        ps->clicktime = time;
    }

    int sel = y;
    if (y == -1) {
        y = ps->binding->count () - 1;
    }
    int prev = cursor;
    ps->binding->set_cursor (y);
    ps->shift_sel_anchor = ps->binding->cursor ();
    // handle multiple selection
    if (!(state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)))
    {
        DdbListviewIter it = ps->binding->get_for_idx (sel);
        if (!it || !ps->binding->is_selected (it)) {
            // reset selection, and set it to single item
            ddb_listview_select_single (ps, sel);
            ps->areaselect = 1;
            ps->areaselect_x = ex;
            ps->areaselect_y = ey;
            ps->areaselect_dx = -1;
            ps->areaselect_dy = -1;
            ps->shift_sel_anchor = ps->binding->cursor ();
        }
        else {
            ps->dragwait = 1;
            DdbListviewIter item = ps->binding->get_for_idx (prev);
            ddb_listview_draw_row (ps, prev, item);
            UNREF (item);
            int cursor = ps->binding->cursor ();
            if (cursor != prev) {
                DdbListviewIter item = ps->binding->get_for_idx (cursor);
                ddb_listview_draw_row (ps, cursor, item);
                UNREF (item);
            }
        }
        UNREF (it);
    }
    else if (state & GDK_CONTROL_MASK) {
        // toggle selection
        if (y != -1) {
            DdbListviewIter it = ps->binding->get_for_idx (y);
            if (it) {
                ps->binding->select (it, 1 - ps->binding->is_selected (it));
                ddb_listview_draw_row (ps, y, it);
                ps->binding->selection_changed (it, y);
                UNREF (it);
            }
        }
    }
    else if (state & GDK_SHIFT_MASK) {
        // select range
        int cursor = ps->binding->cursor ();
        int start = min (prev, cursor);
        int end = max (prev, cursor);
        int idx = 0;
        for (DdbListviewIter it = ps->binding->head (); it; idx++) {
            if (idx >= start && idx <= end) {
                if (!ps->binding->is_selected (it)) {
                    ps->binding->select (it, 1);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (it, idx);
                }
            }
            else {
                if (ps->binding->is_selected (it)) {
                    ps->binding->select (it, 0);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (it, idx);
                }
            }
            DdbListviewIter next = PL_NEXT (it);
            UNREF (it);
            it = next;
        }
    }
    cursor = ps->binding->cursor ();
    if (cursor != -1 && sel == -1) {
        DdbListviewIter it = ps->binding->get_for_idx (cursor);
        ddb_listview_draw_row (ps, cursor, it);
        UNREF (it);
    }
    if (prev != -1 && prev != cursor) {
        DdbListviewIter it = ps->binding->get_for_idx (prev);
        ddb_listview_draw_row (ps, prev, it);
        UNREF (it);
    }

}

void
ddb_listview_list_mouse1_released (DdbListview *ps, int state, int ex, int ey, double time) {
    if (ps->dragwait) {
        ps->dragwait = 0;
        int y = ey/ps->rowheight + ps->scrollpos;
        ddb_listview_select_single (ps, y);
    }
    else if (ps->areaselect) {
        ps->scroll_direction = 0;
        ps->scroll_pointer_y = -1;
        ps->areaselect = 0;
    }
}

#if 0
void
ddb_listview_list_dbg_draw_areasel (GtkWidget *widget, int x, int y) {
    // erase previous rect using 4 blits from ps->backbuffer
    if (areaselect_dx != -1) {
        int sx = min (areaselect_x, areaselect_dx);
        int sy = min (areaselect_y, areaselect_dy);
        int dx = max (areaselect_x, areaselect_dx);
        int dy = max (areaselect_y, areaselect_dy);
        int w = dx - sx + 1;
        int h = dy - sy + 1;
        //draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx, sy, sx, sy, dx - sx + 1, dy - sy + 1);
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx, sy, sx, sy, w, 1);
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx, sy, sx, sy, 1, h);
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx, sy + h - 1, sx, sy + h - 1, w, 1);
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, sx + w - 1, sy, sx + w - 1, sy, 1, h);
    }
    areaselect_dx = x;
    areaselect_dy = y;
	cairo_t *cr;
	cr = gdk_cairo_create (widget->window);
	if (!cr) {
		return;
	}
	theme_set_fg_color (COLO_PLAYLIST_CURSOR);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width (cr, 1);
    int sx = min (areaselect_x, x);
    int sy = min (areaselect_y, y);
    int dx = max (areaselect_x, x);
    int dy = max (areaselect_y, y);
    cairo_rectangle (cr, sx, sy, dx-sx, dy-sy);
    cairo_stroke (cr);
    cairo_destroy (cr);
}
#endif

static gboolean
ddb_listview_list_scroll_cb (gpointer data) {
    DdbListview *ps = (DdbListview *)data;
    ps->scroll_active = 1;
    struct timeval tm;
    gettimeofday (&tm, NULL);
    if (tm.tv_sec - ps->tm_prevscroll.tv_sec + (tm.tv_usec - ps->tm_prevscroll.tv_usec) / 1000000.0 < ps->scroll_sleep_time) {
        return TRUE;
    }
    memcpy (&ps->tm_prevscroll, &tm, sizeof (tm));
    if (ps->scroll_pointer_y == -1) {
        ps->scroll_active = 0;
        return FALSE;
    }
    if (ps->scroll_direction == 0) {
        ps->scroll_active = 0;
        return FALSE;
    }
    int sc = ps->scrollpos + ps->scroll_direction;
    if (sc < 0) {
        ps->scroll_active = 0;
        return FALSE;
    }
    if (sc >= ps->binding->count ()) {
        ps->scroll_active = 0;
        return FALSE;
    }
    gtk_range_set_value (GTK_RANGE (ps->scrollbar), sc);
    if (ps->scroll_mode == 0) {
        GdkEventMotion ev;
        ev.y = ps->scroll_pointer_y;
        ddb_listview_list_mousemove (ps, &ev);
    }
    else if (ps->scroll_mode == 1) {
        ddb_listview_list_track_dragdrop (ps, ps->scroll_pointer_y);
    }
    ps->scroll_sleep_time -= 0.1;
    if (ps->scroll_sleep_time < 0.05) {
        ps->scroll_sleep_time = 0.05;
    }
    return TRUE;
}

void
ddb_listview_list_mousemove (DdbListview *ps, GdkEventMotion *event) {
    if (ps->dragwait) {
        GtkWidget *widget = ps->list;
        if (gtk_drag_check_threshold (widget, ps->lastpos[0], event->x, ps->lastpos[1], event->y)) {
            ps->dragwait = 0;
            GtkTargetEntry entry = {
                .target = "STRING",
                .flags = GTK_TARGET_SAME_WIDGET,
                .info = TARGET_SAMEWIDGET
            };
            GtkTargetList *lst = gtk_target_list_new (&entry, 1);
            gtk_drag_begin (widget, lst, GDK_ACTION_MOVE, TARGET_SAMEWIDGET, (GdkEvent *)event);
        }
    }
    else if (ps->areaselect) {
        int y = event->y/ps->rowheight + ps->scrollpos;
        //if (y != shift_sel_anchor)
        {
            int start = min (y, ps->shift_sel_anchor);
            int end = max (y, ps->shift_sel_anchor);
            int idx=0;
            DdbListviewIter it;
            for (it = ps->binding->head (); it; idx++) {
                if (idx >= start && idx <= end) {
                    if (!ps->binding->is_selected (it)) {
                        ps->binding->select (it, 1);
                        ddb_listview_draw_row (ps, idx, it);
                        ps->binding->selection_changed (it, idx);
                    }
                }
                else if (ps->binding->is_selected (it)) {
                    ps->binding->select (it, 0);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (it, idx);
                }
                DdbListviewIter next = PL_NEXT(it);
                UNREF (it);
                it = next;
            }
            UNREF (it);
        }

        if (event->y < 10) {
            ps->scroll_mode = 0;
            ps->scroll_pointer_y = event->y;
            ps->scroll_direction = -1;
            // start scrolling up
            if (!ps->scroll_active) {
                ps->scroll_sleep_time = 0.2;
                gettimeofday (&ps->tm_prevscroll, NULL);
                g_idle_add (ddb_listview_list_scroll_cb, ps);
            }
        }
        else if (event->y > ps->list->allocation.height-10) {
            ps->scroll_mode = 0;
            ps->scroll_pointer_y = event->y;
            ps->scroll_direction = 1;
            // start scrolling up
            if (!ps->scroll_active) {
                ps->scroll_sleep_time = 0.2;
                gettimeofday (&ps->tm_prevscroll, NULL);
                g_idle_add (ddb_listview_list_scroll_cb, ps);
            }
        }
        else {
            ps->scroll_direction = 0;
            ps->scroll_pointer_y = -1;
        }
        // debug only
        // ddb_listview_list_dbg_draw_areasel (widget, event->x, event->y);
    }
}

void
ddb_listview_list_set_hscroll (DdbListview *ps, int newscroll) {
    if (newscroll != ps->hscrollpos) {
        ps->hscrollpos = newscroll;
        GtkWidget *widget = ps->list;
        ddb_listview_header_render (ps);
        ddb_listview_header_expose (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        ddb_listview_list_render (ps, 0, 0, widget->allocation.width, widget->allocation.height);
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
    }
}

int
ddb_listview_handle_keypress (DdbListview *ps, int keyval, int state) {
    int prev = ps->binding->cursor ();
    int cursor = prev;
    if (keyval == GDK_Down) {
//        cursor = deadbeef->pl_get_cursor (ps->iterator);
        if (cursor < ps->binding->count () - 1) {
            cursor++;
        }
    }
    else if (keyval == GDK_Up) {
//        cursor = deadbeef->pl_get_cursor (ps->iterator);
        if (cursor > 0) {
            cursor--;
        }
        else if (cursor < 0 && ps->binding->count () > 0) {
            cursor = 0;
        }
    }
    else if (keyval == GDK_Page_Down) {
//        cursor = deadbeef->pl_get_cursor (ps->iterator);
        if (cursor < ps->binding->count () - 1) {
            cursor += 10;
            if (cursor >= ps->binding->count ()) {
                cursor = ps->binding->count () - 1;
            }
        }
    }
    else if (keyval == GDK_Page_Up) {
//        cursor = deadbeef->pl_get_cursor (ps->iterator);
        if (cursor > 0) {
            cursor -= 10;
            if (cursor < 0) {
                cursor = 0;
            }
        }
    }
    else if (keyval == GDK_End) {
        cursor = ps->binding->count () - 1;
    }
    else if (keyval == GDK_Home) {
        cursor = 0;
    }
    else if (keyval == GDK_Delete) {
        ps->binding->delete_selected ();
        cursor = ps->binding->cursor ();
    }
    else {
        return 0 ;
    }
    if (state & GDK_SHIFT_MASK) {
        if (cursor != prev) {
            int newscroll = ps->scrollpos;
            if (cursor < ps->scrollpos) {
                newscroll = cursor;
            }
            else if (cursor >= ps->scrollpos + ps->nvisiblefullrows) {
                newscroll = cursor - ps->nvisiblefullrows + 1;
                if (newscroll < 0) {
                    newscroll = 0;
                }
            }
            if (ps->scrollpos != newscroll) {
                GtkWidget *range = ps->scrollbar;
                gtk_range_set_value (GTK_RANGE (range), newscroll);
            }

            ps->binding->set_cursor (cursor);
            // select all between shift_sel_anchor and deadbeef->pl_get_cursor (ps->iterator)
            int start = min (cursor, ps->shift_sel_anchor);
            int end = max (cursor, ps->shift_sel_anchor);
            int idx=0;
            DdbListviewIter it;
            for (it = ps->binding->head (); it; idx++) {
                if (idx >= start && idx <= end) {
                    ps->binding->select (it, 1);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (it, idx);
                }
                else if (ps->binding->is_selected (it))
                {
                    ps->binding->select (it, 0);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (it, idx);
                }
                DdbListviewIter next = PL_NEXT(it);
                UNREF (it);
                it = next;
            }
            UNREF (it);
        }
    }
    else {
        ps->shift_sel_anchor = cursor;
        ddb_listview_set_cursor (ps, cursor);
    }
    return 1;
}

void
ddb_listview_list_track_dragdrop (DdbListview *ps, int y) {
    GtkWidget *widget = ps->list;
    if (ps->drag_motion_y != -1) {
        // erase previous track
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, ps->drag_motion_y * ps->rowheight-3, 0, ps->drag_motion_y * ps->rowheight-3, widget->allocation.width, 7);

    }
    if (y == -1) {
        ps->drag_motion_y = -1;
        return;
    }
    draw_begin ((uintptr_t)widget->window);
    ps->drag_motion_y = y / ps->rowheight;

    //theme_set_fg_color (COLO_DRAGDROP_MARKER);
    GtkStyle *style = gtk_widget_get_style (GTK_WIDGET (ps));
    float clr[3] = { style->fg[GTK_STATE_NORMAL].red, style->fg[GTK_STATE_NORMAL].green, style->fg[GTK_STATE_NORMAL].blue };
    draw_set_fg_color (clr);

    draw_rect (0, ps->drag_motion_y * ps->rowheight-1, widget->allocation.width, 3, 1);
    draw_rect (0, ps->drag_motion_y * ps->rowheight-3, 3, 7, 1);
    draw_rect (widget->allocation.width-3, ps->drag_motion_y * ps->rowheight-3, 3, 7, 1);
    draw_end ();
    if (y < 10) {
        ps->scroll_pointer_y = y;
        ps->scroll_direction = -1;
        ps->scroll_mode = 1;
        // start scrolling up
        if (!ps->scroll_active) {
            ps->scroll_sleep_time = 0.2;
            gettimeofday (&ps->tm_prevscroll, NULL);
            g_idle_add (ddb_listview_list_scroll_cb, ps);
        }
    }
    else if (y > ps->list->allocation.height-10) {
        ps->scroll_mode = 1;
        ps->scroll_pointer_y = y;
        ps->scroll_direction = 1;
        // start scrolling up
        if (!ps->scroll_active) {
            ps->scroll_sleep_time = 0.2;
            gettimeofday (&ps->tm_prevscroll, NULL);
            g_idle_add (ddb_listview_list_scroll_cb, ps);
        }
    }
    else {
        ps->scroll_direction = 0;
        ps->scroll_pointer_y = -1;
    }
}

void
ddb_listview_list_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    ddb_listview_refresh (ps, DDB_REFRESH_LIST|DDB_EXPOSE_LIST);
    ps->scroll_direction = 0;
    ps->scroll_pointer_y = -1;
}

void
ddb_listview_header_render (DdbListview *ps) {
    GtkWidget *widget = ps->header;
    int x = -ps->hscrollpos;
    int w = 100;
    int h = widget->allocation.height;
    const char *detail = "button";

    // fill background
    gtk_paint_box (widget->style, ps->backbuf_header, GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, widget, detail, -10, -10, widget->allocation.width+20, widget->allocation.height+20);
    gdk_draw_line (ps->backbuf_header, widget->style->mid_gc[GTK_STATE_NORMAL], 0, widget->allocation.height-1, widget->allocation.width, widget->allocation.height-1);
    draw_begin ((uintptr_t)ps->backbuf_header);
    x = -ps->hscrollpos;
    DdbListviewColIter c;
    int need_draw_moving = 0;
    int idx = 0;
    for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c), idx++) {
        w = ps->binding->col_get_width (c);
        int xx = x;
#if 0
        if (colhdr_anim.anim_active) {
            if (idx == colhdr_anim.c2) {
                xx = colhdr_anim.ax1;
            }
            else if (idx == colhdr_anim.c1) {
                xx = colhdr_anim.ax2;
            }
        }
#endif
        if (ps->header_dragging < 0 || idx != ps->header_dragging) {
            if (xx >= widget->allocation.width) {
                continue;
            }
            int arrow_sz = 10;
            int sort = ps->binding->col_get_sort (c);
            if (w > 0) {
                gtk_paint_vline (widget->style, ps->backbuf_header, GTK_STATE_NORMAL, NULL, NULL, NULL, 2, h-4, xx+w - 2);
                GdkColor *gdkfg = &widget->style->fg[0];
                float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
                draw_set_fg_color (fg);
                int ww = w-10;
                if (sort) {
                    ww -= arrow_sz;
                    if (ww < 0) {
                        ww = 0;
                    }
                }
                draw_text (xx + 5, h/2-draw_get_font_size()/2, ww, 0, ps->binding->col_get_title (c));
            }
            if (sort) {
                int dir = sort == 1 ? GTK_ARROW_DOWN : GTK_ARROW_UP;
                gtk_paint_arrow (widget->style, ps->backbuf_header, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, dir, TRUE, xx + w-arrow_sz-5, widget->allocation.height/2-arrow_sz/2, arrow_sz, arrow_sz);
            }
        }
        else {
            need_draw_moving = 1;
        }
        x += w;
    }
    if (need_draw_moving) {
        x = -ps->hscrollpos;
        idx = 0;
        for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c), idx++) {
            w = ps->binding->col_get_width (c);
            if (idx == ps->header_dragging) {
#if 0
                if (colhdr_anim.anim_active) {
                    if (idx == colhdr_anim.c2) {
                        x = colhdr_anim.ax1;
                    }
                    else if (idx == colhdr_anim.c1) {
                        x = colhdr_anim.ax2;
                    }
                }
#endif
                // draw empty slot
                if (x < widget->allocation.width) {
                    gtk_paint_box (widget->style, ps->backbuf_header, GTK_STATE_ACTIVE, GTK_SHADOW_ETCHED_IN, NULL, widget, "button", x, 0, w, h);
                }
                x = ps->col_movepos;
                if (x >= widget->allocation.width) {
                    break;
                }
                if (w > 0) {
                    gtk_paint_box (widget->style, ps->backbuf_header, GTK_STATE_SELECTED, GTK_SHADOW_OUT, NULL, widget, "button", x, 0, w, h);
                    GdkColor *gdkfg = &widget->style->fg[GTK_STATE_SELECTED];
                    float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
                    draw_set_fg_color (fg);
                    draw_text (x + 5, h/2-draw_get_font_size()/2, ps->binding->col_get_width (c)-10, 0, ps->binding->col_get_title (c));
                }
                break;
            }
            x += w;
        }
    }
    draw_end ();
}

gboolean
ddb_listview_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    ddb_listview_header_render (ps); // FIXME
    ddb_listview_header_expose (ps, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}


gboolean
ddb_listview_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    if (ps->backbuf_header) {
        g_object_unref (ps->backbuf_header);
        ps->backbuf_header = NULL;
    }
    ps->backbuf_header = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);
    ddb_listview_header_render (ps);
    return FALSE;
}


void
ddb_listview_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data)
{
    // create cursor for sizing headers
    int h = draw_get_font_size ();
    gtk_widget_set_size_request (widget, -1, h + 10);
    cursor_sz = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    cursor_drag = gdk_cursor_new (GDK_FLEUR);
}

gboolean
ddb_listview_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    int ev_x, ev_y;
    GdkModifierType ev_state;

    if (event->is_hint)
        gdk_window_get_pointer (event->window, &ev_x, &ev_y, &ev_state);
    else
    {
        ev_x = event->x;
        ev_y = event->y;
        ev_state = event->state;
    }


    if ((ev_state & GDK_BUTTON1_MASK) && ps->header_prepare) {
        if (gtk_drag_check_threshold (widget, ev_x, ps->prev_header_x, 0, 0)) {
            ps->header_prepare = 0;
        }
    }
    if (!ps->header_prepare && ps->header_dragging >= 0) {
        gdk_window_set_cursor (widget->window, cursor_drag);
        DdbListviewColIter c;
        int i;
        for (i = 0, c = ps->binding->col_first (); i < ps->header_dragging && c; c = ps->binding->col_next (c), i++);
        ps->col_movepos = ev_x - ps->header_dragpt[0];

        // find closest column to the left
        int inspos = -1;
        DdbListviewColIter cc;
        int x = 0;
        int idx = 0;
        int x1 = -1, x2 = -1;
        for (cc = ps->binding->col_first (); cc; cc = ps->binding->col_next (cc), idx++) {
            if (x < ps->col_movepos && x + ps->binding->col_get_width (c) > ps->col_movepos) {
                inspos = idx;
                x1 = x;
            }
            else if (idx == ps->header_dragging) {
                x2 = x;
            }
            x += ps->binding->col_get_width (cc);
        }
        if (inspos >= 0 && inspos != ps->header_dragging) {
            ps->binding->col_move (c, inspos);
            ps->header_dragging = inspos;
//            colhdr_anim_swap (ps, c1, c2, x1, x2);
            // force redraw of everything
//            ddb_listview_list_setup_hscroll (ps);
            ddb_listview_list_render (ps, 0, 0, ps->list->allocation.width, ps->list->allocation.height);
            ddb_listview_list_expose (ps, 0, 0, ps->list->allocation.width, ps->list->allocation.height);
        }
        else {
            // only redraw that if not animating
            ddb_listview_header_render (ps);
            ddb_listview_header_expose (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        }
    }
    else if (ps->header_sizing >= 0) {
        ps->last_header_motion_ev = event->time;
        ps->prev_header_x = ev_x;
        gdk_window_set_cursor (widget->window, cursor_sz);
        // get column start pos
        int x = -ps->hscrollpos;
        int i = 0;
        DdbListviewColIter c;
        for (c = ps->binding->col_first (); c && i < ps->header_sizing; c = ps->binding->col_next (c), i++) {
            x += ps->binding->col_get_width (c);
        }

        int newx = ev_x > x + MIN_COLUMN_WIDTH ? ev_x : x + MIN_COLUMN_WIDTH;
        ps->binding->col_set_width (c, newx-x);
        ddb_listview_list_setup_hscroll (ps);
        ddb_listview_header_render (ps);
        ddb_listview_header_expose (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        ddb_listview_list_render (ps, 0, 0, ps->list->allocation.width, ps->list->allocation.height);
        ddb_listview_list_expose (ps, 0, 0, ps->list->allocation.width, ps->list->allocation.height);
    }
    else {
        int x = -ps->hscrollpos;
        DdbListviewColIter c;
        for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c)) {
            int w = ps->binding->col_get_width (c);
            if (w > 0) { // ignore collapsed columns (hack for search window)
                if (ev_x >= x + w - 2 && ev_x <= x + w) {
                    gdk_window_set_cursor (widget->window, cursor_sz);
                    break;
                }
                else {
                    gdk_window_set_cursor (widget->window, NULL);
                }
            }
            else {
                gdk_window_set_cursor (widget->window, NULL);
            }
            x += w;
        }
    }
    return FALSE;
}

DdbListviewColIter
ddb_listview_header_get_column_for_coord (DdbListview *pl, int click_x) {
    int x = -pl->hscrollpos;
    DdbListviewColIter c;
    for (c = pl->binding->col_first (); c; c = pl->binding->col_next (c)) {
        int w = pl->binding->col_get_width (c);
        if (click_x >= x && click_x < x + w) {
            return c;
        }
        x += w;
    }
    return NULL;
}

gboolean
ddb_listview_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
//    ps->active_column = ddb_listview_header_get_column_for_coord (ps, event->x);
    if (event->button == 1) {
        // start sizing/dragging
        ps->header_dragging = -1;
        ps->header_sizing = -1;
        ps->header_dragpt[0] = event->x;
        ps->header_dragpt[1] = event->y;
        int x = -ps->hscrollpos;
        int i = 0;
        DdbListviewColIter c;
        for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c), i++) {
            int w = ps->binding->col_get_width (c);
            if (event->x >= x + w - 2 && event->x <= x + w) {
                ps->header_sizing = i;
                ps->header_dragging = -1;
                break;
            }
            else if (event->x > x + 2 && event->x < x + w - 2) {
                // prepare to drag or sort
                ps->header_dragpt[0] = event->x - x;
                ps->header_prepare = 1;
                ps->header_dragging = i;
                ps->header_sizing = -1;
                ps->prev_header_x = event->x;
                break;
            }
            x += w;
        }
    }
    else if (event->button == 3) {
        ps->binding->header_context_menu (ps, ddb_listview_header_get_column_for_coord (ps, event->x));
    }
    ps->prev_header_x = -1;
    ps->last_header_motion_ev = -1;
    return FALSE;
}

gboolean
ddb_listview_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    if (event->button == 1) {
        if (ps->header_prepare) {
            ps->header_sizing = -1;
            ps->header_dragging = -1;
            ps->header_prepare = 0;
            // sort
            DdbListviewColIter c;
            int i = 0;
            int x = -ps->hscrollpos;
            int sorted = 0;
            for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c), i++) {
                int w = ps->binding->col_get_width (c);
                if (event->x > x + 2 && event->x < x + w - 2) {
                    int sort_order = ps->binding->col_get_sort (c);
                    if (!sort_order) {
                        ps->binding->col_set_sort (c, 1);
                    }
                    else if (sort_order == 1) {
                        ps->binding->col_set_sort (c, 2);
                    }
                    else if (sort_order == 2) {
                        ps->binding->col_set_sort (c, 1);
                    }
                    ps->binding->col_sort (c);
                    sorted = 1;
                }
                else {
                    ps->binding->col_set_sort (c, 0);
                }
                x += w;
            }
            ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_COLUMNS | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
        }
        else {
            ps->header_sizing = -1;
            int x = 0;
            DdbListviewColIter c;
            for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c)) {
                int w = ps->binding->col_get_width (c);
                if (event->x >= x + w - 2 && event->x <= x + w) {
                    gdk_window_set_cursor (widget->window, cursor_sz);
                    break;
                }
                else {
                    gdk_window_set_cursor (widget->window, NULL);
                }
                x += w;
            }
            if (ps->header_dragging >= 0) {
                ps->header_dragging = -1;
                ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_COLUMNS | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS | DDB_REFRESH_HSCROLL);
            }
        }
    }
    return FALSE;
}

// FIXME: port (maybe)
#if 0
int
gtkpl_get_idx_of (DdbListview *ps, DdbListviewIter it) {
    DdbListviewIter c = ps->binding->head ();
    int idx = 0;
    while (c && c != it) {
        DdbListviewIter next = PL_NEXT(c); 
        UNREF (c);
        c = next;
        idx++;
    }
    if (!c) {
        return -1;
    }
    UNREF (c);
    return idx;
}

DdbListviewIter 
gtkpl_get_for_idx (DdbListview *ps, int idx) {
    DdbListviewIter it = ps->binding->head ();
    while (idx--) {
        if (!it) {
            return NULL;
        }
        DdbListviewIter next = PL_NEXT(it);
        UNREF (it);
        it = next;
    }
    return it;
}
void
gtk_pl_redraw_item_everywhere (DdbListviewIter it) {
    DdbListview *pl = &search_playlist;
    int idx = gtkpl_get_idx_of (pl, it);
    int minvis = pl->scrollpos;
    int maxvis = pl->scrollpos + pl->nvisiblerows-1;
    if (idx >= minvis && idx <= maxvis) {
        ddb_listview_draw_row (pl, idx, it);
    }
    pl = &main_playlist;
    idx = gtkpl_get_idx_of (pl, it);
    minvis = pl->scrollpos;
    maxvis = pl->scrollpos + pl->nvisiblerows-1;
    if (idx >= minvis && idx <= maxvis) {
        ddb_listview_draw_row (pl, idx, it);
    }
}
#endif

struct set_cursor_t {
    int cursor;
    int prev;
    DdbListview *pl;
};

static gboolean
ddb_listview_set_cursor_cb (gpointer data) {
    struct set_cursor_t *sc = (struct set_cursor_t *)data;
    sc->pl->binding->set_cursor (sc->cursor);
    ddb_listview_select_single (sc->pl, sc->cursor);
    DdbListviewIter it;
    int minvis = sc->pl->scrollpos;
    int maxvis = sc->pl->scrollpos + sc->pl->nvisiblerows-1;
    DdbListview *ps = sc->pl;
    if (sc->prev >= minvis && sc->prev <= maxvis) {
        it = sc->pl->binding->get_for_idx (sc->prev);
        ddb_listview_draw_row (sc->pl, sc->prev, it);
        UNREF (it);
    }
    if (sc->cursor >= minvis && sc->cursor <= maxvis) {
        it = sc->pl->binding->get_for_idx (sc->cursor);
        ddb_listview_draw_row (sc->pl, sc->cursor, it);
        UNREF (it);
    }

    int newscroll = sc->pl->scrollpos;
    if (sc->cursor < sc->pl->scrollpos) {
        newscroll = sc->cursor;
    }
    else if (sc->cursor >= sc->pl->scrollpos + sc->pl->nvisiblefullrows) {
        newscroll = sc->cursor - sc->pl->nvisiblefullrows + 1;
        if (newscroll < 0) {
            newscroll = 0;
        }
    }
    if (sc->pl->scrollpos != newscroll) {
        GtkWidget *range = sc->pl->scrollbar;
        gtk_range_set_value (GTK_RANGE (range), newscroll);
    }

    free (data);
    return FALSE;
}

void
ddb_listview_set_cursor (DdbListview *pl, int cursor) {
    int prev = pl->binding->cursor ();
    struct set_cursor_t *data = malloc (sizeof (struct set_cursor_t));
    data->prev = prev;
    data->cursor = cursor;
    data->pl = pl;
    g_idle_add (ddb_listview_set_cursor_cb, data);
}

gboolean
ddb_listview_list_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    if (event->button == 1) {
        ddb_listview_list_mouse1_pressed (ps, event->state, event->x, event->y, event->time);
    }
    else if (event->button == 3) {
        // get item under cursor
        int y = event->y / ps->rowheight + ps->scrollpos;
        if (y < 0 || y >= ps->binding->count ()) {
            y = -1;
        }
        DdbListviewIter it = ps->binding->get_for_idx (y);
        if (!it) {
            // clicked empty space -- deselect everything and show insensitive menu
            ps->binding->set_cursor (-1);
            it = ps->binding->head ();
            int idx = 0;
            while (it) {
                ps->binding->select (it, 0);
                ddb_listview_draw_row (ps, idx, it);
                ps->binding->selection_changed (it, idx);
                it = PL_NEXT (it);
                idx++;
            }
            // no menu
        }
        else {
            if (!ps->binding->is_selected (it)) {
                // item is unselected -- reset selection and select this
                DdbListviewIter it2 = ps->binding->head ();
                int idx = 0;
                while (it2) {
                    if (ps->binding->is_selected (it2) && it2 != it) {
                        ps->binding->select (it2, 0);
                        ddb_listview_draw_row (ps, idx, it2);
                        ps->binding->selection_changed (it2, idx);
                    }
                    else if (it2 == it) {
                        ps->binding->set_cursor (y);
                        ps->binding->select (it2, 1);
                        ddb_listview_draw_row (ps, idx, it2);
                        ps->binding->selection_changed (it2, idx);
                    }
                    it2 = PL_NEXT (it2);
                    idx++;
                }
            }
            else {
                // something is selected; move cursor but keep selection
                ps->binding->set_cursor (y);
                ddb_listview_draw_row (ps, y, it);
            }
            ps->binding->list_context_menu (ps, it, y);
        }
    }
    return FALSE;
}

gboolean
ddb_listview_list_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    if (event->button == 1) {
        ddb_listview_list_mouse1_released (ps, event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
ddb_listview_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (gtk_object_get_data (GTK_OBJECT (widget), "owner"));
    ddb_listview_list_mousemove (ps, event);
    return FALSE;
}

void
ddb_listview_set_binding (DdbListview *listview, DdbListviewBinding *binding) {
    listview->binding = binding;
}

DdbListviewIter
ddb_listview_get_iter_from_coord (DdbListview *listview, int x, int y) {
    return listview->binding->get_for_idx ((y + listview->scrollpos)/listview->rowheight);
}

void
ddb_listview_scroll_to (DdbListview *listview, int pos) {
    if (pos < listview->scrollpos || pos >= listview->scrollpos + listview->nvisiblefullrows) {
        gtk_range_set_value (GTK_RANGE (listview->scrollbar), pos - listview->nvisiblerows/2);
    }
}
int
ddb_listview_is_scrolling (DdbListview *listview) {
    return listview->dragwait;
}
