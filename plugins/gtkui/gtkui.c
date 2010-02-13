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
#include "gtkui.h"
#include "gtkplaylist.h"
#include "search.h"
#include "progress.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include "tabs.h"
#include "parser.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_gui_t plugin;
DB_functions_t *deadbeef;

static intptr_t gtk_tid;

// main widgets
GtkWidget *mainwin;
GtkWidget *searchwin;
GtkStatusIcon *trayicon;
GtkWidget *traymenu;

// playlist configuration structures
//DdbListview main_playlist;
//DdbListview search_playlist;

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];
static float last_songpos = -1;
static char sbitrate[20] = "";
static struct timeval last_br_update;

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
    DB_fileinfo_t *c = deadbeef->streamer_get_current_fileinfo ();

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

static int
gtkui_on_songchanged (DB_event_trackchange_t *ev, uintptr_t data) {
    gtkpl_songchanged_wrapper (ev->from, ev->to);
    return 0;
}

struct trackinfo_t {
    int index;
    DB_playItem_t *track;
};

static void
current_track_changed (DB_playItem_t *it) {
    char str[600];
    if (it) {
        char dname[512];
        deadbeef->pl_format_item_display_name (it, dname, 512);
        snprintf (str, sizeof (str), "DeaDBeeF - %s", dname);
    }
    else {
        strcpy (str, "DeaDBeeF");
    }
    gtk_window_set_title (GTK_WINDOW (mainwin), str);
    set_tray_tooltip (str);
}

