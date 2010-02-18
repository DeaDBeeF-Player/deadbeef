/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "ddblistview.h"
#include "ddbtabstrip.h"
#include "ddbvolumebar.h"
#include "search.h"
#include "progress.h"
#include "../../session.h"
#include "gtkui.h"
#include "parser.h"

#define trace(...) { fprintf (stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define SELECTED(it) (deadbeef->pl_is_selected(it))
#define SELECT(it, sel) (deadbeef->pl_set_selected(it,sel))
#define VSELECT(it, sel) {deadbeef->pl_set_selected(it,sel);gtk_pl_redraw_item_everywhere (it);}
#define PL_NEXT(it, iter) (deadbeef->pl_get_next(it, iter))

DdbListview *last_playlist;
extern DB_functions_t *deadbeef; // defined in gtkui.c

static gboolean
file_filter_func (const GtkFileFilterInfo *filter_info, gpointer data) {
    // get ext
    const char *p = filter_info->filename + strlen (filter_info->filename)-1;
    while (p >= filter_info->filename) {
        if (*p == '.') {
            break;
        }
        p--;
    }
    if (*p != '.') {
        return FALSE;
    }
    p++;
    DB_decoder_t **codecs = deadbeef->plug_get_decoder_list ();
    for (int i = 0; codecs[i]; i++) {
        if (codecs[i]->exts && codecs[i]->insert) {
            const char **exts = codecs[i]->exts;
            if (exts) {
                for (int e = 0; exts[e]; e++) {
                    if (!strcasecmp (exts[e], p)) {
                        return TRUE;
                    }
                }
            }
        }
    }
    if (!strcasecmp (p, "pls")) {
        return TRUE;
    }
    if (!strcasecmp (p, "m3u")) {
        return TRUE;
    }
    return FALSE;
}

static GtkFileFilter *
set_file_filter (GtkWidget *dlg, const char *name) {
    if (!name) {
        name = "Supported sound formats";
    }

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, name);

    gtk_file_filter_add_custom (flt, GTK_FILE_FILTER_FILENAME, file_filter_func, NULL, NULL);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Other files (*)");
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
}

void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Open file(s)...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    set_file_filter (dlg, NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
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
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add file(s) to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    set_file_filter (dlg, NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);

    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
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
on_add_folders_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add folder(s) to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    set_file_filter (dlg, NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
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
    deadbeef->sendmessage (M_TERMINATE, 0, 0, 0);
}



void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_select_all ();
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_refresh (pl, DDB_REFRESH_LIST | DDB_EXPOSE_LIST);
}





void
on_stopbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (M_STOPSONG, 0, 0, 0);
}


void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (M_PLAYSONG, 0, 0, 0);
}


void
on_pausebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (M_PAUSESONG, 0, 0, 0);
}


void
on_prevbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (M_PREVSONG, 0, 0, 0);
}


void
on_nextbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (M_NEXTSONG, 0, 0, 0);
}


void
on_playrand_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (M_PLAYRANDOM, 0, 0, 0);
}


gboolean
on_mainwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{

    if (event->keyval == GDK_n) {
        // button for that one is not in toolbar anymore, so handle it manually
        deadbeef->sendmessage (M_PLAYRANDOM, 0, 0, 0);
    }
    else {
        ddb_listview_handle_keypress (DDB_LISTVIEW (lookup_widget (mainwin, "playlist")), event->keyval, event->state);
    }
    return FALSE;
}


void
on_order_linear_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", 0);
}


void
on_order_shuffle_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", 1);
}


void
on_order_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", 2);
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

char last_playlist_save_name[1024] = "";

void
save_playlist_as (void) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Save Playlist As", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg), TRUE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "untitled.dbpl");

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
// the code below cannot be used, because it breaks gtk overwrite confirmation
#if 0
            // check extension and append .dbpl if none
            size_t sz = strlen (fname);
            char ext[] = ".dbpl";
            const char *p = fname + sz - 1;
            while (p > fname && *p != '/' && *p != '.') {
                p--;
            }
            if (*p != '.') {
                // extension not found
                char *n = g_malloc (sz + sizeof (ext));
                memcpy (n, fname, sz);
                memcpy (n+sz, ext, sizeof (ext));
                g_free (fname);
                fname = n;
            }
#endif
            int res = deadbeef->pl_save (fname);
            if (res >= 0 && strlen (fname) < 1024) {
                strcpy (last_playlist_save_name, fname);
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
        /*int res = */deadbeef->pl_save (last_playlist_save_name);
    }
}


void
on_playlist_save_as_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    save_playlist_as ();
}


