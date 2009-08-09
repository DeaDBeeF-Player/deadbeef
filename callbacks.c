/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "common.h"

#include "playlist.h"
#include "gtkplaylist.h"
#include "messagepump.h"
#include "messages.h"
#include "codec.h"
#include "playback.h"
#include "search.h"

#include "cwav.h"
#include "cvorbis.h"
#include "cdumb.h"
#include "cmp3.h"
#include "cgme.h"
#include "cflac.h"
#include "csid.h"

extern GtkWidget *mainwin;
extern gtkplaylist_t main_playlist;
extern gtkplaylist_t search_playlist;

static int main_playlist_initialized = 0;
static int search_playlist_initialized = 0;

void
main_playlist_init (GtkWidget *widget) {
    if (!main_playlist_initialized) {
        printf ("main_playlist_init\n");
        main_playlist_initialized = 1;
        // init playlist control structure, and put it into widget user-data
        memset (&main_playlist, 0, sizeof (main_playlist));
        main_playlist.playlist = widget;
        main_playlist.header = lookup_widget (mainwin, "header");
        main_playlist.scrollbar = lookup_widget (mainwin, "playscroll");
        main_playlist.pcurr = &playlist_current_ptr;
        main_playlist.count = &ps_count;
        main_playlist.iterator = PS_MAIN;
        main_playlist.multisel = 1;
        main_playlist.scrollpos = 0;
        main_playlist.row = -1;
        main_playlist.clicktime = -1;
        main_playlist.nvisiblerows = 0;
        main_playlist.fmtcache = NULL;
        int colwidths[ps_ncolumns] = { 50, 200, 50, 200, 50 };
        memcpy (main_playlist.colwidths, colwidths, sizeof (colwidths));
        gtk_object_set_data (GTK_OBJECT (main_playlist.playlist), "ps", &main_playlist);
        gtk_object_set_data (GTK_OBJECT (main_playlist.header), "ps", &main_playlist);
        gtk_object_set_data (GTK_OBJECT (main_playlist.scrollbar), "ps", &main_playlist);
    }
}

void
search_playlist_init (GtkWidget *widget) {
    if (!search_playlist_initialized) {
        extern GtkWidget *searchwin;
        printf ("search_playlist_init\n");
        search_playlist_initialized = 1;
        // init playlist control structure, and put it into widget user-data
        memset (&search_playlist, 0, sizeof (search_playlist));
        search_playlist.playlist = widget;
        search_playlist.header = lookup_widget (searchwin, "searchheader");
        search_playlist.scrollbar = lookup_widget (searchwin, "searchscroll");
        assert (search_playlist.header);
        assert (search_playlist.scrollbar);
    //    main_playlist.pcurr = &search_current;
        search_playlist.count = &search_count;
        search_playlist.multisel = 0;
        search_playlist.iterator = PS_SEARCH;
        search_playlist.scrollpos = 0;
        search_playlist.row = -1;
        search_playlist.clicktime = -1;
        search_playlist.nvisiblerows = 0;
        search_playlist.fmtcache = NULL;
        int colwidths[ps_ncolumns] = { 0, 200, 50, 200, 50 };
        memcpy (search_playlist.colwidths, colwidths, sizeof (colwidths));
        gtk_object_set_data (GTK_OBJECT (search_playlist.playlist), "ps", &search_playlist);
        gtk_object_set_data (GTK_OBJECT (search_playlist.header), "ps", &search_playlist);
        gtk_object_set_data (GTK_OBJECT (search_playlist.scrollbar), "ps", &search_playlist);
    }
}

void
on_volume_value_changed                (GtkRange        *range,
        gpointer         user_data)
{
    //float db = -(40 - (gtk_range_get_value (range) * 0.4f));
    //float a = db <= -40.f ? 0 : pow (10, db/20.f);
    //p_set_volume (a);
    float a = gtk_range_get_value (range) * 0.01;
    p_set_volume (a*a);
}

int g_disable_seekbar_handler = 0;
void
on_playpos_value_changed               (GtkRange        *range,
        gpointer         user_data)
{
    if (g_disable_seekbar_handler) {
        return;
    }
    if (playlist_current.codec) {
        if (playlist_current.duration > 0) {
            int val = gtk_range_get_value (range);
            int upper = gtk_adjustment_get_upper (gtk_range_get_adjustment (range));
            float time = playlist_current.duration / (float)upper * (float)val;
            messagepump_push (M_SONGSEEK, 0, (int)time * 1000, 0);
        }
    }
}


// redraw
gboolean
on_playlist_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    GTKPS_PROLOGUE;
    // draw visible area of playlist
    gtkps_expose (ps, event->area.x, event->area.y, event->area.width, event->area.height);

    return FALSE;
}


gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GTKPS_PROLOGUE;
    if (event->button == 1) {
        gtkps_mouse1_pressed (ps, event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
on_playlist_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GTKPS_PROLOGUE;
    if (event->button == 1) {
        gtkps_mouse1_released (ps, event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
on_playlist_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    GTKPS_PROLOGUE;
    gtkps_mousemove (ps, event);
    return FALSE;
}


void
on_playscroll_value_changed            (GtkRange        *widget,
                                        gpointer         user_data)
{
    GTKPS_PROLOGUE;
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    gtkps_scroll (ps, newscroll);
}


void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}


void
on_add_files_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add file(s) to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Supported music files");

    codec_t *codecs[] = {
        &cdumb, &cvorbis, &cflac, &cgme, &cmp3, &csid, NULL
    };
    for (int i = 0; codecs[i]; i++) {
        if (codecs[i]->getexts && codecs[i]->insert) {
            const char **exts = codecs[i]->getexts ();
            if (exts) {
                for (int e = 0; exts[e]; e++) {
                    char filter[20];
                    snprintf (filter, 20, "*.%s", exts[e]);
                    gtk_file_filter_add_pattern (flt, filter);
                }
            }
        }
    }

    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Other files (*)");
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        GSList *lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (lst) {
            messagepump_push (M_ADDFILES, (uintptr_t)lst, 0, 0);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

void
on_add_folder1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add folder to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Supported music files");

    codec_t *codecs[] = {
        &cdumb, &cvorbis, &cflac, &cgme, &cmp3, &csid, NULL
    };
    for (int i = 0; codecs[i]; i++) {
        if (codecs[i]->getexts && codecs[i]->insert) {
            const char **exts = codecs[i]->getexts ();
            if (exts) {
                for (int e = 0; exts[e]; e++) {
                    char filter[20];
                    snprintf (filter, 20, "*.%s", exts[e]);
                    gtk_file_filter_add_pattern (flt, filter);
                }
            }
        }
    }

    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Other files (*)");
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        gchar *folder = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (folder) {
            messagepump_push (M_ADDDIR, (uintptr_t)folder, 0, 0);
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
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *d = create_aboutdialog ();
    gtk_dialog_run (GTK_DIALOG (d));
    gtk_widget_destroy (d);
}


gboolean
on_playlist_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    GTKPS_PROLOGUE;
	GdkEventScroll *ev = (GdkEventScroll*)event;
    gtkps_handle_scroll_event (ps, ev->direction);
    return FALSE;
}


void
on_stopbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_STOPSONG, 0, 0, 0);
}


void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PLAYSONG, 0, 0, 0);
}


void
on_pausebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PAUSESONG, 0, 0, 0);
}


void
on_prevbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PREVSONG, 0, 0, 0);
}


void
on_nextbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_NEXTSONG, 0, 0, 0);
}


void
on_playrand_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PLAYRANDOM, 0, 0, 0);
}


gboolean
on_mainwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    gtkps_keypress (&main_playlist, event->keyval, event->state);
    return FALSE;
}


void
on_playlist_drag_begin                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
}

gboolean
on_playlist_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    GTKPS_PROLOGUE;
    gtkps_track_dragdrop (ps, y);
    return FALSE;
}


gboolean
on_playlist_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
    if (drag_context->targets) {
        GdkAtom target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets, TARGET_SAMEWIDGET));
        if (!target_type) {
            return FALSE;
        }
        gtk_drag_get_data (widget, drag_context, target_type, time);
        return TRUE;
    }
    return FALSE;
}


void
on_playlist_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *selection_data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    switch (target_type) {
    case TARGET_SAMEWIDGET:
        {
            // format as "STRING" consisting of array of pointers
            int nsel = ps_getselcount ();
            if (!nsel) {
                break; // something wrong happened
            }
            uint32_t *ptr = malloc (nsel * sizeof (uint32_t));
            int idx = 0;
            int i = 0;
            for (playItem_t *it = playlist_head[PS_MAIN]; it; it = it->next[PS_MAIN], idx++) {
                if (it->selected) {
                    ptr[i] = idx;
                    i++;
                }
            }
            gtk_selection_data_set (selection_data, selection_data->target, sizeof (uint32_t) * 8, (gchar *)ptr, nsel * sizeof (uint32_t));
            free (ptr);
        }
        break;
    default:
        g_assert_not_reached ();
    }
}


void
on_playlist_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            target_type,
                                        guint            time,
                                        gpointer         user_data)
{
    GTKPS_PROLOGUE;
    gchar *ptr=(char*)data->data;
    if (target_type == 0) { // uris
        if (!strncmp(ptr,"file:///",8)) {
            gtkps_handle_fm_drag_drop (ps, y, ptr, data->length);
        }
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        gtkps_handle_drag_drop (ps, y, d, data->length/4);
    }
    gtk_drag_finish (drag_context, FALSE, FALSE, time);
}


void
on_playlist_drag_data_delete           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{
}


gboolean
on_playlist_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data)
{
    return TRUE;
}


void
on_playlist_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data)
{
    GTKPS_PROLOGUE;
    gtkps_track_dragdrop (ps, -1);
}

void
on_voice1_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (0, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}


void
on_voice2_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (1, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}


void
on_voice3_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (2, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}


void
on_voice4_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (3, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}


void
on_voice5_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    codec_lock ();
    if (playlist_current.codec && playlist_current.codec->mutevoice) {
        playlist_current.codec->mutevoice (4, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) ? 0 : 1);
    }
    codec_unlock ();
}

void
on_order_linear_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ps_set_order (0);
}


void
on_order_shuffle_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ps_set_order (1);
}


void
on_order_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ps_set_order (2);
}


void
on_loop_all_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ps_set_loop_mode (0);
}


void
on_loop_single_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ps_set_loop_mode (2);
}


void
on_loop_disable_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ps_set_loop_mode (1);
}

void
on_playlist_realize                    (GtkWidget       *widget,
        gpointer         user_data)
{
    printf ("on_playlist_realize\n");
    GtkTargetEntry entry = {
        .target = "STRING",
        .flags = GTK_TARGET_SAME_WIDGET/* | GTK_TARGET_OTHER_APP*/,
        TARGET_SAMEWIDGET
    };
    // setup drag-drop source
//    gtk_drag_source_set (widget, GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_MOVE);
    // setup drag-drop target
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP, &entry, 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_drag_dest_add_uri_targets (widget);
//    gtk_drag_dest_set_track_motion (widget, TRUE);
}

void
on_searchlist_realize                  (GtkWidget       *widget,
                                        gpointer         user_data)
{
    printf ("on_searchlist_realize\n");
}




