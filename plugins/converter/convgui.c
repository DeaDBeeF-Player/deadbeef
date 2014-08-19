/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#  include "../../config.h"
#endif
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_SYSLIMITS_H
#include <sys/syslimits.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "converter.h"
#include "support.h"
#include "interface.h"
#include "../gtkui/gtkui_api.h"

DB_functions_t *deadbeef;

ddb_converter_t *converter_plugin;
ddb_gtkui_t *gtkui_plugin;

static int converter_active;

typedef struct {
    GtkWidget *converter;
    ddb_encoder_preset_t *current_encoder_preset;
    ddb_dsp_preset_t *current_dsp_preset;

    DB_playItem_t **convert_items;
    int convert_items_count;
    char *outfolder;
    char *outfile;
    int preserve_folder_structure;
    int write_to_source_folder;
    int output_bps;
    int output_is_float;
    int overwrite_action;
    ddb_encoder_preset_t *encoder_preset;
    ddb_dsp_preset_t *dsp_preset;
    GtkWidget *progress;
    GtkWidget *progress_entry;
    int cancelled;
    char *progress_text;
} converter_ctx_t;

converter_ctx_t *current_ctx;

enum {
    PRESET_TYPE_ENCODER,
    PRESET_TYPE_DSP
};

static void
fill_presets (GtkListStore *mdl, ddb_preset_t *head, int type) {
    ddb_preset_t *p = head;
    while (p) {
        GtkTreeIter iter;
        gtk_list_store_append (mdl, &iter);
        const char *s = p->title;
        if (type == PRESET_TYPE_ENCODER && ((ddb_encoder_preset_t *)p)->readonly) {
            char stock[1000];
            snprintf (stock, sizeof (stock), _("[Built-in] %s"), p->title);
            s = stock;
        }
        gtk_list_store_set (mdl, &iter, 0, s, -1);
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

struct overwrite_prompt_ctx {
    char *fname;
    uintptr_t mutex;
    uintptr_t cond;
    int result;
};

static gboolean
overwrite_prompt_cb (void *ctx) {
    struct overwrite_prompt_ctx *ctl = ctx;
    GtkWidget *mainwin = gtkui_plugin->get_mainwin ();
    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("The file already exists. Overwrite?"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Converter warning"));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), "%s", ctl->fname);

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    ctl->result = response == GTK_RESPONSE_YES ? 1 : 0;
    deadbeef->cond_signal (ctl->cond);
    return FALSE;
}

static void
converter_worker (void *ctx) {
    deadbeef->background_job_increment ();
    converter_ctx_t *conv = ctx;

    char root[2000] = "";
    int rootlen = 0;
    // prepare for preserving folder struct
    if (conv->preserve_folder_structure && conv->convert_items_count >= 1) {
        // start with the 1st track path
        deadbeef->pl_get_meta (conv->convert_items[0], ":URI", root, sizeof (root));
        char *sep = strrchr (root, '/');
        if (sep) {
            *sep = 0;
        }
        // reduce
        rootlen = strlen (root);
        for (int n = 1; n < conv->convert_items_count; n++) {
            deadbeef->pl_lock ();
            const char *path = deadbeef->pl_find_meta (conv->convert_items[n], ":URI");
            if (strncmp (path, root, rootlen)) {
                // find where path splits
                char *r = root;
                while (*path && *r) {
                    if (*path != *r) {
                        // find new separator
                        while (r > root && *r != '/') {
                            r--;
                        }
                        *r = 0;
                        rootlen = r-root;
                        break;
                    }
                    path++;
                    r++;
                }
            }
            deadbeef->pl_unlock ();
        }
        fprintf (stderr, "common root path: %s\n", root);
    }

    for (int n = 0; n < conv->convert_items_count; n++) {
        update_progress_info_t *info = malloc (sizeof (update_progress_info_t));
        info->entry = conv->progress_entry;
        g_object_ref (info->entry);
        deadbeef->pl_lock ();
        info->text = strdup (deadbeef->pl_find_meta (conv->convert_items[n], ":URI"));
        deadbeef->pl_unlock ();
        g_idle_add (update_progress_cb, info);

        char outpath[2000];

        converter_plugin->get_output_path (conv->convert_items[n], conv->outfolder, conv->outfile, conv->encoder_preset, conv->preserve_folder_structure, root, conv->write_to_source_folder, outpath, sizeof (outpath));

        int skip = 0;

        // need to unescape path before passing to stat
        char unesc_path[2000];
        char invalid[] = "$\"`\\";
        const char *p = outpath;
        char *o = unesc_path;
        while (*p) {
            if (*p == '\\') {
                p++;
            }
            *o++ = *p++;
        }
        *o = 0;

        struct stat st;
        int res = stat(unesc_path, &st);
        if (res == 0) {
            if (conv->overwrite_action > 1 || conv->overwrite_action < 0) {
                conv->overwrite_action = 0;
            }
            if (conv->overwrite_action == 0) {
                // prompt if file exists
                struct overwrite_prompt_ctx ctl;
                ctl.mutex = deadbeef->mutex_create ();
                ctl.cond = deadbeef->cond_create ();
                ctl.fname = unesc_path;
                ctl.result = 0;
                gdk_threads_add_idle (overwrite_prompt_cb, &ctl);
                deadbeef->cond_wait (ctl.cond, ctl.mutex);
                deadbeef->cond_free (ctl.cond);
                deadbeef->mutex_free (ctl.mutex);
                if (ctl.result) {
                    unlink (outpath);
                }
                else {
                    skip = 1;
                }
            }
            else if (conv->overwrite_action == 1) {
                unlink (outpath);
            }
        }

        if (!skip) {
            converter_plugin->convert (conv->convert_items[n], outpath, conv->output_bps, conv->output_is_float, conv->encoder_preset, conv->dsp_preset, &conv->cancelled);
        }
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
    deadbeef->background_job_decrement ();
}

int
converter_process (converter_ctx_t *conv)
{
    conv->outfolder = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (conv->converter, "output_folder"))));
    const char *outfile = gtk_entry_get_text (GTK_ENTRY (lookup_widget (conv->converter, "output_file")));
    if (outfile[0] == 0) {
        outfile = "%a - %t";
    }
    conv->outfile = strdup (outfile);
    conv->preserve_folder_structure = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "preserve_folders")));
    conv->write_to_source_folder = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "write_to_source_folder")));
    conv->overwrite_action = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (conv->converter, "overwrite_action")));

    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "output_format"));
    int selected_format = gtk_combo_box_get_active (combo);
    switch (selected_format) {
    case 1 ... 4:
        conv->output_bps = selected_format * 8;
        conv->output_is_float = 0;
        break;
    case 5:
        conv->output_bps = 32;
        conv->output_is_float = 1;
        break;
    default:
        conv->output_bps = -1; // same as input, or encoder default
        break;
    }

    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "encoder"));
    int enc_preset = gtk_combo_box_get_active (combo);
    ddb_encoder_preset_t *encoder_preset = NULL;

    if (enc_preset >= 0) {
        encoder_preset = converter_plugin->encoder_preset_get_for_idx (enc_preset);
    }
    if (!encoder_preset) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (conv->converter), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Please select encoder"));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (conv->converter));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Converter error"));

        gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        return -1;
    }

    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "dsp_preset"));
    int dsp_idx = gtk_combo_box_get_active (combo) - 1;

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
    GtkWidget *vbox = gtk_dialog_get_content_area (GTK_DIALOG (progress));
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
    return 0;
}

