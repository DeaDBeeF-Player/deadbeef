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
#include "clipboard.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define DEFAULT_GROUP_TITLE_HEIGHT 30
#define SCROLL_STEP 20
#define AUTOSCROLL_UPDATE_FREQ 0.01f
#define NUM_ROWS_TO_NOTIFY_SINGLY 10
#define MIN_COLUMN_WIDTH 16
#define BLANK_GROUP_SUBDIVISION 100

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

//#define REF(it) {if (it) ps->binding->ref (it);}
#define UNREF(it) {if (it) ps->binding->unref(it);}

enum {
    INFO_TARGET_URIS, // gtk uses 0 info by default
    INFO_TARGET_PLAYLIST_ITEM_INDEXES,
    INFO_TARGET_PLAYITEM_POINTERS,
};

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
    unsigned show_tooltip : 1;
    unsigned is_artwork : 1;
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
static void ddb_listview_destroy(GObject *object);

// fwd decls
static void
ddb_listview_build_groups (DdbListview *listview);

static int
ddb_listview_resize_subgroup (DdbListview *listview, DdbListviewGroup *grp, int group_depth, int min_height, int min_no_artwork_height);
static void
ddb_listview_resize_groups (DdbListview *listview);
static void
ddb_listview_free_group (DdbListview *listview, DdbListviewGroup *group);
static void
ddb_listview_free_all_groups (DdbListview *listview);

static void
ddb_listview_update_fonts (DdbListview *ps);
static void
ddb_listview_header_update_fonts (DdbListview *ps);

////// list functions ////
static void
ddb_listview_list_render (DdbListview *ps, cairo_t *cr, GdkRectangle *clip);
static void
ddb_listview_list_render_row_background (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h, GdkRectangle *clip);
static void
ddb_listview_list_render_row_foreground (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int even, int idx, int y, int w, int h, int x1, int x2);
static void
ddb_listview_list_render_album_art (DdbListview *ps, cairo_t *cr, DdbListviewGroup *grp, int min_y, int grp_next_y, int y, GdkRectangle *clip);
static void
ddb_listview_list_track_dragdrop (DdbListview *ps, int x, int y);
int
ddb_listview_dragdrop_get_row_from_coord (DdbListview *listview, int x, int y);
void
ddb_listview_list_mousemove (DdbListview *ps, GdkEventMotion *event, int x, int y);
static gboolean
ddb_listview_list_setup_vscroll (void *user_data);
static gboolean
ddb_listview_list_setup_hscroll (void *user_data);
int
ddb_listview_get_row_pos (DdbListview *listview, int pos, int *accumulated_title_height);

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

static void
ddb_listview_list_realize                    (GtkWidget       *widget,
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

gboolean
ddb_listview_list_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
ddb_listview_list_popup_menu (GtkWidget *widget, gpointer user_data);

static void
ddb_listview_list_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

static gboolean
ddb_listview_list_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

static gboolean
ddb_listview_list_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

static void
ddb_listview_list_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

static void
ddb_listview_list_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

static gboolean
ddb_listview_list_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data);

static void
ddb_listview_list_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data);

static gboolean
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

static gboolean
header_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer p);

static gboolean
list_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer p);

static void
ddb_listview_class_init(DdbListviewClass *class)
{
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
    listview->fwidth = -1;

    listview->columns = NULL;
    listview->lock_columns = -1;
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

    listview->header = gtk_drawing_area_new ();
    gtk_widget_show (listview->header);
    gtk_box_pack_start (GTK_BOX (vbox), listview->header, FALSE, TRUE, 0);
    gtk_widget_set_events (listview->header, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK);

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

    GValue value = {0, };
    g_value_init (&value, G_TYPE_BOOLEAN);
    g_value_set_boolean (&value, TRUE);
    g_object_set_property (G_OBJECT (listview->header), "has-tooltip", &value);
    g_signal_connect (G_OBJECT (listview->header), "query-tooltip", G_CALLBACK (header_tooltip_handler), listview);
    g_object_set_property (G_OBJECT (listview->list), "has-tooltip", &value);
    g_signal_connect (G_OBJECT (listview->list), "query-tooltip", G_CALLBACK (list_tooltip_handler), listview);
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

    ddb_listview_free_all_groups (listview);

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
    DdbListviewGroupFormat *fmt = listview->group_formats;
    while (fmt) {
        DdbListviewGroupFormat *next_fmt = fmt->next;
        free (fmt->format);
        free (fmt->bytecode);
        free (fmt);
        fmt = next_fmt;
    }
    ddb_listview_cancel_autoredraw (listview);

    draw_free (&listview->listctx);
    draw_free (&listview->grpctx);
    draw_free (&listview->hdrctx);
}

void
ddb_listview_refresh (DdbListview *listview, uint32_t flags) {
    if (flags & DDB_REFRESH_CONFIG) {
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
        g_idle_add_full(GTK_PRIORITY_RESIZE, ddb_listview_list_setup_vscroll, listview, NULL);
    }
    if (flags & DDB_REFRESH_HSCROLL) {
        g_idle_add_full(GTK_PRIORITY_RESIZE, ddb_listview_list_setup_hscroll, listview, NULL);
    }
    if (flags & DDB_REFRESH_COLUMNS) {
        gtk_widget_queue_draw (listview->header);
    }
}

static gboolean
ddb_listview_reconf_scrolling (void *ps) {
    ddb_listview_list_setup_vscroll (ps);
    ddb_listview_list_setup_hscroll (ps);
    return FALSE;
}

static void
ddb_listview_update_scroll_ref_point (DdbListview *ps);

static void
set_fwidth (DdbListview *ps, float list_width);

static void
autoresize_columns (DdbListview *listview, int list_width, int list_height);

static void
_update_fwidth (DdbListview *ps, int prev_width) {
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (ps), &a);
    if (ps->lock_columns != -1 && ps->view_realized) {
        if (!deadbeef->conf_get_int ("gtkui.autoresize_columns", 0) || ps->header_sizing != -1) {
            set_fwidth (ps, a.width);
        }
        else if (a.width != prev_width) {
            ddb_listview_update_scroll_ref_point (ps);
            if (ps->fwidth == -1) {
                set_fwidth (ps, prev_width);
            }
            autoresize_columns (ps, a.width, a.height);
        }
    }
}

static gboolean
_initial_resizing_finished (void *ctx) {
    DdbListview *ps = ctx;
    ps->view_realized = 1;
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (ps), &a);
    _update_fwidth (ps, a.width);
    gtk_widget_queue_draw (GTK_WIDGET(ps));
    return FALSE;
}

static void
ddb_listview_list_realize                    (GtkWidget       *widget,
        gpointer         user_data)
{
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(widget), "owner"));
    if (listview->binding->drag_n_drop) {
        GtkTargetEntry entries[] = {
            {
                .target = TARGET_PLAYLIST_AND_ITEM_INDEXES,
                .flags = GTK_TARGET_SAME_APP,
                .info = INFO_TARGET_PLAYLIST_ITEM_INDEXES
            },
            {
                .target = TARGET_PLAYITEM_POINTERS,
                .flags = GTK_TARGET_SAME_APP,
                .info = INFO_TARGET_PLAYITEM_POINTERS,
            }
        };
        // setup drag-drop target
        gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, entries, 2, GDK_ACTION_COPY | GDK_ACTION_MOVE);
        gtk_drag_dest_add_uri_targets (widget);
    }
    ddb_listview_update_fonts(listview);

    // defer column autoresizing until after the initial window resizing settles down
    g_timeout_add (100, _initial_resizing_finished, listview);
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
    listview->totalwidth = max (columns_width, width);
}

static DdbListviewIter
next_playitem (DdbListview *listview, DdbListviewIter it) {
    DdbListviewIter next = listview->binding->next(it);
    listview->binding->unref(it);
    return next;
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
    int col_x = -listview->hscrollpos;
    for (DdbListviewColumn *c = listview->columns; c && col_x <= x; c = c->next) {
        if (x <= col_x + c->width && listview->binding->is_album_art_column(c->user_data)) {
            return 1;
        }
        col_x += c->width;
    }
    return 0;
}

