/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include "timeline.h"
#include "threading.h"

timeline_t *
timeline_create (void) {
    timeline_t *tl = malloc (sizeof (timeline_t));
    memset (tl, 0, sizeof (timeline_t));
    return tl;
}

void
timeline_free (timeline_t *tl, int wait) {
    if (tl->tid && wait) {
        int tid = tl->tid;
        tl->destroy = 1;
        thread_join (tid);
    }
    else {
        tl->destroy = 1;
    }
}

void
timeline_init (timeline_t *tl, float seconds, float fps, int (*callback)(float _progress, int _last, void *_ctx), void *ctx) {
    tl->fps = fps;
    tl->duration = seconds;
    tl->progress = 0;
    tl->callback = callback;
    tl->callback_ctx = ctx;
    tl->destroy = 0;
    tl->stop = 0;
}

void
timeline_stop (timeline_t *tl, int wait) {
    int tid = tl->tid;
    if (tid) {
        tl->stop = 1;
        if (wait) {
            thread_join (tid);
        }
    }
}

void
timeline_thread_func (void *ctx) {
    printf ("timeline thread started\n");
    timeline_t *tl = (timeline_t *)ctx;

    for (;;) {
        if (tl->stop || tl->destroy) {
            tl->callback (1, 1, tl->callback_ctx);
            break;
        }
        struct timeval tm;
        gettimeofday (&tm, NULL);
        float dt = (tm.tv_sec - tl->time.tv_sec) + (tm.tv_usec - tl->time.tv_usec) / 1000000.0;
        float t = tl->progress;
        tl->progress += dt;
        memcpy (&tl->time, &tm, sizeof (tm));
        if (t > tl->duration) {
            tl->callback (1, 1, tl->callback_ctx);
            break;
        }
        else {
            if (tl->callback (t/tl->duration, 0, tl->callback_ctx) < 0) {
                break;
            }
        }
        printf ("progress: %f\n", tl->progress);

        // sleep until next frame
        usleep (1000000 / tl->fps);
    }
    tl->tid = 0;
    if (tl->destroy) {
        printf ("timeline %p destroyed\n", tl);
        free (tl);
    }
    printf ("timeline %p thread terminated\n", tl);
}


void
timeline_start (timeline_t *tl) {
    gettimeofday (&tl->time, NULL);
    tl->progress = 0;
    tl->stop = 0;
    tl->destroy = 0;
    if (!tl->tid) {
        tl->tid = thread_start (timeline_thread_func, tl);
    }
    else {
        printf ("reusing existing thread\n");
    }
}

