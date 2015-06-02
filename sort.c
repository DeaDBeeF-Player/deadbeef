/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static void
plt_sort_internal (playlist_t *playlist, int iter, int id, const char *format, int order, int version);

void plt_sort_v2 (playlist_t *plt, int iter, int id, const char *format, int order) {
    plt_sort_internal (plt, iter, id, format, order, 1);
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
        float dur_a = a->_duration * 100000;
        float dur_b = b->_duration * 100000;
        return !pl_sort_ascending ? dur_b - dur_a : dur_a - dur_b;
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
    if (!playlist->head[iter] || !playlist->head[iter]->next[iter]) {
        return;
    }

    pl_lock ();

    const int playlist_count = playlist->count[iter];
    playItem_t **array = malloc (playlist_count * sizeof (playItem_t *));
    int idx = 0;
    for (playItem_t *it = playlist->head[iter]; it; it = it->next[iter], idx++) {
        array[idx] = it;
    }

    //randomize array
    for (int swap_a = 0; swap_a < playlist_count - 1; swap_a++) {
        //select random item above swap_a-1
        const int swap_b = swap_a + (rand() / (float)RAND_MAX * (playlist_count - swap_a));

        //swap a with b
        playItem_t* const swap_temp = array[swap_a];
        array[swap_a] = array[swap_b];
        array[swap_b] = swap_temp;

    }

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

    playItem_t **array = malloc (playlist->count[iter] * sizeof (playItem_t *));
    int idx = 0;
    for (playItem_t *it = playlist->head[iter]; it; it = it->next[iter], idx++) {
        array[idx] = it;
    }
    qsort (array, playlist->count[iter], sizeof (playItem_t *), qsort_cmp_func);
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

    struct timeval tm2;
    gettimeofday (&tm2, NULL);
    int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    trace ("sort time: %f seconds\n", ms / 1000.f);

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
