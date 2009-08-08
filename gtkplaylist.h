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

enum {
    TARGET_URILIST,
    TARGET_SAMEWIDGET,
};

#define ps_ncolumns 5
#define ps_colname_max 100

// structure of this kind must be set as user data for playlist, header and scrollbar widgets
// pointer to this structure must be passed too all functions that
// implement playlist functionality (like this pointer)
typedef struct {
    // cached gtk/gdk object pointers
    GtkWidget *playlist;
    GtkWidget *header;
    GtkWidget *scrollbar;
    GdkPixmap *backbuf;
    // parameters
    playItem_t **phead; // pointer to head of list to display
    playItem_t **pcurr; // pointer to current item
    int update_statusbar; // whether it needs to update status bar in certain cases
    int has_dragndrop; // whether it has drag and drop capability
    int lastpos[2]; // last mouse position (for playlist widget)
    // current state
    int scrollpos;
    int row;
    double clicktime; // for doubleclick detection
    int nvisiblerows;
    int16_t *fmtcache; // cached text formatting
    int header_fitted[ps_ncolumns];
    char colnames_fitted[ps_ncolumns][ps_colname_max]; // cached formatted names of columns
    int colwidths[ps_ncolumns]; // current column widths
} gtkplaylist_t;

#define GTKPS_PROLOGUE \
    gtkplaylist_t *ps = (gtkplaylist_t *)gtk_object_get_data (GTK_OBJECT (widget), "ps"); assert (ps); 

void
gtkps_redraw_ps_row (gtkplaylist_t *ps, int row);

void
gtkps_redraw_ps_row_novis (gtkplaylist_t *ps, int row);

void
gtkps_setup_scrollbar (gtkplaylist_t *ps);

void
gtkps_draw_ps_row_back (gtkplaylist_t *ps, cairo_t *cr, int row, playItem_t *it);

void
gtkps_draw_ps_row (gtkplaylist_t *ps, cairo_t *cr, int row, playItem_t *it);

void
gtkps_draw_playlist (gtkplaylist_t *ps, int x, int y, int w, int h);

void
gtkps_reconf (gtkplaylist_t *ps);

void
gtkps_expose (gtkplaylist_t *ps, int x, int y, int w, int h);

void
gtkps_mouse1_pressed (gtkplaylist_t *ps, int state, int ex, int ey, double time);

void
gtkps_mouse1_released (gtkplaylist_t *ps, int state, int ex, int ey, double time);

void
gtkps_mousemove (gtkplaylist_t *ps, GdkEventMotion *event);

void
gtkps_scroll (gtkplaylist_t *ps, int newscroll);

void
gtkps_handle_scroll_event (gtkplaylist_t *ps, int direction);

void
gtkps_keypress (gtkplaylist_t *ps, int keyval, int state);

void
gtkps_track_dragdrop (gtkplaylist_t *ps, int y);

void
gtkps_handle_drag_drop (gtkplaylist_t *ps, int drop_y, uint32_t *d, int length);

void
gtkps_handle_fm_drag_drop (gtkplaylist_t *ps, int drop_y, void *ptr, int length);

void
gtkps_add_fm_dropped_files (gtkplaylist_t *ps, char *ptr, int length, int drop_y);

void
gtkps_select_single (gtkplaylist_t *ps, int sel);

void
gtkps_header_draw (gtkplaylist_t *ps);

void
gtkps_add_dir (gtkplaylist_t *ps, char *folder);

void
gtkps_add_files (gtkplaylist_t *ps, GSList *lst);

void
gtkps_configure (gtkplaylist_t *ps);

// this functions take value from passed playlist, that's why it's here
void
gtkps_playsong (gtkplaylist_t *ps);

void
gtkps_songchanged (gtkplaylist_t *ps, int from, int to);

// these functions operate on global playlist level,
// no need to pass gtkplaylist_t ptr to them

void
gtkps_add_fm_dropped_files (gtkplaylist_t *ps, char *ptr, int length, int drop_y);

// these functions should not belong here
void
gtkps_prevsong (void);

void
gtkps_nextsong (void);

void
gtkps_randomsong (void);

void
gtkps_pausesong (void);

void
gtkps_playsongnum (int idx);

#endif // __GTKPLAYLIST_H
