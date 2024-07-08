/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  playlist management

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
#ifdef HAVE_CONFIG_H
#    include "config.h"
#endif
#ifdef HAVE_ALLOCA_H
#    include <alloca.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#ifndef __linux__
#    define _POSIX_C_SOURCE 1
#endif
#include <limits.h>
#include <errno.h>
#include <math.h>
#include "buffered_file_writer.h"
#include "filereader/filereader.h"
#include "gettext.h"
#include "playlist.h"
#include "plmeta.h"
#include "streamer.h"
#include "messagepump.h"
#include "plugins.h"
#include "junklib.h"
#include "vfs.h"
#include "conf.h"
#include "utf8.h"
#include <deadbeef/common.h>
#include "threading.h"
#include "metacache.h"
#include "volume.h"
#include "pltmeta.h"
#include "escape.h"
#include <deadbeef/strdupa.h>
#include "tf.h"
#include "playqueue.h"
#include "sort.h"
#include "cueutil.h"
#include "playmodes.h"
#include "undo/undomanager.h"
#include "undo/undo_playlist.h"

// disable custom title function, until we have new title formatting (0.7)
#define DISABLE_CUSTOM_TITLE

#define DISABLE_LOCKING 0
#define DEBUG_LOCKING 0
#define DETECT_PL_LOCK_RC 0

#if DETECT_PL_LOCK_RC
#    include <pthread.h>
#endif

// file format revision history
// 1.0->1.1 changelog:
//    added sample-accurate seek positions for sub-tracks
// 1.1->1.2 changelog:
//    added flags field
// 1.2->1.3 changelog:
//    removed legacy data used for compat with 0.4.4
//    note: ddb-0.5.0 should keep using 1.2 playlist format
//    1.3 support is designed for transition to ddb-0.6.0
#define PLAYLIST_MAJOR_VER 1
#define PLAYLIST_MINOR_VER 2

#if (PLAYLIST_MINOR_VER < 2)
#    error writing playlists in format <1.2 is not supported
#endif

static int _playlists_count = 0;
static playlist_t *_playlists_head = NULL;
static playlist_t *_current_playlist = NULL; // current playlist
static int _plt_loading = 0; // disable sending event about playlist switch, config regen, etc

#if !DISABLE_LOCKING
static uintptr_t _playlist_mutex;
#endif

#define LOCK \
    { pl_lock (); }
#define UNLOCK \
    { pl_unlock (); }

// used at startup to prevent crashes
static playlist_t _dummy_playlist = { .refc = 1 };

static int no_remove_notify;

static playlist_t *addfiles_playlist; // current playlist for adding files/folders; set in pl_add_files_begin

int conf_cue_prefer_embedded = 0;

typedef struct ddb_fileadd_listener_s {
    int id;
    int (*callback) (ddb_fileadd_data_t *data, void *user_data);
    void *user_data;
    struct ddb_fileadd_listener_s *next;
} ddb_fileadd_listener_t;

typedef struct ddb_fileadd_filter_s {
    int id;
    int (*callback) (ddb_file_found_data_t *data, void *user_data);
    void *user_data;
    struct ddb_fileadd_filter_s *next;
} ddb_fileadd_filter_t;

static ddb_fileadd_listener_t *file_add_listeners;
static ddb_fileadd_filter_t *file_add_filters;

typedef struct ddb_fileadd_beginend_listener_s {
    int id;
    void (*callback_begin) (ddb_fileadd_data_t *data, void *user_data);
    void (*callback_end) (ddb_fileadd_data_t *data, void *user_data);
    void *user_data;
    struct ddb_fileadd_beginend_listener_s *next;
} ddb_fileadd_beginend_listener_t;

static ddb_fileadd_beginend_listener_t *file_add_beginend_listeners;

static void
_plt_set_curr_int (playlist_t *plt);

void
pl_reshuffle_all (void) {
    for (playlist_t *plt = _playlists_head; plt; plt = plt->next) {
        plt_reshuffle (plt, NULL, NULL);
    }
}

int
pl_init (void) {
    if (_current_playlist) {
        return 0; // avoid double init
    }
    _current_playlist = &_dummy_playlist;
#if !DISABLE_LOCKING
    _playlist_mutex = mutex_create ();
#endif
    return 0;
}

void
pl_free (void) {
    LOCK;
    playqueue_clear ();
    _plt_loading = 1;
    while (_playlists_head) {

        for (playItem_t *it = _playlists_head->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
            if (it->_refc > 1) {
                fprintf (
                    stderr,
                    "\033[0;31mWARNING: playitem %p %s(%s) has refc=%d at delete time\033[37;0m\n",
                    it,
                    pl_find_meta_raw (it, ":URI"),
                    pl_find_meta_raw (it, "track"),
                    it->_refc);
            }
        }

        //fprintf (stderr, "\033[0;31mplt refc %d\033[37;0m\n", playlists_head->refc);

        if (_playlists_head->refc > 1) {
            fprintf (
                stderr,
                "\033[0;31mWARNING: PLAYLIST %p %s has refc=%d at delete time\033[37;0m\n",
                _playlists_head,
                _playlists_head->title,
                _playlists_head->refc);
        }

        plt_remove (0);
    }
    _plt_loading = 0;
    UNLOCK;
#if !DISABLE_LOCKING
    if (_playlist_mutex) {
        mutex_free (_playlist_mutex);
        _playlist_mutex = 0;
    }
#endif
    _current_playlist = NULL;
}

#if DEBUG_LOCKING
volatile int pl_lock_cnt = 0;
#endif
#if DETECT_PL_LOCK_RC
static pthread_t tids[1000];
static int ntids = 0;
pthread_t pl_lock_tid = 0;
#endif
void
pl_lock (void) {
#if !DISABLE_LOCKING
    mutex_lock (_playlist_mutex);
#    if DETECT_PL_LOCK_RC
    pl_lock_tid = pthread_self ();
    tids[ntids++] = pl_lock_tid;
#    endif

#    if DEBUG_LOCKING
    pl_lock_cnt++;
    printf ("pcnt: %d\n", pl_lock_cnt);
#    endif
#endif
}

void
pl_unlock (void) {
#if !DISABLE_LOCKING
#    if DETECT_PL_LOCK_RC
    if (ntids > 0) {
        ntids--;
    }
    if (ntids > 0) {
        pl_lock_tid = tids[ntids - 1];
    }
    else {
        pl_lock_tid = 0;
    }
#    endif
    mutex_unlock (_playlist_mutex);
#    if DEBUG_LOCKING
    pl_lock_cnt--;
    printf ("pcnt: %d\n", pl_lock_cnt);
#    endif
#endif
}

static void
pl_item_free (playItem_t *it);

static void
plt_gen_conf (void) {
    if (_plt_loading) {
        return;
    }
    LOCK;
    int cnt = plt_get_count ();
    int i;
    conf_remove_items ("playlist.tab.");

    playlist_t *p = _playlists_head;
    for (i = 0; i < cnt; i++, p = p->next) {
        char s[100];
        snprintf (s, sizeof (s), "playlist.tab.%05d", i);
        conf_set_str (s, p->title);
        snprintf (s, sizeof (s), "playlist.cursor.%d", i);
        conf_set_int (s, p->current_row[PL_MAIN]);
        snprintf (s, sizeof (s), "playlist.scroll.%d", i);
        conf_set_int (s, p->scroll);
    }
    UNLOCK;
}

playlist_t *
plt_get_list (void) {
    return _playlists_head;
}

playlist_t *
plt_get_curr (void) {
    LOCK;
    if (!_current_playlist) {
        _current_playlist = _playlists_head;
    }
    playlist_t *plt = _current_playlist;
    if (plt) {
        plt_ref (plt);
        assert (plt->refc > 1);
    }
    UNLOCK;
    return plt;
}

playlist_t *
plt_get_for_idx (int idx) {
    LOCK;
    playlist_t *p = _playlists_head;
    for (int i = 0; p && i <= idx; i++, p = p->next) {
        if (i == idx) {
            plt_ref (p);
            UNLOCK;
            return p;
        }
    }
    UNLOCK;
    return NULL;
}

void
plt_ref (playlist_t *plt) {
    __atomic_fetch_add (&plt->refc, 1, __ATOMIC_SEQ_CST);
}

void
plt_unref (playlist_t *plt) {
    int refc = __atomic_fetch_sub (&plt->refc, 1, __ATOMIC_SEQ_CST);
    assert (refc > 0);
    if (refc <= 0) {
        trace ("\033[0;31mplaylist: bad refcount on playlist %p (%s)\033[37;0m\n", plt, plt->title);
    }
    if (refc <= 1) {
        plt_free (plt);
    }
}

int
plt_get_count (void) {
    return _playlists_count;
}

playItem_t *
plt_get_head_item (playlist_t *p, int iter) {
    pl_lock ();
    playItem_t *head = p->head[iter];
    if (head) {
        pl_item_ref (head);
    }
    pl_unlock ();
    return head;
}

playItem_t *
plt_get_tail_item (playlist_t *p, int iter) {
    pl_lock ();
    playItem_t *tail = p->tail[iter];
    if (tail) {
        pl_item_ref (tail);
    }
    pl_unlock ();
    return tail;
}

playItem_t *
plt_get_head (int plt) {
    playlist_t *p = _playlists_head;
    for (int i = 0; p && i <= plt; i++, p = p->next) {
        if (i == plt) {
            return plt_get_head_item (p, PL_MAIN);
        }
    }
    return NULL;
}

int
plt_get_sel_count (int plt) {
    playlist_t *p = _playlists_head;
    for (int i = 0; p && i <= plt; i++, p = p->next) {
        if (i == plt) {
            return plt_getselcount (p);
        }
    }
    return 0;
}

playlist_t *
plt_alloc (const char *title) {
    playlist_t *plt = malloc (sizeof (playlist_t));
    memset (plt, 0, sizeof (playlist_t));
    plt->refc = 1;
    plt->title = strdup (title);
    return plt;
}

int
plt_add (int before, const char *title) {
    assert (before >= 0);
    playlist_t *plt = plt_alloc (title);
    plt_modified (plt);

    LOCK;
    playlist_t *p_before = NULL;
    playlist_t *p_after = _playlists_head;

    for (int i = 0; i < before; i++) {
        if (i >= before + 1) {
            break;
        }
        p_before = p_after;
        if (p_after) {
            p_after = p_after->next;
        }
        else {
            p_after = _playlists_head;
        }
    }

    if (p_before) {
        p_before->next = plt;
    }
    else {
        _playlists_head = plt;
    }
    plt->next = p_after;
    _playlists_count++;

    if (!_current_playlist) {
        _current_playlist = plt;
        if (!_plt_loading) {
            // shift files
            for (int i = _playlists_count - 1; i >= before + 1; i--) {
                char path1[PATH_MAX];
                char path2[PATH_MAX];
                if (snprintf (path1, sizeof (path1), "%s/playlists/%d.dbpl", dbconfdir, i) > sizeof (path1)) {
                    fprintf (stderr, "error: failed to make path string for playlist file\n");
                    continue;
                }
                if (snprintf (path2, sizeof (path2), "%s/playlists/%d.dbpl", dbconfdir, i + 1) > sizeof (path2)) {
                    fprintf (stderr, "error: failed to make path string for playlist file\n");
                    continue;
                }
                int err = rename (path1, path2);
                if (err != 0) {
                    fprintf (stderr, "playlist rename failed: %s\n", strerror (errno));
                }
            }
        }
    }
    plt->undo_enabled = 1;
    UNLOCK;

    plt_gen_conf ();
    if (!_plt_loading) {
        plt_save_n (before);
        conf_save ();
        messagepump_push (DB_EV_PLAYLISTSWITCHED, 0, 0, 0);
        messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CREATED, 0);
    }
    return before;
}

playlist_t *
plt_append (const char *title) {
    plt_add (plt_get_count (), title);
    playlist_t *p;
    for (p = _playlists_head; p && p->next; p = p->next)
        ;
    plt_ref (p);
    return p;
}

// NOTE: caller must ensure that configuration is saved after that call
void
plt_remove (int plt) {
    LOCK;
    if (plt < 0 || plt >= _playlists_count) {
        UNLOCK;
        return;
    }

    // find playlist and notify streamer
    playlist_t *p = _playlists_head;
    playlist_t *prev = NULL;
    for (int i = 0; p && i < plt; i++) {
        prev = p;
        p = p->next;
    }

    if (!p) {
        UNLOCK;
        return;
    }

    streamer_notify_playlist_deleted (p);
    if (!_plt_loading) {
        // move files (will decrease number of files by 1)
        for (int i = plt + 1; i < _playlists_count; i++) {
            char path1[PATH_MAX];
            char path2[PATH_MAX];
            if (snprintf (path1, sizeof (path1), "%s/playlists/%d.dbpl", dbconfdir, i - 1) > sizeof (path1)) {
                fprintf (stderr, "error: failed to make path string for playlist file\n");
                continue;
            }
            if (snprintf (path2, sizeof (path2), "%s/playlists/%d.dbpl", dbconfdir, i) > sizeof (path2)) {
                fprintf (stderr, "error: failed to make path string for playlist file\n");
                continue;
            }
            int err = rename (path2, path1);
            if (err != 0) {
                fprintf (stderr, "playlist rename failed: %s\n", strerror (errno));
            }
        }
    }

    if (!_plt_loading && (_playlists_head && !_playlists_head->next)) {
        pl_clear ();
        free (_current_playlist->title);
        _current_playlist->title = strdup (_ ("Default"));
        UNLOCK;
        plt_gen_conf ();
        conf_save ();
        plt_save_n (0);
        messagepump_push (DB_EV_PLAYLISTSWITCHED, 0, 0, 0);
        messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_DELETED, 0);
        return;
    }
    if (p) {
        if (!prev) {
            _playlists_head = p->next;
        }
        else {
            prev->next = p->next;
        }
    }
    playlist_t *next = p->next;
    playlist_t *old = _current_playlist;
    _current_playlist = p;
    _current_playlist = old;

    playlist_t *new_current_playlist = _current_playlist;

    // deleted the current playlist?
    if (p == new_current_playlist) {
        if (next) {
            new_current_playlist = next;
        }
        else {
            new_current_playlist = prev ? prev : _playlists_head;
        }
    }

    _plt_set_curr_int (new_current_playlist);

    plt_unref (p);
    _playlists_count--;
    UNLOCK;

    plt_gen_conf ();
    conf_save ();
    if (!_plt_loading) {
        messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_DELETED, 0);
    }
}

int
plt_find (const char *name) {
    playlist_t *p = _playlists_head;
    int i = -1;
    for (i = 0; p; i++, p = p->next) {
        if (!strcmp (p->title, name)) {
            return i;
        }
    }
    return -1;
}

playlist_t *
plt_find_by_name (const char *name) {
    playlist_t *p = _playlists_head;
    for (; p; p = p->next) {
        if (!strcmp (p->title, name)) {
            plt_ref (p);
            return p;
        }
    }
    return NULL;
}

static void
_plt_set_curr_int (playlist_t *plt) {
    int old_index = conf_get_int ("playlist.current", 0);

    playlist_t *new_playlist = plt ? plt : &_dummy_playlist;

    int playlist_switched = new_playlist != _current_playlist;

    _current_playlist = new_playlist;

    int new_index = plt_get_idx (_current_playlist);

    if (!_plt_loading) {
        if (old_index != new_index) {
            conf_set_int ("playlist.current", plt_get_curr_idx ());
            conf_save ();
        }
        if (playlist_switched) {
            messagepump_push (DB_EV_PLAYLISTSWITCHED, 0, 0, 0);
        }
    }
}

