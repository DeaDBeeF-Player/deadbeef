/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif

#include <gtk/gtk.h>
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
#include "support.h"
#include "callbacks.h"
#include "actionhandlers.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define DEFAULT_GROUP_TITLE_HEIGHT 30
#define SCROLL_STEP 20
#define AUTOSCROLL_UPDATE_FREQ 0.01f
#define NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW 10

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
    float fwidth; // only in autoresize mode
    int minheight;
    struct _DdbListviewColumn *next;
    int color_override;
    GdkColor color;
    void *user_data;
    unsigned align_right : 2; // 0=left, 1=right, 2=center
    unsigned sort_order : 2; // 0=none, 1=asc, 2=desc
};
typedef struct _DdbListviewColumn DdbListviewColumn;

static void ddb_listview_class_init(DdbListviewClass *klass);
static void ddb_listview_init(DdbListview *listview);
//static void ddb_listview_size_request(GtkWidget *widget, GtkRequisition *requisition);
//static void ddb_listview_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
//static void ddb_listview_realize(GtkWidget *widget);
//static void ddb_listview_paint(GtkWidget *widget);
static void ddb_listview_destroy(GObject *object);

void
ddb_listview_build_groups (DdbListview *listview);

static void
ddb_listview_resize_groups (DdbListview *listview);
// fwd decls
void
ddb_listview_free_groups (DdbListview *listview);

//static inline void
//draw_drawable (GdkDrawable *window, GdkGC *gc, GdkDrawable *drawable, int x1, int y1, int x2, int y2, int w, int h);

////// list functions ////
void
ddb_listview_list_render (DdbListview *ps, cairo_t *cr, int x, int y, int w, int h);
void
ddb_listview_list_render_row_background (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h);
void
ddb_listview_list_render_row_foreground (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int idx, int even, int cursor, int x, int y, int w, int h);
void
ddb_listview_list_render_album_art (DdbListview *ps, cairo_t *cr, DdbListviewIter group_it, int group_pinned, int grp_next_y, int x, int y, int w, int h);
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
ddb_listview_header_render (DdbListview *ps, cairo_t *cr);

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
ddb_listview_header_draw                 (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         user_data);
#if !GTK_CHECK_VERSION(3,0,0)

gboolean
ddb_listview_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);
#endif

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

gboolean
ddb_listview_list_draw               (GtkWidget       *widget,
        cairo_t *cr,
        gpointer         user_data);

void
ddb_listview_list_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
ddb_listview_list_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_list_popup_menu (GtkWidget *widget, gpointer user_data);

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

gboolean
ddb_listview_list_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data);

static void
ddb_listview_class_init(DdbListviewClass *class)
{
  GtkTableClass *widget_class = (GtkTableClass *) class;
  GObjectClass *object_class = (GObjectClass *) class;
  object_class->finalize = ddb_listview_destroy;
}

static void
ddb_listview_init(DdbListview *listview)
{
    // init instance - create all subwidgets, and insert into table
    drawctx_init (&listview->listctx);
    drawctx_init (&listview->grpctx);
    drawctx_init (&listview->hdrctx);
    listview->rowheight = -1;

    listview->col_movepos = -1;
    listview->drag_motion_y = -1;

    listview->ref_point = -1;
    listview->ref_point_offset = -1;

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
    listview->header_width = -1;

    listview->columns = NULL;
    listview->lock_columns = 1;
    listview->groups = NULL;
    listview->plt = NULL;

    listview->block_redraw_on_scroll = 0;
    listview->calculated_grouptitle_height = DEFAULT_GROUP_TITLE_HEIGHT;

    listview->cursor_sz = NULL;
    listview->cursor_drag = NULL;

    listview->area_selection_start = 0;
    listview->area_selection_end = 0;

    listview->cover_size = -1;
    listview->new_cover_size = -1;
    listview->cover_refresh_timeout_id = 0;
    listview->tf_redraw_timeout_id = 0;
    listview->tf_redraw_track_idx = -1;

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

    GtkWidget *sepbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (sepbox);
    gtk_container_set_border_width (GTK_CONTAINER (sepbox), 1);
    gtk_box_pack_start (GTK_BOX (vbox), sepbox, FALSE, TRUE, 0);

    GtkWidget *hsep  = gtk_hseparator_new ();
    gtk_widget_show (hsep);
    gtk_box_pack_start (GTK_BOX (sepbox), hsep, FALSE, TRUE, 0);

    listview->header = gtk_drawing_area_new ();
    gtk_widget_show (listview->header);
    gtk_box_pack_start (GTK_BOX (vbox), listview->header, FALSE, TRUE, 0);
    gtk_widget_set_events (listview->header, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    listview->list = gtk_drawing_area_new ();
    gtk_widget_show (listview->list);
    gtk_box_pack_start (GTK_BOX (vbox), listview->list, TRUE, TRUE, 0);
    gtk_widget_set_can_focus (listview->list, TRUE);
    gtk_widget_set_can_default (listview->list, TRUE);
    int events = GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK;
#if GTK_CHECK_VERSION(3,0,0)
    events |= GDK_SCROLL_MASK;
#endif
#if GTK_CHECK_VERSION(3,4,0)
    events |= GDK_SMOOTH_SCROLL_MASK;
#endif
    gtk_widget_set_events (listview->list, events);

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
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect ((gpointer) listview->header, "expose_event",
            G_CALLBACK (ddb_listview_header_expose_event),
            NULL);
#else
    g_signal_connect ((gpointer) listview->header, "draw",
            G_CALLBACK (ddb_listview_header_draw),
            NULL);
#endif
    g_signal_connect ((gpointer) listview->header, "configure_event",
            G_CALLBACK (ddb_listview_header_configure_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "realize",
            G_CALLBACK (ddb_listview_header_realize),
            NULL);
    g_signal_connect ((gpointer) listview->header, "motion_notify_event",
            G_CALLBACK (ddb_listview_header_motion_notify_event),
            NULL);
    g_signal_connect_after ((gpointer) listview->header, "button_press_event",
            G_CALLBACK (ddb_listview_header_button_press_event),
            NULL);
    g_signal_connect ((gpointer) listview->header, "button_release_event",
            G_CALLBACK (ddb_listview_header_button_release_event),
            NULL);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect ((gpointer) listview->list, "expose_event",
            G_CALLBACK (ddb_listview_list_expose_event),
            NULL);
#else
    g_signal_connect ((gpointer) listview->list, "draw",
            G_CALLBACK (ddb_listview_list_draw),
            NULL);
#endif
    g_signal_connect ((gpointer) listview->list, "realize",
            G_CALLBACK (ddb_listview_list_realize),
            NULL);
    g_signal_connect_after ((gpointer) listview->list, "button_press_event",
            G_CALLBACK (ddb_listview_list_button_press_event),
            NULL);
    g_signal_connect ((gpointer) listview->list, "popup_menu",
            G_CALLBACK (ddb_listview_list_popup_menu),
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

    g_signal_connect ((gpointer)listview->list, "key_press_event", G_CALLBACK (ddb_listview_list_key_press_event), NULL);
}

GtkWidget * ddb_listview_new()
{
   return g_object_newv (ddb_listview_get_type(), 0, NULL);//GTK_WIDGET(gtk_type_new(ddb_listview_get_type()));
}

static void
ddb_listview_destroy(GObject *object)
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
  if (listview->group_format) {
      free (listview->group_format);
      listview->group_format = NULL;
  }
  if (listview->group_title_bytecode) {
      free (listview->group_title_bytecode);
      listview->group_title_bytecode = NULL;
  }
  ddb_listview_cancel_autoredraw (listview);

  draw_free (&listview->listctx);
  draw_free (&listview->grpctx);
  draw_free (&listview->hdrctx);
}

void
ddb_listview_refresh (DdbListview *listview, uint32_t flags) {
    if (flags & DDB_LIST_CHANGED) {
        ddb_listview_build_groups (listview);
    }
    if (flags & DDB_REFRESH_LIST) {
        gtk_widget_queue_draw (listview->list);
    }
    if (flags & DDB_REFRESH_VSCROLL) {
        ddb_listview_list_setup_vscroll (listview);
    }
    if (flags & DDB_REFRESH_HSCROLL) {
        ddb_listview_list_setup_hscroll (listview);
    }
    if (flags & DDB_REFRESH_COLUMNS) {
        gtk_widget_queue_draw (listview->header);
    }
}

void
ddb_listview_list_realize                    (GtkWidget       *widget,
        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (!ps->binding->drag_n_drop) {
        // playlist doesn't support drag and drop (e.g. searchlist)
        return;
    }
    GtkTargetEntry entry = {
        .target = TARGET_PLAYITEMS,
        .flags = GTK_TARGET_SAME_APP,
        .info = TARGET_SAMEWIDGET
    };
    // setup drag-drop target
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, &entry, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_drag_dest_add_uri_targets (widget);
//    gtk_drag_dest_set_track_motion (widget, TRUE);
}

static gboolean
ddb_listview_reconf_scrolling (void *ps) {
    ddb_listview_list_setup_vscroll (ps);
    ddb_listview_list_setup_hscroll (ps);
    return FALSE;
}

static void
ddb_listview_list_update_total_width (DdbListview *lv, int size) {
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (lv->list), &a);
    lv->totalwidth = size;
    if (lv->totalwidth < a.width) {
        lv->totalwidth = a.width;
    }
}

gboolean
ddb_listview_list_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));

    draw_init_font (&ps->listctx, DDB_LIST_FONT, 1);
    draw_init_font (&ps->grpctx, DDB_GROUP_FONT, 1);
    ddb_listview_update_fonts (ps);

    return FALSE;
}