void
on_write_to_source_folder_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

static gboolean
converter_show_cb (void *data) {
    int ctx = (intptr_t)data;
    converter_ctx_t *conv = malloc (sizeof (converter_ctx_t));
    current_ctx = conv;
    memset (conv, 0, sizeof (converter_ctx_t));

    deadbeef->pl_lock ();
    switch (ctx) {
    case DDB_ACTION_CTX_MAIN:
    case DDB_ACTION_CTX_SELECTION:
        {
            // copy list
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                conv->convert_items_count = deadbeef->plt_getselcount (plt);
                if (0 < conv->convert_items_count) {
                    conv->convert_items = malloc (sizeof (DB_playItem_t *) * conv->convert_items_count);
                    if (conv->convert_items) {
                        int n = 0;
                        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
                        while (it) {
                            if (deadbeef->pl_is_selected (it)) {
                                assert (n < conv->convert_items_count);
                                deadbeef->pl_item_ref (it);
                                conv->convert_items[n++] = it;
                            }
                            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                            deadbeef->pl_item_unref (it);
                            it = next;
                        }
                    }
                }
            }
            break;
        }
    case DDB_ACTION_CTX_PLAYLIST:
        {
            // copy list
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                conv->convert_items_count = deadbeef->plt_get_item_count (plt, PL_MAIN);
                if (0 < conv->convert_items_count) {
                    conv->convert_items = malloc (sizeof (DB_playItem_t *) * conv->convert_items_count);
                    if (conv->convert_items) {
                        int n = 0;
                        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
                        while (it) {
                            conv->convert_items[n++] = it;
                            it = deadbeef->pl_get_next (it, PL_MAIN);
                        }
                    }
                }
                deadbeef->plt_unref (plt);
            }
            break;
        }
    case DDB_ACTION_CTX_NOWPLAYING:
        {
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
            if (it) {
                conv->convert_items_count = 1;
                conv->convert_items = malloc (sizeof (DB_playItem_t *) * conv->convert_items_count);
                if (conv->convert_items) {
                    conv->convert_items[1] = it;
                }
            }
        }
        break;
    }
    deadbeef->pl_unlock ();

    conv->converter = create_converterdlg ();
    deadbeef->conf_lock ();
    const char *out_folder = deadbeef->conf_get_str_fast ("converter.output_folder", "");
    if (!out_folder[0]) {
        out_folder = getenv("HOME");
    }
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (conv->converter, "output_folder")), out_folder);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (conv->converter, "output_file")), deadbeef->conf_get_str_fast ("converter.output_file", ""));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "preserve_folders")), deadbeef->conf_get_int ("converter.preserve_folder_structure", 0));
    int write_to_source_folder = deadbeef->conf_get_int ("converter.write_to_source_folder", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "write_to_source_folder")), write_to_source_folder);

    g_signal_connect ((gpointer) lookup_widget (conv->converter, "write_to_source_folder"), "toggled",
            G_CALLBACK (on_write_to_source_folder_toggled),
            conv);

    gtk_widget_set_sensitive (lookup_widget (conv->converter, "output_folder"), !write_to_source_folder);
    gtk_widget_set_sensitive (lookup_widget (conv->converter, "preserve_folders"), !write_to_source_folder);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (conv->converter, "overwrite_action")), deadbeef->conf_get_int ("converter.overwrite_action", 0));
    deadbeef->conf_unlock ();

    GtkComboBox *combo;
    // fill encoder presets
    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "encoder"));
    GtkListStore *mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->encoder_preset_get_list (), PRESET_TYPE_ENCODER);
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.encoder_preset", 0));

    // fill dsp presets
    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "dsp_preset"));
    mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    GtkTreeIter iter;
    gtk_list_store_append (mdl, &iter);
    gtk_list_store_set (mdl, &iter, 0, "Pass through", -1);
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->dsp_preset_get_list (), PRESET_TYPE_DSP);

    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.dsp_preset", -1) + 1);

    // select output format
    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "output_format"));
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.output_format", 0));

    // overwrite action
    combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "overwrite_action"));
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.overwrite_action", 0));

    for (;;) {
        int response = gtk_dialog_run (GTK_DIALOG (conv->converter));
        if (response == GTK_RESPONSE_OK) {
            int err = converter_process (conv);
            if (err != 0) {
                continue;
            }
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
        current_ctx = NULL;
        break;
    }
    converter_active = 0;
    return FALSE;
}

