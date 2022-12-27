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

#ifndef __DDBLISTVIEW_H
#define __DDBLISTVIEW_H

#include <gtk/gtk.h>
#include <sys/time.h>
#include <stdint.h>
#include "../drawing.h"
#include "../../../deadbeef.h"

#define DDB_LISTVIEW_MIN_COLUMN_WIDTH 16

// drag and drop targets
#define TARGET_PLAYLIST_AND_ITEM_INDEXES "DDB_PLAYLIST_AND_ITEM_INDEXES"
#define TARGET_URIS "DDB_PLAYLIST_URIS"
#define TARGET_PLAYITEM_POINTERS "DDB_PLAYITEM_POINTERLIST"

G_BEGIN_DECLS

#define DDB_LISTVIEW_TYPE (ddb_listview_get_type ())
#define DDB_LISTVIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), DDB_LISTVIEW_TYPE, DdbListview))
#define DDB_LISTVIEW_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), DDB_LISTVIEW_TYPE, DdbListviewClass))
#define DDB_IS_LISTVIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DDB_LISTVIEW_TYPE))
#define DDB_IS_LISTVIEW_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), DDB_LISTVIEW_TYPE))

typedef struct _DdbListview DdbListview;
typedef struct _DdbListviewPrivate DdbListviewPrivate;
typedef struct _DdbListviewClass DdbListviewClass;

typedef void * DdbListviewIter;
typedef void * DdbPlaylistHandle;

typedef struct _DdbListviewGroup {
    DdbListviewIter head;
    struct _DdbListviewGroup *subgroups;
    int32_t height;
    int32_t num_items;
    int group_label_visible;

    struct _DdbListviewGroup *next;
} DdbListviewGroup;

typedef int (*minheight_cb_t) (void *user_data, int width);

typedef enum {
    DdbListviewColumnSortOrderNone,
    DdbListviewColumnSortOrderAscending,
    DdbListviewColumnSortOrderDescending
} DdbListviewColumnSortOrder;

typedef struct _DdbListviewColumn {
    char *title;
    int width;
    float fwidth; // only in autoresize mode
    minheight_cb_t minheight_cb;
    struct _DdbListviewColumn *next;
    int color_override;
    GdkColor color;
    void *user_data;
    DdbListviewColumnSortOrder sort_order;
    unsigned align_right : 2; // 0=left, 1=right, 2=center
    unsigned show_tooltip : 1;
    unsigned is_artwork : 1;
} DdbListviewColumn;

typedef struct {
    void (*drag_n_drop) (DdbListviewIter before, DdbPlaylistHandle playlist_from, uint32_t *indices, int length, int copy);
    void (*external_drag_n_drop) (DdbListviewIter before, char *mem, int length);
    void (*tracks_copy_drag_n_drop) (DdbListviewIter before, DdbListviewIter *tracks, int count);

    void (*columns_changed) (DdbListview *listview);
    void (*col_sort) (DdbListviewColumnSortOrder sort_order, void *user_data);
    void (*col_free_user_data) (void *user_data);

    void (*list_context_menu) (ddb_playlist_t *playlist, int plt_iter);
    void (*header_context_menu) (DdbListview *listview, int col);
    void (*handle_doubleclick) (DdbListview *listview, DdbListviewIter iter, int idx);
    gboolean (*list_handle_keypress) (DdbListview *ps, int keyval, int state, int iter);
    void (*selection_changed) (DdbListview *listview, DdbListviewIter it, int idx);
    void (*groups_changed) (const char *format);
    void (*vscroll_changed) (int pos);
    void (*cursor_changed) (int pos);
} ddb_listview_delegate_t;

typedef struct {
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

    int (*is_album_art_column) (void *user_data);

    int (*modification_idx) (void);
    int (*get_group_text) (DdbListview *listview, DdbListviewIter it, char *str, int size, int index);
} ddb_listview_datasource_t;

typedef struct {
    void (*draw_group_title) (DdbListview *listview, cairo_t *drawable, DdbListviewIter iter, int x, int y, int width, int height, int group_depth);
    void (*draw_album_art) (DdbListview *listview, cairo_t *cr, DdbListviewGroup *grp, void *user_data, int pinned, int next_y, int x, int y, int width, int height, int alignment);
    void (*draw_column_data) (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int align, void *user_data, GdkColor *fg_clr, int x, int y, int width, int height, int even);
} ddb_listview_renderer_t;

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

    ddb_listview_datasource_t *datasource;
    ddb_listview_delegate_t *delegate;
    ddb_listview_renderer_t *renderer;

    // cached gtk/gdk object pointers
    GtkWidget *list;
    GtkWidget *header;
    GtkWidget *scrollbar;
    GtkWidget *hscrollbar;

    DdbListviewPrivate *priv;
};

struct _DdbListviewClass {
    GtkTableClass parent_class;
};

GType ddb_listview_get_type(void);

GtkWidget * ddb_listview_new(void);

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

DdbListviewGroup *
ddb_listview_get_group_by_head (DdbListview *listview, DdbListviewIter head);

int
ddb_listview_get_scroll_pos (DdbListview *listview);

void
ddb_listview_redraw_tf (DdbListview *listview);

void
ddb_listview_schedule_draw_tf(DdbListview *listview, int row_idx, guint source_id, DdbListviewIter it);

int
ddb_listview_get_list_height (DdbListview *listview);

int
ddb_listview_get_shift_sel_anchor (DdbListview *listview);

void
ddb_listview_set_shift_sel_anchor(DdbListview *listview, int anchor);

DdbListviewGroupFormat *
ddb_listview_get_group_formats (DdbListview *listview);

void
ddb_listview_set_group_formats (DdbListview *listview, DdbListviewGroupFormat *formats);

drawctx_t * const
ddb_listview_get_listctx(DdbListview *listview);

drawctx_t * const
ddb_listview_get_grpctx(DdbListview *listview);

void
ddb_listview_set_artwork_subgroup_level(DdbListview *listview, int artwork_subgroup_level);

void
ddb_listview_set_subgroup_title_padding(DdbListview *listview, int subgroup_title_padding);

void
ddb_listview_reset_artwork (DdbListview *listview);

G_END_DECLS

#endif // __DDBLISTVIEW_H
