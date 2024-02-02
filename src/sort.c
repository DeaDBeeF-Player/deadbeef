/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
*/

#include <sys/time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "utf8.h"
#include "sort.h"
#include "tf.h"
#include "pltmeta.h"
#include "plmeta.h"
#include "messagepump.h"
#include "undo/undobuffer.h"
#include "undo/undomanager.h"
#include "undo/undo_playlist.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static void
plt_sort_internal (playlist_t *playlist, int iter, int id, const char *format, int order, int version);

void
plt_sort_v2 (playlist_t *plt, int iter, int id, const char *format, int order) {
    ddb_undobuffer_t *undobuffer = ddb_undomanager_get_buffer (ddb_undomanager_shared ());
    if (plt->undo_enabled) {
        pl_lock ();
        int count = plt->count[PL_MAIN];
        playItem_t **tracks = calloc (count, sizeof (playItem_t *));
        int index = 0;
        for (playItem_t *it = plt->head[PL_MAIN]; it != NULL; it = it->next[PL_MAIN]) {
            tracks[index++] = it;
        }
        undo_remove_items(undobuffer, plt, tracks, count);
        free (tracks);
    }

    plt_sort_internal (plt, iter, id, format, order, 1);

    if (plt->undo_enabled) {
        int count = plt->count[PL_MAIN];
        playItem_t **tracks = calloc (count, sizeof (playItem_t *));
        int index = 0;
        for (playItem_t *it = plt->head[PL_MAIN]; it != NULL; it = it->next[PL_MAIN]) {
            tracks[index++] = it;
        }
        undo_insert_items(undobuffer, plt, tracks, count);
        free (tracks);
        pl_unlock ();
    }
}

// sort for title formatting v1
void
plt_sort (playlist_t *playlist, int iter, int id, const char *format, int order) {
    plt_sort_internal (playlist, iter, id, format, order, 0);
}

static int pl_sort_is_duration;
static int pl_sort_is_track;
static int pl_sort_ascending;
static int pl_sort_id;
static int pl_sort_version; // 0: use pl_sort_format, 1: use pl_sort_tf_bytecode
static const char *pl_sort_format;
static char *pl_sort_tf_bytecode;
static ddb_tf_context_t pl_sort_tf_ctx;

static int
strcasecmp_numeric (const char *a, const char *b) {
    if (isdigit (*a) && isdigit (*b)) {
        int anum = *a-'0';
        const char *ae = a+1;
        while (*ae && isdigit (*ae)) {
            anum *= 10;
            anum += *ae-'0';
            ae++;
        }
        int bnum = *b-'0';
        const char *be = b+1;
        while (*be && isdigit (*be)) {
            bnum *= 10;
            bnum += *be-'0';
            be++;
        }
        if (anum == bnum) {
            return u8_strcasecmp (ae, be);
        }
        return anum - bnum;
    }
    return u8_strcasecmp (a,b);
}

static int
pl_sort_compare_str (playItem_t *a, playItem_t *b) {
    if (pl_sort_is_duration) {
        int64_t dur_a = (int64_t)((double)a->_duration * 100000);
        int64_t dur_b = (int64_t)((double)b->_duration * 100000);
        return !pl_sort_ascending ? (int)(dur_b - dur_a) : (int)(dur_a - dur_b);
    }
    else if (pl_sort_is_track) {
        int t1;
        int t2;
        const char *t;
        t = pl_find_meta_raw (a, "track");
        if (t && !isdigit (*t)) {
            t1 = 999999;
        }
        else {
            t1 = t ? atoi (t) : -1;
        }
        t = pl_find_meta_raw (b, "track");
        if (t && !isdigit (*t)) {
            t2 = 999999;
        }
        else {
            t2 = t ? atoi (t) : -1;
        }
        return !pl_sort_ascending ? t2 - t1 : t1 - t2;
    }
    else {
        char tmp1[1024];
        char tmp2[1024];
        if (pl_sort_version == 0) {
            pl_format_title (a, -1, tmp1, sizeof (tmp1), pl_sort_id, pl_sort_format);
            pl_format_title (b, -1, tmp2, sizeof (tmp2), pl_sort_id, pl_sort_format);
        }
        else {
            pl_sort_tf_ctx.id = pl_sort_id;
            pl_sort_tf_ctx.it = (ddb_playItem_t *)a;
            tf_eval(&pl_sort_tf_ctx, pl_sort_tf_bytecode, tmp1, sizeof(tmp1));
            pl_sort_tf_ctx.it = (ddb_playItem_t *)b;
            tf_eval(&pl_sort_tf_ctx, pl_sort_tf_bytecode, tmp2, sizeof(tmp2));
        }
        int res = strcasecmp_numeric (tmp1, tmp2);
        if (!pl_sort_ascending) {
            res = -res;
        }
        return res;
    }
}

