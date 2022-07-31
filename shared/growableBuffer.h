//
//  growableBuffer.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 3/31/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#ifndef growableBuffer_h
#define growableBuffer_h

#include <stdlib.h>

typedef struct {
    size_t size;
    size_t avail;
    size_t offs;
    char *buffer;
} growableBuffer_t;

growableBuffer_t *
growableBufferAlloc (void);

void
growableBufferDealloc (growableBuffer_t *buffer);

void
growableBufferFree (growableBuffer_t *buffer);

growableBuffer_t *
growableBufferInitWithSize (growableBuffer_t *buffer, size_t size);

void
growableBufferGrowBy (growableBuffer_t *buffer, size_t sizeIncrement);

void
growableBufferAdvanceBy (growableBuffer_t *buffer, size_t size);

void
growableBufferPrintf (growableBuffer_t *buffer, const char *fmt, ...);

#endif /* growableBuffer_h */