static gboolean
trackinfochanged_cb (gpointer data) {
    struct trackinfo_t *ti = (struct trackinfo_t *)data;
    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
//    gtkpl_redraw_pl_row (&main_playlist, ti->index, ti->track);
    ddb_listview_draw_row (DDB_LISTVIEW (playlist), ti->index, (DdbListviewIter)ti->track);
    if (ti->track == deadbeef->pl_getcurrent ()) {
        current_track_changed (ti->track);
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
    DB_playItem_t *curr = deadbeef->pl_getcurrent ();
    if (curr) {
        int idx = deadbeef->pl_get_idx_of (curr);
        GtkWidget *playlist = lookup_widget (mainwin, "playlist");
        //gtkpl_redraw_pl_row (&main_playlist, idx, curr);
        ddb_listview_draw_row (DDB_LISTVIEW (playlist), idx, (DdbListviewIter)curr);
    }
    return FALSE;
}

static int
gtkui_on_paused (DB_event_state_t *ev, uintptr_t data) {
    g_idle_add (paused_cb, NULL);
}

static gboolean
playlistchanged_cb (gpointer none) {
    playlist_refresh ();
    search_refresh ();
    return FALSE;
}

static int
gtkui_on_playlistchanged (DB_event_t *ev, uintptr_t data) {
    g_idle_add (playlistchanged_cb, NULL);
}

static gboolean
playlistswitch_cb (gpointer none) {
    playlist_refresh ();
    search_refresh ();
    tabbar_draw (lookup_widget (mainwin, "tabbar"));
    //GtkWidget *widget = lookup_widget (mainwin, "tabbar");
    //gdk_window_invalidate_rect (widget->window, NULL, FALSE);
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
    void volumebar_notify_changed (void); // FIXME: do it properly
    volumebar_notify_changed ();
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

/////// column management code

typedef struct gtkpl_column_s {
    char *title;
    int id; // id is faster than format, set to -1 to use format
    char *format;
    int width;
    int movepos; // valid only while `moving' is 1
    struct gtkpl_column_s *next;
    unsigned align_right : 1;
    unsigned sort_order : 2; // 0=none, 1=asc, 2=desc
} gtkpl_column_t;

static gtkpl_column_t *main_columns = NULL;
static gtkpl_column_t *search_columns = NULL;

gtkpl_column_t * 
gtkpl_column_alloc (const char *title, int width, int id, const char *format, int align_right) {
    gtkpl_column_t * c = malloc (sizeof (gtkpl_column_t));
    memset (c, 0, sizeof (gtkpl_column_t));
    c->title = strdup (title);
    c->id = id;
    c->format = format ? strdup (format) : NULL;
    c->width = width;
    c->align_right = align_right;
    return c;
}

void
gtkpl_column_append (gtkpl_column_t **head, gtkpl_column_t * c) {
    int idx = 0;
    gtkpl_column_t * columns = *head;
    if (columns) {
        idx++;
        gtkpl_column_t * tail = *head;
        while (tail->next) {
            tail = tail->next;
            idx++;
        }
        tail->next = c;
    }
    else {
        *head = c;
    }
//    gtkpl_column_update_config (pl, c, idx);
}

void
gtkpl_column_insert_before (gtkpl_column_t **head, gtkpl_column_t * before, gtkpl_column_t * c) {
    if (*head) {
        gtkpl_column_t * prev = NULL;
        gtkpl_column_t * next = *head;
        while (next) {
            if (next == before) {
                break;
            }
            prev = next;
            next = next->next;
        }
        c->next = next;
        if (prev) {
            prev->next = c;
        }
        else {
            *head  = c;
        }
//        gtkpl_column_rewrite_config (pl);
    }
    else {
        *head = c;
 //       gtkpl_column_update_config (pl, c, 0);
    }
}

void
gtkpl_column_free (gtkpl_column_t * c) {
    if (c->title) {
        free (c->title);
    }
    if (c->format) {
        free (c->format);
    }
    free (c);
}

void
gtkpl_column_remove (gtkpl_column_t **head, gtkpl_column_t * c) {
    if (*head == c) {
        *head = (*head)->next;
        gtkpl_column_free (c);
        return;
    }
    gtkpl_column_t * cc = *head;
    while (cc) {
        if (cc->next == c) {
            cc->next = cc->next->next;
            gtkpl_column_free (c);
            return;
        }
        cc = cc->next;
    }

    if (!cc) {
        trace ("gtkpl: attempted to remove column that is not in list\n");
    }
}

void
gtkpl_append_column_from_textdef (gtkpl_column_t **head, const uint8_t *def) {
    // syntax: "title" "format" id width alignright
    char token[MAX_TOKEN];
    const char *p = def;
    char title[MAX_TOKEN];
    int id;
    char fmt[MAX_TOKEN];
    int width;
    int align;

    parser_init ();

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    strcpy (title, token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    strcpy (fmt, token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    id = atoi (token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    width = atoi (token);

    p = gettoken_warn_eof (p, token);
    if (!p) {
        return;
    }
    align = atoi (token);

    gtkpl_column_append (head, gtkpl_column_alloc (title, width, id, fmt, align));
}

void
gtkpl_column_update_config (const char *title, gtkpl_column_t * c, int idx) {
    char key[128];
    char value[128];
    snprintf (key, sizeof (key), "%s.column.%d", title, idx);
    snprintf (value, sizeof (value), "\"%s\" \"%s\" %d %d %d", c->title, c->format ? c->format : "", c->id, c->width, c->align_right);
    deadbeef->conf_set_str (key, value);
}

void
gtkpl_column_rewrite_config (gtkpl_column_t *head, const char *title) {
    char key[128];
    snprintf (key, sizeof (key), "%s.column.", title);
    deadbeef->conf_remove_items (key);

    gtkpl_column_t * c;
    int i = 0;
    for (c = head; c; c = c->next, i++) {
        gtkpl_column_update_config (title, c, i);
    }
}

/////// end of column management code

static int
main_get_count (void) {
    return deadbeef->pl_getcount (PL_MAIN);
}

static int
main_get_cursor (void) {
    return deadbeef->pl_get_cursor (PL_MAIN);
}
static DdbListviewIter main_head (void) {
    return (DdbListviewIter)deadbeef->pl_get_first (PL_MAIN);
}

static DdbListviewIter main_tail (void) {
    return (DdbListviewIter)deadbeef->pl_get_last(PL_MAIN);
}

static DdbListviewIter main_next (DdbListviewIter it) {
    return (DdbListviewIter)deadbeef->pl_get_next(it, PL_MAIN);
}

static DdbListviewIter main_prev (DdbListviewIter it) {
    return (DdbListviewIter)deadbeef->pl_get_prev(it, PL_MAIN);
}

static DdbListviewIter main_get_for_idx (int idx) {
    return deadbeef->pl_get_for_idx_and_iter (idx, PL_MAIN);
}

static int main_get_idx (DdbListviewIter it) {
    DB_playItem_t *c = deadbeef->pl_get_first (PL_MAIN);
    int idx = 0;
    while (c && c != it) {
        DB_playItem_t *next = deadbeef->pl_get_next (c, PL_MAIN); 
        deadbeef->pl_item_unref (c);
        c = next;
        idx++;
    }
    if (!c) {
        return -1;
    }
    deadbeef->pl_item_unref (c);
    return idx;
}

static int main_col_count (void) {
    int cnt = 0;
    for (gtkpl_column_t *c = main_columns; c; c = c->next, cnt++);
    return cnt;
}

DdbListviewColIter main_col_first (void) {
    return (DdbListviewColIter)main_columns;
}
DdbListviewColIter main_col_next (DdbListviewColIter c) {
    return (DdbListviewColIter)((gtkpl_column_t *)c)->next;
}

static const char *main_col_get_title (DdbListviewColIter c) {
    return ((gtkpl_column_t *)c)->title;
}

int main_col_get_width (DdbListviewColIter c) {
    return ((gtkpl_column_t *)c)->width;
}

int main_col_get_justify (DdbListviewColIter c) {
    return ((gtkpl_column_t *)c)->align_right;
}

int main_col_get_sort (DdbListviewColIter c) {
    return ((gtkpl_column_t *)c)->sort_order;
}

gboolean
playlist_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer unused)
{
    GtkWidget *pl = lookup_widget (mainwin, "playlist");
    DB_playItem_t *item = (DB_playItem_t *)ddb_listview_get_iter_from_coord (DDB_LISTVIEW (pl), 0, y);
    if (item && item->fname) {
        gtk_tooltip_set_text (tooltip, item->fname);
        return TRUE;
    }
    return FALSE;
}

void main_col_sort (DdbListviewColIter c) {
}

void
columns_free (gtkpl_column_t **head) {
    while (*head) {
        DdbListviewColIter next = (*head)->next;
        gtkpl_column_free (*head);
        *head = next;
    }
}

void
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}


void
on_edit_column_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}


void
on_remove_column_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}

void
add_column_dlg (DdbListview *listview, const char *title, gtkpl_column_t **head, gtkpl_column_t *active_column) {
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_window_set_title (GTK_WINDOW (dlg), "Add column");
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), 0);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        if (id >= DB_COLUMN_ID_MAX) {
            id = -1;
        }
        gtkpl_column_insert_before (head, active_column, gtkpl_column_alloc (title, 100, id, format, align));
        gtkpl_column_rewrite_config (*head, title);
        ddb_listview_refresh (listview, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
    }
    gtk_widget_destroy (dlg);
}

