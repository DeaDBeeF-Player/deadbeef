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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include "converter.h"
#include "support.h"
#include "interface.h"
//#include "gtkui.h"
//#include "pluginconf.h"
#include "../gtkui/gtkui_api.h"

#pragma GCC optimize("O0")

DB_functions_t *deadbeef;

ddb_converter_t *converter_plugin;
ddb_gtkui_t *gtkui_plugin;

typedef struct {
    GtkWidget *converter;
    ddb_encoder_preset_t *current_encoder_preset;
    ddb_dsp_preset_t *current_dsp_preset;

    DB_playItem_t **convert_items;
    int convert_items_count;
    char *outfolder;
    int selected_format;
    ddb_encoder_preset_t *encoder_preset;
    ddb_dsp_preset_t *dsp_preset;
    GtkWidget *progress;
    GtkWidget *progress_entry;
    int cancelled;
    char *progress_text;
} converter_ctx_t;

converter_ctx_t *current_ctx;

void
fill_presets (GtkListStore *mdl, ddb_preset_t *head) {
    ddb_preset_t *p = head;
    while (p) {
        GtkTreeIter iter;
        gtk_list_store_append (mdl, &iter);
        gtk_list_store_set (mdl, &iter, 0, p->title, -1);
        p = p->next;
    }
}

void
on_converter_progress_cancel (GtkDialog *dialog, gint response_id, gpointer user_data) {
    converter_ctx_t *ctx = user_data;
    ctx->cancelled = 1;
}

typedef struct {
    GtkWidget *entry;
    char *text;
} update_progress_info_t;

static gboolean
update_progress_cb (gpointer ctx) {
    update_progress_info_t *info = ctx;
    gtk_entry_set_text (GTK_ENTRY (info->entry), info->text);
    free (info->text);
    g_object_unref (info->entry);
    free (info);
    return FALSE;
}

static gboolean
destroy_progress_cb (gpointer ctx) {
    gtk_widget_destroy (ctx);
    return FALSE;
}

static void
converter_worker (void *ctx) {
    converter_ctx_t *conv = ctx;

    for (int n = 0; n < conv->convert_items_count; n++) {
        update_progress_info_t *info = malloc (sizeof (update_progress_info_t));
        info->entry = conv->progress_entry;
        g_object_ref (info->entry);
        info->text = strdup (conv->convert_items[n]->fname);
        g_idle_add (update_progress_cb, info);

        converter_plugin->convert (conv->convert_items[n], conv->outfolder, conv->selected_format, conv->encoder_preset, conv->dsp_preset, &conv->cancelled);
        if (conv->cancelled) {
            for (; n < conv->convert_items_count; n++) {
                deadbeef->pl_item_unref (conv->convert_items[n]);
            }
            break;
        }
        deadbeef->pl_item_unref (conv->convert_items[n]);
    }
    g_idle_add (destroy_progress_cb, conv->progress);
    if (conv->convert_items) {
        free (conv->convert_items);
    }
    free (conv->outfolder);
    converter_plugin->encoder_preset_free (conv->encoder_preset);
    converter_plugin->dsp_preset_free (conv->dsp_preset);
    free (conv);
}

void
converter_process (converter_ctx_t *conv)
{
    conv->outfolder = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (conv->converter, "output_folder"))));
    deadbeef->conf_set_str ("converter.output_folder", conv->outfolder);
    deadbeef->conf_save ();

    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "encoder"));
    int enc_preset = gtk_combo_box_get_active (combo);
    if (enc_preset < 0) {
        fprintf (stderr, "Encoder preset not selected\n");
        return;
    }

    ddb_encoder_preset_t *encoder_preset = converter_plugin->encoder_preset_get_for_idx (enc_preset);
    if (!encoder_preset) {
        return;
    }
    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "dsp_preset"));
    int dsp_idx = gtk_combo_box_get_active (combo) - 1;

    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "output_format"));
