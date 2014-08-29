/*
    Album Art plugin for DeaDBeeF
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#ifdef __linux__
    #include <sys/prctl.h>
#endif
#include "artwork_internal.h"
#include "../../deadbeef.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

static uintptr_t files_mutex;
static uintptr_t files_cond;
static int32_t cache_expiry_seconds;
static int terminate;
static intptr_t tid;

void cache_lock(void)
{
    deadbeef->mutex_lock(files_mutex);
}

void cache_unlock(void)
{
    deadbeef->mutex_unlock(files_mutex);
}

int make_cache_root_path(char *path, const size_t size)
{
    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    const char *cache_root = xdg_cache ? xdg_cache : getenv("HOME");
    if (snprintf(path, size, xdg_cache ? "%s/deadbeef/" : "%s/.cache/deadbeef/", cache_root) >= size) {
        trace("Cache root path truncated at %d bytes\n", (int)size);
        return -1;
    }
    return 0;
}

static int
filter_scaled_dirs (const struct dirent *f)
{
    return !strncasecmp(f->d_name, "covers-", 7);
}

void remove_cache_item(const char *entry_path, const char *subdir_path, const char *subdir_name, const char *entry_name)
{
    /* Unlink the expired file, and the artist directory if it is empty */
    deadbeef->mutex_lock(files_mutex);
    unlink(entry_path);
    rmdir(subdir_path);

    /* Remove any scaled copies of this file, plus parent directories that are now empty */
    char cache_root_path[PATH_MAX];
    make_cache_root_path(cache_root_path, PATH_MAX-7);
    struct dirent **scaled_dirs = NULL;
    const int scaled_dirs_count = scandir(cache_root_path, &scaled_dirs, filter_scaled_dirs, NULL);
    for (size_t i = 0; i < scaled_dirs_count; i++) {
        char scaled_entry_path[PATH_MAX];
        snprintf(scaled_entry_path, PATH_MAX, "%s%s/%s/%s", cache_root_path, scaled_dirs[i]->d_name, subdir_name, entry_name);
        unlink(scaled_entry_path);
        char *scaled_entry_dir = dirname(scaled_entry_path);
        rmdir(scaled_entry_dir);
        rmdir(dirname(scaled_entry_dir));
        free(scaled_dirs[i]);
    }
    free(scaled_dirs);
    deadbeef->mutex_unlock(files_mutex);
}

static void
cache_cleaner_thread(void *none)
{
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-artwork-cc", 0, 0, 0, 0);
#endif

    /* Find where it all happens */
    char covers_path[PATH_MAX];
    make_cache_root_path(covers_path, PATH_MAX-7);
    strcat(covers_path, "covers/");

    cache_lock();
    while (!terminate) {
        cache_unlock();
        time_t oldest_mtime = time(NULL);

        /* Loop through the artist directories */
        DIR *covers_dir = opendir(covers_path);
        struct dirent *covers_subdir;
        while (covers_dir && (covers_subdir = readdir(covers_dir))) {
            cache_lock();
            const int32_t cache_secs = cache_expiry_seconds;
            cache_unlock();
            if (cache_secs > 0 && strcmp(covers_subdir->d_name, ".") && strcmp(covers_subdir->d_name, "..")) {
                const time_t cache_expiry = time(NULL) - cache_secs;

                /* Loop through the image files in this artist directory */
                char subdir_path[PATH_MAX];
                snprintf(subdir_path, PATH_MAX, "%s%s", covers_path, covers_subdir->d_name);
                DIR *subdir = opendir(subdir_path);
                struct dirent *entry;
                while (subdir && (entry = readdir(subdir))) {
                    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                        char entry_path[PATH_MAX];
                        snprintf(entry_path, PATH_MAX, "%s/%s", subdir_path, entry->d_name);

                        /* Test against the cache expiry time (cache invalidation resets are not handled here) */
                        struct stat stat_buf;
                        if (!stat(entry_path, &stat_buf)) {
                            if (stat_buf.st_mtime <= cache_expiry) {
                                trace("%s expired from cache\n", entry_path);
                                remove_cache_item(entry_path, subdir_path, covers_subdir->d_name, entry->d_name);
                            }
                            else if (stat_buf.st_mtime < oldest_mtime) {
                                oldest_mtime = stat_buf.st_mtime;
                            }
                        }
                    }
                }
                closedir(subdir);
            }
            usleep(100000);
        }
        closedir(covers_dir);

        /* Sleep until just after the oldest file expires */
        cache_lock();
        if (cache_expiry_seconds > 0) {
            struct timespec wake_time = {
                .tv_sec = time(NULL) + max(60, oldest_mtime - time(NULL) + cache_expiry_seconds),
                .tv_nsec = 999999
            };
            fprintf(stderr, "Sleeping for %d seconds (oldest file modified %d seconds ago)\n", oldest_mtime - time(NULL) + cache_expiry_seconds, time(NULL) - oldest_mtime);
            pthread_cond_timedwait((pthread_cond_t *)files_cond, (pthread_mutex_t *)files_mutex, &wake_time);
        }
        else {
            fprintf(stderr, "Sleeping forever\n");
            pthread_cond_wait((pthread_cond_t *)files_cond, (pthread_mutex_t *)files_mutex);
        }
    }
    cache_unlock();
}

void cache_configchanged(void)
{
    int32_t new_cache_expiry_seconds = deadbeef->conf_get_int("artwork.cache.period", 48) * 60 * 60;
    if (new_cache_expiry_seconds != cache_expiry_seconds) {
        cache_lock();
        cache_expiry_seconds = new_cache_expiry_seconds;
        deadbeef->cond_signal(files_cond);
        cache_unlock();
    }
}

void start_cache_cleaner(void)
{
    terminate = 0;
    cache_expiry_seconds = deadbeef->conf_get_int("artwork.cache.period", 48) * 60 * 60;
    files_mutex = deadbeef->mutex_create_nonrecursive();
    files_cond = deadbeef->cond_create ();
    tid = deadbeef->thread_start_low_priority(cache_cleaner_thread, NULL);
}

void stop_cache_cleaner(void)
{
    if (tid && files_mutex && files_cond) {
        cache_lock();
        terminate = 1;
        deadbeef->cond_signal(files_cond);
        cache_unlock();
        deadbeef->thread_join(tid);
        tid = 0;
    }
    if (files_mutex) {
        deadbeef->mutex_free(files_mutex);
        files_mutex = 0;
    }
    if (files_cond) {
        deadbeef->cond_free(files_cond);
        files_cond = 0;
    }
}
