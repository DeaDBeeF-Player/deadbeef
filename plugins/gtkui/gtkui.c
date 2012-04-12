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
#include "../../gettext.h"
#include "gtkui.h"
#include "ddblistview.h"
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
#include "actions.h"
#include "pluginconf.h"
#include "gtkui_api.h"
#include "wingeom.h"
#include "widgets.h"

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

int gtkui_embolden_current_track;

#define TRAY_ICON "deadbeef_tray_icon"

// that must be called before gtk_init
void
gtkpl_init (void) {
    theme_treeview = gtk_tree_view_new ();
    gtk_widget_set_can_focus (theme_treeview, FALSE);
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
        const char *filetype = deadbeef->pl_find_meta (track, ":FILETYPE");
        if (!filetype) {
            filetype = "-";
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
#if !GTK_CHECK_VERSION(2,16,0) || defined(ULTRA_COMPATIBLE)
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

#if !GTK_CHECK_VERSION(2,14,0) || defined(ULTRA_COMPATIBLE)

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
//    DdbListview *ps = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
//    ddb_listview_refresh (ps, DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
    search_refresh ();
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

#if !GTK_CHECK_VERSION(2,14,0) || defined(ULTRA_COMPATIBLE)
    g_signal_connect ((gpointer)trayicon, "activate", G_CALLBACK (on_trayicon_activate), NULL);
#else
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
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "scroll_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.followplayback", 0) ? TRUE : FALSE);

    // cursor follows playback
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "cursor_follows_playback")), deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0) ? TRUE : FALSE);

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

char last_playlist_save_name[1024] = "";

void
save_playlist_as (void) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Save Playlist As"), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg), TRUE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "untitled.dbpl");
    // restore folder
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast ("filechooser.playlist.lastdir", ""));
    deadbeef->conf_unlock ();

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("DeaDBeeF playlist files (*.dbpl)"));
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    DB_playlist_t **plug = deadbeef->plug_get_playlist_list ();
    for (int i = 0; plug[i]; i++) {
        if (plug[i]->extensions && plug[i]->load) {
            const char **exts = plug[i]->extensions;
            if (exts && plug[i]->save) {
                for (int e = 0; exts[e]; e++) {
                    char s[100];
                    flt = gtk_file_filter_new ();
                    gtk_file_filter_set_name (flt, exts[e]);
                    snprintf (s, sizeof (s), "*.%s", exts[e]);
                    gtk_file_filter_add_pattern (flt, s);
                    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
                }
            }
        }
    }

    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.playlist.lastdir", folder);
        g_free (folder);
    }
    if (res == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);

        if (fname) {
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                int res = deadbeef->plt_save (plt, NULL, NULL, fname, NULL, NULL, NULL);
                if (res >= 0 && strlen (fname) < 1024) {
                    strcpy (last_playlist_save_name, fname);
                }
                deadbeef->plt_unref (plt);
            }
            g_free (fname);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

void
on_playlist_save_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
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


void
on_playlist_save_as_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    save_playlist_as ();
}

static gboolean
playlist_filter_func (const GtkFileFilterInfo *filter_info, gpointer data) {
    // get ext
    const char *p = strrchr (filter_info->filename, '.');
    if (!p) {
        return FALSE;
    }
    p++;
    DB_playlist_t **plug = deadbeef->plug_get_playlist_list ();
    for (int i = 0; plug[i]; i++) {
        if (plug[i]->extensions && plug[i]->load) {
            const char **exts = plug[i]->extensions;
            if (exts) {
                for (int e = 0; exts[e]; e++) {
                    if (!strcasecmp (exts[e], p)) {
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

void
load_playlist_thread (void *data) {
    char *fname = data;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_clear (plt);
        int abort = 0;
        DB_playItem_t *it = deadbeef->plt_load (plt, NULL, fname, &abort, NULL, NULL);
        if (it) {
            deadbeef->pl_item_unref (it);
        }
        deadbeef->plt_unref (plt);
    }
    g_free (fname);
    gtkui_playlist_changed ();
}

void
on_playlist_load_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Load Playlist"), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    // restore folder
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast ("filechooser.playlist.lastdir", ""));
    deadbeef->conf_unlock ();

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Supported playlist formats");
    gtk_file_filter_add_custom (flt, GTK_FILE_FILTER_FILENAME, playlist_filter_func, NULL, NULL);
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("Other files (*)"));
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.playlist.lastdir", folder);
        g_free (folder);
    }
    if (res == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (fname) {
            uintptr_t tid = deadbeef->thread_start (load_playlist_thread, fname);
            deadbeef->thread_detach (tid);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
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
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (!deadbeef->pl_add_files_begin (plt)) {
                    deadbeef->plt_add_file (plt, text, NULL, NULL);
                    deadbeef->pl_add_files_end ();
                    playlist_refresh ();
                }
                if (plt) {
                    deadbeef->plt_unref (plt);
                }
            }
        }
    }
    gtk_widget_destroy (dlg);
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

int
gtkui_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
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
    }
    return 0;
}

