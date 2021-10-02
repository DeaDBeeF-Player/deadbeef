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


#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <gtk/gtk.h>
#include <jansson.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <deadbeef/deadbeef.h>
#include "../../gettext.h"
#include "../hotkeys/hotkeys.h"
#include "../libparser/parser.h"
#include "actionhandlers.h"
#include "actions.h"
#include "callbacks.h"
#include "clipboard.h"
#include "covermanager/covermanager.h"
#include "ddbtabstrip.h"
#include "drawing.h"
#include "eq.h"
#include "gtkui.h"
#include "gtkui_api.h"
#include "hotkeys.h"
#include "interface.h"
#include "medialib/medialibmanager.h"
#include "medialib/medialibwidget.h"
#include "playlist/plcommon.h"
#include "pluginconf.h"
#include "prefwin/prefwin.h"
#include "progress.h"
#include "rg.h"
#include "search.h"
#include "support.h"
#include "trkproperties.h"
#include "widgets.h"
#include "wingeom.h"
#include "plmenu.h"
#include "covermanager/albumartwidget.h"
#include "selpropertieswidget.h"

#define USE_GTK_APPLICATION 1

#if GTK_CHECK_VERSION(3,10,0) && USE_GTK_APPLICATION
#include "deadbeefapp.h"
#endif

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

ddb_gtkui_t plugin;
DB_functions_t *deadbeef;

// main widgets
GtkWidget *mainwin;
int gtkui_override_statusicon = 0;
GtkStatusIcon *trayicon;
GtkWidget *traymenu;
GtkWidget *logwindow;

static int gtkui_accept_messages = 0;

static gint refresh_timeout = 0;
static guint set_title_timeout_id;

int fileadded_listener_id;
int fileadd_beginend_listener_id;

// cached config variables
int gtkui_embolden_current_track;
int gtkui_embolden_tracks;
int gtkui_embolden_selected_tracks;
int gtkui_italic_selected_tracks;
int gtkui_italic_tracks;
int gtkui_italic_current_track;

int gtkui_tabstrip_embolden_playing;
int gtkui_tabstrip_italic_playing;
int gtkui_tabstrip_embolden_selected;
int gtkui_tabstrip_italic_selected;

int gtkui_groups_pinned;
int gtkui_listview_busy;
int gtkui_groups_spacing;

int gtkui_unicode_playstate = 0;
int gtkui_disable_seekbar_overlay = 0;

static gboolean _quitting_normally = FALSE;

#define TRAY_ICON "deadbeef_tray_icon"

const char *gtkui_default_titlebar_playing = "DeaDBeeF - %artist% - %title%";
const char *gtkui_default_titlebar_stopped = "DeaDBeeF";

static char *titlebar_playing_bc;
static char *titlebar_stopped_bc;

static char *statusbar_bc;
static char *statusbar_stopped_bc;

enum {
    INFO_TARGET_URIS
};

static void
init_widget_layout (void);

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];

static void
format_timestr (char *buf, int sz, float time) {
    time = roundf(time);
    int daystotal = (int)time / (3600*24);
    int hourtotal = ((int)time / 3600) % 24;
    int mintotal = ((int)time/60) % 60;
    int sectotal = ((int)time) % 60;

    if (daystotal == 0) {
        snprintf (buf, sz, "%d:%02d:%02d", hourtotal, mintotal, sectotal);
    }
    else if (daystotal == 1) {
        snprintf (buf, sz, _("1 day %d:%02d:%02d"), hourtotal, mintotal, sectotal);
    }
    else {
        snprintf (buf, sz, _("%d days %d:%02d:%02d"), daystotal, hourtotal, mintotal, sectotal);
    }
}

static gboolean
update_songinfo (gpointer unused) {
    if (w_get_rootwidget() == NULL) return FALSE;
 
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    DB_output_t *output = deadbeef->get_output ();
    char sbtext_new[512] = "-";

    float pl_totaltime = deadbeef->pl_get_totaltime ();
    char totaltime_str[512] = "";
    format_timestr(totaltime_str, sizeof (totaltime_str), pl_totaltime);

    DB_playItem_t *track = deadbeef->streamer_get_playing_track_safe ();

    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = track,
        .iter = PL_MAIN,
        .plt = deadbeef->plt_get_curr()
    };

    char buffer[200];
    char *bc = NULL;


    if (!output || (output->state () == DDB_PLAYBACK_STATE_STOPPED || !track)) {
        bc = statusbar_stopped_bc;
    }
    else {
        bc = statusbar_bc;
    }

    deadbeef->tf_eval (&ctx, bc, buffer, sizeof (buffer));
    snprintf (sbtext_new,
              sizeof (sbtext_new),
              "%s | %d tracks | %s %s",
              buffer,
              deadbeef->pl_getcount (PL_MAIN),
              totaltime_str,
              _("total playtime"));

    if (strcmp (sbtext_new, sb_text)) {
        strcpy (sb_text, sbtext_new);

        // form statusline
        GtkStatusbar *sb = GTK_STATUSBAR (lookup_widget (mainwin, "statusbar"));
        if (sb_context_id == -1) {
            sb_context_id = gtk_statusbar_get_context_id (sb, "msg");
        }

        gtk_statusbar_pop (sb, sb_context_id);
        gtk_statusbar_push (sb, sb_context_id, sb_text);
    }

    if (track) {
        deadbeef->pl_item_unref (track);
    }

    deadbeef->plt_unref (ctx.plt);

    return FALSE;
}

void
set_tray_tooltip (const char *text) {
    if (trayicon) {
#if !GTK_CHECK_VERSION(2,16,0)
        gtk_status_icon_set_tooltip (trayicon, text);
#else
        gtk_status_icon_set_tooltip_text (trayicon, text);
#endif
    }
}

gboolean
on_trayicon_scroll_event               (GtkWidget       *widget,
                                        GdkEventScroll  *event,
                                        gpointer         user_data)
{
    int change_track = deadbeef->conf_get_int ("tray.scroll_changes_track", 0)
        ? ((event->state & GDK_CONTROL_MASK) == 0)
        : ((event->state & GDK_CONTROL_MASK) != 0)
        ;
    if (change_track) {
        if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
            deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
        }
        else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
            deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
        }
        return FALSE;
    }

    float vol = deadbeef->volume_get_db ();
    int sens = deadbeef->conf_get_int ("gtkui.tray_volume_sensitivity", 1);
    if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
        vol += sens;
    }
    else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
        vol -= sens;
    }
    if (vol > 0) {
        vol = 0;
    }
    else if (vol < deadbeef->volume_get_min_db ()) {
        vol = deadbeef->volume_get_min_db ();
    }
    deadbeef->volume_set_db (vol);

#if 0
    char str[100];
    if (deadbeef->conf_get_int ("gtkui.show_gain_in_db", 1)) {
        snprintf (str, sizeof (str), "Gain: %s%d dB", vol == 0 ? "+" : "", (int)vol);
    }
    else {
        snprintf (str, sizeof (str), "Gain: %d%%", (int)(deadbeef->volume_get_amp () * 100));
    }
    set_tray_tooltip (str);
#endif

    return FALSE;
}

static gboolean
show_traymenu_cb (gpointer data) {
    if (!traymenu) {
        traymenu = create_traymenu ();
    }
    gtk_menu_popup (GTK_MENU (traymenu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
    return FALSE;
}

static void
show_traymenu (void) {
    g_idle_add (show_traymenu_cb, NULL);
}

#if GTK_CHECK_VERSION(3,0,0)
static gboolean
mainwin_hide_cb (gpointer data) {
     gtk_widget_hide (mainwin);
     return FALSE;
 }
#endif

void
mainwin_toggle_visible (void) {
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (gtk_widget_get_visible (mainwin) && !iconified) {
        gtk_widget_hide (mainwin);
    }
    else {
        if (w_get_rootwidget() == NULL) init_widget_layout ();
      
        wingeom_restore (mainwin, "mainwin", 40, 40, 500, 300, 0);
        if (iconified) {
            gtk_window_deiconify (GTK_WINDOW(mainwin));
        }
        else {
            gtk_window_present (GTK_WINDOW (mainwin));
        }
    }
}

#if !GTK_CHECK_VERSION(2,14,0)
gboolean
on_trayicon_activate (GtkWidget       *widget,
                                        gpointer         user_data)
{
    mainwin_toggle_visible ();
    return FALSE;
}

#else

gboolean
on_trayicon_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
        mainwin_toggle_visible ();
    }
    else if (event->button == 2 && event->type == GDK_BUTTON_PRESS) {
        deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
    }
    return FALSE;
}
#endif

gboolean
on_trayicon_popup_menu (GtkWidget       *widget,
        guint button,
        guint time,
                                        gpointer         user_data)
{
    gtk_menu_popup (GTK_MENU (traymenu), NULL, NULL, gtk_status_icon_position_menu, trayicon, button, time);
    return FALSE;
}

