//
//  growableBuffer.c
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 3/31/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "growableBuffer.h"

growableBuffer_t *
growableBufferAlloc (void) {
    return calloc (1, sizeof (growableBuffer_t));
}

void
growableBufferDealloc (growableBuffer_t *buffer) {
    free (buffer->buffer);
}

void
growableBufferFree (growableBuffer_t *buffer) {
    if (buffer) {
        growableBufferDealloc(buffer);
        free (buffer);
    }
}

growableBuffer_t *
growableBufferInitWithSize (growableBuffer_t *buffer, size_t size) {
    memset (buffer, 0, sizeof (growableBuffer_t));
    buffer->buffer = calloc (1, size);
    buffer->size = size;
    buffer->avail = size;
    return buffer;
}

void
growableBufferGrowBy (growableBuffer_t *buffer, size_t sizeIncrement) {
    buffer->size += sizeIncrement;
    buffer->buffer = realloc (buffer->buffer, buffer->size);
    buffer->avail += sizeIncrement;
}

void
growableBufferAdvanceBy (growableBuffer_t *buffer, size_t size) {
    buffer->offs += size;
    buffer->avail -= size;
}

void
growableBufferPrintf (growableBuffer_t *buffer, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    size_t res;
    while ((res = vsnprintf (buffer->buffer + buffer->offs, buffer->avail, fmt, ap)) >= buffer->avail) {
        growableBufferGrowBy(buffer, 1000);
        va_end(ap);
        va_start(ap, fmt);
    }
    va_end(ap);
    growableBufferAdvanceBy(buffer, res);
}
