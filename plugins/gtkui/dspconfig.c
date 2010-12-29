/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "deadbeef.h"
#include "gtkui.h"
#include "pluginconf.h"

static ddb_dsp_context_t *chain;
static GtkWidget *prefwin;

static ddb_dsp_context_t *
dsp_clone (ddb_dsp_context_t *from) {
    ddb_dsp_context_t *dsp = from->plugin->open ();
    char param[2000];
    if (from->plugin->num_params) {
        int n = from->plugin->num_params ();
        for (int i = 0; i < n; i++) {
            from->plugin->get_param (from, i, param, sizeof (param));
            dsp->plugin->set_param (dsp, i, param);
        }
    }
    return dsp;
}

static void
fill_dsp_chain (GtkListStore *mdl) {
    ddb_dsp_context_t *dsp = chain;
    while (dsp) {
        GtkTreeIter iter;
        gtk_list_store_append (mdl, &iter);
        gtk_list_store_set (mdl, &iter, 0, dsp->plugin->plugin.name, -1);
        dsp = dsp->next;
    }
}

void
dsp_setup_init (GtkWidget *_prefwin) {
    prefwin = _prefwin;
    // copy current dsp chain
    ddb_dsp_context_t *streamer_chain = deadbeef->streamer_get_dsp_chain ();

    ddb_dsp_context_t *tail = NULL;
    while (streamer_chain) {
        ddb_dsp_context_t *new = dsp_clone (streamer_chain);
        if (tail) {
            tail->next = new;
            tail = new;
        }
        else {
            chain = tail = new;
        }
        streamer_chain = streamer_chain->next;
    }

    // fill dsp_listview
    GtkWidget *listview = lookup_widget (prefwin, "dsp_listview");


    GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Plugin"), title_cell, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (listview), GTK_TREE_VIEW_COLUMN (col));
    GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (listview), GTK_TREE_MODEL (mdl));

    fill_dsp_chain (mdl);
}

void
dsp_setup_free (void) {
    while (chain) {
        ddb_dsp_context_t *next = chain->next;
        chain->plugin->close (chain);
        chain = next;
    }
    prefwin = NULL;
}

static void
fill_dsp_plugin_list (GtkListStore *mdl) {
    struct DB_dsp_s **dsp = deadbeef->plug_get_dsp_list ();
    int i;
    for (i = 0; dsp[i]; i++) {
        GtkTreeIter iter;
        gtk_list_store_append (mdl, &iter);
        gtk_list_store_set (mdl, &iter, 0, dsp[i]->plugin.name, -1);
    }
}

void
on_dsp_add_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_select_dsp_plugin ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefwin));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Add plugin to DSP chain"));
    
    GtkComboBox *combo;
    // fill encoder presets
    combo = GTK_COMBO_BOX (lookup_widget (dlg, "plugin"));
    GtkListStore *mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    fill_dsp_plugin_list (mdl);
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.last_selected_dsp", 0));

    int r = gtk_dialog_run (GTK_DIALOG (dlg));
    if (r == GTK_RESPONSE_OK) {
        // create new instance of the selected plugin
        int idx = gtk_combo_box_get_active (combo);
        struct DB_dsp_s **dsp = deadbeef->plug_get_dsp_list ();
        int i;
        ddb_dsp_context_t *inst = NULL;
        for (i = 0; dsp[i]; i++) {
            if (i == idx) {
                inst = dsp[i]->open ();
                break;
            }
        }
        if (inst) {
            // append to DSP chain
            ddb_dsp_context_t *tail = chain;
            while (tail && tail->next) {
                tail = tail->next;
            }
            if (tail) {
                tail->next = inst;
            }
            else {
                chain = inst;
            }

            // reinit list of instances
            GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
            GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
            gtk_list_store_clear (mdl);
            fill_dsp_chain (mdl);
        }
        else {
            fprintf (stderr, "prefwin: failed to add DSP plugin to chain\n");
        }
    }
    gtk_widget_destroy (dlg);
}

static int
listview_get_index (GtkWidget *list) {
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (!path || !col) {
        // nothing selected
        return - 1;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);
    return idx;
}

void
on_dsp_remove_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx == -1) {
        return;
    }

    ddb_dsp_context_t *p = chain;
    ddb_dsp_context_t *prev = NULL;
    int i = idx;
    while (p && i--) {
        prev = p;
        p = p->next;
    }
    if (p) {
        if (prev) {
            prev->next = p->next;
        }
        else {
            chain = p->next;
        }
        p->plugin->close (p);
        GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
        gtk_list_store_clear (mdl);
        fill_dsp_chain (mdl);
        GtkTreePath *path = gtk_tree_path_new_from_indices (idx, -1);
        GtkTreeViewColumn *col;
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
        gtk_tree_path_free (path);
    }
}

static ddb_dsp_context_t *current_dsp_context = NULL;

void
dsp_ctx_set_param (const char *key, const char *value) {
    current_dsp_context->plugin->set_param (current_dsp_context, atoi (key), value);
}

void
dsp_ctx_get_param (const char *key, char *value, int len, const char *def) {
    strncpy (value, def, len);
    current_dsp_context->plugin->get_param (current_dsp_context, atoi (key), value, len);
}

void
on_dsp_configure_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (!path || !col) {
        // nothing selected
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);
    if (idx == -1) {
        return;
    }
    ddb_dsp_context_t *p = chain;
    int i = idx;
    while (p && i--) {
        p = p->next;
    }
    if (!p || !p->plugin->configdialog) {
        return;
    }
    current_dsp_context = p;
    ddb_dialog_t conf = {
        .title = p->plugin->plugin.name,
        .layout = p->plugin->configdialog,
        .set_param = dsp_ctx_set_param,
        .get_param = dsp_ctx_get_param,
    };
    gtkui_run_dialog (prefwin, &conf, 0);
    current_dsp_context = NULL;
}

static int
swap_items (GtkWidget *list, int idx) {
    ddb_dsp_context_t *prev = NULL;
    ddb_dsp_context_t *p = chain;

    int n = idx;
    while (n > 0 && p) {
        prev = p;
        p = p->next;
        n--;
    }

    if (!p || !p->next) {
        return -1;
    }

    ddb_dsp_context_t *moved = p->next;

    if (!moved) {
        return -1;
    }

    ddb_dsp_context_t *last = moved ? moved->next : NULL;

    if (prev) {
        p->next = last;
        prev->next = moved;
        moved->next = p;
    }
    else {
        p->next = last;
        chain = moved;
        moved->next = p;
    }
    GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
    gtk_list_store_clear (mdl);
    fill_dsp_chain (mdl);
    return 0;
}


void
on_dsp_up_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx <= 0) {
        return;
    }

    if (-1 == swap_items (list, idx-1)) {
        return;
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices (idx-1, -1);
    GtkTreeViewColumn *col;
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
    gtk_tree_path_free (path);
}


void
on_dsp_down_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *list = lookup_widget (prefwin, "dsp_listview");
    int idx = listview_get_index (list);
    if (idx == -1) {
        return;
    }

    if (-1 == swap_items (list, idx)) {
        return;
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices (idx+1, -1);
    GtkTreeViewColumn *col;
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
    gtk_tree_path_free (path);
}

