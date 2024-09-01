/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  threading functions wrapper

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
#ifndef __THREADING_H
#define __THREADING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

intptr_t
thread_start (void (*fn) (void *ctx), void *ctx);

intptr_t
thread_start_low_priority (void (*fn) (void *ctx), void *ctx);

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

#ifdef __cplusplus
}
#endif

#endif
