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
#include "gtkplaylist.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "search.h"
#include "progress.h"
#include "drawing.h"
#include "../../session.h"
#include "parser.h"
#include "gtkui.h"

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

// signal handlers
void
on_vscroll_value_changed            (GtkRange        *widget,
                                        gpointer         user_data);
void
on_hscroll_value_changed           (GtkRange        *widget,
                                        gpointer         user_data);

void
on_playlist_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_playlist_drag_data_delete           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

gboolean
on_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

void
on_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_playlist_configure_event            (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_playlist_expose_event               (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

void
on_playlist_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_playlist_drag_begin                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

gboolean
on_playlist_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_playlist_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

void
on_playlist_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_playlist_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

gboolean
on_playlist_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data);

void
on_playlist_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_playlist_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_playlist_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);
gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_playlist_expose_event               (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_playlist_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_playlist_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_playlist_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);


static void
ddb_listview_class_init(DdbListviewClass *class)
{
  GtkTableClass *widget_class;
  widget_class = (GtkTableClass *) class;

//  widget_class->realize = ddb_listview_realize;
//  widget_class->size_request = ddb_listview_size_request;
//  widget_class->size_allocate = ddb_listview_size_allocate;
//  widget_class->expose_event = ddb_listview_expose;
//  widget_class->destroy = ddb_listview_destroy;
}

static void
ddb_listview_init(DdbListview *listview)
{
    // init instance - create all subwidgets, and insert into table
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

    listview->playlist = gtk_drawing_area_new ();
    gtk_widget_show (listview->playlist);
    gtk_box_pack_start (GTK_BOX (vbox), listview->playlist, TRUE, TRUE, 0);
    gtk_widget_set_events (listview->playlist, GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

    listview->hscrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 0, 0, 0)));
    gtk_widget_show (listview->hscrollbar);
    gtk_table_attach (GTK_TABLE (listview), listview->hscrollbar, 0, 1, 1, 2,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (GTK_FILL), 0, 0);

    g_signal_connect ((gpointer) listview->scrollbar, "value_changed",
            G_CALLBACK (on_vscroll_value_changed),
            NULL);
    g_signal_connect ((gpointer) listview->header, "expose_event",
            G_CALLBACK (on_header_expose_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "configure_event",
            G_CALLBACK (on_header_configure_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "realize",
            G_CALLBACK (on_header_realize),
            NULL);
    g_signal_connect ((gpointer) listview->header, "motion_notify_event",
            G_CALLBACK (on_header_motion_notify_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "button_press_event",
            G_CALLBACK (on_header_button_press_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "button_release_event",
            G_CALLBACK (on_header_button_release_event),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "configure_event",
            G_CALLBACK (on_playlist_configure_event),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "expose_event",
            G_CALLBACK (on_playlist_expose_event),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "realize",
            G_CALLBACK (on_playlist_realize),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "button_press_event",
            G_CALLBACK (on_playlist_button_press_event),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "scroll_event",
            G_CALLBACK (on_playlist_scroll_event),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_begin",
            G_CALLBACK (on_playlist_drag_begin),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_motion",
            G_CALLBACK (on_playlist_drag_motion),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_drop",
            G_CALLBACK (on_playlist_drag_drop),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_data_get",
            G_CALLBACK (on_playlist_drag_data_get),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_end",
            G_CALLBACK (on_playlist_drag_end),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_failed",
            G_CALLBACK (on_playlist_drag_failed),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_leave",
            G_CALLBACK (on_playlist_drag_leave),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "button_release_event",
            G_CALLBACK (on_playlist_button_release_event),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "motion_notify_event",
            G_CALLBACK (on_playlist_motion_notify_event),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_data_received",
            G_CALLBACK (on_playlist_drag_data_received),
            NULL);
    g_signal_connect ((gpointer) listview->playlist, "drag_data_delete",
            G_CALLBACK (on_playlist_drag_data_delete),
            NULL);
    g_signal_connect ((gpointer) listview->hscrollbar, "value_changed",
            G_CALLBACK (on_hscroll_value_changed),
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
    if (flags & (DDB_REFRESH_HSCROLL|DDB_REFRESH_VSCROLL)) {
        gtkpl_setup_scrollbar (listview);
    }
    if (flags & DDB_REFRESH_COLUMNS) {
    }
    if (flags & DDB_REFRESH_LIST) {
        gtkpl_draw_playlist (listview, 0, 0, listview->playlist->allocation.width, listview->playlist->allocation.height);
    }
    if (flags & DDB_EXPOSE_COLUMNS) {
    }
    if (flags & DDB_EXPOSE_LIST) {
        gtkpl_expose (listview, 0, 0, listview->playlist->allocation.width, listview->playlist->allocation.height);
    }
}

gboolean
on_playlist_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    // draw visible area of playlist
    gtkpl_expose (DDB_LISTVIEW (widget), event->area.x, event->area.y, event->area.width, event->area.height);

    return FALSE;
}

gboolean
on_playlist_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	GdkEventScroll *ev = (GdkEventScroll*)event;
    gtkpl_handle_scroll_event (DDB_LISTVIEW (widget), ev->direction);
    return FALSE;
}

void
on_vscroll_value_changed            (GtkRange        *widget,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (widget);
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    gtkpl_scroll (pl, newscroll);
}

void
on_hscroll_value_changed           (GtkRange        *widget,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (widget);
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    gtkpl_hscroll (pl, newscroll);
}

void
on_playlist_drag_begin                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
}

gboolean
on_playlist_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    gtkpl_track_dragdrop (DDB_LISTVIEW (widget), y);
    return FALSE;
}


gboolean
on_playlist_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
#if 0
    if (drag_context->targets) {
        GdkAtom target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets, TARGET_SAMEWIDGET));
        if (!target_type) {
            return FALSE;
        }
        gtk_drag_get_data (widget, drag_context, target_type, time);
        return TRUE;
    }
#endif
    return FALSE;
}


void
on_playlist_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *selection_data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
// FIXME: port
#if 0
    switch (target_type) {
    case TARGET_SAMEWIDGET:
        {
            // format as "STRING" consisting of array of pointers
            int nsel = deadbeef->pl_getselcount ();
            if (!nsel) {
                break; // something wrong happened
            }
            uint32_t *ptr = malloc (nsel * sizeof (uint32_t));
            int idx = 0;
            int i = 0;
            DdbListviewIter it = ps->binding->first ();
            for (; it; idx++) {
                if (deadbeef->pl_is_selected (it)) {
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
#endif
}


void
on_playlist_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
// FIXME: port
#if 0
    DdbListview *pl = DDB_LISTVIEW (widget);
    gchar *ptr=(char*)data->data;
    if (target_type == 0) { // uris
        // this happens when dropped from file manager
        char *mem = malloc (data->length+1);
        memcpy (mem, ptr, data->length);
        mem[data->length] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        gtkui_receive_fm_drop (mem, data->length, y);
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        int length = data->length/4;
        int drop_row = y / rowheight + pl->scrollpos;
        DdbListviewIter drop_before = deadbeef->pl_get_for_idx_and_iter (drop_row, pl->iterator);
        while (drop_before && ps->binding->is_selected (drop_before)) {
            drop_before = PL_NEXT(drop_before, pl->iterator);
        }
        deadbeef->pl_move_items (pl->iterator, drop_before, d, length);
    }
    gtk_drag_finish (drag_context, TRUE, FALSE, time);
#endif
}


void
on_playlist_drag_data_delete           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
}


gboolean
on_playlist_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data)
{
    return TRUE;
}


void
on_playlist_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data)
{
    gtkpl_track_dragdrop (DDB_LISTVIEW (widget), -1);
}

