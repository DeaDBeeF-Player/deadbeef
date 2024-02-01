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

#ifndef __GTKUI_H
#define __GTKUI_H

#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#    include "../../config.h"
#endif

#include <deadbeef/deadbeef.h>

#if GTK_CHECK_VERSION(3, 0, 0)
#    include "deadbeefapp.h"
extern DeadbeefApp *gapp;
#endif

extern DB_functions_t *deadbeef;
extern GtkWidget *mainwin;

extern int gtkui_embolden_selected_tracks;
extern int gtkui_embolden_tracks;
extern int gtkui_embolden_current_track;
extern int gtkui_italic_selected_tracks;
extern int gtkui_italic_tracks;
extern int gtkui_italic_current_track;

extern int gtkui_unicode_playstate;
extern int gtkui_disable_seekbar_overlay;

extern int gtkui_tabstrip_embolden_selected;
extern int gtkui_tabstrip_embolden_playing;
extern int gtkui_tabstrip_italic_selected;
extern int gtkui_tabstrip_italic_playing;

struct _GSList;

extern int gtkui_groups_pinned;
extern int gtkui_groups_spacing;
extern int gtkui_listview_busy;

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
gtkui_add_location (const char *path, const char *custom_title);

void
gtkui_receive_fm_drop (DB_playItem_t *before, char *mem, int length);

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

int
gtkui_add_new_playlist (void);

int
gtkui_copy_playlist (ddb_playlist_t *plt);

void
seekbar_draw (GtkWidget *widget, cairo_t *cr);

gboolean
on_seekbar_button_press_event (GtkWidget *widget, GdkEventButton *event);

gboolean
on_seekbar_button_release_event (GtkWidget *widget, GdkEventButton *event);

gboolean
on_seekbar_motion_notify_event (GtkWidget *widget, GdkEventMotion *event);

void
gtkui_set_titlebar (DB_playItem_t *it);

gboolean
gtkui_progress_show_idle (gpointer data);

gboolean
gtkui_progress_hide_idle (gpointer data);

gboolean
gtkui_set_progress_text_idle (gpointer data);

int
gtkui_rename_playlist (ddb_playlist_t *plt);

int
gtkui_rename_playlist_at_index (int plt_idx);

int
gtkui_remove_playlist (ddb_playlist_t *plt);

int
gtkui_remove_playlist_at_index (int plt_idx);

int
gtkui_get_curr_playlist_mod (void);

void
mainwin_toggle_visible (void);

void
gtkui_show_info_window (const char *fname, const char *title, GtkWidget **pwindow);

void
on_gtkui_info_window_delete (GtkWidget *widget, GtkTextDirection previous_direction, GtkWidget **pwindow);

GtkWidget *
gtkui_create_pltmenu (ddb_playlist_t *plt);

void
gtkui_free_pltmenu (void);

void
gtkui_quit (void);

extern int gtkui_hotkey_grabbing;
gboolean
on_hotkeys_set_key_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data);

int
gtkui_get_gui_refresh_rate (void);

void
gtkui_titlebar_tf_init (void);

void
gtkui_show_log_window (gboolean show);

void
gtkui_toggle_log_window (void);

void
gtkui_mainwin_init (void);

void
gtkui_mainwin_free (void);

enum GtkuiFileChooserType {
    GTKUI_FILECHOOSER_OPENFILE,
    GTKUI_FILECHOOSER_OPENFOLDER,
    GTKUI_FILECHOOSER_LOADPLAYLIST,
    GTKUI_FILECHOOSER_SAVEPLAYLIST
};

GSList *
show_file_chooser (const gchar *title, enum GtkuiFileChooserType type, gboolean select_multiple);

char *
gtkui_trim_whitespace (char *p, size_t len);

void
gtkui_dispatch_on_main (void (^block) (void));

#endif
