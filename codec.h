#ifndef __CODEC_H
#define __CODEC_H

#include <stdint.h>
//#include "playlist.h"

typedef struct {
    int bitsPerSample;
    int channels;
    int samplesPerSecond;
    float duration;
    float position;
} fileinfo_t;

struct playItem_s;

typedef struct codec_s {
    int (*init) (const char *fname, int track, float start, float end);
    void (*free) (void);
    // player is responsible for starting next song if -1 is returned
    int (*read) (char *bytes, int size);
    int (*seek) (float time);
    struct playItem_s * (*insert) (struct playItem_s *after, const char *fname); // after==NULL means "prepend to beginning"
    const char ** (*getexts) (void);
    int (*numvoices) (void);
    void (*mutevoice) (int voice, int mute);
    fileinfo_t info;
} codec_t;

codec_t *get_codec_for_file (const char *fname);

void
codec_init_locking (void);

void
codec_free_locking (void);

void
codec_lock (void);

void
codec_unlock (void);

#endif	// __CODEC_H
