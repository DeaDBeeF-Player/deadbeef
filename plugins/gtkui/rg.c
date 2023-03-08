/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2017 Oleksiy Yakovenko and other contributors

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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/time.h>
#include <math.h>
#include "interface.h"
#include "support.h"
#include <deadbeef/deadbeef.h>

#include "../rg_scanner/rg_scanner.h"

extern DB_functions_t *deadbeef;

typedef struct rgs_controller_s {
    GtkWidget *progress_window;
    GtkWidget *results_window;
    GtkWidget *update_progress_window;

    ddb_rg_scanner_settings_t _rg_settings;
    ddb_rg_scanner_t *_rg;
    int _abort_flag;
    struct timeval _rg_start_tv;
    int _abortTagWriting;
    struct rgs_controller_s *next;
} rgs_controller_t;

static rgs_controller_t *g_rgControllers;
static char *_title_tf;
static ddb_rg_scanner_t *_rg;

typedef struct {
    rgs_controller_t *ctl;
    int current;
} progress_data_t;

static float
_getScanSpeed (uint64_t cd_samples_processed, float time) {
    return cd_samples_processed / 44100.f / time;
}

static void
_formatTime (float sec, int extraPrecise, char *buf, int bufsize) {
    int hr;
    int min;
    hr = (int)floor (sec / 3600);
    sec -= hr * 3600;
    min = (int)floor (sec / 60);
    sec -= min * 60;

    if (extraPrecise) {
        if (hr > 0) {
            snprintf (buf, bufsize, "%d:%02d:%0.3f", hr, min, sec);
            return;
        }
        snprintf (buf, bufsize, "%02d:%0.3f", min, sec);
        return;
    }
    if (hr > 0) {
        snprintf (buf, bufsize, "%d:%02d:%02d", hr, min, (int)floor(sec));
        return;
    }
    snprintf (buf, bufsize, "%02d:%02d", min, (int)floor(sec));
}


static void
_ctl_progress (rgs_controller_t *ctl, int current) {
    deadbeef->pl_lock ();
    const char *uri = deadbeef->pl_find_meta (ctl->_rg_settings.tracks[current], ":URI");

    GtkWidget *progressText = lookup_widget (ctl->progress_window, "rg_scan_progress_file");
    gtk_entry_set_text (GTK_ENTRY (progressText), uri);
    GtkWidget *progressBar = lookup_widget (ctl->progress_window, "rg_scan_progress_bar");
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progressBar), (double)current/ctl->_rg_settings.num_tracks);
    GtkWidget *statusLabel = lookup_widget (ctl->progress_window, "rg_scan_progress_status");

    struct timeval tv;
    gettimeofday (&tv, NULL);
    float timePassed = (tv.tv_sec-ctl->_rg_start_tv.tv_sec) + (tv.tv_usec - ctl->_rg_start_tv.tv_usec) / 1000000.f;
    if (timePassed > 0 && ctl->_rg_settings.cd_samples_processed > 0 && current > 0) {
        float speed = _getScanSpeed (ctl->_rg_settings.cd_samples_processed, timePassed);
        float predicted_samples_total = ctl->_rg_settings.cd_samples_processed / (float)current * ctl->_rg_settings.num_tracks;

        float frac = (float)((double)predicted_samples_total / ctl->_rg_settings.cd_samples_processed);
        float est = timePassed * frac;

        char elapsed[50];
        _formatTime (timePassed, 0, elapsed, sizeof (elapsed));
        char estimated[50];
        _formatTime (est, 0, estimated, sizeof (estimated));

        char status[200];
        snprintf (status, sizeof (status), "Time elapsed: %s, estimated: %s, speed: %0.2fx (%i of %i files)", elapsed, estimated, speed, current, ctl->_rg_settings.num_tracks);
        gtk_label_set_text (GTK_LABEL (statusLabel), status);
    }
    else {
        gtk_label_set_text (GTK_LABEL (statusLabel), "");
    }

    deadbeef->pl_unlock ();
}

static gboolean
_scan_progress_cb (void *ctx) {
    progress_data_t *dt = ctx;
    _ctl_progress (dt->ctl, dt->current);
    free (dt);
    return FALSE;
}

