/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2017 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifndef dsp_h
#define dsp_h

#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

// how much bigger should read-buffer be to allow upsampling.
// e.g. 8000Hz -> 192000Hz upsampling requires 24x buffer size,
// so if we originally request 4096 bytes blocks -
// that will require 24x buffer size, which is 98304 bytes buffer
#define MAX_DSP_RATIO 24

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
dsp_apply (
    ddb_waveformat_t *input_fmt,
    char *input,
    int inputsize,
    ddb_waveformat_t *out_fmt,
    char **out_bytes,
    int *out_numbytes,
    float *out_dsp_ratio);

void
dsp_get_output_format (ddb_waveformat_t *in_fmt, ddb_waveformat_t *out_fmt);

int
dsp_apply_simple_downsampler (
    int input_samplerate,
    int channels,
    char *input,
    int inputsize,
    int output_samplerate,
    char **out_bytes,
    int *out_numbytes);

#ifdef __cplusplus
}
#endif

#endif /* dsp_h */
