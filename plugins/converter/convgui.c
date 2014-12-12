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
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "converter.h"
#include "support.h"
#include "interface.h"
#include "../gtkui/gtkui_api.h"

DB_functions_t *deadbeef;
ddb_gtkui_t *gtkui_plugin;
ddb_converter_t *converter_plugin;

enum convert_columns {
    LIST_STORE_ICON = 0,
    LIST_STORE_NAME,
    LIST_STORE_PERCENT,
    LIST_STORE_MESSAGE,
    LIST_STORE_COLUMNS
};

enum convert_status {
    LIST_STATUS_WAITING = 0,
    LIST_STATUS_SKIPPED,
    LIST_STATUS_CONVERTING,
    LIST_STATUS_CONVERTED,
    LIST_STATUS_FAILED
};

struct converter_ctx {
    ddb_encoder_preset_t *encoder_preset;
    int overwrite_action;
    ddb_dsp_preset_t *dsp_preset;
    int output_bps;
    int output_is_float;
    GtkWidget *progress;
    uintptr_t mutex;
    enum ddb_convert_api api;
    struct converter_ctx_item {
        GtkTreeRowReference *row;
        DB_playItem_t *item;
        enum convert_status status;
        char *outpath;
        unsigned elapsed;
        float readpos;
    } *items;
};

struct overwrite_info {
    GtkWidget *progress;
    const char *path;
    uintptr_t mutex;
    uintptr_t cond;
    int result;
};

static void
alert_dialog(GtkWindow *win, const char *msg, const char *secondary_text)
{
    GtkWidget *dlg = gtk_message_dialog_new(win, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, msg);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), secondary_text);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static int
confirm_dialog(GtkWindow *win, const char *prompt, const char *secondary_text, const char *button_name)
{
    GtkWidget *dlg = gtk_message_dialog_new(win, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_CANCEL, prompt);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), secondary_text);
    GtkWidget *button = gtk_dialog_add_button(GTK_DIALOG(dlg), button_name, GTK_RESPONSE_YES);
    gtk_widget_grab_focus(button);
    const int res = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    return res;
}

static gboolean
overwrite_prompt_cb(void *ctx)
{
    struct overwrite_info *info = ctx;
    info->result = confirm_dialog(GTK_WINDOW(info->progress), _("The file already exists and will be overwritten"), info->path, _("_Overwrite"));
    deadbeef->cond_signal(info->cond);
    return FALSE;
}

static int
overwrite_dialog(GtkWidget *progress, const char *overwrite_path)
{
    struct overwrite_info info;
    info.result = GTK_RESPONSE_CANCEL;
    info.progress = progress;
    info.mutex = deadbeef->mutex_create();
    if (info.mutex) {
        info.cond = deadbeef->cond_create();
        if (info.cond) {
            info.path = overwrite_path;
            gdk_threads_add_idle(overwrite_prompt_cb, &info);
            deadbeef->cond_wait(info.cond, info.mutex);
            deadbeef->cond_free(info.cond);
        }
        deadbeef->mutex_free(info.mutex);
    }
    return info.result;
}

const char *
status_icon(const enum convert_status status)
{
    switch (status) {
        case LIST_STATUS_WAITING:
            return NULL;
        case LIST_STATUS_SKIPPED:
            return "gtk-refresh";
        case LIST_STATUS_CONVERTING:
            return "gtk-media-play";
        case LIST_STATUS_CONVERTED:
            return "gtk-yes";
        case LIST_STATUS_FAILED:
            return "gtk-no";
        default:
            return NULL;
    }
}

const char *
status_description(const enum convert_status status)
{
    switch (status) {
        case LIST_STATUS_WAITING:
            return _("Waiting...");
        case LIST_STATUS_SKIPPED:
            return _("Skipped");
        case LIST_STATUS_CONVERTING:
            return _("Converting...");
        case LIST_STATUS_CONVERTED:
            return _("Successfully converted");
        case LIST_STATUS_FAILED:
            return _("Failed");
        default:
            return _("Internal error");
    }
}

static gboolean
all_items_done_cb(gpointer ctx)
{
    GtkWidget *dlg = ctx;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "keep_open"))) && gtk_widget_get_visible(dlg)) {
        GtkWidget *abort = lookup_widget(dlg, "abort");
        GtkWidget *close = lookup_widget(dlg, "close");
        gtk_widget_show(close);
        if (gtk_widget_is_focus(abort)) {
            gtk_widget_grab_focus(close);
        }
        gtk_widget_hide(abort);
    }
    else {
        gtk_widget_destroy(dlg);
    }
    deadbeef->background_job_decrement();
    return FALSE;
}

static void
clear_context(struct converter_ctx *conv)
{
    if (conv) {
        deadbeef->mutex_free(conv->mutex);
        converter_plugin->encoder_preset_free(conv->encoder_preset);
        converter_plugin->dsp_preset_free(conv->dsp_preset);
        if (conv->items) {
            for (struct converter_ctx_item *item = conv->items; item->item; item++) {
                deadbeef->pl_item_unref(item->item);
                gtk_tree_row_reference_free(item->row);
                free(item->outpath);
            }
            free(conv->items);
        }
        free(conv);
    }
}

static void
on_progress_keep_open(GtkToggleButton *button, gpointer user_data)
{
    deadbeef->conf_set_int("converter.keep_open", gtk_toggle_button_get_active(button));
}

static void
on_progress_abort(GtkWidget *dialog, struct converter_ctx *conv)
{
    conv->api = DDB_CONVERT_API_ABORT;
}

static void
update_remaining(GtkWidget *dlg, const unsigned elapsed, const float readpos, const float duration)
{
    if (elapsed > 4 && readpos > 0) {
        const float fraction = duration / readpos - 1;
        unsigned remaining = elapsed * fraction + 1;
        const char *units = remaining < 120 ? _("seconds") : _("minutes");
        if (remaining >= 120) {
            remaining /= 60;
        }
        const char *remaining_pattern = _(" %d %s remaining");
        char remaining_str[strlen(remaining_pattern) + strlen(units) + (size_t)log10(remaining)];
        sprintf(remaining_str, remaining_pattern, remaining, units);
        gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "remaining")), remaining_str);
    }
}

static void
update_timing(GtkWidget *dlg, const unsigned elapsed, const float readpos)
{
    char elapsed_str[sizeof("%02d:%02d") + (size_t)log10(elapsed/60)];
    sprintf(elapsed_str, "%02d:%02d", elapsed/60, elapsed%60);

    if (readpos < 0) {
        const char *progress_pattern = _("%s (using temporary file)");
        char progress_label[strlen(progress_pattern) + strlen(elapsed_str)];
        sprintf(progress_label, progress_pattern, elapsed_str);
        gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "message")), progress_label);
    }
    else {
        const unsigned readpos_int = readpos;
        char track_str[sizeof("%02u:%02u:%02u") + (size_t)log10(readpos/3600)];
        sprintf(track_str, readpos_int < 3600 ? "%02u:%02u" : "%3$02u:%02u:%02u", readpos_int/60%60, readpos_int%60, readpos_int/3600);
        const float speed = readpos/elapsed;
        char speed_str[sizeof("%.1fx") + (size_t)log10(speed)];
        sprintf(speed_str, speed < 100 ? " (%.1fx)" : " (%.0fx)", speed);
        const char *progress_pattern = _("Elapsed time: %s, track time: %s%s");
        char progress_label[strlen(progress_pattern) + strlen(elapsed_str) + strlen(track_str) + strlen(speed_str)];
        sprintf(progress_label, progress_pattern, elapsed_str, track_str, elapsed ? speed_str : "");
        gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "message")), progress_label);
    }
}

static gboolean
update_timing_cb(gpointer ctx)
{
    struct converter_ctx_item *item = ctx;
    GtkTreeModel *mdl = gtk_tree_row_reference_get_model(item->row);
    GtkWidget *list = GTK_WIDGET(g_object_get_data(G_OBJECT(mdl), "list"));
    GtkWidget *dlg = gtk_widget_get_toplevel(list);
    if (!gtk_widget_get_visible(dlg)) {
        return FALSE;
    }

    GtkTreePath *path = gtk_tree_row_reference_get_path(item->row);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(mdl, &iter, path);
    gtk_tree_path_free(path);

    const float duration = deadbeef->pl_get_item_duration(item->item);
    const unsigned percent = item->readpos < 0 ? item->readpos*-1 : 5 + 90*item->readpos/duration;
    gtk_list_store_set(GTK_LIST_STORE(mdl), &iter, LIST_STORE_PERCENT, percent, -1);

    if (g_object_get_data(G_OBJECT(list), "current-item") == item) {
        update_remaining(dlg, item->elapsed, item->readpos, duration);
        update_timing(dlg, item->elapsed, item->readpos);
    }

    return FALSE;
}

