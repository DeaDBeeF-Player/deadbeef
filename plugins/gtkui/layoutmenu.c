/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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
#    include "../../config.h"
#endif
#include <gtk/gtk.h>
#include <jansson.h>
#include <stdlib.h>
#include <string.h>
#include <deadbeef/deadbeef.h>
#include "gtkui.h"
#include "layoutmanager.h"
#include "layoutmenu.h"
#include "support.h"
#include "widgets.h"

// The user layouts go to <config>/gtkui_layouts, the built-in ones are installed into <plugin resources>/ui_layouts
#define USER_LAYOUTS_DIR_NAME "gtkui_layouts"
#define BUILTIN_LAYOUTS_DIR_NAME "ui_layouts"
#define LAYOUT_PATH_DATA_KEY "layout_path"

static layoutmanager_entry_t *_builtin_layouts;

static char *
_user_layouts_dir (void) {
    return g_build_filename (deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), USER_LAYOUTS_DIR_NAME, NULL);
}

static char *
_builtin_layouts_dir (void) {
    return g_build_filename (deadbeef->get_system_dir (DDB_SYS_DIR_PLUGIN_RESOURCES), BUILTIN_LAYOUTS_DIR_NAME, NULL);
}

static void
_show_error (const char *message, const char *details) {
    GtkWidget *dlg = gtk_message_dialog_new (
        GTK_WINDOW (mainwin),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s",
        message);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), "%s", details);
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
}

static const char *
_error_details (layoutmanager_error_t error) {
    switch (error) {
    case LAYOUTMANAGER_ERROR_IO:
        return _("The file could not be read or written.");
    case LAYOUTMANAGER_ERROR_WRONG_MAGIC:
        return _("The file is not a DeaDBeeF layout file.");
    case LAYOUTMANAGER_ERROR_UNSUPPORTED_VERSION:
        return _("The layout file was created by a newer version of DeaDBeeF. Please update DeaDBeeF.");
    case LAYOUTMANAGER_ERROR_INVALID_FILE:
    case LAYOUTMANAGER_OK:
        break;
    }
    return _("The file is not a valid layout file.");
}

static void
_apply_layout_file (const char *path) {
    layoutmanager_error_t error;
    json_t *layout = layoutmanager_load_file (path, DDB_GTKUI_LAYOUT_VERSION, &error);
    if (layout == NULL) {
        _show_error (_("Failed to load the layout"), _error_details (error));
        return;
    }

    int res = w_load_layout_from_json (layout);
    json_decref (layout);

    if (res != 0) {
        _show_error (_("Failed to apply the layout"), _("The layout contains invalid widgets."));
    }
}

static GtkFileFilter *
_create_file_filter (void) {
    GtkFileFilter *filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("DeaDBeeF layout files (*.ddblayout)"));
    gtk_file_filter_add_pattern (filter, "*." LAYOUTMANAGER_FILE_EXTENSION);
    return filter;
}

static void
_on_preset_activate (GtkMenuItem *menuitem, gpointer user_data) {
    const char *path = g_object_get_data (G_OBJECT (menuitem), LAYOUT_PATH_DATA_KEY);
    if (path != NULL) {
        _apply_layout_file (path);
    }
}

static void
_on_load_from_file_activate (GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new (
        _("Load UI Layout"),
        GTK_WINDOW (mainwin),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL,
        GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN,
        GTK_RESPONSE_OK,
        NULL);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), _create_file_filter ());

    char *path = NULL;
    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK) {
        path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
    }
    gtk_widget_destroy (dlg);

    if (path != NULL) {
        _apply_layout_file (path);
        g_free (path);
    }
}

static void
_on_save_to_file_activate (GtkMenuItem *menuitem, gpointer user_data) {
    json_t *layout = w_save_layout_to_json ();
    if (layout == NULL) {
        return;
    }

    GtkWidget *dlg = gtk_file_chooser_dialog_new (
        _("Save UI Layout"),
        GTK_WINDOW (mainwin),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL,
        GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE,
        GTK_RESPONSE_OK,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg), TRUE);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), _create_file_filter ());

    // New user layouts go to the config folder. The folder picked by the user is not remembered.
    char *user_dir = _user_layouts_dir ();
    if (layoutmanager_create_directory (user_dir) == 0) {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg), user_dir);
    }
    g_free (user_dir);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "Layout." LAYOUTMANAGER_FILE_EXTENSION);

    char *path = NULL;
    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK) {
        path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
    }
    gtk_widget_destroy (dlg);

    if (path != NULL) {
        if (!g_str_has_suffix (path, "." LAYOUTMANAGER_FILE_EXTENSION)) {
            char *path_with_ext = g_strconcat (path, "." LAYOUTMANAGER_FILE_EXTENSION, NULL);
            g_free (path);
            path = path_with_ext;
        }

        layoutmanager_error_t error = layoutmanager_save_file (path, DDB_GTKUI_LAYOUT_VERSION, layout);
        if (error != LAYOUTMANAGER_OK) {
            _show_error (_("Failed to save the layout"), _error_details (error));
        }
        g_free (path);
    }

    json_decref (layout);
}

