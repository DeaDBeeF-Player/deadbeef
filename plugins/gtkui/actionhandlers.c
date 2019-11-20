/*
    gtkui hotkey handlers
    Copyright (C) 2009-2013 Alexey Yakovenko and other contributors

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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <unistd.h>
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
#include "callbacks.h"
#include <sys/stat.h>

// disable custom title function, until we have new title formatting (0.7)
#define DISABLE_CUSTOM_TITLE

extern GtkWidget *mainwin;
extern DB_functions_t *deadbeef;
static DB_plugin_t *plugin;
#define trace(...) { deadbeef->log_detailed (plugin, 0, __VA_ARGS__); }

gboolean
action_open_files_handler_cb (void *userdata) {
    GSList *lst = show_file_chooser(_("Open file(s)..."), GTKUI_FILECHOOSER_OPENFILE, TRUE);
    if (lst) {
        gtkui_open_files (lst);
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
    GSList *lst = show_file_chooser(_("Add file(s) to playlist..."), GTKUI_FILECHOOSER_OPENFILE, TRUE);
    if (lst) {
        gtkui_add_files (lst);
    }
    return FALSE;
}

int
action_add_files_handler (struct DB_plugin_action_s *action, int ctx) {
    gdk_threads_add_idle (action_add_files_handler_cb, NULL);
    return 0;
}

gboolean
action_add_folders_handler_cb (void *user_data) {
    GSList *lst = show_file_chooser(_("Add folder(s) to playlist..."), GTKUI_FILECHOOSER_OPENFOLDER, TRUE);
    if (lst) {
        gtkui_add_dirs (lst);
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
    gtkui_quit ();
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
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

// FIXME: this functionality was supposed to be enabled in 0.7.0, but was forgotten
#ifndef DISABLE_CUSTOM_TITLE
static void
on_toggle_set_custom_title (GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("gtkui.location_set_custom_title", active);

    GtkWidget *ct = lookup_widget (GTK_WIDGET (user_data), "custom_title");
    gtk_widget_set_sensitive (ct, active);

    deadbeef->conf_save ();
}
#endif

gboolean
action_add_location_handler_cb (void *user_data) {
    GtkWidget *dlg = create_addlocationdlg ();

    GtkWidget *sct = lookup_widget (dlg, "set_custom_title");
    GtkWidget *ct = lookup_widget (dlg, "custom_title");

#ifndef DISABLE_CUSTOM_TITLE
    if (deadbeef->conf_get_int ("gtkui.location_set_custom_title", 0)) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sct), TRUE);
        gtk_widget_set_sensitive (ct, TRUE);
    }
    else
#endif
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sct), FALSE);
        gtk_widget_set_sensitive (ct, FALSE);
    }

#ifndef DISABLE_CUSTOM_TITLE
    g_signal_connect ((gpointer) sct, "toggled",
            G_CALLBACK (on_toggle_set_custom_title),
            dlg);
#endif

    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        GtkEntry *entry = GTK_ENTRY (lookup_widget (dlg, "addlocation_entry"));
        if (entry) {
            const char *text = gtk_entry_get_text (entry);
            if (text) {
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (!deadbeef->plt_add_files_begin (plt, 0)) {
                    DB_playItem_t *tail = deadbeef->plt_get_last (plt, PL_MAIN);
#ifndef DISABLE_CUSTOM_TITLE
                    DB_playItem_t *it = deadbeef->plt_insert_file2 (0, plt, tail, text, NULL, NULL, NULL);
                    if (it && deadbeef->conf_get_int ("gtkui.location_set_custom_title", 0)) {
                        deadbeef->pl_replace_meta (it, ":CUSTOM_TITLE", gtk_entry_get_text (GTK_ENTRY (ct)));
                    }
#endif
                    if (tail) {
                        deadbeef->pl_item_unref (tail);
                    }
                    deadbeef->plt_add_files_end (plt, 0);
                    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
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
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_system_dir(DDB_SYS_DIR_DOC), _("help.txt"));
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
            deadbeef->plt_save_config (plt);
            deadbeef->plt_unref (plt);
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        }
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        ddb_playlist_t *plt_curr = deadbeef->plt_get_curr ();
        ddb_playlist_t *plt = deadbeef->action_get_playlist ();
        deadbeef->plt_clear (plt);
        deadbeef->plt_save_config (plt);
        if (plt == plt_curr) {
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        }
        deadbeef->plt_unref (plt);
        deadbeef->plt_unref (plt_curr);
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        deadbeef->pl_lock ();
        DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
        if (it) {
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                int idx = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
                if (idx != -1) {
                    deadbeef->plt_remove_item (plt, it);
                    deadbeef->plt_save_config (plt);
                    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
                }
                deadbeef->plt_unref (plt);
            }
            deadbeef->pl_item_unref (it);
        }
        deadbeef->pl_unlock ();
    }
    return 0;
}

void
remove_deleted_file_from_all_playlists (const char *search_uri) {
    // The caller is responsible for pl_lock
    int n = deadbeef->plt_get_count ();
    for (int i = 0; i < n; ++i) {
        ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (strcmp (uri, search_uri) == 0) {
                deadbeef->plt_remove_item (plt, it);
            }
            deadbeef->pl_item_unref (it);
            it = next;
        }

        deadbeef->plt_unref (plt);
    }
}

void
delete_and_remove_track (const char *uri, ddb_playlist_t *plt, ddb_playItem_t *it) {
    (void)unlink (uri);

    // check if file exists
    struct stat buf;
    memset (&buf, 0, sizeof (buf));
    int stat_res = stat (uri, &buf);
    
    if (stat_res != 0) {
        deadbeef->plt_remove_item (plt, it);
        remove_deleted_file_from_all_playlists (uri);
    } else {
        trace("Failed to delete file: %s\n", uri);
    }
}

gboolean
action_delete_from_disk_handler_cb (void *data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (!plt) {
        return FALSE;
    }
    
    int ctx = (int)(intptr_t)data;
    if (deadbeef->conf_get_int ("gtkui.delete_files_ask", 1)) {
        char buf[1000];
        const char *buf2 = _(" The files will be lost.\n\n(This dialog can be turned off in GTKUI plugin settings)");

        if (ctx == DDB_ACTION_CTX_SELECTION) {
            int selected_files = deadbeef->pl_getselcount ();
            if (selected_files == 1) {
                snprintf(buf, sizeof (buf), _("Do you really want to delete the selected file?%s"), buf2);
            } else {
                snprintf(buf, sizeof (buf), _("Do you really want to delete all %d selected files?%s"), selected_files, buf2);
            }
        }
        else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
            int files = deadbeef->plt_get_item_count (plt, PL_MAIN);
            snprintf(buf, sizeof (buf), _("Do you really want to delete all %d files from the current playlist?%s"), files, buf2);
        }
        else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
            snprintf(buf, sizeof (buf), _("Do you really want to delete the currently playing file?%s"), buf2);
        }

        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Delete files from disk"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), buf);
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return FALSE;
        }
    }
    deadbeef->pl_lock ();
    
    DB_playItem_t *it_current_song = deadbeef->streamer_get_playing_track ();
    int idx_current_song = -1;
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->pl_is_selected (it) && deadbeef->is_local_file (uri)) {
                if (it == it_current_song) {
                    idx_current_song = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
                }
                delete_and_remove_track (uri, plt, it);
            }
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->is_local_file (uri)) {
                delete_and_remove_track (uri, plt, it);
            }
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
        if (it) {
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->is_local_file (uri)) {
                int idx = idx_current_song = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
                if (idx != -1) {
                    delete_and_remove_track (uri, plt, it);
                }
            }
            deadbeef->pl_item_unref (it);
        }
    }

    deadbeef->pl_save_all ();
    deadbeef->pl_item_unref (it_current_song);
    deadbeef->pl_unlock ();
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    
    if (deadbeef->conf_get_int ("gtkui.skip_deleted_songs", 0) 
        && deadbeef->plt_get_item_idx (plt, it_current_song, PL_MAIN) == -1 
        && deadbeef->streamer_get_current_playlist () == deadbeef->plt_get_curr_idx () 
        && deadbeef->get_output ()->state () == OUTPUT_STATE_PLAYING) {
        
        if (idx_current_song != -1 
            && deadbeef->playqueue_get_count () == 0 
            && deadbeef->streamer_get_shuffle () == DDB_SHUFFLE_OFF) {
            deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, idx_current_song, 0);
        }
        else {
            deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
        }
    }
    
    return FALSE;
}

int
action_delete_from_disk_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_delete_from_disk_handler_cb, (void *)(intptr_t)ctx);
    return 0;
}

typedef struct {
    int ctx;
    ddb_playlist_t *plt;
} trkproperties_action_ctx_t;

gboolean
action_show_track_properties_handler_cb (void *data) {
    trkproperties_action_ctx_t *ctx = data;
    show_track_properties_dlg (ctx->ctx, ctx->plt);
    deadbeef->plt_unref (ctx->plt);
    free (data);
    return FALSE;
}

int
action_show_track_properties_handler (DB_plugin_action_t *act, int ctx) {
    trkproperties_action_ctx_t *data = calloc (1, sizeof (trkproperties_action_ctx_t));
    data->ctx = ctx;
    data->plt = deadbeef->action_get_playlist ();
    gdk_threads_add_idle (action_show_track_properties_handler_cb, data);
    return 0;
}

gboolean
action_find_handler_cb (void *data) {
    search_start ();
    return FALSE;
}

int
action_find_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_find_handler_cb, NULL);
    return 0;
}

gboolean
action_scroll_follows_playback_handler_cb (void *data) {
    int val = 1 - deadbeef->conf_get_int ("playlist.scroll.followplayback", 1);
    deadbeef->conf_set_int ("playlist.scroll.followplayback", val);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "scroll_follows_playback")), val);
    return FALSE;
}

int
action_scroll_follows_playback_handler (DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_scroll_follows_playback_handler_cb, NULL);
    return 0;
}

gboolean
action_cursor_follows_playback_handler_cb (void *data) {
    int val = 1 - deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1);
    deadbeef->conf_set_int ("playlist.scroll.cursorfollowplayback", val);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "cursor_follows_playback")), val);
    return FALSE;
}

int
action_cursor_follows_playback_handler (DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_cursor_follows_playback_handler_cb, NULL);
    return 0;
}

static void
load_playlist_thread (void *data) {
    char *fname = data;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        if (!deadbeef->plt_add_files_begin (plt, 0)) {
            deadbeef->plt_clear (plt);
            int abort = 0;
            (void)deadbeef->plt_load2 (0, plt, NULL, fname, &abort, NULL, NULL);
            deadbeef->plt_save_config (plt);
            deadbeef->plt_add_files_end (plt, 0);
        }
        deadbeef->plt_unref (plt);
    }
    g_free (fname);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

gboolean
action_load_playlist_handler_cb (void *data) {
    GSList *lst = show_file_chooser(_("Load Playlist"), GTKUI_FILECHOOSER_LOADPLAYLIST, FALSE);

    if (lst)
    {
        gchar *fname = g_slist_nth_data (lst, 0);
        if (fname) {
            uintptr_t tid = deadbeef->thread_start (load_playlist_thread, fname);
            deadbeef->thread_detach (tid);
        }
        g_slist_free(lst);
    }
    return FALSE;
}

int
action_load_playlist_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_load_playlist_handler_cb, NULL);
    return 0;
}

gboolean
action_save_playlist_handler_cb (void *data) {
    GSList *lst = show_file_chooser(_("Save Playlist As"), GTKUI_FILECHOOSER_SAVEPLAYLIST, FALSE);


    if (lst)
    {
        gchar *fname = g_slist_nth_data (lst, 0);

        if (fname) {
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                int res = deadbeef->plt_save (plt, NULL, NULL, fname, NULL, NULL, NULL);
                if (res >= 0 && strlen (fname) < 1024) {
                    deadbeef->conf_set_str ("gtkui.last_playlist_save_name", fname);
                }
                deadbeef->plt_unref (plt);
            }
            g_free (fname);
            g_slist_free(lst);
        }
    }
    return FALSE;
}

int
action_save_playlist_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_save_playlist_handler_cb, NULL);
    return 0;
}

gboolean
action_toggle_menu_handler_cb (void *data) {
    GtkWidget *menu = lookup_widget (mainwin, "menubar");
    int val = 1-deadbeef->conf_get_int ("gtkui.show_menu", 1);
    val ? gtk_widget_show (menu) : gtk_widget_hide (menu);
    deadbeef->conf_set_int ("gtkui.show_menu", val);
    return FALSE;
}

int
action_toggle_menu_handler (DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_toggle_menu_handler_cb, NULL);
    return 0;
}

gboolean
action_toggle_statusbar_handler_cb (void *data) {
    GtkWidget *sb = lookup_widget (mainwin, "statusbar");
    if (sb) {
        int val = 1 - deadbeef->conf_get_int ("gtkui.statusbar.visible", 1);
        deadbeef->conf_set_int ("gtkui.statusbar.visible", val);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "view_status_bar")), val);
        val ? gtk_widget_show (sb) : gtk_widget_hide (sb);
        deadbeef->conf_save ();
    }
    return FALSE;
}

int
action_toggle_statusbar_handler (DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_toggle_statusbar_handler_cb, NULL);
    return 0;
}

gboolean
action_toggle_designmode_handler_cb (void *data) {
    GtkWidget *menuitem = lookup_widget (mainwin, "design_mode1");
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    act = !act;
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), act);
    return FALSE;
}

int
action_toggle_designmode_handler (DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_toggle_designmode_handler_cb, NULL);
    return 0;
}

gboolean
action_preferences_handler_cb (void *data) {
    gtkui_run_preferences_dlg ();
    return FALSE;
}

int
action_preferences_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_preferences_handler_cb, NULL);
    return 0;
}

gboolean
action_sort_custom_handler_cb (void *data) {
    GtkWidget *dlg = create_sortbydlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (dlg, "sortorder"));
    GtkEntry *entry = GTK_ENTRY (lookup_widget (dlg, "sortfmt"));

    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("gtkui.sortby_order", 0));
    deadbeef->conf_lock ();
    gtk_entry_set_text (entry, deadbeef->conf_get_str_fast ("gtkui.sortby_fmt_v2", ""));
    deadbeef->conf_unlock ();

    int r = gtk_dialog_run (GTK_DIALOG (dlg));

    if (r == GTK_RESPONSE_OK) {
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (dlg, "sortorder"));
        GtkEntry *entry = GTK_ENTRY (lookup_widget (dlg, "sortfmt"));
        int order = gtk_combo_box_get_active (combo);
        const char *fmt = gtk_entry_get_text (entry);

        deadbeef->conf_set_int ("gtkui.sortby_order", order);
        deadbeef->conf_set_str ("gtkui.sortby_fmt_v2", fmt);

        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, fmt, order == 0 ? DDB_SORT_ASCENDING : DDB_SORT_DESCENDING);
        deadbeef->plt_save_config (plt);
        deadbeef->plt_unref (plt);

        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    }

    gtk_widget_destroy (dlg);
    dlg = NULL;
    return FALSE;
}

int
action_sort_custom_handler (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (action_sort_custom_handler_cb, NULL);
    return 0;
}

int
action_crop_selected_handler (DB_plugin_action_t *act, int ctx) {
    deadbeef->pl_crop_selected ();
    deadbeef->pl_save_current ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    return 0;
}

gboolean
action_toggle_eq_handler_cb (void *data) {
    GtkWidget *menuitem = lookup_widget (mainwin, "view_eq");
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    act = !act;
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), act);
    return FALSE;
}

int
action_toggle_eq_handler (DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_toggle_eq_handler_cb, NULL);
    return 0;
}

gboolean
action_show_eq_handler_cb (void *data) {
    GtkWidget *menuitem = lookup_widget (mainwin, "view_eq");
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    if (!act) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), 1);
    }
    return FALSE;
}

int
action_show_eq_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_show_eq_handler_cb, NULL);
    return 0;
}

gboolean
action_hide_eq_handler_cb (void *data) {
    GtkWidget *menuitem = lookup_widget (mainwin, "view_eq");
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    if (act) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), 0);
    }
    return FALSE;
}

int
action_hide_eq_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_hide_eq_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_loop_off_handler_cb (void *data) {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "loop_disable")), 1);
    return FALSE;
}

int
action_playback_loop_off_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_loop_off_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_loop_single_handler_cb (void *data) {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "loop_single")), 1);
    return FALSE;
}

int
action_playback_loop_single_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_loop_single_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_loop_all_handler_cb (void *data) {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "loop_all")), 1);
    return FALSE;
}

int
action_playback_loop_all_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_loop_all_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_order_random_handler_cb (void *data) {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "order_random")), 1);
    return FALSE;
}

int
action_playback_order_random_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_order_random_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_order_shuffle_albums_handler_cb (void *data) {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "order_shuffle_albums")), 1);
    return FALSE;
}

int
action_playback_order_shuffle_albums_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_order_shuffle_albums_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_order_shuffle_handler_cb (void *data) {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "order_shuffle")), 1);
    return FALSE;
}

int
action_playback_order_shuffle_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_order_shuffle_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_order_linear_handler_cb (void *data) {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "order_linear")), 1);
    return FALSE;
}

int
action_playback_order_linear_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_order_linear_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_order_cycle_handler_cb (void *data) {
    int shuffle = deadbeef->streamer_get_shuffle ();
    switch (shuffle) {
    case DDB_SHUFFLE_OFF:
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "order_shuffle")), 1);
        break;
    case DDB_SHUFFLE_TRACKS:
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "order_shuffle_albums")), 1);
        break;
    case DDB_SHUFFLE_ALBUMS:
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "order_random")), 1);
        break;
    case DDB_SHUFFLE_RANDOM:
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "order_linear")), 1);
        break;
    }
    return FALSE;
}

int
action_playback_order_cycle_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_order_cycle_handler_cb, NULL);
    return 0;
}

gboolean
action_playback_loop_cycle_handler_cb (void *data) {
    int repeat = deadbeef->streamer_get_repeat ();
    switch (repeat) {
    case DDB_REPEAT_ALL:
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "loop_single")), 1);
        break;
    case DDB_REPEAT_SINGLE:
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "loop_disable")), 1);
        break;
    case DDB_REPEAT_OFF:
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "loop_all")), 1);
        break;
    }
    return FALSE;
}

int
action_playback_loop_cycle_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_playback_loop_cycle_handler_cb, NULL);
    return 0;
}

gboolean
action_toggle_logwindow_handler_cb (void *data) {
    gtkui_toggle_log_window();
    return FALSE;
}

int
action_toggle_logwindow_handler(DB_plugin_action_t *act, int ctx) {
    g_idle_add (action_toggle_logwindow_handler_cb, NULL);
    return 0;
}