void
ddb_listview_groupcheck (DdbListview *listview) {
    int idx = listview->binding->modification_idx ();
    if (idx != listview->groups_build_idx) {
        ddb_listview_build_groups (listview);
    }
}

// returns 1 if X coordinate in list belongs to album art column and 0 if not
int
ddb_listview_is_album_art_column (DdbListview *listview, int x)
{
    int album_art_column = 0;
    int col_x = -listview->hscrollpos;
    int cnt = ddb_listview_column_get_count (listview);
    for (int i = 0; i < cnt, col_x <= x; i++) {
        const char *title;
        int width;
        int align_right;
        col_info_t *info;
        int minheight;
        int color_override;
        GdkColor color;
        int res = ddb_listview_column_get_info (listview, i, &title, &width, &align_right, &minheight, &color_override, &color, (void **)&info);
        if (res != -1 && x <= col_x + width && info->id == DB_COLUMN_ALBUM_ART) {
            return 1;
        }
        col_x += width;
    }
    return 0;
}

// returns 1 if column is album art column
int
ddb_listview_is_album_art_column_idx (DdbListview *listview, int cidx)
{
    const char *title;
    int width;
    int align_right;
    col_info_t *info;
    int minheight;
    int color_override;
    GdkColor color;
    int res = ddb_listview_column_get_info (listview, cidx, &title, &width, &align_right, &minheight, &color_override, &color, (void **)&info);
    if (res != -1 && info->id == DB_COLUMN_ALBUM_ART) {
        return 1;
    }
    return 0;
}

// returns Y coordinate of an item by its index
int
ddb_listview_get_row_pos (DdbListview *listview, int row_idx) {
    int y = 0;
    int idx = 0;
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
    DdbListviewGroup *grp = listview->groups;
    while (grp) {
        if (idx + grp->num_items > row_idx) {
            int i = y + listview->grouptitle_height + (row_idx - idx) * listview->rowheight;
            deadbeef->pl_unlock ();
            return i;
        }
        y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }
    deadbeef->pl_unlock ();
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
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
    DdbListviewGroup *grp = listview->groups;
    while (grp) {
        int h = grp->height;
        if (y >= grp_y && y < grp_y + h) {
            *group = grp;
            y -= grp_y;
            if (y < listview->grouptitle_height) {
                *group_idx = -1;
                *global_idx = idx;
            }
            else if (y >= listview->grouptitle_height + grp->num_items * listview->rowheight) {
                *group_idx = (y - listview->grouptitle_height) / listview->rowheight;
                *global_idx = -1;
            }
            else {
                *group_idx = (y - listview->grouptitle_height) / listview->rowheight;
                *global_idx = idx + *group_idx;
            }
            deadbeef->pl_unlock ();
            return 0;
        }
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
        gidx++;
    }
    deadbeef->pl_unlock ();
    return -1;
}

int render_idx = 0;

void
ddb_listview_list_render (DdbListview *listview, cairo_t *cr, int x, int y, int w, int h) {
    render_idx = 0;
    cairo_set_line_width (cr, 1);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    GtkWidget *treeview = theme_treeview;

#if !GTK_CHECK_VERSION(3,0,0)
// FIXME?
    if (gtk_widget_get_style (treeview)->depth == -1) {
        return; // drawing was called too early
    }
#endif
    int idx = 0;
    int abs_idx = 0;
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
    // find 1st group
    DdbListviewGroup *grp = listview->groups;
    int grp_y = 0;
    int grp_next_y = 0;
    DdbListviewGroup *pinned_grp = NULL;

    while (grp && grp_y + grp->height < y + listview->scrollpos) {
        if (grp_y < listview->scrollpos && grp_y + grp->height >= listview->scrollpos) {
            pinned_grp = grp;
            grp->pinned = 1;
        }
        grp_y += grp->height;
        idx += grp->num_items + 1;
        abs_idx += grp->num_items;
        grp = grp->next;
    }

    draw_begin (&listview->listctx, cr);
    draw_begin (&listview->grpctx, cr);

    if (grp && !pinned_grp && grp_y < listview->scrollpos) {
        grp->pinned = 1;
        pinned_grp = grp;
    }
    else if (grp && pinned_grp && pinned_grp->next == grp) {
        grp->pinned = 2;
    }

    int ii = 0;
    while (grp && grp_y < y + h + listview->scrollpos) {
        DdbListviewIter it = grp->head;
        int grp_height = listview->grouptitle_height + grp->num_items * listview->rowheight;
        int grp_height_total = grp->height;
        int group_idx = idx;

        if (grp_y >= y + h + listview->scrollpos) {
            break;
        }
        listview->binding->ref (it);

        grp_next_y = grp_y + grp_height_total;
        for (int i = 0; i < grp->num_items; i++) {
            ii++;
            int grp_row_y = grp_y + listview->grouptitle_height + i * listview->rowheight;
            if (grp_row_y >= y + h + listview->scrollpos) {
                break;
            }
            if (grp_y + listview->grouptitle_height + (i+1) * listview->rowheight >= y + listview->scrollpos
                    && grp_row_y < y + h + listview->scrollpos) {
                GtkStyle *st = gtk_widget_get_style (listview->list);
                gdk_cairo_set_source_color (cr, &st->bg[GTK_STATE_NORMAL]);
                cairo_rectangle (cr, -listview->hscrollpos, grp_row_y - listview->scrollpos, listview->totalwidth, listview->rowheight);
                cairo_fill (cr);
                ddb_listview_list_render_row_background (listview, cr, it, i & 1, (abs_idx+i) == listview->binding->cursor () ? 1 : 0, -listview->hscrollpos, grp_row_y - listview->scrollpos, listview->totalwidth, listview->rowheight);
                ddb_listview_list_render_row_foreground (listview, cr, it, abs_idx + i, (idx + 1 + i) & 1, (idx+i) == listview->binding->cursor () ? 1 : 0, -listview->hscrollpos, grp_row_y - listview->scrollpos, listview->totalwidth, listview->rowheight);
            }
            DdbListviewIter next = listview->binding->next (it);
            listview->binding->unref (it);
            it = next;
            if (!it) {
                break; // sanity check, in case groups were not rebuilt yet
            }
        }
        idx += grp->num_items + 1;
        abs_idx += grp->num_items;

        int filler = grp_height_total - (grp_height);
        if (filler > 0) {
            int theming = !gtkui_override_listview_colors ();
            if (theming) {
#if GTK_CHECK_VERSION(3,0,0)
                gtk_paint_flat_box (gtk_widget_get_style (treeview), cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, treeview, "cell_even_ruled", x, grp_y - listview->scrollpos + grp_height, w, filler);
#else
                gtk_paint_flat_box (gtk_widget_get_style (treeview), gtk_widget_get_window (listview->list), GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, treeview, "cell_even_ruled", x, grp_y - listview->scrollpos + grp_height, w, filler);
#endif
            }
            else {
                GdkColor clr;
                gtkui_get_listview_even_row_color (&clr);
                gdk_cairo_set_source_color (cr, &clr);
                cairo_rectangle (cr, x, grp_y - listview->scrollpos + grp_height, w, filler);
                cairo_fill (cr);
            }
        }

        // draw album art
        ddb_listview_list_render_album_art (listview, cr, grp->head, grp->pinned, grp_next_y - listview->scrollpos, -listview->hscrollpos, grp_y + listview->grouptitle_height - listview->scrollpos, listview->totalwidth, grp_height_total);
        if (grp->pinned == 1 && gtkui_groups_pinned && y <= 0) {
            // draw pinned group title
            int pushback = 0;
            if (grp_next_y - listview->scrollpos <= listview->grouptitle_height) {
                pushback = listview->grouptitle_height - (grp_next_y - listview->scrollpos);
            }
            ddb_listview_list_render_row_background (listview, cr, NULL, 1, 0, -listview->hscrollpos, y - pushback, listview->totalwidth, listview->grouptitle_height);
            if (listview->binding->draw_group_title && listview->grouptitle_height > 0) {
                listview->binding->draw_group_title (listview, cr, grp->head, PL_MAIN, -listview->hscrollpos, y - pushback, listview->totalwidth, listview->grouptitle_height);
            }
        }
        else if (grp_y + listview->grouptitle_height >= y + listview->scrollpos && grp_y < y + h + listview->scrollpos) {
            // draw normal group title
            ddb_listview_list_render_row_background (listview, cr, NULL, 1, 0, -listview->hscrollpos, grp_y - listview->scrollpos, listview->totalwidth, listview->grouptitle_height);
            if (listview->binding->draw_group_title && listview->grouptitle_height > 0) {
                listview->binding->draw_group_title (listview, cr, grp->head, PL_MAIN, -listview->hscrollpos, grp_y - listview->scrollpos, listview->totalwidth, listview->grouptitle_height);
            }
        }

        if (it) {
            listview->binding->unref (it);
        }
        grp_y += grp_height_total;
        if (grp->pinned == 1) {
            grp = grp->next;
            if (grp) {
                grp->pinned = 2;
            }
        }
        else {
            grp = grp->next;
            if (grp) {
                grp->pinned = 0;
            }
        }
    }
    if (grp_y < y + h + listview->scrollpos) {
        int hh = y + h - (grp_y - listview->scrollpos);
//        gdk_draw_rectangle (listview->list->window, listview->list->style->bg_gc[GTK_STATE_NORMAL], TRUE, x, grp_y - listview->scrollpos, w, hh);
        int theming = !gtkui_override_listview_colors ();
        if (theming) {
#if GTK_CHECK_VERSION(3,0,0)
            gtk_paint_flat_box (gtk_widget_get_style (treeview), cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, treeview, "cell_even_ruled", x, grp_y - listview->scrollpos, w, hh);
#else
            gtk_paint_flat_box (gtk_widget_get_style (treeview), listview->list->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, treeview, "cell_even_ruled", x, grp_y - listview->scrollpos, w, hh);
#endif
        }
        else {
            GdkColor clr;
            gtkui_get_listview_even_row_color (&clr);
            cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);
            cairo_rectangle (cr, x, grp_y - listview->scrollpos, w, hh);
            cairo_fill (cr);
        }
    }
    deadbeef->pl_unlock ();
    draw_end (&listview->listctx);
    draw_end (&listview->grpctx);
}

