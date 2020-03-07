/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  basic ring buffer implementation

  Copyright (C) 2009-2013 Alexey Yakovenko

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

  Alexey Yakovenko waker@users.sourceforge.net
*/
#ifndef __RINGBUF_H
#define __RINGBUF_H

#include <sys/types.h>

typedef struct {
    char *bytes;
    size_t size;
    size_t cursor;
    size_t remaining;
} ringbuf_t;

void
ringbuf_init (ringbuf_t *p, char *buffer, size_t size);

int
ringbuf_write (ringbuf_t *p, char *bytes, size_t size);

size_t
ringbuf_read (ringbuf_t *p, char *bytes, size_t size);

#endif
