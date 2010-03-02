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
    while (!terminate) {
        if (!queue) {
            usleep (300000);
            continue;
        }
        int cache_min = 0;
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
        trace ("loading image %s\n", queue->fname);

        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (queue->fname, NULL);
        if (!pixbuf) {
            trace ("GDK failed to load pixbuf from file %s\n", queue->fname);
        }

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
            GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
            g_object_unref (pixbuf);
            pixbuf = scaled;
        }
        if (cache[cache_min].pixbuf) {
            g_object_unref (cache[cache_min].pixbuf);
        }
        if (cache[cache_min].fname) {
            free (cache[cache_min].fname);
        }
        cache[cache_min].pixbuf = pixbuf;
        cache[cache_min].fname = strdup (queue->fname);
        gettimeofday (&cache[cache_min].tm, NULL);
        cache[cache_min].width = queue->width;
        queue_pop ();
    }
    tid = 0;
}

void
cover_avail_callback (const char *artist, const char *album) {
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
get_cover_art (DB_playItem_t *it, int width) {
    if (!coverart_plugin) {
        return NULL;
    }
    char *fname = coverart_plugin->get_album_art (it, cover_avail_callback);
    if (fname) {
        GdkPixbuf *pb = get_pixbuf (fname, width);
        free (fname);
        return pb;
    }
    return NULL;
}

void
cover_art_init (void) {
    terminate = 0;
    mutex = deadbeef->mutex_create ();
    tid = deadbeef->thread_start (loading_thread, NULL);
}

void
cover_art_free (void) {
    trace ("terminating cover art loader thread\n");
    terminate = 1;
    if (tid) {
        deadbeef->thread_join (tid);
    }
    while (queue) {
        queue_pop ();
    }
    deadbeef->mutex_free (mutex);
}
