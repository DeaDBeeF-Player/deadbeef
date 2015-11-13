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
#define MIN_COLUMN_WIDTH 16

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define PL_NEXT(it) (ps->binding->next (it))
#define PL_PREV(it) (ps->binding->prev (it))
//#define REF(it) {if (it) ps->binding->ref (it);}
#define UNREF(it) {if (it) ps->binding->unref(it);}

extern GtkWidget *theme_treeview;
extern GtkWidget *theme_button;

G_DEFINE_TYPE (DdbListview, ddb_listview, GTK_TYPE_TABLE);

struct _DdbListviewColumn {
    char *title;
    int width;
    float fwidth; // only in autoresize mode
    minheight_cb_t minheight_cb;
    struct _DdbListviewColumn *next;
    int color_override;
    GdkColor color;
    void *user_data;
    unsigned align_right : 2; // 0=left, 1=right, 2=center
    unsigned sort_order : 2; // 0=none, 1=asc, 2=desc
};
typedef struct _DdbListviewColumn DdbListviewColumn;

typedef enum {
    PICK_ITEM,
    PICK_GROUP_TITLE,
    PICK_ALBUM_ART,
    PICK_EMPTY_SPACE,
    PICK_ABOVE_PLAYLIST,
    PICK_BELOW_PLAYLIST,
} area_type_t;

struct _DdbListviewPickContext {
    int item_idx; // index of playitem in playlist
    int item_grp_idx; // index of first playitem in group in playlist
    int grp_idx; // index of playitem within grp
    area_type_t type;
    DdbListviewGroup *grp;
};
typedef struct _DdbListviewPickContext DdbListviewPickContext;

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
static void
ddb_listview_list_render (DdbListview *ps, cairo_t *cr, GdkRectangle *clip);
static void
ddb_listview_list_render_row_background (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h, GdkRectangle *clip);
static void
ddb_listview_list_render_row_foreground (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int idx, int y, int w, int h, int x1, int x2);
static void
ddb_listview_list_render_album_art (DdbListview *ps, cairo_t *cr, DdbListviewGroup *grp, int grp_next_y, int y, GdkRectangle *clip);
static void
ddb_listview_list_track_dragdrop (DdbListview *ps, int x, int y);
int
ddb_listview_dragdrop_get_row_from_coord (DdbListview *listview, int x, int y);
void
ddb_listview_list_mousemove (DdbListview *ps, GdkEventMotion *event, int x, int y);
static void
ddb_listview_list_setup_vscroll (DdbListview *ps);
static void
ddb_listview_list_setup_hscroll (DdbListview *ps);
void
ddb_listview_set_cursor (DdbListview *pl, int cursor);
int
ddb_listview_get_row_pos (DdbListview *listview, int pos);

////// header functions ////
static void
ddb_listview_header_render (DdbListview *ps, cairo_t *cr, int x1, int x2);

static int
ddb_listview_header_get_column_idx_for_coord (DdbListview *pl, int click_x);

////// column management functions ////
void
ddb_listview_column_move (DdbListview *listview, DdbListviewColumn *which, int inspos);
void
ddb_listview_column_free (DdbListview *listview, DdbListviewColumn *c);


// signal handlers
static void
ddb_listview_vscroll_value_changed            (GtkRange        *widget,
                                        gpointer         user_data);
static void
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

static gboolean
ddb_listview_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

