/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <gtk/gtk.h>
#include <stdlib.h>
#include "../../shared/deletefromdisk.h"
#include <deadbeef/strdupa.h>
#include "actions.h"
#include "actionhandlers.h"
#include "clipboard.h"
#include "gtkui_deletefromdisk.h"
#include "interface.h"
#include "playlist/plcommon.h"
#include "plmenu.h"
#include "support.h"

// disable custom title function, until we have new title formatting (0.7)
#define DISABLE_CUSTOM_TITLE

extern DB_functions_t *deadbeef;

static ddbUtilTrackList_t _menuPlaylist;
static ddbUtilTrackList_t _menuTrackList;
static ddb_action_context_t _menuActionContext;
static int _menuPlaylistIsDynamic; // Hide items like Remove, Cut, Paste
static trkproperties_delegate_t _trkproperties_delegate;

static void
_run_menu (int show_paste);

static void
_set_playlist (ddb_playlist_t *playlist) {
    if (_menuPlaylist != NULL) {
        deadbeef->plt_unref (_menuPlaylist);
    }
    _menuPlaylist = playlist;
    if (_menuPlaylist != NULL) {
        deadbeef->plt_ref (_menuPlaylist);
    }

    deadbeef->action_set_playlist(playlist);
}

void
plmenu_free (void) {
    if (_menuTrackList != NULL) {
        ddbUtilTrackListFree(_menuTrackList);
        _menuTrackList = NULL;
    }
    _set_playlist(NULL);
}

static void
_capture_selected_track_list (void) {
    if (_menuTrackList != NULL) {
        ddbUtilTrackListFree(_menuTrackList);
        _menuTrackList = NULL;
    }

    ddb_playItem_t **tracks = NULL;

    ddb_playItem_t *current = deadbeef->streamer_get_playing_track_safe ();

    deadbeef->pl_lock ();

    int count = 0;

    if (_menuActionContext == DDB_ACTION_CTX_SELECTION) {
        count = deadbeef->plt_getselcount(_menuPlaylist);
    }
    else if (_menuActionContext == DDB_ACTION_CTX_PLAYLIST) {
        count = deadbeef->plt_get_item_count (_menuPlaylist, PL_MAIN);
    }

    if (count == 0) {
        deadbeef->pl_unlock ();
        if (current != NULL) {
            deadbeef->pl_item_unref (current);
        }
        return;
    }

    int current_idx = -1;
    int all_idx = 0;
    int idx = 0;
    if (count) {
        tracks = calloc (count, sizeof (ddb_playItem_t *));

        ddb_playItem_t *it = deadbeef->plt_get_first (_menuPlaylist, PL_MAIN);
        while (it) {
            ddb_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            if (current != NULL && it == current) {
                current_idx = all_idx;
            }
            if (_menuActionContext != DDB_ACTION_CTX_SELECTION || deadbeef->pl_is_selected (it)) {
                tracks[idx++] = it;
            }
            else {
                deadbeef->pl_item_unref (it);
            }
            it = next;
            all_idx++;
        }
    }


    deadbeef->pl_unlock ();

    _menuTrackList = ddbUtilTrackListInitWithWithTracks(ddbUtilTrackListAlloc(), _menuPlaylist, _menuActionContext, tracks, count, current, current_idx);

    if (current) {
        deadbeef->pl_item_unref (current);
        current = NULL;
    }

    for (int i = 0; i < idx; i++) {
        deadbeef->pl_item_unref (tracks[i]);
    }

    free (tracks);
}

void
trk_context_menu_update_with_playlist (ddb_playlist_t *playlist, ddb_action_context_t action_context) {
    _set_playlist(playlist);
    _menuPlaylistIsDynamic = 0;
    _menuActionContext = action_context;
    _capture_selected_track_list();
}

static void
play_later_activate (GtkMenuItem     *menuitem, gpointer         user_data) {
    int count = ddbUtilTrackListGetTrackCount(_menuTrackList);
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(_menuTrackList);

    for (int i = 0; i < count; i++) {
        deadbeef->playqueue_push (tracks[i]);
    }
}

static void
play_next_activate (GtkMenuItem     *menuitem, gpointer         user_data) {
    int count = ddbUtilTrackListGetTrackCount(_menuTrackList);
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(_menuTrackList);

    for (int i = 0; i < count; i++) {
        deadbeef->playqueue_insert_at (i, tracks[i]);
    }
}

