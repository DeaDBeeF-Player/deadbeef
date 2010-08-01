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
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <sys/time.h>
#include "ddblistview.h"
#include "drawing.h"
#include "gtkui.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define DEFAULT_GROUP_TITLE_HEIGHT 30
#define SCROLL_STEP 20
#define AUTOSCROLL_UPDATE_FREQ 0.01f

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define PL_NEXT(it) (ps->binding->next (it))
#define PL_PREV(it) (ps->binding->prev (it))
//#define REF(it) {if (it) ps->binding->ref (it);}
#define UNREF(it) {if (it) ps->binding->unref(it);}

// HACK!!
extern GtkWidget *theme_treeview;
extern GtkWidget *theme_button;

G_DEFINE_TYPE (DdbListview, ddb_listview, GTK_TYPE_TABLE);

struct _DdbListviewColumn {
    char *title;
    int width;
    int minheight;
    struct _DdbListviewColumn *next;
    void *user_data;
    unsigned align_right : 1;
    unsigned sort_order : 2; // 0=none, 1=asc, 2=desc
};
typedef struct _DdbListviewColumn DdbListviewColumn;

struct _DdbListviewGroup {
    DdbListviewIter head;
    int32_t height;
    int32_t num_items;
    struct _DdbListviewGroup *next;
};
typedef struct _DdbListviewGroup DdbListviewGroup;

static void ddb_listview_class_init(DdbListviewClass *klass);
static void ddb_listview_init(DdbListview *listview);
//static void ddb_listview_size_request(GtkWidget *widget, GtkRequisition *requisition);
//static void ddb_listview_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
//static void ddb_listview_realize(GtkWidget *widget);
//static void ddb_listview_paint(GtkWidget *widget);
static void ddb_listview_destroy(GtkObject *object);

// fwd decls
void
ddb_listview_free_groups (DdbListview *listview);
static inline void
draw_drawable (GdkDrawable *window, GdkGC *gc, GdkDrawable *drawable, int x1, int y1, int x2, int y2, int w, int h);

////// list functions ////
void
ddb_listview_list_render (DdbListview *ps, int x, int y, int w, int h);
void
ddb_listview_list_expose (DdbListview *ps, int x, int y, int w, int h);
void
ddb_listview_list_render_row_background (DdbListview *ps, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h);
void
ddb_listview_list_render_row_foreground (DdbListview *ps, DdbListviewIter it, DdbListviewIter group_it, int even, int cursor, int group_y, int x, int y, int w, int h);
void
ddb_listview_list_render_row (DdbListview *ps, int row, DdbListviewIter it, int expose);
void
ddb_listview_list_track_dragdrop (DdbListview *ps, int y);
int
ddb_listview_dragdrop_get_row_from_coord (DdbListview *listview, int y);
void
ddb_listview_list_mousemove (DdbListview *ps, GdkEventMotion *event, int x, int y);
void
ddb_listview_list_setup_vscroll (DdbListview *ps);
void
ddb_listview_list_setup_hscroll (DdbListview *ps);
void
ddb_listview_list_set_hscroll (DdbListview *ps, int newscroll);
void
ddb_listview_set_cursor (DdbListview *pl, int cursor);
int
ddb_listview_get_row_pos (DdbListview *listview, int pos);

////// header functions ////
void
ddb_listview_header_render (DdbListview *ps);
void
ddb_listview_header_expose (DdbListview *ps, int x, int y, int w, int h);

////// column management functions ////
void
ddb_listview_column_move (DdbListview *listview, DdbListviewColumn *which, int inspos);
void
ddb_listview_column_free (DdbListview *listview, DdbListviewColumn *c);


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
  GtkTableClass *widget_class = (GtkTableClass *) class;
  GtkObjectClass *object_class = (GtkObjectClass *) class;
  // FIXME!!!
  object_class->destroy = ddb_listview_destroy;
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
//    listview->areaselect_x = -1;
    listview->areaselect_y = -1;
//    listview->areaselect_dx = -1;
//    listview->areaselect_dy = -1;
    listview->dragwait = 0;
    listview->drag_source_playlist = -1;
    listview->shift_sel_anchor = -1;

    listview->header_dragging = -1;
    listview->header_sizing = -1;
    listview->header_dragpt[0] = 0;
    listview->header_dragpt[1] = 0;
    listview->last_header_motion_ev = -1; //is it subject to remove?
    listview->prev_header_x = -1;
    listview->header_prepare = 0;

    listview->columns = NULL;
    listview->groups = NULL;

    listview->block_redraw_on_scroll = 0;
    listview->grouptitle_height = DEFAULT_GROUP_TITLE_HEIGHT;

    listview->cursor_sz = NULL;
    listview->cursor_drag = NULL;

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


    g_object_set_data (G_OBJECT (listview->list), "owner", listview);
    g_object_set_data (G_OBJECT (listview->header), "owner", listview);
    g_object_set_data (G_OBJECT (listview->scrollbar), "owner", listview);
    g_object_set_data (G_OBJECT (listview->hscrollbar), "owner", listview);

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
//            G_CALLBACK (on_list_drag_begin),
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
   return g_object_newv (ddb_listview_get_type(), 0, NULL);//GTK_WIDGET(gtk_type_new(ddb_listview_get_type()));
}

static void
ddb_listview_destroy(GtkObject *object)
{
  DdbListview *listview;

  g_return_if_fail(object != NULL);
  g_return_if_fail(DDB_IS_LISTVIEW(object));

  listview = DDB_LISTVIEW(object);

  ddb_listview_free_groups (listview);

  while (listview->columns) {
      DdbListviewColumn *next = listview->columns->next;
      ddb_listview_column_free (listview, listview->columns);
      listview->columns = next;
  }

  if (listview->cursor_sz) {
      gdk_cursor_unref (listview->cursor_sz);
      listview->cursor_sz = NULL;
  }
  if (listview->cursor_drag) {
      gdk_cursor_unref (listview->cursor_drag);
      listview->cursor_drag = NULL;
  }
  if (listview->backbuf) {
      g_object_unref (listview->backbuf);
      listview->backbuf = NULL;
  }
  if (listview->backbuf_header) {
      g_object_unref (listview->backbuf_header);
      listview->backbuf_header = NULL;
  }

//  if (G_OBJECT_CLASS (ddb_listview_parent_class)) {
//      G_OBJECT_CLASS (ddb_listview_parent_class)->destroy (object);
//  }
}

void
ddb_listview_refresh (DdbListview *listview, uint32_t flags) {
    if (flags & DDB_REFRESH_LIST) {
        int height = listview->fullheight;
        ddb_listview_build_groups (listview);
        if (height != listview->fullheight) {
            flags |= DDB_REFRESH_VSCROLL;
        }
        ddb_listview_list_render (listview, 0, 0, listview->list->allocation.width, listview->list->allocation.height);
    }
    if (flags & DDB_REFRESH_VSCROLL) {
        ddb_listview_list_setup_vscroll (listview);
    }
    if (flags & DDB_REFRESH_HSCROLL) {
        ddb_listview_list_setup_hscroll (listview);
    }
    if (flags & DDB_REFRESH_COLUMNS) {
        ddb_listview_header_render (listview);
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
        .flags = GTK_TARGET_SAME_APP,
        TARGET_SAMEWIDGET
    };
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
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));

    draw_init_font (widget->style);
    int height = draw_get_font_size () + 12;
    if (height != ps->rowheight) {
        ps->rowheight = height;
        ddb_listview_build_groups (ps);
    }

    ddb_listview_list_setup_vscroll (ps);
    ddb_listview_list_setup_hscroll (ps);
    widget = ps->list;
    if (ps->backbuf) {
        g_object_unref (ps->backbuf);
        ps->backbuf = NULL;
    }
    ps->backbuf = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);

    ddb_listview_list_render (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}

// returns Y coordinate of an item by its index
int
ddb_listview_get_row_pos (DdbListview *listview, int row_idx) {
    int y = 0;
    int idx = 0;
    DdbListviewGroup *grp = listview->groups;
    while (grp) {
        if (idx + grp->num_items > row_idx) {
            return y + listview->grouptitle_height + (row_idx - idx) * listview->rowheight;
        }
        y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }
    return y;
}

