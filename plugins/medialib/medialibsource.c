/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <jansson.h>
#include <limits.h>
#include <sys/time.h>
#include "medialibcommon.h"
#include "medialibfilesystem.h"
#include "medialibscanner.h"
#include "medialibsource.h"
#include "medialibtree.h"

static DB_functions_t *deadbeef;

static char **
_ml_source_get_music_paths (medialib_source_t *source, size_t *medialib_paths_count);

json_t *
_ml_get_music_paths (medialib_source_t *source) {
    char conf_name[200];
    snprintf (conf_name, sizeof (conf_name), "%spaths", source->source_conf_prefix);
    const char *paths = deadbeef->conf_get_str_fast (conf_name, NULL);
    if (!paths) {
        return json_array();
    }
    json_error_t error;
    json_t *json = json_loads (paths, 0, &error);

    return json;
}

/// Load the current stored medialib playlist
static void
_ml_load_playlist (medialib_source_t *source, const char *plpath) {
    struct timeval tm1, tm2;

    source->_ml_state = DDB_MEDIASOURCE_STATE_LOADING;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);

    ddb_playlist_t *plt = deadbeef->plt_alloc ("medialib");

    gettimeofday (&tm1, NULL);
    if (!source->disable_file_operations) {
        deadbeef->plt_load2 (-1, plt, NULL, plpath, NULL, NULL, NULL);
    }
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "ml playlist load time: %f seconds\n", ms / 1000.f);

    // create scanner state
    __block scanner_state_t scanner;
    memset (&scanner, 0, sizeof (scanner));
    scanner.source = source;
    scanner.track_count = deadbeef->plt_get_item_count (plt, PL_MAIN);
    scanner.tracks = calloc (scanner.track_count, sizeof (ddb_playItem_t *));

    int idx = 0;
    ddb_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
    while (it) {
        scanner.tracks[idx++] = it;
        it = deadbeef->pl_get_next (it, PL_MAIN);
    }

    ml_scanner_configuration_t conf;
    conf.medialib_paths = _ml_source_get_music_paths (source, &conf.medialib_paths_count);

    dispatch_sync(source->sync_queue, ^{
        ml_index (&scanner, &conf, 0);
    });

    ml_free_music_paths (conf.medialib_paths, conf.medialib_paths_count);

    // re-add all items (indexing may have removed some!)
    deadbeef->plt_clear(plt);
    ddb_playItem_t *after = NULL;
    for (int i = 0; i < scanner.track_count; i++) {
        if (scanner.tracks[i] != NULL) {
            after = deadbeef->plt_insert_item(plt, after, scanner.tracks[i]);
            deadbeef->pl_item_unref (scanner.tracks[i]);
        }
    }
    free (scanner.tracks);
    scanner.tracks = NULL;

    dispatch_sync(source->sync_queue, ^{
        source->ml_playlist = plt;
        memcpy (&source->db, &scanner.db, sizeof (ml_db_t));
    });

    source->_ml_state = DDB_MEDIASOURCE_STATE_IDLE;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE);
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);
}

// Get a copy of medialib folder paths
static char **
_ml_source_get_music_paths (medialib_source_t *source, size_t *medialib_paths_count) {
    if (!source->musicpaths_json) {
        source->musicpaths_json = _ml_get_music_paths(source);
    }

    char **medialib_paths = NULL;
    size_t count = 0;

    count = json_array_size(source->musicpaths_json);
    if (count == 0) {
        return NULL;
    }

    medialib_paths = calloc (count, sizeof (char *));

    for (int i = 0; i < count; i++) {
        json_t *data = json_array_get (source->musicpaths_json, i);
        if (!json_is_string (data)) {
            continue;
        }
        medialib_paths[i] = strdup (json_string_value (data));
    }

    *medialib_paths_count = count;

    return medialib_paths;
}

static void
_fs_watch_callback (void *userdata) {
    medialib_source_t *source = userdata;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_OUT_OF_SYNC);
}

void
ml_source_update_fs_watch(medialib_source_t *source) {
    ml_watch_fs_stop(source->fs_watcher);
    source->fs_watcher = ml_watch_fs_start(source->musicpaths_json, _fs_watch_callback, source);
}

