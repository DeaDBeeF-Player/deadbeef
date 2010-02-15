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
#ifndef __DDBLISTVIEW_H
#define __DDBLISTVIEW_H

#include <gtk/gtk.h>
#include <stdint.h>
#include <assert.h>
#include "../../deadbeef.h"

// drag and drop targets
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
typedef void * DdbListviewColIter;

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

    // drag-n-drop
    void (*drag_n_drop) (DdbListviewIter before, uint32_t *indices, int length);

    // columns
    int (*col_count) (void);
    DdbListviewColIter (*col_first) (void);
    DdbListviewColIter (*col_next) (DdbListviewColIter);
    const char *(*col_get_title) (DdbListviewColIter);
    int (*col_get_width) (DdbListviewColIter);
    int (*col_get_justify) (DdbListviewColIter);
    int (*col_get_sort) (DdbListviewColIter);
    void (*col_move) (DdbListviewColIter which, int inspos);
    void (*col_sort) (DdbListviewColIter);

    void (*col_set_width) (DdbListviewColIter c, int width);
    void (*col_set_sort) (DdbListviewColIter c, int sort);

    // callbacks
    void (*draw_column_data) (GdkDrawable *drawable, DdbListviewIter iter, int idx, DdbListviewColIter column, int x, int y, int width, int height);
    void (*list_context_menu) (DdbListview *listview, DdbListviewIter iter, int idx);
    void (*header_context_menu) (DdbListview *listview, DdbListviewColIter c);
    void (*handle_doubleclick) (DdbListview *listview, DdbListviewIter iter, int idx);
    void (*selection_changed) (DdbListviewIter it, int idx);
    void (*delete_selected) (void);
} DdbListviewBinding;

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
    GdkPixmap *backbuf;
    GdkPixmap *backbuf_header;
    const char *title; // unique id, used for config writing, etc
//    // parameters
//    int (*get_count)(void); // function pointer to get number of tracks
//    int iterator; // index into next array of DB_playItem_t struct
    int lastpos[2]; // last mouse position (for list widget)
    // current state
    int scrollpos;
    int hscrollpos;
    double clicktime; // for doubleclick detection
    int nvisiblerows;
    int nvisiblefullrows;
    int rowheight;

    int col_movepos;

    int drag_motion_y;

    // scrolling
    int scroll_mode; // 0=select, 1=dragndrop
    int scroll_pointer_y;
    int scroll_direction;
    int scroll_active;
    struct timeval tm_prevscroll;
    float scroll_sleep_time;

    // selection
    int areaselect;
    int areaselect_x;
    int areaselect_y;
    int areaselect_dx;
    int areaselect_dy;
    int dragwait;
    int shift_sel_anchor;

    // header
    int header_dragging;
    int header_sizing;
    int header_dragpt[2];
    float last_header_motion_ev; //is it subject to remove?
    int prev_header_x;
    int header_prepare;

//    gtkpl_column_t *columns;
//    gtkpl_column_t *active_column; // required for column editing
};

struct _DdbListviewClass {
  GtkTableClass parent_class;
};

GtkType ddb_listview_get_type(void);
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
ddb_listview_scroll_to (DdbListview *listview, int pos);
int
ddb_listview_is_scrolling (DdbListview *listview);

enum {
    DDB_REFRESH_COLUMNS = 1,
    DDB_REFRESH_HSCROLL = 2,
    DDB_REFRESH_VSCROLL = 4,
    DDB_REFRESH_LIST    = 8,
    DDB_EXPOSE_COLUMNS  = 16,
    DDB_EXPOSE_LIST     = 32,
};

void ddb_listview_refresh (DdbListview *listview, uint32_t flags);

G_END_DECLS

#if 0
//extern DdbListview main_playlist;
//extern DdbListview search_playlist;

//#define GTKPL_PROLOGUE \
//    DdbListview *ps = (DdbListview *)gtk_object_get_data (GTK_OBJECT (widget), "ps"); assert (ps); 


extern int rowheight;

// that must be called before gtk_init
void
gtkpl_init (void);

void
gtkpl_free (DdbListview *pl);

void
gtkpl_redraw_pl_row (DdbListview *ps, int row, DdbListviewIter it);

void
gtkpl_redraw_pl_row_novis (DdbListview *ps, int row, DdbListviewIter it);

void
gtkpl_setup_scrollbar (DdbListview *ps);

void
gtkpl_setup_hscrollbar (DdbListview *ps);

void
gtkpl_draw_pl_row_back (DdbListview *ps, int row, DdbListviewIter it);

void
gtkpl_draw_pl_row (DdbListview *ps, int row, DdbListviewIter it);

void
gtkpl_draw_playlist (DdbListview *ps, int x, int y, int w, int h);

void
gtkpl_reconf (DdbListview *ps);

void
gtkpl_expose (DdbListview *ps, int x, int y, int w, int h);

void
gtkpl_mouse1_pressed (DdbListview *ps, int state, int ex, int ey, double time);

void
gtkpl_mouse1_released (DdbListview *ps, int state, int ex, int ey, double time);

void
gtkpl_mousemove (DdbListview *ps, GdkEventMotion *event);

void
gtkpl_scroll (DdbListview *ps, int newscroll);

void
gtkpl_hscroll (DdbListview *ps, int newscroll);

void
gtkpl_handle_scroll_event (DdbListview *ps, int direction);

// returns 1 if keypress was handled, 0 otherwise
int
gtkpl_keypress (DdbListview *ps, int keyval, int state);

void
gtkpl_track_dragdrop (DdbListview *ps, int y);

void
gtkpl_select_single (DdbListview *ps, int sel);

void
gtkpl_header_draw (DdbListview *ps);

void
gtkpl_add_dir (DdbListview *ps, char *folder);

void
gtkpl_add_files (GSList *lst);

//int
//gtkpl_get_idx_of (DdbListview *ps, DdbListviewIter it);
//
//DdbListviewIter 
//gtkpl_get_for_idx (DdbListview *ps, int idx);

//// this functions take value from passed playlist, that's why it's here
//void
//gtkpl_playsong (DdbListview *ps);

void
gtkpl_songchanged (DdbListview *ps, int from, int to);

// these functions operate on global playlist level,
// no need to pass DdbListview ptr to them

void
gtkpl_add_fm_dropped_files (char *ptr, int length, int drop_y);

// these functions should not belong here
void
gtkpl_prevsong (void);

void
gtkpl_nextsong (void);

void
gtkpl_randomsong (void);

void
gtkpl_pausesong (void);

void
gtkpl_playsongnum (int idx);

void
theme_set_fg_color (int col);

void
theme_set_bg_color (int col);

void
theme_set_cairo_source_rgb (cairo_t *cr, int col);

void
playlist_refresh (void);

void
gtkpl_expose_header (DdbListview *ps, int x, int y, int w, int h);

void
set_tray_tooltip (const char *text);

void
gtkpl_songchanged_wrapper (int from, int to);

void
gtkpl_current_track_changed (DdbListviewIter it);

//void
//gtk_pl_redraw_item_everywhere (DdbListviewIter it);

void
gtkpl_set_cursor (DdbListview *pl, int cursor);

void
main_refresh (void);
#endif

#endif // __DDBLISTVIEW_H
