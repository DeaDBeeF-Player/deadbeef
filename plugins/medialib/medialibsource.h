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

#ifndef medialibsource_h
#define medialibsource_h

#include <dispatch/dispatch.h>
#include "medialibdb.h"
#include "medialibfilesystem.h"

#define MAX_LISTENERS 10

typedef struct medialib_source_s {
    /// Tell scanner to abort as quickly as possible
    int scanner_terminate;

    /// Tell scanner it's not permitted to restart, because source is being deleted
    int deleting_source;

    /// Tells that source is being initialized, and should not be interrupted
    int initializing;

    dispatch_queue_t scanner_queue;
    dispatch_queue_t sync_queue;

    ml_watch_t *fs_watcher;

    // The following properties should only be accessed / changed on the sync_queue
    int64_t scanner_current_index;
    int64_t scanner_cancel_index;
    struct json_t *musicpaths_json;
    int disable_file_operations;

    /// Whether the source is enabled.
    /// Disabled means that the scanner should never run, and that queries should return empty tree.
    /// Only access on @c sync_queue.
    int enabled;

    ddb_playlist_t *ml_playlist; // this playlist contains the actual data of the media library in plain list
    int playlist_modification_idx;

    // this is the index, which can be rebuilt from the playlist at any given time
    ml_db_t db;

    /// Selected / expanded state.
    /// State is associated with IDs, therefore it survives updates/scans, as long as IDs are reused correctly.
    ml_collection_state_t state;

    ddb_medialib_listener_t ml_listeners[MAX_LISTENERS];
    void *ml_listeners_userdatas[MAX_LISTENERS];
    int _ml_state;
    char source_conf_prefix[100];

    struct scriptableItem_s *last_build_tree_preset;
    int last_build_tree_playlist_modification_idx;
} medialib_source_t;

ddb_mediasource_source_t *
ml_create_source (const char *source_path);

void
ml_free_source (ddb_mediasource_source_t *_source);

void
ml_set_source_enabled (ddb_mediasource_source_t *_source, int enabled);

int
ml_is_source_enabled (ddb_mediasource_source_t *_source);

void
ml_refresh (ddb_mediasource_source_t *_source);

struct json_t *
_ml_get_music_paths (medialib_source_t *source);

void
ml_source_init (DB_functions_t *_deadbeef);

void
ml_source_update_fs_watch(medialib_source_t *source);

#endif /* medialibsource_h */
