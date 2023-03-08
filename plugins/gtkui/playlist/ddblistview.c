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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <deadbeef/deadbeef.h>
#include "../../artwork/artwork.h"
#include "../actionhandlers.h"
#include "../callbacks.h"
#include "../clipboard.h"
#include "../drawing.h"
#include "../gtkui.h"
#include "../support.h"
#include "ddblistview.h"
#include "ddblistviewheader.h"

// FIXME: these are owned by plcommon, which we don't want to include
// Should be owned by something else, like "shared resources"
extern GtkWidget *theme_treeview;
extern GtkWidget *theme_button;

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define DEFAULT_GROUP_TITLE_HEIGHT 30
#define SCROLL_STEP 20
#define AUTOSCROLL_UPDATE_FREQ 0.01f
#define NUM_ROWS_TO_NOTIFY_SINGLY 10
#define BLANK_GROUP_SUBDIVISION 100

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

//#define REF(it) {if (it) listview->datasource->ref (it);}
#define UNREF(it) {if (it) listview->datasource->unref(it);}

enum {
    INFO_TARGET_URIS, // gtk uses 0 info by default
    INFO_TARGET_PLAYLIST_ITEM_INDEXES,
    INFO_TARGET_PLAYITEM_POINTERS,
};

struct _DdbListviewPrivate {
    int list_width; // width if the list widget as of the last resize
    int list_height; // heught of the list widget as of the last resize
    int totalwidth; // width of listview, including any invisible (scrollable) part
    int fullheight; // total height of all groups
    const char *title; // unique id, used for config writing, etc
    int lastpos[2]; // last mouse position (for list widget)
                    // current state
    int scrollpos;
    int hscrollpos;
    int rowheight;

    int drag_motion_y;

    int ref_point; // idx of anchor when columns are resized
    int ref_point_offset; // y pixel-coordinate of anchor relative to view

    // scrolling
    int scroll_mode; // 0=select, 1=dragndrop
    int scroll_pointer_x;
    int scroll_pointer_y;
    float scroll_direction;
    int scroll_active;
    struct timeval tm_prevscroll;
    float scroll_sleep_time;

    // selection
    int areaselect; // boolean, whether area selection is active (1), or not (0)
    int areaselect_x; // x pixel-coordinate of anchor click relative to playlist origin
    int areaselect_y; // y pixel-coordinate of anchor click relative to playlist origin
    int dragwait; // set to 1 when mouse was pressed down on already selected track, but not moved since (so we're waiting for dnd to begin)
    int drag_source_playlist;
    int shift_sel_anchor;

    // header
    int col_autoresize;
    float fwidth;
    int view_realized;

    struct _DdbListviewColumn *columns;
    gboolean lock_columns;

    ddb_playlist_t *plt; // current playlist (refcounted), must be unreffed with the group
    struct _DdbListviewGroup *groups;
    int artwork_subgroup_level;
    int subgroup_title_padding;
    int groups_build_idx; // must be the same as playlist modification idx
    int grouptitle_height;
    int calculated_grouptitle_height;

    // previous area selection range
    int area_selection_start;
    int area_selection_end;

    // drawing contexts
    drawctx_t listctx;
    drawctx_t grpctx;

    // FIXME: should be owned by the delegate (plcommon)
    DdbListviewGroupFormat *group_formats;

    // FIXME: should be owned by the delegate (plcommon)
    guint tf_redraw_timeout_id;
    int tf_redraw_track_idx;
    DdbListviewIter tf_redraw_track;
};

#define DDB_LISTVIEW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), DDB_LISTVIEW_TYPE, DdbListviewPrivate))
G_DEFINE_TYPE (DdbListview, ddb_listview, GTK_TYPE_TABLE);

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

#pragma mark - fwd decls
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
ddb_listview_update_fonts (DdbListview *listview);

////// list functions ////
static void
ddb_listview_list_render (DdbListview *listview, cairo_t *cr, GdkRectangle *clip);
static void
ddb_listview_list_render_row_background (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h, GdkRectangle *clip);
static void
ddb_listview_list_render_row_foreground (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int even, int idx, int y, int w, int h, int x1, int x2);
static void
ddb_listview_list_render_album_art (DdbListview *listview, cairo_t *cr, DdbListviewGroup *grp, int min_y, int grp_next_y, int y, GdkRectangle *clip);
static void
ddb_listview_list_track_dragdrop (DdbListview *listview, int x, int y);
int
ddb_listview_dragdrop_get_row_from_coord (DdbListview *listview, int x, int y);
void
ddb_listview_list_mousemove (DdbListview *listview, GdkEventMotion *event, int x, int y);
static gboolean
ddb_listview_list_setup_vscroll (void *user_data);
static gboolean
ddb_listview_list_setup_hscroll (void *user_data);
int
ddb_listview_get_row_pos (DdbListview *listview, int pos, int *accumulated_title_height);

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

static void
ddb_listview_list_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);

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
list_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer p);

static void
ddb_listview_class_init(DdbListviewClass *class) {
    GObjectClass *object_class = (GObjectClass *) class;
    object_class->finalize = ddb_listview_destroy;
    g_type_class_add_private(class, sizeof(DdbListviewPrivate));
}

static void
set_column_width (DdbListview *listview, DdbListviewColumn *c, float new_width);

static void
ddb_listview_column_size_changed (DdbListview *listview, DdbListviewColumn *c);

static void
ddb_listview_list_update_total_width (DdbListview *listview, int columns_width, int width);

static int
total_columns_width (DdbListview *listview);

static void
ddb_listview_update_scroll_ref_point (DdbListview *listview);

static void
set_fwidth (DdbListview *listview, float list_width);

static void
autoresize_columns (DdbListview *listview, int list_width, int list_height);

#pragma mark - Header delegate wrappers

static void _header_context_menu (DdbListviewHeader *header, int col) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(header), "owner"));
    listview->delegate->header_context_menu(listview, col);
}
static struct _DdbListviewColumn *_header_get_columns(DdbListviewHeader *header) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(header), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    return priv->columns;
}
static void _header_move_column(DdbListviewHeader *header, DdbListviewColumn *c, int pos) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(header), "owner"));
    ddb_listview_column_move (listview, c, pos);
    gtk_widget_queue_draw (listview->list);
}

static void _header_set_column_width(DdbListviewHeader *header, DdbListviewColumn *c, int width) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(header), "owner"));
    set_column_width(listview, c, width);
    ddb_listview_column_size_changed(listview, c);
    g_idle_add_full(GTK_PRIORITY_RESIZE, ddb_listview_list_setup_hscroll, listview, NULL);
    gtk_widget_queue_draw(listview->list);
}

static void _header_columns_changed(DdbListviewHeader *header) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(header), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    listview->delegate->columns_changed (listview);
    ddb_listview_list_update_total_width (listview, total_columns_width(listview), priv->list_width);
}

static int _header_get_list_height(DdbListviewHeader *header) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(header), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    return priv->list_height;
}

static void _header_col_sort (DdbListviewHeader *header, DdbListviewColumnSortOrder sort_order, void *user_data) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(header), "owner"));
    listview->delegate->col_sort (sort_order, user_data);
    gtk_widget_queue_draw (listview->list);
}

static void _header_update_scroll_ref_point (DdbListviewHeader *header) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(header), "owner"));
    ddb_listview_update_scroll_ref_point (listview);
}

#pragma mark -

static void
ddb_listview_init(DdbListview *listview) {
    GtkWidget *hbox;
    GtkWidget *vbox;

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


    static ddb_listview_header_delegate_t _header_delegate = {
        .context_menu = _header_context_menu,
        .get_columns = _header_get_columns,
        .move_column = _header_move_column,
        .set_column_width = _header_set_column_width,
        .columns_changed = _header_columns_changed,
        .get_list_height = _header_get_list_height,
        .col_sort = _header_col_sort,
        .update_scroll_ref_point = _header_update_scroll_ref_point,
    };

    DdbListviewHeader *header = DDB_LISTVIEW_HEADER(ddb_listview_header_new());
    header->delegate = &_header_delegate;
    listview->header = GTK_WIDGET(header);

    gtk_widget_show (listview->header);
    gtk_box_pack_start (GTK_BOX (vbox), listview->header, FALSE, TRUE, 0);

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
    g_object_set_property (G_OBJECT (listview->list), "has-tooltip", &value);
    g_signal_connect (G_OBJECT (listview->list), "query-tooltip", G_CALLBACK (list_tooltip_handler), listview);

    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    memset (priv, 0, sizeof (DdbListviewPrivate));
    // init instance - create all subwidgets, and insert into table
    drawctx_init (&priv->listctx);
    drawctx_init (&priv->grpctx);

    priv->scrollpos = -1;

    priv->rowheight = -1;

    priv->drag_motion_y = -1;

    priv->ref_point = -1;
    priv->ref_point_offset = -1;

    priv->scroll_mode = 0;
    priv->scroll_pointer_x = -1;
    priv->scroll_pointer_y = -1;
    priv->scroll_direction = 0;
    priv->scroll_active = 0;
    memset (&priv->tm_prevscroll, 0, sizeof (priv->tm_prevscroll));
    priv->scroll_sleep_time = 0;

    priv->areaselect = 0;
    priv->areaselect_x = -1;
    priv->areaselect_y = -1;
    //    listview->areaselect_dx = -1;
    //    listview->areaselect_dy = -1;
    priv->dragwait = 0;
    priv->drag_source_playlist = -1;
    priv->shift_sel_anchor = -1;

    priv->fwidth = -1;

    priv->columns = NULL;
    priv->lock_columns = -1;
    priv->groups = NULL;
    priv->plt = NULL;

    priv->calculated_grouptitle_height = DEFAULT_GROUP_TITLE_HEIGHT;

    priv->area_selection_start = 0;
    priv->area_selection_end = 0;

    priv->tf_redraw_timeout_id = 0;
    priv->tf_redraw_track_idx = -1;

}

GtkWidget *
ddb_listview_new(void) {
    return GTK_WIDGET(g_object_new(ddb_listview_get_type(), NULL));
}