static void
_scan_progress (int current, void *user_data) {
    progress_data_t *dt = calloc (1, sizeof (progress_data_t));
    dt->current = current;
    dt->ctl = user_data;
    g_idle_add (_scan_progress_cb, dt);
}

static void
_ctl_dismiss (rgs_controller_t *ctl) {
    if (ctl->_rg_settings.tracks) {
        for (int i = 0; i < ctl->_rg_settings.num_tracks; i++) {
            deadbeef->pl_item_unref (ctl->_rg_settings.tracks[i]);
        }
        free (ctl->_rg_settings.tracks);
    }
    if (ctl->_rg_settings.results) {
        free (ctl->_rg_settings.results);
    }
    memset (&ctl->_rg_settings, 0, sizeof (ctl->_rg_settings));

    // remove from list    
    rgs_controller_t *prev = NULL;
    for (rgs_controller_t *c = g_rgControllers; c; c = c->next) {
        if (c == ctl) {
            if (prev) {
                prev->next = ctl->next;
            }
            else {
                g_rgControllers = ctl->next;
            }
            break;
        }
        prev = c;
    }

    if (ctl->progress_window) {
        gtk_widget_destroy (ctl->progress_window);
        ctl->progress_window = NULL;
    }

    if (ctl->results_window) {
        gtk_widget_destroy (ctl->results_window);
        ctl->results_window = NULL;
    }

    if (ctl->update_progress_window) {
        gtk_widget_destroy (ctl->update_progress_window);
        ctl->update_progress_window = NULL;
    }

    free (ctl);
}

static void
on_results_cancel_btn (GtkButton *button, gpointer user_data) {
    _ctl_dismiss (user_data);
}

static void
on_results_delete_event (GtkWidget *window,GdkEvent *event, gpointer user_data) {
    _ctl_dismiss (user_data);
}

typedef struct {
    rgs_controller_t *ctl;
    int current;
} updateProgressData_t;

static gboolean
_setUpdateProgress (void *ctx) {
    updateProgressData_t *dt = ctx;
    rgs_controller_t *ctl = dt->ctl;
    const char *path = deadbeef->pl_find_meta_raw (ctl->_rg_settings.tracks[dt->current], ":URI");
    GtkWidget *progressText = lookup_widget (ctl->update_progress_window, "rg_scan_progress_file");
    gtk_entry_set_text (GTK_ENTRY (progressText), path);

    GtkWidget *progressBar = lookup_widget (ctl->update_progress_window, "rg_scan_progress_bar");
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progressBar), (double)dt->current/ctl->_rg_settings.num_tracks);
    free (dt);

    return FALSE;
}

static gboolean
_ctl_dismiss_cb (void *ctx) {
    _ctl_dismiss (ctx);
    return FALSE;
}

static void
_update_tags (void *ctx) {
    rgs_controller_t *ctl = ctx;

    for (int i = 0; i < ctl->_rg_settings.num_tracks; i++) {
        if (ctl->_abortTagWriting) {
            break;
        }

        if (ctl->_rg_settings.results[i].scan_result == DDB_RG_SCAN_RESULT_SUCCESS) {
            updateProgressData_t *dt = calloc (1, sizeof (updateProgressData_t));
            dt->ctl = ctl;
            dt->current = i;

            g_idle_add (_setUpdateProgress, dt);
            
            uint32_t flags = (1<<DDB_REPLAYGAIN_TRACKGAIN)|(1<<DDB_REPLAYGAIN_TRACKPEAK);
            if (ctl->_rg_settings.mode != DDB_RG_SCAN_MODE_TRACK) {
                flags |= (1<<DDB_REPLAYGAIN_ALBUMGAIN)|(1<<DDB_REPLAYGAIN_ALBUMPEAK);
            }
            _rg->apply (ctl->_rg_settings.tracks[i], flags, ctl->_rg_settings.results[i].track_gain, ctl->_rg_settings.results[i].track_peak, ctl->_rg_settings.results[i].album_gain, ctl->_rg_settings.results[i].album_peak);
        }
    }

    deadbeef->pl_save_all ();

    g_idle_add (_ctl_dismiss_cb, ctl);
}