//    int selected_format = gtk_combo_box_get_active (combo);

    ddb_dsp_preset_t *dsp_preset = NULL;
    if (dsp_idx >= 0) {
        dsp_preset = converter_plugin->dsp_preset_get_for_idx (dsp_idx);
    }

    if (encoder_preset) {
        conv->encoder_preset = converter_plugin->encoder_preset_alloc ();
        converter_plugin->encoder_preset_copy (conv->encoder_preset, encoder_preset);
    }
    if (dsp_preset) {
        conv->dsp_preset = converter_plugin->dsp_preset_alloc ();
        converter_plugin->dsp_preset_copy (conv->dsp_preset, dsp_preset);
    }

    GtkWidget *progress = gtk_dialog_new_with_buttons (_("Converting..."), GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    GtkWidget *vbox = GTK_DIALOG (progress)->vbox;
    GtkWidget *entry = gtk_entry_new ();
    gtk_widget_set_size_request (entry, 400, -1);
    gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 12);

    g_signal_connect ((gpointer)progress, "response", G_CALLBACK (on_converter_progress_cancel), conv);

    gtk_widget_show (progress);

    conv->progress = progress;
    conv->progress_entry = entry;
    intptr_t tid = deadbeef->thread_start (converter_worker, conv);
    deadbeef->thread_detach (tid);
}

static int
converter_show (DB_plugin_action_t *act, DB_playItem_t *it) {
    if (!converter_plugin) {
        converter_plugin = (ddb_converter_t *)deadbeef->plug_get_for_id ("converter");
        if (!converter_plugin) {
            return -1;
        }
    }
    if (!gtkui_plugin) {
        gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id ("gtkui");
        if (!gtkui_plugin) {
            return -1;
        }
    }

    converter_ctx_t *conv = malloc (sizeof (converter_ctx_t));
    current_ctx = conv;
    memset (conv, 0, sizeof (converter_ctx_t));

    deadbeef->pl_lock ();
    // copy list
    int nsel = deadbeef->pl_getselcount ();
    conv->convert_items_count = nsel;
    if (0 < nsel) {
        conv->convert_items = malloc (sizeof (DB_playItem_t *) * nsel);
        if (conv->convert_items) {
            int n = 0;
            DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
            while (it) {
                if (deadbeef->pl_is_selected (it)) {
                    assert (n < nsel);
                    deadbeef->pl_item_ref (it);
                    conv->convert_items[n++] = it;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
            }
        }
    }
    deadbeef->pl_unlock ();

    conv->converter = create_converterdlg ();
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (conv->converter, "output_folder")), deadbeef->conf_get_str ("converter.output_folder", ""));

    GtkComboBox *combo;
    // fill encoder presets
    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "encoder"));
    GtkListStore *mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->encoder_preset_get_list ());
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.encoder_preset", 0));

    // fill dsp presets
    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "dsp_preset"));
    mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    GtkTreeIter iter;
    gtk_list_store_append (mdl, &iter);
    gtk_list_store_set (mdl, &iter, 0, "Pass through", -1);
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->dsp_preset_get_list ());

    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.dsp_preset", -1) + 1);

    // select output format
    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "output_format"));
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.output_format", 0));


    int response = gtk_dialog_run (GTK_DIALOG (conv->converter));
    current_ctx = NULL;
    if (response == GTK_RESPONSE_OK) {
        converter_process (conv);
        gtk_widget_destroy (conv->converter);
    }
    else {
        // FIXME: clean up properly
        gtk_widget_destroy (conv->converter);
        if (conv->convert_items) {
            for (int n = 0; n < conv->convert_items_count; n++) {
                deadbeef->pl_item_unref (conv->convert_items[n]);
            }
            free (conv->convert_items);
        }
        free (conv);
    }
    return 0;
}

void
on_converter_encoder_changed           (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "encoder"));
    int act = gtk_combo_box_get_active (combo);
    deadbeef->conf_set_int ("converter.encoder_preset", act);
}

void
on_converter_dsp_preset_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "dsp_preset"));
    int act = gtk_combo_box_get_active (combo);
    deadbeef->conf_set_int ("converter.dsp_preset", act-1);
}

