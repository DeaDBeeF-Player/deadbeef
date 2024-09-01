/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  message pump implementation

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
#ifndef __MESSAGEPUMP_H
#define __MESSAGEPUMP_H

#include <stdint.h>
#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

int
messagepump_init (void);
void
messagepump_free (void);
int
messagepump_push (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);
int
messagepump_pop (uint32_t *id, uintptr_t *ctx, uint32_t *p1, uint32_t *p2);
void
messagepump_wait (void);

ddb_event_t *
messagepump_event_alloc (uint32_t id);
void
messagepump_event_free (ddb_event_t *ev);
int
messagepump_push_event (ddb_event_t *ev, uint32_t p1, uint32_t p2);

#ifdef __cplusplus
}
#endif

#endif // __MESSAGEPUMP_H
