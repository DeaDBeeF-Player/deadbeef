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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "messagepump.h"
#include "threading.h"
#include "playlist.h"
#include <deadbeef/common.h>

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
static uintptr_t cond;

static void
messagepump_reset (void);

int
messagepump_init (void) {
    messagepump_reset ();
    mutex = mutex_create ();
    cond = cond_create ();
    return 0;
}

void
messagepump_free () {
    mutex_lock (mutex);

    // this helps catching any ref leaks caused by messages sent at exit
    for (message_t *m = mqueue; m; m = m->next) {
        switch (m->id) {
        case DB_EV_SONGCHANGED:
        case DB_EV_SONGSTARTED:
        case DB_EV_SONGFINISHED:
        case DB_EV_TRACKINFOCHANGED:
        case DB_EV_CURSOR_MOVED:
        case DB_EV_SEEKED:
            assert (0);
        }
    }

    messagepump_reset ();
    mutex_unlock (mutex);
    mutex_free (mutex);
    cond_free (cond);
    mutex = 0;
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
    mutex_lock (mutex);
    if (!mfree) {
        mutex_unlock (mutex);
        //fprintf (stderr, "WARNING: message queue is full! message ignored (%d %p %d %d)\n", id, (void*)ctx, p1, p2);
        if (id >= DB_EV_FIRST && ctx) {
            messagepump_event_free ((ddb_event_t *)ctx);
        }
        return -1;
    }
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
    cond_signal (cond);
    return 0;
}

void
messagepump_wait (void) {
    cond_wait (cond, mutex);
    mutex_unlock (mutex);
}

int
messagepump_pop (uint32_t *id, uintptr_t *ctx, uint32_t *p1, uint32_t *p2) {
    mutex_lock (mutex);
    if (!mqueue) {
        mutex_unlock (mutex);
        return -1;
    }
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

ddb_event_t *
messagepump_event_alloc (uint32_t id) {
    int sz = 0;
    ddb_event_t *ev;
    switch (id) {
    case DB_EV_SONGCHANGED:
        sz = sizeof (ddb_event_trackchange_t);
        break;
    case DB_EV_SONGSTARTED:
    case DB_EV_SONGFINISHED:
    case DB_EV_TRACKINFOCHANGED:
    case DB_EV_CURSOR_MOVED:
        sz = sizeof (ddb_event_track_t);
        break;
    case DB_EV_SEEKED:
        sz = sizeof (ddb_event_playpos_t);
        break;
    default:
        trace ("Invalid event %d to use with messagepump_event_alloc, use sendmessage instead\n", id);
        return NULL;
    }
    ev = malloc (sz);
    memset (ev, 0, sz);
    ev->event = id;
    ev->size = sz;
    return ev;
}

void
messagepump_event_free (ddb_event_t *ev) {
    switch (ev->event) {
    case DB_EV_SONGCHANGED:
        {
            ddb_event_trackchange_t *tc = (ddb_event_trackchange_t*)ev;
            if (tc->from) {
                pl_item_unref ((playItem_t *)tc->from);
            }
            if (tc->to) {
                pl_item_unref ((playItem_t *)tc->to);
            }
        }
        break;
    case DB_EV_SONGSTARTED:
    case DB_EV_SONGFINISHED:
    case DB_EV_TRACKINFOCHANGED:
    case DB_EV_CURSOR_MOVED:
        {
            ddb_event_track_t *tc = (ddb_event_track_t*)ev;
            if (tc->track) {
                pl_item_unref ((playItem_t *)tc->track);
            }
        }
        break;
    case DB_EV_SEEKED:
        {
            ddb_event_playpos_t *tc = (ddb_event_playpos_t*)ev;
            if (tc->track) {
                pl_item_unref ((playItem_t *)tc->track);
            }
        }
        break;
    }
    free (ev);
}

int
messagepump_push_event (ddb_event_t *ev, uint32_t p1, uint32_t p2) {
    return messagepump_push (ev->event, (uintptr_t)ev, p1, p2);
}