static void
ddb_listview_draw_dnd_marker (DdbListview *ps, cairo_t *cr) {
    if (ps->drag_motion_y < 0) {
        return;
    }
    int drag_motion_y = ps->drag_motion_y - ps->scrollpos;

    GtkWidget *widget = ps->list;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    GdkColor clr;
    gtkui_get_listview_cursor_color (&clr);
    cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.0);
    cairo_rectangle (cr, 0, drag_motion_y-1, a.width, 3);
    cairo_fill (cr);
    cairo_rectangle (cr, 0, drag_motion_y-3, 3, 7);
    cairo_fill (cr);
    cairo_rectangle (cr, a.width-3, drag_motion_y-3, 3, 7);
    cairo_fill (cr);

}

void
ddb_listview_update_fonts (DdbListview *ps)
{
    draw_init_font (&ps->listctx, DDB_LIST_FONT, 1);
    draw_init_font (&ps->grpctx, DDB_GROUP_FONT, 1);
    int row_height = draw_get_listview_rowheight (&ps->listctx);
    int grptitle_height = draw_get_listview_rowheight (&ps->grpctx);
    if (row_height != ps->rowheight || grptitle_height != ps->calculated_grouptitle_height) {
        ps->rowheight = row_height;
        ps->calculated_grouptitle_height = grptitle_height;
        ddb_listview_build_groups (ps);
    }

    GtkAllocation a;
    gtk_widget_get_allocation (ps->list, &a);
    int w = a.width;
    int size = 0;
    DdbListviewColumn *c;
    for (c = ps->columns; c; c = c->next) {
        size += c->width;
    }
    ddb_listview_list_update_total_width (ps, size);
    g_idle_add (ddb_listview_reconf_scrolling, ps);
}

#if GTK_CHECK_VERSION(3,0,0)
gboolean
ddb_listview_list_draw               (GtkWidget       *widget,
        cairo_t *cr,
        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    widget = ps->list;

    // FIXME: clip region
    ddb_listview_list_render (ps, cr, 0, 0, gtk_widget_get_allocated_width (widget), gtk_widget_get_allocated_height (widget));
    if (ps->drag_motion_y >= 0/* && ps->drag_motion_y-ps->scrollpos-3 < event->area.y+event->area.height && ps->drag_motion_y-ps->scrollpos+3 >= event->area.y*/) {
        ddb_listview_draw_dnd_marker (ps, cr);
    }
    return FALSE;
}
#else
gboolean
ddb_listview_list_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    widget = ps->list;

    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    ddb_listview_list_render (ps, cr, event->area.x, event->area.y, event->area.width, event->area.height);
    if (ps->drag_motion_y >= 0 && ps->drag_motion_y-ps->scrollpos-3 < event->area.y+event->area.height && ps->drag_motion_y-ps->scrollpos+3 >= event->area.y) {
        ddb_listview_draw_dnd_marker (ps, cr);
    }
    cairo_destroy (cr);
    return FALSE;
}
#endif

gboolean
ddb_listview_vscroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));

	GdkEventScroll *ev = (GdkEventScroll*)event;

    GtkWidget *rangeh = ps->hscrollbar;
    GtkWidget *rangev = ps->scrollbar;

    gdouble deltah = SCROLL_STEP * 2;
    gdouble deltav = SCROLL_STEP * 2;
    gdouble scrollh = gtk_range_get_value (GTK_RANGE (rangeh));
    gdouble scrollv = gtk_range_get_value (GTK_RANGE (rangev));
    // pass event to scrollbar
    if (ev->direction == GDK_SCROLL_UP) {
        gtk_range_set_value (GTK_RANGE (rangev), scrollv - deltav);
    }
    else if (ev->direction == GDK_SCROLL_DOWN) {
        gtk_range_set_value (GTK_RANGE (rangev), scrollv + deltav);
    }
    else if (ev->direction == GDK_SCROLL_LEFT) {
        gtk_range_set_value (GTK_RANGE (rangeh), scrollh - deltah);
    }
    else if (ev->direction == GDK_SCROLL_RIGHT) {
        gtk_range_set_value (GTK_RANGE (rangeh), scrollh + deltah);
    }
#if GTK_CHECK_VERSION(3,4,0)
    else if (ev->direction == GDK_SCROLL_SMOOTH) {
        gdouble x, y;
        if (gdk_event_get_scroll_deltas(event, &x, &y)) {
            gtk_range_set_value (GTK_RANGE (rangeh), scrollh + deltah * x);
            gtk_range_set_value (GTK_RANGE (rangev), scrollv + deltav * y);
        }
    }
#endif

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
        ps->scrollpos = newscroll;
        gtk_widget_queue_draw (ps->list);
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


gboolean
ddb_listview_list_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    return TRUE;
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
            GdkAtom target = gtk_selection_data_get_target (selection_data);
            gtk_selection_data_set (selection_data, target, sizeof (uint32_t) * 8, (gchar *)ptr, (nsel+1) * sizeof (uint32_t));
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
    ps->scroll_direction = 0; // interrupt autoscrolling, if on
    ps->scroll_active = 0;
    ps->drag_motion_y = -1;
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
    gchar *ptr=(char*)gtk_selection_data_get_data (data);
    gint len = gtk_selection_data_get_length (data);
    if (target_type == TARGET_URILIST) { // uris
        // this happens when dropped from file manager
        char *mem = malloc (len+1);
        memcpy (mem, ptr, len);
        mem[len] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        ps->binding->external_drag_n_drop (it, mem, len);
        if (it) {
            UNREF (it);
        }
    }
    else if (target_type == TARGET_SAMEWIDGET && gtk_selection_data_get_format(data) == 32) { // list of 32bit ints, DDB_URI_LIST target
        uint32_t *d= (uint32_t *)ptr;
        int plt = *d;
        d++;
        int length = (len/4)-1;
        DdbListviewIter drop_before = it;
        // find last selected
        if (plt == deadbeef->plt_get_curr_idx ()) {
            while (drop_before && ps->binding->is_selected (drop_before)) {
                DdbListviewIter next = PL_NEXT(drop_before);
                UNREF (drop_before);
                drop_before = next;
            }
        }
        while (plt == deadbeef->plt_get_curr_idx () && drop_before && ps->binding->is_selected (drop_before)) {
        }
        ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
        if (p) {
            // FIXME
            ps->binding->drag_n_drop (drop_before, p, d, length, gdk_drag_context_get_selected_action (drag_context) == GDK_ACTION_COPY ? 1 : 0);
            deadbeef->plt_unref (p);
        }
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
//static inline void
//draw_drawable (GdkDrawable *window, GdkGC *gc, GdkDrawable *drawable, int x1, int y1, int x2, int y2, int w, int h) {
//    gint width1, height1;
//    gint width2, height2;
//    gdk_drawable_get_size (window, &width1, &height1);
//    gdk_drawable_get_size (drawable, &width2, &height2);
////    assert (y1 >= 0 && y1 + h < height2);
////    assert (y2 >= 0 && y2 + h < height1);
////    printf ("dd: %p %p %p %d %d %d %d %d %d\n", window, gc, drawable, x1, y1, x2, y2, w, h);
//    gdk_draw_drawable (window, gc, drawable, x1, y1, x2, y2, w, h);
//}

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

void
ddb_listview_list_setup_vscroll (DdbListview *ps) {
    ddb_listview_groupcheck (ps);
    GtkWidget *list = ps->list;
    GtkWidget *scroll = ps->scrollbar;
    int vheight = ps->fullheight;
    GtkAllocation a;
    gtk_widget_get_allocation (ps->list, &a);
    if (ps->fullheight <= a.height) {
        gtk_widget_hide (scroll);
        ps->scrollpos = 0;
        gtk_widget_queue_draw (ps->list);
    }
    else {
        gtk_widget_show (scroll);
        if (ps->scrollpos >= vheight - a.height) {
            ps->scrollpos = vheight - a.height;
        }
    }
    int h = a.height;
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, vheight, SCROLL_STEP, h/2, h);
    gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
    gtk_range_set_value (GTK_RANGE (scroll), ps->scrollpos);
}

