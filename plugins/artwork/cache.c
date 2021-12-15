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
static int32_t _cache_clean_period;

int
make_cache_root_path (char *path, const size_t size) {
    const char *xdg_cache = getenv ("XDG_CACHE_HOME");
    const char *cache_root = xdg_cache ? xdg_cache : getenv ("HOME");
    if (snprintf (path, size, xdg_cache ? "%s/deadbeef/" : "%s/.cache/deadbeef/", cache_root) >= size) {
        trace ("Cache root path truncated at %d bytes\n", (int)size);
        return -1;
    }
    return 0;
}

static int
filter_scaled_dirs (const struct dirent *f) {
    return !strncasecmp (f->d_name, "covers2-", 7);
}

void
remove_cache_item (const char *entry_path, const char *subdir_path, const char *subdir_name, const char *entry_name) {
    /* Unlink the expired file, and the artist directory if it is empty */
    unlink (entry_path);
    rmdir (subdir_path);

    /* Remove any scaled copies of this file, plus parent directories that are now empty */
    char cache_root_path[PATH_MAX];
    make_cache_root_path (cache_root_path, PATH_MAX);
    struct dirent **scaled_dirs = NULL;
    int scaled_dirs_count = scandir (cache_root_path, &scaled_dirs, filter_scaled_dirs, NULL);
    if (scaled_dirs_count < 0) {
        return;
    }
    for (int i = 0; i < scaled_dirs_count; i++) {
        char scaled_entry_path[PATH_MAX];
        if (snprintf (scaled_entry_path, PATH_MAX, "%s%s/%s/%s", cache_root_path, scaled_dirs[i]->d_name, subdir_name, entry_name) < PATH_MAX) {
            unlink (scaled_entry_path);
            char *scaled_entry_dir = strdup (dirname (scaled_entry_path));
            rmdir (scaled_entry_dir);
            rmdir (dirname (scaled_entry_dir));
            free (scaled_entry_dir);
        }
        free (scaled_dirs[i]);
    }
    free (scaled_dirs);
}

static int
path_ok (const size_t dir_length, const char *entry) {
    return strcmp (entry, ".") && strcmp (entry, "..") && dir_length + strlen (entry) + 1 < PATH_MAX;
}

static int should_terminate(void) {
    __block int terminate = 0;
    dispatch_sync(sync_queue, ^{
        terminate = _terminate;
    });
    return terminate;
}

static void
cache_cleaner_worker (void) {
    /* Find where it all happens */
    char covers_path[PATH_MAX];
    if (make_cache_root_path (covers_path, PATH_MAX-10)) {
        return;
    }
    strcat (covers_path, "covers2");
    const size_t covers_path_length = strlen (covers_path);

    while (!should_terminate()) {
        time_t oldest_mtime = time (NULL);

        /* Loop through the artist directories */
        DIR *covers_dir = opendir (covers_path);
        struct dirent *covers_subdir;
        while (!should_terminate() && covers_dir && (covers_subdir = readdir (covers_dir))) {
            const int32_t cache_secs = _cache_clean_period;
            if (cache_secs > 0 && path_ok (covers_path_length, covers_subdir->d_name)) {
                trace ("Analyse %s for expired files\n", covers_subdir->d_name);
                const time_t cache_expiry = time (NULL) - cache_secs;

                /* Loop through the image files in this artist directory */
                char subdir_path[PATH_MAX];
                sprintf (subdir_path, "%s/%s", covers_path, covers_subdir->d_name);
                const size_t subdir_path_length = strlen (subdir_path);
                DIR *subdir = opendir (subdir_path);
                struct dirent *entry;
                while (!should_terminate() && subdir && (entry = readdir (subdir))) {
                    if (path_ok (subdir_path_length, entry->d_name)) {
                        char entry_path[PATH_MAX];
                        sprintf (entry_path, "%s/%s", subdir_path, entry->d_name);

                        /* Test against the cache expiry time (cache invalidation resets are not handled here) */
                        struct stat stat_buf;
                        if (!stat (entry_path, &stat_buf)) {
                            if (stat_buf.st_mtime <= cache_expiry) {
                                trace ("%s expired from cache\n", entry_path);
                                remove_cache_item (entry_path, subdir_path, covers_subdir->d_name, entry->d_name);
                            }
                            else if (stat_buf.st_mtime < oldest_mtime) {
                                oldest_mtime = stat_buf.st_mtime;
                            }
                        }
                    }
                }
                if (subdir) {
                    closedir (subdir);
                }
            }
            usleep (100000);
        }
        if (covers_dir) {
            closedir (covers_dir);
            covers_dir = NULL;
        }

        if (!should_terminate()) {
            break;
        }

        __block int32_t cache_clean_period;
        dispatch_sync(sync_queue, ^{
            cache_clean_period = _cache_clean_period;
        });

        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, USEC_PER_SEC * cache_clean_period), worker_queue, ^{
            cache_cleaner_worker ();
        });
    }
}

void
cache_configchanged (void) {
    dispatch_sync(sync_queue, ^{
        const int32_t new_cache_expiry_seconds = deadbeef->conf_get_int ("artwork.cache.period", 48) * 60 * 60;
        if (new_cache_expiry_seconds != _cache_clean_period) {
            _cache_clean_period = new_cache_expiry_seconds;
        }
    });
}

int
start_cache_cleaner (void) {
    _terminate = 0;

    sync_queue = dispatch_queue_create("ArtworkCacheSyncQueue", NULL);
    worker_queue = dispatch_queue_create("ArtworkCacheCleanerQueue", NULL);

    cache_configchanged();

    dispatch_async(worker_queue, ^{
        cache_cleaner_worker();
    });
    trace ("Cache cleaner started\n");

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