static int
qsort_cmp_func (const void *a, const void *b) {
    playItem_t *aa = *((playItem_t **)a);
    playItem_t *bb = *((playItem_t **)b);
    return pl_sort_compare_str (aa, bb);
}

void
plt_sort_random (playlist_t *playlist, int iter) {
    plt_replace_meta (playlist, "autosort_mode", "random");
    if (!playlist->head[iter] || !playlist->head[iter]->next[iter]) {
        return;
    }

    pl_lock ();

    const int playlist_count = playlist->count[iter];
    playItem_t **array = calloc (playlist_count, sizeof (playItem_t *));
    int idx = 0;
    int count = 0;
    for (playItem_t *it = playlist->head[iter]; it; it = it->next[iter], idx++, count++) {
        array[idx] = it;
    }

    //randomize array
    for (int swap_a = 0; swap_a < playlist_count - 1; swap_a++) {
        //select random item above swap_a-1
        const int swap_b = (int)(swap_a + (rand() / (float)RAND_MAX * (playlist_count - swap_a)));

        //swap a with b
        playItem_t* const swap_temp = array[swap_a];
        array[swap_a] = array[swap_b];
        array[swap_b] = swap_temp;

    }

    playItem_t *prev = NULL;
    playlist->head[iter] = 0;
    for (idx = 0; idx < count; idx++) {
        playItem_t *it = array[idx];
        it->prev[iter] = prev;
        it->next[iter] = NULL;
        if (!prev) {
            playlist->head[iter] = it;
        }
        else {
            prev->next[iter] = it;
        }
        prev = it;
    }
    playlist->tail[iter] = array[playlist->count[iter]-1];

    free (array);

    plt_modified (playlist);

    pl_unlock ();
}

// version 0: title formatting v1
// version 1: title formatting v2
void
plt_sort_internal (playlist_t *playlist, int iter, int id, const char *format, int order, int version) {
    if (order == DDB_SORT_RANDOM) {
        plt_sort_random (playlist, iter);
        return;
    }
    int ascending = order == DDB_SORT_DESCENDING ? 0 : 1;
    plt_set_meta_int (playlist, "autosort_ascending", ascending);
    if (version != 0 && format != NULL) {
        plt_replace_meta (playlist, "autosort_mode", "tf");
        plt_replace_meta (playlist, "autosort_tf", format);
    }

    if (format == NULL || id == DB_COLUMN_FILENUMBER || !playlist->head[iter] || !playlist->head[iter]->next[iter]) {
        return;
    }
    pl_lock ();
    struct timeval tm1;
    gettimeofday (&tm1, NULL);
    pl_sort_ascending = ascending;
    trace ("ascending: %d\n", ascending);
    pl_sort_id = id;

    pl_sort_version = version;
    if (version == 0) {
        pl_sort_format = format;
        pl_sort_tf_bytecode = NULL;
    }
    else {
        pl_sort_format = NULL;
        pl_sort_tf_bytecode = tf_compile (format);
        pl_sort_tf_ctx._size = sizeof (pl_sort_tf_ctx);
        pl_sort_tf_ctx.it = NULL;
        pl_sort_tf_ctx.plt = (ddb_playlist_t *)playlist;
        pl_sort_tf_ctx.idx = -1;
        pl_sort_tf_ctx.id = id;
    }

    if (format && id == -1
        && ((version == 0 && !strcmp (format, "%l"))
            || (version == 1 && !strcmp (format, "%length%")))
        ) {
        pl_sort_is_duration = 1;
    }
    else {
        pl_sort_is_duration = 0;
    }
    if (format && id == -1
        && ((version == 0 && !strcmp (format, "%n"))
            || (version == 1 && (!strcmp (format, "%track number%") || !strcmp (format, "%tracknumber%"))))
        ) {
        pl_sort_is_track = 1;
    }
    else {
        pl_sort_is_track = 0;
    }

    int cursor = plt_get_cursor (playlist, PL_MAIN);
    playItem_t *track_under_cursor = NULL;
    if (cursor != -1) {
        track_under_cursor = plt_get_item_for_idx (playlist, cursor, PL_MAIN);
    }
    playItem_t **array = malloc (playlist->count[iter] * sizeof (playItem_t *));
    int idx = 0;
    for (playItem_t *it = playlist->head[iter]; it; it = it->next[iter], idx++) {
        array[idx] = it;
    }

#if HAVE_MERGESORT
    mergesort (array, playlist->count[iter], sizeof (playItem_t *), qsort_cmp_func);
#else
    qsort (array, playlist->count[iter], sizeof (playItem_t *), qsort_cmp_func);
#endif
    playItem_t *prev = NULL;
    playlist->head[iter] = 0;
    for (idx = 0; idx < playlist->count[iter]; idx++) {
        playItem_t *it = array[idx];
        it->prev[iter] = prev;
        it->next[iter] = NULL;
        if (!prev) {
            playlist->head[iter] = it;
        }
        else {
            prev->next[iter] = it;
        }
        prev = it;
    }

    playlist->tail[iter] = array[playlist->count[iter]-1];

    free (array);

    if (track_under_cursor) {
        cursor = plt_get_item_idx (playlist, track_under_cursor, PL_MAIN);
        plt_set_cursor (playlist, PL_MAIN, cursor);
        pl_item_unref (track_under_cursor);
    }

    plt_modified (playlist);

    if (version == 0) {
        pl_sort_format = NULL;
    }

    if (version == 1) {
        tf_free (pl_sort_tf_bytecode);
        pl_sort_tf_bytecode = NULL;
        memset (&pl_sort_tf_ctx, 0, sizeof (pl_sort_tf_ctx));
    }

    pl_unlock ();
}