void
on_converter_output_browse_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Select folder..."), GTK_WINDOW (current_ctx->converter), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (current_ctx->converter));

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), FALSE);
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
        deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    }
    if (response == GTK_RESPONSE_OK) {
        folder = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (folder) {
            GtkWidget *entry = lookup_widget (current_ctx->converter, "output_folder");
            gtk_entry_set_text (GTK_ENTRY (entry), folder);
            g_free (folder);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

DB_decoder_t *
plug_get_decoder_for_id (const char *id) {
    DB_decoder_t **plugins = deadbeef->plug_get_decoder_list ();
    for (int c = 0; plugins[c]; c++) {
        if (!strcmp (id, plugins[c]->plugin.id)) {
            return plugins[c];
        }
    }
    return NULL;
}

void
init_encoder_preset_from_dlg (GtkWidget *dlg, ddb_encoder_preset_t *p) {
    p->title = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title"))));
    p->fname = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "fname"))));
    p->encoder = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "encoder"))));
    int method_idx = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "method")));
    switch (method_idx) {
    case 0:
        p->method = DDB_ENCODER_METHOD_PIPE;
        break;
    case 1:
        p->method = DDB_ENCODER_METHOD_FILE;
        break;
    }

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_8bit")))) {
        p->formats |= DDB_ENCODER_FMT_8BIT;
    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_16bit")))) {
        p->formats |= DDB_ENCODER_FMT_16BIT;
    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_24bit")))) {
        p->formats |= DDB_ENCODER_FMT_24BIT;
    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_32bit")))) {
        p->formats |= DDB_ENCODER_FMT_32BIT;
    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_32bitfloat")))) {
        p->formats |= DDB_ENCODER_FMT_32BITFLOAT;
    }
}

int
edit_encoder_preset (char *title, GtkWidget *toplevel, int overwrite) {
    GtkWidget *dlg = create_convpreset_editor ();
    gtk_window_set_title (GTK_WINDOW (dlg), title);
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));

    ddb_encoder_preset_t *p = current_ctx->current_encoder_preset;

    if (p->title) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), p->title);
    }
    if (p->fname) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "fname")), p->fname);
    }
    if (p->encoder) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "encoder")), p->encoder);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "method")), p->method);
    if (p->formats & DDB_ENCODER_FMT_8BIT) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_8bit")), 1);
    }
    if (p->formats & DDB_ENCODER_FMT_16BIT) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_16bit")), 1);
    }
    if (p->formats & DDB_ENCODER_FMT_24BIT) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_24bit")), 1);
    }
    if (p->formats & DDB_ENCODER_FMT_32BIT) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_32bit")), 1);
    }
    if (p->formats & DDB_ENCODER_FMT_32BITFLOAT) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_32bitfloat")), 1);
    }

    ddb_encoder_preset_t *old = p;
    int r = GTK_RESPONSE_CANCEL;
    for (;;) {
        r = gtk_dialog_run (GTK_DIALOG (dlg));
        if (r == GTK_RESPONSE_OK) {
            ddb_encoder_preset_t *p = converter_plugin->encoder_preset_alloc ();
            if (p) {
                init_encoder_preset_from_dlg (dlg, p);
                int err = converter_plugin->encoder_preset_save (p, overwrite);
                if (!err) {
                    if (old->title && strcmp (p->title, old->title)) {
                        char path[1024];
                        if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", deadbeef->get_config_dir (), old->title) > 0) {
                            unlink (path);
                        }
                    }
                    free (old->title);
                    free (old->fname);
                    free (old->encoder);
                    old->title = p->title;
                    old->fname = p->fname;
                    old->encoder = p->encoder;
                    old->method = p->method;
                    old->formats = p->formats;
                    free (p);
                }
                else {
                    GtkWidget *warndlg = gtk_message_dialog_new (GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Failed to save encoder preset"));
                    gtk_window_set_transient_for (GTK_WINDOW (warndlg), GTK_WINDOW (dlg));
                    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (warndlg), err == -1 ? _("Check preset folder permissions, try to pick different title, or free up some disk space") : _("Preset with the same name already exists. Try to pick another title."));
                    gtk_window_set_title (GTK_WINDOW (warndlg), _("Error"));

                    /*int response = */gtk_dialog_run (GTK_DIALOG (warndlg));
                    gtk_widget_destroy (warndlg);
                    continue;
                }
            }
        }
        break;
    }
    
    gtk_widget_destroy (dlg);
    return r;
}

void
refresh_encoder_lists (GtkComboBox *combo, GtkTreeView *list) {
    // presets list view
    GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list)));

    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (path && col) {
        int *indices = gtk_tree_path_get_indices (path);
        int idx = *indices;
        g_free (indices);

        gtk_list_store_clear (mdl);
        fill_presets (mdl, (ddb_preset_t *)converter_plugin->encoder_preset_get_list ());
        path = gtk_tree_path_new_from_indices (idx, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
        gtk_tree_path_free (path);
    }

    // presets combo box
    int act = gtk_combo_box_get_active (combo);
    mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    gtk_list_store_clear (mdl);
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->encoder_preset_get_list ());
    gtk_combo_box_set_active (combo, act);
}

