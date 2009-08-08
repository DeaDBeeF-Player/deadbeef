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
    int update_statusbar; // whether it needs to update status bar in certain cases
    int has_dragndrop; // whether it has drag and drop capability
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

void
redraw_ps_row (GtkWidget *widget, int row);

void
gtkps_setup_scrollbar (void);

void
draw_ps_row_back (GdkDrawable *drawable, cairo_t *cr, int row, playItem_t *it);

void
draw_ps_row (GdkDrawable *drawable, cairo_t *cr, int row, playItem_t *it);

void
draw_playlist (GtkWidget *widget, int x, int y, int w, int h);

void
gtkps_reconf (GtkWidget *widget);

void
gtkps_expose (GtkWidget       *widget, int x, int y, int w, int h);

void
gtkps_mouse1_pressed (int state, int ex, int ey, double time);

void
gtkps_mouse1_released (int state, int ex, int ey, double time);

void
gtkps_mousemove (GdkEventMotion *event);

void
gtkps_scroll (int newscroll);

void
gtkps_stopsong (void);

void
gtkps_playsong (void);

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

void
gtkps_update_songinfo (void);

void
gtkps_songchanged (int from, int to);

void
gtkps_handle_scroll_event (int direction);

void
gtkps_keypress (int keyval, int state);

void
gtkps_track_dragdrop (int y);

void
gtkps_handle_drag_drop (int drop_y, uint32_t *d, int length);

void
gtkps_handle_fm_drag_drop (int drop_y, void *ptr, int length);

void
gtkps_add_dir (char *dir);

void
gtkps_add_files (GSList *lst);

void
gtkps_add_fm_dropped_files (char *ptr, int length, int drop_y);

#endif