void
plt_set_curr (playlist_t *plt) {
    LOCK;
    _plt_set_curr_int (plt);
    UNLOCK;
}

void
plt_set_curr_idx (int plt) {
    int i;
    LOCK;
    playlist_t *p = _playlists_head;
    for (i = 0; p && p->next && i < plt; i++) {
        p = p->next;
    }

    _plt_set_curr_int (p);

    UNLOCK;
}

int
plt_get_curr_idx (void) {
    int i;
    LOCK;
    playlist_t *p = _playlists_head;
    for (i = 0; p && i < _playlists_count; i++) {
        if (p == _current_playlist) {
            UNLOCK;
            return i;
        }
        p = p->next;
    }
    UNLOCK;
    return -1;
}

int
plt_get_idx_of (playlist_t *plt) {
    int i;
    LOCK;
    playlist_t *p = _playlists_head;
    for (i = 0; p && i < _playlists_count; i++) {
        if (p == plt) {
            UNLOCK;
            return i;
        }
        p = p->next;
    }
    UNLOCK;
    return -1;
}

int
plt_get_title (playlist_t *p, char *buffer, int bufsize) {
    LOCK;
    if (!buffer) {
        int l = (int)strlen (p->title);
        UNLOCK;
        return l;
    }
    strncpy (buffer, p->title, bufsize);
    buffer[bufsize - 1] = 0;
    UNLOCK;
    return 0;
}

int
plt_set_title (playlist_t *p, const char *title) {
    LOCK;
    free (p->title);
    p->title = strdup (title);
    plt_gen_conf ();
    UNLOCK;
    conf_save ();
    if (!_plt_loading) {
        messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_TITLE, 0);
    }
    return 0;
}

void
plt_modified (playlist_t *plt) {
    pl_lock ();
    plt->modification_idx++;
    pl_unlock ();
}

int
plt_get_modification_idx (playlist_t *plt) {
    pl_lock ();
    int idx = plt->modification_idx;
    pl_unlock ();
    return idx;
}

void
plt_free (playlist_t *plt) {
    LOCK;
    plt->undo_enabled = 0;
    plt_clear (plt);

    if (plt->title) {
        free (plt->title);
    }

    while (plt->meta) {
        DB_metaInfo_t *m = plt->meta;
        plt->meta = m->next;
        metacache_remove_string (m->key);
        metacache_remove_string (m->value);
        free (m);
    }

    free (plt);
    UNLOCK;
}

void
plt_move (int from, int to) {
    if (from == to) {
        return;
    }
    if (from >= _playlists_count || to >= _playlists_count) {
        fprintf (
            stderr,
            "plt_move: attempt to move playlist to/from out of bounds index; from=%d, to=%d, count=%d\n",
            from,
            to,
            _playlists_count);
        return;
    }
    LOCK;
    playlist_t *p = _playlists_head;

    playlist_t *pfrom = NULL;
    playlist_t *prev = NULL;

    // move 'from' to temp file
    char path1[PATH_MAX];
    char temp[PATH_MAX];
    if (snprintf (path1, sizeof (path1), "%s/playlists/%d.dbpl", dbconfdir, from) > sizeof (path1)) {
        fprintf (stderr, "error: failed to make path string for playlist file\n");
        UNLOCK;
        return;
    }
    if (snprintf (temp, sizeof (temp), "%s/playlists/temp.dbpl", dbconfdir) > sizeof (temp)) {
        fprintf (stderr, "error: failed to make path string for playlist file\n");
        UNLOCK;
        return;
    }

    struct stat st;
    int err = stat (path1, &st);
    if (err == 0) {
        err = rename (path1, temp);
        if (err != 0) {
            fprintf (stderr, "playlist rename %s->%s failed: %s\n", path1, temp, strerror (errno));
            UNLOCK;
            return;
        }
    }

    // remove 'from' from list
    for (int i = 0; p && i <= from; i++) {
        if (i == from) {
            pfrom = p;
            if (prev) {
                prev->next = p->next;
            }
            else {
                _playlists_head = p->next;
            }
            break;
        }
        prev = p;
        p = p->next;
    }

    if (!pfrom) {
        UNLOCK;
        return;
    }

    // shift files to fill the gap
    for (int i = from; i < _playlists_count - 1; i++) {
        char path2[PATH_MAX];
        if (snprintf (path1, sizeof (path1), "%s/playlists/%d.dbpl", dbconfdir, i) > sizeof (path1)) {
            fprintf (stderr, "error: failed to make path string for playlist file\n");
            continue;
        }
        if (snprintf (path2, sizeof (path2), "%s/playlists/%d.dbpl", dbconfdir, i + 1) > sizeof (path2)) {
            fprintf (stderr, "error: failed to make path string for playlist file\n");
            continue;
        }
        err = stat (path2, &st);
        if (err == 0) {
            err = rename (path2, path1);
            if (err != 0) {
                fprintf (stderr, "playlist rename %s->%s failed: %s\n", path2, path1, strerror (errno));
            }
        }
    }
    // open new gap
    for (int i = _playlists_count - 2; i >= to; i--) {
        char path2[PATH_MAX];
        if (snprintf (path1, sizeof (path1), "%s/playlists/%d.dbpl", dbconfdir, i) > sizeof (path1)) {
            fprintf (stderr, "error: failed to make path string for playlist file\n");
            continue;
        }
        if (snprintf (path2, sizeof (path2), "%s/playlists/%d.dbpl", dbconfdir, i + 1) > sizeof (path2)) {
            fprintf (stderr, "error: failed to make path string for playlist file\n");
            continue;
        }
        err = stat (path1, &st);
        if (err == 0) {
            err = rename (path1, path2);
            if (err != 0) {
                fprintf (stderr, "playlist rename %s->%s failed: %s\n", path1, path2, strerror (errno));
            }
        }
    }
    // move temp file
    if (snprintf (path1, sizeof (path1), "%s/playlists/%d.dbpl", dbconfdir, to) > sizeof (path1)) {
        fprintf (stderr, "error: failed to make path string for playlist file\n");
    }
    else {
        err = stat (temp, &st);
        if (err == 0) {
            err = rename (temp, path1);
            if (err != 0) {
                fprintf (stderr, "playlist rename %s->%s failed: %s\n", temp, path1, strerror (errno));
            }
        }
    }

    // move pointers
    if (to == 0) {
        // insert 'from' as head
        pfrom->next = _playlists_head;
        _playlists_head = pfrom;
    }
    else {
        // insert 'from' in the middle
        p = _playlists_head;
        for (int i = 0; p && i < to; i++) {
            if (i == to - 1) {
                playlist_t *next = p->next;
                p->next = pfrom;
                pfrom->next = next;
                break;
            }
            p = p->next;
        }
    }

    UNLOCK;
    plt_gen_conf ();
    conf_save ();
}

void
plt_clear (playlist_t *plt) {
    // Remove all tracks in bulk,
    // instead of removing each track individually.
    // Helps to avoid a long slow lock, and eliminates some concurrency issues.

    playItem_t *it = NULL;

    pl_lock ();

    it = plt->head[PL_MAIN];
    plt->count[PL_MAIN] = 0;
    plt->count[PL_SEARCH] = 0;
    plt->totaltime = 0;
    plt->seltime = 0;
    plt->head[PL_MAIN] = NULL;
    plt->head[PL_SEARCH] = NULL;
    plt->tail[PL_MAIN] = NULL;
    plt->tail[PL_SEARCH] = NULL;
    plt->current_row[PL_MAIN] = -1;
    plt->current_row[PL_SEARCH] = -1;
    plt->scroll = 0;
    plt->modification_idx++;

    pl_unlock ();

    ddb_undobuffer_t *undobuffer = ddb_undomanager_get_buffer (ddb_undomanager_shared ());
    ddb_undobuffer_group_begin (undobuffer);
    while (it != NULL) {
        playItem_t *next = it->next[PL_MAIN];
        playItem_t *next_search = it->next[PL_SEARCH];

        undo_remove_items(undobuffer, plt, &it, 1);

        if (next != NULL) {
            next->prev[PL_MAIN] = NULL;
        }
        if (next_search != NULL) {
            next_search->prev[PL_SEARCH] = NULL;
        }

        it->next[PL_MAIN] = NULL;
        it->next[PL_SEARCH] = NULL;

        streamer_song_removed_notify (it);
        playqueue_remove (it);
        pl_item_unref (it);
        it = next;
    }
    ddb_undobuffer_group_end (undobuffer);
}

void
pl_clear (void) {
    LOCK;
    plt_clear (_current_playlist);
    UNLOCK;
}

playItem_t * /* insert internal cuesheet - called by plugins */
plt_insert_cue_from_buffer (
    playlist_t *plt,
    playItem_t *after,
    playItem_t *origin,
    const uint8_t *buffer,
    int buffersize,
    int numsamples,
    int samplerate) {
    if (plt->loading_cue) {
        // means it was called from _load_cue
        plt->cue_numsamples = numsamples;
        plt->cue_samplerate = samplerate;
    }
    return NULL;
}

playItem_t * /* insert external cuesheet - called by plugins */
plt_insert_cue (playlist_t *plt, playItem_t *after, playItem_t *origin, int numsamples, int samplerate) {
    if (plt->loading_cue) {
        // means it was called from _load_cue
        plt->cue_numsamples = numsamples;
        plt->cue_samplerate = samplerate;
    }
    return NULL;
}

static playItem_t *
plt_insert_dir_int (
    int visibility,
    uint32_t flags,
    playlist_t *playlist,
    DB_vfs_t *vfs,
    playItem_t *after,
    const char *dirname,
    int *pabort,
    int (*callback) (playItem_t *it, void *data),
    int (*callback_with_result) (ddb_insert_file_result_t result, const char *fname, void *user_data),
    void *user_data);

static playItem_t *
plt_load_int (
    int visibility,
    playlist_t *plt,
    playItem_t *after,
    const char *fname,
    int *pabort,
    int (*cb) (playItem_t *it, void *data),
    void *user_data);

int
fileadd_filter_test (ddb_file_found_data_t *data) {
    for (ddb_fileadd_filter_t *f = file_add_filters; f; f = f->next) {
        int res = f->callback (data, f->user_data);
        if (res < 0) {
            return res;
        }
    }
    return 0;
}

static playItem_t *
plt_insert_file_int (
    int visibility,
    uint32_t flags,
    playlist_t *plt,
    playItem_t *after,
    const char *fname,
    int *pabort,
    int (*callback) (playItem_t *it, void *data),
    int (*callback_with_result) (ddb_insert_file_result_t result, const char *fname, void *user_data),
    void *user_data) {
    if (!fname || !(*fname)) {
        if (callback_with_result) {
            callback_with_result (DDB_INSERT_FILE_RESULT_NULL_FILENAME, fname, user_data);
        }
        return NULL;
    }

    printf("%s\n", fname);
    // check if that is supported container format
    if (!plt->ignore_archives) {
        DB_vfs_t **vfsplugs = plug_get_vfs_list ();
        for (int i = 0; vfsplugs[i]; i++) {
            if (vfsplugs[i]->is_container) {
                if (vfsplugs[i]->is_container (fname)) {
                    playItem_t *it = plt_insert_dir_int (
                        visibility,
                        flags,
                        plt,
                        vfsplugs[i],
                        after,
                        fname,
                        pabort,
                        callback,
                        NULL,
                        user_data);
                    if (it) {
                        if (callback_with_result) {
                            callback_with_result (DDB_INSERT_FILE_RESULT_SUCCESS, fname, user_data);
                        }
                        return it;
                    }
                }
            }
        }
    }

    const char *fn = strrchr (fname, '/');
    if (!fn) {
        fn = fname;
    }
    else {
        fn++;
    }

    // add all possible streams as special-case:
    // set decoder to NULL, and filetype to "content"
    // streamer is responsible to determine content type on 1st access and
    // update decoder and filetype fields
    if (strncasecmp (fname, "file://", 7)) {
        const char *p = fname;
        int detect_on_access = 1;

        // check if it's URI
        for (; *p; p++) {
            if (!strncmp (p, "://", 3)) {
                break;
            }
            if (!isalpha (*p)) {
                detect_on_access = 0;
                break;
            }
        }

        if (detect_on_access) {
            // find vfs plugin
            DB_vfs_t **vfsplugs = plug_get_vfs_list ();
            for (int i = 0; vfsplugs[i]; i++) {
                if (vfsplugs[i]->get_schemes) {
                    const char **sch = vfsplugs[i]->get_schemes ();
                    int s = 0;
                    for (s = 0; sch[s]; s++) {
                        if (!strncasecmp (sch[s], fname, strlen (sch[s]))) {
                            break;
                        }
                    }
                    if (sch[s]) {
                        if (!vfsplugs[i]->is_streaming || !vfsplugs[i]->is_streaming ()) {
                            detect_on_access = 0;
                        }
                        break;
                    }
                }
            }
        }

        if (detect_on_access && *p == ':') {
            // check for wrong chars like CR/LF, TAB, etc
            // they are not allowed and might lead to corrupt display in GUI
            int32_t i = 0;
            while (fname[i]) {
                uint32_t c = u8_nextchar (fname, &i);
                if (c < 0x20) {
                    if (callback_with_result) {
                        callback_with_result (DDB_INSERT_FILE_RESULT_NULL_FILENAME, fname, user_data);
                    }
                    return NULL;
                }
            }

            playItem_t *it = pl_item_alloc_init (fname, NULL);
            pl_replace_meta (it, ":FILETYPE", "content");
            after = plt_insert_item (addfiles_playlist ? addfiles_playlist : plt, after, it);
            pl_item_unref (it);
            if (callback_with_result) {
                callback_with_result (DDB_INSERT_FILE_RESULT_SUCCESS, fname, user_data);
            }
            return after;
        }
    }
    else {
        char *escaped = uri_unescape (fname, (int)strlen (fname));
        if (escaped) {
            fname = strdupa (escaped);
            free (escaped);
        }
        fname += 7;
    }

#ifdef __MINGW32__
    // replace backslashes with normal slashes
    char fname_conv[strlen (fname) + 1];
    if (strchr (fname, '\\')) {
        trace ("plt_insert_file_int: backslash(es) detected: %s\n", fname);
        strcpy (fname_conv, fname);
        char *slash_p = fname_conv;
        while (slash_p = strchr (slash_p, '\\')) {
            *slash_p = '/';
            slash_p++;
        }
        fname = fname_conv;
    }
    // path should start with "X:/", not "/X:/", fixing to avoid file opening problems
    if (fname[0] == '/' && isalpha (fname[1]) && fname[2] == ':') {
        fname++;
    }
#endif

    // now that it's known we're not dealing with URL, check if it's a relative path
    if (is_relative_path (fname)) {
        if (callback_with_result) {
            callback_with_result (DDB_INSERT_FILE_RESULT_RELATIVE_PATH, fname, user_data);
        }
        return NULL;
    }

    // detect decoder
    const char *eol = strrchr (fname, '.');
    if (!eol) {
        if (callback_with_result) {
            callback_with_result (DDB_INSERT_FILE_RESULT_NO_FILE_EXTENSION, fname, user_data);
        }
        return NULL;
    }
    eol++;

    // handle cue files
    if (!strcasecmp (eol, "cue")) {
        playItem_t *inserted = plt_load_cue_file (plt, after, fname, NULL, NULL, 0);
        if (callback_with_result) {
            callback_with_result (
                inserted ? DDB_INSERT_FILE_RESULT_SUCCESS : DDB_INSERT_FILE_RESULT_CUESHEET_ERROR,
                fname,
                user_data);
        }
        return inserted;
    }

    int filter_done = 0;
    int file_recognized = 0;

    DB_decoder_t **decoders = plug_get_decoder_list ();
    // match by decoder
    for (int i = 0; decoders[i]; i++) {
        if (decoders[i]->exts && decoders[i]->insert) {
            const char **exts = decoders[i]->exts;
            for (int e = 0; exts[e]; e++) {
                if (!strcasecmp (exts[e], eol) || !strcmp (exts[e], "*")) {
                    if (!filter_done) {
                        ddb_file_found_data_t dt;
                        dt.filename = fname;
                        dt.plt = (ddb_playlist_t *)plt;
                        dt.is_dir = 0;
                        if (fileadd_filter_test (&dt) < 0) {
                            return NULL;
                        }
                        filter_done = 1;
                    }

                    file_recognized = 1;

                    playItem_t *inserted =
                        (playItem_t *)decoders[i]->insert ((ddb_playlist_t *)plt, DB_PLAYITEM (after), fname);
                    if (inserted != NULL) {
                        if (callback && callback (inserted, user_data) < 0) {
                            *pabort = 1;
                        }
                        else if (
                            callback_with_result &&
                            callback_with_result (DDB_INSERT_FILE_RESULT_SUCCESS, fname, user_data) < 0) {
                            *pabort = 1;
                        }
                        if (file_add_listeners) {
                            ddb_fileadd_data_t d;
                            memset (&d, 0, sizeof (d));
                            d.visibility = visibility;
                            d.plt = (ddb_playlist_t *)plt;
                            d.track = (ddb_playItem_t *)inserted;
                            for (ddb_fileadd_listener_t *l = file_add_listeners; l; l = l->next) {
                                if (pabort && l->callback (&d, l->user_data) < 0) {
                                    *pabort = 1;
                                    break;
                                }
                            }
                        }
                        return inserted;
                    }
                }
            }
        }
        if (decoders[i]->prefixes && decoders[i]->insert) {
            const char **prefixes = decoders[i]->prefixes;
            for (int e = 0; prefixes[e]; e++) {
                if (!strncasecmp (prefixes[e], fn, strlen (prefixes[e])) && *(fn + strlen (prefixes[e])) == '.') {
                    if (!filter_done) {
                        ddb_file_found_data_t dt;
                        dt.filename = fname;
                        dt.plt = (ddb_playlist_t *)plt;
                        dt.is_dir = 0;
                        if (fileadd_filter_test (&dt) < 0) {
                            return NULL;
                        }
                        filter_done = 1;
                    }

                    file_recognized = 1;
                    playItem_t *inserted =
                        (playItem_t *)decoders[i]->insert ((ddb_playlist_t *)plt, DB_PLAYITEM (after), fname);
                    if (inserted != NULL) {
                        if (callback && callback (inserted, user_data) < 0) {
                            *pabort = 1;
                        }
                        else if (
                            callback_with_result &&
                            callback_with_result (DDB_INSERT_FILE_RESULT_SUCCESS, fname, user_data) < 0) {
                            *pabort = 1;
                        }
                        if (file_add_listeners) {
                            ddb_fileadd_data_t d;
                            memset (&d, 0, sizeof (d));
                            d.visibility = visibility;
                            d.plt = (ddb_playlist_t *)plt;
                            d.track = (ddb_playItem_t *)inserted;
                            for (ddb_fileadd_listener_t *l = file_add_listeners; l; l = l->next) {
                                if (l->callback (&d, l->user_data) < 0) {
                                    *pabort = 1;
                                    break;
                                }
                            }
                        }
                        return inserted;
                    }
                }
            }
        }
    }
    if (file_recognized) {
        if (callback_with_result) {
            callback_with_result (DDB_INSERT_FILE_RESULT_RECOGNIZED_FAILED, fname, user_data);
        }
        else {
            trace_err ("ERROR: could not load: %s\n", fname);
        }
    }
    else if (callback_with_result) {
        callback_with_result (DDB_INSERT_FILE_RESULT_UNRECOGNIZED_FILE, fname, user_data);
    }
    return NULL;
}

