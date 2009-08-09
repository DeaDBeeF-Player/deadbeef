/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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

typedef struct metaInfo_s {
    const char *key;
    char *value;
    struct metaInfo_s *next;
} metaInfo_t;

#define PS_MAX_ITERATORS 3
#define PS_MAIN 0
#define PS_SEARCH 1
#define PS_SHUFFLE 2

typedef struct playItem_s {
    char *fname; // full pathname
    struct codec_s *codec; // codec to use with this file
    int tracknum; // used for stuff like sid, nsf, cue (will be ignored by most codecs)
    float timestart; // start time of cue track, or -1
    float timeend; // end time of cue track, or -1
    float duration; // in seconds
    const char *filetype; // e.g. MP3 or OGG
    struct playItem_s *next[PS_MAX_ITERATORS]; // next item in linked list
    struct playItem_s *prev[PS_MAX_ITERATORS]; // prev item in linked list
//    struct playItem_s *shufflenext; // next item in shuffle list
//    struct playItem_s *searchnext; // next in search results list
    struct metaInfo_s *meta; // linked list storing metainfo
    unsigned selected : 1;
} playItem_t;

extern playItem_t *playlist_head[PS_MAX_ITERATORS]; // head of linked list
extern playItem_t *playlist_tail[PS_MAX_ITERATORS]; // tail of linked list
extern playItem_t *playlist_current_ptr; // pointer to a real current playlist item
extern playItem_t playlist_current; // copy of playlist item being played (stays in memory even if removed from playlist)
extern int ps_count;

int
ps_add_dir (const char *dirname, int (*cb)(playItem_t *it, void *data), void *user_data);

int
ps_add_file (const char *fname, int (*cb)(playItem_t *it, void *data), void *user_data);

playItem_t *
ps_insert_dir (playItem_t *after, const char *dirname, int (*cb)(playItem_t *it, void *data), void *user_data);

playItem_t *
ps_insert_file (playItem_t *after, const char *fname, int (*cb)(playItem_t *it, void *data), void *user_data);

playItem_t *
ps_insert_item (playItem_t *after, playItem_t *it);

int
ps_append_item (playItem_t *it);

int
ps_remove (playItem_t *i);

void
ps_item_free (playItem_t *it);

void
ps_free (void);

int
ps_getcount (void);

int
ps_getselcount (void);

playItem_t *
ps_get_for_idx (int idx);

int
ps_get_idx_of (playItem_t *it);

playItem_t *
ps_insert_cue (playItem_t *after, const char *cuename, const char *ftype);

int
ps_set_current (playItem_t *it);

// returns -1 if theres no next song, or playlist finished
// reason 0 means "song finished", 1 means "user clicked next"
int
ps_nextsong (int reason);

int
ps_prevsong (void);

int
ps_randomsong (void);

// starts current playlist item from position 0
// only if the item is still in playlist
void
ps_start_current (void);

void
ps_add_meta (playItem_t *it, const char *key, const char *value);

void
ps_format_item_display_name (playItem_t *it, char *str, int len);

const char *
ps_find_meta (playItem_t *it, const char *key);

void
ps_delete_selected (void);

void
ps_set_order (int order);

void
ps_set_loop_mode (int mode);

#endif // __PLAYLIST_H
