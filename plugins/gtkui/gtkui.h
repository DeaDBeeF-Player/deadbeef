/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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

#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;
extern GtkWidget *mainwin;
extern GtkWidget *searchwin;

extern int gtkui_embolden_selected_tracks;
extern int gtkui_embolden_tracks;
extern int gtkui_embolden_current_track;
extern int gtkui_italic_selected_tracks;
extern int gtkui_italic_tracks;
extern int gtkui_italic_current_track;

extern int gtkui_is_retina;
extern int gtkui_unicode_playstate;
extern int gtkui_disable_seekbar_overlay;

extern int gtkui_tabstrip_embolden_selected;
extern int gtkui_tabstrip_embolden_playing;
extern int gtkui_tabstrip_italic_selected;
extern int gtkui_tabstrip_italic_playing;

struct _GSList;

extern int gtkui_groups_pinned;

extern const char *gtkui_default_titlebar_playing;
extern const char *gtkui_default_titlebar_stopped;

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
gtkui_set_titlebar (DB_playItem_t *it);

gboolean
gtkui_progress_show_idle (gpointer data);

gboolean
gtkui_progress_hide_idle (gpointer data);

gboolean
gtkui_set_progress_text_idle (gpointer data);

void
gtkui_playlist_set_curr (int playlist);

int
gtkui_get_curr_playlist_mod (void);

void
gtkui_trackinfochanged (DB_playItem_t *it);

gboolean
redraw_queued_tracks_cb (gpointer plt);

void
mainwin_toggle_visible (void);

void
gtkui_show_info_window (const char *fname, const char *title, GtkWidget **pwindow);

void
on_gtkui_info_window_delete (GtkWidget *widget, GtkTextDirection previous_direction, GtkWidget **pwindow);

GtkWidget*
gtkui_create_pltmenu (int plt_idx);

void
plt_get_title_wrapper (int plt, char *buffer, int len);

void
gtkui_quit (void);

void
gtkui_run_preferences_dlg (void);

int
gtkui_get_gui_refresh_rate ();

#endif
