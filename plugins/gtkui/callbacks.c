/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h>
#include "../../gettext.h"

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "ddblistview.h"
#include "ddbtabstrip.h"
#include "ddbvolumebar.h"
#include "ddbseekbar.h"
#include "search.h"
#include "progress.h"
#include "gtkui.h"
#include "parser.h"
#include "drawing.h"
#include "eq.h"
#include "wingeom.h"
#include "widgets.h"

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define SELECTED(it) (deadbeef->pl_is_selected(it))
#define SELECT(it, sel) (deadbeef->pl_set_selected(it,sel))
#define VSELECT(it, sel) {deadbeef->pl_set_selected(it,sel);gtk_pl_redraw_item_everywhere (it);}
#define PL_NEXT(it, iter) (deadbeef->pl_get_next(it, iter))

DdbListview *last_playlist;
extern DB_functions_t *deadbeef; // defined in gtkui.c

static gboolean
file_filter_func (const GtkFileFilterInfo *filter_info, gpointer data) {
    // get ext
    const char *p = strrchr (filter_info->filename, '.');
    if (!p) {
        return FALSE;
    }
    p++;

    // get beginning of fname
    const char *fn = strrchr (filter_info->filename, '/');
    if (!fn) {
        fn = filter_info->filename;
    }
    else {
        fn++;
    }


    DB_decoder_t **codecs = deadbeef->plug_get_decoder_list ();
    for (int i = 0; codecs[i]; i++) {
        if (codecs[i]->exts && codecs[i]->insert) {
            const char **exts = codecs[i]->exts;
            for (int e = 0; exts[e]; e++) {
                if (!strcasecmp (exts[e], p)) {
                    return TRUE;
                }
            }
        }
        if (codecs[i]->prefixes && codecs[i]->insert) {
            const char **prefixes = codecs[i]->prefixes;
            for (int e = 0; prefixes[e]; e++) {
                if (!strncasecmp (prefixes[e], fn, strlen(prefixes[e])) && *(fn + strlen (prefixes[e])) == '.') {
                    return TRUE;
                }
            }
        }
    }
#if 0
    if (!strcasecmp (p, "pls")) {
        return TRUE;
    }
    if (!strcasecmp (p, "m3u")) {
        return TRUE;
    }
#endif

    // test container (vfs) formats
    DB_vfs_t **vfsplugs = deadbeef->plug_get_vfs_list ();
    for (int i = 0; vfsplugs[i]; i++) {
        if (vfsplugs[i]->is_container) {
            if (vfsplugs[i]->is_container (filter_info->filename)) {
                return TRUE;
            }
        }
    }


    return FALSE;
}

static GtkFileFilter *
set_file_filter (GtkWidget *dlg, const char *name) {
    if (!name) {
        name = _("Supported sound formats");
    }

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, name);

    gtk_file_filter_add_custom (flt, GTK_FILE_FILTER_FILENAME, file_filter_func, NULL, NULL);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("All files (*)"));
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    return flt;
}

void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Open file(s)..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    set_file_filter (dlg, NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);
    // restore folder
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast ("filechooser.lastdir", ""));
    deadbeef->conf_unlock ();
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
    }
    if (response == GTK_RESPONSE_OK)
    {
        deadbeef->pl_clear ();
        GSList *lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (lst) {
            gtkui_open_files (lst);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}


void
on_add_files_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Add file(s) to playlist..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    set_file_filter (dlg, NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);

    // restore folder
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast ("filechooser.lastdir", ""));
    deadbeef->conf_unlock ();
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
    }
    if (response == GTK_RESPONSE_OK)
    {
        GSList *lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (lst) {
            gtkui_add_files (lst);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

void
on_follow_symlinks_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("add_folders_follow_symlinks", gtk_toggle_button_get_active (togglebutton));
}

void
on_add_folders_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Add folder(s) to playlist..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkWidget *box = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (box);

    GtkWidget *check = gtk_check_button_new_with_mnemonic (_("Follow symlinks"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), deadbeef->conf_get_int ("add_folders_follow_symlinks", 0));
    g_signal_connect ((gpointer) check, "toggled",
            G_CALLBACK (on_follow_symlinks_toggled),
            NULL);
    gtk_widget_show (check);
    gtk_box_pack_start (GTK_BOX (box), check, FALSE, FALSE, 0);

    gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dlg), box);

    set_file_filter (dlg, NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);
    // restore folder
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast ("filechooser.lastdir", ""));
    deadbeef->conf_unlock ();
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
    }
    if (response == GTK_RESPONSE_OK)
    {
        //gchar *folder = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        GSList *lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (lst) {
            gtkui_add_dirs (lst);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}


