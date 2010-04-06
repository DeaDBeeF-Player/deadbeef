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
#include "../../deadbeef.h"
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#if HAVE_NOTIFY
#include <libnotify/notify.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include "gtkui.h"
#include "ddblistview.h"
#include "mainplaylist.h"
#include "search.h"
#include "progress.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include "parser.h"
#include "drawing.h"
#include "trkproperties.h"
#include "../artwork/artwork.h"
#include "coverart.h"
#include "plcommon.h"
#include "ddbtabstrip.h"
#include "eq.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_gui_t plugin;
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

// that must be called before gtk_init
void
gtkpl_init (void) {
    theme_treeview = gtk_tree_view_new ();
    GTK_WIDGET_UNSET_FLAGS (theme_treeview, GTK_CAN_FOCUS);
    gtk_widget_show (theme_treeview);
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
    int from;
    int to;
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
    int iconified = gdk_window_get_state(mainwin->window) & GDK_WINDOW_STATE_ICONIFIED;
    if (!GTK_WIDGET_VISIBLE (mainwin) || iconified) {
        return FALSE;
    }
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
        snprintf (totaltime_str, sizeof (totaltime_str), "1 day %d:%02d:%02d", hourtotal, mintotal, sectotal);
    }
    else {
        snprintf (totaltime_str, sizeof (totaltime_str), "%d days %d:%02d:%02d", daystotal, hourtotal, mintotal, sectotal);
    }

    DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
    DB_fileinfo_t *c = deadbeef->streamer_get_current_fileinfo (); // FIXME: might crash streamer

    float duration = track ? deadbeef->pl_get_item_duration (track) : -1;

    if (deadbeef->get_output ()->state () == OUTPUT_STATE_STOPPED || !track || !c) {
        snprintf (sbtext_new, sizeof (sbtext_new), "Stopped | %d tracks | %s total playtime", deadbeef->pl_getcount (PL_MAIN), totaltime_str);
        songpos = 0;
    }
    else {
        float playpos = deadbeef->streamer_get_playpos ();
        int minpos = playpos / 60;
        int secpos = playpos - minpos * 60;
        int mindur = duration / 60;
        int secdur = duration - mindur * 60;

        const char *mode = c->channels == 1 ? "Mono" : "Stereo";
        int samplerate = c->samplerate;
        int bitspersample = c->bps;
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
                snprintf (sbitrate, sizeof (sbitrate), "| %4d kbps ", bitrate);
            }
            else {
                sbitrate[0] = 0;
            }
        }
        const char *spaused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED ? "Paused | " : "";
        snprintf (sbtext_new, sizeof (sbtext_new), "%s%s %s| %dHz | %d bit | %s | %d:%02d / %s | %d tracks | %s total playtime", spaused, track->filetype ? track->filetype:"-", sbitrate, samplerate, bitspersample, mode, minpos, secpos, t, deadbeef->pl_getcount (PL_MAIN), totaltime_str);
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

    void seekbar_draw (GtkWidget *widget);
    void seekbar_expose (GtkWidget *widget, int x, int y, int w, int h);
    if (mainwin) {
        GtkWidget *widget = lookup_widget (mainwin, "seekbar");
        // translate volume to seekbar pixels
        songpos /= duration;
        songpos *= widget->allocation.width;
        if (fabs (songpos - last_songpos) > 0.01) {
            seekbar_draw (widget);
            seekbar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
            last_songpos = songpos;
        }
    }
    if (track) {
        deadbeef->pl_item_unref (track);
    }
    return FALSE;
}

gboolean
on_trayicon_scroll_event               (GtkWidget       *widget,
                                        GdkEventScroll  *event,
                                        gpointer         user_data)
{
    float vol = deadbeef->volume_get_db ();
    if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
        vol += 1;
    }
    else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
        vol -= 1;
    }
    if (vol > 0) {
        vol = 0;
    }
    else if (vol < -60) {
        vol = -60;
    }
    deadbeef->volume_set_db (vol);
    volumebar_redraw ();
    return FALSE;
}

