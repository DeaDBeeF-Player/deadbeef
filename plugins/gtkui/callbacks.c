/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
#    include <config.h>
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

#include "playlist/ddblistview.h"
#include "ddbtabstrip.h"
#include "ddbvolumebar.h"
#include "ddbseekbar.h"
#include "search.h"
#include "progress.h"
#include "gtkui.h"
#include "../libparser/parser.h"
#include "drawing.h"
#include "eq.h"
#include "undo.h"
#include "wingeom.h"
#include "widgets.h"
#include "../hotkeys/hotkeys.h"
#include "actionhandlers.h"
#include "actions.h"
#include "playlist/plcommon.h"

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt, ...)

#define SELECTED(it) (deadbeef->pl_is_selected (it))
#define SELECT(it, sel) (deadbeef->pl_set_selected (it, sel))
#define VSELECT(it, sel)                     \
    {                                        \
        deadbeef->pl_set_selected (it, sel); \
        gtk_pl_redraw_item_everywhere (it);  \
    }
#define PL_NEXT(it, iter) (deadbeef->pl_get_next (it, iter))

DdbListview *last_playlist;
extern DB_functions_t *deadbeef; // defined in gtkui.c

void
on_open_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gdk_threads_add_idle (action_open_files_handler_cb, NULL);
}

void
on_add_files_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gdk_threads_add_idle (action_add_files_handler_cb, NULL);
}

void
on_add_folders_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gdk_threads_add_idle (action_add_folders_handler_cb, NULL);
}

void
on_quit_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gdk_threads_add_idle (action_quit_handler_cb, NULL);
}

void
on_select_all1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gdk_threads_add_idle (action_select_all_handler_cb, NULL);
}

void
on_stopbtn_clicked (GtkButton *button, gpointer user_data) {
    deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
}

void
on_playbtn_clicked (GtkButton *button, gpointer user_data) {
    // NOTE: this function is a copy of action_play_cb
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == DDB_PLAYBACK_STATE_PAUSED) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cur != -1) {
            ddb_playItem_t *it = deadbeef->plt_get_item_for_idx (plt, cur, PL_MAIN);
            ddb_playItem_t *it_playing = deadbeef->streamer_get_playing_track_safe ();

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
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = -1;
        if (plt) {
            cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
            deadbeef->plt_unref (plt);
        }
        if (cur == -1) {
            cur = 0;
        }
        deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, cur, 0);
    }
}

void
on_pausebtn_clicked (GtkButton *button, gpointer user_data) {
    deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
}

void
on_prevbtn_clicked (GtkButton *button, gpointer user_data) {
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
}

void
on_nextbtn_clicked (GtkButton *button, gpointer user_data) {
    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
}

void
on_playrand_clicked (GtkButton *button, gpointer user_data) {
    deadbeef->sendmessage (DB_EV_PLAY_RANDOM, 0, 0, 0);
}

gboolean
on_mainwin_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    // local hotkeys
    GdkModifierType consumed_modifiers;
    guint accel_key;

    guint accel_mods = event->state & gtk_accelerator_get_default_mod_mask ();

    GdkDisplay *display = gtk_widget_get_display (widget);

    gdk_keymap_translate_keyboard_state (
        gdk_keymap_get_for_display (display),
        event->hardware_keycode,
        accel_mods & (~GDK_SHIFT_MASK),
        0,
        &accel_key,
        NULL,
        NULL,
        &consumed_modifiers);
    if (accel_key == GDK_ISO_Left_Tab)
        accel_key = GDK_Tab;
    trace (
        "pressed: keycode: %x, mods: %x, hw: %x, translated: %x\n",
        event->keyval,
        mods,
        event->hardware_keycode,
        accel_key);

    accel_mods &= ~(consumed_modifiers & ~GDK_SHIFT_MASK);

    DB_plugin_t *hkplug = deadbeef->plug_get_for_id ("hotkeys");
    if (hkplug) {
        ddb_action_context_t ctx;
        DB_plugin_action_t *act =
            ((DB_hotkeys_plugin_t *)hkplug)->get_action_for_keycombo (accel_key, accel_mods, 0, &ctx);
        // Don't allow selection hotkeys to come from any other widget
        // than the playlist
        if (ctx == DDB_ACTION_CTX_SELECTION && !DDB_IS_LISTVIEW (widget)) {
            return FALSE;
        }
        if (act && act->callback2) {
            trace ("executing action %s in ctx %d\n", act->name, ctx);
            deadbeef->action_set_playlist (NULL);
            act->callback2 (act, ctx);
            return TRUE;
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        else if (act && act->callback) {
#pragma GCC diagnostic pop
            gtkui_exec_action_14 (act, -1);
        }
    }
    trace ("action not found\n");
    return FALSE;
}