static GdkPixmap *seekbar_backbuf;

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
seekbar_draw (GtkWidget *widget) {
    if (!widget) {
        return;
    }
    gdk_draw_rectangle (seekbar_backbuf, widget->style->bg_gc[0], TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
	cairo_t *cr;
	cr = gdk_cairo_create (seekbar_backbuf);
	if (!cr) {
        return;
    }
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    DB_fileinfo_t *dec = deadbeef->streamer_get_current_fileinfo ();
    if (!dec || !trk || deadbeef->pl_get_item_duration (trk) < 0) {
        clearlooks_rounded_rectangle (cr, 2, widget->allocation.height/2-4, widget->allocation.width-4, 8, 4, 0xff);
        theme_set_cairo_source_rgb (cr, COLO_SEEKBAR_FRONT);
        cairo_stroke (cr);
        cairo_destroy (cr);
        return;
    }
    float pos = 0;
    if (seekbar_moving) {
        int x = seekbar_move_x;
        if (x < 0) {
            x = 0;
        }
        if (x > widget->allocation.width-1) {
            x = widget->allocation.width-1;
        }
        pos = x;
    }
    else {
        if (dec && deadbeef->pl_get_item_duration (trk) > 0) {
            pos = deadbeef->streamer_get_playpos () / deadbeef->pl_get_item_duration (trk);
            pos *= widget->allocation.width;
        }
    }
    // left
    if (pos > 0) {
        theme_set_cairo_source_rgb (cr, COLO_SEEKBAR_FRONT);
        cairo_rectangle (cr, 0, widget->allocation.height/2-4, pos, 8);
        cairo_clip (cr);
        clearlooks_rounded_rectangle (cr, 0, widget->allocation.height/2-4, widget->allocation.width, 8, 4, 0xff);
        cairo_fill (cr);
        cairo_reset_clip (cr);
    }

    // right
    theme_set_cairo_source_rgb (cr, COLO_SEEKBAR_BACK);
    cairo_rectangle (cr, pos, widget->allocation.height/2-4, widget->allocation.width-pos, 8);
    cairo_clip (cr);
    clearlooks_rounded_rectangle (cr, 0, widget->allocation.height/2-4, widget->allocation.width, 8, 4, 0xff);
    cairo_fill (cr);
    cairo_reset_clip (cr);

    cairo_destroy (cr);
}

void
seekbar_expose (GtkWidget *widget, int x, int y, int w, int h) {
	gdk_draw_drawable (widget->window, widget->style->black_gc, seekbar_backbuf, x, y, x, y, w, h);
}

gboolean
on_seekbar_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    if (seekbar_backbuf) {
        g_object_unref (seekbar_backbuf);
        seekbar_backbuf = NULL;
    }
    seekbar_backbuf = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);
    seekbar_draw (widget);
    return FALSE;
}

gboolean
on_seekbar_expose_event                (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    seekbar_expose (widget, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}

gboolean
on_seekbar_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    if (seekbar_moving) {
        seekbar_move_x = event->x;
        seekbar_draw (widget);
        seekbar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    }
    return FALSE;
}

gboolean
on_seekbar_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (deadbeef->get_output ()->state () == OUTPUT_STATE_STOPPED) {
        return FALSE;
    }
    seekbar_moving = 1;
    seekbar_move_x = event->x;
    seekbar_draw (widget);
    seekbar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}


gboolean
on_seekbar_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    seekbar_moving = 0;
    seekbar_draw (widget);
    seekbar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (trk) {
        float time = event->x * deadbeef->pl_get_item_duration (trk) / (widget->allocation.width);
        if (time < 0) {
            time = 0;
        }
        deadbeef->streamer_seek (time);
    }
    return FALSE;
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
        deadbeef->sendmessage (M_TERMINATE, 0, 0, 0);
    }
    return TRUE;
}

gboolean
on_mainwin_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
#if GTK_CHECK_VERSION(2,2,0)
	GdkWindowState window_state = gdk_window_get_state (GDK_WINDOW (widget->window));
#else
	GdkWindowState window_state = gdk_window_get_state (G_OBJECT (widget));
#endif
    if (!(window_state & GDK_WINDOW_STATE_MAXIMIZED) && GTK_WIDGET_VISIBLE (widget)) {
        int x, y;
        int w, h;
        gtk_window_get_position (GTK_WINDOW (widget), &x, &y);
        gtk_window_get_size (GTK_WINDOW (widget), &w, &h);
        deadbeef->conf_set_int ("mainwin.geometry.x", x);
        deadbeef->conf_set_int ("mainwin.geometry.y", y);
        deadbeef->conf_set_int ("mainwin.geometry.w", w);
        deadbeef->conf_set_int ("mainwin.geometry.h", h);
    }
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
    search_restore_attrs ();
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
            const char *error = "Failed while reading help file";
            gtk_text_buffer_set_text (buffer, error, strlen (error));
        }
        else {
            buf[s] = 0;
            gtk_text_buffer_set_text (buffer, buf, s);
        }
        fclose (fp);
    }
    else {
        const char *error = "Failed to load help file";
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
    show_info_window (DOCDIR "/help.txt", "Help", &helpwindow);
}