static void
_on_browse_user_layouts_activate (GtkMenuItem *menuitem, gpointer user_data) {
    char *user_dir = _user_layouts_dir ();
    if (layoutmanager_create_directory (user_dir) != 0) {
        _show_error (_("Failed to open the folder"), user_dir);
        g_free (user_dir);
        return;
    }

    // run in background, the file manager may not return until it's closed
    char *quoted_dir = g_shell_quote (user_dir);
#ifdef __APPLE__
    const char *opener = "open";
#else
    const char *opener = "xdg-open";
#endif
    char *command = g_strdup_printf ("%s %s >/dev/null 2>&1 &", opener, quoted_dir);
    if (system (command) != 0) {
        _show_error (_("Failed to open the file manager"), user_dir);
    }
    g_free (command);
    g_free (quoted_dir);
    g_free (user_dir);
}

static void
_add_menu_item (GtkWidget *menu, const char *label, GCallback callback, const char *layout_path) {
    GtkWidget *item = gtk_menu_item_new_with_label (label);
    if (callback != NULL) {
        g_signal_connect (item, "activate", callback, NULL);
    }
    else {
        gtk_widget_set_sensitive (item, FALSE);
    }
    if (layout_path != NULL) {
        g_object_set_data_full (G_OBJECT (item), LAYOUT_PATH_DATA_KEY, g_strdup (layout_path), g_free);
    }
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
}

static void
_add_separator (GtkWidget *menu) {
    GtkWidget *separator = gtk_separator_menu_item_new ();
    gtk_widget_show (separator);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator);
}

static void
_add_layouts (GtkWidget *menu, const char *heading, layoutmanager_entry_t *entries) {
    _add_menu_item (menu, heading, NULL, NULL);
    for (layoutmanager_entry_t *entry = entries; entry != NULL; entry = entry->next) {
        char *label = g_strconcat ("  ", entry->name, NULL);
        _add_menu_item (menu, label, G_CALLBACK (_on_preset_activate), entry->path);
        g_free (label);
    }
}

// The built-in layouts are loaded on startup, the user layouts are rescanned each time the menu is shown.
// The menu must never be left empty, because add_mainmenu_actions removes empty submenus from the menubar.
static void
_rebuild_layouts_menu (GtkWidget *menu) {
    gtk_container_foreach (GTK_CONTAINER (menu), (GtkCallback)gtk_widget_destroy, NULL);

    char *user_dir = _user_layouts_dir ();
    layoutmanager_entry_t *user_layouts = layoutmanager_scan_directory (user_dir);
    g_free (user_dir);

    if (_builtin_layouts == NULL && user_layouts == NULL) {
        _add_menu_item (menu, _("No layouts found"), NULL, NULL);
    }
    if (_builtin_layouts != NULL) {
        _add_layouts (menu, _("Built-in layouts:"), _builtin_layouts);
    }
    if (_builtin_layouts != NULL && user_layouts != NULL) {
        _add_separator (menu);
    }
    if (user_layouts != NULL) {
        _add_layouts (menu, _("User layouts:"), user_layouts);
    }

    _add_separator (menu);
    _add_menu_item (menu, _("Load from File..."), G_CALLBACK (_on_load_from_file_activate), NULL);
    _add_menu_item (menu, _("Save to File..."), G_CALLBACK (_on_save_to_file_activate), NULL);
    _add_menu_item (menu, _("Browse User Layouts in File Manager"), G_CALLBACK (_on_browse_user_layouts_activate), NULL);

    layoutmanager_entries_free (user_layouts);
}

static void
_on_layouts_menu_show (GtkWidget *menu, gpointer user_data) {
    _rebuild_layouts_menu (menu);
}

void
layoutmenu_init (void) {
    char *builtin_dir = _builtin_layouts_dir ();
    layoutmanager_entries_free (_builtin_layouts);
    _builtin_layouts = layoutmanager_scan_directory (builtin_dir);
    g_free (builtin_dir);

    GtkWidget *design_mode_item = lookup_widget (mainwin, "design_mode1");
    GtkWidget *view_menu = design_mode_item != NULL ? gtk_widget_get_parent (design_mode_item) : NULL;
    if (view_menu == NULL) {
        return;
    }

    GList *children = gtk_container_get_children (GTK_CONTAINER (view_menu));
    gint position = g_list_index (children, design_mode_item) + 1;
    g_list_free (children);

    GtkWidget *item = gtk_menu_item_new_with_label (_("UI Layouts"));
    GtkWidget *layouts_menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), layouts_menu);
    g_signal_connect (layouts_menu, "show", G_CALLBACK (_on_layouts_menu_show), NULL);
    _rebuild_layouts_menu (layouts_menu);

    gtk_menu_shell_insert (GTK_MENU_SHELL (view_menu), item, position);
    gtk_widget_show (item);
}

void
layoutmenu_free (void) {
    layoutmanager_entries_free (_builtin_layouts);
    _builtin_layouts = NULL;
}
