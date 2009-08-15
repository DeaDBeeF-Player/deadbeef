/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
#include "playlist.h"

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

#define pl_ncolumns 5
#define pl_colname_max 100

// structure of this kind must be set as user data for playlist, header and scrollbar widgets
// pointer to this structure must be passed too all functions that
// implement playlist functionality (like this pointer)
typedef struct {
    // cached gtk/gdk object pointers
    GtkWidget *playlist;
    GtkWidget *header;
    GtkWidget *scrollbar;
    GdkPixmap *backbuf;
    GdkPixmap *backbuf_header;
    // parameters
    playItem_t **pcurr; // pointer to current item
    int *pcount; // pointer to count of items in list
    int iterator; // index into next array of playItem_t struct
    int lastpos[2]; // last mouse position (for playlist widget)
    int multisel; // if it uses multiple selection
    // current state
    int scrollpos;
    int row;
    double clicktime; // for doubleclick detection
    int nvisiblerows;
// array of lengths and widths
// N = number of columns
// M = number of visible rows,
// cache[(ROW*ncolumns+COLUMN)*3+0] --- position to insert "...", or -1 if the whole line fits
// cache[(ROW*ncolumns+COLUMN)*3+1] --- width extent in pixels
// cache[(ROW*ncolumns+COLUMN)*3+2] --- 0 if needs recalc
    int16_t *fmtcache; // cached text formatting
    int header_fitted[pl_ncolumns];
    char colnames_fitted[pl_ncolumns][pl_colname_max]; // cached formatted names of columns
    int colwidths[pl_ncolumns]; // current column widths
} gtkplaylist_t;

#define GTKPL_PROLOGUE \
    gtkplaylist_t *ps = (gtkplaylist_t *)gtk_object_get_data (GTK_OBJECT (widget), "ps"); assert (ps); 

// that must be called before gtk_init
void
gtkpl_init (void);

void
gtkpl_redraw_pl_row (gtkplaylist_t *ps, int row);

void
gtkpl_redraw_pl_row_novis (gtkplaylist_t *ps, int row);

void
gtkpl_setup_scrollbar (gtkplaylist_t *ps);

void
gtkpl_draw_pl_row_back (gtkplaylist_t *ps, cairo_t *cr, int row, playItem_t *it);

void
gtkpl_draw_pl_row (gtkplaylist_t *ps, cairo_t *cr, int row, playItem_t *it);

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
gtkpl_add_files (gtkplaylist_t *ps, GSList *lst);

void
gtkpl_configure (gtkplaylist_t *ps);

int
gtkpl_get_idx_of (gtkplaylist_t *ps, playItem_t *it);

playItem_t *
gtkpl_get_for_idx (gtkplaylist_t *ps, int idx);

// this functions take value from passed playlist, that's why it's here
void
gtkpl_playsong (gtkplaylist_t *ps);

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
gtkpl_set_cairo_source_rgb (cairo_t *cr, int col);

void
gtkpl_set_cairo_font (cairo_t *cr);

#endif // __GTKPLAYLIST_H
