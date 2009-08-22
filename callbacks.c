/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include "streamer.h"
#include "progress.h"
#include "volume.h"

#include "cvorbis.h"
#include "cdumb.h"
#include "cmp3.h"
#include "cgme.h"
#include "cflac.h"
#include "csid.h"

extern GtkWidget *mainwin;
extern gtkplaylist_t main_playlist;
extern gtkplaylist_t search_playlist;

void
main_playlist_init (GtkWidget *widget) {
    // init playlist control structure, and put it into widget user-data
    memset (&main_playlist, 0, sizeof (main_playlist));
    main_playlist.playlist = widget;
    main_playlist.header = lookup_widget (mainwin, "header");
    main_playlist.scrollbar = lookup_widget (mainwin, "playscroll");
    main_playlist.pcurr = &playlist_current_ptr;
    main_playlist.pcount = &pl_count;
    main_playlist.iterator = PL_MAIN;
    main_playlist.multisel = 1;
    main_playlist.scrollpos = 0;
    main_playlist.row = -1;
    main_playlist.clicktime = -1;
    main_playlist.nvisiblerows = 0;
    main_playlist.fmtcache = NULL;
    int colwidths[pl_ncolumns] = { 50, 150, 50, 150, 50 };
    memcpy (main_playlist.colwidths, colwidths, sizeof (colwidths));
    gtk_object_set_data (GTK_OBJECT (main_playlist.playlist), "ps", &main_playlist);
    gtk_object_set_data (GTK_OBJECT (main_playlist.header), "ps", &main_playlist);
    gtk_object_set_data (GTK_OBJECT (main_playlist.scrollbar), "ps", &main_playlist);
}

void
search_playlist_init (GtkWidget *widget) {
    extern GtkWidget *searchwin;
    // init playlist control structure, and put it into widget user-data
    memset (&search_playlist, 0, sizeof (search_playlist));
    search_playlist.playlist = widget;
    search_playlist.header = lookup_widget (searchwin, "searchheader");
    search_playlist.scrollbar = lookup_widget (searchwin, "searchscroll");
    assert (search_playlist.header);
    assert (search_playlist.scrollbar);
    //    main_playlist.pcurr = &search_current;
    search_playlist.pcount = &search_count;
    search_playlist.multisel = 0;
    search_playlist.iterator = PL_SEARCH;
    search_playlist.scrollpos = 0;
    search_playlist.row = -1;
    search_playlist.clicktime = -1;
    search_playlist.nvisiblerows = 0;
    search_playlist.fmtcache = NULL;
    int colwidths[pl_ncolumns] = { 0, 150, 50, 150, 50 };
    memcpy (search_playlist.colwidths, colwidths, sizeof (colwidths));
    gtk_object_set_data (GTK_OBJECT (search_playlist.playlist), "ps", &search_playlist);
    gtk_object_set_data (GTK_OBJECT (search_playlist.header), "ps", &search_playlist);
    gtk_object_set_data (GTK_OBJECT (search_playlist.scrollbar), "ps", &search_playlist);
}

// redraw
gboolean
on_playlist_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    // draw visible area of playlist
    gtkpl_expose (ps, event->area.x, event->area.y, event->area.width, event->area.height);

    return FALSE;
}


gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    if (event->button == 1) {
        gtkpl_mouse1_pressed (ps, event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
on_playlist_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    if (event->button == 1) {
        gtkpl_mouse1_released (ps, event->state, event->x, event->y, event->time);
    }
    return FALSE;
}

gboolean
on_playlist_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    gtkpl_mousemove (ps, event);
    return FALSE;
}


void
on_playscroll_value_changed            (GtkRange        *widget,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    gtkpl_scroll (ps, newscroll);
}


void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Open file(s)...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

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
                    char *p;
                    for (p = filter; *p; p++) {
                        *p = toupper (*p);
                    }
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
        pl_free ();
        GSList *lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (lst) {
            messagepump_push (M_OPENFILES, (uintptr_t)lst, 0, 0);
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
                    char *p;
                    for (p = filter; *p; p++) {
                        *p = toupper (*p);
                    }
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
                    char *p;
                    for (p = filter; *p; p++) {
                        *p = toupper (*p);
                    }
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
    progress_abort ();
    messagepump_push (M_TERMINATE, 0, 0, 0);
}


void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    pl_free ();
    gtkplaylist_t *ps = &main_playlist;
    GtkWidget *widget = ps->playlist;
    gtkpl_setup_scrollbar (ps);
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkpl_expose (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    search_refresh ();
}


void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    pl_select_all ();
    gtkplaylist_t *ps = &main_playlist;
    GtkWidget *widget = ps->playlist;
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
}


void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkplaylist_t *ps = &main_playlist;
    GtkWidget *widget = ps->playlist;
    pl_delete_selected ();
    gtkpl_setup_scrollbar (ps);
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkpl_expose (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    search_refresh ();
}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkplaylist_t *ps = &main_playlist;
    GtkWidget *widget = ps->playlist;
    pl_crop_selected ();
    gtkpl_setup_scrollbar (ps);
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkpl_expose (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    search_refresh ();
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
    GTKPL_PROLOGUE;
	GdkEventScroll *ev = (GdkEventScroll*)event;
    gtkpl_handle_scroll_event (ps, ev->direction);
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
    gtkpl_keypress (&main_playlist, event->keyval, event->state);
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
    GTKPL_PROLOGUE;
    gtkpl_track_dragdrop (ps, y);
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
#if 0
    if (drag_context->targets) {
        GdkAtom target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets, TARGET_SAMEWIDGET));
        if (!target_type) {
            return FALSE;
        }
        gtk_drag_get_data (widget, drag_context, target_type, time);
        return TRUE;
    }