static int
converter_show (DB_plugin_action_t *act, int ctx) {
    if (converter_active) {
        return -1;
    }
    converter_active = 1;
    if (converter_plugin->misc.plugin.version_minor >= 1) {
        // reload all presets
        converter_plugin->free_encoder_presets ();
        converter_plugin->load_encoder_presets ();
        converter_plugin->free_dsp_presets ();
        converter_plugin->load_dsp_presets ();
    }
    // this can be called from non-gtk thread
    gdk_threads_add_idle (converter_show_cb, (void *)(intptr_t)ctx);
    return 0;
}

void
on_converter_encoder_changed           (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "encoder"));
    int act = gtk_combo_box_get_active (combo);
    deadbeef->conf_set_int ("converter.encoder_preset", act);
    deadbeef->conf_save ();
}

void
on_converter_dsp_preset_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "dsp_preset"));
    int act = gtk_combo_box_get_active (combo);
    deadbeef->conf_set_int ("converter.dsp_preset", act-1);
    deadbeef->conf_save ();
}

void
on_converter_output_browse_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Select folder..."), GTK_WINDOW (current_ctx->converter), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (current_ctx->converter));

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), FALSE);
    // restore folder
    deadbeef->conf_lock ();
    char dir[2000];
    deadbeef->conf_get_str ("converter.lastdir", "", dir, sizeof (dir));
    if (!dir[0]) {
        const char *out_folder = deadbeef->conf_get_str_fast ("converter.output_folder", "");
        if (!out_folder[0]) {
            out_folder = getenv("HOME");
        }
        snprintf (dir, sizeof (dir), "file://%s", out_folder);
    }

    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), dir);
    deadbeef->conf_unlock ();
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("converter.lastdir", folder);
        g_free (folder);
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