void
sort_track_array (playlist_t *playlist, playItem_t **tracks, int num_tracks, const char *format, int order) {
    if (order != DDB_SORT_DESCENDING && order != DDB_SORT_ASCENDING) {
        // unsupported
        return;
    }
    int ascending = order == DDB_SORT_DESCENDING ? 0 : 1;

    if (format == NULL || !tracks || !num_tracks) {
        return;
    }

    pl_lock ();
    pl_sort_ascending = ascending;
    pl_sort_id = -1;

    pl_sort_version = 1;
    pl_sort_format = NULL;
    pl_sort_tf_bytecode = tf_compile (format);
    pl_sort_tf_ctx._size = sizeof (pl_sort_tf_ctx);
    pl_sort_tf_ctx.it = NULL;
    pl_sort_tf_ctx.plt = (ddb_playlist_t *)playlist;
    pl_sort_tf_ctx.idx = -1;
    pl_sort_tf_ctx.id = -1;

    if (format
        && !strcmp (format, "%length%")) {
        pl_sort_is_duration = 1;
    }
    else {
        pl_sort_is_duration = 0;
    }
    if (format
        && (!strcmp (format, "%track number%") || !strcmp (format, "%tracknumber%"))) {
        pl_sort_is_track = 1;
    }
    else {
        pl_sort_is_track = 0;
    }

#if HAVE_MERGESORT
    mergesort (tracks, num_tracks, sizeof (playItem_t *), qsort_cmp_func);
#else
    qsort (tracks, num_tracks, sizeof (playItem_t *), qsort_cmp_func);
#endif

    tf_free (pl_sort_tf_bytecode);
    pl_sort_tf_bytecode = NULL;
    memset (&pl_sort_tf_ctx, 0, sizeof (pl_sort_tf_ctx));

    pl_unlock ();
}

void
plt_autosort (playlist_t *plt) {
    int autosort_enabled = plt_find_meta_int (plt, "autosort_enabled", 0);
    if (!autosort_enabled) {
        return;
    }

    const char *autosort_mode = plt_find_meta (plt, "autosort_mode");
    if (!autosort_mode) {
        return;
    }

    if (!strcmp (autosort_mode, "tf")) {
        int ascending = plt_find_meta_int (plt, "autosort_ascending", 0);
        const char *fmt = plt_find_meta (plt, "autosort_tf");
        if (!fmt) {
            return;
        }
        plt_sort_v2 (plt, PL_MAIN, -1, fmt, ascending ? DDB_SORT_ASCENDING : DDB_SORT_DESCENDING);
    }
    else if (!strcmp (autosort_mode, "random")) {
        plt_sort_v2 (plt, PL_MAIN, -1, NULL, DDB_SORT_RANDOM);
    }

    plt_save_config (plt);

    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}