static GtkWidget *aboutwindow;

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    show_info_window (DOCDIR "/about.txt", "About DeaDBeeF " VERSION, &aboutwindow);
}

static GtkWidget *changelogwindow;

void
on_changelog1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    show_info_window (DOCDIR "/ChangeLog", "DeaDBeeF " VERSION " ChangeLog", &changelogwindow);
}

static GtkWidget *gplwindow;

void
on_gpl1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    show_info_window (DOCDIR "/COPYING.GPLv2", "GNU GENERAL PUBLIC LICENSE Version 2", &gplwindow);
}

static GtkWidget *lgplwindow;

void
on_lgpl1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    show_info_window (DOCDIR "/COPYING.LGPLv2.1", "GNU LESSER GENERAL PUBLIC LICENSE Version 2.1", &lgplwindow);
}



void
on_searchhscroll_value_changed         (GtkRange        *widget,
                                        gpointer         user_data)
{
// KILLME
//    GTKPL_PROLOGUE;
//    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
//    gtkpl_hscroll (ps, newscroll);
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

static GtkWidget *prefwin;

static char alsa_device_names[100][64];
static int num_alsa_devices;

static void
gtk_enum_sound_callback (const char *name, const char *desc, void *userdata) {
    if (num_alsa_devices >= 100) {
        fprintf (stderr, "wtf!! more than 100 alsa devices??\n");
        return;
    }
    GtkComboBox *combobox = GTK_COMBO_BOX (userdata);
    gtk_combo_box_append_text (combobox, desc);

    if (!strcmp (deadbeef->conf_get_str ("alsa_soundcard", "default"), name)) {
        gtk_combo_box_set_active (combobox, num_alsa_devices);
    }

    strncpy (alsa_device_names[num_alsa_devices], name, 63);
    alsa_device_names[num_alsa_devices][63] = 0;
    num_alsa_devices++;
}

void
on_plugin_active_toggled (GtkCellRendererToggle *cell_renderer, gchar *path, GtkTreeModel *model) {
    GtkTreePath *p = gtk_tree_path_new_from_string (path);
    if (p) {
        int *indices = gtk_tree_path_get_indices (p);
        //gtk_tree_path_free (p); // wtf?? gtk crashes on this
        if (indices) {
            DB_plugin_t **plugins = deadbeef->plug_get_list ();
            DB_plugin_t *plug = plugins[*indices];
            gboolean state;
            GtkTreeIter iter;
            gtk_tree_model_get_iter (model, &iter, p);
            gtk_tree_model_get (model, &iter, 0, &state, -1);
            if (!deadbeef->plug_activate (plug, !state)) {
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, !state, -1);
            }
        }
        g_free (indices);
    }
}

void
preferences_fill_soundcards (void) {
    if (!prefwin) {
        return;
    }
    const char *s = deadbeef->conf_get_str ("alsa_soundcard", "default");
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (prefwin, "pref_soundcard"));
    GtkTreeModel *mdl = gtk_combo_box_get_model (combobox);
    gtk_list_store_clear (GTK_LIST_STORE (mdl));

    gtk_combo_box_append_text (combobox, "Default Audio Device");
    if (!strcmp (s, "default")) {
        gtk_combo_box_set_active (combobox, 0);
    }
    num_alsa_devices = 1;
    strcpy (alsa_device_names[0], "default");
    if (deadbeef->get_output ()->enum_soundcards) {
        deadbeef->get_output ()->enum_soundcards (gtk_enum_sound_callback, combobox);
        gtk_widget_set_sensitive (GTK_WIDGET (combobox), TRUE);
    }
    else {
        gtk_widget_set_sensitive (GTK_WIDGET (combobox), FALSE);
    }
}