static uint32_t
_get_insert_file_flags_from_config (void) {
    uint32_t flags = 0;
    if (conf_get_int ("add_folders_follow_symlinks", 0)) {
        flags |= DDB_INSERT_FILE_FLAG_FOLLOW_SYMLINKS;
    }
    if (!conf_get_int ("ignore_archives", 1)) {
        flags |= DDB_INSERT_FILE_FLAG_ENTER_ARCHIVES;
    }
    return flags;
}

playItem_t *
plt_insert_file (
    playlist_t *plt,
    playItem_t *after,
    const char *fname,
    int *pabort,
    int (*cb) (playItem_t *it, void *data),
    void *user_data) {
    return plt_insert_file_int (
        0,
        _get_insert_file_flags_from_config (),
        plt,
        after,
        fname,
        pabort,
        cb,
        NULL,
        user_data);
}

static int
dirent_alphasort (const struct dirent **a, const struct dirent **b) {
    return strcmp ((*a)->d_name, (*b)->d_name);
}

static void
_get_fullname_and_dir (
    char *fullname,
    int sz,
    char *dir,
    int dirsz,
    DB_vfs_t *vfs,
    const char *dirname,
    const char *d_name) {
    char resolved_dirname[PATH_MAX];

    char *res = realpath (dirname, resolved_dirname);
    if (res) {
        dirname = resolved_dirname;
    }

    if (!vfs) {
        // prevent double-slashes
        char *stripped_dirname = strdupa (dirname);
        char *p = stripped_dirname + strlen (stripped_dirname) - 1;
        while (p > stripped_dirname && (*p == '/' || *p == '\\')) {
            p--;
        }
        p++;
        *p = 0;

        snprintf (fullname, sz, "%s/%s", stripped_dirname, d_name);
        if (dir) {
            *dir = 0;
            strncat (dir, stripped_dirname, dirsz - 1);
        }
    }
    else {
        const char *sch = NULL;
        if (vfs->plugin.api_vminor >= 6 && vfs->get_scheme_for_name) {
            sch = vfs->get_scheme_for_name (dirname);
        }
        if (sch && strncmp (sch, d_name, strlen (sch))) {
            snprintf (fullname, sz, "%s%s:%s", sch, dirname, d_name);
            if (dir) {
                snprintf (dir, dirsz, "%s%s:", sch, dirname);
            }
        }
        else {
            *fullname = 0;
            strncat (fullname, d_name, sz - 1);
            if (dir) {
                *dir = 0;
                strncat (dir, dirname, dirsz - 1);
            }
        }
    }

#ifdef __MINGW32__
    if (fullname && strchr (fullname, '\\')) {
        char *slash_p = fullname;
        while (slash_p = strchr (slash_p, '\\')) {
            *slash_p = '/';
            slash_p++;
        }
        trace ("_get_fullname_and_dir backslash(es) found, converted: %s\n", fullname);
    }
    if (dir && strchr (dir, '\\')) {
        char *slash_p = dir;
        while (slash_p = strchr (slash_p, '\\')) {
            *slash_p = '/';
            slash_p++;
        }
        trace ("_get_fullname_and_dir backslash(es) found, converted: %s\n", dir);
    }
#endif
}

static playItem_t *
plt_insert_dir_int (
    int visibility,
    uint32_t flags,
    playlist_t *plt,
    DB_vfs_t *vfs,
    playItem_t *after,
    const char *dirname,
    int *pabort,
    int (*callback) (playItem_t *it, void *data),
    int (*callback_with_result) (ddb_insert_file_result_t result, const char *fname, void *user_data),
    void *user_data) {
    plt->follow_symlinks = (flags & DDB_INSERT_FILE_FLAG_FOLLOW_SYMLINKS) ? 1 : 0;
    plt->ignore_archives = (flags & DDB_INSERT_FILE_FLAG_ENTER_ARCHIVES) ? 0 : 1;

    if (!strncmp (dirname, "file://", 7)) {
        dirname += 7;
    }

#ifdef __MINGW32__
    // replace backslashes with normal slashes
    char dirname_conv[strlen (dirname) + 1];
    if (strchr (dirname, '\\')) {
        trace ("plt_insert_dir_int: backslash(es) detected: %s\n", dirname);
        strcpy (dirname_conv, dirname);
        char *slash_p = dirname_conv;
        while (slash_p = strchr (slash_p, '\\')) {
            *slash_p = '/';
            slash_p++;
        }
        dirname = dirname_conv;
    }
    // path should start with "X:/", not "/X:/", fixing to avoid file opening problems
    if (dirname[0] == '/' && isalpha (dirname[1]) && dirname[2] == ':') {
        dirname++;
    }
#endif

    if (is_relative_path (dirname)) {
        return NULL;
    }

    if (!plt->follow_symlinks && !vfs) {
        struct stat buf;
        lstat (dirname, &buf);
        if (S_ISLNK (buf.st_mode)) {
            return NULL;
        }
    }

    ddb_file_found_data_t dt;
    dt.filename = dirname;
    dt.plt = (ddb_playlist_t *)plt;
    dt.is_dir = 1;
    if (fileadd_filter_test (&dt) < 0) {
        return NULL;
    }

    struct dirent **namelist = NULL;
    int n;

    if (vfs && vfs->scandir) {
        n = vfs->scandir (dirname, &namelist, NULL, dirent_alphasort);
// we can't rely on vfs plugins to set d_type
// windows/svr4 unixes: missing dirent[]->d_type
#if !defined(__MINGW32__) && !defined(__SVR4)
        for (int i = 0; i < n; i++) {
            namelist[i]->d_type = DT_REG;
        }
#endif
    }
    else {
        n = scandir (dirname, &namelist, NULL, dirent_alphasort);
    }
    if (n < 0) {
        if (namelist) {
            free (namelist);
        }
        return NULL; // not a dir or no read access
    }

    // find all cue files in the folder
    int cuefiles[n];
    int ncuefiles = 0;

    for (int i = 0; i < n; i++) {
// no hidden files
// windows/svr4 unixes: missing dirent[]->d_type
#if defined(__MINGW32__) || defined(__SVR4)
        if (namelist[i]->d_name[0] == '.')
#else
        if (namelist[i]->d_name[0] == '.' || (namelist[i]->d_type != DT_REG && namelist[i]->d_type != DT_UNKNOWN))
#endif
        {
            continue;
        }

        size_t l = strlen (namelist[i]->d_name);
        if (l > 4 && !strcasecmp (namelist[i]->d_name + l - 4, ".cue")) {
            cuefiles[ncuefiles++] = i;
        }
    }

    char fullname[PATH_MAX];
    char fulldir[PATH_MAX];

    // try loading cuesheets first
    for (int c = 0; c < ncuefiles; c++) {
        int i = cuefiles[c];
        _get_fullname_and_dir (
            fullname,
            sizeof (fullname),
            fulldir,
            sizeof (fulldir),
            vfs,
            dirname,
            namelist[i]->d_name);

        playItem_t *inserted = plt_load_cue_file (plt, after, fullname, fulldir, namelist, n);
        namelist[i]->d_name[0] = 0;

        if (inserted) {
            after = inserted;
        }
        if (pabort && *pabort) {
            break;
        }
    }

    // load the rest of the files
    if (!pabort || !*pabort) {
        for (int i = 0; i < n; i++) {
            // no hidden files
            if (!namelist[i]->d_name[0] || namelist[i]->d_name[0] == '.') {
                continue;
            }
            _get_fullname_and_dir (fullname, sizeof (fullname), NULL, 0, vfs, dirname, namelist[i]->d_name);
            playItem_t *inserted = NULL;
            if (!vfs) {
                inserted = plt_insert_dir_int (
                    visibility,
                    flags,
                    plt,
                    vfs,
                    after,
                    fullname,
                    pabort,
                    callback,
                    callback_with_result,
                    user_data);
            }
            if (!inserted) {
                inserted = plt_insert_file_int (
                    visibility,
                    flags,
                    plt,
                    after,
                    fullname,
                    pabort,
                    callback,
                    callback_with_result,
                    user_data);
            }

            if (inserted) {
                after = inserted;
            }

            if (pabort && *pabort) {
                break;
            }
        }
    }

    // free data
    for (int i = 0; i < n; i++) {
        free (namelist[i]);
    }
    free (namelist);

    return after;
}

playItem_t *
plt_insert_dir (
    playlist_t *playlist,
    playItem_t *after,
    const char *dirname,
    int *pabort,
    int (*cb) (playItem_t *it, void *data),
    void *user_data) {

    playItem_t *ret = plt_insert_dir_int (
        0,
        _get_insert_file_flags_from_config (),
        playlist,
        NULL,
        after,
        dirname,
        pabort,
        cb,
        NULL,
        user_data);

    return ret;
}

static int
plt_add_file_int (
    int visibility,
    uint32_t flags,
    playlist_t *plt,
    const char *fname,
    int (*cb) (playItem_t *it, void *data),
    void *user_data) {
    int abort = 0;
    playItem_t *it =
        plt_insert_file_int (visibility, flags, plt, plt->tail[PL_MAIN], fname, &abort, cb, NULL, user_data);
    if (it) {
        // pl_insert_file doesn't hold reference, don't unref here
        return 0;
    }
    return -1;
}

int
plt_add_file (playlist_t *plt, const char *fname, int (*cb) (playItem_t *it, void *data), void *user_data) {
    return plt_add_file_int (0, _get_insert_file_flags_from_config (), plt, fname, cb, user_data);
}

int
plt_add_dir (playlist_t *plt, const char *dirname, int (*cb) (playItem_t *it, void *data), void *user_data) {
    int abort = 0;
    playItem_t *it = plt_insert_dir (plt, plt->tail[PL_MAIN], dirname, &abort, cb, user_data);
    if (it) {
        // pl_insert_file doesn't hold reference, don't unref here
        return 0;
    }
    return -1;
}

int
pl_add_files_begin (playlist_t *plt) {
    return plt_add_files_begin (plt, 0);
}

void
pl_add_files_end (void) {
    return plt_add_files_end (addfiles_playlist, 0);
}

int
plt_remove_item (playlist_t *playlist, playItem_t *it) {
    if (!it)
        return -1;

    if (!no_remove_notify) {
        streamer_song_removed_notify (it);
        playqueue_remove (it);
    }

    // remove from both lists
    LOCK;
    undo_remove_items(ddb_undomanager_get_buffer(ddb_undomanager_shared()), playlist, &it, 1);

    for (int iter = PL_MAIN; iter <= PL_SEARCH; iter++) {
        if (it->prev[iter] || it->next[iter] || playlist->head[iter] == it || playlist->tail[iter] == it) {
            playlist->count[iter]--;
        }

        playItem_t *next = it->next[iter];
        playItem_t *prev = it->prev[iter];

        if (prev) {
            prev->next[iter] = next;
        }
        else {
            playlist->head[iter] = next;
        }
        if (next) {
            next->prev[iter] = prev;
        }
        else {
            playlist->tail[iter] = prev;
        }

        it->next[iter] = NULL;
        it->prev[iter] = NULL;
    }

    float dur = pl_get_item_duration (it);
    if (dur > 0) {
        // totaltime
        playlist->totaltime -= dur;
        if (playlist->totaltime < 0) {
            playlist->totaltime = 0;
        }

        // selected time
        if (it->selected) {
            playlist->seltime -= dur;
            if (playlist->seltime < 0) {
                playlist->seltime = 0;
            }
        }
    }

    plt_modified (playlist);
    pl_item_unref (it);
    UNLOCK;
    return 0;
}

int
plt_get_item_count (playlist_t *plt, int iter) {
    return plt->count[iter];
}

int
pl_getcount (int iter) {
    LOCK;
    if (!_current_playlist) {
        UNLOCK;
        return 0;
    }

    int cnt = _current_playlist->count[iter];
    UNLOCK;
    return cnt;
}