static gboolean
activate_cb (gpointer nothing) {
    if (w_get_rootwidget() == NULL) init_widget_layout ();
    wingeom_restore (mainwin, "mainwin", 40, 40, 500, 300, 0);
    gtk_widget_show (mainwin);
    gtk_window_present (GTK_WINDOW (mainwin));
    return FALSE;
}

static void
titlebar_tf_free (void) {
    if (titlebar_playing_bc) {
        deadbeef->tf_free (titlebar_playing_bc);
        titlebar_playing_bc = NULL;
    }

    if (titlebar_stopped_bc) {
        deadbeef->tf_free (titlebar_stopped_bc);
        titlebar_stopped_bc = NULL;
    }

    if (statusbar_bc) {
        deadbeef->tf_free (statusbar_bc);
        statusbar_bc = NULL;
    }
    if (statusbar_stopped_bc) {
        deadbeef->tf_free (statusbar_stopped_bc);
        statusbar_stopped_bc = NULL;
    }
}

void
gtkui_titlebar_tf_init (void) {
    titlebar_tf_free ();

    char fmt[500];
    deadbeef->conf_get_str ("gtkui.titlebar_playing_tf", gtkui_default_titlebar_playing, fmt, sizeof (fmt));
    titlebar_playing_bc = deadbeef->tf_compile (fmt);
    deadbeef->conf_get_str ("gtkui.titlebar_stopped_tf", gtkui_default_titlebar_stopped, fmt, sizeof (fmt));
    titlebar_stopped_bc = deadbeef->tf_compile (fmt);

    static const char statusbar_tf_with_seltime_fmt[] = "$if2($strcmp(%%ispaused%%,),%s | )$if2($upper(%%codec%%),-) |[ %%playback_bitrate%% kbps |][ %%samplerate%%Hz |][ %%:BPS%% %s |][ %%channels%% |] %%playback_time%% / %%length%% | %%selection_playback_time%% %s";

    static const char statusbar_tf_fmt[] = "$if2($strcmp(%%ispaused%%,),%s | )$if2($upper(%%codec%%),-) |[ %%playback_bitrate%% kbps |][ %%samplerate%%Hz |][ %%:BPS%% %s |][ %%channels%% |] %%playback_time%% / %%length%%";

    const char statusbar_stopped_tf_with_seltime_fmt[] = "%s | %%selection_playback_time%% %s";
    const char statusbar_stopped_tf_fmt[] = "%s";

    char statusbar_tf[1024];
    char statusbar_stopped_tf[1024];

    if (deadbeef->conf_get_int ("gtkui.statusbar_seltime", 0)) {
        snprintf (statusbar_tf, sizeof (statusbar_tf), statusbar_tf_with_seltime_fmt, _("Paused"), _("bit"), _("selection playtime"));
        snprintf (statusbar_stopped_tf, sizeof (statusbar_stopped_tf), statusbar_stopped_tf_with_seltime_fmt, _("Stopped"), _("selection playtime"));
    }
    else {
        snprintf (statusbar_tf, sizeof (statusbar_tf), statusbar_tf_fmt, _("Paused"), _("bit"));
        snprintf (statusbar_stopped_tf, sizeof (statusbar_stopped_tf), statusbar_stopped_tf_fmt, _("Stopped"));
    }

    statusbar_bc = deadbeef->tf_compile (statusbar_tf);
    statusbar_stopped_bc = deadbeef->tf_compile (statusbar_stopped_tf);
}

static gboolean
set_title_cb (gpointer data) {
    gtkui_set_titlebar (NULL);
    return FALSE;
}

void
gtkui_set_titlebar (DB_playItem_t *it) {
    if (!it) {
        it = deadbeef->streamer_get_playing_track_safe ();
    }
    else {
        deadbeef->pl_item_ref (it);
    }
    char str[1024];
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = it,
        // FIXME: current playlist is not correct here.
        // need the playlist corresponding to the pointed track
        .plt = deadbeef->plt_get_curr (),
    };
    deadbeef->tf_eval (&ctx, it ? titlebar_playing_bc : titlebar_stopped_bc, str, sizeof (str));
    if (ctx.plt) {
        deadbeef->plt_unref (ctx.plt);
        ctx.plt = NULL;
    }
    gtk_window_set_title (GTK_WINDOW (mainwin), str);
    if (it) {
        deadbeef->pl_item_unref (it);
    }
    set_tray_tooltip (str);
    if (ctx.update > 0) {
        set_title_timeout_id = g_timeout_add(ctx.update, set_title_cb, NULL);
    }
}

static gboolean
trackinfochanged_cb (gpointer data) {
    DB_playItem_t *track = data;
    DB_playItem_t *curr = deadbeef->streamer_get_playing_track_safe ();
    if (track == curr) {
        gtkui_set_titlebar (track);
    }
    if (track) {
        deadbeef->pl_item_unref (track);
    }
    if (curr) {
        deadbeef->pl_item_unref (curr);
    }
    return FALSE;
}

static gboolean
playlistcontentchanged_cb (gpointer none) {
    trkproperties_fill_metadata ();
    return FALSE;
}

static gboolean
gtkui_on_frameupdate (gpointer data) {
    update_songinfo (NULL);

    return TRUE;
}

static gboolean
gtkui_update_status_icon (gpointer unused) {
    int hide_tray_icon = deadbeef->conf_get_int ("gtkui.hide_tray_icon", 0);
    if (gtkui_override_statusicon) {
        hide_tray_icon = 1;
    }
    if (hide_tray_icon && !trayicon) {
        return FALSE;
    }
    if (trayicon) {
        if (hide_tray_icon) {
            g_object_set (trayicon, "visible", FALSE, NULL);
        }
        else {
            g_object_set (trayicon, "visible", TRUE, NULL);
        }
        return FALSE;
    }
    // system tray icon
    traymenu = create_traymenu ();

    char tmp[1000];
    const char *icon_name = tmp;
    deadbeef->conf_get_str ("gtkui.custom_tray_icon", TRAY_ICON, tmp, sizeof (tmp));
    GtkIconTheme *theme = gtk_icon_theme_get_default();

    if (!gtk_icon_theme_has_icon(theme, icon_name))
        icon_name = "deadbeef";
    else {
        GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(theme, icon_name, 48, GTK_ICON_LOOKUP_USE_BUILTIN);
        const gboolean icon_is_builtin = gtk_icon_info_get_filename(icon_info) == NULL;
        gtk_icon_info_free(icon_info);
        icon_name = icon_is_builtin ? "deadbeef" : icon_name;
    }

    if (!gtk_icon_theme_has_icon(theme, icon_name)) {
        char iconpath[1024];
        snprintf (iconpath, sizeof (iconpath), "%s/deadbeef.png", deadbeef->get_system_dir(DDB_SYS_DIR_PREFIX));
        trayicon = gtk_status_icon_new_from_file(iconpath);
    }
    else {
        trayicon = gtk_status_icon_new_from_icon_name(icon_name);
    }
    if (hide_tray_icon) {
        g_object_set (trayicon, "visible", FALSE, NULL);
    }

#if !GTK_CHECK_VERSION(2,14,0)
    g_signal_connect ((gpointer)trayicon, "activate", G_CALLBACK (on_trayicon_activate), NULL);
#else
    printf ("connecting button tray signals\n");
    g_signal_connect ((gpointer)trayicon, "scroll_event", G_CALLBACK (on_trayicon_scroll_event), NULL);
    g_signal_connect ((gpointer)trayicon, "button_press_event", G_CALLBACK (on_trayicon_button_press_event), NULL);
#endif
    g_signal_connect ((gpointer)trayicon, "popup_menu", G_CALLBACK (on_trayicon_popup_menu), NULL);

    gtkui_set_titlebar (NULL);

    return FALSE;
}

static void
override_builtin_statusicon (int override) {
    gtkui_override_statusicon = override;
    g_idle_add (gtkui_update_status_icon, NULL);
}

static void
gtkui_hide_status_icon (void) {
    if (trayicon) {
        g_object_set (trayicon, "visible", FALSE, NULL);
    }
}

int
gtkui_get_curr_playlist_mod (void) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    int res = plt ? deadbeef->plt_get_modification_idx (plt) : 0;
    if (plt) {
        deadbeef->plt_unref (plt);
    }
    return res;
}

int
gtkui_rename_playlist (ddb_playlist_t *plt) {
    GtkWidget *dlg = create_entrydialog ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Rename Playlist"));
    GtkWidget *e;
    e = lookup_widget (dlg, "title_label");
    gtk_label_set_text (GTK_LABEL(e), _("Title:"));
    e = lookup_widget (dlg, "title");
    char t[1000];
    deadbeef->plt_get_title (plt, t, sizeof (t));
    gtk_entry_set_text (GTK_ENTRY (e), t);
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        const char *text = gtk_entry_get_text (GTK_ENTRY (e));
        deadbeef->plt_set_title (plt, text);
    }
    gtk_widget_destroy (dlg);
    return 0;
}

