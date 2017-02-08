//
//  dsp.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 09/02/2017.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#ifndef dsp_h
#define dsp_h

void
dsp_free (void);

void
dsp_reset (void);

int
streamer_dsp_chain_save (void);

void
dsp_chain_free (ddb_dsp_context_t *dsp_chain);

void
streamer_dsp_postinit (void);

void
streamer_set_dsp_chain_real (ddb_dsp_context_t *chain);

void
streamer_dsp_init (void);

void
streamer_set_dsp_chain_real (ddb_dsp_context_t *chain);

ddb_dsp_context_t *
dsp_clone (ddb_dsp_context_t *from);

int
dsp_apply (ddb_waveformat_t *input_fmt, char *input, int inputsize,
           ddb_waveformat_t *out_fmt, char **out_bytes, int *out_numbytes, float *out_dsp_ratio);

#endif /* dsp_h */
