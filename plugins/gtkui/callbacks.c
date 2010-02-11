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

#include "gtkplaylist.h"
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

gtkplaylist_t *last_playlist;
extern GtkWidget *mainwin;
extern gtkplaylist_t main_playlist;
extern gtkplaylist_t search_playlist;
extern DB_functions_t *deadbeef; // defined in gtkui.c
GtkWidget *trackproperties;

gboolean
playlist_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer unused)
{
    DB_playItem_t *item = deadbeef->pl_get_for_idx (main_playlist.scrollpos + y / rowheight);
    if (item && item->fname) {
        gtk_tooltip_set_text (tooltip, item->fname);
        return TRUE;
    }
    return FALSE;
}

static int
main_get_count (void) {
    return deadbeef->pl_getcount (PL_MAIN);
}

static int
search_get_count (void) {
    return deadbeef->pl_getcount (PL_SEARCH);
}

void
main_playlist_init (GtkWidget *widget) {
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

    DB_conf_item_t *col = deadbeef->conf_find ("playlist.column.", NULL);
    if (!col) {
        // create default set of columns
        gtkpl_column_append (&main_playlist, gtkpl_column_alloc ("Playing", 50, DB_COLUMN_PLAYING, NULL, 0));
        gtkpl_column_append (&main_playlist, gtkpl_column_alloc ("Artist / Album", 150, DB_COLUMN_ARTIST_ALBUM, NULL, 0));
        gtkpl_column_append (&main_playlist, gtkpl_column_alloc ("Track №", 50, DB_COLUMN_TRACK, NULL, 1));
        gtkpl_column_append (&main_playlist, gtkpl_column_alloc ("Title / Track Artist", 150, DB_COLUMN_TITLE, NULL, 0));
        gtkpl_column_append (&main_playlist, gtkpl_column_alloc ("Duration", 50, DB_COLUMN_DURATION, NULL, 0));
    }
    else {
        while (col) {
            gtkpl_append_column_from_textdef (&main_playlist, col->value);
            col = deadbeef->conf_find ("playlist.column.", col);
        }
    }

    gtk_object_set_data (GTK_OBJECT (main_playlist.playlist), "ps", &main_playlist);
    gtk_object_set_data (GTK_OBJECT (main_playlist.header), "ps", &main_playlist);
    gtk_object_set_data (GTK_OBJECT (main_playlist.scrollbar), "ps", &main_playlist);
    gtk_object_set_data (GTK_OBJECT (main_playlist.hscrollbar), "ps", &main_playlist);

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

void
search_playlist_init (GtkWidget *widget) {
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
    else if (event->button == 3) {
        // get item under cursor
        int y = event->y / rowheight + ps->scrollpos;
        if (y < 0 || y >= ps->get_count ()) {
            y = -1;
        }
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (y, ps->iterator);
        if (!it) {
            // clicked empty space -- deselect everything and show insensitive menu
            deadbeef->pl_set_cursor (ps->iterator, -1);
            it = deadbeef->pl_get_first (ps->iterator);
            while (it) {
                SELECT (it, 0);
                it = PL_NEXT (it, ps->iterator);
            }
            playlist_refresh ();
            // no menu
        }
        else {
            if (!SELECTED (it)) {
                // item is unselected -- reset selection and select this
                DB_playItem_t *it2 = deadbeef->pl_get_first (ps->iterator);
                while (it2) {
                    if (SELECTED (it2) && it2 != it) {
                        SELECT (it2, 0);
                    }
                    else if (it2 == it) {
                        deadbeef->pl_set_cursor (ps->iterator, y);
                        SELECT (it2, 1);
                    }
                    it2 = PL_NEXT (it2, ps->iterator);
                }
                playlist_refresh ();
            }
            else {
                // something is selected; move cursor but keep selection
                deadbeef->pl_set_cursor (ps->iterator, y);
                playlist_refresh ();
            }
            {
                int inqueue = deadbeef->pl_playqueue_test (it);
                GtkWidget *playlist_menu;
                GtkWidget *add_to_playback_queue1;
                GtkWidget *remove_from_playback_queue1;
                GtkWidget *separator9;
                GtkWidget *remove2;
                GtkWidget *separator8;
                GtkWidget *properties1;

                playlist_menu = gtk_menu_new ();
                add_to_playback_queue1 = gtk_menu_item_new_with_mnemonic ("Add to playback queue");
                gtk_widget_show (add_to_playback_queue1);
                gtk_container_add (GTK_CONTAINER (playlist_menu), add_to_playback_queue1);
                gtk_object_set_data (GTK_OBJECT (add_to_playback_queue1), "ps", ps);

                remove_from_playback_queue1 = gtk_menu_item_new_with_mnemonic ("Remove from playback queue");
                if (inqueue == -1) {
                    gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
                }
                gtk_widget_show (remove_from_playback_queue1);
                gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_playback_queue1);
                gtk_object_set_data (GTK_OBJECT (remove_from_playback_queue1), "ps", ps);

                separator9 = gtk_separator_menu_item_new ();
                gtk_widget_show (separator9);
                gtk_container_add (GTK_CONTAINER (playlist_menu), separator9);
                gtk_widget_set_sensitive (separator9, FALSE);

                remove2 = gtk_menu_item_new_with_mnemonic ("Remove");
                gtk_widget_show (remove2);
                gtk_container_add (GTK_CONTAINER (playlist_menu), remove2);
                gtk_object_set_data (GTK_OBJECT (remove2), "ps", ps);

                separator8 = gtk_separator_menu_item_new ();
                gtk_widget_show (separator8);
                gtk_container_add (GTK_CONTAINER (playlist_menu), separator8);
                gtk_widget_set_sensitive (separator8, FALSE);

                properties1 = gtk_menu_item_new_with_mnemonic ("Properties");
                gtk_widget_show (properties1);
                gtk_container_add (GTK_CONTAINER (playlist_menu), properties1);
                gtk_object_set_data (GTK_OBJECT (properties1), "ps", ps);

                g_signal_connect ((gpointer) add_to_playback_queue1, "activate",
                        G_CALLBACK (on_add_to_playback_queue1_activate),
                        NULL);
                g_signal_connect ((gpointer) remove_from_playback_queue1, "activate",
                        G_CALLBACK (on_remove_from_playback_queue1_activate),
                        NULL);
                g_signal_connect ((gpointer) remove2, "activate",
                        G_CALLBACK (on_remove2_activate),
                        NULL);
                g_signal_connect ((gpointer) properties1, "activate",
                        G_CALLBACK (on_properties1_activate),
                        NULL);
                gtk_menu_popup (GTK_MENU (playlist_menu), NULL, NULL, NULL, widget, 0, gtk_get_current_event_time());
            }
        }
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
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_clear ();
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
    deadbeef->pl_select_all ();
    gtkplaylist_t *ps = &main_playlist;
    GtkWidget *widget = ps->playlist;
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gdk_draw_drawable (widget->window, widget->style->black_gc, ps->backbuf, 0, 0, 0, 0, widget->allocation.width, widget->allocation.height);
}


void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int cursor = deadbeef->pl_delete_selected ();
    if (cursor >= main_playlist.get_count ()) {
        cursor = main_playlist.get_count ()-1;
    }
    gtkpl_set_cursor (main_playlist.iterator, cursor);
    main_refresh ();
    search_refresh ();
}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkplaylist_t *ps = &main_playlist;
    GtkWidget *widget = ps->playlist;
    deadbeef->pl_crop_selected ();
    gtkpl_setup_scrollbar (ps);
    gtkpl_draw_playlist (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkpl_expose (ps, 0, 0, widget->allocation.width, widget->allocation.height);
    search_refresh ();
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
        gtkpl_keypress (&main_playlist, event->keyval, event->state);
    }
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
            int nsel = deadbeef->pl_getselcount ();
            if (!nsel) {
                break; // something wrong happened
            }
            uint32_t *ptr = malloc (nsel * sizeof (uint32_t));
            int idx = 0;
            int i = 0;
            DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
            for (; it; it = deadbeef->pl_get_next (it, PL_MAIN), idx++) {
                if (deadbeef->pl_is_selected (it)) {
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
        // this happens when dropped from file manager
        char *mem = malloc (data->length+1);
        memcpy (mem, ptr, data->length);
        mem[data->length] = 0;
        // we don't pass control structure, but there's only one drag-drop view currently
        gtkui_receive_fm_drop (mem, data->length, y);
    }
    else if (target_type == 1) {
        uint32_t *d= (uint32_t *)ptr;
        int length = data->length/4;
        int drop_row = y / rowheight + ps->scrollpos;
        DB_playItem_t *drop_before = deadbeef->pl_get_for_idx_and_iter (drop_row, ps->iterator);
        while (drop_before && SELECTED (drop_before)) {
            drop_before = PL_NEXT(drop_before, ps->iterator);
        }
        deadbeef->pl_move_items (ps->iterator, drop_before, d, length);
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
            /*int res = */deadbeef->pl_load (fname);
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
    float range = -deadbeef->volume_get_min_db ();
    int n = widget->allocation.width / 4;
    float vol = (range + deadbeef->volume_get_db ()) / range * n;
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
        float range = -deadbeef->volume_get_min_db ();
        float volume = event->x / widget->allocation.width * range - range;
        if (volume > 0) {
            volume = 0;
        }
        if (volume < -range) {
            volume = -range;
        }
        deadbeef->volume_set_db (volume);
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
    float range = -deadbeef->volume_get_min_db ();
    float volume = event->x / widget->allocation.width * range - range;
    if (volume < -range) {
        volume = -range;
    }
    if (volume > 0) {
        volume = 0;
    }
    deadbeef->volume_set_db (volume);
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

void
volumebar_notify_changed (void) {
    GtkWidget *widget = lookup_widget (mainwin, "volumebar");
    volumebar_draw (widget);
    volumebar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
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
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event,
                                        gpointer         user_data)
{
    float range = -deadbeef->volume_get_min_db ();
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
    else if (vol < -range) {
        vol = -range;
    }
    deadbeef->volume_set_db (vol);
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    volumebar_draw (volumebar);
    volumebar_expose (volumebar, 0, 0, volumebar->allocation.width, volumebar->allocation.height);
    return FALSE;
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
on_playhscroll_value_changed           (GtkRange        *widget,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    gtkpl_hscroll (ps, newscroll);
}


void
on_searchhscroll_value_changed         (GtkRange        *widget,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    int newscroll = gtk_range_get_value (GTK_RANGE (widget));
    gtkpl_hscroll (ps, newscroll);
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
on_add_audio_cd_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_add_file ("all.cda", NULL, NULL);
    playlist_refresh ();
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
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkplaylist_t *ps = last_playlist;
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
        gtkpl_column_insert_before (ps, ps->active_column, gtkpl_column_alloc (title, 100, id, format, align));
        gtkpl_setup_hscrollbar (ps);
        gtkpl_header_draw (ps);
        gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);

        gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
        gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    }
    gtk_widget_destroy (dlg);
}


void
on_edit_column_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkplaylist_t *ps = last_playlist;
    if (!ps->active_column)
        return;
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_window_set_title (GTK_WINDOW (dlg), "Edit column");
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), ps->active_column->title);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "format")), ps->active_column->format);
    if (ps->active_column->id == -1) {
        gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), DB_COLUMN_ID_MAX);
    }
    else {
        gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), ps->active_column->id);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), ps->active_column->align_right);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        if (id >= DB_COLUMN_ID_MAX) {
            id = -1;
        }
        free (ps->active_column->title);
        free (ps->active_column->format);
        ps->active_column->title = strdup (title);
        ps->active_column->format = strdup (format);
        ps->active_column->id = id;
        ps->active_column->align_right = align;
        gtkpl_column_rewrite_config (ps);

        gtkpl_header_draw (ps);
        gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);

        gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
        gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    }
    gtk_widget_destroy (dlg);
}


