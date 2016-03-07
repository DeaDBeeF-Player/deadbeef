/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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


#include "../../deadbeef.h"
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include "../../gettext.h"
#include "gtkui.h"
#include "ddblistview.h"
#include "search.h"
#include "progress.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include "../libparser/parser.h"
#include "drawing.h"
#include "trkproperties.h"
#include "../artwork/artwork.h"
#include "coverart.h"
#include "plcommon.h"
#include "ddbtabstrip.h"
#include "eq.h"
#include "actions.h"
#include "pluginconf.h"
#include "gtkui_api.h"
#include "wingeom.h"
#include "widgets.h"
#ifndef __APPLE__
#include "X11/Xlib.h"
#else
#include "retina.h"
#endif
#include "actionhandlers.h"
#include "hotkeys.h"
#include "../hotkeys/hotkeys.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static ddb_gtkui_t plugin;
DB_functions_t *deadbeef;

// main widgets
GtkWidget *mainwin;
GtkWidget *searchwin;
GtkStatusIcon *trayicon;
GtkWidget *traymenu;

static int gtkui_accept_messages = 0;

static gint refresh_timeout = 0;

int fileadded_listener_id;
int fileadd_beginend_listener_id;
// overriden API methods
#if 0
int (*gtkui_original_plt_add_dir) (ddb_playlist_t *plt, const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
int (*gtkui_original_plt_add_file) (ddb_playlist_t *plt, const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
int (*gtkui_original_pl_add_files_begin) (ddb_playlist_t *plt);
void (*gtkui_original_pl_add_files_end) (void);
#endif

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

#ifdef __APPLE__
int gtkui_is_retina = 0;
#endif

int gtkui_unicode_playstate = 0;
int gtkui_disable_seekbar_overlay = 0;

#define TRAY_ICON "deadbeef_tray_icon"

void
gtkpl_free (DdbListview *pl) {
#if 0
    if (colhdr_anim.timeline) {
        timeline_free (colhdr_anim.timeline, 1);
        colhdr_anim.timeline = 0;
    }
#endif
}

struct fromto_t {
    DB_playItem_t *from;
    DB_playItem_t *to;
};

static gboolean
update_win_title_idle (gpointer data);

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];
static float last_songpos = -1;
static char sbitrate[20] = "";
static struct timeval last_br_update;

static gboolean
update_songinfo (gpointer ctx) {
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    DB_output_t *output = deadbeef->get_output ();
    char sbtext_new[512] = "-";

    float pl_totaltime = deadbeef->pl_get_totaltime ();
    int daystotal = (int)pl_totaltime / (3600*24);
    int hourtotal = ((int)pl_totaltime / 3600) % 24;
    int mintotal = ((int)pl_totaltime/60) % 60;
    int sectotal = ((int)pl_totaltime) % 60;

    char totaltime_str[512] = "";
    if (daystotal == 0) {
        snprintf (totaltime_str, sizeof (totaltime_str), "%d:%02d:%02d", hourtotal, mintotal, sectotal);
    }
    else if (daystotal == 1) {
        snprintf (totaltime_str, sizeof (totaltime_str), _("1 day %d:%02d:%02d"), hourtotal, mintotal, sectotal);
    }
    else {
        snprintf (totaltime_str, sizeof (totaltime_str), _("%d days %d:%02d:%02d"), daystotal, hourtotal, mintotal, sectotal);
    }

    DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
    DB_fileinfo_t *c = deadbeef->streamer_get_current_fileinfo (); // FIXME: might crash streamer

    float duration = track ? deadbeef->pl_get_item_duration (track) : -1;

    if (!output || (output->state () == OUTPUT_STATE_STOPPED || !track || !c)) {
        snprintf (sbtext_new, sizeof (sbtext_new), _("Stopped | %d tracks | %s total playtime"), deadbeef->pl_getcount (PL_MAIN), totaltime_str);
    }
    else {
        float playpos = deadbeef->streamer_get_playpos ();
        int minpos = playpos / 60;
        int secpos = playpos - minpos * 60;
        int mindur = duration / 60;
        int secdur = duration - mindur * 60;

        const char *mode;
        char temp[20];
        if (c->fmt.channels <= 2) {
            mode = c->fmt.channels == 1 ? _("Mono") : _("Stereo");
        }
        else {
            snprintf (temp, sizeof (temp), "%dch Multichannel", c->fmt.channels);
            mode = temp;
        }
        int samplerate = c->fmt.samplerate;
        int bitspersample = c->fmt.bps;
        //        codec_unlock ();

        char t[100];
        if (duration >= 0) {
            snprintf (t, sizeof (t), "%d:%02d", mindur, secdur);
        }
        else {
            strcpy (t, "-:--");
        }

        struct timeval tm;
        gettimeofday (&tm, NULL);
        if (tm.tv_sec - last_br_update.tv_sec + (tm.tv_usec - last_br_update.tv_usec) / 1000000.0 >= 0.3) {
            memcpy (&last_br_update, &tm, sizeof (tm));
            int bitrate = deadbeef->streamer_get_apx_bitrate ();
            if (bitrate > 0) {
                snprintf (sbitrate, sizeof (sbitrate), _("| %4d kbps "), bitrate);
            }
            else {
                sbitrate[0] = 0;
            }
        }
        const char *spaused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED ? _("Paused | ") : "";
        char filetype[20];
        if (!deadbeef->pl_get_meta (track, ":FILETYPE", filetype, sizeof (filetype))) {
            strcpy (filetype, "-");
        }
        snprintf (sbtext_new, sizeof (sbtext_new), _("%s%s %s| %dHz | %d bit | %s | %d:%02d / %s | %d tracks | %s total playtime"), spaused, filetype, sbitrate, samplerate, bitspersample, mode, minpos, secpos, t, deadbeef->pl_getcount (PL_MAIN), totaltime_str);
    }

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

void
mainwin_toggle_visible (void) {
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (gtk_widget_get_visible (mainwin) && !iconified) {
        gtk_widget_hide (mainwin);
    }
    else {
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
    gtk_widget_show (mainwin);
    gtk_window_present (GTK_WINDOW (mainwin));
    return FALSE;
}

void
redraw_queued_tracks (DdbListview *pl) {
    DB_playItem_t *it;
    int idx = 0;
    deadbeef->pl_lock ();
    for (it = deadbeef->pl_get_first (PL_MAIN); it; idx++) {
        if (deadbeef->pl_playqueue_test (it) != -1) {
            ddb_listview_draw_row (pl, idx, (DdbListviewIter)it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
}

gboolean
redraw_queued_tracks_cb (gpointer plt) {
    DdbListview *list = plt;
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    redraw_queued_tracks (list);
    return FALSE;
}

void
gtkpl_songchanged_wrapper (DB_playItem_t *from, DB_playItem_t *to) {
    struct fromto_t *ft = malloc (sizeof (struct fromto_t));
    ft->from = from;
    ft->to = to;
    if (from) {
        deadbeef->pl_item_ref (from);
    }
    if (to) {
        deadbeef->pl_item_ref (to);
    }
    g_idle_add (update_win_title_idle, ft);
    if (searchwin && gtk_widget_get_window (searchwin)) {
        int iconified = gdk_window_get_state(gtk_widget_get_window (searchwin)) & GDK_WINDOW_STATE_ICONIFIED;
        if (gtk_widget_get_visible (searchwin) && !iconified) {
            g_idle_add (redraw_queued_tracks_cb, DDB_LISTVIEW (lookup_widget (searchwin, "searchlist")));
        }
    }
}

const char *gtkui_default_titlebar_playing = "%artist% - %title% - DeaDBeeF-%_deadbeef_version%";
const char *gtkui_default_titlebar_stopped = "DeaDBeeF-%_deadbeef_version%";

static char *titlebar_playing_bc;
static char *titlebar_stopped_bc;

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
}

void
gtkui_titlebar_tf_init (void) {
    titlebar_tf_free ();

    char fmt[500];
    deadbeef->conf_get_str ("gtkui.titlebar_playing_tf", gtkui_default_titlebar_playing, fmt, sizeof (fmt));
    titlebar_playing_bc = deadbeef->tf_compile (fmt);
    deadbeef->conf_get_str ("gtkui.titlebar_stopped_tf", gtkui_default_titlebar_stopped, fmt, sizeof (fmt));
    titlebar_stopped_bc = deadbeef->tf_compile (fmt);
}

void
gtkui_set_titlebar (DB_playItem_t *it) {
    if (!it) {
        it = deadbeef->streamer_get_playing_track ();
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
}

static void
trackinfochanged_wrapper (DdbListview *playlist, DB_playItem_t *track, int iter) {
    if (track) {
        int idx = deadbeef->pl_get_idx_of_iter (track, iter);
        if (idx != -1) {
            ddb_listview_draw_row (playlist, idx, (DdbListviewIter)track);
        }
    }
}

void
gtkui_trackinfochanged (DB_playItem_t *track) {
    if (searchwin && gtk_widget_get_visible (searchwin)) {
        GtkWidget *search = lookup_widget (searchwin, "searchlist");
        trackinfochanged_wrapper (DDB_LISTVIEW (search), track, PL_SEARCH);
    }

    DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
    if (track == curr) {
        gtkui_set_titlebar (track);
    }
    if (curr) {
        deadbeef->pl_item_unref (curr);
    }
}

static gboolean
trackinfochanged_cb (gpointer data) {
    gtkui_trackinfochanged (data);
    if (data) {
        deadbeef->pl_item_unref ((DB_playItem_t *)data);
    }
    return FALSE;
}

void
playlist_refresh (void) {
    search_refresh ();
    trkproperties_fill_metadata ();
}

static gboolean
playlistcontentchanged_cb (gpointer none) {
    playlist_refresh ();
    return FALSE;
}

static gboolean
playlistswitch_cb (gpointer none) {
    search_refresh ();
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
        snprintf (iconpath, sizeof (iconpath), "%s/deadbeef.png", deadbeef->get_prefix ());
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
gtkui_hide_status_icon () {
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
    w = orderwidgets[deadbeef->conf_get_int ("playback.order", PLAYBACK_ORDER_LINEAR)];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);

    // looping
    const char *loopingwidgets[3] = { "loop_all", "loop_disable", "loop_single" };
    w = loopingwidgets[deadbeef->conf_get_int ("playback.loop", PLAYBACK_MODE_LOOP_ALL)];
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
    preferences_fill_soundcards ();
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

static gboolean
update_win_title_idle (gpointer data) {
    struct fromto_t *ft = (struct fromto_t *)data;
    DB_playItem_t *from = ft->from;
    DB_playItem_t *to = ft->to;
    free (ft);

    // update window title
    if (from || to) {
        if (to) {
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
            if (it) { // it might have been deleted after event was sent
                gtkui_set_titlebar (it);
                deadbeef->pl_item_unref (it);
            }
            else {
                gtkui_set_titlebar (NULL);
            }
        }
        else {
            gtkui_set_titlebar (NULL);
        }
    }
    if (from) {
        deadbeef->pl_item_unref (from);
    }
    if (to) {
        deadbeef->pl_item_unref (to);
    }
    return FALSE;
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
gtkui_get_gui_refresh_rate () {
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
gtkui_thread (void *ctx);

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

int
gtkui_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (!gtkui_accept_messages) {
        return -1;
    }
    ddb_gtkui_widget_t *rootwidget = w_get_rootwidget ();
    if (rootwidget) {
        send_messages_to_widgets (rootwidget, id, ctx, p1, p2);
    }

    switch (id) {
    case DB_EV_ACTIVATED:
        g_idle_add (activate_cb, NULL);
        break;
    case DB_EV_SONGCHANGED:
        {
            ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
            gtkpl_songchanged_wrapper (ev->from, ev->to);
        }
        break;
    case DB_EV_TRACKINFOCHANGED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                deadbeef->pl_item_ref (ev->track);
            }
            g_idle_add (trackinfochanged_cb, ev->track);
        }
        break;
//    case DB_EV_PAUSED:
//        g_idle_add (paused_cb, NULL);
//        break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT) {
            g_idle_add (playlistcontentchanged_cb, NULL);
        }
        break;
    case DB_EV_CONFIGCHANGED:
        g_idle_add (gtkui_on_configchanged, NULL);
        break;
    case DB_EV_OUTPUTCHANGED:
        g_idle_add (outputchanged_cb, NULL);
        break;
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (playlistswitch_cb, NULL);
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

static const char gtkui_def_layout[] = "vbox expand=\"0 1\" fill=\"1 1\" homogeneous=0 {hbox expand=\"0 1 0\" fill=\"1 1 1\" homogeneous=0 {playtb {} seekbar {} volumebar {} } tabbed_playlist hideheaders=0 {} } ";

static void
init_widget_layout (void) {
    w_init ();
    ddb_gtkui_widget_t *rootwidget = w_get_rootwidget ();
    gtk_widget_show (rootwidget->widget);
    gtk_box_pack_start (GTK_BOX(lookup_widget(mainwin, "plugins_bottom_vbox")), rootwidget->widget, TRUE, TRUE, 0);

    // load layout
    // config var name is defined in DDB_GTKUI_CONF_LAYOUT
    // gtkui.layout: 0.6.0 and 0.6.1
    // gtkui.layout.major.minor.point: later versions

    char layout[20000];
    deadbeef->conf_get_str (DDB_GTKUI_CONF_LAYOUT, "-", layout, sizeof (layout));
    if (!strcmp (layout, "-")) {
        // upgrade from 0.6.0 to 0.6.2
        char layout_060[20000];
        deadbeef->conf_get_str ("gtkui.layout", "-", layout_060, sizeof (layout_060));
        if (!strcmp (layout_060, "-")) {
            // new setup
            strcpy (layout, gtkui_def_layout);
        }
        else {
            // upgrade with top bar
            snprintf (layout, sizeof (layout), "vbox expand=\"0 1\" fill=\"1 1\" homogeneous=0 {hbox expand=\"0 1 0\" fill=\"1 1 1\" homogeneous=0 {playtb {} seekbar {} volumebar {} } %s }", layout_060);
            deadbeef->conf_set_str (DDB_GTKUI_CONF_LAYOUT, layout);
            deadbeef->conf_save ();
        }
    }

    ddb_gtkui_widget_t *w = NULL;
    w_create_from_string (layout, &w);
    if (!w) {
        ddb_gtkui_widget_t *plt = w_create ("tabbed_playlist");
        w_append (rootwidget, plt);
        gtk_widget_show (plt->widget);
    }
    else {
        w_append (rootwidget, w);
    }
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

int
gtkui_thread (void *ctx) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-gtkui", 0, 0, 0, 0);
#endif

    int argc = 2;
    const char **argv = alloca (sizeof (char *) * argc);
    argv[0] = "deadbeef";
    argv[1] = "--sync";
    //argv[1] = "--g-fatal-warnings";
    if (!deadbeef->conf_get_int ("gtkui.sync", 0)) {
        argc = 1;
    }

    gtk_disable_setlocale ();
    add_pixmap_directory (deadbeef->get_pixmap_dir ());

    // let's start some gtk
    g_thread_init (NULL);
    gdk_threads_init ();
    gdk_threads_enter ();

    gtk_init (&argc, (char ***)&argv);

    // register widget types
    w_reg_widget (_("Playlist with tabs"), DDB_WF_SINGLE_INSTANCE, w_tabbed_playlist_create, "tabbed_playlist", NULL);
    w_reg_widget (_("Playlist"), DDB_WF_SINGLE_INSTANCE, w_playlist_create, "playlist", NULL);
    w_reg_widget (NULL, 0, w_box_create, "box", NULL);
    w_reg_widget (NULL, 0, w_dummy_create, "dummy", NULL);
    w_reg_widget (_("Splitter (top and bottom)"), 0, w_vsplitter_create, "vsplitter", NULL);
    w_reg_widget (_("Splitter (left and right)"), 0, w_hsplitter_create, "hsplitter", NULL);
    w_reg_widget (NULL, 0, w_placeholder_create, "placeholder", NULL);
    w_reg_widget (_("Tabs"), 0, w_tabs_create, "tabs", NULL);
    w_reg_widget (_("Playlist tabs"), 0, w_tabstrip_create, "tabstrip", NULL);
    w_reg_widget (_("Selection properties"), 0, w_selproperties_create, "selproperties", NULL);
    w_reg_widget (_("Album art display"), 0, w_coverart_create, "coverart", NULL);
    w_reg_widget (_("Scope"), 0, w_scope_create, "scope", NULL);
    w_reg_widget (_("Spectrum"), 0, w_spectrum_create, "spectrum", NULL);
    w_reg_widget (_("HBox"), 0, w_hbox_create, "hbox", NULL);
    w_reg_widget (_("VBox"), 0, w_vbox_create, "vbox", NULL);
    w_reg_widget (_("Button"), 0, w_button_create, "button", NULL);
    w_reg_widget (_("Seekbar"), 0, w_seekbar_create, "seekbar", NULL);
    w_reg_widget (_("Playback controls"), 0, w_playtb_create, "playtb", NULL);
    w_reg_widget (_("Volume bar"), 0, w_volumebar_create, "volumebar", NULL);
    w_reg_widget (_("Chiptune voices"), 0, w_ctvoices_create, "ctvoices", NULL);

    mainwin = create_mainwin ();

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
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_events (GTK_WIDGET (mainwin), gtk_widget_get_events (GTK_WIDGET (mainwin)) | GDK_SCROLL_MASK);
#endif

    pl_common_init();

    GtkIconTheme *theme = gtk_icon_theme_get_default();
    if (gtk_icon_theme_has_icon(theme, "deadbeef")) {
        gtk_window_set_icon_name (GTK_WINDOW (mainwin), "deadbeef");
    }
    else {
        // try loading icon from $prefix/deadbeef.png (for static build)
        char iconpath[1024];
        snprintf (iconpath, sizeof (iconpath), "%s/deadbeef.png", deadbeef->get_prefix ());
        gtk_window_set_icon_from_file (GTK_WINDOW (mainwin), iconpath, NULL);
    }

    wingeom_restore (mainwin, "mainwin", 40, 40, 500, 300, 0);

    gtkui_on_configchanged (NULL);
    gtkui_init_theme_colors ();

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

    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));

    DdbListview *search_playlist = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    search_playlist_init (GTK_WIDGET (search_playlist));

    progress_init ();
    cover_art_init ();

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
    gtk_widget_show (mainwin);

    init_widget_layout ();

    gtkui_set_titlebar (NULL);

    fileadded_listener_id = deadbeef->listen_file_added (gtkui_add_file_info_cb, NULL);
    fileadd_beginend_listener_id = deadbeef->listen_file_add_beginend (gtkui_add_file_begin_cb, gtkui_add_file_end_cb, NULL);

    supereq_plugin = deadbeef->plug_get_for_id ("supereq");

    gtkui_connect_cb (NULL);

    gtkui_accept_messages = 1;
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

#ifdef __APPLE__
    gtkui_is_retina = is_retina (mainwin);
#endif

    gtk_main ();

    deadbeef->unlisten_file_added (fileadded_listener_id);
    deadbeef->unlisten_file_add_beginend (fileadd_beginend_listener_id);

    w_free ();

    if (refresh_timeout) {
        g_source_remove (refresh_timeout);
        refresh_timeout = 0;
    }
    cover_art_free ();
    eq_window_destroy ();
    trkproperties_destroy ();
    progress_destroy ();
    gtkui_hide_status_icon ();
    pl_common_free();
//    draw_free ();
    titlebar_tf_free ();
    if (mainwin) {
        gtk_widget_destroy (mainwin);
        mainwin = NULL;
    }
    if (searchwin) {
        gtk_widget_destroy (searchwin);
        searchwin = NULL;
    }
    gdk_threads_leave ();
    return 0;
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
gtkui_playlist_set_curr (int playlist) {
    deadbeef->plt_set_curr_idx (playlist);
    deadbeef->conf_set_int ("playlist.current", playlist);
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
            gtk_text_buffer_set_text (buffer, error, strlen (error));
        }
        else {
            buf[s] = 0;
            gtk_text_buffer_set_text (buffer, buf, s);
        }
        fclose (fp);
    }
    else {
        const char *error = _("Failed to load help file");
        gtk_text_buffer_set_text (buffer, error, strlen (error));
    }
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (txt), buffer);
    g_object_unref (buffer);
    gtk_widget_show (widget);
}

gboolean
gtkui_quit_cb (void *ctx) {
    w_save ();

    if (deadbeef->have_background_jobs ()) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("The player is currently running background tasks. If you quit now, the tasks will be cancelled or interrupted. This may result in data loss."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Do you still want to quit?"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return FALSE;
        }
        else {
            exit (0);
        }
    }
    else {
        progress_abort ();
        deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
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

static int
gtkui_start (void) {
    fprintf (stderr, "gtkui plugin compiled for gtk version: %d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);

    import_legacy_tf ("gtkui.titlebar_playing", "gtkui.titlebar_playing_tf");
    import_legacy_tf ("gtkui.titlebar_stopped", "gtkui.titlebar_stopped_tf");

    import_legacy_tf ("playlist.group_by", "gtkui.playlist.group_by_tf");

    gtkui_thread (NULL);

    return 0;
}

static int
gtkui_connect (void) {
    return 0;
}

static int
gtkui_disconnect (void) {
    supereq_plugin = NULL;
    return 0;
}


static gboolean
quit_gtk_cb (gpointer nothing) {
    extern int trkproperties_modified;
    trkproperties_modified = 0;
    trkproperties_destroy ();
    search_destroy ();
    gtk_main_quit ();
    trace ("gtkui_stop completed\n");
    return FALSE;
}

static int
gtkui_stop (void) {
    trace ("quitting gtk\n");
    cover_art_disconnect();
    g_idle_add (quit_gtk_cb, NULL);
    return 0;
}

GtkWidget *
gtkui_get_mainwin (void) {
    return mainwin;
}

static DB_plugin_action_t action_deselect_all = {
    .title = "Edit/Deselect All",
    .name = "deselect_all",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_deselect_all_handler,
    .next = NULL
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
    .title = "Remove From Disk",
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
    .title = "Playback/Cycle Playback Looping Mode",
    .name = "loop_cycle",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_loop_cycle_handler,
    .next = &action_show_help
};

static DB_plugin_action_t action_playback_loop_off = {
    .title = "Playback/Playback Looping - Don't loop",
    .name = "loop_off",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_loop_off_handler,
    .next = &action_playback_loop_cycle
};

static DB_plugin_action_t action_playback_loop_single = {
    .title = "Playback/Playback Looping - Single track",
    .name = "loop_track",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_loop_single_handler,
    .next = &action_playback_loop_off
};

static DB_plugin_action_t action_playback_loop_all = {
    .title = "Playback/Playback Looping - All",
    .name = "loop_all",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_loop_all_handler,
    .next = &action_playback_loop_single
};

static DB_plugin_action_t action_playback_order_cycle = {
    .title = "Playback/Cycle Playback Order",
    .name = "order_cycle",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_cycle_handler,
    .next = &action_playback_loop_all
};

static DB_plugin_action_t action_playback_order_random = {
    .title = "Playback/Playback Order - Random",
    .name = "order_random",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_random_handler,
    .next = &action_playback_order_cycle
};

static DB_plugin_action_t action_playback_order_shuffle_albums = {
    .title = "Playback/Playback Order - Shuffle albums",
    .name = "order_shuffle_albums",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_shuffle_albums_handler,
    .next = &action_playback_order_random
};

static DB_plugin_action_t action_playback_order_shuffle = {
    .title = "Playback/Playback Order - Shuffle tracks",
    .name = "order_shuffle",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_playback_order_shuffle_handler,
    .next = &action_playback_order_shuffle_albums
};

static DB_plugin_action_t action_playback_order_linear = {
    .title = "Playback/Playback Order - Linear",
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
    .title = "Edit/Remove Track(s) From Playlist",
    .name = "remove_from_playlist",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
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

static DB_plugin_action_t action_toggle_eq = {
    .title = "View/Show\\/Hide Equalizer",
    .name = "toggle_eq",
    .flags = DB_ACTION_COMMON,
    .callback2 = action_toggle_eq_handler,
    .next = &action_new_playlist
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

static DB_plugin_action_t *
gtkui_get_actions (DB_playItem_t *it)
{
    return &action_find;
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

static const char settings_dlg[] =
    "property \"Ask confirmation to delete files from disk\" checkbox gtkui.delete_files_ask 1;\n"
    "property \"Status icon volume control sensitivity\" entry gtkui.tray_volume_sensitivity 1;\n"
    "property \"Custom status icon\" entry gtkui.custom_tray_icon \"" TRAY_ICON "\" ;\n"
    "property \"Run gtk_init with --sync (debug mode)\" checkbox gtkui.sync 0;\n"
    "property \"Add separators between plugin context menu items\" checkbox gtkui.action_separators 0;\n"
    "property \"Use unicode chars instead of images for track state\" checkbox gtkui.unicode_playstate 0;\n"
    "property \"Disable seekbar overlay text\" checkbox gtkui.disable_seekbar_overlay 0;\n"
;

// define plugin interface
static ddb_gtkui_t plugin = {
    .gui.plugin.api_vmajor = 1,
    .gui.plugin.api_vminor = 6,
    .gui.plugin.version_major = DDB_GTKUI_API_VERSION_MAJOR,
    .gui.plugin.version_minor = DDB_GTKUI_API_VERSION_MINOR,
    .gui.plugin.type = DB_PLUGIN_GUI,
    .gui.plugin.id = DDB_GTKUI_PLUGIN_ID,
#if GTK_CHECK_VERSION(3,0,0)
    .gui.plugin.name = "GTK3 user interface",
    .gui.plugin.descr = "User interface using GTK+ 3.x",
#else
    .gui.plugin.name = "GTK2 user interface",
    .gui.plugin.descr = "User interface using GTK+ 2.x",
#endif
    .gui.plugin.copyright = 
        "GTK+ user interface for DeaDBeeF Player.\n"
        "Copyright (C) 2009-2015 Alexey Yakovenko and other contributors\n"
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
    .gui.plugin.connect = gtkui_connect,
    .gui.plugin.disconnect = gtkui_disconnect,
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
    .create_pltmenu = gtkui_create_pltmenu,
    .get_cover_art_pixbuf = get_cover_art_callb, // deprecated
    .get_cover_art_primary = get_cover_art_primary,
    .get_cover_art_thumb = get_cover_art_thumb,
    .cover_get_default_pixbuf = cover_get_default_pixbuf,
    .add_window_init_hook = add_window_init_hook,
};
