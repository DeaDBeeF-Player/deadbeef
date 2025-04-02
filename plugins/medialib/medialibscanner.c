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

#include <limits.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include "medialib.h"
#include "medialibcommon.h"
#include "medialibdb.h"
#include "medialibscanner.h"

#define trace(...) { deadbeef->log_detailed (&plugin->plugin, 0, __VA_ARGS__); }

//#define FILTER_PERF 1 // measure / log file add filtering performance

static DB_functions_t *deadbeef;
static DB_mediasource_t *plugin;

static char *artist_album_id_bc;

// This should be called only on pre-existing ml playlist.
// Subsequent indexing should be done on the fly, using fileadd listener.
void
ml_index (scanner_state_t *scanner, const ml_scanner_configuration_t *conf, int can_terminate) {
    fprintf (stderr, "building index...\n");

    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);

    for (int i = 0; i < scanner->track_count && (!can_terminate || !(scanner->source->scanner_terminate || scanner->source->deleting_source)); i++) {
        ddb_playItem_t *it = scanner->tracks[i];

        const char *uri = deadbeef->pl_find_meta (it, ":URI");

        uint32_t hash = ml_collection_hash_for_ptr ((void *)uri);

        ml_filename_hash_item_t *en = scanner->db.filename_hash[hash];
        while (en) {
            if (en->file == uri) {
                break;
            }
            en = en->bucket_next;
        }

        // add to the hash table
        // at this point, we only have unique pointers, and don't need a duplicate check
        if (en == NULL) {
            en = calloc (1, sizeof (ml_filename_hash_item_t));
            deadbeef->metacache_add_string(uri);
            en->file = uri;
            en->bucket_next = scanner->db.filename_hash[hash];
            scanner->db.filename_hash[hash] = en;
        }
        en->tracks = realloc(en->tracks, (en->track_count + 1) * sizeof (ddb_playItem_t *));
        en->tracks[en->track_count++] = it;
        deadbeef->pl_item_ref(it);
    }

    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    fprintf (stderr, "index build time: %f seconds\n", ms / 1000.f);
}

static int
_status_callback (ddb_insert_file_result_t result, const char *fname, void *user_data) {
    return 0;
}

static int
_should_update_track(ddb_playItem_t *track, time_t mtime) {
    // NOTE: seemingly thread-unsafe, but really it is, since only the scanner thread is allowed to change this field
    const char *stimestamp = deadbeef->pl_find_meta (track, ":MEDIALIB_SCAN_TIME");

    if (!stimestamp) {
        // no scan time - add the file
        return 0;
    }
    int64_t timestamp;
    if (sscanf (stimestamp, "%lld", &timestamp) != 1) {
        // parse error
        return 0;
    }
    if (timestamp < mtime) {
        return 0;
    }

    return -1;
}

// NOTE: make sure to run on sync_queue
/// Returns 1 for the files which need to be included in the scan, based on their timestamp and metadata
static int
ml_filter_int (ddb_file_found_data_t *data, time_t mtime, scanner_state_t *state) {
    int res = 0;

    const char *s = deadbeef->metacache_get_string (data->filename);
    if (!s) {
        return 0;
    }

    uint32_t hash = ml_collection_hash_for_ptr((void *)s);

    if (!state->source->db.filename_hash[hash]) {
        deadbeef->metacache_remove_string (s);
        return 0;
    }

    // Check if the file needs to be reloaded or reused
    ml_filename_hash_item_t *en = state->source->db.filename_hash[hash];
    while (en) {
        if (en->file == s) {
            break;
        }
        en = en->bucket_next;
    }

    if (en) {
        res = _should_update_track(en->tracks[0], mtime);

        if (res != 0) {
            // File is reused.

            // Because of cuesheets, the same track may get added multiple times,
            // since all items reference the same filename.
            // Check if this track is still in ml_playlist.

            for (size_t hi = 0; hi < en->track_count; hi++) {
                int track_found = 0;
                for (int i = state->track_count-1; i >= 0; i--) {
                    if (state->tracks[i] == en->tracks[hi]) {
                        track_found = 1;
                        break;
                    }
                }
                if (track_found) {
                    continue;
                }
                deadbeef->pl_item_ref (en->tracks[hi]);
                // Allocated space precisely matches playlist count, so no check is necessary
                state->tracks[state->track_count++] = en->tracks[hi];
            }
        }
    }

#if FILTER_PERF
    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);

    if (!res) {
        fprintf (stderr, "ADD %s: file presence check took %f sec\n", data->filename, ms / 1000.f);
    }
    else {
        fprintf (stderr, "SKIP %s: file presence check took %f sec\n", data->filename, ms / 1000.f);
    }
#endif

    deadbeef->metacache_remove_string (s);

    return res;
}

// intention is to skip the files which are already indexed
// how to speed this up:
// first check if a folder exists (early out?)
static int
ml_fileadd_filter (ddb_file_found_data_t *data, void *user_data) {
    __block int res = 0;

    scanner_state_t *state = user_data;

    if (!user_data || data->plt != state->plt || data->is_dir) {
        return 0;
    }

#if FILTER_PERF
    struct timeval tm1, tm2;
    gettimeofday (&tm1, NULL);
#endif

    time_t mtime = 0;
    struct stat st = {0};
    if (stat (data->filename, &st) == 0) {
        mtime = st.st_mtime;
    }

    medialib_source_t *source = state->source;

    dispatch_sync(source->sync_queue, ^{
        res = ml_filter_int(data, mtime, state);
    });

    return res;
}

