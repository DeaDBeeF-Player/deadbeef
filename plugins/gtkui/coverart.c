/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include "../../deadbeef.h"
#include "../artwork-legacy/artwork.h"
#include "gtkui.h"
#include "coverart.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

static DB_artwork_plugin_t *artwork_plugin;

typedef struct {
    struct timeval tm;
    time_t file_time;
    char *fname;
    int width;
    int height;
    GdkPixbuf *pixbuf;
} cached_pixbuf_t;

typedef enum {
    CACHE_TYPE_PRIMARY = 0,
    CACHE_TYPE_THUMB
} cache_type_t;

#define PRIMARY_CACHE_SIZE 1
static size_t thumb_cache_size;
static cached_pixbuf_t primary_cache[PRIMARY_CACHE_SIZE];
static cached_pixbuf_t *thumb_cache;
static GdkPixbuf *pixbuf_default;
static size_t thrash_count;

typedef struct cover_callback_s {
    cover_avail_callback_t cb;
    void *ud;
    struct cover_callback_s *next;
} cover_callback_t;

typedef struct load_query_s {
    cache_type_t cache_type;
    char *fname;
    int width;
    int height;
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
    cache_type_t cache_type;
    char *cache_path;
    int width;
    int height;
    cover_avail_callback_t callback;
    void *user_data;
} cover_avail_info_t;

GdkPixbuf *
cover_get_default_pixbuf(void)
{
    if (!artwork_plugin) {
        return NULL;
    }

    /* get_default_cover=NULL means it was reset and we call again to get the new value */
    if (!artwork_plugin->get_default_cover() && pixbuf_default) {
        g_object_unref(pixbuf_default);
        pixbuf_default = NULL;
    }

    /* Load the default cover image into a pixbuf */
    if (!pixbuf_default) {
        const char *defpath = artwork_plugin->get_default_cover();
        if (defpath && defpath[0]) {
            pixbuf_default = gdk_pixbuf_new_from_file(defpath, NULL);
#if 0
            GError *error = NULL;
            pixbuf_default = gdk_pixbuf_new_from_file(defpath, &error);
            if (!pixbuf_default) {
                fprintf (stderr, "default cover: gdk_pixbuf_new_from_file %s failed, error: %s\n", defpath, error->message);
            }
            if (error) {
                g_error_free (error);
                error = NULL;
            }
#endif
        }

        /* If we have a blank path or an error, just create a transparent pixbuf */
        if (!pixbuf_default) {
            pixbuf_default = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 2, 2);
            gdk_pixbuf_fill(pixbuf_default, 0x00000000);
        }
    }

    g_object_ref(pixbuf_default);
    return pixbuf_default;
}

int
gtkui_is_default_pixbuf (GdkPixbuf *pb) {
    return pb == pixbuf_default;
}

static cover_callback_t *
add_callback(cover_avail_callback_t cb, void *ud)
{
    if (!cb) {
        return NULL;
    }

    cover_callback_t *callback = malloc(sizeof(cover_callback_t));
    if (!callback) {
        trace("coverart: callback alloc failed\n");
        return NULL;
    }

    callback->cb = cb;
    callback->ud = ud;
    callback->next = NULL;
    return callback;
}

static void
process_query_callbacks(cover_callback_t *callback, int send)
{
    if (callback) {
        if (send) {
            trace("coverart: make callback to %p (next=%p)\n", callback->cb, callback->next);
            callback->cb(callback->ud);
        }
        process_query_callbacks(callback->next, send);
        free(callback);
    }
}