void
on_order_linear_activate (GtkMenuItem *menuitem, gpointer user_data) {
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->streamer_set_shuffle (DDB_SHUFFLE_OFF);
    }
}

void
on_order_shuffle_activate (GtkMenuItem *menuitem, gpointer user_data) {
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->streamer_set_shuffle (DDB_SHUFFLE_TRACKS);
    }
}

void
on_order_shuffle_albums_activate (GtkMenuItem *menuitem, gpointer user_data) {
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->streamer_set_shuffle (DDB_SHUFFLE_ALBUMS);
    }
}

void
on_order_random_activate (GtkMenuItem *menuitem, gpointer user_data) {
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->streamer_set_shuffle (DDB_SHUFFLE_RANDOM);
    }
}

void
on_loop_all_activate (GtkMenuItem *menuitem, gpointer user_data) {
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->streamer_set_repeat (DDB_REPEAT_ALL);
    }
}

void
on_loop_single_activate (GtkMenuItem *menuitem, gpointer user_data) {
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->streamer_set_repeat (DDB_REPEAT_SINGLE);
    }
}

void
on_loop_disable_activate (GtkMenuItem *menuitem, gpointer user_data) {
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->streamer_set_repeat (DDB_REPEAT_OFF);
    }
}

gboolean
on_mainwin_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    int conf_close_send_to_tray = deadbeef->conf_get_int ("close_send_to_tray", 0);
    if (conf_close_send_to_tray) {
        gtk_widget_hide (widget);
    }
    else {
        gtkui_quit ();
    }
    return TRUE;
}

gboolean
on_mainwin_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer user_data) {
    wingeom_save (widget, "mainwin");
    return FALSE;
}

void
on_scroll_follows_playback_activate (GtkMenuItem *menuitem, gpointer user_data) {
    deadbeef->conf_set_int (
        "playlist.scroll.followplayback",
        gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_find_activate (GtkMenuItem *menuitem, gpointer user_data) {
    search_start ();
}

void
on_help1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gdk_threads_add_idle (action_show_help_handler_cb, NULL);
}

static GtkWidget *aboutwindow;

void
on_about1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    char s[200];
    snprintf (s, sizeof (s), _ ("About DeaDBeeF %s"), VERSION);
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_system_dir (DDB_SYS_DIR_DOC), "about.txt");
    gtkui_show_info_window (fname, s, &aboutwindow);
}

static GtkWidget *changelogwindow;

void
on_changelog1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    char s[200];
    snprintf (s, sizeof (s), _ ("DeaDBeeF %s ChangeLog"), VERSION);
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_system_dir (DDB_SYS_DIR_DOC), "ChangeLog");
    gtkui_show_info_window (fname, s, &changelogwindow);
}

static GtkWidget *gplwindow;

void
on_gpl1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_system_dir (DDB_SYS_DIR_DOC), "COPYING.GPLv2");
    gtkui_show_info_window (fname, "GNU GENERAL PUBLIC LICENSE Version 2", &gplwindow);
}

static GtkWidget *lgplwindow;

void
on_lgpl1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_system_dir (DDB_SYS_DIR_DOC), "COPYING.LGPLv2.1");
    gtkui_show_info_window (fname, "GNU LESSER GENERAL PUBLIC LICENSE Version 2.1", &lgplwindow);
}

