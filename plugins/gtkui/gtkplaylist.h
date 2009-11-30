/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

//#define pl_ncolumns 5
//#define pl_colname_max 100

typedef struct gtkpl_column_s {
    char *title;
    int id; // id is faster than format, set to -1 to use format
    char *format;
    int width;
    int movepos; // valid only while `moving' is 1
    struct gtkpl_column_s *next;
    unsigned align_right : 1;
//    unsigned moving : 1;
} gtkpl_column_t;

// structure of this kind must be set as user data for playlist, header and scrollbar widgets
// pointer to this structure must be passed too all functions that
// implement playlist functionality (like this pointer)
typedef struct {
    // cached gtk/gdk object pointers
    GtkWidget *playlist;
    GtkWidget *header;
    GtkWidget *scrollbar;
    GtkWidget *hscrollbar;
    GdkPixmap *backbuf;
    GdkPixmap *backbuf_header;
    const char *title; // unique id, used for config writing, etc
    // parameters
    int (*get_count)(void); // function pointer to get number of tracks
    int iterator; // index into next array of DB_playItem_t struct
    int lastpos[2]; // last mouse position (for playlist widget)
    int multisel; // if it uses multiple selection
    // current state
    int scrollpos;
    int hscrollpos;
    double clicktime; // for doubleclick detection
    int nvisiblerows;
    int nvisiblefullrows;
    gtkpl_column_t *columns;
} gtkplaylist_t;

extern gtkplaylist_t main_playlist;
extern gtkplaylist_t search_playlist;

#define GTKPL_PROLOGUE \
    gtkplaylist_t *ps = (gtkplaylist_t *)gtk_object_get_data (GTK_OBJECT (widget), "ps"); assert (ps); 

extern int rowheight;

// that must be called before gtk_init
void
gtkpl_init (void);

void
gtkpl_free (gtkplaylist_t *pl);

void
gtkpl_redraw_pl_row (gtkplaylist_t *ps, int row, DB_playItem_t *it);

void
gtkpl_redraw_pl_row_novis (gtkplaylist_t *ps, int row, DB_playItem_t *it);

void
gtkpl_setup_scrollbar (gtkplaylist_t *ps);

void
gtkpl_draw_pl_row_back (gtkplaylist_t *ps, int row, DB_playItem_t *it);

void
gtkpl_draw_pl_row (gtkplaylist_t *ps, int row, DB_playItem_t *it);

void
gtkpl_draw_playlist (gtkplaylist_t *ps, int x, int y, int w, int h);

void
gtkpl_reconf (gtkplaylist_t *ps);

void
gtkpl_expose (gtkplaylist_t *ps, int x, int y, int w, int h);

void
gtkpl_mouse1_pressed (gtkplaylist_t *ps, int state, int ex, int ey, double time);

void
gtkpl_mouse1_released (gtkplaylist_t *ps, int state, int ex, int ey, double time);

void
gtkpl_mousemove (gtkplaylist_t *ps, GdkEventMotion *event);

void
gtkpl_scroll (gtkplaylist_t *ps, int newscroll);

void
gtkpl_hscroll (gtkplaylist_t *ps, int newscroll);

void
gtkpl_handle_scroll_event (gtkplaylist_t *ps, int direction);

void
gtkpl_keypress (gtkplaylist_t *ps, int keyval, int state);

void
gtkpl_track_dragdrop (gtkplaylist_t *ps, int y);

void
gtkpl_handle_drag_drop (gtkplaylist_t *ps, int drop_y, uint32_t *d, int length);

void
gtkpl_handle_fm_drag_drop (gtkplaylist_t *ps, int drop_y, void *ptr, int length);

void
gtkpl_add_fm_dropped_files (gtkplaylist_t *ps, char *ptr, int length, int drop_y);

void
gtkpl_select_single (gtkplaylist_t *ps, int sel);

void
gtkpl_header_draw (gtkplaylist_t *ps);

void
gtkpl_add_dir (gtkplaylist_t *ps, char *folder);

void
gtkpl_add_dirs (gtkplaylist_t *ps, GSList *lst);

void
gtkpl_add_files (gtkplaylist_t *ps, GSList *lst);

void
gtkpl_configure (gtkplaylist_t *ps);

int
gtkpl_get_idx_of (gtkplaylist_t *ps, DB_playItem_t *it);

DB_playItem_t *
gtkpl_get_for_idx (gtkplaylist_t *ps, int idx);

//// this functions take value from passed playlist, that's why it's here
//void
//gtkpl_playsong (gtkplaylist_t *ps);

void
gtkpl_songchanged (gtkplaylist_t *ps, int from, int to);

// these functions operate on global playlist level,
// no need to pass gtkplaylist_t ptr to them

void
gtkpl_add_fm_dropped_files (gtkplaylist_t *ps, char *ptr, int length, int drop_y);

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

// column utilities
gtkpl_column_t *
gtkpl_column_alloc (const char *title, int width, int id, const char *format, int align_right);

void
gtkpl_column_append (gtkplaylist_t *pl, gtkpl_column_t *c);

void
gtkpl_column_remove (gtkplaylist_t *pl, gtkpl_column_t *c);

void
gtkpl_column_free (gtkpl_column_t *c);

void
gtkpl_append_column_from_textdef (gtkplaylist_t *pl, const uint8_t *def);

void
gtkpl_column_update_config (gtkplaylist_t *pl, gtkpl_column_t *c, int idx);

void
gtkpl_column_rewrite_config (gtkplaylist_t *pl);

void
gtkpl_expose_header (gtkplaylist_t *ps, int x, int y, int w, int h);

void
set_tray_tooltip (const char *text);

void
gtkpl_songchanged_wrapper (int from, int to);

void
gtkpl_current_track_changed (DB_playItem_t *it);

#endif // __GTKPLAYLIST_H
