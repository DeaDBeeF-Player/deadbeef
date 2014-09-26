/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include "../artwork/artwork.h"
#include "gtkui.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

static DB_artwork_plugin_t *artwork_plugin;

typedef struct {
    struct timeval tm;
    time_t file_time;
    char *fname;
    int width;
    GdkPixbuf *pixbuf;
} cached_pixbuf_t;

#define PRIMARY_CACHE_SIZE 1
#define THUMB_CACHE_SIZE 20
static cached_pixbuf_t primary_cache[PRIMARY_CACHE_SIZE];
static cached_pixbuf_t thumb_cache[THUMB_CACHE_SIZE];
static GdkPixbuf *pixbuf_default;

typedef struct cover_callback_s {
    void (*cb)(void *ud);
    void *ud;
    struct cover_callback_s *next;
} cover_callback_t;

typedef struct load_query_s {
    cached_pixbuf_t *cache;
    char *fname;
    int width;
    cover_callback_t *callback;
    struct load_query_s *next;
} load_query_t;

static int terminate;
static uintptr_t mutex;
static uintptr_t cond;
static uintptr_t tid;
static load_query_t *queue;
static load_query_t *tail;

typedef struct {
    cached_pixbuf_t *cache;
    int width;
    void (*callback)(void *user_data);
    void *user_data;
} cover_avail_info_t;

static void cover_avail_callback(const char *fname, const char *artist, const char *album, void *user_data);

GdkPixbuf *
cover_get_default_pixbuf (void) {
    if (!artwork_plugin) {
        return NULL;
    }
    if (!pixbuf_default) {
        const char *defpath = artwork_plugin->get_default_cover ();
        pixbuf_default = gdk_pixbuf_new_from_file (defpath, NULL);
#if 0
        GError *error = NULL;
        pixbuf_default = gdk_pixbuf_new_from_file (defpath, &error);
        if (!pixbuf_default) {
            fprintf (stderr, "default cover: gdk_pixbuf_new_from_file %s failed, error: %s\n", defpath, error->message);
        }
        if (error) {
            g_error_free (error);
            error = NULL;
        }
#endif
        if (!pixbuf_default) {
            pixbuf_default = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 2, 2);
        }
        assert(pixbuf_default);
    }

    g_object_ref (pixbuf_default);
    return pixbuf_default;
}

int
gtkui_is_default_pixbuf (GdkPixbuf *pb) {
    return pb == pixbuf_default;
}

static size_t
cache_elements(const cached_pixbuf_t *cache)
{
    return cache == primary_cache ? PRIMARY_CACHE_SIZE : THUMB_CACHE_SIZE;
}

static cover_callback_t *
add_callback(void (*cb)(void *ud), void *ud)
{
    if (!cb) {
        return NULL;
    }

    cover_callback_t *callback = malloc(sizeof(cover_callback_t));
    if (!callback) {
        trace("coverart: callback alloc failed\n");
        cb(ud);
        return NULL;
    }

    callback->cb = cb;
    callback->ud = ud;
    callback->next = NULL;
    return callback;
}

static void
queue_add (cached_pixbuf_t *cache, char *fname, const int width, void (*cb)(void *), void *ud)
{
    trace("coverart: queue_add %s @ %d pixels\n", fname, width);
    load_query_t *q = malloc(sizeof(load_query_t));
    if (!q) {
        free(fname);
        if (cb) {
            cb(ud);
        }
        return;
    }

    q->cache = cache;
    q->fname = fname;
    q->width = width;
    q->callback = add_callback(cb, ud);
    q->next = NULL;

    if (tail) {
        tail->next = q;
        tail = q;
    }
    else {
        queue = tail = q;
    }
    deadbeef->cond_signal (cond);
}