// returns Y coordinate of an item by its index
static int
ddb_listview_get_row_pos_subgroup (DdbListview *listview, DdbListviewGroup *grp, int y, int idx, int row_idx, int *accum) {
    while (grp) {
        int title_height = 0;
        if (grp->group_label_visible) {
            title_height = listview->grouptitle_height;
        }
        if (idx + grp->num_items > row_idx) {
            int i;
            if (grp->subgroups) {
                i = ddb_listview_get_row_pos_subgroup (listview, grp->subgroups, y + title_height, idx, row_idx, accum);
            }
            else {
                i = y + title_height + (row_idx - idx) * listview->rowheight;
            }
            *accum += title_height;
            return i;
        }
        y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }
    return y;
}

int
ddb_listview_get_row_pos (DdbListview *listview, int row_idx, int *accumulated_title_height) {
    int accum = 0;
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
    int y = ddb_listview_get_row_pos_subgroup (listview, listview->groups, 0, 0, row_idx, &accum);
    deadbeef->pl_unlock ();
    if (accumulated_title_height) {
        *accumulated_title_height = accum;
    }
    return y;
}

static int
ddb_listview_is_empty_region (DdbListviewPickContext *pick_ctx)
{
    switch (pick_ctx->type) {
        case PICK_BELOW_PLAYLIST:
        case PICK_ABOVE_PLAYLIST:
        case PICK_EMPTY_SPACE:
            return 1;
        default:
            return 0;
    }
}