static void
convert_callback(const time_t start, const time_t now, const float readpos, void *user_data)
{
    struct converter_ctx_item *item = user_data;
    item->elapsed = now - start;
    item->readpos = readpos;
    gdk_threads_add_idle(update_timing_cb, item);
}

static void
update_detail(struct converter_ctx_item *items, GtkTreeView *list)
{
    size_t selected = 0, skipped = 0, converting = 0, converted = 0, failed = 0;
    struct converter_ctx_item *item;
    for (struct converter_ctx_item *test_item = items; test_item->item; test_item++) {
        selected++;
        switch (test_item->status) {
            case LIST_STATUS_SKIPPED:
                if (!converting && !failed && !converted) {
                    item = test_item;
                }
                skipped++;
                break;
            case LIST_STATUS_CONVERTING:
                if (!converting) {
                    item = test_item;
                }
                converting++;
                break;
            case LIST_STATUS_CONVERTED:
                if (!converting && !failed) {
                    item = test_item;
                }
                converted++;
                break;
            case LIST_STATUS_FAILED:
                if (!converting && !failed) {
                    item = test_item;
                }
                failed++;
                break;
            default:
                break;
        }
    }

    GtkWidget *dlg = gtk_widget_get_toplevel(GTK_WIDGET(list));
    if (selected > 1) {
        const char *tracks_pattern = _("%d tracks selected (%d skipped, %d converted, %d failed)");
        char tracks_label[strlen(tracks_pattern) + (size_t)(log10(selected) + log10(skipped) + log10(converted) + log10(failed))];
        sprintf(tracks_label, tracks_pattern, selected, skipped, converted, failed);
        gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "tracks")), tracks_label);
    }

    GtkTreeModel *mdl = gtk_tree_view_get_model(list);
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(list), &mdl, &iter)) {
        GtkTreePath *selected_path = gtk_tree_model_get_path(mdl, &iter);
        for (struct converter_ctx_item *test_item = items; test_item->item; test_item++) {
            GtkTreePath *item_path = gtk_tree_row_reference_get_path(test_item->row);
            if (!gtk_tree_path_compare(item_path, selected_path)) {
                item = test_item;
            }
            gtk_tree_path_free(item_path);
        }
        gtk_tree_path_free(selected_path);
    }
    GtkTreePath *path = gtk_tree_row_reference_get_path(item->row);
    gtk_tree_model_get_iter(mdl, &iter, path);
    gtk_tree_path_free(path);

    GtkTreePath *start_path;
    GtkTreePath *end_path;
    if (gtk_tree_view_get_visible_range(list, &start_path, &end_path)) {
        GtkTreePath *path = gtk_tree_model_get_path(mdl, &iter);
        if (gtk_tree_path_compare(path, start_path) <= 0 || gtk_tree_path_compare(path, end_path) >= 0) {
            gtk_tree_view_scroll_to_cell(list, path, NULL, TRUE, 1.0, 1.0);
        }
        gtk_tree_path_free(start_path);
        gtk_tree_path_free(end_path);
    }

    char *message;
    gtk_tree_model_get(mdl, &iter, LIST_STORE_MESSAGE, &message, -1);
    gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "status")), status_description(item->status));
    gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "remaining")), NULL);
    gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "message")), message);
    deadbeef->pl_lock();
    gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "from")), deadbeef->pl_find_meta(item->item, ":URI"));
    deadbeef->pl_unlock();
    gtk_label_set_text(GTK_LABEL(lookup_widget(dlg, "to")), item->outpath);
    g_free(message);
    g_object_set_data(G_OBJECT(list), "current-item", item);

    if (item->status == LIST_STATUS_CONVERTING || item->status == LIST_STATUS_CONVERTED && !message) {
        if (item->status == LIST_STATUS_CONVERTING) {
            update_remaining(dlg, item->elapsed, item->readpos, deadbeef->pl_get_item_duration(item->item));
        }
        update_timing(dlg, item->elapsed, item->readpos);
    }
}

struct update_status_ctx {
    struct converter_ctx_item *items;
    GtkTreeRowReference *row;
    enum convert_status status;
    guint percent;
    const gchar *message;
};

static gboolean
update_status_cb(gpointer ctx)
{
    struct update_status_ctx *update = ctx;

    GtkTreeModel *mdl = gtk_tree_row_reference_get_model(update->row);
    GtkTreePath *path = gtk_tree_row_reference_get_path(update->row);
    GtkTreeIter iter;
    gtk_tree_model_get_iter(mdl, &iter, path);
    gtk_tree_path_free(path);

    gtk_list_store_set(GTK_LIST_STORE(mdl), &iter, LIST_STORE_ICON, status_icon(update->status), LIST_STORE_PERCENT, update->percent, LIST_STORE_MESSAGE, update->message, -1);
    update_detail(update->items, g_object_get_data(G_OBJECT(mdl), "list"));
    free(ctx);
    return FALSE;
}

static void
set_status(struct converter_ctx_item *items, struct converter_ctx_item *item, const enum convert_status status, const guint percent, const char *message)
{
    item->status = status;
    struct update_status_ctx *ctx = malloc(sizeof(struct update_status_ctx));
    if (ctx) {
        ctx->items = items;
        ctx->row = item->row;
        ctx->status = status;
        ctx->percent = percent;
        ctx->message = message;
        gdk_threads_add_idle(update_status_cb, ctx);
    }
}

static void
on_selection_changed(GtkTreeSelection *selection, struct converter_ctx_item *items)
{
    update_detail(items, gtk_tree_selection_get_tree_view(selection));
}

static const char *
get_output_folder(void)
{
    const char *out_folder = deadbeef->conf_get_str_fast("converter.output_folder", "");
    return *out_folder ? out_folder : getenv("HOME");
}

static const char *
get_output_pattern(void)
{
    const char *output_file = deadbeef->conf_get_str_fast("converter.output_file", "");
    return output_file[0] ? output_file : "%a - %t";
}

static char *
get_root_folder(GtkWidget *widget, DB_playItem_t **items)
{
    const int preserve_folder_structure = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "preserve_folders")));
    return preserve_folder_structure ? converter_plugin->get_root_folder(items) : NULL;
}

static struct converter_ctx_item *
first_waiting_item(struct converter_ctx_item *items)
{
    for (struct converter_ctx_item *item = items; item->item; item++) {
        if (item->status == LIST_STATUS_WAITING) {
            return item;
        }
    }
    return NULL;
}

static void
convert_item(struct converter_ctx *conv, struct converter_ctx_item *item)
{
    set_status(conv->items, item, LIST_STATUS_CONVERTING, 0, NULL);

    if (!item->outpath) {
        set_status(conv->items, item, LIST_STATUS_FAILED, 0, _("Internal error"));
        return;
    }

    char *real_out = realpath(item->outpath, NULL);
    if (real_out) {
        deadbeef->mutex_unlock(conv->mutex);
        deadbeef->pl_lock();
        char *real_in = realpath(deadbeef->pl_find_meta(item->item, ":URI"), NULL);
        deadbeef->pl_unlock();
        const int paths_match = real_in && !strcmp(real_in, real_out);
        free(real_in);
        free(real_out);
        sched_yield();
        deadbeef->mutex_lock(conv->mutex);
        if (paths_match) {
            set_status(conv->items, item, LIST_STATUS_FAILED, 0, _("Destination file would overwrite source file"));
            return;
        }

        if (conv->overwrite_action == 0 || conv->overwrite_action == 1 && overwrite_dialog(conv->progress, item->outpath) != GTK_RESPONSE_YES) {
            set_status(conv->items, item, LIST_STATUS_SKIPPED, 100, _("File already exists"));
            return;
        }

        unlink(item->outpath);
    }

    deadbeef->mutex_unlock(conv->mutex);
    char *message = NULL;
    const int res = converter_plugin->convert(item->item, conv->encoder_preset, item->outpath, conv->dsp_preset, conv->output_bps, conv->output_is_float, &conv->api, &message, convert_callback, item);
    deadbeef->mutex_lock(conv->mutex);
    if (conv->api == DDB_CONVERT_API_ABORT) {
        message = _("User aborted");
    }
    set_status(conv->items, item, res ? LIST_STATUS_FAILED : LIST_STATUS_CONVERTED, res ? 0 : 100, message);
}

