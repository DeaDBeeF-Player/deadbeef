/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __THREADING_H
#define __THREADING_H

#include <stdint.h>

intptr_t
thread_start (void (*fn)(void *ctx), void *ctx);

intptr_t
thread_start_low_priority (void (*fn)(void *ctx), void *ctx);

int
thread_join (intptr_t tid);

int
thread_detach (intptr_t tid);

void
thread_exit (void *retval);

uintptr_t
mutex_create (void);

uintptr_t
mutex_create_nonrecursive (void);

void
mutex_free (uintptr_t mtx);

int
mutex_lock (uintptr_t mtx);

int
mutex_unlock (uintptr_t mtx);

uintptr_t
cond_create (void);

void
cond_free (uintptr_t cond);

int
cond_wait (uintptr_t cond, uintptr_t mutex);

int
cond_signal (uintptr_t cond);

int
cond_broadcast (uintptr_t cond);

#endif

