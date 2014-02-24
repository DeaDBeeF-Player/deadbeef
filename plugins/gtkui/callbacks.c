/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __APPLE__
#include <X11/Xlib.h>
#endif
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
#include "../libparser/parser.h"
#include "drawing.h"
#include "eq.h"
#include "wingeom.h"
#include "widgets.h"
#include "../hotkeys/hotkeys.h"
#include "actionhandlers.h"
#include "actions.h"

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define SELECTED(it) (deadbeef->pl_is_selected(it))
#define SELECT(it, sel) (deadbeef->pl_set_selected(it,sel))
#define VSELECT(it, sel) {deadbeef->pl_set_selected(it,sel);gtk_pl_redraw_item_everywhere (it);}
#define PL_NEXT(it, iter) (deadbeef->pl_get_next(it, iter))

DdbListview *last_playlist;
extern DB_functions_t *deadbeef; // defined in gtkui.c


void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_open_files_handler_cb, NULL);
}


void
on_add_files_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_add_files_handler_cb, NULL);
}

void
on_add_folders_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_add_folders_handler_cb, NULL);
}


void
on_quit_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_quit_handler_cb, NULL);
}



void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_select_all_handler_cb, NULL);
}





void
on_stopbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
}


void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    DB_output_t *output = deadbeef->get_output ();
    if (output->state () == OUTPUT_STATE_PAUSED) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cur != -1) {
            ddb_playItem_t *it = deadbeef->plt_get_item_for_idx (plt, cur, PL_MAIN);
            ddb_playItem_t *it_playing = deadbeef->streamer_get_playing_track ();
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
        deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
    }
}


void
on_pausebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
}


void
on_prevbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_PREV, 0, 0, 0);
}


void
on_nextbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
}


void
on_playrand_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_PLAY_RANDOM, 0, 0, 0);
}

gboolean
on_mainwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    // local hotkeys
    GdkModifierType consumed;
    guint accel_key;
    gdk_keymap_translate_keyboard_state (gdk_keymap_get_default (), event->hardware_keycode, event->state, 0, &accel_key, NULL, NULL, &consumed);
    if (accel_key == GDK_ISO_Left_Tab)
        accel_key = GDK_Tab;
    int mods = event->state & gtk_accelerator_get_default_mod_mask ();
    mods &= ~(consumed&~GDK_SHIFT_MASK);
    int lower = gdk_keyval_to_lower (accel_key);
    if (lower != accel_key) {
        accel_key = lower;
    }
    trace ("pressed: keycode: %x, mods: %x, hw: %x, translated: %x\n", event->keyval, mods, event->hardware_keycode, accel_key);

    DB_plugin_t *hkplug = deadbeef->plug_get_for_id ("hotkeys");
    if (hkplug) {
        int ctx;
        DB_plugin_action_t *act = ((DB_hotkeys_plugin_t *)hkplug)->get_action_for_keycombo (accel_key, mods, 0, &ctx);
        if (act && act->callback2) {
            trace ("executing action %s in ctx %d\n", act->name, ctx);
            act->callback2 (act, ctx);
            return TRUE;
        }
        else if (act && act->callback) {
            gtkui_exec_action_14 (act, -1);
        }
    }
    trace ("action not found\n");
    return FALSE;
}


void
on_order_linear_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_LINEAR);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_order_shuffle_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_SHUFFLE_TRACKS);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_order_shuffle_albums_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_SHUFFLE_ALBUMS);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_order_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_RANDOM);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_loop_all_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.loop", 0);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_loop_single_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.loop", 2);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_loop_disable_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playback.loop", 1);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_searchlist_realize                  (GtkWidget       *widget,
                                        gpointer         user_data)
{
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
        gtkui_quit ();
    }
    return TRUE;
}

gboolean
on_mainwin_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    wingeom_save (widget, "mainwin");
    return FALSE;
}


void
on_scroll_follows_playback_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.scroll.followplayback", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_find_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    search_start ();
}

void
on_help1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_show_help_handler_cb, NULL);
}

static GtkWidget *aboutwindow;

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char s[200];
    snprintf (s, sizeof (s), _("About DeaDBeeF %s"), VERSION);
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "about.txt");
    gtkui_show_info_window (fname, s, &aboutwindow);
}

static GtkWidget *changelogwindow;

void
on_changelog1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char s[200];
    snprintf (s, sizeof (s), _("DeaDBeeF %s ChangeLog"), VERSION);
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "ChangeLog");
    gtkui_show_info_window (fname, s, &changelogwindow);
}

static GtkWidget *gplwindow;

void
on_gpl1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "COPYING.GPLv2");
    gtkui_show_info_window (fname, "GNU GENERAL PUBLIC LICENSE Version 2", &gplwindow);
}