void
gtkpl_configure (DdbListview *ps) {
    gtkpl_setup_scrollbar (ps);
    gtkpl_setup_hscrollbar (ps);
    GtkWidget *widget = ps->playlist;
    if (ps->backbuf) {
        g_object_unref (ps->backbuf);
        ps->backbuf = NULL;
    }
    ps->nvisiblerows = ceil (widget->allocation.height / (float)rowheight);
    ps->nvisiblefullrows = floor (widget->allocation.height / (float)rowheight);
    ps->backbuf = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);

    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
}

// change properties
gboolean
on_playlist_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
    gtkpl_configure (DDB_LISTVIEW (widget));
    return FALSE;
}

GtkWidget*
create_headermenu (void)
{
  GtkWidget *headermenu;
  GtkWidget *add_column;
  GtkWidget *edit_column;
  GtkWidget *remove_column;

  headermenu = gtk_menu_new ();

  add_column = gtk_menu_item_new_with_mnemonic ("Add column");
  gtk_widget_show (add_column);
  gtk_container_add (GTK_CONTAINER (headermenu), add_column);

  edit_column = gtk_menu_item_new_with_mnemonic ("Edit column");
  gtk_widget_show (edit_column);
  gtk_container_add (GTK_CONTAINER (headermenu), edit_column);

  remove_column = gtk_menu_item_new_with_mnemonic ("Remove column");
  gtk_widget_show (remove_column);
  gtk_container_add (GTK_CONTAINER (headermenu), remove_column);

  g_signal_connect ((gpointer) add_column, "activate",
                    G_CALLBACK (on_add_column_activate),
                    NULL);
  g_signal_connect ((gpointer) edit_column, "activate",
                    G_CALLBACK (on_edit_column_activate),
                    NULL);
  g_signal_connect ((gpointer) remove_column, "activate",
                    G_CALLBACK (on_remove_column_activate),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
//  GLADE_HOOKUP_OBJECT_NO_REF (headermenu, headermenu, "headermenu");
//  GLADE_HOOKUP_OBJECT (headermenu, add_column, "add_column");
//  GLADE_HOOKUP_OBJECT (headermenu, edit_column, "edit_column");
//  GLADE_HOOKUP_OBJECT (headermenu, remove_column, "remove_column");

  return headermenu;
}
// debug function for gdk_draw_drawable
static inline void
draw_drawable (GdkDrawable *window, GdkGC *gc, GdkDrawable *drawable, int x1, int y1, int x2, int y2, int w, int h) {
//    printf ("dd: %p %p %p %d %d %d %d %d %d\n", window, gc, drawable, x1, y1, x2, y2, w, h);
    gdk_draw_drawable (window, gc, drawable, x1, y1, x2, y2, w, h);
}


extern GtkWidget *mainwin;
extern GtkStatusIcon *trayicon;

static GtkWidget *theme_treeview;

// orange on dark color scheme
float colo_dark_orange[COLO_COUNT][3] = {
    { 0x7f/255.f, 0x7f/255.f, 0x7f/255.f }, // cursor
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // odd
    { 0x21/255.f, 0x23/255.f, 0x1f/255.f }, // even
    { 0xaf/255.f, 0xa7/255.f, 0x9e/255.f }, // sel odd
    { 0xa7/255.f, 0x9f/255.f, 0x96/255.f }, // sel even
    { 0xf4/255.f, 0x7e/255.f, 0x46/255.f }, // text
    { 0,          0,          0          }, // sel text
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // seekbar back
    { 0xf4/255.f, 0x7e/255.f, 0x46/255.f }, // seekbar front
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // volumebar back
    { 0xf4/255.f, 0x7e/255.f, 0x46/255.f }, // volumebar front
    { 0xf4/255.f, 0x7e/255.f, 0x46/255.f }, // dragdrop marker
};

float colo_white_blue[COLO_COUNT][3] = {
    { 0x7f/255.f, 0x7f/255.f, 0x7f/255.f }, // cursor
    { 1,          1,          1          }, // odd
    { 0xea/255.f, 0xeb/255.f, 0xec/255.f }, // even
    { 0x24/255.f, 0x89/255.f, 0xb8/255.f }, // sel odd
    { 0x20/255.f, 0x85/255.f, 0xb4/255.f }, // sel even
    { 0,          0,          0          }, // text
    { 1,          1,          1          }, // sel text
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // seekbar back
    { 0x24/255.f, 0x89/255.f, 0xb8/255.f }, // seekbar front
    { 0x1d/255.f, 0x1f/255.f, 0x1b/255.f }, // volumebar back
    { 0x24/255.f, 0x89/255.f, 0xb8/255.f }, // volumebar front
    { 0x09/255.f, 0x22/255.f, 0x3a/255.f }, // dragdrop marker
};

#define MIN_COLUMN_WIDTH 16

// current color scheme
float colo_current[COLO_COUNT][3];

// playlist row height
int rowheight = -1;

// playlist scrolling during dragging
static int playlist_scroll_mode = 0; // 0=select, 1=dragndrop
static int playlist_scroll_pointer_y = -1;
static int playlist_scroll_direction = 0;
static int playlist_scroll_active = 0;
static struct timeval tm_prevscroll;
static float scroll_sleep_time = 0;

static uintptr_t play16_pixbuf;
static uintptr_t pause16_pixbuf;
static uintptr_t buffering16_pixbuf;

static GdkCursor* cursor_sz;
static GdkCursor* cursor_drag;
static int header_dragging = -1;
static int header_sizing = -1;
static int header_dragpt[2];

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
    gtkpl_header_draw (anim->pl);
    gtkpl_expose_header (anim->pl, 0, 0, anim->pl->header->allocation.width, anim->pl->header->allocation.height);
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

// that must be called before gtk_init
void
gtkpl_init (void) {
    //memcpy (colo_current, colo_system_gtk, sizeof (colo_current));
    //memcpy (colo_current, colo_dark_orange, sizeof (colo_current));
    play16_pixbuf = draw_load_pixbuf ("play_16.png");
    pause16_pixbuf = draw_load_pixbuf ("pause_16.png");
    buffering16_pixbuf = draw_load_pixbuf ("buffering_16.png");
    rowheight = draw_get_font_size () + 12;
    memcpy (colo_current, colo_white_blue, sizeof (colo_current));
    theme_treeview = gtk_tree_view_new ();
    GTK_WIDGET_UNSET_FLAGS (theme_treeview, GTK_CAN_FOCUS);
    gtk_widget_show (theme_treeview);
    GtkWidget *vbox1 = lookup_widget (mainwin, "vbox1");
    gtk_box_pack_start (GTK_BOX (vbox1), theme_treeview, FALSE, FALSE, 0);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (theme_treeview), TRUE);
}

void
gtkpl_free (DdbListview *pl) {
#if 0
    if (colhdr_anim.timeline) {
        timeline_free (colhdr_anim.timeline, 1);
        colhdr_anim.timeline = 0;
    }
#endif
//    g_object_unref (theme_treeview);
}

void
theme_set_cairo_source_rgb (cairo_t *cr, int col) {
    cairo_set_source_rgb (cr, colo_current[col][0], colo_current[col][1], colo_current[col][2]);
}

void
theme_set_fg_color (int col) {
    draw_set_fg_color (colo_current[col]);
}

void
theme_set_bg_color (int col) {
    draw_set_bg_color (colo_current[col]);
}

