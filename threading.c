#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "threading.h"

void thread_start (void (*fn)(uintptr_t ctx), uintptr_t ctx) {
    if (!SDL_CreateThread ((int (*)(void*))fn, (void*)ctx)) {
        printf ("SDL_CreateThread failed!\n");
    }
}
uintptr_t mutex_create (void) {
    SDL_mutex *mtx = SDL_CreateMutex ();
    if (!mtx) {
        printf ("SDL_CreateMutex failed!\n");
    }
    return (uintptr_t)mtx;
}
void mutex_free (uintptr_t mtx) {
    SDL_mutexP ((SDL_mutex*)mtx); // grant that no thread does processing now
    SDL_DestroyMutex ((SDL_mutex*)mtx);
}
int mutex_lock (uintptr_t mtx) {
    return SDL_mutexP ((SDL_mutex*)mtx);
}
int mutex_unlock (uintptr_t mtx) {
    return SDL_mutexV ((SDL_mutex*)mtx);
}