void
on_remove_column_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkplaylist_t *ps = last_playlist;

    if (!ps->active_column)
        return;

    gtkpl_column_remove (ps, ps->active_column);

    gtkpl_setup_hscrollbar (ps);
    gtkpl_header_draw (ps);
    gtkpl_expose_header (ps, 0, 0, ps->header->allocation.width, ps->header->allocation.height);

    gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);

    gtkpl_column_rewrite_config (ps);
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
on_add_location_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg= create_addlocationdlg ();
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
on_add_to_playback_queue1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *widget = GTK_WIDGET (menuitem);
    GTKPL_PROLOGUE;
    DB_playItem_t *it = deadbeef->pl_get_first (ps->iterator);
    while (it) {
        if (SELECTED (it)) {
            deadbeef->pl_playqueue_push (it);
        }
        it = PL_NEXT (it, ps->iterator);
    }
    playlist_refresh ();
}


void
on_remove_from_playback_queue1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *widget = GTK_WIDGET (menuitem);
    GTKPL_PROLOGUE;
    DB_playItem_t *it = deadbeef->pl_get_first (ps->iterator);
    while (it) {
        if (SELECTED (it)) {
            deadbeef->pl_playqueue_remove (it);
        }
        it = PL_NEXT (it, ps->iterator);
    }
    playlist_refresh ();
}