void
scanner_thread (medialib_source_t *source, ml_scanner_configuration_t conf) {
    struct timeval tm1, tm2;

    source->_ml_state = DDB_MEDIASOURCE_STATE_SCANNING;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);

    __block int reserve_tracks = 0;
    dispatch_sync(source->sync_queue, ^{
        if (source->ml_playlist != NULL) {
            reserve_tracks = deadbeef->plt_get_item_count (source->ml_playlist, PL_MAIN);
        }
    });

    if (reserve_tracks < 1000) {
        reserve_tracks = 1000;
    }

    scanner_state_t scanner = {0};
    scanner.source = source;
    scanner.plt = deadbeef->plt_alloc("medialib");
    scanner.tracks = calloc (reserve_tracks, sizeof (ddb_playItem_t *));
    scanner.track_count = 0;
    scanner.track_reserved_count = reserve_tracks;

    int filter_id = deadbeef->register_fileadd_filter (ml_fileadd_filter, &scanner);
    gettimeofday (&tm1, NULL);

    for (int i = 0; i < conf.medialib_paths_count; i++) {
        const char *musicdir = conf.medialib_paths[i];
        printf ("adding dir: %s\n", musicdir);
        // Create a new playlist, by looking back into the existing playlist.
        // The reusable tracks get moved to the new playlist.
        deadbeef->plt_insert_dir3 (-1, 0, scanner.plt, NULL, musicdir, &source->scanner_terminate, _status_callback, NULL);
    }
    deadbeef->unregister_fileadd_filter (filter_id);

    if (source->scanner_terminate) {
        goto error;
    }

    // move from playlist to the track list
    int plt_track_count = deadbeef->plt_get_item_count (scanner.plt, PL_MAIN);
    if (scanner.track_count + plt_track_count > scanner.track_reserved_count) {
        scanner.track_reserved_count = scanner.track_count + plt_track_count;
        scanner.tracks = realloc(scanner.tracks, scanner.track_reserved_count * sizeof (ddb_playItem_t));
        if (scanner.tracks == NULL) {
            trace ("medialib: failed to allocate memory for tracks\n");
            goto error;
        }
    }

    time_t timestamp = time(NULL);
    char stimestamp[100];
    snprintf (stimestamp, sizeof (stimestamp), "%lld", (int64_t)timestamp);
    ddb_playItem_t *it = deadbeef->plt_get_head_item (scanner.plt, PL_MAIN);
    while (it) {
        deadbeef->pl_replace_meta (it, ":MEDIALIB_SCAN_TIME", stimestamp);
        scanner.tracks[scanner.track_count++] = it;
        it = deadbeef->pl_get_next (it, PL_MAIN);
    }
    deadbeef->plt_unref (scanner.plt);
    scanner.plt = NULL;

    gettimeofday (&tm2, NULL);
    long ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    fprintf (stderr, "scan time: %f seconds (%d tracks)\n", ms / 1000.f, deadbeef->plt_get_item_count (source->ml_playlist, PL_MAIN));

    source->_ml_state = DDB_MEDIASOURCE_STATE_INDEXING;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);

    ml_index(&scanner, &conf, 1);
    if (source->scanner_terminate) {
        goto error;
    }

    source->_ml_state = DDB_MEDIASOURCE_STATE_SAVING;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);

    // Create playlist from tracks
    ddb_playlist_t *new_plt = deadbeef->plt_alloc("Medialib Playlist");

    dispatch_sync(source->sync_queue, ^{
        deadbeef->plt_unref (source->ml_playlist);
        source->ml_playlist = new_plt;
        ml_db_free(&source->db);
        memcpy (&source->db, &scanner.db, sizeof (ml_db_t));

        ddb_playItem_t *after = NULL;
        for (int i = 0; i < scanner.track_count; i++) {
            after = deadbeef->plt_insert_item(new_plt, after, scanner.tracks[i]);
            deadbeef->pl_item_unref(scanner.tracks[i]);
            scanner.tracks[i] = NULL;
        }
    });

    free (scanner.tracks);
    scanner.tracks = NULL;

    if (!source->disable_file_operations) {
        char plpath[PATH_MAX];
        snprintf (plpath, sizeof (plpath), "%s/medialib.dbpl", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG));
        deadbeef->plt_save (new_plt, NULL, NULL, plpath, NULL, NULL, NULL);
    }

    ml_free_music_paths (conf.medialib_paths, conf.medialib_paths_count);

    source->_ml_state = DDB_MEDIASOURCE_STATE_IDLE;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE);

    return;
error:
    // scanning or indexing has was cancelled, cleanup
    for (int i = 0; i < scanner.track_count; i++) {
        if (scanner.tracks[i] != NULL) {
            deadbeef->pl_item_unref(scanner.tracks[i]);
        }
    }
    free (scanner.tracks);
    scanner.tracks = NULL;

    ml_db_free (&scanner.db);
    memset (&scanner.db, 0, sizeof (ml_db_t));

    if (scanner.plt) {
        deadbeef->plt_unref (scanner.plt);
        scanner.plt = NULL;
    }
    source->_ml_state = DDB_MEDIASOURCE_STATE_IDLE;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE);
}

void
ml_scanner_init (DB_mediasource_t *_plugin, DB_functions_t *_deadbeef) {
    plugin = _plugin;
    deadbeef = _deadbeef;
    artist_album_id_bc = deadbeef->tf_compile ("artist=$if2(%album artist%,Unknown Artist);album=$if2(%album%,Unknown Album)");
}

void
ml_scanner_free (void) {
    if (artist_album_id_bc) {
        deadbeef->tf_free (artist_album_id_bc);
        artist_album_id_bc = NULL;
    }
}