static void
queue_add (cache_type_t cache_type, char *fname, int width, int height, cover_avail_callback_t cb, void *ud)
{
    trace("coverart: queue_add %s @ %ix%i pixels\n", fname, width, height);
    load_query_t *q = malloc(sizeof(load_query_t));
    if (!q) {
        free(fname);
        return;
    }

    q->cache_type = cache_type;
    q->fname = fname;
    q->width = width;
    q->height = height;
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
queue_add_load (cache_type_t cache_type, char *fname, int width, int height, cover_avail_callback_t cb, void *ud)
{
    for (load_query_t *q = queue; q; q = q->next) {
        if (q->fname && !strcmp (q->fname, fname) && width == q->width && height == q->height) {
            trace("coverart: %s already in queue, add to callbacks\n", fname);
            cover_callback_t **last_callback = &q->callback;
            while (*last_callback) {
                last_callback = &(*last_callback)->next;
            }
            *last_callback = add_callback(cb, ud);
            free(fname);
            return;
        }
    }

    queue_add(cache_type, fname, width, height, cb, ud);
}

static load_query_t *
queue_remove(load_query_t *q)
{
    process_query_callbacks(q->callback, FALSE);
    load_query_t *next = q->next;
    if (q->fname) {
        free(q->fname);
    }
    free(q);
    return next;
}

static void
queue_pop (void)
{
    queue = queue_remove(queue);
    if (!queue) {
        tail = NULL;
    }
}

static cached_pixbuf_t *
cache_location(cache_type_t cache_type)
{
    return cache_type == CACHE_TYPE_PRIMARY ? primary_cache : thumb_cache;
}

static size_t
cache_elements(cache_type_t cache_type)
{
    return cache_type == CACHE_TYPE_PRIMARY ? PRIMARY_CACHE_SIZE : thumb_cache_size;
}

static int
cache_qsort(const void *a, const void *b)
{
    const cached_pixbuf_t *x = (cached_pixbuf_t *)a;
    const cached_pixbuf_t *y = (cached_pixbuf_t *)b;
    if (x->pixbuf && y->pixbuf) {
        const int cmp = strcmp(x->fname, y->fname);
        if (cmp) {
            return cmp;
        }

        if (y->width != x->width) {
            return y->width - x->width;
        }

        return y->height - x->height;
    }

    return x->pixbuf ? -1 : y->pixbuf ? 1 : 0;
}

static int
timeval_older(const struct timeval *tm1, const struct timeval *tm2)
{
    return tm1->tv_sec < tm2->tv_sec || tm1->tv_sec == tm2->tv_sec && tm1->tv_usec < tm2->tv_usec;
}

static void
evict_pixbuf(cached_pixbuf_t *cached)
{
    trace("covercache: evict %s\n", cached->fname);
    g_object_unref(cached->pixbuf);
    cached->pixbuf = NULL;
    free(cached->fname);
}

static cached_pixbuf_t *
oldest_slot(cached_pixbuf_t *cache, const size_t cache_size)
{
    cached_pixbuf_t *oldest_slot = cache;
    for (size_t i = 1; i < cache_size; i++) {
        if (timeval_older(&cache[i].tm, &oldest_slot->tm)) {
            oldest_slot = cache + i;
        }
    }
    return oldest_slot;
}

static int
adjust_cache(struct timeval *oldest)
{
    /* A possible thrash is when the oldest pixbuf is still fresh */
    struct timeval now;
    gettimeofday(&now, NULL);
    now.tv_sec -= 2 + thumb_cache_size / 10;
    thrash_count = timeval_older(&now, oldest) ? thrash_count+1 : 0;

    /* Grab more space more quickly at small cache sizes */
    if (thrash_count*2 >= thumb_cache_size) {
        cached_pixbuf_t *new_thumb_cache = realloc(thumb_cache, sizeof(cached_pixbuf_t) * thumb_cache_size * 2);
        if (new_thumb_cache) {
            memset(&new_thumb_cache[thumb_cache_size], '\0', sizeof(cached_pixbuf_t) * thumb_cache_size);
            thumb_cache_size *= 2;
            thumb_cache = new_thumb_cache;
            trace("coverart: pixbuf cache size increased to %d\n", (int)thumb_cache_size);
            return 1;
        }
    }

    return 0;
}

static void
cache_add(cache_type_t cache_type, GdkPixbuf *pixbuf, char *fname, const time_t file_time, int width, int height)
{
    cached_pixbuf_t *cache = cache_location(cache_type);
    size_t cache_size = cache_elements(cache_type);
    cached_pixbuf_t *cache_slot = &cache[cache_size-1];

    /* If the last slot is filled then evict the oldest entry */
    if (cache_slot->pixbuf) {
        if (cache_type == CACHE_TYPE_THUMB) {
            cache_slot = oldest_slot(cache, cache_size);
            if (adjust_cache(&cache_slot->tm)) {
                cache = cache_location(cache_type);
                cache_slot = &cache[cache_size];
                cache_size = cache_elements(cache_type);
            }
        }
        if (cache_slot->pixbuf) {
            evict_pixbuf(cache_slot);
        }
    }

    /* Set the pixbuf in the cache slot */
    cache_slot->pixbuf = pixbuf;
    cache_slot->fname = fname;
    cache_slot->file_time = file_time;
    gettimeofday(&cache_slot->tm, NULL);
    cache_slot->width = width;
    cache_slot->height = height;

    /* Sort by fname, largest first, with empty slots at the end */
    qsort(cache, cache_size, sizeof(cached_pixbuf_t), cache_qsort);
}

static void
load_image(load_query_t *query)
{
    deadbeef->mutex_unlock(mutex);
    struct stat stat_buf;
    if (stat(query->fname, &stat_buf)) {
        deadbeef->mutex_lock(mutex);
        return;
    }

    /* Create a new pixbuf from this file */
    int width = query->width;
    int height = query->height;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(query->fname, width, height, NULL);
#if 0
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(query->fname, width, height, &error);
    if (error) {
        fprintf (stderr, "gdk_pixbuf_new_from_file_at_size %s %d failed, error: %s\n", query->fname, width, error ? error->message : "n/a");
        g_error_free(error);
    }
#endif
    if (!pixbuf) {
        trace("covercache: unable to create pixbuf from cached image file %s, use default\n", query->fname);
        pixbuf = cover_get_default_pixbuf();
        width = -1;
        height = -1;
    }
    trace("covercache: loaded pixbuf from %s\n", query->fname);

    /* Cache the pixbuf */
    deadbeef->mutex_lock(mutex);
    cache_add(query->cache_type, pixbuf, query->fname, stat_buf.st_mtime, width, height);
    query->fname = NULL;
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
                process_query_callbacks(queue->callback, TRUE);
                queue->callback = NULL;
            }
            queue_pop();
        }
    }

    deadbeef->mutex_unlock(mutex);
}

