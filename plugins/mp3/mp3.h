/*
 MPEG decoder plugin for DeaDBeeF Player
 Copyright (C) 2009-2014 Oleksiy Yakovenko

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
#ifndef deadbeef_mp3_h
#define deadbeef_mp3_h

#include <deadbeef/deadbeef.h>

#ifdef USE_LIBMAD
#include <mad.h>
#endif
#ifdef USE_LIBMPG123
#include <mpg123.h>
#endif

#include "mp3parser.h"

extern DB_functions_t *deadbeef;

#define READBUFFER 0x2800 // 10k is enough for single frame

struct mp3_decoder_api_s;

typedef struct {
    DB_fileinfo_t info;

    uint32_t startoffs;
    uint32_t endoffs;

    int64_t startsample;
    int64_t endsample;

    mp3info_t mp3info;
    uint32_t mp3flags; // extra flags to pass to mp3parser

    int64_t currentsample;
    int64_t skipsamples; // how many samples to skip after seek, usually "seek_sample - mp3info.pcmsample"

    DB_FILE *file;
    DB_playItem_t *it;

    // output buffer, supplied by player
    int bytes_to_decode; // how many bytes is asked to be written to `out`
    int decoded_samples_remaining; // number of samples left of current decoded mpeg frame
    char *out;

    // temp buffer for 32bit decoding, before converting to 16 bit
    char *conv_buf;
    int conv_buf_size;

    char input[READBUFFER]; // input buffer, for MPEG data

    union {
#ifdef USE_LIBMAD
        struct {
            struct mad_stream mad_stream;
            struct mad_frame mad_frame;
            struct mad_synth mad_synth;
            long input_remaining_bytes;
        };
#endif
#ifdef USE_LIBMPG123
        struct {
            mpg123_handle *mpg123_handle;
            int mpg123_status;
            unsigned char *mpg123_audio;
            int total_decoded_samples;
        };
#endif
    };

    int want_16bit;
    int raw_signal;
    struct mp3_decoder_api_s *dec;
} mp3_info_t;

typedef struct mp3_decoder_api_s {
    // initialize the decoder, get ready to receive/decode packets
    void (*init)(mp3_info_t *info);

    // free the decoder
    void (*free)(mp3_info_t *info);

    // consume decoded samples into output buffer
    void (*consume_decoded_data)(mp3_info_t *info);

    // read and decode a single mpeg frame, only if `decoded_samples_remaining` is <=0
    // return 1 if eof
    int (*decode_next_packet)(mp3_info_t *info);
} mp3_decoder_api_t;

#endif
