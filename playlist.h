#ifndef __PLAYLIST_H
#define __PLAYLIST_H

typedef struct metaInfo_s {
    const char *key;
    char *value;
    struct metaInfo_s *next;
} metaInfo_t;

typedef struct playItem_s {
    char *fname; // full pathname
    struct codec_s *codec; // codec to use with this file
    int tracknum; // used for stuff like sid, nsf, cue (will be ignored by most codecs)
    float timestart; // start time of cue track, or -1
    float timeend; // end time of cue track, or -1
    float duration; // in seconds
    const char *filetype; // e.g. MP3 or OGG
    struct playItem_s *next; // next item in linked list
    struct playItem_s *prev; // prev item in linked list
    struct playItem_s *shufflenext; // next item in shuffle list
    struct metaInfo_s *meta; // linked list storing metainfo
    unsigned selected : 1;
} playItem_t;

extern playItem_t *playlist_head; // head of linked list
extern playItem_t *playlist_tail; // tail of linked list
extern playItem_t *playlist_shuffle_head; // head of shuffled playlist
extern playItem_t *playlist_current_ptr; // pointer to a real current playlist item
extern playItem_t playlist_current; // copy of playlist item being played (stays in memory even if removed from playlist)

int
ps_add_dir (const char *dirname);

int
ps_add_file (const char *fname);

playItem_t *
ps_insert_dir (playItem_t *after, const char *dirname);

playItem_t *
ps_insert_file (playItem_t *after, const char *fname);

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
ps_shuffle (void);

void
ps_set_order (int order);

void
ps_set_loop_mode (int mode);

#endif // __PLAYLIST_H