static void
converter_worker(void *ctx)
{
    deadbeef->background_job_increment();
    struct converter_ctx *conv = ctx;

    deadbeef->mutex_lock(conv->mutex);
    struct converter_ctx_item *next_item;
    while (conv->api == DDB_CONVERT_API_CONTINUE && (next_item = first_waiting_item(conv->items))) {
        convert_item(conv, next_item);
    }

    for (struct converter_ctx_item *item = conv->items; item->item; item++) {
        if (conv->api == DDB_CONVERT_API_CONTINUE && item->status == LIST_STATUS_WAITING || item->status == LIST_STATUS_CONVERTING) {
            deadbeef->mutex_unlock(conv->mutex);
            deadbeef->background_job_decrement();
            return;
        }
    }

    gdk_threads_add_idle(all_items_done_cb, conv->progress);
    deadbeef->mutex_unlock(conv->mutex);
}

static int
converter_process(GtkWidget *dlg, DB_playItem_t **items)
{
    struct converter_ctx *conv = calloc(1, sizeof(struct converter_ctx));
    if (!conv) {
        return -1;
    }

    size_t items_size = 0;
    while (items[items_size++]);
    conv->items = calloc(1, sizeof(struct converter_ctx_item) * (items_size));
    const int encoder_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(lookup_widget(dlg, "encoder")));
    ddb_encoder_preset_t *encoder_preset = encoder_idx == -1 ? NULL : converter_plugin->encoder_preset_get_for_idx(encoder_idx);
    if (encoder_preset) {
        conv->encoder_preset = converter_plugin->encoder_preset_duplicate(encoder_preset);
    }
    conv->mutex = deadbeef->mutex_create();
    if (!conv->items || !conv->encoder_preset || !conv->mutex) {
        clear_context(conv);
        return -1;
    }

    conv->overwrite_action = gtk_combo_box_get_active(GTK_COMBO_BOX(lookup_widget(dlg, "overwrite_action")));
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "force_format")))) {
        conv->output_bps = -1;
    }
    else {
        const int selected_format = gtk_combo_box_get_active(GTK_COMBO_BOX(lookup_widget(dlg, "output_format")));
        if (selected_format < 4) {
            conv->output_bps = (selected_format+1) * 8;
        }
        else {
            conv->output_bps = 32;
            conv->output_is_float = 1;
        }
    }

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "apply_dsp")))) {
        const int dsp_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(lookup_widget(dlg, "dsp_preset")));
        if (dsp_idx != -1) {
            ddb_dsp_preset_t *dsp_preset = converter_plugin->dsp_preset_get_for_idx(dsp_idx);
            if (dsp_preset) {
                conv->dsp_preset = converter_plugin->dsp_preset_duplicate(dsp_preset);
            }
        }
    }

    GtkWidget *progress = create_progressdlg();
    gtk_window_set_transient_for(GTK_WINDOW(progress), GTK_WINDOW(gtkui_plugin->get_mainwin()));
    conv->progress = progress;

    if (items[1]) {
        gtk_widget_show(gtk_widget_get_parent(lookup_widget(progress, "tracks")));
    }

    GtkWidget *keep_open = lookup_widget(progress, "keep_open");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keep_open), deadbeef->conf_get_int("converter.keep_open", 0));
    g_signal_connect(keep_open, "toggled", G_CALLBACK(on_progress_keep_open), conv);
    g_signal_connect(lookup_widget(progress, "abort"), "clicked", G_CALLBACK(on_progress_abort), conv);
    g_signal_connect_swapped(lookup_widget(progress, "close"), "clicked", G_CALLBACK(gtk_widget_destroy), progress);

    GtkTreeView *list = GTK_TREE_VIEW(lookup_widget(progress, "items"));
    GtkCellRenderer *icon_cell = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *icon_col = gtk_tree_view_column_new_with_attributes("", icon_cell, "stock-id", LIST_STORE_ICON, NULL);
    gtk_tree_view_append_column(list, icon_col);
    GtkCellRenderer *progress_cell = gtk_cell_renderer_progress_new();
    GtkTreeViewColumn *progress_col = gtk_tree_view_column_new_with_attributes("", progress_cell, "text", LIST_STORE_NAME, "value", LIST_STORE_PERCENT, NULL);
    gtk_tree_view_append_column(list, progress_col);
    g_object_set(G_OBJECT(progress_cell), "text-xalign", 0.0);

    GtkListStore *mdl = gtk_list_store_new(LIST_STORE_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);
    g_object_set_data(G_OBJECT(mdl), "list", list);
    gtk_tree_view_set_model(list, GTK_TREE_MODEL(mdl));
    g_object_unref(mdl);

    const int use_source_folder = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "use_source_folder")));
    char *rootfolder = get_root_folder(dlg, items);
    deadbeef->conf_lock();
    const char *outfolder = get_output_folder();
    const char *outfile = get_output_pattern();
    for (size_t i = 0; items[i]; i++) {
        GtkTreeIter iter;
        deadbeef->pl_lock();
        const char *title = deadbeef->pl_find_meta(items[i], "title");
        const char *name = title ? title : basename(deadbeef->pl_find_meta(items[i], ":URI"));
        gtk_list_store_insert_with_values(mdl, &iter, -1, LIST_STORE_NAME, name, -1);
        deadbeef->pl_unlock();
        GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(mdl), &iter);
        conv->items[i].row = gtk_tree_row_reference_new(GTK_TREE_MODEL(mdl), path);
        gtk_tree_path_free(path);
        char outpath[PATH_MAX];
        converter_plugin->get_output_path(items[i], encoder_preset, rootfolder, outfolder, outfile, use_source_folder, outpath, sizeof(outpath));
        conv->items[i].outpath = strdup(outpath);
        conv->items[i].item = items[i];
        conv->items[i].status = LIST_STATUS_WAITING;
    }
    deadbeef->conf_unlock();
    free(rootfolder);

    g_signal_connect(gtk_tree_view_get_selection(list), "changed", G_CALLBACK(on_selection_changed), conv->items);
    g_signal_connect_swapped(progress, "destroy", G_CALLBACK(clear_context), conv);
    g_signal_connect(progress, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    const int rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(mdl), NULL);
    GtkScrolledWindow *scrollwin = GTK_SCROLLED_WINDOW(lookup_widget(progress, "scrolled_window"));
    gtk_scrolled_window_set_policy(scrollwin, GTK_POLICY_NEVER, rows > 5 ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER);
    gtk_widget_set_size_request(GTK_WIDGET(scrollwin), -1, rows > 5 ? 100 : -1);
    gtk_widget_show(progress);

    const int numthreads = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(dlg, "numthreads")));
    for (size_t i = 0; i < (rows > numthreads ? numthreads : rows); i++) {
        intptr_t tid = deadbeef->thread_start(converter_worker, conv);
        deadbeef->thread_detach(tid);
    }

    free(items);
    return 0;
}

static void
init_encoder_dlg_from_preset(ddb_encoder_preset_t *preset, GtkWidget *title_combo)
{
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(title_combo))), preset->title);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(title_combo, "extension")), preset->extension);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(title_combo, "encoder_cmd")), preset->encoder);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(title_combo, "temporary_file")), preset->method);
    gtk_combo_box_set_active(GTK_COMBO_BOX(lookup_widget(title_combo, "id3v2_version")), preset->id3v2_version);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(title_combo, "id3v1")), preset->tag_id3v1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(title_combo, "id3v2")), preset->tag_id3v2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(title_combo, "flac")), preset->tag_flac);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(title_combo, "oggvorbis")), preset->tag_oggvorbis);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(title_combo, "apev2")), preset->tag_apev2);

    GtkWidget *builtin_button = lookup_widget(title_combo, "encpreset_default");
    gtk_widget_set_visible(gtk_widget_get_parent(builtin_button), preset->builtin);
    gtk_widget_set_sensitive(builtin_button, preset->builtin == DDB_PRESET_MODIFIED);
}

