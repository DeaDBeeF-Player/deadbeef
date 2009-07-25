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
    int (*init) (const char *fname, int track, float start, float end);
    void (*free) (void);
    // player is responsible for starting next song if -1 is returned
    int (*read) (char *bytes, int size);
    int (*seek) (float time);
    int (*add) (const char *fname);
    const char ** (*getexts) (void);
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