gboolean
on_helpwindow_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
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

// defined in plcommon.c
extern int editcolumn_title_changed;

void
on_editcolumn_title_changed (GtkEditable *editable, gpointer user_data) {
    editcolumn_title_changed = 1;
}

void
on_column_id_changed (GtkComboBox *combobox, gpointer user_data) {
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
    gtk_widget_set_sensitive (fmt, act == find_first_preset_column_type (DB_COLUMN_CUSTOM) ? TRUE : FALSE);

    if (!editcolumn_title_changed) {
        GtkWidget *title = lookup_widget (toplevel, "title");
        if (title) {
            gtk_entry_set_text (GTK_ENTRY (title), gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combobox)));
            editcolumn_title_changed = 0;
        }
    }
}

gboolean
on_mainwin_window_state_event (GtkWidget *widget, GdkEventWindowState *event, gpointer user_data) {
    wingeom_save_max (event, widget, "mainwin");
    return FALSE;
}

void
on_toggle_status_bar_activate (GtkMenuItem *menuitem, gpointer user_data) {
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
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_stop_after_current_activate (GtkMenuItem *menuitem, gpointer user_data) {
    deadbeef->conf_set_int (
        "playlist.stop_after_current",
        gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_stop_after_album_activate (GtkMenuItem *menuitem, gpointer user_data) {
    deadbeef->conf_set_int (
        "playlist.stop_after_album",
        gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_cursor_follows_playback_activate (GtkMenuItem *menuitem, gpointer user_data) {
    deadbeef->conf_set_int (
        "playlist.scroll.cursorfollowplayback",
        gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

GtkWidget *
create_ddb_listview_widget (gchar *widget_name, gchar *string1, gchar *string2, gint int1, gint int2) {
    return ddb_listview_new ();
}

GtkWidget *
create_tabstrip_widget (gchar *widget_name, gchar *string1, gchar *string2, gint int1, gint int2) {
    return ddb_tabstrip_new ();
}

GtkWidget *
create_volumebar_widget (gchar *widget_name, gchar *string1, gchar *string2, gint int1, gint int2) {
    return ddb_volumebar_new ();
}

void
on_mainwin_realize (GtkWidget *widget, gpointer user_data) {
    gtkui_init_theme_colors ();
}

void
on_toggle_eq (GtkMenuItem *menuitem, gpointer user_data) {
    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
        deadbeef->conf_set_int ("gtkui.eq.visible", 0);
        eq_window_hide ();
    }
    else {
        deadbeef->conf_set_int ("gtkui.eq.visible", 1);
        eq_window_show ();
    }
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_deselect_all1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    action_deselect_all_handler_cb (NULL);
}

void
on_invert_selection1_activate (GtkMenuItem *menuitem, gpointer user_data) {
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
}

void
on_new_playlist1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    action_new_playlist_handler_cb (NULL);
}

gboolean
on_mainwin_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    GtkWidget *statusbar = lookup_widget (mainwin, "statusbar");
    GtkAllocation c;
    gtk_widget_get_allocation (statusbar, &c);
    if (event->x >= c.x && event->x < c.x + c.width && event->y >= c.y && event->y < c.y + c.height) {
        if (event->type == GDK_2BUTTON_PRESS) {
            deadbeef->sendmessage (DB_EV_TRACKFOCUSCURRENT, 0, 0, 0);
        }
    }

    return FALSE;
}

GtkWidget *
create_seekbar (gchar *widget_name, gchar *string1, gchar *string2, gint int1, gint int2) {
    return GTK_WIDGET (ddb_seekbar_new ());
}

void
on_jump_to_current_track1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    deadbeef->sendmessage (DB_EV_TRACKFOCUSCURRENT, 0, 0, 0);
}

static GtkWidget *translatorswindow;

void
on_translators1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    char s[200];
    snprintf (s, sizeof (s), _ ("DeaDBeeF Translators"));
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_system_dir (DDB_SYS_DIR_DOC), "translators.txt");
    gtkui_show_info_window (fname, s, &translatorswindow);
}

GtkWidget *
title_formatting_help_link_create (gchar *widget_name, gchar *string1, gchar *string2, gint int1, gint int2) {
    GtkWidget *link = gtk_link_button_new_with_label (
        "http://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0",
        _ ("Help"));
    return link;
}

void
on_sortfmt_activate (GtkEntry *entry, gpointer user_data) {
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (entry));
    gtk_dialog_response (GTK_DIALOG (toplevel), GTK_RESPONSE_OK);
}

