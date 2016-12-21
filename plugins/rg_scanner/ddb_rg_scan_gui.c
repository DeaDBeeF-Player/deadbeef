/*
 * ddb_rg_scan_gui.c - GUI for the libEBUR128-based Replay Gain scanner plugin
 *                     for the DeaDBeeF audio player
 *
 * Copyright (c) 2015 Ivan Pilipenko
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "support.h"
#include "interface.h"
#include "ddb_misc_rg_scan.h"
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

DB_functions_t *deadbeef;

rg_scan_t *scanner_plugin;
ddb_gtkui_t *gtkui_plugin;

enum // used to populate results dialog
{
  COL_URI = 0,
  COL_TRG,
  COL_TPK,
  COL_ARG,
  COL_APK,
  NUM_COLS
} ;

static const char settings_dlg[] =
    "property \"Target db volume level\" entry rgscan.target 89.0;\n" \
    "property \"Number of threads (0 = auto)\" entry rgscan.num_threads 0;\n"
;

typedef struct {
    GtkWidget *scanres;

    DB_playItem_t **scan_items;
    float *track_gain;
    float *album_gain;
    float *track_peak;
    float *album_peak;

    GtkWidget *progress;
    GtkWidget *progress_entry;

    GtkWidget *results;
    GtkWidget *confirm;

    float targetdb;
    int num_items;
    int num_threads;
    int cancelled;

} scanner_ctx_t;

scanner_ctx_t *current_ctx;

typedef struct {
    GtkWidget *entry;
    char *text;
} update_progress_info_t;


static int
has_rg_tags (DB_playItem_t *track) {
    deadbeef->pl_lock();
    DB_metaInfo_t *tags = deadbeef->pl_get_metadata_head (track);
    deadbeef->pl_unlock();
    while (tags){
        if (strstr (tags->key, "REPLAYGAIN")){
            return 1;
        }
        tags = tags->next;
    }
    return 0;
}

static void 
rg_cleanup () {
    trace ("rg scan: cleaning up scanning object\n");

    if (current_ctx->track_gain) {
        free (current_ctx->track_gain);
    }
    if (current_ctx->track_peak) {
        free (current_ctx->track_peak);
    }
    if (current_ctx->album_gain) {
        free (current_ctx->album_gain);
    }
    if (current_ctx->album_peak) {
        free (current_ctx->album_peak);
    }
    free (current_ctx);
    current_ctx = NULL;
    trace ("rg scan: cleaning complete, exiting\n");
}


static gboolean
destroy_progress_cb (gpointer ctx) {
    gtk_widget_destroy (ctx);
    return FALSE;
}

static gboolean
destroy_results_cb (gpointer ctx) {
    gtk_widget_destroy (ctx);
    rg_cleanup ();
    return FALSE;
}

void
on_scan_progress_cancel (GtkDialog *dialog, gint response_id, gpointer user_data) {
    scanner_ctx_t *ctx = user_data;
    ctx->cancelled = 1;
    g_idle_add (destroy_progress_cb, ctx->progress);
}


static gboolean
update_progress_cb (gpointer ctx) {
    trace ("rg scan: update progress info ");
    update_progress_info_t *info = ctx;
    gtk_entry_set_text (GTK_ENTRY (info->entry), info->text);
    free (info->text);
    g_object_unref (info->entry);
    free (info);
    trace ("rg scan: done\n");
    return FALSE;
}


void
on_btn_apply_rg_released (GtkButton *button, gpointer user_data)
{
    deadbeef->background_job_increment ();
    scanner_ctx_t *scan = current_ctx;
    for (int i = 0; i < scan->num_items; ++i) {
        // TODO: show progress
        if (scanner_plugin->rg_apply (scan->scan_items[i], &scan->track_gain[i], &scan->track_peak[i], scan->album_gain, scan->album_peak)) {
            deadbeef->pl_lock();
            fprintf (stderr, "rg scan: failed to apply RG tags to %s\n", deadbeef->pl_find_meta (scan->scan_items[i], ":URI"));
            deadbeef->pl_unlock();
        }
        deadbeef->pl_item_unref (scan->scan_items[i]);
    }
    g_idle_add (destroy_results_cb, scan->results);
    deadbeef->background_job_decrement ();
}

void
on_btn_cancel_rg_released (GtkButton *button, gpointer user_data)
{
    scanner_ctx_t *ctx = current_ctx;
    ctx->cancelled = 1;
    for (int i = 0; i < ctx->num_items; ++i){
        deadbeef->pl_item_unref (ctx->scan_items[i]);
    }
    g_idle_add (destroy_results_cb, ctx->results);
}

static gboolean
results_cb (void *ctx) {
    scanner_ctx_t *scan = ctx;

    // we're done with scanning, destroy the progress dialog
    g_idle_add (destroy_progress_cb, scan->progress);

    // setup our results dialog
    GtkListStore *store;        // model storing our data
    GtkTreeIter it;             // iterator used for the model
    GtkWidget *treeview;        // our treewidget
    GtkCellRenderer *renderer;  // used to set appearance of the cell(s)

    // setup columns
    trace ("rg scan: num_cols: %d\n", NUM_COLS);
    store = gtk_list_store_new (NUM_COLS,       // number of columns
                                G_TYPE_STRING,  // track title/URI
                                G_TYPE_FLOAT,   // track gain
                                G_TYPE_FLOAT,   // track peak
                                G_TYPE_FLOAT,   // album gain
                                G_TYPE_FLOAT);  // album peak

    // fill rows with data
    for (int i = 0; i < scan->num_items; ++i){
        deadbeef->pl_lock ();
        gtk_list_store_append (store, &it);
        trace ("rg scan: %s | %f | %f | %f | %f\n", deadbeef->pl_find_meta (scan->scan_items[i], ":URI"), 
                                                     scan->track_gain[i],
                                                     scan->track_peak[i],
                                                     *scan->album_gain,
                                                     *scan->album_peak);
        gtk_list_store_set (store, &it,
                            COL_URI, deadbeef->pl_find_meta (scan->scan_items[i], ":URI"),
                            COL_TRG, (gfloat) scan->track_gain[i],
                            COL_TPK, (gfloat) scan->track_peak[i],
                            COL_ARG, (gfloat) *scan->album_gain,
                            COL_APK, (gfloat) *scan->album_peak,
                            -1);
        deadbeef->pl_unlock();
    }

    // get our treeview widget
    treeview = GTK_TREE_VIEW (lookup_widget (scan->results, "results_table"));

    // setup column headers
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                                 0,
                                                 "File",
                                                 renderer,
                                                 "text", COL_URI,
                                                 NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                                 1,
                                                 "Track Gain",
                                                 renderer,
                                                 "text", COL_TRG,
                                                 NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                                 2,
                                                 "Track Peak",
                                                 renderer,
                                                 "text", COL_TPK,
                                                 NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                                 3,
                                                 "Album Gain",
                                                 renderer,
                                                 "text", COL_ARG,
                                                 NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
                                                 4,
                                                 "Album Peak",
                                                 renderer,
                                                 "text", COL_APK,
                                                 NULL);

    // set model
    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

    // widget takes our model, no need to reference it
    g_object_unref (store);

    // resize colums to fit
    gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview));

    // show dialogue
    gtk_widget_show (scan->results);

    return FALSE;
}

static void
scanner_worker (void *ctx) {
    deadbeef->background_job_increment ();
    scanner_ctx_t *scan = ctx;

    // initialize RG result arrays
    scan->track_gain = (float *) malloc (scan->num_items * sizeof (float));
    scan->track_peak = (float *) malloc (scan->num_items * sizeof (float));
    scan->album_gain = (float *) malloc (sizeof (float));
    scan->album_peak = (float *) malloc (sizeof (float));

    update_progress_info_t *info = malloc (sizeof (update_progress_info_t));
    info->entry = scan->progress_entry;
    g_object_ref (info->entry);
    info->text = strdup ("Please wait...");
    g_idle_add (update_progress_cb, info);

    int result = -1;

    result = scanner_plugin->rg_scan (scan->scan_items, &scan->num_items, scan->track_gain, scan->track_peak, scan->album_gain, scan->album_peak, &scan->targetdb, &scan->num_threads, &scan->cancelled);

    if (result == 0)
    {
        trace ("rg scan: Track gains & peaks:\n");
        for (int i = 0; i < scan->num_items; ++i){
            deadbeef->pl_lock();
            fprintf (stdout, "rg scan: %s: %f %f\n", deadbeef->pl_find_meta (scan->scan_items[i], ":URI"), scan->track_gain[i], scan->track_peak[i]);
            deadbeef->pl_unlock();
        }
        trace ("rg scan: \nrg scan: Album gain/peak: %f %f\n", *scan->album_gain, *scan->album_peak);

        GtkWidget *res = create_dlgResults ();

        scan->results = res;

        trace ("rg scan: dialog created, callback will be called now\n");
        g_idle_add (results_cb, scan);
    }
    else { // something went wrong/user aborted, unref all playlist items
        for (int i = 0; i < scan->num_items; i++) {
            if (scan->scan_items[i]){
                deadbeef->pl_item_unref (scan->scan_items[i]);
            }
        }
        rg_cleanup ();
    }
    deadbeef->background_job_decrement ();
}

static gboolean
rg_scan_run_cb (void *data) {
    int ctx = (intptr_t)data;
    scanner_ctx_t *scan = malloc (sizeof (scanner_ctx_t));
    current_ctx = scan;
    memset (scan, 0, sizeof (scanner_ctx_t));

    deadbeef->pl_lock ();
    switch (ctx) {
    case DDB_ACTION_CTX_MAIN:
    case DDB_ACTION_CTX_SELECTION:
        {
            // copy list
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                scan->num_items = deadbeef->plt_getselcount (plt);
                if (0 < scan->num_items) {
                    scan->scan_items = malloc (sizeof (DB_playItem_t *) * scan->num_items);
                    if (scan->scan_items) {
                        int n = 0;
                        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
                        while (it) {
                            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                            deadbeef->pl_item_unref (it);
                            if (deadbeef->pl_is_selected (it)) {
                                deadbeef->pl_lock ();
                                if (!deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI"))) {
                                    fprintf (stderr, "rg scan: %s is not a local file, skipped\n", deadbeef->pl_find_meta (it, ":URI"));
                                    deadbeef->pl_unlock ();
                                    scan->num_items -= 1;
                                    it = next;
                                    continue;
                                }
                                deadbeef->pl_unlock ();
                                assert (n < scan->num_items);
                                deadbeef->pl_item_ref (it);
                                scan->scan_items[n++] = it;
                            }
                            it = next;
                        }
                    }
                }
            }
            break;
        }
    }
    deadbeef->pl_unlock ();

    scan->targetdb = deadbeef->conf_get_float ("rgscan.target", 89);
    scan->num_threads = deadbeef->conf_get_int("rgscan.num_threads", 0);

    /* define number of threads for rg_scan */
    if(scan->num_threads == 0)
    {
        long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
        if(num_cores <= 0)
        {
            num_cores = 1;
        }
        scan->num_threads = num_cores;
    }

    GtkWidget *progress = gtk_dialog_new_with_buttons (_("Scanning..."), GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    GtkWidget *vbox = gtk_dialog_get_content_area (GTK_DIALOG (progress));
    GtkWidget *entry = gtk_entry_new ();
    gtk_widget_set_size_request (entry, 400, -1);
    gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 12);

    g_signal_connect ((gpointer)progress, "response", G_CALLBACK (on_scan_progress_cancel), scan);

    gtk_widget_show (progress);

    scan->progress = progress;
    scan->progress_entry = entry;

    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Replay Gain Tags Present"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (gtkui_plugin->get_mainwin ()));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("One or more files already have Replay Gain information. Do you want to re-scan the files anyway?"));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

    int rescan = 1;
    int response = 0;
    for (int i = 0; i < scan->num_items; ++i)
    {
        if (has_rg_tags (scan->scan_items[i])){
            rescan = 0;
            break;
        }
    }
    if (!rescan) {
        response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
    }
    if (response == GTK_RESPONSE_NO) {
        g_idle_add (destroy_progress_cb, scan->progress);
        if (scan->scan_items) {
            free (scan->scan_items);
        }
        if (scan) {
            free (scan);
        }
        return FALSE;
    }
    // start a worker thread that will do the actual scanning
    intptr_t tid = deadbeef->thread_start (scanner_worker, scan);
    deadbeef->thread_detach (tid);

    return FALSE;
}

