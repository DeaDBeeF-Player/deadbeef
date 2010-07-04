/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <stdio.h>
#include <string.h>
#include "messagepump.h"
#include "threading.h"

typedef struct message_s {
    uint32_t id;
    uintptr_t ctx;
    uint32_t p1;
    uint32_t p2;
    struct message_s *next;
} message_t;

enum { MAX_MESSAGES = 100 };
static message_t pool[MAX_MESSAGES];
static message_t *mfree;
static message_t *mqueue;
static message_t *mqtail;
static uintptr_t mutex;

static void
messagepump_reset (void);

int
messagepump_init (void) {
    messagepump_reset ();
    mutex = mutex_create ();
    return 0;
}

void
messagepump_free () {
    mutex_lock (mutex);
    messagepump_reset ();
    mutex_unlock (mutex);
    mutex_free (mutex);
}

static void
messagepump_reset (void) {
    mqueue = NULL;
    mfree = NULL;
    mqtail = NULL;
    memset (pool, 0, sizeof (pool));
    for (int i = 0; i < MAX_MESSAGES; i++) {
        pool[i].next = mfree;
        mfree = &pool[i];
    }
}

int
messagepump_push (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (!mfree) {
        printf ("WARNING: message queue is full! message ignored (%d %p %d %d)\n", id, (void*)ctx, p1, p2);
        return -1;
    }
    mutex_lock (mutex);
    message_t *msg = mfree;
    mfree = mfree->next;
    if (mqtail) {
        mqtail->next = msg;
    }
    mqtail = msg;
    if (!mqueue) {
        mqueue = msg;
    }

    msg->next = NULL;
    msg->id = id;
    msg->ctx = ctx;
    msg->p1 = p1;
    msg->p2 = p2;
    mutex_unlock (mutex);
    return 0;
}

int
messagepump_pop (uint32_t *id, uintptr_t *ctx, uint32_t *p1, uint32_t *p2) {
    if (!mqueue) {
        return -1;
    }
    mutex_lock (mutex);
    *id = mqueue->id;
    *ctx = mqueue->ctx;
    *p1 = mqueue->p1;
    *p2 = mqueue->p2;
    message_t *next = mqueue->next;
    mqueue->next = mfree;
    mfree = mqueue;
    mqueue = next;
    if (!mqueue) {
        mqtail = NULL;
    }
    mutex_unlock (mutex);
    return 0;
}

int
messagepump_hasmessages (void) {
    return mqueue ? 1 : 0;
}