int gtkui_remove_playlist (ddb_playlist_t *plt) {
    int plt_idx = deadbeef->plt_get_idx(plt);
    if (plt_idx == -1) {
        return -1;
    }

    char title[500];
    if (deadbeef->plt_get_first(plt, PL_MAIN) != NULL) {
        // playlist not empty, confirm first.
        deadbeef->plt_get_title (plt, title, sizeof (title));
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Removing playlist"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Do you really want to remove the playlist '%s'?"), title);
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));
        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return -1;
        }
    }

    deadbeef->plt_remove (plt_idx);
    return 0;
}

int gtkui_remove_playlist_at_index (int plt_idx) {
    int res = -1;
    ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt_idx);
    if (p != NULL) {
        res = gtkui_remove_playlist (p);
    }
    return res;
}

int
gtkui_rename_playlist_at_index (int plt_idx) {
    int res = -1;
    ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt_idx);
    if (p != NULL) {
        res = gtkui_rename_playlist(p);
        deadbeef->plt_unref (p);
    }
    return res;
}

void
gtkui_setup_gui_refresh (void) {
    int tm = 1000/gtkui_get_gui_refresh_rate ();

    if (refresh_timeout) {
        g_source_remove (refresh_timeout);
        refresh_timeout = 0;
    }

    refresh_timeout = g_timeout_add (tm, gtkui_on_frameupdate, NULL);
}


static gboolean
gtkui_on_configchanged (void *data) {
    // order and looping
    const char *w;

    // order
    const char *orderwidgets[4] = { "order_linear", "order_shuffle", "order_random", "order_shuffle_albums" };
    w = orderwidgets[deadbeef->streamer_get_shuffle ()];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);

    // looping
    const char *loopingwidgets[3] = { "loop_all", "loop_disable", "loop_single" };
    w = loopingwidgets[deadbeef->streamer_get_repeat ()];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);

    // scroll follows playback
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "scroll_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.followplayback", 1) ? TRUE : FALSE);

    // cursor follows playback
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "cursor_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1) ? TRUE : FALSE);

    // stop after current track
    int stop_after_current = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "stop_after_current")), stop_after_current ? TRUE : FALSE);

    // stop after current album
    int stop_after_album = deadbeef->conf_get_int ("playlist.stop_after_album", 0);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "stop_after_album")), stop_after_album ? TRUE : FALSE);

    gtkui_embolden_current_track = deadbeef->conf_get_int ("gtkui.embolden_current_track", 0);
    gtkui_embolden_tracks = deadbeef->conf_get_int ("gtkui.embolden_tracks", 0);
    gtkui_embolden_selected_tracks = deadbeef->conf_get_int ("gtkui.embolden_selected_tracks", 0);
    gtkui_italic_current_track = deadbeef->conf_get_int ("gtkui.italic_current_track", 0);
    gtkui_italic_tracks = deadbeef->conf_get_int ("gtkui.italic_tracks", 0);
    gtkui_italic_selected_tracks = deadbeef->conf_get_int ("gtkui.italic_selected_tracks", 0);
    gtkui_tabstrip_embolden_playing = deadbeef->conf_get_int ("gtkui.tabstrip_embolden_playing", 0);
    gtkui_tabstrip_italic_playing = deadbeef->conf_get_int ("gtkui.tabstrip_italic_playing", 0);
    gtkui_tabstrip_embolden_selected = deadbeef->conf_get_int ("gtkui.tabstrip_embolden_selected", 0);
    gtkui_tabstrip_italic_selected = deadbeef->conf_get_int ("gtkui.tabstrip_italic_selected", 0);

    // titlebar tf
    gtkui_titlebar_tf_init ();

    // pin groups
    gtkui_groups_pinned = deadbeef->conf_get_int ("playlist.pin.groups", 0);
    gtkui_groups_spacing = deadbeef->conf_get_int ("playlist.groups.spacing", 0);

    // play state images
    gtkui_unicode_playstate = deadbeef->conf_get_int ("gtkui.unicode_playstate", 0);

    // seekbar overlay
    gtkui_disable_seekbar_overlay = deadbeef->conf_get_int ("gtkui.disable_seekbar_overlay", 0);

    // tray icon
    gtkui_update_status_icon (NULL);

    // statusbar refresh
    gtkui_setup_gui_refresh ();

    return FALSE;
}

static gboolean
outputchanged_cb (gpointer nothing) {
    prefwin_fill_soundcards ();
    return FALSE;
}

void
save_playlist_as (void) {
    gdk_threads_add_idle (action_save_playlist_handler_cb, NULL);
}

void
on_playlist_save_as_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    save_playlist_as ();
}

void
on_playlist_load_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_load_playlist_handler_cb, NULL);
}

void
on_add_location_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_add_location_handler_cb, NULL);
}

int
gtkui_add_new_playlist (void) {
    int cnt = deadbeef->plt_get_count ();
    int i;
    int idx = 0;
    for (;;) {
        char name[100];
        if (!idx) {
            strcpy (name, _("New Playlist"));
        }
        else {
            snprintf (name, sizeof (name), _("New Playlist (%d)"), idx);
        }
        deadbeef->pl_lock ();
        for (i = 0; i < cnt; i++) {
            char t[100];
            ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
            deadbeef->plt_get_title (plt, t, sizeof (t));
            deadbeef->plt_unref (plt);
            if (!strcasecmp (t, name)) {
                break;
            }
        }
        deadbeef->pl_unlock ();
        if (i == cnt) {
            return deadbeef->plt_add (cnt, name);
        }
        idx++;
    }
    return -1;
}

int
gtkui_get_gui_refresh_rate (void) {
    int fps = deadbeef->conf_get_int ("gtkui.refresh_rate", 10);
    if (fps < 1) {
        fps = 1;
    }
    else if (fps > 30) {
        fps = 30;
    }
    return fps;
}

static void
send_messages_to_widgets (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    for (ddb_gtkui_widget_t *c = w->children; c; c = c->next) {
        send_messages_to_widgets (c, id, ctx, p1, p2);
    }
    if (w->message) {
        w->message (w, id, ctx, p1, p2);
    }
}

gboolean
add_mainmenu_actions_cb (void *data) {
    add_mainmenu_actions ();
    return FALSE;
}

int
gtkui_plt_add_dir (ddb_playlist_t *plt, const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

int
gtkui_plt_add_file (ddb_playlist_t *plt, const char *filename, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

int
gtkui_pl_add_files_begin (ddb_playlist_t *plt);

void
gtkui_pl_add_files_end (void);

DB_playItem_t *
gtkui_plt_load (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);


static int
is_current_playlist (DB_playItem_t *it) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        ddb_playlist_t *it_plt = deadbeef->pl_get_playlist (it);
        if (it_plt) {
            if (it_plt == plt) {
                deadbeef->plt_unref (it_plt);
                deadbeef->plt_unref (plt);
                return 1;
            }
            deadbeef->plt_unref (it_plt);
        }
        deadbeef->plt_unref (plt);
    }
    return 0;
}

// The intended scroll position is set for background tabs, etc.
// This will be overwritten if there is a current main listview, or when the playlist is loaded by a listview
static void
playlist_set_intended_scroll (DB_playItem_t *it) {
    ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
    if (plt) {
        int idx = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
        if (idx != -1) {
            deadbeef->plt_set_scroll (plt, -1 * idx);
        }
        deadbeef->plt_unref (plt);
    }
}

static gboolean
pre_songstarted_cb (gpointer data) {
    DB_playItem_t *it = data;
    if ((!gtkui_listview_busy || !is_current_playlist (it)) && deadbeef->conf_get_int ("playlist.scroll.followplayback", 1)) {
        playlist_set_intended_scroll (it);
    }
    deadbeef->pl_item_unref (it);
    return FALSE;
}

static gboolean
pre_trackfocus_cb (gpointer data) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        playlist_set_intended_scroll (it);
        deadbeef->pl_item_unref (it);
    }
    return FALSE;
}

// These functions handle cases that need a playlist switch, or when there is no main listview to take the message
static void
playlist_set_cursor (ddb_playlist_t *plt, DB_playItem_t *it) {
    int idx = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
    if (idx != -1) {
        if (deadbeef->pl_is_selected (it) || idx != deadbeef->plt_get_cursor (plt, PL_MAIN) || deadbeef->plt_getselcount (plt) != 1) {
            deadbeef->plt_deselect_all (plt);
            deadbeef->pl_set_selected (it, 1);
            deadbeef->plt_set_cursor (plt, PL_MAIN, idx);
            ddb_event_track_t *event = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_CURSOR_MOVED);
            event->track = it;
            deadbeef->pl_item_ref (it);
            deadbeef->event_send ((ddb_event_t *)event, PL_MAIN, 0);
        }
    }
}

static gboolean
songstarted_cb (gpointer data) {
    DB_playItem_t *it = data;
    if ((!gtkui_listview_busy || !is_current_playlist (it)) && deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1)) {
        deadbeef->pl_lock ();
        ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
        if (plt) {
            playlist_set_cursor (plt, it);
            deadbeef->plt_unref (plt);
        }
        deadbeef->pl_unlock ();
    }
    deadbeef->pl_item_unref (it);
    return FALSE;
}

