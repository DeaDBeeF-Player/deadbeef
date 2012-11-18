/*
    gtkui hotkey handlers
    Copyright (C) 2009-2012 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "../../gettext.h"
#include "../../deadbeef.h"
#include "gtkui.h"
#include "progress.h"
#include "ddblistview.h"
#include "search.h"
#include "support.h"
#include "wingeom.h"
#include "interface.h"
#include "trkproperties.h"

extern GtkWidget *mainwin;
extern DB_functions_t *deadbeef;

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

gboolean
action_open_files_handler_cb (void *userdata) {
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
    return FALSE;
}

int
action_open_files_handler (struct DB_plugin_action_s *action, int ctx) {
    gdk_threads_add_idle (action_open_files_handler_cb, NULL);
    return 0;
}

gboolean
action_add_files_handler_cb (void *user_data) {
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
    return FALSE;
}

int
action_add_files_handler (struct DB_plugin_action_s *action, int ctx) {
    gdk_threads_add_idle (action_add_files_handler_cb, NULL);
    return 0;
}

static void
on_follow_symlinks_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("add_folders_follow_symlinks", gtk_toggle_button_get_active (togglebutton));
}

gboolean
action_add_folders_handler_cb (void *user_data) {
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
    return FALSE;
}

int
action_add_folders_handler (struct DB_plugin_action_s *action, int ctx) {
    gdk_threads_add_idle (action_add_folders_handler_cb, NULL);
    return 0;
}

gboolean
action_quit_handler_cb (void *user_data) {
    progress_abort ();
    deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
    return FALSE;
}

int
action_quit_handler (DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_quit_handler_cb, NULL);
    return 0;
}

gboolean
action_deselect_all_handler_cb (void *user_data) {
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
    return FALSE;
}

int
action_deselect_all_handler (struct DB_plugin_action_s *action, int ctx) {
    g_idle_add (action_deselect_all_handler_cb, NULL);
    return 0;
}

gboolean
action_select_all_handler_cb (void *user_data) {
    deadbeef->pl_select_all ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    DdbListview *pl = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    if (pl) {
        ddb_listview_refresh (pl, DDB_REFRESH_LIST);
    }
    return FALSE;
}

int
action_select_all_handler (struct DB_plugin_action_s *action, int ctx) {
    g_idle_add (action_select_all_handler_cb, NULL);
    return 0;
}

gboolean
action_new_playlist_handler_cb (void *user_data) {
    int pl = gtkui_add_new_playlist ();
    if (pl != -1) {
        deadbeef->plt_set_curr_idx (pl);
        deadbeef->conf_set_int ("playlist.current", pl);
    }
    return FALSE;
}

int
action_new_playlist_handler (struct DB_plugin_action_s *action, int ctx) {
    gdk_threads_add_idle (action_new_playlist_handler_cb, NULL);
    return 0;
}

int
action_remove_current_playlist_handler (struct DB_plugin_action_s *action, int ctx) {
    int idx = deadbeef->plt_get_curr_idx ();
    if (idx != -1) {
        deadbeef->plt_remove (idx);
    }
    return 0;
}

gboolean
action_toggle_mainwin_handler_cb (void *user_data) {
    mainwin_toggle_visible ();
    return FALSE;
}

int
action_toggle_mainwin_handler (struct DB_plugin_action_s *action, int ctx) {
    g_idle_add (action_toggle_mainwin_handler_cb, NULL);
    return 0;
}

gboolean
action_show_mainwin_handler_cb (void *user_data) {
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (!(gtk_widget_get_visible (mainwin) && !iconified)) {
        wingeom_restore (mainwin, "mainwin", 40, 40, 500, 300, 0);
        if (iconified) {
            gtk_window_deiconify (GTK_WINDOW(mainwin));
        }
        else {
            gtk_window_present (GTK_WINDOW (mainwin));
        }
    }
    return FALSE;
}

int
action_show_mainwin_handler (struct DB_plugin_action_s *action, int ctx) {
    g_idle_add (action_show_mainwin_handler_cb, NULL);
    return 0;
}

gboolean
action_hide_mainwin_handler_cb (void *user_data) {
    int iconified = gdk_window_get_state(gtk_widget_get_window(mainwin)) & GDK_WINDOW_STATE_ICONIFIED;
    if (gtk_widget_get_visible (mainwin) && !iconified) {
        gtk_widget_hide (mainwin);
    }
    return FALSE;
}

int
action_hide_mainwin_handler (struct DB_plugin_action_s *action, int ctx) {
    g_idle_add (action_hide_mainwin_handler_cb, NULL);
    return 0;
}

gboolean
action_add_location_handler_cb (void *user_data) {
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
    return FALSE;
}

int
action_add_location_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_add_location_handler_cb, NULL);
    return 0;
}

static GtkWidget *helpwindow;

gboolean
action_show_help_handler_cb (void *user_data) {
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), _("help.txt"));
    gtkui_show_info_window (fname, _("Help"), &helpwindow);
    return FALSE;
}

int
action_show_help_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_show_help_handler_cb, NULL);
    return 0;
}

int
action_remove_from_playlist_handler (DB_plugin_action_t *act, int ctx) {
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            int cursor = deadbeef->plt_delete_selected (plt);
            if (cursor != -1) {
                DB_playItem_t *it = deadbeef->plt_get_item_for_idx (plt, cursor, PL_MAIN);
                if (it) {
                    deadbeef->pl_set_selected (it, 1);
                    deadbeef->pl_item_unref (it);
                }
            }
            deadbeef->plt_unref (plt);
            deadbeef->pl_save_all ();
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
        }
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        deadbeef->pl_clear ();
        deadbeef->pl_save_all ();
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        int success = 0;
        deadbeef->pl_lock ();
        DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
        if (it) {
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                int idx = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
                if (idx != -1) {
                    deadbeef->plt_remove_item (plt, it);
                    success = 1;
                }
                deadbeef->plt_unref (plt);
            }
            deadbeef->pl_item_unref (it);
        }
        deadbeef->pl_unlock ();
        if (success) {
            deadbeef->pl_save_all ();
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
        }
    }
    return 0;
}

gboolean
action_delete_from_disk_handler_cb (void *data) {
    int ctx = (intptr_t)data;
    if (deadbeef->conf_get_int ("gtkui.delete_files_ask", 1)) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Delete files from disk"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Files will be lost. Proceed?\n(This dialog can be turned off in GTKUI plugin settings)"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return FALSE;
        }
    }

    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (!plt) {
        return FALSE;
    }
    deadbeef->pl_lock ();

    if (ctx == DDB_ACTION_CTX_SELECTION) {
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->pl_is_selected (it) && deadbeef->is_local_file (uri)) {
                unlink (uri);
                deadbeef->plt_remove_item (plt, it);
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }

        deadbeef->pl_save_all ();
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->is_local_file (uri)) {
                unlink (uri);
                deadbeef->plt_remove_item (plt, it);
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }

        deadbeef->pl_save_all ();
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
        if (it) {
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->is_local_file (uri)) {
                int idx = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
                if (idx != -1) {
                    unlink (uri);
                    deadbeef->plt_remove_item (plt, it);
                }
            }
            deadbeef->pl_item_unref (it);
        }
    }
    deadbeef->pl_unlock ();
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    return FALSE;
}

int
action_delete_from_disk_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_delete_from_disk_handler_cb, (void *)(intptr_t)ctx);
    return 0;
}

gboolean
action_show_track_properties_handler_cb (void *data) {
    show_track_properties_dlg ((intptr_t)data);
    return FALSE;
}

int
action_show_track_properties_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_show_track_properties_handler_cb, (void *)(intptr_t)ctx);
    return 0;
}
