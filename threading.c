/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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

