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
#include "X11/Xlib.h"
#undef EGG_SM_CLIENT_BACKEND_XSMP
#ifdef EGG_SM_CLIENT_BACKEND_XSMP
#include "smclient/eggsmclient.h"
#endif
#include "actionhandlers.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static ddb_gtkui_t plugin;
DB_functions_t *deadbeef;

static intptr_t gtk_tid;

// cover art loading plugin
DB_artwork_plugin_t *coverart_plugin = NULL;

// main widgets
GtkWidget *mainwin;
GtkWidget *searchwin;
GtkStatusIcon *trayicon;
GtkWidget *traymenu;

// playlist theming
GtkWidget *theme_treeview;
GtkWidget *theme_button;

// overriden API methods
int (*gtkui_original_plt_add_dir) (ddb_playlist_t *plt, const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
int (*gtkui_original_plt_add_file) (ddb_playlist_t *plt, const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
int (*gtkui_original_pl_add_files_begin) (ddb_playlist_t *plt);
void (*gtkui_original_pl_add_files_end) (void);

// cached config variable
int gtkui_embolden_current_track;

#define TRAY_ICON "deadbeef_tray_icon"

// that must be called before gtk_init
void
gtkpl_init (void) {
    theme_treeview = gtk_tree_view_new ();
    gtk_widget_show (theme_treeview);
    gtk_widget_set_can_focus (theme_treeview, FALSE);
    GtkWidget *vbox1 = lookup_widget (mainwin, "vbox1");
    gtk_box_pack_start (GTK_BOX (vbox1), theme_treeview, FALSE, FALSE, 0);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (theme_treeview), TRUE);

    theme_button = lookup_widget (mainwin, "stopbtn");
}

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
static gboolean
redraw_seekbar_cb (gpointer nothing);

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
    float songpos = last_songpos;

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
        songpos = 0;
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
        songpos = playpos;
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
        // FIXME: don't update if window is not visible
        GtkStatusbar *sb = GTK_STATUSBAR (lookup_widget (mainwin, "statusbar"));
        if (sb_context_id == -1) {
            sb_context_id = gtk_statusbar_get_context_id (sb, "msg");
        }

        gtk_statusbar_pop (sb, sb_context_id);
        gtk_statusbar_push (sb, sb_context_id, sb_text);
    }

    if (mainwin) {
        GtkWidget *widget = lookup_widget (mainwin, "seekbar");
        // translate volume to seekbar pixels
        songpos /= duration;
        GtkAllocation a;
        gtk_widget_get_allocation (widget, &a);
        songpos *= a.width;
        if (fabs (songpos - last_songpos) > 0.01) {
            gtk_widget_queue_draw (widget);
            last_songpos = songpos;
        }
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
    volumebar_redraw ();

    //Update volume bar tooltip
    if (mainwin) {
        GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
        char s[100];
        int db = vol;
        snprintf (s, sizeof (s), "%s%ddB", db < 0 ? "" : "+", db);
        gtk_widget_set_tooltip_text (volumebar, s);
        gtk_widget_trigger_tooltip_query (volumebar);
    }

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
    g_idle_add (redraw_seekbar_cb, NULL);
    if (searchwin && gtk_widget_get_window (searchwin)) {
        int iconified = gdk_window_get_state(gtk_widget_get_window (searchwin)) & GDK_WINDOW_STATE_ICONIFIED;
        if (gtk_widget_get_visible (searchwin) && !iconified) {
            g_idle_add (redraw_queued_tracks_cb, DDB_LISTVIEW (lookup_widget (searchwin, "searchlist")));
        }
    }
}