static void
queue_add_load (cached_pixbuf_t *cache, const char *fname, const int width, void (*cb)(void *), void *ud)
{
    for (load_query_t *q = queue; q; q = q->next) {
        if (q->fname && !strcmp (q->fname, fname) && width == q->width) {
            trace("coverart: %s already in queue, add to callbacks\n", fname);
            cover_callback_t **last_callback = &q->callback;
            while (*last_callback) {
                last_callback = &(*last_callback)->next;
            }
            *last_callback = add_callback(cb, ud);
            return;
        }
    }

    queue_add(cache, strdup(fname), width, cb, ud);
}

static void
queue_pop (void) {
    load_query_t *next = queue->next;
    if (queue->fname) {
        free (queue->fname);
    }
    free (queue);
    queue = next;
    if (!queue) {
        tail = NULL;
    }
}

static void
send_query_callbacks(cover_callback_t *callback)
{
    if (callback) {
        trace("coverart: make callback to %p (next=%p)\n", callback->cb, callback->next);
        callback->cb(callback->ud);
        send_query_callbacks(callback->next);
        free(callback);
    }
}

static int
cache_sort_order(const char *fname1, const char *fname2, const int width1, const int width2)
{
    const int cmp = strcmp(fname1, fname2);
    return cmp ? cmp : width2 - width1;
}

static int
cache_qsort(const void *a, const void *b)
{
    const cached_pixbuf_t *x = (cached_pixbuf_t *)a;
    const cached_pixbuf_t *y = (cached_pixbuf_t *)b;
    if (x->pixbuf && y->pixbuf) {
        return cache_sort_order(x->fname, y->fname, x->width, y->width);
    }

    return x->pixbuf ? -1 : y->pixbuf ? 1 : 0;
}

static void
load_image(const load_query_t *query)
{
    struct stat stat_buf;
    if (stat(query->fname, &stat_buf)) {
        return;
    }

    char *fname_copy = strdup(query->fname);
    if (!fname_copy) {
        return;
    }

    /* Create a new pixbuf from this file */
    deadbeef->mutex_unlock(mutex);
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(query->fname, query->width, query->width, TRUE, NULL);
#if 0
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(query->fname, query->width, query->width, TRUE, &error);
    if (error) {
        fprintf (stderr, "gdk_pixbuf_new_from_file_at_scale %s %d failed, error: %s\n", query->fname, query->width, error ? error->message : "n/a");
        g_error_free(error);
    }
#endif
    if (!pixbuf) {
        trace("covercache: unable to create pixbuf from cached image file %s, use default\n", query->fname);
        pixbuf = cover_get_default_pixbuf();
    }
    trace("covercache: loaded pixbuf from %s\n", query->fname);
    deadbeef->mutex_lock(mutex);

    /* If the last slot is filled then evict the oldest entry */
    cached_pixbuf_t *cache = query->cache;
    const size_t cache_size = cache_elements(cache);
    size_t cache_idx = cache_size - 1;
    if (cache[cache_idx].pixbuf) {
        struct timeval *min_time = &cache[cache_idx].tm;
        for (size_t i = 0; i < cache_size-1; i++) {
            if (cache[i].tm.tv_sec < min_time->tv_sec || cache[i].tm.tv_sec == min_time->tv_sec && cache[i].tm.tv_usec < min_time->tv_usec) {
                cache_idx = i;
                min_time = &cache[i].tm;
            }
        }
        trace("covercache: evict %s\n", cache[cache_idx].fname);
        g_object_unref(cache[cache_idx].pixbuf);
        free(cache[cache_idx].fname);
    }

    /* Set the pixbuf in the cache slot */
    cache[cache_idx].pixbuf = pixbuf;
    cache[cache_idx].fname = fname_copy;
    cache[cache_idx].file_time = stat_buf.st_mtime;
    gettimeofday(&cache[cache_idx].tm, NULL);
    cache[cache_idx].width = query->width;

    /* Sort the cache by fname, largest first, then empty slots at the end */
    qsort(cache, cache_size, sizeof(cached_pixbuf_t), cache_qsort);
}

