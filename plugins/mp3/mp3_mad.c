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
#include "mp3_mad.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

void
mp3_mad_init (mp3_info_t *info) {
    mad_stream_init(&info->mad_stream);
    mad_stream_options (&info->mad_stream, MAD_OPTION_IGNORECRC);
    mad_frame_init(&info->mad_frame);
    mad_synth_init(&info->mad_synth);
}

void
mp3_mad_free (mp3_info_t *info) {
    mad_synth_finish (&info->mad_synth);
    mad_frame_finish (&info->mad_frame);
    mad_stream_finish (&info->mad_stream);
}

/****************************************************************************
 * Converts a sample from libmad's fixed point number format to a signed	*
 * short (16 bits).															*
 ****************************************************************************/
static inline int16_t
MadFixedToSshort(mad_fixed_t Fixed)
{
    /* A fixed point number is formed of the following bit pattern:
     *
     * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
     * MSB                          LSB
     * S ==> Sign (0 is positive, 1 is negative)
     * W ==> Whole part bits
     * F ==> Fractional part bits
     *
     * This pattern contains MAD_F_FRACBITS fractional bits, one
     * should alway use this macro when working on the bits of a fixed
     * point number. It is not guaranteed to be constant over the
     * different platforms supported by libmad.
     *
     * The signed short value is formed, after clipping, by the least
     * significant whole part bit, followed by the 15 most significant
     * fractional part bits. Warning: this is a quick and dirty way to
     * compute the 16-bit number, madplay includes much better
     * algorithms.
     */

    /* Clipping */
    if(Fixed>=MAD_F_ONE)
        return(32767);
    if(Fixed<=-MAD_F_ONE)
        return(-32768);

    /* Conversion. */
    Fixed=Fixed>>(MAD_F_FRACBITS-15);
    return((signed short)Fixed);
}

static inline float
MadFixedToFloat (mad_fixed_t Fixed) {
    return (float)((Fixed) / (float)(1L << MAD_F_FRACBITS));
}

#define MadErrorString(x) mad_stream_errorstr(x)

static void
mp3_mad_decode_int16 (mp3_info_t *info) {
    // copy synthesized samples into readbuffer
    int idx = info->mad_synth.pcm.length-info->decoded_samples_remaining;
    // stereo
    if (MAD_NCHANNELS(&info->mad_frame.header) == 2 && info->info.fmt.channels == 2) {
        while (info->decoded_samples_remaining > 0 && info->bytes_to_decode > 0) {
            *((int16_t*)info->out) = MadFixedToSshort (info->mad_synth.pcm.samples[0][idx]);
            info->bytes_to_decode -= 2;
            info->out += 2;
            *((int16_t*)info->out) = MadFixedToSshort (info->mad_synth.pcm.samples[1][idx]);
            info->bytes_to_decode -= 2;
            info->out += 2;
            info->decoded_samples_remaining--;
            idx++;
        }
    }
    // mono
    else if (MAD_NCHANNELS(&info->mad_frame.header) == 1 && info->info.fmt.channels == 1){
        while (info->decoded_samples_remaining > 0 && info->bytes_to_decode > 0) {
            *((int16_t*)info->out) = MadFixedToSshort (info->mad_synth.pcm.samples[0][idx]);
            info->bytes_to_decode -= 2;
            info->out += 2;
            info->decoded_samples_remaining--;
            idx++;
        }
    }
    // workaround for bad mp3s that have both mono and stereo frames
    else if (MAD_NCHANNELS(&info->mad_frame.header) == 1 && info->info.fmt.channels == 2) {
        while (info->decoded_samples_remaining > 0 && info->bytes_to_decode > 0) {
            int16_t sample = MadFixedToSshort (info->mad_synth.pcm.samples[0][idx]);
            *((int16_t*)info->out) = sample;
            info->bytes_to_decode -= 2;
            info->out += 2;
            *((int16_t*)info->out) = sample;
            info->bytes_to_decode -= 2;
            info->out += 2;
            info->decoded_samples_remaining--;
            idx++;
        }
    }
    else if (MAD_NCHANNELS(&info->mad_frame.header) == 2 && info->info.fmt.channels == 1) {
        while (info->decoded_samples_remaining > 0 && info->bytes_to_decode > 0) {
            int16_t sample = MadFixedToSshort (info->mad_synth.pcm.samples[0][idx]);
            *((int16_t*)info->out) = sample;
            info->bytes_to_decode -= 2;
            info->out += 2;
            info->decoded_samples_remaining--;
            idx++;
        }
    }
}