static int
rg_scan_run (DB_plugin_action_t *act, int ctx) {
    // this can be called from non-gtk thread
    gdk_threads_add_idle (rg_scan_run_cb, (void *)(intptr_t)ctx);
    return 0;
}

static gboolean
rg_remove_run_cb (void *data) {
    int ctx = (intptr_t)data;
    scanner_ctx_t *work = malloc (sizeof (scanner_ctx_t));
    current_ctx = work;
    memset (work, 0, sizeof (scanner_ctx_t));

    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (gtkui_plugin->get_mainwin ()), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Remove Replay Gain Information"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (gtkui_plugin->get_mainwin ()));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("This action will remove Replay Gain tags from selected files. Are you sure?"));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    if (response == GTK_RESPONSE_YES) {
        deadbeef->pl_lock ();
        switch (ctx) {
        case DDB_ACTION_CTX_MAIN:
        case DDB_ACTION_CTX_SELECTION:
            {
                // copy list
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (plt) {
                    work->num_items = deadbeef->plt_getselcount (plt);
                    if (0 < work->num_items) {
                        work->scan_items = malloc (sizeof (DB_playItem_t *) * work->num_items);
                        if (work->scan_items) {
                            int n = 0;
                            DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
                            while (it) {
                                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                                deadbeef->pl_item_unref (it);
                                if (deadbeef->pl_is_selected (it)) {
                                    deadbeef->pl_lock ();
                                    if (!deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI"))) {
                                        fprintf (stderr, "rg scan: %s is not a local file, skipped\n", deadbeef->pl_find_meta (it, ":URI"));
                                        deadbeef->pl_unlock ();
                                        work->num_items -= 1;
                                        it = next;
                                        continue;
                                    }
                                    deadbeef->pl_unlock ();
                                    assert (n < work->num_items);
                                    deadbeef->pl_item_ref (it);
                                    work->scan_items[n++] = it;
                                }
                                it = next;
                            }
                        }
                    }
                }
                break;
            }
        }
        deadbeef->pl_unlock ();
        scanner_plugin->rg_remove (work->scan_items, &work->num_items);
    }
    rg_cleanup ();
    return FALSE; // should this be called again - false?
}