void
on_encoder_preset_add                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));

    current_ctx->current_encoder_preset = converter_plugin->encoder_preset_alloc ();
    if (GTK_RESPONSE_OK == edit_encoder_preset (_("Add new encoder"), toplevel, 0)) {
        converter_plugin->encoder_preset_append (current_ctx->current_encoder_preset);
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "encoder"));
        GtkWidget *list = lookup_widget (toplevel, "presets");
        refresh_encoder_lists (combo, GTK_TREE_VIEW (list));
    }

    current_ctx->current_encoder_preset = NULL;
}

void
on_encoder_preset_edit                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    GtkWidget *list = lookup_widget (toplevel, "presets");
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

    ddb_encoder_preset_t *p = converter_plugin->encoder_preset_get_for_idx (idx);
    current_ctx->current_encoder_preset = p;

    int r = edit_encoder_preset (_("Edit encoder"), toplevel, 1);
    if (r == GTK_RESPONSE_OK) {
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "encoder"));
        refresh_encoder_lists (combo, GTK_TREE_VIEW (list));
    }

    current_ctx->current_encoder_preset = NULL;
}

void
on_encoder_preset_remove                     (GtkButton       *button,
                                        gpointer         user_data)
{

    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    GtkWidget *list = lookup_widget (toplevel, "presets");
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

    ddb_encoder_preset_t *p = converter_plugin->encoder_preset_get_for_idx (idx);
    if (!p) {
        return;
    }

    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Remove preset"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("This action will delete the selected preset. Are you sure?"));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    if (response == GTK_RESPONSE_YES) {
        char path[1024];
        if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", deadbeef->get_config_dir (), p->title) > 0) {
            unlink (path);
        }

        converter_plugin->encoder_preset_remove (p);
        converter_plugin->encoder_preset_free (p);

        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "encoder"));
        refresh_encoder_lists (combo, GTK_TREE_VIEW (list));
    }
}

void
on_edit_encoder_presets_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_preset_list ();
    gtk_window_set_title (GTK_WINDOW (dlg), _("Encoders"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (current_ctx->converter));
    g_signal_connect ((gpointer)lookup_widget (dlg, "add"), "clicked", G_CALLBACK (on_encoder_preset_add), NULL);
    g_signal_connect ((gpointer)lookup_widget (dlg, "remove"), "clicked", G_CALLBACK (on_encoder_preset_remove), NULL);
    g_signal_connect ((gpointer)lookup_widget (dlg, "edit"), "clicked", G_CALLBACK (on_encoder_preset_edit), NULL);

    GtkWidget *list = lookup_widget (dlg, "presets");
    GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Title"), title_cell, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (list), GTK_TREE_VIEW_COLUMN (col));
    GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (mdl));
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->encoder_preset_get_list ());
    int curr = deadbeef->conf_get_int ("converter.encoder_preset", 0);
    GtkTreePath *path = gtk_tree_path_new_from_indices (curr, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
    gtk_tree_path_free (path);
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
}

///// dsp preset gui

void
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
fill_dsp_preset_chain (GtkListStore *mdl) {
    ddb_dsp_context_t *dsp = current_ctx->current_dsp_preset->chain;
    while (dsp) {
        GtkTreeIter iter;
        gtk_list_store_append (mdl, &iter);
        gtk_list_store_set (mdl, &iter, 0, dsp->plugin->plugin.name, -1);
        dsp = dsp->next;
    }
}

void
on_dsp_preset_add_plugin_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_select_dsp_plugin ();
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
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
            ddb_dsp_context_t *tail = current_ctx->current_dsp_preset->chain;
            while (tail && tail->next) {
                tail = tail->next;
            }
            if (tail) {
                tail->next = inst;
            }
            else {
                current_ctx->current_dsp_preset->chain = inst;
            }

            // reinit list of instances
            GtkWidget *list = lookup_widget (toplevel, "plugins");
            GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
            gtk_list_store_clear (mdl);
            fill_dsp_preset_chain (mdl);
        }
        else {
            fprintf (stderr, "converter: failed to add DSP plugin to chain\n");
        }
    }
    gtk_widget_destroy (dlg);
}


