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
#include <errno.h>
#include <string.h>
#include "threading.h"

intptr_t
thread_start (void (*fn)(void *ctx), void *ctx) {
    pthread_t tid;
    pthread_attr_t attr;
    int s = pthread_attr_init (&attr);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_init failed: %s\n", strerror (s));
        return 0;
    }

    s = pthread_create (&tid, &attr, (void *(*)(void *))fn, (void*)ctx);
    if (s != 0) {
        fprintf (stderr, "pthread_create failed: %s\n", strerror (s));
        return 0;
    }
    s = pthread_attr_destroy (&attr);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_destroy failed: %s\n", strerror (s));
        pthread_cancel (tid);
        return 0;
    }
    return tid;
}

intptr_t
thread_start_low_priority (void (*fn)(void *ctx), void *ctx) {
    pthread_t tid;
    pthread_attr_t attr;
    int s = pthread_attr_init (&attr);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_init failed: %s\n", strerror (s));
        return 0;
    }
    int policy;
    s = pthread_attr_getschedpolicy (&attr, &policy);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_getschedpolicy failed: %s\n", strerror (s));
        return 0;
    }
    int minprio = sched_get_priority_min (policy);

    s = pthread_create (&tid, &attr, (void *(*)(void *))fn, (void*)ctx);
    if (s != 0) {
        fprintf (stderr, "pthread_create failed: %s\n", strerror (s));
        return 0;
    }
    s = pthread_setschedprio (tid, minprio);
    if (s != 0) {
        fprintf (stderr, "pthread_setschedprio failed: %s\n", strerror (s));
        pthread_cancel (tid);
        return 0;
    }

    s = pthread_attr_destroy (&attr);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_destroy failed: %s\n", strerror (s));
        pthread_cancel (tid);
        return 0;
    }
    return tid;
}

int
thread_join (intptr_t tid) {
    void *retval;
    int s = pthread_join ((pthread_t)tid, &retval);
    if (s) {
        fprintf (stderr, "pthread_join failed: %s\n", strerror (s));
        return -1;
    }
    return 0;
}

uintptr_t
mutex_create_nonrecursive (void) {
    pthread_mutex_t *mtx = malloc (sizeof (pthread_mutex_t));
    pthread_mutexattr_t attr = {0};
    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_NORMAL);
    int err = pthread_mutex_init (mtx, &attr);
    if (err != 0) {
        fprintf (stderr, "pthread_mutex_init failed: %s\n", strerror (err));
        return 0;
    }
    pthread_mutexattr_destroy (&attr);
    return (uintptr_t)mtx;
}

uintptr_t
mutex_create (void) {
    pthread_mutex_t *mtx = malloc (sizeof (pthread_mutex_t));
    pthread_mutexattr_t attr = {0};
    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
    int err = pthread_mutex_init (mtx, &attr);
    if (err != 0) {
        fprintf (stderr, "pthread_mutex_init failed: %s\n", strerror (err));
        return 0;
    }
    pthread_mutexattr_destroy (&attr);
    return (uintptr_t)mtx;
}

void
mutex_free (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    pthread_mutex_destroy (mtx);
    free (mtx);
}

int
mutex_lock (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    int err = pthread_mutex_lock (mtx);
    if (err != 0) {
        fprintf (stderr, "pthread_mutex_lock failed: %s\n", strerror (err));
    }
    return err;
}

int
mutex_unlock (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    int err = pthread_mutex_unlock (mtx);
    if (err != 0) {
        fprintf (stderr, "pthread_mutex_unlock failed: %s\n", strerror (err));
    }
    return err;
}

uintptr_t
cond_create (void) {
    pthread_cond_t *cond = malloc (sizeof (pthread_cond_t));
    int err = pthread_cond_init (cond, NULL);
    if (err != 0) {
        fprintf (stderr, "pthread_cond_init failed: %s\n", strerror (err));
        return 0;
    }
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
    int err = pthread_cond_wait (cond, mutex);
    if (err != 0) {
        fprintf (stderr, "pthread_cond_wait failed: %s\n", strerror (err));
    }
    return err;
}

int
cond_signal (uintptr_t c) {
    pthread_cond_t *cond = (pthread_cond_t *)c;
    int err = pthread_cond_signal (cond);
    if (err != 0) {
        fprintf (stderr, "pthread_cond_signal failed: %s\n", strerror (err));
    }
    return err;
}

int
cond_broadcast (uintptr_t c) {
    pthread_cond_t *cond = (pthread_cond_t *)c;
    int err = pthread_cond_broadcast (cond);
    if (err != 0) {
        fprintf (stderr, "pthread_cond_broadcast failed: %s\n", strerror (err));
    }
    return err;
}