static void
loading_thread (void *none) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-gtkui-artwork", 0, 0, 0, 0);
#endif

    deadbeef->mutex_lock(mutex);

    while (!terminate) {
        trace("covercache: waiting for signal...\n");
        pthread_cond_wait((pthread_cond_t *)cond, (pthread_mutex_t *)mutex);
        trace("covercache: signal received (terminate=%d, queue=%p)\n", terminate, queue);

        while (!terminate && queue) {
            if (queue->fname) {
                load_image(queue);
            }
            if (artwork_plugin) {
                send_query_callbacks(queue->callback);
            }
            queue_pop();
        }
    }

    deadbeef->mutex_unlock(mutex);
}

static GdkPixbuf *
get_pixbuf (cached_pixbuf_t *cache, const char *fname, int width, void (*callback)(void *user_data), void *user_data) {
    /* Look in the pixbuf cache */
    const size_t cache_size = cache_elements(cache);
    for (size_t i = 0; i < cache_size && cache[i].pixbuf; i++) {
        if (!cache_sort_order(cache[i].fname, fname, cache[i].width, width)) {
            struct stat stat_buf;
            if (!stat(fname, &stat_buf) && stat_buf.st_mtime == cache[i].file_time) {
                gettimeofday(&cache[i].tm, NULL);
                g_object_ref(cache[i].pixbuf);
                return cache[i].pixbuf;
            }
            g_object_unref(cache[i].pixbuf);
            cache[i].pixbuf = NULL;
            free(cache[i].fname);
            qsort(cache, cache_size, sizeof(cached_pixbuf_t), cache_qsort);
        }
    }

#if 0
    printf ("cache miss: %s/%d\n", fname, width);
    for (size_t i = 0; i < cache_size; i++) {
        if (cache[i].pixbuf) {
            printf ("    cache line %d: %s/%d\n", i, cache[i].fname, cache[i].width);
        }
    }
#endif

    /* Request to load this image into the pixbuf cache */
    queue_add_load(cache, fname, width, callback, user_data);
    return NULL;
}

void
queue_cover_callback (void (*callback)(void *user_data), void *user_data) {
    if (artwork_plugin && callback) {
        deadbeef->mutex_lock (mutex);
        queue_add(NULL, NULL, -1, callback, user_data);
        deadbeef->mutex_unlock (mutex);
    }
}

static GdkPixbuf *
best_cached_pixbuf(const cached_pixbuf_t *cache, const char *path)
{
    /* Find the largest pixbuf in the cache for this file */
    const size_t cache_size = cache_elements(cache);
    for (size_t i = 0; i < cache_size && cache[i].pixbuf; i++) {
        if (!strcmp(cache[i].fname, path)) {
            g_object_ref(cache[i].pixbuf);
            return cache[i].pixbuf;
        }
    }

    return NULL;
}

static GdkPixbuf *
get_cover_art_int(cached_pixbuf_t *cache, const char *fname, const char *artist, const char *album, int width, void (*callback)(void *), void *user_data)
{
    if (!artwork_plugin) {
        return NULL;
    }

    if (width == -1) {
        /* Find the largest cached pixmap for this file */
        char path[PATH_MAX];
        artwork_plugin->make_cache_path2(path, sizeof (path), fname, album, artist, -1);
        trace("coverart: get largest pixbuf matching %s\n", path);
        deadbeef->mutex_lock(mutex);
        GdkPixbuf *best_pixbuf = best_cached_pixbuf(cache, path);
        deadbeef->mutex_unlock(mutex);
        return best_pixbuf;
    }

    /* Get a pixbuf of an exact size */
    trace("coverart: get_album_art for %s %s %s %d\n", fname, artist, album, width);
    cover_avail_info_t *dt = malloc(sizeof (cover_avail_info_t));
    if (dt) {
        dt->cache = cache;
        dt->width = width;
        dt->callback = callback;
        dt->user_data = user_data;
    }
    char *image_fname = artwork_plugin->get_album_art(fname, artist, album, -1, cover_avail_callback, dt);
    if (image_fname) {
        deadbeef->mutex_lock(mutex);
        GdkPixbuf *pb = get_pixbuf(cache, image_fname, width, callback, user_data);
        deadbeef->mutex_unlock(mutex);
        free(image_fname);
        return pb;
    }
    return cover_get_default_pixbuf();
}