int
plt_getselcount (playlist_t *playlist) {
    LOCK;
    int cnt = 0;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        if (it->selected) {
            cnt++;
        }
    }
    UNLOCK;
    return cnt;
}

int
pl_getselcount (void) {
    LOCK;
    int cnt = plt_getselcount (_current_playlist);
    UNLOCK;
    return cnt;
}

playItem_t *
plt_get_item_for_idx (playlist_t *playlist, int idx, int iter) {
    LOCK;
    playItem_t *it = playlist->head[iter];
    while (idx--) {
        if (!it) {
            UNLOCK;
            return NULL;
        }
        it = it->next[iter];
    }
    if (it) {
        pl_item_ref (it);
    }
    UNLOCK;
    return it;
}

playItem_t *
pl_get_for_idx_and_iter (int idx, int iter) {
    LOCK;
    playItem_t *it = plt_get_item_for_idx (_current_playlist, idx, iter);
    UNLOCK;
    return it;
}

playItem_t *
pl_get_for_idx (int idx) {
    return pl_get_for_idx_and_iter (idx, PL_MAIN);
}

int
plt_get_item_idx (playlist_t *playlist, playItem_t *it, int iter) {
    LOCK;
    playItem_t *c = playlist->head[iter];
    int idx = 0;
    while (c && c != it) {
        c = c->next[iter];
        idx++;
    }
    if (!c) {
        UNLOCK;
        return -1;
    }
    UNLOCK;
    return idx;
}

int
pl_get_idx_of (playItem_t *it) {
    return pl_get_idx_of_iter (it, PL_MAIN);
}

int
pl_get_idx_of_iter (playItem_t *it, int iter) {
    LOCK;
    int idx = plt_get_item_idx (_current_playlist, it, iter);
    UNLOCK;
    return idx;
}

playItem_t *
plt_insert_item (playlist_t *playlist, playItem_t *after, playItem_t *it) {
    LOCK;
    undo_insert_items(ddb_undomanager_get_buffer(ddb_undomanager_shared()), playlist, &it, 1);
    pl_item_ref (it);
    if (!after) {
        it->next[PL_MAIN] = playlist->head[PL_MAIN];
        it->prev[PL_MAIN] = NULL;
        if (playlist->head[PL_MAIN]) {
            playlist->head[PL_MAIN]->prev[PL_MAIN] = it;
        }
        else {
            playlist->tail[PL_MAIN] = it;
        }
        playlist->head[PL_MAIN] = it;
    }
    else {
        it->prev[PL_MAIN] = after;
        it->next[PL_MAIN] = after->next[PL_MAIN];
        if (after->next[PL_MAIN]) {
            after->next[PL_MAIN]->prev[PL_MAIN] = it;
        }
        after->next[PL_MAIN] = it;
        if (after == playlist->tail[PL_MAIN]) {
            playlist->tail[PL_MAIN] = it;
        }
    }
    it->in_playlist = 1;

    playlist->count[PL_MAIN]++;

    // shuffle
    playItem_t *prev = it->prev[PL_MAIN];
    if (streamer_get_shuffle () == DDB_SHUFFLE_ALBUMS && prev && pl_items_from_same_album (prev, it)) {
        it->shufflerating = prev->shufflerating;
    }
    else {
        it->shufflerating = rand ();
    }
    it->played = 0;

    // totaltime
    float dur = pl_get_item_duration (it);
    if (dur > 0) {
        playlist->totaltime += dur;
    }

    plt_modified (playlist);

    UNLOCK;
    return it;
}

playItem_t *
pl_insert_item (playItem_t *after, playItem_t *it) {
    return plt_insert_item (addfiles_playlist ? addfiles_playlist : _current_playlist, after, it);
}

void
pl_item_copy (playItem_t *out, playItem_t *it) {
    LOCK;
    out->startsample = it->startsample;
    out->endsample = it->endsample;
    out->startsample64 = it->startsample64;
    out->endsample64 = it->endsample64;
    out->shufflerating = it->shufflerating;
    out->_duration = it->_duration;
    out->next[PL_MAIN] = it->next[PL_MAIN];
    out->prev[PL_MAIN] = it->prev[PL_MAIN];
    out->next[PL_SEARCH] = it->next[PL_SEARCH];
    out->prev[PL_SEARCH] = it->prev[PL_SEARCH];

    for (DB_metaInfo_t *meta = it->meta; meta; meta = meta->next) {
        pl_add_meta_copy (out, meta);
    }
    UNLOCK;
}

playItem_t *
pl_item_alloc (void) {
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->_duration = -1;
    it->_refc = 1;
    return it;
}

playItem_t *
pl_item_alloc_init (const char *fname, const char *decoder_id) {
    playItem_t *it = pl_item_alloc ();
    pl_add_meta (it, ":URI", fname);
    pl_add_meta (it, ":DECODER", decoder_id);
    return it;
}

void
pl_item_ref (playItem_t *it) {
    __atomic_fetch_add (&it->_refc, 1, __ATOMIC_SEQ_CST);
    //fprintf (stderr, "\033[0;34m+it %p: refc=%d: %s\033[37;0m\n", it, it->_refc, pl_find_meta_raw (it, ":URI"));
}

static void
pl_item_free (playItem_t *it) {
    LOCK;
    if (it) {
        while (it->meta) {
            pl_meta_free_values (it->meta);
            DB_metaInfo_t *m = it->meta;
            it->meta = m->next;
            free (m);
        }

        free (it);
    }
    UNLOCK;
}

void
pl_item_unref (playItem_t *it) {
    int refc = __atomic_fetch_sub (&it->_refc, 1, __ATOMIC_SEQ_CST);
    if (refc <= 0) {
        trace ("\033[0;31mplaylist: bad refcount on item %p\033[37;0m\n", it);
        assert (0);
    }
    if (refc <= 1) {
        pl_item_free (it);
    }
}

int
plt_delete_selected (playlist_t *playlist) {
    LOCK;
    int count = plt_getselcount (playlist);
    if (count == 0) {
        UNLOCK;
        return -1;
    }
    playItem_t **items_to_delete = calloc (count, sizeof (playItem_t *));
    playItem_t *next = NULL;
    int i = 0;
    int ret = -1;
    int current_index = 0;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = next, current_index++) {
        next = it->next[PL_MAIN];
        if (it->selected) {
            items_to_delete[i++] = it;
            if (ret == -1) {
                ret = current_index;
            }
            pl_item_ref (it);
        }
    }
    UNLOCK;

    ddb_undobuffer_group_begin (ddb_undomanager_get_buffer (ddb_undomanager_shared ()));
    for (i = 0; i < count; i++) {
        plt_remove_item (playlist, items_to_delete[i]);
        pl_item_unref (items_to_delete[i]);
    }
    ddb_undobuffer_group_end (ddb_undomanager_get_buffer (ddb_undomanager_shared ()));
    free (items_to_delete);

    LOCK;
    if (playlist->current_row[PL_MAIN] >= playlist->count[PL_MAIN]) {
        playlist->current_row[PL_MAIN] = playlist->count[PL_MAIN] - 1;
    }
    if (playlist->current_row[PL_SEARCH] >= playlist->count[PL_SEARCH]) {
        playlist->current_row[PL_SEARCH] = playlist->count[PL_SEARCH] - 1;
    }
    UNLOCK;
    return ret;
}

int
pl_delete_selected (void) {
    LOCK;
    playlist_t *curr = _current_playlist;
    plt_ref (curr);
    UNLOCK;
    int res = plt_delete_selected (curr);
    plt_unref (curr);
    return res;
}

void
plt_crop_selected (playlist_t *playlist) {
    LOCK;
    int count = plt_get_item_count (playlist, PL_MAIN) - plt_getselcount (playlist);
    if (count == 0) {
        UNLOCK;
        return;
    }
    playItem_t **items_to_delete = calloc (count, sizeof (playItem_t *));
    playItem_t *next = NULL;
    int i = 0;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = next) {
        next = it->next[PL_MAIN];
        if (!it->selected) {
            items_to_delete[i++] = it;
            pl_item_ref (it);
        }
    }
    UNLOCK;

    for (i = 0; i < count; i++) {
        plt_remove_item (playlist, items_to_delete[i]);
        pl_item_unref (items_to_delete[i]);
    }
    free (items_to_delete);
}

void
pl_crop_selected (void) {
    plt_crop_selected (_current_playlist);
}

static uint16_t
length_to_uint16 (size_t len) {
    return (uint16_t)min (0xffff, len);
}

static uint8_t
length_to_uint8 (size_t len) {
    return (uint8_t)min (0xff, len);
}

static int
_plt_save_to_buffered_writer (
    playlist_t *plt,
    buffered_file_writer_t *writer,
    int (*cb) (playItem_t *it, void *data),
    void *user_data
) {
    LOCK;

    const char magic[] = "DBPL";
    uint8_t majorver = PLAYLIST_MAJOR_VER;
    uint8_t minorver = PLAYLIST_MINOR_VER;

    if (buffered_file_writer_write (writer, magic, 4) < 0) {
        goto save_fail;
    }
    if (buffered_file_writer_write (writer, &majorver, 1) < 0) {
        goto save_fail;
    }
    if (buffered_file_writer_write (writer, &minorver, 1) < 0) {
        goto save_fail;
    }
    uint32_t cnt = plt->count[PL_MAIN];
    if (buffered_file_writer_write (writer, &cnt, 4) < 0) {
        goto save_fail;
    }
    for (playItem_t *it = plt->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        uint16_t l;
        uint8_t ll;
        if (cb) {
            cb (it, user_data);
        }
#if (PLAYLIST_MINOR_VER == 2)
        const char *item_uri = pl_find_meta_raw (it, ":URI");
        l = length_to_uint16 (strlen (item_uri));
        if (buffered_file_writer_write (writer, &l, 2) < 0) {
            goto save_fail;
        }
        if (buffered_file_writer_write (writer, item_uri, l) < 0) {
            goto save_fail;
        }
        const char *decoder_id = pl_find_meta_raw (it, ":DECODER");
        if (decoder_id) {
            ll = length_to_uint8 (strlen (decoder_id));
            if (buffered_file_writer_write (writer, &ll, 1) < 0) {
                goto save_fail;
            }
            if (buffered_file_writer_write (writer, decoder_id, ll) < 0) {
                goto save_fail;
            }
        }
        else {
            ll = 0;
            if (buffered_file_writer_write (writer, &ll, 1) < 0) {
                goto save_fail;
            }
        }
        l = length_to_uint8 (pl_find_meta_int (it, ":TRACKNUM", 0));
        if (buffered_file_writer_write (writer, &l, 2) < 0) {
            goto save_fail;
        }
#endif
        if (buffered_file_writer_write (writer, &it->startsample, 4) < 0) {
            goto save_fail;
        }
        if (buffered_file_writer_write (writer, &it->endsample, 4) < 0) {
            goto save_fail;
        }
        if (buffered_file_writer_write (writer, &it->_duration, 4) < 0) {
            goto save_fail;
        }
#if (PLAYLIST_MINOR_VER == 2)
        const char *filetype = pl_find_meta_raw (it, ":FILETYPE");
        if (!filetype) {
            filetype = "";
        }
        uint8_t ft = length_to_uint8 (strlen (filetype));
        if (buffered_file_writer_write (writer, &ft, 1) < 0) {
            goto save_fail;
        }
        if (ft) {
            if (buffered_file_writer_write (writer, filetype, ft) < 0) {
                goto save_fail;
            }
        }
        float rg_albumgain = pl_get_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN);
        float rg_albumpeak = pl_get_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK);
        float rg_trackgain = pl_get_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN);
        float rg_trackpeak = pl_get_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK);
        if (buffered_file_writer_write (writer, &rg_albumgain, 4) < 0) {
            goto save_fail;
        }
        if (buffered_file_writer_write (writer, &rg_albumpeak, 4) < 0) {
            goto save_fail;
        }
        if (buffered_file_writer_write (writer, &rg_trackgain, 4) < 0) {
            goto save_fail;
        }
        if (buffered_file_writer_write (writer, &rg_trackpeak, 4) < 0) {
            goto save_fail;
        }
#endif
        if (buffered_file_writer_write (writer, &it->_flags, 4) < 0) {
            goto save_fail;
        }

        int16_t nm = 0;
        DB_metaInfo_t *m;
        for (m = it->meta; m; m = m->next) {
            if (m->key[0] == '_' || m->key[0] == '!') {
                continue; // skip reserved names
            }
            nm++;
        }
        if (buffered_file_writer_write (writer, &nm, 2) < 0) {
            goto save_fail;
        }
        for (m = it->meta; m; m = m->next) {
            if (m->key[0] == '_' || m->key[0] == '!') {
                continue; // skip reserved names
            }

            l = length_to_uint16 (strlen (m->key));
            if (buffered_file_writer_write (writer, &l, 2) < 0) {
                goto save_fail;
            }
            if (l) {
                if (buffered_file_writer_write (writer, m->key, l) < 0) {
                    goto save_fail;
                }
            }
            int value_length = max (0, m->valuesize - 1);
            l = length_to_uint16 (value_length);
            if (buffered_file_writer_write (writer, &l, 2) < 0) {
                goto save_fail;
            }
            if (l) {
                if (buffered_file_writer_write (writer, m->value, l) < 0) {
                    goto save_fail;
                }
            }
        }
    }

    // write playlist metadata
    int16_t nm = 0;
    DB_metaInfo_t *m;
    for (m = plt->meta; m; m = m->next) {
        nm++;
    }
    if (buffered_file_writer_write (writer, &nm, 2) < 0) {
        goto save_fail;
    }

    for (m = plt->meta; m; m = m->next) {
        uint16_t l;
        l = length_to_uint16 (strlen (m->key));
        if (buffered_file_writer_write (writer, &l, 2) < 0) {
            goto save_fail;
        }
        if (l) {
            if (buffered_file_writer_write (writer, m->key, l) < 0) {
                goto save_fail;
            }
        }
        l = length_to_uint16 (strlen (m->value));
        if (buffered_file_writer_write (writer, &l, 2) < 0) {
            goto save_fail;
        }
        if (l) {
            if (buffered_file_writer_write (writer, m->value, l) < 0) {
                goto save_fail;
            }
        }
    }

    if (buffered_file_writer_flush (writer) < 0) {
        goto save_fail;
    }

    UNLOCK;
    return 0;

save_fail:
    return -1;
}

ssize_t
plt_save_to_buffer(
                   playlist_t *plt,
                   uint8_t **out_buffer
                   ) {
    buffered_file_writer_t *writer = buffered_file_writer_new (NULL, 64 * 1024);

    LOCK;

    int result = _plt_save_to_buffered_writer (plt, writer, NULL, NULL);

    UNLOCK;

    if (result != 0) {
        buffered_file_writer_free (writer);
        *out_buffer = NULL;
        return -1;
    }

    size_t size = buffered_file_writer_get_size (writer);
    uint8_t *buffer = malloc(size);
    memcpy(buffer, buffered_file_writer_get_buffer (writer), size);
    buffered_file_writer_free (writer);

    *out_buffer = buffer;
    return size;
}

