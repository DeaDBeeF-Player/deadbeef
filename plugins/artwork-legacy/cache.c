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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <limits.h>
#include "artwork_internal.h"
#include "../../deadbeef.h"
#include "../../common.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

extern DB_functions_t *deadbeef;

static uintptr_t files_mutex;
static intptr_t tid;
static uintptr_t thread_mutex;
static uintptr_t thread_cond;
static int terminate;
static int32_t cache_expiry_seconds;

void cache_lock (void)
{
    deadbeef->mutex_lock (files_mutex);
}

void cache_unlock (void)
{
    deadbeef->mutex_unlock (files_mutex);
}

int make_cache_root_path (char *path, const size_t size)
{
    const char *xdg_cache = deadbeef->get_system_dir (DDB_SYS_DIR_CACHE);

    #ifdef __MINGW32__
    // replace backslashes with normal slashes
    char xdg_cache_conv[strlen(xdg_cache)+1];
    if (strchr(xdg_cache, '\\')) {
        trace ("make_cache_root_path: backslash(es) detected: %s\n", xdg_cache);
        strcpy (xdg_cache_conv, xdg_cache);
        char *slash_p = xdg_cache_conv;
        while (slash_p = strchr(slash_p, '\\')) {
            *slash_p = '/';
            slash_p++;
        }
        xdg_cache = xdg_cache_conv;
    }
    #endif

    const char *cache_root = xdg_cache ? xdg_cache : getenv (HOMEDIR);
    if (snprintf (path, size, xdg_cache ? "%s" : "%s/.cache/deadbeef/", cache_root) >= size) {
        trace ("Cache root path truncated at %d bytes\n", (int)size);
        return -1;
    }
    return 0;
}

static int
filter_scaled_dirs (const struct dirent *f)
{
    return !strncasecmp (f->d_name, "covers-", 7);
}

void remove_cache_item (const char *entry_path, const char *subdir_path, const char *subdir_name, const char *entry_name)
{
    /* Unlink the expired file, and the artist directory if it is empty */
    cache_lock ();
    unlink (entry_path);
    rmdir (subdir_path);

    /* Remove any scaled copies of this file, plus parent directories that are now empty */
    char cache_root_path[PATH_MAX];
    make_cache_root_path (cache_root_path, PATH_MAX);
    struct dirent **scaled_dirs = NULL;
    int scaled_dirs_count = scandir (cache_root_path, &scaled_dirs, filter_scaled_dirs, NULL);
    if (scaled_dirs_count < 0) {
        cache_unlock ();
        return;
    }
    for (int i = 0; i < scaled_dirs_count; i++) {
        char scaled_entry_path[PATH_MAX];
        if (snprintf (scaled_entry_path, PATH_MAX, "%s%s/%s/%s", cache_root_path, scaled_dirs[i]->d_name, subdir_name, entry_name) < PATH_MAX) {
            unlink (scaled_entry_path);
            char *scaled_entry_dir = dirname (scaled_entry_path);
            rmdir (scaled_entry_dir);
            rmdir (dirname (scaled_entry_dir));
        }
        free (scaled_dirs[i]);
    }
    free (scaled_dirs);
    cache_unlock ();
}

static int
path_ok (const size_t dir_length, const char *entry)
{
    return strcmp (entry, ".") && strcmp (entry, "..") && dir_length + strlen (entry) + 1 < PATH_MAX;
}