void
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (prefwin) {
        return;
    }
    GtkWidget *w = prefwin = create_prefwin ();
    gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (mainwin));

    GtkComboBox *combobox = NULL;

    // output plugin selection
    const char *outplugname = deadbeef->conf_get_str ("output_plugin", "ALSA output plugin");
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_output_plugin"));

    DB_output_t **out_plugs = deadbeef->plug_get_output_list ();
    for (int i = 0; out_plugs[i]; i++) {
        gtk_combo_box_append_text (combobox, out_plugs[i]->plugin.name);
        if (!strcmp (outplugname, out_plugs[i]->plugin.name)) {
            gtk_combo_box_set_active (combobox, i);
        }
    }

    // soundcard (output device) selection
    preferences_fill_soundcards ();

    g_signal_connect ((gpointer) combobox, "changed",
            G_CALLBACK (on_pref_output_plugin_changed),
            NULL);
    GtkWidget *pref_soundcard = lookup_widget (prefwin, "pref_soundcard");
    g_signal_connect ((gpointer) pref_soundcard, "changed",
            G_CALLBACK (on_pref_soundcard_changed),
            NULL);

    // alsa resampling
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "pref_dynsamplerate")), deadbeef->conf_get_int ("playback.dynsamplerate", 0));

    // src_quality
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_src_quality"));
    gtk_combo_box_set_active (combobox, deadbeef->conf_get_int ("src_quality", 2));

    // replaygain_mode
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_replaygain_mode"));
    gtk_combo_box_set_active (combobox, deadbeef->conf_get_int ("replaygain_mode", 0));

    // replaygain_scale
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "pref_replaygain_scale")), deadbeef->conf_get_int ("replaygain_scale", 1));

    // close_send_to_tray
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "pref_close_send_to_tray")), deadbeef->conf_get_int ("close_send_to_tray", 0));

    // network
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "pref_network_enableproxy")), deadbeef->conf_get_int ("network.proxy", 0));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyaddress")), deadbeef->conf_get_str ("network.proxy.address", ""));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyport")), deadbeef->conf_get_str ("network.proxy.port", "8080"));
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_network_proxytype"));
    const char *type = deadbeef->conf_get_str ("network.proxy.type", "HTTP");
    if (!strcasecmp (type, "HTTP")) {
        gtk_combo_box_set_active (combobox, 0);
    }
    else if (!strcasecmp (type, "HTTP_1_0")) {
        gtk_combo_box_set_active (combobox, 1);
    }
    else if (!strcasecmp (type, "SOCKS4")) {
        gtk_combo_box_set_active (combobox, 2);
    }
    else if (!strcasecmp (type, "SOCKS5")) {
        gtk_combo_box_set_active (combobox, 3);
    }
    else if (!strcasecmp (type, "SOCKS4A")) {
        gtk_combo_box_set_active (combobox, 4);
    }
    else if (!strcasecmp (type, "SOCKS5_HOSTNAME")) {
        gtk_combo_box_set_active (combobox, 5);
    }

    // list of plugins
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
#if 0
    GtkCellRenderer *rend_toggle = gtk_cell_renderer_toggle_new ();
    GtkListStore *store = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN);
    g_signal_connect ((gpointer)rend_toggle, "toggled",
            G_CALLBACK (on_plugin_active_toggled),
            store);
    GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes ("Active", rend_toggle, "active", 0, "activatable", 2, NULL);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes ("Title", rend_text, "text", 1, NULL);
    gtk_tree_view_append_column (tree, col1);
    gtk_tree_view_append_column (tree, col2);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugins[i]; i++) {
        GtkTreeIter it;
        gtk_list_store_append (store, &it);
        gtk_list_store_set (store, &it, 0, plugins[i]->inactive ? FALSE : TRUE, 1, plugins[i]->name, 2, plugins[i]->nostop ? FALSE : TRUE, -1);
    }
#else
    GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes ("Title", rend_text, "text", 0, NULL);
    gtk_tree_view_append_column (tree, col2);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugins[i]; i++) {
        GtkTreeIter it;
        gtk_list_store_append (store, &it);
        gtk_list_store_set (store, &it, 0, plugins[i]->name, -1);
    }
#endif
    gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));

    gtk_widget_set_sensitive (lookup_widget (prefwin, "configure_plugin"), FALSE);
//    gtk_widget_show (w);
    gtk_dialog_run (GTK_DIALOG (prefwin));
    gtk_widget_destroy (prefwin);
    prefwin = NULL;
}


void
on_pref_soundcard_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    if (active >= 0 && active < num_alsa_devices) {
        const char *soundcard = deadbeef->conf_get_str ("alsa_soundcard", "default");
        if (strcmp (soundcard, alsa_device_names[active])) {
            deadbeef->conf_set_str ("alsa_soundcard", alsa_device_names[active]);
            deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
        }
    }
}