void
gtkpl_setup_scrollbar (DdbListview *ps) {
    GtkWidget *playlist = ps->playlist;
    int h = playlist->allocation.height / rowheight;
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
gtkpl_setup_hscrollbar (DdbListview *ps) {
    GtkWidget *playlist = ps->playlist;
    int w = playlist->allocation.width;
    int size = 0;
    DdbListviewColIter c;
    for (c = ps->binding->col_first (); c; c = ps->binding->col_next (c)) {
        size += ps->binding->col_get_width (c);
    }
    ps->totalwidth = size;
    if (ps->totalwidth < ps->playlist->allocation.width) {
        ps->totalwidth = ps->playlist->allocation.width;
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
gtkpl_redraw_pl_row_novis (DdbListview *ps, int row, DdbListviewIter it) {
    draw_begin ((uintptr_t)ps->backbuf);
    gtkpl_draw_pl_row_back (ps, row, it);
	if (it) {
        gtkpl_draw_pl_row (ps, row, it);
    }
    draw_end ();
}

void
gtkpl_redraw_pl_row (DdbListview *ps, int row, DdbListviewIter it) {
    if (row < ps->scrollpos || row >= ps->scrollpos+ps->nvisiblerows) {
        return;
    }
    int x, y, w, h;
    GtkWidget *widget = ps->playlist;
    x = 0;
    y = (row  - ps->scrollpos) * rowheight;
    w = widget->allocation.width;
    h = rowheight;

    gtkpl_redraw_pl_row_novis (ps, row, it);
	draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, x, y, x, y, w, h);
}

void
gtkpl_draw_pl_row_back (DdbListview *ps, int row, DdbListviewIter it) {
	// draw background
	GtkWidget *treeview = theme_treeview;
	if (treeview->style->depth == -1) {
        return; // drawing was called too early
    }
    GTK_OBJECT_FLAGS (treeview) |= GTK_HAS_FOCUS;
    int x = -ps->hscrollpos;
    int w = ps->totalwidth;
    // clear area -- workaround for New Wave theme
    if (ps->playlist->style->bg_gc[GTK_STATE_NORMAL]) {
        gdk_draw_rectangle (ps->backbuf, ps->playlist->style->bg_gc[GTK_STATE_NORMAL], TRUE, 0, row * rowheight - ps->scrollpos * rowheight, ps->playlist->allocation.width, rowheight);
    }
    gtk_paint_flat_box (treeview->style, ps->backbuf, (it && ps->binding->is_selected(it)) ? GTK_STATE_SELECTED : GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, treeview, (row & 1) ? "cell_even_ruled" : "cell_odd_ruled", x, row * rowheight - ps->scrollpos * rowheight, w, rowheight);
	if (row == ps->binding->cursor ()) {
        // not all gtk engines/themes render focus rectangle in treeviews
        // but we want it anyway
        gdk_draw_rectangle (ps->backbuf, treeview->style->fg_gc[GTK_STATE_NORMAL], FALSE, x, row * rowheight - ps->scrollpos * rowheight, w-1, rowheight-1);
        // gtkstyle focus drawing, for reference
//        gtk_paint_focus (treeview->style, ps->backbuf, (it && ps->binding->is_selected(it)) ? GTK_STATE_SELECTED : GTK_STATE_NORMAL, NULL, treeview, "treeview", x, row * rowheight - ps->scrollpos * rowheight, w, rowheight);
    }
}

void
gtkpl_draw_pl_row (DdbListview *ps, int row, DdbListviewIter it) {
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
            ps->binding->draw_column_data (ps->backbuf, it, row, c, x, row * rowheight - ps->scrollpos * rowheight, cw, rowheight);
        }
#if 0
        // FIXME: port
        if (it == deadbeef->streamer_get_playing_track () && c->id == DB_COLUMN_PLAYING) {
            int paused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED;
            int buffering = !deadbeef->streamer_ok_to_read (-1);
            uintptr_t pixbuf;
            if (paused) {
                pixbuf = pause16_pixbuf;
            }
            else if (!buffering) {
                pixbuf = play16_pixbuf;
            }
            else {
                pixbuf = buffering16_pixbuf;
            }
            draw_pixbuf ((uintptr_t)ps->backbuf, pixbuf, x + c->width/2 - 8 - ps->hscrollpos, (row - ps->scrollpos) * rowheight + rowheight/2 - 8, 0, 0, 16, 16);
        }
        else {
            char text[1024];
            deadbeef->pl_format_title (it, row, text, sizeof (text), c->id, c->format);

            if (c->align_right) {
                draw_text (x+5, row * rowheight - ps->scrollpos * rowheight + rowheight/2 - draw_get_font_size ()/2 - 2, c->width-10, 1, text);
            }
            else {
                draw_text (x + 5, row * rowheight - ps->scrollpos * rowheight + rowheight/2 - draw_get_font_size ()/2 - 2, c->width-10, 0, text);
            }
        }
#endif
        x += cw;
    }
}


void
gtkpl_draw_playlist (DdbListview *ps, int x, int y, int w, int h) {
    if (!ps->backbuf) {
        return;
    }
    draw_begin ((uintptr_t)ps->backbuf);
	int row;
	int row1;
	int row2;
	int row2_full;
	row1 = max (0, y / rowheight + ps->scrollpos);
	int cnt = ps->binding->count ();
	row2 = min (cnt, (y+h) / rowheight + ps->scrollpos + 1);
	row2_full = (y+h) / rowheight + ps->scrollpos + 1;
	// draw background
	DdbListviewIter it = ps->binding->get_for_idx (ps->scrollpos);
	DdbListviewIter it_copy = it;
	if (it_copy) {
        REF (it_copy);
    }
	for (row = row1; row < row2_full; row++) {
		gtkpl_draw_pl_row_back (ps, row, it);
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
        gtkpl_draw_pl_row (ps, row, it);
        DdbListviewIter next = PL_NEXT (it);
        UNREF (it);
        it = next;
	}
    UNREF (it);

    draw_end ();
}

void
gtkpl_expose (DdbListview *ps, int x, int y, int w, int h) {
    GtkWidget *widget = ps->playlist;
    if (widget->window) {
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, x, y, x, y, w, h);
    }
}

void
gtkpl_expose_header (DdbListview *ps, int x, int y, int w, int h) {
    GtkWidget *widget = ps->header;
	draw_drawable (widget->window, widget->style->black_gc, ps->backbuf_header, x, y, x, y, w, h);
}

