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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include "../../gettext.h"
#include "gtkui.h"
#include "support.h"
#include "interface.h"
#include "callbacks.h"
#include "../../shared/parser.h"
#include "ctmapping.h"

static GtkWidget *ctmapping_dlg;
static GtkWidget *prefwin;

static void
ctmapping_fill (GtkWidget *dlg) {
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (dlg, "ctmappinglist"));
    GtkTreeModel *mdl = gtk_tree_view_get_model (tree);
    gtk_list_store_clear (GTK_LIST_STORE (mdl));

    char mapstr[2048];
    deadbeef->conf_get_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING, mapstr, sizeof (mapstr));

    const char *p = mapstr;
    char t[MAX_TOKEN];
    char ct[MAX_TOKEN];
    char plugins[MAX_TOKEN*5];
    for (;;) {
        p = gettoken (p, t);

        if (!p) {
            break;
        }
        strcpy (ct, t);

        p = gettoken (p, t);
        if (!p || strcmp (t, "{")) {
            break;
        }

        plugins[0] = 0;
        for (;;) {
            p = gettoken (p, t);
            if (!p || !strcmp (t, "}")) {
                break;
            }

            if (plugins[0] != 0) {
                strcat (plugins, " ");
            }
            strcat (plugins, t);
        }

        GtkTreeIter it;
        gtk_list_store_append (GTK_LIST_STORE (mdl), &it);
        gtk_list_store_set (GTK_LIST_STORE (mdl), &it, 0, ct, 1, plugins, -1);
    }
}

static void
ctmapping_apply (void) {
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (ctmapping_dlg, "ctmappinglist"));
    GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (tree));

    char mapstr[2048] = "";
    int s = sizeof (mapstr);
    char *p = mapstr;

    GtkTreeIter iter;
    gboolean res = gtk_tree_model_get_iter_first (model, &iter);
    while (res) {
        GValue key = {0,};
        gtk_tree_model_get_value (model, &iter, 0, &key);
        const char *skey = g_value_get_string (&key);
        GValue val = {0,};
        gtk_tree_model_get_value (model, &iter, 1, &val);
        const char *sval = g_value_get_string (&val);

        int l = snprintf (p, s, "%s {%s} ", skey, sval);
        p += l;
        s -= l;

        res = gtk_tree_model_iter_next (model, &iter);
        if (s <= 0) {
            break;
        }
    }
    deadbeef->conf_set_str ("network.ctmapping", mapstr);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_edit_content_type_mapping_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_ctmappingdlg ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefwin));
    ctmapping_dlg = dlg;

    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (dlg, "ctmappinglist"));
    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
    GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Content-Type"), rend_text, "text", 0, NULL);
    gtk_tree_view_append_column (tree, col);
    col = gtk_tree_view_column_new_with_attributes (_("Plugins"), rend_text, "text", 1, NULL);
    gtk_tree_view_append_column (tree, col);


    gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));

    ctmapping_fill (dlg);

    for (;;) {
        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        if (response == GTK_RESPONSE_OK) {
            ctmapping_apply ();
        }
        else if (response == GTK_RESPONSE_APPLY) {
            ctmapping_apply ();
            continue;
        }
        break;
    }
    gtk_widget_destroy (dlg);
    ctmapping_dlg = NULL;
}

static int
validate_ct (const char *s) {
    if (*s == 0) {
        return 1;
    }
    while (*s) {
        if (*s != '/' && !isalnum (*s) && *s != '-') {
            return 1;
        }
        s++;
    }
    return 0;
}

static int
validate_plugid (const char *s) {
    if (*s == 0) {
        return 1;
    }
    while (*s) {
        if (!isalnum (*s) && *s != ' ') {
            return 1;
        }
        s++;
    }
    return 0;
}

