/*
    Album Art plugin Cache Cleaner for DeaDBeeF
    Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>

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
    #include "../../config.h"
#endif
#include "../../deadbeef.h"
#ifdef __APPLE__
#include "applesupport.h"
#endif
#include "artwork_internal.h"
#include <dirent.h>
#include <dispatch/dispatch.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

extern DB_functions_t *deadbeef;

static dispatch_queue_t sync_queue;
static dispatch_queue_t worker_queue;
static int _terminate;
static int32_t _file_expiration_time; // Seconds since the file creation, until the file expires.

int
make_cache_root_path (char *path, const size_t size) {
#ifdef __APPLE__
    apple_get_artwork_cache_path(path, size);
    size_t remaining_size = size - strlen(path);
    strncat(path, "/Deadbeef/Covers", remaining_size-1);
#else
    const char *xdg_cache = getenv ("XDG_CACHE_HOME");
    const char *cache_root = xdg_cache ? xdg_cache : getenv ("HOME");
    if (snprintf (path, size, xdg_cache ? "%s/deadbeef/covers2" : "%s/.cache/deadbeef/covers2", cache_root) >= size) {
        trace ("Cache root path truncated at %d bytes\n", (int)size);
        return -1;
    }
#endif
    return 0;
}

char *
get_cache_marker_path(const char *path) {
    size_t path_len = strlen (path);
    size_t marker_path_len = path_len + 8;

    char *marker_path = malloc (marker_path_len);

    memcpy (marker_path, path, path_len);
    memcpy (marker_path + path_len, ".marker", 8);

    return marker_path;
}

void
remove_cache_item (const char *cache_path) {
    /* Unlink the expired file, and the artist directory if it is empty */
    unlink (cache_path);
    char *marker_path = get_cache_marker_path(cache_path);
    (void)unlink(marker_path);
    free (marker_path);
}

static int
path_ok (const size_t dir_length, const char *entry) {
    return strcmp (entry, ".") && strcmp (entry, "..") && dir_length + strlen (entry) + 1 < PATH_MAX;
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
    /* Find where it all happens */
    char covers_path[PATH_MAX];
    if (make_cache_root_path (covers_path, sizeof (covers_path))) {
        return;
    }

    const int32_t cache_secs = _file_expiration_time;
    const time_t cache_expiry = time (NULL) - cache_secs;

    /* Loop through the artist directories */
    DIR *covers_dir = opendir (covers_path);
    if (covers_dir == NULL) {
        return;
    }
    struct dirent *entry;
    while (!should_terminate() && (entry = readdir (covers_dir))) {
        const char *marker = strstr(entry->d_name,".marker");
        if (marker != NULL && strlen(marker) == 7) {
            continue; // remove_cache_item will deal with it
        }
        char entry_path[PATH_MAX];
        sprintf (entry_path, "%s/%s", covers_path, entry->d_name);
        const size_t entry_path_length = strlen (entry_path);
        if (path_ok (entry_path_length, entry->d_name)) {
            /* Test against the cache expiry time (cache invalidation resets are not handled here) */
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

    if (should_terminate()) {
        return;
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