void
gtkpl_select_single (DdbListview *ps, int sel) {
    int idx=0;
    DdbListviewIter it = ps->binding->head ();
    for (; it; idx++) {
        if (idx == sel) {
            if (!ps->binding->is_selected (it)) {
                ps->binding->select (it, 1);
                gtk_pl_redraw_item_everywhere (it);
            }
        }
        else if (ps->binding->is_selected (it)) {
            ps->binding->select (it, 0);
            gtk_pl_redraw_item_everywhere (it);
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
static int areaselect = 0;
static int areaselect_x = -1;
static int areaselect_y = -1;
static int areaselect_dx = -1;
static int areaselect_dy = -1;
static int dragwait = 0;
static int shift_sel_anchor = -1;

void
gtkpl_mouse1_pressed (DdbListview *ps, int state, int ex, int ey, double time) {
    // cursor must be set here, but selection must be handled in keyrelease
    int cnt = ps->binding->count ();
    if (cnt == 0) {
        return;
    }
    // remember mouse coords for doubleclick detection
    ps->lastpos[0] = ex;
    ps->lastpos[1] = ey;
    // select item
    int y = ey/rowheight + ps->scrollpos;
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
    shift_sel_anchor = ps->binding->cursor ();
    // handle multiple selection
    if (!(state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)))
    {
        DdbListviewIter it = ps->binding->get_for_idx (sel);
        if (!it || !ps->binding->is_selected (it)) {
            // reset selection, and set it to single item
            gtkpl_select_single (ps, sel);
            areaselect = 1;
            areaselect_x = ex;
            areaselect_y = ey;
            areaselect_dx = -1;
            areaselect_dy = -1;
            shift_sel_anchor = ps->binding->cursor ();
        }
        else {
            dragwait = 1;
            DdbListviewIter item = gtkpl_get_for_idx (ps, prev);
            gtkpl_redraw_pl_row (ps, prev, item);
            UNREF (item);
            int cursor = ps->binding->cursor ();
            if (cursor != prev) {
                DdbListviewIter item = ps->binding->get_for_idx (cursor);
                gtkpl_redraw_pl_row (ps, cursor, item);
                UNREF (item);
            }
        }
        UNREF (it);
    }
    else if (state & GDK_CONTROL_MASK) {
        // toggle selection
        if (y != -1) {
            DdbListviewIter it = gtkpl_get_for_idx (ps, y);
            if (it) {
                ps->binding->select (it, 1 - ps->binding->is_selected (it));
                gtk_pl_redraw_item_everywhere (it);
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
                    gtk_pl_redraw_item_everywhere (it);
                }
            }
            else {
                if (ps->binding->is_selected (it)) {
                    ps->binding->select (it, 0);
                    gtk_pl_redraw_item_everywhere (it);
                }
            }
            DdbListviewIter next = PL_NEXT (it);
            UNREF (it);
        }
    }
    cursor = ps->binding->cursor ();
    if (cursor != -1 && sel == -1) {
        DdbListviewIter it = gtkpl_get_for_idx (ps, cursor);
        gtkpl_redraw_pl_row (ps, cursor, it);
        UNREF (it);
    }
    if (prev != -1 && prev != cursor) {
        DdbListviewIter it = gtkpl_get_for_idx (ps, prev);
        gtkpl_redraw_pl_row (ps, prev, it);
        UNREF (it);
    }

}

void
gtkpl_mouse1_released (DdbListview *ps, int state, int ex, int ey, double time) {
    if (dragwait) {
        dragwait = 0;
        int y = ey/rowheight + ps->scrollpos;
        gtkpl_select_single (ps, y);
    }
    else if (areaselect) {
        playlist_scroll_direction = 0;
        playlist_scroll_pointer_y = -1;
        areaselect = 0;
    }
}

#if 0
void
gtkpl_draw_areasel (GtkWidget *widget, int x, int y) {
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
    gtkpl_cairo_destroy (cr);
}
#endif

static gboolean
gtkpl_scroll_playlist_cb (gpointer data) {
    DdbListview *ps = (DdbListview *)data;
    playlist_scroll_active = 1;
    struct timeval tm;
    gettimeofday (&tm, NULL);
    if (tm.tv_sec - tm_prevscroll.tv_sec + (tm.tv_usec - tm_prevscroll.tv_usec) / 1000000.0 < scroll_sleep_time) {
        return TRUE;
    }
    memcpy (&tm_prevscroll, &tm, sizeof (tm));
    if (playlist_scroll_pointer_y == -1) {
        playlist_scroll_active = 0;
        return FALSE;
    }
    if (playlist_scroll_direction == 0) {
        playlist_scroll_active = 0;
        return FALSE;
    }
    int sc = ps->scrollpos + playlist_scroll_direction;
    if (sc < 0) {
        playlist_scroll_active = 0;
        return FALSE;
    }
    if (sc >= ps->binding->count ()) {
        playlist_scroll_active = 0;
        return FALSE;
    }
    gtk_range_set_value (GTK_RANGE (ps->scrollbar), sc);
    if (playlist_scroll_mode == 0) {
        GdkEventMotion ev;
        ev.y = playlist_scroll_pointer_y;
        gtkpl_mousemove (ps, &ev);
    }
    else if (playlist_scroll_mode == 1) {
        gtkpl_track_dragdrop (ps, playlist_scroll_pointer_y);
    }
    scroll_sleep_time -= 0.1;
    if (scroll_sleep_time < 0.05) {
        scroll_sleep_time = 0.05;
    }
    return TRUE;
}

void
gtkpl_mousemove (DdbListview *ps, GdkEventMotion *event) {
    if (dragwait) {
        GtkWidget *widget = ps->playlist;
        if (gtk_drag_check_threshold (widget, ps->lastpos[0], event->x, ps->lastpos[1], event->y)) {
            dragwait = 0;
            GtkTargetEntry entry = {
                .target = "STRING",
                .flags = GTK_TARGET_SAME_WIDGET,
                .info = TARGET_SAMEWIDGET
            };
            GtkTargetList *lst = gtk_target_list_new (&entry, 1);
            gtk_drag_begin (widget, lst, GDK_ACTION_MOVE, TARGET_SAMEWIDGET, (GdkEvent *)event);
        }
    }
    else if (areaselect) {
        int y = event->y/rowheight + ps->scrollpos;
        //if (y != shift_sel_anchor)
        {
            int start = min (y, shift_sel_anchor);
            int end = max (y, shift_sel_anchor);
            int idx=0;
            DdbListviewIter it;
            for (it = ps->binding->head (); it; idx++) {
                if (idx >= start && idx <= end) {
                    if (!ps->binding->is_selected (it)) {
                        ps->binding->select (it, 1);
                        gtk_pl_redraw_item_everywhere (it);
                    }
                }
                else if (ps->binding->is_selected (it)) {
                    ps->binding->select (it, 0);
                    gtk_pl_redraw_item_everywhere (it);
                }
                DdbListviewIter next = PL_NEXT(it);
                UNREF (it);
                it = next;
            }
            UNREF (it);
        }

        if (event->y < 10) {
            playlist_scroll_mode = 0;
            playlist_scroll_pointer_y = event->y;
            playlist_scroll_direction = -1;
            // start scrolling up
            if (!playlist_scroll_active) {
                scroll_sleep_time = 0.2;
                gettimeofday (&tm_prevscroll, NULL);
                g_idle_add (gtkpl_scroll_playlist_cb, ps);
            }
        }
        else if (event->y > ps->playlist->allocation.height-10) {
            playlist_scroll_mode = 0;
            playlist_scroll_pointer_y = event->y;
            playlist_scroll_direction = 1;
            // start scrolling up
            if (!playlist_scroll_active) {
                scroll_sleep_time = 0.2;
                gettimeofday (&tm_prevscroll, NULL);
                g_idle_add (gtkpl_scroll_playlist_cb, ps);
            }
        }
        else {
            playlist_scroll_direction = 0;
            playlist_scroll_pointer_y = -1;
        }
        // debug only
        // gtkpl_draw_areasel (widget, event->x, event->y);
    }
}

void
gtkpl_handle_scroll_event (DdbListview *ps, int direction) {
    GtkWidget *range = ps->scrollbar;;
    GtkWidget *playlist = ps->playlist;
    int h = playlist->allocation.height / rowheight;
    int size = ps->binding->count ();
    if (h >= size) {
        size = 0;
    }
    if (size == 0) {
        return;
    }
    // pass event to scrollbar
    int newscroll = gtk_range_get_value (GTK_RANGE (range));
    if (direction == GDK_SCROLL_UP) {
        newscroll -= 2;
    }
    else if (direction == GDK_SCROLL_DOWN) {
        newscroll += 2;
    }
    gtk_range_set_value (GTK_RANGE (range), newscroll);
}

void
gtkpl_scroll (DdbListview *ps, int newscroll) {
    if (newscroll != ps->scrollpos) {
        GtkWidget *widget = ps->playlist;
        int di = newscroll - ps->scrollpos;
        int d = abs (di);
        if (d < ps->nvisiblerows) {
            if (di > 0) {
                draw_drawable (ps->backbuf, widget->style->black_gc, ps->backbuf, 0, d * rowheight, 0, 0, widget->allocation.width, widget->allocation.height-d * rowheight);
                int i;
                ps->scrollpos = newscroll;
                int start = ps->nvisiblerows-d-1;
                start = max (0, ps->nvisiblerows-d-1);
                for (i = start; i <= ps->nvisiblerows; i++) {
                    DdbListviewIter it = gtkpl_get_for_idx (ps, i+ps->scrollpos);
                    gtkpl_redraw_pl_row_novis (ps, i+ps->scrollpos, it);
                    UNREF (it);
                }
            }
            else {
                draw_drawable (ps->backbuf, widget->style->black_gc, ps->backbuf, 0, 0, 0, d*rowheight, widget->allocation.width, widget->allocation.height);
                ps->scrollpos = newscroll;
                int i;
                for (i = 0; i <= d+1; i++) {
                    DdbListviewIter it = gtkpl_get_for_idx (ps, i+ps->scrollpos);
                    gtkpl_redraw_pl_row_novis (ps, i+ps->scrollpos, it);
                    UNREF (it);
                }
            }
        }
        else {
            ps->scrollpos = newscroll;
            gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
        }
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
    }
}

void
gtkpl_hscroll (DdbListview *ps, int newscroll) {
    if (newscroll != ps->hscrollpos) {
        ps->hscrollpos = newscroll;
        GtkWidget *widget = ps->playlist;
        gtkpl_header_draw (ps);
        gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
    }
}

void
gtkpl_songchanged (DdbListview *ps, int from, int to) {
    if (!dragwait && to != -1) {
        if (deadbeef->conf_get_int ("playlist.scroll.followplayback", 0)) {
            if (to < ps->scrollpos || to >= ps->scrollpos + ps->nvisiblefullrows) {
                gtk_range_set_value (GTK_RANGE (ps->scrollbar), to - ps->nvisiblerows/2);
            }
        }
        if (deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0)) {
            gtkpl_set_cursor (PL_MAIN, to);
        }
    }

    if (from >= 0) {
        DdbListviewIter it = gtkpl_get_for_idx (ps, from);
        gtkpl_redraw_pl_row (ps, from, it);
        UNREF (it);
    }
    if (to >= 0) {
        DdbListviewIter it = gtkpl_get_for_idx (ps, to);
        gtkpl_redraw_pl_row (ps, to, it);
        UNREF (it);
    }
}

