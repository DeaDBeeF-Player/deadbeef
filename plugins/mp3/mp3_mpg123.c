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

#include <string.h>
#include "mp3_mpg123.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

void
mp3_mpg123_init (mp3_info_t *info) {
    int ret;
    mpg123_init();
    info->mpg123_handle = mpg123_new (NULL, &ret);
//    ret = mpg123_param (info->mpg123_handle, MPG123_VERBOSE, 2, 0);
    ret = mpg123_format_none (info->mpg123_handle);
//    ret = mpg123_param (info->mpg123_handle, MPG123_FLAGS, MPG123_FUZZY | MPG123_SEEKBUFFER | MPG123_GAPLESS, 0);
    ret = mpg123_param (info->mpg123_handle, MPG123_FLAGS, MPG123_QUIET, 0);
    ret = mpg123_format (info->mpg123_handle, info->info.fmt.samplerate, MPG123_MONO | MPG123_STEREO, MPG123_ENC_FLOAT_32);
    ret = mpg123_param (info->mpg123_handle, MPG123_FORCE_RATE, info->info.fmt.samplerate, 0);
    ret = mpg123_open_feed (info->mpg123_handle);

    info->mpg123_status = MPG123_NEED_MORE;
}

void
mp3_mpg123_free (mp3_info_t *info) {
    if (info->mpg123_handle) {
        mpg123_delete (info->mpg123_handle);
        info->mpg123_handle = NULL;
    }
    info->mpg123_status = MPG123_NEED_MORE;
//    mpg123_exit();
}

void
mp3_mpg123_consume_decoded_data (mp3_info_t *info) {
    int samplesize = (info->info.fmt.bps>>3)*info->info.fmt.channels;
    unsigned char *dataptr = info->mpg123_audio + (info->total_decoded_samples - info->decoded_samples_remaining) * samplesize;
    int bytes = info->decoded_samples_remaining * samplesize;
    bytes = min (bytes, info->bytes_to_decode);
    memcpy (info->out, dataptr, bytes);
    info->out += bytes;
    info->bytes_to_decode -= bytes;
    info->decoded_samples_remaining -= bytes / samplesize;
}

int
mp3_mpg123_decode_next_packet (mp3_info_t *info) {
    int eof = 0;
    while (!eof && (info->decoded_samples_remaining <= 0)) {
        if (info->mpg123_status == MPG123_NEED_MORE) {
            size_t bytesread = 0;
            bytesread = deadbeef->fread (info->input, 1, READBUFFER, info->file);
            if (!bytesread) {
                // add guard
                eof = 1;
                memset (info->input, 0, 8);
                bytesread = 8;
            }
            info->mpg123_status = mpg123_feed (info->mpg123_handle, (const uint8_t *)info->input, bytesread);

            if (info->mpg123_status == MPG123_ERR || info->mpg123_status == MPG123_NEED_MORE) {
                continue;
            }
        }

        off_t num;
        unsigned char *audio;
        size_t nbytes;
        for (;;) {
            info->mpg123_status = mpg123_decode_frame (info->mpg123_handle, &num, &audio, &nbytes);
            if (info->mpg123_status == MPG123_NEW_FORMAT) {
                long rate;
                int channels, enc;
                mpg123_getformat (info->mpg123_handle, &rate, &channels, &enc);
                info->info.fmt.samplerate = (int)rate;
                info->info.fmt.channels = channels;
                continue;
            }
            else if (info->mpg123_status != MPG123_OK) {
                break;
            }

            break;
        }
        if (info->mpg123_status != MPG123_OK) {
            continue;
        }

        // synthesize single frame
        int samplesize = (info->info.fmt.bps>>3)*info->info.fmt.channels;
        info->total_decoded_samples = info->decoded_samples_remaining = (int)nbytes/samplesize;
        info->mpg123_audio = audio;

        // NOTE: calling frame_bitrate directly would be much faster, but the API is private,
        // and mpg123_frameinfo struct is opaque, so we can't even get the parameters directly
        struct mpg123_frameinfo inf;
        mpg123_info (info->mpg123_handle, &inf);
        deadbeef->streamer_set_bitrate (inf.bitrate);
        break;
    }

    return eof;
}

mp3_decoder_api_t mpg123_api = {
    .init = mp3_mpg123_init,
    .free = mp3_mpg123_free,
    .consume_decoded_data = mp3_mpg123_consume_decoded_data,
    .decode_next_packet = mp3_mpg123_decode_next_packet,
};