// Deprecated
GdkPixbuf *
get_cover_art_callb (const char *fname, const char *artist, const char *album, int width, void (*callback) (void *), void *user_data)
{
    get_cover_art_int(thumb_cache, fname, artist, album, width, callback, user_data);
}

GdkPixbuf *
get_cover_art_primary (const char *fname, const char *artist, const char *album, int width, void (*callback) (void *), void *user_data)
{
    get_cover_art_int(primary_cache, fname, artist, album, width, callback, user_data);
}

GdkPixbuf *
get_cover_art_thumb (const char *fname, const char *artist, const char *album, int width, void (*callback) (void *), void *user_data)
{
    get_cover_art_int(thumb_cache, fname, artist, album, width, callback, user_data);
}

static void
cover_avail_callback (const char *fname, const char *artist, const char *album, void *user_data) {
    if (!fname || !user_data) {
        free(user_data);
        return;
    }

    /* Image file has been saved into disk cache, now load it into pixbuf cache */
    cover_avail_info_t *dt = user_data;
    GdkPixbuf *pb = get_cover_art_int(dt->cache, fname, artist, album, dt->width, dt->callback, dt->user_data);
    if (pb) {
        g_object_unref(pb);
    }
    free(dt);
}

void
coverart_reset_queue (void) {
    deadbeef->mutex_lock (mutex);
    if (queue) {
        load_query_t *q = queue->next;
        while (q) {
            load_query_t *next = q->next;
            if (q->fname) {
                free (q->fname);
            }
            free (q);
            q = next;
        }
        queue->next = NULL;
        tail = queue;
    }
    deadbeef->mutex_unlock (mutex);

    if (artwork_plugin) {
        artwork_plugin->reset (1);
    }
}

void
cover_art_init (void) {
    terminate = 0;
    mutex = deadbeef->mutex_create_nonrecursive ();
    cond = deadbeef->cond_create ();
    if (mutex && cond) {
        tid = deadbeef->thread_start_low_priority (loading_thread, NULL);
    }

    if (tid) {
        const DB_plugin_t *plugin = deadbeef->plug_get_for_id("artwork");
        if (plugin && PLUG_TEST_COMPAT(plugin, 1, 2)) {
            artwork_plugin = (DB_artwork_plugin_t *)plugin;
        }
    }
}

void
cover_art_disconnect (void) {
    if (artwork_plugin) {
        const DB_artwork_plugin_t *plugin = artwork_plugin;
        artwork_plugin = NULL;
        trace("coverart: resetting artwork plugin...\n");
        plugin->reset(0);
    }
}

static void
clear_pixbuf_cache(cached_pixbuf_t *cache, const size_t cache_size)
{
    for (size_t i = 0; i < cache_size; i++) {
        if (cache[i].pixbuf) {
            g_object_unref(cache[i].pixbuf);
            free(cache[i].fname);
        }
    }
    memset(cache, '\0', sizeof(cached_pixbuf_t) * cache_size);
}

void
cover_art_free (void) {
    trace ("coverart: terminating cover art loader...\n");

    if (tid) {
        deadbeef->mutex_lock(mutex);
        terminate = 1;
        trace("coverart: sending terminate signal to art loader thread...\n");
        deadbeef->cond_signal(cond);
        deadbeef->mutex_unlock(mutex);
        deadbeef->thread_join(tid);
        tid = 0;
    }

    while (queue) {
        queue_pop();
    }

    if (cond) {
        deadbeef->cond_free(cond);
        cond = 0;
    }
    if (mutex) {
        deadbeef->mutex_free(mutex);
        mutex = 0;
    }

    clear_pixbuf_cache(primary_cache, PRIMARY_CACHE_SIZE);
    clear_pixbuf_cache(thumb_cache, THUMB_CACHE_SIZE);
    if (pixbuf_default) {
        g_object_unref(pixbuf_default);
        pixbuf_default = NULL;
    }

    trace("coverart: objects all freed\n");
}
