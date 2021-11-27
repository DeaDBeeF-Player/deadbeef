/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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
#include "../../strdupa.h"
#include "actionhandlers.h"
#include "clipboard.h"
#include "gtkui_deletefromdisk.h"
#include "interface.h"
#include "plcommon.h"
#include "plmenu.h"
#include "support.h"

// disable custom title function, until we have new title formatting (0.7)
#define DISABLE_CUSTOM_TITLE

extern DB_functions_t *deadbeef;

static ddbUtilTrackList_t _menuTrackList;
static trkproperties_delegate_t _trkproperties_delegate;

static void
_run_menu (int show_paste);

void
plmenu_free (void) {
    if (_menuTrackList != NULL) {
        ddbUtilTrackListFree(_menuTrackList);
        _menuTrackList = NULL;
    }
}

static void
_capture_selected_track_list (void) {
    if (_menuTrackList != NULL) {
        ddbUtilTrackListFree(_menuTrackList);
        _menuTrackList = NULL;
    }

    ddb_playItem_t **tracks = NULL;

    ddb_playlist_t *plt = deadbeef->plt_get_curr();

    deadbeef->pl_lock ();

    ddb_playItem_t *current = deadbeef->streamer_get_playing_track ();
    int current_idx = -1;

    int count = deadbeef->plt_getselcount(plt);
    int all_idx = 0;
    int idx = 0;
    if (count) {
        tracks = calloc (sizeof (ddb_playItem_t *), count);

        ddb_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            ddb_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            if (current != NULL && it == current) {
                current_idx = all_idx;
            }
            if (deadbeef->pl_is_selected (it)) {
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

    _menuTrackList = ddbUtilTrackListInitWithWithTracks(ddbUtilTrackListAlloc(), plt, DDB_ACTION_CTX_SELECTION, tracks, count, current, current_idx);

    if (current) {
        deadbeef->pl_item_unref (current);
        current = NULL;
    }

    for (int i = 0; i < idx; i++) {
        deadbeef->pl_item_unref (tracks[i]);
    }

    free (tracks);

    deadbeef->plt_unref (plt);
}

static GtkWidget*
find_popup (GtkWidget *widget)
{
    GtkWidget *parent = widget;
    do {
        widget = parent;
        if (GTK_IS_MENU (widget))
            parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
        else
            parent = gtk_widget_get_parent (widget);
        if (!parent)
            parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
    } while (parent);

    return widget;
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
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        clipboard_cut_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
}

void
on_copy_activate (GtkMenuItem     *menuitem,
                  gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        clipboard_copy_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
}

void
on_paste_activate (GtkMenuItem     *menuitem,
                   gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        clipboard_paste_selection (plt, DDB_ACTION_CTX_SELECTION);
        deadbeef->plt_unref (plt);
    }
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
    action_remove_from_playlist_handler (NULL, DDB_ACTION_CTX_SELECTION);
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
        action->callback2 (action, DDB_ACTION_CTX_SELECTION);
    }
}

#define HOOKUP_OBJECT(component,widget,name) \
g_object_set_data_full (G_OBJECT (component), name, \
g_object_ref (widget), (GDestroyNotify) g_object_unref)

void
list_empty_region_context_menu (void) {
    GtkWidget *playlist_menu;
    GtkWidget *paste;
    GtkWidget *paste_image;
    GtkAccelGroup *accel_group = NULL;
    accel_group = gtk_accel_group_new ();

    playlist_menu = gtk_menu_new ();

    paste = gtk_image_menu_item_new_with_mnemonic (_("_Paste"));
    gtk_widget_show (paste);
    gtk_container_add (GTK_CONTAINER (playlist_menu), paste);
    gtk_widget_add_accelerator (paste, "activate", accel_group, GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    if (clipboard_is_clipboard_data_available ()) {
        gtk_widget_set_sensitive (paste, TRUE);
    }
    else {
        gtk_widget_set_sensitive (paste, FALSE);
    }

    paste_image = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_MENU);
    gtk_widget_show (paste_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (paste), paste_image);

    g_signal_connect ((gpointer) paste, "activate",
                      G_CALLBACK (on_paste_activate),
                      NULL);

    gtk_menu_popup_at_pointer (GTK_MENU (playlist_menu), NULL);
}

void
list_context_menu_with_track_list (ddb_playItem_t **tracks, int count, trkproperties_delegate_t *delegate) {
    if (_menuTrackList != NULL) {
        ddbUtilTrackListFree(_menuTrackList);
        _menuTrackList = NULL;
    }

    _menuTrackList = ddbUtilTrackListInitWithWithTracks(ddbUtilTrackListAlloc(), NULL, DDB_ACTION_CTX_SELECTION, tracks, count, NULL, -1);

    _trkproperties_delegate.trkproperties_did_update_tracks = delegate->trkproperties_did_update_tracks;
    _trkproperties_delegate.trkproperties_did_reload_metadata = delegate->trkproperties_did_reload_metadata;
    _trkproperties_delegate.trkproperties_did_delete_files = delegate->trkproperties_did_delete_files;
    _trkproperties_delegate.user_data = delegate->user_data;

    _run_menu (0);
}

