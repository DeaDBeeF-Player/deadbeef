#ifndef __PLAYLIST_H
#define __PLAYLIST_H

typedef struct playItem_s {
    char *fname; // full pathname
    char *displayname; // all required metainfo columns packed in single string, separated with zeroes
    struct codec_s *codec; // codec to use with this file
    int tracknum; // used for stuff like sid, nsf, cue (will be ignored by most codecs)
    float timestart; // start time of cue track, or -1
    float timeend; // end time of cue track, or -1
    struct playItem_s *next; // next item in linked list
    struct playItem_s *prev; // prev item in linked list
} playItem_t;

extern playItem_t *playlist_head; // head of linked list
extern playItem_t *playlist_tail; // tail of linked list
extern playItem_t *playlist_current;

int
ps_add_file (const char *fname);

int
ps_add_dir (const char *dirname);

int
ps_remove (playItem_t *i);

void
ps_free (void);

int
ps_getcount (void);

playItem_t *
ps_get_for_idx (int idx);

int
ps_get_idx_of (playItem_t *it);

#endif // __PLAYLIST_H
