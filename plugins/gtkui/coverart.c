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
#include <gtk/gtk.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include "coverart.h"
#include "../artwork/artwork.h"
#include "gtkui.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

extern DB_artwork_plugin_t *coverart_plugin;


GdkPixbuf *pixbuf_default;

#define MAX_ID 256
#define CACHE_SIZE 20

typedef struct {
    struct timeval tm;
    time_t file_time;
    char *fname;
    int width;
    GdkPixbuf *pixbuf;
} cached_pixbuf_t;

#define MAX_CALLBACKS 200

typedef struct cover_callback_s {
    void (*cb) (void*ud);
    void *ud;
} cover_callback_t;

typedef struct load_query_s {
    char *fname;
    int width;
    cover_callback_t callbacks[MAX_CALLBACKS];
    int numcb;
    struct load_query_s *next;
} load_query_t;

static cached_pixbuf_t cache[CACHE_SIZE];
static int terminate = 0;
static uintptr_t mutex;
static uintptr_t cond;
static uintptr_t tid;
load_query_t *queue;
load_query_t *tail;

static int64_t artwork_reset_time;

static void
queue_add (const char *fname, int width, void (*callback) (void *user_data), void *user_data) {
    deadbeef->mutex_lock (mutex);
    load_query_t *q;
    if (fname) {
        for (q = queue; q; q = q->next) {
            if (q->fname && !strcmp (q->fname, fname) && width == q->width) {
                if (q->numcb < MAX_CALLBACKS && callback) {
                    q->callbacks[q->numcb].cb = callback;
                    q->callbacks[q->numcb].ud = user_data;
                    q->numcb++;
                }
                deadbeef->mutex_unlock (mutex);
                return;
            }
        }
    }
    q = malloc (sizeof (load_query_t));
    memset (q, 0, sizeof (load_query_t));
    if (fname) {
        q->fname = strdup (fname);
    }
    q->width = width;
    q->callbacks[q->numcb].cb = callback;
    q->callbacks[q->numcb].ud = user_data;
    q->numcb++;
    if (tail) {
        tail->next = q;
        tail = q;
    }
    else {
        queue = tail = q;
    }
    deadbeef->mutex_unlock (mutex);
    deadbeef->cond_signal (cond);
}

static void
queue_pop (void) {
    deadbeef->mutex_lock (mutex);
    load_query_t *next = queue->next;
    if (queue->fname) {
        free (queue->fname);
    }
    free (queue);
    queue = next;
    if (!queue) {
        tail = NULL;
    }
    deadbeef->mutex_unlock (mutex);
}

void
loading_thread (void *none) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-gtkui-artwork", 0, 0, 0, 0);
#endif
    for (;;) {
        trace ("covercache: waiting for signal\n");
        deadbeef->cond_wait (cond, mutex);
        trace ("covercache: signal received (terminate=%d, queue=%p)\n", terminate, queue);
        deadbeef->mutex_unlock (mutex);
        while (!terminate && queue) {
            int cache_min = 0;
            deadbeef->mutex_lock (mutex);
            for (int i = 0; i < CACHE_SIZE; i++) {
                if (!cache[i].pixbuf) {
                    cache_min = i;
                    break;
                }
                if (cache[cache_min].pixbuf && cache[i].pixbuf) {
                    if (cache[cache_min].tm.tv_sec > cache[i].tm.tv_sec) {
                        cache_min = i;
                    }
                }
            }
            if (cache_min != -1) {
                if (cache[cache_min].pixbuf) {
                    g_object_unref (cache[cache_min].pixbuf);
                    cache[cache_min].pixbuf = NULL;
                }
                if (cache[cache_min].fname) {
                    free (cache[cache_min].fname);
                    cache[cache_min].fname = NULL;
                }
            }
            deadbeef->mutex_unlock (mutex);
            if (!queue->fname) {
                for (int i = 0; i < queue->numcb; i++) {
                    if (queue->callbacks[i].cb) {
                        queue->callbacks[i].cb (queue->callbacks[i].ud);
                    }
                }
                queue_pop ();
                continue;
            }

            if (cache_min == -1) {
                trace ("coverart pixbuf cache overflow, waiting...\n");
                usleep (500000);
                continue;
            }
            GdkPixbuf *pixbuf = NULL;
            GError *error = NULL;
            struct stat stat_buf;
            if (!stat (queue->fname, &stat_buf)) {
                pixbuf = gdk_pixbuf_new_from_file_at_scale (queue->fname, queue->width, queue->width, TRUE, &error);
                if (error) {
                    //fprintf (stderr, "gdk_pixbuf_new_from_file_at_scale %s %d failed, error: %s\n", queue->fname, queue->width, error ? error->message : "n/a");
                    g_error_free (error);
                    error = NULL;
                }
            }
            if (!pixbuf) {
                pixbuf = pixbuf_default;
                g_object_ref (pixbuf);
            }
            if (cache_min != -1) {
                deadbeef->mutex_lock (mutex);
                cache[cache_min].pixbuf = pixbuf;
                cache[cache_min].fname = strdup (queue->fname);
                cache[cache_min].file_time  = stat_buf.st_mtime;
                gettimeofday (&cache[cache_min].tm, NULL);
                cache[cache_min].width = queue->width;
                deadbeef->mutex_unlock (mutex);
            }

            for (int i = 0; i < queue->numcb; i++) {
                if (queue->callbacks[i].cb) {
                    queue->callbacks[i].cb (queue->callbacks[i].ud);
                }
            }
            queue_pop ();
        }
        if (terminate) {
            break;
        }
    }
}

typedef struct {
    int width;
    void (*callback)(void *user_data);
    void *user_data;
} cover_avail_info_t;

