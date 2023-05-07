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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <assert.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "../../../gettext.h"
#include "../callbacks.h"
#include "../ctmapping.h"
#include "../drawing.h"
#include "../dspconfig.h"
#include "../eq.h"
#include "../gtkui.h"
#include "../hotkeys.h"
#include "../interface.h"
#include "../pluginconf.h"
#include "../support.h"
#include "../wingeom.h"
#include "prefwin.h"
#include "prefwinappearance.h"
#if ENABLE_MEDIALIVB
#include "prefwinmedialib.h"
#endif
#include "prefwinmisc.h"
#include "prefwinnetwork.h"
#include "prefwinplayback.h"
#include "prefwinplugins.h"
#include "prefwinsound.h"

int PREFWIN_TAB_INDEX_SOUND = 0;
int PREFWIN_TAB_INDEX_PLAYBACK = 1;
int PREFWIN_TAB_INDEX_DSP = 2;
int PREFWIN_TAB_INDEX_GUI = 3;
int PREFWIN_TAB_INDEX_APPEARANCE = 4;
int PREFWIN_TAB_INDEX_MEDIALIB = 5;
int PREFWIN_TAB_INDEX_NETWORK = 6;
int PREFWIN_TAB_INDEX_HOTKEYS = 7;
int PREFWIN_TAB_INDEX_PLUGINS = 8;

static GtkWidget *prefwin;

void
prefwin_init_theme_colors (void) {
    GdkColor clr;
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_background")), ((void)(gtkui_get_bar_background_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_foreground")), ((void)(gtkui_get_bar_foreground_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_dark")), ((void)(gtkui_get_tabstrip_dark_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_mid")), ((void)(gtkui_get_tabstrip_mid_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_light")), ((void)(gtkui_get_tabstrip_light_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_base")), ((void)(gtkui_get_tabstrip_base_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_text")), ((void)(gtkui_get_tabstrip_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_playing_text")), ((void)(gtkui_get_tabstrip_playing_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_selected_text")), ((void)(gtkui_get_tabstrip_selected_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_even_row")), ((void)(gtkui_get_listview_even_row_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_odd_row")), ((void)(gtkui_get_listview_odd_row_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_row")), ((void)(gtkui_get_listview_selection_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_text")), ((void)(gtkui_get_listview_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_text")), ((void)(gtkui_get_listview_selected_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_playing_text")), ((void)(gtkui_get_listview_playing_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_group_text")), ((void)(gtkui_get_listview_group_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_column_text")), ((void)(gtkui_get_listview_column_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_cursor")), ((void)(gtkui_get_listview_cursor_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "visualization_custom_color_button")), ((void)(gtkui_get_vis_custom_base_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "visualization_custom_background_color_button")), ((void)(gtkui_get_vis_custom_background_color (&clr)), &clr));
}

