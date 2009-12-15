/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include <string.h>
#include "gtkplaylist.h"
#include "search.h"
#include "progress.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_gui_t plugin;
DB_functions_t *deadbeef;

// main widgets
GtkWidget *mainwin;
GtkWidget *searchwin;
GtkStatusIcon *trayicon;
GtkWidget *traymenu;

// playlist configuration structures
gtkplaylist_t main_playlist;
gtkplaylist_t search_playlist;

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];
static float last_songpos = -1;

static gboolean
update_songinfo (gpointer ctx) {
    char sbtext_new[512] = "-";
    float songpos = last_songpos;

    float pl_totaltime = deadbeef->pl_get_totaltime ();
    int daystotal = (int)pl_totaltime / (3600*24);
    int hourtotal = ((int)pl_totaltime / 3600) % 24;
    int mintotal = ((int)pl_totaltime/60) % 60;
    int sectotal = ((int)pl_totaltime) % 60;

    char totaltime_str[512] = "";
    if (daystotal == 0)
        snprintf (totaltime_str, sizeof (totaltime_str), "%d:%02d:%02d", hourtotal, mintotal, sectotal);

    else if (daystotal == 1)
        snprintf (totaltime_str, sizeof (totaltime_str), "1 day %d:%02d:%02d", hourtotal, mintotal, sectotal);

    else
        snprintf (totaltime_str, sizeof (totaltime_str), "%d days %d:%02d:%02d", daystotal, hourtotal, mintotal, sectotal);



    DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
    float duration = deadbeef->pl_get_item_duration (track);

    if (deadbeef->get_output ()->state () == OUTPUT_STATE_STOPPED) {
        snprintf (sbtext_new, sizeof (sbtext_new), "Stopped | %d tracks | %s total playtime", deadbeef->pl_getcount (), totaltime_str);
        songpos = 0;
    }
    else if (track->decoder) {
//        codec_lock ();
        DB_decoder_t *c = track->decoder;
        float playpos = deadbeef->streamer_get_playpos ();
        int minpos = playpos / 60;
        int secpos = playpos - minpos * 60;
        int mindur = duration / 60;
        int secdur = duration - mindur * 60;

        const char *mode = c->info.channels == 1 ? "Mono" : "Stereo";
        int samplerate = c->info.samplerate;
        int bitspersample = c->info.bps;
        songpos = playpos;
//        codec_unlock ();

        char t[100];
        if (duration >= 0) {
            snprintf (t, sizeof (t), "%d:%02d", mindur, secdur);
        }
        else {
            strcpy (t, "-:--");
        }

        char sbitrate[20] = "";
#if 0 // NOTE: do not enable that for stable branch yet
        int bitrate = streamer_get_bitrate ();
        if (bitrate > 0) {
            snprintf (sbitrate, sizeof (sbitrate), "%d kbps ", bitrate);
        }
#endif
        const char *spaused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED ? "Paused | " : "";
        snprintf (sbtext_new, sizeof (sbtext_new), "%s%s %s| %dHz | %d bit | %s | %d:%02d / %s | %d tracks | %s total playtime", spaused, track->filetype ? track->filetype:"-", sbitrate, samplerate, bitspersample, mode, minpos, secpos, t, deadbeef->pl_getcount (), totaltime_str);
    }

    if (strcmp (sbtext_new, sb_text)) {
        strcpy (sb_text, sbtext_new);

        // form statusline
        GDK_THREADS_ENTER();
        // FIXME: don't update if window is not visible
        GtkStatusbar *sb = GTK_STATUSBAR (lookup_widget (mainwin, "statusbar"));
        if (sb_context_id == -1) {
            sb_context_id = gtk_statusbar_get_context_id (sb, "msg");
        }

        gtk_statusbar_pop (sb, sb_context_id);
        gtk_statusbar_push (sb, sb_context_id, sb_text);

        GDK_THREADS_LEAVE();
    }

    if (songpos != last_songpos) {
        void seekbar_draw (GtkWidget *widget);
        void seekbar_expose (GtkWidget *widget, int x, int y, int w, int h);
        if (mainwin) {
            GtkWidget *widget = lookup_widget (mainwin, "seekbar");
            // translate volume to seekbar pixels
            songpos /= duration;
            songpos *= widget->allocation.width;
            if (songpos != last_songpos) {
                GDK_THREADS_ENTER();
                seekbar_draw (widget);
                seekbar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
                GDK_THREADS_LEAVE();
                last_songpos = songpos;
            }
        }
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
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    volumebar_draw (volumebar);
    volumebar_expose (volumebar, 0, 0, volumebar->allocation.width, volumebar->allocation.height);
    return FALSE;
}

#if GTK_MINOR_VERSION<=14

gboolean
on_trayicon_activate (GtkWidget       *widget,
                                        GdkEvent  *event,
                                        gpointer         user_data)
{
    if (GTK_WIDGET_VISIBLE (mainwin)) {
        gtk_widget_hide (mainwin);
    }
    else {
        int x = deadbeef->conf_get_int ("mainwin.geometry.x", 40);
        int y = deadbeef->conf_get_int ("mainwin.geometry.y", 40);
        int w = deadbeef->conf_get_int ("mainwin.geometry.w", 500);
        int h = deadbeef->conf_get_int ("mainwin.geometry.h", 300);
        printf ("restore: %d %d %d %d\n", x, y, w, h);
        gtk_widget_show (mainwin);
        gtk_window_move (mainwin, x, y);
        gtk_window_resize (mainwin, w, h);
        if (deadbeef->conf_get_int ("mainwin.geometry.maximized", 0)) {
            gtk_window_maximize (GTK_WINDOW (mainwin));
        }
        gtk_window_present (GTK_WINDOW (mainwin));
    }
    return FALSE;
}

#else

gboolean
on_trayicon_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 1) {
        if (GTK_WIDGET_VISIBLE (mainwin)) {
            gtk_widget_hide (mainwin);
        }
        else {
            gtk_widget_show (mainwin);
            int x = deadbeef->conf_get_int ("mainwin.geometry.x", 40);
            int y = deadbeef->conf_get_int ("mainwin.geometry.y", 40);
            int w = deadbeef->conf_get_int ("mainwin.geometry.w", 500);
            int h = deadbeef->conf_get_int ("mainwin.geometry.h", 300);
            gtk_window_move (GTK_WINDOW (mainwin), x, y);
            gtk_window_resize (GTK_WINDOW (mainwin), w, h);
            if (deadbeef->conf_get_int ("mainwin.geometry.maximized", 0)) {
                gtk_window_maximize (GTK_WINDOW (mainwin));
            }
            gtk_window_present (GTK_WINDOW (mainwin));
        }
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

void
guiplug_showwindow (void) {
    GDK_THREADS_ENTER();
    gtk_widget_show (mainwin);
    gtk_window_present (GTK_WINDOW (mainwin));
    GDK_THREADS_LEAVE();
}

void
guiplug_shutdown (void) {
    GDK_THREADS_ENTER();
    gtk_widget_hide (mainwin);
    gtk_main_quit ();
    GDK_THREADS_LEAVE();
}


static int
gtkui_on_activate (DB_event_t *ev, uintptr_t data) {
    GDK_THREADS_ENTER();
    gtk_widget_show (mainwin);
    gtk_window_present (GTK_WINDOW (mainwin));
    GDK_THREADS_LEAVE();
}

static int
gtkui_on_songchanged (DB_event_trackchange_t *ev, uintptr_t data) {
    gtkpl_songchanged_wrapper (ev->from, ev->to);
}

static int
gtkui_on_trackinfochanged (DB_event_track_t *ev, uintptr_t data) {
    GDK_THREADS_ENTER();
    gtkpl_redraw_pl_row (&main_playlist, ev->index, ev->track);
    if (ev->track == deadbeef->pl_getcurrent ()) {
        gtkpl_current_track_changed (ev->track);
    }
    GDK_THREADS_LEAVE();
}

static int
gtkui_on_paused (DB_event_state_t *ev, uintptr_t data) {
    GDK_THREADS_ENTER();
    DB_playItem_t *curr = deadbeef->pl_getcurrent ();
    if (curr) {
        int idx = deadbeef->pl_get_idx_of (curr);
        gtkpl_redraw_pl_row (&main_playlist, idx, curr);
    }
    GDK_THREADS_LEAVE();
}

static int
gtkui_on_playlistchanged (DB_event_t *ev, uintptr_t data) {
    GDK_THREADS_ENTER();
    playlist_refresh ();
    search_refresh ();
    GDK_THREADS_LEAVE();
}

static int
gtkui_on_frameupdate (DB_event_t *ev, uintptr_t data) {
    g_idle_add (update_songinfo, NULL);
}

static int
gtkui_on_volumechanged (DB_event_t *ev, uintptr_t data) {
    void volumebar_notify_changed (void); // FIXME: do it properly
    volumebar_notify_changed ();
}

void
gtkui_thread (void *ctx) {
    // let's start some gtk
    g_thread_init (NULL);
    add_pixmap_directory (PREFIX "/share/deadbeef/pixmaps");
    gdk_threads_init ();
    gdk_threads_enter ();
    gtk_set_locale ();
    gtk_init (0, NULL);

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
    // order and looping
    const char *orderwidgets[3] = { "order_linear", "order_shuffle", "order_random" };
    const char *loopingwidgets[3] = { "loop_all", "loop_disable", "loop_single" };
    const char *w;
    w = orderwidgets[deadbeef->conf_get_int ("playback.order", 0)];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);
    w = loopingwidgets[deadbeef->conf_get_int ("playback.loop", 0)];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "scroll_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.followplayback", 0) ? TRUE : FALSE);
    // visibility of statusbar and headers
    GtkWidget *header_mi = lookup_widget (mainwin, "view_headers");
    GtkWidget *sb_mi = lookup_widget (mainwin, "view_status_bar");
    GtkWidget *header = lookup_widget (mainwin, "header");
    GtkWidget *sb = lookup_widget (mainwin, "statusbar");
    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (header_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (header_mi), FALSE);
        gtk_widget_hide (header);
    }
    if (deadbeef->conf_get_int ("gtkui.statusbar.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (sb_mi), FALSE);
        gtk_widget_hide (sb);
    }

    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));
    extern void main_playlist_init (GtkWidget *widget);
    main_playlist_init (lookup_widget (mainwin, "playlist"));
    extern void search_playlist_init (GtkWidget *widget);
    search_playlist_init (lookup_widget (searchwin, "searchlist"));

    progress_init ();
    gtk_widget_show (mainwin);

    gtk_main ();
    gdk_threads_leave ();
}