static void
on_default_clicked(GtkButton *button, ddb_encoder_preset_t **preset)
{
    ddb_encoder_preset_t *p = converter_plugin->encoder_preset_load_builtin((*preset)->title);
    if (p) {
        p->builtin = DDB_PRESET_BUILTIN;
        init_encoder_dlg_from_preset(p, lookup_widget(GTK_WIDGET(button), "title"));
        converter_plugin->encoder_preset_free(p);
    }
}

static void
on_encoder_default_field_changed(GtkWidget *widget, GtkWidget *default_button)
{
    gtk_widget_set_sensitive(default_button, gtk_widget_get_visible(gtk_widget_get_parent(default_button)));
}

static void
on_entry_changed(GtkWidget *widget, ddb_encoder_preset_t **preset)
{
    GtkWidget *title_combo = lookup_widget(widget, "title");
    const char *encoder_title = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(title_combo));
    const int preset_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(title_combo));
    ddb_encoder_preset_t *new = preset_idx == -1 ? converter_plugin->encoder_preset_get(encoder_title) : converter_plugin->encoder_preset_get_for_idx(preset_idx);
    if (widget == title_combo && preset_idx != -1) {
        *preset = new;
        init_encoder_dlg_from_preset(converter_plugin->encoder_preset_get_for_idx(preset_idx), title_combo);
    }

    const char *extension = gtk_entry_get_text(GTK_ENTRY(lookup_widget(widget, "extension")));

    gtk_widget_set_sensitive(lookup_widget(widget, "encpreset_save"), *encoder_title && *extension && (new == *preset || !new && !(*preset)->builtin));
    gtk_widget_set_sensitive(lookup_widget(widget, "encpreset_new"), *encoder_title && *extension && !new);
    gtk_widget_set_sensitive(lookup_widget(widget, "encpreset_delete"), *preset && !(*preset)->builtin);
}

static void
fill_encoder_presets(GtkComboBox *combo)
{
    GtkListStore *mdl = GTK_LIST_STORE(gtk_combo_box_get_model(combo));
    gtk_list_store_clear(mdl);

    GtkTreeIter iter;
    const char *builtin_pattern = _("[Built-in] %s");
    for (ddb_encoder_preset_t *p = converter_plugin->encoder_preset_get_for_idx(0); p; p = converter_plugin->encoder_preset_get_next(p)) {
        const char *pattern = p->builtin ? builtin_pattern : "%s";
        char title[strlen(pattern) + strlen(p->title)];
        sprintf(title, pattern, p->title);
        gtk_list_store_insert_with_values(mdl, &iter, -1, 0, title, -1);
    }

    deadbeef->conf_lock();
    const char *encoder_title = deadbeef->conf_get_str_fast("converter.encoder_title", NULL);
    const int idx = encoder_title ? converter_plugin->encoder_preset_get_idx(encoder_title) : -1;
    deadbeef->conf_unlock();
    gtk_combo_box_set_active(combo, idx < 0 ? 0 : idx);
}

static ddb_encoder_preset_t *
save_encoder_preset(GtkWidget *dlg)
{
    ddb_encoder_preset_t *p = converter_plugin->encoder_preset_alloc();
    if (!p) {
        return NULL;
    }

    GtkWidget *default_button = lookup_widget(dlg, "encpreset_default");
    if (gtk_widget_get_sensitive(default_button)) {
        p->builtin = DDB_PRESET_MODIFIED;
    }
    else if (gtk_widget_get_visible(gtk_widget_get_parent(default_button))) {
        p->builtin = DDB_PRESET_BUILTIN;
    }
    else {
        p->builtin = DDB_PRESET_CUSTOM;
    }

    p->title = strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(lookup_widget(dlg, "title"))))));
    p->extension = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(dlg, "extension"))));
    p->encoder = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(dlg, "encoder_cmd"))));
    p->method = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "temporary_file"))) ? DDB_ENCODER_METHOD_FILE : DDB_ENCODER_METHOD_PIPE;
    p->id3v2_version = gtk_combo_box_get_active(GTK_COMBO_BOX(lookup_widget(dlg, "id3v2_version")));
    p->tag_id3v2 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "id3v2")));
    p->tag_id3v1 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "id3v1")));
    p->tag_apev2 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "apev2")));
    p->tag_flac = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "flac")));
    p->tag_oggvorbis = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dlg, "oggvorbis")));

    ddb_encoder_preset_t *saved = converter_plugin->encoder_preset_save(p);
    if (!saved) {
        alert_dialog(GTK_WINDOW(gtkui_plugin->get_mainwin()), _("Failed to save encoder preset"), _("Check preset folder permissions or free up some disk space, then save again"));
    }

    return saved;
}

static int
edit_encoder_preset(GtkWidget *dlg, ddb_encoder_preset_t **preset_ptr)
{
    ddb_encoder_preset_t *orig = *preset_ptr;
    while (TRUE) {
        const int res = gtk_dialog_run(GTK_DIALOG(dlg));
        switch (res) {
            case GTK_RESPONSE_REJECT: {
                ddb_encoder_preset_t *preset = *preset_ptr;
                const int remove_res = confirm_dialog(GTK_WINDOW(dlg), _("You are about to permanently delete this encoder preset"), preset->title, _("_Delete"));
                if (remove_res == GTK_RESPONSE_YES) {
                    converter_plugin->encoder_preset_remove(preset);
                    deadbeef->conf_remove_items("converter.encoder_title");
                    return GTK_RESPONSE_OK;
                }
                break;
            }
            case GTK_RESPONSE_ACCEPT: {
                ddb_encoder_preset_t *preset = *preset_ptr;
                ddb_encoder_preset_t *saved = save_encoder_preset(dlg);
                if (saved) {
                    if (orig == preset && strcmp(saved->title, preset->title)) {
                        deadbeef->conf_set_str("converter.encoder_title", saved->title);
                        converter_plugin->encoder_preset_remove(preset);
                    }
                    return GTK_RESPONSE_OK;
                }
                break;
            }
            case GTK_RESPONSE_APPLY: {
                if (save_encoder_preset(dlg)) {
                    return GTK_RESPONSE_OK;
                }
                break;
            }
            case GTK_RESPONSE_DELETE_EVENT:
            case GTK_RESPONSE_CANCEL:
            default:
                return res;
        }
    }
}

static void
on_edit_encoder_presets_clicked(GtkWidget *button, GtkComboBox *encoder)
{
    ddb_encoder_preset_t *preset;

    GtkWidget *dlg = create_encpreset_editor();
    gtk_window_set_title(GTK_WINDOW(dlg), _("Edit encoder"));
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(gtk_widget_get_toplevel(button)));

    GtkWidget *encoder_cmd = lookup_widget(dlg, "encoder_cmd");
    GtkWidget *help_link = gtk_link_button_new_with_label("http://github.com/Alexey-Yakovenko/deadbeef/wiki/Encoder-Command-Line", _("Help"));
    gtk_widget_set_tooltip_text(help_link, _("%o substitutes full output path\n%i substitutes temporary file\nClick for more help"));
    gtk_widget_show(help_link);
    gtk_box_pack_start(GTK_BOX(gtk_widget_get_parent(encoder_cmd)), help_link, FALSE, FALSE, 0);
    gtk_widget_set_can_default(help_link, FALSE);
    gtk_button_set_focus_on_click(GTK_BUTTON(help_link), FALSE);

    GtkComboBox *title_combo = GTK_COMBO_BOX(lookup_widget(dlg, "title"));
    g_signal_connect(title_combo, "changed", G_CALLBACK(on_entry_changed), &preset);
    GtkEntry *title_entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(title_combo)));
    gtk_entry_set_activates_default(title_entry, TRUE);
    gtk_entry_set_max_length(GTK_ENTRY(title_entry), NAME_MAX-4);
    fill_encoder_presets(title_combo);

    GtkWidget *default_button = lookup_widget(dlg, "encpreset_default");
    GtkWidget *extension = lookup_widget(dlg, "extension");
    gtk_entry_set_max_length(GTK_ENTRY(extension), NAME_MAX-2);
    g_signal_connect(extension, "changed", G_CALLBACK(on_entry_changed), &preset);
    g_signal_connect(encoder_cmd, "changed", G_CALLBACK(on_encoder_default_field_changed), default_button);
    g_signal_connect(extension, "changed", G_CALLBACK(on_encoder_default_field_changed), default_button);
    g_signal_connect(lookup_widget(dlg, "temporary_file"), "toggled", G_CALLBACK(on_encoder_default_field_changed), default_button);
    g_signal_connect(default_button, "clicked", G_CALLBACK(on_default_clicked), &preset);

    const int res = edit_encoder_preset(dlg, &preset);
    if (res == GTK_RESPONSE_OK) {
        fill_encoder_presets(encoder);
    }
    gtk_widget_destroy(dlg);
    deadbeef->conf_save();
}

