/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

int
ringbuf_read (ringbuf_t *p, char *bytes, size_t size) {
    if (p->remaining < size) {
        size = p->remaining;
    }
    int rb = size;

    if (p->size - p->cursor >= size) {
        memcpy (bytes, p->bytes + p->cursor, size);
        p->cursor += size;
        p->remaining -= size;
    }
    else {
        size_t n = p->size - p->cursor;
        if (n > 0) {
            memcpy (bytes, p->bytes + p->cursor, n);
            p->cursor += n;
            p->remaining -= n;
            bytes += n;
            size -= n;
        }
        memcpy (bytes, p->bytes, size);
        p->cursor = size;
        p->remaining -= size;
    }
    return rb;
}