static gboolean
trackfocus_cb (gpointer data) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        deadbeef->pl_lock ();
        ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
        if (plt) {
            deadbeef->plt_set_curr (plt);
            playlist_set_cursor (plt, it);
            deadbeef->plt_unref (plt);
        }
        deadbeef->pl_unlock ();
        deadbeef->pl_item_unref (it);
    }
    return FALSE;
}

static int
gtkui_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (!gtkui_accept_messages) {
        return -1;
    }

    switch (id) {
    case DB_EV_SONGSTARTED:
    {
        ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
        if (ev->track) {
            deadbeef->pl_item_ref (ev->track);
            g_idle_add (pre_songstarted_cb, ev->track);
        }
        break;
    }
    case DB_EV_TRACKFOCUSCURRENT:
        g_idle_add (pre_trackfocus_cb, NULL);
        break;
    case DB_EV_SONGCHANGED:
        // update titlebar when to==NULL, i.e. playback has stopped
        if (!((ddb_event_trackchange_t *)ctx)->to) {
            g_idle_add (trackinfochanged_cb, NULL);
        }
        break;
    }

    search_message(id, ctx, p1, p2);
    ddb_gtkui_widget_t *rootwidget = w_get_rootwidget ();
    if (rootwidget) {
        send_messages_to_widgets (rootwidget, id, ctx, p1, p2);
    }

    switch (id) {
    case DB_EV_ACTIVATED:
        g_idle_add (activate_cb, NULL);
        break;
    case DB_EV_SONGSTARTED:
        g_idle_add (set_title_cb, NULL);
        ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
        if (ev->track) {
            deadbeef->pl_item_ref (ev->track);
            g_idle_add (songstarted_cb, ev->track);
        }
        break;
    case DB_EV_TRACKINFOCHANGED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                deadbeef->pl_item_ref (ev->track);
                g_idle_add (trackinfochanged_cb, ev->track);
            }
        }
        break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT) {
            g_idle_add (playlistcontentchanged_cb, NULL);
        }
        break;
    case DB_EV_TRACKFOCUSCURRENT:
        g_idle_add (trackfocus_cb, NULL);
        break;
    case DB_EV_CONFIGCHANGED:
        g_idle_add (gtkui_on_configchanged, NULL);
        break;
    case DB_EV_OUTPUTCHANGED:
        g_idle_add (outputchanged_cb, NULL);
        break;
    case DB_EV_ACTIONSCHANGED:
        g_idle_add (add_mainmenu_actions_cb, NULL);
        break;
    case DB_EV_DSPCHAINCHANGED:
        eq_refresh ();
        break;
    }
    return 0;
}

static json_t *
_convert_062_layout_to_json (const char **script) {
    const char *s = *script;

    json_t *node = NULL;
    json_t *type = NULL;
    json_t *legacy_params = NULL;
    json_t *children = NULL;
    json_t *settings = NULL;

    char t[MAX_TOKEN];
    s = gettoken (s, t);
    if (!s) {
        goto error;
    }

    type = json_string(t);

    // load widget params
    const char *params_begin = s;
    for (;;) {
        s = gettoken_ext (s, t, "={}();");
        if (!s) {
            goto error;
        }
        if (!strcmp (t, "{")) {
            break;
        }

        char *key = strdup(t);

        // match '='
        char eq[MAX_TOKEN];
        s = gettoken_ext (s, eq, "={}();");
        if (!s || strcmp (eq, "=")) {
            free(key);
            goto error;
        }
        s = gettoken_ext (s, t, "={}();");
        if (!s) {
            free(key);
            goto error;
        }

        if (settings == NULL) {
            settings = json_object();
        }

        json_object_set (settings, key, json_string(t));
        free (key);
    }

    // trim
    while (*params_begin == ' ') {
        params_begin++;
    }

    const char *params_end = s - 1;

    while (params_end > params_begin && *(params_end-1) == ' ') {
        params_end--;
    }

    if (params_end > params_begin) {
        char *params_str = malloc (params_end - params_begin + 1);
        memcpy (params_str, params_begin, params_end - params_begin);
        params_str[params_end - params_begin] = 0;
        legacy_params = json_string (params_str);
        free (params_str);
    }

    if (strcmp (t, "{")) {
        goto error;
    }
    for (;;) {
        const char *save = s;
        s = gettoken (s, t);
        if (!s) {
            goto error;
        }

        if (!strcmp (t, "}")) {
            break;
        }

        s = save;

        json_t *child = _convert_062_layout_to_json (&s);
        if (!child) {
            goto error;
        }

        if (children == NULL) {
            children = json_array();
        }

        json_array_append(children, child);
        json_decref(child);
        child = NULL;
    }

    node = json_object();
    json_object_set(node, "type", type);
    if (legacy_params != NULL) {
        json_object_set(node, "legacy_params", legacy_params);
    }
    if (settings != NULL) {
        json_object_set(node, "settings", settings);
    }
    if (children != NULL) {
        json_object_set(node, "children", children);
    }

    *script = s;

error:

    if (type != NULL) {
        json_decref(type);
        type = NULL;
    }
    if (legacy_params != NULL) {
        json_decref(legacy_params);
        legacy_params = NULL;
    }
    if (settings != NULL) {
        json_decref(settings);
        settings = NULL;
    }
    if (children != NULL) {
        json_decref(children);
        children = NULL;
    }

    return node;
}

static void
init_widget_layout (void) {
    w_init ();
    ddb_gtkui_widget_t *rootwidget = w_get_rootwidget ();
    gtk_widget_show (rootwidget->widget);
    gtk_box_pack_start (GTK_BOX(lookup_widget(mainwin, "plugins_bottom_vbox")), rootwidget->widget, TRUE, TRUE, 0);

    // load layout
    // config var name is defined in DDB_GTKUI_CONF_LAYOUT
    // gtkui.layout.major.minor.point: later versions

    static const char *gtkui_def_layout = "{\"type\":\"vbox\",\"legacy_params\":\"expand=\\\"0 1\\\" fill=\\\"1 1\\\" homogeneous=0\",\"children\":[{\"type\":\"hbox\",\"legacy_params\":\"expand=\\\"0 1 0\\\" fill=\\\"1 1 1\\\" homogeneous=0\",\"children\":[{\"type\":\"playtb\"},{\"type\":\"seekbar\"},{\"type\":\"volumebar\",\"legacy_params\":\"scale=0\"}]},{\"type\":\"tabbed_playlist\",\"legacy_params\":\"hideheaders=0\"}]}";

    json_t *layout = NULL;

    char *layout_json = NULL;

    deadbeef->conf_lock();
    const char *layout_json_conf = deadbeef->conf_get_str_fast(DDB_GTKUI_CONF_LAYOUT, NULL);
    if (layout_json_conf != NULL) {
        layout_json = strdup (layout_json_conf);
    }
    else {
        // migrate from 0.6.2 layout format
        const char *layout_062 = deadbeef->conf_get_str_fast("gtkui.layout.0.6.2", NULL);
        if (layout_062 != NULL) {
            const char *script = layout_062;
            layout = _convert_062_layout_to_json(&script);

            if (layout != NULL) {
                char * str = json_dumps(layout, JSON_COMPACT);
                deadbeef->conf_set_str(DDB_GTKUI_CONF_LAYOUT, str);
                layout_json = str;
                deadbeef->conf_save();
            }
        }
    }
    deadbeef->conf_unlock();

    if (layout_json != NULL) {
        layout = json_loads(layout_json, 0, NULL);
        free (layout_json);
        layout_json = NULL;
    }

    gboolean is_default = FALSE;

    if (layout == NULL) {
        layout = json_loads(gtkui_def_layout, 0, NULL);
        is_default = TRUE;
    }

    ddb_gtkui_widget_t *w = NULL;
    w_create_from_json (layout, &w);
    json_delete(layout);
    layout = NULL;

    if (w == NULL && !is_default) {
        layout = json_loads(gtkui_def_layout, 0, NULL);
        w_create_from_json (layout, &w);
        json_delete(layout);
        layout = NULL;
    }

    if (w == NULL) {
        abort(); // could not recover
    }
    w_append (rootwidget, w);
}

static DB_plugin_t *supereq_plugin;

gboolean
gtkui_connect_cb (void *none) {
    // equalizer
    GtkWidget *eq_mi = lookup_widget (mainwin, "view_eq");
    if (!supereq_plugin) {
        gtk_widget_hide (GTK_WIDGET (eq_mi));
    }
    else {
        if (deadbeef->conf_get_int ("gtkui.eq.visible", 0)) {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (eq_mi), TRUE);
            eq_window_show ();
        }
        else {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (eq_mi), FALSE);
        }
    }

    add_mainmenu_actions ();
    ddb_event_t *e = deadbeef->event_alloc (DB_EV_TRACKINFOCHANGED);
    deadbeef->event_send(e, 0, 0);
    return FALSE;
}