static GtkWidget *lgplwindow;

void
on_lgpl1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "COPYING.LGPLv2.1");
    gtkui_show_info_window (fname, "GNU LESSER GENERAL PUBLIC LICENSE Version 2.1", &lgplwindow);
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

// defined in plcommon.c
extern int editcolumn_title_changed;

void
on_editcolumn_title_changed            (GtkEditable     *editable,
                                        gpointer         user_data)
{
    editcolumn_title_changed = 1;
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
    gtk_widget_set_sensitive (fmt, act >= 10 ? TRUE : FALSE);

    if (!editcolumn_title_changed) {
        GtkWidget *title= lookup_widget (toplevel, "title");
        if (title) {
            gtk_entry_set_text (GTK_ENTRY (title), gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combobox)));
            editcolumn_title_changed = 0;
        }
    }
}


gboolean
on_mainwin_window_state_event          (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    wingeom_save_max (event, widget, "mainwin");
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
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_stop_after_current_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.stop_after_current", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_cursor_follows_playback_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.scroll.cursorfollowplayback", gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
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
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}



void
on_deselect_all1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_deselect_all_handler_cb (NULL);
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    action_new_playlist_handler_cb (NULL);
}


static GtkWidget *capture = NULL;

gboolean
on_mainwin_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->window != gtk_widget_get_window (mainwin)) {
        return FALSE;
    }
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    GtkWidget *seekbar = lookup_widget (mainwin, "seekbar");
    GtkWidget *statusbar = lookup_widget (mainwin, "statusbar");
    GtkAllocation a, b, c;
    gtk_widget_get_allocation (volumebar, &a);
    gtk_widget_get_allocation (seekbar, &b);
    gtk_widget_get_allocation (statusbar, &c);
    if (event->x >= a.x && event->x < a.x + a.width
            && event->y >= a.y && event->y < a.y + a.height) {
        capture = volumebar;
        return gtk_widget_event (volumebar, (GdkEvent *)event);
    }
    else if (event->x >= b.x && event->x < b.x + b.width
            && event->y >= b.y && event->y < b.y + b.height) {
        capture = seekbar;
        return gtk_widget_event (seekbar, (GdkEvent *)event);
    }
    else if (event->x >= c.x && event->x < c.x + c.width
            && event->y >= c.y && event->y < c.y + c.height) {
        if (event->type == GDK_2BUTTON_PRESS) {
            deadbeef->sendmessage (DB_EV_TRACKFOCUSCURRENT, 0, 0, 0);
        }
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
    if (event->window != gtk_widget_get_window (mainwin)) {
        return FALSE;
    }
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    GtkWidget *seekbar = lookup_widget (mainwin, "seekbar");
    GtkAllocation a;
    gtk_widget_get_allocation (volumebar, &a);
    GtkAllocation b;
    gtk_widget_get_allocation (seekbar, &b);
    if (event->x >= a.x && event->x < a.x + a.width
            && event->y >= a.y && event->y < a.y + a.height) {
        return gtk_widget_event (volumebar, (GdkEvent *)event);
    }
    else if (event->x >= b.x && event->x < b.x + b.width
            && event->y >= b.y && event->y < b.y + b.height) {
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


void
on_jump_to_current_track1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    deadbeef->sendmessage (DB_EV_TRACKFOCUSCURRENT, 0, 0, 0);
}

static GtkWidget *translatorswindow;

void
on_translators1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char s[200];
    snprintf (s, sizeof (s), _("DeaDBeeF Translators"));
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/%s", deadbeef->get_doc_dir (), "translators.txt");
    gtkui_show_info_window (fname, s, &translatorswindow);
}


GtkWidget*
title_formatting_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("http://sourceforge.net/apps/mediawiki/deadbeef/index.php?title=Title_Formatting", "Help");
    return link;
}



void
on_sortfmt_activate                    (GtkEntry        *entry,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (entry));
    gtk_dialog_response (GTK_DIALOG (toplevel), GTK_RESPONSE_OK);
}



GtkWidget*
create_plugin_weblink (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("", "WWW");
    gtk_widget_set_sensitive (link, FALSE);
    return link;
}

void
on_sort_by_title_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%t", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_track_nr_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%n", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_album_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%b", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_artist_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%a", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_date_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, "%y", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_MAIN, -1, NULL, DDB_SORT_RANDOM);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}


void
on_sort_by_custom_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_sort_custom_handler_cb, NULL);
}

void
on_design_mode1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gboolean act = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    w_set_design_mode (act ? 1 : 0);
}

void
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gdk_threads_add_idle (action_preferences_handler_cb, NULL);
}