ddb_mediasource_source_t *
ml_create_source (const char *source_path) {
    medialib_source_t *source = calloc (1, sizeof (medialib_source_t));
    snprintf (source->source_conf_prefix, sizeof (source->source_conf_prefix), "medialib.%s.", source_path);

    source->musicpaths_json = _ml_get_music_paths(source);

    source->sync_queue = dispatch_queue_create("MediaLibSyncQueue", NULL);
    source->scanner_queue = dispatch_queue_create("MediaLibScanQueue", NULL);

    char conf_name[200];
    snprintf (conf_name, sizeof (conf_name), "%senabled", source->source_conf_prefix);

    source->enabled = deadbeef->conf_get_int (conf_name, 1);

    // load and index the stored playlist
    dispatch_async(source->scanner_queue, ^{
        char plpath[PATH_MAX];
        snprintf (plpath, sizeof (plpath), "%s/medialib.dbpl", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG));
        _ml_load_playlist(source, plpath);
        dispatch_sync(source->sync_queue, ^{
            ml_source_update_fs_watch(source);
        });
    });

    return (ddb_mediasource_source_t *)source;
}

void
ml_free_source (ddb_mediasource_source_t *_source) {
    medialib_source_t *source = (medialib_source_t *)_source;

    dispatch_sync(source->sync_queue, ^{
        ml_watch_fs_stop(source->fs_watcher);
        source->scanner_terminate = 1;
    });

    printf ("waiting for scanner queue to finish\n");
    dispatch_sync(source->scanner_queue, ^{
    });
    printf ("scanner queue finished\n");

    dispatch_release(source->scanner_queue);
    dispatch_release(source->sync_queue);

    if (source->ml_playlist) {
        printf ("free medialib database\n");
        deadbeef->plt_free (source->ml_playlist);
        ml_db_free(&source->db);
    }

    if (source->musicpaths_json) {
        json_decref(source->musicpaths_json);
        source->musicpaths_json = NULL;
    }
}

void
ml_set_source_enabled (ddb_mediasource_source_t *_source, int enabled) {
    __block int notify = 0;
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        if (source->enabled != enabled) {
            source->enabled = enabled;
            if (!enabled) {
                source->scanner_terminate = 1;
            }
            char conf_name[200];
            snprintf (conf_name, sizeof (conf_name), "%senabled", source->source_conf_prefix);
            deadbeef->conf_set_int(conf_name, enabled);
            deadbeef->conf_save();
            notify = 1;
        }
    });
    if (notify) {
        ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_ENABLED_DID_CHANGE);
        ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE);
    }
}

int
ml_is_source_enabled (ddb_mediasource_source_t *_source) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block int enabled = 0;
    dispatch_sync(source->sync_queue, ^{
        enabled = source->enabled;
    });
    return enabled;
}

void
ml_refresh (ddb_mediasource_source_t *_source) {
    medialib_source_t *source = (medialib_source_t *)_source;

    __block int64_t scanner_current_index = -1;
    dispatch_sync(source->sync_queue, ^{
        // interrupt plt_insert_dir
        source->scanner_terminate = 1;
        // interrupt all queued scanners
        source->scanner_cancel_index = source->scanner_current_index;
        source->scanner_current_index += 1;
        scanner_current_index = source->scanner_current_index;
    });

    dispatch_async(source->scanner_queue, ^{
        __block int enabled = 0;
        __block int cancel = 0;
        dispatch_sync(source->sync_queue, ^{
            if (source->scanner_cancel_index >= scanner_current_index) {
                cancel = 1;
                return;
            }
            source->scanner_terminate = 0;
        });

        if (cancel) {
            return;
        }

        __block ml_scanner_configuration_t conf = {0};
        dispatch_sync(source->sync_queue, ^{
            conf.medialib_paths = _ml_source_get_music_paths (source, &conf.medialib_paths_count);
            enabled = source->enabled;
            if (!conf.medialib_paths || !source->enabled) {
                // no paths: early out
                // empty playlist + empty index
                if (!source->ml_playlist) {
                    source->ml_playlist = deadbeef->plt_alloc("medialib");
                }
                deadbeef->plt_clear (source->ml_playlist);
                ml_db_free(&source->db);
                ml_free_music_paths (conf.medialib_paths, conf.medialib_paths_count);
                return;
            }
        });

        if (conf.medialib_paths == NULL || !enabled) {
            // content became empty
            ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE);
            return;
        }

        scanner_thread(source, conf);
    });
}

void
ml_source_init (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
}