void
ddb_listview_list_setup_hscroll (DdbListview *ps) {
    GtkWidget *list = ps->list;
    GtkAllocation a;
    gtk_widget_get_allocation (ps->list, &a);
    int w = a.width;
    int size = 0;
    DdbListviewColumn *c;
    for (c = ps->columns; c; c = c->next) {
        size += c->width;
    }
    ddb_listview_list_update_total_width (ps, size);
    GtkWidget *scroll = ps->hscrollbar;
    if (w >= size) {
        gtk_widget_hide (scroll);
        ps->hscrollpos = 0;
        gtk_widget_queue_draw (ps->list);
    }
    else {
        if (ps->hscrollpos >= size-w) {
            int n = size-w-1;
            ps->hscrollpos = max (0, n);
            gtk_range_set_value (GTK_RANGE (scroll), ps->hscrollpos);
        }
        gtk_widget_show (scroll);
    }
    GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new (gtk_range_get_value (GTK_RANGE (scroll)), 0, size, 1, w, w);
    gtk_range_set_adjustment (GTK_RANGE (scroll), adj);
}

// returns -1 if row not found
int
ddb_listview_list_get_drawinfo (DdbListview *listview, int row, DdbListviewGroup **pgrp, int *even, int *cursor, int *group_y, int *x, int *y, int *w, int *h) {
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
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
            deadbeef->pl_unlock ();
            return 0;
        }
        *y += grpheight;
        idx += grp->num_items;
        idx2 += grp->num_items + 1;
        grp = grp->next;
    }
    deadbeef->pl_unlock ();
    return -1;
}

void
ddb_listview_draw_row (DdbListview *listview, int row, DdbListviewIter it) {
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

    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (listview->list), &a);

    if (y > a.height) {
        return;
    }
    gtk_widget_queue_draw_area (listview->list, 0, y, a.width, h);
}

// coords passed are window-relative
void
ddb_listview_list_render_row_background (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h) {
    // draw background
    GtkWidget *treeview = theme_treeview;
    int theming = !gtkui_override_listview_colors ();

    if (theming) {
#if !GTK_CHECK_VERSION(3,0,0)
        if (gtk_widget_get_style (treeview)->depth == -1) {
            return; // drawing was called too early
        }
#endif
    }
    int sel = it && ps->binding->is_selected (it);

    if (theming || !sel) {
        if (theming) {
            // draw background for selection -- workaround for New Wave theme (translucency)
#if GTK_CHECK_VERSION(3,0,0)
            gtk_paint_flat_box (gtk_widget_get_style (treeview), cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, treeview, even ? "cell_even_ruled" : "cell_odd_ruled", x, y, w, h);
#else
            gtk_paint_flat_box (gtk_widget_get_style (treeview), ps->list->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, treeview, even ? "cell_even_ruled" : "cell_odd_ruled", x, y, w, h);
#endif
        }
        else {
            GdkColor clr;
            even ? gtkui_get_listview_even_row_color (&clr) : gtkui_get_listview_odd_row_color (&clr);
            gdk_cairo_set_source_color (cr, &clr);
            cairo_rectangle (cr, x, y, w, h);
            cairo_fill (cr);
        }
    }

    if (sel) {
        if (theming) {
#if GTK_CHECK_VERSION(3,0,0)
            gtk_paint_flat_box (gtk_widget_get_style (treeview), cr, GTK_STATE_SELECTED, GTK_SHADOW_NONE, treeview, even ? "cell_even_ruled" : "cell_odd_ruled", x-1, y-1, w+1, h+1);
#else
            gtk_paint_flat_box (gtk_widget_get_style (treeview), ps->list->window, GTK_STATE_SELECTED, GTK_SHADOW_NONE, NULL, treeview, even ? "cell_even_ruled" : "cell_odd_ruled", x, y, w, h);
            //            if (gtk_widget_has_focus (ps->list)) {
            //                gtk_paint_focus (gtk_widget_get_style (treeview), ps->list->window, GTK_STATE_SELECTED, NULL, treeview, "treeview", x, y, w, h);
            //            }
#endif
        }
        else {
            GdkColor clr;
#if !GTK_CHECK_VERSION(3,0,0)
            GdkGC *gc = gdk_gc_new (ps->list->window);
            gdk_gc_set_rgb_fg_color (gc, (gtkui_get_listview_selection_color (&clr), &clr));
            gdk_draw_rectangle (ps->list->window, gc, TRUE, x, y, w, h);
            g_object_unref (gc);

#else
            gtkui_get_listview_selection_color (&clr);
            cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);
            cairo_rectangle (cr, x, y, w, h);
            cairo_fill (cr);
#endif
        }
    }
    if (cursor && gtk_widget_has_focus (ps->list)) {
        // not all gtk engines/themes render focus rectangle in treeviews
        // but we want it anyway
        //treeview->style->fg_gc[GTK_STATE_NORMAL]
        GdkColor clr;
#if !GTK_CHECK_VERSION(3,0,0)
        GdkGC *gc = gdk_gc_new (ps->list->window);
        gdk_gc_set_rgb_fg_color (gc, (gtkui_get_listview_cursor_color (&clr), &clr));
        gdk_draw_rectangle (ps->list->window, gc, FALSE, x, y, w-1, h-1);
        g_object_unref (gc);
#else
        gtkui_get_listview_cursor_color (&clr);
        cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);
        cairo_rectangle (cr, x+1, y+1, w-1, h-1);
        cairo_stroke (cr);
#endif
    }
}

void
ddb_listview_list_render_row_foreground (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int idx, int even, int cursor, int x, int y, int w, int h) {
    int width, height;
    GtkAllocation a;
    gtk_widget_get_allocation (ps->list, &a);
    width = a.width;
    height = a.height;
    if (it && ps->binding->is_selected (it)) {
        GdkColor *clr = &gtk_widget_get_style (theme_treeview)->fg[GTK_STATE_SELECTED];
        float rgb[3] = { clr->red/65535.f, clr->green/65535.f, clr->blue/65535.f };
        draw_set_fg_color (&ps->listctx, rgb);
    }
    else {
        GdkColor *clr = &gtk_widget_get_style (theme_treeview)->fg[GTK_STATE_NORMAL];
        float rgb[3] = { clr->red/65535.f, clr->green/65535.f, clr->blue/65535.f };
        draw_set_fg_color (&ps->listctx, rgb);
    }
    DdbListviewColumn *c;
    int cidx = 0;
    for (c = ps->columns; c; c = c->next, cidx++) {
        int cw = c->width;
        if (!ddb_listview_is_album_art_column_idx (ps, cidx)) {
            ps->binding->draw_column_data (ps, cr, it, idx, cidx, PL_MAIN, x, y, cw, h);
        }
        x += cw;
    }
}

void
ddb_listview_list_render_album_art (DdbListview *ps, cairo_t *cr, DdbListviewIter group_it, int group_pinned, int grp_next_y, int x, int y, int w, int h) {
    DdbListviewColumn *c;
    int cidx = 0;
    for (c = ps->columns; c; c = c->next, cidx++) {
        int cw = c->width;
        if (ddb_listview_is_album_art_column_idx (ps, cidx)) {
            ps->binding->draw_album_art (ps, cr, ps->grouptitle_height > 0 ? group_it : NULL, cidx, group_pinned, grp_next_y, x, y, cw, h);
        }
        x += cw;
    }
}

void
ddb_listview_header_expose (DdbListview *ps, cairo_t *cr, int x, int y, int w, int h) {
    ddb_listview_header_render (ps, cr);
}

