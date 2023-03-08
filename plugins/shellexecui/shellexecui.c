/*
    ShellExec GUI plugin for DeaDBeeF Player
    Copyright (C) 2012 Azeem Arshad <kr00r4n@gmail.com>
    Copyright (C) 2013-2014 Oleksiy Yakovenko
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include <deadbeef/deadbeef.h>
#include "../gtkui/gtkui_api.h"
#include "../shellexec/shellexec.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }

DB_functions_t *deadbeef;
static DB_misc_t plugin;
static ddb_gtkui_t *gtkui_plugin;
static Shx_plugin_t *shellexec_plugin;

static Shx_action_t *actions; // list of actions being edited
static GtkWidget *conf_dlg;
static GtkWidget *edit_dlg;
static Shx_action_t *current_action; // selection action when edit window is active

enum {
    COL_TITLE = 0,
    COL_META,
    COL_COUNT,
};

static int
name_exists(const char *name, Shx_action_t *skip) {
    DB_plugin_t **p = deadbeef->plug_get_list ();
    for (int i = 0; p[i]; i++) {
        if (!p[i]->get_actions) {
            continue;
        }
        DB_plugin_action_t *action = p[i]->get_actions (NULL);
        while(action) {
            if(action != (DB_plugin_action_t*)skip && action->name && !strcmp(action->name, name)) {
                return 1;
            }
            action = action->next;
        }
    }
    return 0;
}

static int
is_empty(const char *name) {
    char *p = (char *)name;
    while(*p) {
        if(*p != ' ' || *p != '\t') {
            return 0;
        }
        p++;
    }
    return 1;
}

void
on_save_button_clicked (GtkButton *button,
                          gpointer user_data) {
    gtk_widget_destroy(conf_dlg);
}

GtkWidget *create_edit_dlg(void) {
    GtkWidget *dlg = create_shellexec_conf_edit_dialog();
    gtk_window_set_transient_for(GTK_WINDOW(dlg),
                                 GTK_WINDOW(conf_dlg));
    return dlg;
}

void
on_add_button_clicked (GtkButton *button,
                          gpointer user_data) {
    current_action = NULL;
    edit_dlg = create_edit_dlg();
    gtk_window_set_title(GTK_WINDOW(edit_dlg), _("Add Command"));

    // generate unique command name
    char name[15] = "new_cmd";
    int suffix = 0;
    while(name_exists(name, NULL) && suffix < 1000) { // create a unique name
        snprintf(name, sizeof (name), "new_cmd%d", suffix);
        suffix++;
    }
    if (name_exists (name, NULL)) {
        return;
    }
    // Set default values in text fields
    gtk_entry_set_text(
        GTK_ENTRY(lookup_widget(edit_dlg, "name_entry")),
        name);
    gtk_entry_set_text(
        GTK_ENTRY(lookup_widget(edit_dlg, "title_entry")),
        "New Command");

    // Set default values in check boxes
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "single_check")),
        TRUE);
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "local_check")),
        TRUE);

    gtk_widget_show(edit_dlg);
}

void
on_remove_button_clicked (GtkButton *button,
                          gpointer user_data) {
    GtkTreeView *treeview = GTK_TREE_VIEW(lookup_widget(conf_dlg, "command_treeview"));
    GtkTreeModel *treemodel = gtk_tree_view_get_model(treeview);
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection(treeview);
    if(gtk_tree_selection_get_selected(selection, &treemodel, &iter)) {
        // ask confirmation
        GtkWidget *confirm_dlg = gtk_message_dialog_new (GTK_WINDOW(conf_dlg), GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
                                                 _("Delete"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (confirm_dlg),
                                                  "%s", _("This action will delete the selected shell command. Are you sure?"));
        gtk_window_set_transient_for(GTK_WINDOW (confirm_dlg), GTK_WINDOW (conf_dlg));
        gtk_window_set_title (GTK_WINDOW (confirm_dlg), _("Confirm Remove"));
        int response = gtk_dialog_run (GTK_DIALOG (confirm_dlg));
        gtk_widget_destroy(confirm_dlg);
        if(response == GTK_RESPONSE_NO) {
            return;
        }

        Shx_action_t *action;
        gtk_tree_model_get(treemodel, &iter, COL_META, &action, -1);

        //remove action from list
        shellexec_plugin->action_remove (action);
        actions = (Shx_action_t *)shellexec_plugin->misc.plugin.get_actions(NULL);

        GtkTreeIter next_iter = iter;
        if(gtk_tree_model_iter_next(treemodel, &next_iter)) {
            gtk_tree_selection_select_iter(selection, &next_iter);
        }
        else {
            int count = gtk_tree_model_iter_n_children(treemodel, NULL);
            if(count >= 2) {
                GtkTreePath *last = gtk_tree_path_new_from_indices(count-2, -1);
                gtk_tree_selection_select_path(selection, last);
            }
        }
        gtk_list_store_remove(GTK_LIST_STORE(treemodel), &iter);

        shellexec_plugin->save_actions();
        deadbeef->sendmessage (DB_EV_ACTIONSCHANGED, 0, 0, 0);
    }
}

void
on_edit_button_clicked(GtkButton *button, gpointer user_data) {
    GtkTreeView *treeview = GTK_TREE_VIEW(lookup_widget(conf_dlg, "command_treeview"));
    GtkTreeModel *treemodel = gtk_tree_view_get_model(treeview);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeIter iter;
    if(gtk_tree_selection_get_selected(selection, &treemodel, &iter)) {
        gtk_tree_model_get(treemodel, &iter, COL_META, &current_action, -1);
        edit_dlg = create_edit_dlg();
        // Set text fields
        gtk_entry_set_text(
            GTK_ENTRY(lookup_widget(edit_dlg, "name_entry")),
            current_action->parent.name);
        gtk_entry_set_text(
            GTK_ENTRY(lookup_widget(edit_dlg, "title_entry")),
            current_action->parent.title);
        gtk_entry_set_text(
            GTK_ENTRY(lookup_widget(edit_dlg, "cmd_entry")),
            current_action->shcommand);

        // Set check boxes
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "single_check")),
            current_action->parent.flags & DB_ACTION_SINGLE_TRACK);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "multiple_check")),
            current_action->parent.flags & DB_ACTION_MULTIPLE_TRACKS);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "local_check")),
            current_action->shx_flags & SHX_ACTION_LOCAL_ONLY);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "remote_check")),
            current_action->shx_flags & SHX_ACTION_REMOTE_ONLY);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "common_check")),
            current_action->parent.flags & DB_ACTION_COMMON);

        gtk_widget_show(edit_dlg);
    }
}

void
on_edit_cancel_button_clicked (GtkButton *button, gpointer user_data) {
    gtk_widget_destroy(edit_dlg);
}

static int
validate_command_edit () {
    const char *text;
    char message[256] = "";
    int valid = 1;

    text = gtk_entry_get_text(GTK_ENTRY(lookup_widget(edit_dlg, "name_entry")));
    if(is_empty(text) || name_exists(text, current_action)) {
        strcat(message, _("ID must be non-empty and unique.\n"));
        valid = 0;
    }

    text = gtk_entry_get_text(GTK_ENTRY(lookup_widget(edit_dlg, "title_entry")));
    if(is_empty(text)) {
        strcat(message, _("Title must be non-empty.\n"));
        valid = 0;
    }

    text = gtk_entry_get_text(GTK_ENTRY(lookup_widget(edit_dlg, "cmd_entry")));
    if(is_empty(text)) {
        strcat(message, _("Shell Command must be non-empty.\n"));
        valid = 0;
    }

    if(!valid) {
        GtkWidget *invalid_dlg = gtk_message_dialog_new (GTK_WINDOW(conf_dlg), GTK_DIALOG_MODAL,
                                                         GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                         _("Invalid Values"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (invalid_dlg),
                                                  "%s", message);
        gtk_window_set_transient_for(GTK_WINDOW (invalid_dlg), GTK_WINDOW (conf_dlg));
        gtk_window_set_title (GTK_WINDOW (invalid_dlg), _("Invalid Values"));
        gtk_dialog_run (GTK_DIALOG (invalid_dlg));
        gtk_widget_destroy(invalid_dlg);
    }
    return valid;
}

void
on_edit_ok_button_clicked (GtkButton *button, gpointer user_data) {
    if(!validate_command_edit()) {
        return;
    }
    // Update the main window tree view
    GtkTreeView *treeview = GTK_TREE_VIEW(lookup_widget(conf_dlg, "command_treeview"));
    GtkTreeModel *treemodel = gtk_tree_view_get_model(treeview);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeIter iter;

    if(current_action == NULL) {
        current_action = shellexec_plugin->action_add ();
        actions = (Shx_action_t *)shellexec_plugin->misc.plugin.get_actions(NULL);
        gtk_list_store_append(GTK_LIST_STORE(treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter, COL_META, current_action, -1);
        gtk_tree_selection_select_iter(selection, &iter);
    }
    else {
        gtk_tree_selection_get_selected(selection, &treemodel, &iter);
    }
    // Store all the text fields in the current action
    GtkEntry *entry;
    entry = GTK_ENTRY(lookup_widget(edit_dlg, "name_entry"));
    current_action->parent.name = strdup(gtk_entry_get_text(entry));
    entry = GTK_ENTRY(lookup_widget(edit_dlg, "title_entry"));
    current_action->parent.title = strdup(gtk_entry_get_text(entry));
    entry = GTK_ENTRY(lookup_widget(edit_dlg, "cmd_entry"));
    current_action->shcommand = strdup(gtk_entry_get_text(entry));

    gboolean active;
    int flags = current_action->parent.flags;
    int shx_flags = current_action->shx_flags;
    // Store all the check button values in the current action
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "single_check")));
    flags = (flags & ~DB_ACTION_SINGLE_TRACK) | (active?DB_ACTION_SINGLE_TRACK:0);
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "multiple_check")));
    flags = (flags & ~DB_ACTION_MULTIPLE_TRACKS) | (active?DB_ACTION_MULTIPLE_TRACKS:0);
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "local_check")));
    shx_flags = (shx_flags & ~SHX_ACTION_LOCAL_ONLY) | (active?SHX_ACTION_LOCAL_ONLY:0);
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "remote_check")));
    shx_flags = (shx_flags & ~SHX_ACTION_REMOTE_ONLY) | (active?SHX_ACTION_REMOTE_ONLY:0);
    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(edit_dlg, "common_check")));
    flags = (flags & ~DB_ACTION_COMMON) | (active?DB_ACTION_COMMON:0);

    current_action->parent.flags = flags | DB_ACTION_ADD_MENU;
    current_action->shx_flags = shx_flags;

    gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter,
                       COL_TITLE, current_action->parent.title, -1);

    gtk_widget_destroy(edit_dlg);
    edit_dlg = NULL;
    current_action = NULL;

    shellexec_plugin->save_actions();
    deadbeef->sendmessage (DB_EV_ACTIONSCHANGED, 0, 0, 0);
}

static void
init_treeview() {
    // Create the tree view and cell renderers
    GtkTreeView *treeview = GTK_TREE_VIEW(lookup_widget(conf_dlg, "command_treeview"));
    GtkCellRenderer *cell_renderer;
    cell_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(treeview, -1, _("Title"), cell_renderer,
                                                "text", COL_TITLE, NULL);

    // Create the tree view data model and fill it with values
    GtkListStore *liststore;
    liststore = gtk_list_store_new(COL_COUNT, 
                                   G_TYPE_STRING,
                                   //G_TYPE_BOOLEAN,
                                   G_TYPE_POINTER);
    actions = (Shx_action_t *)shellexec_plugin->misc.plugin.get_actions(NULL);
    Shx_action_t *action = actions;
    GtkTreeIter iter;
    while(action) {
        gtk_list_store_append(liststore, &iter);
        gtk_list_store_set(liststore, &iter,
                           COL_TITLE,    action->parent.title,
                           COL_META,     action, -1);
        action = (Shx_action_t *)action->parent.next;
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(liststore));
    g_object_unref(liststore);
}

void
on_shellexec_conf_dialog_destroy       (GObject       *object,
                                        gpointer         user_data)
{
    conf_dlg = NULL;
}


static gboolean
shellexecui_action_gtk (void *data)
{
    if (conf_dlg) {
        return FALSE;
    }
    conf_dlg = create_shellexec_conf_dialog();
    gtk_widget_set_size_request (conf_dlg, 400, 400);
    gtk_window_set_transient_for(GTK_WINDOW(conf_dlg),
                                 GTK_WINDOW(gtkui_plugin->get_mainwin()));
    init_treeview();
    gtk_widget_show(conf_dlg);
    return FALSE;
}

static int
shellexecui_action_callback(DB_plugin_action_t *action, ddb_action_context_t ctx) {
    g_idle_add (shellexecui_action_gtk, NULL);
    return 0;
}

static DB_plugin_action_t shellexecui_action = {
    .title = "Edit/Configure Custom Shell Commands",
    .name = "shellexec_conf",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = shellexecui_action_callback,
    .next = NULL,
};

static DB_plugin_action_t *
shxui_getactions(DB_playItem_t *it) {
    return &shellexecui_action;
}

int shxui_connect(void) {
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if (!gtkui_plugin) {
        fprintf (stderr, "shellexecui: can't find gtkui plugin\n");
        return -1;
    }
    shellexec_plugin = (Shx_plugin_t *)deadbeef->plug_get_for_id ("shellexec");
    if (!shellexec_plugin) {
        fprintf (stderr, "shellexecui: can't find shellexec plugin\n");
        return -1;
    }
    if(shellexec_plugin->misc.plugin.version_major != 1 ||
       shellexec_plugin->misc.plugin.version_minor < 1) {
        fprintf (stderr, "shellexecui: requires shellexec version 1.1 or higher\n");
        return -1;
    }
    return 0;
}

static DB_misc_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.id = "shellexecui_gtk3",
#else
    .plugin.id = "shellexecui_gtk2",
#endif
    .plugin.name = "Shellexec UI",
    .plugin.descr = "A GTK UI for the Shellexec plugin",
    .plugin.copyright = 
        "ShellExec GUI plugin for DeaDBeeF Player\n"
        "Copyright (C) 2012 Azeem Arshad <kr00r4n@gmail.com>\n"
        "Copyright (C) 2013-2014 Oleksiy Yakovenko\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n",
    .plugin.website = "http://azeemarshad.in",
    .plugin.get_actions = shxui_getactions,
    .plugin.connect = shxui_connect,
};

DB_plugin_t *
#if GTK_CHECK_VERSION(3,0,0)
shellexecui_gtk3_load (DB_functions_t *api) {
#else
shellexecui_gtk2_load (DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