void
on_output_folder_changed               (GtkEntry     *entry,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("converter.output_folder", gtk_entry_get_text (entry));
    deadbeef->conf_save ();
}


void
on_numthreads_changed                  (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("converter.threads", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (editable)));
    deadbeef->conf_save ();
}

void
on_overwrite_action_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("converter.overwrite_action", gtk_combo_box_get_active (combobox));
    deadbeef->conf_save ();
}

void
on_encoder_changed                     (GtkEditable     *editable,
                                        gpointer         user_data)
{
     gtk_widget_set_has_tooltip (GTK_WIDGET (editable), TRUE);

     char enc[2000];
     const char *e = gtk_entry_get_text (GTK_ENTRY (editable));
     char *o = enc;
     *o = 0;
     int len = sizeof (enc);
     while (e && *e) {
         if (len <= 0) {
             break;
         }
         if (e[0] == '%' && e[1]) {
             if (e[1] == 'o') {
                 int l = snprintf (o, len, "\"OUTPUT_FILE_NAME\"");
                 o += l;
                 len -= l;
             }
             else if (e[1] == 'i') {
                 int l = snprintf (o, len, "\"TEMP_FILE_NAME\"");
                 o += l;
                 len -= l;
             }
             else {
                 strncpy (o, e, 2);
                 o += 2;
                 len -= 2;
             }
             e += 2;
         }
         else {
             *o++ = *e++;
             *o = 0;
             len--;
         }
     }

     gtk_widget_set_tooltip_text (GTK_WIDGET (editable), enc);
}

void
on_output_file_changed                 (GtkEntry        *entry,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("converter.output_file", gtk_entry_get_text (entry));
    deadbeef->conf_save ();
}

void
on_preserve_folders_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("converter.preserve_folder_structure", gtk_toggle_button_get_active (togglebutton));
    deadbeef->conf_save ();
}

void
on_write_to_source_folder_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    converter_ctx_t *conv = user_data;
    deadbeef->conf_set_int ("converter.write_to_source_folder", active);
    gtk_widget_set_sensitive (lookup_widget (conv->converter, "output_folder"), !active);
    gtk_widget_set_sensitive (lookup_widget (conv->converter, "preserve_folders"), !active);
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
    p->ext = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "ext"))));
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

    p->id3v2_version = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id3v2_version")));
    p->tag_id3v2 = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "id3v2")));
    p->tag_id3v1 = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "id3v1")));
    p->tag_apev2 = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "apev2")));
    p->tag_flac = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "flac")));
    p->tag_oggvorbis = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "oggvorbis")));
}