int
gtkui_add_file_info_cb (ddb_fileadd_data_t *data, void *user_data) {
    if (data->visibility == 0) {
        if (progress_is_aborted ()) {
            return -1;
        }
        deadbeef->pl_lock ();
        const char *fname = deadbeef->pl_find_meta (data->track, ":URI");
        g_idle_add (gtkui_set_progress_text_idle, (gpointer)strdup(fname)); // slowwwww
        deadbeef->pl_unlock ();
    }
    return 0;
}

void
gtkui_add_file_begin_cb (ddb_fileadd_data_t *data, void *user_data) {
    if (data->visibility == 0) {
        progress_show ();
    }
}

void
gtkui_add_file_end_cb (ddb_fileadd_data_t *data, void *user_data) {
    if (data->visibility == 0) {
        progress_hide ();
    }
}

typedef struct {
    void (*callback) (void *userdata);
    void *userdata;
} window_init_hook_t;

#define WINDOW_INIT_HOOK_MAX 10
static window_init_hook_t window_init_hooks[WINDOW_INIT_HOOK_MAX];
static int window_init_hooks_count;

static void
add_window_init_hook (void (*callback) (void *userdata), void *userdata) {
    if (window_init_hooks_count >= WINDOW_INIT_HOOK_MAX) {
        fprintf (stderr, "gtkui: add_window_init_hook can't add another hook, maximum number of hooks (%d) exceeded\n", (int)WINDOW_INIT_HOOK_MAX);
        return;
    }

    window_init_hooks[window_init_hooks_count].callback = callback;
    window_init_hooks[window_init_hooks_count].userdata = userdata;
    window_init_hooks_count++;
}


void
gtkui_toggle_log_window(void) {
    if (gtk_widget_get_visible (logwindow)) {
        gtkui_show_log_window(FALSE);
    }
    else {
        gtkui_show_log_window(TRUE);
    }
}

void
gtkui_show_log_window_internal(gboolean show) {
    show ? gtk_widget_show (logwindow) : gtk_widget_hide (logwindow);
    GtkWidget *menuitem = lookup_widget (mainwin, "view_log");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), show);

#if GTK_CHECK_VERSION(3,10,0)
#if USE_GTK_APPLICATION
    GSimpleAction *act = deadbeef_app_get_log_action (gapp);
    if (act) {
        g_simple_action_set_state (act, g_variant_new_boolean (show));
    }
#endif
#endif
}

void
gtkui_show_log_window(gboolean show) {
    if (show) {
        wingeom_restore (logwindow, "logwindow", 40, 40, 400, 200, 0);
    }
    else {
        wingeom_save(logwindow, "logwindow");
    }
    gtkui_show_log_window_internal(show);
}

gboolean
on_gtkui_log_window_delete (GtkWidget *widget, GdkEventAny *event, GtkWidget **pwindow) {
    gtkui_show_log_window(FALSE);
    return TRUE; // don't destroy window
}

static int logwindow_scroll_bottomed=1;

void
logwindow_scroll_changed (GtkAdjustment *adjustment, gpointer user_data)
{
    if (gtk_adjustment_get_value (adjustment) >=
        gtk_adjustment_get_upper (adjustment) -
        gtk_adjustment_get_page_size (adjustment) - 1e-12)
    {
        logwindow_scroll_bottomed = 1;
    } else {
        logwindow_scroll_bottomed = 0;
    }
}

GtkWidget *
gtkui_create_log_window (void) {
    GtkWidget *pwindow = create_log_window ();
    GtkWidget *scrolledwindow14=lookup_widget(pwindow, "scrolledwindow14");
    GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment ( GTK_SCROLLED_WINDOW (scrolledwindow14));
    GtkWidget *textview=lookup_widget(pwindow, "logwindow_textview");

    PangoFontDescription *descr = pango_font_description_from_string("monospace");
    gtk_widget_modify_font(textview, descr);
    pango_font_description_free(descr);

    g_signal_connect (adjustment, "value-changed", G_CALLBACK (logwindow_scroll_changed), NULL);
    g_signal_connect (pwindow, "delete_event", G_CALLBACK (on_gtkui_log_window_delete), pwindow);
    gtk_window_set_transient_for (GTK_WINDOW (pwindow), GTK_WINDOW (mainwin));
    return pwindow;
}

typedef struct {
    char *str;
    uint32_t layers;
} addtext_struct_t;

static gboolean
logwindow_addtext_cb (gpointer data) {
    addtext_struct_t *addtext = (addtext_struct_t *)data;

    GtkWidget *textview=lookup_widget(logwindow, "logwindow_textview");
    GtkTextBuffer *buffer;
    GtkTextIter iter;
    size_t len;
    len = strlen(addtext->str);
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert (buffer, &iter, addtext->str, (gint)len);
    // Make sure it ends on a newline
    if (addtext->str[len-1] != '\n') {
        gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert(buffer, &iter, "\n", 1);
    }
    if (logwindow_scroll_bottomed) {
        gtk_text_buffer_get_end_iter(buffer, &iter);
        GtkTextMark *mark = gtk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (textview), mark);
    }
    if (!w_logviewer_is_present() && addtext->layers == DDB_LOG_LAYER_DEFAULT)
        gtkui_show_log_window_internal(TRUE);
    free(addtext->str);
    free(addtext);
    return FALSE;
}

static void
logwindow_logger_callback (struct DB_plugin_s *plugin, uint32_t layers, const char *text, void *ctx) {
    addtext_struct_t *data = malloc(sizeof(addtext_struct_t));
    data->str = strdup(text);
    data->layers = layers;
    g_idle_add(logwindow_addtext_cb, (gpointer)data);
}


static gboolean
gtkui_mainwin_drag_motion              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    // Don't allow drag from within application
    if (gtk_drag_get_source_widget (drag_context)) {
        gdk_drag_status (drag_context, 0, time);
    }

    return FALSE;
}

static void
gtkui_mainwin_drag_data_received       (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    gchar *ptr=(char*)gtk_selection_data_get_data (data);
    gint len = gtk_selection_data_get_length (data);
    if (target_type == INFO_TARGET_URIS) { // uris
        // this happens when dropped from file manager
        char *mem = malloc (len+1);
        memcpy (mem, ptr, len);
        mem[len] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        gtkui_receive_fm_drop (NULL, mem, len);
    }

    gtk_drag_finish (drag_context, TRUE, FALSE, time);
}