static int
ddb_listview_list_pickpoint_subgroup (DdbListview *listview, DdbListviewGroup *grp, int x, int y, int idx, int grp_y, int group_level, int pin_offset, DdbListviewPickContext *pick_ctx) {
    const int orig_y = y;
    const int ry = y - listview->scrollpos;
    const int rowheight = listview->rowheight;
    const int is_album_art_column = ddb_listview_is_album_art_column (listview, x);

    while (grp) {
        const int h = grp->height;
        const int grp_title_height = grp->group_label_visible ? listview->grouptitle_height : 0;
        if (y >= grp_y && y < grp_y + h) {
            pick_ctx->grp = grp;
            y -= grp_y;
            if (y < grp_title_height || (pin_offset < ry && ry < grp_title_height + pin_offset && gtkui_groups_pinned)) {
                // group title
                pick_ctx->type = PICK_GROUP_TITLE;
                pick_ctx->item_grp_idx = idx;
                pick_ctx->item_idx = idx;
                pick_ctx->grp_idx = 0;
            }
            else if (is_album_art_column && group_level == listview->artwork_subgroup_level) {
                pick_ctx->type = PICK_ALBUM_ART;
                pick_ctx->item_grp_idx = idx;
                pick_ctx->grp_idx = min ((y - grp_title_height) / rowheight, grp->num_items - 1);
                pick_ctx->item_idx = idx + pick_ctx->grp_idx;
            }
            else if (grp->subgroups && ddb_listview_list_pickpoint_subgroup (listview, grp->subgroups, x, orig_y, idx, grp_y + grp_title_height, group_level + 1, pin_offset + grp_title_height, pick_ctx)) {
                // do nothing: the recursive call already set pick_ctx
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
            return 1;
        }
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }
    return 0;
}

// input: absolute y coord in list (not in window)
// returns -1 if nothing was hit, otherwise returns pointer to a group, and item idx
// item idx may be set to -1 if group title was hit
static void
ddb_listview_list_pickpoint (DdbListview *listview, int x, int y, DdbListviewPickContext *pick_ctx) {
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
    ddb_listview_groupcheck (listview);
    int found = ddb_listview_list_pickpoint_subgroup (listview, listview->groups, x, y, 0, 0, 0, 0, pick_ctx);
    deadbeef->pl_unlock ();

    if (!found) {
        // area at the end of playlist or unknown
        pick_ctx->type = PICK_EMPTY_SPACE;
        pick_ctx->item_grp_idx = -1;
        pick_ctx->grp_idx = -1;
        pick_ctx->item_idx = listview->binding->count () - 1;
        pick_ctx->grp = NULL;
    }
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
        gtk_style_context_save (context);
        gtk_style_context_set_state(context, selected ? GTK_STATE_FLAG_SELECTED : GTK_STATE_FLAG_NORMAL);
        gtk_style_context_add_region(context, GTK_STYLE_REGION_ROW, even ? GTK_REGION_EVEN : GTK_REGION_ODD);
        gtk_render_background(context, cr, x, y, w, h);
        gtk_style_context_restore (context);
#else
        GTK_WIDGET_SET_FLAGS(theme_treeview, GTK_HAS_FOCUS);
        gtk_paint_flat_box(gtk_widget_get_style(theme_treeview), gtk_widget_get_window(listview->list), selected ? GTK_STATE_SELECTED : GTK_STATE_NORMAL, GTK_SHADOW_NONE, clip, theme_treeview, even ? "cell_even_ruled" : "cell_odd_ruled", x, y, w, h);
        GTK_WIDGET_UNSET_FLAGS(theme_treeview, GTK_HAS_FOCUS);
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
ddb_listview_list_render_subgroup (DdbListview *listview, cairo_t *cr, GdkRectangle *clip, DdbListviewGroup *grp, int idx, int grp_y, const int cursor_index, const int current_group_depth, int title_offset, const int subgroup_artwork_offset, const int pin_offset) {
    const int scrollx = -listview->hscrollpos;
    const int row_height = listview->rowheight;
    const int total_width = listview->totalwidth;

    // find 1st group
    while (grp && grp_y + grp->height < clip->y) {
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }

    while (grp && grp_y < clip->y + clip->height) {
        const int title_height = grp->group_label_visible ? listview->grouptitle_height : 0;
        const int is_pinned = gtkui_groups_pinned && grp_y < pin_offset && grp_y + grp->height >= 0;

        // only render list items when at the deepest group level
        if (!grp->subgroups) {
            DdbListviewIter it = grp->head;
            listview->binding->ref(it);
            for (int i = 0, yy = grp_y + title_height; it && i < grp->num_items && yy < clip->y + clip->height; i++, yy += row_height) {
                if (yy + row_height >= clip->y && (!gtkui_groups_pinned || yy + row_height >= pin_offset)) {
                    ddb_listview_list_render_row_background(listview, cr, it, i & 1, idx+i == cursor_index, scrollx, yy, total_width, row_height, clip);
                    ddb_listview_list_render_row_foreground(listview, cr, it, i & 1, idx+i, yy, total_width, row_height, clip->x, clip->x+clip->width);
                }
                it = next_playitem(listview, it);
            }
            if (it) {
                listview->binding->unref(it);
            }
        }

        int subgroup_title_offset;
        if (current_group_depth == listview->artwork_subgroup_level) {
            subgroup_title_offset = subgroup_artwork_offset;
        }
        else {
            subgroup_title_offset = title_offset + (grp->group_label_visible ? listview->subgroup_title_padding : 0);
        }

        if (grp->subgroups) {
            // render subgroups before album art and titles
            ddb_listview_list_render_subgroup(listview, cr, clip, grp->subgroups, idx, grp_y + title_height, cursor_index, current_group_depth + 1, subgroup_title_offset, subgroup_artwork_offset, pin_offset + title_height);
        }

        int grp_next_y = grp_y + grp->height;
        if (current_group_depth == listview->artwork_subgroup_level) {
            // draw album art
            int min_y = 0;
            if (is_pinned) {
                if (grp->group_label_visible) {
                    min_y = min(title_height+pin_offset, grp_next_y);
                }
            }
            else {
                min_y = grp_y+title_height;
            }
            ddb_listview_list_render_album_art(listview, cr, grp, min_y, grp_next_y, grp_y + title_height, clip);
        }

        if (is_pinned && clip->y <= title_height + pin_offset) {
            // draw pinned group title
            int y = min(pin_offset, grp_next_y-title_height);
            fill_list_background(listview, cr, scrollx, y, total_width, title_height, clip);
            if (listview->binding->draw_group_title && title_height > 0) {
                listview->binding->draw_group_title(listview, cr, grp->head, title_offset, y, total_width-title_offset, title_height, current_group_depth);
            }
        }
        else if (clip->y <= grp_y + title_height) {
            // draw normal group title
            if (listview->binding->draw_group_title && title_height > 0) {
                listview->binding->draw_group_title(listview, cr, grp->head, title_offset, grp_y, total_width, title_height, current_group_depth);
            }
        }

        idx += grp->num_items;
        grp_y += grp->height;
        grp = grp->next;
    }
}

static void
ddb_listview_list_render (DdbListview *listview, cairo_t *cr, GdkRectangle *clip) {
    if (listview->scrollpos == -1) {
        return; // too early
    }
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);

    int cursor_index = listview->binding->cursor();

    // Calculate which side of the playlist the (first) album art cover is on to tell where to draw subgroup titles
    int subgroup_artwork_offset = listview->subgroup_title_padding;
    int x = 0;
    for (DdbListviewColumn *c = listview->columns; c; x += c->width, c = c->next) {
        if (listview->binding->is_album_art_column(c->user_data)) {
            int middle = x + c->width / 2;
            if (middle < listview->totalwidth / 2) {
                subgroup_artwork_offset = -listview->hscrollpos + x + c->width;
            }
            break;
        }
    }

    draw_begin (&listview->listctx, cr);
    draw_begin (&listview->grpctx, cr);
    fill_list_background(listview, cr, clip->x, clip->y, clip->width, clip->height, clip);

    ddb_listview_list_render_subgroup(listview, cr, clip, listview->groups, 0, -listview->scrollpos, cursor_index, 0, -listview->hscrollpos, subgroup_artwork_offset, 0);

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

    GdkColor clr;
    gtkui_get_listview_cursor_color (&clr);
    draw_cairo_rectangle(cr, &clr, 0, drag_motion_y-1, ps->list_width, 3);
    draw_cairo_rectangle(cr, &clr, 0, drag_motion_y-3, 3, 7);
    draw_cairo_rectangle(cr, &clr, ps->list_width-3, drag_motion_y-3, 3, 7);

}

static void
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
draw_list_rectangle (DdbListview *ps, cairo_t *cr, GdkRectangle *clip)
{
    cairo_rectangle(cr, clip->x, clip->y, clip->width, clip->height);
    cairo_clip(cr);
    cairo_set_line_width(cr, 1);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    ddb_listview_list_render(ps, cr, clip);
    if (ps->drag_motion_y >= 0 && ps->drag_motion_y-ps->scrollpos-3 < clip->y+clip->height && ps->drag_motion_y-ps->scrollpos+3 >= clip->y) {
        ddb_listview_draw_dnd_marker(ps, cr);
    }
}

#if GTK_CHECK_VERSION(3,0,0)
static int
list_is_realized (DdbListview *listview) {
    return gtk_widget_get_realized (GTK_WIDGET (listview));
}

static gboolean
ddb_listview_list_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data(G_OBJECT (widget), "owner"));
    if (!list_is_realized (ps)) {
        return FALSE;
    }
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
        draw_list_rectangle(ps, cr, &clip);
        cairo_restore(cr);
    }
    cairo_rectangle_list_destroy(list);
    return TRUE;
}
#else
static int
list_is_realized (DdbListview *listview) {
    return GTK_OBJECT_FLAGS(listview) & GTK_REALIZED && gtk_widget_get_style(theme_treeview)->depth != -1;
}
static gboolean
ddb_listview_list_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data(G_OBJECT (widget), "owner"));

    if (!ps->view_realized) {
        return FALSE; // drawing was called too early
    }

    GdkRectangle *rectangles;
    int num_rectangles;
    gdk_region_get_rectangles(event->region, &rectangles, &num_rectangles);
    for (int i = 0; i < num_rectangles; i++) {
        GdkRectangle *clip = &rectangles[i];
        cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
        draw_list_rectangle(ps, cr, clip);
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

static int
find_subgroup_title_heights (DdbListview *ps, DdbListviewGroup *group, int group_y, int at_y)
{
    while (group->next && group_y + group->height < at_y) {
        group_y += group->height;
        group = group->next;
    }

    int height = group->group_label_visible ? ps->grouptitle_height : 0;
    if (group->subgroups) {
        height += find_subgroup_title_heights(ps, group->subgroups, group_y, at_y);
    }

    return height;
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

    int group_titles_height = group->group_label_visible ? ps->grouptitle_height : 0;
    if (group->subgroups) {
        group_titles_height += find_subgroup_title_heights(ps, group->subgroups, next_group_y - group->height, at_y);
    }

    int group_height = next_group_y - at_y;
    if (next_group_y > at_y) {
        gtk_widget_queue_draw_area (ps->list, 0, 0, ps->list_width, min(group_titles_height, group_height));
    }
    if (group_height > group_titles_height) {
        invalidate_album_art_cells (ps, 0, ps->list_width, group_titles_height, group_height);
    }
}

static void
ddb_listview_vscroll_value_changed (GtkRange *widget, gpointer user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    int newscroll = round(gtk_range_get_value (GTK_RANGE (widget)));
    if (newscroll == ps->scrollpos) {
        return;
    }

    if (ps->binding->vscroll_changed) {
        ps->binding->vscroll_changed (newscroll);
    }
    if (gtkui_groups_pinned && ps->grouptitle_height > 0) {
        invalidate_group(ps, max(ps->scrollpos, newscroll));
    }
    GdkWindow *list_window = gtk_widget_get_window(ps->list);
    if (list_window) {
        gdk_window_scroll(list_window, 0, ps->scrollpos - newscroll);
    }
    ps->scrollpos = newscroll;
}

static void
ddb_listview_hscroll_value_changed (GtkRange *widget, gpointer user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    int newscroll = round(gtk_range_get_value (GTK_RANGE (widget)));
    if (newscroll == ps->hscrollpos) {
        return;
    }

    int diff = ps->hscrollpos - newscroll;
    GdkWindow *list_window = gtk_widget_get_window(ps->list);
    if (list_window) {
        gdk_window_scroll(gtk_widget_get_window(ps->header), diff, 0);
        gdk_window_scroll(gtk_widget_get_window(ps->list), diff, 0);
    }
    ps->hscrollpos = newscroll;
}

static gboolean
ddb_listview_list_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_list_track_dragdrop (pl, x, y);

    // NOTE: we need to check whether the source supports TARGET_PLAYLIST_AND_ITEM_INDEXES
    // in order to know which GdkDragAction to use
    GList *targets = gdk_drag_context_list_targets (drag_context);
    gboolean source_is_listview = FALSE;
    int cnt = g_list_length (targets);
    for (int i = 0; i < cnt; i++) {
        GdkAtom a = GDK_POINTER_TO_ATOM (g_list_nth_data (targets, i));
        gchar *nm = gdk_atom_name (a);
        if (!strcmp (nm, TARGET_PLAYLIST_AND_ITEM_INDEXES)) {
            source_is_listview = TRUE;
            g_free (nm);
            break;
        }
        g_free (nm);
    }

    if (!source_is_listview) {
        // source doesn't support TARGET_PLAYITEMS, so it's probably a file manager
        // and GDK_ACTION_MOVE wouldn't make any sense: only support GDK_ACTION_COPY
        gdk_drag_status (drag_context, GDK_ACTION_COPY, time);
    }
    else {
        // source is listview, so we can support both GDK_ACTION_MOVE and GDK_ACTION_COPY
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


static gboolean
ddb_listview_list_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    return TRUE;
}

static gchar **
ddb_listview_build_drag_uri_list (DdbListview *ps)
{
    deadbeef->pl_lock ();
    int num_selected = deadbeef->plt_get_sel_count (ps->drag_source_playlist);
    if (num_selected < 1) {
        // no track selected
        deadbeef->pl_unlock ();
        return NULL;
    }
    gchar **uri_list = g_new0 (gchar *, num_selected + 1);
    if (!uri_list) {
        deadbeef->pl_unlock ();
        return NULL;
    }
    // NOTE: hash table is used to detect duplicates since we don't want to
    // copy files more than once
    GHashTable *table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);


    DdbListviewIter it = deadbeef->plt_get_head (ps->drag_source_playlist);
    int idx = 0;
    while (it) {
        if (ps->binding->is_selected (it)) {
            const char *path = deadbeef->pl_find_meta (it, ":URI");

            gboolean is_local_file = FALSE;
            gboolean is_uri_scheme = FALSE;
            if (path[0] == '/') {
                is_local_file = TRUE;
            }
            else if (!strncasecmp (path, "file://", 7)) {
                is_local_file = TRUE;
                is_uri_scheme = TRUE;
            }
            if (is_local_file && !g_hash_table_lookup (table, path)) {
                // new track, add to hash table and uri list
                gchar *key = g_strdup (path);
                g_hash_table_replace (table, key, key);

                gchar *uri = NULL;
                if (!is_uri_scheme) {
                    // we need to convert file name to URI scheme
                    uri = g_filename_to_uri (path, NULL, NULL);
                }
                else {
                    // path is already URI scheme, just copy string
                    uri = g_strdup (path);
                }
                if (uri) {
                    uri_list[idx++] = uri;
                }
            }
        }
        it = next_playitem (ps, it);
    }
    uri_list[idx] = NULL;

    deadbeef->pl_unlock ();

    g_hash_table_destroy (table);
    table = NULL;

    return uri_list;
}