void
ddb_listview_select_single (DdbListview *ps, int sel) {
    int nchanged = 0;
    deadbeef->pl_lock ();

    DdbListviewIter sel_it = ps->binding->get_for_idx (sel);
    if (!sel_it) {
        deadbeef->pl_unlock ();
        return;
    }

    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        int selected = deadbeef->pl_is_selected (it);
        if (selected) {
            deadbeef->pl_set_selected (it, 0);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        UNREF (it);
        it = next;
    }
    UNREF (it);

    ps->binding->select (sel_it, 1);

    UNREF (sel_it);
    deadbeef->pl_unlock ();

    ddb_listview_refresh (ps, DDB_REFRESH_LIST);
    ps->binding->selection_changed (ps, NULL, -1); // that means "selection changed a lot, redraw everything"
    ps->area_selection_start = sel;
    ps->area_selection_end = sel;
}

void
ddb_listview_click_selection (DdbListview *ps, int ex, int ey, DdbListviewGroup *grp, int grp_index, int sel, int dnd, int button) {
    deadbeef->pl_lock ();
    ps->areaselect = 0;
    ddb_listview_groupcheck (ps);

    // clicked album art column?
    int album_art_column = ddb_listview_is_album_art_column (ps, ex);

    if (sel == -1 && !album_art_column && (!grp || (ey > ps->grouptitle_height && grp_index >= grp->num_items))) {
        // clicked empty space, deselect everything
        DdbListviewIter it;
        int idx = 0;
        for (it = ps->binding->head (); it; idx++) {
            if (ps->binding->is_selected (it)) {
                ps->binding->select (it, 0);
                ddb_listview_draw_row (ps, idx, it);
                ps->binding->selection_changed (ps, it, idx);
            }
            DdbListviewIter next = ps->binding->next (it);
            ps->binding->unref (it);
            it = next;
        }
    }
    else if ((sel != -1 && grp && grp_index == -1) || (ey <= ps->grouptitle_height && gtkui_groups_pinned) || album_art_column) {
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
                    ps->binding->selection_changed (ps, it, idx);
                }
                cnt--;
            }
            else {
                if (ps->binding->is_selected (it)) {
                    ps->binding->select (it, 0);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (ps, it, idx);
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
        if (!it || !ps->binding->is_selected (it)
                || (!ps->binding->drag_n_drop && button == 1)) // HACK: don't reset selection by right click in search window
        {
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
    deadbeef->pl_unlock ();
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
ddb_listview_list_mouse1_pressed (DdbListview *ps, int state, int ex, int ey, GdkEventType type) {
    // cursor must be set here, but selection must be handled in keyrelease
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (ps);
    int cnt = ps->binding->count ();
    if (cnt == 0) {
        deadbeef->pl_unlock ();
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
        deadbeef->pl_unlock ();
        return;
    }

    int cursor = ps->binding->cursor ();
    if (type == GDK_2BUTTON_PRESS
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
            deadbeef->pl_unlock ();
            return;
        }
    }

    int prev = cursor;
    if (sel != -1) {
        // pick 1st item in group in case album art column was clicked
        if (ddb_listview_is_album_art_column (ps, ex) && grp_index != -1) {
            sel -= grp_index;
        }

        ps->binding->set_cursor (sel);
        DdbListviewIter it = ps->binding->get_for_idx (sel);
        if (it) {
            ddb_listview_draw_row (ps, sel, it);
            UNREF (it);
        }
        ps->shift_sel_anchor = ps->binding->cursor ();
    }
    // handle multiple selection
#ifndef __APPLE__
    int selmask = GDK_CONTROL_MASK;
#else
    int selmask = GDK_MOD2_MASK;
#endif
    if (!(state & (selmask|GDK_SHIFT_MASK)))
    {
        ddb_listview_click_selection (ps, ex, ey, grp, grp_index, sel, 1, 1);
    }
    else if (state & selmask) {
        // toggle selection
        if (sel != -1) {
            DdbListviewIter it = ps->binding->get_for_idx (sel);
            if (it) {
                ps->binding->select (it, 1 - ps->binding->is_selected (it));
                ddb_listview_draw_row (ps, sel, it);
                ps->binding->selection_changed (ps, it, sel);
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
                    ps->binding->selection_changed (ps, it, idx);
                }
            }
            else {
                if (ps->binding->is_selected (it)) {
                    ps->binding->select (it, 0);
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (ps, it, idx);
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
    deadbeef->pl_unlock ();
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
                    ps->binding->selection_changed (ps, it, idx);
                    DdbListviewIter next = PL_NEXT(it);
                    UNREF (it);
                    it = next;
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
    // erase previous rect using 4 blits from ps->list->windowfer
    if (areaselect_dx != -1) {
        int sx = min (areaselect_x, areaselect_dx);
        int sy = min (areaselect_y, areaselect_dy);
        int dx = max (areaselect_x, areaselect_dx);
        int dy = max (areaselect_y, areaselect_dy);
        int w = dx - sx + 1;
        int h = dy - sy + 1;
        //draw_drawable (widget->window, widget->style->black_gc, ps->list->window, sx, sy, sx, sy, dx - sx + 1, dy - sy + 1);
        draw_drawable (widget->window, widget->style->black_gc, ps->list->window, sx, sy, sx, sy, w, 1);
        draw_drawable (widget->window, widget->style->black_gc, ps->list->window, sx, sy, sx, sy, 1, h);
        draw_drawable (widget->window, widget->style->black_gc, ps->list->window, sx, sy + h - 1, sx, sy + h - 1, w, 1);
        draw_drawable (widget->window, widget->style->black_gc, ps->list->window, sx + w - 1, sy, sx + w - 1, sy, 1, h);
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
    deadbeef->pl_lock ();
    if (ps->dragwait) {
        GtkWidget *widget = ps->list;
        if (gtk_drag_check_threshold (widget, ps->lastpos[0], ps->lastpos[1], ex, ey)) {
            ps->dragwait = 0;
            ps->drag_source_playlist = deadbeef->plt_get_curr_idx ();
            GtkTargetEntry entry = {
                .target = TARGET_PLAYITEMS,
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
                ddb_listview_groupcheck (ps);
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

            int nchanged = 0;

            // don't touch anything in process_start/end range
            int process_start = min (start, ps->area_selection_start);
            int process_end = max (end, ps->area_selection_end);

            idx=process_start;
            DdbListviewIter it = ps->binding->get_for_idx (idx);
            for (; it && idx <= process_end; idx++) {
                int selected = ps->binding->is_selected (it);
                if (idx >= start && idx <= end) {
                    if (!selected) {
                        ps->binding->select (it, 1);
                        nchanged++;
                        if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                            ddb_listview_draw_row (ps, idx, it);
                            ps->binding->selection_changed (ps, it, idx);
                        }
                    }
                }
                else if (selected) {
                    ps->binding->select (it, 0);
                    nchanged++;
                    if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                        ddb_listview_draw_row (ps, idx, it);
                        ps->binding->selection_changed (ps, it, idx);
                    }
                }
                DdbListviewIter next = PL_NEXT(it);
                UNREF (it);
                it = next;
            }
            UNREF (it);
            if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                ddb_listview_refresh (ps, DDB_REFRESH_LIST);
                ps->binding->selection_changed (ps, it, -1); // that means "selection changed a lot, redraw everything"
            }
            ps->area_selection_start = start;
            ps->area_selection_end = end;
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

        GtkAllocation a;
        gtk_widget_get_allocation (ps->list, &a);

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
        else if (ey > a.height-10) {
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
    deadbeef->pl_unlock ();
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
        gtk_widget_queue_draw (ps->header);
        gtk_widget_queue_draw (ps->list);
    }
}

gboolean
ddb_listview_handle_keypress (DdbListview *ps, int keyval, int state) {
    int prev = ps->binding->cursor ();
    int cursor = prev;
    GtkWidget *range = ps->scrollbar;
    GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (range));

    state &= (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD4_MASK);

    if (state & ~GDK_SHIFT_MASK) {
        return FALSE;
    }

    if (keyval == GDK_Down) {
        if (cursor < ps->binding->count () - 1) {
            cursor++;
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
        }
    }
    else if (keyval == GDK_Up) {
        if (cursor > 0) {
            cursor--;
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
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
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
        }
    }
    else if (keyval == GDK_Page_Up) {
        if (cursor > 0) {
            cursor -= 10;
            if (cursor < 0) {
                gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
                cursor = 0;
            }
        }
        else {
            if (cursor < 0 && ps->binding->count () > 0) {
                cursor = 0;
            }
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
        }
    }
    else if (keyval == GDK_End) {
        cursor = ps->binding->count () - 1;
        gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
    }
    else if (keyval == GDK_Home) {
        cursor = 0;
        gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
    }
    else {
        return FALSE;
    }

    if (state & GDK_SHIFT_MASK) {
        GtkAllocation a;
        gtk_widget_get_allocation (ps->list, &a);
        if (cursor != prev) {
            int newscroll = ps->scrollpos;
            int cursor_scroll = ddb_listview_get_row_pos (ps, cursor);
            if (cursor_scroll < ps->scrollpos) {
                newscroll = cursor_scroll;
            }
            else if (cursor_scroll >= ps->scrollpos + a.height) {
                newscroll = cursor_scroll - a.height + 1;
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

            int nchanged = 0;
            int idx=0;

            DdbListviewIter it;
            for (it = ps->binding->head (); it; idx++) {
                if (idx >= start && idx <= end) {
                    ps->binding->select (it, 1);
                    if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                        ddb_listview_draw_row (ps, idx, it);
                        ps->binding->selection_changed (ps, it, idx);
                    }
                }
                else if (ps->binding->is_selected (it))
                {
                    ps->binding->select (it, 0);
                    if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                        ddb_listview_draw_row (ps, idx, it);
                        ps->binding->selection_changed (ps, it, idx);
                    }
                }
                DdbListviewIter next = PL_NEXT(it);
                UNREF (it);
                it = next;
            }
            UNREF (it);
            if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                ddb_listview_refresh (ps, DDB_REFRESH_LIST);
                ps->binding->selection_changed (ps, it, -1); // that means "selection changed a lot, redraw everything"
            }
        }
    }
    else {
        ps->shift_sel_anchor = cursor;
        ddb_listview_set_cursor (ps, cursor);
    }
    return TRUE;
}

gboolean
ddb_listview_list_popup_menu (GtkWidget *widget, gpointer user_data) {
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DdbListviewIter it = ps->binding->head ();
    while (it && !ps->binding->is_selected (it)) {
        DdbListviewIter next = ps->binding->next (it);
        ps->binding->unref (it);
        it = next;
    }
    if (it) {
        int sel = ps->binding->get_idx (it);
        ps->binding->list_context_menu (ps, it, sel);
        ps->binding->unref (it);
    }
    return TRUE;
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
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    if (ps->drag_motion_y != -1) {
        // erase previous track
        gtk_widget_queue_draw_area (ps->list, 0, ps->drag_motion_y-ps->scrollpos-3, a.width, 7);

    }
    if (y == -1) {
        ps->drag_motion_y = -1;
        ps->scroll_active = 0;
        ps->scroll_direction = 0;
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

#if !GTK_CHECK_VERSION(3,0,0)
    // FIXME
//    ddb_listview_draw_dnd_marker (ps, cr);
#endif

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
    else if (y > a.height-10) {
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    ps->scroll_direction = 0;
    ps->scroll_pointer_y = -1;
}

// #define HEADERS_GTKTHEME

void
ddb_listview_header_render (DdbListview *ps, cairo_t *cr) {
    cairo_set_line_width (cr, 1);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    GtkWidget *widget = ps->header;
    int x = -ps->hscrollpos;
    int w = 100;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    int h = a.height;
    const char *detail = "button";

    // fill background and draw bottom line
#if !HEADERS_GTKTHEME
    GdkColor clr;
    gtkui_get_tabstrip_base_color (&clr);
    cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);
    cairo_rectangle (cr, 0, 0,  a.width, a.height);
    cairo_fill (cr);
    gtkui_get_tabstrip_dark_color (&clr);
    cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);
    cairo_move_to (cr, 0, a.height);
    cairo_line_to (cr, a.width, a.height);
    cairo_stroke (cr);
#else
#if GTK_CHECK_VERSION(3,0,0)
    gtk_paint_box (gtk_widget_get_style (theme_button), cr, GTK_STATE_NORMAL, GTK_SHADOW_OUT, widget, detail, -10, -10, a.width+20, a.height+20);
#else
    gtk_paint_box (theme_button->style, ps->header->window, GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, widget, detail, -10, -10, a.width+20, a.height+20);
#endif
    clr = gtk_widget_get_style (widget)->mid[GTK_STATE_NORMAL]
    cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);
    cairo_move_to (cr, 0, a.height-1);
    cairo_line_to (cr, a.width, a.height-1);
    cairo_stroke (cr);
#endif
    draw_begin (&ps->hdrctx, cr);
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
            if (xx >= a.width) {
                continue;
            }
            int arrow_sz = 10;
            int sort = c->sort_order;
            if (w > 0) {
#if !HEADERS_GTKTHEME
                gtkui_get_tabstrip_dark_color (&clr);
                cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);
                cairo_move_to (cr, xx+w - 2, 2);
                cairo_line_to (cr, xx+w - 2, h-4);
                cairo_stroke (cr);

                gtkui_get_tabstrip_light_color (&clr);
                cairo_set_source_rgb (cr, clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f);

                cairo_move_to (cr, xx+w - 1, 2);
                cairo_line_to (cr, xx+w - 1, h-4);
                cairo_stroke (cr);
#else
#if GTK_CHECK_VERSION(3,0,0)
                gtk_paint_vline (gtk_widget_get_style (widget), cr, GTK_STATE_NORMAL, widget, NULL, 2, h-4, xx+w - 3);
#else
                gtk_paint_vline (widget->style, ps->header->window, GTK_STATE_NORMAL, NULL, widget, NULL, 2, h-4, xx+w - 3);
#endif
#endif
                GdkColor *gdkfg;
                if (!gtkui_override_listview_colors ()) {
                    gdkfg = &gtk_widget_get_style (theme_button)->fg[0];
                }
                else {
                    gdkfg = (gtkui_get_listview_column_text_color (&clr), &clr);
                }
                float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
                draw_set_fg_color (&ps->hdrctx, fg);
                int ww = w-10;
                if (sort) {
                    ww -= arrow_sz;
                    if (ww < 0) {
                        ww = 0;
                    }
                }
                draw_text_custom (&ps->hdrctx, xx + 5, 3, ww, 0, DDB_COLUMN_FONT, 0, 0, c->title);
            }
            if (sort) {
                int dir = sort == 1 ? GTK_ARROW_DOWN : GTK_ARROW_UP;
#if GTK_CHECK_VERSION(3,0,0)
                gtk_paint_arrow (gtk_widget_get_style (widget), cr, GTK_STATE_NORMAL, GTK_SHADOW_NONE, widget, NULL, dir, TRUE, xx + w-arrow_sz-5, a.height/2-arrow_sz/2, arrow_sz, arrow_sz);
#else
                gtk_paint_arrow (widget->style, ps->header->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, dir, TRUE, xx + w-arrow_sz-5, a.height/2-arrow_sz/2, arrow_sz, arrow_sz);
#endif
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
                if (x < a.width) {
#if GTK_CHECK_VERSION(3,0,0)
                    gtk_paint_box (gtk_widget_get_style (theme_button), cr, GTK_STATE_ACTIVE, GTK_SHADOW_ETCHED_IN, widget, "button", x, 0, w, h);
#else
                    gtk_paint_box (theme_button->style, ps->header->window, GTK_STATE_ACTIVE, GTK_SHADOW_ETCHED_IN, NULL, widget, "button", x, 0, w, h);
#endif
                }
                x = ps->col_movepos - ps->hscrollpos;
                if (x >= a.width) {
                    break;
                }
                if (w > 0) {
#if GTK_CHECK_VERSION(3,0,0)
                    gtk_paint_box (gtk_widget_get_style (theme_button), cr, GTK_STATE_SELECTED, GTK_SHADOW_OUT, widget, "button", x, 0, w, h);
#else
                    gtk_paint_box (theme_button->style, ps->header->window, GTK_STATE_SELECTED, GTK_SHADOW_OUT, NULL, widget, "button", x, 0, w, h);
#endif
                    GdkColor *gdkfg = &gtk_widget_get_style (theme_button)->fg[GTK_STATE_SELECTED];
                    float fg[3] = {(float)gdkfg->red/0xffff, (float)gdkfg->green/0xffff, (float)gdkfg->blue/0xffff};
                    draw_set_fg_color (&ps->hdrctx, fg);
                    draw_text_custom (&ps->hdrctx, x + 5, 3, c->width-10, 0, DDB_COLUMN_FONT, 0, 0, c->title);
                }
                break;
            }
            x += w;
        }
    }
    draw_end (&ps->hdrctx);
}

