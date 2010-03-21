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
#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include <stdint.h>
#include <time.h>

#define PL_MAX_ITERATORS 2

typedef struct playItem_s {
    char *fname; // full pathname
    const char *decoder_id;
    int tracknum; // used for stuff like sid, nsf, cue (will be ignored by most codecs)
    int startsample;
    int endsample;
    int shufflerating; // sort order for shuffle mode
    float playtime; // total playtime
    time_t started_timestamp; // result of calling time(NULL)
    const char *filetype; // e.g. MP3 or OGG
    float replaygain_album_gain;
    float replaygain_album_peak;
    float replaygain_track_gain;
    float replaygain_track_peak;
    // private area, must not be visible to plugins
    float _duration; // in seconds
    int _refc;
    struct playItem_s *next[PL_MAX_ITERATORS]; // next item in linked list
    struct playItem_s *prev[PL_MAX_ITERATORS]; // prev item in linked list
    struct DB_metaInfo_s *meta; // linked list storing metainfo
    unsigned selected : 1;
    unsigned played : 1; // mark as played in shuffle mode
    unsigned in_playlist : 1; // 1 if item is in playlist
} playItem_t;

typedef struct playlist_s {
    char *title;
    int count[2];
    float totaltime;
    playItem_t *head[PL_MAX_ITERATORS]; // head of linked list
    playItem_t *tail[PL_MAX_ITERATORS]; // tail of linked list
    int current_row[PL_MAX_ITERATORS]; // current row (cursor)
    struct playlist_s *next;
} playlist_t;

// global playlist control functions
int
pl_init (void);

void
pl_free (void);

void
pl_lock (void);

void
pl_unlock (void);

void
plt_lock (void);

void
plt_unlock (void);

void
pl_global_lock (void);

void
pl_global_unlock (void);


// playlist management functions

// it is highly recommended to access that from inside plt_lock/unlock block
playlist_t *
plt_get_curr_ptr (void);

int
plt_get_count (void);

int
plt_add (int before, const char *title);

void
plt_remove (int plt);

void
plt_free (void);

void
plt_set_curr (int plt);

int
plt_get_curr (void);

int
plt_get_idx_of (playlist_t *plt);

int
plt_get_title (int plt, char *buffer, int bufsize);

int
plt_set_title (int plt, const char *title);

void
plt_move (int from, int before);

// playlist access functions
void
pl_clear (void);

int
pl_add_dir (const char *dirname, int (*cb)(playItem_t *it, void *data), void *user_data);

int
pl_add_file (const char *fname, int (*cb)(playItem_t *it, void *data), void *user_data);

playItem_t *
pl_insert_dir (playItem_t *after, const char *dirname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data);

playItem_t *
pl_insert_file (playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data);

playItem_t *
pl_insert_item (playItem_t *after, playItem_t *it);

int
pl_remove_item (playItem_t *i);

playItem_t *
pl_item_alloc (void);

void
pl_item_ref (playItem_t *it);

void
pl_item_unref (playItem_t *it);

void
pl_item_copy (playItem_t *out, playItem_t *it);

int
pl_getcount (int iter);

int
pl_getselcount (void);

playItem_t *
pl_get_for_idx (int idx);

playItem_t *
pl_get_for_idx_and_iter (int idx, int iter);

int
pl_get_idx_of (playItem_t *it);

playItem_t *
pl_insert_cue_from_buffer (playItem_t *after, playItem_t *origin, const uint8_t *buffer, int buffersize, int numsamples, int samplerate);

playItem_t *
pl_insert_cue (playItem_t *after, playItem_t *origin, int numsamples, int samplerate);

void
pl_add_meta (playItem_t *it, const char *key, const char *value);

// must be used in explicit pl_lock/unlock block
// that makes it possible to avoid copying metadata on every access
const char *
pl_find_meta (playItem_t *it, const char *key);

void
pl_replace_meta (playItem_t *it, const char *key, const char *value);

void
pl_format_item_display_name (playItem_t *it, char *str, int len);

void
pl_delete_all_meta (playItem_t *it);

// returns index of 1st deleted item
int
pl_delete_selected (void);

void
pl_crop_selected (void);

int
pl_save (const char *fname);

int
pl_save_all (void);

int
pl_load (const char *fname);

int
pl_load_all (void);

void
pl_select_all (void);

void
pl_reshuffle (playItem_t **ppmin, playItem_t **ppmax);

// required to calculate total playtime
void
pl_set_item_duration (playItem_t *it, float duration);

float
pl_get_item_duration (playItem_t *it);

// returns number of characters printed, not including trailing 0
// [a]rtist, [t]itle, al[b]um, [l]ength, track[n]umber
int
pl_format_title (playItem_t *it, int idx, char *s, int size, int id, const char *fmt);

void
pl_reset_cursor (void);

float
pl_get_totaltime (void);

void
pl_set_selected (playItem_t *it, int sel);

int
pl_is_selected (playItem_t *it);

playItem_t *
pl_get_first (int iter);

playItem_t *
pl_get_last (int iter);

playItem_t *
pl_get_next (playItem_t *it, int iter);

playItem_t *
pl_get_prev (playItem_t *it, int iter);

int
pl_get_cursor (int iter);

void
pl_set_cursor (int iter, int cursor);

void
pl_move_items (int iter, playItem_t *drop_before, uint32_t *indexes, int count);

void
pl_search_reset (void);

void
pl_search_process (const char *text);

void
pl_sort (int iter, int id, const char *format, int ascending);

// playqueue support functions
int
pl_playqueue_push (playItem_t *it);

void
pl_playqueue_clear (void);

void
pl_playqueue_pop (void);

void
pl_playqueue_remove (playItem_t *it);

int
pl_playqueue_test (playItem_t *it);

playItem_t *
pl_playqueue_getnext (void);

int
pl_playqueue_getcount (void);

void
pl_items_copy_junk (struct playItem_s *from, struct playItem_s *first, struct playItem_s *last);

struct DB_metaInfo_s *
pl_get_metadata (playItem_t *it);

#endif // __PLAYLIST_H