// input: absolute y coord in list (not in window)
// returns -1 if nothing was hit, otherwise returns pointer to a group, and item idx
// item idx may be set to -1 if group title was hit
static int
ddb_listview_list_pickpoint_y (DdbListview *listview, int y, DdbListviewGroup **group, int *group_idx, int *global_idx) {
    int idx = 0;
    int grp_y = 0;
    int gidx = 0;
    DdbListviewGroup *grp = listview->groups;
    while (grp) {
        int h = grp->height;
        if (y >= grp_y && y < grp_y + h) {
            *group = grp;
            y -= grp_y;
            if (y < listview->grouptitle_height) {
                *group_idx = -1;
                *global_idx = -1;
            }
            else if (y >= listview->grouptitle_height + grp->num_items * listview->rowheight) {
                *group_idx = (y - listview->grouptitle_height) / listview->rowheight;
                *global_idx = -1;
            }
            else {
                *group_idx = (y - listview->grouptitle_height) / listview->rowheight;
                *global_idx = idx + *group_idx;
            }
            return 0;
        }
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
        gidx++;
    }
    return -1;
}

void
ddb_listview_list_render (DdbListview *listview, int x, int y, int w, int h) {
    if (!listview->backbuf) {
        return;
    }
    GtkWidget *treeview = theme_treeview;
    if (treeview->style->depth == -1) {
        return; // drawing was called too early
    }
    int idx = 0;
    int abs_idx = 0;
    deadbeef->pl_lock ();
    // find 1st group
    DdbListviewGroup *grp = listview->groups;
    int grp_y = 0;
    while (grp && grp_y + grp->height < y + listview->scrollpos) {
        grp_y += grp->height;
        idx += grp->num_items + 1;
        abs_idx += grp->num_items;
        grp = grp->next;
    }
    draw_begin ((uintptr_t)listview->backbuf);

    int ii = 0;
    while (grp && grp_y < y + h + listview->scrollpos) {
        // render title
        DdbListviewIter it = grp->head;
        int grpheight = grp->height;
        if (grp_y >= y + h + listview->scrollpos) {
            break;
        }
        listview->binding->ref (it);
        if (grp_y + listview->grouptitle_height >= y + listview->scrollpos && grp_y < y + h + listview->scrollpos) {
            ddb_listview_list_render_row_background (listview, NULL, idx & 1, 0, -listview->hscrollpos, grp_y - listview->scrollpos, listview->totalwidth, listview->grouptitle_height);
            if (listview->binding->draw_group_title && listview->grouptitle_height > 0) {
                listview->binding->draw_group_title (listview, listview->backbuf, it, -listview->hscrollpos, grp_y - listview->scrollpos, listview->totalwidth, listview->grouptitle_height);
            }
        }
        for (int i = 0; i < grp->num_items; i++) {
            ii++;
//            if (grp_y + listview->grouptitle_height + (i+1) * listview->rowheight >= y + h + listview->scrollpos) {
//                break;
//            }
            if (grp_y + listview->grouptitle_height + i * listview->rowheight >= y + h + listview->scrollpos) {
                break;
            }
            if (grp_y + listview->grouptitle_height + (i+1) * listview->rowheight >= y + listview->scrollpos
                    && grp_y + listview->grouptitle_height + i * listview->rowheight < y + h + listview->scrollpos) {
                gdk_draw_rectangle (listview->backbuf, listview->list->style->bg_gc[GTK_STATE_NORMAL], TRUE, -listview->hscrollpos, grp_y + listview->grouptitle_height + i * listview->rowheight - listview->scrollpos, listview->totalwidth, listview->rowheight);
                ddb_listview_list_render_row_background (listview, it, (idx + 1 + i) & 1, (abs_idx+i) == listview->binding->cursor () ? 1 : 0, -listview->hscrollpos, grp_y + listview->grouptitle_height + i * listview->rowheight - listview->scrollpos, listview->totalwidth, listview->rowheight);
                ddb_listview_list_render_row_foreground (listview, it, grp->head, (idx + 1 + i) & 1, (idx+i) == listview->binding->cursor () ? 1 : 0, i * listview->rowheight, -listview->hscrollpos, grp_y + listview->grouptitle_height + i * listview->rowheight - listview->scrollpos, listview->totalwidth, listview->rowheight);
            }
            DdbListviewIter next = listview->binding->next (it);
            listview->binding->unref (it);
            it = next;
            if (!it) {
                break; // sanity check, in case groups were not rebuilt yet
            }
        }
        if (it) {
            listview->binding->unref (it);
        }
        idx += grp->num_items + 1;
        abs_idx += grp->num_items;

        int filler = grpheight - (listview->grouptitle_height + listview->rowheight * grp->num_items);
        if (filler > 0) {
            int theming = !gtkui_override_listview_colors ();
            if (theming) {
                gtk_paint_flat_box (treeview->style, listview->backbuf, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, treeview, "cell_even_ruled", x, grp_y - listview->scrollpos + listview->grouptitle_height + listview->rowheight * grp->num_items, w, filler);
            }
            else {
                GdkColor clr;
                GdkGC *gc = gdk_gc_new (listview->backbuf);
                gdk_gc_set_rgb_fg_color (gc, (gtkui_get_listview_even_row_color (&clr), &clr));
                gdk_draw_rectangle (listview->backbuf, gc, TRUE, x, grp_y - listview->scrollpos + listview->grouptitle_height + listview->rowheight * grp->num_items, w, filler);
                g_object_unref (gc);
            }


            ddb_listview_list_render_row_foreground (listview, NULL, grp->head, 0, 0, grp->num_items * listview->rowheight, -listview->hscrollpos, grp_y - listview->scrollpos + listview->grouptitle_height + listview->rowheight * grp->num_items, listview->totalwidth, filler);
        }

        grp_y += grpheight;
        grp = grp->next;
    }
    if (grp_y < y + h + listview->scrollpos) {
        int hh = y + h - (grp_y - listview->scrollpos);
//        gdk_draw_rectangle (listview->backbuf, listview->list->style->bg_gc[GTK_STATE_NORMAL], TRUE, x, grp_y - listview->scrollpos, w, hh);
        int theming = !gtkui_override_listview_colors ();
        if (theming) {
            gtk_paint_flat_box (treeview->style, listview->backbuf, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, treeview, "cell_even_ruled", x, grp_y - listview->scrollpos, w, hh);
        }
        else {
            GdkColor clr;
            GdkGC *gc = gdk_gc_new (listview->backbuf);
            gdk_gc_set_rgb_fg_color (gc, (gtkui_get_listview_even_row_color (&clr), &clr));
            gdk_draw_rectangle (listview->backbuf, gc, TRUE, x, grp_y - listview->scrollpos, w, hh);
            g_object_unref (gc);
        }
    }
    deadbeef->pl_unlock ();
    draw_end ();
}

gboolean
ddb_listview_list_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_list_expose (ps, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}

static void
ddb_listview_draw_dnd_marker (DdbListview *ps) {
    if (ps->drag_motion_y < 0) {
        return;
    }
    int drag_motion_y = ps->drag_motion_y - ps->scrollpos;

    GtkWidget *widget = ps->list;
    GdkColor clr;
    gtkui_get_listview_cursor_color (&clr);
    GdkGC *gc = gdk_gc_new (widget->window);
    gdk_gc_set_rgb_fg_color (gc, &clr);
    gdk_draw_rectangle (widget->window, gc, TRUE, 0, drag_motion_y-1, widget->allocation.width, 3);
    gdk_draw_rectangle (widget->window, gc, TRUE, 0, drag_motion_y-3, 3, 7);
    gdk_draw_rectangle (widget->window, gc, TRUE, widget->allocation.width-3, drag_motion_y-3, 3, 7);
    g_object_unref (gc);

}