static void
set_list_cursor(GtkTreeView *list, GtkTreePath *path)
{
    gtk_tree_view_set_cursor(list, path, NULL, FALSE);
    gtk_tree_view_scroll_to_cell(list, path, NULL, TRUE, 0.5, 0.5);
    gtk_tree_path_free(path);
}

static void
on_dsp_add_item_activate(GtkWidget *widget, DB_dsp_t *dsp)
{
    ddb_dsp_context_t *p = dsp->open();
    if (p) {
        GtkTreeView *list = GTK_TREE_VIEW(lookup_widget(widget, "plugins"));
        GtkListStore *mdl = GTK_LIST_STORE(gtk_tree_view_get_model(list));
        GtkTreeIter iter;
        gtk_list_store_insert_with_values(mdl, &iter, -1, 0, p->plugin->plugin.name, 1, p, -1);
        set_list_cursor(list, gtk_tree_model_get_path(GTK_TREE_MODEL(mdl), &iter));
        gtk_widget_set_sensitive(lookup_widget(widget, "remove"), TRUE);
        gtk_widget_set_sensitive(lookup_widget(widget, "configure"), TRUE);
    }
}

static void
menu_set_position(GtkWidget *menu, gint *x, gint *y, gboolean *push_in, GtkWidget *button)
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(button, &allocation);

    GtkRequisition requisition;
    gtk_widget_size_request(menu, &requisition);
    if (allocation.width > requisition.width) {
        gtk_widget_set_size_request(menu, allocation.width, -1);
    }

    gdk_window_get_origin(gtk_widget_get_window(button), x, y);
    *x += allocation.x;
    *y += allocation.y + allocation.height;
}

static void
on_dsp_plugins_list_add_clicked(GtkWidget *menuitem, GtkMenu *menu)
{
    gtk_menu_popup(menu, NULL, NULL, (GtkMenuPositionFunc)menu_set_position, menuitem, 0, gtk_get_current_event_time());
}

static gboolean
on_dsp_plugins_list_clicked(GtkWidget *list, GdkEvent *event, GtkWidget *menu)
{
    if (event->button.button == 3) {
        const int rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(list)), NULL);
        gtk_widget_set_sensitive(lookup_widget(menu, "menu_remove"), rows);
        gtk_widget_set_sensitive(lookup_widget(menu, "menu_configure"), rows);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, list, 3, gtk_get_current_event_time());
    }

    return FALSE;
}

static void
on_dsp_add_plugin_clicked(GtkWidget *button, GtkMenu *menu)
{
    gtk_menu_popup(menu, NULL, NULL, (GtkMenuPositionFunc)menu_set_position, button, 0, gtk_get_current_event_time());
}

static void
on_dsp_remove_plugin_clicked(GtkWidget *button, GtkTreeView *list)
{
    GtkTreeIter iter;
    GtkTreeModel *mdl;
    gtk_tree_selection_get_selected(gtk_tree_view_get_selection(list), &mdl, &iter);
    ddb_dsp_context_t *dsp;
    gtk_tree_model_get(mdl, &iter, 1, &dsp, -1);
    const gboolean valid_iter = gtk_list_store_remove(GTK_LIST_STORE(mdl), &iter);

    const int rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(mdl), NULL);
    if (rows > 0) {
        GtkTreePath *path = valid_iter ? gtk_tree_model_get_path(mdl, &iter) : gtk_tree_path_new_from_indices(rows-1, -1);
        set_list_cursor(list, path);
    }
    else {
        gtk_widget_set_sensitive(lookup_widget(button, "remove"), FALSE);
        gtk_widget_set_sensitive(lookup_widget(button, "configure"), FALSE);
    }

    dsp->plugin->close(dsp);
}

static ddb_dsp_context_t *current_dsp_context = NULL;

static void
dsp_ctx_set_param (const char *key, const char *value)
{
    current_dsp_context->plugin->set_param(current_dsp_context, atoi(key), value);
}

static void
dsp_ctx_get_param (const char *key, char *value, int len, const char *def)
{
    *value = '\0';
    strncat(value, def, len);
    current_dsp_context->plugin->get_param(current_dsp_context, atoi(key), value, len);
}

static void
on_dsp_configure_plugin_clicked(GtkButton *button, GtkTreeView *list)
{
    GtkTreeIter iter;
    GtkTreeModel *mdl;
    gtk_tree_selection_get_selected(gtk_tree_view_get_selection(list), &mdl, &iter);
    ddb_dsp_context_t *dsp;
    gtk_tree_model_get(mdl, &iter, 1, &dsp, -1);
    current_dsp_context = dsp;
    ddb_dialog_t conf = {
        .title = dsp->plugin->plugin.name,
        .layout = dsp->plugin->configdialog,
        .set_param = dsp_ctx_set_param,
        .get_param = dsp_ctx_get_param,
        .parent = GTK_DIALOG(gtk_widget_get_toplevel(GTK_WIDGET(list)))
    };
    gtkui_plugin->gui.run_dialog(&conf, 0, NULL, NULL);
    current_dsp_context = NULL;
}

static void
on_dsp_plugin_up_clicked(GtkButton *button, GtkTreeView *list)
{
    GtkTreeIter iter;
    GtkTreeModel *mdl;
    gtk_tree_selection_get_selected(gtk_tree_view_get_selection(list), &mdl, &iter);
    GtkTreeIter iter_prev = iter;
    if (gtk_tree_model_iter_previous(mdl, &iter_prev)) {
        gtk_list_store_swap(GTK_LIST_STORE(mdl), &iter, &iter_prev);
    }
}

static void
on_dsp_plugin_down_clicked(GtkButton *button, GtkTreeView *list)
{
    GtkTreeIter iter;
    GtkTreeModel *mdl;
    gtk_tree_selection_get_selected(gtk_tree_view_get_selection(list), &mdl, &iter);
    GtkTreeIter iter_next = iter;
    if (gtk_tree_model_iter_next(mdl, &iter_next)) {
        gtk_list_store_swap(GTK_LIST_STORE(mdl), &iter, &iter_next);
    }
}

static void
clear_dsp_plugin_chain_list(GtkTreeView *list)
{
    GtkTreeModel *mdl = gtk_tree_view_get_model(list);
    GtkTreeIter iter;
    for (gboolean valid_iter = gtk_tree_model_get_iter_first(mdl, &iter); valid_iter; valid_iter = gtk_tree_model_iter_next(mdl, &iter)) {
        ddb_dsp_context_t *dsp;
        gtk_tree_model_get(mdl, &iter, 1, &dsp, -1);
        dsp->plugin->close(dsp);
    }
}

static void
fill_dsp_plugin_chain_list(GtkTreeView *list, ddb_dsp_context_t *dsp)
{
    GtkListStore *mdl = GTK_LIST_STORE(gtk_tree_view_get_model(list));
    gtk_list_store_clear(mdl);
    GtkTreeIter iter;
    while (dsp) {
        ddb_dsp_context_t *store_dsp = converter_plugin->dsp_plugin_duplicate(dsp);
        if (store_dsp) {
            gtk_list_store_insert_with_values(mdl, &iter, -1, 0, store_dsp->plugin->plugin.name, 1, store_dsp, -1);
        }
        dsp = dsp->next;
    }
}

static void
init_dsp_dlg_from_preset(ddb_dsp_preset_t *preset, GtkWidget *title_combo)
{
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(title_combo))), preset->title);
    GtkTreeView *list = GTK_TREE_VIEW(lookup_widget(title_combo, "plugins"));
    clear_dsp_plugin_chain_list(list);
    gtk_widget_set_sensitive(lookup_widget(title_combo, "remove"), !!preset->chain);
    gtk_widget_set_sensitive(lookup_widget(title_combo, "configure"), !!preset->chain);
    fill_dsp_plugin_chain_list(list, preset->chain);
    if (preset->chain) {
        set_list_cursor(list, gtk_tree_path_new_first());
    }
}

