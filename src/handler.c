/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  generic message queue implementation

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
#include "handler.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "handler.h"
#include "threading.h"

typedef struct message_s {
    uint32_t id;
    uintptr_t ctx;
    uint32_t p1;
    uint32_t p2;
    struct message_s *next;
} message_t;

typedef struct handler_s {
    int queue_size;
    message_t *mfree;
    message_t *mqueue;
    message_t *mqtail;
    uintptr_t mutex;
    uintptr_t cond;
    message_t pool[1];
} handler_t;

void
handler_reset (handler_t *h) {
    h->mqueue = NULL;
    h->mfree = NULL;
    h->mqtail = NULL;
    memset (h->pool, 0, sizeof (message_t) * h->queue_size);
    for (int i = 0; i < h->queue_size; i++) {
        h->pool[i].next = h->mfree;
        h->mfree = &h->pool[i];
    }
}

handler_t *
handler_alloc (int queue_size) {
    int sz = sizeof (handler_t) + (queue_size-1) * sizeof (message_t);
    handler_t *h = malloc (sz);
    memset (h, 0, sz);
    h->queue_size = queue_size;
    h->mutex = mutex_create ();
    h->cond = cond_create ();
    handler_reset (h);
    return h;
}

void
handler_free (handler_t *h) {
    mutex_lock (h->mutex);
    handler_reset (h);
    mutex_unlock (h->mutex);
    mutex_free (h->mutex);
    cond_free (h->cond);
    free (h);
}

int
handler_push (handler_t *h, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (!h) {
        return -1;
    }
    mutex_lock (h->mutex);
    if (!h->mfree) {
        mutex_unlock (h->mutex);
        return -1;
    }
    message_t *msg = h->mfree;
    h->mfree = h->mfree->next;
    if (h->mqtail) {
        h->mqtail->next = msg;
    }
    h->mqtail = msg;
    if (!h->mqueue) {
        h->mqueue = msg;
    }

    msg->next = NULL;
    msg->id = id;
    msg->ctx = ctx;
    msg->p1 = p1;
    msg->p2 = p2;
    mutex_unlock (h->mutex);
    cond_signal (h->cond);
    return 0;
}

void
handler_wait (handler_t *h) {
    cond_wait (h->cond, h->mutex);
    mutex_unlock (h->mutex);
}

int
handler_pop (handler_t *h, uint32_t *id, uintptr_t *ctx, uint32_t *p1, uint32_t *p2) {
    mutex_lock (h->mutex);
    if (!h->mqueue) {
        mutex_unlock (h->mutex);
        return -1;
    }
    *id = h->mqueue->id;
    *ctx = h->mqueue->ctx;
    *p1 = h->mqueue->p1;
    *p2 = h->mqueue->p2;
    message_t *next = h->mqueue->next;
    h->mqueue->next = h->mfree;
    h->mfree = h->mqueue;
    h->mqueue = next;
    if (!h->mqueue) {
        h->mqtail = NULL;
    }
    mutex_unlock (h->mutex);
    return 0;
}

int
handler_hasmessages (handler_t *h) {
    mutex_lock (h->mutex);
    int res = h->mqueue ? 1 : 0;
    mutex_unlock (h->mutex);
    return res;
}