void
list_context_menu (int iter) {
    _capture_selected_track_list();

    _trkproperties_delegate.trkproperties_did_update_tracks = _trkproperties_did_update_tracks;
    _trkproperties_delegate.trkproperties_did_reload_metadata = _trkproperties_did_reload_metadata;
    _trkproperties_delegate.trkproperties_did_delete_files = _trkproperties_did_delete_files;
    _trkproperties_delegate.user_data = NULL;
    _run_menu (iter != PL_SEARCH);
}

static void
_run_menu (int show_paste) {
    GtkWidget *playlist_menu;
    GtkWidget *play_later;
    GtkWidget *play_next;
    GtkWidget *remove_from_playback_queue1;
    GtkWidget *separator;
    GtkWidget *remove2;
    GtkWidget *remove_from_disk = NULL;
    GtkWidget *separator8;
    GtkWidget *cut;
    GtkWidget *cut_image;
    GtkWidget *copy;
    GtkWidget *copy_image;
    GtkWidget *paste;
    GtkWidget *paste_image;
    GtkWidget *separator9;
    GtkWidget *properties1;
    GtkWidget *reload_metadata;

    GtkAccelGroup *accel_group = NULL;
    accel_group = gtk_accel_group_new ();

#ifndef DISABLE_CUSTOM_TITLE
    GtkWidget *set_custom_title;
#endif

    int selected_count = ddbUtilTrackListGetTrackCount (_menuTrackList);
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(_menuTrackList);

    playlist_menu = gtk_menu_new ();

    play_next = gtk_menu_item_new_with_mnemonic (_("Play Next"));
    gtk_widget_show (play_next);
    gtk_container_add (GTK_CONTAINER (playlist_menu), play_next);

    play_later = gtk_menu_item_new_with_mnemonic (_("Play Later"));
    gtk_widget_show (play_later);
    gtk_container_add (GTK_CONTAINER (playlist_menu), play_later);

    remove_from_playback_queue1 = gtk_menu_item_new_with_mnemonic (_("Remove from Playback Queue"));
    if (selected_count > 0) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int pqlen = deadbeef->playqueue_get_count ();
        int no_playqueue_items = 1;
        for (int i = 0; i < pqlen && no_playqueue_items; i++) {
            DB_playItem_t *pqitem = deadbeef->playqueue_get_item (i);
            if (deadbeef->pl_get_playlist (pqitem) == plt && deadbeef->pl_is_selected (pqitem)) {
                no_playqueue_items = 0;
            }
            deadbeef->pl_item_unref (pqitem);
        }
        if (no_playqueue_items) {
            gtk_widget_set_sensitive (remove_from_playback_queue1, FALSE);
        }
    }
    gtk_widget_show (remove_from_playback_queue1);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_playback_queue1);

    reload_metadata = gtk_menu_item_new_with_mnemonic (_("Reload Metadata"));
    gtk_widget_show (reload_metadata);
    gtk_container_add (GTK_CONTAINER (playlist_menu), reload_metadata);

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);

    cut = gtk_image_menu_item_new_with_mnemonic (_("Cu_t"));
    gtk_widget_show (cut);
    gtk_container_add (GTK_CONTAINER (playlist_menu), cut);
    gtk_widget_add_accelerator (cut, "activate", accel_group, GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    cut_image = gtk_image_new_from_stock ("gtk-cut", GTK_ICON_SIZE_MENU);
    gtk_widget_show (cut_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (cut), cut_image);

    copy = gtk_image_menu_item_new_with_mnemonic (_("_Copy"));
    gtk_widget_show (copy);
    gtk_container_add (GTK_CONTAINER (playlist_menu), copy);
    gtk_widget_add_accelerator (copy, "activate", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    copy_image = gtk_image_new_from_stock ("gtk-copy", GTK_ICON_SIZE_MENU);
    gtk_widget_show (copy_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (copy), copy_image);

    paste = gtk_image_menu_item_new_with_mnemonic (_("_Paste"));
    if (show_paste) {
        gtk_widget_show (paste);
    }
    else {
        gtk_widget_hide (paste);
    }
    gtk_container_add (GTK_CONTAINER (playlist_menu), paste);
    gtk_widget_add_accelerator (paste, "activate", accel_group, GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    if (clipboard_is_clipboard_data_available ()) {
        gtk_widget_set_sensitive (paste, TRUE);
    }
    else {
        gtk_widget_set_sensitive (paste, FALSE);
    }

    paste_image = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_MENU);
    gtk_widget_show (paste_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (paste), paste_image);

    separator9 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator9);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator9);
    gtk_widget_set_sensitive (separator9, FALSE);

    remove2 = gtk_menu_item_new_with_mnemonic (_("Remove"));
    gtk_widget_show (remove2);
    gtk_container_add (GTK_CONTAINER (playlist_menu), remove2);

    int hide_remove_from_disk = deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0);

    if (!hide_remove_from_disk) {
        remove_from_disk = gtk_menu_item_new_with_mnemonic (_("Remove from Disk"));
        gtk_widget_show (remove_from_disk);
        gtk_container_add (GTK_CONTAINER (playlist_menu), remove_from_disk);
    }

    separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_container_add (GTK_CONTAINER (playlist_menu), separator);
    gtk_widget_set_sensitive (separator, FALSE);

    DB_playItem_t *selected = NULL;

    if (selected_count > 0) {
        selected = tracks[0];
    }

    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;
    int added_entries = 0;
    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (selected);
        DB_plugin_action_t *action;

        int count = 0;
        for (action = actions; action; action = action->next)
        {
            if ((action->flags & DB_ACTION_COMMON) || !((action->callback2 && (action->flags & DB_ACTION_ADD_MENU)) || action->callback) || !(action->flags & (DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK)))
                continue;

            // create submenus (separated with '/')
            const char *prev = action->title;
            while (*prev && *prev == '/') {
                prev++;
            }

            GtkWidget *popup = NULL;

            for (;;) {
                const char *slash = strchr (prev, '/');
                if (slash && *(slash-1) != '\\') {
                    char name[slash-prev+1];
                    // replace \/ with /
                    const char *p = prev;
                    char *t = name;
                    while (*p && p < slash) {
                        if (*p == '\\' && *(p+1) == '/') {
                            *t++ = '/';
                            p += 2;
                        }
                        else {
                            *t++ = *p++;
                        }
                    }
                    *t = 0;

                    // add popup
                    GtkWidget *prev_menu = popup ? popup : playlist_menu;

                    popup = GTK_WIDGET (g_object_get_data (G_OBJECT (find_popup (prev_menu)), name));
                    if (!popup) {
                        GtkWidget *item = gtk_image_menu_item_new_with_mnemonic (_(name));
                        gtk_widget_show (item);
                        gtk_container_add (GTK_CONTAINER (prev_menu), item);
                        popup = gtk_menu_new ();
                        HOOKUP_OBJECT (prev_menu, popup, name);
                        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), popup);
                    }
                }
                else {
                    break;
                }
                prev = slash+1;
            }


            count++;
            added_entries++;
            GtkWidget *actionitem;

            // replace \/ with /
            const char *p = popup ? prev : action->title;
            char title[strlen (p)+1];
            char *t = title;
            while (*p) {
                if (*p == '\\' && *(p+1) == '/') {
                    *t++ = '/';
                    p += 2;
                }
                else {
                    *t++ = *p++;
                }
            }
            *t = 0;

            actionitem = gtk_menu_item_new_with_mnemonic (_(title));
            gtk_widget_show (actionitem);
            gtk_container_add (popup ? GTK_CONTAINER (popup) : GTK_CONTAINER (playlist_menu), actionitem);

            g_signal_connect ((gpointer) actionitem, "activate",
                              G_CALLBACK (actionitem_activate),
                              action);
            if ((selected_count > 1 && !(action->flags & DB_ACTION_MULTIPLE_TRACKS)) ||
                (action->flags & DB_ACTION_DISABLED)) {
                gtk_widget_set_sensitive (GTK_WIDGET (actionitem), FALSE);
            }
        }
        if (count > 0 && deadbeef->conf_get_int ("gtkui.action_separators", 0))
        {
            separator8 = gtk_separator_menu_item_new ();
            gtk_widget_show (separator8);
            gtk_container_add (GTK_CONTAINER (playlist_menu), separator8);
            gtk_widget_set_sensitive (separator8, FALSE);
        }
    }
    if (added_entries > 0 && !deadbeef->conf_get_int ("gtkui.action_separators", 0))
    {
        separator8 = gtk_separator_menu_item_new ();
        gtk_widget_show (separator8);
        gtk_container_add (GTK_CONTAINER (playlist_menu), separator8);
        gtk_widget_set_sensitive (separator8, FALSE);
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
    gtk_container_add (GTK_CONTAINER (playlist_menu), properties1);

    g_signal_connect ((gpointer) play_later, "activate",
                      G_CALLBACK (play_later_activate),
                      NULL);
    g_signal_connect ((gpointer) play_next, "activate",
                      G_CALLBACK (play_next_activate),
                      NULL);
    g_signal_connect ((gpointer) remove_from_playback_queue1, "activate",
                      G_CALLBACK (remove_from_playback_queue_activate),
                      NULL);
    g_signal_connect ((gpointer) reload_metadata, "activate",
                      G_CALLBACK (reload_metadata_activate),
                      NULL);
    g_signal_connect ((gpointer) cut, "activate",
                      G_CALLBACK (on_cut_activate),
                      NULL);
    g_signal_connect ((gpointer) copy, "activate",
                      G_CALLBACK (on_copy_activate),
                      NULL);
    g_signal_connect ((gpointer) paste, "activate",
                      G_CALLBACK (on_paste_activate),
                      NULL);
    g_signal_connect ((gpointer) remove2, "activate",
                      G_CALLBACK (on_remove2_activate),
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

    gtk_menu_popup_at_pointer (GTK_MENU (playlist_menu), NULL);
}