static void
on_update_progress_cancel_btn (GtkWidget *button, void *user_data) {
    rgs_controller_t *ctl = user_data;
    ctl->_abortTagWriting = 1;
}

static void
on_update_progress_delete_event (GtkWidget *window, GdkEvent *event, void *user_data) {
    rgs_controller_t *ctl = user_data;
    ctl->_abortTagWriting = 1;
}

static void
on_results_update_btn (GtkButton *button, gpointer user_data) {
    rgs_controller_t *ctl = user_data;

    gtk_widget_hide (ctl->results_window);

    ctl->update_progress_window = create_rg_scan_progress ();

    GtkWidget *cancel_btn = lookup_widget (ctl->update_progress_window, "rg_scan_progress_cancel");
    g_signal_connect ((gpointer) cancel_btn, "clicked",
            G_CALLBACK (on_update_progress_cancel_btn),
            ctl);
    g_signal_connect ((gpointer) ctl->update_progress_window, "delete-event",
            G_CALLBACK (on_update_progress_delete_event),
            ctl);

    gtk_window_set_title (GTK_WINDOW (ctl->update_progress_window), _("Updating File Tags Progress"));
    gtk_widget_show (ctl->update_progress_window);
    ctl->_abortTagWriting = 0;

    deadbeef->thread_detach (deadbeef->thread_start (_update_tags, ctl));
}

void _ctl_scanFinished (rgs_controller_t *ctl) {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    float timePassed = (tv.tv_sec-ctl->_rg_start_tv.tv_sec) + (tv.tv_usec - ctl->_rg_start_tv.tv_usec) / 1000000.f;

    char elapsed[50];
    _formatTime (timePassed, 1, elapsed, sizeof (elapsed));

    float speed = _getScanSpeed (ctl->_rg_settings.cd_samples_processed, timePassed);

    gtk_widget_hide (ctl->progress_window);

    ctl->results_window = create_rg_scan_results ();
    GtkWidget *status = lookup_widget (ctl->results_window, "rg_scan_results_status");
    char statusText[200];
    snprintf (statusText, sizeof (statusText), "Calculated in: %s, speed: %0.2fx", elapsed, speed);
    gtk_label_set_text (GTK_LABEL (status), statusText);
    gtk_widget_show (ctl->results_window);

    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (ctl->results_window, "rg_scan_results_list"));
    GtkListStore *store = gtk_list_store_new (6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_append_column (tree, gtk_tree_view_column_new_with_attributes (_("Name"), gtk_cell_renderer_text_new (), "text", 0, NULL));
    gtk_tree_view_append_column (tree, gtk_tree_view_column_new_with_attributes (_("Status"), gtk_cell_renderer_text_new (), "text", 1, NULL));
    gtk_tree_view_append_column (tree, gtk_tree_view_column_new_with_attributes (_("Album Gain"), gtk_cell_renderer_text_new (), "text", 2, NULL));
    gtk_tree_view_append_column (tree, gtk_tree_view_column_new_with_attributes (_("Track Gain"), gtk_cell_renderer_text_new (), "text", 3, NULL));
    gtk_tree_view_append_column (tree, gtk_tree_view_column_new_with_attributes (_("Album Peak"), gtk_cell_renderer_text_new (), "text", 4, NULL));
    gtk_tree_view_append_column (tree, gtk_tree_view_column_new_with_attributes (_("Track Peak"), gtk_cell_renderer_text_new (), "text", 5, NULL));

    const char *status_str[] = {
        _("Success"),
        _("File not found"),
        _("Invalid file"),
    };

    for (int i = 0; i < ctl->_rg_settings.num_tracks; i++) {
        GtkTreeIter it;
        gtk_list_store_append (store, &it);

        ddb_tf_context_t ctx;
        memset (&ctx, 0, sizeof(ctx));
        ctx._size = sizeof (ddb_tf_context_t);
        ctx.it = ctl->_rg_settings.tracks[i];

        char name[100];
        deadbeef->tf_eval (&ctx, _title_tf, name, sizeof (name));

        const char *status = -ctl->_rg_settings.results[i].scan_result < 3 ? status_str[-ctl->_rg_settings.results[i].scan_result] : "Unknown error";

        char albumGain[50] = "";
        if (ctl->_rg_settings.mode != DDB_RG_SCAN_MODE_TRACK) {
            snprintf (albumGain, sizeof (albumGain), "%0.2f dB", ctl->_rg_settings.results[i].album_gain);
        }

        char trackGain[50] = "";
        snprintf (trackGain, sizeof (trackGain), "%0.2f dB", ctl->_rg_settings.results[i].track_gain);

        char albumPeak[50] = "";
        if (ctl->_rg_settings.mode != DDB_RG_SCAN_MODE_TRACK) {
           snprintf (albumPeak, sizeof (albumPeak), "%0.6f", ctl->_rg_settings.results[i].album_peak);
        }

        char trackPeak[50] = "";
        snprintf (trackPeak, sizeof (trackPeak), "%0.6f", ctl->_rg_settings.results[i].track_peak);

        gtk_list_store_set (store, &it, 0, name, 1, status, 2, albumGain, 3, trackGain, 4, albumPeak, 5, trackPeak, -1);
    }
    gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));

    GtkWidget *cancel_btn = lookup_widget (ctl->results_window, "rg_scan_results_cancel");
    GtkWidget *update_btn = lookup_widget (ctl->results_window, "rg_scan_results_update");
    g_signal_connect ((gpointer) cancel_btn, "clicked",
            G_CALLBACK (on_results_cancel_btn),
            ctl);
    g_signal_connect ((gpointer) ctl->results_window, "delete-event",
            G_CALLBACK (on_results_delete_event),
            ctl);
    g_signal_connect ((gpointer) update_btn, "clicked",
            G_CALLBACK (on_results_update_btn),
            ctl);
}