void
gtkui_set_titlebar (DB_playItem_t *it) {
    if (!it) {
        it = deadbeef->streamer_get_playing_track ();
    }
    else {
        deadbeef->pl_item_ref (it);
    }
    char fmt[500];
    char str[600];
    if (it) {
        deadbeef->conf_get_str ("gtkui.titlebar_playing", "%a - %t - DeaDBeeF-%V", fmt, sizeof (fmt));
    }
    else {
        deadbeef->conf_get_str ("gtkui.titlebar_stopped", "DeaDBeeF-%V", fmt, sizeof (fmt));
    }
    deadbeef->pl_format_title (it, -1, str, sizeof (str), -1, fmt);
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
//    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
//    trackinfochanged_wrapper (DDB_LISTVIEW (playlist), track, PL_MAIN);

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
playlistchanged_cb (gpointer none) {
    playlist_refresh ();
    return FALSE;
}

void
gtkui_playlist_changed (void) {
    g_idle_add (playlistchanged_cb, NULL);
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
gtkui_volumechanged_cb (gpointer ctx) {
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    gdk_window_invalidate_rect (gtk_widget_get_window (volumebar), NULL, FALSE);
    return FALSE;
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

    // stop after current
    int stop_after_current = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "stop_after_current")), stop_after_current ? TRUE : FALSE);

    // embolden current track
    gtkui_embolden_current_track = deadbeef->conf_get_int ("gtkui.embolden_current_track", 0);

    // tray icon
    gtkui_update_status_icon (NULL);

    return FALSE;
}

static gboolean
outputchanged_cb (gpointer nothing) {
    preferences_fill_soundcards ();
    return FALSE;
}

void
save_playlist_as (void) {
    action_save_playlist_handler_cb (NULL);
}

#if 0
void
on_playlist_save_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char last_playlist_save_name[1024];
    deadbeef->conf_get_str ("gtkui.last_playlist_save_name", "", last_playlist_save_name, sizeof (last_playlist_save_name));
    if (!last_playlist_save_name[0]) {
        save_playlist_as ();
    }
    else {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_save (plt, NULL, NULL, last_playlist_save_name, NULL, NULL, NULL);
            deadbeef->plt_unref (plt);
        }
    }
}
#endif


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
    action_load_playlist_handler_cb (NULL);
}

void
on_add_location_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_add_location_handler_cb (NULL);
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