void
ddb_listview_list_expose (DdbListview *listview, int x, int y, int w, int h) {
    GtkWidget *widget = listview->list;
    if (widget->window && listview->backbuf) {
        draw_drawable (widget->window, widget->style->black_gc, listview->backbuf, x, y, x, y, w, h);
    }
    if (listview->drag_motion_y >= 0 && listview->drag_motion_y-listview->scrollpos-3 < y+h && listview->drag_motion_y-listview->scrollpos+3 >= y) {
        ddb_listview_draw_dnd_marker (listview);
    }
}

gboolean
ddb_listview_vscroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
	GdkEventScroll *ev = (GdkEventScroll*)event;
    GtkWidget *range = ps->scrollbar;;
    GtkWidget *list = ps->list;
    // pass event to scrollbar
    int newscroll = gtk_range_get_value (GTK_RANGE (range));
    if (ev->direction == GDK_SCROLL_UP) {
        newscroll -= SCROLL_STEP * 2;
    }
    else if (ev->direction == GDK_SCROLL_DOWN) {
        newscroll += SCROLL_STEP * 2;
    }
    gtk_range_set_value (GTK_RANGE (range), newscroll);

    return FALSE;
}

void
ddb_listview_vscroll_value_changed            (GtkRange        *widget,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    if (ps->binding->vscroll_changed) {
        ps->binding->vscroll_changed (newscroll);
    }
    if (ps->block_redraw_on_scroll) {
        ps->scrollpos = newscroll; 
        return;
    }
    if (newscroll != ps->scrollpos) {
        GtkWidget *widget = ps->list;
        int di = newscroll - ps->scrollpos;
        int d = abs (di);
        int height = ps->list->allocation.height;
        if (d < height) {
            if (di > 0) {
                // scroll down
                // copy scrolled part of buffer
                draw_drawable (ps->backbuf, widget->style->black_gc, ps->backbuf, 0, d, 0, 0, widget->allocation.width, widget->allocation.height-d);
                // redraw other part
                int start = height-d-1;
                ps->scrollpos = newscroll;
                ddb_listview_list_render (ps, 0, start, ps->list->allocation.width, widget->allocation.height-start);
            }
            else {
                // scroll up
                // copy scrolled part of buffer
                draw_drawable (ps->backbuf, widget->style->black_gc, ps->backbuf, 0, 0, 0, d, widget->allocation.width, widget->allocation.height-d);
                // redraw other part
                ps->scrollpos = newscroll;
                ddb_listview_list_render (ps, 0, 0, ps->list->allocation.width, d+1);
            }
        }
        else {
            // scrolled more than view height, redraw everything
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
    DdbListview *pl = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
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
    DdbListview *pl = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_list_track_dragdrop (pl, y);
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


gboolean
ddb_listview_list_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    return TRUE;
#if 0
    if (drag_context->targets) {
        GdkAtom target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets, TARGET_SAMEWIDGET));
        if (!target_type) {
            return FALSE;
        }
//        gtk_drag_get_data (widget, drag_context, target_type, time);
        return TRUE;
    }
    return FALSE;
#endif
}