void
on_pref_output_plugin_changed          (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    const char *outplugname = deadbeef->conf_get_str ("output_plugin", "ALSA output plugin");
    int active = gtk_combo_box_get_active (combobox);

    DB_output_t **out_plugs = deadbeef->plug_get_output_list ();
    DB_output_t *prev = NULL;
    DB_output_t *new = NULL;

    for (int i = 0; out_plugs[i]; i++) {
        if (!strcmp (out_plugs[i]->plugin.name, outplugname)) {
            prev = out_plugs[i];
        }
        if (i == active) {
            new = out_plugs[i];
        }
    }

    if (!new) {
        fprintf (stderr, "failed to find output plugin selected in preferences window\n");
    }
    else {
        if (prev != new) {
            deadbeef->conf_set_str ("output_plugin", new->plugin.name);
            deadbeef->sendmessage (M_REINIT_SOUND, 0, 0, 0);
        }
    }
}

void
on_pref_dynsamplerate_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("playback.dynsamplerate", active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_src_quality_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    deadbeef->conf_set_int ("src_quality", active == -1 ? 2 : active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_replaygain_mode_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    deadbeef->conf_set_int ("replaygain_mode", active == -1 ? 0 : active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}

void
on_pref_replaygain_scale_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("replaygain_scale", active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_close_send_to_tray_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("close_send_to_tray", active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}

void
on_pref_pluginlist_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // reset
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    DB_plugin_t *p = plugins[*indices];
    g_free (indices);
    assert (p);
    GtkWidget *w = prefwin;//GTK_WIDGET (gtk_widget_get_parent_window (GTK_WIDGET (treeview)));
    assert (w);
    GtkEntry *e = GTK_ENTRY (lookup_widget (w, "pref_plugin_descr"));
    gtk_entry_set_text (e, p->descr ? p->descr : "");
    e = GTK_ENTRY (lookup_widget (w, "pref_plugin_author"));
    gtk_entry_set_text (e, p->author ? p->author : "");
    e = GTK_ENTRY (lookup_widget (w, "pref_plugin_email"));
    gtk_entry_set_text (e, p->email ? p->email : "");
    e = GTK_ENTRY (lookup_widget (w, "pref_plugin_website"));
    gtk_entry_set_text (e, p->website ? p->website : "");

    gtk_widget_set_sensitive (lookup_widget (prefwin, "configure_plugin"), p->configdialog ? TRUE : FALSE);
}

gboolean
on_prefwin_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    prefwin = NULL;
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


void
on_pref_network_proxyaddress_changed   (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.proxy.address", gtk_entry_get_text (GTK_ENTRY (editable)));
}


void
on_pref_network_enableproxy_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("network.proxy", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}


void
on_pref_network_proxyport_changed      (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("network.proxy.port", atoi (gtk_entry_get_text (GTK_ENTRY (editable))));
}


void
on_pref_network_proxytype_changed      (GtkComboBox     *combobox,
                                        gpointer         user_data)
{

    int active = gtk_combo_box_get_active (combobox);
    switch (active) {
    case 0:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP");
        break;
    case 1:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP_1_0");
        break;
    case 2:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS4");
        break;
    case 3:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS5");
        break;
    case 4:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS4A");
        break;
    case 5:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS5_HOSTNAME");
        break;
    default:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP");
        break;
    }
}


gboolean
on_prefwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if (event->keyval == GDK_Escape) {
        gtk_widget_hide (widget);
        gtk_widget_destroy (widget);
    }
    return FALSE;
}

void
on_pref_close_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    gtk_widget_hide (prefwin);
    gtk_widget_destroy (prefwin);
}


void
on_configure_plugin_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // reset
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    DB_plugin_t *p = plugins[*indices];
    if (p->configdialog) {
        plugin_configure (prefwin, p);
    }
}


gboolean
on_mainwin_window_state_event          (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    // based on pidgin maximization handler
#if GTK_CHECK_VERSION(2,2,0)
    if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
        if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
            deadbeef->conf_set_int ("mainwin.geometry.maximized", 1);
        }
        else {
            deadbeef->conf_set_int ("mainwin.geometry.maximized", 0);
        }
    }
#else
	GdkWindowState new_window_state = gdk_window_get_state(G_OBJECT(widget));

	if (new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        deadbeef->conf_set_int ("mainwin.geometry.maximized", 1);
    }
	else {
        deadbeef->conf_set_int ("mainwin.geometry.maximized", 0);
    }
#endif
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
}

void
on_toggle_column_headers_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *header = lookup_widget (mainwin, "header");
    if (header) {
        if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
            deadbeef->conf_set_int ("gtkui.headers.visible", 0);
            gtk_widget_hide (header);
        }
        else {
            deadbeef->conf_set_int ("gtkui.headers.visible", 1);
            gtk_widget_show (header);
        }
    }
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

