 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Sound emulation stuff
  *
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */

#ifndef _UADE_AUDIO_H_
#define _UADE_AUDIO_H_

#include "sinctable.h"

#define AUDIO_DEBUG 0
/* Queue length 256 implies minimum emulated period of 8. This should be
 * sufficient for all imaginable purposes. This must be power of two. */
#define SINC_QUEUE_LENGTH 256

typedef struct {
    int time, output;
} sinc_queue_t;

extern struct audio_channel_data {
    unsigned long adk_mask;
    unsigned long evtime;
    unsigned char dmaen, intreq2, data_written;
    uaecptr lc, pt;

    int state, wper, wlen;
    int current_sample;
    int sample_accum, sample_accum_time;
    int output_state;
    sinc_queue_t sinc_queue[SINC_QUEUE_LENGTH];
    int sinc_queue_time;
    int sinc_queue_head;
    int vol;
    uae_u16 dat, nextdat, per, len;    

    /* Debug variables */
    uaecptr ptend, nextdatpt, nextdatptend, datpt, datptend;
} audio_channel[4];

extern void AUDxDAT (int nr, uae_u16 value);
extern void AUDxVOL (int nr, uae_u16 value);
extern void AUDxPER (int nr, uae_u16 value);
extern void AUDxLCH (int nr, uae_u16 value);
extern void AUDxLCL (int nr, uae_u16 value);
extern void AUDxLEN (int nr, uae_u16 value);

void audio_reset (void);
void audio_set_filter(int filter_type, int filter_force);
void audio_set_rate (int rate);
void audio_set_resampler(char *name);
void audio_use_text_scope(void);
void update_audio (void);

#endif