static gboolean
_rgs_finished_cb (void *ctx) {
    rgs_controller_t *ctl = ctx;
    if (ctl->_abort_flag) {
        _ctl_dismiss (ctl);
        return FALSE;
    }
    _ctl_scanFinished (ctl);
    return FALSE;
}

static void
_rgs_job (void *ctx) {
    rgs_controller_t *ctl = ctx;
    _rg->scan (&ctl->_rg_settings);
    deadbeef->background_job_decrement ();

    g_idle_add (_rgs_finished_cb, ctl);
}

static void
on_progress_cancel_btn (GtkButton *button, gpointer user_data) {
    rgs_controller_t *ctl = user_data;
    ctl->_abort_flag = 1;
}

static void
on_progress_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    rgs_controller_t *ctl = user_data;
    ctl->_abort_flag = 1;
}

static int
_init_plugin (void) {
    if (_rg) {
        return 1;
    }

    _rg = (ddb_rg_scanner_t *)deadbeef->plug_get_for_id ("rg_scanner");
    if (_rg && _rg->misc.plugin.version_major != 1) {
        _rg = NULL;
        deadbeef->log ("Invalid version of rg_scanner plugin");
        return 0;
    }

    if (!_rg) {
        deadbeef->log ("ReplayGain plugin is not found");
        return 0;
    }

    return 1;
}

static void
runScanner (int mode, DB_playItem_t ** tracks, int count) {
    if (!_init_plugin ()) {
        return;
    }

    deadbeef->background_job_increment ();

    rgs_controller_t *ctl = calloc (1, sizeof (rgs_controller_t));

    if (!_title_tf) {
        _title_tf = deadbeef->tf_compile ("%title%");
    }

    ctl->progress_window = create_rg_scan_progress ();
    GtkWidget *cancel_btn = lookup_widget (ctl->progress_window, "rg_scan_progress_cancel");
    g_signal_connect ((gpointer) cancel_btn, "clicked",
            G_CALLBACK (on_progress_cancel_btn),
            ctl);
    g_signal_connect ((gpointer) ctl->progress_window, "delete-event",
            G_CALLBACK (on_progress_delete_event),
            ctl);

    gtk_widget_show (ctl->progress_window);

    memset (&ctl->_rg_settings, 0, sizeof (ddb_rg_scanner_settings_t));
    ctl->_rg_settings._size = sizeof (ddb_rg_scanner_settings_t);
    ctl->_rg_settings.mode = mode;
    ctl->_rg_settings.tracks = tracks;
    ctl->_rg_settings.num_tracks = count;
    ctl->_rg_settings.ref_loudness = deadbeef->conf_get_float ("rg_scanner.target_db", DDB_RG_SCAN_DEFAULT_LOUDNESS);
    ctl->_rg_settings.results = calloc (count, sizeof (ddb_rg_scanner_result_t));
    ctl->_rg_settings.pabort = &ctl->_abort_flag;
    ctl->_rg_settings.progress_callback = _scan_progress;
    ctl->_rg_settings.progress_cb_user_data = ctl;

    gettimeofday (&ctl->_rg_start_tv, NULL);
    _ctl_progress (ctl, 0);

    // FIXME: need to use some sort of job queue
    uint64_t tid = deadbeef->thread_start (_rgs_job, ctl);
    deadbeef->thread_detach (tid);

    ctl->next = g_rgControllers;
    g_rgControllers = ctl;
}