int
plt_save(
    playlist_t *plt,
    playItem_t *first,
    playItem_t *last,
    const char *fname,
    int *pabort,
    int (*cb) (playItem_t *it, void *data),
    void *user_data
) {
    LOCK;
    plt->last_save_modification_idx = plt->modification_idx;
    const char *ext = strrchr (fname, '.');
    if (ext) {
        DB_playlist_t **plug = deadbeef->plug_get_playlist_list ();
        for (int i = 0; plug[i]; i++) {
            if (plug[i]->extensions && plug[i]->load) {
                const char **exts = plug[i]->extensions;
                if (exts && plug[i]->save) {
                    for (int e = 0; exts[e]; e++) {
                        if (!strcasecmp (exts[e], ext + 1)) {
                            int res = plug[i]->save (
                                                     (ddb_playlist_t *)plt,
                                                     fname,
                                                     (DB_playItem_t *)_current_playlist->head[PL_MAIN],
                                                     NULL);
                            UNLOCK;
                            return res;
                        }
                    }
                }
            }
        }
    }

    char tempfile[PATH_MAX];
    snprintf (tempfile, sizeof (tempfile), "%s.tmp", fname);
    FILE *fp = fopen (tempfile, "w+b");
    if (!fp) {
        UNLOCK;
        return -1;
    }


    buffered_file_writer_t *writer = buffered_file_writer_new (fp, 64 * 1024);

    int result = _plt_save_to_buffered_writer (plt, writer, cb, user_data);

    buffered_file_writer_free (writer);
    writer = NULL;
    if (result != 0) {
        goto save_fail;
    }
    if (EOF == fclose (fp)) {
        fp = NULL;
        goto save_fail;
    }
    UNLOCK;
    if (rename (tempfile, fname) != 0) {
        fprintf (stderr, "playlist rename %s -> %s failed: %s\n", tempfile, fname, strerror (errno));
        return -1;
    }
    return 0;
save_fail:
    UNLOCK;
    if (writer != NULL) {
        buffered_file_writer_free (writer);
    }
    if (fp != NULL) {
        fclose (fp);
    }
    unlink (tempfile);
    return -1;
}

int
plt_save_n (int n) {
    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/playlists", dbconfdir) > sizeof (path)) {
        fprintf (stderr, "error: failed to make path string for playlists folder\n");
        return -1;
    }
    // make folder
    mkdir (path, 0755);

    LOCK;
    int err = 0;

    _plt_loading = 1;
    if (snprintf (path, sizeof (path), "%s/playlists/%d.dbpl", dbconfdir, n) > sizeof (path)) {
        fprintf (stderr, "error: failed to make path string for playlist file\n");
        UNLOCK;
        return -1;
    }

    int i;
    playlist_t *plt;
    for (i = 0, plt = _playlists_head; plt && i < n; i++, plt = plt->next)
        ;
    err = plt_save (plt, NULL, NULL, path, NULL, NULL, NULL);
    _plt_loading = 0;
    UNLOCK;
    return err;
}

int
pl_save_current (void) {
    return plt_save_n (plt_get_curr_idx ());
}

int
pl_save_all (void) {
    char path[PATH_MAX];
    if (snprintf (path, sizeof (path), "%s/playlists", dbconfdir) > sizeof (path)) {
        fprintf (stderr, "error: failed to make path string for playlists folder\n");
        return -1;
    }
    // make folder
    mkdir (path, 0755);

    LOCK;
    playlist_t *p = _playlists_head;
    int i;
    int cnt = plt_get_count ();
    int err = 0;

    plt_gen_conf ();
    _plt_loading = 1;
    for (i = 0; i < cnt; i++, p = p->next) {
        if (snprintf (path, sizeof (path), "%s/playlists/%d.dbpl", dbconfdir, i) > sizeof (path)) {
            fprintf (stderr, "error: failed to make path string for playlist file\n");
            err = -1;
            break;
        }
        if (p->last_save_modification_idx == p->modification_idx) {
            continue;
        }
        err = plt_save (p, NULL, NULL, path, NULL, NULL, NULL);
        if (err < 0) {
            break;
        }
    }
    _plt_loading = 0;
    UNLOCK;
    return err;
}

static int
_plt_load_from_file (playlist_t *plt, ddb_file_handle_t *fp, playItem_t **last_added) {
    int result = -1;
    playItem_t *it = NULL;
    uint8_t majorver;
    uint8_t minorver;
    char magic[4];
    if (ddb_file_read (magic, 1, 4, fp) != 4) {
        //        trace ("failed to read magic\n");
        goto load_fail;
    }
    if (strncmp (magic, "DBPL", 4)) {
        //        trace ("bad signature\n");
        goto load_fail;
    }
    if (ddb_file_read (&majorver, 1, 1, fp) != 1) {
        goto load_fail;
    }
    if (majorver != PLAYLIST_MAJOR_VER) {
        //        trace ("bad majorver=%d\n", majorver);
        goto load_fail;
    }
    if (ddb_file_read (&minorver, 1, 1, fp) != 1) {
        goto load_fail;
    }
    if (minorver < 1) {
        //        trace ("bad minorver=%d\n", minorver);
        goto load_fail;
    }
    uint32_t cnt;
    if (ddb_file_read (&cnt, 1, 4, fp) != 4) {
        goto load_fail;
    }
    
    for (uint32_t i = 0; i < cnt; i++) {
        it = pl_item_alloc ();
        if (!it) {
            goto load_fail;
        }
        uint16_t l;
        int16_t tracknum = 0;
        if (minorver <= 2) {
            // fname
            if (ddb_file_read (&l, 1, 2, fp) != 2) {
                goto load_fail;
            }
            char uri[l + 1];
            if (ddb_file_read (uri, 1, l, fp) != l) {
                goto load_fail;
            }
            uri[l] = 0;
            pl_add_meta (it, ":URI", uri);
            // decoder
            uint8_t ll;
            if (ddb_file_read (&ll, 1, 1, fp) != 1) {
                goto load_fail;
            }
            if (ll >= 20) {
                goto load_fail;
            }
            char decoder_id[20] = "";
            if (ll) {
                if (ddb_file_read (decoder_id, 1, ll, fp) != ll) {
                    goto load_fail;
                }
                decoder_id[ll] = 0;
                pl_add_meta (it, ":DECODER", decoder_id);
            }
            // tracknum
            if (ddb_file_read (&tracknum, 1, 2, fp) != 2) {
                goto load_fail;
            }
            pl_set_meta_int (it, ":TRACKNUM", tracknum);
        }
        // startsample
        if (ddb_file_read (&it->startsample, 1, 4, fp) != 4) {
            goto load_fail;
        }
        // endsample
        if (ddb_file_read (&it->endsample, 1, 4, fp) != 4) {
            goto load_fail;
        }
        // duration
        if (ddb_file_read (&it->_duration, 1, 4, fp) != 4) {
            goto load_fail;
        }
        char s[100];
        pl_format_time (it->_duration, s, sizeof (s));
        pl_replace_meta (it, ":DURATION", s);

        if (minorver <= 2) {
            // legacy filetype support
            uint8_t ft;
            if (ddb_file_read (&ft, 1, 1, fp) != 1) {
                goto load_fail;
            }
            if (ft) {
                char ftype[ft + 1];
                if (ddb_file_read (ftype, 1, ft, fp) != ft) {
                    goto load_fail;
                }
                ftype[ft] = 0;
                pl_replace_meta (it, ":FILETYPE", ftype);
            }
            
            float f;
            
            if (ddb_file_read (&f, 1, 4, fp) != 4) {
                goto load_fail;
            }
            if (f != 0) {
                pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, f);
            }
            
            if (ddb_file_read (&f, 1, 4, fp) != 4) {
                goto load_fail;
            }
            if (f == 0) {
                f = 1;
            }
            if (f != 1) {
                pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, f);
            }
            
            if (ddb_file_read (&f, 1, 4, fp) != 4) {
                goto load_fail;
            }
            if (f != 0) {
                pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, f);
            }
            
            if (ddb_file_read (&f, 1, 4, fp) != 4) {
                goto load_fail;
            }
            if (f == 0) {
                f = 1;
            }
            if (f != 1) {
                pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, f);
            }
        }
        
        uint32_t flg = 0;
        if (minorver >= 2) {
            if (ddb_file_read (&flg, 1, 4, fp) != 4) {
                goto load_fail;
            }
        }
        else {
            if (pl_item_get_startsample (it) > 0 || pl_item_get_endsample (it) > 0 || tracknum > 0) {
                flg |= DDB_IS_SUBTRACK;
            }
        }
        pl_set_item_flags (it, flg);
        
        int16_t nm = 0;
        if (ddb_file_read (&nm, 1, 2, fp) != 2) {
            goto load_fail;
        }
        for (int j = 0; j < nm; j++) {
            if (ddb_file_read (&l, 1, 2, fp) != 2) {
                goto load_fail;
            }
            if (l >= 20000) {
                goto load_fail;
            }
            char key[l + 1];
            if (ddb_file_read (key, 1, l, fp) != l) {
                goto load_fail;
            }
            key[l] = 0;
            if (ddb_file_read (&l, 1, 2, fp) != 2) {
                goto load_fail;
            }
            if (l >= 20000) {
                // skip
                ddb_file_seek (fp, l, SEEK_CUR);
            }
            else {
                char value[l + 1];
                int res = (int)ddb_file_read (value, 1, l, fp);
                if (res != l) {
                    //                    trace ("playlist read error: requested %d, got %d\n", l, res);
                    goto load_fail;
                }
                value[l] = 0;
                if (key[0] == ':') {
                    if (!strcmp (key, ":STARTSAMPLE")) {
                        pl_item_set_startsample (it, atoll (value));
                    }
                    else if (!strcmp (key, ":ENDSAMPLE")) {
                        pl_item_set_endsample (it, atoll (value));
                    }
                    else {
                        // some values are stored twice:
                        // once in legacy format, and once in metadata format
                        // here, we delete what was set from legacy, and overwrite with metadata
                        pl_replace_meta (it, key, value);
                    }
                }
                else {
                    pl_add_meta_full (it, key, value, l + 1);
                }
            }
        }
        plt_insert_item (plt, plt->tail[PL_MAIN], it);
        if (*last_added) {
            pl_item_unref (*last_added);
        }
        *last_added = it;
        it = NULL;
    }
    
    // load playlist metadata
    int16_t nm = 0;
    // for backwards format compatibility, don't fail if metadata is not found
    if (ddb_file_read (&nm, 1, 2, fp) == 2) {
        for (int i = 0; i < nm; i++) {
            int16_t l;
            if (ddb_file_read (&l, 1, 2, fp) != 2) {
                goto load_fail;
            }
            if (l < 0 || l >= 20000) {
                goto load_fail;
            }
            char key[l + 1];
            if (ddb_file_read (key, 1, l, fp) != l) {
                goto load_fail;
            }
            key[l] = 0;
            if (ddb_file_read (&l, 1, 2, fp) != 2) {
                goto load_fail;
            }
            if (l < 0 || l >= 20000) {
                // skip
                // FIXME: error check
                ddb_file_seek (fp, l, SEEK_CUR);
            }
            else {
                char value[l + 1];
                int res = (int)ddb_file_read (value, 1, l, fp);
                if (res != l) {
                    //                    trace ("playlist read error: requested %d, got %d\n", l, res);
                    goto load_fail;
                }
                value[l] = 0;
                // FIXME: multivalue support
                plt_add_meta (plt, key, value);
            }
        }
    }

    result = 0;
load_fail:
    if (it) {
        pl_item_unref (it);
        it = NULL;
    }
    return result;
}

static playItem_t *
plt_load_int (
    int visibility,
    playlist_t *plt,
    playItem_t *after,
    const char *fname,
    int *pabort,
    int (*cb) (playItem_t *it, void *data),
    void *user_data) {
    playItem_t *last_added = NULL;

    unsigned undo_enabled = plt->undo_enabled;
    plt->undo_enabled = 0;

#ifdef __MINGW32__
    if (!strncmp (fname, "file://", 7)) {
        fname += 7;
    }
    // replace backslashes with normal slashes
    char fname_conv[strlen (fname) + 1];
    if (strchr (fname, '\\')) {
        trace ("plt_load_int: backslash(es) detected: %s\n", fname);
        strcpy (fname_conv, fname);
        char *slash_p = fname_conv;
        while (slash_p = strchr (slash_p, '\\')) {
            *slash_p = '/';
            slash_p++;
        }
        fname = fname_conv;
    }
    // path should start with "X:/", not "/X:/", fixing to avoid file opening problems
    if (fname[0] == '/' && isalpha (fname[1]) && fname[2] == ':') {
        fname++;
    }
#endif

    // try plugins 1st
    char *escaped = uri_unescape (fname, (int)strlen (fname));
    if (escaped) {
        fname = strdupa (escaped);
        free (escaped);
    }

    const char *ext = strrchr (fname, '.');
    if (ext) {
        ext++;
        DB_playlist_t **plug = plug_get_playlist_list ();
        int p, e;
        for (p = 0; plug[p]; p++) {
            for (e = 0; plug[p]->extensions[e]; e++) {
                if (plug[p]->load && !strcasecmp (ext, plug[p]->extensions[e])) {
                    DB_playItem_t *loaded_it = NULL;
                    if (cb || (plug[p]->load && !plug[p]->load2)) {
                        loaded_it = plug[p]->load (
                            (ddb_playlist_t *)plt,
                            (DB_playItem_t *)after,
                            fname,
                            pabort,
                            (int (*) (DB_playItem_t *, void *))cb,
                            user_data);
                    }
                    else if (plug[p]->plugin.api_vminor >= 5 && plug[p]->load2) {
                        loaded_it =
                            plug[p]->load2 (visibility, (ddb_playlist_t *)plt, (DB_playItem_t *)after, fname, pabort);
                    }
                    plt->undo_enabled = undo_enabled;
                    return (playItem_t *)loaded_it;
                }
            }
        }
    }
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        //        trace ("plt_load: failed to open %s\n", fname);
        plt->undo_enabled = undo_enabled;
        return NULL;
    }

    ddb_file_handle_t fh;
    ddb_file_init_stdio(&fh, fp);

    if (0 != _plt_load_from_file(plt, &fh, &last_added)) {
        goto load_fail;
    }

    if (fp) {
        fclose (fp);
    }
    if (last_added) {
        pl_item_unref (last_added);
    }
    plt->undo_enabled = undo_enabled;
    return last_added;
load_fail:
    //    trace ("playlist load fail (%s)!\n", fname);
    if (fp) {
        fclose (fp);
    }
    if (last_added) {
        pl_item_unref (last_added);
    }
    plt->undo_enabled = undo_enabled;
    return last_added;
}

int
plt_load_from_buffer (playlist_t *plt, const uint8_t *buffer, size_t size) {
    ddb_file_handle_t fh;
    ddb_file_init_buffer(&fh, buffer, size);

    playItem_t *last_added = NULL;
    int res = _plt_load_from_file(plt, &fh, &last_added);
    if (last_added) {
        pl_item_unref (last_added);
    }

    ddb_file_deinit (&fh);

    return res;
}

playItem_t *
plt_load (
    playlist_t *plt,
    playItem_t *after,
    const char *fname,
    int *pabort,
    int (*cb) (playItem_t *it, void *data),
    void *user_data) {
    return plt_load_int (0, plt, after, fname, pabort, cb, user_data);
}

