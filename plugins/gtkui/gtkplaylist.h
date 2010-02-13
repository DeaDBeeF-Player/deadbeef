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
#ifndef __GTKPLAYLIST_H
#define __GTKPLAYLIST_H

#include <gtk/gtk.h>
#include <stdint.h>
#include <assert.h>
#include "../../deadbeef.h"

// drag and drop targets
enum {
    TARGET_URILIST,
    TARGET_SAMEWIDGET,
};

// color scheme constants
enum {
    COLO_PLAYLIST_CURSOR,
    COLO_PLAYLIST_ODD,
    COLO_PLAYLIST_EVEN,
    COLO_PLAYLIST_SEL_ODD,
    COLO_PLAYLIST_SEL_EVEN,
    COLO_PLAYLIST_TEXT,
    COLO_PLAYLIST_SEL_TEXT,
    COLO_SEEKBAR_BACK,
    COLO_SEEKBAR_FRONT,
    COLO_VOLUMEBAR_BACK,
    COLO_VOLUMEBAR_FRONT,
    COLO_DRAGDROP_MARKER,
    COLO_COUNT
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

    int (*is_selected) (DdbListviewIter);
    void (*select) (DdbListviewIter, int sel);

    // columns
    int (*col_count) (void);
    DdbListviewColIter (*col_first) (void);
    DdbListviewColIter (*col_next) (DdbListviewColIter);
    const char *(*col_get_title) (DdbListviewColIter);
    int (*col_get_width) (DdbListviewColIter);
    int (*col_get_justify) (DdbListviewColIter);
    int (*col_get_sort) (DdbListviewColIter);
    void (*col_sort) (DdbListviewColIter);

    // callbacks
    void (*draw_column_data) (GdkDrawable *drawable, DdbListviewIter iter, int idx, DdbListviewColIter column, int x, int y, int width, int height);
    void (*edit_column) (DdbListviewColIter);
    void (*add_column) (DdbListviewColIter);
    void (*remove_column) (DdbListviewColIter);
    void (*list_context_menu) (DdbListview *listview, DdbListviewIter *iter, int idx);
    void (*handle_doubleclick) (DdbListview *listview, DdbListviewIter *iter, int idx);
} DdbListviewBinding;

// structure of this kind must be set as user data for playlist, header and scrollbar widgets
// pointer to this structure must be passed too all functions that
// implement playlist functionality (like this pointer)
struct _DdbListview {
    GtkTable parent;

    // interaction with client
    DdbListviewBinding *binding;

    // cached gtk/gdk object pointers
    GtkWidget *playlist;
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
    int lastpos[2]; // last mouse position (for playlist widget)
    // current state
    int scrollpos;
    int hscrollpos;
    double clicktime; // for doubleclick detection
    int nvisiblerows;
    int nvisiblefullrows;

//    gtkpl_column_t *columns;
//    gtkpl_column_t *active_column; // required for column editing
};

struct _DdbListviewClass {
  GtkTableClass parent_class;
};

GtkType ddb_listview_get_type(void);
GtkWidget * ddb_listview_new();

void ddb_listview_set_binding (DdbListview *listview, DdbListviewBinding *binding);
void ddb_listview_draw_row (DdbListview *listview, int idx, DdbListviewIter iter); // same as gtkpl_redraw_pl_row
int ddb_listview_get_vscroll_pos (DdbListview *listview);
int ddb_listview_get_hscroll_pos (DdbListview *listview);
DdbListviewIter ddb_listview_get_iter_from_coord (DdbListview *listview, int x, int y);

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
gtkpl_add_dirs (GSList *lst);

void
gtkpl_add_files (GSList *lst);

void
gtkpl_configure (DdbListview *ps);

int
gtkpl_get_idx_of (DdbListview *ps, DdbListviewIter it);

DdbListviewIter 
gtkpl_get_for_idx (DdbListview *ps, int idx);

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

void
gtk_pl_redraw_item_everywhere (DdbListviewIter it);

void
gtkpl_set_cursor (int iter, int cursor);

void
main_refresh (void);

#endif // __GTKPLAYLIST_H