int
edit_encoder_preset (char *title, GtkWidget *toplevel) {
    GtkWidget *dlg = create_convpreset_editor ();
    gtk_window_set_title (GTK_WINDOW (dlg), title);
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
    ddb_encoder_preset_t *p = current_ctx->current_encoder_preset;

    if (p->title) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), p->title);
    }
    if (p->ext) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "ext")), p->ext);
    }
    if (p->encoder) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "encoder")), p->encoder);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "method")), p->method);

    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id3v2_version")), p->id3v2_version);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "id3v2")), p->tag_id3v2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "id3v1")), p->tag_id3v1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "apev2")), p->tag_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "flac")), p->tag_flac);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "oggvorbis")), p->tag_oggvorbis);

    ddb_encoder_preset_t *old = p;
    int r = GTK_RESPONSE_CANCEL;
    for (;;) {
        r = gtk_dialog_run (GTK_DIALOG (dlg));
        if (r == GTK_RESPONSE_OK) {
            ddb_encoder_preset_t *p = converter_plugin->encoder_preset_alloc ();
            if (p) {
                init_encoder_preset_from_dlg (dlg, p);
                int err = 0;

                ddb_encoder_preset_t *pp = converter_plugin->encoder_preset_get_list ();
                for (; pp; pp = pp->next) {
                    if (pp != old && !strcmp (pp->title, p->title)) {
                        err = -2;
                        break;
                    }
                }

                if (!err) {
                    err = converter_plugin->encoder_preset_save (p, 1);
                }
                if (!err) {
                    if (old->title && strcmp (p->title, old->title)) {
                        char path[1024];
                        if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", deadbeef->get_config_dir (), old->title) > 0) {
                            unlink (path);
                        }
                    }
                    free (old->title);
                    free (old->ext);
                    free (old->encoder);

                    converter_plugin->encoder_preset_copy (old, p);
                    converter_plugin->encoder_preset_free (p);
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
    int idx = -1;
    if (path && col) {
        int *indices = gtk_tree_path_get_indices (path);
        idx = *indices;
        g_free (indices);
    }

    gtk_list_store_clear (mdl);
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->encoder_preset_get_list (), PRESET_TYPE_ENCODER);
    if (idx != -1) {
        path = gtk_tree_path_new_from_indices (idx, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
        gtk_tree_path_free (path);
    }

    // presets combo box
    int act = gtk_combo_box_get_active (combo);
    mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    gtk_list_store_clear (mdl);
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->encoder_preset_get_list (), PRESET_TYPE_ENCODER);
    gtk_combo_box_set_active (combo, act);
}

void
on_encoder_preset_add                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));

    current_ctx->current_encoder_preset = converter_plugin->encoder_preset_alloc ();

    if (GTK_RESPONSE_OK == edit_encoder_preset (_("Add new encoder"), toplevel)) {
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

    int r = edit_encoder_preset (_("Edit encoder"), toplevel);
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

static void
on_encoder_preset_cursor_changed (GtkTreeView     *treeview,
                                        gpointer         user_data) {
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (treeview));
    GtkWidget *edit = lookup_widget (toplevel, "edit");
    GtkWidget *remove = lookup_widget (toplevel, "remove");

    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // nothing selected
        gtk_widget_set_sensitive (edit, FALSE);
        gtk_widget_set_sensitive (remove, FALSE);
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);

    ddb_encoder_preset_t *p = converter_plugin->encoder_preset_get_for_idx (idx);
    gtk_widget_set_sensitive (edit, !p->readonly);
    gtk_widget_set_sensitive (remove, !p->readonly);
}


void
on_encoder_preset_copy (GtkButton *button, gpointer user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));

    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (toplevel, "presets"));

    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);

    ddb_encoder_preset_t *p = converter_plugin->encoder_preset_get_for_idx (idx);

    current_ctx->current_encoder_preset = converter_plugin->encoder_preset_alloc ();
    if (!current_ctx->current_encoder_preset) {
        return;
    }
    converter_plugin->encoder_preset_copy (current_ctx->current_encoder_preset, p);

    if (GTK_RESPONSE_OK == edit_encoder_preset (_("Add new encoder"), toplevel)) {
        converter_plugin->encoder_preset_append (current_ctx->current_encoder_preset);
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "encoder"));
        refresh_encoder_lists (combo, treeview);
    }

    current_ctx->current_encoder_preset = NULL;
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
    g_signal_connect ((gpointer)lookup_widget (dlg, "copy"), "clicked", G_CALLBACK (on_encoder_preset_copy), NULL);

    GtkWidget *list = lookup_widget (dlg, "presets");
    g_signal_connect ((gpointer)list, "cursor-changed", G_CALLBACK (on_encoder_preset_cursor_changed), NULL);
    GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Title"), title_cell, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (list), GTK_TREE_VIEW_COLUMN (col));
    GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (mdl));
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->encoder_preset_get_list (), PRESET_TYPE_ENCODER);
    int curr = deadbeef->conf_get_int ("converter.encoder_preset", -1);
    if (curr != -1) {
        GtkTreePath *path = gtk_tree_path_new_from_indices (curr, -1);
        if (path && gtk_tree_path_get_depth (path) > 0) {
            gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
            gtk_tree_path_free (path);
        }
    }
    on_encoder_preset_cursor_changed (GTK_TREE_VIEW (list), NULL);
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