static int
gtkui_start (void) {
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (gtkui_on_activate), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (gtkui_on_songchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_TRACKINFOCHANGED, DB_CALLBACK (gtkui_on_trackinfochanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PAUSED, DB_CALLBACK (gtkui_on_paused), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTCHANGED, DB_CALLBACK (gtkui_on_playlistchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, DB_CALLBACK (gtkui_on_frameupdate), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_VOLUMECHANGED, DB_CALLBACK (gtkui_on_volumechanged), 0);
    // gtk must be running in separate thread
    deadbeef->thread_start (gtkui_thread, NULL);

    return 0;
}

static int
gtkui_stop (void) {
    GDK_THREADS_ENTER();
    gtk_main_quit ();
    GDK_THREADS_LEAVE();
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (gtkui_on_activate), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (gtkui_on_songchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_TRACKINFOCHANGED, DB_CALLBACK (gtkui_on_trackinfochanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PAUSED, DB_CALLBACK (gtkui_on_paused), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTCHANGED, DB_CALLBACK (gtkui_on_playlistchanged), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, DB_CALLBACK (gtkui_on_frameupdate), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_VOLUMECHANGED, DB_CALLBACK (gtkui_on_volumechanged), 0);
    gtkpl_free (&main_playlist);
    gtkpl_free (&search_playlist);
    return 0;
}

DB_plugin_t *
gtkui_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// define plugin interface
static DB_gui_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.nostop = 1,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.name = "Standard GTK2 user interface",
    .plugin.descr = "",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = gtkui_start,
    .plugin.stop = gtkui_stop
};