static DB_playItem_t **
_get_action_track_list (DB_plugin_action_t *action, int ctx, int *pcount, int onlyWithRgInfo) {
    int count = 0;
    DB_playItem_t **tracks = NULL;

    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    if (!plt) {
        return NULL;
    }

    ddb_replaygain_settings_t s;
    s._size = sizeof (ddb_replaygain_settings_t);

    DB_playItem_t *playing_track = NULL;
    if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        deadbeef->streamer_get_playing_track_safe ();
    }

    deadbeef->pl_lock ();

    if (ctx == DDB_ACTION_CTX_SELECTION) {
        int tc = deadbeef->plt_getselcount (plt);
        if (!tc) {
            deadbeef->pl_unlock ();
            deadbeef->plt_unref (plt);
            if (playing_track != NULL) {
                deadbeef->pl_item_unref (playing_track);
            }
            return NULL;
        }
        tracks = calloc (tc, sizeof (DB_playItem_t *));

        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            const char *uri = deadbeef->pl_find_meta (it, ":URI");

            if (deadbeef->pl_is_selected (it) && deadbeef->is_local_file (uri)) {
                int hasRgTags = 0;
                if (onlyWithRgInfo) {
                    deadbeef->replaygain_init_settings (&s, it);
                    if (s.has_album_gain || s.has_track_gain) {
                        hasRgTags = 1;
                    }
                }

                if (!onlyWithRgInfo || hasRgTags) {
                    tracks[count++] = it;
                    deadbeef->pl_item_ref (it);
                }
            }

            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        int tc = deadbeef->plt_get_item_count (plt, PL_MAIN);
        if (!tc) {
            deadbeef->pl_unlock ();
            deadbeef->plt_unref (plt);
            if (playing_track != NULL) {
                deadbeef->pl_item_unref (playing_track);
            }
            return NULL;
        }
        tracks = calloc (tc, sizeof (DB_playItem_t *));

        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->is_local_file (uri)) {
                int hasRgTags = 0;
                if (onlyWithRgInfo) {
                    deadbeef->replaygain_init_settings (&s, it);
                    if (s.has_album_gain || s.has_track_gain) {
                        hasRgTags = 1;
                    }
                }

                if (!onlyWithRgInfo || hasRgTags) {
                    tracks[count++] = it;
                    deadbeef->pl_item_ref (it);
                }
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }

        deadbeef->pl_save_current ();
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        if (playing_track) {
            const char *uri = deadbeef->pl_find_meta (playing_track, ":URI");
            if (deadbeef->is_local_file (uri)) {
                int hasRgTags = 0;
                if (onlyWithRgInfo) {
                    deadbeef->replaygain_init_settings (&s, playing_track);
                    if (s.has_album_gain || s.has_track_gain) {
                        hasRgTags = 1;
                    }
                }

                if (!onlyWithRgInfo || hasRgTags) {
                    count = 1;
                    tracks = calloc (1, sizeof (DB_playItem_t *));
                    tracks[0] = playing_track;
                    deadbeef->pl_item_ref (playing_track);
                }
            }
        }
    }
    deadbeef->pl_unlock ();
    deadbeef->plt_unref (plt);
    if (playing_track != NULL) {
        deadbeef->pl_item_unref (playing_track);
    }

    if (!count) {
        free (tracks);
        return NULL;
    }
    *pcount = count;
    return tracks;
}