static int
listview_get_index (GtkWidget *list) {
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (!path) {
        // nothing selected
        return -1;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);
    return idx;
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
            int idx = listview_get_index (list);
            gtk_list_store_clear (mdl);
            fill_dsp_preset_chain (mdl);
            GtkTreePath *path = gtk_tree_path_new_from_indices (idx, -1);
            gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
            gtk_tree_path_free (path);
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
    int idx = listview_get_index (list);
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
        .parent = toplevel
    };
    gtkui_plugin->gui.run_dialog (&conf, 0, NULL, NULL);
    current_dsp_context = NULL;
}

static int
swap_items (GtkWidget *list, int idx) {
    ddb_dsp_context_t *prev = NULL;
    ddb_dsp_context_t *p = current_ctx->current_dsp_preset->chain;

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
        current_ctx->current_dsp_preset->chain = moved;
        moved->next = p;
    }
    GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW(list)));
    gtk_list_store_clear (mdl);
    fill_dsp_preset_chain (mdl);
    return 0;
}

void
on_dsp_preset_plugin_up_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    GtkWidget *list = lookup_widget (toplevel, "plugins");
    int idx = listview_get_index (list);
    if (idx <= 0) {
        return;
    }

    if (-1 == swap_items (list, idx-1)) {
        return;
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices (idx-1, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
    gtk_tree_path_free (path);
}


void
on_dsp_preset_plugin_down_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    GtkWidget *list = lookup_widget (toplevel, "plugins");
    int idx = listview_get_index (list);
    if (idx == -1) {
        return;
    }

    if (-1 == swap_items (list, idx)) {
        return;
    }
    GtkTreePath *path = gtk_tree_path_new_from_indices (idx+1, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
    gtk_tree_path_free (path);
}


int
edit_dsp_preset (const char *title, GtkWidget *toplevel, ddb_dsp_preset_t *orig) {
    int r = GTK_RESPONSE_CANCEL;

    GtkWidget *dlg = create_dsppreset_editor ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
    gtk_window_set_title (GTK_WINDOW (dlg), title);

    ddb_dsp_preset_t *p = current_ctx->current_dsp_preset;

    // title
    if (p->title) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), p->title);
    }

    {
        GtkWidget *list = lookup_widget (dlg, "plugins");
        GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
        GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Plugin"), title_cell, "text", 0, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (list), GTK_TREE_VIEW_COLUMN (col));
        GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (mdl));

        fill_dsp_preset_chain (mdl);
        GtkTreePath *path = gtk_tree_path_new_from_indices (0, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, NULL, FALSE);
        gtk_tree_path_free (path);
    }

    for (;;) {
        r = gtk_dialog_run (GTK_DIALOG (dlg));

        if (r == GTK_RESPONSE_OK) {
            const char *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
            int err = 0;

            // don't allow duplicate title with existing presets
            ddb_dsp_preset_t *pp = converter_plugin->dsp_preset_get_list ();
            for (; pp; pp = pp->next) {
                if (pp != orig && !strcmp (pp->title, title)) {
                    err = -2;
                    break;
                }
            }

            if (!err) {
                if (current_ctx->current_dsp_preset->title) {
                    free (current_ctx->current_dsp_preset->title);
                }
                current_ctx->current_dsp_preset->title = strdup (title);
                err = converter_plugin->dsp_preset_save (current_ctx->current_dsp_preset, 1);
            }

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
    int idx = -1;

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (path && col) {
        int *indices = gtk_tree_path_get_indices (path);
        idx = *indices;
        g_free (indices);
    }

    gtk_list_store_clear (mdl);
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->dsp_preset_get_list (), PRESET_TYPE_DSP);
    if (idx != -1) {
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
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->dsp_preset_get_list (), PRESET_TYPE_DSP);
    gtk_combo_box_set_active (combo, act);
}