GtkWidget *
create_plugin_weblink (gchar *widget_name, gchar *string1, gchar *string2, gint int1, gint int2) {
    GtkWidget *link = gtk_link_button_new_with_label ("", "WWW");
    gtk_widget_set_sensitive (link, FALSE);
    return link;
}

void
on_sort_by_title_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%title%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
on_sort_by_track_nr_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%tracknumber%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
on_sort_by_album_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%album%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
on_sort_by_artist_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%artist%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
on_sort_by_date_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%year%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
on_sort_by_random_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, NULL, DDB_SORT_RANDOM);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
on_sort_by_custom_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gdk_threads_add_idle (action_sort_custom_handler_cb, NULL);
}

void
on_design_mode1_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    w_set_design_mode (act ? 1 : 0);
}

void
on_preferences_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gdk_threads_add_idle (action_preferences_handler_cb, NULL);
}

gboolean
on_prefwin_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (gtkui_hotkey_grabbing) {
        on_hotkeys_set_key_key_press_event (widget, event, user_data);
        return TRUE;
    }
    return FALSE;
}

void
on_view_log_activate (GtkMenuItem *menuitem, gpointer user_data) {
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    gtkui_show_log_window (act);
}

void
on_log_clear_clicked (GtkButton *button, gpointer user_data) {
    GtkWidget *textview = lookup_widget (gtk_widget_get_toplevel (GTK_WIDGET (button)), "logwindow_textview");
    GtkTextBuffer *buffer;
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
    gtk_text_buffer_set_text (buffer, "", 0);
}

gboolean
on_log_window_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->keyval == GDK_Escape) {
        gtkui_show_log_window (FALSE);
    }
    return FALSE;
}

void
on_copy_plugin_report_menuitem_activate (GtkMenuItem *menuitem, gpointer user_data) {
    GString *list = g_string_sized_new (1024);

    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugins[i]; i++) {
        const char *path = deadbeef->plug_get_path_for_plugin_ptr (plugins[i]);
        g_string_append_printf (
            list,
            "%s: %s (%d.%d)\n",
            path ? path : "(builtin)",
            plugins[i]->name,
            plugins[i]->version_major,
            plugins[i]->version_minor);
    }

    GtkClipboard *clp = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text (clp, list->str, -1);
    g_string_free (list, TRUE);
}

void
on_sortcancel_clicked (GtkButton *button, gpointer user_data) {
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    gtk_dialog_response (GTK_DIALOG (toplevel), GTK_RESPONSE_CANCEL);
}

void
on_sortok_clicked (GtkButton *button, gpointer user_data) {
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    gtk_dialog_response (GTK_DIALOG (toplevel), GTK_RESPONSE_OK);
}

void
on_sortfmt_show (GtkWidget *widget, gpointer user_data) {
    // libglade format does not allow assigning a GtkTextBuffer to a GtkTextView
    // the buffer is unref'd by the TextView when it's destroyed
    GtkTextBuffer *sortbuffer;
    sortbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
    if (sortbuffer == NULL) {
        sortbuffer = gtk_text_buffer_new (NULL);
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (widget), sortbuffer);
        g_object_unref (G_OBJECT (sortbuffer));
    }
}

void
on_mainwin_undo_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkui_perform_undo ();
}


void
on_mainwin_redo_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtkui_perform_redo ();
}
