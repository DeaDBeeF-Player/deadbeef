/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  basic ring buffer implementation

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

#include <string.h>
#include "ringbuf.h"

void
ringbuf_init (ringbuf_t *p, char *buffer, size_t size) {
    memset (p, 0, sizeof (ringbuf_t));
    p->bytes = buffer;
    p->cursor = 0;
    p->size = size;
    p->remaining = 0;
}

void
ringbuf_flush (ringbuf_t *p) {
    p->cursor = 0;
    p->remaining = 0;
}

int
ringbuf_write (ringbuf_t *p, char *bytes, size_t size) {
    if (p->size - p->remaining < size) {
        return -1;
    }

    size_t cursor = p->cursor + p->remaining;
    cursor %= p->size;

    if (p->size - cursor >= size) { // split
        memcpy (p->bytes + cursor, bytes, size);
        p->remaining += size;
    }
    else {
        size_t n = p->size - cursor;
        if (n > 0) {
            memcpy (p->bytes + cursor, bytes, n);
            p->remaining += n;
            size -= n;
            bytes += n;
        }
        memcpy (p->bytes, bytes, size);
        p->remaining += size;
    }
    return 0;
}

size_t
ringbuf_read_int (ringbuf_t * restrict p, char *bytes, size_t size, int keep) {
    if (p->remaining < size) {
        size = p->remaining;
    }

    size_t readsize1 = p->size - p->cursor;
    size_t readsize2 = 0;
    if (readsize1 < size) {
        readsize2 = size - readsize1;
    }
    else {
        readsize1 = size;
    }
    memcpy (bytes, p->bytes + p->cursor, readsize1);
    if (readsize2 != 0) {
        memcpy (bytes + readsize1, p->bytes, readsize2);
    }

    if (!keep) {
        p->remaining -= size;
        p->cursor += size;
        p->cursor %= p->size;
    }
    return size;
}

size_t
ringbuf_read (ringbuf_t *p, char *bytes, size_t size) {
    return ringbuf_read_int(p, bytes, size, 0);
}

size_t
ringbuf_read_keep (ringbuf_t *p, char *bytes, size_t size) {
    return ringbuf_read_int(p, bytes, size, 1);
}