static void
ddb_listview_list_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *selection_data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));

    switch (target_type) {
    case INFO_TARGET_URIS:
        {
            // format as URI_LIST
            gchar **uris = ddb_listview_build_drag_uri_list (ps);
            if (uris) {
                gtk_selection_data_set_uris (selection_data, uris);
                g_strfreev (uris);
                uris = NULL;
            }
        }
        break;
    case INFO_TARGET_PLAYLIST_ITEM_INDEXES:
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
                it = next_playitem(ps, it);
            }
            GdkAtom target = gtk_selection_data_get_target (selection_data);
            gtk_selection_data_set (selection_data, target, sizeof (uint32_t) * 8, (const guchar *)ptr, (nsel+1) * sizeof (uint32_t));
            free (ptr);
        }
        break;
    default:
        g_assert_not_reached ();
    }
}


static void
ddb_listview_list_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *selection_data,
                                        guint            info,
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

    gchar *ptr=(char*)gtk_selection_data_get_data (selection_data);
    gint len = gtk_selection_data_get_length (selection_data);
    if (info == INFO_TARGET_PLAYITEM_POINTERS) {
        ddb_listview_clear_sort (ps);

        DdbListviewIter *tracks = (DdbListviewIter *)ptr;
        int count = len / sizeof (DdbListviewIter);

        if (ps->binding->tracks_copy_drag_n_drop != NULL) {
            ps->binding->tracks_copy_drag_n_drop (it, tracks, count);
        }

        for (int i = 0; i < count; i++) {
            ps->binding->unref (tracks[i]);
        }
    }
    else if (info == INFO_TARGET_URIS) {
        ddb_listview_clear_sort (ps);
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
    // list of 32bit ints, DDB_URI_LIST target
    else if (info == INFO_TARGET_PLAYLIST_ITEM_INDEXES) {
        ddb_listview_clear_sort (ps);
        uint32_t *d= (uint32_t *)ptr;
        int plt = *d;
        d++;
        int length = (len/4)-1;
        DdbListviewIter drop_before = it;
        // find last selected
        if (plt == deadbeef->plt_get_curr_idx ()) {
            while (drop_before && ps->binding->is_selected (drop_before)) {
                drop_before = next_playitem(ps, drop_before);
            }
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

static gboolean
ddb_listview_list_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data)
{
    return TRUE;
}


static void
ddb_listview_list_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data)
{
    DdbListview *pl = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_list_track_dragdrop (pl, -1, -1);
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

static gboolean
ddb_listview_list_setup_vscroll (void *user_data) {
    DdbListview *ps = user_data;
    ddb_listview_groupcheck (ps);
    adjust_scrollbar (ps->scrollbar, ps->fullheight, ps->list_height);
    return FALSE;
}

static gboolean
ddb_listview_list_setup_hscroll (void *user_data) {
    DdbListview *ps = user_data;
    int size = total_columns_width (ps);
    ddb_listview_list_update_total_width (ps, size, ps->list_width);
    adjust_scrollbar (ps->hscrollbar, size, ps->list_width);
    return FALSE;
}

void
ddb_listview_draw_row (DdbListview *listview, int row, DdbListviewIter it) {
    int y = ddb_listview_get_row_pos(listview, row, NULL) - listview->scrollpos;
    if (y + listview->rowheight > 0 && y <= listview->list_height) {
        gtk_widget_queue_draw_area (listview->list, 0, y, listview->list_width, listview->rowheight);
    }
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
ddb_listview_list_render_row_foreground (DdbListview *ps, cairo_t *cr, DdbListviewIter it, int even, int idx, int y, int w, int h, int x1, int x2) {
    int x = -ps->hscrollpos;
    for (DdbListviewColumn *c = ps->columns; c && x < x2; x += c->width, c = c->next) {
        if (x + c->width > x1 && !ps->binding->is_album_art_column(c->user_data)) {
            ps->binding->draw_column_data (ps, cr, it, idx, c->align_right, c->user_data, c->color_override ? &c->color : NULL, x, y, c->width, h, even);
        }
    }
}

static void
ddb_listview_list_render_album_art (DdbListview *ps, cairo_t *cr, DdbListviewGroup *grp, int min_y, int grp_next_y, int y, GdkRectangle *clip) {
    int x = -ps->hscrollpos;
    for (DdbListviewColumn *c = ps->columns; c && x < clip->x+clip->width; x += c->width, c = c->next) {
        if (ps->binding->is_album_art_column(c->user_data) && x + c->width > clip->x) {
            fill_list_background(ps, cr, x, y, c->width, grp->height-ps->grouptitle_height, clip);
            if (ps->grouptitle_height > 0) {
                ps->binding->draw_album_art(ps, cr, grp->head, c->user_data, min_y, grp_next_y, x, y, c->width, grp->height-ps->grouptitle_height);
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
    int notify_singly = listview->binding->sel_count() <= NUM_ROWS_TO_NOTIFY_SINGLY;
    DdbListviewIter it;
    int idx = 0;
    for (it = listview->binding->head (); it; idx++) {
        if (listview->binding->is_selected (it)) {
            listview->binding->select (it, 0);
            if (notify_singly) {
                ddb_listview_draw_row (listview, idx, it);
                listview->binding->selection_changed (listview, it, idx);
            }
        }
        it = next_playitem(listview, it);
    }
    if (!notify_singly) {
        ddb_listview_refresh (listview, DDB_REFRESH_LIST);
        listview->binding->selection_changed(listview, NULL, -1);
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
    int notify_singly = grp->num_items <= NUM_ROWS_TO_NOTIFY_SINGLY;
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
        if (notify_singly) {
            ddb_listview_draw_row (listview, first_item_idx + group_idx, it);
            listview->binding->selection_changed (listview, it, first_item_idx + group_idx);
        }
        it = next_playitem(listview, it);
    }
    if (it) {
        listview->binding->unref (it);
    }

    if (!notify_singly) {
        ddb_listview_refresh (listview, DDB_REFRESH_LIST);
        listview->binding->selection_changed(listview, NULL, -1);
    }
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
        it = next_playitem(listview, it);
    }
    if (it) {
        listview->binding->unref (it);
    }

    ddb_listview_select_group (listview, grp, item_idx, deselect);
}

void
ddb_listview_select_single (DdbListview *ps, int sel) {
    deadbeef->pl_lock ();
    ddb_listview_deselect_all(ps);
    DdbListviewIter sel_it = ps->binding->get_for_idx (sel);
    if (sel_it) {
        ps->binding->select (sel_it, 1);
        ddb_listview_draw_row (ps, sel, sel_it);
        ps->binding->selection_changed (ps, sel_it, sel);
        ps->binding->unref(sel_it);
    }
    deadbeef->pl_unlock ();
}

 void
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

struct set_cursor_t {
    int cursor;
    DdbListview *pl;
};

static gboolean
set_cursor_and_scroll_cb (gpointer data) {
    struct set_cursor_t *sc = (struct set_cursor_t *)data;
    ddb_listview_update_cursor (sc->pl, sc->cursor);
    ddb_listview_select_single (sc->pl, sc->cursor);

    int accumulated_title_height;
    int cursor_scroll = ddb_listview_get_row_pos (sc->pl, sc->cursor, &accumulated_title_height);
    int newscroll = sc->pl->scrollpos;
    if (!gtkui_groups_pinned && cursor_scroll < sc->pl->scrollpos) {
         newscroll = cursor_scroll;
    }
    else if (gtkui_groups_pinned && cursor_scroll < sc->pl->scrollpos + accumulated_title_height) {
        newscroll = cursor_scroll - accumulated_title_height;
    }
    else if (cursor_scroll + sc->pl->rowheight >= sc->pl->scrollpos + sc->pl->list_height) {
        newscroll = cursor_scroll + sc->pl->rowheight - sc->pl->list_height + 1;
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
ddb_listview_set_cursor_and_scroll (DdbListview *listview, int cursor) {
    struct set_cursor_t *data = malloc (sizeof (struct set_cursor_t));
    data->cursor = cursor;
    data->pl = listview;
    g_idle_add (set_cursor_and_scroll_cb, data);
}

void
ddb_listview_select_range (DdbListview *ps, int start, int end)
{
    int nchanged = 0;
    int idx = 0;
    DdbListviewIter it;
    for (it = ps->binding->head (); it; idx++) {
        if (idx >= start && idx <= end) {
            if (!ps->binding->is_selected (it)) {
                nchanged++;
                ps->binding->select (it, 1);
                ddb_listview_draw_row (ps, idx, it);
                if (nchanged <= NUM_ROWS_TO_NOTIFY_SINGLY) {
                    ps->binding->selection_changed (ps, it, idx);
                }
            }
        }
        else {
            if (ps->binding->is_selected (it)) {
                nchanged++;
                ps->binding->select (it, 0);
                ddb_listview_draw_row (ps, idx, it);
                if (nchanged <= NUM_ROWS_TO_NOTIFY_SINGLY) {
                    ps->binding->selection_changed (ps, it, idx);
                }
            }
        }
        it = next_playitem(ps, it);
    }
    if (nchanged > NUM_ROWS_TO_NOTIFY_SINGLY) {
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
        it = next_playitem(ps, it);
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
            && abs(ps->lastpos[0] - ex) < 3
            && abs(ps->lastpos[1] - ey) < 3) {
        // doubleclick - play this item
        if (pick_ctx.item_idx != -1
            && !ddb_listview_is_empty_region (&pick_ctx)
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

    gtkui_listview_busy = 1;

    // set cursor
    int prev = cursor;
    if (!ddb_listview_is_empty_region (&pick_ctx)
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
                ddb_listview_toggle_group_selection (ps, pick_ctx.grp, pick_ctx.item_grp_idx);
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
            ddb_listview_select_group (ps, pick_ctx.grp, pick_ctx.item_grp_idx, 0);
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
    gtkui_listview_busy = 0;

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
                .target = TARGET_PLAYLIST_AND_ITEM_INDEXES,
                .flags = GTK_TARGET_SAME_WIDGET,
                .info = INFO_TARGET_PLAYLIST_ITEM_INDEXES
            };
            GtkTargetList *lst = gtk_target_list_new (&entry, 1);
            gtk_target_list_add_uri_targets (lst, INFO_TARGET_URIS);
            gtk_drag_begin (widget, lst, GDK_ACTION_COPY | GDK_ACTION_MOVE, 1, (GdkEvent *)ev);
        }
        else if (ps->areaselect) {
            ddb_listview_select_tracks_within_region (ps, ps->areaselect_x, ps->areaselect_y, ey + ps->scrollpos);

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
            else if (ey > ps->list_height-10) {
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

gboolean
ddb_listview_list_popup_menu (GtkWidget *widget, gpointer user_data) {
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));

    ddb_playlist_t *playlist = deadbeef->plt_get_curr();
    if (playlist != NULL) {
        ps->binding->list_context_menu (playlist, PL_MAIN);
        deadbeef->plt_unref (playlist);
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
        int it_y = ddb_listview_get_row_pos (listview, row_idx, NULL) - listview->scrollpos;
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
                ps->drag_motion_y = ddb_listview_get_row_pos (ps, ps->binding->count ()-1, NULL) + ps->rowheight;
            }
        }
        else {
            ps->drag_motion_y = ddb_listview_get_row_pos (ps, sel, NULL);
        }
        if (ps->scrollpos > 0 && ps->drag_motion_y == ps->fullheight) {
            ps->drag_motion_y -= 3;
        }
    }

    if (prev_drag_y != ps->drag_motion_y) {
        if (prev_drag_y != -1) {
            // erase previous track
            gtk_widget_queue_draw_area (ps->list, 0, prev_drag_y-ps->scrollpos-3, ps->list_width, 7);
        }
        if (ps->drag_motion_y != -1) {
            // new track
            gtk_widget_queue_draw_area (ps->list, 0, ps->drag_motion_y-ps->scrollpos-3, ps->list_width, 7);
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
    else if (y > ps->list_height-10) {
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

static void
ddb_listview_list_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
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
    c->show_tooltip = draw_is_ellipsized(&ps->hdrctx);
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
                GdkColor clr;
                gtkui_get_tabstrip_dark_color (&clr);
                draw_cairo_line(cr, &clr, xx-2, 2, xx-2, h-4);
                gtkui_get_tabstrip_light_color (&clr);
                draw_cairo_line(cr, &clr, xx-1, 2, xx-1, h-4);
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

        if (!c) {
            draw_end (&ps->hdrctx);
            return;
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

static void
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

static void
ddb_listview_column_size_changed (DdbListview *listview, DdbListviewColumn *c)
{
    if (listview->binding->is_album_art_column(c->user_data)) {
        ddb_listview_resize_groups(listview);
        if (!listview->lock_columns) {
            int pos = ddb_listview_get_row_pos(listview, listview->ref_point, NULL);
            gtk_range_set_value(GTK_RANGE(listview->scrollbar), pos - listview->ref_point_offset);
        }
    }
}

static void
ddb_listview_update_scroll_ref_point_subgroup (DdbListview *ps, DdbListviewGroup *grp, int abs_idx, int grp_y)
{
    // find 1st group
    while (grp && grp_y + grp->height < ps->scrollpos) {
        grp_y += grp->height;
        abs_idx += grp->num_items;
        grp = grp->next;
    }

    if (!grp) {
        ps->ref_point = 0;
        ps->ref_point_offset = 0;
        return;
    }

    int grp_content_pos = grp_y + (grp->group_label_visible ? ps->grouptitle_height : 0);

    if (grp->subgroups) {
        // search subgroups for anchor
        ddb_listview_update_scroll_ref_point_subgroup (ps, grp->subgroups, abs_idx, grp_content_pos);
    }
    else {
        // choose first visible item as anchor
        int first_item_idx = max(0, (ps->scrollpos - grp_content_pos)/ps->rowheight);
        ps->ref_point = abs_idx + first_item_idx;
        ps->ref_point_offset = grp_content_pos + (first_item_idx * ps->rowheight) - ps->scrollpos;
    }
}

static void
ddb_listview_update_scroll_ref_point (DdbListview *ps)
{
    ddb_listview_groupcheck (ps);

    if (ps->groups) {
        ps->ref_point = 0;
        ps->ref_point_offset = 0;

        // choose cursor_pos as anchor
        int cursor_pos = ddb_listview_get_row_pos (ps, ps->binding->cursor (), NULL);
        if (ps->scrollpos < cursor_pos && cursor_pos < ps->scrollpos + ps->list_height && cursor_pos < ps->fullheight) {
            ps->ref_point = ps->binding->cursor ();
            ps->ref_point_offset = cursor_pos - ps->scrollpos;
        }
        else {
            ddb_listview_update_scroll_ref_point_subgroup (ps, ps->groups, 0, 0);
        }
    }
}

static void
set_column_width (DdbListview *listview, DdbListviewColumn *c, float new_width) {
    if (listview->fwidth != -1) {
        listview->fwidth -= (float)c->width / listview->list_width;
        c->fwidth = new_width / listview->list_width;
        listview->fwidth += c->fwidth;
    }
    c->width = new_width;
}

static void
set_fwidth (DdbListview *ps, float list_width)
{
    int total_width = 0;
    for (DdbListviewColumn *c = ps->columns; c; c = c->next) {
        c->fwidth = c->width / list_width;
        total_width += c->width;
    }
    ps->fwidth = total_width / list_width;
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

// Calculate the total height of all groups for a given min-height column width
static int
groups_full_height (DdbListview *listview, DdbListviewColumn *c, int new_width) {
    int min_height = c->minheight_cb (c->user_data, new_width);
    int full_height = 0;
    deadbeef->pl_lock ();
    for (DdbListviewGroup *grp = listview->groups; grp; grp = grp->next) {
        full_height += listview->grouptitle_height + max (grp->num_items * listview->rowheight, min_height);
    }
    deadbeef->pl_unlock ();
    return full_height;
}

// Calculate whether this width would cause auto-resize thrashing
static int
unsafe_group_height (DdbListview *listview, DdbListviewColumn *c, int new_width, int list_width, int list_height) {
    if (!c->minheight_cb) {
        return 0;
    }

    GtkAllocation a;
    gtk_widget_get_allocation (listview->scrollbar, &a);
    int scrollbar_width = a.width > 1 ? a.width : 16;
    if (listview->fullheight > list_height) {
        if (groups_full_height (listview, c, new_width) <= list_height) {
            int width_wo_scrollbar = roundf ((list_width + scrollbar_width) * c->fwidth);
            if (groups_full_height (listview, c, width_wo_scrollbar) >= list_height) {
                return 1;
            }
        }
    }
    else {
        if (groups_full_height (listview, c, new_width) >= list_height) {
            int width_w_scrollbar = roundf ((list_width - scrollbar_width) * c->fwidth);
            if (groups_full_height (listview, c, width_w_scrollbar) <= list_height) {
                return 1;
            }
        }
    }
    return 0;
}

static void
autoresize_columns (DdbListview *listview, int list_width, int list_height) {
    int total_width;
    int expected_width = roundf (list_width * listview->fwidth);
    float working_width = list_width;
    if (listview->fwidth > 1) {
        do {
            total_width = 0;
            for (DdbListviewColumn *c = listview->columns; c; c = c->next) {
                int new_width = max (MIN_COLUMN_WIDTH, roundf(working_width * c->fwidth));
                if (unsafe_group_height (listview, c, new_width, list_width, list_height)) {
                    new_width = c->width;
                }
                total_width += new_width;
                if (new_width != c->width) {
                    c->width = new_width;
                    ddb_listview_column_size_changed (listview, c);
                }
            }
            working_width++;
        } while (total_width <= expected_width);
    }
    else {
        do {
            total_width = 0;
            for (DdbListviewColumn *c = listview->columns; c; c = c->next) {
                int new_width = roundf (working_width * c->fwidth);
                if (new_width < MIN_COLUMN_WIDTH) {
                    working_width -= MIN_COLUMN_WIDTH - new_width;
                    new_width = MIN_COLUMN_WIDTH;
                }
                else if (unsafe_group_height (listview, c, new_width, list_width, list_height)) {
                    new_width = c->width;
                }
                total_width += new_width;
                if (new_width != c->width) {
                    c->width = new_width;
                    ddb_listview_column_size_changed (listview, c);
                }
            }
            working_width--;
        } while (total_width > expected_width && working_width > 0);
    }
    listview->binding->columns_changed (listview);
    ddb_listview_list_update_total_width (listview, total_width, list_width);
}

static gboolean
ddb_listview_list_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    int prev_width = ps->list_width;

    // the values in event->width/height are broken since GTK-3.22.1, so let's use widget's allocation instead, and hope this is reliable.
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (widget), &a);

    if (a.width != prev_width || a.height != ps->list_height) {
        ps->list_width = a.width;
        ps->list_height = a.height;
        g_idle_add_full (GTK_PRIORITY_RESIZE, ddb_listview_reconf_scrolling, ps, NULL);
    }
    if (a.width != prev_width) {
        ddb_listview_list_update_total_width (ps, total_columns_width(ps), a.width);
    }

    _update_fwidth (ps, prev_width);

    return FALSE;
}

void
ddb_listview_size_columns_without_scrollbar (DdbListview *listview) {
    if (deadbeef->conf_get_int ("gtkui.autoresize_columns", 0) && gtk_widget_get_visible (listview->scrollbar)) {
        GtkAllocation a;
        gtk_widget_get_allocation (listview->scrollbar, &a);
        autoresize_columns (listview, listview->list_width + a.width, listview->list_height);
    }
}

static void
ddb_listview_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data)
{
    // create cursor for sizing headers
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    listview->cursor_sz = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    listview->cursor_drag = gdk_cursor_new (GDK_FLEUR);
    ddb_listview_header_update_fonts (listview);
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
        if (c) {
            int left = event->x - ps->header_dragpt[0] + ps->hscrollpos;
            int right = left + c->width;
            DdbListviewColumn *cc = ps->columns;
            for (int xx = 0, ii = 0; cc; xx += cc->width, cc = cc->next, ii++) {
                if ((ps->header_dragging > ii && left < xx + cc->width/2) || (ps->header_dragging < ii && right > xx + cc->width/2)) {
                    ddb_listview_column_move (ps, c, ii);
                    ps->header_dragging = ii;
                    gtk_widget_queue_draw (ps->list);
                    break;
                }
            }
            ps->col_movepos = left;
            gtk_widget_queue_draw (ps->header);
        }
    }
    else if (ps->header_sizing >= 0) {
        int x = -ps->hscrollpos;
        DdbListviewColumn *c = ps->columns;
        for (int i = 0; i < ps->header_sizing; i++, c = c->next) {
            x += c->width;
        }
        set_column_width (ps, c, max(MIN_COLUMN_WIDTH, round (event->x) - ps->header_dragpt[0] - x));

        ddb_listview_column_size_changed(ps, c);
        g_idle_add_full(GTK_PRIORITY_RESIZE, ddb_listview_list_setup_hscroll, ps, NULL);
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
    int idx = 0;
    for (DdbListviewColumn *c = pl->columns; c; c = c->next, idx++) {
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
        if (ps->header_dragging != -1) {
            gtk_widget_queue_draw (ps->header);
            ps->header_dragging = -1;
        }
        ps->header_sizing = -1;
        ps->header_prepare = 0;
        int idx = ddb_listview_header_get_column_idx_for_coord (ps, event->x);
        ps->binding->header_context_menu (ps, idx);
    }
    return TRUE;
}

void
ddb_listview_col_sort_update (DdbListview *listview) {
    if (deadbeef->conf_get_int ("gtkui.sticky_sort", 0)) {
        for (DdbListviewColumn *c = listview->columns; c; c = c->next) {
            if (c->sort_order) {
                listview->binding->col_sort(c->sort_order, c->user_data);
            }
        }
    }
    else {
        ddb_listview_clear_sort (listview);
    }
}

static gboolean
ddb_listview_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (event->button == 1) {
        if (ps->header_sizing != -1) {
            ps->binding->columns_changed (ps);
            ddb_listview_list_update_total_width (ps, total_columns_width(ps), ps->list_width);
            ps->header_sizing = -1;
        }
        else if (ps->header_dragging != -1) {
            if (ps->header_prepare) {
                if (event->y >= 0 && event->y <= ps->list_height) {
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
                        c->sort_order = (c->sort_order + 1) % 3;
                        ps->binding->col_sort (c->sort_order, c->user_data);
                        gtk_widget_queue_draw (ps->list);
                        gtk_widget_queue_draw (ps->header);
                    }
                }
            }
            else {
                gtk_widget_queue_draw (ps->header);
            }
            ps->header_dragging = -1;
        }
        ps->header_prepare = 0;
        set_header_cursor(ps, event->x);
    }
    return FALSE;
}

static gboolean
ddb_listview_header_enter (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
    DdbListview *ps = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (!ps->header_prepare && ps->header_dragging == -1 && ps->header_sizing == -1) {
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
    return FALSE;
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

        ddb_playlist_t *playlist = deadbeef->plt_get_curr();
        if (playlist != NULL) {
            ps->binding->list_context_menu (playlist, PL_MAIN);
            deadbeef->plt_unref (playlist);
        }
    }
    return TRUE;
}

static gboolean
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

void
ddb_listview_scroll_to (DdbListview *listview, int pos) {
    pos = ddb_listview_get_row_pos (listview, pos, NULL);
    if (pos < listview->scrollpos || pos + listview->rowheight >= listview->scrollpos + listview->list_height) {
        gtk_range_set_value (GTK_RANGE (listview->scrollbar), pos - listview->list_height/2);
    }
}

/////// column management code

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

static DdbListviewColumn *
ddb_listview_column_alloc (const char *title, int align_right, minheight_cb_t minheight_cb, int is_artwork, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn * c = malloc (sizeof (DdbListviewColumn));
    memset (c, 0, sizeof (DdbListviewColumn));
    c->title = strdup (title);
    c->align_right = align_right;
    c->color_override = color_override;
    c->color = color;
    c->minheight_cb = minheight_cb;
    c->is_artwork = is_artwork;
    c->user_data = user_data;
    return c;
}

void
ddb_listview_column_append (DdbListview *listview, const char *title, int width, int align_right, minheight_cb_t minheight_cb, int is_artwork, int color_override, GdkColor color, void *user_data) {
    ddb_listview_column_insert (listview, -1, title, width, align_right, minheight_cb, is_artwork, color_override, color, user_data);
}

void
ddb_listview_column_insert (DdbListview *listview, int before, const char *title, int width, int align_right, minheight_cb_t minheight_cb, int is_artwork, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn *c = ddb_listview_column_alloc (title, align_right, minheight_cb, is_artwork, color_override, color, user_data);
    set_column_width (listview, c, c->width);
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
    set_column_width (listview, c, width);
    listview->binding->columns_changed (listview);
}

void
ddb_listview_column_free (DdbListview *listview, DdbListviewColumn *c) {
    if (c->title) {
        free (c->title);
    }
    listview->binding->col_free_user_data (c->user_data);
    free (c);
}

static void
remove_column (DdbListview *listview, DdbListviewColumn **c_ptr) {
    DdbListviewColumn *c = *c_ptr;
    assert (c);
    DdbListviewColumn *next = c->next;
    if (c->sort_order) {
        // HACK: do nothing on main playlist, refresh search playlist
        listview->binding->col_sort (0, c->user_data);
    }
    set_column_width (listview, c, 0);
    ddb_listview_column_free (listview, c);
    *c_ptr = next;
    listview->binding->columns_changed (listview);
}

void
ddb_listview_column_remove (DdbListview *listview, int idx) {
    DdbListviewColumn *c = listview->columns;
    if (idx == 0) {
        remove_column (listview, &listview->columns);
        return;
    }
    int i = 1;
    while (c) {
        if (i == idx) {
            remove_column (listview, &c->next);
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
ddb_listview_column_get_info (DdbListview *listview, int col, const char **title, int *width, int *align_right, minheight_cb_t *minheight_cb, int *is_artwork, int *color_override, GdkColor *color, void **user_data) {
    DdbListviewColumn *c;
    int idx = 0;
    for (c = listview->columns; c; c = c->next, idx++) {
        if (idx == col) {
            *title = c->title;
            *width = c->width;
            *align_right = c->align_right;
            if (minheight_cb) *minheight_cb = c->minheight_cb;
            if (is_artwork) *is_artwork = c->is_artwork;
            *color_override = c->color_override;
            *color = c->color;
            *user_data = c->user_data;
            return 0;
        }
    }
    return -1;
}

int
ddb_listview_column_set_info (DdbListview *listview, int col, const char *title, int width, int align_right, minheight_cb_t minheight_cb, int is_artwork, int color_override, GdkColor color, void *user_data) {
    DdbListviewColumn *c;
    int idx = 0;
    for (c = listview->columns; c; c = c->next, idx++) {
        if (idx == col) {
            free (c->title);
            c->title = strdup (title);
            set_column_width (listview, c, width);
            c->align_right = align_right;
            c->minheight_cb = minheight_cb;
            c->is_artwork = is_artwork;
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
    invalidate_album_art_cells (listview, 0, listview->list_width, 0, listview->list_height);
}
/////// end of column management code

/////// grouping /////
static void
ddb_listview_free_group (DdbListview *listview, DdbListviewGroup *group) {
    while (group) {
        if (group->subgroups) {
            ddb_listview_free_group(listview, group->subgroups);
        }
        DdbListviewGroup *next = group->next;
        if (group->head) {
            listview->binding->unref (group->head);
        }
        free (group);
        group = next;
    }
}

static void
ddb_listview_free_all_groups (DdbListview *listview) {
    ddb_listview_free_group(listview, listview->groups);
    listview->groups = NULL;
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

static int
ddb_listview_min_no_artwork_group_height(DdbListviewColumn *columns) {
    int min_height = 0;
    for (DdbListviewColumn *c = columns; c; c = c->next) {
        if (c->minheight_cb && !c->is_artwork) {
            int col_min_height = c->minheight_cb(c->user_data, c->width);
            if (min_height < col_min_height) {
                min_height = col_min_height;
            }
        }
    }
    return min_height;
}

static DdbListviewGroup *
new_group (DdbListview *listview, DdbListviewIter it, int group_label_visible) {
    listview->binding->ref(it);
    DdbListviewGroup *grp = calloc(1, sizeof(DdbListviewGroup));
    grp->head = it;
    grp->group_label_visible = group_label_visible;
    return grp;
}

static int
calc_subgroups_height(DdbListviewGroup *grp) {
    int height = 0;
    while (grp) {
        height += grp->height;
        grp = grp->next;
    }
    return height;
}

static int
calc_group_height(DdbListview *listview, DdbListviewGroup *grp, int min_height, int is_last) {
    // if we have subgroups, add their heights
    if (grp->subgroups) {
        grp->height = max(calc_subgroups_height(grp->subgroups), min_height);
    }
    else {
        grp->height = max(grp->num_items * listview->rowheight, min_height);
    }
    if (grp->group_label_visible) {
        grp->height += listview->grouptitle_height;
    }
    if (!is_last) {
        grp->height += gtkui_groups_spacing;
    }
    return grp->height;
}

static int
build_groups (DdbListview *listview) {
    listview->groups_build_idx = listview->binding->modification_idx();
    ddb_listview_free_all_groups(listview);
    listview->plt = deadbeef->plt_get_curr();

    DdbListviewIter it = listview->binding->head();
    if (!it) {
        return 0;
    }
    if (!listview->group_formats->format || !listview->group_formats->format[0]) {
        listview->grouptitle_height = 0;
    }
    else {
        listview->grouptitle_height = listview->calculated_grouptitle_height;
    }
    int group_depth = 1;
    DdbListviewGroupFormat *fmt = listview->group_formats;
    while (fmt->next) {
        group_depth++;
        fmt = fmt->next;
    }
    listview->groups = new_group(listview, it, 0);
    DdbListviewGroup *grps = listview->groups;
    for (int i = 1; i < group_depth; i++) {
        grps->subgroups = new_group(listview, it, 0);
        grps = grps->subgroups;
    }
    int min_height = ddb_listview_min_group_height(listview->columns);
    int min_no_artwork_height = ddb_listview_min_no_artwork_group_height(listview->columns);
    int full_height = 0;
    // groups
    if (listview->grouptitle_height) {
        DdbListviewGroup *last_group[group_depth];
        char (*group_titles)[1024] = malloc(sizeof(char[1024]) * group_depth);
        DdbListviewGroup *grp = listview->groups;
        // populate all subgroups from the first item
        for (int i = 0; i < group_depth; i++) {
            last_group[i] = grp;
            grp = grp->subgroups;
            listview->binding->get_group(listview, it, group_titles[i], sizeof(*group_titles), i);
            last_group[i]->group_label_visible = group_titles[i][0] != 0;
        }
        while ((it = next_playitem(listview, it))) {
            int make_new_group_offset = -1;
            for (int i = 0; i < group_depth; i++) {
                char next_title[1024];
                listview->binding->get_group(listview, it, next_title, sizeof(next_title), i);
                if (strcmp (group_titles[i], next_title)) {
                    make_new_group_offset = i;
                    break;
                }
                last_group[i]->num_items++;
            }
            if (make_new_group_offset >= 0) {
                // finish remaining groups
                // must be done in reverse order so heights are calculated correctly
                for (int i = group_depth - 1; i >= make_new_group_offset; i--) {
                    char next_title[1024];
                    last_group[i]->num_items++;
                    listview->binding->get_group(listview, it, next_title, sizeof(next_title), i);
                    int height = calc_group_height (listview, last_group[i], i == listview->artwork_subgroup_level ? min_height : min_no_artwork_height, !(it > 0));
                    if (i == 0) {
                        full_height += height;
                    }
                    DdbListviewGroup *new_grp = it ? new_group(listview, it, next_title[0] != 0) : NULL;
                    if (i == make_new_group_offset) {
                        last_group[i]->next = new_grp;
                    }
                    last_group[i] = new_grp;
                    if (last_group[i] && i < group_depth - 1) {
                        last_group[i]->subgroups = last_group[i + 1];
                    }
                    strcpy (group_titles[i], next_title);
                }
            }
        }
        // calculate final group heights
        for (int i = group_depth - 1; i >= 0; i--) {
            last_group[i]->num_items++;
            int height = calc_group_height (listview, last_group[i], i == listview->artwork_subgroup_level ? min_height : min_no_artwork_height, 1);
            if (i == 0) {
                full_height += height;
            }
        }
        free(group_titles);
    }
    // no groups fast path
    else {
        for (DdbListviewGroup *grp = listview->groups; grp; grp = grp->next) {
            do {
                grp->num_items++;
                it = next_playitem(listview, it);
            } while (it && grp->num_items < BLANK_GROUP_SUBDIVISION);
            full_height += calc_group_height (listview, grp, min_height, !(it > 0));
            if (it) {
                grp->next = new_group(listview, it, 0);
            }
        }
    }
    return full_height;
}

static void
ddb_listview_build_groups (DdbListview *listview) {
    deadbeef->pl_lock();
    int height = build_groups(listview);
    if (height != listview->fullheight) {
        listview->fullheight = height;
        g_idle_add_full(GTK_PRIORITY_RESIZE, ddb_listview_list_setup_vscroll, listview, NULL);
    }
    deadbeef->pl_unlock();
}

static int
ddb_listview_resize_subgroup (DdbListview *listview, DdbListviewGroup *grp, int group_depth, int min_height, int min_no_artwork_height) {
    int full_height = 0;
    while (grp) {
        if (grp->subgroups) {
            ddb_listview_resize_subgroup (listview, grp->subgroups, group_depth + 1, min_height, min_no_artwork_height);
        }
        full_height += calc_group_height (listview, grp, group_depth == listview->artwork_subgroup_level ? min_height : min_no_artwork_height, !grp->next);
        grp = grp->next;
    }
    return full_height;
}

static void
ddb_listview_resize_groups (DdbListview *listview) {
    int min_height = ddb_listview_min_group_height(listview->columns);
    int min_no_artwork_height = ddb_listview_min_no_artwork_group_height(listview->columns);
    int full_height = ddb_listview_resize_subgroup (listview, listview->groups, 0, min_height, min_no_artwork_height);

    if (full_height != listview->fullheight) {
        listview->fullheight = full_height;
        adjust_scrollbar (listview->scrollbar, listview->fullheight, listview->list_height);
    }
}

static gboolean
unlock_columns_cb (gpointer p) {
    DDB_LISTVIEW(p)->lock_columns = 0;
    return FALSE;
}

int
ddb_listview_list_setup (DdbListview *listview, int scroll_to) {
    if (!list_is_realized(listview)) {
        return FALSE;
    }
    listview->lock_columns = 1;
    if (listview->scrollpos == -1) {
        listview->scrollpos = 0;
    }
    deadbeef->pl_lock();
    listview->fullheight = build_groups(listview);
    deadbeef->pl_unlock();
    adjust_scrollbar (listview->scrollbar, listview->fullheight, listview->list_height);
    gtk_range_set_value (GTK_RANGE (listview->scrollbar), scroll_to);
    g_idle_add (unlock_columns_cb, listview);
    return TRUE;
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
    if (!listview->binding->list_handle_keypress (listview, event->keyval, event->state, PL_MAIN)) {
        return on_mainwin_key_press_event (widget, event, user_data);
    }
    return TRUE;
}

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
header_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer p)
{
    DdbListview *listview = DDB_LISTVIEW (p);
    DdbListviewColumn *c;
    int col_x = -listview->hscrollpos;
    for (c = listview->columns; c && col_x + c->width < x; col_x += c->width, c = c->next);
    if (c && c->show_tooltip && x < col_x + c->width - (c->sort_order ? 14 : 4)) {
        GtkAllocation a;
        gtk_widget_get_allocation(listview->header, &a);
        set_tooltip (tooltip, c->title, col_x, 0, c->width - 4, a.height);
        return TRUE;
    }
    return FALSE;
}

static gboolean
list_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer p)
{
    DdbListview *listview = DDB_LISTVIEW (p);
    DdbListviewPickContext pick_ctx;
    ddb_listview_list_pickpoint (listview, x, y + listview->scrollpos, &pick_ctx);
    if (pick_ctx.type == PICK_ITEM) {
        int idx = pick_ctx.item_idx;
        DdbListviewIter it = listview->binding->get_for_idx (idx);
        if (it) {
            DdbListviewColumn *c;
            int col_x = -listview->hscrollpos;
            for (c = listview->columns; c && col_x + c->width < x; col_x += c->width, c = c->next);
            if (c) {
                cairo_t *cr = gdk_cairo_create (gtk_widget_get_window(widget));
                draw_begin (&listview->listctx, cr);
                cairo_rectangle (cr, 0, 0, 0, 0);
                cairo_clip (cr);
                GdkColor clr = { 0 };
                int row_y = ddb_listview_get_row_pos (listview, idx, NULL) - listview->scrollpos;
                listview->binding->draw_column_data (listview, cr, it, idx, c->align_right, c->user_data, &clr, col_x, row_y, c->width, listview->rowheight, 0);
                cairo_destroy (cr);
                if (draw_is_ellipsized (&listview->listctx)) {
                    set_tooltip (tooltip, draw_get_text (&listview->listctx), col_x, row_y, c->width, listview->rowheight);
                    deadbeef->pl_item_unref (it);
                    return TRUE;
                }
            }
            deadbeef->pl_item_unref (it);
        }
    }
    return FALSE;
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