void
ddb_listview_list_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *selection_data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    switch (target_type) {
    case TARGET_SAMEWIDGET:
        {
            // format as "STRING" consisting of array of pointers
            int nsel = deadbeef->plt_get_sel_count (ps->drag_source_playlist);
            if (!nsel) {
                break; // something wrong happened
            }
            uint32_t *ptr = malloc ((nsel+1) * sizeof (uint32_t));
            *ptr = ps->drag_source_playlist;
            int idx = 0;
            int i = 1;
            DdbListviewIter it = deadbeef->plt_get_head (ps->drag_source_playlist);
            for (; it; idx++) {
                if (ps->binding->is_selected (it)) {
                    ptr[i] = idx;
                    i++;
                }
                DdbListviewIter next = ps->binding->next (it);
                ps->binding->unref (it);
                it = next;
            }
            gtk_selection_data_set (selection_data, selection_data->target, sizeof (uint32_t) * 8, (gchar *)ptr, (nsel+1) * sizeof (uint32_t));
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
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (!ps->binding->external_drag_n_drop || !ps->binding->drag_n_drop) {
        gtk_drag_finish (drag_context, TRUE, FALSE, time);
        return;
    }
    int sel = ddb_listview_dragdrop_get_row_from_coord (ps, y);
    DdbListviewIter it = NULL;
    if (sel == -1) {
        if (ps->binding->count () != 0) {
            sel = ps->binding->count ();
        }
    }
    if (sel != -1) {
        it = ps->binding->get_for_idx (sel);
    }
    gchar *ptr=(char*)data->data;
    if (target_type == 0) { // uris
        // this happens when dropped from file manager
        char *mem = malloc (data->length+1);
        memcpy (mem, ptr, data->length);
        mem[data->length] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        ps->binding->external_drag_n_drop (it, mem, data->length);
        if (it) {
            UNREF (it);
        }
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        int plt = *d;
        d++;
        int length = (data->length/4)-1;
        DdbListviewIter drop_before = it;
        // find last selected
        while (drop_before && ps->binding->is_selected (drop_before)) {
            DdbListviewIter next = PL_NEXT(drop_before);
            UNREF (drop_before);
            drop_before = next;
        }
        ps->binding->drag_n_drop (drop_before, plt, d, length, drag_context->action == GDK_ACTION_COPY ? 1 : 0);
        if (drop_before) {
            UNREF (drop_before);
        }
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
    DdbListview *pl = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_list_track_dragdrop (pl, -1);
}

// debug function for gdk_draw_drawable
static inline void
draw_drawable (GdkDrawable *window, GdkGC *gc, GdkDrawable *drawable, int x1, int y1, int x2, int y2, int w, int h) {
    gint width1, height1;
    gint width2, height2;
    gdk_drawable_get_size (window, &width1, &height1);
    gdk_drawable_get_size (drawable, &width2, &height2);
//    assert (y1 >= 0 && y1 + h < height2);
//    assert (y2 >= 0 && y2 + h < height1);
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
    DdbListviewColumn *c;
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
    GtkWidget *list = ps->list;
    GtkWidget *scroll = ps->scrollbar;
    int vheight = ps->fullheight;
    if (ps->fullheight <= ps->list->allocation.height) {
        gtk_widget_hide (scroll);
        ps->scrollpos = 0;
        ddb_listview_list_render (ps, 0, 0, list->allocation.width, list->allocation.height);
        ddb_listview_list_expose (ps, 0, 0, list->allocation.width, list->allocation.height);
    }
    else {
        gtk_widget_show (scroll);
        if (ps->scrollpos >= vheight) {
            ps->scrollpos = vheight-1;
        }
    }
    int h = list->allocation.height;
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, vheight, SCROLL_STEP, h/2, h);
    gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
    gtk_range_set_value (GTK_RANGE (scroll), ps->scrollpos);
}

void
ddb_listview_list_setup_hscroll (DdbListview *ps) {
    GtkWidget *list = ps->list;
    int w = list->allocation.width;
    int size = 0;
    DdbListviewColumn *c;
    for (c = ps->columns; c; c = c->next) {
        size += c->width;
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

// returns -1 if row not found
int
ddb_listview_list_get_drawinfo (DdbListview *listview, int row, DdbListviewGroup **pgrp, int *even, int *cursor, int *group_y, int *x, int *y, int *w, int *h) {
    DdbListviewGroup *grp = listview->groups;
    int idx = 0;
    int idx2 = 0;
    *y = -listview->scrollpos;
    while (grp) {
        int grpheight = grp->height;
        if (idx <= row && idx + grp->num_items > row) {
            // found
            int idx_in_group = row - idx;
            *pgrp = grp;
            *even = (idx2 + 1 + idx_in_group) & 1;
            *cursor = (row == listview->binding->cursor ()) ? 1 : 0;
            *group_y = idx_in_group * listview->rowheight;
            *x = -listview->hscrollpos;
            *y += listview->grouptitle_height + (row - idx) * listview->rowheight;
            *w = listview->totalwidth;
            *h = listview->rowheight;
            return 0;
        }
        *y += grpheight;
        idx += grp->num_items;
        idx2 += grp->num_items + 1;
        grp = grp->next;
    }
    return -1;
}

void
ddb_listview_list_render_row (DdbListview *listview, int row, DdbListviewIter it, int expose) {
    DdbListviewGroup *grp;
    int even;
    int cursor;
    int x, y, w, h;
    int group_y;
    if (ddb_listview_list_get_drawinfo (listview, row, &grp, &even, &cursor, &group_y, &x, &y, &w, &h) == -1) {
        return;
    }

    if (y + h <= 0) {
        return;
    }

    if (y > GTK_WIDGET (listview)->allocation.height) {
        return;
    }

    draw_begin ((uintptr_t)listview->backbuf);
    ddb_listview_list_render_row_background (listview, it, even, cursor, x, y, w, h);
	if (it) {
        ddb_listview_list_render_row_foreground (listview, it, grp->head, even, cursor, group_y, x, y, w, h);
    }
    draw_end ();
    if (expose) {
        draw_drawable (listview->list->window, listview->list->style->black_gc, listview->backbuf, 0, y, 0, y, listview->list->allocation.width, h);
    }
}

void
ddb_listview_draw_row (DdbListview *listview, int row, DdbListviewIter it) {
    ddb_listview_list_render_row (listview, row, it, 1);
}

// coords passed are window-relative
void
ddb_listview_list_render_row_background (DdbListview *ps, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h) {
	// draw background
	GtkWidget *treeview = theme_treeview;
	int theming = !gtkui_override_listview_colors ();

	if (theming) {
        if (treeview->style->depth == -1) {
            return; // drawing was called too early
        }
        GTK_WIDGET_SET_FLAGS (GTK_WIDGET (treeview), GTK_HAS_FOCUS);
        //G_OBJECT_FLAGS (treeview) |= GTK_HAS_FOCUS;
    }
    int sel = it && ps->binding->is_selected (it);
    if (theming || !sel) {
        if (theming) {
            // draw background for selection -- workaround for New Wave theme (translucency)
            gtk_paint_flat_box (treeview->style, ps->backbuf, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, treeview, even ? "cell_even_ruled" : "cell_odd_ruled", x, y, w, h);
        }
        else {
            GdkColor clr;
            GdkGC *gc = gdk_gc_new (ps->backbuf);
            gdk_gc_set_rgb_fg_color (gc, even ? (gtkui_get_listview_even_row_color (&clr), &clr) : (gtkui_get_listview_odd_row_color (&clr), &clr));
            gdk_draw_rectangle (ps->backbuf, gc, TRUE, x, y, w, h);
            g_object_unref (gc);
        }
    }

    if (sel) {
        if (theming) {
            gtk_paint_flat_box (treeview->style, ps->backbuf, GTK_STATE_SELECTED, GTK_SHADOW_NONE, NULL, treeview, even ? "cell_even_ruled" : "cell_odd_ruled", x, y, w, h);
        }
        else {
            GdkColor clr;
            GdkGC *gc = gdk_gc_new (ps->backbuf);
            gdk_gc_set_rgb_fg_color (gc, (gtkui_get_listview_selection_color (&clr), &clr));
            gdk_draw_rectangle (ps->backbuf, gc, TRUE, x, y, w, h);
            g_object_unref (gc);
        }
    }
	if (cursor) {
        // not all gtk engines/themes render focus rectangle in treeviews
        // but we want it anyway
        //treeview->style->fg_gc[GTK_STATE_NORMAL]
        GdkColor clr;
        GdkGC *gc = gdk_gc_new (ps->backbuf);
        gdk_gc_set_rgb_fg_color (gc, (gtkui_get_listview_cursor_color (&clr), &clr));
        gdk_draw_rectangle (ps->backbuf, gc, FALSE, x, y, w-1, h-1);
        g_object_unref (gc);
    }
}

void
ddb_listview_list_render_row_foreground (DdbListview *ps, DdbListviewIter it, DdbListviewIter group_it, int even, int cursor, int group_y, int x, int y, int w, int h) {
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
    DdbListviewColumn *c;
    int cidx = 0;
    for (c = ps->columns; c; c = c->next, cidx++) {
        int cw = c->width;
        ps->binding->draw_column_data (ps, ps->backbuf, it, ps->grouptitle_height > 0 ? group_it : NULL, cidx, group_y, x, y, cw, h);
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
            else if (ps->binding->cursor () == idx) {
                ddb_listview_draw_row (ps, idx, it);
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

void
ddb_listview_click_selection (DdbListview *ps, int ex, int ey, DdbListviewGroup *grp, int grp_index, int sel, int dnd) {
    if (sel == -1 && (!grp || grp_index >= grp->num_items)) {
        // clicked empty space, deselect everything
        DdbListviewIter it;
        int idx = 0;
        for (it = ps->binding->head (); it; idx++) {
            if (ps->binding->is_selected (it)) {
                ps->binding->select (it, 0);
                ddb_listview_draw_row (ps, idx, it);
                ps->binding->selection_changed (it, idx);
            }
            DdbListviewIter next = ps->binding->next (it);
            ps->binding->unref (it);
            it = next;
        }
    }
    else if (sel == -1 && grp) {
        // clicked group title, select group
        DdbListviewIter it;
        int idx = 0;
        int cnt = -1;
        for (it = ps->binding->head (); it; idx++) {
            if (it == grp->head) {
                cnt = grp->num_items;
            }
            if (cnt > 0) {
                if (!ps->binding->is_selected (it)) {
                    ps->binding->select (it, 1);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (it, idx);
                }
                cnt--;
            }
            else {
                if (ps->binding->is_selected (it)) {
                    ps->binding->select (it, 0);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (it, idx);
                }
            }
            DdbListviewIter next = ps->binding->next (it);
            ps->binding->unref (it);
            it = next;
        }
    }
    else {
        // clicked specific item - select, or start drag-n-drop
        DdbListviewIter it = ps->binding->get_for_idx (sel);
        if (!it || !ps->binding->is_selected (it) || !ps->binding->drag_n_drop) {
            // reset selection, and set it to single item
            ddb_listview_select_single (ps, sel);
            if (dnd) {
                ps->areaselect = 1;
                ps->areaselect_y = ey + ps->scrollpos;
                ps->shift_sel_anchor = ps->binding->cursor ();
            }
        }
        else if (dnd) {
            ps->dragwait = 1;
        }
        UNREF (it);
    }
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
    DdbListviewGroup *grp;
    int grp_index;
    int sel;
    if (ddb_listview_list_pickpoint_y (ps, ey + ps->scrollpos, &grp, &grp_index, &sel) == -1) {
        return;
    }

    int cursor = ps->binding->cursor ();
    if (time - ps->clicktime < 0.5
            && fabs(ps->lastpos[0] - ex) < 3
            && fabs(ps->lastpos[1] - ey) < 3) {
        // doubleclick - play this item
        if (sel != -1 && cursor != -1) {
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

    int prev = cursor;
    if (sel != -1) {
        ps->binding->set_cursor (sel);
        DdbListviewIter it = ps->binding->get_for_idx (sel);
        if (it) {
            ddb_listview_draw_row (ps, sel, it);
            UNREF (it);
        }
        ps->shift_sel_anchor = ps->binding->cursor ();
    }
    // handle multiple selection
    if (!(state & (GDK_CONTROL_MASK|GDK_SHIFT_MASK)))
    {
        ddb_listview_click_selection (ps, ex, ey, grp, grp_index, sel, 1);
    }
    else if (state & GDK_CONTROL_MASK) {
        // toggle selection
        if (sel != -1) {
            DdbListviewIter it = ps->binding->get_for_idx (sel);
            if (it) {
                ps->binding->select (it, 1 - ps->binding->is_selected (it));
                ddb_listview_draw_row (ps, sel, it);
                ps->binding->selection_changed (it, sel);
                UNREF (it);
            }
        }
    }
    else if (state & GDK_SHIFT_MASK) {
        // select range
        int cursor = sel;//ps->binding->cursor ();
        if (cursor == -1) {
            // find group
            DdbListviewGroup *g = ps->groups;
            int idx = 0;
            while (g) {
                if (g == grp) {
                    cursor = idx - 1;
                    break;
                }
                idx += g->num_items;
                g = g->next;
            }
        }
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
        DdbListviewGroup *grp;
        int grp_index;
        int sel;
        if (!ddb_listview_list_pickpoint_y (ps, ey + ps->scrollpos, &grp, &grp_index, &sel)) {
            ddb_listview_select_single (ps, sel);
        }
        else {
            ps->binding->set_cursor (-1);
            DdbListviewIter it = ps->binding->head ();
            int idx = 0;
            while (it) {
                if (ps->binding->is_selected (it)) {
                    ps->binding->select (it, 0);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (it, idx);
                    it = PL_NEXT (it);
                }
                idx++;
            }
        }
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
    float dt = tm.tv_sec - ps->tm_prevscroll.tv_sec + (tm.tv_usec - ps->tm_prevscroll.tv_usec) / 1000000.0;
    if (dt < ps->scroll_sleep_time) {
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
    int sc = ps->scrollpos + (ps->scroll_direction * 100 * dt);
    if (sc < 0) {
        ps->scroll_active = 0;
        return FALSE;
    }
//    trace ("scroll to %d speed %f\n", sc, ps->scroll_direction);
    gtk_range_set_value (GTK_RANGE (ps->scrollbar), sc);
    if (ps->scroll_mode == 0) {
        ddb_listview_list_mousemove (ps, NULL, 0, ps->scroll_pointer_y);
    }
    else if (ps->scroll_mode == 1) {
        ddb_listview_list_track_dragdrop (ps, ps->scroll_pointer_y);
    }
    if (ps->scroll_direction < 0) {
        ps->scroll_direction -= (10 * dt);
        if (ps->scroll_direction < -30) {
            ps->scroll_direction = -30;
        }
    }
    else {
        ps->scroll_direction += (10 * dt);
        if (ps->scroll_direction > 30) {
            ps->scroll_direction = 30;
        }
    }
    return TRUE;
}

void
ddb_listview_list_mousemove (DdbListview *ps, GdkEventMotion *ev, int ex, int ey) {
    if (ps->dragwait) {
        GtkWidget *widget = ps->list;
        if (gtk_drag_check_threshold (widget, ps->lastpos[0], ex, ps->lastpos[1], ey)) {
            ps->dragwait = 0;
            ps->drag_source_playlist = deadbeef->plt_get_curr ();
            GtkTargetEntry entry = {
                .target = "STRING",
                .flags = GTK_TARGET_SAME_WIDGET,
                .info = TARGET_SAMEWIDGET
            };
            GtkTargetList *lst = gtk_target_list_new (&entry, 1);
            gtk_drag_begin (widget, lst, GDK_ACTION_COPY | GDK_ACTION_MOVE, 1, (GdkEvent *)ev);
        }
    }
    else if (ps->areaselect) {
        DdbListviewGroup *grp;
        int grp_index;
        int sel;
        if (ddb_listview_list_pickpoint_y (ps, ey + ps->scrollpos, &grp, &grp_index, &sel) == -1) {
            // past playlist bounds -> set to last track
            sel = ps->binding->count () - 1;
        }
        else if (sel == -1) {
            if (grp_index == -1) {
                if (ps->areaselect_y < ey + ps->scrollpos) {
                    // below anchor, take last track in prev group
                    sel = ps->binding->get_idx (grp->head) - 1;
                }
                else if (ps->areaselect_y > ey + ps->scrollpos) {
                    // above, select 1st track in group
                        sel = ps->binding->get_idx (grp->head);
                }
                else {
                    sel = ps->shift_sel_anchor;
                }
            }
            else {
                if (ps->areaselect_y < ey + ps->scrollpos) {
                    // below anchor, take last track in group
                    sel = ps->binding->get_idx (grp->head) + grp->num_items - 1;
                }
                else if (ps->areaselect_y > ey + ps->scrollpos) {
                    // above, select 1st track in next group
                    if (grp->next) {
                        sel = ps->binding->get_idx (grp->next->head);
                    }
                }
                else {
                    sel = ps->shift_sel_anchor;
                }
            }
        }
        int prev = ps->binding->cursor ();
        if (sel != -1) {
            ps->binding->set_cursor (sel);
        }
        {
            // select range of items
            int y = sel;
            int idx = 0;
            if (y == -1) {
                // find group
                DdbListviewGroup *g = ps->groups;
                while (g) {
                    if (g == grp) {
                        y = idx - 1;
                        break;
                    }
                    idx += g->num_items;
                    g = g->next;
                }
            }
            int start = min (y, ps->shift_sel_anchor);
            int end = max (y, ps->shift_sel_anchor);

            idx=0;
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
        if (sel != -1 && sel != prev) {
            if (prev != -1) {
                DdbListviewIter it = ps->binding->get_for_idx (prev);
                if (it) {
                    ddb_listview_draw_row (ps, prev, it);
                    UNREF (it);
                }
            }
            DdbListviewIter it = ps->binding->get_for_idx (sel);
            if (it) {
                ddb_listview_draw_row (ps, sel, it);
                UNREF (it);
            }
        }

        if (ey < 10) {
            ps->scroll_mode = 0;
            ps->scroll_pointer_y = ey;
            // start scrolling up
            if (!ps->scroll_active) {
                ps->scroll_direction = -1;
                ps->scroll_sleep_time = AUTOSCROLL_UPDATE_FREQ;
                gettimeofday (&ps->tm_prevscroll, NULL);
                g_idle_add (ddb_listview_list_scroll_cb, ps);
            }
        }
        else if (ey > ps->list->allocation.height-10) {
            ps->scroll_mode = 0;
            ps->scroll_pointer_y = ey;
            // start scrolling down
            if (!ps->scroll_active) {
                ps->scroll_direction = 1;
                ps->scroll_sleep_time = AUTOSCROLL_UPDATE_FREQ;
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
    if (ps->block_redraw_on_scroll) {
        ps->hscrollpos = newscroll; 
        return;
    }
//    if (newscroll != ps->hscrollpos)
    // need to redraw because this might be window resize
    {
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
    GtkWidget *range = ps->scrollbar;
    GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (range));

    state &= (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD4_MASK);

    if (state & ~GDK_SHIFT_MASK) {
        return 0;
    }

    if (keyval == GDK_Down) {
        if (cursor < ps->binding->count () - 1) {
            cursor++;
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), adj->upper);
        }
    }
    else if (keyval == GDK_Up) {
        if (cursor > 0) {
            cursor--;
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), adj->lower);
            if (cursor < 0 && ps->binding->count () > 0) {
                cursor = 0;
            }
        }
    }
    else if (keyval == GDK_Page_Down) {
        if (cursor < ps->binding->count () - 1) {
            cursor += 10;
            if (cursor >= ps->binding->count ()) {
                cursor = ps->binding->count () - 1;
            }
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), adj->upper);
        }
    }
    else if (keyval == GDK_Page_Up) {
        if (cursor > 0) {
            cursor -= 10;
            if (cursor < 0) {
                gtk_range_set_value (GTK_RANGE (range), adj->lower);
                cursor = 0;
            }
        }
        else {
            if (cursor < 0 && ps->binding->count () > 0) {
                cursor = 0;
            }
            gtk_range_set_value (GTK_RANGE (range), adj->lower);
        }
    }
    else if (keyval == GDK_End) {
        cursor = ps->binding->count () - 1;
        gtk_range_set_value (GTK_RANGE (range), adj->upper);
    }
    else if (keyval == GDK_Home) {
        cursor = 0;
        gtk_range_set_value (GTK_RANGE (range), adj->lower);
    }
    else if (keyval == GDK_Delete) {
        ps->binding->delete_selected ();
        cursor = ps->binding->cursor ();
    }
    else {
        return 0;
    }

    if (state & GDK_SHIFT_MASK) {
        if (cursor != prev) {
            int newscroll = ps->scrollpos;
            int cursor_scroll = ddb_listview_get_row_pos (ps, cursor);
            if (cursor_scroll < ps->scrollpos) {
                newscroll = cursor_scroll;
            }
            else if (cursor_scroll >= ps->scrollpos + ps->list->allocation.height) {
                newscroll = cursor_scroll - ps->list->allocation.height + 1;
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

int
ddb_listview_dragdrop_get_row_from_coord (DdbListview *listview, int y) {
    if (y == -1) {
        return -1;
    }
    DdbListviewGroup *grp;
    int grp_index;
    int sel;
    if (ddb_listview_list_pickpoint_y (listview, y + listview->scrollpos, &grp, &grp_index, &sel) == -1) {
        return -1;
    }
    else {
        if (sel == -1) {
            if (grp_index == -1) {
                sel = listview->binding->get_idx (grp->head);
            }
            else {
                sel = listview->binding->get_idx (grp->head) + grp->num_items;
            }
        }
    }
    if (sel != -1) {
        int it_y = ddb_listview_get_row_pos (listview, sel) - listview->scrollpos;
        if (y > it_y + listview->rowheight/2 && y < it_y + listview->rowheight) {
            sel++;
        }
    }
    return sel;
}

void
ddb_listview_list_track_dragdrop (DdbListview *ps, int y) {
    GtkWidget *widget = ps->list;
    if (ps->drag_motion_y != -1) {
        // erase previous track
        draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, ps->drag_motion_y-3-ps->scrollpos, 0, ps->drag_motion_y-ps->scrollpos-3, widget->allocation.width, 7);

    }
    if (y == -1) {
        ps->drag_motion_y = -1;
        return;
    }
    int sel = ddb_listview_dragdrop_get_row_from_coord (ps, y);
    if (sel == -1) {
        if (ps->binding->count () == 0) {
            ps->drag_motion_y = 0;
        }
        else {
            // after last row
            ps->drag_motion_y = ddb_listview_get_row_pos (ps, ps->binding->count ()-1) + ps->rowheight;
        }
    }
    else {
        ps->drag_motion_y = ddb_listview_get_row_pos (ps, sel);
    }

    ddb_listview_draw_dnd_marker (ps);
    
    if (y < 10) {
        ps->scroll_pointer_y = y;
        ps->scroll_mode = 1;
        // start scrolling up
        if (!ps->scroll_active) {
            ps->scroll_direction = -1;
            ps->scroll_sleep_time = AUTOSCROLL_UPDATE_FREQ;
            gettimeofday (&ps->tm_prevscroll, NULL);
            g_idle_add (ddb_listview_list_scroll_cb, ps);
        }
    }
    else if (y > ps->list->allocation.height-10) {
        ps->scroll_mode = 1;
        ps->scroll_pointer_y = y;
        // start scrolling up
        if (!ps->scroll_active) {
            ps->scroll_direction = 1;
            ps->scroll_sleep_time = AUTOSCROLL_UPDATE_FREQ;
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
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_refresh (ps, DDB_REFRESH_LIST|DDB_EXPOSE_LIST);
    ps->scroll_direction = 0;
    ps->scroll_pointer_y = -1;
}

// #define HEADERS_GTKTHEME

void
ddb_listview_header_render (DdbListview *ps) {
    GtkWidget *widget = ps->header;
    int x = -ps->hscrollpos;
    int w = 100;
    int h = widget->allocation.height;
    const char *detail = "button";

    // fill background and draw bottom line
#if !HEADERS_GTKTHEME
    GdkGC *gc = gdk_gc_new (ps->backbuf_header);
    GdkColor clr;
    gdk_gc_set_rgb_fg_color (gc, (gtkui_get_tabstrip_base_color (&clr), &clr));
    gdk_draw_rectangle (ps->backbuf_header, gc, TRUE, 0, 0,  widget->allocation.width, widget->allocation.height);
    gdk_gc_set_rgb_fg_color (gc, (gtkui_get_tabstrip_dark_color (&clr), &clr));
    gdk_draw_line (ps->backbuf_header, gc, 0, widget->allocation.height-1, widget->allocation.width, widget->allocation.height-1);
#else
    gtk_paint_box (theme_button->style, ps->backbuf_header, GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, widget, detail, -10, -10, widget->allocation.width+20, widget->allocation.height+20);
    gdk_draw_line (ps->backbuf_header, widget->style->mid_gc[GTK_STATE_NORMAL], 0, widget->allocation.height-1, widget->allocation.width, widget->allocation.height-1);
#endif
    draw_begin ((uintptr_t)ps->backbuf_header);
    x = -ps->hscrollpos;
    DdbListviewColumn *c;
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
        if (ps->header_dragging < 0 || idx != ps->header_dragging) {
            if (xx >= widget->allocation.width) {
                continue;
            }
            int arrow_sz = 10;
            int sort = c->sort_order;
            if (w > 0) {
#if !HEADERS_GTKTHEME
                gdk_gc_set_rgb_fg_color (gc, (gtkui_get_tabstrip_dark_color (&clr), &clr));
                gdk_draw_line (ps->backbuf_header, gc, xx+w - 2, 2, xx+w - 2, h-4);
                gdk_gc_set_rgb_fg_color (gc, (gtkui_get_tabstrip_light_color (&clr), &clr));
                gdk_draw_line (ps->backbuf_header, gc, xx+w - 1, 2, xx+w - 1, h-4);
#else
                gtk_paint_vline (widget->style, ps->backbuf_header, GTK_STATE_NORMAL, NULL, widget, NULL, 2, h-4, xx+w - 2);
#endif
                GdkColor *gdkfg = &theme_button->style->fg[0];
                float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
                draw_set_fg_color (fg);
                int ww = w-10;
                if (sort) {
                    ww -= arrow_sz;
                    if (ww < 0) {
                        ww = 0;
                    }
                }
                draw_text (xx + 5, h/2-draw_get_font_size()/2, ww, 0, c->title);
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
        for (c = ps->columns; c; c = c->next, idx++) {
            w = c->width;
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
                    gtk_paint_box (theme_button->style, ps->backbuf_header, GTK_STATE_ACTIVE, GTK_SHADOW_ETCHED_IN, NULL, widget, "button", x, 0, w, h);
                }
                x = ps->col_movepos;
                if (x >= widget->allocation.width) {
                    break;
                }
                if (w > 0) {
                    gtk_paint_box (theme_button->style, ps->backbuf_header, GTK_STATE_SELECTED, GTK_SHADOW_OUT, NULL, widget, "button", x, 0, w, h);
                    GdkColor *gdkfg = &theme_button->style->fg[GTK_STATE_SELECTED];
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

#if !HEADERS_GTKTHEME
    g_object_unref (gc);
#endif
}

gboolean
ddb_listview_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_header_expose (ps, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}


gboolean
ddb_listview_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    draw_init_font (widget->style);
    int height = draw_get_font_size () + 12;
    if (height != widget->allocation.height) {
        gtk_widget_set_size_request (widget, -1, height);
    }
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
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    int h = draw_get_font_size ();
    gtk_widget_set_size_request (widget, -1, h + 10);
    listview->cursor_sz = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    listview->cursor_drag = gdk_cursor_new (GDK_FLEUR);
}

gboolean
ddb_listview_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    int ev_x, ev_y;
    GdkModifierType ev_state;

#if 0
    if (event->is_hint)
        gdk_window_get_pointer (event->window, &ev_x, &ev_y, &ev_state);
    else
    {
        ev_x = event->x;
        ev_y = event->y;
        ev_state = event->state;
    }
#endif
    ev_x = event->x;
    ev_y = event->y;
    ev_state = event->state;
    gdk_event_request_motions (event);

    if ((ev_state & GDK_BUTTON1_MASK) && ps->header_prepare) {
        if (gtk_drag_check_threshold (widget, ev_x, ps->prev_header_x, 0, 0)) {
            ps->header_prepare = 0;
        }
    }
    if (!ps->header_prepare && ps->header_dragging >= 0) {
        gdk_window_set_cursor (widget->window, ps->cursor_drag);
        DdbListviewColumn *c;
        int i;
        for (i = 0, c = ps->columns; i < ps->header_dragging && c; c = c->next, i++);
        ps->col_movepos = ev_x - ps->header_dragpt[0];

        // find closest column to the left
        int inspos = -1;
        DdbListviewColumn *cc;
        int x = 0;
        int idx = 0;
        int x1 = -1, x2 = -1;
        for (cc = ps->columns; cc; cc = cc->next, idx++) {
            if (x < ps->col_movepos && x + c->width > ps->col_movepos) {
                inspos = idx;
                x1 = x;
            }
            else if (idx == ps->header_dragging) {
                x2 = x;
            }
            x += cc->width;
        }
        if (inspos >= 0 && inspos != ps->header_dragging) {
            ddb_listview_column_move (ps, c, inspos);
//            ps->binding->col_move (c, inspos);
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
        gdk_window_set_cursor (widget->window, ps->cursor_sz);
        // get column start pos
        int x = -ps->hscrollpos;
        int i = 0;
        DdbListviewColumn *c;
        for (c = ps->columns; c && i < ps->header_sizing; c = c->next, i++) {
            x += c->width;
        }

        int newx = ev_x > x + MIN_COLUMN_WIDTH ? ev_x : x + MIN_COLUMN_WIDTH;
        c->width = newx-x;
        if (c->minheight) {
            ddb_listview_build_groups (ps);
        }
        ps->block_redraw_on_scroll = 1;
        ddb_listview_list_setup_vscroll (ps);
        ddb_listview_list_setup_hscroll (ps);
        ps->block_redraw_on_scroll = 0;
        ddb_listview_header_render (ps);
        ddb_listview_header_expose (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);
        ddb_listview_list_render (ps, 0, 0, ps->list->allocation.width, ps->list->allocation.height);
        ddb_listview_list_expose (ps, 0, 0, ps->list->allocation.width, ps->list->allocation.height);
        ps->binding->column_size_changed (ps, ps->header_sizing);
    }
    else {
        int x = -ps->hscrollpos;
        DdbListviewColumn *c;
        for (c = ps->columns; c; c = c->next) {
            int w = c->width;
            if (w > 0) { // ignore collapsed columns (hack for search window)
                if (ev_x >= x + w - 2 && ev_x <= x + w) {
                    gdk_window_set_cursor (widget->window, ps->cursor_sz);
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

static int
ddb_listview_header_get_column_idx_for_coord (DdbListview *pl, int click_x) {
    int x = -pl->hscrollpos;
    DdbListviewColumn *c;
    int idx = 0;
    for (c = pl->columns; c; c = c->next, idx++) {
        int w = c->width;
        if (click_x >= x && click_x < x + w) {
            return idx;
        }
        x += w;
    }
    return -1;
}

gboolean
ddb_listview_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
//    ps->active_column = ddb_listview_header_get_column_for_coord (ps, event->x);
    if (event->button == 1) {
        // start sizing/dragging
        ps->header_dragging = -1;
        ps->header_sizing = -1;
        ps->header_dragpt[0] = event->x;
        ps->header_dragpt[1] = event->y;
        int x = -ps->hscrollpos;
        int i = 0;
        DdbListviewColumn *c;
        for (c = ps->columns; c; c = c->next, i++) {
            int w = c->width;
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
        int idx = ddb_listview_header_get_column_idx_for_coord (ps, event->x);
        ps->binding->header_context_menu (ps, idx);
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
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (event->button == 1) {
        if (ps->header_prepare) {
            ps->header_sizing = -1;
            ps->header_dragging = -1;
            ps->header_prepare = 0;
            // sort
            DdbListviewColumn *c;
            int i = 0;
            int x = -ps->hscrollpos;
            int sorted = 0;
            for (c = ps->columns; c; c = c->next, i++) {
                int w = c->width;
                if (event->x > x + 2 && event->x < x + w - 2) {
                    int sort_order = c->sort_order;
                    if (!sort_order) {
                        c->sort_order = 1;
                    }
                    else if (sort_order == 1) {
                        c->sort_order = 2;
                    }
                    else if (sort_order == 2) {
                        c->sort_order = 1;
                    }
                    ps->binding->col_sort (i, c->sort_order, c->user_data);
                    sorted = 1;
                }
                else {
                    c->sort_order = 0;
                }
                x += w;
            }
            ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_COLUMNS | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
        }
        else {
            ps->header_sizing = -1;
            int x = 0;
            DdbListviewColumn *c;
            for (c = ps->columns; c; c = c->next) {
                int w = c->width;
                if (event->x >= x + w - 2 && event->x <= x + w) {
                    gdk_window_set_cursor (widget->window, ps->cursor_sz);
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
        ps->binding->columns_changed (ps);
    }
    return FALSE;
}

struct set_cursor_t {
    int cursor;
    int prev;
    DdbListview *pl;
};

static gboolean
ddb_listview_set_cursor_cb (gpointer data) {
    struct set_cursor_t *sc = (struct set_cursor_t *)data;

    DdbListviewIter prev_it = sc->pl->binding->get_for_idx (sc->prev);
    sc->pl->binding->set_cursor (sc->cursor);
    int prev_selected = 0;

    if (prev_it) {
        prev_selected = sc->pl->binding->is_selected (prev_it);
    }

    ddb_listview_select_single (sc->pl, sc->cursor);

    if (prev_it && !prev_selected) {
        ddb_listview_draw_row (sc->pl, sc->prev, prev_it);
    }

    if (prev_it) {
        sc->pl->binding->unref (prev_it);
    }

    DdbListviewIter it;
    DdbListview *ps = sc->pl;

    int cursor_scroll = ddb_listview_get_row_pos (sc->pl, sc->cursor);
    int newscroll = sc->pl->scrollpos;
    if (cursor_scroll < sc->pl->scrollpos) {
        newscroll = cursor_scroll;
    }
    else if (cursor_scroll + sc->pl->rowheight >= sc->pl->scrollpos + sc->pl->list->allocation.height) {
        newscroll = cursor_scroll + sc->pl->rowheight - sc->pl->list->allocation.height + 1;
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
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (event->button == 1) {
        ddb_listview_list_mouse1_pressed (ps, event->state, event->x, event->y, event->time);
    }
    else if (event->button == 3) {
        // get item under cursor
        DdbListviewGroup *grp;
        int grp_index;
        int sel;
        DdbListviewIter it = NULL;
        int prev = ps->binding->cursor ();
        if (ddb_listview_list_pickpoint_y (ps, event->y + ps->scrollpos, &grp, &grp_index, &sel) != -1) {
            if (sel != -1) {
                ps->binding->set_cursor (sel);
            }
            ddb_listview_click_selection (ps, event->x, event->y, grp, grp_index, sel, 0);
            if (sel == -1 && grp_index < grp->num_items) {
                sel = ps->binding->get_idx (grp->head);
            }
            if (sel != -1) {
                it = ps->binding->get_for_idx (sel);
            }
        }
        if (it) {
            ps->binding->list_context_menu (ps, it, sel);
        }
        int cursor = ps->binding->cursor ();
        if (cursor != -1 && sel != -1) {
            DdbListviewIter it = ps->binding->get_for_idx (cursor);
            ddb_listview_draw_row (ps, cursor, it);
            UNREF (it);
        }
        if (prev != -1 && prev != cursor) {
            DdbListviewIter it = ps->binding->get_for_idx (prev);
            ddb_listview_draw_row (ps, prev, it);
            UNREF (it);
        }
        if (it) {
            UNREF (it);
        }
    }
    return FALSE;
}

gboolean
ddb_listview_list_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
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
    int x = event->x;
    int y = event->y;
    gdk_event_request_motions (event);
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_list_mousemove (ps, event, x, y);
    return FALSE;
}

void
ddb_listview_set_binding (DdbListview *listview, DdbListviewBinding *binding) {
    listview->binding = binding;
}

DdbListviewIter
ddb_listview_get_iter_from_coord (DdbListview *listview, int x, int y) {
    DdbListviewGroup *grp;
    int grp_index;
    int sel;
    DdbListviewIter it = NULL;
    if (ddb_listview_list_pickpoint_y (listview, y + listview->scrollpos, &grp, &grp_index, &sel) != -1) {
        if (sel == -1) {
            sel = listview->binding->get_idx (grp->head);
        }
        it = listview->binding->get_for_idx (sel);
    }
    return it;
}

void
ddb_listview_scroll_to (DdbListview *listview, int pos) {
    pos = ddb_listview_get_row_pos (listview, pos);
    if (pos < listview->scrollpos || pos >= listview->scrollpos + listview->list->allocation.height) {
        gtk_range_set_value (GTK_RANGE (listview->scrollbar), pos - listview->list->allocation.height/2);
    }
}
int
ddb_listview_is_scrolling (DdbListview *listview) {
    return listview->dragwait;
}

/////// column management code

DdbListviewColumn * 
ddb_listview_column_alloc (const char *title, int width, int align_right, int minheight, void *user_data) {
    DdbListviewColumn * c = malloc (sizeof (DdbListviewColumn));
    memset (c, 0, sizeof (DdbListviewColumn));
    c->title = strdup (title);
    c->width = width;
    c->align_right = align_right;
    c->minheight = minheight;
    c->user_data = user_data;
    return c;
}

int
ddb_listview_column_get_count (DdbListview *listview) {
    int cnt = 0;
    DdbListviewColumn *c = listview->columns;
    while (c) {
        cnt++;
        c = c->next;
    }
    return cnt;
}

void
ddb_listview_column_append (DdbListview *listview, const char *title, int width, int align_right, int minheight, void *user_data) {
    DdbListviewColumn* c = ddb_listview_column_alloc (title, width, align_right, minheight, user_data);
    int idx = 0;
    DdbListviewColumn * columns = listview->columns;
    if (columns) {
        idx++;
        DdbListviewColumn * tail = listview->columns;
        while (tail->next) {
            tail = tail->next;
            idx++;
        }
        tail->next = c;
    }
    else {
        listview->columns = c;
    }
    listview->binding->columns_changed (listview);
}

void
ddb_listview_column_insert (DdbListview *listview, int before, const char *title, int width, int align_right, int minheight, void *user_data) {
    DdbListviewColumn *c = ddb_listview_column_alloc (title, width, align_right, minheight ,user_data);
    if (listview->columns) {
        DdbListviewColumn * prev = NULL;
        DdbListviewColumn * next = listview->columns;
        int idx = 0;
        while (next) {
            if (idx == before) {
                break;
            }
            prev = next;
            next = next->next;
            idx++;
        }
        c->next = next;
        if (prev) {
            prev->next = c;
        }
        else {
            listview->columns = c;
        }
    }
    else {
        listview->columns = c;
    }
    listview->binding->columns_changed (listview);
}

void
ddb_listview_column_free (DdbListview *listview, DdbListviewColumn * c) {
    if (c->title) {
        free (c->title);
    }
    listview->binding->col_free_user_data (c->user_data);
    free (c);
}

void
ddb_listview_column_remove (DdbListview *listview, int idx) {
    DdbListviewColumn *c;
    if (idx == 0) {
        c = listview->columns;
        assert (c);
        listview->columns = c->next;
        ddb_listview_column_free (listview, c);
        listview->binding->columns_changed (listview);
        return;
    }
    c = listview->columns;
    int i = 0;
    while (c) {
        if (i+1 == idx) {
            assert (c->next);
            DdbListviewColumn *next = c->next->next;
            ddb_listview_column_free (listview, c->next);
            c->next = next;
            listview->binding->columns_changed (listview);
            return;
        }
        c = c->next;
        i++;
    }

    if (!c) {
        trace ("ddblv: attempted to remove column that is not in list\n");
    }
}

void
ddb_listview_column_move (DdbListview *listview, DdbListviewColumn *which, int inspos) {
    // remove c from list
    DdbListviewColumn *c = (DdbListviewColumn *)which;
    if (c == listview->columns) {
        listview->columns = c->next;
    }
    else {
        DdbListviewColumn *cc;
        for (cc = listview->columns; cc; cc = cc->next) {
            if (cc->next == c) {
                cc->next = c->next;
                break;
            }
        }
    }
    c->next = NULL;
    // reinsert c at position inspos update header_dragging to new idx
    if (inspos == 0) {
        c->next = listview->columns;
        listview->columns = c;
    }
    else {
        int idx = 0;
        DdbListviewColumn *prev = NULL;
        DdbListviewColumn *cc = NULL;
        for (cc = listview->columns; cc; cc = cc->next, idx++, prev = cc) {
            if (idx+1 == inspos) {
                DdbListviewColumn *next = cc->next;
                cc->next = c;
                c->next = next;
                break;
            }
        }
    }
    listview->binding->columns_changed (listview);
}

int
ddb_listview_column_get_info (DdbListview *listview, int col, const char **title, int *width, int *align_right, int *minheight, void **user_data) {
    DdbListviewColumn *c;
    int idx = 0;
    for (c = listview->columns; c; c = c->next, idx++) {
        if (idx == col) {
            *title = c->title;
            *width = c->width;
            *align_right = c->align_right;
            *minheight = c->minheight;
            *user_data = c->user_data;
            return 0;
        }
    }
    return -1;
}

int
ddb_listview_column_set_info (DdbListview *listview, int col, const char *title, int width, int align_right, int minheight, void *user_data) {
    DdbListviewColumn *c;
    int idx = 0;
    for (c = listview->columns; c; c = c->next, idx++) {
        if (idx == col) {
            free (c->title);
            c->title = strdup (title);
            c->width = width;
            c->align_right = align_right;
            c->minheight = minheight;
            c->user_data = user_data;
            listview->binding->columns_changed (listview);
            return 0;
        }
    }
    return -1;
}
/////// end of column management code

/////// grouping /////
void
ddb_listview_free_groups (DdbListview *listview) {
    DdbListviewGroup *next;
    while (listview->groups) {
        next = listview->groups->next;
        if (listview->groups->head) {
            listview->binding->unref (listview->groups->head);
        }
        free (listview->groups);
        listview->groups = next;
    }
}

void
ddb_listview_build_groups (DdbListview *listview) {
    ddb_listview_free_groups (listview);
    listview->fullheight = 0;

    DdbListviewGroup *grp = NULL;
    char str[1024];
    char curr[1024];

    int min_height= 0;
    DdbListviewColumn *c;
    for (c = listview->columns; c; c = c->next) {
        if (c->minheight && c->width > min_height) {
            min_height = c->width;
        }
    }

    listview->grouptitle_height = DEFAULT_GROUP_TITLE_HEIGHT;
    DdbListviewIter it = listview->binding->head ();
    while (it) {
        int res = listview->binding->get_group (it, curr, sizeof (curr));
        if (res == -1) {
            grp = malloc (sizeof (DdbListviewGroup));
            listview->groups = grp;
            memset (grp, 0, sizeof (DdbListviewGroup));
            grp->head = it;
            grp->num_items = listview->binding->count ();
            listview->grouptitle_height = 0;
            grp->height = listview->grouptitle_height + grp->num_items * listview->rowheight;
//            if (grp->height < min_height) {
//                grp->height = min_height;
//            }
            listview->fullheight = grp->height;
            listview->fullheight += listview->grouptitle_height;
            return;
        }
        if (!grp || strcmp (str, curr)) {
            strcpy (str, curr);
            DdbListviewGroup *newgroup = malloc (sizeof (DdbListviewGroup));
            if (grp) {
                if (grp->height - listview->grouptitle_height < min_height) {
                    grp->height = min_height + listview->grouptitle_height;
                }
                listview->fullheight += grp->height;
                grp->next = newgroup;
            }
            else {
                listview->groups = newgroup;
            }
            grp = newgroup;
            memset (grp, 0, sizeof (DdbListviewGroup));
            grp->head = it;
            listview->binding->ref (it);
            grp->num_items = 0;
            grp->height = listview->grouptitle_height;
        }
        grp->height += listview->rowheight;
        grp->num_items++;
        DdbListviewIter next = listview->binding->next (it);
        listview->binding->unref (it);
        it = next;
    }
    if (grp) {
        if (grp->height - listview->grouptitle_height < min_height) {
            grp->height = min_height + listview->grouptitle_height;
        }
        listview->fullheight += grp->height;
    }
}
void
ddb_listview_set_vscroll (DdbListview *listview, gboolean scroll) {
    gtk_range_set_value (GTK_RANGE (listview->scrollbar), scroll);
}

void
ddb_listview_show_header (DdbListview *listview, int show) {
    show ? gtk_widget_show (listview->header) : gtk_widget_hide (listview->header);
}