static void
remove_from_playback_queue_activate (GtkMenuItem     *menuitem,     gpointer         user_data) {
    int count = ddbUtilTrackListGetTrackCount(_menuTrackList);
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(_menuTrackList);

    for (int i = 0; i < count; i++) {
        deadbeef->playqueue_remove (tracks[i]);
    }
}

void
on_cut_activate (GtkMenuItem     *menuitem,
                 gpointer         user_data)
{
    clipboard_cut_selection (_menuPlaylist, _menuActionContext);
}

void
on_copy_activate (GtkMenuItem     *menuitem,
                  gpointer         user_data)
{
    clipboard_copy_selection (_menuPlaylist, _menuActionContext);
}

void
on_paste_activate (GtkMenuItem     *menuitem,
                   gpointer         user_data)
{
    clipboard_paste_selection (_menuPlaylist, _menuActionContext);
}

static void
reload_metadata_activate (GtkMenuItem     *menuitem, gpointer         user_data) {
    int count = ddbUtilTrackListGetTrackCount(_menuTrackList);
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(_menuTrackList);

    for (int i = 0; i < count; i++) {
        ddb_playItem_t *it = tracks[i];
        deadbeef->pl_lock ();
        char decoder_id[100];
        const char *dec = deadbeef->pl_find_meta (it, ":DECODER");
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = deadbeef->pl_is_selected (it) && deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;
        deadbeef->pl_unlock ();

        if (match) {
            uint32_t f = deadbeef->pl_get_item_flags (it);
            if (!(f & DDB_IS_SUBTRACK)) {
                f &= ~DDB_TAG_MASK;
                deadbeef->pl_set_item_flags (it, f);
                DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
                for (int i = 0; decoders[i]; i++) {
                    if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                        if (decoders[i]->read_metadata) {
                            decoders[i]->read_metadata (it);
                        }
                        break;
                    }
                }
            }
        }
    }

    if (_trkproperties_delegate.trkproperties_did_reload_metadata != NULL) {
        _trkproperties_delegate.trkproperties_did_reload_metadata (_trkproperties_delegate.user_data);
    }
}

static void
_trkproperties_did_update_tracks (void *user_data) {
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

static void
_trkproperties_did_reload_metadata (void *user_data) {
    deadbeef->pl_save_current ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

static void
_trkproperties_did_delete_files (void *user_data, int cancelled) {
    if (!cancelled) {
        deadbeef->pl_save_all ();
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    }
}

static void
properties_activate                (GtkMenuItem     *menuitem,
                                    gpointer         user_data)
{
    int count = ddbUtilTrackListGetTrackCount(_menuTrackList);
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(_menuTrackList);
    show_track_properties_dlg_with_track_list (tracks, count);
    trkproperties_set_delegate(&_trkproperties_delegate);
}

void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->pl_clear ();
    deadbeef->pl_save_current ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_remove_from_playlist_handler (NULL, _menuActionContext);
}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_crop_selected_handler (NULL, 0);
}

static void
on_remove2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int count = ddbUtilTrackListGetTrackCount(_menuTrackList);
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(_menuTrackList);
    ddb_playlist_t *plt = ddbUtilTrackListGetPlaylist(_menuTrackList);

    for (int i = 0; i < count; i++) {
        deadbeef->plt_remove_item (plt, tracks[i]);
    }
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

#ifndef DISABLE_CUSTOM_TITLE
static void
on_toggle_set_custom_title (GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("gtkui.location_set_custom_title", active);

    GtkWidget *ct = lookup_widget (GTK_WIDGET (user_data), "custom_title");
    gtk_widget_set_sensitive (ct, active);

    deadbeef->conf_save ();
}