static GdkPixbuf *
get_pixbuf (cache_type_t cache_type, const char *fname, int width, int height) {
    /* Look in the pixbuf cache */
    cached_pixbuf_t *cache = cache_location(cache_type);
    const size_t cache_size = cache_elements(cache_type);
    for (size_t i = 0; i < cache_size && cache[i].pixbuf; i++) {
        /* Look for a cached pixbuf that matches the filename and size required */
        if (!strcmp(cache[i].fname, fname) && (cache[i].width == -1 || cache[i].width == width && cache[i].height == height)) {
            struct stat stat_buf;
            /* Keep the pixbuf for now if the disk file is missing */
            if (stat(fname, &stat_buf) || stat_buf.st_mtime == cache[i].file_time) {
                gettimeofday(&cache[i].tm, NULL);
                return cache[i].pixbuf;
            }
            /* Discard all pixbufs for this file if the disk modification time doesn't match */
            for (size_t j = 0; j < cache_size && cache[j].pixbuf; j++) {
                if (!strcmp(cache[j].fname, fname)) {
                    evict_pixbuf(&cache[j]);
                }
            }
            qsort(cache, cache_size, sizeof(cached_pixbuf_t), cache_qsort);
        }
    }
    return NULL;
}

void
queue_cover_callback (cover_avail_callback_t callback, void *user_data) {
    if (artwork_plugin && callback) {
        deadbeef->mutex_lock (mutex);
        queue_add(-1, NULL, -1, -1, callback, user_data);
        deadbeef->mutex_unlock (mutex);
    }
}

