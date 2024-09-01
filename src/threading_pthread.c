/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  pthread wrapper

  Copyright (C) 2009-2013 Oleksiy Yakovenko

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

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "threading.h"
#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif

intptr_t
thread_start (void (*fn) (void *ctx), void *ctx) {
    pthread_t tid;
    pthread_attr_t attr;
    int s = pthread_attr_init (&attr);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_init failed: %s\n", strerror (s));
        return 0;
    }

    s = pthread_create (&tid, &attr, (void *(*)(void *))fn, (void *)ctx);
    if (s != 0) {
        fprintf (stderr, "pthread_create failed: %s\n", strerror (s));
        return 0;
    }
    s = pthread_attr_destroy (&attr);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_destroy failed: %s\n", strerror (s));
        return 0;
    }
    return (intptr_t)tid;
}

intptr_t
thread_start_low_priority (void (*fn) (void *ctx), void *ctx) {
#if defined(__linux__) && !defined(ANDROID)
    pthread_t tid;
    pthread_attr_t attr;
    int s = pthread_attr_init (&attr);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_init failed: %s\n", strerror (s));
        return 0;
    }
#    if !STATICLINK && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 4
    int policy;
    s = pthread_attr_getschedpolicy (&attr, &policy);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_getschedpolicy failed: %s\n", strerror (s));
        return 0;
    }
    int minprio = sched_get_priority_min (policy);
#    endif

    s = pthread_create (&tid, &attr, (void *(*)(void *))fn, (void *)ctx);
    if (s != 0) {
        fprintf (stderr, "pthread_create failed: %s\n", strerror (s));
        return 0;
    }
#    if !STATICLINK && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 4
    s = pthread_setschedprio (tid, minprio);
    if (s != 0) {
        fprintf (stderr, "pthread_setschedprio failed: %s\n", strerror (s));
        pthread_cancel (tid);
        return 0;
    }
#    endif

    s = pthread_attr_destroy (&attr);
    if (s != 0) {
        fprintf (stderr, "pthread_attr_destroy failed: %s\n", strerror (s));
        pthread_cancel (tid);
        return 0;
    }
    return (intptr_t)tid;
#else
    return thread_start (fn, ctx);
#endif
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

int
thread_detach (intptr_t tid) {
    int s = pthread_detach ((pthread_t)tid);
    if (s) {
        fprintf (stderr, "pthread_detach failed: %s\n", strerror (s));
        return -1;
    }
    return 0;
}

void
thread_exit (void *retval) {
    pthread_exit (retval);
}

uintptr_t
mutex_create_nonrecursive (void) {
    pthread_mutex_t *mtx = malloc (sizeof (pthread_mutex_t));
    pthread_mutexattr_t attr = { 0 };
    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_NORMAL);
    int err = pthread_mutex_init (mtx, &attr);
    if (err != 0) {
        fprintf (stderr, "pthread_mutex_init failed: %s\n", strerror (err));
        free (mtx);
        return 0;
    }
    pthread_mutexattr_destroy (&attr);
    return (uintptr_t)mtx;
}

uintptr_t
mutex_create (void) {
    pthread_mutex_t *mtx = malloc (sizeof (pthread_mutex_t));
    pthread_mutexattr_t attr = { 0 };
    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
    int err = pthread_mutex_init (mtx, &attr);
    if (err != 0) {
        fprintf (stderr, "pthread_mutex_init failed: %s\n", strerror (err));
        free (mtx);
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
        free (cond);
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
    int err = mutex_lock (m);
    if (err != 0) {
        fprintf (stderr, "pthread_cond_wait mutex_lock failed: %s\n", strerror (err));
        return err;
    }
    err = pthread_cond_wait (cond, mutex);
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