static void
on_set_custom_title_activate (GtkMenuItem *menuitem, gpointer user_data)
{
    DdbListview *lv = user_data;
    int idx = lv->binding->cursor ();
    if (idx < 0) {
        return;
    }
    DdbListviewIter it = lv->binding->get_for_idx (idx);
    if (!it) {
        return;
    }

    GtkWidget *dlg = create_setcustomtitledlg ();
    GtkWidget *sct = lookup_widget (dlg, "set_custom_title");
    GtkWidget *ct = lookup_widget (dlg, "custom_title");
    if (deadbeef->conf_get_int ("gtkui.location_set_custom_title", 0)) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sct), TRUE);
        gtk_widget_set_sensitive (ct, TRUE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sct), FALSE);
        gtk_widget_set_sensitive (ct, FALSE);
    }
    deadbeef->pl_lock ();
    const char *custom_title = deadbeef->pl_find_meta ((DB_playItem_t *)it, ":CUSTOM_TITLE");
    if (custom_title) {
        custom_title = strdupa (custom_title);
    }
    else {
        custom_title = "";
    }
    deadbeef->pl_unlock ();

    g_signal_connect ((gpointer) sct, "toggled",
                      G_CALLBACK (on_toggle_set_custom_title),
                      dlg);
    gtk_entry_set_text (GTK_ENTRY (ct), custom_title);

    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        if (it && deadbeef->conf_get_int ("gtkui.location_set_custom_title", 0)) {
            deadbeef->pl_replace_meta ((DB_playItem_t *)it, ":CUSTOM_TITLE", gtk_entry_get_text (GTK_ENTRY (ct)));
        }
        else {
            deadbeef->pl_delete_meta ((DB_playItem_t *)it, ":CUSTOM_TITLE");
        }
    }
    gtk_widget_destroy (dlg);
    lv->binding->unref (it);
}
#endif

static ddbDeleteFromDiskController_t _deleteCtl;

static void
_deleteCompleted (ddbDeleteFromDiskController_t ctl, int cancelled) {
    ddbDeleteFromDiskControllerFree(ctl);
    _deleteCtl = NULL;

    if (_trkproperties_delegate.trkproperties_did_delete_files != NULL) {
        _trkproperties_delegate.trkproperties_did_delete_files(_trkproperties_delegate.user_data, cancelled);
    }
}

static void
delete_from_disk_with_track_list (ddbUtilTrackList_t trackList) {
    if (_deleteCtl) {
        return;
    }

    ddbDeleteFromDiskControllerDelegate_t delegate = {
        .warningMessageForCtx = gtkui_warning_message_for_ctx,
        .deleteFile = gtkui_delete_file,
        .completed = _deleteCompleted,
    };

    _deleteCtl =  ddbDeleteFromDiskControllerInitWithTrackList(ddbDeleteFromDiskControllerAlloc(), trackList);

    ddbDeleteFromDiskControllerSetShouldSkipDeletedTracks(_deleteCtl, deadbeef->conf_get_int ("gtkui.skip_deleted_songs", 0));

    ddbDeleteFromDiskControllerRunWithDelegate(_deleteCtl, delegate);
}

static void
on_remove_from_disk_activate                    (GtkMenuItem     *menuitem,
                                                 gpointer         user_data)
{
    delete_from_disk_with_track_list(_menuTrackList);
}

static void
actionitem_activate (GtkMenuItem     *menuitem,
                     DB_plugin_action_t *action)
{
    if (action->callback2) {
        action->callback2 (action, _menuActionContext);
    }
}

void
list_context_menu_with_dynamic_track_list (ddb_playlist_t *playlist, trkproperties_delegate_t *delegate) {
    _set_playlist(playlist);
    _menuPlaylistIsDynamic = 1;
    _menuActionContext = DDB_ACTION_CTX_PLAYLIST;

    _capture_selected_track_list();

    _trkproperties_delegate.trkproperties_did_update_tracks = delegate->trkproperties_did_update_tracks;
    _trkproperties_delegate.trkproperties_did_reload_metadata = delegate->trkproperties_did_reload_metadata;
    _trkproperties_delegate.trkproperties_did_delete_files = delegate->trkproperties_did_delete_files;
    _trkproperties_delegate.user_data = delegate->user_data;

    _run_menu (0);
}

void
list_context_menu (ddb_playlist_t *playlist, int iter) {
    _menuActionContext = DDB_ACTION_CTX_SELECTION;
    _set_playlist (playlist);
    _menuPlaylistIsDynamic = 0;
    _capture_selected_track_list();

    _trkproperties_delegate.trkproperties_did_update_tracks = _trkproperties_did_update_tracks;
    _trkproperties_delegate.trkproperties_did_reload_metadata = _trkproperties_did_reload_metadata;
    _trkproperties_delegate.trkproperties_did_delete_files = _trkproperties_did_delete_files;
    _trkproperties_delegate.user_data = NULL;
    _run_menu (iter != PL_SEARCH);
}