void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_quit_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    progress_abort ();
    deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
}



void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_select_all ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    if (pl) {
        ddb_listview_refresh (pl, DDB_REFRESH_LIST);
    }
}





void
on_stopbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
}


void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == OUTPUT_STATE_PAUSED) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cur != -1) {
            ddb_playItem_t *it = deadbeef->plt_get_item_for_idx (plt, cur, PL_MAIN);
            ddb_playItem_t *it_playing = deadbeef->streamer_get_playing_track ();
            if (it) {
                deadbeef->pl_item_unref (it);
            }
            if (it_playing) {
                deadbeef->pl_item_unref (it_playing);
            }
            if (it != it_playing) {
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, cur, 0);
            }
            else {
                deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
            }
        }
        else {
            deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
        }
        deadbeef->plt_unref (plt);
    }
    else {
        deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    }
}


void
on_pausebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
}


void
on_prevbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
}


void
on_nextbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
}


void
on_playrand_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_PLAY_RANDOM, 0, 0, 0);
}

gboolean
on_mainwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    uint32_t maskedstate = (event->state &~ (GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD5_MASK)) & 0xfff;
    if ((maskedstate == GDK_MOD1_MASK || maskedstate == 0) && event->keyval == GDK_n) {
        // button for that one is not in toolbar anymore, so handle it manually
        deadbeef->sendmessage (DB_EV_PLAY_RANDOM, 0, 0, 0);
    }
    else if ((maskedstate == GDK_MOD1_MASK || maskedstate == 0) && event->keyval >= GDK_1 && event->keyval <= GDK_9) {
        int pl = event->keyval - GDK_1;
        if (pl >= 0 && pl < deadbeef->plt_get_count ()) {
            deadbeef->plt_set_curr_idx (pl);
            deadbeef->conf_set_int ("playlist.current", pl);
        }
    }
    return FALSE;
}


void
on_order_linear_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_LINEAR);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_order_shuffle_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_SHUFFLE_TRACKS);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_order_shuffle_albums_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_SHUFFLE_ALBUMS);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_order_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_RANDOM);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_loop_all_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.loop", 0);
}


void
on_loop_single_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.loop", 2);
}


void
on_loop_disable_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.loop", 1);
}

void
on_searchlist_realize                  (GtkWidget       *widget,
                                        gpointer         user_data)
{
}

//static GdkPixmap *seekbar_backbuf;

enum
{
	CORNER_NONE        = 0,
	CORNER_TOPLEFT     = 1,
	CORNER_TOPRIGHT    = 2,
	CORNER_BOTTOMLEFT  = 4,
	CORNER_BOTTOMRIGHT = 8,
	CORNER_ALL         = 15
};

static void
clearlooks_rounded_rectangle (cairo_t * cr,
			      double x, double y, double w, double h,
			      double radius, uint8_t corners)
{
    if (radius < 0.01 || (corners == CORNER_NONE)) {
        cairo_rectangle (cr, x, y, w, h);
        return;
    }
	
    if (corners & CORNER_TOPLEFT)
        cairo_move_to (cr, x + radius, y);
    else
        cairo_move_to (cr, x, y);

    if (corners & CORNER_TOPRIGHT)
        cairo_arc (cr, x + w - radius, y + radius, radius, M_PI * 1.5, M_PI * 2);
    else
        cairo_line_to (cr, x + w, y);

    if (corners & CORNER_BOTTOMRIGHT)
        cairo_arc (cr, x + w - radius, y + h - radius, radius, 0, M_PI * 0.5);
    else
        cairo_line_to (cr, x + w, y + h);

    if (corners & CORNER_BOTTOMLEFT)
        cairo_arc (cr, x + radius, y + h - radius, radius, M_PI * 0.5, M_PI);
    else
        cairo_line_to (cr, x, y + h);

    if (corners & CORNER_TOPLEFT)
        cairo_arc (cr, x + radius, y + radius, radius, M_PI, M_PI * 1.5);
    else
        cairo_line_to (cr, x, y);
	
}

int seekbar_moving = 0;
int seekbar_move_x = 0;