static void
ddb_listview_destroy(GObject *object) {
    DdbListview *listview;

    g_return_if_fail(object != NULL);
    g_return_if_fail(DDB_IS_LISTVIEW(object));

    listview = DDB_LISTVIEW(object);

    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    ddb_listview_free_all_groups (listview);

    while (priv->columns) {
        DdbListviewColumn *next = priv->columns->next;
        ddb_listview_column_free (listview, priv->columns);
        priv->columns = next;
    }

    DdbListviewGroupFormat *fmt = priv->group_formats;
    while (fmt) {
        DdbListviewGroupFormat *next_fmt = fmt->next;
        free (fmt->format);
        free (fmt->bytecode);
        free (fmt);
        fmt = next_fmt;
    }
    ddb_listview_cancel_autoredraw (listview);

    draw_free (&priv->listctx);
    draw_free (&priv->grpctx);
}

void
ddb_listview_refresh (DdbListview *listview, uint32_t flags) {
    if (flags & DDB_REFRESH_CONFIG) {
        ddb_listview_update_fonts (listview);
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
ddb_listview_reconf_scrolling (void *listview) {
    ddb_listview_list_setup_vscroll (listview);
    ddb_listview_list_setup_hscroll (listview);
    return FALSE;
}

static void
_update_fwidth (DdbListview *listview, int prev_width) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (listview), &a);
    if (priv->lock_columns != -1 && priv->view_realized) {
        if (!deadbeef->conf_get_int ("gtkui.autoresize_columns", 0) ||
            ddb_listview_header_is_sizing(DDB_LISTVIEW_HEADER(listview->header))) {
            set_fwidth (listview, a.width);
        }
        else if (a.width != prev_width) {
            ddb_listview_update_scroll_ref_point (listview);
            if (priv->fwidth == -1) {
                set_fwidth (listview, prev_width);
            }
            autoresize_columns (listview, a.width, a.height);
        }
    }
}

static gboolean
_initial_resizing_finished (void *ctx) {
    DdbListview *listview = ctx;
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    priv->view_realized = 1;
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (listview), &a);
    _update_fwidth (listview, a.width);
    gtk_widget_queue_draw (GTK_WIDGET(listview));
    return FALSE;
}

static void
ddb_listview_list_realize                    (GtkWidget       *widget,
        gpointer         user_data) {
    DdbListview *listview = DDB_LISTVIEW(g_object_get_data(G_OBJECT(widget), "owner"));
    if (listview->delegate->drag_n_drop) {
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int size = 0;
    for (DdbListviewColumn *c = priv->columns; c; c = c->next) {
        size += c->width;
    }
    return size;
}

static void
ddb_listview_list_update_total_width (DdbListview *listview, int columns_width, int width) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->totalwidth = max (columns_width, width);
}

static DdbListviewIter
next_playitem (DdbListview *listview, DdbListviewIter it) {
    DdbListviewIter next = listview->datasource->next(it);
    listview->datasource->unref(it);
    return next;
}

void
ddb_listview_groupcheck (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int idx = listview->datasource->modification_idx ();
    if (idx != priv->groups_build_idx) {
        ddb_listview_build_groups (listview);
    }
}

// returns 1 if X coordinate in list belongs to album art column and 0 if not
static int
ddb_listview_is_album_art_column (DdbListview *listview, int x) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int col_x = -priv->hscrollpos;
    for (DdbListviewColumn *c = priv->columns; c && col_x <= x; c = c->next) {
        if (x <= col_x + c->width && listview->datasource->is_album_art_column(c->user_data)) {
            return 1;
        }
        col_x += c->width;
    }
    return 0;
}