void
gtkui_mainwin_init(void) {
    // register widget types
    w_reg_widget (_("Playlist with tabs"), DDB_WF_SINGLE_INSTANCE, w_tabbed_playlist_create, "tabbed_playlist", NULL);
    w_reg_widget (_("Playlist"), DDB_WF_SINGLE_INSTANCE, w_playlist_create, "playlist", NULL);
    w_reg_widget (NULL, 0, w_box_create, "box", NULL);
    w_reg_widget (NULL, 0, w_dummy_create, "dummy", NULL);
    w_reg_widget (_("Splitter (top and bottom)"), 0, w_vsplitter_create, "vsplitter", NULL);
    w_reg_widget (_("Splitter (left and right)"), 0, w_hsplitter_create, "hsplitter", NULL);
    w_reg_widget (NULL, 0, w_placeholder_create, "placeholder", NULL);
    w_reg_widget (_("Tabs"), DDB_WF_SUPPORTS_EXTENDED_API, w_tabs_create, "tabs", NULL);
    w_reg_widget (_("Playlist tabs"), 0, w_tabstrip_create, "tabstrip", NULL);
    w_reg_widget (_("Selection properties"), DDB_WF_SUPPORTS_EXTENDED_API, w_selproperties_create, "selproperties", NULL);
    w_reg_widget (_("Album art display"), DDB_WF_SUPPORTS_EXTENDED_API, w_albumart_create, "coverart", NULL);
    w_reg_widget (_("Oscilloscope"), DDB_WF_SUPPORTS_EXTENDED_API, w_scope_create, "scope", NULL);
    w_reg_widget (_("Spectrum"), DDB_WF_SUPPORTS_EXTENDED_API, w_spectrum_create, "spectrum", NULL);
    w_reg_widget (_("HBox"), 0, w_hbox_create, "hbox", NULL);
    w_reg_widget (_("VBox"), 0, w_vbox_create, "vbox", NULL);
    w_reg_widget (_("Button"), 0, w_button_create, "button", NULL);
    w_reg_widget (_("Seekbar"), 0, w_seekbar_create, "seekbar", NULL);
    w_reg_widget (_("Playback controls"), 0, w_playtb_create, "playtb", NULL);
    w_reg_widget (_("Volume bar"), DDB_WF_SUPPORTS_EXTENDED_API, w_volumebar_create, "volumebar", NULL);
    w_reg_widget (_("Chiptune voices"), 0, w_ctvoices_create, "ctvoices", NULL);
    w_reg_widget (_("Log viewer"), 0, w_logviewer_create, "logviewer", NULL);
#if ENABLE_MEDIALIB
    w_reg_widget (_("Media library viewer"), 0, w_medialib_viewer_create, "medialibviewer", NULL);
#endif

    mainwin = create_mainwin ();

#if GTK_CHECK_VERSION(3,10,0) && USE_GTK_APPLICATION
     // This must be called before window is shown
     gtk_application_add_window ( GTK_APPLICATION (gapp), GTK_WINDOW (mainwin));
#endif

   wingeom_restore (mainwin, "mainwin", 40, 40, 500, 300, 0); 
    
#if GTK_CHECK_VERSION(3,0,0)
    init_widget_layout ();
    gtk_widget_set_events (GTK_WIDGET (mainwin), gtk_widget_get_events (GTK_WIDGET (mainwin)) | GDK_SCROLL_MASK);

    if (deadbeef->conf_get_int ("gtkui.start_hidden", 0)) {
        g_idle_add (mainwin_hide_cb, NULL);
    }
#elif GTK_CHECK_VERSION(2,16,0)
    if (!deadbeef->conf_get_int ("gtkui.start_hidden", 0)) {
        init_widget_layout ();
        gtk_widget_show (mainwin);
    }
#endif 

    logwindow = gtkui_create_log_window();
    deadbeef->log_viewer_register (logwindow_logger_callback, logwindow);

    // initialize default hotkey mapping
    if (!deadbeef->conf_get_int ("hotkeys_created", 0)) {
        // check if any hotkeys were created manually (e.g. beta versions of 0.6)
        if (!deadbeef->conf_find ("hotkey.key", NULL)) {
            gtkui_set_default_hotkeys ();
            gtkui_import_0_5_global_hotkeys ();
            DB_plugin_t *hkplug = deadbeef->plug_get_for_id ("hotkeys");
            if (hkplug) {
                ((DB_hotkeys_plugin_t *)hkplug)->reset ();
            }
        }
        deadbeef->conf_set_int ("hotkeys_created", 1);
        deadbeef->conf_save ();
    }

    pl_common_init();

    // setup drag-drop target
    gtk_drag_dest_set (mainwin, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
    gtk_drag_dest_add_uri_targets (mainwin);
    g_signal_connect ((gpointer) mainwin, "drag_data_received",
            G_CALLBACK (gtkui_mainwin_drag_data_received),
            NULL);
    g_signal_connect ((gpointer) mainwin, "drag_motion",
            G_CALLBACK (gtkui_mainwin_drag_motion),
            NULL);

    GtkIconTheme *theme = gtk_icon_theme_get_default();
    if (gtk_icon_theme_has_icon(theme, "deadbeef")) {
        // NOTE: according to valgrind, this leaks memory - seems to be a GTK bug
        gtk_window_set_icon_name (GTK_WINDOW (mainwin), "deadbeef");
    }
    else {
        // try loading icon from $prefix/deadbeef.png (for static build)
        char iconpath[1024];
        snprintf (iconpath, sizeof (iconpath), "%s/deadbeef.png", deadbeef->get_system_dir(DDB_SYS_DIR_PREFIX));
        struct stat st = {0};
        if (stat (iconpath, &st)) {
            snprintf (iconpath, sizeof (iconpath), "%s/deadbeef.png", deadbeef->get_system_dir(DDB_SYS_DIR_PLUGIN_RESOURCES));
        }
        if (!stat (iconpath, &st)) {
            gtk_window_set_icon_from_file (GTK_WINDOW (mainwin), iconpath, NULL);
        }
    }

    gtkui_on_configchanged (NULL);

    // visibility of statusbar and headers
    GtkWidget *sb_mi = lookup_widget (mainwin, "view_status_bar");
    GtkWidget *sb = lookup_widget (mainwin, "statusbar");
    if (deadbeef->conf_get_int ("gtkui.statusbar.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), FALSE);
        gtk_widget_hide (sb);
    }

    GtkWidget *menu = lookup_widget (mainwin, "menubar");
    if (deadbeef->conf_get_int ("gtkui.show_menu", 1)) {
        gtk_widget_show (menu);
    }
    else {
        gtk_widget_hide (menu);
    }

    search_playlist_init (mainwin);
    progress_init ();

#ifdef __APPLE__
#if 0
    GtkWidget *menubar = lookup_widget (mainwin, "menubar");
    gtk_widget_hide (menubar);
    GtkosxApplication *theApp = g_object_new(GTKOSX_TYPE_APPLICATION, NULL);
    gtkosx_application_set_menu_bar(theApp, GTK_MENU_SHELL(menubar));
#endif
#endif

    for (int i = 0; i < window_init_hooks_count; i++) {
        window_init_hooks[i].callback (window_init_hooks[i].userdata);
    }
    gtkui_set_titlebar (NULL);

    fileadded_listener_id = deadbeef->listen_file_added (gtkui_add_file_info_cb, NULL);
    fileadd_beginend_listener_id = deadbeef->listen_file_add_beginend (gtkui_add_file_begin_cb, gtkui_add_file_end_cb, NULL);

    supereq_plugin = deadbeef->plug_get_for_id ("supereq");

    gtkui_connect_cb (NULL);

    gtkui_accept_messages = 1;
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
gtkui_mainwin_free(void) {
    deadbeef->unlisten_file_added (fileadded_listener_id);
    deadbeef->unlisten_file_add_beginend (fileadd_beginend_listener_id);

    covermanager_terminate(covermanager_shared());
// FIXME: can't do this, since artwork holds non-refcounted references to it
//    covermanager_shared_free ();

    w_free ();

    if (refresh_timeout) {
        g_source_remove (refresh_timeout);
        refresh_timeout = 0;
    }

    if (set_title_timeout_id) {
        g_source_remove (set_title_timeout_id);
        set_title_timeout_id = 0;
    }

    clipboard_free_current ();
    eq_window_destroy ();
    trkproperties_destroy ();
    progress_destroy ();
    gtkui_hide_status_icon ();
    pl_common_free();
    search_destroy ();
//    draw_free ();
    titlebar_tf_free ();
    gtkui_free_pltmenu ();
    plmenu_free();

    if (logwindow) {
        deadbeef->log_viewer_unregister (logwindow_logger_callback, logwindow);
        gtk_widget_destroy (logwindow);
        logwindow = NULL;
    }
    if (mainwin) {
        gtk_widget_destroy (mainwin);
        mainwin = NULL;
    }

}

gboolean
gtkui_set_progress_text_idle (gpointer data) {
    char *text = (char *)data;
    if (text) {
        progress_settext (text);
        free (text);
    }
    return FALSE;
}

void
on_gtkui_info_window_delete (GtkWidget *widget, GtkTextDirection previous_direction, GtkWidget **pwindow) {
    *pwindow = NULL;
    gtk_widget_hide (widget);
    gtk_widget_destroy (widget);
}

void
gtkui_show_info_window (const char *fname, const char *title, GtkWidget **pwindow) {
    if (*pwindow) {
        return;
    }
    GtkWidget *widget = *pwindow = create_helpwindow ();
    g_object_set_data (G_OBJECT (widget), "pointer", pwindow);
    g_signal_connect (widget, "delete_event", G_CALLBACK (on_gtkui_info_window_delete), pwindow);
    gtk_window_set_title (GTK_WINDOW (widget), title);
    gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (mainwin));
    GtkWidget *txt = lookup_widget (widget, "helptext");

    PangoFontDescription *descr = pango_font_description_from_string("monospace");
    gtk_widget_modify_font(txt, descr);
    pango_font_description_free(descr);

    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

    FILE *fp = fopen (fname, "rb");
    if (fp) {
        fseek (fp, 0, SEEK_END);
        size_t s = ftell (fp);
        rewind (fp);
        char buf[s+1];
        if (fread (buf, 1, s, fp) != s) {
            fprintf (stderr, "error reading help file contents\n");
            const char *error = _("Failed while reading help file");
            gtk_text_buffer_set_text (buffer, error, (int)strlen (error));
        }
        else {
            buf[s] = 0;
            gtk_text_buffer_set_text (buffer, buf, (gint)s);
        }
        fclose (fp);
    }
    else {
        const char *error = _("Failed to load help file");
        gtk_text_buffer_set_text (buffer, error, (gint)strlen (error));
    }
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (txt), buffer);
    g_object_unref (buffer);
    gtk_widget_show (widget);
}

typedef enum {
    SHUTDOWN_INHIBIT,
    SHUTDOWN_NORMAL,
    SHUTDOWN_FORCE
} shutdown_type;

static shutdown_type
_should_allow_shutdown (void) {
    if (deadbeef->have_background_jobs ()) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("The player is currently running background tasks. If you quit now, the tasks will be cancelled or interrupted. This may result in data loss."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Do you still want to quit?"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return SHUTDOWN_INHIBIT;
        }
        else {
            return SHUTDOWN_FORCE;
        }
    }
    else {
        return SHUTDOWN_NORMAL;
    }
}

static void
_delete_running_marker(void) {
    char crash_marker[PATH_MAX];
    snprintf (crash_marker, sizeof (crash_marker), "%s/running", deadbeef->get_system_dir(DDB_SYS_DIR_CONFIG));
    unlink(crash_marker);
}

gboolean
gtkui_quit_cb (void *ctx) {
    _quitting_normally = 1;
    w_save ();

    shutdown_type type = _should_allow_shutdown();

    if (type == 2) {
        _delete_running_marker();
        exit(0);
    }
    else if (type == 1) {
        progress_abort ();
        deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
    }
    else {
        _quitting_normally = 0;
    }
    return FALSE;
}