void
main_refresh (void) {
    if (mainwin && GTK_WIDGET_VISIBLE (mainwin)) {
        DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
        ddb_listview_refresh (pl, DDB_REFRESH_VSCROLL | DDB_REFRESH_LIST | DDB_EXPOSE_LIST);
    }
}

int
gtkpl_keypress (DdbListview *ps, int keyval, int state) {
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
        cursor = deadbeef->pl_delete_selected ();
        if (cursor >= ps->binding->count ()) {
            cursor = ps->binding->count ()-1;
        }
        main_refresh ();
        search_refresh ();
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
            int start = min (cursor, shift_sel_anchor);
            int end = max (cursor, shift_sel_anchor);
            int idx=0;
            DdbListviewIter it;
            for (it = ps->binding->head (); it; idx++) {
                if (idx >= start && idx <= end) {
                    ps->binding->select (it, 1);
                    gtk_pl_redraw_item_everywhere (it);
                }
                else if (ps->binding->is_selected (it))
                {
                    ps->binding->select (it, 0);
                    gtk_pl_redraw_item_everywhere (it);
                }
                DdbListviewIter next = PL_NEXT(it);
                UNREF (it);
                it = next;
            }
            UNREF (it);
        }
    }
    else {
        shift_sel_anchor = cursor;
        gtkpl_set_cursor (ps->iterator, cursor);
    }
    return 1;
}

static int drag_motion_y = -1;

void
gtkpl_track_dragdrop (DdbListview *ps, int y) {
    GtkWidget *widget = ps->playlist;
    if (drag_motion_y != -1) {
        // erase previous track
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, drag_motion_y * rowheight-3, 0, drag_motion_y * rowheight-3, widget->allocation.width, 7);

    }
    if (y == -1) {
        drag_motion_y = -1;
        return;
    }
    draw_begin ((uintptr_t)widget->window);
    drag_motion_y = y / rowheight;

    theme_set_fg_color (COLO_DRAGDROP_MARKER);
    draw_rect (0, drag_motion_y * rowheight-1, widget->allocation.width, 3, 1);
    draw_rect (0, drag_motion_y * rowheight-3, 3, 7, 1);
    draw_rect (widget->allocation.width-3, drag_motion_y * rowheight-3, 3, 7, 1);
    draw_end ();
    if (y < 10) {
        playlist_scroll_pointer_y = y;
        playlist_scroll_direction = -1;
        playlist_scroll_mode = 1;
        // start scrolling up
        if (!playlist_scroll_active) {
            scroll_sleep_time = 0.2;
            gettimeofday (&tm_prevscroll, NULL);
            g_idle_add (gtkpl_scroll_playlist_cb, ps);
        }
    }
    else if (y > ps->playlist->allocation.height-10) {
        playlist_scroll_mode = 1;
        playlist_scroll_pointer_y = y;
        playlist_scroll_direction = 1;
        // start scrolling up
        if (!playlist_scroll_active) {
            scroll_sleep_time = 0.2;
            gettimeofday (&tm_prevscroll, NULL);
            g_idle_add (gtkpl_scroll_playlist_cb, ps);
        }
    }
    else {
        playlist_scroll_direction = 0;
        playlist_scroll_pointer_y = -1;
    }
}

void
on_playlist_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    // invalidate entire cache - slow, but rare
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkpl_expose (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    playlist_scroll_direction = 0;
    playlist_scroll_pointer_y = -1;
}

void
strcopy_special (char *dest, const char *src, int len) {
    while (len > 0) {
        if (*src == '%' && len >= 3) {
            int charcode = 0;
            int byte;
            byte = tolower (src[2]);
            if (byte >= '0' && byte <= '9') {
                charcode = byte - '0';
            }
            else if (byte >= 'a' && byte <= 'f') {
                charcode = byte - 'a' + 10;
            }
            else {
                charcode = '?';
            }
            if (charcode != '?') {
                byte = tolower (src[1]);
                if (byte >= '0' && byte <= '9') {
                    charcode |= (byte - '0') << 4;
                }
                else if (byte >= 'a' && byte <= 'f') {
                    charcode |= (byte - 'a' + 10) << 4;
                }
                else {
                    charcode = '?';
                }
            }
            *dest = charcode;
            dest++;
            src += 3;
            len -= 3;
            continue;
        }
        else {
            *dest++ = *src++;
            len--;
        }
    }
    *dest = 0;
}

static gboolean
set_progress_text_idle (gpointer data) {
    const char *text = (const char *)data;
    progress_settext (text);
    return FALSE;
}

int
gtkpl_add_file_info_cb (DdbListviewIter it, void *data) {
    if (progress_is_aborted ()) {
        return -1;
    }
    g_idle_add (set_progress_text_idle, it->fname);
    return 0;
}

static gboolean
progress_show_idle (gpointer data) {
    progress_show ();
    return FALSE;
}

static gboolean
progress_hide_idle (gpointer data) {
    progress_hide ();
    playlist_refresh ();
    return FALSE;
}

