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

#include <dirent.h>
#include <dispatch/dispatch.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <deadbeef/deadbeef.h>
#include "artwork.h"
#include "artwork_internal.h"

extern DB_functions_t *deadbeef;
extern ddb_artwork_plugin_t plugin;

#define trace(...) { deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); }
#define trace_err(...) { deadbeef->log_detailed (&plugin.plugin.plugin, DDB_LOG_LAYER_DEFAULT, __VA_ARGS__); }

extern DB_functions_t *deadbeef;

static dispatch_queue_t sync_queue;
static dispatch_queue_t worker_queue;
static int _terminate;
static int32_t _file_expiration_time; // Seconds since the file creation, until the file expires.

int
make_cache_root_path (char *path, const size_t size) {
    const char *cache_root_path = deadbeef->get_system_dir(DDB_SYS_DIR_CACHE);
    size_t res;
    res = snprintf(path, size, "%s/covers2", cache_root_path);
    if (res >= size) {
        trace ("artwork: cache root path truncated at %d bytes\n", (int)size);
        return -1;
    }
    return 0;
}

void
remove_cache_item (const char *cache_path) {
    // Unlink the expired file, and the artist directory if it is empty
    (void)unlink (cache_path);
}

static int
path_ok (const char *entry) {
    return strcmp (entry, ".") && strcmp (entry, "..");
}

static int
should_terminate(void) {
    __block int terminate = 0;
    dispatch_sync(sync_queue, ^{
        terminate = (_terminate || _file_expiration_time == 0);
    });
    return terminate;
}

static void
cache_cleaner_worker (void) {
    char covers_path[PATH_MAX];
    if (make_cache_root_path (covers_path, sizeof (covers_path))) {
        return;
    }

    const int32_t cache_secs = _file_expiration_time;
    const time_t cache_expiry = time (NULL) - cache_secs;

    DIR *covers_dir = opendir (covers_path);
    if (covers_dir == NULL) {
        return;
    }
    struct dirent *entry;
    char entry_path[PATH_MAX];

    while (!should_terminate() && (entry = readdir (covers_dir))) {
        if (path_ok (entry->d_name)) {
            if (sizeof (entry_path) < snprintf (entry_path, sizeof(entry_path), "%s/%s", covers_path, entry->d_name)) {
                trace("artwork: cache cleaner entry_path buffer too small for path:\n%s/%s\n", covers_path, entry->d_name);
                continue; // buffer too small
            }

            // Test against the cache expiry time
            struct stat stat_buf;
            if (!stat (entry_path, &stat_buf)) {
                if (stat_buf.st_mtime <= cache_expiry) {
                    trace ("%s expired from cache\n", entry_path);
                    remove_cache_item (entry_path);
                }
            }
        }
    }
    if (covers_dir) {
        closedir (covers_dir);
        covers_dir = NULL;
    }
}

void
cache_configchanged (void) {
    dispatch_sync(sync_queue, ^{
        int32_t old_expiration_time = _file_expiration_time;
        _file_expiration_time = deadbeef->conf_get_int ("artwork.cache.expiration_time", 0) * 60 * 60;
        if (old_expiration_time == 0 && _file_expiration_time != 0) {
            dispatch_async(worker_queue, ^{
                cache_cleaner_worker();
            });
            trace ("Cache cleaner started\n");
        }
    });
}

int
start_cache_cleaner (void) {
    _terminate = 0;

    sync_queue = dispatch_queue_create("ArtworkCacheSyncQueue", NULL);
    worker_queue = dispatch_queue_create("ArtworkCacheCleanerQueue", NULL);

    cache_configchanged();

    return 0;
}

void
stop_cache_cleaner (void) {
    dispatch_sync(sync_queue, ^{
        _terminate = 1;
    });

    dispatch_release(worker_queue);
    worker_queue = NULL;
    dispatch_release(sync_queue);
    sync_queue = NULL;

    trace ("Cache cleaner stopped\n");
}