void
mainwin_toggle_visible (void) {
    int iconified = gdk_window_get_state(mainwin->window) & GDK_WINDOW_STATE_ICONIFIED;
    if (GTK_WIDGET_VISIBLE (mainwin) && !iconified) {
        gtk_widget_hide (mainwin);
    }
    else {
        int x = deadbeef->conf_get_int ("mainwin.geometry.x", 40);
        int y = deadbeef->conf_get_int ("mainwin.geometry.y", 40);
        int w = deadbeef->conf_get_int ("mainwin.geometry.w", 500);
        int h = deadbeef->conf_get_int ("mainwin.geometry.h", 300);
        gtk_window_move (GTK_WINDOW (mainwin), x, y);
        gtk_window_resize (GTK_WINDOW (mainwin), w, h);
        if (deadbeef->conf_get_int ("mainwin.geometry.maximized", 0)) {
            gtk_window_maximize (GTK_WINDOW (mainwin));
        }
        if (iconified) {
            gtk_window_deiconify (GTK_WINDOW(mainwin));
        }
        else {
            gtk_window_present (GTK_WINDOW (mainwin));
        }
    }
}

#if GTK_MINOR_VERSION<=14

gboolean
on_trayicon_activate (GtkWidget       *widget,
                                        GdkEvent  *event,
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
    if (event->button == 1) {
        mainwin_toggle_visible ();
    }
    else if (event->button == 2) {
        deadbeef->sendmessage (M_PAUSESONG, 0, 0, 0);
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

static int
gtkui_on_activate (DB_event_t *ev, uintptr_t data) {
    g_idle_add (activate_cb, NULL);
    return 0;
}

void
redraw_queued_tracks (DdbListview *pl, int list) {
    DB_playItem_t *it;
    int idx = 0;
    for (it = deadbeef->pl_get_first (PL_MAIN); it; idx++) {
        if (deadbeef->pl_playqueue_test (it) != -1) {
            ddb_listview_draw_row (pl, idx, (DdbListviewIter)it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

static gboolean
redraw_queued_tracks_cb (gpointer nothing) {
    int iconified = gdk_window_get_state(mainwin->window) & GDK_WINDOW_STATE_ICONIFIED;
    if (!GTK_WIDGET_VISIBLE (mainwin) || iconified) {
        return FALSE;
    }
    redraw_queued_tracks (DDB_LISTVIEW (lookup_widget (mainwin, "playlist")), PL_MAIN);
    redraw_queued_tracks (DDB_LISTVIEW (lookup_widget (searchwin, "searchlist")), PL_SEARCH);
    return FALSE;
}

void
gtkpl_songchanged_wrapper (int from, int to) {
    struct fromto_t *ft = malloc (sizeof (struct fromto_t));
    ft->from = from;
    ft->to = to;
    g_idle_add (update_win_title_idle, ft);
    if (ft->to == -1) {
        // redraw seekbar
        g_idle_add (redraw_seekbar_cb, NULL);
    }
    g_idle_add (redraw_queued_tracks_cb, NULL);
}

static int
gtkui_on_songchanged (DB_event_trackchange_t *ev, uintptr_t data) {
    gtkpl_songchanged_wrapper (ev->from, ev->to);
    return 0;
}

void
set_tray_tooltip (const char *text) {
#if (GTK_MINOR_VERSION < 16)
        gtk_status_icon_set_tooltip (trayicon, text);
#else
        gtk_status_icon_set_tooltip_text (trayicon, text);
#endif
}

struct trackinfo_t {
    int index;
    DB_playItem_t *track;
};

static void
current_track_changed (DB_playItem_t *it) {
    char str[600];
    if (it) {
        deadbeef->pl_format_title (it, -1, str, sizeof (str), -1, "DeaDBeeF-" VERSION " - %a - %t");
    }
    else {
        strcpy (str, "DeaDBeeF-" VERSION);
    }
    gtk_window_set_title (GTK_WINDOW (mainwin), str);
    set_tray_tooltip (str);
}

static gboolean
trackinfochanged_cb (gpointer data) {
    struct trackinfo_t *ti = (struct trackinfo_t *)data;
    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
    ddb_listview_draw_row (DDB_LISTVIEW (playlist), ti->index, (DdbListviewIter)ti->track);
    DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
    if (ti->track == curr) {
        current_track_changed (ti->track);
    }
    if (curr) {
        deadbeef->pl_item_unref (curr);
    }
    free (ti);
    return FALSE;
}

static int
gtkui_on_trackinfochanged (DB_event_track_t *ev, uintptr_t data) {
    struct trackinfo_t *ti = malloc (sizeof (struct trackinfo_t));
    ti->index = ev->index;
    ti->track = ev->track;
    g_idle_add (trackinfochanged_cb, ti);
    return 0;
}

static gboolean
paused_cb (gpointer nothing) {
    DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
    if (curr) {
        int idx = deadbeef->pl_get_idx_of (curr);
        GtkWidget *playlist = lookup_widget (mainwin, "playlist");
        ddb_listview_draw_row (DDB_LISTVIEW (playlist), idx, (DdbListviewIter)curr);
        deadbeef->pl_item_unref (curr);
    }
    return FALSE;
}

static int
gtkui_on_paused (DB_event_state_t *ev, uintptr_t data) {
    g_idle_add (paused_cb, NULL);
}

void
playlist_refresh (void) {
    DdbListview *ps = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL | DDB_EXPOSE_LIST);
    search_refresh ();
}

static gboolean
playlistchanged_cb (gpointer none) {
    playlist_refresh ();
    return FALSE;
}

static int
gtkui_on_playlistchanged (DB_event_t *ev, uintptr_t data) {
    g_idle_add (playlistchanged_cb, NULL);
}

static gboolean
playlistswitch_cb (gpointer none) {
    GtkWidget *tabstrip = lookup_widget (mainwin, "tabstrip");
    int curr = deadbeef->plt_get_curr ();
    char conf[100];
    snprintf (conf, sizeof (conf), "playlist.scroll.%d", curr);
    int scroll = deadbeef->conf_get_int (conf, 0);
//    gdk_window_invalidate_rect (tabstrip->window, NULL, FALSE);
    ddb_tabstrip_refresh (DDB_TABSTRIP (tabstrip));
    DdbListview *listview = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    playlist_refresh ();
    ddb_listview_set_vscroll (listview, scroll);
    search_refresh ();
    return FALSE;
}

static int
gtkui_on_playlistswitch (DB_event_t *ev, uintptr_t data) {
    g_idle_add (playlistswitch_cb, NULL);
}

static int
gtkui_on_frameupdate (DB_event_t *ev, uintptr_t data) {
    g_idle_add (update_songinfo, NULL);
}

static int
gtkui_on_volumechanged (DB_event_t *ev, uintptr_t data) {
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    gdk_window_invalidate_rect (volumebar->window, NULL, FALSE);
    return 0;
}

static int
gtkui_on_configchanged (DB_event_t *ev, uintptr_t data) {
    // order and looping
    const char *w;

    // order
    const char *orderwidgets[3] = { "order_linear", "order_shuffle", "order_random" };
    w = orderwidgets[deadbeef->conf_get_int ("playback.order", PLAYBACK_ORDER_LINEAR)];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);

    // looping
    const char *loopingwidgets[3] = { "loop_all", "loop_disable", "loop_single" };
    w = loopingwidgets[deadbeef->conf_get_int ("playback.loop", PLAYBACK_MODE_LOOP_ALL)];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);

    // scroll follows playback
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "scroll_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.followplayback", 0) ? TRUE : FALSE);

    // cursor follows playback
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "cursor_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0) ? TRUE : FALSE);

    // stop after current
    int stop_after_current = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "stop_after_current")), stop_after_current ? TRUE : FALSE);

    return 0;
}