gboolean
ddb_listview_header_draw                 (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         user_data) {
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    // FIXME: clip region
    cairo_set_line_width (cr, 1);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    ddb_listview_header_expose (ps, cr, 0, 0, a.width, a.height);
    return FALSE;
}


#if !GTK_CHECK_VERSION(3,0,0)
gboolean
ddb_listview_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    ddb_listview_header_expose (ps, cr, event->area.x, event->area.y, event->area.width, event->area.height);
    cairo_destroy (cr);
    return FALSE;
}
#endif

void
ddb_listview_header_update_fonts (DdbListview *ps)
{
    draw_init_font (&ps->hdrctx, DDB_COLUMN_FONT, 1);
    int height = draw_get_listview_rowheight (&ps->hdrctx);
    GtkAllocation a;
    gtk_widget_get_allocation (ps->header, &a);
    if (height != a.height) {
        gtk_widget_set_size_request (ps->header, -1, height);
    }
}

void
ddb_listview_column_size_changed (DdbListview *listview, int col)
{
    if (ddb_listview_is_album_art_column_idx(listview, col)) {
        ddb_listview_resize_groups (listview);
        if (listview->scrollpos > 0) {
            int pos = ddb_listview_get_row_pos (listview, listview->ref_point);
            gtk_range_set_value (GTK_RANGE (listview->scrollbar), pos - listview->ref_point_offset);
        }
    }
}