static void
on_dsp_title_changed(GtkWidget *widget, ddb_dsp_preset_t **preset)
{
    GtkWidget *title_combo = lookup_widget(widget, "title");
    const char *dsp_title = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(title_combo));
    const int idx = gtk_combo_box_get_active(GTK_COMBO_BOX(title_combo));
    ddb_dsp_preset_t *new = idx == -1 ? converter_plugin->dsp_preset_get(dsp_title) : converter_plugin->dsp_preset_get_for_idx(idx);
    if (idx != -1) {
        *preset = new;
        init_dsp_dlg_from_preset(new, title_combo);
    }

    gtk_widget_set_sensitive(lookup_widget(widget, "dsppreset_save"), *dsp_title && (new == *preset || !new));
    gtk_widget_set_sensitive(lookup_widget(widget, "dsppreset_new"), *dsp_title && !new);
}

static int
fill_dsp_presets(GtkComboBox *combo)
{
    GtkListStore *mdl = GTK_LIST_STORE(gtk_combo_box_get_model(combo));
    gtk_list_store_clear(mdl);
    GtkTreeIter iter;
    for (ddb_dsp_preset_t *p = converter_plugin->dsp_preset_get_for_idx(0); p; p = converter_plugin->dsp_preset_get_next(p)) {
        gtk_list_store_insert_with_values(mdl, &iter, -1, 0, p->title, -1);
    }

    deadbeef->conf_lock();
    const char *dsp_title = deadbeef->conf_get_str_fast("converter.dsp_title", NULL);
    const int idx = dsp_title ? converter_plugin->dsp_preset_get_idx(dsp_title) : -1;
    deadbeef->conf_unlock();
    gtk_combo_box_set_active(combo, idx < 0 ? 0 : idx);
    return idx;
}

static ddb_dsp_preset_t *
save_dsp_preset(GtkWidget *dlg)
{
    ddb_dsp_preset_t *p = converter_plugin->dsp_preset_alloc();
    if (!p) {
        return NULL;
    }

    p->title = strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(lookup_widget(dlg, "title"))))));

    GtkTreeModel *mdl = gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(dlg, "plugins")));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(mdl, &iter)) {
        ddb_dsp_context_t *dsp;
        gtk_tree_model_get(mdl, &iter, 1, &dsp, -1);
        p->chain = dsp;
        ddb_dsp_context_t *tail = dsp;
        while (gtk_tree_model_iter_next(mdl, &iter)) {
            gtk_tree_model_get(mdl, &iter, 1, &dsp, -1);
            tail->next = dsp;
            tail = dsp;
        }
        tail->next = NULL;
    }

    ddb_dsp_preset_t *saved = converter_plugin->dsp_preset_save(p);
    if (!saved) {
        alert_dialog(GTK_WINDOW(gtkui_plugin->get_mainwin()), _("Failed to save DSP preset"), _("Check preset folder permissions or free up some disk space"));
    }

    return saved;
}

static int
edit_dsp_preset(GtkWidget *dlg, ddb_dsp_preset_t *preset)
{
    ddb_dsp_preset_t *orig = preset;
    while (TRUE) {
        const int res = gtk_dialog_run(GTK_DIALOG(dlg));
        switch (res) {
            case GTK_RESPONSE_REJECT: {
                const int remove_res = confirm_dialog(GTK_WINDOW(dlg), _("You are about to permanently delete this DSP preset"), preset->title, _("_Delete"));
                if (remove_res == GTK_RESPONSE_YES) {
                    converter_plugin->dsp_preset_remove(preset);
                    clear_dsp_plugin_chain_list(GTK_TREE_VIEW(lookup_widget(dlg, "plugins")));
                    deadbeef->conf_remove_items("converter.dsp_title");
                    return GTK_RESPONSE_OK;
                }
                break;
            }
            case GTK_RESPONSE_ACCEPT: {
                ddb_dsp_preset_t *saved = save_dsp_preset(dlg);
                if (saved) {
                    if (orig == preset && strcmp(saved->title, preset->title)) {
                        deadbeef->conf_set_str("converter.dsp_title", saved->title);
                        converter_plugin->dsp_preset_remove(preset);
                    }
                    return GTK_RESPONSE_OK;
                }
                break;
            }
            case GTK_RESPONSE_APPLY: {
                if (save_dsp_preset(dlg)) {
                    return GTK_RESPONSE_OK;
                }
                break;
            }
            case GTK_RESPONSE_CANCEL:
            case GTK_RESPONSE_DELETE_EVENT:
            default:
                clear_dsp_plugin_chain_list(GTK_TREE_VIEW(lookup_widget(dlg, "plugins")));
                return res;
        }
    }
}

static GtkWidget *
create_dsp_preset_dlg(ddb_dsp_preset_t **preset_ptr)
{
    GtkWidget *dlg = create_dsppreset_editor();
    gtk_window_set_title(GTK_WINDOW (dlg), _("Edit DSP preset"));
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);

    GtkTreeView *list = GTK_TREE_VIEW(lookup_widget(dlg, "plugins"));
    GtkCellRenderer *title_cell = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("", title_cell, "text", 0, NULL);
    gtk_tree_view_append_column(list, GTK_TREE_VIEW_COLUMN(col));
    GtkListStore *mdl = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_tree_view_set_model(list, GTK_TREE_MODEL(mdl));
    g_object_unref(mdl);

    GtkWidget *context_menu = create_context_menu();
    g_signal_connect(list, "button-press-event", G_CALLBACK(on_dsp_plugins_list_clicked), context_menu);
    gtk_menu_attach_to_widget(GTK_MENU(context_menu), GTK_WIDGET(list), NULL);
    GtkWidget *plugin_menu = gtk_menu_new();
    struct DB_dsp_s **dsp = deadbeef->plug_get_dsp_list();
    for (size_t i = 0; dsp[i]; i++) {
        GtkWidget *item = gtk_menu_item_new_with_label(dsp[i]->plugin.name);
        gtk_widget_show(item);
        gtk_container_add(GTK_CONTAINER(plugin_menu), item);
        g_signal_connect(item, "activate", G_CALLBACK(on_dsp_add_item_activate), dsp[i]);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(lookup_widget(context_menu, "menu_add")), plugin_menu);
    g_signal_connect(lookup_widget(context_menu, "menu_remove"), "activate", G_CALLBACK(on_dsp_remove_plugin_clicked), list);
    g_signal_connect(lookup_widget(context_menu, "menu_configure"), "activate", G_CALLBACK(on_dsp_configure_plugin_clicked), list);
    g_signal_connect(lookup_widget(context_menu, "menu_up"), "activate", G_CALLBACK(on_dsp_plugin_up_clicked), list);
    g_signal_connect(lookup_widget(context_menu, "menu_down"), "activate", G_CALLBACK(on_dsp_plugin_down_clicked), list);

    GtkWidget *down_button = lookup_widget(dlg, "down");
    gtk_button_set_image(GTK_BUTTON(down_button), gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON));
    GtkWidget *add_button = lookup_widget(dlg, "add");
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_dsp_add_plugin_clicked), plugin_menu);
    g_signal_connect(add_button, "activate", G_CALLBACK(on_dsp_add_plugin_clicked), plugin_menu);
    GtkWidget *remove_button = lookup_widget(dlg, "remove");
    g_signal_connect(remove_button, "clicked", G_CALLBACK(on_dsp_remove_plugin_clicked), list);
    GtkWidget *configure_button = lookup_widget(dlg, "configure");
    g_signal_connect(configure_button, "clicked", G_CALLBACK(on_dsp_configure_plugin_clicked), list);
    g_signal_connect(lookup_widget(dlg, "up"), "clicked", G_CALLBACK(on_dsp_plugin_up_clicked), list);
    g_signal_connect(down_button, "clicked", G_CALLBACK(on_dsp_plugin_down_clicked), list);

    GtkComboBox *title_combo = GTK_COMBO_BOX(lookup_widget(dlg, "title"));
    GtkEntry *title_entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(title_combo)));
    gtk_entry_set_max_length(title_entry, NAME_MAX-4);
    gtk_entry_set_activates_default(title_entry, TRUE);
    g_signal_connect(title_combo, "changed", G_CALLBACK(on_dsp_title_changed), preset_ptr);
    if (fill_dsp_presets(title_combo) == -1) {
        gtk_widget_set_sensitive(remove_button, FALSE);
        gtk_widget_set_sensitive(configure_button, FALSE);
        gtk_widget_set_sensitive(lookup_widget(dlg, "dsppreset_delete"), FALSE);
    }

    return dlg;
}