void
on_dsp_preset_remove_plugin_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    GtkWidget *list = lookup_widget (toplevel, "plugins");
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

    ddb_dsp_context_t *p = current_ctx->current_dsp_preset->chain;
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
            current_ctx->current_dsp_preset->chain = p->next;
        }
        p->plugin->close (p);
        GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
        gtk_list_store_clear (mdl);
        fill_dsp_preset_chain (mdl);
        path = gtk_tree_path_new_from_indices (idx, -1);
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
on_dsp_preset_plugin_configure_clicked (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    GtkWidget *list = lookup_widget (toplevel, "plugins");
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
    ddb_dsp_context_t *p = current_ctx->current_dsp_preset->chain;
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
    gtkui_plugin->gui.run_dialog (&conf, 0);
    current_dsp_context = NULL;
}

void
on_dsp_preset_plugin_up_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_dsp_preset_plugin_down_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{

}


int
edit_dsp_preset (const char *title, GtkWidget *toplevel, int overwrite) {
    int r = GTK_RESPONSE_CANCEL;

    GtkWidget *dlg = create_dsppreset_editor ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
    gtk_window_set_title (GTK_WINDOW (dlg), title);


    // title
    if (current_ctx->current_dsp_preset->title) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), current_ctx->current_dsp_preset->title);
    }

    {
        GtkWidget *list = lookup_widget (dlg, "plugins");
        GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
        GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Plugin"), title_cell, "text", 0, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (list), GTK_TREE_VIEW_COLUMN (col));
        GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (mdl));

        fill_dsp_preset_chain (mdl);
    }

    for (;;) {
        r = gtk_dialog_run (GTK_DIALOG (dlg));

        if (r == GTK_RESPONSE_OK) {
            if (current_ctx->current_dsp_preset->title) {
                free (current_ctx->current_dsp_preset->title);
            }
            current_ctx->current_dsp_preset->title = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title"))));
            int err = converter_plugin->dsp_preset_save (current_ctx->current_dsp_preset, overwrite);
            if (err < 0) {
                GtkWidget *warndlg = gtk_message_dialog_new (GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Failed to save DSP preset"));
                gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (warndlg), err == -1 ? _("Check preset folder permissions, try to pick different title, or free up some disk space") : _("Preset with the same name already exists. Try to pick another title."));
                gtk_window_set_title (GTK_WINDOW (warndlg), _("Error"));

                gtk_window_set_transient_for (GTK_WINDOW (warndlg), GTK_WINDOW (dlg));
                /*int response = */gtk_dialog_run (GTK_DIALOG (warndlg));
                gtk_widget_destroy (warndlg);
                continue;
            }

        }

        break;
    }

    gtk_widget_destroy (dlg);
    return r;
}

void
refresh_dsp_lists (GtkComboBox *combo, GtkTreeView *list) {
    // presets list view
    GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list)));

    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (path && col) {
        int *indices = gtk_tree_path_get_indices (path);
        int idx = *indices;
        g_free (indices);

        gtk_list_store_clear (mdl);
        fill_presets (mdl, (ddb_preset_t *)converter_plugin->dsp_preset_get_list ());
        path = gtk_tree_path_new_from_indices (idx, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
        gtk_tree_path_free (path);
    }

    // presets combo box
    int act = gtk_combo_box_get_active (combo);
    mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    gtk_list_store_clear (mdl);
    GtkTreeIter iter;
    gtk_list_store_append (mdl, &iter);
    gtk_list_store_set (mdl, &iter, 0, "Pass through", -1);
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->dsp_preset_get_list ());
    gtk_combo_box_set_active (combo, act);
}


void
on_dsp_preset_add                     (GtkButton       *button,
                                        gpointer         user_data)
{

    current_ctx->current_dsp_preset = converter_plugin->dsp_preset_alloc ();
    
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));

    if (GTK_RESPONSE_OK == edit_dsp_preset (_("New DSP Preset"), toplevel, 0)) {
        converter_plugin->dsp_preset_append (current_ctx->current_dsp_preset);
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "dsp_preset"));
        GtkWidget *list = lookup_widget (toplevel, "presets");
        refresh_dsp_lists (combo, GTK_TREE_VIEW (list));
    }
    else {
        converter_plugin->dsp_preset_free (current_ctx->current_dsp_preset);
    }

    current_ctx->current_dsp_preset = NULL;
}

