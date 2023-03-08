/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2013 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include "../../gettext.h"
#include <string.h>
#include <stdlib.h>
#include "gtkui.h"
#include "clipboard.h"
#include "interface.h"
#include "support.h"
#include "plmenu.h"

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

// selected playlist for the context menu
static ddb_playlist_t *current_playlist;

static void
_set_playlist (ddb_playlist_t *playlist) {
    if (current_playlist != NULL) {
        deadbeef->plt_unref (current_playlist);
        current_playlist = NULL;
    }

    current_playlist = playlist;
    if (current_playlist != NULL) {
        deadbeef->plt_ref (current_playlist);
    }
}

void
gtkui_free_pltmenu (void) {
    _set_playlist(NULL);
}

static void
on_rename_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (current_playlist != NULL) {
        gtkui_rename_playlist(current_playlist);
    }
}

static void
on_remove_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (current_playlist == NULL) {
        return;
    }
    if (gtkui_remove_playlist(current_playlist) == -1) {
        return;
    }
    _set_playlist(NULL);
}

static void
on_add_new_playlist1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int playlist = gtkui_add_new_playlist ();
    if (playlist != -1) {
        deadbeef->plt_set_curr_idx (playlist);
    }
}

static void
on_autosort_toggled (GtkMenuItem     *menuitem,
                    gpointer         user_data)
{
    if (current_playlist == NULL) {
        return;
    }
    int enabled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(menuitem));
    deadbeef->plt_set_meta_int (current_playlist, "autosort_enabled", enabled);
}

GtkWidget*
gtkui_create_pltmenu (ddb_playlist_t *playlist) {
    GtkWidget *menu = NULL;
    GtkWidget *rename_playlist1 = NULL;
    GtkWidget *remove_playlist1 = NULL;
    GtkWidget *add_new_playlist1 = NULL;
    GtkWidget *autosort = NULL;
    GtkWidget *separator11 = NULL;

    _set_playlist(playlist);

    menu = gtk_menu_new ();

    if (playlist != NULL) {
        int selected_count = 0;
        ddb_playItem_t *track = NULL;
        selected_count = deadbeef->plt_get_item_count (playlist, PL_MAIN);
        if (selected_count != 0) {
            track = deadbeef->plt_get_first (playlist, PL_MAIN);
        }

        trk_context_menu_update_with_playlist(playlist, DDB_ACTION_CTX_PLAYLIST);
        trk_context_menu_build(menu);

        if (track != NULL) {
            deadbeef->pl_item_unref (track);
            track = NULL;
        }

        rename_playlist1 = gtk_menu_item_new_with_mnemonic (_("Rename Playlist"));
        if (current_playlist == NULL) {
            gtk_widget_set_sensitive (rename_playlist1, FALSE);
        }
        gtk_widget_show (rename_playlist1);
        gtk_menu_shell_insert(GTK_MENU_SHELL(menu), rename_playlist1, 0);

        remove_playlist1 = gtk_menu_item_new_with_mnemonic (_("Remove Playlist"));
        if (current_playlist == NULL) {
            gtk_widget_set_sensitive (remove_playlist1, FALSE);
        }
        gtk_widget_show (remove_playlist1);
        gtk_menu_shell_insert(GTK_MENU_SHELL(menu), remove_playlist1, 1);
    }

    add_new_playlist1 = gtk_menu_item_new_with_mnemonic (_("Add New Playlist"));
    gtk_widget_show (add_new_playlist1);
    gtk_menu_shell_insert(GTK_MENU_SHELL(menu), add_new_playlist1, playlist != NULL ? 2 : 0);

    if (playlist != NULL) {
        int autosort_enabled = 0;
        if (current_playlist != NULL) {
            autosort_enabled = deadbeef->plt_find_meta_int (current_playlist, "autosort_enabled", 0);
        }
        autosort = gtk_check_menu_item_new_with_label (_("Enable Autosort"));
        gtk_widget_set_tooltip_text(autosort, _("Re-apply the last sort you chose every time when adding new files to this playlist"));
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(autosort), autosort_enabled);
        gtk_widget_show (autosort);
        gtk_menu_shell_insert(GTK_MENU_SHELL(menu), autosort, 3);
        if (current_playlist == NULL) {
            gtk_widget_set_sensitive (autosort, FALSE);
        }

        separator11 = gtk_separator_menu_item_new ();
        gtk_widget_show (separator11);
        gtk_menu_shell_insert(GTK_MENU_SHELL(menu), separator11, 4);
        gtk_widget_set_sensitive (separator11, FALSE);
    }

    g_signal_connect ((gpointer) add_new_playlist1, "activate",
                      G_CALLBACK (on_add_new_playlist1_activate),
                      NULL);

    if (playlist != NULL) {
        g_signal_connect ((gpointer) rename_playlist1, "activate",
                G_CALLBACK (on_rename_playlist1_activate),
                NULL);
        g_signal_connect ((gpointer) remove_playlist1, "activate",
                G_CALLBACK (on_remove_playlist1_activate),
                NULL);
        g_signal_connect ((gpointer) autosort, "toggled",
                G_CALLBACK (on_autosort_toggled),
                NULL);
    }

    return menu;
}