void
on_remove2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *widget = GTK_WIDGET (menuitem);
    GTKPL_PROLOGUE;
    int cursor = deadbeef->pl_delete_selected ();
    if (cursor >= ps->get_count ()) {
        cursor = ps->get_count ()-1;
    }
    main_refresh ();
    search_refresh ();
    gtkpl_set_cursor (ps->iterator, cursor);
}


void
on_properties1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *widget = GTK_WIDGET (menuitem);
    GTKPL_PROLOGUE;
    GtkWidget *w;
    const char *meta;
    DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (deadbeef->pl_get_cursor (ps->iterator), ps->iterator);
    if (!it) {
        fprintf (stderr, "attempt to view properties of non-existing item\n");
        return;
    }
    if (!trackproperties) {
        trackproperties = create_trackproperties ();
        gtk_window_set_transient_for (GTK_WINDOW (trackproperties), GTK_WINDOW (mainwin));
    }
    widget = trackproperties;
    // fill in metadata
    // location
    w = lookup_widget (widget, "location");
    gtk_entry_set_text (GTK_ENTRY (w), it->fname);
    // title
    w = lookup_widget (widget, "title");
    meta = deadbeef->pl_find_meta (it, "title");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // artist
    w = lookup_widget (widget, "artist");
    meta = deadbeef->pl_find_meta (it, "artist");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // album
    w = lookup_widget (widget, "album");
    meta = deadbeef->pl_find_meta (it, "album");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // genre
    w = lookup_widget (widget, "genre");
    meta = deadbeef->pl_find_meta (it, "genre");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // year
    w = lookup_widget (widget, "year");
    meta = deadbeef->pl_find_meta (it, "year");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // track
    w = lookup_widget (widget, "track");
    meta = deadbeef->pl_find_meta (it, "track");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // comment
    w = lookup_widget (widget, "comment");
    meta = deadbeef->pl_find_meta (it, "comment");
    if (!meta) {
        meta = "";
    }
    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
    gtk_text_buffer_set_text (buffer, meta, strlen (meta));
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (w), buffer);
    g_object_unref (buffer);

    gtk_widget_show (widget);
    gtk_window_present (GTK_WINDOW (widget));
}

gboolean
on_trackproperties_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if (event->keyval == GDK_Escape) {
        trackproperties = NULL;
        gtk_widget_destroy (widget);
    }
    return FALSE;
}




void
on_cursor_follows_playback_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.scroll.cursorfollowplayback", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
}




gboolean
on_trackproperties_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    trackproperties = NULL;
    return FALSE;
}