void
gtkpl_add_fm_dropped_files (char *ptr, int length, int drop_y) {
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    g_idle_add (progress_show_idle, NULL);

//    int drop_row = drop_y / rowheight + ddb_get_vscroll_pos (pl);
    DdbListviewIter iter = ddb_listview_get_iter_from_coord (0, drop_y);
    drop_before = ((DdbListviewIter )iter);
    int drop_row = deadbeef->pl_get_idx_of (drop_before);
//    DdbListviewIter drop_before = deadbeef->pl_get_for_idx_and_iter (drop_row, PL_MAIN);
    DdbListviewIter after = NULL;
    if (drop_before) {
        after = PL_PREV (drop_before, PL_MAIN);
        UNREF (drop_before);
        drop_before = NULL;
    }
    else {
        after = PL_TAIL (ps->iterator);
    }
    const uint8_t *p = (const uint8_t*)ptr;
    while (*p) {
        const uint8_t *pe = p;
        while (*pe && *pe > ' ') {
            pe++;
        }
        if (pe - p < 4096 && pe - p > 7) {
            char fname[(int)(pe - p)];
            strcopy_special (fname, p, pe-p);
            //strncpy (fname, p, pe - p);
            //fname[pe - p] = 0;
            int abort = 0;
            DdbListviewIter inserted = deadbeef->pl_insert_dir (after, fname, &abort, gtkpl_add_file_info_cb, NULL);
            if (!inserted && !abort) {
                inserted = deadbeef->pl_insert_file (after, fname, &abort, gtkpl_add_file_info_cb, NULL);
            }
            if (inserted) {
                if (after) {
                    UNREF (after);
                }
                after = inserted;
            }
        }
        p = pe;
        // skip whitespace
        while (*p && *p <= ' ') {
            p++;
        }
    }
    free (ptr);

    if (after) {
        UNREF (after);
    }
    g_idle_add (progress_hide_idle, NULL);
}

void
gtkpl_header_draw (DdbListview *ps) {
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
    for (c = ps->columns; c; c = c->next, idx++) {
        w = c->width;
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
        if (header_dragging < 0 || idx != header_dragging) {
            if (xx >= widget->allocation.width) {
                continue;
            }
            int arrow_sz = 10;
            if (w > 0) {
                gtk_paint_vline (widget->style, ps->backbuf_header, GTK_STATE_NORMAL, NULL, NULL, NULL, 2, h-4, xx+w - 2);
                GdkColor *gdkfg = &widget->style->fg[0];
                float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
                draw_set_fg_color (fg);
                int w = c->width-10;
                if (c->sort_order) {
                    w -= arrow_sz;
                    if (w < 0) {
                        w = 0;
                    }
                }
                draw_text (xx + 5, h/2-draw_get_font_size()/2, w, 0, c->title);
            }
            if (c->sort_order != 0) {
                int dir = c->sort_order == 1 ? GTK_ARROW_DOWN : GTK_ARROW_UP;
                gtk_paint_arrow (widget->style, ps->backbuf_header, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, dir, TRUE, xx + c->width-arrow_sz-5, widget->allocation.height/2-arrow_sz/2, arrow_sz, arrow_sz);
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
        for (c = ps->columns; c; c = c->next, idx++) {
            w = c->width;
            if (idx == header_dragging) {
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
                x = c->movepos;
                if (x >= widget->allocation.width) {
                    break;
                }
                if (w > 0) {
                    gtk_paint_box (widget->style, ps->backbuf_header, GTK_STATE_SELECTED, GTK_SHADOW_OUT, NULL, widget, "button", x, 0, w, h);
                    GdkColor *gdkfg = &widget->style->fg[GTK_STATE_SELECTED];
                    float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
                    draw_set_fg_color (fg);
                    draw_text (x + 5, h/2-draw_get_font_size()/2, c->width-10, 0, c->title);
                }
                break;
            }
            x += w;
        }
    }
    draw_end ();
}

gboolean
on_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    gtkpl_header_draw (ps);
    gtkpl_expose_header (ps, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}


gboolean
on_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    if (ps->backbuf_header) {
        g_object_unref (ps->backbuf_header);
        ps->backbuf_header = NULL;
    }
    ps->backbuf_header = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);
    gtkpl_header_draw (ps);
    return FALSE;
}


void
on_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data)
{
    // create cursor for sizing headers
    int h = draw_get_font_size ();
    gtk_widget_set_size_request (widget, -1, h + 10);
    cursor_sz = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    cursor_drag = gdk_cursor_new (GDK_FLEUR);
}

static float last_header_motion_ev = -1; //is it subject to remove?
static int prev_header_x = -1;
static int header_prepare = 0;

gboolean
on_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
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


    if ((ev_state & GDK_BUTTON1_MASK) && header_prepare) {
        if (gtk_drag_check_threshold (widget, ev_x, prev_header_x, 0, 0)) {
            header_prepare = 0;
        }
    }
    if (!header_prepare && header_dragging >= 0) {
        gdk_window_set_cursor (widget->window, cursor_drag);
        DdbListviewColIter c;
        int i;
        for (i = 0, c = ps->columns; i < header_dragging && c; c = c->next, i++);
        c->movepos = ev_x - header_dragpt[0];

        // find closest column to the left
        int inspos = -1;
        DdbListviewColIter cc;
        int x = 0;
        int idx = 0;
        int x1 = -1, x2 = -1;
        for (cc = ps->columns; cc; cc = cc->next, idx++) {
            if (x < c->movepos && x + c->width > c->movepos) {
                inspos = idx;
                x1 = x;
            }
            else if (idx == header_dragging) {
                x2 = x;
            }
            x += cc->width;
        }
        if (inspos >= 0 && inspos != header_dragging) {
            // remove c from list
            if (c == ps->columns) {
                ps->columns = c->next;
            }
            else {
                for (cc = ps->columns; cc; cc = cc->next) {
                    if (cc->next == c) {
                        cc->next = c->next;
                    }
                }
            }
            c->next = NULL;
            // reinsert c at position inspos update header_dragging to new idx
            header_dragging = inspos;
            if (inspos == 0) {
                c->next = ps->columns;
                ps->columns = c;
            }
            else {
                idx = 0;
                DdbListviewColIter prev = NULL;
                for (cc = ps->columns; cc; cc = cc->next, idx++, prev = cc) {
                    if (idx+1 == inspos) {
                        DdbListviewColIter next = cc->next;
                        cc->next = c;
                        c->next = next;
                        break;
                    }
                }
            }
//            colhdr_anim_swap (ps, c1, c2, x1, x2);
            // force redraw of everything
//            gtkpl_setup_hscrollbar (ps);
            gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
            gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
            gtkpl_column_update_config (ps, c, i);
        }
        else {
            // only redraw that if not animating
            gtkpl_header_draw (ps);
            gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        }
    }
    else if (header_sizing >= 0) {
        last_header_motion_ev = event->time;
        prev_header_x = ev_x;
        gdk_window_set_cursor (widget->window, cursor_sz);
        // get column start pos
        int x = -ps->hscrollpos;
        int i = 0;
        DdbListviewColIter c;
        for (c = ps->columns; c && i < header_sizing; c = c->next, i++) {
            x += c->width;
        }

        int newx = ev_x > x + MIN_COLUMN_WIDTH ? ev_x : x + MIN_COLUMN_WIDTH;
        c->width = newx - x;
        gtkpl_setup_hscrollbar (ps);
        gtkpl_header_draw (ps);
        gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
        gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
        gtkpl_column_update_config (ps, c, i);
    }
    else {
        int x = -ps->hscrollpos;
        DdbListviewColIter c;
        for (c = ps->columns; c; c = c->next) {
            int w = c->width;
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

gtkpl_column_t*
gtkpl_get_column_for_click (DdbListview *pl, int click_x) {
    int x = -pl->hscrollpos;
    DdbListviewColIter c;
    for (c = pl->columns; c; c = c->next) {
        int w = c->width;
        if (click_x >= x && click_x < x + w) {
            return c;
        }
        x += w;
    }
    return NULL;
}

gboolean
on_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    ps->active_column = gtkpl_get_column_for_click (ps, event->x);
    if (event->button == 1) {
        // start sizing/dragging
        header_dragging = -1;
        header_sizing = -1;
        header_dragpt[0] = event->x;
        header_dragpt[1] = event->y;
        int x = -ps->hscrollpos;
        int i = 0;
        DdbListviewColIter c;
        for (c = ps->columns; c; c = c->next, i++) {
            int w = c->width;
            if (event->x >= x + w - 2 && event->x <= x + w) {
                header_sizing = i;
                header_dragging = -1;
                break;
            }
            else if (event->x > x + 2 && event->x < x + w - 2) {
                // prepare to drag or sort
                header_dragpt[0] = event->x - x;
                header_prepare = 1;
                header_dragging = i;
                header_sizing = -1;
                prev_header_x = event->x;
                break;
            }
            x += w;
        }
    }
    else if (event->button == 3) {
        GtkWidget *menu = create_headermenu ();
        last_playlist = ps;
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, widget, 3, gtk_get_current_event_time());
    }
    prev_header_x = -1;
    last_header_motion_ev = -1;
    return FALSE;
}