static void
album_art_avail_callback(const char *fname, const char *artist, const char *album, void *user_data)
{
    if (!user_data) {
        return;
    }

    cover_avail_info_t *dt = user_data;

    deadbeef->mutex_lock(mutex);
    if (fname) {
        /* An image file is in the disk cache, load it to the pixbuf cache */
        trace("album_art_avail_callback: add to queue %s, %s\n", fname, dt->cache_path);
        queue_add_load(dt->cache_type, dt->cache_path, dt->width, dt->height, dt->callback, dt->user_data);
    }
    else if (get_pixbuf(dt->cache_type, dt->cache_path, dt->width, dt->height)) {
        /* Pixbuf (usually the default) already cached */
        trace("album_art_avail_callback: pixbuf already in cache, do nothing for %s\n", dt->cache_path);
        free(dt->cache_path);
    }
    else {
        /* Put the default pixbuf in the cache because no artwork was found */
        struct stat stat_buf;
        if (!stat(dt->cache_path, &stat_buf)) {
            trace("album_art_avail_callback: cache default pixbuf for %s\n", dt->cache_path);
            cache_add(dt->cache_type, cover_get_default_pixbuf(), dt->cache_path, stat_buf.st_mtime, -1, -1);
        }
        else {
            /* Image file unexpectedly missing or not empty (unlucky timing on cache expiry, reset, etc) */
            trace("album_art_avail_callback: file gone, do nothing for %s\n", dt->cache_path);
            free(dt->cache_path);
        }
        if (dt->callback) {
            dt->callback(dt->user_data);
        }
    }
    deadbeef->mutex_unlock(mutex);

    free(user_data);
}

static GdkPixbuf *
best_cached_pixbuf(cache_type_t cache_type, const char *path)
{
    /* Find the largest pixbuf in the cache for this file */
    cached_pixbuf_t *cache = cache_location(cache_type);
    const size_t cache_size = cache_elements(cache_type);
    for (size_t i = 0; i < cache_size && cache[i].pixbuf; i++) {
        if (!strcmp(cache[i].fname, path)) {
            g_object_ref(cache[i].pixbuf);
            return cache[i].pixbuf;
        }
    }

    return NULL;
}

static cover_avail_info_t *
cover_avail_info(cache_type_t cache_type, char *cache_path, int width, int height, cover_avail_callback_t callback, void *user_data)
{
    if (cache_path) {
        cover_avail_info_t *dt = malloc(sizeof(cover_avail_info_t));
        if (dt) {
            dt->cache_type = cache_type;
            dt->cache_path = cache_path;
            dt->width = width;
            dt->height = height;
            dt->callback = callback;
            dt->user_data = user_data;
            return dt;
        }
    }

    if (callback) {
        callback(user_data);
    }
    return NULL;
}

static GdkPixbuf *
get_cover_art_int(cache_type_t cache_type, const char *fname, const char *artist, const char *album, int width, int height, cover_avail_callback_t callback, void *user_data)
{
    if (!artwork_plugin) {
        return NULL;
    }

    char cache_path[PATH_MAX];
    artwork_plugin->make_cache_path2(cache_path, PATH_MAX, fname, album, artist, -1);

    if (width == -1) {
        /* Find the largest cached pixmap for this file */
        trace("coverart: get largest pixbuf matching %s\n", cache_path);
        deadbeef->mutex_lock(mutex);
        GdkPixbuf *best_pixbuf = best_cached_pixbuf(cache_type, cache_path);
        deadbeef->mutex_unlock(mutex);
        return best_pixbuf;
    }

    /* Get a pixbuf of an exact size (or the default pixbuf) */
    trace("coverart: get_album_art for %s %s %s %ix%i\n", fname, artist, album, width, height);
    cover_avail_info_t *dt = cover_avail_info(cache_type, strdup(cache_path), width, height, callback, user_data);
    char *image_fname = artwork_plugin->get_album_art(fname, artist, album, -1, album_art_avail_callback, dt);
    if (image_fname) {
        /* There will be no callback */
        free(dt->cache_path);
        free(dt);
    }

    deadbeef->mutex_lock(mutex);
    GdkPixbuf *pb = get_pixbuf(cache_type, cache_path, width, height);
    if (pb) {
        /* We already have the proper pixbuf in memory */
        g_object_ref(pb);
        if (image_fname) {
            free(image_fname);
        }
    }
    else if (image_fname) {
        /* Got a cached file, need to load a pixbuf into memory */
        queue_add_load(cache_type, image_fname, width, height, callback, user_data);
    }
    deadbeef->mutex_unlock(mutex);
    return pb;
}