static gboolean
outputchanged_cb (gpointer nothing) {
    preferences_fill_soundcards ();
    return FALSE;
}

static int
gtkui_on_outputchanged (DB_event_t *ev, uintptr_t nothing) {
    g_idle_add (outputchanged_cb, NULL);
    return 0;
}

void
on_playlist_load_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Load Playlist", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "DeaDBeeF playlist files (*.dbpl)");
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (fname) {
            /*int res = */deadbeef->pl_load (fname);
            g_free (fname);
            main_refresh ();
            search_refresh ();
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}
void
on_add_audio_cd_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_add_file ("all.cda", NULL, NULL);
    playlist_refresh ();
}

void
on_add_location_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_addlocationdlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        GtkEntry *entry = GTK_ENTRY (lookup_widget (dlg, "addlocation_entry"));
        if (entry) {
            const char *text = gtk_entry_get_text (entry);
            if (text) {
                deadbeef->pl_add_file (text, NULL, NULL);
                playlist_refresh ();
            }
        }
    }
    gtk_widget_destroy (dlg);
}

static void
songchanged (DdbListview *ps, int from, int to) {
    int str_plt = deadbeef->streamer_get_current_playlist ();
    int plt = deadbeef->plt_get_curr ();
    if (plt != str_plt) {
        // have nothing to do here -- active playlist is not the one with playing song
        return;
    }
    if (!ddb_listview_is_scrolling (ps) && to != -1) {
        if (deadbeef->conf_get_int ("playlist.scroll.followplayback", 0)) {
            ddb_listview_scroll_to (ps, to);
        }
        if (deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0)) {
            ddb_listview_set_cursor (ps, to);
        }
    }

    if (from >= 0) {
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (from, PL_MAIN);
        if (it) {
            ddb_listview_draw_row (ps, from, it);
            deadbeef->pl_item_unref (it);
        }
    }
    if (to >= 0) {
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (to, PL_MAIN);
        if (it) {
            ddb_listview_draw_row (ps, to, it);
            deadbeef->pl_item_unref (it);
        }
    }
}