void
gtkui_thread (void *ctx) {
    XInitThreads (); // gtkglext/xcb doesn't work without this
    // let's start some gtk
    g_thread_init (NULL);
//    add_pixmap_directory (PREFIX "/share/deadbeef/pixmaps");
    add_pixmap_directory (deadbeef->get_pixmap_dir ());
    gdk_threads_init ();
    gdk_threads_enter ();

    int argc = 2;
    const char **argv = alloca (sizeof (char *) * argc);
    argv[0] = "deadbeef";
    argv[1] = "--sync";
    //argv[1] = "--g-fatal-warnings";
    if (!deadbeef->conf_get_int ("gtkui.sync", 0)) {
        argc = 1;
    }
    gtk_disable_setlocale ();
    gtk_init (&argc, (char ***)&argv);

    // register widget types
    w_reg_widget ("tabbed_playlist", _("Playlist with tabs"), w_tabbed_playlist_create);
    w_reg_widget ("box", NULL, w_box_create);
    w_reg_widget ("vsplitter", _("Splitter (top and bottom)"), w_vsplitter_create);
    w_reg_widget ("hsplitter", _("Splitter (left and right)"), w_hsplitter_create);
    w_reg_widget ("placeholder", NULL, w_placeholder_create);
    w_reg_widget ("tabs", _("Tabs"), w_tabs_create);
    w_reg_widget ("tabstrip", _("Playlist tabs"), w_tabstrip_create);
    w_reg_widget ("playlist", _("Playlist"), w_playlist_create);
    w_reg_widget ("selproperties", _("Selection properties"), w_selproperties_create);
    w_reg_widget ("coverart", _("Album art display"), w_coverart_create);
    w_reg_widget ("scope", _("Scope"), w_scope_create);

    mainwin = create_mainwin ();

    // construct mainwindow widgets
    {

        w_init ();
        ddb_gtkui_widget_t *rootwidget = w_get_rootwidget ();
        gtk_widget_show (rootwidget->widget);
        gtk_box_pack_start (GTK_BOX(lookup_widget(mainwin, "plugins_bottom_vbox")), rootwidget->widget, TRUE, TRUE, 0);

        // load layout
        char layout[4000];
        deadbeef->conf_get_str ("gtkui.layout", "tabbed_playlist { }", layout, sizeof (layout));

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
    GtkWidget *header_mi = lookup_widget (mainwin, "view_headers");
    GtkWidget *sb_mi = lookup_widget (mainwin, "view_status_bar");
    GtkWidget *ts_mi = lookup_widget (mainwin, "view_tabs");
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
    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));

//    DdbListview *main_playlist = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
//    main_playlist_init (GTK_WIDGET (main_playlist));
    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (header_mi), TRUE);
    }
    else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (header_mi), FALSE);
    }

    DdbListview *search_playlist = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    search_playlist_init (GTK_WIDGET (search_playlist));

    progress_init ();
    cover_art_init ();
    add_mainmenu_actions (lookup_widget (mainwin, "menubar1"));

    gtk_widget_show (mainwin);

    gtkui_setup_gui_refresh ();

    char fmt[500];
    char str[600];
    deadbeef->conf_get_str ("gtkui.titlebar_stopped", "DeaDBeeF-%V", fmt, sizeof (fmt));
    deadbeef->pl_format_title (NULL, -1, str, sizeof (str), -1, fmt);
    gtk_window_set_title (GTK_WINDOW (mainwin), str);
    gtk_initialized = 1;

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
    draw_free ();
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
    gdk_threads_leave ();
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

int (*gtkui_original_plt_add_dir) (ddb_playlist_t *plt, const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
int (*gtkui_original_plt_add_file) (ddb_playlist_t *plt, const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
int (*gtkui_original_pl_add_files_begin) (ddb_playlist_t *plt);
void (*gtkui_original_pl_add_files_end) (void);

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

static int
gtkui_start (void) {
    fprintf (stderr, "gtkui plugin compiled for gtk version: %d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);

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
    .gui.plugin.api_vminor = 0,
    .gui.plugin.version_major = 1,
    .gui.plugin.version_minor = 0,
    .gui.plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .gui.plugin.id = "gtkui3",
    .gui.plugin.name = "GTK3 user interface",
    .gui.plugin.descr = "User interface using GTK+ 3.x",
#else
    .gui.plugin.id = "gtkui",
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
    .get_mainwin = gtkui_get_mainwin,
    .w_reg_widget = w_reg_widget,
    .w_unreg_widget = w_unreg_widget,
    .w_is_registered = w_is_registered,
    .w_get_rootwidget = w_get_rootwidget,
    .w_create = w_create,
    .w_set_name = w_set_name,
    .w_destroy = w_destroy,
    .w_append = w_append,
    .w_replace = w_replace,
    .w_remove = w_remove,
    .api_version = GTKUI_API_VERSION,
};