void
seekbar_draw (GtkWidget *widget, cairo_t *cr) {
    if (!widget) {
        return;
    }

#if GTK_CHECK_VERSION(3,0,0)
    GtkAllocation allocation;
    gtk_widget_get_allocation (widget, &allocation);
    cairo_translate (cr, -allocation.x, -allocation.y);
#endif

    GdkColor clr_selection, clr_back;
    gtkui_get_bar_foreground_color (&clr_selection);
    gtkui_get_bar_background_color (&clr_back);

    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    int ax = a.x;
    int ay = a.y;
    int aw = a.width;
    int ah = a.height;

    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (!trk || deadbeef->pl_get_item_duration (trk) < 0) {
        if (trk) {
            deadbeef->pl_item_unref (trk);
        }
        // empty seekbar, just a frame
        clearlooks_rounded_rectangle (cr, 2+ax, a.height/2-4+ay, aw-4, 8, 4, 0xff);
        cairo_set_source_rgb (cr, clr_selection.red/65535.f, clr_selection.green/65535.f, clr_selection.blue/65535.f );
        cairo_set_line_width (cr, 2);
        cairo_stroke (cr);
        return;
    }
    float pos = 0;
    if (seekbar_moving) {
        int x = seekbar_move_x;
        if (x < 0) {
            x = 0;
        }
        if (x > a.width-1) {
            x = a.width-1;
        }
        pos = x;
    }
    else {
        if (deadbeef->pl_get_item_duration (trk) > 0) {
            pos = deadbeef->streamer_get_playpos () / deadbeef->pl_get_item_duration (trk);
            pos *= a.width;
        }
    }
    // left
    if (pos > 0) {
        cairo_set_source_rgb (cr, clr_selection.red/65535.f, clr_selection.green/65535.f, clr_selection.blue/65535.f );
        cairo_rectangle (cr, ax, ah/2-4+ay, pos, 8);
        cairo_clip (cr);
        clearlooks_rounded_rectangle (cr, 0+ax, ah/2-4 + ay, aw, 8, 4, 0xff);
        cairo_fill (cr);
        cairo_reset_clip (cr);
    }

    // right
    cairo_set_source_rgb (cr, clr_back.red/65535.f, clr_back.green/65535.f, clr_back.blue/65535.f );
    cairo_rectangle (cr, pos+ax, ah/2-4+ay, aw-pos, 8);
    cairo_clip (cr);
    clearlooks_rounded_rectangle (cr, 0+ax, ah/2-4+ay, aw, 8, 4, 0xff);
    cairo_fill (cr);
    cairo_reset_clip (cr);

    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
}

gboolean
on_seekbar_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event)
{
    if (seekbar_moving) {
        GtkAllocation a;
        gtk_widget_get_allocation (widget, &a);
        seekbar_move_x = event->x - a.x;
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}

gboolean
on_seekbar_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    if (deadbeef->get_output ()->state () == OUTPUT_STATE_STOPPED) {
        return FALSE;
    }
    seekbar_moving = 1;
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    seekbar_move_x = event->x - a.x;
    gtk_widget_queue_draw (widget);
    return FALSE;
}


gboolean
on_seekbar_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    seekbar_moving = 0;
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (trk) {
        GtkAllocation a;
        gtk_widget_get_allocation (widget, &a);
        float time = (event->x - a.x) * deadbeef->pl_get_item_duration (trk) / (a.width);
        if (time < 0) {
            time = 0;
        }
        deadbeef->sendmessage (DB_EV_SEEK, 0, time * 1000, 0);
        deadbeef->pl_item_unref (trk);
    }
    gtk_widget_queue_draw (widget);
    return FALSE;
}

void
seekbar_redraw (void) {
    GtkWidget *widget = lookup_widget (mainwin, "seekbar");
    gtk_widget_queue_draw (widget);
}

gboolean
on_mainwin_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    int conf_close_send_to_tray = deadbeef->conf_get_int ("close_send_to_tray", 0);
    if (conf_close_send_to_tray) {
        gtk_widget_hide (widget);
    }
    else {
        deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
    }
    return TRUE;
}

gboolean
on_mainwin_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    wingeom_save (widget, "mainwin");
    return FALSE;
}


void
on_scroll_follows_playback_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.scroll.followplayback", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}


void
on_find_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    search_start ();       
}

void
on_info_window_delete (GtkWidget *widget, GtkTextDirection previous_direction, GtkWidget **pwindow) {
    *pwindow = NULL;
    gtk_widget_hide (widget);
    gtk_widget_destroy (widget);
}

static void
show_info_window (const char *fname, const char *title, GtkWidget **pwindow) {
    if (*pwindow) {
        return;
    }
    GtkWidget *widget = *pwindow = create_helpwindow ();
    g_object_set_data (G_OBJECT (widget), "pointer", pwindow);
    g_signal_connect (widget, "delete_event", G_CALLBACK (on_info_window_delete), pwindow);
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

static GtkWidget *helpwindow;

void
on_help1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), _("help.txt"));
    show_info_window (fname, _("Help"), &helpwindow);
}