#if HAVE_NOTIFY
static NotifyNotification* notification;
#endif

static gboolean
update_win_title_idle (gpointer data) {
    struct fromto_t *ft = (struct fromto_t *)data;
    int from = ft->from;
    int to = ft->to;
    free (ft);

    // show notification
#if HAVE_NOTIFY
    if (to != -1 && deadbeef->conf_get_int ("gtkui.notify.enable", 0)) {
        DB_playItem_t *track = deadbeef->streamer_get_playing_track ();//deadbeef->pl_get_for_idx (to);
        if (track) {
            char cmd [1024];
            deadbeef->pl_format_title (track, -1, cmd, sizeof (cmd), -1, deadbeef->conf_get_str ("gtkui.notify.format", NOTIFY_DEFAULT_FORMAT));
            if (notify_is_initted ()) {
                if (notification) {
                    notify_notification_close (notification, NULL);
                }
                notification = notify_notification_new ("DeaDBeeF", cmd, NULL, NULL);
                if (notification) {
                    notify_notification_set_timeout (notification, NOTIFY_EXPIRES_DEFAULT);
                    notify_notification_show (notification, NULL);
                }
            }
            deadbeef->pl_item_unref (track);
        }
    }
#endif

    // update window title
    if (from >= 0 || to >= 0) {
        if (to >= 0) {
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();;
            if (it) { // it might have been deleted after event was sent
                current_track_changed (it);
                deadbeef->pl_item_unref (it);
            }
        }
        else {
            gtk_window_set_title (GTK_WINDOW (mainwin), "DeaDBeeF-" VERSION);
            set_tray_tooltip ("DeaDBeeF-" VERSION);
        }
    }
    // update playlist view
    songchanged (DDB_LISTVIEW (lookup_widget (mainwin, "playlist")), from, to);
    return FALSE;
}

static gboolean
redraw_seekbar_cb (gpointer nothing) {
    int iconified = gdk_window_get_state(mainwin->window) & GDK_WINDOW_STATE_ICONIFIED;
    if (!GTK_WIDGET_VISIBLE (mainwin) || iconified) {
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
            strcpy (name, "New Playlist");
        }
        else {
            snprintf (name, sizeof (name), "New Playlist (%d)", idx);
        }
        for (i = 0; i < cnt; i++) {
            char t[100];
            deadbeef->plt_get_title (i, t, sizeof (t));
            if (!strcasecmp (t, name)) {
                break;
            }
        }
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
    gdk_window_invalidate_rect (volumebar->window, NULL, FALSE);
}

void
tabstrip_redraw (void) {
    GtkWidget *ts = lookup_widget (mainwin, "tabstrip");
    ddb_tabstrip_refresh (DDB_TABSTRIP (ts));
}

static int gtk_initialized = 0;

