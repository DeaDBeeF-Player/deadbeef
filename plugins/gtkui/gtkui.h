/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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

// workaround to make older gtk compatible with vala codegen
#if !GTK_CHECK_VERSION(2,14,0)
#define gtk_widget_get_window(widget) ((widget)->window)
#endif

#if !GTK_CHECK_VERSION(2,18,0)
#define gtk_widget_set_has_window(widget, has_window) \
  if (has_window) GTK_WIDGET_UNSET_FLAGS (widget, GTK_NO_WINDOW); \
  else GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);

#define gtk_widget_get_visible(widget) (GTK_WIDGET_VISIBLE(widget))
#define gtk_widget_get_has_window(widget) (!GTK_WIDGET_NO_WINDOW(widget))
#endif

#if !GTK_CHECK_VERSION(2,20,0)
#define gtk_widget_get_realized(widget) (GTK_WIDGET_REALIZED(widget))
#endif

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

// plugin configuration dialogs

void
plugin_configure (GtkWidget *parentwin, DB_plugin_t *p);

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
seekbar_draw (GtkWidget *widget);

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

void
tabstrip_redraw (void);

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

extern int (*gtkui_original_pl_add_dir) (const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
extern int (*gtkui_original_pl_add_file) (const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

void
gtkui_focus_on_playing_track (void);

#endif