static GtkWidget *aboutwindow;

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char s[200];
    snprintf (s, sizeof (s), _("About DeaDBeeF %s"), VERSION);
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "about.txt");
    show_info_window (fname, s, &aboutwindow);
}

static GtkWidget *changelogwindow;

void
on_changelog1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char s[200];
    snprintf (s, sizeof (s), _("DeaDBeeF %s ChangeLog"), VERSION);
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "ChangeLog");
    show_info_window (fname, s, &changelogwindow);
}

static GtkWidget *gplwindow;

void
on_gpl1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "COPYING.GPLv2");
    show_info_window (fname, "GNU GENERAL PUBLIC LICENSE Version 2", &gplwindow);
}

static GtkWidget *lgplwindow;

void
on_lgpl1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "COPYING.LGPLv2.1");
    show_info_window (fname, "GNU LESSER GENERAL PUBLIC LICENSE Version 2.1", &lgplwindow);
}

gboolean
on_helpwindow_key_press_event          (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if (event->keyval == GDK_Escape) {
        GtkWidget **pwindow = (GtkWidget **)g_object_get_data (G_OBJECT (widget), "pointer");
        if (pwindow) {
            *pwindow = NULL;
        }
        gtk_widget_hide (widget);
        gtk_widget_destroy (widget);
    }
    return FALSE;
}

void
on_column_id_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (combobox));
    if (!toplevel) {
        trace ("failed to get toplevel widget for column id combobox\n");
        return;
    }
    int act = gtk_combo_box_get_active (combobox);
    GtkWidget *fmt = lookup_widget (toplevel, "format");
    if (!fmt) {
        trace ("failed to get column format widget\n");
        return;
    }
    gtk_widget_set_sensitive (fmt, act >= DB_COLUMN_ID_MAX ? TRUE : FALSE);
}


gboolean
on_mainwin_window_state_event          (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    wingeom_save_max (event, widget, "mainwin");
    return FALSE;
}


void
on_toggle_status_bar_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *sb = lookup_widget (mainwin, "statusbar");
    if (sb) {
        if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
            deadbeef->conf_set_int ("gtkui.statusbar.visible", 0);
            gtk_widget_hide (sb);
        }
        else {
            deadbeef->conf_set_int ("gtkui.statusbar.visible", 1);
            gtk_widget_show (sb);
        }
    }
    deadbeef->conf_save ();
}

void
on_toggle_column_headers_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    // FIXME!
    return;
    GtkWidget *playlist = lookup_widget (mainwin, "playlist");
    if (playlist) {
        if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
            deadbeef->conf_set_int ("gtkui.headers.visible", 0);
            ddb_listview_show_header (DDB_LISTVIEW (playlist), 0);
        }
        else {
            deadbeef->conf_set_int ("gtkui.headers.visible", 1);
            ddb_listview_show_header (DDB_LISTVIEW (playlist), 1);
        }
    }
    deadbeef->conf_save ();
}

void
on_stop_after_current_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.stop_after_current", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}

void
on_cursor_follows_playback_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.scroll.cursorfollowplayback", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}

GtkWidget*
create_ddb_listview_widget (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    return ddb_listview_new ();
}


GtkWidget*
create_tabstrip_widget (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    return ddb_tabstrip_new ();
}


GtkWidget*
create_volumebar_widget (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    return ddb_volumebar_new ();
}



void
on_mainwin_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
    gtkui_init_theme_colors ();
}



void
on_toggle_tabs                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    // FIXME!
    return;
    GtkWidget *ts = lookup_widget (mainwin, "tabstrip");
    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->conf_set_int ("gtkui.tabs.visible", 0);
        gtk_widget_hide (ts);
    }
    else {
        deadbeef->conf_set_int ("gtkui.tabs.visible", 1);
        gtk_widget_show (ts);
    }
    deadbeef->conf_save ();
}


void
on_toggle_eq                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->conf_set_int ("gtkui.eq.visible", 0);
        eq_window_hide ();
    }
    else {
        deadbeef->conf_set_int ("gtkui.eq.visible", 1);
        eq_window_show ();
    }
    deadbeef->conf_save ();
}



void
on_deselect_all1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_set_selected (it, 0);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    if (pl) {
        ddb_listview_refresh (pl, DDB_REFRESH_LIST);
    }
}


void
on_invert_selection1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_set_selected (it, 0);
        }
        else {
            deadbeef->pl_set_selected (it, 1);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int pl = gtkui_add_new_playlist ();
    if (pl != -1) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
}


static GtkWidget *capture = NULL;