static void
cache_cleaner_thread (void *none)
{
    /* Find where it all happens */
    char covers_path[PATH_MAX];
    if (make_cache_root_path (covers_path, PATH_MAX-10)) {
        return;
    }
    strcat (covers_path, "covers");
    const size_t covers_path_length = strlen (covers_path);

    deadbeef->mutex_lock (thread_mutex);
    while (!terminate) {
        time_t oldest_mtime = time (NULL);

        /* Loop through the artist directories */
        DIR *covers_dir = opendir (covers_path);
        struct dirent *covers_subdir;
        while (!terminate && covers_dir && (covers_subdir = readdir (covers_dir))) {
            const int32_t cache_secs = cache_expiry_seconds;
            deadbeef->mutex_unlock (thread_mutex);
            if (cache_secs > 0 && path_ok (covers_path_length, covers_subdir->d_name)) {
                trace ("Analyse %s for expired files\n", covers_subdir->d_name);
                const time_t cache_expiry = time (NULL) - cache_secs;

                /* Loop through the image files in this artist directory */
                char subdir_path[PATH_MAX];
                sprintf (subdir_path, "%s/%s", covers_path, covers_subdir->d_name);
                const size_t subdir_path_length = strlen (subdir_path);
                DIR *subdir = opendir (subdir_path);
                struct dirent *entry;
                while (subdir && (entry = readdir (subdir))) {
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
            deadbeef->mutex_lock (thread_mutex);
        }
        if (covers_dir) {
            closedir (covers_dir);
            covers_dir = NULL;
        }

        /* Sleep until just after the oldest file expires */
        if (cache_expiry_seconds > 0 && !terminate) {
            struct timespec wake_time = {
                .tv_sec = time (NULL) + max (60, oldest_mtime - time (NULL) + cache_expiry_seconds),
                .tv_nsec = 999999
            };
            trace ("Cache cleaner sleeping for %d seconds\n", max (60, oldest_mtime - time (NULL) + cache_expiry_seconds));
            pthread_cond_timedwait ( (pthread_cond_t *)thread_cond, (pthread_mutex_t *)thread_mutex, &wake_time);
        }

        /* Just go back to sleep if cache expiry is disabled */
        while (cache_expiry_seconds <= 0 && !terminate) {
            trace ("Cache cleaner sleeping forever\n");
            pthread_cond_wait ( (pthread_cond_t *)thread_cond, (pthread_mutex_t *)thread_mutex);
        }
    }
    deadbeef->mutex_unlock (thread_mutex);
}

void cache_configchanged (void)
{
    const int32_t new_cache_expiry_seconds = deadbeef->conf_get_int ("artwork.cache.period", 48) * 60 * 60;
    if (new_cache_expiry_seconds != cache_expiry_seconds) {
        deadbeef->mutex_lock (thread_mutex);
        cache_expiry_seconds = new_cache_expiry_seconds;
        deadbeef->cond_signal (thread_cond);
        deadbeef->mutex_unlock (thread_mutex);
    }
}

void stop_cache_cleaner (void)
{
    if (tid) {
        deadbeef->mutex_lock (thread_mutex);
        terminate = 1;
        deadbeef->cond_signal (thread_cond);
        deadbeef->mutex_unlock (thread_mutex);
        deadbeef->thread_join (tid);
        tid = 0;
        trace ("Cache cleaner thread stopped\n");
    }

    if (thread_mutex) {
        deadbeef->mutex_free (thread_mutex);
        thread_mutex = 0;
    }

    if (thread_cond) {
        deadbeef->cond_free (thread_cond);
        thread_cond = 0;
    }

    if (files_mutex) {
        deadbeef->mutex_free (files_mutex);
        files_mutex = 0;
    }
}

int start_cache_cleaner (void)
{
    terminate = 0;
    cache_expiry_seconds = deadbeef->conf_get_int ("artwork.cache.period", 48) * 60 * 60;
    files_mutex = deadbeef->mutex_create_nonrecursive ();
    thread_mutex = deadbeef->mutex_create_nonrecursive ();
    thread_cond = deadbeef->cond_create ();
    if (files_mutex && thread_mutex && thread_cond) {
        tid = deadbeef->thread_start_low_priority (cache_cleaner_thread, NULL);
        trace ("Cache cleaner thread started\n");
    }

    if (!tid) {
        stop_cache_cleaner ();
        return -1;
    }

    return 0;
}