// Deprecated
GdkPixbuf *
get_cover_art_callb (const char *fname, const char *artist, const char *album, int width, cover_avail_callback_t callback, void *user_data)
{
    return get_cover_art_int(CACHE_TYPE_THUMB, fname, artist, album, width, -1, callback, user_data);
}

GdkPixbuf *
get_cover_art_primary (const char *fname, const char *artist, const char *album, int width, cover_avail_callback_t callback, void *user_data)
{
    return get_cover_art_int(CACHE_TYPE_PRIMARY, fname, artist, album, width, -1, callback, user_data);
}

GdkPixbuf *
get_cover_art_primary_by_size (const char *fname, const char *artist, const char *album, int width, int height, cover_avail_callback_t callback, void *user_data)
{
    return get_cover_art_int(CACHE_TYPE_PRIMARY, fname, artist, album, width, height, callback, user_data);
}

GdkPixbuf *
get_cover_art_thumb (const char *fname, const char *artist, const char *album, int width, cover_avail_callback_t callback, void *user_data)
{
    return get_cover_art_int(CACHE_TYPE_THUMB, fname, artist, album, width, -1, callback, user_data);
}

GdkPixbuf *
get_cover_art_thumb_by_size (const char *fname, const char *artist, const char *album, int width, int height, cover_avail_callback_t callback, void *user_data)
{
    return get_cover_art_int(CACHE_TYPE_THUMB, fname, artist, album, width, height, callback, user_data);
}

void
coverart_reset_queue (void) {
    if (!artwork_plugin) {
        return;
    }
    trace("coverart: reset queue\n");
    deadbeef->mutex_lock (mutex);
    if (queue) {
        load_query_t *keep_primary = NULL;
        load_query_t *q = queue->next;
        while (q) {
            if (q->cache_type == CACHE_TYPE_PRIMARY) {
                if (keep_primary) {
                    queue_remove(keep_primary);
                }
                keep_primary = q;
                q = q->next;
            }
            else {
                q = queue_remove(q);
            }
        }
        if (keep_primary) {
            queue->next = keep_primary;
            keep_primary->next = NULL;
            tail = keep_primary;
        }
        else {
            queue->next = NULL;
            tail = queue;
        }
    }
    thrash_count /= 2;
    deadbeef->mutex_unlock (mutex);

    if (artwork_plugin) {
        artwork_plugin->reset (1);
    }
}

void
cover_art_init (void) {
    const DB_plugin_t *plugin = deadbeef->plug_get_for_id("artwork");
    if (plugin && PLUG_TEST_COMPAT(plugin, 1, DDB_ARTWORK_VERSION)) {
        artwork_plugin = (DB_artwork_plugin_t *)plugin;
    }
    if (!artwork_plugin) {
        return;
    }
    thumb_cache_size = 2;
    thumb_cache = calloc(2, sizeof(cached_pixbuf_t));
    if (!thumb_cache) {
        return;
    }

    terminate = 0;
    mutex = deadbeef->mutex_create_nonrecursive ();
    cond = deadbeef->cond_create ();
    if (mutex && cond) {
        tid = deadbeef->thread_start_low_priority (loading_thread, NULL);
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
    for (size_t i = 0; i < cache_size && cache[i].pixbuf; i++) {
        evict_pixbuf(cache+i);
    }
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
        queue = queue_remove(queue);
    }
    tail = NULL;

    if (cond) {
        deadbeef->cond_free(cond);
        cond = 0;
    }
    if (mutex) {
        deadbeef->mutex_free(mutex);
        mutex = 0;
    }

    clear_pixbuf_cache(primary_cache, PRIMARY_CACHE_SIZE);
    clear_pixbuf_cache(thumb_cache, thumb_cache_size);
    free(thumb_cache);
    thumb_cache_size = 0;

    if (pixbuf_default) {
        g_object_unref(pixbuf_default);
        pixbuf_default = NULL;
    }

    trace("coverart: objects all freed\n");
}
