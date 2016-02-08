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

#ifndef __DDBLISTVIEW_H
#define __DDBLISTVIEW_H

#include <gtk/gtk.h>
#include <sys/time.h>
#include <stdint.h>
#include "drawing.h"
#include "../../deadbeef.h"

// drag and drop targets
#define TARGET_PLAYITEMS "DDB_PLAYITEM_LIST"
enum {
    TARGET_URILIST,
    TARGET_SAMEWIDGET,
};

G_BEGIN_DECLS

#define DDB_TYPE_LISTVIEW (ddb_listview_get_type ())
#define DDB_LISTVIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), DDB_TYPE_LISTVIEW, DdbListview))
#define DDB_LISTVIEW_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), DDB_TYPE_LISTVIEW, DdbListviewClass))
#define DDB_IS_LISTVIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_TYPE_LISTVIEW))
#define DDB_IS_LISTVIEW_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), DDB_TYPE_LISTVIEW))
#define DDB_LISTVIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DDB_TYPE_LISTVIEW, DdbListviewClass))

typedef struct {
    int id; // predefined col type
    char *format;
    char *bytecode;
} col_info_t;

typedef struct _DdbListview DdbListview;
typedef struct _DdbListviewClass DdbListviewClass;

typedef void * DdbListviewIter;
typedef void * DdbPlaylistHandle;

struct _DdbListviewGroup {
    DdbListviewIter head;
    int32_t height;
    int32_t min_height;
    int32_t num_items;
    int pinned;
    struct _DdbListviewGroup *next;
};

typedef struct _DdbListviewGroup DdbListviewGroup;
//typedef void * DdbListviewColIter;

typedef struct {
    // rows
    int (*count) (void);
    int (*sel_count) (void);

    int (*cursor) (void);
    void (*set_cursor) (int cursor);

    DdbListviewIter (*head) (void);
    DdbListviewIter (*tail) (void);
    DdbListviewIter (*next) (DdbListviewIter);
    DdbListviewIter (*prev) (DdbListviewIter);

    DdbListviewIter (*get_for_idx) (int idx);
    int (*get_idx) (DdbListviewIter);

    void (*ref) (DdbListviewIter);
    void (*unref) (DdbListviewIter);

    void (*select) (DdbListviewIter, int sel);
    int (*is_selected) (DdbListviewIter);

    int (*get_group) (DdbListview *listview, DdbListviewIter it, char *str, int size);

    // drag-n-drop
    void (*drag_n_drop) (DdbListviewIter before, DdbPlaylistHandle playlist_from, uint32_t *indices, int length, int copy);
    void (*external_drag_n_drop) (DdbListviewIter before, char *mem, int length);

    // callbacks
    void (*draw_group_title) (DdbListview *listview, cairo_t *drawable, DdbListviewIter iter, int pl_iter, int x, int y, int width, int height);
    void (*draw_album_art) (DdbListview *listview, cairo_t *drawable, DdbListviewIter group_iter, int column, int group_pinned, int grp_next_y, int x, int y, int width, int height);
    void (*draw_column_data) (DdbListview *listview, cairo_t *drawable, DdbListviewIter iter, int idx, int column, int pl_iter, int x, int y, int width, int height);
    void (*list_context_menu) (DdbListview *listview, DdbListviewIter iter, int idx);
    void (*header_context_menu) (DdbListview *listview, int col);
    void (*handle_doubleclick) (DdbListview *listview, DdbListviewIter iter, int idx);
    void (*selection_changed) (DdbListview *listview, DdbListviewIter it, int idx);
    void (*delete_selected) (void);
    void (*groups_changed) (DdbListview *listview, const char *format);
    void (*columns_changed) (DdbListview *listview);
    void (*col_sort) (int col, int sort_order, void *user_data);
    void (*col_free_user_data) (void *user_data);
    void (*vscroll_changed) (int pos);
    void (*cursor_changed) (int pos);
    int (*modification_idx) (void);
} DdbListviewBinding;

struct _DdbListviewColumn;
struct _DdbListviewGroup;

struct _DdbListview {
    GtkTable parent;

    // interaction with client
    DdbListviewBinding *binding;

    // cached gtk/gdk object pointers
    GtkWidget *list;
    GtkWidget *header;
    GtkWidget *scrollbar;
    GtkWidget *hscrollbar;

    int totalwidth; // width of listview, including invisible (scrollable) part
    const char *title; // unique id, used for config writing, etc
    int lastpos[2]; // last mouse position (for list widget)
    // current state
    int scrollpos;
    int hscrollpos;
    int rowheight;

    int col_movepos;

    int drag_motion_y;

    int ref_point; // idx of anchor when columns are resized
    int ref_point_offset; // y pixel-coordinate of anchor relative to view

    // scrolling
    int scroll_mode; // 0=select, 1=dragndrop
    int scroll_pointer_y;
    float scroll_direction;
    int scroll_active;
    struct timeval tm_prevscroll;
    float scroll_sleep_time;

