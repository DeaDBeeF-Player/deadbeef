/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "coverart.h"
#include "../artwork/artwork.h"
#include "gtkui.h"

#pragma GCC optimize("O0")

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(...)

extern DB_artwork_plugin_t *coverart_plugin;

#define MAX_ID 256
#define CACHE_SIZE 20

typedef struct {
    struct timeval tm;
    char *fname;
    int width;
    GdkPixbuf *pixbuf;
} cached_pixbuf_t;

typedef struct load_query_s {
    char *fname;
    int width;
    struct load_query_s *next;
} load_query_t;

static cached_pixbuf_t cache[CACHE_SIZE];
static int terminate = 0;
static uintptr_t mutex;
static uintptr_t cond;
static uintptr_t tid;
load_query_t *queue;
load_query_t *tail;

static void
queue_add (const char *fname, int width) {
    deadbeef->mutex_lock (mutex);
    load_query_t *q;
    for (q = queue; q; q = q->next) {
        if (!strcmp (q->fname, fname) && width == q->width) {
            deadbeef->mutex_unlock (mutex);
            return; // dupe
        }
    }
    q = malloc (sizeof (load_query_t));
    memset (q, 0, sizeof (load_query_t));
    q->fname = strdup (fname);
    q->width = width;
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

gboolean
redraw_playlist_cb (gpointer dt) {
    void main_refresh (void);
    main_refresh ();
    return FALSE;
}

void
loading_thread (void *none) {
    for (;;) {
        trace ("covercache: waiting for signal\n");
        deadbeef->cond_wait (cond, mutex);
        trace ("covercache: signal received\n");
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
            if (cache_min == -1) {
                trace ("coverart pixbuf cache overflow, waiting...\n");
                usleep (500000);
                continue;
            }
            trace ("loading image %s\n", queue->fname);

            GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (queue->fname, NULL);
            if (!pixbuf) {
                trace ("GDK failed to load pixbuf from file %s\n", queue->fname);
                // make default empty image
                pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 2, 2);
            }
            else {
                int w, h;
                w = gdk_pixbuf_get_width (pixbuf);
                h = gdk_pixbuf_get_height (pixbuf);
                int width = queue->width;
                if (w != width) {
                    int height;
                    if (w > h) {
                        height = width * h / w;
                    }
                    else if (h > w) {
                        height = width;
                        width = height * w / h;
                    }
                    else {
                        height = width;
                    }
                    if (width < 5 || height < 5) {
                        trace ("will not scale to %dx%d\n", width, height);
                        queue_pop ();
                        continue;
                    }
                    trace ("scaling %dx%d -> %dx%d\n", w, h, width, height);
                    GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
                    g_object_unref (pixbuf);
                    pixbuf = scaled;
                }
            }
            if (cache_min != -1) {
                deadbeef->mutex_lock (mutex);
                cache[cache_min].pixbuf = pixbuf;
                cache[cache_min].fname = strdup (queue->fname);
                gettimeofday (&cache[cache_min].tm, NULL);
                cache[cache_min].width = queue->width;
                deadbeef->mutex_unlock (mutex);
            }
            queue_pop ();
            g_idle_add (redraw_playlist_cb, NULL);
        }
        if (terminate) {
            break;
        }
    }
}

void
cover_avail_callback (const char *fname, const char *artist, const char *album, void *user_data) {
    // means requested image is now in disk cache
    // load it into main memory
    GdkPixbuf *pb = get_cover_art (fname, artist, album, (intptr_t)user_data);
    if (pb) {
        g_object_unref (pb);
        // already in cache, redraw
        g_idle_add (redraw_playlist_cb, NULL);
    }
}

static GdkPixbuf *
get_pixbuf (const char *fname, int width) {
    int requested_width = width;
    // find in cache
    deadbeef->mutex_lock (mutex);
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].pixbuf) {
            if (!strcmp (fname, cache[i].fname) && cache[i].width == width) {
                gettimeofday (&cache[i].tm, NULL);
                g_object_ref (cache[i].pixbuf);
                deadbeef->mutex_unlock (mutex);
                return cache[i].pixbuf;
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
    queue_add (fname, width);
    return NULL;
}

GdkPixbuf *
get_cover_art (const char *fname, const char *artist, const char *album, int width) {
    if (!coverart_plugin) {
        return NULL;
    }
    char *image_fname = coverart_plugin->get_album_art (fname, artist, album, cover_avail_callback, (void *)(intptr_t)width);
    if (fname) {
        GdkPixbuf *pb = get_pixbuf (image_fname, width);
        free (image_fname);
        return pb;
    }
    return NULL;
}

void
reset_cover_art_cache (void) {
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
}

void
cover_art_init (void) {
    terminate = 0;
    mutex = deadbeef->mutex_create_nonrecursive ();
    cond = deadbeef->cond_create ();
    tid = deadbeef->thread_start (loading_thread, NULL);
}

void
cover_art_free (void) {
    trace ("terminating cover art loader...\n");

    if (coverart_plugin) {
        trace ("resetting artwork plugin...\n");
        coverart_plugin->reset ();
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
    deadbeef->mutex_free (cond);
    deadbeef->mutex_free (mutex);
}

