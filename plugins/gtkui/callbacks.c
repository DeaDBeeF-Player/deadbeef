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
        name = _("Supported sound formats");
    }

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, name);

    gtk_file_filter_add_custom (flt, GTK_FILE_FILTER_FILENAME, file_filter_func, NULL, NULL);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("Other files (*)"));
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
}

void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Open file(s)..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

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
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Add file(s) to playlist..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

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
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Add folder(s) to playlist..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

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

    if (event->keyval == GDK_n && !event->state) {
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
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Save Playlist As"), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg), TRUE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "untitled.dbpl");

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("DeaDBeeF playlist files (*.dbpl)"));
    gtk_file_filter_add_pattern (flt, "*.dbpl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);

        if (fname) {
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
seekbar_draw (GtkWidget *widget) {
    if (!widget) {
        return;
    }
//    gdk_draw_rectangle (seekbar_backbuf, widget->style->bg_gc[0], TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
	cairo_t *cr;
	cr = gdk_cairo_create (gtk_widget_get_window (widget));
	if (!cr) {
        return;
    }
    GdkColor clr_selection, clr_back;
    gtkui_get_bar_foreground_color (&clr_selection);
    gtkui_get_bar_background_color (&clr_back);

    int ax = widget->allocation.x;
    int ay = widget->allocation.y;
    int aw = widget->allocation.width;
    int ah = widget->allocation.height;

    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (!trk || deadbeef->pl_get_item_duration (trk) < 0) {
        if (trk) {
            deadbeef->pl_item_unref (trk);
        }
        clearlooks_rounded_rectangle (cr, 2+ax, widget->allocation.height/2-4+ay, aw-4, 8, 4, 0xff);
        // empty seekbar, just a frame
        cairo_set_source_rgb (cr, clr_selection.red/65535.f, clr_selection.green/65535.f, clr_selection.blue/65535.f );
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
        if (deadbeef->pl_get_item_duration (trk) > 0) {
            pos = deadbeef->streamer_get_playpos () / deadbeef->pl_get_item_duration (trk);
            pos *= widget->allocation.width;
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

    cairo_destroy (cr);
    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
}

#if 0
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
#endif

gboolean
on_seekbar_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event)
{
    if (seekbar_moving) {
        seekbar_move_x = event->x - widget->allocation.x;
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
    seekbar_move_x = event->x - widget->allocation.x;
    gtk_widget_queue_draw (widget);
    return FALSE;
}


gboolean
on_seekbar_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event)
{
    seekbar_moving = 0;
    gtk_widget_queue_draw (widget);
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (trk) {
        float time = (event->x - widget->allocation.x) * deadbeef->pl_get_item_duration (trk) / (widget->allocation.width);
        if (time < 0) {
            time = 0;
        }
        deadbeef->streamer_seek (time);
        deadbeef->pl_item_unref (trk);
    }
    return FALSE;
}

void
seekbar_redraw (void) {
    GtkWidget *widget = lookup_widget (mainwin, "seekbar");
    gtk_widget_queue_draw (widget);
    //seekbar_draw (widget);
    //seekbar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
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
    if (!(window_state & GDK_WINDOW_STATE_MAXIMIZED) && gtk_widget_get_visible (widget)) {
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
    GtkWidget *ts = lookup_widget (mainwin, "tabstrip");
    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->conf_set_int ("gtkui.tabs.visible", 0);
        gtk_widget_hide (ts);
    }
    else {
        deadbeef->conf_set_int ("gtkui.tabs.visible", 1);
        gtk_widget_show (ts);
    }
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
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_refresh (pl, DDB_REFRESH_LIST | DDB_EXPOSE_LIST);
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
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    ddb_listview_refresh (pl, DDB_REFRESH_LIST | DDB_EXPOSE_LIST);
}


void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int pl = gtkui_add_new_playlist ();
    if (pl != -1) {
        deadbeef->plt_set_curr (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
}


static GtkWidget *capture = NULL;

gboolean
on_mainwin_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->window != mainwin->window) {
        return FALSE;
    }
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    GtkWidget *seekbar = lookup_widget (mainwin, "seekbar");
    if (event->x >= volumebar->allocation.x && event->x < volumebar->allocation.x + volumebar->allocation.width
            && event->y >= volumebar->allocation.y && event->y < volumebar->allocation.y + volumebar->allocation.height) {
        capture = volumebar;
        return gtk_widget_event (volumebar, (GdkEvent *)event);
    }
    else if (event->x >= seekbar->allocation.x && event->x < seekbar->allocation.x + seekbar->allocation.width
            && event->y >= seekbar->allocation.y && event->y < seekbar->allocation.y + seekbar->allocation.height) {
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
    if (event->window != mainwin->window) {
        return FALSE;
    }
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    GtkWidget *seekbar = lookup_widget (mainwin, "seekbar");
    if (event->x >= volumebar->allocation.x && event->x < volumebar->allocation.x + volumebar->allocation.width
            && event->y >= volumebar->allocation.y && event->y < volumebar->allocation.y + volumebar->allocation.height) {
        return gtk_widget_event (volumebar, (GdkEvent *)event);
    }
    else if (event->x >= seekbar->allocation.x && event->x < seekbar->allocation.x + seekbar->allocation.width
            && event->y >= seekbar->allocation.y && event->y < seekbar->allocation.y + seekbar->allocation.height) {
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
