#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "threading.h"

void thread_start (void (*fn)(uintptr_t ctx), uintptr_t ctx) {
    printf ("thread_start called!\n");
    pthread_t tid;
    pthread_attr_t attr;
    printf ("pthread_attr_init!\n");
    int s = pthread_attr_init (&attr);
    if (s) {
        printf ("pthread_attr_init failed\n");
        return;
    }

    printf ("pthread_create!\n");
    if (pthread_create (&tid, &attr, (void *(*)(void *))fn, (void*)ctx)) {
        printf ("pthread_create failed\n");
        return;
    }
    printf ("pthread_attr_destroy!\n");
    s = pthread_attr_destroy (&attr);
    if (s) {
        printf ("pthread_attr_destroy failed\n");
        return;
    }
#if 0
    void *res;
    printf ("pthread_join!\n");
    s = pthread_join (tid, &res);
    if (s) {
        printf ("pthread_join failed\n");
        return;
    }
    free (res);
#endif
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
    pthread_mutex_lock (mtx);
}
int mutex_unlock (uintptr_t _mtx) {
    pthread_mutex_t *mtx = (pthread_mutex_t *)_mtx;
    pthread_mutex_unlock (mtx);
}