static void
on_edit_dsp_presets_clicked(GtkWidget *button, GtkComboBox *dsp)
{
    ddb_dsp_preset_t *preset = NULL;

    GtkWidget *dlg = create_dsp_preset_dlg(&preset);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(gtk_widget_get_toplevel(button)));

    const int res = edit_dsp_preset(dlg, preset);
    if (res == GTK_RESPONSE_OK) {
        fill_dsp_presets(dsp);
    }

    gtk_widget_destroy(dlg);
    deadbeef->conf_save();
}

static void
set_output_path_label(GtkWidget *widget, DB_playItem_t **items)
{
    char outpath[PATH_MAX] = "";
    deadbeef->conf_lock();
    const char *encoder_title = deadbeef->conf_get_str_fast("converter.encoder_title", NULL);
    ddb_encoder_preset_t *preset = encoder_title ? converter_plugin->encoder_preset_get(encoder_title) : NULL;
    if (preset) {
        const char *out_folder = get_output_folder();
        const char *output_file = get_output_pattern();
        char *root_folder = get_root_folder(widget, items);
        const int use_source_folder = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "use_source_folder")));
        converter_plugin->get_output_path(items[0], preset, root_folder, out_folder, output_file, use_source_folder, outpath, sizeof(outpath));
        if (root_folder) {
            free(root_folder);
        }
    }
    deadbeef->conf_unlock();
    gtk_label_set_text(GTK_LABEL(lookup_widget(widget, "path_label")), outpath);
}

static void
on_output_folder_changed(GtkEntry *entry, DB_playItem_t **item)
{
    deadbeef->conf_set_str("converter.output_folder", gtk_entry_get_text(entry));
    set_output_path_label(GTK_WIDGET(entry), item);
}

static void
on_converter_output_browse_clicked(GtkButton *button, GtkWindow *win)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new(_("Select folder..."), win, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), win);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dlg), FALSE);

    deadbeef->conf_lock();
    const char *lastdir = deadbeef->conf_get_str_fast("converter.lastdir", NULL);
    if (lastdir) {
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dlg), lastdir);
    }
    else {
        const char *out_folder = get_output_folder();
        char dir[sizeof("file://") + strlen(out_folder)];
        sprintf(dir, "file://%s", out_folder);
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dlg), dir);
    }
    deadbeef->conf_unlock();

    int response = gtk_dialog_run(GTK_DIALOG(dlg));
    gchar *folder = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dlg));
    if (folder) {
        deadbeef->conf_set_str("converter.lastdir", folder);
        g_free(folder);
    }
    if (response == GTK_RESPONSE_OK) {
        folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        if (folder) {
            gtk_entry_set_text(GTK_ENTRY(lookup_widget(GTK_WIDGET(win), "output_folder")), folder);
            g_free(folder);
        }
    }
    gtk_widget_destroy(dlg);
    deadbeef->conf_save();
}

static void
on_use_source_folder_toggled(GtkToggleButton *togglebutton, DB_playItem_t **items)
{
    const int active = gtk_toggle_button_get_active(togglebutton);
    deadbeef->conf_set_int("converter.use_source_folder", active);
    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(togglebutton), "output_folder"), !active);
    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(togglebutton), "converter_output_browse"), !active);
    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(togglebutton), "preserve_folders"), !active);
    set_output_path_label(GTK_WIDGET(togglebutton), items);
}

static void
on_preserve_folders_toggled(GtkToggleButton *togglebutton, DB_playItem_t **items)
{
    const int active = gtk_toggle_button_get_active(togglebutton);
    deadbeef->conf_set_int("converter.preserve_folder_structure", active);
    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(togglebutton), "use_source_folder"), !active);
    set_output_path_label(GTK_WIDGET(togglebutton), items);
}

static void
on_output_file_changed(GtkEntry *entry, DB_playItem_t **items)
{
    deadbeef->conf_set_str("converter.output_file", gtk_entry_get_text(entry));
    set_output_path_label(GTK_WIDGET(entry), items);
}

static void
on_encoder_changed(GtkComboBox *combobox, DB_playItem_t **items)
{
    const int idx = gtk_combo_box_get_active(combobox);
    gtk_widget_set_sensitive(lookup_widget(GTK_WIDGET(combobox), "converter_convert"), idx != -1);
    ddb_encoder_preset_t *preset = idx == -1 ? NULL : converter_plugin->encoder_preset_get_for_idx(idx);
    if (preset) {
        deadbeef->conf_set_str("converter.encoder_title", preset->title);
        set_output_path_label(GTK_WIDGET(combobox), items);
    }
}

static void
on_numthreads_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
    deadbeef->conf_set_int("converter.threads", gtk_spin_button_get_value_as_int(spinbutton));
}

static void
on_overwrite_action_changed(GtkComboBox *combobox, gpointer user_data)
{
    deadbeef->conf_set_int("converter.overwrite_action", gtk_combo_box_get_active(combobox));
}

static void
on_apply_dsp_toggled(GtkToggleButton *button, GtkComboBox *dsp_combo)
{
    const int dsp_idx = gtk_combo_box_get_active(dsp_combo);
    const int apply_dsp = gtk_toggle_button_get_active(button);
    gtk_widget_set_sensitive(GTK_WIDGET(dsp_combo), apply_dsp && dsp_idx != -1);
    if (dsp_idx != -1) {
        deadbeef->conf_set_int("converter.apply_dsp", apply_dsp);
    }
}

static void
on_dsp_preset_changed(GtkComboBox *combobox, gpointer apply_dsp)
{
    const int idx = gtk_combo_box_get_active(combobox);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(apply_dsp), idx != -1 && deadbeef->conf_get_int("converter.apply_dsp", 0));
    gtk_widget_set_sensitive(GTK_WIDGET(apply_dsp), idx != -1);
    ddb_dsp_preset_t *preset = idx == -1 ? NULL : converter_plugin->dsp_preset_get_for_idx(idx);
    if (preset) {
        deadbeef->conf_set_str("converter.dsp_title", preset->title);
    }
}

static void
on_force_format_toggled(GtkToggleButton *button, GtkWidget *output_format_combo)
{
    const int force_format = gtk_toggle_button_get_active(button);
    gtk_widget_set_sensitive(output_format_combo, force_format);
    deadbeef->conf_set_int("converter.force_format", force_format);
}

static void
on_converter_output_format_changed(GtkComboBox *combobox, gpointer user_data)
{
    deadbeef->conf_set_int("converter.output_format", gtk_combo_box_get_active(combobox));
}

static int
can_use_source_folder(DB_playItem_t **items)
{
    for (DB_playItem_t **item = items; *item; item++) {
        deadbeef->pl_lock();
        const char *uri = deadbeef->pl_find_meta(*item, ":URI");
        const int can_use_source_folder = uri[0] == '/' && strncmp(uri, "/dev", 4) || !strncmp(uri, "file:///", 8);
        deadbeef->pl_unlock();
        if (!can_use_source_folder) {
            return 0;
        }
    }
    return 1;
}