    // selection
    int areaselect; // boolean, whether area selection is active (1), or not (0)
    int areaselect_y; // pixel-coordinate of anchor click relative to playlist origin
    int dragwait; // set to 1 when mouse was pressed down on already selected track, but not moved since (so we're waiting for dnd to begin)
    int drag_source_playlist;
    int shift_sel_anchor;

    // header
    int header_dragging;
    int header_sizing;
    int header_dragpt[2];
    float last_header_motion_ev; //is it subject to remove?
    int prev_header_x;
    int header_prepare;
    int header_width; // previous width before resize
    int col_autoresize;

    struct _DdbListviewColumn *columns;
    gboolean lock_columns;

    ddb_playlist_t *plt; // current playlist (refcounted), must be unreffed with the group
    struct _DdbListviewGroup *groups;
    int groups_build_idx; // must be the same as playlist modification idx
    int fullheight;
    int block_redraw_on_scroll;
    int grouptitle_height;
    int calculated_grouptitle_height;

    // previous area selection range
    int area_selection_start;
    int area_selection_end;

    GdkCursor *cursor_sz;
    GdkCursor *cursor_drag;

    // drawing contexts
    drawctx_t listctx;
    drawctx_t grpctx;
    drawctx_t hdrctx;

    // cover art size
    int cover_size;
    int new_cover_size;
    guint cover_refresh_timeout_id;

    // group format string that's supposed to get parsed by tf
    char *group_format;
    // tf bytecode for group title
    char *group_title_bytecode;

    guint tf_redraw_timeout_id;
    int tf_redraw_track_idx;
    DdbListviewIter tf_redraw_track;
};

struct _DdbListviewClass {
  GtkTableClass parent_class;
};

GType ddb_listview_get_type(void);

GtkWidget * ddb_listview_new();

void
ddb_listview_set_binding (DdbListview *listview, DdbListviewBinding *binding);
void
ddb_listview_draw_row (DdbListview *listview, int idx, DdbListviewIter iter);
int
ddb_listview_get_vscroll_pos (DdbListview *listview);
int
ddb_listview_get_hscroll_pos (DdbListview *listview);
DdbListviewIter
ddb_listview_get_iter_from_coord (DdbListview *listview, int x, int y);
int
ddb_listview_handle_keypress (DdbListview *ps, int keyval, int state);
void
ddb_listview_set_cursor (DdbListview *pl, int cursor);
void
ddb_listview_set_cursor_noscroll (DdbListview *pl, int cursor);
void
ddb_listview_scroll_to (DdbListview *listview, int rowpos);
void
ddb_listview_set_vscroll (DdbListview *listview, int scroll);
int
ddb_listview_is_scrolling (DdbListview *listview);
int
ddb_listview_column_get_count (DdbListview *listview);
void
ddb_listview_column_append (DdbListview *listview, const char *title, int width, int align_right, int minheight, int color_override, GdkColor color, void *user_data);
void
ddb_listview_column_insert (DdbListview *listview, int before, const char *title, int width, int align_right, int minheight, int color_override, GdkColor color, void *user_data);
void
ddb_listview_column_remove (DdbListview *listview, int idx);
int
ddb_listview_column_get_info (DdbListview *listview, int col, const char **title, int *width, int *align_right, int *minheight, int *color_override, GdkColor *color, void **user_data);
int
ddb_listview_column_set_info (DdbListview *listview, int col, const char *title, int width, int align_right, int minheight, int color_override, GdkColor color, void *user_data);
void
ddb_listview_show_header (DdbListview *listview, int show);
void
ddb_listview_init_autoresize (DdbListview *ps, int totalwidth);

enum {
    DDB_REFRESH_COLUMNS = 1,
    DDB_REFRESH_HSCROLL = 2,
    DDB_REFRESH_VSCROLL = 4,
    DDB_REFRESH_LIST    = 8,
    DDB_LIST_CHANGED    = 16,
};

void ddb_listview_refresh (DdbListview *listview, uint32_t flags);

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

void
ddb_listview_list_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data);

void
ddb_listview_list_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data);

void
ddb_listview_list_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

void
ddb_listview_clear_sort (DdbListview *listview);

void
ddb_listview_lock_columns (DdbListview *lv, gboolean lock);

int
ddb_listview_get_row_pos (DdbListview *listview, int row_idx);

void
ddb_listview_groupcheck (DdbListview *listview);

int
ddb_listview_is_album_art_column (DdbListview *listview, int x);

int
ddb_listview_is_album_art_column_idx (DdbListview *listview, int cidx);

void
ddb_listview_update_fonts (DdbListview *ps);

void
ddb_listview_header_update_fonts (DdbListview *ps);

void
ddb_listview_cancel_autoredraw (DdbListview *listview);

G_END_DECLS

#endif // __DDBLISTVIEW_H