void
gtkui_thread (void *ctx) {
    // let's start some gtk
    g_thread_init (NULL);
    add_pixmap_directory (PREFIX "/share/deadbeef/pixmaps");
    gdk_threads_init ();
    gdk_threads_enter ();
    gtk_set_locale ();
#if HAVE_NOTIFY
    notify_init ("DeaDBeeF");
#endif

    int argc = 2;
    const char **argv = alloca (sizeof (char *) * argc);
    argv[0] = "deadbeef";
    argv[1] = "--sync";
    if (!deadbeef->conf_get_int ("gtkui.sync", 0)) {
        argc = 1;
    }
    gtk_init (&argc, (char ***)&argv);

    // system tray icon
    traymenu = create_traymenu ();
    GdkPixbuf *trayicon_pixbuf = create_pixbuf ("play_24.png");
    trayicon = gtk_status_icon_new_from_pixbuf (trayicon_pixbuf);
    set_tray_tooltip ("DeaDBeeF");
    //gtk_status_icon_set_title (GTK_STATUS_ICON (trayicon), "DeaDBeeF");
#if GTK_MINOR_VERSION <= 14
    g_signal_connect ((gpointer)trayicon, "activate", G_CALLBACK (on_trayicon_activate), NULL);
#else
    g_signal_connect ((gpointer)trayicon, "scroll_event", G_CALLBACK (on_trayicon_scroll_event), NULL);
    g_signal_connect ((gpointer)trayicon, "button_press_event", G_CALLBACK (on_trayicon_button_press_event), NULL);
#endif
    g_signal_connect ((gpointer)trayicon, "popup_menu", G_CALLBACK (on_trayicon_popup_menu), NULL);

    mainwin = create_mainwin ();
    gtkpl_init ();

    GdkPixbuf *mainwin_icon_pixbuf;
    mainwin_icon_pixbuf = create_pixbuf ("play_24.png");
    if (mainwin_icon_pixbuf)
    {
        gtk_window_set_icon (GTK_WINDOW (mainwin), mainwin_icon_pixbuf);
        gdk_pixbuf_unref (mainwin_icon_pixbuf);
    }
    {
        int x = deadbeef->conf_get_int ("mainwin.geometry.x", 40);
        int y = deadbeef->conf_get_int ("mainwin.geometry.y", 40);
        int w = deadbeef->conf_get_int ("mainwin.geometry.w", 500);
        int h = deadbeef->conf_get_int ("mainwin.geometry.h", 300);
        gtk_window_move (GTK_WINDOW (mainwin), x, y);
        gtk_window_resize (GTK_WINDOW (mainwin), w, h);
        if (deadbeef->conf_get_int ("mainwin.geometry.maximized", 0)) {
            gtk_window_maximize (GTK_WINDOW (mainwin));
        }
    }

    gtkui_on_configchanged (NULL, 0);
    gtkui_init_theme_colors ();

    // visibility of statusbar and headers
    GtkWidget *header_mi = lookup_widget (mainwin, "view_headers");
    GtkWidget *sb_mi = lookup_widget (mainwin, "view_status_bar");
    GtkWidget *ts_mi = lookup_widget (mainwin, "view_tabs");
    GtkWidget *eq_mi = lookup_widget (mainwin, "view_eq");
    GtkWidget *sb = lookup_widget (mainwin, "statusbar");
    GtkWidget *ts = lookup_widget (mainwin, "tabstrip");
    if (deadbeef->conf_get_int ("gtkui.statusbar.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), FALSE);
        gtk_widget_hide (sb);
    }
    if (deadbeef->conf_get_int ("gtkui.tabs.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (ts_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (ts_mi), FALSE);
        gtk_widget_hide (ts);
    }
    if (deadbeef->conf_get_int ("gtkui.eq.visible", 0)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (eq_mi), TRUE);
        eq_window_show ();
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (eq_mi), FALSE);
    }

    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));

#if 0
    // get saved scrollpos before creating listview, to avoid reset
    int curr = deadbeef->plt_get_curr ();
    char conf[100];
    snprintf (conf, sizeof (conf), "playlist.scroll.%d", curr);
    int scroll = deadbeef->conf_get_int (conf, 0);
