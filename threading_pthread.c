#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "threading.h"

void thread_start (void (*fn)(uintptr_t ctx), uintptr_t ctx) {
    pthread_t tid;
    pthread_attr_t attr;
    int s = pthread_attr_init (&attr);
    if (s) {
        printf ("pthread_attr_init failed\n");
        return;
    }

    if (pthread_create (&tid, &attr, (void *(*)(void *))fn, (void*)ctx)) {
        printf ("pthread_create failed\n");
        return;
    }
    s = pthread_attr_destroy (&attr);
    if (s) {
        printf ("pthread_attr_destroy failed\n");
        return;
    }
}
uintptr_t mutex_create (void) {
    pthread_mutex_t *mtx = malloc (sizeof (pthread_mutex_t));
    if (pthread_mutex_init (mtx, NULL)) {
        printf ("pthread_mutex_init failed!\n");
    }
    return (uintptr_t)mtx;
}
void mutex_free (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    mutex_lock (_mtx);
    mutex_unlock (_mtx);
    pthread_mutex_destroy (mtx);
    free (mtx);
}
int mutex_lock (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    if (pthread_mutex_lock (mtx)) {
        printf ("pthread_mutex_lock failed\n");
    }
}
int mutex_unlock (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    if (pthread_mutex_unlock (mtx)) {
        printf ("pthread_mutex_unlock failed\n");
    };
}