void
edit_column_dlg (DdbListview *listview, const char *title, gtkpl_column_t **head, gtkpl_column_t *active_column) {
    if (!active_column)
        return;
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_window_set_title (GTK_WINDOW (dlg), "Edit column");
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), active_column->title);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "format")), active_column->format);
    if (active_column->id == -1) {
        gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), DB_COLUMN_ID_MAX);
    }
    else {
        gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), active_column->id);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), active_column->align_right);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        if (id >= DB_COLUMN_ID_MAX) {
            id = -1;
        }
        free (active_column->title);
        free (active_column->format);
        active_column->title = strdup (title);
        active_column->format = strdup (format);
        active_column->id = id;
        active_column->align_right = align;
        gtkpl_column_rewrite_config (*head, title);

        ddb_listview_refresh (listview, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
    }
    gtk_widget_destroy (dlg);
}

void
remove_column_dlg (DdbListview *listview, const char *title, gtkpl_column_t **head, gtkpl_column_t *active_column) {
    if (!active_column)
        return;

    gtkpl_column_remove (head, active_column);
    gtkpl_column_rewrite_config (*head, title);

    ddb_listview_refresh (listview, DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL | DDB_EXPOSE_LIST | DDB_EXPOSE_COLUMNS);
}

void main_add_column (DdbListviewColIter c) {
    add_column_dlg (DDB_LISTVIEW(lookup_widget(mainwin,"playlist")), "playlist", &main_columns, (gtkpl_column_t*)c);
}

void main_edit_column (DdbListviewColIter c) {
    edit_column_dlg (DDB_LISTVIEW(lookup_widget(mainwin,"playlist")), "playlist", &main_columns, (gtkpl_column_t*)c);
}

void main_remove_column (DdbListviewColIter c) {
    remove_column_dlg (DDB_LISTVIEW(lookup_widget(mainwin,"playlist")), "playlist", &main_columns, (gtkpl_column_t*)c);
}

