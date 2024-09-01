/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  playback queue management

  Copyright (C) 2009-2015 Oleksiy Yakovenko

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
#include "playqueue.h"
#include "messagepump.h"

#define PLAYQUEUE_SIZE 100
static playItem_t *playqueue[100];
static int playqueue_count = 0;

#define trace(...) \
    { fprintf (stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static void
playqueue_send_trackinfochanged (playItem_t *track) {
    ddb_event_track_t *ev = (ddb_event_track_t *)messagepump_event_alloc (DB_EV_TRACKINFOCHANGED);
    ev->track = DB_PLAYITEM (track);
    if (track) {
        pl_item_ref (track);
    }
    messagepump_push_event ((ddb_event_t *)ev, DDB_PLAYLIST_CHANGE_PLAYQUEUE, 0);
}

int
playqueue_push (playItem_t *it) {
    if (playqueue_count == PLAYQUEUE_SIZE) {
        trace ("playqueue is full\n");
        return -1;
    }
    pl_lock ();
    pl_item_ref (it);
    playqueue[playqueue_count++] = it;
    pl_unlock ();
    playqueue_send_trackinfochanged (it);
    return 0;
}

void
playqueue_clear (void) {
    pl_lock ();
    for (int i = 0; i < playqueue_count; i++) {
        pl_item_unref (playqueue[i]);
        playqueue[i] = NULL;
    }
    playqueue_count = 0;
    pl_unlock ();
    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_PLAYQUEUE, 0);
}

void
playqueue_pop (void) {
    if (!playqueue_count) {
        return;
    }
    pl_lock ();
    if (playqueue_count == 1) {
        playqueue_count = 0;
        playqueue_send_trackinfochanged (playqueue[0]);
        pl_item_unref (playqueue[0]);
        pl_unlock ();
        return;
    }
    playItem_t *it = playqueue[0];
    memmove (&playqueue[0], &playqueue[1], (playqueue_count - 1) * sizeof (playItem_t *));
    playqueue_count--;
    pl_item_unref (it);
    pl_unlock ();
    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_PLAYQUEUE, 0);
}

void
playqueue_remove (playItem_t *it) {
    pl_lock ();
    for (;;) {
        int i;
        for (i = 0; i < playqueue_count; i++) {
            if (playqueue[i] == it) {
                if (i < playqueue_count - 1) {
                    memmove (&playqueue[i], &playqueue[i + 1], (playqueue_count - i) * sizeof (playItem_t *));
                    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_PLAYQUEUE, 0);
                }
                else {
                    playqueue_send_trackinfochanged (it);
                }
                pl_item_unref (it);
                playqueue_count--;
                break;
            }
        }
        if (i == playqueue_count) {
            break;
        }
    }
    pl_unlock ();
}

int
playqueue_test (playItem_t *it) {
    pl_lock ();
    for (int i = 0; i < playqueue_count; i++) {
        if (playqueue[i] == it) {
            pl_unlock ();
            return i;
        }
    }
    pl_unlock ();
    return -1;
}

playItem_t *
playqueue_getnext (void) {
    pl_lock ();
    if (playqueue_count > 0) {
        playItem_t *val = playqueue[0];
        pl_item_ref (val);
        pl_unlock ();
        return val;
    }
    pl_unlock ();
    return NULL;
}

int
playqueue_getcount (void) {
    return playqueue_count;
}

playItem_t *
playqueue_get_item (int i) {
    pl_lock ();
    playItem_t *it = playqueue[i];
    pl_item_ref (it);
    pl_unlock ();
    return it;
}

void
playqueue_remove_nth (int n) {
    pl_lock ();
    playItem_t *it = playqueue[n];
    if (n < playqueue_count - 1) {
        memmove (playqueue + n, playqueue + n + 1, (playqueue_count - n) * sizeof (playItem_t *));
    }
    playqueue_count--;

    pl_item_unref (it);
    pl_unlock ();
    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_PLAYQUEUE, 0);
}

void
playqueue_insert_at (int n, playItem_t *it) {
    if (playqueue_count == PLAYQUEUE_SIZE) {
        trace ("playqueue is full\n");
        return;
    }
    pl_lock ();
    if (n == playqueue_count) {
        playqueue_push (it);
        pl_unlock ();
        return;
    }
    memmove (playqueue + n + 1, playqueue + n, (playqueue_count - n) * sizeof (playItem_t *));
    playqueue[n] = it;
    pl_item_ref (it);
    playqueue_count++;
    pl_unlock ();
    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_PLAYQUEUE, 0);
}
