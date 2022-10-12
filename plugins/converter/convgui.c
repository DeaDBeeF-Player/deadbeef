/*
    Converter UI for DeaDBeeF Player
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
#  include "../../config.h"
#endif
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include "../../deadbeef.h"
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
    ddb_playlist_t *convert_playlist;

    int convert_items_count;
    char *outfolder;
    char *outfile;
    int preserve_folder_structure;
    int write_to_source_folder;
    int bypass_same_format;
    int retag_after_copy;
    int output_bps;
    int output_is_float;
    int overwrite_action;
    ddb_encoder_preset_t *encoder_preset;
    ddb_dsp_preset_t *dsp_preset;
    GtkWidget *progress_dialog;
    GtkTextView *text_view;
    int cancelled;
} converter_ctx_t;

converter_ctx_t *current_ctx;

ddb_converter_settings_t get_converter_settings (converter_ctx_t *conv) {
    ddb_converter_settings_t settings = {
        .output_bps = conv->output_bps,
        .output_is_float = conv->output_is_float,
        .encoder_preset = conv->encoder_preset,
        .dsp_preset = conv->dsp_preset,
        .bypass_conversion_on_same_format = conv->bypass_same_format,
        .rewrite_tags_after_copy = conv->retag_after_copy,
    };
    return settings;
}

static void
get_folder_root (converter_ctx_t *conv, char* root) {
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
    }
}

static gboolean
destroy_progress_cb (gpointer ctx) {
    gtk_widget_destroy (ctx);
    return FALSE;
}

static void
free_conversion_utils (converter_ctx_t *conv) {
    g_idle_add (destroy_progress_cb, conv->progress_dialog);
    if (conv->convert_items) {
        free (conv->convert_items);
    }
    if (conv->convert_playlist) {
        deadbeef->plt_unref (conv->convert_playlist);
    }
    if (conv->outfolder) {
        free (conv->outfolder);
    }
    if (conv->outfile) {
        free (conv->outfile);
    }
    converter_plugin->encoder_preset_free (conv->encoder_preset);
    converter_plugin->dsp_preset_free (conv->dsp_preset);
    free (conv);
}

typedef struct {
    int next_index;
    int threads;
    pthread_t *pids;
    pthread_mutex_t item_mutex;
    pthread_mutex_t cancel_mutex;
    converter_ctx_t *conv;
    ddb_converter_settings_t settings;
    char root[2000];
} converter_thread_ctx_t;

static int
get_number_of_threads()
{
    int number_of_threads = deadbeef->conf_get_int("converter.threads", 0);
    if(number_of_threads <= 0) number_of_threads = 1;//against negative user value
    return number_of_threads;
}

static DB_playItem_t*
get_converter_thread_item(converter_thread_ctx_t *self, int index)
{
    return self->conv->convert_items[index];
}

static int
get_converter_thread_relative_item_id(converter_thread_ctx_t *self, int item_index)
{
    return item_index % self->threads + 1;
}

static void
init_converter_thread_ctx(converter_thread_ctx_t *thread_ctx, converter_ctx_t *conv, int threads)
{
    thread_ctx->next_index = 0;
    thread_ctx->threads = threads;
    thread_ctx->pids = malloc(thread_ctx->threads * sizeof(pthread_t));
    pthread_mutex_init(&thread_ctx->item_mutex, NULL);
    pthread_mutex_init(&thread_ctx->cancel_mutex, NULL);
    thread_ctx->conv = conv;
    thread_ctx->settings = get_converter_settings (conv);
    get_folder_root (conv, thread_ctx->root);
}

static converter_thread_ctx_t*
make_converter_thread_ctx(converter_ctx_t *conv, int threads)
{
    converter_thread_ctx_t *thread_ctx = malloc(sizeof(*thread_ctx));
    if (thread_ctx) init_converter_thread_ctx (thread_ctx, conv, threads);
    return thread_ctx;
}

void
free_converter_thread_ctx (converter_thread_ctx_t *self) {
    if(self) {
        free(self->pids);
        pthread_mutex_destroy(&self->item_mutex);
        pthread_mutex_destroy(&self->cancel_mutex);
    }
}

void
free_converter_thread_utils (converter_thread_ctx_t *self) {
    free_conversion_utils (self->conv);
    free_converter_thread_ctx (self);
}

static int
get_converter_thread_cancel (converter_thread_ctx_t *self) {
    return self->conv->cancelled;
}

static int
get_converter_thread_cancel_lock (converter_thread_ctx_t *self) {
    pthread_mutex_lock(&self->cancel_mutex);
    int cancelled = get_converter_thread_cancel(self);
    pthread_mutex_unlock(&self->cancel_mutex);
    return cancelled;
}

static void
set_converter_thread_cancel_lock (converter_thread_ctx_t *self, int cancel) {
    pthread_mutex_lock(&self->cancel_mutex);
    self->conv->cancelled = cancel;
    pthread_mutex_unlock(&self->cancel_mutex);
}

static int
pop_next_item_id (converter_thread_ctx_t *self)
{
    return self->next_index++;
}

static int
pop_next_item_id_lock (converter_thread_ctx_t *self)
{
    pthread_mutex_lock(&self->item_mutex);
    int id = pop_next_item_id(self);
    pthread_mutex_unlock(&self->item_mutex);
    return id;
}

static int
is_valid_converter_item (converter_thread_ctx_t *self, int item_index) {
    return item_index < self->conv->convert_items_count;
}

static int
conversion_may_proceed (converter_thread_ctx_t *self, int index)
{
    return !get_converter_thread_cancel_lock (self) && is_valid_converter_item(self, index);
}

enum {
    PRESET_TYPE_ENCODER,
    PRESET_TYPE_DSP
};

static const char *default_format = "[%tracknumber%. ][%artist% - ]%title%";

static void
fill_presets (GtkListStore *mdl, ddb_preset_t *head, int type) {
    ddb_preset_t *p = head;
    while (p) {
        GtkTreeIter iter;
        gtk_list_store_append (mdl, &iter);
        char stock[1000];
        const char *s = p->title;
        if (type == PRESET_TYPE_ENCODER && ((ddb_encoder_preset_t *)p)->readonly) {
            snprintf (stock, sizeof (stock), _("[Built-in] %s"), p->title);
            s = stock;
        }
        gtk_list_store_set (mdl, &iter, 0, s, -1);
        p = p->next;
    }
}

void
on_converter_progress_cancel (GtkDialog *dialog, gint response_id, gpointer user_data) {
    converter_thread_ctx_t *thread_ctx = user_data;
    set_converter_thread_cancel_lock(thread_ctx, 1);
}

void
on_output_file_changed                 (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_converter_realize                 (GtkWidget        *widget,
                                        gpointer         user_data);

typedef struct {
    GtkTextView *text_view;
    char text[8*1024];
} update_progress_info_t;

static gboolean
update_progress_cb (gpointer ctx) {
    update_progress_info_t *info = ctx;
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(info->text_view), info->text, -1);
    g_object_unref (info->text_view);
    free (info);
    return FALSE;
}

static int
print_progress_msg (char *buffer, size_t buffer_size, DB_playItem_t *item, int relative_item_id) {
    deadbeef->pl_lock ();
    int bytes = snprintf (buffer, buffer_size, "Th %02d: '%s' (from: %s)\n", relative_item_id, deadbeef->pl_find_meta_raw(item, "title"), deadbeef->pl_find_meta (item, ":URI"));
    deadbeef->pl_unlock ();
    return bytes;
}

static update_progress_info_t*
make_progress_info(converter_thread_ctx_t *self, int item_id) {
    update_progress_info_t *info = malloc (sizeof (*info));
    info->text_view = self->conv->text_view;
    g_object_ref (info->text_view);
    DB_playItem_t *item = get_converter_thread_item(self, item_id);
    int relative_id = get_converter_thread_relative_item_id(self, item_id);
    print_progress_msg (info->text, sizeof(info->text), item, relative_id);
    return info;
}

struct overwrite_prompt_ctx {
    const char *fname;
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

static int
overwrite_prompt (const char *outpath) {
    struct overwrite_prompt_ctx ctl;
    ctl.mutex = deadbeef->mutex_create ();
    ctl.cond = deadbeef->cond_create ();
    ctl.fname = outpath;
    ctl.result = 0;
    gdk_threads_add_idle (overwrite_prompt_cb, &ctl);
    deadbeef->cond_wait (ctl.cond, ctl.mutex);
    deadbeef->cond_free (ctl.cond);
    deadbeef->mutex_free (ctl.mutex);
    return ctl.result;
}

static int
get_skip_conversion (DB_playItem_t *item, const char* outpath, converter_ctx_t *conv) {
    int skip = 0;
    char *real_out = realpath(outpath, NULL);
    if (real_out) {
        skip = 1;
        deadbeef->pl_lock();
        char *real_in = realpath(deadbeef->pl_find_meta(item, ":URI"), NULL);
        deadbeef->pl_unlock();
        const int paths_match = real_in && !strcmp(real_in, real_out);
        free(real_in);
        free(real_out);
        if (paths_match) {
            fprintf (stderr, "converter: destination file is the same as source file, skipping\n");
        }
        else if (conv->overwrite_action == 2 || (conv->overwrite_action == 1 && overwrite_prompt(outpath))) {
            unlink (outpath);
            skip = 0;
        }
    }
    return skip;
}

static void
update_gui_convert (converter_thread_ctx_t *self, int item_id) {
    update_progress_info_t *info = make_progress_info (self, item_id);
    g_idle_add (update_progress_cb, info);

    char outpath[2000];
    converter_ctx_t *conv = self->conv;
    DB_playItem_t *item = get_converter_thread_item(self, item_id);
    converter_plugin->get_output_path2 (item, conv->convert_playlist, conv->outfolder, conv->outfile, conv->encoder_preset, conv->preserve_folder_structure, self->root, conv->write_to_source_folder, outpath, sizeof (outpath));
    int skip = get_skip_conversion (item, outpath, conv);
    if (!skip) {
        int cancelled = get_converter_thread_cancel_lock (self);
        converter_plugin->convert2 (&self->settings, item, outpath, &cancelled);
    }
}

static void
unref_convert_items (DB_playItem_t **convert_items, int begin, int end) {
    for (int k = begin; k < end; ++k) {
        deadbeef->pl_item_unref (convert_items[k]);
    }
}

static void*
converter_thread_worker (void *ctx) {
    converter_thread_ctx_t *thread_ctx = ctx;
    int item_id = pop_next_item_id_lock (thread_ctx);
    while (conversion_may_proceed (thread_ctx, item_id)) {
        update_gui_convert (thread_ctx, item_id);
        deadbeef->pl_item_unref (thread_ctx->conv->convert_items[item_id]);
        item_id = pop_next_item_id_lock (thread_ctx);
    }
    return NULL;
}

static void
create_pool_threads(converter_thread_ctx_t* self)
{
    for(int k = 0; k < self->threads; ++k)
        pthread_create(&self->pids[k], NULL, &converter_thread_worker, (void*)self);
}

static void join_pool_threads(converter_thread_ctx_t* self)
{
    for(int k = 0; k < self->threads; ++k)
        pthread_join(self->pids[k], NULL);
}

static void
converter_worker (void *ctx) {
    deadbeef->background_job_increment ();
    converter_thread_ctx_t *thread_ctx = ctx;
    create_pool_threads (thread_ctx);
    join_pool_threads (thread_ctx);
    if (get_converter_thread_cancel (thread_ctx)) {
        converter_ctx_t* conv = thread_ctx->conv;
        unref_convert_items (conv->convert_items, thread_ctx->next_index, conv->convert_items_count);
    }
    free_converter_thread_utils (thread_ctx);
    free (thread_ctx);
    deadbeef->background_job_decrement ();
}

int
converter_process (converter_ctx_t *conv)
{
    conv->outfolder = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (conv->converter, "output_folder"))));
    const char *outfile = gtk_entry_get_text (GTK_ENTRY (lookup_widget (conv->converter, "output_file")));
    if (outfile[0] == 0) {
        outfile = default_format;
    }
    conv->outfile = strdup (outfile);
    conv->preserve_folder_structure = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "preserve_folders")));
    conv->write_to_source_folder = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "write_to_source_folder")));
    conv->bypass_same_format = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "bypass_same_format")));
    conv->retag_after_copy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "retag_after_copy")));
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

    GtkWidget *progress_dialog = gtk_dialog_new_with_buttons (_("Converting..."), GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    GtkScrolledWindow* scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    GtkTextView* text_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_left_margin(text_view, 5);
    gtk_text_view_set_right_margin(text_view, 5);
    gtk_text_view_set_editable(text_view, FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(text_view));
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(progress_dialog))), GTK_WIDGET(scrolled_window), TRUE, TRUE, 0);

    converter_thread_ctx_t* thread_ctx = make_converter_thread_ctx (conv, get_number_of_threads());
    g_signal_connect ((gpointer)progress_dialog, "response", G_CALLBACK (on_converter_progress_cancel), thread_ctx);
    gtk_window_set_default_size(GTK_WINDOW(progress_dialog), 600, 10 * thread_ctx->threads);
    gtk_widget_show_all(progress_dialog);

    conv->progress_dialog = progress_dialog;
    conv->text_view = text_view;
    intptr_t tid = deadbeef->thread_start (converter_worker, thread_ctx);
    deadbeef->thread_detach (tid);
    return 0;
}

static int preview_delay_timer = 0;

static gboolean
preview_update (gpointer user_data)
{
    // reset delay
    if (preview_delay_timer) {
        g_source_remove (preview_delay_timer);
        preview_delay_timer = 0;
    }

    converter_ctx_t *conv = current_ctx;
    if (!conv) {
        return FALSE;
    }

    GtkTreeView *tree_view = GTK_TREE_VIEW (lookup_widget (conv->converter, "preview_tree"));
    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (tree_view));

    if (!tree_view || !store) {
        return FALSE;
    }
    gtk_list_store_clear (store);

    // get encoder preset to access filename extension
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (conv->converter, "encoder"));
    int enc_preset = gtk_combo_box_get_active (combo);
    ddb_encoder_preset_t *p = NULL;
    if (enc_preset >= 0) {
        p = converter_plugin->encoder_preset_get_for_idx (enc_preset);
    }
    else {
        return FALSE;
    }

    // get filename format
    GtkEntry *output_file = GTK_ENTRY (lookup_widget (conv->converter, "output_file"));
    const char *format = gtk_entry_get_text (output_file);
    if (!format || format[0] == 0) {
        // use default format when entry is empty
        format = default_format;
    }

    char *tf = deadbeef->tf_compile (format);
    if (!tf) {
        return FALSE;
    }

    // detach model from treeview while adding lots of items to increase performance
    g_object_ref (store);
    gtk_tree_view_set_model (tree_view, NULL);

    // NOTE: while displaying is not a problem, adding lots of rows to a GtkListStore
    // can be quite slow (about 20s for ~60.000 rows)
    // so we better prevent those huge numbers from locking the UI
    int num_items = MIN (1000, conv->convert_items_count);
    for (int n = 0; n < num_items; n++) {
        DB_playItem_t *it = conv->convert_items[n];
        if (it) {
            char outpath[PATH_MAX];
            const char *output_folder = gtk_entry_get_text (GTK_ENTRY(lookup_widget (conv->converter, "output_folder")));

            int preserve_folders = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "preserve_folders")));
            int write_to_source = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "write_to_source_folder")));
            converter_plugin->get_output_path2 (it, conv->convert_playlist, output_folder, format, p, preserve_folders, "", write_to_source, outpath, sizeof (outpath));

            GtkTreeIter iter;
            gtk_list_store_insert_with_values (store, &iter, -1, 0, outpath, -1);
        }
    }
    // reattach model to treeview
    gtk_tree_view_set_model (tree_view, GTK_TREE_MODEL (store));
    g_object_unref (store);
    deadbeef->tf_free (tf);
    return FALSE;
}

static void
preview_timeout_add (void)
{
    if (preview_delay_timer) {
        g_source_remove (preview_delay_timer);
        preview_delay_timer = 0;
    }
    preview_delay_timer = g_timeout_add (100, preview_update, NULL);
}

static void
create_preview_treeview (converter_ctx_t *conv)
{
    GtkWidget *treeview = lookup_widget (conv->converter, "preview_tree");
    GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

    GtkCellRenderer   *renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("Preview",
                                                       renderer,
                                                       "text", 0,
                                                       NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, 0);
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

    ddb_playItem_t *playing_track = NULL;
    if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        playing_track = deadbeef->streamer_get_playing_track_safe ();
    }

    deadbeef->pl_lock ();
    switch (ctx) {
    case DDB_ACTION_CTX_MAIN:
    case DDB_ACTION_CTX_SELECTION:
        {
            // copy list
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                conv->convert_playlist = plt;
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
            ddb_playlist_t *plt = deadbeef->action_get_playlist ();
            if (plt) {
                conv->convert_playlist = plt;
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
            }
            break;
        }
    case DDB_ACTION_CTX_NOWPLAYING:
        {
            if (playing_track) {
                conv->convert_playlist = deadbeef->pl_get_playlist (playing_track);
                conv->convert_items_count = 1;
                conv->convert_items = malloc (sizeof (DB_playItem_t *) * conv->convert_items_count);
                if (conv->convert_items) {
                    conv->convert_items[0] = playing_track;
                    deadbeef->pl_item_ref(playing_track);
                }
            }
        }
        break;
    }
    deadbeef->pl_unlock ();

    if (playing_track != NULL) {
        deadbeef->pl_item_unref (playing_track);
        playing_track = NULL;
    }

    conv->converter = create_converterdlg ();
    GtkWidget *mainwin = gtkui_plugin->get_mainwin ();
    gtk_window_set_transient_for (GTK_WINDOW (conv->converter), GTK_WINDOW (mainwin));

    create_preview_treeview (conv);

    deadbeef->conf_lock ();
    const char *out_folder = deadbeef->conf_get_str_fast ("converter.output_folder", "");
    if (!out_folder[0]) {
        out_folder = getenv("HOME");
    }
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (conv->converter, "output_folder")), out_folder);

    GtkWidget *output_file = lookup_widget (conv->converter, "output_file");
    const char *output_file_text = deadbeef->conf_get_str_fast ("converter.output_file_tf", "");
    gtk_entry_set_text (GTK_ENTRY (output_file), output_file_text);

    g_signal_connect ((gpointer) output_file, "changed",
            G_CALLBACK (on_output_file_changed),
            conv);

    // fill preview on window launch
    g_signal_connect ((gpointer) conv->converter, "realize",
            G_CALLBACK (on_converter_realize),
            (void *)output_file_text);


    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "preserve_folders")), deadbeef->conf_get_int ("converter.preserve_folder_structure", 0));

    int write_to_source_folder = deadbeef->conf_get_int ("converter.write_to_source_folder", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "write_to_source_folder")), write_to_source_folder);

    int bypass_same_format = deadbeef->conf_get_int ("converter.bypass_same_format", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "bypass_same_format")), bypass_same_format);

    int retag_after_copy = deadbeef->conf_get_int ("converter.retag_after_copy", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (conv->converter, "retag_after_copy")), retag_after_copy);
    gtk_widget_set_sensitive (lookup_widget (conv->converter, "retag_after_copy"), bypass_same_format);

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
converter_show (DB_plugin_action_t *act, ddb_action_context_t ctx) {
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
    // update preview to show new filename extensions
    preview_update (NULL);
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
    preview_timeout_add ();
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
on_converter_realize                 (GtkWidget        *widget,
                                        gpointer         user_data)
{
    preview_timeout_add ();
}

void
on_output_file_changed                 (GtkEntry        *entry,
                                        gpointer         user_data)
{
    preview_timeout_add ();
    deadbeef->conf_set_str ("converter.output_file_tf", gtk_entry_get_text (entry));
    deadbeef->conf_save ();
}

void
on_preserve_folders_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    preview_update (NULL);
    deadbeef->conf_set_int ("converter.preserve_folder_structure", gtk_toggle_button_get_active (togglebutton));
    deadbeef->conf_save ();
}

void
on_write_to_source_folder_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    preview_update (NULL);
    int active = gtk_toggle_button_get_active (togglebutton);
    converter_ctx_t *conv = user_data;
    deadbeef->conf_set_int ("converter.write_to_source_folder", active);
    gtk_widget_set_sensitive (lookup_widget (conv->converter, "output_folder"), !active);
    gtk_widget_set_sensitive (lookup_widget (conv->converter, "preserve_folders"), !active);
    deadbeef->conf_save ();
}

void
on_bypass_same_format_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("converter.bypass_same_format", active);
    deadbeef->conf_save ();
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (togglebutton));
    gtk_widget_set_sensitive (lookup_widget (toplevel, "retag_after_copy"), active);
}

void
on_retag_after_copy_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("converter.retag_after_copy", active);
    deadbeef->conf_save ();
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
    p->tag_mp4 = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "mp4")));
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
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "mp4")), p->tag_mp4);

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
                        if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), old->title) > 0) {
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
        if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), p->title) > 0) {
            unlink (path);
        }

        converter_plugin->encoder_preset_remove (p);
        converter_plugin->encoder_preset_free (p);

        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (current_ctx->converter, "encoder"));
        refresh_encoder_lists (combo, GTK_TREE_VIEW (list));
    }
}

static GtkWidget *encpreset_dialog;

static void
on_encoder_preset_cursor_changed (GtkTreeView     *treeview,
                                        gpointer         user_data) {
    if (!encpreset_dialog) {
        return;
    }
    GtkWidget *edit = lookup_widget (encpreset_dialog, "edit");
    GtkWidget *remove = lookup_widget (encpreset_dialog, "remove");

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
    // FIXME: this is a hack to prevent deleting the original preset after
    // copying. Needs a proper fix.
    if (current_ctx->current_encoder_preset->title) {
        free (current_ctx->current_encoder_preset->title);
        current_ctx->current_encoder_preset->title = NULL;
    }

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
    encpreset_dialog = dlg;
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
    encpreset_dialog = NULL;
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
                if (current_ctx->current_dsp_preset->title && strcmp (title, current_ctx->current_dsp_preset->title)) {
                    char path[1024];
                    if (snprintf (path, sizeof (path), "%s/presets/dsp/%s.txt", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), current_ctx->current_dsp_preset->title) > 0) {
                        unlink (path);
                    }
                }
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

    // FIXME: this is a hack to prevent deleting the original preset after
    // copying. Needs a proper fix.
    if (current_ctx->current_dsp_preset->title) {
        free (current_ctx->current_dsp_preset->title);
        current_ctx->current_dsp_preset->title = NULL;
    }

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
        if (snprintf (path, sizeof (path), "%s/presets/dsp/%s.txt", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), p->title) > 0) {
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
    GtkWidget *link = gtk_link_button_new_with_label ("http://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0", _("Help"));
    return link;
}

GtkWidget*
encoder_cmdline_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
    GtkWidget *link = gtk_link_button_new_with_label ("http://github.com/DeaDBeeF-Player/deadbeef/wiki/Encoder-Command-Line", _("Help"));
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

static int
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
#define REQ_CONV_VERSION 4
    if (!PLUG_TEST_COMPAT(&converter_plugin->misc.plugin, 1, REQ_CONV_VERSION)) {
        fprintf (stderr, "convgui: need converter>=1.%d, but found %d.%d\n", REQ_CONV_VERSION, converter_plugin->misc.plugin.version_major, converter_plugin->misc.plugin.version_minor);
        return -1;
    }
    return 0;
}

static void
import_legacy_tf (const char *key_from, const char *key_to) {
    deadbeef->conf_lock ();
    if (!deadbeef->conf_get_str_fast (key_to, NULL)
            && deadbeef->conf_get_str_fast (key_from, NULL)) {
        char old[200], new[200];
        deadbeef->conf_get_str (key_from, "", old, sizeof (old));
        deadbeef->tf_import_legacy (old, new, sizeof (new));
        deadbeef->conf_set_str (key_to, new);
    }
    deadbeef->conf_unlock ();
}

static int
convgui_start (void) {
    import_legacy_tf ("converter.output_file", "converter.output_file_tf");
    return 0;
}

DB_misc_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 2,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.id = "converter-gtk3",
#else
    .plugin.id = "converter-gtk2",
#endif
    .plugin.name = "Converter UI",
    .plugin.descr = "GTK User interface for the Converter plugin\n"
        "Usage:\n"
        "· select some tracks in playlist\n"
        "· right click\n"
        "· select «Convert»\n",
    .plugin.copyright =
        "Converter UI for DeaDBeeF Player\n"
        "Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.get_actions = convgui_get_actions,
    .plugin.connect = convgui_connect,
    .plugin.start = convgui_start,
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

