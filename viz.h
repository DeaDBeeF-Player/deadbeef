#ifndef viz_h
#define viz_h

#include "deadbeef.h"

void
viz_process (char * restrict bytes, int bytes_size, DB_output_t *output);

void
viz_init (void);

void
viz_free (void);

void
viz_sync (int frames_played);

void
viz_waveform_listen (void *ctx, void (*callback)(void *ctx, ddb_audio_data_t *data));

void
viz_waveform_unlisten (void *ctx);

void
viz_spectrum_listen (void *ctx, void (*callback)(void *ctx, ddb_audio_data_t *data));

void
viz_spectrum_unlisten (void *ctx);

#endif /* viz_h */