void
ddb_listview_update_scroll_ref_point (DdbListview *ps)
{
    ddb_listview_groupcheck (ps);
    DdbListviewGroup *grp = ps->groups;
    DdbListviewGroup *grp_next;

    if (grp && ps->scrollpos > 0) {
        int abs_idx = 0;
        int grp_y = 0;

        GtkAllocation a;
        gtk_widget_get_allocation (ps->list, &a);
        int cursor_pos = ddb_listview_get_row_pos (ps, ps->binding->cursor ());
        ps->ref_point = 0;
        ps->ref_point_offset = 0;

        // find 1st group
        while (grp && grp_y + grp->height < ps->scrollpos) {
            grp_y += grp->height;
            abs_idx += grp->num_items;
            grp = grp->next;
        }
        // choose cursor_pos as anchor
        if (ps->scrollpos < cursor_pos && cursor_pos < ps->scrollpos + a.height && cursor_pos < ps->fullheight) {
            ps->ref_point = ps->binding->cursor ();
            ps->ref_point_offset = cursor_pos - ps->scrollpos;
        }
        // choose first group as anchor
        else if (ps->scrollpos < grp_y + ps-> grouptitle_height + (grp->num_items * ps->rowheight) && grp_y + ps-> grouptitle_height + (grp->num_items * ps->rowheight) < ps->scrollpos + a.height) {
            ps->ref_point = abs_idx;
            ps->ref_point_offset = (grp_y + ps->grouptitle_height) - ps->scrollpos;
        }
        // choose next group as anchor
        else {
            grp_y += grp->height;
            abs_idx += grp->num_items;
            ps->ref_point = abs_idx;
            ps->ref_point_offset = (grp_y + ps->grouptitle_height) - ps->scrollpos;
        }
    }
}

void
ddb_listview_init_autoresize (DdbListview *ps, int totalwidth)
{
    if (totalwidth > 0) {
        DdbListviewColumn *c;
        if (!ps->col_autoresize) {
            for (c = ps->columns; c; c = c->next) {
                c->fwidth = (float)c->width / (float)totalwidth;
            }
            ps->col_autoresize = 1;
        }
    }
}

gboolean
ddb_listview_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_header_update_fonts (ps);
    GtkAllocation lva;
    gtk_widget_get_allocation (GTK_WIDGET (ps), &lva);
    int totalwidth = lva.width;

    // col_autoresize flag indicates whether fwidth is valid
    if (!ps->lock_columns) {
        DdbListviewColumn *c;
        if (deadbeef->conf_get_int ("gtkui.autoresize_columns", 0)) {
            if (ps->header_width != totalwidth) {
                ddb_listview_update_scroll_ref_point (ps);
                if (!ps->col_autoresize) {
                    for (c = ps->columns; c; c = c->next) {
                        c->fwidth = (float)c->width / (float)totalwidth;
                    }
                    ps->col_autoresize = 1;
                }
                // use the fwidth
                int changed = 0;
                int i = 0;
                for (c = ps->columns; c; c = c->next, i++) {
                    int newwidth = totalwidth * c->fwidth;
                    if (newwidth != c->width) {
                        c->width = newwidth;
                        changed = 1;
                        ddb_listview_column_size_changed (ps, i);
                    }
                }
                if (changed) {
                    ps->binding->columns_changed (ps);
                }
            }
        }
        else {
            for (c = ps->columns; c; c = c->next) {
                c->fwidth = (float)c->width / (float)totalwidth;
            }
            ps->col_autoresize = 1;
        }
        ps->header_width = totalwidth;
    }

    return FALSE;
}

void
ddb_listview_lock_columns (DdbListview *lv, gboolean lock) {
    lv->lock_columns = lock;

    // NOTE: at this point, it's still not guaranteed that the allocation contains
    // the final size, so we don't calc initial autoresize state here
}


void
ddb_listview_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data)
{
    // create cursor for sizing headers
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
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

    ev_x = event->x;
    ev_y = event->y;
    ev_state = event->state;
#if GTK_CHECK_VERSION(2,12,0)
    gdk_event_request_motions (event);
#endif

    if ((ev_state & GDK_BUTTON1_MASK) && ps->header_prepare) {
        if (gtk_drag_check_threshold (widget, ev_x, ps->prev_header_x, 0, 0)) {
            ps->header_prepare = 0;
        }
    }
    if (!ps->header_prepare && ps->header_dragging >= 0) {
        gdk_window_set_cursor (gtk_widget_get_window (widget), ps->cursor_drag);
        DdbListviewColumn *c = ps->columns;
        for (int i = 0; c && i < ps->header_dragging; c = c->next, i++);
        const int left = ev_x - ps->header_dragpt[0] + ps->hscrollpos;
        const int right = left + c->width;
        DdbListviewColumn *cc = ps->columns;
        for (int xx = 0, ii = 0; cc; xx += cc->width, cc = cc->next, ii++) {
            if (ps->header_dragging > ii && left < xx + cc->width/2 || ps->header_dragging < ii && right > xx + cc->width/2) {
                ddb_listview_column_move (ps, c, ii);
                ps->header_dragging = ii;
                gtk_widget_queue_draw (ps->list);
                break;
            }
        }
        ps->col_movepos = left;
        gtk_widget_queue_draw (ps->header);
    }
    else if (ps->header_sizing >= 0) {
        ps->last_header_motion_ev = event->time;
        ps->prev_header_x = ev_x;
        gdk_window_set_cursor (gtk_widget_get_window (widget), ps->cursor_sz);
        // get column start pos
        int x = -ps->hscrollpos;
        int i = 0;
        int size = 0;
        DdbListviewColumn *c;
        for (c = ps->columns; c; c = c->next) {
            size += c->width;
        }
        for (c = ps->columns; c && i < ps->header_sizing; c = c->next, i++) {
            x += c->width;
        }

        int newx = ev_x > x + MIN_COLUMN_WIDTH ? ev_x : x + MIN_COLUMN_WIDTH;
        c->width = newx-x;
        if (ps->col_autoresize) {
            c->fwidth = (float)c->width / ps->header_width;
        }
        ps->block_redraw_on_scroll = 1;
        //ddb_listview_list_setup_vscroll (ps);
        ddb_listview_list_setup_hscroll (ps);
        ps->block_redraw_on_scroll = 0;
        ddb_listview_column_size_changed (ps, ps->header_sizing);
        ddb_listview_list_update_total_width (ps, size);
        gtk_widget_queue_draw (ps->header);
        gtk_widget_queue_draw (ps->list);
    }
    else {
        int x = -ps->hscrollpos;
        DdbListviewColumn *c;
        for (c = ps->columns; c; c = c->next) {
            int w = c->width;
            if (w > 0) { // ignore collapsed columns (hack for search window)
                if (ev_x >= x + w - 4 && ev_x <= x + w) {
                    gdk_window_set_cursor (gtk_widget_get_window (widget), ps->cursor_sz);
                    break;
                }
                else {
                    gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
                }
            }
            else {
                gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
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
    if (TEST_LEFT_CLICK (event)) {
        ddb_listview_update_scroll_ref_point (ps);

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
            if (event->x >= x + w - 4 && event->x <= x + w) {
                ps->header_sizing = i;
                ps->header_dragging = -1;
                break;
            }
            else if (event->x > x && event->x < x + w - 4) {
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
    else if (TEST_RIGHT_CLICK (event)) {
        int idx = ddb_listview_header_get_column_idx_for_coord (ps, event->x);
        ps->binding->header_context_menu (ps, idx);
    }
    ps->prev_header_x = -1;
    ps->last_header_motion_ev = -1;
    return TRUE;
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
                    ps->binding->col_sort (i, c->sort_order-1, c->user_data);
                    sorted = 1;
                }
                else {
                    c->sort_order = 0;
                }
                x += w;
            }
            ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_COLUMNS);
        }
        else {
            ps->header_sizing = -1;
            int x = 0;
            DdbListviewColumn *c;
            for (c = ps->columns; c; c = c->next) {
                int w = c->width;
                if (event->x >= x + w - 4 && event->x <= x + w) {
                    gdk_window_set_cursor (gtk_widget_get_window (widget), ps->cursor_sz);
                    break;
                }
                else {
                    gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
                }
                x += w;
            }
            if (ps->header_dragging >= 0) {
                ps->header_dragging = -1;
                ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_COLUMNS | DDB_REFRESH_HSCROLL);
            }
        }
        ps->binding->columns_changed (ps);
        int size = 0;
        DdbListviewColumn *c;
        for (c = ps->columns; c; c = c->next) {
            size += c->width;
        }
        ddb_listview_list_update_total_width (ps, size);
    }
    return FALSE;
}