void
prefwin_set_scale (const char *scale_name, int value) {
    GtkWidget *scale = lookup_widget (prefwin, scale_name);
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("value_changed");
    g_signal_handlers_block_matched ((gpointer)scale, mask, detail, 0, NULL, NULL, NULL);
    gtk_range_set_value (GTK_RANGE (scale), value);
    g_signal_handlers_unblock_matched ((gpointer)scale, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_combobox (GtkComboBox *combo, int i) {
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("changed");
    g_signal_handlers_block_matched ((gpointer)combo, mask, detail, 0, NULL, NULL, NULL);
    gtk_combo_box_set_active (combo, i);
    g_signal_handlers_unblock_matched ((gpointer)combo, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_toggle_button (const char *button_name, int value) {
    GtkToggleButton *button = GTK_TOGGLE_BUTTON (lookup_widget (prefwin, button_name));
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("toggled");
    g_signal_handlers_block_matched ((gpointer)button, mask, detail, 0, NULL, NULL, NULL);
    gtk_toggle_button_set_active (button, value);
    g_signal_handlers_unblock_matched ((gpointer)button, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_entry_text (const char *entry_name, const char *text) {
    GtkEntry *entry = GTK_ENTRY (lookup_widget (prefwin, entry_name));
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("changed");
    g_signal_handlers_block_matched ((gpointer)entry, mask, detail, 0, NULL, NULL, NULL);
    gtk_entry_set_text (entry, text);
    g_signal_handlers_unblock_matched ((gpointer)entry, mask, detail, 0, NULL, NULL, NULL);
}

void
on_prefwin_response_cb (GtkDialog *dialog,
                        int        response_id,
                        gpointer   user_data) {
    if (response_id != GTK_RESPONSE_CLOSE && response_id != GTK_RESPONSE_DELETE_EVENT) {
        return;
    }

    if (gtkui_hotkeys_changed) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (prefwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("You modified the hotkeys settings, but didn't save your changes."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefwin));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Are you sure you want to continue without saving?"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));
        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response == GTK_RESPONSE_NO) {
            return;
        }
    }

    dsp_setup_free ();
    ctmapping_setup_free ();
    gtk_widget_destroy (prefwin);
    deadbeef->conf_save ();
    prefwin_free_plugins ();
#if ENABLE_MEDIALIVB
    prefwin_free_medialib ();
#endif

    prefwin = NULL;
}

static void
_init_prefwin(void) {
    if (prefwin != NULL) {
        return;
    }
    GtkWidget *w = prefwin = create_prefwin ();

    // hide unavailable tabs
    if (!deadbeef->plug_get_for_id ("hotkeys")) {
        gtk_notebook_remove_page (GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), 7);
        PREFWIN_TAB_INDEX_HOTKEYS = -1;
    }
    if (!deadbeef->plug_get_for_id ("medialib")) {
        gtk_notebook_remove_page (GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), 5);
        PREFWIN_TAB_INDEX_MEDIALIB = -1;
    }

    gtkui_init_theme_colors();

    gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (mainwin));

    deadbeef->conf_lock ();

    // output plugin selection
    prefwin_init_sound_tab (prefwin);

    // replaygain_mode
    prefwin_init_playback_tab (prefwin);

    // dsp
    dsp_setup_init (prefwin);

    // minimize_on_startup
    prefwin_init_gui_misc_tab (prefwin);

    // override bar colors
    prefwin_init_appearance_tab (prefwin);

#if ENABLE_MEDIALIB
    prefwin_init_medialib (prefwin);
#endif

    // network
    ctmapping_setup_init (w);
    prefwin_init_network_tab (prefwin);

    // list of plugins
    prefwin_init_plugins_tab (prefwin);

    // hotkeys
    if (PREFWIN_TAB_INDEX_HOTKEYS != -1) {
        prefwin_init_hotkeys (prefwin);
    }

    deadbeef->conf_unlock ();

    g_signal_connect (GTK_DIALOG (prefwin), "response", G_CALLBACK (on_prefwin_response_cb), NULL);

    gtk_window_set_modal (GTK_WINDOW (prefwin), FALSE);
    gtk_window_set_position (GTK_WINDOW (prefwin), GTK_WIN_POS_CENTER_ON_PARENT);
}

void
prefwin_run (int tab_index) {
    _init_prefwin();

    if (tab_index != -1) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), tab_index);
    }

#if GTK_CHECK_VERSION(2,28,0)
    gtk_window_present_with_time (GTK_WINDOW(prefwin), (guint32)(g_get_monotonic_time() / 1000));
#else
    gtk_window_present_with_time (GTK_WINDOW(prefwin), GDK_CURRENT_TIME);
#endif
}

gboolean
on_prefwin_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    wingeom_save (widget, "prefwin");
    return FALSE;
}


gboolean
on_prefwin_window_state_event          (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    wingeom_save_max (event, widget, "prefwin");
    return FALSE;
}


void
on_prefwin_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
    wingeom_restore (widget, "prefwin", -1, -1, -1, -1, 0);
}