#endif
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
            int nsel = pl_getselcount ();
            if (!nsel) {
                break; // something wrong happened
            }
            uint32_t *ptr = malloc (nsel * sizeof (uint32_t));
            int idx = 0;
            int i = 0;
            for (playItem_t *it = playlist_head[PL_MAIN]; it; it = it->next[PL_MAIN], idx++) {
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
    GTKPL_PROLOGUE;
    gchar *ptr=(char*)data->data;
    if (target_type == 0) { // uris
        if (!strncmp(ptr,"file:///",8)) {
            gtkpl_handle_fm_drag_drop (ps, y, ptr, data->length);
        }
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        gtkpl_handle_drag_drop (ps, y, d, data->length/4);
    }
    gtk_drag_finish (drag_context, TRUE, FALSE, time);
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
    GTKPL_PROLOGUE;
    gtkpl_track_dragdrop (ps, -1);
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
    pl_set_order (0);
}


void
on_order_shuffle_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    pl_set_order (1);
}


void
on_order_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    pl_set_order (2);
}


void
on_loop_all_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    pl_set_loop_mode (0);
}


void
on_loop_single_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    pl_set_loop_mode (2);
}


void
on_loop_disable_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    pl_set_loop_mode (1);
}

void
on_playlist_realize                    (GtkWidget       *widget,
        gpointer         user_data)
{
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
            int res = pl_load (fname);
            printf ("load result: %d\n", res);
            g_free (fname);
            gtkplaylist_t *ps = &main_playlist;
            gtkpl_setup_scrollbar (ps);
            gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
            gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
            search_refresh ();
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

char last_playlist_save_name[1024] = "";

void
save_playlist_as (void) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Save Playlist As", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

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
            int res = pl_save (fname);
            printf ("save as res: %d\n", res);
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
        int res = pl_save (last_playlist_save_name);
        printf ("save res: %d\n", res);
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
        if (playlist_current.codec && playlist_current.duration > 0) {
            pos = streamer_get_playpos () / playlist_current.duration;
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
    float time = event->x * playlist_current.duration / (widget->allocation.width-8);
    if (time < 0) {
        time = 0;
    }
    streamer_set_seek (time);
//    messagepump_push (M_SONGSEEK, 0, time * 1000, 0);
    return FALSE;
}



static GdkPixmap *volumebar_backbuf;

void
volumebar_draw (GtkWidget *widget) {
    if (!widget) {
        return;
    }
    gdk_draw_rectangle (volumebar_backbuf, widget->style->bg_gc[0], TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
	cairo_t *cr;
	cr = gdk_cairo_create (volumebar_backbuf);
	if (!cr) {
        return;
    }

    int n = widget->allocation.width / 4;
    float vol = (60.f + volume_get_db ()) / 60.f * n;
    float h = 16;
    for (int i = 0; i < n; i++) {
        float iy = (float)i + 3;
        if (i <= vol) {
            theme_set_cairo_source_rgb (cr, COLO_VOLUMEBAR_FRONT);
        }
        else {
            theme_set_cairo_source_rgb (cr, COLO_VOLUMEBAR_BACK);
        }
        cairo_rectangle (cr, i * 4, (widget->allocation.height/2-h/2) + h - 1 - (h* i / n), 3, h * iy / n);
        cairo_fill (cr);
    }

    cairo_destroy (cr);
}

void
volumebar_expose (GtkWidget *widget, int x, int y, int w, int h) {
	gdk_draw_drawable (widget->window, widget->style->black_gc, volumebar_backbuf, x, y, x, y, w, h);
}

gboolean
on_volumebar_configure_event           (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    if (volumebar_backbuf) {
        g_object_unref (volumebar_backbuf);
        volumebar_backbuf = NULL;
    }
    volumebar_backbuf = gdk_pixmap_new (widget->window, widget->allocation.width, widget->allocation.height, -1);
    volumebar_draw (widget);
    return FALSE;
}

gboolean
on_volumebar_expose_event              (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
    volumebar_expose (widget, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}


gboolean
on_volumebar_motion_notify_event       (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    if (event->state & GDK_BUTTON1_MASK) {
        float volume = event->x / widget->allocation.width * 60.f - 60.f;
        if (volume > 0) {
            volume = 0;
        }
        if (volume < -60) {
            volume = -60;
        }
        volume_set_db (volume);
        volumebar_draw (widget);
        volumebar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    }
    return FALSE;
}

gboolean
on_volumebar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    float volume = event->x / widget->allocation.width * 60.f - 60.f;
    if (volume < -60) {
        volume = -60;
    }
    if (volume > 0) {
        volume = 0;
    }
    volume_set_db (volume);
    volumebar_draw (widget);
    volumebar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    return FALSE;
}


gboolean
on_volumebar_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_mainwin_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
//    messagepump_push (M_TERMINATE, 0, 0, 0);

    gtk_widget_hide (widget);
    return TRUE;
}




gboolean
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event,
                                        gpointer         user_data)
{
    float vol = volume_get_db ();
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
    volume_set_db (vol);
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    volumebar_draw (volumebar);
    volumebar_expose (volumebar, 0, 0, volumebar->allocation.width, volumebar->allocation.height);
    return FALSE;
}