#endif

    DdbListview *main_playlist = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    main_playlist_init (GTK_WIDGET (main_playlist));

    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (header_mi), TRUE);
        ddb_listview_show_header (main_playlist, 1);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (header_mi), FALSE);
        ddb_listview_show_header (main_playlist, 0);
    }

    DdbListview *search_playlist = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    search_playlist_init (GTK_WIDGET (search_playlist));

    progress_init ();
    cover_art_init ();
    gtk_widget_show (mainwin);

    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (gtkui_on_activate), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (gtkui_on_songchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_TRACKINFOCHANGED, DB_CALLBACK (gtkui_on_trackinfochanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PAUSED, DB_CALLBACK (gtkui_on_paused), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTCHANGED, DB_CALLBACK (gtkui_on_playlistchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, DB_CALLBACK (gtkui_on_frameupdate), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_VOLUMECHANGED, DB_CALLBACK (gtkui_on_volumechanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (gtkui_on_configchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_OUTPUTCHANGED, DB_CALLBACK (gtkui_on_outputchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTSWITCH, DB_CALLBACK (gtkui_on_playlistswitch), 0);

//    playlist_refresh ();
//    ddb_listview_set_vscroll (main_playlist, scroll);
    gtk_window_set_title (GTK_WINDOW (mainwin), "DeaDBeeF-" VERSION);
    gtk_initialized = 1;
    gtk_main ();
    cover_art_free ();
    eq_window_destroy ();
    gtk_widget_destroy (mainwin);
    gtk_widget_destroy (searchwin);
#if HAVE_NOTIFY
    notify_uninit ();
#endif
    gdk_threads_leave ();
}

static int
gtkui_start (void) {
    // find coverart plugin
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->id && !strcmp (p->id, "cover_loader")) {
            fprintf (stderr, "gtkui: found cover-art loader plugin\n");
            coverart_plugin = (DB_artwork_plugin_t *)p;
            break;
        }
    }

    // gtk must be running in separate thread
    gtk_initialized = 0;
    gtk_tid = deadbeef->thread_start (gtkui_thread, NULL);
    // wait until gtk finishes initializing
    while (!gtk_initialized) {
        usleep (10000);
    }

    return 0;
}

static gboolean
quit_gtk_cb (gpointer nothing) {
    gtk_main_quit ();
    return FALSE;
}

static int
gtkui_stop (void) {
    trace ("unsubscribing events\n");
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (gtkui_on_activate), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (gtkui_on_songchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_TRACKINFOCHANGED, DB_CALLBACK (gtkui_on_trackinfochanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PAUSED, DB_CALLBACK (gtkui_on_paused), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTCHANGED, DB_CALLBACK (gtkui_on_playlistchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, DB_CALLBACK (gtkui_on_frameupdate), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_VOLUMECHANGED, DB_CALLBACK (gtkui_on_volumechanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (gtkui_on_configchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_OUTPUTCHANGED, DB_CALLBACK (gtkui_on_outputchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTSWITCH, DB_CALLBACK (gtkui_on_playlistswitch), 0);
    trace ("quitting gtk\n");
    g_idle_add (quit_gtk_cb, NULL);
    trace ("waiting for gtk thread to finish\n");
    deadbeef->thread_join (gtk_tid);
    trace ("gtk thread finished\n");
    gtk_tid = 0;
    main_playlist_free ();
    trace ("gtkui_stop completed\n");
    return 0;
}

DB_plugin_t *
gtkui_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static const char settings_dlg[] =
    "property \"Run gtk_init with --sync (debug mode)\" checkbox gtkui.sync 0;\n"
#if HAVE_NOTIFY
    "property \"Enable OSD notifications\" checkbox gtkui.notify.enable 0;\n"
    "property \"Notification format\" entry gtkui.notify.format \"" NOTIFY_DEFAULT_FORMAT "\";\n"
#endif
;

// define plugin interface
static DB_gui_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.nostop = 1,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.name = "Standard GTK2 user interface",
    .plugin.descr = "Default DeaDBeeF GUI",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = gtkui_start,
    .plugin.stop = gtkui_stop,
    .plugin.configdialog = settings_dlg,
};