struct set_cursor_t {
    int cursor;
    int prev;
    DdbListview *pl;
    int noscroll;
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

    if (!sc->noscroll) {
        DdbListview *ps = sc->pl;

        int cursor_scroll = ddb_listview_get_row_pos (sc->pl, sc->cursor);
        int newscroll = sc->pl->scrollpos;
        GtkAllocation a;
        gtk_widget_get_allocation (sc->pl->list, &a);
        if (!gtkui_groups_pinned && cursor_scroll < sc->pl->scrollpos) {
             newscroll = cursor_scroll;
        }
        else if (gtkui_groups_pinned && cursor_scroll < sc->pl->scrollpos + ps->grouptitle_height) {
            newscroll = cursor_scroll - ps->grouptitle_height;
        }
        else if (cursor_scroll + sc->pl->rowheight >= sc->pl->scrollpos + a.height) {
            newscroll = cursor_scroll + sc->pl->rowheight - a.height + 1;
            if (newscroll < 0) {
                newscroll = 0;
            }
        }
        if (sc->pl->scrollpos != newscroll) {
            GtkWidget *range = sc->pl->scrollbar;
            gtk_range_set_value (GTK_RANGE (range), newscroll);
        }

        free (data);
    }
    return FALSE;
}

void
ddb_listview_set_cursor (DdbListview *pl, int cursor) {
    int prev = pl->binding->cursor ();
    struct set_cursor_t *data = malloc (sizeof (struct set_cursor_t));
    data->prev = prev;
    data->cursor = cursor;
    data->pl = pl;
    data->noscroll = 0;
    g_idle_add (ddb_listview_set_cursor_cb, data);
}

void
ddb_listview_set_cursor_noscroll (DdbListview *pl, int cursor) {
    int prev = pl->binding->cursor ();
    struct set_cursor_t *data = malloc (sizeof (struct set_cursor_t));
    data->prev = prev;
    data->cursor = cursor;
    data->pl = pl;
    data->noscroll = 1;
    g_idle_add (ddb_listview_set_cursor_cb, data);
}

gboolean
ddb_listview_list_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    gtk_widget_grab_focus (widget);
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (TEST_LEFT_CLICK (event)) {
        ddb_listview_list_mouse1_pressed (ps, event->state, event->x, event->y, event->type);
    }
    else if (TEST_RIGHT_CLICK(event)) {
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
            ddb_listview_click_selection (ps, event->x, event->y, grp, grp_index, sel, 0, event->button);
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
    return TRUE;
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
#if GTK_CHECK_VERSION(2,12,0)
    gdk_event_request_motions (event);
#endif
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
    GtkAllocation a;
    gtk_widget_get_allocation (listview->list, &a);
    if (pos < listview->scrollpos || pos + listview->rowheight >= listview->scrollpos + a.height) {
        gtk_range_set_value (GTK_RANGE (listview->scrollbar), pos - a.height/2);
    }
}

int
ddb_listview_is_scrolling (DdbListview *listview) {
    return listview->dragwait;
}

/////// column management code

DdbListviewColumn *
ddb_listview_column_alloc (const char *title, int width, int align_right, int minheight, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn * c = malloc (sizeof (DdbListviewColumn));
    memset (c, 0, sizeof (DdbListviewColumn));
    c->title = strdup (title);
    c->width = width;
    c->align_right = align_right;
    c->color_override = color_override;
    c->color = color;
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
ddb_listview_column_append (DdbListview *listview, const char *title, int width, int align_right, int minheight, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn* c = ddb_listview_column_alloc (title, width, align_right, minheight, color_override, color, user_data);
    if (listview->col_autoresize) {
        c->fwidth = (float)c->width / listview->header_width;
    }
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
ddb_listview_column_insert (DdbListview *listview, int before, const char *title, int width, int align_right, int minheight, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn *c = ddb_listview_column_alloc (title, width, align_right, minheight, color_override, color, user_data);
    if (listview->col_autoresize) {
        c->fwidth = (float)c->width / listview->header_width;
    }
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
ddb_listview_column_get_info (DdbListview *listview, int col, const char **title, int *width, int *align_right, int *minheight, int *color_override, GdkColor *color, void **user_data) {
    DdbListviewColumn *c;
    int idx = 0;
    for (c = listview->columns; c; c = c->next, idx++) {
        if (idx == col) {
            *title = c->title;
            *width = c->width;
            *align_right = c->align_right;
            *minheight = c->minheight;
            *color_override = c->color_override;
            *color = c->color;
            *user_data = c->user_data;
            return 0;
        }
    }
    return -1;
}

int
ddb_listview_column_set_info (DdbListview *listview, int col, const char *title, int width, int align_right, int minheight, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn *c;
    int idx = 0;
    for (c = listview->columns; c; c = c->next, idx++) {
        if (idx == col) {
            free (c->title);
            c->title = strdup (title);
            c->width = width;
            if (listview->col_autoresize) {
                c->fwidth = (float)c->width / listview->header_width;
            }
            c->align_right = align_right;
            c->minheight = minheight;
            c->color_override = color_override;
            c->color = color;
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
    if (listview->plt) {
        deadbeef->plt_unref (listview->plt);
        listview->plt = NULL;
    }
}

void
ddb_listview_build_groups (DdbListview *listview) {
    deadbeef->pl_lock ();
    int old_height = listview->fullheight;
    listview->groups_build_idx = listview->binding->modification_idx ();
    ddb_listview_free_groups (listview);

    listview->plt = deadbeef->plt_get_curr ();

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

    listview->grouptitle_height = listview->calculated_grouptitle_height;
    DdbListviewIter it = listview->binding->head ();
    while (it) {
        int res = listview->binding->get_group (listview, it, curr, sizeof (curr));
        if (res == -1) {
            grp = malloc (sizeof (DdbListviewGroup));
            listview->groups = grp;
            memset (grp, 0, sizeof (DdbListviewGroup));
            grp->head = it;
            grp->num_items = listview->binding->count ();
            listview->grouptitle_height = 0;
            grp->height = listview->grouptitle_height + grp->num_items * listview->rowheight;
            listview->fullheight = grp->height;
            listview->fullheight += listview->grouptitle_height;
            deadbeef->pl_unlock ();
            if (old_height != listview->fullheight) {
                ddb_listview_refresh (listview, DDB_REFRESH_VSCROLL);
            }
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
    deadbeef->pl_unlock ();
    if (old_height != listview->fullheight) {
        ddb_listview_refresh (listview, DDB_REFRESH_VSCROLL);
    }
}

void
ddb_listview_resize_groups (DdbListview *listview) {
    deadbeef->pl_lock ();
    int old_height = listview->fullheight;
    int grp_height_old = 0;
    listview->fullheight = 0;

    int min_height= 0;
    DdbListviewColumn *c;
    for (c = listview->columns; c; c = c->next) {
        if (c->minheight && c->width > min_height) {
            min_height = c->width;
        }
    }

    DdbListviewGroup *grp = listview->groups;
    while (grp) {
        grp->height = listview->grouptitle_height + grp->num_items * listview->rowheight;
        if (grp->height - listview->grouptitle_height < min_height) {
            grp_height_old = grp->height;
            grp->height = min_height + listview->grouptitle_height;
        }
        listview->fullheight += grp->height;
        grp = grp->next;
    }

    deadbeef->pl_unlock ();
    if (old_height != listview->fullheight) {
        ddb_listview_refresh (listview, DDB_REFRESH_VSCROLL);
    }
}

void
ddb_listview_set_vscroll (DdbListview *listview, int scroll) {
    GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (listview->scrollbar));
    gtk_range_set_value (GTK_RANGE (listview->scrollbar), scroll);
}

void
ddb_listview_show_header (DdbListview *listview, int show) {
    show ? gtk_widget_show (listview->header) : gtk_widget_hide (listview->header);
}

void
ddb_listview_clear_sort (DdbListview *listview) {
    DdbListviewColumn *c;
    for (c = listview->columns; c; c = c->next) {
        c->sort_order = 0;
    }
    gtk_widget_queue_draw (listview->header);
}

gboolean
ddb_listview_list_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (!ddb_listview_handle_keypress (listview, event->keyval, event->state)) {
        return on_mainwin_key_press_event (widget, event, user_data);
    }
    return TRUE;
}

void
ddb_listview_cancel_autoredraw (DdbListview *listview) {
    if (listview->tf_redraw_timeout_id) {
        g_source_remove (listview->tf_redraw_timeout_id);
        listview->tf_redraw_timeout_id = 0;
    }
    if (listview->tf_redraw_track) {
        listview->binding->unref (listview->tf_redraw_track);
        listview->tf_redraw_track = NULL;
    }
}
