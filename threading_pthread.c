/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "threading.h"

intptr_t
thread_start (void (*fn)(void *ctx), void *ctx) {
    pthread_t tid;
    pthread_attr_t attr;
    int s = pthread_attr_init (&attr);
    if (s) {
        fprintf (stderr, "pthread_attr_init failed\n");
        return -1;
    }

    if (pthread_create (&tid, &attr, (void *(*)(void *))fn, (void*)ctx)) {
        fprintf (stderr, "pthread_create failed\n");
        return -1;
    }
    s = pthread_attr_destroy (&attr);
    if (s) {
        fprintf (stderr, "pthread_attr_destroy failed\n");
        pthread_cancel (tid);
        return -1;
    }
    return tid;
}

int
thread_join (intptr_t tid) {
    void *retval;
    int s = pthread_join ((pthread_t)tid, &retval);
    if (s) {
        fprintf (stderr, "pthread_join failed\n");
        return -1;
    }
    return 0;
}

uintptr_t
mutex_create (void) {
    pthread_mutex_t *mtx = malloc (sizeof (pthread_mutex_t));
    if (pthread_mutex_init (mtx, NULL)) {
        fprintf (stderr, "pthread_mutex_init failed!\n");
    }
    return (uintptr_t)mtx;
}

void
mutex_free (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    mutex_lock (_mtx);
    mutex_unlock (_mtx);
    pthread_mutex_destroy (mtx);
    free (mtx);
}

int
mutex_lock (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    int err = pthread_mutex_lock (mtx);
    if (err < 0) {
        fprintf (stderr, "pthread_mutex_lock failed (error %d)\n", err);
    }
    return err;
}

int
mutex_unlock (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    int err = pthread_mutex_unlock (mtx);
    if (err < 0) {
        printf ("pthread_mutex_unlock failed (error %d)\n", err);
    }
    return err;
}

uintptr_t
cond_create (void) {
    pthread_cond_t *cond = malloc (sizeof (pthread_cond_t));
    pthread_cond_init (cond, NULL);
    return (uintptr_t)cond;
}

void
cond_free (uintptr_t c) {
    if (c) {
        pthread_cond_t *cond = (pthread_cond_t *)c;
        pthread_cond_destroy (cond);
        free (cond);
    }
}

int
cond_wait (uintptr_t c, uintptr_t m) {
    pthread_cond_t *cond = (pthread_cond_t *)c;
    pthread_mutex_t *mutex = (pthread_mutex_t *)m;
    return pthread_cond_wait (cond, mutex);
}

int
cond_signal (uintptr_t c) {
    pthread_cond_t *cond = (pthread_cond_t *)c;
    return pthread_cond_signal (cond);
}

int
cond_broadcast (uintptr_t c) {
    pthread_cond_t *cond = (pthread_cond_t *)c;
    return pthread_cond_broadcast (cond);
}
