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

typedef struct _DdbListview DdbListview;
typedef struct _DdbListviewClass DdbListviewClass;

typedef void * DdbListviewIter;
typedef void * DdbPlaylistHandle;

struct _DdbListviewGroup {
    DdbListviewIter head;
    struct _DdbListviewGroup *subgroups;
    int32_t height;
    int32_t num_items;
    int group_label_visible;
    struct _DdbListviewGroup *next;
};

typedef int (*minheight_cb_t) (void *user_data, int width);
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

    int (*get_group) (DdbListview *listview, DdbListviewIter it, char *str, int size, int index);

    void (*drag_n_drop) (DdbListviewIter before, DdbPlaylistHandle playlist_from, uint32_t *indices, int length, int copy);
    void (*external_drag_n_drop) (DdbListviewIter before, char *mem, int length);

    void (*draw_group_title) (DdbListview *listview, cairo_t *drawable, DdbListviewIter iter, int x, int y, int width, int height, int group_depth);
    void (*draw_album_art) (DdbListview *listview, cairo_t *cr, DB_playItem_t *it, void *user_data, int pinned, int next_y, int x, int y, int width, int height);
    void (*draw_column_data) (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int align, void *user_data, GdkColor *fg_clr, int x, int y, int width, int height, int even);

    // cols
    int (*is_album_art_column) (void *user_data);
    void (*columns_changed) (DdbListview *listview);
    void (*col_sort) (int sort_order, void *user_data);
    void (*col_free_user_data) (void *user_data);

    // callbacks
    void (*list_context_menu) (DdbListview *listview, DdbListviewIter iter, int idx, int plt_iter);
    void (*list_empty_region_context_menu) (DdbListview *listview);
    void (*header_context_menu) (DdbListview *listview, int col);
    void (*handle_doubleclick) (DdbListview *listview, DdbListviewIter iter, int idx);
    gboolean (*list_handle_keypress) (DdbListview *ps, int keyval, int state, int iter);
    void (*selection_changed) (DdbListview *listview, DdbListviewIter it, int idx);
    void (*delete_selected) (void);
    void (*groups_changed) (const char *format);
    void (*vscroll_changed) (int pos);
    void (*cursor_changed) (int pos);
    int (*modification_idx) (void);
} DdbListviewBinding;

struct _DdbListviewColumn;
struct _DdbListviewGroup;

struct _DdbListviewGroupFormat {
    // group format string that's supposed to get parsed by tf
    char *format;
    // tf bytecode for group title
    char *bytecode;

    struct _DdbListviewGroupFormat *next;
};

typedef struct _DdbListviewGroupFormat DdbListviewGroupFormat;

struct _DdbListview {
    GtkTable parent;

    // interaction with client
    DdbListviewBinding *binding;

    // cached gtk/gdk object pointers
    GtkWidget *list;
    GtkWidget *header;
    GtkWidget *scrollbar;
    GtkWidget *hscrollbar;

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

    int col_movepos;

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
    int header_dragging;
    int header_sizing;
    int header_dragpt[2];
    gdouble prev_header_x;
    int header_prepare;
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

    GdkCursor *cursor_sz;
    GdkCursor *cursor_drag;

    // drawing contexts
    drawctx_t listctx;
    drawctx_t grpctx;
    drawctx_t hdrctx;

    DdbListviewGroupFormat *group_formats;

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
void
ddb_listview_select_single (DdbListview *listview, int sel);
void
ddb_listview_select_range (DdbListview *ps, int start, int end);
void
ddb_listview_scroll_to (DdbListview *listview, int rowpos);
int
ddb_listview_list_setup (DdbListview *listview, int scroll_to);
void
ddb_listview_size_columns_without_scrollbar (DdbListview *listview);
int
ddb_listview_column_get_count (DdbListview *listview);
void
ddb_listview_column_append (DdbListview *listview, const char *title, int width, int align_right, minheight_cb_t, int is_artwork, int color_override, GdkColor color, void *user_data);
void
ddb_listview_column_insert (DdbListview *listview, int before, const char *title, int width, int align_right, minheight_cb_t, int is_artwork, int color_override, GdkColor color, void *user_data);
void
ddb_listview_column_remove (DdbListview *listview, int idx);
int
ddb_listview_column_get_info (DdbListview *listview, int col, const char **title, int *width, int *align_right, minheight_cb_t *, int *is_artwork, int *color_override, GdkColor *color, void **user_data);
int
ddb_listview_column_set_info (DdbListview *listview, int col, const char *title, int width, int align_right, minheight_cb_t, int is_artwork, int color_override, GdkColor color, void *user_data);

// if 'gtkui.sticky_sort' is 1: sort columns by their current sort order
// otherwise clear sort order
void
ddb_listview_col_sort_update (DdbListview *listview);

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
    DDB_REFRESH_CONFIG  = 32,
};

void
ddb_listview_refresh (DdbListview *listview, uint32_t flags);

void
ddb_listview_invalidate_album_art_columns (DdbListview *listview);

void
ddb_listview_clear_sort (DdbListview *listview);

int
ddb_listview_get_row_pos (DdbListview *listview, int row_idx, int *accumulated_title_height);

void
ddb_listview_groupcheck (DdbListview *listview);

void
ddb_listview_cancel_autoredraw (DdbListview *listview);

void
ddb_listview_update_cursor (DdbListview *ps, int cursor);

void
ddb_listview_set_cursor_and_scroll (DdbListview *listview, int cursor);
G_END_DECLS

#endif // __DDBLISTVIEW_H
