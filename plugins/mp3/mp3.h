/*
 MPEG decoder plugin for DeaDBeeF Player
 Copyright (C) 2009-2014 Alexey Yakovenko

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

#include "../../deadbeef.h"

#ifdef USE_LIBMAD
#include <mad.h>
#endif
#ifdef USE_LIBMPG123
#include <mpg123.h>
#endif

extern DB_functions_t *deadbeef;

#define READBUFFER 0x2800 // 10k is enough for single frame

// vbrmethod constants
#define XING_CBR  1
#define XING_ABR  2
#define XING_VBR1 3
#define XING_VBR2 4
#define XING_VBR3 5
#define XING_VBR4 6
#define XING_CBR2 8
#define XING_ABR2 9
#define DETECTED_VBR 100

// xing header flags
#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

struct mp3_decoder_api_s;

typedef struct {
    DB_FILE *file;
    DB_playItem_t *it;

    // input buffer, for MPEG data
    char input[READBUFFER];
    int remaining;

    // output buffer, supplied by player
    int readsize;
    int decode_remaining; // number of decoded samples of current mpeg frame
    char *out;

    // information, filled by cmp3_scan_stream
    int version;
    int layer;
    int bitrate;
    int samplerate;
    int packetlength;
    int channels;
    float duration;

    // currentsample and totalsamples are in the entire file scope (delay/padding inclusive)
    int64_t currentsample;
    int64_t totalsamples;

    int skipsamples;

    int64_t startoffset; // in bytes (id3v2, xing/lame)
    int64_t endoffset; // in bytes (apev2, id3v1)

    // startsample and endsample exclude delay/padding
    int64_t startsample;
    int64_t endsample;

    // number of samples to skip at the start/end of file
    int delay;
    int padding;

    float avg_packetlength;
    int avg_samplerate;
    int avg_samples_per_frame;
    int nframes;
    int last_comment_update;
    int vbr;
    uint16_t lamepreset;
    int have_xing_header;
    int lead_in_frames;
} buffer_t;

typedef struct {
    DB_fileinfo_t info;

    // input buffer, for MPEG data
    buffer_t buffer;

    // temp buffer for 32bit decoding, before converting to 16 bit
    char *conv_buf;
    int conv_buf_size;

    union {
#ifdef USE_LIBMAD
        struct {
            struct mad_stream mad_stream;
            struct mad_frame mad_frame;
            struct mad_synth mad_synth;
        };
#endif
#ifdef USE_LIBMPG123
        struct {
            mpg123_handle *mpg123_handle;
            int mpg123_status;
            unsigned char *mpg123_audio;
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

    // read samples from decoder, convert into output format, and write into output buffer
    void (*decode)(mp3_info_t *info);

    // read and synthesize single frame, skip lead_in_frames count if needed
    // return 1 if eof
    int (*stream_frame)(mp3_info_t *info);
} mp3_decoder_api_t;

#endif