int
action_rg_scan_per_file_handler (struct DB_plugin_action_s *action, int ctx) {
    int count;
    DB_playItem_t **tracks = _get_action_track_list (action, ctx, &count, 0);

    if (!tracks) {
        return 0;
    }

    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }

    runScanner (DDB_RG_SCAN_MODE_TRACK, tracks, count);
    return 0;
}

int
action_rg_scan_selection_as_albums_handler (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    int count;
    DB_playItem_t **tracks = _get_action_track_list (action, ctx, &count, 0);

    if (!tracks) {
        return 0;
    }

    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }

    runScanner (DDB_RG_SCAN_MODE_ALBUMS_FROM_TAGS, tracks, count);
    return 0;
}

int
action_rg_scan_selection_as_album_handler (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    int count;
    DB_playItem_t **tracks = _get_action_track_list (action, ctx, &count, 0);

    if (!tracks) {
        return 0;
    }

    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }

    runScanner (DDB_RG_SCAN_MODE_SINGLE_ALBUM, tracks, count);
    return 0;
}

static void
_remove_rg_tags (void *ctx) {
    rgs_controller_t *ctl = ctx;

    for (int i = 0; i < ctl->_rg_settings.num_tracks; i++) {
        _rg->remove (ctl->_rg_settings.tracks[i]);
        if (ctl->_abortTagWriting) {
            break;
        }

        updateProgressData_t *dt = calloc (1, sizeof (updateProgressData_t));
        dt->ctl = ctl;
        dt->current = i;

        g_idle_add (_setUpdateProgress, dt);
    }

    deadbeef->pl_save_all ();
    deadbeef->background_job_decrement ();

    g_idle_add (_ctl_dismiss_cb, ctl);
}

int
action_rg_remove_info_handler (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    if (!_init_plugin ()) {
        return -1;
    }

    int count;
    DB_playItem_t **tracks = _get_action_track_list (action, ctx, &count, 1);

    if (!tracks) {
        return 0;
    }

    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }

    deadbeef->background_job_increment ();

    rgs_controller_t *ctl = calloc (1, sizeof (rgs_controller_t));
    memset (&ctl->_rg_settings, 0, sizeof (ddb_rg_scanner_settings_t));
    ctl->_rg_settings._size = sizeof (ddb_rg_scanner_settings_t);
    ctl->_rg_settings.tracks = tracks;
    ctl->_rg_settings.num_tracks = count;
    ctl->update_progress_window = create_rg_scan_progress ();

    gtk_widget_show (ctl->update_progress_window);
    ctl->_abortTagWriting = 0;

    deadbeef->thread_detach (deadbeef->thread_start (_remove_rg_tags, ctl));

    return 0;
}

int
action_scan_all_tracks_without_rg_handler (struct DB_plugin_action_s *action, ddb_action_context_t ctx) {
    int count = 0;
    DB_playItem_t **tracks = NULL;

    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    if (!plt) {
        return 0;
    }

    ddb_replaygain_settings_t s;
    s._size = sizeof (ddb_replaygain_settings_t);

    deadbeef->pl_lock ();

    int tc = deadbeef->plt_get_item_count (plt, PL_MAIN);
    if (!tc) {
        deadbeef->pl_unlock ();
        deadbeef->plt_unref (plt);
        return 0;
    }
    tracks = calloc (tc, sizeof (DB_playItem_t *));
 
    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        const char *uri = deadbeef->pl_find_meta (it, ":URI");

        if (deadbeef->is_local_file (uri)) {
            deadbeef->replaygain_init_settings (&s, it);
            if (!s.has_track_gain) {
                tracks[count++] = it;
                deadbeef->pl_item_ref (it);
            }
        }

        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    deadbeef->pl_unlock ();

    if (count > 0) {
         deadbeef->plt_modified (plt);
    }

    deadbeef->plt_unref (plt);

    if (count > 0) {
        runScanner (DDB_RG_SCAN_MODE_TRACK, tracks, count);
    }

    return 0;
}