void
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
ddb_listview_list_configure_event            (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

#if GTK_CHECK_VERSION(3,0,0)
static gboolean
ddb_listview_list_draw               (GtkWidget       *widget,
        cairo_t *cr,
        gpointer         user_data);
#else
static gboolean
ddb_listview_list_expose_event               (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

#endif

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

static gboolean
ddb_listview_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

static gboolean
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
    listview->scroll_pointer_x = -1;
    listview->scroll_pointer_y = -1;
    listview->scroll_direction = 0;
    listview->scroll_active = 0;
    memset (&listview->tm_prevscroll, 0, sizeof (listview->tm_prevscroll));
    listview->scroll_sleep_time = 0;

    listview->areaselect = 0;
    listview->areaselect_x = -1;
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
    listview->prev_header_x = -1;
    listview->header_prepare = 0;
    listview->header_width = -1;

    listview->columns = NULL;
    listview->lock_columns = 1;
    listview->groups = NULL;
    listview->plt = NULL;

    listview->calculated_grouptitle_height = DEFAULT_GROUP_TITLE_HEIGHT;

    listview->cursor_sz = NULL;
    listview->cursor_drag = NULL;

    listview->area_selection_start = 0;
    listview->area_selection_end = 0;

    listview->tf_redraw_timeout_id = 0;
    listview->tf_redraw_track_idx = -1;

    GtkWidget *hbox;
    GtkWidget *vbox;

    listview->scrollpos = -1;
    gtk_table_resize (GTK_TABLE (listview), 2, 2);
    listview->scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (NULL));
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
    gtk_widget_set_events (listview->header, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK);

    listview->list = gtk_drawing_area_new ();
    g_object_ref(listview->list);
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

    listview->hscrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (NULL));
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
    g_signal_connect ((gpointer) listview->hscrollbar, "value_changed",
            G_CALLBACK (ddb_listview_hscroll_value_changed),
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
    g_signal_connect ((gpointer) listview->header, "enter-notify-event",
            G_CALLBACK (ddb_listview_header_enter),
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
            G_CALLBACK (ddb_listview_scroll_event),
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
    g_object_unref(listview->list);

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
    if (listview->tf_redraw_timeout_id) {
        g_source_remove (listview->tf_redraw_timeout_id);
        listview->tf_redraw_timeout_id = 0;
    }
    if (listview->tf_redraw_track) {
        listview->binding->unref (listview->tf_redraw_track);
        listview->tf_redraw_track = NULL;
    }
    draw_free (&listview->listctx);
    draw_free (&listview->grpctx);
    draw_free (&listview->hdrctx);
}

void
ddb_listview_refresh (DdbListview *listview, uint32_t flags) {
    if (flags & DDB_REFRESH_FONTS) {
        ddb_listview_update_fonts (listview);
        ddb_listview_header_update_fonts (listview);
    }
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
    GtkTargetEntry entry = {
        .target = "DDB_URI_LIST",
        .flags = GTK_TARGET_SAME_APP,
        .info = TARGET_SAMEWIDGET
    };
    // setup drag-drop target
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, &entry, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_drag_dest_add_uri_targets (widget);
//    gtk_drag_dest_set_track_motion (widget, TRUE);
}

static int
total_columns_width (DdbListview *listview) {
    int size = 0;
    for (DdbListviewColumn *c = listview->columns; c; c = c->next) {
        size += c->width;
    }
    return size;
}

static void
ddb_listview_list_update_total_width (DdbListview *listview, int columns_width, int width) {
    listview->totalwidth = max(columns_width, width);
}

void
ddb_listview_groupcheck (DdbListview *listview) {
    int idx = listview->binding->modification_idx ();
    if (idx != listview->groups_build_idx) {
        ddb_listview_build_groups (listview);
    }
}

// returns 1 if X coordinate in list belongs to album art column and 0 if not
static int
ddb_listview_is_album_art_column (DdbListview *listview, int x)
{
    int album_art_column = 0;
    int col_x = -listview->hscrollpos;
    int cnt = ddb_listview_column_get_count (listview);
    for (DdbListviewColumn *c = listview->columns; c && col_x <= x; c = c->next) {
        if (x <= col_x + c->width && listview->binding->is_album_art_column(c->user_data)) {
            return 1;
        }
        col_x += c->width;
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
static void
ddb_listview_list_pickpoint (DdbListview *listview, int x, int y, DdbListviewPickContext *pick_ctx) {
    int idx = 0;
    int grp_y = 0;
    int gidx = 0;
    const int ey = y;
    const int ry = ey - listview->scrollpos;
    const int grp_title_height = listview->grouptitle_height;
    const int rowheight = listview->rowheight;

    if (y < 0) {
        // area above playlist
        pick_ctx->type = PICK_ABOVE_PLAYLIST;
        pick_ctx->item_grp_idx = 0;
        pick_ctx->grp_idx = 0;
        // select first playlist item
        pick_ctx->item_idx = 0;
        pick_ctx->grp = NULL;
        return;
    }
    else if (y > listview->fullheight) {
        // area below playlist
        pick_ctx->type = PICK_BELOW_PLAYLIST;
        pick_ctx->item_grp_idx = -1;
        pick_ctx->grp_idx = -1;
        // select last playlist item
        pick_ctx->item_idx = listview->binding->count () - 1;
        pick_ctx->grp = NULL;
        return;
    }

    deadbeef->pl_lock ();

    const int is_album_art_column = ddb_listview_is_album_art_column (listview, x);

    ddb_listview_groupcheck (listview);
    DdbListviewGroup *grp = listview->groups;
    while (grp) {
        const int h = grp->height;
        if (y >= grp_y && y < grp_y + h) {
            pick_ctx->grp = grp;
            y -= grp_y;
            if (y < grp_title_height || (0 < ry && ry < grp_title_height && gtkui_groups_pinned)) {
                // group title
                pick_ctx->type = PICK_GROUP_TITLE;
                pick_ctx->item_grp_idx = idx;
                pick_ctx->item_idx = idx;
                pick_ctx->grp_idx = 0;
            }
            else if (is_album_art_column) {
                pick_ctx->type = PICK_ALBUM_ART;
                pick_ctx->item_grp_idx = idx;
                pick_ctx->grp_idx = min ((y - grp_title_height) / rowheight, grp->num_items - 1);
                pick_ctx->item_idx = idx + pick_ctx->grp_idx;
            }
            else if (y >= grp_title_height + grp->num_items * rowheight) {
                // whitespace after tracks
                pick_ctx->type = PICK_EMPTY_SPACE;
                pick_ctx->item_grp_idx = idx;
                pick_ctx->grp_idx = grp->num_items - 1;
                pick_ctx->item_idx = idx + pick_ctx->grp_idx;
            }
            else {
                pick_ctx->type = PICK_ITEM;
                pick_ctx->item_grp_idx = idx;
                pick_ctx->grp_idx = (y - grp_title_height) / rowheight;
                pick_ctx->item_idx = idx + pick_ctx->grp_idx;
            }
            deadbeef->pl_unlock ();
            return;
        }
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
        gidx++;
    }

    // area at the end of playlist or unknown
    pick_ctx->type = PICK_EMPTY_SPACE;
    pick_ctx->item_grp_idx = -1;
    pick_ctx->grp_idx = -1;
    pick_ctx->item_idx = listview->binding->count () - 1;
    pick_ctx->grp = NULL;

    deadbeef->pl_unlock ();
    return;
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

#if GTK_CHECK_VERSION(3,0,0)
static void
render_column_button (DdbListview *listview, cairo_t *cr, GtkStateFlags state, int x, int y, int w, int h, GdkColor *clr)
{
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
render_treeview_background (DdbListview *listview, cairo_t *cr, int selected, int even, int x, int y, int w, int h, GdkRectangle *clip)
{
    if (gtkui_override_listview_colors()) {
        GdkColor clr;
        if (selected) {
            gtkui_get_listview_selection_color(&clr);
        }
        else {
            even ? gtkui_get_listview_even_row_color(&clr) : gtkui_get_listview_odd_row_color(&clr);
        }
        draw_cairo_rectangle(cr, &clr, x, y, w, h);
    }
    else {
#if GTK_CHECK_VERSION(3,0,0)
        GtkStyleContext *context = gtk_widget_get_style_context(theme_treeview);
        gtk_style_context_set_state(context, selected ? GTK_STATE_FLAG_SELECTED : GTK_STATE_FLAG_NORMAL);
        gtk_style_context_add_region(context, GTK_STYLE_REGION_ROW, even ? GTK_REGION_EVEN : GTK_REGION_ODD);
        gtk_render_background(context, cr, x, y, w, h);
        gtk_style_context_remove_region(context, GTK_STYLE_REGION_ROW);
#else
        gtk_paint_flat_box(gtk_widget_get_style(theme_treeview), gtk_widget_get_window(listview->list), selected ? GTK_STATE_SELECTED : GTK_STATE_NORMAL, GTK_SHADOW_NONE, clip, theme_treeview, even ? "cell_even_ruled" : "cell_odd_ruled", x, y, w, h);
#endif
    }
}

static void
fill_list_background (DdbListview *listview, cairo_t *cr, int x, int y, int w, int h, GdkRectangle *clip)
{
    if (!gtkui_override_listview_colors()) {
#if GTK_CHECK_VERSION(3,0,0)
        gtk_render_background(gtk_widget_get_style_context(mainwin), cr, x, y, w, h);
        gtk_render_background(gtk_widget_get_style_context(listview->list), cr, x, y, w, h);
#else
        gtk_paint_flat_box(gtk_widget_get_style(mainwin), gtk_widget_get_window(listview->list), GTK_STATE_NORMAL, GTK_SHADOW_NONE, clip, mainwin, NULL, x, y, w, h);
        gtk_paint_flat_box(gtk_widget_get_style(listview->list), gtk_widget_get_window(listview->list), GTK_STATE_NORMAL, GTK_SHADOW_NONE, clip, listview->list, NULL, x, y, w, h);
#endif
    }
    render_treeview_background(listview, cr, FALSE, TRUE, x, y, w, h, clip);
}

static void
ddb_listview_list_render (DdbListview *listview, cairo_t *cr, GdkRectangle *clip) {
    if (listview->scrollpos == -1) {
        return; // too early
    }
    int scrollx = -listview->hscrollpos;
    int title_height = listview->grouptitle_height;
    int row_height = listview->rowheight;
    int total_width = listview->totalwidth;
    int cursor_index = listview->binding->cursor();

    draw_begin (&listview->listctx, cr);
    draw_begin (&listview->grpctx, cr);
    GtkAllocation a;
    gtk_widget_get_allocation(listview->list, &a);
    fill_list_background(listview, cr, scrollx, -listview->scrollpos, total_width, max(listview->fullheight, a.height), clip);

    // find 1st group
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
    for (DdbListviewGroup *grp_unpin = listview->groups; grp_unpin; grp_unpin = grp_unpin->next) {
        grp_unpin->pinned = 0;
    }
    DdbListviewGroup *grp = listview->groups;
    int idx = 0;
    int grp_y = -listview->scrollpos;
    while (grp && grp_y + grp->height < clip->y) {
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }
    if (grp && grp_y < 0 && grp_y + grp->height >= 0) {
        grp->pinned = 1;
    }

    while (grp && grp_y < clip->y + clip->height) {
        int grp_height = title_height + grp->num_items * row_height;

        DdbListviewIter it = grp->head;
        listview->binding->ref(it);
        for (int i = 0, yy = grp_y + title_height; it && i < grp->num_items && yy < clip->y + clip->height; i++, yy += row_height) {
            if (yy + row_height >= clip->y) {
                ddb_listview_list_render_row_background(listview, cr, it, i & 1, idx+i == cursor_index, scrollx, yy, total_width, row_height, clip);
                ddb_listview_list_render_row_foreground(listview, cr, it, idx+i, yy, total_width, row_height, clip->x, clip->x+clip->width);
            }
            DdbListviewIter next = listview->binding->next(it);
            listview->binding->unref(it);
            it = next;
        }
        if (it) {
            listview->binding->unref(it);
        }
//        if (grp->height > grp_height) {
//            render_treeview_background(listview, cr, FALSE, TRUE, scrollx, grp_y+grp_height, total_width, grp->height-grp_height, clip);
//        }

        // draw album art
        int grp_next_y = grp_y + grp->height;
        ddb_listview_list_render_album_art(listview, cr, grp, grp_next_y, grp_y + title_height, clip);

        if (grp->pinned == 1 && gtkui_groups_pinned && clip->y <= title_height) {
            // draw pinned group title
            fill_list_background(listview, cr, scrollx, 0, total_width, min(title_height, grp_next_y), clip);
//            render_treeview_background(listview, cr, FALSE, TRUE, scrollx, 0, total_width, min(title_height, grp_next_y), clip);
            if (listview->binding->draw_group_title && title_height > 0) {
                listview->binding->draw_group_title(listview, cr, grp->head, PL_MAIN, scrollx, min(0, grp_next_y-title_height), total_width, title_height);
            }
        }
        else if (clip->y <= grp_y + title_height) {
            // draw normal group title
//            render_treeview_background(listview, cr, FALSE, TRUE, scrollx, grp_y, total_width, title_height, clip);
            if (listview->binding->draw_group_title && title_height > 0) {
                listview->binding->draw_group_title(listview, cr, grp->head, PL_MAIN, scrollx, grp_y, total_width, title_height);
            }
        }

        idx += grp->num_items;
        grp_y += grp->height;
        grp = grp->next;
    }

//    if (grp_y < clip->y + clip->height) {
//        render_treeview_background(listview, cr, FALSE, TRUE, scrollx, grp_y, total_width, clip->y+clip->height-grp_y, clip);
//    }

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

    GtkAllocation a;
    gtk_widget_get_allocation (ps->list, &a);
    GdkColor clr;
    gtkui_get_listview_cursor_color (&clr);
    draw_cairo_rectangle(cr, &clr, 0, drag_motion_y-1, a.width, 3);
    draw_cairo_rectangle(cr, &clr, 0, drag_motion_y-3, 3, 7);
    draw_cairo_rectangle(cr, &clr, a.width-3, drag_motion_y-3, 3, 7);

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
}

static void
draw_list_rectangle (GtkWidget *widget, cairo_t *cr, GdkRectangle *clip)
{
    cairo_rectangle(cr, clip->x, clip->y, clip->width, clip->height);
    cairo_clip(cr);
    cairo_set_line_width(cr, 1);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data(G_OBJECT (widget), "owner"));
    ddb_listview_list_render(ps, cr, clip);
    if (ps->drag_motion_y >= 0 && ps->drag_motion_y-ps->scrollpos-3 < clip->y+clip->height && ps->drag_motion_y-ps->scrollpos+3 >= clip->y) {
        ddb_listview_draw_dnd_marker(ps, cr);
    }
}

#if GTK_CHECK_VERSION(3,0,0)
static gboolean
ddb_listview_list_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    cairo_rectangle_list_t *list = cairo_copy_clip_rectangle_list(cr);
    for (int i = 0; i < list->num_rectangles; i++) {
        cairo_save(cr);
        GdkRectangle clip =
        {
            .x = floor(list->rectangles[i].x),
            .y = floor(list->rectangles[i].y),
            .width = ceil(list->rectangles[i].width),
            .height = ceil(list->rectangles[i].height)
        };
        draw_list_rectangle(widget, cr, &clip);
        cairo_restore(cr);
    }
    cairo_rectangle_list_destroy(list);
    return TRUE;
}
#else
static gboolean
ddb_listview_list_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
    // FIXME?
    if (gtk_widget_get_style(theme_treeview)->depth == -1) {
        return FALSE; // drawing was called too early
    }

    GdkRectangle *rectangles;
    int num_rectangles;
    gdk_region_get_rectangles(event->region, &rectangles, &num_rectangles);
    for (int i = 0; i < num_rectangles; i++) {
        GdkRectangle *clip = &rectangles[i];
        cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
        draw_list_rectangle(widget, cr, clip);
        cairo_destroy(cr);
    }
    g_free(rectangles);
    return TRUE;
}
#endif

static void
scroll_by (GtkWidget *scrollbar, gdouble delta)
{
    GtkRange *range = GTK_RANGE(scrollbar);
    gdouble step = pow(gtk_adjustment_get_page_size(gtk_range_get_adjustment(range)), 2./3.);
    gtk_range_set_value(range, max(0, gtk_range_get_value(range) + step * delta));
}

static gboolean
ddb_listview_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    GdkEventScroll *ev = (GdkEventScroll*)event;

    switch(ev->direction) {
        case GDK_SCROLL_UP:
            scroll_by(ps->scrollbar, -1);
            break;
        case GDK_SCROLL_DOWN:
            scroll_by(ps->scrollbar, 1);
            break;
        case GDK_SCROLL_LEFT:
            scroll_by(ps->hscrollbar, -1);
            break;
        case GDK_SCROLL_RIGHT:
            scroll_by(ps->hscrollbar, 1);
            break;
#if GTK_CHECK_VERSION(3,4,0)
        case GDK_SCROLL_SMOOTH:
        {
            gdouble x, y;
            if (gdk_event_get_scroll_deltas(event, &x, &y)) {
                scroll_by(ps->hscrollbar, x);
                scroll_by(ps->scrollbar, y);
            }
            break;
        }
#endif
        default:
            break;
    }

    return FALSE;
}

static void
invalidate_album_art_cells (DdbListview *listview, int x1, int x2, int y, int h) {
    int x = -listview->hscrollpos;
    for (DdbListviewColumn *c = listview->columns; c && x < x2; x += c->width, c = c->next) {
        if (x + c->width > x1 && listview->binding->is_album_art_column(c->user_data)) {
            gtk_widget_queue_draw_area(listview->list, x, y, c->width, h);
        }
    }
}

static void
invalidate_group (DdbListview *ps, int at_y)
{
    DdbListviewGroup *group = ps->groups;
    if (!group) {
        return;
    }

    int next_group_y = group->height;
    while (group->next && next_group_y < at_y) {
        group = group->next;
        next_group_y += group->height;
    }

    GtkAllocation a;
    gtk_widget_get_allocation(ps->list, &a);
    int group_height = next_group_y - at_y;
    gtk_widget_queue_draw_area(ps->list, 0, 0, a.width, min(ps->grouptitle_height, group_height));
    if (group_height > ps->grouptitle_height) {
        invalidate_album_art_cells(ps, 0, a.width, ps->grouptitle_height, group_height - ps->grouptitle_height);
    }
}

static void
ddb_listview_vscroll_value_changed (GtkRange *widget, gpointer user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    if (ps->binding->vscroll_changed) {
        ps->binding->vscroll_changed (newscroll);
    }
    if (gtkui_groups_pinned && ps->grouptitle_height > 0) {
        invalidate_group(ps, max(ps->scrollpos, newscroll));
    }
    gdk_window_scroll(gtk_widget_get_window(ps->list), 0, ps->scrollpos - newscroll);
    ps->scrollpos = newscroll;
}

static void
ddb_listview_hscroll_value_changed (GtkRange *widget, gpointer user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    int diff = ps->hscrollpos - newscroll;
    gdk_window_scroll(gtk_widget_get_window(ps->header), diff, 0);
    gdk_window_scroll(gtk_widget_get_window(ps->list), diff, 0);
    ps->hscrollpos = newscroll;
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
    ddb_listview_list_track_dragdrop (pl, x, y);
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

    GdkWindow *win = gtk_widget_get_window (widget);
#if GTK_CHECK_VERSION(3,0,0)
        GdkDeviceManager *device_manager = gdk_display_get_device_manager (gdk_window_get_display (win));
        GdkDevice *pointer = gdk_device_manager_get_client_pointer (device_manager);
        gdk_window_get_device_position (win, pointer, NULL, NULL, &mask);
#else
        gdk_window_get_pointer (win, NULL, NULL, &mask);
#endif
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
    int sel = ddb_listview_dragdrop_get_row_from_coord (ps, x, y);
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
    if (target_type == 0) { // uris
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
    else if (target_type == 1 && gtk_selection_data_get_format(data) == 32) { // list of 32bit ints, DDB_URI_LIST target
        uint32_t *d= (uint32_t *)ptr;
        int plt = *d;
        d++;
        int length = (len/4)-1;
        DdbListviewIter drop_before = it;
        // find last selected
        while (drop_before && ps->binding->is_selected (drop_before)) {
            DdbListviewIter next = PL_NEXT(drop_before);
            UNREF (drop_before);
            drop_before = next;
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
    ddb_listview_list_track_dragdrop (pl, -1, -1);
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

static void
adjust_scrollbar (GtkWidget *scrollbar, int upper, int page_size) {
    GtkRange *range = GTK_RANGE(scrollbar);
    if (page_size >= upper) {
        gtk_range_set_value(range, 0);
        gtk_range_set_adjustment(range, NULL);
        gtk_widget_hide(scrollbar);
        return;
    }

    gdouble scrollpos = gtk_range_get_value(range);
    GtkAdjustment *adj = gtk_range_get_adjustment(range);
    int old_page_size = gtk_adjustment_get_page_size(adj);
    int old_upper = gtk_adjustment_get_upper(adj);
    if (scrollpos > 0 && page_size != old_page_size && scrollpos >= old_upper - old_page_size) {
        scrollpos = upper - page_size;
    }
    gtk_range_set_adjustment(range, GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, upper, SCROLL_STEP, page_size/2, page_size)));
    gtk_range_set_value(range, round(scrollpos));
    gtk_widget_show(scrollbar);
}

static void
ddb_listview_list_setup_vscroll (DdbListview *ps) {
    ddb_listview_groupcheck (ps);
    GtkAllocation a;
    gtk_widget_get_allocation (ps->list, &a);
    adjust_scrollbar(ps->scrollbar, ps->fullheight, a.height);
}

static void
ddb_listview_list_setup_hscroll (DdbListview *ps) {
    int size = total_columns_width(ps);
    ddb_listview_list_update_total_width(ps, size, ps->list_width);
    adjust_scrollbar(ps->hscrollbar, size, ps->list_width);
}

static gboolean
ddb_listview_reconf_scrolling (void *ps) {
    ddb_listview_list_setup_vscroll (ps);
    ddb_listview_list_setup_hscroll (ps);
    return FALSE;
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
static void
ddb_listview_list_render_row_background (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h, GdkRectangle *clip) {
    // draw background even for selection -- for theme translucency
    int draw_selected = it && ps->binding->is_selected (it);
    int draw_normal = !gtkui_override_listview_colors() || !draw_selected;
    if (draw_normal && !even) {
        render_treeview_background(ps, cr, FALSE, even, x, y, w, h, clip);
    }
    if (draw_selected) {
        render_treeview_background(ps, cr, TRUE, even, x, y, w, h, clip);
    }

    if (cursor && gtk_widget_has_focus (ps->list)) {
        // not all gtk engines/themes render focus rectangle in treeviews but we want it anyway
        GdkColor clr;
        gtkui_get_listview_cursor_color (&clr);
        cairo_set_source_rgb (cr, clr.red/65535., clr.green/65535., clr.blue/65535.);
        cairo_rectangle (cr, x+1, y+1, w-1, h-1);
        cairo_stroke (cr);
    }
}

static void
ddb_listview_list_render_row_foreground (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int idx, int y, int w, int h, int x1, int x2) {
    int cidx = 0;
    int x = -ps->hscrollpos;
    for (DdbListviewColumn *c = ps->columns; c && x < x2; x += c->width, c = c->next, cidx++) {
        if (x + c->width > x1 && !ps->binding->is_album_art_column(c->user_data)) {
            ps->binding->draw_column_data (ps, cr, it, idx, cidx, PL_MAIN, x, y, c->width, h);
        }
    }
}

static void
ddb_listview_list_render_album_art (DdbListview *ps, cairo_t *cr, DdbListviewGroup *grp, int grp_next_y, int y, GdkRectangle *clip) {
    int x = -ps->hscrollpos;
    for (DdbListviewColumn *c = ps->columns; c && x < clip->x+clip->width; x += c->width, c = c->next) {
        if (ps->binding->is_album_art_column(c->user_data) && x + c->width > clip->x) {
            fill_list_background(ps, cr, x, y, c->width, grp->height-ps->grouptitle_height, clip);
//            render_treeview_background(ps, cr, FALSE, TRUE, x, y, c->width, grp->height, clip);
            if (ps->grouptitle_height > 0) {
                ps->binding->draw_album_art(ps, cr, grp->head, c->user_data, grp->pinned, grp_next_y, x, y, c->width, grp->height-ps->grouptitle_height);
            }
        }
    }
}

static void
ddb_listview_header_expose (DdbListview *ps, cairo_t *cr, int x, int y, int w, int h) {
    ddb_listview_header_render (ps, cr, x, x+w);
}

// Deselect all items in the current list
static void
ddb_listview_deselect_all (DdbListview *listview)
{
    DdbListviewIter it;
    int idx = 0;
    for (it = listview->binding->head (); it; idx++) {
        if (listview->binding->is_selected (it)) {
            listview->binding->select (it, 0);
            ddb_listview_draw_row (listview, idx, it);
            listview->binding->selection_changed (listview, it, idx);
        }
        DdbListviewIter next = listview->binding->next (it);
        listview->binding->unref (it);
        it = next;
    }
    if (it) {
        listview->binding->unref (it);
    }
}

// (De)select a whole group
// grp = group to be (de)selected
// item_idx = index of first item in the group or -1
// deselect: 0 = select group, 1 = deselect group
static void
ddb_listview_select_group (DdbListview *listview, DdbListviewGroup *grp, int first_item_idx, int deselect)
{
    if (grp == NULL) {
        return;
    }
    DdbListviewIter it = grp->head;
    listview->binding->ref (it);
    if (first_item_idx == -1) {
        first_item_idx = listview->binding->get_idx (it);
    }
    for (int group_idx = 0; it && group_idx < grp->num_items; group_idx++) {
        if (deselect) {
            listview->binding->select (it, 0);
        }
        else {
            listview->binding->select (it, 1);
        }
        ddb_listview_draw_row (listview, first_item_idx + group_idx, it);
        listview->binding->selection_changed (listview, it, first_item_idx + group_idx);
        DdbListviewIter next = listview->binding->next (it);
        listview->binding->unref (it);
        it = next;
    }
    if (it) {
        listview->binding->unref (it);
    }

    ddb_listview_refresh (listview, DDB_REFRESH_LIST);
}

// Toggle selection of group
// if at least one item of the group is selected, deselect the whole group
// if no item of the group is selected, select all group items
// grp: group to be toggled
// item_idx: index of first item in the group or -1
static void
ddb_listview_toggle_group_selection (DdbListview *listview, DdbListviewGroup *grp, int item_idx)
{
    if (grp == NULL) {
        return;
    }
    DdbListviewIter it = grp->head;
    listview->binding->ref (it);
    if (item_idx == -1) {
        item_idx = listview->binding->get_idx (it);
    }
    int deselect = 0;
    int group_idx = 0;
    // check if at least one item is selected
    for (group_idx = 0; it && group_idx < grp->num_items; group_idx++) {
        if (listview->binding->is_selected (it)) {
            deselect = 1;
            break;
        }
        DdbListviewIter next = listview->binding->next (it);
        listview->binding->unref (it);
        it = next;
    }
    if (it) {
        listview->binding->unref (it);
    }

    ddb_listview_select_group (listview, grp, item_idx, deselect);
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
}

static void
ddb_listview_update_cursor (DdbListview *ps, int cursor)
{
    int prev = ps->binding->cursor ();
    ps->binding->set_cursor (cursor);
    if (cursor != -1) {
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

static void
ddb_listview_select_range (DdbListview *ps, int start, int end)
{
    int nchanged = 0;
    int idx = 0;
    DdbListviewIter it;
    for (it = ps->binding->head (); it; idx++) {
        if (idx >= start && idx <= end) {
            if (!ps->binding->is_selected (it)) {
                ps->binding->select (it, 1);
                nchanged++;
                if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (ps, it, idx);
                }
            }
        }
        else {
            if (ps->binding->is_selected (it)) {
                ps->binding->select (it, 0);
                nchanged++;
                if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                    ddb_listview_draw_row (ps, idx, it);
                    ps->binding->selection_changed (ps, it, idx);
                }
            }
        }
        DdbListviewIter next = PL_NEXT (it);
        UNREF (it);
        it = next;
    }
    if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
        ddb_listview_refresh (ps, DDB_REFRESH_LIST);
        ps->binding->selection_changed (ps, NULL, -1);
    }
}

static int
ddb_listview_is_group_selected (DdbListview *ps, DdbListviewGroup *grp)
{
    if (!ps || !grp) {
        return 0;
    }

    DdbListviewIter it = grp->head;
    ps->binding->ref(it);
    for (int i = 0; i < grp->num_items && it; ++i) {
        if (!ps->binding->is_selected (it)) {
            ps->binding->unref (it);
            return 0;
        }
        DdbListviewIter next = ps->binding->next (it);
        ps->binding->unref (it);
        it = next;
    }
    if (it) {
        ps->binding->unref (it);
    }

    return 1;
}

void
ddb_listview_click_selection (DdbListview *ps, int ex, int ey, DdbListviewPickContext *pick_ctx, int dnd, int button) {
    deadbeef->pl_lock ();
    ps->areaselect = 0;
    ddb_listview_groupcheck (ps);

    if (dnd) {
        // prepare area selection or drag
        int selected = 0;
        if (pick_ctx->type == PICK_ALBUM_ART || pick_ctx->type == PICK_GROUP_TITLE) {
            selected = ddb_listview_is_group_selected (ps, pick_ctx->grp);
        }
        else {
            DdbListviewIter it = ps->binding->get_for_idx (pick_ctx->item_idx);
            if (it) {
                selected = ps->binding->is_selected (it);
                UNREF (it);
            }
        }
        if (!selected || pick_ctx->type == PICK_EMPTY_SPACE) {
            ps->areaselect = 1;
            ps->areaselect_x = ex + ps->hscrollpos;
            ps->areaselect_y = ey + ps->scrollpos;
            ps->shift_sel_anchor = pick_ctx->item_idx;
        }
        else if (selected && pick_ctx->type != PICK_EMPTY_SPACE && ps->binding->drag_n_drop) {
            ps->dragwait = 1;
        }
    }

    if (pick_ctx->type == PICK_EMPTY_SPACE) {
        // clicked empty space, deselect everything
        ddb_listview_deselect_all (ps);
    }
    else if (pick_ctx->item_idx != -1
            && (pick_ctx->type == PICK_GROUP_TITLE
                || pick_ctx->type == PICK_ALBUM_ART)) {
        // clicked group title or album art column, select group
        int start = pick_ctx->item_grp_idx;
        int end = start + pick_ctx->grp->num_items - 1;
        ddb_listview_select_range (ps, start, end);
    }
    else if (pick_ctx->item_idx != -1 && pick_ctx->type == PICK_ITEM) {
        // clicked specific item - select, or start drag-n-drop
        DdbListviewIter it = ps->binding->get_for_idx (pick_ctx->item_idx);
        if (it) {
            if (!ps->binding->is_selected (it)) {
                // reset selection, and set it to single item
                ddb_listview_select_single (ps, pick_ctx->item_idx);
            }
            UNREF (it);
        }
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

    // get item under cursor
    DdbListviewPickContext pick_ctx;
    ddb_listview_list_pickpoint (ps, ex, ey + ps->scrollpos, &pick_ctx);

    int group_clicked = (pick_ctx.type == PICK_ALBUM_ART
                        || pick_ctx.type == PICK_GROUP_TITLE) ? 1 : 0;

    int cursor = ps->binding->cursor ();
    if (type == GDK_2BUTTON_PRESS
            && fabs(ps->lastpos[0] - ex) < 3
            && fabs(ps->lastpos[1] - ey) < 3) {
        // doubleclick - play this item
        if (pick_ctx.item_idx != -1
                && pick_ctx.type != PICK_EMPTY_SPACE
                && cursor != -1) {
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

    // set cursor
    int prev = cursor;
    if (pick_ctx.type != PICK_EMPTY_SPACE
            && pick_ctx.item_idx != -1) {
        int new_cursor = -1;
        if (pick_ctx.type == PICK_ALBUM_ART) {
            // set cursor to first item in clicked group
            new_cursor = pick_ctx.item_grp_idx;
        }
        else {
            // set cursor on clicked item
            new_cursor = pick_ctx.item_idx;
        }
        ddb_listview_update_cursor (ps, new_cursor);
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
        ddb_listview_click_selection (ps, ex, ey, &pick_ctx, 1, 1);
    }
    else if (state & selmask) {
        // toggle selection
        if (pick_ctx.type != PICK_EMPTY_SPACE
                && pick_ctx.item_idx != -1) {
            if (group_clicked) {
                // toggle group items
                ddb_listview_toggle_group_selection (ps, pick_ctx.grp, pick_ctx.item_idx);
            }
            else if (pick_ctx.type == PICK_ITEM) {
                // toggle single item
                DdbListviewIter it = ps->binding->get_for_idx (pick_ctx.item_idx);
                if (it) {
                    ps->binding->select (it, 1 - ps->binding->is_selected (it));
                    ddb_listview_draw_row (ps, pick_ctx.item_idx, it);
                    ps->binding->selection_changed (ps, it, pick_ctx.item_idx);
                    UNREF (it);
                }
            }
        }
    }
    else if (state & GDK_SHIFT_MASK) {
        if (group_clicked) {
            // deselect everything
            ddb_listview_deselect_all (ps);
            // select group
            ddb_listview_select_group (ps, pick_ctx.grp, pick_ctx.item_idx, 0);
        }
        else if (pick_ctx.type == PICK_ITEM || pick_ctx.type == PICK_EMPTY_SPACE) {
            // select range
            int cursor = pick_ctx.item_idx;
            if (prev > cursor && pick_ctx.type == PICK_EMPTY_SPACE) {
                cursor++;
            }
            int start = min (prev, cursor);
            int end = max (prev, cursor);

            ddb_listview_select_range (ps, start, end);
            ddb_listview_update_cursor (ps, cursor);
        }
    }
    cursor = ps->binding->cursor ();
    if (cursor != -1 && pick_ctx.item_idx == -1) {
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

#ifndef __APPLE__
    int selmask = GDK_CONTROL_MASK;
#else
    int selmask = GDK_MOD2_MASK;
#endif

    int select_single = 0;

    if (!ps->binding->drag_n_drop) {
        // playlist doesn't support drag and drop (e.g. search list)
        // select single track
        select_single = 1;
    }

    if (ps->dragwait) {
        // reset dragdrop and select single track
        ps->dragwait = 0;
        select_single = 1;
    }

    if (ps->areaselect) {
        // reset areaselection ctx without clearing selection
        ps->scroll_direction = 0;
        ps->scroll_pointer_x = -1;
        ps->scroll_pointer_y = -1;
        ps->areaselect = 0;
        ps->areaselect_x = -1;
        ps->areaselect_y = -1;
    }
    else if (select_single && !(state & (selmask|GDK_SHIFT_MASK))) {
        // clear selection and select single track
        DdbListviewPickContext pick_ctx;
        ddb_listview_list_pickpoint (ps, ex, ey + ps->scrollpos, &pick_ctx);
        if (pick_ctx.type == PICK_ITEM) {
            ddb_listview_select_single (ps, pick_ctx.item_idx);
        }
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
        ddb_listview_list_mousemove (ps, NULL, ps->scroll_pointer_x, ps->scroll_pointer_y);
    }
    else if (ps->scroll_mode == 1) {
        ddb_listview_list_track_dragdrop (ps, ps->scroll_pointer_x, ps->scroll_pointer_y);
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

static void
ddb_listview_select_tracks_within_region (DdbListview *ps, int x, int start_y, int end_y)
{
    DdbListviewPickContext pick_ctx_start;
    DdbListviewPickContext pick_ctx_end;

    int start_idx = -1;
    int end_idx = -1;

    // find start point
    ddb_listview_list_pickpoint (ps, x, start_y, &pick_ctx_start);
    start_idx = pick_ctx_start.item_idx;

    // find end point
    ddb_listview_list_pickpoint (ps, x, end_y, &pick_ctx_end);
    end_idx = pick_ctx_end.item_idx;

    if (start_idx == -1 || end_idx == -1) {
        // failed to find start or end track
        return;
    }

    if (start_idx == end_idx) {
        if (pick_ctx_start.type != PICK_EMPTY_SPACE || pick_ctx_end.type != PICK_EMPTY_SPACE) {
            ddb_listview_select_range (ps, start_idx, start_idx);
            ddb_listview_update_cursor (ps, start_idx);
        }
        else {
            ddb_listview_deselect_all (ps);
        }
        return;
    }

    int swapped = 0;
    if (start_idx > end_idx) {
        // swap start and end track
        swapped = 1;
        int temp_idx = end_idx;
        end_idx = start_idx;
        start_idx = temp_idx;

        DdbListviewPickContext pick_ctx_temp = pick_ctx_end;
        pick_ctx_end = pick_ctx_start;
        pick_ctx_start = pick_ctx_temp;
    }

    int total_tracks = ps->binding->count ();
    if (pick_ctx_start.type == PICK_EMPTY_SPACE) {
        start_idx = min (total_tracks - 1, start_idx + 1);
    }

    int cursor = -1;
    if (swapped) {
        cursor = start_idx;
    }
    else {
        cursor = end_idx;
    }

    ddb_listview_select_range (ps, start_idx, end_idx);
    ddb_listview_update_cursor (ps, cursor);
}

void
ddb_listview_list_mousemove (DdbListview *ps, GdkEventMotion *ev, int ex, int ey) {
    deadbeef->pl_lock ();
    GtkWidget *widget = ps->list;
    int move_threshold = gtk_drag_check_threshold (widget, ps->lastpos[0], ps->lastpos[1], ex, ey);

    if (move_threshold) {
        if (ps->dragwait) {
            ps->dragwait = 0;
            ps->drag_source_playlist = deadbeef->plt_get_curr_idx ();
            GtkTargetEntry entry = {
                .target = "DDB_URI_LIST",
                .flags = GTK_TARGET_SAME_WIDGET,
                .info = TARGET_SAMEWIDGET
            };
            GtkTargetList *lst = gtk_target_list_new (&entry, 1);
            gtk_drag_begin (widget, lst, GDK_ACTION_COPY | GDK_ACTION_MOVE, 1, (GdkEvent *)ev);
        }
        else if (ps->areaselect) {
            ddb_listview_select_tracks_within_region (ps, ps->areaselect_x, ps->areaselect_y, ey + ps->scrollpos);

            GtkAllocation a;
            gtk_widget_get_allocation (ps->list, &a);

            if (ey < 10) {
                ps->scroll_mode = 0;
                ps->scroll_pointer_x = ex;
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
                ps->scroll_pointer_x = ex;
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
                ps->scroll_pointer_x = -1;
                ps->scroll_pointer_y = -1;
            }
            // debug only
            // ddb_listview_list_dbg_draw_areasel (widget, event->x, event->y);
        }
    }
    deadbeef->pl_unlock ();
}

int
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
                gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
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

            // select all between shift_sel_anchor and deadbeef->pl_get_cursor (ps->iterator)
            int start = min (cursor, ps->shift_sel_anchor);
            int end = max (cursor, ps->shift_sel_anchor);

            ddb_listview_select_range (ps, start, end);
            ddb_listview_update_cursor (ps, cursor);
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
ddb_listview_dragdrop_get_row_from_coord (DdbListview *listview, int x, int y) {
    if (y == -1) {
        return -1;
    }
    DdbListviewPickContext pick_ctx;
    ddb_listview_list_pickpoint (listview, x, y + listview->scrollpos, &pick_ctx);

    int row_idx = -1;
    if (pick_ctx.type == PICK_ITEM || pick_ctx.type == PICK_ALBUM_ART) {
        row_idx = pick_ctx.item_idx;
        int it_y = ddb_listview_get_row_pos (listview, row_idx) - listview->scrollpos;
        if (y > it_y + listview->rowheight/2) {
            row_idx++;
        }
    }
    else if (pick_ctx.type == PICK_GROUP_TITLE) {
        // select first item item group
        row_idx = pick_ctx.item_grp_idx;
    }
    else if (pick_ctx.type == PICK_EMPTY_SPACE
            || pick_ctx.type == PICK_BELOW_PLAYLIST) {
        row_idx = pick_ctx.item_idx + 1;
    }
    else if (pick_ctx.type == PICK_ABOVE_PLAYLIST) {
        row_idx = 0;
    }
    return row_idx;
}

static void
ddb_listview_list_track_dragdrop (DdbListview *ps, int x, int y) {
    int prev_drag_y = ps->drag_motion_y;
    GtkAllocation a;
    gtk_widget_get_allocation (ps->list, &a);

    if (y == -1) {
        ps->drag_motion_y = -1;
        ps->scroll_active = 0;
        ps->scroll_direction = 0;
    }
    else {
        int sel = ddb_listview_dragdrop_get_row_from_coord (ps, x, y);
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
        if (ps->scrollpos > 0 && ps->drag_motion_y == ps->fullheight) {
            ps->drag_motion_y -= 3;
        }
    }

    if (prev_drag_y != ps->drag_motion_y) {
        if (prev_drag_y != -1) {
            // erase previous track
            gtk_widget_queue_draw_area (ps->list, 0, prev_drag_y-ps->scrollpos-3, a.width, 7);
        }
        if (ps->drag_motion_y != -1) {
            // new track
            gtk_widget_queue_draw_area (ps->list, 0, ps->drag_motion_y-ps->scrollpos-3, a.width, 7);
        }
    }

    if (y < 10) {
        ps->scroll_pointer_x = x;
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
        ps->scroll_pointer_x = x;
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
        ps->scroll_pointer_x = -1;
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
    ps->scroll_pointer_x = -1;
    ps->scroll_pointer_y = -1;
}

static void
draw_header_fg(DdbListview *ps, cairo_t *cr, DdbListviewColumn *c, GdkColor *clr, int x, int xx, int h) {
    int text_width = xx - x - 10;
    if (c->sort_order) {
        int arrow_sz = 10;
        text_width = max(0, text_width - arrow_sz);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_render_arrow(gtk_widget_get_style_context(theme_treeview), cr, c->sort_order*G_PI, xx-arrow_sz-5, h/2-arrow_sz/2, arrow_sz);
#else
        int dir = c->sort_order == 1 ? GTK_ARROW_DOWN : GTK_ARROW_UP;
        gtk_paint_arrow(ps->header->style, ps->header->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, ps->header, NULL, dir, TRUE, xx-arrow_sz-5, h/2-arrow_sz/2, arrow_sz, arrow_sz);
#endif
    }

    float fg[3] = {clr->red/65535., clr->green/65535., clr->blue/65535.};
    draw_set_fg_color(&ps->hdrctx, fg);
    cairo_save(cr);
    cairo_rectangle(cr, x+5, 0, text_width, h);
    cairo_clip(cr);
    draw_text_custom(&ps->hdrctx, x+5, 3, text_width, 0, DDB_COLUMN_FONT, 0, 0, c->title);
    cairo_restore(cr);
}

static void
ddb_listview_header_render (DdbListview *ps, cairo_t *cr, int x1, int x2) {
    cairo_set_line_width (cr, 1);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    GtkAllocation a;
    gtk_widget_get_allocation (ps->header, &a);
    draw_begin(&ps->hdrctx, cr);
    int h = a.height;

    // Paint the background for the whole header
    GdkColor gdkfg;
#if !GTK_HEADERS
    GdkColor clr;
    gtkui_get_tabstrip_base_color(&clr);
    draw_cairo_rectangle(cr, &clr, 0, 0, a.width, h);
    gtkui_get_tabstrip_dark_color(&clr);
    draw_cairo_line(cr, &clr, 0, h, a.width, h);
    gtkui_get_listview_column_text_color(&gdkfg);
    draw_cairo_line(cr, &gtk_widget_get_style(ps->header)->mid[GTK_STATE_NORMAL], 0, h, a.width, h);
#else
#if GTK_CHECK_VERSION(3,0,0)
    render_column_button(ps, cr, GTK_STATE_FLAG_NORMAL, 0, -1, a.width, h+2, &gdkfg);
#else
    gtk_paint_box(gtk_widget_get_style(theme_button), gtk_widget_get_window(ps->header), GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, ps->header, "button", -2, -2, a.width+4, h+4);
    gdkfg = gtk_widget_get_style(theme_button)->fg[GTK_STATE_NORMAL];
    draw_cairo_line(cr, &gtk_widget_get_style(ps->header)->mid[GTK_STATE_NORMAL], 0, h, a.width, h);
#endif
#endif
    int x = -ps->hscrollpos;
    int idx = 0;
    // Add a column header pseudo-button for each configured treeview column, by drawing lines across the background
    for (DdbListviewColumn *c = ps->columns; c && x < x2; c = c->next, idx++) {
        int xx = x + c->width;

        // Only render for columns within the clip region, and not any column which is being dragged
        if (idx != ps->header_dragging && xx >= x1) {
            // Paint the button text
            draw_header_fg(ps, cr, c, &gdkfg, x, xx, h);

            // Add a vertical line near the right side of the column width, but not right next to an empty slot
            if (c->width > 0 && ps->header_dragging != idx + 1) {
                if (gtkui_override_tabstrip_colors()) {
                    GdkColor clr;
                    gtkui_get_tabstrip_dark_color (&clr);
                    draw_cairo_line(cr, &clr, xx-2, 2, xx-2, h-4);
                    gtkui_get_tabstrip_light_color (&clr);
                    draw_cairo_line(cr, &clr, xx-1, 2, xx-1, h-4);
                }
                else {
#if GTK_CHECK_VERSION(3,0,0)
                    GtkStyleContext *context = gtk_widget_get_style_context(theme_treeview);
                    gtk_style_context_add_class(context, "separator");
                    gtk_render_line(context, cr, xx-3, 2, xx-3, h-4);
                    gtk_style_context_remove_class(context, "separator");
#else
                    gtk_paint_vline (ps->header->style, ps->header->window, GTK_STATE_NORMAL, NULL, ps->header, NULL, 2, h-4, xx-2);
#endif
                }
            }
        }
        x = xx;
    }

    // Do special drawing when a column is being dragged
    if (ps->header_dragging != -1) {
        x = -ps->hscrollpos;
        idx = 0;
        DdbListviewColumn *c = ps->columns;
        while (c && idx++ != ps->header_dragging) {
            x += c->width;
            c = c->next;
        }

        // Mark the position where the dragged column used to be with an indented/active/dark position
        int xx = x - 2; // Where the divider line is
        int w = c->width + 2;
        if (xx < x2) {
#if GTK_CHECK_VERSION(3,0,0)
            render_column_button(ps, cr, GTK_STATE_FLAG_ACTIVE, xx, 0, w, h, NULL);
#else
            gtk_paint_box(gtk_widget_get_style(theme_button), gtk_widget_get_window(ps->header), GTK_STATE_ACTIVE, GTK_SHADOW_ETCHED_IN, NULL, ps->header, "button", xx, 0, w, h);
#endif
        }

        // Draw a highlighted/selected "button" wherever the dragged column is currently positioned
        xx = ps->col_movepos - ps->hscrollpos - 2;
        if (w > 0 && xx < x2) {
#if GTK_CHECK_VERSION(3,0,0)
            render_column_button(ps, cr, GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_FOCUSED, xx, 0, w, h, &gdkfg);
#else
            gtk_paint_box(gtk_widget_get_style(theme_button), gtk_widget_get_window(ps->header), GTK_STATE_SELECTED, GTK_SHADOW_OUT, NULL, ps->header, "button", xx, 0, w, h);
            gdkfg = gtk_widget_get_style(theme_button)->fg[GTK_STATE_SELECTED];
#endif
            if (gtkui_override_listview_colors()) {
                gtkui_get_listview_selected_text_color(&gdkfg);
            }
            draw_header_fg(ps, cr, c, &gdkfg, xx, xx+w, h);
        }
    }

    draw_end (&ps->hdrctx);
}

#if GTK_CHECK_VERSION(3,0,0)
static gboolean
ddb_listview_header_draw                 (GtkWidget       *widget,
                                        cairo_t *cr,
                                        gpointer         user_data) {
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    GdkRectangle clip;
    gdk_cairo_get_clip_rectangle(cr, &clip);
    ddb_listview_header_expose (ps, cr, clip.x, clip.y, clip.width, clip.height);
    return TRUE;
}
#else
static gboolean
ddb_listview_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    ddb_listview_header_expose (ps, cr, event->area.x, event->area.y, event->area.width, event->area.height);
    cairo_destroy (cr);
    return TRUE;
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
ddb_listview_column_size_changed (DdbListview *listview, DdbListviewColumn *c)
{
    if (listview->binding->is_album_art_column(c->user_data)) {
        ddb_listview_resize_groups(listview);
        int pos = ddb_listview_get_row_pos(listview, listview->ref_point);
        gtk_range_set_value(GTK_RANGE(listview->scrollbar), pos - listview->ref_point_offset);
    }
}

void
ddb_listview_update_scroll_ref_point (DdbListview *ps)
{
    ddb_listview_groupcheck (ps);
    DdbListviewGroup *grp = ps->groups;
    DdbListviewGroup *grp_next;

    if (grp) {
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

static gboolean
ddb_listview_list_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));

    int prev_width = ps->list_width;
    if (event->width != prev_width || event->height != ps->list_height) {
        ps->list_width = event->width;
        ps->list_height = event->height;
        g_idle_add(ddb_listview_reconf_scrolling, ps);
    }
    if (event->width != prev_width) {
        ddb_listview_list_update_total_width(ps, total_columns_width(ps), event->width);
    }

    return FALSE;
}

static gboolean
ddb_listview_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
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
                        ddb_listview_column_size_changed (ps, c);
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

static void
set_header_cursor (DdbListview *listview, gdouble mousex)
{
    int x = -listview->hscrollpos;
    for (DdbListviewColumn *c = listview->columns; c; c = c->next) {
        if (mousex >= x + c->width - 4 && mousex <= x + c->width) {
            gdk_window_set_cursor(gtk_widget_get_window(listview->header), listview->cursor_sz);
            return;
        }
        x += c->width;
    }

    gdk_window_set_cursor(gtk_widget_get_window(listview->header), NULL);
}

static gboolean
ddb_listview_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
#if GTK_CHECK_VERSION(2,12,0)
    gdk_event_request_motions (event);
#endif

    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (ps->header_prepare) {
        if (ps->header_dragging != -1 && gtk_drag_check_threshold(widget, round(ps->prev_header_x), 0, round(event->x), 0)) {
            ps->header_prepare = 0;
        }
        else {
            return FALSE;
        }
    }

    if (ps->header_dragging >= 0) {
        gdk_window_set_cursor (gtk_widget_get_window (widget), ps->cursor_drag);
        DdbListviewColumn *c = ps->columns;
        for (int i = 0; c && i < ps->header_dragging; c = c->next, i++);
        int left = event->x - ps->header_dragpt[0] + ps->hscrollpos;
        int right = left + c->width;
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
        int x = -ps->hscrollpos;
        int i = 0;
        DdbListviewColumn *c;
        for (c = ps->columns; i < ps->header_sizing; c = c->next) {
            x += c->width;
            i++;
        }
        c->width = max(MIN_COLUMN_WIDTH, event->x - ps->header_dragpt[0] - x);
        if (ps->col_autoresize) {
            c->fwidth = (float)c->width / ps->header_width;
        }

        ddb_listview_column_size_changed(ps, c);
        ddb_listview_list_setup_hscroll(ps);
        gtk_widget_queue_draw(ps->header);
        gtk_widget_queue_draw(ps->list);
    }
    else {
        set_header_cursor(ps, event->x);
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

static gboolean
ddb_listview_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ps->prev_header_x = -1;
    if (TEST_LEFT_CLICK (event)) {
        ddb_listview_update_scroll_ref_point (ps);

        ps->header_dragging = -1;
        ps->header_sizing = -1;
        int x = -ps->hscrollpos;
        int i = 0;
        DdbListviewColumn *c = ps->columns;
        while (c && event->x > x + c->width) {
            i++;
            x += c->width;
            c = c->next;

        }
        ps->header_dragpt[0] = round(event->x);
        ps->header_dragpt[1] = round(event->y);
        ps->prev_header_x = event->x;
        if (!c) {
            ps->header_prepare = 1;
        }
        else if (event->x < x + c->width - 4) {
            ps->header_prepare = 1;
            ps->header_dragging = i;
            ps->header_dragpt[0] -= x;
        }
        else {
            ps->header_sizing = i;
            ps->header_dragpt[0] -= (x + c->width);
        }
    }
    else if (TEST_RIGHT_CLICK (event)) {
        int idx = ddb_listview_header_get_column_idx_for_coord (ps, event->x);
        ps->binding->header_context_menu (ps, idx);
    }
    return TRUE;
}

static gboolean
ddb_listview_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (event->button == 1) {
        GtkAllocation a;
        gtk_widget_get_allocation(ps->header, &a);
        if (ps->header_sizing != -1) {
            ps->binding->columns_changed (ps);
            ddb_listview_list_update_total_width(ps, total_columns_width(ps), a.width);
        }
        else if (ps->header_dragging != -1) {
            if (ps->header_prepare) {
                GtkAllocation a;
                gtk_widget_get_allocation(ps->header, &a);
                if (event->y >= 0 && event->y <= a.height) {
                    // sort
                    int x = -ps->hscrollpos;
                    int i = 0;
                    DdbListviewColumn *c = ps->columns;
                    while (c && event->x > x + c->width) {
                        i++;
                        x += c->width;
                        c = c->next;

                    }
                    if (c && event->x > x + 1 && event->x < x + c->width - 5) {
                        for (DdbListviewColumn *cc = ps->columns; cc; cc = cc->next) {
                            if (cc != c) {
                                cc->sort_order = 0;
                            }
                        }
                        if (!c->sort_order || c->sort_order == 2) {
                            c->sort_order = 1;
                        }
                        else {
                            c->sort_order = 2;
                        }
                        ps->binding->col_sort(i, c->sort_order-1, c->user_data);
                        ddb_listview_refresh(ps, DDB_REFRESH_LIST | DDB_REFRESH_COLUMNS);
                    }
                }
            }
            else {
                ps->header_dragging = -1;
                ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_COLUMNS | DDB_REFRESH_HSCROLL);
            }
        }
        set_header_cursor(ps, event->x);
        ps->header_sizing = -1;
        ps->header_dragging = -1;
        ps->header_prepare = 0;
    }
    return FALSE;
}

static gboolean
ddb_listview_header_enter (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (ps->header_prepare || ps->header_dragging != -1 || ps->header_sizing != -1) {
        return FALSE;
    }

    int x = event->x;
#if GTK_CHECK_VERSION(3,0,0)
    if (event->send_event) {
        GdkWindow *win = gtk_widget_get_window(widget);
        GdkDeviceManager *device_manager = gdk_display_get_device_manager(gdk_window_get_display(win));
        gdk_window_get_device_position(win, gdk_device_manager_get_client_pointer(device_manager), &x, NULL, NULL);
    }
#endif
    set_header_cursor(ps, x);
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
        DdbListviewPickContext pick_ctx;
        ddb_listview_list_pickpoint (ps, event->x, event->y + ps->scrollpos, &pick_ctx);

        ddb_listview_click_selection (ps, event->x, event->y, &pick_ctx, 0, event->button);

        int cursor = pick_ctx.item_idx;
        int group_clicked = (pick_ctx.type == PICK_ALBUM_ART
                || pick_ctx.type == PICK_GROUP_TITLE) ? 1 : 0;

        if (group_clicked) {
            cursor = pick_ctx.item_grp_idx;
        }
        ddb_listview_update_cursor (ps, cursor);

        if (pick_ctx.type != PICK_EMPTY_SPACE) {
            DdbListviewIter it = ps->binding->get_for_idx (pick_ctx.item_idx);
            if (it) {
                ps->binding->list_context_menu (ps, it, pick_ctx.item_idx);
                UNREF (it);
            }
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
    DdbListviewPickContext pick_ctx;
    ddb_listview_list_pickpoint (listview, x, y + listview->scrollpos, &pick_ctx);
    DdbListviewIter it = listview->binding->get_for_idx (pick_ctx.item_idx);
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
ddb_listview_column_alloc (const char *title, int width, int align_right, minheight_cb_t minheight_cb, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn * c = malloc (sizeof (DdbListviewColumn));
    memset (c, 0, sizeof (DdbListviewColumn));
    c->title = strdup (title);
    c->width = width;
    c->align_right = align_right;
    c->color_override = color_override;
    c->color = color;
    c->minheight_cb = minheight_cb;
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
ddb_listview_column_append (DdbListview *listview, const char *title, int width, int align_right, minheight_cb_t minheight_cb, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn* c = ddb_listview_column_alloc (title, width, align_right, minheight_cb, color_override, color, user_data);
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
ddb_listview_column_insert (DdbListview *listview, int before, const char *title, int width, int align_right, minheight_cb_t minheight_cb, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn *c = ddb_listview_column_alloc (title, width, align_right, minheight_cb, color_override, color, user_data);
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
ddb_listview_column_get_info (DdbListview *listview, int col, const char **title, int *width, int *align_right, minheight_cb_t *minheight_cb, int *color_override, GdkColor *color, void **user_data) {
    DdbListviewColumn *c;
    int idx = 0;
    for (c = listview->columns; c; c = c->next, idx++) {
        if (idx == col) {
            *title = c->title;
            *width = c->width;
            *align_right = c->align_right;
            if (minheight_cb) *minheight_cb = c->minheight_cb;
            *color_override = c->color_override;
            *color = c->color;
            *user_data = c->user_data;
            return 0;
        }
    }
    return -1;
}

int
ddb_listview_column_set_info (DdbListview *listview, int col, const char *title, int width, int align_right, minheight_cb_t minheight_cb, int color_override, GdkColor color, void *user_data) {
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
            c->minheight_cb = minheight_cb;
            c->color_override = color_override;
            c->color = color;
            c->user_data = user_data;
            listview->binding->columns_changed (listview);
            return 0;
        }
    }
    return -1;
}

void
ddb_listview_invalidate_album_art_columns (DdbListview *listview) {
    GtkAllocation a;
    gtk_widget_get_allocation (listview->list, &a);
    invalidate_album_art_cells(listview, 0, a.width, 0, a.height);
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

static int
ddb_listview_min_group_height(DdbListviewColumn *columns) {
    int min_height = 0;
    for (DdbListviewColumn *c = columns; c; c = c->next) {
        if (c->minheight_cb) {
            int col_min_height = c->minheight_cb(c->user_data, c->width);
            if (min_height < col_min_height) {
                min_height = col_min_height;
            }
        }
    }
    return min_height;
}

void
ddb_listview_build_groups (DdbListview *listview) {
    deadbeef->pl_lock ();
    int old_height = listview->fullheight;
    listview->groups_build_idx = listview->binding->modification_idx ();
    ddb_listview_free_groups (listview);
    listview->fullheight = 0;

    int min_height = ddb_listview_min_group_height(listview->columns);
    DdbListviewGroup *grp = NULL;
    char curr[1024];
    char str[1024];
    listview->plt = deadbeef->plt_get_curr ();

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
    listview->fullheight = 0;

    int min_height = ddb_listview_min_group_height(listview->columns);
    DdbListviewGroup *grp = listview->groups;
    while (grp) {
        grp->height = listview->grouptitle_height + grp->num_items * listview->rowheight;
        if (grp->height - listview->grouptitle_height < min_height) {
            grp->height = min_height + listview->grouptitle_height;
        }
        listview->fullheight += grp->height;
        grp = grp->next;
    }

    deadbeef->pl_unlock ();
    if (old_height != listview->fullheight) {
        g_idle_add(ddb_listview_reconf_scrolling, listview);
    }
}

void
ddb_listview_set_vscroll (DdbListview *listview, int scroll) {
    if (listview->scrollpos == -1) {
        listview->scrollpos = 0;
    }
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

static gboolean
ddb_listview_list_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (!ddb_listview_handle_keypress (listview, event->keyval, event->state)) {
        return on_mainwin_key_press_event (widget, event, user_data);
    }
    return TRUE;
}
