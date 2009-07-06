#ifndef __CODEC_H
#define __CODEC_H

#include <stdint.h>

typedef struct {
    int bitsPerSample;
    int channels;
    int samplesPerSecond;
    float duration;
    float position;
} fileinfo_t;

typedef struct codec_s {
    fileinfo_t info;
    int (*init) (const char *fname);
    void (*free) (void);
    // player is responsible for starting next song if -1 is returned
    int (*read) (char *bytes, int size);
    int (*seek) (float time);
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