void
on_ctmapping_add_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_ctmappingeditdlg ();
    for (;;) {
        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        if (response == GTK_RESPONSE_OK) {
            GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (ctmapping_dlg, "ctmappinglist"));
            GtkWidget *ct = lookup_widget (dlg, "content_type");
            GtkWidget *plugins = lookup_widget (dlg, "plugins");

            const char *ct_text = gtk_entry_get_text (GTK_ENTRY (ct));
            const char *plugins_text = gtk_entry_get_text (GTK_ENTRY (plugins));
            // validate for non-empty without spaces, only [a-z0-9], - and / allowed in ct,
            // only [a-z0-9] and spaces allowed in plugins
            if (validate_ct (ct_text) || validate_plugid (plugins_text)) {
                GtkWidget *dlg_err = gtk_message_dialog_new (GTK_WINDOW (dlg), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, _("Invalid value(s)."));
                gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg_err), _("Content-type and Plugins fields must be non-empty, and comply with the rules.\nExample content-type: 'audio/mpeg'.\nExample plugin ids: 'stdmpg ffmpeg'.\nSpaces must be used as separators in plugin ids list.\nContent type should be only letters, numbers and '-' sign.\nPlugin id can contain only letters and numbers."));
                gtk_window_set_transient_for (GTK_WINDOW (dlg_err), GTK_WINDOW (dlg));
                gtk_window_set_title (GTK_WINDOW (dlg_err), _("Error"));
                gtk_dialog_run (GTK_DIALOG (dlg_err));
                gtk_widget_destroy (dlg_err);
                continue;
            }

            GtkTreeModel *store = gtk_tree_view_get_model (treeview);
            GtkTreeIter iter;
            gtk_list_store_append (GTK_LIST_STORE (store), &iter);
            gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, gtk_entry_get_text (GTK_ENTRY (ct)), 1, gtk_entry_get_text (GTK_ENTRY (plugins)), -1);
        }
        break;
    }
    gtk_widget_destroy (dlg);
}


void
on_ctmapping_remove_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (ctmapping_dlg, "ctmappinglist"));
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (ctmapping_dlg), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, _("Nothing is selected."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (ctmapping_dlg));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Error"));
        gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        return;
    }

//    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (ctmapping_dlg), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Really delete the selected content type?"));
//    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (ctmapping_dlg));
//    gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));
//    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
//
//    if (response == GTK_RESPONSE_YES) {
        GtkTreeModel *store = gtk_tree_view_get_model (treeview);
        GtkTreeIter iter;
        gtk_tree_model_get_iter (store, &iter, path);
        gtk_list_store_remove (GTK_LIST_STORE (store), &iter);
//    }
//    gtk_widget_destroy (dlg);
}


void
on_ctmapping_edit_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (ctmapping_dlg, "ctmappinglist"));
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (ctmapping_dlg), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, _("Nothing is selected."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (ctmapping_dlg));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Error"));
        gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        return;
    }

    GtkWidget *dlg = create_ctmappingeditdlg ();

    GtkTreeModel *store = gtk_tree_view_get_model (treeview);
    GtkTreeIter iter;
    gtk_tree_model_get_iter (store, &iter, path);
    GValue value = {0,};
    gtk_tree_model_get_value (store, &iter, 0, &value);
    const char *svalue = g_value_get_string (&value);
    GtkWidget *ct = lookup_widget (dlg, "content_type");
    gtk_entry_set_text (GTK_ENTRY (ct), svalue);
    GValue value2 = {0,};
    gtk_tree_model_get_value (store, &iter, 1, &value2);
    svalue = g_value_get_string (&value2);
    GtkWidget *plugins = lookup_widget (dlg, "plugins");
    gtk_entry_set_text (GTK_ENTRY (plugins), svalue);

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, gtk_entry_get_text (GTK_ENTRY (ct)), 1, gtk_entry_get_text (GTK_ENTRY (plugins)), -1);
    }
    gtk_widget_destroy (dlg);
}


void
on_ctmapping_reset_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING);
    ctmapping_fill (ctmapping_dlg);
}

void
ctmapping_setup_init (GtkWidget *_prefwin) {
    prefwin = _prefwin;
}

void
ctmapping_setup_free (void) {
    if (ctmapping_dlg) {
        gtk_widget_destroy (ctmapping_dlg);
    }
    ctmapping_dlg = NULL;
}