void
trk_context_menu_build (GtkWidget *menu) {
    int selected_count = 0;
    ddb_playItem_t *selected_track = NULL;

    if (_menuTrackList != NULL) {
        selected_count = ddbUtilTrackListGetTrackCount (_menuTrackList);
        if (selected_count != 0) {
            ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(_menuTrackList);
            selected_track = tracks[0];
        }

    }

    // remove all items
    GList *children = gtk_container_get_children(GTK_CONTAINER(menu));
    for (GList *item = children; item; item = item->next) {
        gtk_container_remove(GTK_CONTAINER(menu), GTK_WIDGET(item));
    }
    g_list_free(children);

    // add all items
    GtkWidget *separator;
    GtkWidget *remove_from_disk = NULL;
    GtkWidget *separator9;
    GtkWidget *properties1;
    GtkWidget *reload_metadata;

    GtkAccelGroup *accel_group = NULL;
    accel_group = gtk_accel_group_new ();

#ifndef DISABLE_CUSTOM_TITLE
    GtkWidget *set_custom_title;
#endif

    if (!_menuPlaylistIsDynamic) {
        GtkWidget *play_later;
        GtkWidget *play_next;
        GtkWidget *remove_from_playback_queue1;
        play_next = gtk_menu_item_new_with_mnemonic (_("Play Next"));
        gtk_widget_show (play_next);
        gtk_container_add (GTK_CONTAINER (menu), play_next);
        gtk_widget_set_sensitive(play_next, selected_count != 0);

        play_later = gtk_menu_item_new_with_mnemonic (_("Play Later"));
        gtk_widget_show (play_later);
        gtk_container_add (GTK_CONTAINER (menu), play_later);
        gtk_widget_set_sensitive(play_later, selected_count != 0);

        remove_from_playback_queue1 = gtk_menu_item_new_with_mnemonic (_("Remove from Playback Queue"));
        if (selected_count > 0 && _menuPlaylist != NULL) {
            int pqlen = deadbeef->playqueue_get_count ();
            int no_playqueue_items = 1;
            // NOTE: this can be an extremely slow operation
            for (int i = 0; i < pqlen && no_playqueue_items; i++) {
                DB_playItem_t *pqitem = deadbeef->playqueue_get_item (i);
                if (deadbeef->pl_get_playlist (pqitem) == _menuPlaylist
                    && (_menuActionContext != DDB_ACTION_CTX_SELECTION || deadbeef->pl_is_selected (pqitem))) {
                    no_playqueue_items = 0;
                }
                deadbeef->pl_item_unref (pqitem);
            }
            if (no_playqueue_items) {
                gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
            }
        }
        gtk_widget_show (remove_from_playback_queue1);
        gtk_container_add (GTK_CONTAINER (menu), remove_from_playback_queue1);

        g_signal_connect ((gpointer) play_later, "activate",
                          G_CALLBACK (play_later_activate),
                          NULL);
        g_signal_connect ((gpointer) play_next, "activate",
                          G_CALLBACK (play_next_activate),
                          NULL);
        g_signal_connect ((gpointer) remove_from_playback_queue1, "activate",
                          G_CALLBACK (remove_from_playback_queue_activate),
                          NULL);
    }


    reload_metadata = gtk_menu_item_new_with_mnemonic (_("Reload Metadata"));
    gtk_widget_show (reload_metadata);
    gtk_container_add (GTK_CONTAINER (menu), reload_metadata);
    gtk_widget_set_sensitive(reload_metadata, selected_count != 0);

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);

    if (!_menuPlaylistIsDynamic) {
        GtkWidget *cut;
        GtkWidget *cut_image;

        cut = gtk_image_menu_item_new_with_mnemonic (_("Cu_t"));
        gtk_widget_show (cut);
        gtk_container_add (GTK_CONTAINER (menu), cut);
        gtk_widget_set_sensitive(cut, selected_count != 0);
        gtk_widget_add_accelerator (cut, "activate", accel_group, GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        cut_image = gtk_image_new_from_stock ("gtk-cut", GTK_ICON_SIZE_MENU);
        gtk_widget_show (cut_image);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (cut), cut_image);
        g_signal_connect ((gpointer) cut, "activate",
                          G_CALLBACK (on_cut_activate),
                          NULL);
    }

    GtkWidget *copy;
    GtkWidget *copy_image;
    copy = gtk_image_menu_item_new_with_mnemonic (_("_Copy"));
    gtk_widget_show (copy);
    gtk_container_add (GTK_CONTAINER (menu), copy);
    gtk_widget_set_sensitive(copy, selected_count != 0);
    gtk_widget_add_accelerator (copy, "activate", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    copy_image = gtk_image_new_from_stock ("gtk-copy", GTK_ICON_SIZE_MENU);
    gtk_widget_show (copy_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (copy), copy_image);

    g_signal_connect ((gpointer) copy, "activate",
                      G_CALLBACK (on_copy_activate),
                      NULL);

    if (!_menuPlaylistIsDynamic) {
        GtkWidget *paste;
        GtkWidget *paste_image;
        paste = gtk_image_menu_item_new_with_mnemonic (_("_Paste"));
        int show_paste = 1; // FIXME
        if (show_paste) {
            gtk_widget_show (paste);
        }
        else {
            gtk_widget_hide (paste);
        }
        gtk_container_add (GTK_CONTAINER (menu), paste);
        gtk_widget_set_sensitive(paste, clipboard_is_clipboard_data_available());
        gtk_widget_add_accelerator (paste, "activate", accel_group, GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        paste_image = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_MENU);
        gtk_widget_show (paste_image);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (paste), paste_image);

        g_signal_connect ((gpointer) paste, "activate",
                          G_CALLBACK (on_paste_activate),
                          NULL);
    }

    separator9 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator9);
    gtk_container_add (GTK_CONTAINER (menu), separator9);
    gtk_widget_set_sensitive (separator9, FALSE);

    if (!_menuPlaylistIsDynamic) {
        GtkWidget *remove2;
        remove2 = gtk_menu_item_new_with_mnemonic (_("Remove"));
        gtk_widget_show (remove2);
        gtk_container_add (GTK_CONTAINER (menu), remove2);
        gtk_widget_set_sensitive(remove2, selected_count != 0 && _menuPlaylist != NULL);

        g_signal_connect ((gpointer) remove2, "activate",
                          G_CALLBACK (on_remove2_activate),
                          NULL);
    }

    int hide_remove_from_disk = deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0);

    if (!hide_remove_from_disk) {
        remove_from_disk = gtk_menu_item_new_with_mnemonic (_("Delete from Disk"));
        gtk_widget_show (remove_from_disk);
        gtk_container_add (GTK_CONTAINER (menu), remove_from_disk);
        gtk_widget_set_sensitive(remove_from_disk, selected_count != 0);
    }

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);

    int added_entries = menu_add_action_items(menu, selected_count, selected_track, _menuActionContext, actionitem_activate);
    if (added_entries > 0 && !deadbeef->conf_get_int ("gtkui.action_separators", 0))
    {
        GtkWidget *separator = gtk_separator_menu_item_new ();
        gtk_widget_show (separator);
        gtk_container_add (GTK_CONTAINER (menu), separator);
        gtk_widget_set_sensitive (separator, FALSE);
    }