void
mp3_mad_consume_decoded_data (mp3_info_t *info) {
    // copy synthesized samples into readbuffer
    int idx = info->mad_synth.pcm.length-info->decoded_samples_remaining;
    // stereo
    if (MAD_NCHANNELS(&info->mad_frame.header) == 2 && info->info.fmt.channels == 2) {
        while (info->decoded_samples_remaining > 0 && info->bytes_to_decode > 0) {
            *((float*)info->out) = MadFixedToFloat (info->mad_synth.pcm.samples[0][idx]);
            info->bytes_to_decode -= 4;
            info->out += 4;
            *((float*)info->out) = MadFixedToFloat (info->mad_synth.pcm.samples[1][idx]);
            info->bytes_to_decode -= 4;
            info->out += 4;
            info->decoded_samples_remaining--;
            idx++;
        }
    }
    // mono
    else if (MAD_NCHANNELS(&info->mad_frame.header) == 1 && info->info.fmt.channels == 1){
        while (info->decoded_samples_remaining > 0 && info->bytes_to_decode > 0) {
            *((float*)info->out) = MadFixedToFloat (info->mad_synth.pcm.samples[0][idx]);
            info->bytes_to_decode -= 4;
            info->out += 4;
            info->decoded_samples_remaining--;
            idx++;
        }
    }
    // workaround for bad mp3s that have both mono and stereo frames
    else if (MAD_NCHANNELS(&info->mad_frame.header) == 1 && info->info.fmt.channels == 2) {
        while (info->decoded_samples_remaining > 0 && info->bytes_to_decode > 0) {
            int16_t sample = MadFixedToFloat (info->mad_synth.pcm.samples[0][idx]);
            *((float*)info->out) = sample;
            info->bytes_to_decode -= 4;
            info->out += 4;
            *((float*)info->out) = sample;
            info->bytes_to_decode -= 4;
            info->out += 4;
            info->decoded_samples_remaining--;
            idx++;
        }
    }
    else if (MAD_NCHANNELS(&info->mad_frame.header) == 2 && info->info.fmt.channels == 1) {
        while (info->decoded_samples_remaining > 0 && info->bytes_to_decode > 0) {
            float sample = MadFixedToFloat (info->mad_synth.pcm.samples[0][idx]);
            *((float*)info->out) = sample;
            info->bytes_to_decode -= 4;
            info->out += 4;
            info->decoded_samples_remaining--;
            idx++;
        }
    }
}

int
mp3_mad_decode_next_packet (mp3_info_t *info) {
    int eof = 0;
    while (!eof && (info->mad_stream.buffer == NULL || info->decoded_samples_remaining <= 0)) {
        // read more MPEG data if needed
        if(info->mad_stream.buffer==NULL || info->mad_stream.error==MAD_ERROR_BUFLEN) {
            // copy part of last frame to beginning
            if (info->mad_stream.next_frame && info->mad_stream.bufend <= info->mad_stream.next_frame) {
                eof = 1;
                break;
            }
            if (info->mad_stream.next_frame != NULL) {
                info->input_remaining_bytes = info->mad_stream.bufend - info->mad_stream.next_frame;
                memmove (info->input, info->mad_stream.next_frame, info->input_remaining_bytes);
            }
            int size = READBUFFER - info->input_remaining_bytes;
            int bytesread = 0;
            uint8_t *bytes = info->input + info->input_remaining_bytes;
            bytesread = deadbeef->fread (bytes, 1, size, info->file);
            if (!bytesread) {
                // add guard
                eof = 1;
                memset (bytes, 0, 8);
                bytesread = 8;
            }
            if (bytesread < size) {
                // end of file
                size -= bytesread;
                bytes += bytesread;
            }
            bytesread += info->input_remaining_bytes;
            mad_stream_buffer(&info->mad_stream,info->input,bytesread);
            if (info->mad_stream.buffer==NULL) {
                // check sync bits
                if (bytes[0] != 0xff || (bytes[1]&(3<<5)) != (3<<5)) {
                    trace ("mp3: read didn't start at frame boundary!\ncmp3_scan_stream is broken\n");
                }
                else {
                    trace ("mp3: streambuffer=NULL\n");
                }
            }
        }
        info->mad_stream.error=0;

        // decode next frame
        if(mad_frame_decode(&info->mad_frame, &info->mad_stream))
        {
            if(MAD_RECOVERABLE(info->mad_stream.error))
            {
                if(info->mad_stream.error!=MAD_ERROR_LOSTSYNC) {
                    //                    printf ("mp3: recoverable frame level error (%s)\n", MadErrorString(&info->stream));
                }
                continue;
            }
            else {
                if(info->mad_stream.error==MAD_ERROR_BUFLEN) {
                    //                    printf ("mp3: recoverable frame level error (%s)\n", MadErrorString(&info->stream));
                    continue;
                }
                else
                {
                    //                    printf ("mp3: unrecoverable frame level error (%s).\n", MadErrorString(&info->stream));
                    return -1; // fatal error
                }
            }
        }
        mad_synth_frame(&info->mad_synth,&info->mad_frame);

        info->info.fmt.samplerate = info->mad_frame.header.samplerate;

        // synthesize single frame
        info->decoded_samples_remaining = info->mad_synth.pcm.length;
        deadbeef->streamer_set_bitrate (info->mad_frame.header.bitrate/1000);
        break;
    }
    
    return eof;
}

mp3_decoder_api_t mad_api = {
    .init = mp3_mad_init,
    .free = mp3_mad_free,
    .consume_decoded_data = mp3_mad_consume_decoded_data,
    .decode_next_packet = mp3_mad_decode_next_packet,
};
