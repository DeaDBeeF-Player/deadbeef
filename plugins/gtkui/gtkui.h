/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __GTKUI_H
#define __GTKUI_H

#if HAVE_NOTIFY
#define NOTIFY_DEFAULT_FORMAT "%a - %t"
#endif

#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

//#if defined(ULTRA_COMPATIBLE)
//#warning compiling for compatibility with gtk <2.14
//#endif

#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;
extern GtkWidget *mainwin;
extern GtkWidget *searchwin;
extern int gtkui_embolden_current_track;

struct _GSList;

// misc utility functions

void
gtkui_add_dirs (struct _GSList *lst);

void
gtkui_add_files (struct _GSList *lst);

void
gtkui_open_files (struct _GSList *lst);

void
gtkui_receive_fm_drop (DB_playItem_t *before, char *mem, int length);

void
preferences_fill_soundcards (void);

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

void
theme_set_cairo_source_rgb (cairo_t *cr, int col);

void
theme_set_fg_color (int col);

void
theme_set_bg_color (int col);

void
playlist_refresh (void);

void
search_refresh (void);

int
gtkui_add_new_playlist (void);

void
seekbar_redraw (void);

void
seekbar_draw (GtkWidget *widget, cairo_t *cr);

gboolean
on_seekbar_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_seekbar_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event);

gboolean
on_seekbar_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event);

void
volumebar_redraw (void);

//void
//tabstrip_redraw (void);

void
gtkui_playlist_changed (void);

void
gtkui_set_titlebar (DB_playItem_t *it);

gboolean
gtkui_progress_show_idle (gpointer data);

gboolean
gtkui_progress_hide_idle (gpointer data);

gboolean
gtkui_set_progress_text_idle (gpointer data);

int
gtkui_add_file_info_cb (DB_playItem_t *it, void *data);

extern int (*gtkui_original_plt_add_dir) (ddb_playlist_t *plt, const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
extern int (*gtkui_original_plt_add_file) (ddb_playlist_t *plt, const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

void
gtkui_focus_on_playing_track (void);

void
gtkui_playlist_set_curr (int playlist);

void
gtkui_setup_gui_refresh ();

int
gtkui_get_curr_playlist_mod (void);

void
gtkui_trackinfochanged (DB_playItem_t *it);

gboolean
redraw_queued_tracks_cb (gpointer plt);

extern DB_playItem_t * (*gtkui_original_plt_load) (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

#endif
