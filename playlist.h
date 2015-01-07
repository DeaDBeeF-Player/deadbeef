/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  playlist management

  Copyright (C) 2009-2013 Alexey Yakovenko

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

  Alexey Yakovenko waker@users.sourceforge.net
*/

#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include <stdint.h>
#include <time.h>
#include "deadbeef.h"

#define PL_MAX_ITERATORS 2

// predefined properties stored in metadata for storage unification:
// :URI - full pathname
// :DECODER - decoder id
// :TRACKNUM - subsong index (sid, nsf, cue, etc)
// :DURATION - length in seconds

typedef struct playItem_s {
    int startsample;
    int endsample;
    int shufflerating; // sort order for shuffle mode
    // private area, must not be visible to plugins
    float _duration;
    uint32_t _flags;
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
    struct playlist_s *next;
    int count[2];
    float totaltime;
    int modification_idx;
    int last_save_modification_idx;
    playItem_t *head[PL_MAX_ITERATORS]; // head of linked list
    playItem_t *tail[PL_MAX_ITERATORS]; // tail of linked list
    int current_row[PL_MAX_ITERATORS]; // current row (cursor)
    int scroll;
    struct DB_metaInfo_s *meta; // linked list storing metainfo
    int refc;
    int files_add_visibility;
    unsigned fast_mode : 1;
    unsigned files_adding : 1;
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

//void
//plt_lock (void);
//
//void
//plt_unlock (void);

// playlist management functions

// it is highly recommended to access that from inside plt_lock/unlock block
playlist_t *
plt_get_curr_ptr (void);

playlist_t *
plt_get_for_idx (int idx);

void
plt_ref (playlist_t *plt);

void
plt_unref (playlist_t *plt);

playlist_t *
plt_alloc (const char *title);

void
plt_free (playlist_t *plt);

int
plt_get_count (void);

playlist_t *
plt_get_list (void);

playItem_t *
plt_get_head (int plt);

int
plt_get_sel_count (int plt);

int
plt_add (int before, const char *title);

void
plt_remove (int plt);

int
plt_find (const char *name);

void
plt_set_curr_idx (int plt);

int
plt_get_curr_idx (void);

void
plt_set_curr (playlist_t *plt);

playlist_t *
plt_get_curr (void);

int
plt_get_idx_of (playlist_t *plt);

int
plt_get_title (playlist_t *plt, char *buffer, int bufsize);

int
plt_set_title (playlist_t *plt, const char *title);

void
plt_modified (playlist_t *plt);

int
plt_get_modification_idx (playlist_t *plt);

// moves playlist #from to position #to
void
plt_move (int from, int to);

// playlist access functions
void
pl_clear (void);

void
plt_clear (playlist_t *plt);

int
plt_add_dir (playlist_t *plt, const char *dirname, int (*cb)(playItem_t *it, void *data), void *user_data);

int
plt_add_file (playlist_t *plt, const char *fname, int (*cb)(playItem_t *it, void *data), void *user_data);

int
pl_add_files_begin (playlist_t *plt);

void
pl_add_files_end (void);

playItem_t *
plt_insert_dir (playlist_t *plt, playItem_t *after, const char *dirname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data);

playItem_t *
plt_insert_file (playlist_t *playlist, playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data);

playItem_t *
pl_insert_item (playItem_t *after, playItem_t *it);

playItem_t *
plt_insert_item (playlist_t *playlist, playItem_t *after, playItem_t *it);

int
plt_remove_item (playlist_t *playlist, playItem_t *it);

playItem_t *
pl_item_alloc (void);

playItem_t *
pl_item_alloc_init (const char *fname, const char *decoder_id);

void
pl_item_ref (playItem_t *it);

void
pl_item_unref (playItem_t *it);

void
pl_item_copy (playItem_t *out, playItem_t *it);

int
pl_getcount (int iter);

int
plt_get_item_count (playlist_t *plt, int iter);

int
pl_getselcount (void);

int
plt_getselcount (playlist_t *playlist);

playItem_t *
pl_get_for_idx (int idx);

playItem_t *
plt_get_item_for_idx (playlist_t *playlist, int idx, int iter);

playItem_t *
pl_get_for_idx_and_iter (int idx, int iter);

int
plt_get_item_idx (playlist_t *playlist, playItem_t *it, int iter);

int
pl_get_idx_of (playItem_t *it);

int
pl_get_idx_of_iter (playItem_t *it, int iter);

playItem_t *
plt_insert_cue_from_buffer (playlist_t *plt, playItem_t *after, playItem_t *origin, const uint8_t *buffer, int buffersize, int numsamples, int samplerate);

playItem_t *
plt_insert_cue (playlist_t *plt, playItem_t *after, playItem_t *origin, int numsamples, int samplerate);

void
pl_add_meta (playItem_t *it, const char *key, const char *value);

void
pl_append_meta (playItem_t *it, const char *key, const char *value);

// must be used in explicit pl_lock/unlock block
// that makes it possible to avoid copying metadata on every access
// pl_find_meta may return overriden value (where the key is prefixed with '!')
const char *
pl_find_meta (playItem_t *it, const char *key);

const char *
pl_find_meta_raw (playItem_t *it, const char *key);

int
pl_find_meta_int (playItem_t *it, const char *key, int def);

float
pl_find_meta_float (playItem_t *it, const char *key, float def);

void
pl_replace_meta (playItem_t *it, const char *key, const char *value);

void
pl_set_meta_int (playItem_t *it, const char *key, int value);

void
pl_set_meta_float (playItem_t *it, const char *key, float value);

void
pl_delete_meta (playItem_t *it, const char *key);

void
pl_delete_all_meta (playItem_t *it);

// returns index of 1st deleted item
int
plt_delete_selected (playlist_t *plt);

int
pl_delete_selected (void);

void
plt_crop_selected (playlist_t *playlist);

void
pl_crop_selected (void);

int
plt_save (playlist_t *plt, playItem_t *first, playItem_t *last, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data);

int
plt_save_n (int n);

int
pl_save_current (void);

int
pl_save_all (void);

playItem_t *
plt_load (playlist_t *plt, playItem_t *after, const char *fname, int *pabort, int (*cb)(playItem_t *it, void *data), void *user_data);

int
pl_load_all (void);

void
plt_select_all (playlist_t *plt);

void
pl_select_all (void);

void
plt_reshuffle (playlist_t *playlist, playItem_t **ppmin, playItem_t **ppmax);

// required to calculate total playtime
void
plt_set_item_duration (playlist_t *playlist, playItem_t *it, float duration);

float
pl_get_item_duration (playItem_t *it);

void
pl_set_item_replaygain (playItem_t *it, int idx, float value);

float
pl_get_item_replaygain (playItem_t *it, int idx);

uint32_t
pl_get_item_flags (playItem_t *it);

void
pl_set_item_flags (playItem_t *it, uint32_t flags);

// returns number of characters printed, not including trailing 0
// [a]rtist, [t]itle, al[b]um, [l]ength, track[n]umber
int
pl_format_title (playItem_t *it, int idx, char *s, int size, int id, const char *fmt);

int
pl_format_title_escaped (playItem_t *it, int idx, char *s, int size, int id, const char *fmt);

void
pl_format_time (float t, char *dur, int size);

void
plt_reset_cursor (playlist_t *plt);

float
plt_get_totaltime (playlist_t *plt);

float
pl_get_totaltime (void);

void
pl_set_selected (playItem_t *it, int sel);

int
pl_is_selected (playItem_t *it);

playItem_t *
plt_get_first (playlist_t *playlist, int iter);

playItem_t *
pl_get_first (int iter);

playItem_t *
plt_get_last (playlist_t *playlist, int iter);

playItem_t *
pl_get_last (int iter);

playItem_t *
pl_get_next (playItem_t *it, int iter);

playItem_t *
pl_get_prev (playItem_t *it, int iter);

int
plt_get_cursor (playlist_t *plt, int iter);

int
pl_get_cursor (int iter);

void
plt_set_cursor (playlist_t *plt, int iter, int cursor);

void
pl_set_cursor (int iter, int cursor);

void
plt_move_items (playlist_t *to, int iter, playlist_t *from, playItem_t *drop_before, uint32_t *indexes, int count);

void
plt_copy_items (playlist_t *to, int iter, playlist_t *from, playItem_t *before, uint32_t *indices, int cnt);

void
plt_search_reset (playlist_t *plt);

void
plt_search_process (playlist_t *plt, const char *text);

void
plt_sort (playlist_t *plt, int iter, int id, const char *format, int order);

void
plt_sort_random (playlist_t *plt, int iter);

void
pl_items_copy_junk (struct playItem_s *from, struct playItem_s *first, struct playItem_s *last);

struct DB_metaInfo_s *
pl_get_metadata_head (playItem_t *it);

void
pl_delete_metadata (playItem_t *it, struct DB_metaInfo_s *meta);

void
pl_set_order (int order);

int
pl_get_order (void);

playlist_t *
pl_get_playlist (playItem_t *it);

void
plt_init_shuffle_albums (playlist_t *plt, int r);

void
plt_set_fast_mode (playlist_t *plt, int fast);

int
plt_is_fast_mode (playlist_t *plt);

void
pl_ensure_lock (void);

int
pl_get_meta (playItem_t *it, const char *key, char *val, int size);

int
pl_get_meta_raw (playItem_t *it, const char *key, char *val, int size);

int
pl_meta_exists (playItem_t *it, const char *key);

int
plt_get_meta (playlist_t *handle, const char *key, char *val, int size);

int
plt_get_idx (playlist_t *plt);

int
plt_save_config (playlist_t *plt);

int
listen_file_added (int (*callback)(ddb_fileadd_data_t *data, void *user_data), void *user_data);

void
unlisten_file_added (int id);

int
listen_file_add_beginend (void (*callback_begin) (ddb_fileadd_data_t *data, void *user_data), void (*callback_end)(ddb_fileadd_data_t *data, void *user_data), void *user_data);

void
unlisten_file_add_beginend (int id);

playItem_t *
plt_load2 (int visibility, playlist_t *plt, playItem_t *after, const char *fname, int *pabort, int (*callback)(playItem_t *it, void *user_data), void *user_data);

int
plt_add_file2 (int visibility, playlist_t *plt, const char *fname, int (*callback)(playItem_t *it, void *user_data), void *user_data);

int
plt_add_dir2 (int visibility, playlist_t *plt, const char *dirname, int (*callback)(playItem_t *it, void *user_data), void *user_data);

playItem_t *
plt_insert_file2 (int visibility, playlist_t *playlist, playItem_t *after, const char *fname, int *pabort, int (*callback)(playItem_t *it, void *user_data), void *user_data);

playItem_t *
plt_insert_dir2 (int visibility, playlist_t *plt, playItem_t *after, const char *dirname, int *pabort, int (*callback)(playItem_t *it, void *user_data), void *user_data);

int
plt_add_files_begin (playlist_t *plt, int visibility);

void
plt_add_files_end (playlist_t *plt, int visibility);

void
plt_deselect_all (playlist_t *plt);

void
plt_set_scroll (playlist_t *plt, int scroll);

int
plt_get_scroll (playlist_t *plt);

const char *
pl_format_duration (playItem_t *it, const char *ret, char *dur, int size);

int
pl_format_item_queue (playItem_t *it, char *s, int size);

void
send_trackinfochanged (playItem_t *track);

#endif // __PLAYLIST_H