static gboolean
converter_show_cb (void *data)
{
    DB_playItem_t **items = (DB_playItem_t **)data;
    GtkWidget *dlg = create_converterdlg();
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(gtkui_plugin->get_mainwin()));

    if (can_use_source_folder(items)) {
        GtkWidget *use_source_folder_widget = lookup_widget(dlg, "use_source_folder");
        gtk_widget_set_visible(use_source_folder_widget, TRUE);
        g_signal_connect(use_source_folder_widget, "toggled", G_CALLBACK(on_use_source_folder_toggled), items);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_source_folder_widget), deadbeef->conf_get_int("converter.use_source_folder", 0));
    }

    if (items[1]) {
        GtkWidget *preserve_folders_widget = lookup_widget(dlg, "preserve_folders");
        gtk_widget_set_visible(preserve_folders_widget, TRUE);
        g_signal_connect(preserve_folders_widget, "toggled", G_CALLBACK(on_preserve_folders_toggled), items);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(preserve_folders_widget), deadbeef->conf_get_int("converter.preserve_folder_structure", 0));
    }

    GtkWidget *output_folder_widget = lookup_widget(dlg, "output_folder");
    GtkWidget *output_file_widget = lookup_widget(dlg, "output_file");
    deadbeef->conf_lock ();
    gtk_entry_set_text(GTK_ENTRY(output_folder_widget), get_output_folder());
    gtk_entry_set_text(GTK_ENTRY(output_file_widget), get_output_pattern());
    deadbeef->conf_unlock();
    g_signal_connect(output_folder_widget, "changed", G_CALLBACK(on_output_folder_changed), items);
    g_signal_connect(output_file_widget, "changed", G_CALLBACK(on_output_file_changed), items);
    g_signal_connect(lookup_widget(dlg, "converter_output_browse"), "clicked", G_CALLBACK(on_converter_output_browse_clicked), dlg);

    GtkWidget *help_link = gtk_link_button_new_with_label("http://github.com/Alexey-Yakovenko/deadbeef/wiki/Title-formatting", _("Help"));
    gtk_widget_set_tooltip_text(help_link, _("%a = artist name\n%b = album title\n%t = track title\n%g = genre\n%n = track #\nClick for full list"));
    gtk_widget_show(help_link);
    gtk_box_pack_start(GTK_BOX(gtk_widget_get_parent(output_file_widget)), help_link, FALSE, FALSE, 0);
    gtk_button_set_focus_on_click(GTK_BUTTON(help_link), FALSE);
    gtk_widget_set_can_default(help_link, FALSE);

    GtkComboBox *overwrite_combo = GTK_COMBO_BOX(lookup_widget(dlg, "overwrite_action"));
    gtk_combo_box_set_active(overwrite_combo, deadbeef->conf_get_int("converter.overwrite_action", 0));
    g_signal_connect(overwrite_combo, "changed", G_CALLBACK(on_overwrite_action_changed), items);

    GtkSpinButton *numthreads = GTK_SPIN_BUTTON(lookup_widget(dlg, "numthreads"));
    gtk_spin_button_set_value(numthreads, deadbeef->conf_get_int ("converter.threads", 1));
    g_signal_connect(numthreads, "changed", G_CALLBACK(on_numthreads_changed), items);

    GtkWidget *apply_dsp = lookup_widget(dlg, "apply_dsp");
    GtkWidget *converter_dsp_widget = lookup_widget(dlg, "dsp_preset");
    g_signal_connect(apply_dsp, "toggled", G_CALLBACK(on_apply_dsp_toggled), converter_dsp_widget);
    g_signal_connect(converter_dsp_widget, "changed", G_CALLBACK(on_dsp_preset_changed), apply_dsp);
    fill_dsp_presets(GTK_COMBO_BOX(converter_dsp_widget));
    g_signal_connect(lookup_widget(dlg, "edit_dsp_presets"), "clicked", G_CALLBACK(on_edit_dsp_presets_clicked), converter_dsp_widget);

    GtkWidget *output_format_combo = lookup_widget(dlg, "output_format");
    g_signal_connect(output_format_combo, "changed", G_CALLBACK(on_converter_output_format_changed), items);
    gtk_combo_box_set_active(GTK_COMBO_BOX(output_format_combo), deadbeef->conf_get_int("converter.output_format", 0));
    GtkWidget *force_format = lookup_widget(dlg, "force_format");
    g_signal_connect(force_format, "toggled", G_CALLBACK(on_force_format_toggled), output_format_combo);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(force_format), deadbeef->conf_get_int("converter.force_format", 0));

    gtk_widget_show(dlg);
    GtkWidget *encoder_widget = lookup_widget(dlg, "encoder");
    g_signal_connect(encoder_widget, "changed", G_CALLBACK(on_encoder_changed), items);
    fill_encoder_presets(GTK_COMBO_BOX(encoder_widget));
    g_signal_connect(lookup_widget(dlg, "edit_encoder_presets"), "clicked", G_CALLBACK(on_edit_encoder_presets_clicked), encoder_widget);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_OK || converter_process(dlg, items)) {
        for (DB_playItem_t **item = items; *item; item++) {
            deadbeef->pl_item_unref(*item);
        }
        free(items);
    }

    gtk_widget_destroy(dlg);
    deadbeef->conf_save();

    return FALSE;
}

static int
converter_show (DB_plugin_action_t *act, int ctx)
{
    GList *list = gtk_window_list_toplevels();
    while (list) {
        GtkWindow *w = GTK_WINDOW(list->data);
        if (gtk_window_get_window_type(w) == GTK_WINDOW_TOPLEVEL && gtk_widget_get_visible(GTK_WIDGET(w))) {
            const char *title = gtk_window_get_title(w);
            if (title && !strcmp(title, "Converter")) {
                gtk_window_present(w);
                g_list_free(list);
                return 0;
            }
        }
        list = list->next;
    }
    g_list_free(list);

    DB_playItem_t **items = NULL;
    deadbeef->pl_lock ();
    switch (ctx) {
        case DDB_ACTION_CTX_MAIN:
        case DDB_ACTION_CTX_SELECTION:
        {
            ddb_playlist_t *plt = deadbeef->plt_get_curr();
            if (plt) {
                const int count = deadbeef->plt_getselcount(plt);
                items = malloc(sizeof(DB_playItem_t *) * (count + 1));
                if (items) {
                    items[count] = NULL;
                    DB_playItem_t *it = deadbeef->pl_get_first(PL_MAIN);
                    DB_playItem_t **item = items;
                    while (it) {
                        if (deadbeef->pl_is_selected(it)) {
                            deadbeef->pl_item_ref(it);
                            *item++ = it;
                        }
                        DB_playItem_t *next = deadbeef->pl_get_next(it, PL_MAIN);
                        deadbeef->pl_item_unref(it);
                        it = next;
                    }
                }
                deadbeef->plt_unref(plt);
            }
            break;
        }
        case DDB_ACTION_CTX_PLAYLIST:
        {
            ddb_playlist_t *plt = deadbeef->plt_get_curr();
            if (plt) {
                const int count = deadbeef->plt_get_item_count(plt, PL_MAIN);
                items = malloc(sizeof(DB_playItem_t *) * (count + 1));
                if (items) {
                    items[count] = NULL;
                    DB_playItem_t *it = deadbeef->pl_get_first(PL_MAIN);
                    DB_playItem_t **item = items;
                    while (it) {
                        *item++ = it;
                        it = deadbeef->pl_get_next(it, PL_MAIN);
                    }
                }
                deadbeef->plt_unref(plt);
            }
            break;
        }
        case DDB_ACTION_CTX_NOWPLAYING:
        {
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
            if (it) {
                items = malloc(sizeof(DB_playItem_t *) * 2);
                if (items) {
                    items[0] = it;
                    items[1] = NULL;
                }
            }
        }
        break;
    }
    deadbeef->pl_unlock();
    if (!items) {
        return -1;
    }

    gdk_threads_add_idle(converter_show_cb, items);

    return 0;
}

static DB_plugin_action_t convert_action =
{
    .title = "Convert",
    .name = "convert",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK | DB_ACTION_ADD_MENU | DB_ACTION_DISABLED,
    .callback2 = converter_show,
    .next = NULL
};

static DB_plugin_action_t *
convgui_get_actions (DB_playItem_t *it)
{
    return &convert_action;
}

static int
deferred_connect(void *ctx)
{
    converter_plugin->load();
    convert_action.flags &= ~DB_ACTION_DISABLED;
    return FALSE;
}

static int
convgui_connect (void)
{
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id(DDB_GTKUI_PLUGIN_ID);
    converter_plugin = (ddb_converter_t *)deadbeef->plug_get_for_id("converter");
    if (!gtkui_plugin) {
        fprintf(stderr, "convgui: gtkui plugin not found\n");
        return -1;
    }
    if (!converter_plugin) {
        fprintf(stderr, "convgui: converter plugin not found\n");
        return -1;
    }
    if (!PLUG_TEST_COMPAT(&converter_plugin->misc.plugin, 2, 0)) {
        fprintf(stderr, "convgui: need converter>=2.0, but found %d.%d\n", converter_plugin->misc.plugin.version_major, converter_plugin->misc.plugin.version_minor);
        return -1;
    }
    gdk_threads_add_idle(deferred_connect, converter_plugin);
    return 0;
}

static int
convgui_disconnect (void)
{
    if (converter_plugin)
        converter_plugin->unload();
    return 0;
}

DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 5,
    .plugin.version_major = 2,
    .plugin.version_minor = 0,
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
    .plugin.disconnect = convgui_disconnect,
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