static gboolean
redraw_seekbar_cb (gpointer nothing) {
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (!gtk_widget_get_visible (mainwin) || iconified) {
        return FALSE;
    }
    seekbar_redraw ();
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

void
volumebar_redraw (void) {
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    gdk_window_invalidate_rect (gtk_widget_get_window (volumebar), NULL, FALSE);
}

//void
//tabstrip_redraw (void) {
//    GtkWidget *ts = lookup_widget (mainwin, "tabstrip");
//    ddb_tabstrip_refresh (DDB_TABSTRIP (ts));
//}

static int gtk_initialized = 0;
static gint refresh_timeout = 0;

void
gtkui_setup_gui_refresh (void) {
    int fps = deadbeef->conf_get_int ("gtkui.refresh_rate", 10);
    if (fps < 1) {
        fps = 1;
    }
    else if (fps > 30) {
        fps = 30;
    }

    int tm = 1000/fps;

    if (refresh_timeout) {
        g_source_remove (refresh_timeout);
        refresh_timeout = 0;
    }

    refresh_timeout = g_timeout_add (tm, gtkui_on_frameupdate, NULL);
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

void
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
    ddb_gtkui_widget_t *rootwidget = w_get_rootwidget ();
    if (rootwidget) {
        send_messages_to_widgets (rootwidget, id, ctx, p1, p2);
    }
    switch (id) {
    case DB_EV_PLUGINSLOADED:
        // gtk must be running in separate thread
        gtk_initialized = 0;
        gtk_tid = deadbeef->thread_start (gtkui_thread, NULL);
        // wait until gtk finishes initializing
        while (!gtk_initialized) {
            usleep (10000);
        }

        // override default file adding APIs to show progress bar
        gtkui_original_plt_add_dir = deadbeef->plt_add_dir;
        deadbeef->plt_add_dir = gtkui_plt_add_dir;

        gtkui_original_plt_add_file = deadbeef->plt_add_file;
        deadbeef->plt_add_file = gtkui_plt_add_file;

        gtkui_original_pl_add_files_begin = deadbeef->pl_add_files_begin;
        deadbeef->pl_add_files_begin = gtkui_pl_add_files_begin;

        gtkui_original_pl_add_files_end = deadbeef->pl_add_files_end;
        deadbeef->pl_add_files_end = gtkui_pl_add_files_end;

        gtkui_original_plt_load = deadbeef->plt_load;
        deadbeef->plt_load = gtkui_plt_load;
        break;
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
        gtkui_playlist_changed ();
        break;
    case DB_EV_VOLUMECHANGED:
        g_idle_add (gtkui_volumechanged_cb, NULL);
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

#ifdef EGG_SM_CLIENT_BACKEND_XSMP
static void
smclient_quit_requested (EggSMClient *client, gpointer user_data) {
    egg_sm_client_will_quit (client, TRUE);
}

static void
smclient_quit_cancelled (EggSMClient *client, gpointer user_data) {
}

static void
smclient_quit (EggSMClient *client, gpointer user_data) {
    gtkui_quit ();
}

static void
smclient_save_state (EggSMClient *client, const char *state_dir, gpointer user_data) {
}
#endif

static gboolean
unlock_playlist_columns_cb (void *ctx) {
//    ddb_listview_lock_columns (DDB_LISTVIEW (lookup_widget (mainwin, "playlist")), 0);
    return FALSE;
}

static void
init_widget_layout (void) {
    w_init ();
    ddb_gtkui_widget_t *rootwidget = w_get_rootwidget ();
    gtk_widget_show (rootwidget->widget);
    gtk_box_pack_start (GTK_BOX(lookup_widget(mainwin, "plugins_bottom_vbox")), rootwidget->widget, TRUE, TRUE, 0);

    // load layout
    char layout[4000];
    deadbeef->conf_get_str ("gtkui.layout", "tabbed_playlist \"\" { }", layout, sizeof (layout));

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

void
gtkui_thread (void *ctx) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-gtkui", 0, 0, 0, 0);
#endif
    XInitThreads (); // gtkglext/xcb doesn't work without this
    // let's start some gtk
    g_thread_init (NULL);
    add_pixmap_directory (deadbeef->get_pixmap_dir ());
    gdk_threads_init ();

    int argc = 2;
    const char **argv = alloca (sizeof (char *) * argc);
    argv[0] = "deadbeef";
    argv[1] = "--sync";
    //argv[1] = "--g-fatal-warnings";
    if (!deadbeef->conf_get_int ("gtkui.sync", 0)) {
        argc = 1;
    }

    gtk_disable_setlocale ();
#ifdef EGG_SM_CLIENT_BACKEND_XSMP
    g_type_init ();
    GOptionContext *goption_context;
    GError *err = NULL;
    goption_context = g_option_context_new (_("- Test logout functionality"));
    g_option_context_add_group (goption_context, gtk_get_option_group (TRUE));
    g_option_context_add_group (goption_context, egg_sm_client_get_option_group ());

    if (!g_option_context_parse (goption_context, &argc, (char ***)&argv, &err))
    {
        g_printerr ("Could not parse arguments: %s\n", err->message);
        g_error_free (err);
    }
    else {
        EggSMClient *client = egg_sm_client_get ();
        g_signal_connect (client, "quit-requested", G_CALLBACK (smclient_quit_requested), NULL);
        g_signal_connect (client, "quit-cancelled", G_CALLBACK (smclient_quit_cancelled), NULL);
        g_signal_connect (client, "quit", G_CALLBACK (smclient_quit), NULL);
        g_signal_connect (client, "save-state", G_CALLBACK (smclient_save_state), NULL);
    }
#endif

    gtk_init (&argc, (char ***)&argv);

    // register widget types
    w_reg_widget (_("Playlist with tabs"), w_tabbed_playlist_create, "tabbed_playlist", NULL);
    w_reg_widget (NULL, w_box_create, "box", NULL);
    w_reg_widget (_("Splitter (top and bottom)"), w_vsplitter_create, "vsplitter", NULL);
    w_reg_widget (_("Splitter (left and right)"), w_hsplitter_create, "hsplitter", NULL);
    w_reg_widget (NULL, w_placeholder_create, "placeholder", NULL);
    w_reg_widget (_("Tabs"), w_tabs_create, "tabs", NULL);
    w_reg_widget (_("Playlist tabs"), w_tabstrip_create, "tabstrip", NULL);
    w_reg_widget (_("Playlist"), w_playlist_create, "playlist", NULL);
    w_reg_widget (_("Selection properties"), w_selproperties_create, "selproperties", NULL);
    w_reg_widget (_("Album art display"), w_coverart_create, "coverart", NULL);
    w_reg_widget (_("Scope"), w_scope_create, "scope", NULL);
    w_reg_widget (_("Spectrum"), w_spectrum_create, "spectrum", NULL);
    w_reg_widget (_("HBox"), w_hbox_create, "hbox", NULL);
    w_reg_widget (_("VBox"), w_vbox_create, "vbox", NULL);
    w_reg_widget (_("Button"), w_button_create, "button", NULL);

    mainwin = create_mainwin ();

    // initialize default hotkey mapping
    struct stat st;
    char checkpath[PATH_MAX];
    snprintf (checkpath, sizeof (checkpath), "%s/config", deadbeef->get_config_dir ());
    if (stat (checkpath, &st)) {
        printf ("file %s doesn't exist\n", checkpath);
        deadbeef->conf_set_str ("hotkey.key01", "\"Ctrl f\" 0 0 find");
        deadbeef->conf_set_str ("hotkey.key02", "\"Ctrl o\" 0 0 open_files");
        deadbeef->conf_set_str ("hotkey.key03", "\"Ctrl q\" 0 0 quit");
        deadbeef->conf_set_str ("hotkey.key04", "\"Ctrl n\" 0 0 new_playlist");
        deadbeef->conf_set_str ("hotkey.key05", "\"Ctrl a\" 0 0 select_all");
        deadbeef->conf_set_str ("hotkey.key06", "\"Escape\" 0 0 deselect_all");
        deadbeef->conf_set_str ("hotkey.key07", "\"Ctrl m\" 0 0 toggle_stop_after_current");
        deadbeef->conf_set_str ("hotkey.key08", "\"Ctrl j\" 0 0 jump_to_current_track");
        deadbeef->conf_set_str ("hotkey.key09", "\"F1\" 0 0 help");
        deadbeef->conf_set_str ("hotkey.key10", "\"Delete\" 1 0 remove_from_playlist");
        deadbeef->conf_set_str ("hotkey.key11", "\"Ctrl w\" 0 0 remove_current_playlist");
        deadbeef->conf_set_str ("hotkey.key11", "\"Ctrl w\" 0 0 remove_current_playlist");
        deadbeef->conf_set_str ("hotkey.key11", "\"Ctrl w\" 0 0 remove_current_playlist");
        deadbeef->conf_set_str ("hotkey.key14", "\"Return\" 0 0 play");
        deadbeef->conf_set_str ("hotkey.key15", "\"Ctrl p\" 0 0 toggle_pause");
        deadbeef->conf_set_str ("hotkey.key16", "\"Alt 1\" 0 0 playlist1");
        deadbeef->conf_set_str ("hotkey.key17", "\"Alt 2\" 0 0 playlist2");
        deadbeef->conf_set_str ("hotkey.key18", "\"Alt 3\" 0 0 playlist3");
        deadbeef->conf_set_str ("hotkey.key19", "\"Alt 4\" 0 0 playlist4");
        deadbeef->conf_set_str ("hotkey.key20", "\"Alt 5\" 0 0 playlist5");
        deadbeef->conf_set_str ("hotkey.key21", "\"Alt 6\" 0 0 playlist6");
        deadbeef->conf_set_str ("hotkey.key22", "\"Alt 7\" 0 0 playlist7");
        deadbeef->conf_set_str ("hotkey.key23", "\"Alt 8\" 0 0 playlist8");
        deadbeef->conf_set_str ("hotkey.key24", "\"Alt 9\" 0 0 playlist9");
        deadbeef->conf_set_str ("hotkey.key25", "\"Alt 0\" 0 0 playlist10");
    }
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_events (GTK_WIDGET (mainwin), gtk_widget_get_events (GTK_WIDGET (mainwin)) | GDK_SCROLL_MASK);
#endif

    gtkpl_init ();

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
    GtkWidget *ts_mi = lookup_widget (mainwin, "view_tabs");
    GtkWidget *sb = lookup_widget (mainwin, "statusbar");
    if (deadbeef->conf_get_int ("gtkui.statusbar.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), FALSE);
        gtk_widget_hide (sb);
    }
    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));

    DdbListview *search_playlist = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    search_playlist_init (GTK_WIDGET (search_playlist));

    progress_init ();
    cover_art_init ();

    gtk_widget_show (mainwin);

    init_widget_layout ();

    gtkui_setup_gui_refresh ();

    char fmt[500];
    char str[600];
    deadbeef->conf_get_str ("gtkui.titlebar_stopped", "DeaDBeeF-%V", fmt, sizeof (fmt));
    deadbeef->pl_format_title (NULL, -1, str, sizeof (str), -1, fmt);
    gtk_window_set_title (GTK_WINDOW (mainwin), str);
    gtk_initialized = 1;

    g_idle_add (unlock_playlist_columns_cb, NULL);

    gtk_main ();

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
//    draw_free ();
    if (theme_treeview) {
        gtk_widget_destroy (theme_treeview);
        theme_treeview = NULL;
    }
    if (mainwin) {
        gtk_widget_destroy (mainwin);
        mainwin = NULL;
    }
    if (searchwin) {
        gtk_widget_destroy (searchwin);
        searchwin = NULL;
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

int
gtkui_add_file_info_cb (DB_playItem_t *it, void *data) {
    if (progress_is_aborted ()) {
        return -1;
    }
    deadbeef->pl_lock ();
    const char *fname = deadbeef->pl_find_meta (it, ":URI");
    g_idle_add (gtkui_set_progress_text_idle, (gpointer)strdup(fname)); // slowwwww
    deadbeef->pl_unlock ();
    return 0;
}

DB_playItem_t * (*gtkui_original_plt_load) (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

int
gtkui_plt_add_dir (ddb_playlist_t *plt, const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    int res = gtkui_original_plt_add_dir (plt, dirname, gtkui_add_file_info_cb, NULL);
    return res;
}

int
gtkui_plt_add_file (ddb_playlist_t *plt, const char *filename, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    int res = gtkui_original_plt_add_file (plt, filename, gtkui_add_file_info_cb, NULL);
    return res;
}

int
gtkui_pl_add_files_begin (ddb_playlist_t *plt) {
    progress_show ();
    return gtkui_original_pl_add_files_begin (plt);
}

void
gtkui_pl_add_files_end (void) {
    progress_hide ();
    gtkui_original_pl_add_files_end ();
}

DB_playItem_t *
gtkui_plt_load (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) {
    if (deadbeef->pl_add_files_begin (plt) < 0) {
        return NULL;
    }
    DB_playItem_t *it = gtkui_original_plt_load (plt, after, fname, pabort, gtkui_add_file_info_cb, user_data);
    deadbeef->pl_add_files_end ();

    return it;
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
    if (deadbeef->have_background_jobs ()) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("The player is currently running backgroud tasks. If you quit now, the tasks will be cancelled or interrupted. This may result in data loss."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Do you still want to quit?"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return FALSE;
        }
    }
    progress_abort ();
    deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
    return FALSE;
}

void
gtkui_quit (void) {
    gdk_threads_add_idle (gtkui_quit_cb, NULL);
}

static int
gtkui_start (void) {
    fprintf (stderr, "gtkui plugin compiled for gtk version: %d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);

    return 0;
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

    // cover_art
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->id && !strcmp (p->id, "artwork")) {
            trace ("gtkui: found cover-art loader plugin\n");
            coverart_plugin = (DB_artwork_plugin_t *)p;
            break;
        }
    }
    gtkui_playlist_changed ();
    add_mainmenu_actions ();
    ddb_event_t *e = deadbeef->event_alloc (DB_EV_TRACKINFOCHANGED);
    deadbeef->event_send(e, 0, 0);
    return FALSE;
}

static int
gtkui_connect (void) {
    supereq_plugin = deadbeef->plug_get_for_id ("supereq");
    // need to do it in gtk thread
    g_idle_add (gtkui_connect_cb, NULL);

    return 0;
}

static int
gtkui_disconnect (void) {
    supereq_plugin = NULL;
    coverart_plugin = NULL;

    return 0;
}


static gboolean
quit_gtk_cb (gpointer nothing) {
    extern int trkproperties_modified;
    trkproperties_modified = 0;
    trkproperties_destroy ();
    search_destroy ();
    gtk_main_quit ();
    return FALSE;
}

static int
gtkui_stop (void) {
    if (coverart_plugin) {
        coverart_plugin->plugin.plugin.stop ();
        coverart_plugin = NULL;
    }
    trace ("quitting gtk\n");
    g_idle_add (quit_gtk_cb, NULL);
    trace ("waiting for gtk thread to finish\n");
    deadbeef->thread_join (gtk_tid);
    trace ("gtk thread finished\n");
    gtk_tid = 0;
    //main_playlist_free ();
    trace ("gtkui_stop completed\n");
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
    .callback = action_deselect_all_handler,
    .next = NULL
};

static DB_plugin_action_t action_select_all = {
    .title = "Edit/Select All",
    .name = "select_all",
    .flags = DB_ACTION_COMMON,
    .callback = action_select_all_handler,
    .next = &action_deselect_all
};

static DB_plugin_action_t action_quit = {
    .title = "Quit",
    .name = "quit",
    .flags = DB_ACTION_COMMON,
    .callback = action_quit_handler,
    .next = &action_select_all
};

static DB_plugin_action_t action_delete_from_disk = {
    .title = "Delete From Disk",
    .name = "delete_from_disk",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback = action_delete_from_disk_handler,
    .next = &action_quit
};

static DB_plugin_action_t action_add_location = {
    .title = "File/Add Location",
    .name = "add_location",
    .flags = DB_ACTION_COMMON,
    .callback = action_add_location_handler,
    .next = &action_delete_from_disk
};

static DB_plugin_action_t action_add_folders = {
    .title = "File/Add Folder(s)",
    .name = "add_folders",
    .flags = DB_ACTION_COMMON,
    .callback = action_add_folders_handler,
    .next = &action_add_location
};

static DB_plugin_action_t action_add_files = {
    .title = "File/Add File(s)",
    .name = "add_files",
    .flags = DB_ACTION_COMMON,
    .callback = action_add_files_handler,
    .next = &action_add_folders
};

static DB_plugin_action_t action_open_files = {
    .title = "File/Open File(s)",
    .name = "open_files",
    .flags = DB_ACTION_COMMON,
    .callback = action_open_files_handler,
    .next = &action_add_files
};


static DB_plugin_action_t action_track_properties = {
    .title = "Track properties",
    .name = "track_properties",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback = action_show_track_properties_handler,
    .next = &action_open_files
};

static DB_plugin_action_t action_show_help = {
    .title = "Help/Show help page",
    .name = "help",
    .flags = DB_ACTION_COMMON,
    .callback = action_show_help_handler,
    .next = &action_track_properties
};

static DB_plugin_action_t action_playback_loop_cycle = {
    .title = "Playback/Cycle playback looping mode",
    .name = "loop_cycle",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_loop_cycle_handler,
    .next = &action_show_help
};

static DB_plugin_action_t action_playback_loop_off = {
    .title = "Playback/Playback looping - Don't loop",
    .name = "loop_off",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_loop_off_handler,
    .next = &action_playback_loop_cycle
};

static DB_plugin_action_t action_playback_loop_single = {
    .title = "Playback/Playback looping - Single track",
    .name = "loop_track",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_loop_single_handler,
    .next = &action_playback_loop_off
};

static DB_plugin_action_t action_playback_loop_all = {
    .title = "Playback/Playback looping - All",
    .name = "loop_all",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_loop_all_handler,
    .next = &action_playback_loop_single
};

static DB_plugin_action_t action_playback_order_cycle = {
    .title = "Playback/Cycle playback order",
    .name = "order_cycle",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_order_cycle_handler,
    .next = &action_playback_loop_all
};

static DB_plugin_action_t action_playback_order_random = {
    .title = "Playback/Playback order - Random",
    .name = "order_random",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_order_random_handler,
    .next = &action_playback_order_cycle
};

static DB_plugin_action_t action_playback_order_shuffle_albums = {
    .title = "Playback/Playback order - Shuffle albums",
    .name = "order_shuffle_albums",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_order_shuffle_albums_handler,
    .next = &action_playback_order_random
};

static DB_plugin_action_t action_playback_order_shuffle = {
    .title = "Playback/Playback order - Shuffle tracks",
    .name = "order_shuffle",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_order_shuffle_handler,
    .next = &action_playback_order_shuffle_albums
};

static DB_plugin_action_t action_playback_order_linear = {
    .title = "Playback/Playback order - Linear",
    .name = "order_linear",
    .flags = DB_ACTION_COMMON,
    .callback = action_playback_order_linear_handler,
    .next = &action_playback_order_shuffle
};


static DB_plugin_action_t action_cursor_follows_playback = {
    .title = "Playback/Cursor follows playback toggle",
    .name = "toggle_cursor_follows_playback",
    .flags = DB_ACTION_COMMON,
    .callback = action_cursor_follows_playback_handler,
    .next = &action_playback_order_linear
};


static DB_plugin_action_t action_scroll_follows_playback = {
    .title = "Playback/Scroll follows playback toggle",
    .name = "toggle_scroll_follows_playback",
    .flags = DB_ACTION_COMMON,
    .callback = action_scroll_follows_playback_handler,
    .next = &action_cursor_follows_playback
};

static DB_plugin_action_t action_toggle_menu = {
    .title = "View/Show\\/Hide menu",
    .name = "toggle_menu",
    .flags = DB_ACTION_COMMON,
    .callback = action_toggle_menu_handler,
    .next = &action_scroll_follows_playback
};

static DB_plugin_action_t action_toggle_statusbar = {
    .title = "View/Show\\/Hide statusbar",
    .name = "toggle_statusbar",
    .flags = DB_ACTION_COMMON,
    .callback = action_toggle_statusbar_handler,
    .next = &action_toggle_menu
};

static DB_plugin_action_t action_toggle_designmode = {
    .title = "Edit/Toggle design mode",
    .name = "toggle_design_mode",
    .flags = DB_ACTION_COMMON,
    .callback = action_toggle_designmode_handler,
    .next = &action_toggle_statusbar
};

static DB_plugin_action_t action_preferences = {
    .title = "Edit/Preferences",
    .name = "preferences",
    .flags = DB_ACTION_COMMON,
    .callback = action_preferences_handler,
    .next = &action_toggle_designmode
};

static DB_plugin_action_t action_sort_custom = {
    .title = "Edit/Sort Custom",
    .name = "sort_custom",
    .flags = DB_ACTION_COMMON,
    .callback = action_sort_custom_handler,
    .next = &action_preferences
};

static DB_plugin_action_t action_crop_selected = {
    .title = "Edit/Crop Selected",
    .name = "crop_selected",
    .flags = DB_ACTION_COMMON,
    .callback = action_crop_selected_handler,
    .next = &action_sort_custom
};

static DB_plugin_action_t action_remove_from_playlist = {
    .title = "Edit/Remove from current playlist",
    .name = "remove_from_playlist",
    .flags = DB_ACTION_MULTIPLE_TRACKS,
    .callback = action_remove_from_playlist_handler,
    .next = &action_crop_selected
};

static DB_plugin_action_t action_save_playlist = {
    .title = "File/Save playlist",
    .name = "save_playlist",
    .flags = DB_ACTION_COMMON,
    .callback = action_save_playlist_handler,
    .next = &action_remove_from_playlist
};

static DB_plugin_action_t action_load_playlist = {
    .title = "File/Load playlist",
    .name = "load_playlist",
    .flags = DB_ACTION_COMMON,
    .callback = action_load_playlist_handler,
    .next = &action_save_playlist
};

static DB_plugin_action_t action_remove_current_playlist = {
    .title = "File/Remove current playlist",
    .name = "remove_current_playlist",
    .flags = DB_ACTION_COMMON,
    .callback = action_remove_current_playlist_handler,
    .next = &action_load_playlist
};


static DB_plugin_action_t action_new_playlist = {
    .title = "File/New Playlist",
    .name = "new_playlist",
    .flags = DB_ACTION_COMMON,
    .callback = action_new_playlist_handler,
    .next = &action_remove_current_playlist
};

static DB_plugin_action_t action_toggle_eq = {
    .title = "View/Show\\/Hide Equalizer",
    .name = "toggle_eq",
    .flags = DB_ACTION_COMMON,
    .callback = action_toggle_eq_handler,
    .next = &action_new_playlist
};

static DB_plugin_action_t action_hide_eq = {
    .title = "View/Hide Equalizer",
    .name = "hide_eq",
    .flags = DB_ACTION_COMMON,
    .callback = action_hide_eq_handler,
    .next = &action_toggle_eq
};

static DB_plugin_action_t action_show_eq = {
    .title = "View/Show Equalizer",
    .name = "show_eq",
    .flags = DB_ACTION_COMMON,
    .callback = action_show_eq_handler,
    .next = &action_hide_eq
};

static DB_plugin_action_t action_toggle_mainwin = {
    .title = "View/Show\\/Hide Player Window",
    .name = "toggle_player_window",
    .flags = DB_ACTION_COMMON,
    .callback = action_toggle_mainwin_handler,
    .next = &action_show_eq
};

static DB_plugin_action_t action_hide_mainwin = {
    .title = "View/Hide Player Window",
    .name = "hide_player_window",
    .flags = DB_ACTION_COMMON,
    .callback = action_hide_mainwin_handler,
    .next = &action_toggle_mainwin
};

static DB_plugin_action_t action_show_mainwin = {
    .title = "View/Show Player Window",
    .name = "show_player_window",
    .flags = DB_ACTION_COMMON,
    .callback = action_show_mainwin_handler,
    .next = &action_hide_mainwin
};

static DB_plugin_action_t action_find = {
    .title = "Edit/Find",
    .name = "find",
    .flags = DB_ACTION_COMMON,
    .callback = action_find_handler,
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
//    "property \"Show volume in dB (percentage otherwise)\" entry gtkui.show_gain_in_db 1\n"
    "property \"Custom status icon\" entry gtkui.custom_tray_icon \"" TRAY_ICON "\" ;\n"
    "property \"Run gtk_init with --sync (debug mode)\" checkbox gtkui.sync 0;\n"
    "property \"Add separators between plugin context menu items\" checkbox gtkui.action_separators 0;\n"
    "property \"Auto-resize columns to fit the main window\" checkbox gtkui.autoresize_columns 0;\n"
;

// define plugin interface
static ddb_gtkui_t plugin = {
    .gui.plugin.api_vmajor = 1,
    .gui.plugin.api_vminor = 5,
    .gui.plugin.version_major = 2,
    .gui.plugin.version_minor = DDB_GTKUI_API_VERSION,
    .gui.plugin.type = DB_PLUGIN_MISC,
    .gui.plugin.id = DDB_GTKUI_PLUGIN_ID,
#if GTK_CHECK_VERSION(3,0,0)
    .gui.plugin.name = "GTK3 user interface",
    .gui.plugin.descr = "User interface using GTK+ 3.x",
#else
    .gui.plugin.name = "GTK2 user interface",
    .gui.plugin.descr = "User interface using GTK+ 2.x",
#endif
    .gui.plugin.copyright = 
        "Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
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
};