void
on_dsp_preset_remove                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    GtkWidget *list = lookup_widget (toplevel, "presets");
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

    if (idx == 0) {
        return;
    }

    ddb_dsp_preset_t *p = converter_plugin->dsp_preset_get_for_idx (idx);
    if (!p) {
        return;
    }

    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Remove preset"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("This action will delete the selected preset. Are you sure?"));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    if (response == GTK_RESPONSE_YES) {
        char path[1024];
        if (snprintf (path, sizeof (path), "%s/presets/dsp/%s.txt", deadbeef->get_config_dir (), p->title) > 0) {
            unlink (path);
        }

        converter_plugin->dsp_preset_remove (p);
        converter_plugin->dsp_preset_free (p);

        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "dsp_preset"));
        refresh_dsp_lists (combo, GTK_TREE_VIEW (list));
    }
}

void
on_dsp_preset_edit                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));

    GtkWidget *list = lookup_widget (toplevel, "presets");
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
    if (idx == 0) {
        return;
    }

    ddb_dsp_preset_t *p = converter_plugin->dsp_preset_get_for_idx (idx);
    if (!p) {
        return;
    }

    current_ctx->current_dsp_preset = converter_plugin->dsp_preset_alloc ();
    converter_plugin->dsp_preset_copy (current_ctx->current_dsp_preset, p);

    int r = edit_dsp_preset (_("Edit DSP Preset"), toplevel, 1);
    if (r == GTK_RESPONSE_OK) {
        // replace preset
        converter_plugin->dsp_preset_replace (p, current_ctx->current_dsp_preset);
        converter_plugin->dsp_preset_free (p);
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "dsp_preset"));
        refresh_dsp_lists (combo, GTK_TREE_VIEW (list));
    }
    else {
        converter_plugin->dsp_preset_free (current_ctx->current_dsp_preset);
    }

    current_ctx->current_dsp_preset = NULL;
}

void
on_edit_dsp_presets_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_preset_list ();
    gtk_window_set_title (GTK_WINDOW (dlg), _("DSP Presets"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (current_ctx->converter));
    g_signal_connect ((gpointer)lookup_widget (dlg, "add"), "clicked", G_CALLBACK (on_dsp_preset_add), NULL);
    g_signal_connect ((gpointer)lookup_widget (dlg, "remove"), "clicked", G_CALLBACK (on_dsp_preset_remove), NULL);
    g_signal_connect ((gpointer)lookup_widget (dlg, "edit"), "clicked", G_CALLBACK (on_dsp_preset_edit), NULL);

    GtkWidget *list = lookup_widget (dlg, "presets");
    GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Title"), title_cell, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (list), GTK_TREE_VIEW_COLUMN (col));
    GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (mdl));
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->dsp_preset_get_list ());
    int curr = deadbeef->conf_get_int ("converter.dsp_preset", -1);
    if (curr >= 0) {
        GtkTreePath *path = gtk_tree_path_new_from_indices (curr, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
        gtk_tree_path_free (path);
    }
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
}


void
on_converter_output_format_changed     (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int idx = gtk_combo_box_get_active (combobox);
    deadbeef->conf_set_int ("converter.output_format", idx);
}

GtkWidget*
title_formatting_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("http://sourceforge.net/apps/mediawiki/deadbeef/index.php?title=Title_Formatting", "Help");
    return link;
}

GtkWidget*
encoder_cmdline_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("http://sourceforge.net/apps/mediawiki/deadbeef/index.php?title=Encoder_Command_Line", "Help");
    return link;
}

static DB_plugin_action_t convert_action = {
    .title = "Convert...",
    .name = "convert",
    .flags = DB_ACTION_CAN_MULTIPLE_TRACKS | DB_ACTION_ALLOW_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK,
    .callback = converter_show,
    .next = NULL
};

static DB_plugin_action_t *
convgui_get_actions (DB_playItem_t *it)
{
    return &convert_action;
}

DB_misc_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.name = "Converter GTK UI",
    .plugin.descr = "User interface to Converter plugin using GTK2",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.get_actions = convgui_get_actions
};

DB_plugin_t *
converter_gtkui_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