// returns Y coordinate of an item by its index
static int
ddb_listview_get_row_pos_subgroup (DdbListview *listview, DdbListviewGroup *grp, int y, int idx, int row_idx, int *accum) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    while (grp) {
        int title_height = 0;
        if (grp->group_label_visible) {
            title_height = priv->grouptitle_height;
        }
        if (idx + grp->num_items > row_idx) {
            int i;
            if (grp->subgroups) {
                i = ddb_listview_get_row_pos_subgroup (listview, grp->subgroups, y + title_height, idx, row_idx, accum);
            }
            else {
                i = y + title_height + (row_idx - idx) * priv->rowheight;
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int accum = 0;
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
    int y = ddb_listview_get_row_pos_subgroup (listview, priv->groups, 0, 0, row_idx, &accum);
    deadbeef->pl_unlock ();
    if (accumulated_title_height) {
        *accumulated_title_height = accum;
    }
    return y;
}

static int
ddb_listview_is_empty_region (DdbListviewPickContext *pick_ctx) {
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    const int orig_y = y;
    const int ry = y - priv->scrollpos;
    const int rowheight = priv->rowheight;
    const int is_album_art_column = ddb_listview_is_album_art_column (listview, x);

    while (grp) {
        const int h = grp->height;
        const int grp_title_height = grp->group_label_visible ? priv->grouptitle_height : 0;
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
            else if (is_album_art_column && group_level == priv->artwork_subgroup_level) {
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
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
    else if (y > priv->fullheight) {
        // area below playlist
        pick_ctx->type = PICK_BELOW_PLAYLIST;
        pick_ctx->item_grp_idx = -1;
        pick_ctx->grp_idx = -1;
        // select last playlist item
        pick_ctx->item_idx = listview->datasource->count () - 1;
        pick_ctx->grp = NULL;
        return;
    }

    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
    int found = ddb_listview_list_pickpoint_subgroup (listview, priv->groups, x, y, 0, 0, 0, 0, pick_ctx);
    deadbeef->pl_unlock ();

    if (!found) {
        // area at the end of playlist or unknown
        pick_ctx->type = PICK_EMPTY_SPACE;
        pick_ctx->item_grp_idx = -1;
        pick_ctx->grp_idx = -1;
        pick_ctx->item_idx = listview->datasource->count () - 1;
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
render_treeview_background (DdbListview *listview, cairo_t *cr, int selected, int even, int x, int y, int w, int h, GdkRectangle *clip) {
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
fill_list_background (DdbListview *listview, cairo_t *cr, int x, int y, int w, int h, GdkRectangle *clip) {
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
ddb_listview_list_render_subgroup (DdbListview *listview, cairo_t *cr, GdkRectangle *clip, DdbListviewGroup *grp, int idx, int grp_y, const int cursor_index, const int current_group_depth, int title_offset, const int subgroup_artwork_offset, const int subgroup_artwork_width, const int pin_offset) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    const int scrollx = -priv->hscrollpos;
    const int row_height = priv->rowheight;
    const int total_width = priv->totalwidth;

    // find 1st group
    while (grp && grp_y + grp->height < clip->y) {
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }

    while (grp && grp_y < clip->y + clip->height) {
        const int title_height = grp->group_label_visible ? priv->grouptitle_height : 0;
        const int is_pinned = gtkui_groups_pinned && grp_y < pin_offset && grp_y + grp->height >= 0;

        // only render list items when at the deepest group level
        if (!grp->subgroups) {
            DdbListviewIter it = grp->head;
            listview->datasource->ref(it);
            for (int i = 0, yy = grp_y + title_height; it && i < grp->num_items && yy < clip->y + clip->height; i++, yy += row_height) {
                if (yy + row_height >= clip->y && (!gtkui_groups_pinned || yy + row_height >= pin_offset)) {
                    ddb_listview_list_render_row_background(listview, cr, it, i & 1, idx+i == cursor_index, scrollx, yy, total_width, row_height, clip);
                    ddb_listview_list_render_row_foreground(listview, cr, it, i & 1, idx+i, yy, total_width, row_height, clip->x, clip->x+clip->width);
                }
                it = next_playitem(listview, it);
            }
            if (it) {
                listview->datasource->unref(it);
            }
        }

        int subgroup_title_offset;
        if (current_group_depth == priv->artwork_subgroup_level) {
            subgroup_title_offset = subgroup_artwork_offset;
        }
        else {
            subgroup_title_offset = title_offset + (grp->group_label_visible ? priv->subgroup_title_padding : 0);
        }

        if (grp->subgroups) {
            // render subgroups before album art and titles
            ddb_listview_list_render_subgroup(listview, cr, clip, grp->subgroups, idx, grp_y + title_height, cursor_index, current_group_depth + 1, subgroup_title_offset, subgroup_artwork_offset, subgroup_artwork_width, pin_offset + title_height);
        }

        int grp_next_y = grp_y + grp->height;
        if (current_group_depth == priv->artwork_subgroup_level) {
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

        int title_width;
        if (current_group_depth > priv->artwork_subgroup_level) {
            title_width = subgroup_artwork_width;
        }
        else {
            title_width = total_width;
        }

        if (is_pinned && clip->y <= title_height + pin_offset) {
            // draw pinned group title
            int y = min(pin_offset, grp_next_y-title_height);
            fill_list_background(listview, cr, scrollx, y, total_width, title_height, clip);
            if (listview->renderer->draw_group_title && title_height > 0) {
                listview->renderer->draw_group_title(listview, cr, grp->head, title_offset, y, title_width, title_height, current_group_depth);
            }
        }
        else if (clip->y <= grp_y + title_height) {
            // draw normal group title
            if (listview->renderer->draw_group_title && title_height > 0) {
                listview->renderer->draw_group_title(listview, cr, grp->head, title_offset, grp_y, title_width, title_height, current_group_depth);
            }
        }

        idx += grp->num_items;
        grp_y += grp->height;
        grp = grp->next;
    }
}

static void
ddb_listview_list_render (DdbListview *listview, cairo_t *cr, GdkRectangle *clip) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    if (priv->scrollpos == -1) {
        return; // too early
    }
    ddb_listview_groupcheck (listview);

    int cursor_index = listview->datasource->cursor();

    // Calculate which side of the playlist the (first) album art cover is on to tell where to draw subgroup titles
    int subgroup_artwork_offset = -priv->hscrollpos + priv->subgroup_title_padding;
    int subgroup_artwork_width = priv->totalwidth;
    int x = 0;
    for (DdbListviewColumn *c = priv->columns; c; x += c->width, c = c->next) {
        if (listview->datasource->is_album_art_column(c->user_data)) {
            int middle = x + c->width / 2;
            if (middle < priv->totalwidth / 2) {
                subgroup_artwork_offset = -priv->hscrollpos + x + c->width;
            }
            else {
                subgroup_artwork_width = -priv->hscrollpos + x;
            }
            break;
        }
    }

    draw_begin (&priv->listctx, cr);
    draw_begin (&priv->grpctx, cr);
    fill_list_background(listview, cr, clip->x, clip->y, clip->width, clip->height, clip);

    ddb_listview_list_render_subgroup(listview, cr, clip, priv->groups, 0, -priv->scrollpos, cursor_index, 0, -priv->hscrollpos, subgroup_artwork_offset, subgroup_artwork_width, 0);

    draw_end (&priv->listctx);
    draw_end (&priv->grpctx);
}

static void
ddb_listview_draw_dnd_marker (DdbListview *listview, cairo_t *cr) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    if (priv->drag_motion_y < 0) {
        return;
    }
    int drag_motion_y = priv->drag_motion_y - priv->scrollpos;

    GdkColor clr;
    gtkui_get_listview_cursor_color (&clr);
    draw_cairo_rectangle(cr, &clr, 0, drag_motion_y-1, priv->list_width, 3);
    draw_cairo_rectangle(cr, &clr, 0, drag_motion_y-3, 3, 7);
    draw_cairo_rectangle(cr, &clr, priv->list_width-3, drag_motion_y-3, 3, 7);

}

static void
ddb_listview_update_fonts (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    draw_init_font (&priv->listctx, DDB_LIST_FONT, 1);
    draw_init_font (&priv->grpctx, DDB_GROUP_FONT, 1);
    int row_height = draw_get_listview_rowheight (&priv->listctx);
    int grptitle_height = draw_get_listview_rowheight (&priv->grpctx);
    if (row_height != priv->rowheight || grptitle_height != priv->calculated_grouptitle_height) {
        priv->rowheight = row_height;
        priv->calculated_grouptitle_height = grptitle_height;
        ddb_listview_build_groups (listview);
    }

    ddb_listview_header_update_fonts (DDB_LISTVIEW_HEADER(listview->header));
}

static void
draw_list_rectangle (DdbListview *listview, cairo_t *cr, GdkRectangle *clip) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    cairo_rectangle(cr, clip->x, clip->y, clip->width, clip->height);
    cairo_clip(cr);
    cairo_set_line_width(cr, 1);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    ddb_listview_list_render(listview, cr, clip);
    if (priv->drag_motion_y >= 0 && priv->drag_motion_y-priv->scrollpos-3 < clip->y+clip->height && priv->drag_motion_y-priv->scrollpos+3 >= clip->y) {
        ddb_listview_draw_dnd_marker(listview, cr);
    }
}

#if GTK_CHECK_VERSION(3,0,0)
static int
list_is_realized (DdbListview *listview) {
    return gtk_widget_get_realized (GTK_WIDGET (listview));
}

static gboolean
ddb_listview_list_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data(G_OBJECT (widget), "owner"));
    if (!list_is_realized (listview)) {
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
        draw_list_rectangle(listview, cr, &clip);
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
ddb_listview_list_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data(G_OBJECT (widget), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    if (!priv->view_realized) {
        return FALSE; // drawing was called too early
    }

    GdkRectangle *rectangles;
    int num_rectangles;
    gdk_region_get_rectangles(event->region, &rectangles, &num_rectangles);
    for (int i = 0; i < num_rectangles; i++) {
        GdkRectangle *clip = &rectangles[i];
        cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
        draw_list_rectangle(listview, cr, clip);
        cairo_destroy(cr);
    }
    g_free(rectangles);
    return TRUE;
}
#endif

static void
scroll_by (GtkWidget *scrollbar, gdouble delta) {
    GtkRange *range = GTK_RANGE(scrollbar);
    gdouble step = pow(gtk_adjustment_get_page_size(gtk_range_get_adjustment(range)), 2./3.);
    gtk_range_set_value(range, max(0, gtk_range_get_value(range) + step * delta));
}

static gboolean
ddb_listview_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    GdkEventScroll *ev = (GdkEventScroll*)event;

    switch(ev->direction) {
        case GDK_SCROLL_UP:
            scroll_by(listview->scrollbar, -1);
            break;
        case GDK_SCROLL_DOWN:
            scroll_by(listview->scrollbar, 1);
            break;
        case GDK_SCROLL_LEFT:
            scroll_by(listview->hscrollbar, -1);
            break;
        case GDK_SCROLL_RIGHT:
            scroll_by(listview->hscrollbar, 1);
            break;
#if GTK_CHECK_VERSION(3,4,0)
        case GDK_SCROLL_SMOOTH:
        {
            gdouble x, y;
            if (gdk_event_get_scroll_deltas(event, &x, &y)) {
                scroll_by(listview->hscrollbar, x);
                scroll_by(listview->scrollbar, y);
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int x = -priv->hscrollpos;
    for (DdbListviewColumn *c = priv->columns; c && x < x2; x += c->width, c = c->next) {
        if (x + c->width > x1 && listview->datasource->is_album_art_column(c->user_data)) {
            gtk_widget_queue_draw_area(listview->list, x, y, c->width, h);
        }
    }
}

static int
find_subgroup_title_heights (DdbListview *listview, DdbListviewGroup *group, int group_y, int at_y) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    while (group->next && group_y + group->height < at_y) {
        group_y += group->height;
        group = group->next;
    }

    int height = group->group_label_visible ? priv->grouptitle_height : 0;
    if (group->subgroups) {
        height += find_subgroup_title_heights(listview, group->subgroups, group_y, at_y);
    }

    return height;
}

static void
invalidate_group (DdbListview *listview, int at_y) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    DdbListviewGroup *group = priv->groups;
    if (!group) {
        return;
    }

    int next_group_y = group->height;
    while (group->next && next_group_y < at_y) {
        group = group->next;
        next_group_y += group->height;
    }

    int group_titles_height = group->group_label_visible ? priv->grouptitle_height : 0;
    if (group->subgroups) {
        group_titles_height += find_subgroup_title_heights(listview, group->subgroups, next_group_y - group->height, at_y);
    }

    int group_height = next_group_y - at_y;
    if (next_group_y > at_y) {
        gtk_widget_queue_draw_area (listview->list, 0, 0, priv->list_width, min(group_titles_height, group_height));
    }
    if (group_height > group_titles_height) {
        invalidate_album_art_cells (listview, 0, priv->list_width, group_titles_height, group_height);
    }
}

static void
ddb_listview_vscroll_value_changed (GtkRange *widget, gpointer user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int newscroll = round(gtk_range_get_value (GTK_RANGE (widget)));
    if (newscroll == priv->scrollpos) {
        return;
    }

    if (listview->delegate->vscroll_changed) {
        listview->delegate->vscroll_changed (newscroll);
    }
    if (gtkui_groups_pinned && priv->grouptitle_height > 0) {
        invalidate_group(listview, max(priv->scrollpos, newscroll));
    }
    GdkWindow *list_window = gtk_widget_get_window(listview->list);
    if (list_window) {
        gdk_window_scroll(list_window, 0, priv->scrollpos - newscroll);
    }
    priv->scrollpos = newscroll;
}

static void
ddb_listview_hscroll_value_changed (GtkRange *widget, gpointer user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int newscroll = round(gtk_range_get_value (GTK_RANGE (widget)));
    if (newscroll == priv->hscrollpos) {
        return;
    }

    int diff = priv->hscrollpos - newscroll;
    GdkWindow *list_window = gtk_widget_get_window(listview->list);
    if (list_window) {
        gdk_window_scroll(gtk_widget_get_window(listview->list), diff, 0);
    }
    priv->hscrollpos = newscroll;
    ddb_listview_header_set_hscrollpos(DDB_LISTVIEW_HEADER(listview->header), newscroll);
}

static gboolean
ddb_listview_list_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_list_track_dragdrop (listview, x, y);

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
                                        gpointer         user_data) {
    return TRUE;
}

static gchar **
ddb_listview_build_drag_uri_list (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    ddb_playlist_t *plt = deadbeef->plt_get_for_idx(priv->drag_source_playlist);
    if (plt == NULL) {
        return NULL;
    }

    deadbeef->pl_lock ();
    int num_selected = deadbeef->plt_get_sel_count (priv->drag_source_playlist);
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

    DdbListviewIter it = deadbeef->plt_get_head_item (plt, PL_MAIN);
    deadbeef->plt_unref(plt);
    int idx = 0;
    while (it) {
        if (listview->datasource->is_selected (it)) {
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
        it = next_playitem (listview, it);
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
                                        gpointer         user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    switch (target_type) {
    case INFO_TARGET_URIS:
        {
            // format as URI_LIST
            gchar **uris = ddb_listview_build_drag_uri_list (listview);
            if (uris) {
                gtk_selection_data_set_uris (selection_data, uris);
                g_strfreev (uris);
                uris = NULL;
            }
        }
        break;
    case INFO_TARGET_PLAYLIST_ITEM_INDEXES:
        {
            ddb_playlist_t *plt = deadbeef->plt_get_for_idx(priv->drag_source_playlist);
            if (plt == NULL) {
                break;
            }
            // format as "STRING" consisting of array of pointers
            int nsel = deadbeef->plt_getselcount (plt);
            if (!nsel) {
                deadbeef->plt_unref(plt);
                break; // something wrong happened
            }
            uint32_t *ptr = malloc ((nsel+1) * sizeof (uint32_t));
            *ptr = priv->drag_source_playlist;
            int idx = 0;
            int i = 1;
            DdbListviewIter it = deadbeef->plt_get_head_item (plt, PL_MAIN);
            deadbeef->plt_unref(plt);
            for (; it; idx++) {
                if (listview->datasource->is_selected (it)) {
                    ptr[i] = idx;
                    i++;
                }
                it = next_playitem(listview, it);
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
                                        gpointer         user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    priv->scroll_direction = 0; // interrupt autoscrolling, if on
    priv->scroll_active = 0;
    priv->drag_motion_y = -1;
    if (!listview->delegate->external_drag_n_drop || !listview->delegate->drag_n_drop) {
        gtk_drag_finish (drag_context, TRUE, FALSE, time);
        return;
    }
    int sel = ddb_listview_dragdrop_get_row_from_coord (listview, x, y);
    DdbListviewIter it = NULL;
    if (sel == -1) {
        if (listview->datasource->count () != 0) {
            sel = listview->datasource->count ();
        }
    }
    if (sel != -1) {
        it = listview->datasource->get_for_idx (sel);
    }

    gchar *ptr=(char*)gtk_selection_data_get_data (selection_data);
    gint len = gtk_selection_data_get_length (selection_data);
    if (info == INFO_TARGET_PLAYITEM_POINTERS) {
        ddb_listview_clear_sort (listview);

        DdbListviewIter *tracks = (DdbListviewIter *)ptr;
        int count = len / sizeof (DdbListviewIter);

        if (listview->delegate->tracks_copy_drag_n_drop != NULL) {
            listview->delegate->tracks_copy_drag_n_drop (it, tracks, count);
        }

        for (int i = 0; i < count; i++) {
            listview->datasource->unref (tracks[i]);
        }
    }
    else if (info == INFO_TARGET_URIS) {
        ddb_listview_clear_sort (listview);
        // this happens when dropped from file manager
        char *mem = malloc (len+1);
        memcpy (mem, ptr, len);
        mem[len] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        listview->delegate->external_drag_n_drop (it, mem, len);
        if (it) {
            UNREF (it);
        }
    }
    // list of 32bit ints, DDB_URI_LIST target
    else if (info == INFO_TARGET_PLAYLIST_ITEM_INDEXES) {
        ddb_listview_clear_sort (listview);
        uint32_t *d= (uint32_t *)ptr;
        int plt = *d;
        d++;
        int length = (len/4)-1;
        DdbListviewIter drop_before = it;
        // find last selected
        if (plt == deadbeef->plt_get_curr_idx ()) {
            while (drop_before && listview->datasource->is_selected (drop_before)) {
                drop_before = next_playitem(listview, drop_before);
            }
        }
        ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
        if (p) {
            listview->delegate->drag_n_drop (drop_before, p, d, length, gdk_drag_context_get_selected_action (drag_context) == GDK_ACTION_COPY ? 1 : 0);
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
                                        gpointer         user_data) {
    return TRUE;
}


static void
ddb_listview_list_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data) {
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
    DdbListview *listview = user_data;
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    ddb_listview_groupcheck (listview);
    adjust_scrollbar (listview->scrollbar, priv->fullheight, priv->list_height);
    return FALSE;
}

static gboolean
ddb_listview_list_setup_hscroll (void *user_data) {
    DdbListview *listview = user_data;
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int size = total_columns_width (listview);
    ddb_listview_list_update_total_width (listview, size, priv->list_width);
    adjust_scrollbar (listview->hscrollbar, size, priv->list_width);
    return FALSE;
}

void
ddb_listview_draw_row (DdbListview *listview, int row, DdbListviewIter it) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int y = ddb_listview_get_row_pos(listview, row, NULL) - priv->scrollpos;
    if (y + priv->rowheight > 0 && y <= priv->list_height) {
        gtk_widget_queue_draw_area (listview->list, 0, y, priv->list_width, priv->rowheight);
    }
}

// coords passed are window-relative
static void
ddb_listview_list_render_row_background (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int even, int cursor, int x, int y, int w, int h, GdkRectangle *clip) {
    // draw background even for selection -- for theme translucency
    int draw_selected = it && listview->datasource->is_selected (it);
    int draw_normal = !gtkui_override_listview_colors() || !draw_selected;
    if (draw_normal && !even) {
        render_treeview_background(listview, cr, FALSE, even, x, y, w, h, clip);
    }
    if (draw_selected) {
        render_treeview_background(listview, cr, TRUE, even, x, y, w, h, clip);
    }

    if (cursor && gtk_widget_has_focus (listview->list)) {
        // not all gtk engines/themes render focus rectangle in treeviews but we want it anyway
        GdkColor clr;
        gtkui_get_listview_cursor_color (&clr);
        cairo_set_source_rgb (cr, clr.red/65535., clr.green/65535., clr.blue/65535.);
        cairo_rectangle (cr, x+1, y+1, w-1, h-1);
        cairo_stroke (cr);
    }
}

static void
ddb_listview_list_render_row_foreground (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int even, int idx, int y, int w, int h, int x1, int x2) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int x = -priv->hscrollpos;
    for (DdbListviewColumn *c = priv->columns; c && x < x2; x += c->width, c = c->next) {
        if (x + c->width > x1 && !listview->datasource->is_album_art_column(c->user_data)) {
            listview->renderer->draw_column_data (listview, cr, it, idx, c->align_right, c->user_data, c->color_override ? &c->color : NULL, x, y, c->width, h, even);
        }
    }
}

static void
ddb_listview_list_render_album_art (DdbListview *listview, cairo_t *cr, DdbListviewGroup *grp, int min_y, int grp_next_y, int y, GdkRectangle *clip) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int x = -priv->hscrollpos;
    for (DdbListviewColumn *c = priv->columns; c && x < clip->x+clip->width; x += c->width, c = c->next) {
        if (listview->datasource->is_album_art_column(c->user_data) && x + c->width > clip->x) {
            fill_list_background(listview, cr, x, y, c->width, grp->height-priv->grouptitle_height, clip);
            if (priv->grouptitle_height > 0) {
                listview->renderer->draw_album_art(listview, cr, grp, c->user_data, min_y, grp_next_y, x, y, c->width, grp->height-priv->grouptitle_height, c->align_right);
            }
        }
    }
}

// Deselect all items in the current list
static void
ddb_listview_deselect_all (DdbListview *listview) {
    int notify_singly = listview->datasource->sel_count() <= NUM_ROWS_TO_NOTIFY_SINGLY;
    DdbListviewIter it;
    int idx = 0;
    for (it = listview->datasource->head (); it; idx++) {
        if (listview->datasource->is_selected (it)) {
            listview->datasource->select (it, 0);
            if (notify_singly) {
                ddb_listview_draw_row (listview, idx, it);
                listview->delegate->selection_changed (listview, it, idx);
            }
        }
        it = next_playitem(listview, it);
    }
    if (!notify_singly) {
        ddb_listview_refresh (listview, DDB_REFRESH_LIST);
        listview->delegate->selection_changed(listview, NULL, -1);
    }
}

// (De)select a whole group
// grp = group to be (de)selected
// item_idx = index of first item in the group or -1
// deselect: 0 = select group, 1 = deselect group
static void
ddb_listview_select_group (DdbListview *listview, DdbListviewGroup *grp, int first_item_idx, int deselect) {
    if (grp == NULL) {
        return;
    }
    int notify_singly = grp->num_items <= NUM_ROWS_TO_NOTIFY_SINGLY;
    DdbListviewIter it = grp->head;
    listview->datasource->ref (it);
    if (first_item_idx == -1) {
        first_item_idx = listview->datasource->get_idx (it);
    }
    for (int group_idx = 0; it && group_idx < grp->num_items; group_idx++) {
        if (deselect) {
            listview->datasource->select (it, 0);
        }
        else {
            listview->datasource->select (it, 1);
        }
        if (notify_singly) {
            ddb_listview_draw_row (listview, first_item_idx + group_idx, it);
            listview->delegate->selection_changed (listview, it, first_item_idx + group_idx);
        }
        it = next_playitem(listview, it);
    }
    if (it) {
        listview->datasource->unref (it);
    }

    if (!notify_singly) {
        ddb_listview_refresh (listview, DDB_REFRESH_LIST);
        listview->delegate->selection_changed(listview, NULL, -1);
    }
}

// Toggle selection of group
// if at least one item of the group is selected, deselect the whole group
// if no item of the group is selected, select all group items
// grp: group to be toggled
// item_idx: index of first item in the group or -1
static void
ddb_listview_toggle_group_selection (DdbListview *listview, DdbListviewGroup *grp, int item_idx) {
    if (grp == NULL) {
        return;
    }
    DdbListviewIter it = grp->head;
    listview->datasource->ref (it);
    if (item_idx == -1) {
        item_idx = listview->datasource->get_idx (it);
    }
    int deselect = 0;
    int group_idx = 0;
    // check if at least one item is selected
    for (group_idx = 0; it && group_idx < grp->num_items; group_idx++) {
        if (listview->datasource->is_selected (it)) {
            deselect = 1;
            break;
        }
        it = next_playitem(listview, it);
    }
    if (it) {
        listview->datasource->unref (it);
    }

    ddb_listview_select_group (listview, grp, item_idx, deselect);
}

void
ddb_listview_select_single (DdbListview *listview, int sel) {
    deadbeef->pl_lock ();
    ddb_listview_deselect_all(listview);
    DdbListviewIter sel_it = listview->datasource->get_for_idx (sel);
    if (sel_it) {
        listview->datasource->select (sel_it, 1);
        ddb_listview_draw_row (listview, sel, sel_it);
        listview->delegate->selection_changed (listview, sel_it, sel);
        listview->datasource->unref(sel_it);
    }
    deadbeef->pl_unlock ();
}

 void
ddb_listview_update_cursor (DdbListview *listview, int cursor) {
    int prev = listview->datasource->cursor ();
    listview->datasource->set_cursor (cursor);
    if (cursor != -1) {
        DdbListviewIter it = listview->datasource->get_for_idx (cursor);
        ddb_listview_draw_row (listview, cursor, it);
        UNREF (it);
    }
    if (prev != -1 && prev != cursor) {
        DdbListviewIter it = listview->datasource->get_for_idx (prev);
        ddb_listview_draw_row (listview, prev, it);
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
    DdbListview *listview = sc->pl;
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    ddb_listview_update_cursor (listview, sc->cursor);
    ddb_listview_select_single (listview, sc->cursor);

    int accumulated_title_height;
    int cursor_scroll = ddb_listview_get_row_pos (listview, sc->cursor, &accumulated_title_height);
    int newscroll = priv->scrollpos;
    if (!gtkui_groups_pinned && cursor_scroll < priv->scrollpos) {
         newscroll = cursor_scroll;
    }
    else if (gtkui_groups_pinned && cursor_scroll < priv->scrollpos + accumulated_title_height) {
        newscroll = cursor_scroll - accumulated_title_height;
    }
    else if (cursor_scroll + priv->rowheight >= priv->scrollpos + priv->list_height) {
        newscroll = cursor_scroll + priv->rowheight - priv->list_height + 1;
        if (newscroll < 0) {
            newscroll = 0;
        }
    }
    if (priv->scrollpos != newscroll) {
        GtkWidget *range = listview->scrollbar;
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
ddb_listview_select_range (DdbListview *listview, int start, int end) {
    int nchanged = 0;
    int idx = 0;
    DdbListviewIter it;
    for (it = listview->datasource->head (); it; idx++) {
        if (idx >= start && idx <= end) {
            if (!listview->datasource->is_selected (it)) {
                nchanged++;
                listview->datasource->select (it, 1);
                ddb_listview_draw_row (listview, idx, it);
                if (nchanged <= NUM_ROWS_TO_NOTIFY_SINGLY) {
                    listview->delegate->selection_changed (listview, it, idx);
                }
            }
        }
        else {
            if (listview->datasource->is_selected (it)) {
                nchanged++;
                listview->datasource->select (it, 0);
                ddb_listview_draw_row (listview, idx, it);
                if (nchanged <= NUM_ROWS_TO_NOTIFY_SINGLY) {
                    listview->delegate->selection_changed (listview, it, idx);
                }
            }
        }
        it = next_playitem(listview, it);
    }
    if (nchanged > NUM_ROWS_TO_NOTIFY_SINGLY) {
        listview->delegate->selection_changed (listview, NULL, -1);
    }
}

static int
ddb_listview_is_group_selected (DdbListview *listview, DdbListviewGroup *grp) {
    if (!listview || !grp) {
        return 0;
    }

    DdbListviewIter it = grp->head;
    listview->datasource->ref(it);
    for (int i = 0; i < grp->num_items && it; ++i) {
        if (!listview->datasource->is_selected (it)) {
            listview->datasource->unref (it);
            return 0;
        }
        it = next_playitem(listview, it);
    }
    if (it) {
        listview->datasource->unref (it);
    }

    return 1;
}

void
ddb_listview_click_selection (DdbListview *listview, int ex, int ey, DdbListviewPickContext *pick_ctx, int dnd, int button) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    deadbeef->pl_lock ();
    priv->areaselect = 0;
    ddb_listview_groupcheck (listview);

    if (dnd) {
        // prepare area selection or drag
        int selected = 0;
        if (pick_ctx->type == PICK_ALBUM_ART || pick_ctx->type == PICK_GROUP_TITLE) {
            selected = ddb_listview_is_group_selected (listview, pick_ctx->grp);
        }
        else {
            DdbListviewIter it = listview->datasource->get_for_idx (pick_ctx->item_idx);
            if (it) {
                selected = listview->datasource->is_selected (it);
                UNREF (it);
            }
        }
        if (!selected || pick_ctx->type == PICK_EMPTY_SPACE) {
            priv->areaselect = 1;
            priv->areaselect_x = ex + priv->hscrollpos;
            priv->areaselect_y = ey + priv->scrollpos;
            priv->shift_sel_anchor = pick_ctx->item_idx;
        }
        else if (selected && pick_ctx->type != PICK_EMPTY_SPACE && listview->delegate->drag_n_drop) {
            priv->dragwait = 1;
        }
    }

    if (pick_ctx->type == PICK_EMPTY_SPACE) {
        // clicked empty space, deselect everything
        ddb_listview_deselect_all (listview);
    }
    else if (pick_ctx->item_idx != -1
            && (pick_ctx->type == PICK_GROUP_TITLE
                || pick_ctx->type == PICK_ALBUM_ART)) {
        // clicked group title or album art column, select group
        int start = pick_ctx->item_grp_idx;
        int end = start + pick_ctx->grp->num_items - 1;
        ddb_listview_select_range (listview, start, end);
    }
    else if (pick_ctx->item_idx != -1 && pick_ctx->type == PICK_ITEM) {
        // clicked specific item - select, or start drag-n-drop
        DdbListviewIter it = listview->datasource->get_for_idx (pick_ctx->item_idx);
        if (it) {
            if (!listview->datasource->is_selected (it)) {
                // reset selection, and set it to single item
                ddb_listview_select_single (listview, pick_ctx->item_idx);
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
//       deadbeef->pl_get_cursor (priv->iterator) = clicked
//       redraw
//       start 'area selection' mode
//   }}}
//   {{{ [+] if clicked selected item:
//       deadbeef->pl_get_cursor (priv->iterator) = clicked
//       redraw
//       wait until next release or motion event, whichever is 1st
//       if release is 1st:
//           unselect all except clicked, redraw
//       else if motion is 1st:
//           enter drag-drop mode
//   }}}
// }}}
void
ddb_listview_list_mouse1_pressed (DdbListview *listview, int state, int ex, int ey, GdkEventType type) {
    // cursor must be set here, but selection must be handled in keyrelease
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    deadbeef->pl_lock ();
    ddb_listview_groupcheck (listview);
    int cnt = listview->datasource->count ();
    if (cnt == 0) {
        deadbeef->pl_unlock ();
        return;
    }
    // remember mouse coords for doubleclick detection
    priv->lastpos[0] = ex;
    priv->lastpos[1] = ey;

    // get item under cursor
    DdbListviewPickContext pick_ctx;
    ddb_listview_list_pickpoint (listview, ex, ey + priv->scrollpos, &pick_ctx);

    int group_clicked = (pick_ctx.type == PICK_ALBUM_ART
                        || pick_ctx.type == PICK_GROUP_TITLE) ? 1 : 0;

    int cursor = listview->datasource->cursor ();
    if (type == GDK_2BUTTON_PRESS
            && abs(priv->lastpos[0] - ex) < 3
            && abs(priv->lastpos[1] - ey) < 3) {
        // doubleclick - play this item
        if (pick_ctx.item_idx != -1
            && !ddb_listview_is_empty_region (&pick_ctx)
            && cursor != -1) {
            int idx = cursor;
            DdbListviewIter it = listview->datasource->get_for_idx (idx);
            if (listview->delegate->handle_doubleclick && it) {
                listview->delegate->handle_doubleclick (listview, it, idx);
            }
            if (it) {
                listview->datasource->unref (it);
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
        ddb_listview_update_cursor (listview, new_cursor);
        priv->shift_sel_anchor = listview->datasource->cursor ();
    }

    // handle multiple selection
#ifndef __APPLE__
    int selmask = GDK_CONTROL_MASK;
#else
    int selmask = GDK_MOD2_MASK;
#endif
    if (!(state & (selmask|GDK_SHIFT_MASK)))
    {
        ddb_listview_click_selection (listview, ex, ey, &pick_ctx, 1, 1);
    }
    else if (state & selmask) {
        // toggle selection
        if (pick_ctx.type != PICK_EMPTY_SPACE
                && pick_ctx.item_idx != -1) {
            if (group_clicked) {
                // toggle group items
                ddb_listview_toggle_group_selection (listview, pick_ctx.grp, pick_ctx.item_grp_idx);
            }
            else if (pick_ctx.type == PICK_ITEM) {
                // toggle single item
                DdbListviewIter it = listview->datasource->get_for_idx (pick_ctx.item_idx);
                if (it) {
                    listview->datasource->select (it, 1 - listview->datasource->is_selected (it));
                    ddb_listview_draw_row (listview, pick_ctx.item_idx, it);
                    listview->delegate->selection_changed (listview, it, pick_ctx.item_idx);
                    UNREF (it);
                }
            }
        }
    }
    else if (state & GDK_SHIFT_MASK) {
        if (group_clicked) {
            // deselect everything
            ddb_listview_deselect_all (listview);
            // select group
            ddb_listview_select_group (listview, pick_ctx.grp, pick_ctx.item_grp_idx, 0);
        }
        else if (pick_ctx.type == PICK_ITEM || pick_ctx.type == PICK_EMPTY_SPACE) {
            // select range
            int cursor = pick_ctx.item_idx;
            if (prev > cursor && pick_ctx.type == PICK_EMPTY_SPACE) {
                cursor++;
            }
            int start = min (prev, cursor);
            int end = max (prev, cursor);

            ddb_listview_select_range (listview, start, end);
            ddb_listview_update_cursor (listview, cursor);
        }
    }
    cursor = listview->datasource->cursor ();
    if (cursor != -1 && pick_ctx.item_idx == -1) {
        DdbListviewIter it = listview->datasource->get_for_idx (cursor);
        ddb_listview_draw_row (listview, cursor, it);
        UNREF (it);
    }
    if (prev != -1 && prev != cursor) {
        DdbListviewIter it = listview->datasource->get_for_idx (prev);
        ddb_listview_draw_row (listview, prev, it);
        UNREF (it);
    }
    deadbeef->pl_unlock ();
}

void
ddb_listview_list_mouse1_released (DdbListview *listview, int state, int ex, int ey, double time) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    gtkui_listview_busy = 0;

#ifndef __APPLE__
    int selmask = GDK_CONTROL_MASK;
#else
    int selmask = GDK_MOD2_MASK;
#endif

    int select_single = 0;

    if (!listview->delegate->drag_n_drop) {
        // playlist doesn't support drag and drop (e.g. search list)
        // select single track
        select_single = 1;
    }

    if (priv->dragwait) {
        // reset dragdrop and select single track
        priv->dragwait = 0;
        select_single = 1;
    }

    if (priv->areaselect) {
        // reset areaselection ctx without clearing selection
        priv->scroll_direction = 0;
        priv->scroll_pointer_x = -1;
        priv->scroll_pointer_y = -1;
        priv->areaselect = 0;
        priv->areaselect_x = -1;
        priv->areaselect_y = -1;
    }
    else if (select_single && !(state & (selmask|GDK_SHIFT_MASK))) {
        // clear selection and select single track
        DdbListviewPickContext pick_ctx;
        ddb_listview_list_pickpoint (listview, ex, ey + priv->scrollpos, &pick_ctx);
        if (pick_ctx.type == PICK_ITEM) {
            ddb_listview_select_single (listview, pick_ctx.item_idx);
        }
    }
}

#if 0
void
ddb_listview_list_dbg_draw_areasel (GtkWidget *widget, int x, int y) {
    // erase previous rect using 4 blits from priv->list->windowfer
    if (areaselect_dx != -1) {
        int sx = min (areaselect_x, areaselect_dx);
        int sy = min (areaselect_y, areaselect_dy);
        int dx = max (areaselect_x, areaselect_dx);
        int dy = max (areaselect_y, areaselect_dy);
        int w = dx - sx + 1;
        int h = dy - sy + 1;
        //draw_drawable (widget->window, widget->style->black_gc, priv->list->window, sx, sy, sx, sy, dx - sx + 1, dy - sy + 1);
        draw_drawable (widget->window, widget->style->black_gc, priv->list->window, sx, sy, sx, sy, w, 1);
        draw_drawable (widget->window, widget->style->black_gc, priv->list->window, sx, sy, sx, sy, 1, h);
        draw_drawable (widget->window, widget->style->black_gc, priv->list->window, sx, sy + h - 1, sx, sy + h - 1, w, 1);
        draw_drawable (widget->window, widget->style->black_gc, priv->list->window, sx + w - 1, sy, sx + w - 1, sy, 1, h);
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
    DdbListview *listview = (DdbListview *)data;
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->scroll_active = 1;
    struct timeval tm;
    gettimeofday (&tm, NULL);
    float dt = tm.tv_sec - priv->tm_prevscroll.tv_sec + (tm.tv_usec - priv->tm_prevscroll.tv_usec) / 1000000.0;
    if (dt < priv->scroll_sleep_time) {
        return TRUE;
    }
    memcpy (&priv->tm_prevscroll, &tm, sizeof (tm));
    if (priv->scroll_pointer_y == -1) {
        priv->scroll_active = 0;
        return FALSE;
    }
    if (priv->scroll_direction == 0) {
        priv->scroll_active = 0;
        return FALSE;
    }
    int sc = priv->scrollpos + (priv->scroll_direction * 100 * dt);
    if (sc < 0) {
        priv->scroll_active = 0;
        return FALSE;
    }
//    trace ("scroll to %d speed %f\n", sc, priv->scroll_direction);
    gtk_range_set_value (GTK_RANGE (listview->scrollbar), sc);
    if (priv->scroll_mode == 0) {
        ddb_listview_list_mousemove (listview, NULL, priv->scroll_pointer_x, priv->scroll_pointer_y);
    }
    else if (priv->scroll_mode == 1) {
        ddb_listview_list_track_dragdrop (listview, priv->scroll_pointer_x, priv->scroll_pointer_y);
    }
    if (priv->scroll_direction < 0) {
        priv->scroll_direction -= (10 * dt);
        if (priv->scroll_direction < -30) {
            priv->scroll_direction = -30;
        }
    }
    else {
        priv->scroll_direction += (10 * dt);
        if (priv->scroll_direction > 30) {
            priv->scroll_direction = 30;
        }
    }
    return TRUE;
}

static void
ddb_listview_select_tracks_within_region (DdbListview *listview, int x, int start_y, int end_y) {
    DdbListviewPickContext pick_ctx_start;
    DdbListviewPickContext pick_ctx_end;

    int start_idx = -1;
    int end_idx = -1;

    // find start point
    ddb_listview_list_pickpoint (listview, x, start_y, &pick_ctx_start);
    start_idx = pick_ctx_start.item_idx;

    // find end point
    ddb_listview_list_pickpoint (listview, x, end_y, &pick_ctx_end);
    end_idx = pick_ctx_end.item_idx;

    if (start_idx == -1 || end_idx == -1) {
        // failed to find start or end track
        return;
    }

    if (start_idx == end_idx) {
        if (pick_ctx_start.type != PICK_EMPTY_SPACE || pick_ctx_end.type != PICK_EMPTY_SPACE) {
            ddb_listview_select_range (listview, start_idx, start_idx);
            ddb_listview_update_cursor (listview, start_idx);
        }
        else {
            ddb_listview_deselect_all (listview);
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

    int total_tracks = listview->datasource->count ();
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

    ddb_listview_select_range (listview, start_idx, end_idx);
    ddb_listview_update_cursor (listview, cursor);
}

void
ddb_listview_list_mousemove (DdbListview *listview, GdkEventMotion *ev, int ex, int ey) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    deadbeef->pl_lock ();
    GtkWidget *widget = listview->list;
    int move_threshold = gtk_drag_check_threshold (widget, priv->lastpos[0], priv->lastpos[1], ex, ey);

    if (move_threshold) {
        if (priv->dragwait) {
            priv->dragwait = 0;
            priv->drag_source_playlist = deadbeef->plt_get_curr_idx ();
            GtkTargetEntry entry = {
                .target = TARGET_PLAYLIST_AND_ITEM_INDEXES,
                .flags = GTK_TARGET_SAME_WIDGET,
                .info = INFO_TARGET_PLAYLIST_ITEM_INDEXES
            };
            GtkTargetList *lst = gtk_target_list_new (&entry, 1);
            gtk_target_list_add_uri_targets (lst, INFO_TARGET_URIS);
            gtk_drag_begin (widget, lst, GDK_ACTION_COPY | GDK_ACTION_MOVE, 1, (GdkEvent *)ev);
        }
        else if (priv->areaselect) {
            ddb_listview_select_tracks_within_region (listview, priv->areaselect_x, priv->areaselect_y, ey + priv->scrollpos);

            if (ey < 10) {
                priv->scroll_mode = 0;
                priv->scroll_pointer_x = ex;
                priv->scroll_pointer_y = ey;
                // start scrolling up
                if (!priv->scroll_active) {
                    priv->scroll_direction = -1;
                    priv->scroll_sleep_time = AUTOSCROLL_UPDATE_FREQ;
                    gettimeofday (&priv->tm_prevscroll, NULL);
                    g_idle_add (ddb_listview_list_scroll_cb, listview);
                }
            }
            else if (ey > priv->list_height-10) {
                priv->scroll_mode = 0;
                priv->scroll_pointer_x = ex;
                priv->scroll_pointer_y = ey;
                // start scrolling down
                if (!priv->scroll_active) {
                    priv->scroll_direction = 1;
                    priv->scroll_sleep_time = AUTOSCROLL_UPDATE_FREQ;
                    gettimeofday (&priv->tm_prevscroll, NULL);
                    g_idle_add (ddb_listview_list_scroll_cb, listview);
                }
            }
            else {
                priv->scroll_direction = 0;
                priv->scroll_pointer_x = -1;
                priv->scroll_pointer_y = -1;
            }
            // debug only
            // ddb_listview_list_dbg_draw_areasel (widget, event->x, event->y);
        }
    }
    deadbeef->pl_unlock ();
}

gboolean
ddb_listview_list_popup_menu (GtkWidget *widget, gpointer user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));

    ddb_playlist_t *playlist = deadbeef->plt_get_curr();
    if (playlist != NULL) {
        listview->delegate->list_context_menu (playlist, PL_MAIN);
        deadbeef->plt_unref (playlist);
    }
    return TRUE;
}

int
ddb_listview_dragdrop_get_row_from_coord (DdbListview *listview, int x, int y) {
    if (y == -1) {
        return -1;
    }
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    DdbListviewPickContext pick_ctx;
    ddb_listview_list_pickpoint (listview, x, y + priv->scrollpos, &pick_ctx);

    int row_idx = -1;
    if (pick_ctx.type == PICK_ITEM || pick_ctx.type == PICK_ALBUM_ART) {
        row_idx = pick_ctx.item_idx;
        int it_y = ddb_listview_get_row_pos (listview, row_idx, NULL) - priv->scrollpos;
        if (y > it_y + priv->rowheight/2) {
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
ddb_listview_list_track_dragdrop (DdbListview *listview, int x, int y) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int prev_drag_y = priv->drag_motion_y;

    if (y == -1) {
        priv->drag_motion_y = -1;
        priv->scroll_active = 0;
        priv->scroll_direction = 0;
    }
    else {
        int sel = ddb_listview_dragdrop_get_row_from_coord (listview, x, y);
        if (sel == -1) {
            if (listview->datasource->count () == 0) {
                priv->drag_motion_y = 0;
            }
            else {
                // after last row
                priv->drag_motion_y = ddb_listview_get_row_pos (listview, listview->datasource->count ()-1, NULL) + priv->rowheight;
            }
        }
        else {
            priv->drag_motion_y = ddb_listview_get_row_pos (listview, sel, NULL);
        }
        if (priv->scrollpos > 0 && priv->drag_motion_y == priv->fullheight) {
            priv->drag_motion_y -= 3;
        }
    }

    if (prev_drag_y != priv->drag_motion_y) {
        if (prev_drag_y != -1) {
            // erase previous track
            gtk_widget_queue_draw_area (listview->list, 0, prev_drag_y-priv->scrollpos-3, priv->list_width, 7);
        }
        if (priv->drag_motion_y != -1) {
            // new track
            gtk_widget_queue_draw_area (listview->list, 0, priv->drag_motion_y-priv->scrollpos-3, priv->list_width, 7);
        }
    }

    if (y < 10) {
        priv->scroll_pointer_x = x;
        priv->scroll_pointer_y = y;
        priv->scroll_mode = 1;
        // start scrolling up
        if (!priv->scroll_active) {
            priv->scroll_direction = -1;
            priv->scroll_sleep_time = AUTOSCROLL_UPDATE_FREQ;
            gettimeofday (&priv->tm_prevscroll, NULL);
            g_idle_add (ddb_listview_list_scroll_cb, listview);
        }
    }
    else if (y > priv->list_height-10) {
        priv->scroll_mode = 1;
        priv->scroll_pointer_x = x;
        priv->scroll_pointer_y = y;
        // start scrolling up
        if (!priv->scroll_active) {
            priv->scroll_direction = 1;
            priv->scroll_sleep_time = AUTOSCROLL_UPDATE_FREQ;
            gettimeofday (&priv->tm_prevscroll, NULL);
            g_idle_add (ddb_listview_list_scroll_cb, listview);
        }
    }
    else {
        priv->scroll_direction = 0;
        priv->scroll_pointer_x = -1;
        priv->scroll_pointer_y = -1;
    }
}

static void
ddb_listview_list_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->scroll_direction = 0;
    priv->scroll_pointer_x = -1;
    priv->scroll_pointer_y = -1;
}

static void
ddb_listview_column_size_changed (DdbListview *listview, DdbListviewColumn *c) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    if (listview->datasource->is_album_art_column(c->user_data)) {
        ddb_listview_resize_groups(listview);
        if (!priv->lock_columns) {
            int pos = ddb_listview_get_row_pos(listview, priv->ref_point, NULL);
            gtk_range_set_value(GTK_RANGE(listview->scrollbar), pos - priv->ref_point_offset);
        }
    }
}

static void
ddb_listview_update_scroll_ref_point_subgroup (DdbListview *listview, DdbListviewGroup *grp, int abs_idx, int grp_y) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    // find 1st group
    while (grp && grp_y + grp->height < priv->scrollpos) {
        grp_y += grp->height;
        abs_idx += grp->num_items;
        grp = grp->next;
    }

    if (!grp) {
        priv->ref_point = 0;
        priv->ref_point_offset = 0;
        return;
    }

    int grp_content_pos = grp_y + (grp->group_label_visible ? priv->grouptitle_height : 0);

    if (grp->subgroups) {
        // search subgroups for anchor
        ddb_listview_update_scroll_ref_point_subgroup (listview, grp->subgroups, abs_idx, grp_content_pos);
    }
    else {
        // choose first visible item as anchor
        int first_item_idx = max(0, (priv->scrollpos - grp_content_pos)/priv->rowheight);
        priv->ref_point = abs_idx + first_item_idx;
        priv->ref_point_offset = grp_content_pos + (first_item_idx * priv->rowheight) - priv->scrollpos;
    }
}

static void
ddb_listview_update_scroll_ref_point (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    ddb_listview_groupcheck (listview);

    if (priv->groups) {
        priv->ref_point = 0;
        priv->ref_point_offset = 0;

        // choose cursor_pos as anchor
        int cursor_pos = ddb_listview_get_row_pos (listview, listview->datasource->cursor (), NULL);
        if (priv->scrollpos < cursor_pos && cursor_pos < priv->scrollpos + priv->list_height && cursor_pos < priv->fullheight) {
            priv->ref_point = listview->datasource->cursor ();
            priv->ref_point_offset = cursor_pos - priv->scrollpos;
        }
        else {
            ddb_listview_update_scroll_ref_point_subgroup (listview, priv->groups, 0, 0);
        }
    }
}

static void
set_column_width (DdbListview *listview, DdbListviewColumn *c, float new_width) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    if (priv->fwidth != -1) {
        priv->fwidth -= (float)c->width / priv->list_width;
        c->fwidth = new_width / priv->list_width;
        priv->fwidth += c->fwidth;
    }
    c->width = new_width;
}

static void
set_fwidth (DdbListview *listview, float list_width) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int total_width = 0;
    for (DdbListviewColumn *c = priv->columns; c; c = c->next) {
        c->fwidth = c->width / list_width;
        total_width += c->width;
    }
    priv->fwidth = total_width / list_width;
}

void
ddb_listview_init_autoresize (DdbListview *listview, int totalwidth) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    if (totalwidth > 0) {
        DdbListviewColumn *c;
        if (!priv->col_autoresize) {
            for (c = priv->columns; c; c = c->next) {
                c->fwidth = (float)c->width / (float)totalwidth;
            }
            priv->col_autoresize = 1;
        }
    }
}

// Calculate the total height of all groups for a given min-height column width
static int
groups_full_height (DdbListview *listview, DdbListviewColumn *c, int new_width) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int min_height = c->minheight_cb (c->user_data, new_width);
    int full_height = 0;
    deadbeef->pl_lock ();
    for (DdbListviewGroup *grp = priv->groups; grp; grp = grp->next) {
        full_height += priv->grouptitle_height + max (grp->num_items * priv->rowheight, min_height);
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    GtkAllocation a;
    gtk_widget_get_allocation (listview->scrollbar, &a);
    int scrollbar_width = a.width > 1 ? a.width : 16;
    if (priv->fullheight > list_height) {
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int total_width;
    int expected_width = roundf (list_width * priv->fwidth);
    float working_width = list_width;
    if (priv->fwidth > 1) {
        do {
            total_width = 0;
            for (DdbListviewColumn *c = priv->columns; c; c = c->next) {
                int new_width = max (DDB_LISTVIEW_MIN_COLUMN_WIDTH, roundf(working_width * c->fwidth));
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
            for (DdbListviewColumn *c = priv->columns; c; c = c->next) {
                int new_width = roundf (working_width * c->fwidth);
                if (new_width < DDB_LISTVIEW_MIN_COLUMN_WIDTH) {
                    working_width -= DDB_LISTVIEW_MIN_COLUMN_WIDTH - new_width;
                    new_width = DDB_LISTVIEW_MIN_COLUMN_WIDTH;
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
    listview->delegate->columns_changed (listview);
    ddb_listview_list_update_total_width (listview, total_width, list_width);
}

static gboolean
ddb_listview_list_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int prev_width = priv->list_width;

    // the values in event->width/height are broken since GTK-3.22.1, so let's use widget's allocation instead, and hope this is reliable.
    GtkAllocation a;
    gtk_widget_get_allocation (GTK_WIDGET (widget), &a);

    if (a.width != prev_width || a.height != priv->list_height) {
        priv->list_width = a.width;
        priv->list_height = a.height;
        g_idle_add_full (GTK_PRIORITY_RESIZE, ddb_listview_reconf_scrolling, listview, NULL);
    }
    if (a.width != prev_width) {
        ddb_listview_list_update_total_width (listview, total_columns_width(listview), a.width);
    }

    _update_fwidth (listview, prev_width);

    return FALSE;
}

void
ddb_listview_size_columns_without_scrollbar (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    if (deadbeef->conf_get_int ("gtkui.autoresize_columns", 0) && gtk_widget_get_visible (listview->scrollbar)) {
        GtkAllocation a;
        gtk_widget_get_allocation (listview->scrollbar, &a);
        autoresize_columns (listview, priv->list_width + a.width, priv->list_height);
    }
}

void
ddb_listview_col_sort_update (DdbListview *listview) {
    if (deadbeef->conf_get_int ("gtkui.sticky_sort", 0)) {
        DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
        for (DdbListviewColumn *c = priv->columns; c; c = c->next) {
            if (c->sort_order != DdbListviewColumnSortOrderNone) {
                listview->delegate->col_sort(c->sort_order, c->user_data);
            }
        }
    }
    else {
        ddb_listview_clear_sort (listview);
    }
}

gboolean
ddb_listview_list_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data) {
    gtk_widget_grab_focus (widget);
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    if (TEST_LEFT_CLICK (event)) {
        ddb_listview_list_mouse1_pressed (listview, event->state, event->x, event->y, event->type);
    }
    else if (TEST_RIGHT_CLICK(event)) {
        // get item under cursor
        DdbListviewPickContext pick_ctx;
        ddb_listview_list_pickpoint (listview, event->x, event->y + priv->scrollpos, &pick_ctx);

        ddb_listview_click_selection (listview, event->x, event->y, &pick_ctx, 0, event->button);

        int cursor = pick_ctx.item_idx;
        int group_clicked = (pick_ctx.type == PICK_ALBUM_ART
                || pick_ctx.type == PICK_GROUP_TITLE) ? 1 : 0;

        if (group_clicked) {
            cursor = pick_ctx.item_grp_idx;
        }
        ddb_listview_update_cursor (listview, cursor);

        ddb_playlist_t *playlist = deadbeef->plt_get_curr();
        if (playlist != NULL) {
            listview->delegate->list_context_menu (playlist, PL_MAIN);
            deadbeef->plt_unref (playlist);
        }
    }
    return TRUE;
}

static gboolean
ddb_listview_list_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (event->button == 1) {
        ddb_listview_list_mouse1_released (listview, event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
ddb_listview_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data) {
    int x = event->x;
    int y = event->y;
#if GTK_CHECK_VERSION(2,12,0)
    gdk_event_request_motions (event);
#endif
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    ddb_listview_list_mousemove (listview, event, x, y);
    return FALSE;
}

void
ddb_listview_scroll_to (DdbListview *listview, int pos) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    pos = ddb_listview_get_row_pos (listview, pos, NULL);
    if (pos < priv->scrollpos || pos + priv->rowheight >= priv->scrollpos + priv->list_height) {
        gtk_range_set_value (GTK_RANGE (listview->scrollbar), pos - priv->list_height/2);
    }
}

/////// column management code

int
ddb_listview_column_get_count (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int cnt = 0;
    DdbListviewColumn *c = priv->columns;
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    DdbListviewColumn *c = ddb_listview_column_alloc (title, align_right, minheight_cb, is_artwork, color_override, color, user_data);
    set_column_width (listview, c, c->width);
    if (priv->columns) {
        DdbListviewColumn * prev = NULL;
        DdbListviewColumn * next = priv->columns;
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
            priv->columns = c;
        }
    }
    else {
        priv->columns = c;
    }
    set_column_width (listview, c, width);
    listview->delegate->columns_changed (listview);
}

void
ddb_listview_column_free (DdbListview *listview, DdbListviewColumn *c) {
    if (c->title) {
        free (c->title);
    }
    listview->delegate->col_free_user_data (c->user_data);
    free (c);
}

static void
remove_column (DdbListview *listview, DdbListviewColumn **c_ptr) {
    DdbListviewColumn *c = *c_ptr;
    assert (c);
    DdbListviewColumn *next = c->next;
    if (c->sort_order != DdbListviewColumnSortOrderNone) {
        // HACK: do nothing on main playlist, refresh search playlist
        listview->delegate->col_sort (0, c->user_data);
    }
    set_column_width (listview, c, 0);
    ddb_listview_column_free (listview, c);
    *c_ptr = next;
    listview->delegate->columns_changed (listview);
}

void
ddb_listview_column_remove (DdbListview *listview, int idx) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    DdbListviewColumn *c = priv->columns;
    if (idx == 0) {
        remove_column (listview, &priv->columns);
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    // remove c from list
    DdbListviewColumn *c = (DdbListviewColumn *)which;
    if (c == priv->columns) {
        priv->columns = c->next;
    }
    else {
        DdbListviewColumn *cc;
        for (cc = priv->columns; cc; cc = cc->next) {
            if (cc->next == c) {
                cc->next = c->next;
                break;
            }
        }
    }
    c->next = NULL;
    // reinsert c at position inspos update header_dragging to new idx
    if (inspos == 0) {
        c->next = priv->columns;
        priv->columns = c;
    }
    else {
        int idx = 0;
        DdbListviewColumn *prev = NULL;
        DdbListviewColumn *cc = NULL;
        for (cc = priv->columns; cc; cc = cc->next, idx++, prev = cc) {
            if (idx+1 == inspos) {
                DdbListviewColumn *next = cc->next;
                cc->next = c;
                c->next = next;
                break;
            }
        }
    }
    listview->delegate->columns_changed (listview);
}

int
ddb_listview_column_get_info (DdbListview *listview, int col, const char **title, int *width, int *align_right, minheight_cb_t *minheight_cb, int *is_artwork, int *color_override, GdkColor *color, void **user_data) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    DdbListviewColumn *c;
    int idx = 0;
    for (c = priv->columns; c; c = c->next, idx++) {
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    DdbListviewColumn *c;
    int idx = 0;
    for (c = priv->columns; c; c = c->next, idx++) {
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
            listview->delegate->columns_changed (listview);
            return 0;
        }
    }
    return -1;
}

void
ddb_listview_invalidate_album_art_columns (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    invalidate_album_art_cells (listview, 0, priv->list_width, 0, priv->list_height);
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
            listview->datasource->unref (group->head);
        }

        free (group);
        group = next;
    }
}

static void
ddb_listview_free_all_groups (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    ddb_listview_free_group(listview, priv->groups);
    priv->groups = NULL;
    if (priv->plt) {
        deadbeef->plt_unref (priv->plt);
        priv->plt = NULL;
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
    listview->datasource->ref(it);
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    // if we have subgroups, add their heights
    if (grp->subgroups) {
        grp->height = max(calc_subgroups_height(grp->subgroups), min_height);
    }
    else {
        grp->height = max(grp->num_items * priv->rowheight, min_height);
    }
    if (grp->group_label_visible) {
        grp->height += priv->grouptitle_height;
    }
    if (!is_last) {
        grp->height += gtkui_groups_spacing;
    }
    return grp->height;
}

static int
build_groups (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->groups_build_idx = listview->datasource->modification_idx();
    ddb_listview_free_all_groups(listview);
    priv->plt = deadbeef->plt_get_curr();

    DdbListviewIter it = listview->datasource->head();
    if (!it) {
        return 0;
    }
    if (!priv->group_formats->format || !priv->group_formats->format[0]) {
        priv->grouptitle_height = 0;
    }
    else {
        priv->grouptitle_height = priv->calculated_grouptitle_height;
    }
    int group_depth = 1;
    DdbListviewGroupFormat *fmt = priv->group_formats;
    while (fmt->next) {
        group_depth++;
        fmt = fmt->next;
    }
    priv->groups = new_group(listview, it, 0);
    DdbListviewGroup *grps = priv->groups;
    for (int i = 1; i < group_depth; i++) {
        grps->subgroups = new_group(listview, it, 0);
        grps = grps->subgroups;
    }
    int min_height = ddb_listview_min_group_height(priv->columns);
    int min_no_artwork_height = ddb_listview_min_no_artwork_group_height(priv->columns);
    int full_height = 0;
    // groups
    if (priv->grouptitle_height) {
        DdbListviewGroup *last_group[group_depth];
        char (*group_titles)[1024] = malloc(sizeof(char[1024]) * group_depth);
        DdbListviewGroup *grp = priv->groups;
        // populate all subgroups from the first item
        for (int i = 0; i < group_depth; i++) {
            last_group[i] = grp;
            grp = grp->subgroups;
            listview->datasource->get_group_text(listview, it, group_titles[i], sizeof(*group_titles), i);
            last_group[i]->group_label_visible = group_titles[i][0] != 0;
        }
        while ((it = next_playitem(listview, it))) {
            int make_new_group_offset = -1;
            for (int i = 0; i < group_depth; i++) {
                char next_title[1024];
                listview->datasource->get_group_text(listview, it, next_title, sizeof(next_title), i);
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
                    listview->datasource->get_group_text(listview, it, next_title, sizeof(next_title), i);
                    int height = calc_group_height (listview, last_group[i], i == priv->artwork_subgroup_level ? min_height : min_no_artwork_height, !(it > 0));
                    if (i == 0) {
                        full_height += height;
                    }
                    DdbListviewGroup *new_grp = NULL;
                    if (it != NULL) {
                        // ensure that the top-level groups always have titles
                        int title_visible = i == 0 || next_title[0] != 0;
                        new_grp = new_group(listview, it, title_visible);
                    }
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
            int height = calc_group_height (listview, last_group[i], i == priv->artwork_subgroup_level ? min_height : min_no_artwork_height, 1);
            if (i == 0) {
                full_height += height;
            }
        }
        free(group_titles);
    }
    // no groups fast path
    else {
        for (DdbListviewGroup *grp = priv->groups; grp; grp = grp->next) {
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    deadbeef->pl_lock();
    int height = build_groups(listview);
    if (height != priv->fullheight) {
        priv->fullheight = height;
        g_idle_add_full(GTK_PRIORITY_RESIZE, ddb_listview_list_setup_vscroll, listview, NULL);
    }
    deadbeef->pl_unlock();
}

static int
ddb_listview_resize_subgroup (DdbListview *listview, DdbListviewGroup *grp, int group_depth, int min_height, int min_no_artwork_height) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int full_height = 0;
    while (grp) {
        if (grp->subgroups) {
            ddb_listview_resize_subgroup (listview, grp->subgroups, group_depth + 1, min_height, min_no_artwork_height);
        }
        full_height += calc_group_height (listview, grp, group_depth == priv->artwork_subgroup_level ? min_height : min_no_artwork_height, !grp->next);
        grp = grp->next;
    }
    return full_height;
}

static void
ddb_listview_resize_groups (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    int min_height = ddb_listview_min_group_height(priv->columns);
    int min_no_artwork_height = ddb_listview_min_no_artwork_group_height(priv->columns);
    int full_height = ddb_listview_resize_subgroup (listview, priv->groups, 0, min_height, min_no_artwork_height);

    if (full_height != priv->fullheight) {
        priv->fullheight = full_height;
        adjust_scrollbar (listview->scrollbar, priv->fullheight, priv->list_height);
    }
}

static gboolean
unlock_columns_cb (gpointer p) {
    DdbListview *listview = DDB_LISTVIEW(p);
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->lock_columns = 0;
    return FALSE;
}

int
ddb_listview_list_setup (DdbListview *listview, int scroll_to) {
    if (!list_is_realized(listview)) {
        return FALSE;
    }
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->lock_columns = 1;
    if (priv->scrollpos == -1) {
        priv->scrollpos = 0;
    }
    deadbeef->pl_lock();
    priv->fullheight = build_groups(listview);
    deadbeef->pl_unlock();
    adjust_scrollbar (listview->scrollbar, priv->fullheight, priv->list_height);
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    DdbListviewColumn *c;
    for (c = priv->columns; c; c = c->next) {
        c->sort_order = DdbListviewColumnSortOrderNone;
    }
    gtk_widget_queue_draw (listview->header);
}

static gboolean
ddb_listview_list_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    DdbListview *listview = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    if (!listview->delegate->list_handle_keypress (listview, event->keyval, event->state, PL_MAIN)) {
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
list_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer p) {
    DdbListview *listview = DDB_LISTVIEW (p);
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    DdbListviewPickContext pick_ctx;
    ddb_listview_list_pickpoint (listview, x, y + priv->scrollpos, &pick_ctx);
    if (pick_ctx.type == PICK_ITEM) {
        int idx = pick_ctx.item_idx;
        DdbListviewIter it = listview->datasource->get_for_idx (idx);
        if (it) {
            DdbListviewColumn *c;
            int col_x = -priv->hscrollpos;
            for (c = priv->columns; c && col_x + c->width < x; col_x += c->width, c = c->next);
            if (c) {
                cairo_t *cr = gdk_cairo_create (gtk_widget_get_window(widget));
                draw_begin (&priv->listctx, cr);
                cairo_rectangle (cr, 0, 0, 0, 0);
                cairo_clip (cr);
                GdkColor clr = { 0 };
                int row_y = ddb_listview_get_row_pos (listview, idx, NULL) - priv->scrollpos;
                listview->renderer->draw_column_data (listview, cr, it, idx, c->align_right, c->user_data, &clr, col_x, row_y, c->width, priv->rowheight, 0);
                cairo_destroy (cr);
                if (draw_is_ellipsized (&priv->listctx)) {
                    set_tooltip (tooltip, draw_get_text (&priv->listctx), col_x, row_y, c->width, priv->rowheight);
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
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    if (priv->tf_redraw_timeout_id) {
        g_source_remove (priv->tf_redraw_timeout_id);
        priv->tf_redraw_timeout_id = 0;
    }
    if (priv->tf_redraw_track) {
        listview->datasource->unref (priv->tf_redraw_track);
        priv->tf_redraw_track = NULL;
    }
}

DdbListviewGroup *
ddb_listview_get_group_by_head (DdbListview *listview, DdbListviewIter head) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    for (DdbListviewGroup *grp = priv->groups; grp != NULL; grp = grp->next) {
        if (grp->head == head) {
            return grp;
        }
    }
    return NULL;
}

int
ddb_listview_get_scroll_pos (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    return priv->scrollpos;
}

void
ddb_listview_redraw_tf (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    ddb_listview_draw_row (listview, priv->tf_redraw_track_idx, priv->tf_redraw_track);
    priv->tf_redraw_track_idx = -1;
    if (priv->tf_redraw_track) {
        listview->datasource->unref (priv->tf_redraw_track);
        priv->tf_redraw_track = NULL;
    }
    priv->tf_redraw_timeout_id = 0;
}

void
ddb_listview_schedule_draw_tf(DdbListview *listview, int row_idx, guint source_id, DdbListviewIter it) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    ddb_listview_cancel_autoredraw (listview);
    priv->tf_redraw_track_idx = row_idx;
    priv->tf_redraw_timeout_id = source_id;
    priv->tf_redraw_track = it;
    listview->datasource->ref (it);
}

int
ddb_listview_get_list_height (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    return priv->list_height;
}

int
ddb_listview_get_shift_sel_anchor (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    return priv->shift_sel_anchor;
}

void
ddb_listview_set_shift_sel_anchor(DdbListview *listview, int anchor) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->shift_sel_anchor = anchor;
}

DdbListviewGroupFormat *
ddb_listview_get_group_formats (DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    return priv->group_formats;
}

void
ddb_listview_set_group_formats (DdbListview *listview, DdbListviewGroupFormat *formats) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);

    DdbListviewGroupFormat *fmt = priv->group_formats;
    while (fmt) {
        DdbListviewGroupFormat *next_fmt = fmt->next;
        free (fmt->format);
        free (fmt->bytecode);
        free (fmt);
        fmt = next_fmt;
    }

    priv->group_formats = formats;
}

drawctx_t * const
ddb_listview_get_listctx(DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    return &priv->listctx;
}

drawctx_t * const
ddb_listview_get_grpctx(DdbListview *listview) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    return &priv->grpctx;
}

void
ddb_listview_set_artwork_subgroup_level(DdbListview *listview, int artwork_subgroup_level) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->artwork_subgroup_level = artwork_subgroup_level;
}

void
ddb_listview_set_subgroup_title_padding(DdbListview *listview, int subgroup_title_padding) {
    DdbListviewPrivate *priv = DDB_LISTVIEW_GET_PRIVATE(listview);
    priv->subgroup_title_padding = subgroup_title_padding;
}

void
ddb_listview_reset_artwork (DdbListview *listview) {
    gtk_widget_queue_draw(GTK_WIDGET(listview));
}