static void
cover_avail_callback (const char *fname, const char *artist, const char *album, void *user_data) {
    if (!fname) {
        free (user_data);
        return;
    }
    cover_avail_info_t *dt = user_data;
    // means requested image is now in disk cache
    // load it into main memory
    GdkPixbuf *pb = get_cover_art_callb (fname, artist, album, dt->width, dt->callback, dt->user_data);
    if (pb) {
        g_object_unref (pb);
    }
    free (dt);
}

static GdkPixbuf *
get_pixbuf (const char *fname, int width, void (*callback)(void *user_data), void *user_data) {
    int requested_width = width;
    // find in cache
    deadbeef->mutex_lock (mutex);
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].pixbuf) {
            if (!strcmp (fname, cache[i].fname) && cache[i].width == width) {
                gettimeofday (&cache[i].tm, NULL);
                GdkPixbuf *pb = cache[i].pixbuf;
                g_object_ref (pb);
                deadbeef->mutex_unlock (mutex);
                return pb;
            }
        }
    }
#if 0
    printf ("cache miss: %s/%d\n", fname, width);
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].pixbuf) {
            printf ("    cache line %d: %s/%d\n", i, cache[i].fname, cache[i].width);
        }
    }
#endif
    deadbeef->mutex_unlock (mutex);
    queue_add (fname, width, callback, user_data);
    return NULL;
}

void
queue_cover_callback (void (*callback)(void *user_data), void *user_data) {
    queue_add (NULL, -1, callback, user_data);
}


GdkPixbuf *
get_cover_art_callb (const char *fname, const char *artist, const char *album, int width, void (*callback) (void *user_data), void *user_data) {
    if (!coverart_plugin) {
        return NULL;
    }

    if (width == -1) {
        char path[2048];
        coverart_plugin->make_cache_path2 (path, sizeof (path), fname, album, artist, -1);
        deadbeef->mutex_lock (mutex);
        int i_largest = -1;
        int size_largest = -1;
        for (int i = 0; i < CACHE_SIZE; i++) {
            if (!cache[i].pixbuf) {
                continue;
            }
            if (!strcmp (cache[i].fname, path)) {
                gettimeofday (&cache[i].tm, NULL);
                if (cache[i].width > size_largest) {
                    size_largest = cache[i].width;
                    i_largest = i;
                }
            }
        }
        if (i_largest != -1) {
            GdkPixbuf *pb = cache[i_largest].pixbuf;
            g_object_ref (pb);
            deadbeef->mutex_unlock (mutex);
            return pb;
        }
        deadbeef->mutex_unlock (mutex);
        return NULL;
    }

    cover_avail_info_t *dt = malloc (sizeof (cover_avail_info_t));
    dt->width = width;
    dt->callback = callback;
    dt->user_data = user_data;
    char *image_fname = coverart_plugin->get_album_art (fname, artist, album, -1, cover_avail_callback, dt);
    if (image_fname) {
        GdkPixbuf *pb = get_pixbuf (image_fname, width, callback, user_data);
        free (image_fname);
        return pb;
    }
    return NULL;
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
    if (coverart_plugin) {
        coverart_plugin->reset (1);
    }
}

void
cover_art_init (void) {
    terminate = 0;
    mutex = deadbeef->mutex_create_nonrecursive ();
    cond = deadbeef->cond_create ();
    tid = deadbeef->thread_start_low_priority (loading_thread, NULL);
}

void
cover_art_free (void) {
    trace ("terminating cover art loader...\n");

    if (coverart_plugin) {
        trace ("resetting artwork plugin...\n");
        coverart_plugin->reset (0);
    }
    
    if (tid) {
        terminate = 1;
        trace ("sending terminate signal to art loader thread...\n");
        deadbeef->cond_signal (cond);
        deadbeef->thread_join (tid);
        tid = 0;
    }
    while (queue) {
        queue_pop ();
    }
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].pixbuf) {
            g_object_unref (cache[i].pixbuf);
        }
    }
    memset (cache, 0, sizeof (cache));
    if (pixbuf_default) {
        g_object_unref (pixbuf_default);
        pixbuf_default = NULL;
    }
    deadbeef->cond_free (cond);
    cond = 0;
    deadbeef->mutex_free (mutex);
    mutex = 0;
}

GdkPixbuf *
cover_get_default_pixbuf (void) {
    if (!coverart_plugin) {
        return NULL;
    }
    if (!pixbuf_default) {
        GError *error = NULL;
        const char *defpath = coverart_plugin->get_default_cover ();
        pixbuf_default = gdk_pixbuf_new_from_file (defpath, &error);
        if (!pixbuf_default) {
            fprintf (stderr, "default cover: gdk_pixbuf_new_from_file %s failed, error: %s\n", defpath, error->message);
        }
        if (error) {
            g_error_free (error);
            error = NULL;
        }
        if (!pixbuf_default) {
            pixbuf_default = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 2, 2);
        }
        assert (pixbuf_default);
    }

    g_object_ref (pixbuf_default);
    return pixbuf_default;
}

int
gtkui_is_default_pixbuf (GdkPixbuf *pb) {
    return pb == pixbuf_default;
}

int
gtkui_cover_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_PLAYLIST_REFRESH:
        {
            int64_t reset_time = deadbeef->conf_get_int64 ("artwork.cache_reset_time", 0);;
            if (reset_time != artwork_reset_time) {
                artwork_reset_time = reset_time;
                deadbeef->mutex_lock (mutex);
                for (int i = 0; i < CACHE_SIZE; i++) {
                    if (cache[i].pixbuf) {
                        g_object_unref (cache[i].pixbuf);
                    }
                }
                memset (cache, 0, sizeof (cache));
                deadbeef->mutex_unlock (mutex);
            }
        }
        break;
    }
    return 0;
}