void handle_playlist_doubleclick (int list_idx, DB_playItem_t *it, int idx) {
#if 0
    int r = deadbeef->pl_get_idx_of (it);
    int prev = deadbeef->pl_get_cursor (list_idx);
    if (prev != r) {
        deadbeef->pl_set_cursor (list_idx, r);
        if (prev != -1) {
            DB_playItem_t *prev_it = deadbeef->pl_get_for_idx (prev);
            gtkpl_redraw_pl_row (&main_playlist, prev, prev_it);
            UNREF (prev_it);
        }
        if (r != -1) {
            gtkpl_redraw_pl_row (&main_playlist, r, it);
        }
    }
    UNREF (it);
#endif
    deadbeef->sendmessage (M_PLAYSONGNUM, 0, idx, 0);
}

void main_handle_doubleclick (DdbListview *listview, DdbListviewIter *iter, int idx) {
    handle_playlist_doubleclick (PL_MAIN, (DB_playItem_t *)iter, idx);
}

DdbListviewBinding main_binding = {
    // rows
    .count = main_get_count,

    .cursor = main_get_cursor,

    .head = main_head,
    .tail = main_tail,
    .next = main_next,
    .prev = main_prev,

    .get_for_idx = main_get_for_idx,
    .get_idx = main_get_idx,

    //void (*draw_column_data) (GdkWindow *window, DdbListviewIter iter, int idx, int column, int x, int y);

    // columns
    .col_count = main_col_count,
    .col_first = main_col_first,
    .col_next = main_col_next,
    .col_get_title = main_col_get_title,
    .col_get_width = main_col_get_width,
    .col_get_justify = main_col_get_justify,
    .col_get_sort = main_col_get_sort,
    .col_sort = main_col_sort,

    // callbacks
    .edit_column = main_edit_column,
    .add_column = main_add_column,
    .remove_column = main_remove_column,
    .handle_doubleclick = main_handle_doubleclick,
};

void
main_playlist_init (GtkWidget *widget) {
    DdbListview *playlist = DDB_LISTVIEW(widget);
    main_binding.ref = (void (*) (DdbListviewIter))deadbeef->pl_item_ref;
    main_binding.unref = (void (*) (DdbListviewIter))deadbeef->pl_item_unref;
    main_binding.is_selected = (int (*) (DdbListviewIter))deadbeef->pl_is_selected;
    ddb_listview_set_binding (playlist, &main_binding);

#if 0
    // init playlist control structure, and put it into widget user-data
    memset (&main_playlist, 0, sizeof (main_playlist));
    main_playlist.title = "playlist";
    main_playlist.playlist = widget;
    main_playlist.header = lookup_widget (mainwin, "header");
    main_playlist.scrollbar = lookup_widget (mainwin, "playscroll");
    main_playlist.hscrollbar = lookup_widget (mainwin, "playhscroll");
//    main_playlist.pcurr = &playlist_current_ptr;
//    main_playlist.pcount = &pl_count;
    main_playlist.get_count = main_get_count;
    main_playlist.iterator = PL_MAIN;
//    main_playlist.multisel = 1;
    main_playlist.scrollpos = 0;
    main_playlist.hscrollpos = 0;
//    main_playlist.row = -1;
    main_playlist.clicktime = -1;
    main_playlist.nvisiblerows = 0;
#endif

    DB_conf_item_t *col = deadbeef->conf_find ("playlist.column.", NULL);
    if (!col) {
        // create default set of columns
        gtkpl_column_append (&main_columns, gtkpl_column_alloc ("Playing", 50, DB_COLUMN_PLAYING, NULL, 0));
        gtkpl_column_append (&main_columns, gtkpl_column_alloc ("Artist / Album", 150, DB_COLUMN_ARTIST_ALBUM, NULL, 0));
        gtkpl_column_append (&main_columns, gtkpl_column_alloc ("Track №", 50, DB_COLUMN_TRACK, NULL, 1));
        gtkpl_column_append (&main_columns, gtkpl_column_alloc ("Title / Track Artist", 150, DB_COLUMN_TITLE, NULL, 0));
        gtkpl_column_append (&main_columns, gtkpl_column_alloc ("Duration", 50, DB_COLUMN_DURATION, NULL, 0));
    }
    else {
        while (col) {
            gtkpl_append_column_from_textdef (&main_columns, col->value);
            col = deadbeef->conf_find ("playlist.column.", col);
        }
    }

//    gtk_object_set_data (GTK_OBJECT (main_playlist.playlist), "ps", &main_playlist);
//    gtk_object_set_data (GTK_OBJECT (main_playlist.header), "ps", &main_playlist);
//    gtk_object_set_data (GTK_OBJECT (main_playlist.scrollbar), "ps", &main_playlist);
//    gtk_object_set_data (GTK_OBJECT (main_playlist.hscrollbar), "ps", &main_playlist);

    // FIXME: filepath should be in properties dialog, while tooltip should be
    // used to show text that doesn't fit in column width
    if (deadbeef->conf_get_int ("playlist.showpathtooltip", 0)) {
        GValue value = {0, };
        g_value_init (&value, G_TYPE_BOOLEAN);
        g_value_set_boolean (&value, TRUE);
        g_object_set_property (G_OBJECT (widget), "has-tooltip", &value);
        g_signal_connect (G_OBJECT (widget), "query-tooltip", G_CALLBACK (playlist_tooltip_handler), NULL);
    }
}