#ifndef DISABLE_CUSTOM_TITLE
    set_custom_title = gtk_menu_item_new_with_mnemonic (_("Set Custom Title"));
    gtk_widget_show (set_custom_title);
    gtk_container_add (GTK_CONTAINER (playlist_menu), set_custom_title);
    if (selected_count != 1) {
        gtk_widget_set_sensitive (GTK_WIDGET (set_custom_title), FALSE);
    }

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);
#endif

    properties1 = gtk_menu_item_new_with_mnemonic (_("Track Properties"));
    gtk_widget_show (properties1);
    gtk_container_add (GTK_CONTAINER (menu), properties1);
    gtk_widget_set_sensitive (properties1, selected_count != 0);

    g_signal_connect ((gpointer) reload_metadata, "activate",
                      G_CALLBACK (reload_metadata_activate),
                      NULL);
    if (!hide_remove_from_disk && remove_from_disk != NULL) {
        g_signal_connect ((gpointer) remove_from_disk, "activate",
                          G_CALLBACK (on_remove_from_disk_activate),
                          NULL);
    }
#ifndef DISABLE_CUSTOM_TITLE
    g_signal_connect ((gpointer) set_custom_title, "activate",
                      G_CALLBACK (on_set_custom_title_activate),
                      NULL);
#endif
    g_signal_connect ((gpointer) properties1, "activate",
                      G_CALLBACK (properties_activate),
                      NULL);
}


static void
_run_menu (int show_paste) {
    GtkWidget *menu = gtk_menu_new();

    trk_context_menu_build (menu);
    gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}