gboolean
on_mainwin_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->window != gtk_widget_get_window (mainwin)) {
        return FALSE;
    }
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    GtkWidget *seekbar = lookup_widget (mainwin, "seekbar");
    GtkAllocation a, b;
    gtk_widget_get_allocation (volumebar, &a);
    gtk_widget_get_allocation (seekbar, &b);
    if (event->x >= a.x && event->x < a.x + a.width
            && event->y >= a.y && event->y < a.y + a.height) {
        capture = volumebar;
        return gtk_widget_event (volumebar, (GdkEvent *)event);
    }
    else if (event->x >= b.x && event->x < b.x + b.width
            && event->y >= b.y && event->y < b.y + b.height) {
        capture = seekbar;
        return gtk_widget_event (seekbar, (GdkEvent *)event);
    }

  return FALSE;
}


gboolean
on_mainwin_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (capture) {
        gboolean res = gtk_widget_event (capture, (GdkEvent *)event);
        capture = NULL;
        return res;
    }

    return FALSE;
}


gboolean
on_mainwin_scroll_event                (GtkWidget       *widget,
                                        GdkEvent        *ev,
                                        gpointer         user_data)
{
    GdkEventScroll *event = (GdkEventScroll *)ev;
    if (event->window != gtk_widget_get_window (mainwin)) {
        return FALSE;
    }
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    GtkWidget *seekbar = lookup_widget (mainwin, "seekbar");
    GtkAllocation a;
    gtk_widget_get_allocation (volumebar, &a);
    GtkAllocation b;
    gtk_widget_get_allocation (seekbar, &b);
    if (event->x >= a.x && event->x < a.x + a.width
            && event->y >= a.y && event->y < a.y + a.height) {
        return gtk_widget_event (volumebar, (GdkEvent *)event);
    }
    else if (event->x >= b.x && event->x < b.x + b.width
            && event->y >= b.y && event->y < b.y + b.height) {
        return gtk_widget_event (seekbar, (GdkEvent *)event);
    }
  return FALSE;
}


gboolean
on_mainwin_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    if (capture) {
        return gtk_widget_event (capture, (GdkEvent *)event);
    }
  return FALSE;
}


GtkWidget*
create_seekbar (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    return GTK_WIDGET (ddb_seekbar_new ());
}


void
on_jump_to_current_track1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkui_focus_on_playing_track ();
}

static GtkWidget *translatorswindow;

void
on_translators1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char s[200];
    snprintf (s, sizeof (s), _("DeaDBeeF Translators"));
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "translators.txt");
    show_info_window (fname, s, &translatorswindow);
}


GtkWidget*
title_formatting_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("http://sourceforge.net/apps/mediawiki/deadbeef/index.php?title=Title_Formatting", "Help");
    return link;
}



void
on_sortfmt_activate                    (GtkEntry        *entry,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (entry));
    gtk_dialog_response (GTK_DIALOG (toplevel), GTK_RESPONSE_OK);
}



GtkWidget*
create_plugin_weblink (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("", "WWW");
    gtk_widget_set_sensitive (link, FALSE);
    return link;
}

void
on_sort_by_title_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%t", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_track_nr_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%n", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_album_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%b", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_artist_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%a", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_date_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%y", DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);

    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_clear_sort (pl);
    ddb_listview_refresh (pl, DDB_REFRESH_LIST | DDB_LIST_CHANGED);
}


void
on_sort_by_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, NULL, DDB_SORT_RANDOM);

    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_custom_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_sortbydlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (dlg, "sortorder"));
    GtkEntry *entry = GTK_ENTRY (lookup_widget (dlg, "sortfmt"));
    
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("gtkui.sortby_order", 0));
    deadbeef->conf_lock ();
    gtk_entry_set_text (entry, deadbeef->conf_get_str_fast ("gtkui.sortby_fmt", ""));
    deadbeef->conf_unlock ();

    int r = gtk_dialog_run (GTK_DIALOG (dlg));

    if (r == GTK_RESPONSE_OK) {
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (dlg, "sortorder"));
        GtkEntry *entry = GTK_ENTRY (lookup_widget (dlg, "sortfmt"));
        int order = gtk_combo_box_get_active (combo);
        const char *fmt = gtk_entry_get_text (entry);

        deadbeef->conf_set_int ("gtkui.sortby_order", order);
        deadbeef->conf_set_str ("gtkui.sortby_fmt", fmt);

        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        deadbeef->plt_sort (plt, PL_MAIN, -1, fmt, order == 0 ? DDB_SORT_ASCENDING : DDB_SORT_DESCENDING);
        deadbeef->plt_unref (plt);

        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    }

    gtk_widget_destroy (dlg);
    dlg = NULL;
}

void
on_design_mode1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    w_set_design_mode (act ? 1 : 0);
}