gboolean
on_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    if (event->button == 1) {
        if (header_prepare) {
            header_sizing = -1;
            header_dragging = -1;
            header_prepare = 0;
            // sort
            DdbListviewColIter c;
            int i = 0;
            int x = -ps->hscrollpos;
            int sorted = 0;
            for (c = ps->columns; c; c = c->next, i++) {
                int w = c->width;
                if (event->x > x + 2 && event->x < x + w - 2) {
                    if (!c->sort_order) {
                        c->sort_order = 1;
                    }
                    else if (c->sort_order == 1) {
                        c->sort_order = 2;
                    }
                    else if (c->sort_order == 2) {
                        c->sort_order = 1;
                    }
                    deadbeef->pl_sort (ps == &main_playlist ? PL_MAIN : PL_SEARCH, c->id, c->format, c->sort_order-1);
                    sorted = 1;
                }
                else {
                    c->sort_order = 0;
                }
                x += w;
            }
            playlist_refresh ();
            gtkpl_header_draw (ps);
            gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        }
        else {
            header_sizing = -1;
            int x = 0;
            DdbListviewColIter c;
            for (c = ps->columns; c; c = c->next) {
                int w = c->width;
                if (event->x >= x + w - 2 && event->x <= x + w) {
                    gdk_window_set_cursor (widget->window, cursor_sz);
                    break;
                }
                else {
                    gdk_window_set_cursor (widget->window, NULL);
                }
                x += w;
            }
            if (header_dragging >= 0) {
                header_dragging = -1;
                gtkpl_setup_hscrollbar (ps);
                gtkpl_header_draw (ps);
                gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
                gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
                gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
                gtkpl_column_rewrite_config (ps);
            }
        }
    }
    return FALSE;
}

void
gtkpl_add_dir (DdbListview *ps, char *folder) {
    g_idle_add (progress_show_idle, NULL);
    deadbeef->pl_add_dir (folder, gtkpl_add_file_info_cb, NULL);
    g_free (folder);
    g_idle_add (progress_hide_idle, NULL);
}

static void
gtkpl_adddir_cb (gpointer data, gpointer userdata) {
    deadbeef->pl_add_dir (data, gtkpl_add_file_info_cb, userdata);
    g_free (data);
}

void
gtkpl_add_dirs (DdbListview *ps, GSList *lst) {
    g_idle_add (progress_show_idle, NULL);
    g_slist_foreach(lst, gtkpl_adddir_cb, NULL);
    g_slist_free (lst);
    g_idle_add (progress_hide_idle, NULL);
}

static void
gtkpl_addfile_cb (gpointer data, gpointer userdata) {
    deadbeef->pl_add_file (data, gtkpl_add_file_info_cb, userdata);
    g_free (data);
}

void
gtkpl_add_files (GSList *lst) {
    g_idle_add (progress_show_idle, NULL);
    g_slist_foreach(lst, gtkpl_addfile_cb, NULL);
    g_slist_free (lst);
    g_idle_add (progress_hide_idle, NULL);
}