static int
search_get_count (void) {
    return deadbeef->pl_getcount (PL_SEARCH);
}

void
search_playlist_init (GtkWidget *widget) {
#if 0
    extern GtkWidget *searchwin;
    // init playlist control structure, and put it into widget user-data
    memset (&search_playlist, 0, sizeof (search_playlist));
    search_playlist.title = "search";
    search_playlist.playlist = widget;
    search_playlist.header = lookup_widget (searchwin, "searchheader");
    search_playlist.scrollbar = lookup_widget (searchwin, "searchscroll");
    search_playlist.hscrollbar = lookup_widget (searchwin, "searchhscroll");
    assert (search_playlist.header);
    assert (search_playlist.scrollbar);
//    main_playlist.pcurr = &search_current;
//    search_playlist.pcount = &search_count;
    search_playlist.get_count = search_get_count;
//    search_playlist.multisel = 0;
    search_playlist.iterator = PL_SEARCH;
    search_playlist.scrollpos = 0;
    search_playlist.hscrollpos = 0;
//    search_playlist.row = -1;
    search_playlist.clicktime = -1;
    search_playlist.nvisiblerows = 0;

    // create default set of columns
    DB_conf_item_t *col = deadbeef->conf_find ("search.column.", NULL);
    if (!col) {
        gtkpl_column_append (&search_playlist, gtkpl_column_alloc ("Artist / Album", 150, DB_COLUMN_ARTIST_ALBUM, NULL, 0));
        gtkpl_column_append (&search_playlist, gtkpl_column_alloc ("Track №", 50, DB_COLUMN_TRACK, NULL, 1));
        gtkpl_column_append (&search_playlist, gtkpl_column_alloc ("Title / Track Artist", 150, DB_COLUMN_TITLE, NULL, 0));
        gtkpl_column_append (&search_playlist, gtkpl_column_alloc ("Duration", 50, DB_COLUMN_DURATION, NULL, 0));
    }
    else {
        while (col) {
            gtkpl_append_column_from_textdef (&search_playlist, col->value);
            col = deadbeef->conf_find ("search.column.", col);
        }
    }
    gtk_object_set_data (GTK_OBJECT (search_playlist.playlist), "ps", &search_playlist);
    gtk_object_set_data (GTK_OBJECT (search_playlist.header), "ps", &search_playlist);
    gtk_object_set_data (GTK_OBJECT (search_playlist.scrollbar), "ps", &search_playlist);
    gtk_object_set_data (GTK_OBJECT (search_playlist.hscrollbar), "ps", &search_playlist);
#endif
}


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
    main_playlist_init (lookup_widget (mainwin, "playlist"));
    search_playlist_init (lookup_widget (searchwin, "searchlist"));

    progress_init ();
    gtk_widget_show (mainwin);

    gtk_main ();
#if HAVE_NOTIFY
    notify_uninit ();
#endif
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
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (gtkui_on_configchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_OUTPUTCHANGED, DB_CALLBACK (gtkui_on_outputchanged), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_PLAYLISTSWITCH, DB_CALLBACK (gtkui_on_playlistswitch), 0);
    // gtk must be running in separate thread
    gtk_tid = deadbeef->thread_start (gtkui_thread, NULL);

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
    columns_free (&main_columns);
    columns_free (&search_columns);
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