void
gtkui_quit (void) {
    gdk_threads_add_idle (gtkui_quit_cb, NULL);
}

static void
import_legacy_tf (const char *key_from, const char *key_to) {
    deadbeef->conf_lock ();
    if (!deadbeef->conf_get_str_fast (key_to, NULL)
            && deadbeef->conf_get_str_fast (key_from, NULL)) {
        char old[200], new[200];
        deadbeef->conf_get_str (key_from, "", old, sizeof (old));
        deadbeef->tf_import_legacy (old, new, sizeof (new));
        deadbeef->conf_set_str (key_to, new);
        deadbeef->conf_save ();
    }
    deadbeef->conf_unlock ();
}

#if GTK_CHECK_VERSION(3,10,0) && USE_GTK_APPLICATION
static void
gapplication_shutdown_handler (GApplication *app, gpointer user_data) {
    if (_quitting_normally) {
        return;
    }
    printf ("gapplication_shutdown_handler\n");
    shutdown_type type = _should_allow_shutdown();
    if (type != SHUTDOWN_INHIBIT) {
        _delete_running_marker();
    }
}
#endif

static int
gtkui_start (void) {
    fprintf (stderr, "gtkui plugin compiled for gtk version: %d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);

    import_legacy_tf ("gtkui.titlebar_playing", "gtkui.titlebar_playing_tf");
    import_legacy_tf ("gtkui.titlebar_stopped", "gtkui.titlebar_stopped_tf");

    import_legacy_tf ("playlist.group_by", "gtkui.playlist.group_by_tf");

    int argc = 1;
    const char **argv = alloca (sizeof (char *) * argc);
    argv[0] = "deadbeef";

    gtk_disable_setlocale ();
    add_pixmap_directory (deadbeef->get_system_dir(DDB_SYS_DIR_PIXMAP));

#if GTK_CHECK_VERSION(3,10,0) && USE_GTK_APPLICATION
    gapp = deadbeef_app_new ();
    GValue val = G_VALUE_INIT;
    g_value_init (&val, G_TYPE_BOOLEAN);
    g_value_set_boolean (&val, TRUE);
    g_object_set_property (G_OBJECT(gapp), "register-session", &val);
    g_signal_connect (gapp, "window-removed", G_CALLBACK (gapplication_shutdown_handler), NULL);
    g_application_run ( G_APPLICATION (gapp), argc, (char**)argv);
    g_object_unref (gapp);
#else
    gtk_init (&argc, (char ***)&argv);

    gtkui_mainwin_init ();
    gtk_main ();
    gtkui_mainwin_free ();
#endif

    return 0;
}

static gboolean
quit_gtk_cb (gpointer nothing) {
    extern int trkproperties_modified;

    gtkui_mainwin_free();

    supereq_plugin = NULL;
    trkproperties_modified = 0;
    trkproperties_destroy ();
    search_destroy ();
    gtkui_medialib_free();
#if GTK_CHECK_VERSION(3,10,0) && USE_GTK_APPLICATION
    g_application_quit (G_APPLICATION (gapp));
#else
    gtk_main_quit ();
#endif
    trace ("gtkui_stop completed\n");
    return FALSE;
}

static int
gtkui_stop (void) {
    trace ("quitting gtk\n");
    g_idle_add (quit_gtk_cb, NULL);
    return 0;
}

GtkWidget *
gtkui_get_mainwin (void) {
    return mainwin;
}

static DB_plugin_action_t action_rg_remove_info = {
    .title = "ReplayGain/Remove ReplayGain Information",
    .name = "rg_remove_info",
    .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU,
    .callback2 = action_rg_remove_info_handler,
    .next = NULL
};

static DB_plugin_action_t action_rg_scan_selection_as_albums = {
    .title = "ReplayGain/Scan Selection As Albums (By Tags)",
    .name = "rg_scan_selection_as_albums",
    .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU,
    .callback2 = action_rg_scan_selection_as_albums_handler,
    .next = &action_rg_remove_info
};

static DB_plugin_action_t action_rg_scan_selection_as_album = {
    .title = "ReplayGain/Scan Selection As Single Album",
    .name = "rg_scan_selection_as_album",
    .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU,
    .callback2 = action_rg_scan_selection_as_album_handler,
    .next = &action_rg_scan_selection_as_albums
};

static DB_plugin_action_t action_rg_scan_per_file = {
    .title = "ReplayGain/Scan Per-file Track Gain",
    .name = "rg_scan_perfile_track_gain",
    .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU,
    .callback2 = action_rg_scan_per_file_handler,
    .next = &action_rg_scan_selection_as_album
};

static DB_plugin_action_t action_scan_all_tracks_without_rg = {
    .title = "ReplayGain/Scan Per-file Track Gain If Not Scanned",
    .name = "scan_all_tracks_without_Rg",
    .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU,
    .callback2 = action_scan_all_tracks_without_rg_handler ,
    .next = &action_rg_scan_per_file
};

static DB_plugin_action_t action_deselect_all = {
    .title = "Edit/Deselect All",
    .name = "deselect_all",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_deselect_all_handler,
    .next = &action_scan_all_tracks_without_rg
};

static DB_plugin_action_t action_select_all = {
    .title = "Edit/Select All",
    .name = "select_all",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_select_all_handler,
    .next = &action_deselect_all
};

static DB_plugin_action_t action_quit = {
    .title = "Quit",
    .name = "quit",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_quit_handler,
    .next = &action_select_all
};

static DB_plugin_action_t action_delete_from_disk = {
    .title = "Delete from Disk",
    .name = "delete_from_disk",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback2 = action_delete_from_disk_handler,
    .next = &action_quit
};

static DB_plugin_action_t action_add_location = {
    .title = "File/Add Location",
    .name = "add_location",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_add_location_handler,
    .next = &action_delete_from_disk
};

static DB_plugin_action_t action_add_folders = {
    .title = "File/Add Folder(s)",
    .name = "add_folders",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_add_folders_handler,
    .next = &action_add_location
};

static DB_plugin_action_t action_add_files = {
    .title = "File/Add File(s)",
    .name = "add_files",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_add_files_handler,
    .next = &action_add_folders
};

static DB_plugin_action_t action_open_files = {
    .title = "File/Open File(s)",
    .name = "open_files",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_open_files_handler,
    .next = &action_add_files
};


static DB_plugin_action_t action_track_properties = {
    .title = "Track Properties",
    .name = "track_properties",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback2 = action_show_track_properties_handler,
    .next = &action_open_files
};

static DB_plugin_action_t action_show_help = {
    .title = "Help/Show Help Page",
    .name = "help",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_show_help_handler,
    .next = &action_track_properties
};

static DB_plugin_action_t action_playback_loop_cycle = {
    .title = "Playback/Cycle Repeat Mode",
    .name = "loop_cycle",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_loop_cycle_handler,
    .next = &action_show_help
};

static DB_plugin_action_t action_playback_loop_off = {
    .title = "Playback/Repeat - Off",
    .name = "loop_off",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_loop_off_handler,
    .next = &action_playback_loop_cycle
};

static DB_plugin_action_t action_playback_loop_single = {
    .title = "Playback/Repeat - Single Track",
    .name = "loop_track",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_loop_single_handler,
    .next = &action_playback_loop_off
};

static DB_plugin_action_t action_playback_loop_all = {
    .title = "Playback/Repeat - All",
    .name = "loop_all",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_loop_all_handler,
    .next = &action_playback_loop_single
};

static DB_plugin_action_t action_playback_order_cycle = {
    .title = "Playback/Cycle Shuffle Mode",
    .name = "order_cycle",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_cycle_handler,
    .next = &action_playback_loop_all
};

static DB_plugin_action_t action_playback_order_random = {
    .title = "Playback/Shuffle - Random Tracks",
    .name = "order_random",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_random_handler,
    .next = &action_playback_order_cycle
};

static DB_plugin_action_t action_playback_order_shuffle_albums = {
    .title = "Playback/Shuffle - Albums",
    .name = "order_shuffle_albums",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_shuffle_albums_handler,
    .next = &action_playback_order_random
};

static DB_plugin_action_t action_playback_order_shuffle = {
    .title = "Playback/Shuffle - Tracks",
    .name = "order_shuffle",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_shuffle_handler,
    .next = &action_playback_order_shuffle_albums
};

static DB_plugin_action_t action_playback_order_linear = {
    .title = "Playback/Shuffle - Off",
    .name = "order_linear",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_linear_handler,
    .next = &action_playback_order_shuffle
};


static DB_plugin_action_t action_cursor_follows_playback = {
    .title = "Playback/Toggle Cursor Follows Playback",
    .name = "toggle_cursor_follows_playback",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_cursor_follows_playback_handler,
    .next = &action_playback_order_linear
};


static DB_plugin_action_t action_scroll_follows_playback = {
    .title = "Playback/Toggle Scroll Follows Playback",
    .name = "toggle_scroll_follows_playback",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_scroll_follows_playback_handler,
    .next = &action_cursor_follows_playback
};

static DB_plugin_action_t action_toggle_menu = {
    .title = "View/Show\\/Hide menu",
    .name = "toggle_menu",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_menu_handler,
    .next = &action_scroll_follows_playback
};

static DB_plugin_action_t action_toggle_statusbar = {
    .title = "View/Show\\/Hide statusbar",
    .name = "toggle_statusbar",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_statusbar_handler,
    .next = &action_toggle_menu
};

static DB_plugin_action_t action_toggle_designmode = {
    .title = "Edit/Toggle Design Mode",
    .name = "toggle_design_mode",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_designmode_handler,
    .next = &action_toggle_statusbar
};

static DB_plugin_action_t action_preferences = {
    .title = "Edit/Preferences",
    .name = "preferences",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_preferences_handler,
    .next = &action_toggle_designmode
};

static DB_plugin_action_t action_sort_custom = {
    .title = "Edit/Sort Custom",
    .name = "sort_custom",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_sort_custom_handler,
    .next = &action_preferences
};

static DB_plugin_action_t action_crop_selected = {
    .title = "Edit/Crop Selected",
    .name = "crop_selected",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_crop_selected_handler,
    .next = &action_sort_custom
};

static DB_plugin_action_t action_remove_from_playlist = {
    .title = "Edit/Remove Track(s) from Playlist",
    .name = "remove_from_playlist",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST,
    .callback2 = action_remove_from_playlist_handler,
    .next = &action_crop_selected
};

static DB_plugin_action_t action_save_playlist = {
    .title = "File/Save Playlist",
    .name = "save_playlist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_save_playlist_handler,
    .next = &action_remove_from_playlist
};

static DB_plugin_action_t action_load_playlist = {
    .title = "File/Load Playlist",
    .name = "load_playlist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_load_playlist_handler,
    .next = &action_save_playlist
};

static DB_plugin_action_t action_remove_current_playlist = {
    .title = "File/Remove Current Playlist",
    .name = "remove_current_playlist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_remove_current_playlist_handler,
    .next = &action_load_playlist
};


static DB_plugin_action_t action_new_playlist = {
    .title = "File/New Playlist",
    .name = "new_playlist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_new_playlist_handler,
    .next = &action_remove_current_playlist
};

static DB_plugin_action_t action_rename_current_playlist = {
    .title = "File/Rename Current Playlist",
    .name = "rename_current_playlist",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_rename_current_playlist_handler,
    .next = &action_new_playlist
};

static DB_plugin_action_t action_toggle_eq = {
    .title = "View/Show\\/Hide Equalizer",
    .name = "toggle_eq",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_eq_handler,
    .next = &action_rename_current_playlist
};

static DB_plugin_action_t action_hide_eq = {
    .title = "View/Hide Equalizer",
    .name = "hide_eq",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_hide_eq_handler,
    .next = &action_toggle_eq
};

static DB_plugin_action_t action_show_eq = {
    .title = "View/Show Equalizer",
    .name = "show_eq",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_show_eq_handler,
    .next = &action_hide_eq
};

static DB_plugin_action_t action_toggle_mainwin = {
    .title = "View/Show\\/Hide Player Window",
    .name = "toggle_player_window",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_mainwin_handler,
    .next = &action_show_eq
};

static DB_plugin_action_t action_hide_mainwin = {
    .title = "View/Hide Player Window",
    .name = "hide_player_window",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_hide_mainwin_handler,
    .next = &action_toggle_mainwin
};

static DB_plugin_action_t action_show_mainwin = {
    .title = "View/Show Player Window",
    .name = "show_player_window",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_show_mainwin_handler,
    .next = &action_hide_mainwin
};

static DB_plugin_action_t action_find = {
    .title = "Edit/Find",
    .name = "find",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_find_handler,
    .next = &action_show_mainwin
};

static DB_plugin_action_t action_view_log = {
    .title = "View/Show\\/Hide Log window",
    .name = "toggle_log_window",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_logwindow_handler,
    .next = &action_find
};

static DB_plugin_action_t *
gtkui_get_actions (DB_playItem_t *it)
{
    return &action_view_log;
}

#if !GTK_CHECK_VERSION(3,0,0)
DB_plugin_t *
ddb_gui_GTK2_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
#else
DB_plugin_t *
ddb_gui_GTK3_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
#endif

static GtkWidget *
_create_pltmenu (int plt_index) {
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (plt_index);
    GtkWidget *menu = NULL;
    if (plt != NULL) {
        menu = gtkui_create_pltmenu(plt);
        deadbeef->plt_unref (plt);
    }
    return menu;
}

static const char settings_dlg[] =
    "property \"Ask confirmation to delete files from disk\" checkbox gtkui.delete_files_ask 1;\n"
    "property \"Status icon settings:\" label l;\n"
    "property \"Status icon volume control sensitivity\" entry gtkui.tray_volume_sensitivity 1;\n"
    "property \"Custom status icon\" entry gtkui.custom_tray_icon \"" TRAY_ICON "\" ;\n"
    "property \"Misc:\" label l;\n"
    "property \"Add separators between plugin context menu items\" checkbox gtkui.action_separators 0;\n"
    "property \"Use unicode chars instead of images for track state\" checkbox gtkui.unicode_playstate 0;\n"
    "property \"Disable seekbar overlay text\" checkbox gtkui.disable_seekbar_overlay 0;\n"
;

#pragma mark - Obsolete cover art API stubs

GdkPixbuf *
_get_cover_art_pixbuf (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data) {
    return NULL;
}

static GdkPixbuf *
_cover_get_default_pixbuf (void) {
    return NULL;
}

static GdkPixbuf *
_get_cover_art_primary (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data) {
    return NULL;
}

GdkPixbuf *
_get_cover_art_thumb (const char *uri, const char *artist, const char *album, int size, void (*callback)(void *user_data), void *user_data) {
    return NULL;
}

#pragma mark -

// define plugin interface
ddb_gtkui_t plugin = {
    .gui.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .gui.plugin.api_vminor = DB_API_VERSION_MINOR,
    .gui.plugin.version_major = DDB_GTKUI_API_VERSION_MAJOR,
    .gui.plugin.version_minor = DDB_GTKUI_API_VERSION_MINOR,
    .gui.plugin.type = DB_PLUGIN_GUI,
    .gui.plugin.id = DDB_GTKUI_PLUGIN_ID,
    .gui.plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
#if GTK_CHECK_VERSION(3,0,0)
    .gui.plugin.name = "GTK3 user interface",
    .gui.plugin.descr = "User interface using GTK+ 3.x",
#else
    .gui.plugin.name = "GTK2 user interface",
    .gui.plugin.descr = "User interface using GTK+ 2.x",
#endif
    .gui.plugin.copyright =
        "GTK+ user interface for DeaDBeeF Player.\n"
        "Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
        "\n"
        "\n"
        "GTK - The GIMP Toolkit\n"
        "Copyright (C) GTK Developers\n"
        "\n"
        "This library is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU Lesser General Public\n"
        "License as published by the Free Software Foundation; either\n"
        "version 2 of the License, or (at your option) any later version.\n"
        "\n"
        "This library is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
        "Lesser General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU Lesser General Public\n"
        "License along with this library. If not, see <http://www.gnu.org/licenses/>.\n"
    ,
    .gui.plugin.website = "http://deadbeef.sf.net",
    .gui.plugin.start = gtkui_start,
    .gui.plugin.stop = gtkui_stop,
    .gui.plugin.configdialog = settings_dlg,
    .gui.plugin.message = gtkui_message,
    .gui.run_dialog = gtkui_run_dialog_root,
    .gui.plugin.get_actions = gtkui_get_actions,
    .get_mainwin = gtkui_get_mainwin,
    .w_reg_widget = w_reg_widget,
    .w_unreg_widget = w_unreg_widget,
    .w_override_signals = w_override_signals,
    .w_is_registered = w_is_registered,
    .w_get_rootwidget = w_get_rootwidget,
    .w_set_design_mode = w_set_design_mode,
    .w_get_design_mode = w_get_design_mode,
    .w_create = w_create,
    .w_destroy = w_destroy,
    .w_append = w_append,
    .w_replace = w_replace,
    .w_remove = w_remove,
    .create_pltmenu = _create_pltmenu,
    .get_cover_art_pixbuf = _get_cover_art_pixbuf,
    .get_cover_art_primary = _get_cover_art_primary,
    .get_cover_art_thumb = _get_cover_art_thumb,
    .cover_get_default_pixbuf = _cover_get_default_pixbuf,
    .add_window_init_hook = add_window_init_hook,
    .mainwin_toggle_visible = mainwin_toggle_visible,
    .show_traymenu = show_traymenu,
    .override_builtin_statusicon = override_builtin_statusicon,
    .copy_selection = clipboard_copy_selection,
    .cut_selection = clipboard_cut_selection,
    .paste_selection = clipboard_paste_selection,
    .w_get_type_flags = w_get_type_flags,
};