void
on_dsp_preset_add                     (GtkButton       *button,
                                        gpointer         user_data)
{

    current_ctx->current_dsp_preset = converter_plugin->dsp_preset_alloc ();
    
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));

    if (GTK_RESPONSE_OK == edit_dsp_preset (_("New DSP Preset"), toplevel, NULL)) {
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
on_dsp_preset_copy (GtkButton *button, gpointer user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));

    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (toplevel, "presets"));

    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);

    ddb_dsp_preset_t *p = converter_plugin->dsp_preset_get_for_idx (idx);

    current_ctx->current_dsp_preset = converter_plugin->dsp_preset_alloc ();
    if (!current_ctx->current_dsp_preset) {
        return;
    }
    converter_plugin->dsp_preset_copy (current_ctx->current_dsp_preset, p);

    if (GTK_RESPONSE_OK == edit_dsp_preset (_("New DSP Preset"), toplevel, NULL)) {
        converter_plugin->dsp_preset_append (current_ctx->current_dsp_preset);
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "dsp_preset"));
        refresh_dsp_lists (combo, treeview);
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

    ddb_dsp_preset_t *p = converter_plugin->dsp_preset_get_for_idx (idx);
    if (!p) {
        return;
    }

    current_ctx->current_dsp_preset = converter_plugin->dsp_preset_alloc ();
    converter_plugin->dsp_preset_copy (current_ctx->current_dsp_preset, p);

    int r = edit_dsp_preset (_("Edit DSP Preset"), toplevel, p);
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
    g_signal_connect ((gpointer)lookup_widget (dlg, "copy"), "clicked", G_CALLBACK (on_dsp_preset_copy), NULL);

    GtkWidget *list = lookup_widget (dlg, "presets");
    GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Title"), title_cell, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (list), GTK_TREE_VIEW_COLUMN (col));
    GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (mdl));
    fill_presets (mdl, (ddb_preset_t *)converter_plugin->dsp_preset_get_list (), PRESET_TYPE_DSP);
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
    deadbeef->conf_save ();
}

GtkWidget*
title_formatting_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("http://github.com/Alexey-Yakovenko/deadbeef/wiki/Title-formatting", _("Help"));
    return link;
}

GtkWidget*
encoder_cmdline_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("http://github.com/Alexey-Yakovenko/deadbeef/wiki/Encoder-Command-Line", _("Help"));
    return link;
}

static DB_plugin_action_t convert_action = {
    .title = "Convert",
    .name = "convert",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK | DB_ACTION_ADD_MENU,
    .callback2 = converter_show,
    .next = NULL
};

static DB_plugin_action_t *
convgui_get_actions (DB_playItem_t *it)
{
    return &convert_action;
}

int
convgui_connect (void) {
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    converter_plugin = (ddb_converter_t *)deadbeef->plug_get_for_id ("converter");
    if (!gtkui_plugin) {
        fprintf (stderr, "convgui: gtkui plugin not found\n");
        return -1;
    }
    if (!converter_plugin) {
        fprintf (stderr, "convgui: converter plugin not found\n");
        return -1;
    }
    if (!PLUG_TEST_COMPAT(&converter_plugin->misc.plugin, 1, 3)) {
        fprintf (stderr, "convgui: need converter>=1.3, but found %d.%d\n", converter_plugin->misc.plugin.version_major, converter_plugin->misc.plugin.version_minor);
        return -1;
    }
    return 0;
}

DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 5,
    .plugin.version_major = 1,
    .plugin.version_minor = 2,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.name = "Converter GTK3 UI",
#else
    .plugin.name = "Converter GTK2 UI",
#endif
    .plugin.descr = "GTK User interface for the Converter plugin\n"
        "Usage:\n"
        "· select some tracks in playlist\n"
        "· right click\n"
        "· select «Convert»\n",
    .plugin.copyright = 
        "Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.get_actions = convgui_get_actions,
    .plugin.connect = convgui_connect,
};

DB_plugin_t *
#if GTK_CHECK_VERSION(3,0,0)
converter_gtk3_load (DB_functions_t *api) {
#else
converter_gtk2_load (DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