int
gtkpl_get_idx_of (DdbListview *ps, DdbListviewIter it) {
    DdbListviewIter c = PL_HEAD(ps->iterator);
    int idx = 0;
    while (c && c != it) {
        DdbListviewIter next = PL_NEXT(c, ps->iterator); 
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
    DdbListviewIter it = PL_HEAD(ps->iterator);
    while (idx--) {
        if (!it) {
            return NULL;
        }
        DdbListviewIter next = PL_NEXT(it, ps->iterator);
        UNREF (it);
        it = next;
    }
    return it;
}

void
playlist_refresh (void) {
    extern DdbListview main_playlist;
    DdbListview *ps = &main_playlist;
    gtkpl_setup_scrollbar (ps);
    GtkWidget *widget = ps->playlist;
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkpl_expose (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    search_refresh ();
}

void
set_tray_tooltip (const char *text) {
#if (GTK_MINOR_VERSION < 16)
        gtk_status_icon_set_tooltip (trayicon, text);
#else
        gtk_status_icon_set_tooltip_text (trayicon, text);
#endif
}

struct fromto_t {
    int from;
    int to;
};

#if HAVE_NOTIFY
static NotifyNotification* notification;
#endif

static gboolean
update_win_title_idle (gpointer data) {
    struct fromto_t *ft = (struct fromto_t *)data;
    int from = ft->from;
    int to = ft->to;
    free (ft);

    // show notification
#if HAVE_NOTIFY
    if (to != -1 && deadbeef->conf_get_int ("gtkui.notify.enable", 0)) {
        DdbListviewIter track = deadbeef->pl_get_for_idx (to);
        if (track) {
            char cmd [1024];
            deadbeef->pl_format_title (track, -1, cmd, sizeof (cmd), -1, deadbeef->conf_get_str ("gtkui.notify.format", NOTIFY_DEFAULT_FORMAT));
            if (notify_is_initted ()) {
                if (notification) {
                    notify_notification_close (notification, NULL);
                }
                notification = notify_notification_new ("DeaDBeeF", cmd, NULL, NULL);
                if (notification) {
                    notify_notification_set_timeout (notification, NOTIFY_EXPIRES_DEFAULT);
                    notify_notification_show (notification, NULL);
                }
            }
            UNREF (track);
        }
    }
#endif

    // update window title
    if (from >= 0 || to >= 0) {
        if (to >= 0) {
            DdbListviewIter it = deadbeef->pl_get_for_idx (to);
            if (it) { // it might have been deleted after event was sent
                gtkpl_current_track_changed (it);
                UNREF (it);
            }
        }
        else {
            gtk_window_set_title (GTK_WINDOW (mainwin), "DeaDBeeF");
            set_tray_tooltip ("DeaDBeeF");
        }
    }
    // update playlist view
    gtkpl_songchanged (&main_playlist, from, to);
    return FALSE;
}

static gboolean
redraw_seekbar_cb (gpointer nothing) {
    void seekbar_draw (GtkWidget *widget);
    void seekbar_expose (GtkWidget *widget, int x, int y, int w, int h);
    GtkWidget *widget = lookup_widget (mainwin, "seekbar");
    seekbar_draw (widget);
    seekbar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}

void
redraw_queued_tracks (DdbListview *pl) {
    DdbListviewIter it = deadbeef->pl_get_for_idx_and_iter (pl->scrollpos, pl->iterator);
    int i = pl->scrollpos;
    while (it && i < pl->scrollpos + pl->nvisiblerows) {
        if (deadbeef->pl_playqueue_test (it) != -1) {
            gtkpl_redraw_pl_row (pl, i, it);
        }
        DdbListviewIter next = PL_NEXT (it, pl->iterator);
        UNREF (it);
        it = next;
        i++;
    }
    UNREF (it);
}

static gboolean
redraw_queued_tracks_cb (gpointer nothing) {
    redraw_queued_tracks (&main_playlist);
    redraw_queued_tracks (&search_playlist);
    return FALSE;
}

void
gtkpl_songchanged_wrapper (int from, int to) {
    struct fromto_t *ft = malloc (sizeof (struct fromto_t));
    ft->from = from;
    ft->to = to;
    g_idle_add (update_win_title_idle, ft);
    if (ft->to == -1) {
        // redraw seekbar
        g_idle_add (redraw_seekbar_cb, NULL);
    }
    g_idle_add (redraw_queued_tracks_cb, NULL);
}

void
gtk_pl_redraw_item_everywhere (DdbListviewIter it) {
    DdbListview *pl = &search_playlist;
    int idx = gtkpl_get_idx_of (pl, it);
    int minvis = pl->scrollpos;
    int maxvis = pl->scrollpos + pl->nvisiblerows-1;
    if (idx >= minvis && idx <= maxvis) {
        gtkpl_redraw_pl_row (pl, idx, it);
    }
    pl = &main_playlist;
    idx = gtkpl_get_idx_of (pl, it);
    minvis = pl->scrollpos;
    maxvis = pl->scrollpos + pl->nvisiblerows-1;
    if (idx >= minvis && idx <= maxvis) {
        gtkpl_redraw_pl_row (pl, idx, it);
    }
}

struct set_cursor_t {
    int iter;
    int cursor;
    int prev;
    DdbListview *pl;
};

static gboolean
gtkpl_set_cursor_cb (gpointer data) {
    struct set_cursor_t *sc = (struct set_cursor_t *)data;
    deadbeef->pl_set_cursor (sc->iter, sc->cursor);
    gtkpl_select_single (sc->pl, sc->cursor);
    DdbListviewIter it;
    int minvis = sc->pl->scrollpos;
    int maxvis = sc->pl->scrollpos + sc->pl->nvisiblerows-1;
    if (sc->prev >= minvis && sc->prev <= maxvis) {
        it = deadbeef->pl_get_for_idx_and_iter (sc->prev, sc->iter);
        gtkpl_redraw_pl_row (sc->pl, sc->prev, it);
        UNREF (it);
    }
    if (sc->cursor >= minvis && sc->cursor <= maxvis) {
        it = deadbeef->pl_get_for_idx_and_iter (sc->cursor, sc->iter);
        gtkpl_redraw_pl_row (sc->pl, sc->cursor, it);
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
gtkpl_set_cursor (int iter, int cursor) {
    DdbListview *pl = (iter == PL_MAIN) ? &main_playlist : &search_playlist;
    int prev = deadbeef->pl_get_cursor (iter);
    struct set_cursor_t *data = malloc (sizeof (struct set_cursor_t));
    data->prev = prev;
    data->iter = iter;
    data->cursor = cursor;
    data->pl = pl;
    g_idle_add (gtkpl_set_cursor_cb, data);
}

gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (widget);
    if (event->button == 1) {
        gtkpl_mouse1_pressed (ps, event->state, event->x, event->y, event->time);
    }
    else if (event->button == 3) {
        // get item under cursor
        int y = event->y / rowheight + ps->scrollpos;
        if (y < 0 || y >= ps->binding->count ()) {
            y = -1;
        }
        DdbListviewIter it = deadbeef->pl_get_for_idx_and_iter (y, ps->iterator);
        if (!it) {
            // clicked empty space -- deselect everything and show insensitive menu
            ps->binding->set_cursor (-1);
            it = ps->binding->first ();
            while (it) {
                ps->binding->select (it, 0);
                it = PL_NEXT (it);
            }
            playlist_refresh ();
            // no menu
        }
        else {
            if (!ps->binding->is_selected (it)) {
                // item is unselected -- reset selection and select this
                DdbListviewIter it2 = ps->binding->first ();
                while (it2) {
                    if (ps->binding->is_selected (it2) && it2 != it) {
                        ps->binding->select (it2, 0);
                    }
                    else if (it2 == it) {
                        deadbeef->pl_set_cursor (ps->iterator, y);
                        ps->binding->select (it2, 1);
                    }
                    it2 = PL_NEXT (it2, ps->iterator);
                }
                playlist_refresh ();
            }
            else {
                // something is selected; move cursor but keep selection
                deadbeef->pl_set_cursor (ps->iterator, y);
                playlist_refresh ();
            }
            {
                int inqueue = deadbeef->pl_playqueue_test (it);
                GtkWidget *playlist_menu;
                GtkWidget *add_to_playback_queue1;
                GtkWidget *remove_from_playback_queue1;
                GtkWidget *separator9;
                GtkWidget *remove2;
                GtkWidget *separator8;
                GtkWidget *properties1;

                playlist_menu = gtk_menu_new ();
                add_to_playback_queue1 = gtk_menu_item_new_with_mnemonic ("Add to playback queue");
                gtk_widget_show (add_to_playback_queue1);
                gtk_container_add (GTK_CONTAINER (playlist_menu), add_to_playback_queue1);
                gtk_object_set_data (GTK_OBJECT (add_to_playback_queue1), "ps", ps);

                remove_from_playback_queue1 = gtk_menu_item_new_with_mnemonic ("Remove from playback queue");
                if (inqueue == -1) {
                    gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
                }
                gtk_widget_show (remove_from_playback_queue1);
                gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_playback_queue1);
                gtk_object_set_data (GTK_OBJECT (remove_from_playback_queue1), "ps", ps);

                separator9 = gtk_separator_menu_item_new ();
                gtk_widget_show (separator9);
                gtk_container_add (GTK_CONTAINER (playlist_menu), separator9);
                gtk_widget_set_sensitive (separator9, FALSE);

                remove2 = gtk_menu_item_new_with_mnemonic ("Remove");
                gtk_widget_show (remove2);
                gtk_container_add (GTK_CONTAINER (playlist_menu), remove2);
                gtk_object_set_data (GTK_OBJECT (remove2), "ps", ps);

                separator8 = gtk_separator_menu_item_new ();
                gtk_widget_show (separator8);
                gtk_container_add (GTK_CONTAINER (playlist_menu), separator8);
                gtk_widget_set_sensitive (separator8, FALSE);

                properties1 = gtk_menu_item_new_with_mnemonic ("Properties");
                gtk_widget_show (properties1);
                gtk_container_add (GTK_CONTAINER (playlist_menu), properties1);
                gtk_object_set_data (GTK_OBJECT (properties1), "ps", ps);

                g_signal_connect ((gpointer) add_to_playback_queue1, "activate",
                        G_CALLBACK (on_add_to_playback_queue1_activate),
                        NULL);
                g_signal_connect ((gpointer) remove_from_playback_queue1, "activate",
                        G_CALLBACK (on_remove_from_playback_queue1_activate),
                        NULL);
                g_signal_connect ((gpointer) remove2, "activate",
                        G_CALLBACK (on_remove2_activate),
                        NULL);
                g_signal_connect ((gpointer) properties1, "activate",
                        G_CALLBACK (on_properties1_activate),
                        NULL);
                gtk_menu_popup (GTK_MENU (playlist_menu), NULL, NULL, NULL, widget, 0, gtk_get_current_event_time());
            }
        }
    }
    return FALSE;
}

gboolean
on_playlist_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (widget);
    if (event->button == 1) {
        gtkpl_mouse1_released (ps, event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
on_playlist_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (widget);
    gtkpl_mousemove (ps, event);
    return FALSE;
}


