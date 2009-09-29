/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

typedef struct metaInfo_s {
    const char *key;
    char *value;
    struct metaInfo_s *next;
} metaInfo_t;

#define PL_MAX_ITERATORS 2
#define PL_MAIN 0
#define PL_SEARCH 1

typedef struct playItem_s {
    char *fname; // full pathname
    struct DB_decoder_s *decoder; // codec to use with this file
    int tracknum; // used for stuff like sid, nsf, cue (will be ignored by most codecs)
    int startsample;
    int endsample;
    float duration; // in seconds
    int shufflerating; // sort order for shuffle mode
    float playtime; // total playtime
    time_t started_timestamp; // result of calling time(NULL)
    const char *filetype; // e.g. MP3 or OGG
    float replaygain_album_gain;
    float replaygain_album_peak;
    float replaygain_track_gain;
    float replaygain_track_peak;
    struct playItem_s *next[PL_MAX_ITERATORS]; // next item in linked list
    struct playItem_s *prev[PL_MAX_ITERATORS]; // prev item in linked list
    struct metaInfo_s *meta; // linked list storing metainfo
    unsigned selected : 1;
    unsigned played : 1; // mark as played in shuffle mode
} playItem_t;

extern playItem_t *playlist_head[PL_MAX_ITERATORS]; // head of linked list
extern playItem_t *playlist_tail[PL_MAX_ITERATORS]; // tail of linked list
extern playItem_t *playlist_current_ptr; // pointer to a real current playlist item (or NULL)

extern int pl_count;

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
pl_append_item (playItem_t *it);

int
pl_remove (playItem_t *i);

playItem_t *
pl_item_alloc (void);

void
pl_item_free (playItem_t *it);

void
pl_item_copy (playItem_t *out, playItem_t *it);

void
pl_free (void);

int
pl_getcount (void);

int
pl_getselcount (void);

playItem_t *
pl_get_for_idx (int idx);

int
pl_get_idx_of (playItem_t *it);

playItem_t *
pl_insert_cue_from_buffer (playItem_t *after, const char *fname, const uint8_t *buffer, int buffersize, struct DB_decoder_s *decoder, const char *ftype, int numsamples, int samplerate);

playItem_t *
pl_insert_cue (playItem_t *after, const char *cuename, struct DB_decoder_s *decoder, const char *ftype, int numsamples, int samplerate);

//int
//pl_set_current (playItem_t *it);

// returns -1 if theres no next song, or playlist finished
// reason 0 means "song finished", 1 means "user clicked next"
int
pl_nextsong (int reason);

int
pl_prevsong (void);

int
pl_randomsong (void);

void
pl_add_meta (playItem_t *it, const char *key, const char *value);

void
pl_format_item_display_name (playItem_t *it, char *str, int len);

const char *
pl_find_meta (playItem_t *it, const char *key);

// returns index of 1st deleted item
int
pl_delete_selected (void);

void
pl_crop_selected (void);

void
pl_set_order (int order);

void
pl_set_loop_mode (int mode);

int
pl_save (const char *fname);

int
pl_load (const char *fname);

void
pl_select_all (void);

void
pl_reshuffle (playItem_t **ppmin, playItem_t **ppmax);

#endif // __PLAYLIST_H