int
pl_load_all (void) {
    int i = 0;
    int err = 0;
    char path[1024];
    DB_conf_item_t *it = conf_find ("playlist.tab.", NULL);
    if (!it) {
        // legacy (0.3.3 and earlier)
        char defpl[1024]; // $HOME/.config/deadbeef/default.dbpl
        if (snprintf (defpl, sizeof (defpl), "%s/default.dbpl", dbconfdir) > sizeof (defpl)) {
            fprintf (stderr, "error: cannot make string with default playlist path\n");
            return -1;
        }
        if (plt_add (plt_get_count (), _ ("Default")) < 0) {
            return -1;
        }

        playlist_t *plt = plt_get_for_idx (0);
        /*playItem_t *it = */ plt_load (plt, NULL, defpl, NULL, NULL, NULL);
        plt_unref (plt);
        return 0;
    }
    LOCK;
    _plt_loading = 1;
    while (it) {
        if (!err) {
            if (plt_add (plt_get_count (), it->value) < 0) {
                return -1;
            }
            plt_set_curr_idx (plt_get_count () - 1);
        }
        err = 0;
        if (snprintf (path, sizeof (path), "%s/playlists/%d.dbpl", dbconfdir, i) > sizeof (path)) {
            fprintf (stderr, "error: failed to make path string for playlist filename\n");
            err = -1;
        }
        else {
            fprintf (stderr, "INFO: from file %s\n", path);

            playlist_t *plt = plt_get_curr ();
            /* playItem_t *trk = */ plt_load (plt, NULL, path, NULL, NULL, NULL);
            char conf[100];
            snprintf (conf, sizeof (conf), "playlist.cursor.%d", i);
            plt->current_row[PL_MAIN] = deadbeef->conf_get_int (conf, -1);
            snprintf (conf, sizeof (conf), "playlist.scroll.%d", i);
            plt->scroll = deadbeef->conf_get_int (conf, 0);
            plt->last_save_modification_idx = plt->modification_idx = 0;
            plt_unref (plt);

            if (!it) {
                fprintf (stderr, "WARNING: there were errors while loading playlist '%s' (%s)\n", it->value, path);
            }
        }
        it = conf_find ("playlist.tab.", it);
        i++;
    }
    plt_set_curr_idx (0);
    _plt_loading = 0;
    plt_gen_conf ();
    messagepump_push (DB_EV_PLAYLISTSWITCHED, 0, 0, 0);
    UNLOCK;
    return err;
}

void
pl_set_selected_in_playlist (playlist_t *playlist, playItem_t *it, int sel) {
    it->selected = sel;
    playlist->recalc_seltime = 1;
}

void
plt_select_all (playlist_t *playlist) {
    LOCK;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        pl_set_selected_in_playlist (playlist, it, 1);
    }
    UNLOCK;
}

void
pl_select_all (void) {
    LOCK;
    plt_select_all (_current_playlist);
    UNLOCK;
}

void
plt_reshuffle (playlist_t *playlist, playItem_t **ppmin, playItem_t **ppmax) {
    LOCK;
    playItem_t *pmin = NULL;
    playItem_t *pmax = NULL;
    playItem_t *prev = NULL;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        if (streamer_get_shuffle () == DDB_SHUFFLE_ALBUMS && prev && pl_items_from_same_album (it, prev)) {
            it->shufflerating = prev->shufflerating;
        }
        else {
            prev = it;
            it->shufflerating = rand ();
        }
        if (!pmin || it->shufflerating < pmin->shufflerating) {
            pmin = it;
        }
        if (!pmax || it->shufflerating > pmax->shufflerating) {
            pmax = it;
        }
        it->played = 0;
    }
    if (ppmin) {
        *ppmin = pmin;
    }
    if (ppmax) {
        *ppmax = pmax;
    }
    UNLOCK;
}

void
plt_set_item_duration (playlist_t *playlist, playItem_t *it, float duration) {
    LOCK;
    if (it->in_playlist && playlist) {
        if (it->_duration > 0) {
            playlist->totaltime -= it->_duration;
        }
        if (duration > 0) {
            playlist->totaltime += duration;
        }
        if (playlist->totaltime < 0) {
            playlist->totaltime = 0;
        }
    }
    it->_duration = duration;
    char s[100];
    pl_format_time (it->_duration, s, sizeof (s));
    pl_replace_meta (it, ":DURATION", s);
    UNLOCK;
}

float
pl_get_item_duration (playItem_t *it) {
    LOCK;
    float res = it->_duration;
    UNLOCK;
    return res;
}

void
pl_set_item_replaygain (playItem_t *it, int idx, float value) {
    char s[100];
    switch (idx) {
    case DDB_REPLAYGAIN_ALBUMGAIN:
    case DDB_REPLAYGAIN_TRACKGAIN:
        snprintf (s, sizeof (s), "%0.2f dB", value);
        break;
    case DDB_REPLAYGAIN_ALBUMPEAK:
    case DDB_REPLAYGAIN_TRACKPEAK:
        snprintf (s, sizeof (s), "%0.6f", value);
        break;
    default:
        return;
    }
    pl_replace_meta (it, ddb_internal_rg_keys[idx], s);
}

float
pl_get_item_replaygain (playItem_t *it, int idx) {
    if (idx < 0 || idx > DDB_REPLAYGAIN_TRACKPEAK) {
        return 0;
    }

    switch (idx) {
    case DDB_REPLAYGAIN_ALBUMGAIN:
    case DDB_REPLAYGAIN_TRACKGAIN:
        return pl_find_meta_float (it, ddb_internal_rg_keys[idx], 0);
    case DDB_REPLAYGAIN_ALBUMPEAK:
    case DDB_REPLAYGAIN_TRACKPEAK:
        return pl_find_meta_float (it, ddb_internal_rg_keys[idx], 1);
    }
    return 0;
}

int
pl_format_item_queue (playItem_t *it, char *s, int size) {
    LOCK;
    *s = 0;
    int initsize = size;
    const char *val = pl_find_meta_raw (it, "_playing");
    while (val && *val) {
        while (*val && *val != '=') {
            val++;
        }
        if (*val == '=') {
            // found value
            val++;
            if (!(*val)) {
                break;
            }
            const char *e = NULL;
            if (*val == '"') {
                val++;
                e = val;
                while (*e && *e != '"') {
                    e++;
                }
            }
            else {
                e = val;
                while (*e && *e != ' ') {
                    e++;
                }
            }
            int n = (int)(e - val);
            if (n > size - 1) {
                n = size - 1;
            }
            strncpy (s, val, n);
            s += n;
            *s++ = ' ';
            *s = 0;
            size -= n + 1;
            val = e;
            if (*val) {
                val++;
            }
            while (*val && *val == ' ') {
                val++;
            }
        }
    }

    int pq_cnt = playqueue_getcount ();

    if (!pq_cnt) {
        UNLOCK;
        return 0;
    }

    int qinitsize = size;
    int init = 1;
    int len;
    for (int i = 0; i < pq_cnt; i++) {
        if (size <= 0) {
            break;
        }

        playItem_t *pqitem = playqueue_get_item (i);
        if (pqitem == it) {
            if (init) {
                init = 0;
                s[0] = '(';
                s++;
                size--;
                len = snprintf (s, size, "%d", i + 1);
            }
            else {
                len = snprintf (s, size, ",%d", i + 1);
            }
            s += len;
            size -= len;
        }
        pl_item_unref (pqitem);
    }
    if (size != qinitsize && size > 0) {
        len = snprintf (s, size, ")");
        s += len;
        size -= len;
    }
    UNLOCK;
    return initsize - size;
}

void
pl_format_time (float t, char *dur, int size) {
    if (t >= 0) {
        t = roundf (t);
        int hourdur = (int)floor (t / (60 * 60));
        int mindur = (int)floor ((t - hourdur * 60 * 60) / 60);
        int secdur = (int)floor (t - hourdur * 60 * 60 - mindur * 60);

        if (hourdur) {
            snprintf (dur, size, "%d:%02d:%02d", hourdur, mindur, secdur);
        }
        else {
            snprintf (dur, size, "%d:%02d", mindur, secdur);
        }
    }
    else {
        strcpy (dur, "∞");
    }
}

const char *
pl_format_duration (playItem_t *it, const char *ret, char *dur, int size) {
    if (ret) {
        return ret;
    }
    pl_format_time (pl_get_item_duration (it), dur, size);
    return dur;
}

static const char *
pl_format_elapsed (const char *ret, char *elapsed, int size) {
    if (ret) {
        return ret;
    }
    float playpos = streamer_get_playpos ();
    pl_format_time (playpos, elapsed, size);
    return elapsed;
}

