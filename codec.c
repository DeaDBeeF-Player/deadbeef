/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include "codec.h"
#include "threading.h"

static uintptr_t mutex;

void
codec_init_locking (void) {
    mutex = mutex_create ();
}

void
codec_free_locking (void) {
    mutex_free (mutex);
}

void
codec_lock (void) {
    mutex_lock (mutex);
}

void
codec_unlock (void) {
    mutex_unlock (mutex);
}