static int
rg_remove_run (DB_plugin_action_t *act, int ctx) {
    gdk_threads_add_idle (rg_remove_run_cb, (void *)(intptr_t)ctx);
    return 0;
}

static DB_plugin_action_t remove_action = {
    .title = "Replay Gain/Remove Replay Gain info",
    .name  = "rg_remove",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK | DB_ACTION_ADD_MENU,
    .callback2 = rg_remove_run,
    .next = NULL
};

static DB_plugin_action_t scan_action = {
    .title = "Replay Gain/Scan as single album",
    .name = "rg_scan",
    .flags = DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK | DB_ACTION_ADD_MENU,
    .callback2 = rg_scan_run,
    .next = &remove_action
};

static DB_plugin_action_t *
rg_scan_gui_get_actions (DB_playItem_t *plt)
{
    deadbeef->pl_lock ();
    remove_action.flags |= DB_ACTION_DISABLED;  // default state is disabled
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it) && has_rg_tags (it)) {
            remove_action.flags &= ~DB_ACTION_DISABLED;
            deadbeef->pl_item_unref (it);
            break;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
    return &scan_action;
}

int
rg_scan_gui_connect (void) {
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    scanner_plugin = (rg_scan_t *)deadbeef->plug_get_for_id ("rgscanner");
    if (!gtkui_plugin) {
        fprintf (stderr, "rgscangui: gtkui plugin not found\n");
        return -1;
    }
    if (!scanner_plugin) {
        fprintf (stderr, "rgscangui: rg_scan plugin not found\n");
        return -1;
    }
    if (!PLUG_TEST_COMPAT(&scanner_plugin->misc.plugin, 1, 0)) {
        fprintf (stderr, "rgscangui: need rg scanner>=1.0, but found %d.%d\n", scanner_plugin->misc.plugin.version_major, scanner_plugin->misc.plugin.version_minor);
        return -1;
    }
    return 0;
}

DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 8,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.name = "Replay Gain Scanner GTK3 UI",
#else
    .plugin.name = "Replay Gain Scanner GTK2 UI",
#endif
    .plugin.descr = "GTK User interface for the Replay Gain Scanner plugin\n"
                    "Usage:\n"
                    "· select some tracks in playlist\n"
                    "· right click\n"
                    "· select «Replay Gain»\n",
    .plugin.copyright = "libEBUR128-based Replay Gain scanner plugin for the DeaDBeeF audio player\n"
                        "\n"
                        "Copyright (c) 2015 Ivan Pilipenko\n"
                        "\n"
                        "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
                        "of this software and associated documentation files (the \"Software\"), to deal\n"
                        "in the Software without restriction, including without limitation the rights\n"
                        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
                        "copies of the Software, and to permit persons to whom the Software is\n"
                        "furnished to do so, subject to the following conditions:\n"
                        "\n"
                        "The above copyright notice and this permission notice shall be included in\n"
                        "all copies or substantial portions of the Software.\n"
                        "\n"
                        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
                        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
                        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
                        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
                        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
                        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
                        "THE SOFTWARE.",
    //.plugin.website = "http://none.net",
    .plugin.get_actions = rg_scan_gui_get_actions,
    .plugin.connect = rg_scan_gui_connect,
    .plugin.configdialog = settings_dlg,
};

DB_plugin_t *
#if GTK_CHECK_VERSION(3,0,0)
ddb_misc_replaygain_scan_GTK3_load (DB_functions_t *api) {
#else
ddb_misc_replaygain_scan_GTK2_load (DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