// this function allows to escape special chars substituted for conversions
// @escape_chars: list of escapable characters terminated with 0, or NULL if none
static int
pl_format_title_int (const char *escape_chars, playItem_t *it, int idx, char *s, int size, int id, const char *fmt) {
    char tmp[50];
    char tags[200];
    char dirname[PATH_MAX];
    const char *duration = NULL;
    const char *elapsed = NULL;
    int escape_slash = 0;

    char *ss = s;

    LOCK;
    if (id != -1 && it) {
        const char *text = NULL;
        switch (id) {
        case DB_COLUMN_FILENUMBER:
            if (idx == -1) {
                idx = pl_get_idx_of (it);
            }
            snprintf (tmp, sizeof (tmp), "%d", idx + 1);
            text = tmp;
            break;
        case DB_COLUMN_PLAYING:
            UNLOCK;
            return pl_format_item_queue (it, s, size);
        }
        if (text) {
            strncpy (s, text, size);
            UNLOCK;
            for (ss = s; *ss; ss++) {
                if (*ss == '\n') {
                    *ss = ';';
                }
            }
            return (int)strlen (s);
        }
        else {
            s[0] = 0;
        }
        UNLOCK;
        return 0;
    }
    int n = size - 1;
    while (*fmt && n > 0) {
        if (*fmt != '%') {
            *s++ = *fmt;
            n--;
        }
        else {
            fmt++;
            const char *meta = NULL;
            if (*fmt == 0) {
                break;
            }
            else if (!it && *fmt != 'V') {
                // only %V (version) works without track pointer
            }
            else if (*fmt == '@') {
                const char *e = fmt;
                e++;
                while (*e && *e != '@') {
                    e++;
                }
                if (*e == '@') {
                    char nm[100];
                    int l = (int)(e - fmt - 1);
                    l = min (l, sizeof (nm) - 1);
                    strncpy (nm, fmt + 1, l);
                    nm[l] = 0;
                    meta = pl_find_meta_raw (it, nm);
                    if (!meta) {
                        meta = "";
                    }
                    fmt = e;
                }
            }
            else if (*fmt == '/') {
                // this means all '/' in the ongoing fields must be replaced with '\'
                escape_slash = 1;
            }
            else if (*fmt == 'a') {
                meta = pl_find_meta_raw (it, "artist");
#ifndef DISABLE_CUSTOM_TITLE
                const char *custom = pl_find_meta_raw (it, ":CUSTOM_TITLE");
#endif
                if (!meta
#ifndef DISABLE_CUSTOM_TITLE
                    && !custom
#endif
                ) {
                    meta = "Unknown artist";
                }

#ifndef DISABLE_CUSTOM_TITLE
                if (custom) {
                    if (!meta) {
                        meta = custom;
                    }
                    else {
                        int l = strlen (custom) + strlen (meta) + 4;
                        char *out = alloca (l);
                        snprintf (out, l, "[%s] %s", custom, meta);
                        meta = out;
                    }
                }
#endif
            }
            else if (*fmt == 't') {
                meta = pl_find_meta_raw (it, "title");
                if (!meta) {
                    const char *f = pl_find_meta_raw (it, ":URI");
                    if (f) {
                        const char *start = strrchr (f, '/');
                        if (start) {
                            start++;
                        }
                        else {
                            start = f;
                        }
                        const char *end = strrchr (start, '.');
                        if (end) {
                            int m = (int)(end - start);
                            m = min (m, sizeof (dirname) - 1);
                            strncpy (dirname, start, m);
                            dirname[m] = 0;
                            meta = dirname;
                        }
                        else {
                            meta = "";
                        }
                    }
                    else {
                        meta = "";
                    }
                }
            }
            else if (*fmt == 'b') {
                meta = pl_find_meta_raw (it, "album");
                if (!meta) {
                    meta = "Unknown album";
                }
            }
            else if (*fmt == 'B') {
                meta = pl_find_meta_raw (it, "band");
                if (!meta) {
                    meta = pl_find_meta_raw (it, "album artist");
                    if (!meta) {
                        meta = pl_find_meta_raw (it, "albumartist");
                        if (!meta) {
                            meta = pl_find_meta_raw (it, "artist");
                        }
                    }
                }

#ifndef DISABLE_CUSTOM_TITLE
                const char *custom = pl_find_meta_raw (it, ":CUSTOM_TITLE");
                if (custom) {
                    if (!meta) {
                        meta = custom;
                    }
                    else {
                        int l = strlen (custom) + strlen (meta) + 4;
                        char *out = alloca (l);
                        snprintf (out, l, "[%s] %s", custom, meta);
                        meta = out;
                    }
                }
#endif
            }
            else if (*fmt == 'C') {
                meta = pl_find_meta_raw (it, "composer");
            }
            else if (*fmt == 'n') {
                meta = pl_find_meta_raw (it, "track");
                if (meta) {
                    // check if it's numbers only
                    const char *p = meta;
                    while (*p) {
                        if (!isdigit (*p)) {
                            break;
                        }
                        p++;
                    }
                    if (!(*p)) {
                        snprintf (tmp, sizeof (tmp), "%02d", atoi (meta));
                        meta = tmp;
                    }
                }
            }
            else if (*fmt == 'N') {
                meta = pl_find_meta_raw (it, "numtracks");
            }
            else if (*fmt == 'y') {
                meta = pl_find_meta_raw (it, "year");
            }
            else if (*fmt == 'Y') {
                meta = pl_find_meta_raw (it, "original_release_time");
                if (!meta) {
                    meta = pl_find_meta_raw (it, "original_release_year");
                    if (!meta) {
                        meta = pl_find_meta_raw (it, "year");
                    }
                }
            }
            else if (*fmt == 'g') {
                meta = pl_find_meta_raw (it, "genre");
            }
            else if (*fmt == 'c') {
                meta = pl_find_meta_raw (it, "comment");
            }
            else if (*fmt == 'r') {
                meta = pl_find_meta_raw (it, "copyright");
            }
            else if (*fmt == 'l') {
                const char *value = (duration = pl_format_duration (it, duration, tmp, sizeof (tmp)));
                while (n > 0 && *value) {
                    *s++ = *value++;
                    n--;
                }
            }
            else if (*fmt == 'e') {
                // what a hack..
                const char *value = (elapsed = pl_format_elapsed (elapsed, tmp, sizeof (tmp)));
                while (n > 0 && *value) {
                    *s++ = *value++;
                    n--;
                }
            }
            else if (*fmt == 'f') {
                const char *f = pl_find_meta_raw (it, ":URI");
                meta = strrchr (f, '/');
                if (meta) {
                    meta++;
                }
                else {
                    meta = f;
                }
            }
            else if (*fmt == 'F') {
                meta = pl_find_meta_raw (it, ":URI");
#ifdef __MINGW32__
                int len = strlen (meta);
                strncpy (dirname, meta, len);
                dirname[len] = 0;
                // Convert to backslashes on windows
                char *str_p = dirname;
                while (str_p = strchr (str_p, '/')) {
                    *str_p = '\\';
                }
                meta = dirname;
#endif
            }
            else if (*fmt == 'T') {
                char *t = tags;
                char *e = tags + sizeof (tags);
                int c;
                *t = 0;

                if (it->_flags & DDB_TAG_ID3V1) {
                    c = snprintf (t, e - t, "ID3v1 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_ID3V22) {
                    c = snprintf (t, e - t, "ID3v2.2 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_ID3V23) {
                    c = snprintf (t, e - t, "ID3v2.3 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_ID3V24) {
                    c = snprintf (t, e - t, "ID3v2.4 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_APEV2) {
                    c = snprintf (t, e - t, "APEv2 | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_VORBISCOMMENTS) {
                    c = snprintf (t, e - t, "VorbisComments | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_CUESHEET) {
                    c = snprintf (t, e - t, "CueSheet | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_ICY) {
                    c = snprintf (t, e - t, "Icy | ");
                    t += c;
                }
                if (it->_flags & DDB_TAG_ITUNES) {
                    c = snprintf (t, e - t, "iTunes | ");
                    t += c;
                }
                if (t != tags) {
                    *(t - 3) = 0;
                }
                meta = tags;
            }
            else if (*fmt == 'd') {
                // directory
                const char *f = pl_find_meta_raw (it, ":URI");
                const char *end = strrchr (f, '/');
                if (!end) {
                    meta = ""; // got relative path without folder (should not happen)
                }
                else {
                    const char *start = end;
                    start--;
                    while (start > f && (*start != '/')) {
                        start--;
                    }

                    if (*start == '/') {
                        start++;
                    }

                    // copy
                    int len = (int)(end - start);
                    len = min (len, sizeof (dirname) - 1);
                    strncpy (dirname, start, len);
                    dirname[len] = 0;
                    meta = dirname;
                }
            }
            else if (*fmt == 'D') {
                const char *f = pl_find_meta_raw (it, ":URI");
                // directory with path
                const char *end = strrchr (f, '/');
                if (!end) {
                    meta = ""; // got relative path without folder (should not happen)
                }
                else {
                    // copy
                    int len = (int)(end - f);
                    len = min (len, sizeof (dirname) - 1);
                    strncpy (dirname, f, len);
                    dirname[len] = 0;
#ifdef __MINGW32__
                    {
                        // Convert to backslashes on windows
                        char *str_p = dirname;
                        while (str_p = strchr (str_p, '/')) {
                            *str_p = '\\';
                        }
                    }
#endif
                    meta = dirname;
                }
            }
            else if (*fmt == 'L') {
                float l = 0;
                for (playItem_t *trk = _current_playlist->head[PL_MAIN]; trk; trk = trk->next[PL_MAIN]) {
                    if (trk->selected) {
                        l += trk->_duration;
                    }
                }
                pl_format_time (l, tmp, sizeof (tmp));
                meta = tmp;
            }
            else if (*fmt == 'X') {
                int m = 0;
                for (playItem_t *trk = _current_playlist->head[PL_MAIN]; trk; trk = trk->next[PL_MAIN]) {
                    if (trk->selected) {
                        m++;
                    }
                }
                snprintf (tmp, sizeof (tmp), "%d", m);
                meta = tmp;
            }
            else if (*fmt == 'Z') {
                DB_fileinfo_t *c = deadbeef->streamer_get_current_fileinfo (); // FIXME: might crash streamer
                if (c) {
                    if (c->fmt.channels <= 2) {
                        meta = c->fmt.channels == 1 ? _ ("Mono") : _ ("Stereo");
                    }
                    else {
                        snprintf (tmp, sizeof (tmp), "%dch Multichannel", c->fmt.channels);
                        meta = tmp;
                    }
                }
            }
#ifdef VERSION
            else if (*fmt == 'V') {
                meta = VERSION;
            }
#endif
            else {
                *s++ = *fmt;
                n--;
            }

            if (meta) {
                const char *value = meta;
                if (escape_chars) {
                    // need space for at least 2 single-quotes
                    if (n < 2) {
                        goto error;
                    }
                    *s++ = '\'';
                    n--;
                    while (n > 2 && *value) {
                        if (strchr (escape_chars, *value)) {
                            *s++ = '\\';
                            n--;
                            *s++ = *value++;
                            n--;
                        }
                        else if (escape_slash && *value == '/') {
                            *s++ = '\\';
                            n--;
                            *s++ = '\\';
                            n--;
                            break;
                        }
                        else {
                            *s++ = *value++;
                        }
                    }
                    if (n < 1) {
                        fprintf (
                            stderr,
                            "pl_format_title_int: got unpredicted state while formatting escaped string. please report a bug.\n");
                        *ss = 0; // should never happen
                        return -1;
                    }
                    *s++ = '\'';
                    n--;
                }
                else {
                    while (n > 0 && *value) {
                        if (escape_slash && *value == '/') {
                            *s++ = '\\';
                            n--;
                            value++;
                        }
                        else {
                            *s++ = *value++;
                            n--;
                        }
                    }
                }
            }
        }
        fmt++;
    }
error:
    *s = 0;
    UNLOCK;

    // replace all \n with ;
    while (*ss) {
        if (*ss == '\n') {
            *ss = ';';
        }
        ss++;
    }

    return size - n - 1;
}

int
pl_format_title (playItem_t *it, int idx, char *s, int size, int id, const char *fmt) {
    return pl_format_title_int (NULL, it, idx, s, size, id, fmt);
}

int
pl_format_title_escaped (playItem_t *it, int idx, char *s, int size, int id, const char *fmt) {
    return pl_format_title_int ("'", it, idx, s, size, id, fmt);
}

void
plt_reset_cursor (playlist_t *playlist) {
    int i;
    LOCK;
    for (i = 0; i < PL_MAX_ITERATORS; i++) {
        playlist->current_row[i] = -1;
    }
    UNLOCK;
}

float
plt_get_totaltime (playlist_t *playlist) {
    if (!playlist) {
        return 0;
    }
    return playlist->totaltime;
}

float
pl_get_totaltime (void) {
    LOCK;
    float t = plt_get_totaltime (_current_playlist);
    UNLOCK;
    return t;
}

float
plt_get_selection_playback_time (playlist_t *playlist) {
    LOCK;

    float t = 0;

    if (!playlist->recalc_seltime) {
        t = playlist->seltime;
        UNLOCK;
        return roundf (t);
    }

    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        if (it->selected) {
            t += roundf (it->_duration);
        }
    }

    playlist->seltime = t;
    playlist->recalc_seltime = 0;

    UNLOCK;

    return t;
}

void
pl_set_selected (playItem_t *it, int sel) {
    LOCK;
    // NOTE: it's not really known here, which playlist the item belongs to, but we're assuming it's in the current playlist.
    // If the item is in another playlist -- this call will make selection playback time
    // to be recalculated for the current playlist, next time it's requested,
    // while the same value will be off in the playlist which the item belongs to.
    pl_set_selected_in_playlist (_current_playlist, it, sel);
    UNLOCK;
}

int
pl_is_selected (playItem_t *it) {
    pl_lock ();
    int res = it->selected;
    pl_unlock ();
    return res;
}

playItem_t *
plt_get_first (playlist_t *playlist, int iter) {
    if (!playlist) {
        return NULL;
    }
    playItem_t *p = playlist->head[iter];
    if (p) {
        pl_item_ref (p);
    }
    return p;
}

playItem_t *
pl_get_first (int iter) {
    LOCK;
    playItem_t *it = plt_get_first (_current_playlist, iter);
    UNLOCK;
    return it;
}

playItem_t *
plt_get_last (playlist_t *playlist, int iter) {
    playItem_t *p = playlist->tail[iter];
    if (p) {
        pl_item_ref (p);
    }
    return p;
}

playItem_t *
pl_get_last (int iter) {
    LOCK;
    playItem_t *it = plt_get_last (_current_playlist, iter);
    UNLOCK;
    return it;
}

playItem_t *
pl_get_next (playItem_t *it, int iter) {
    playItem_t *next = it ? it->next[iter] : NULL;
    if (next) {
        pl_item_ref (next);
    }
    return next;
}

playItem_t *
pl_get_prev (playItem_t *it, int iter) {
    playItem_t *prev = it ? it->prev[iter] : NULL;
    if (prev) {
        pl_item_ref (prev);
    }
    return prev;
}

int
plt_get_cursor (playlist_t *playlist, int iter) {
    if (!playlist) {
        return -1;
    }
    return playlist->current_row[iter];
}

int
pl_get_cursor (int iter) {
    LOCK;
    int c = plt_get_cursor (_current_playlist, iter);
    UNLOCK;
    return c;
}

void
plt_set_cursor (playlist_t *playlist, int iter, int cursor) {
    playlist->current_row[iter] = cursor;
}

void
pl_set_cursor (int iter, int cursor) {
    LOCK;
    plt_set_cursor (_current_playlist, iter, cursor);
    UNLOCK;
}

// this function must move items in playlist
// list of items is indexes[count]
// drop_before is insertion point
void
plt_move_items (playlist_t *to, int iter, playlist_t *from, playItem_t *drop_before, uint32_t *indexes, int count) {
    playItem_t *playing = streamer_get_playing_track ();

    LOCK;

    if (!from || !to) {
        if (playing) {
            pl_item_unref (playing);
        }
        UNLOCK;
        return;
    }

    // don't let streamer think that current song was removed
    no_remove_notify = 1;

    // unlink items from from, and link together
    int processed = 0;
    int idx = 0;
    playItem_t *next = NULL;

    // find insertion point
    playItem_t *drop_after = NULL;
    if (drop_before) {
        drop_after = drop_before->prev[iter];
    }
    else {
        drop_after = to->tail[iter];
    }

    for (playItem_t *it = from->head[iter]; it && processed < count; it = next, idx++) {
        next = it->next[iter];
        if (idx == indexes[processed]) {
            if (it == playing && to != from) {
                streamer_set_streamer_playlist (to);
            }
            pl_item_ref (it);
            if (drop_after == it) {
                drop_after = it->prev[PL_MAIN];
            }
            plt_remove_item (from, it);
            plt_insert_item (to, drop_after, it);
            pl_item_unref (it);
            drop_after = it;
            processed++;
        }
    }

    if (playing) {
        pl_item_unref (playing);
    }

    no_remove_notify = 0;
    UNLOCK;
}

void
plt_move_all_items (playlist_t *to, playlist_t *from, playItem_t *insert_after) {
    LOCK;
    playItem_t *it = from->head[PL_MAIN];
    if (it == NULL) {
        UNLOCK;
        return;
    }
    pl_item_ref(it);
    ddb_undobuffer_group_begin (ddb_undomanager_get_buffer (ddb_undomanager_shared ()));
    while (it != NULL) {
        playItem_t *next = it->next[PL_MAIN];
        if (next != NULL) {
            pl_item_ref (next);
        }

        plt_remove_item (from, it);
        plt_insert_item (to, insert_after, it);
        insert_after = it;
        pl_item_unref (it);
        it = next;
    }
    ddb_undobuffer_group_end (ddb_undomanager_get_buffer (ddb_undomanager_shared ()));
    UNLOCK;
}

void
plt_copy_items (playlist_t *to, int iter, playlist_t *from, playItem_t *before, uint32_t *indices, int cnt) {
    pl_lock ();

    if (!from || !to || cnt == 0) {
        pl_unlock ();
        return;
    }

    playItem_t **items = malloc (cnt * sizeof (playItem_t *));
    for (int i = 0; i < cnt; i++) {
        playItem_t *it = from->head[iter];
        for (int idx = 0; it && idx < indices[i]; idx++) {
            it = it->next[iter];
        }
        items[i] = it;
        if (!it) {
            trace ("plt_copy_items: warning: item %d not found in source plt_to\n", indices[i]);
        }
    }
    playItem_t *after = before ? before->prev[iter] : to->tail[iter];
    for (int i = 0; i < cnt; i++) {
        if (items[i]) {
            playItem_t *new_it = pl_item_alloc ();
            pl_item_copy (new_it, items[i]);
            pl_insert_item (after, new_it);
            pl_item_unref (new_it);
            after = new_it;
        }
    }
    free (items);

    pl_unlock ();
}

static void
plt_search_reset_int (playlist_t *playlist, int clear_selection) {
    LOCK;
    while (playlist->head[PL_SEARCH]) {
        playItem_t *next = playlist->head[PL_SEARCH]->next[PL_SEARCH];
        if (clear_selection) {
            pl_set_selected_in_playlist (playlist, playlist->head[PL_SEARCH], 0);
        }
        playlist->head[PL_SEARCH]->next[PL_SEARCH] = NULL;
        playlist->head[PL_SEARCH]->prev[PL_SEARCH] = NULL;
        playlist->head[PL_SEARCH] = next;
    }
    playlist->tail[PL_SEARCH] = NULL;
    playlist->count[PL_SEARCH] = 0;
    UNLOCK;
}

void
plt_search_reset (playlist_t *playlist) {
    plt_search_reset_int (playlist, 1);
}

static void
_plsearch_append (playlist_t *plt, playItem_t *it, int select_results) {
    it->next[PL_SEARCH] = NULL;
    it->prev[PL_SEARCH] = plt->tail[PL_SEARCH];
    if (plt->tail[PL_SEARCH]) {
        plt->tail[PL_SEARCH]->next[PL_SEARCH] = it;
        plt->tail[PL_SEARCH] = it;
    }
    else {
        plt->head[PL_SEARCH] = plt->tail[PL_SEARCH] = it;
    }
    if (select_results) {
        pl_set_selected_in_playlist (plt, it, 1);
    }
    plt->count[PL_SEARCH]++;
}

void
plt_search_process2 (playlist_t *playlist, const char *text, int select_results) {
    LOCK;
    plt_search_reset_int (playlist, select_results);

    // convert text to lowercase, to save some cycles
    char lc[1000];
    int n = sizeof (lc) - 1;
    const char *p = text;
    char *out = lc;
    while (*p) {
        int32_t i = 0;
        char s[10];
        u8_nextchar (p, &i);
        int l = u8_tolower ((const int8_t *)p, i, s);
        n -= l;
        if (n < 0) {
            break;
        }
        memcpy (out, s, l);
        p += i;
        out += l;
    }
    *out = 0;

    int lc_is_valid_u8 = u8_valid (lc, (int)strlen (lc), NULL);

    playlist->search_cmpidx++;
    if (playlist->search_cmpidx > 127) {
        playlist->search_cmpidx = 1;
    }

    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        if (select_results) {
            pl_set_selected_in_playlist (playlist, it, 0);
        }
        if (*text) {
            DB_metaInfo_t *m = NULL;
            for (m = it->meta; m; m = m->next) {
                int is_uri = !strcmp (m->key, ":URI");
                if ((m->key[0] == ':' && !is_uri) || m->key[0] == '_' || m->key[0] == '!') {
                    break;
                }
                if (!strcasecmp (m->key, "cuesheet") || !strcasecmp (m->key, "log")) {
                    continue;
                }

                const char *value = m->value;
                const char *end = value + m->valuesize;

                if (is_uri) {
                    value = strrchr (value, '/');
                    if (value) {
                        value++;
                    }
                    else {
                        value = m->value;
                    }
                }

                char cmp = *(m->value - 1);

                if (abs (cmp) == playlist->search_cmpidx) { // string was already compared in this search
                    if (cmp > 0) { // it's a match -- append to search results
                        _plsearch_append (playlist, it, select_results);
                        break;
                    }
                }
                else {
                    int match = -playlist->search_cmpidx; // assume no match
                    do {
                        int len = (int)strlen (value);
                        if (lc_is_valid_u8 && u8_valid (value, len, NULL) && utfcasestr_fast (value, lc)) {
                            _plsearch_append (playlist, it, select_results);
                            match = playlist->search_cmpidx; // it's a match
                            break;
                        }
                        value += len + 1;
                    } while (value < end);
                    *((char *)m->value - 1) = (int8_t)match;
                    if (match > 0) {
                        break;
                    }
                }
            }
        }
    }
    UNLOCK;
}

void
plt_search_process (playlist_t *playlist, const char *text) {
    plt_search_process2 (playlist, text, 1);
}

void
send_trackinfochanged (playItem_t *track) {
    ddb_event_track_t *ev = (ddb_event_track_t *)messagepump_event_alloc (DB_EV_TRACKINFOCHANGED);
    ev->track = DB_PLAYITEM (track);
    if (track) {
        pl_item_ref (track);
    }

    messagepump_push_event ((ddb_event_t *)ev, 0, 0);
}

void
pl_items_copy_junk (playItem_t *from, playItem_t *first, playItem_t *last) {
    LOCK;
    DB_metaInfo_t *meta = from->meta;
    while (meta) {
        playItem_t *i;
        for (i = first; i; i = i->next[PL_MAIN]) {
            i->_flags = from->_flags;
            pl_add_meta_copy (i, meta);
            if (i == last) {
                break;
            }
        }
        meta = meta->next;
    }
    UNLOCK;
}

uint32_t
pl_get_item_flags (playItem_t *it) {
    LOCK;
    uint32_t flags = it->_flags;
    UNLOCK;
    return flags;
}

void
pl_set_item_flags (playItem_t *it, uint32_t flags) {
    LOCK;
    it->_flags = flags;

    char s[200];
    pl_format_title (it, -1, s, sizeof (s), -1, "%T");
    pl_replace_meta (it, ":TAGS", s);
    pl_replace_meta (it, ":HAS_EMBEDDED_CUESHEET", (flags & DDB_HAS_EMBEDDED_CUESHEET) ? _ ("Yes") : _ ("No"));
    UNLOCK;
}

playlist_t *
pl_get_playlist (playItem_t *it) {
    LOCK;
    playlist_t *p = _playlists_head;
    while (p) {
        int idx = plt_get_item_idx (p, it, PL_MAIN);
        if (idx != -1) {
            plt_ref (p);
            UNLOCK;
            return p;
        }
        p = p->next;
    }
    UNLOCK;
    return NULL;
}

void
plt_set_fast_mode (playlist_t *plt, int fast) {
    plt->fast_mode = (unsigned)fast;
}

int
plt_is_fast_mode (playlist_t *plt) {
    return plt->fast_mode;
}

void
pl_ensure_lock (void) {
#if DETECT_PL_LOCK_RC
    pthread_t tid = pthread_self ();
    for (int i = 0; i < ntids; i++) {
        if (tids[i] == tid) {
            return;
        }
    }
    fprintf (
        stderr,
        "\033[0;31mnon-thread-safe playlist access function was called outside of pl_lock. please make a backtrace and post a bug. thank you.\033[37;0m\n");
    assert (0);
#endif
}

int
plt_get_idx (playlist_t *plt) {
    int i;
    playlist_t *p;
    for (i = 0, p = _playlists_head; p && p != plt; i++, p = p->next)
        ;
    if (p == 0) {
        return -1;
    }
    return i;
}

int
plt_save_config (playlist_t *plt) {
    int i = plt_get_idx (plt);
    if (i == -1) {
        return -1;
    }
    return plt_save_n (i);
}

int
listen_file_added (int (*callback) (ddb_fileadd_data_t *data, void *user_data), void *user_data) {
    ddb_fileadd_listener_t *l;

    int id = 1;
    for (l = file_add_listeners; l; l = l->next) {
        if (l->id > id) {
            id = l->id + 1;
        }
    }

    l = malloc (sizeof (ddb_fileadd_listener_t));
    memset (l, 0, sizeof (ddb_fileadd_listener_t));
    l->id = id;
    l->callback = callback;
    l->user_data = user_data;
    l->next = file_add_listeners;
    file_add_listeners = l;
    return id;
}

void
unlisten_file_added (int id) {
    ddb_fileadd_listener_t *prev = NULL;
    for (ddb_fileadd_listener_t *l = file_add_listeners; l; prev = l, l = l->next) {
        if (l->id == id) {
            if (prev) {
                prev->next = l->next;
            }
            else {
                file_add_listeners = l->next;
            }
            free (l);
            break;
        }
    }
}

int
listen_file_add_beginend (
    void (*callback_begin) (ddb_fileadd_data_t *data, void *user_data),
    void (*callback_end) (ddb_fileadd_data_t *data, void *user_data),
    void *user_data) {
    ddb_fileadd_beginend_listener_t *l;

    int id = 1;
    for (l = file_add_beginend_listeners; l; l = l->next) {
        if (l->id > id) {
            id = l->id + 1;
        }
    }

    l = malloc (sizeof (ddb_fileadd_beginend_listener_t));
    memset (l, 0, sizeof (ddb_fileadd_beginend_listener_t));
    l->id = id;
    l->callback_begin = callback_begin;
    l->callback_end = callback_end;
    l->user_data = user_data;
    l->next = file_add_beginend_listeners;
    file_add_beginend_listeners = l;
    return id;
}

void
unlisten_file_add_beginend (int id) {
    ddb_fileadd_beginend_listener_t *prev = NULL;
    for (ddb_fileadd_beginend_listener_t *l = file_add_beginend_listeners; l; prev = l, l = l->next) {
        if (l->id == id) {
            if (prev) {
                prev->next = l->next;
            }
            else {
                file_add_beginend_listeners = l->next;
            }
            free (l);
            break;
        }
    }
}

int
register_fileadd_filter (int (*callback) (ddb_file_found_data_t *data, void *user_data), void *user_data) {
    ddb_fileadd_filter_t *l;

    int id = 1;
    for (l = file_add_filters; l; l = l->next) {
        if (l->id > id) {
            id = l->id + 1;
        }
    }

    l = malloc (sizeof (ddb_fileadd_filter_t));
    memset (l, 0, sizeof (ddb_fileadd_filter_t));
    l->id = id;
    l->callback = callback;
    l->user_data = user_data;
    l->next = file_add_filters;
    file_add_filters = l;
    return id;
}

void
unregister_fileadd_filter (int id) {
    ddb_fileadd_filter_t *prev = NULL;
    for (ddb_fileadd_filter_t *l = file_add_filters; l; prev = l, l = l->next) {
        if (l->id == id) {
            if (prev) {
                prev->next = l->next;
            }
            else {
                file_add_filters = l->next;
            }
            free (l);
            break;
        }
    }
}

playItem_t *
plt_load2 (
    int visibility,
    playlist_t *plt,
    playItem_t *after,
    const char *fname,
    int *pabort,
    int (*callback) (playItem_t *it, void *user_data),
    void *user_data) {
    return plt_load_int (visibility, plt, after, fname, pabort, callback, user_data);
}

int
plt_add_file2 (
    int visibility,
    playlist_t *plt,
    const char *fname,
    int (*callback) (playItem_t *it, void *user_data),
    void *user_data) {
    int res = plt_add_file_int (visibility, _get_insert_file_flags_from_config (), plt, fname, callback, user_data);
    return res;
}

playItem_t *
pl_item_init (const char *fname) {
    playlist_t plt;
    memset (&plt, 0, sizeof (plt));
    return plt_insert_file_int (-1, _get_insert_file_flags_from_config (), &plt, NULL, fname, NULL, NULL, NULL, NULL);
}

int
plt_add_dir2 (
    int visibility,
    playlist_t *plt,
    const char *dirname,
    int (*callback) (playItem_t *it, void *user_data),
    void *user_data) {
    int prev_sl = plt->follow_symlinks;
    plt->follow_symlinks = conf_get_int ("add_folders_follow_symlinks", 0);

    int prev = plt->ignore_archives;
    plt->ignore_archives = conf_get_int ("ignore_archives", 1);

    int abort = 0;
    playItem_t *it = plt_insert_dir_int (
        visibility,
        _get_insert_file_flags_from_config (),
        plt,
        NULL,
        plt->tail[PL_MAIN],
        dirname,
        &abort,
        callback,
        NULL,
        user_data);

    plt->ignore_archives = prev;
    plt->follow_symlinks = prev_sl;
    if (it) {
        // pl_insert_file doesn't hold reference, don't unref here
        return 0;
    }
    return -1;
}

playItem_t *
plt_insert_file2 (
    int visibility,
    playlist_t *playlist,
    playItem_t *after,
    const char *fname,
    int *pabort,
    int (*callback) (playItem_t *it, void *user_data),
    void *user_data) {
    return plt_insert_file_int (
        visibility,
        _get_insert_file_flags_from_config (),
        playlist,
        after,
        fname,
        pabort,
        callback,
        NULL,
        user_data);
}

playItem_t *
plt_insert_dir2 (
    int visibility,
    playlist_t *plt,
    playItem_t *after,
    const char *dirname,
    int *pabort,
    int (*callback) (playItem_t *it, void *user_data),
    void *user_data) {
    int prev_sl = plt->follow_symlinks;
    plt->follow_symlinks = conf_get_int ("add_folders_follow_symlinks", 0);
    plt->ignore_archives = conf_get_int ("ignore_archives", 1);

    playItem_t *ret = plt_insert_dir_int (
        visibility,
        _get_insert_file_flags_from_config (),
        plt,
        NULL,
        after,
        dirname,
        pabort,
        callback,
        NULL,
        user_data);

    plt->follow_symlinks = prev_sl;
    plt->ignore_archives = 0;
    return ret;
}

playItem_t *
plt_insert_dir3 (
    int visibility,
    uint32_t flags,
    playlist_t *plt,
    playItem_t *after,
    const char *dirname,
    int *pabort,
    int (*callback) (ddb_insert_file_result_t result, const char *fname, void *user_data),
    void *user_data) {
    return plt_insert_dir_int (visibility, flags, plt, NULL, after, dirname, pabort, NULL, callback, user_data);
}

int
plt_add_files_begin (playlist_t *plt, int visibility) {
    pl_lock ();
    if (addfiles_playlist) {
        pl_unlock ();
        return -1;
    }
    if (plt->files_adding) {
        pl_unlock ();
        return -1;
    }
    addfiles_playlist = plt;
    plt_ref (addfiles_playlist);
    plt->files_adding = 1;
    plt->files_add_visibility = visibility;
    pl_unlock ();
    ddb_fileadd_data_t d;
    memset (&d, 0, sizeof (d));
    d.visibility = visibility;
    d.plt = (ddb_playlist_t *)plt;
    for (ddb_fileadd_beginend_listener_t *l = file_add_beginend_listeners; l; l = l->next) {
        l->callback_begin (&d, l->user_data);
    }
    background_job_increment ();
    return 0;
}

void
plt_add_files_end (playlist_t *plt, int visibility) {
    pl_lock ();
    if (addfiles_playlist) {
        plt_unref (addfiles_playlist);
    }
    addfiles_playlist = NULL;
    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    plt->files_adding = 0;
    pl_unlock ();
    ddb_fileadd_data_t d;
    memset (&d, 0, sizeof (d));
    d.visibility = visibility;
    d.plt = (ddb_playlist_t *)plt;
    for (ddb_fileadd_beginend_listener_t *l = file_add_beginend_listeners; l; l = l->next) {
        l->callback_end (&d, l->user_data);
    }
    background_job_decrement ();

    plt_autosort (plt);
}

void
plt_deselect_all (playlist_t *playlist) {
    LOCK;
    for (playItem_t *it = playlist->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        it->selected = 0;
    }
    playlist->seltime = 0;
    playlist->recalc_seltime = 0;
    UNLOCK;
}

void
plt_set_scroll (playlist_t *plt, int scroll) {
    plt->scroll = scroll;
}

int
plt_get_scroll (playlist_t *plt) {
    return plt->scroll;
}

static playItem_t *
plt_process_embedded_cue (playlist_t *plt, playItem_t *after, playItem_t *it, int64_t totalsamples, int samplerate) {
    if (plt->loading_cue) {
        // means it was called from _load_cue
        plt->cue_numsamples = totalsamples;
        plt->cue_samplerate = samplerate;
        return NULL;
    }
    playItem_t *cue = NULL;
    pl_lock ();
    const char *cuesheet = pl_find_meta (it, "cuesheet");
    if (cuesheet) {
        const char *fname = pl_find_meta (it, ":URI");
        cue = plt_load_cuesheet_from_buffer (
            plt,
            after,
            fname,
            it,
            totalsamples,
            samplerate,
            (const uint8_t *)cuesheet,
            (int)strlen (cuesheet),
            NULL,
            0,
            0);
    }
    pl_unlock ();
    return cue;
}

playItem_t * /* called by plugins */
plt_process_cue (playlist_t *plt, playItem_t *after, playItem_t *it, uint64_t totalsamples, int samplerate) {
    if (plt->loading_cue) {
        // means it was called from _load_cue
        plt->cue_numsamples = totalsamples;
        plt->cue_samplerate = samplerate;
        return NULL;
    }
    return plt_process_embedded_cue (plt, after, it, (int64_t)totalsamples, samplerate);
}

void
pl_configchanged (void) {
    conf_cue_prefer_embedded = conf_get_int ("cue.prefer_embedded", 0);
}

int64_t
pl_item_get_startsample (playItem_t *it) {
    int64_t res;
    pl_lock ();
    if (!it->has_startsample64) {
        res = it->startsample;
    }
    else {
        res = it->startsample64;
    }
    pl_unlock ();
    return res;
}

int64_t
pl_item_get_endsample (playItem_t *it) {
    int64_t res = 0;
    pl_lock ();
    if (!it->has_endsample64) {
        res = it->endsample;
    }
    else {
        res = it->endsample64;
    }
    pl_unlock ();
    return res;
}

void
pl_item_set_startsample (playItem_t *it, int64_t sample) {
    pl_lock ();
    it->startsample64 = sample;
    it->startsample = sample >= 0x7fffffff ? 0x7fffffff : (int32_t)sample;
    it->has_startsample64 = 1;
    pl_set_meta_int64 (it, ":STARTSAMPLE", sample);
    pl_unlock ();
}

void
pl_item_set_endsample (playItem_t *it, int64_t sample) {
    pl_lock ();
    it->endsample64 = sample;
    it->endsample = sample >= 0x7fffffff ? 0x7fffffff : (int32_t)sample;
    it->has_endsample64 = 1;
    pl_set_meta_int64 (it, ":ENDSAMPLE", sample);
    pl_unlock ();
}

int
plt_is_loading_cue (playlist_t *plt) {
    return plt->loading_cue;
}

int
pl_get_played (playItem_t *it) {
    pl_lock ();
    int ret = it->played;
    pl_unlock ();
    return ret;
}

void
pl_set_played (playItem_t *it, int played) {
    pl_lock ();
    it->played = played;
    pl_unlock ();
}

int
pl_get_shufflerating (playItem_t *it) {
    pl_lock ();
    int ret = it->shufflerating;
    pl_unlock ();
    return ret;
}

void
pl_set_shufflerating (playItem_t *it, int rating) {
    pl_lock ();
    it->shufflerating = rating;
    pl_unlock ();
}

int
pl_items_from_same_album (playItem_t *a, playItem_t *b) {
    const char *keys[] = { "band", "album artist", "albumartist", "artist", NULL };

    const char *a_artist = NULL;
    const char *b_artist = NULL;
    for (int i = 0; keys[i]; i++) {
        if (!a_artist) {
            a_artist = pl_find_meta_raw (a, keys[i]);
        }
        if (!b_artist) {
            b_artist = pl_find_meta_raw (b, keys[i]);
        }
        if (a_artist && b_artist) {
            break;
        }
    }
    return pl_find_meta_raw (a, "album") == pl_find_meta_raw (b, "album") && a_artist == b_artist;
}

static size_t
_plt_get_items (playlist_t *plt, playItem_t ***out_items, int selected) {
    LOCK;
    int count = selected ? plt_getselcount (plt) : plt_get_item_count(plt, PL_MAIN);
    if (count == 0) {
        UNLOCK;
        return 0;
    }

    playItem_t **items = calloc (count, sizeof (playItem_t *));

    int index = 0;
    for (playItem_t *item = plt->head[PL_MAIN]; item != NULL; item = item->next[PL_MAIN]) {
        if (!selected || item->selected) {
            items[index++] = item;
            pl_item_ref (item);
        }
    }

    UNLOCK;

    *out_items = items;
    return count;
}

size_t
plt_get_items (playlist_t *plt, playItem_t ***out_items) {
    return _plt_get_items (plt, out_items, 0);
}

size_t
plt_get_selected_items(playlist_t *plt, playItem_t ***out_items) {
    return _plt_get_items (plt, out_items, 1);
}
