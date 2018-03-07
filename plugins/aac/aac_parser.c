/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdio.h>
#include "aac_parser.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static const int aac_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

static const int aac_channels[8] = {
    0, 1, 2, 3, 4, 5, 6, 8
};

int
aac_sync(const uint8_t *buf, int *channels, int *sample_rate, int *bit_rate, int *samples)
{
    int size, rdb;

    // 12 sync bits
    if (buf[0] != 0xff || (buf[1]&0xf0) != 0xf0) {
        //trace ("unsync\n");
        return 0;
    }

    int id = (buf[1] & 0x08) >> 7;

    int version = id == 0 ? 4 : 2;

    int layer = (buf[1] & 0x06) >> 5;
    int protection_abs = (buf[1] & 0x01);

    int header_size = protection_abs > 0 ? 7 : 9;

    int profile_objecttype = (buf[2] & 0xC0) >> 6;

    const char *profiles[4] = {
        "0 Main profile AAC MAIN",
        "1 Low Complexity profile (LC)AAC LC",
        "2 Scalable Sample Rate profile (SSR)AAC SSR",
        "3 (reserved)AAC LTP"
    };
    trace ("profile: %s\n", profiles[profile_objecttype]);

    int sample_freq_index = (buf[2] & 0x3C) >> 2;
    if (!aac_sample_rates[sample_freq_index]) {
        //trace ("invalid samplerate\n");
        return 0;
    }
    trace ("samplerate %d (#%d)\n", aac_sample_rates[sample_freq_index], sample_freq_index);
    int private_bit = (buf[2] & 0x02) >> 1;

    int channel_conf = ((buf[2] & 0x01) << 2) | ((buf[3] & 0xC0) >> 6);
    if (!aac_channels[channel_conf]) {
        //trace ("invalid channels\n");
        return 0;
    }
    trace ("channels %d (#%d)\n", aac_channels[channel_conf], channel_conf);
    int orig_copy = (buf[3] & 0x20) >> 5;
    int home = (buf[3] & 0x10) >> 4;
    int copyright_ident_bit = (buf[3] & 0x08) >> 3;
    int copyright_ident_start = (buf[3] & 0x04) >> 2;
    size = ((buf[3] & 0x03) << 11) | (buf[4] << 3) | ((buf[5] & 0xE0) >> 5);
    if(size < ADTS_HEADER_SIZE) {
        //trace ("invalid size\n");
        return 0;
    }
    int adts_buffer_fullness = ((buf[5] & 0x1F) << 3) | ((buf[6] & 0xFC) >> 2);
    rdb = (buf[6] & 0x03) + 1;
    trace ("rdb: %d\n", rdb);

    *channels = aac_channels[channel_conf];
    *sample_rate = aac_sample_rates[sample_freq_index];
    *samples = rdb * 1024;
    if (*channels <= 0 || *sample_rate <= 0 || *samples <= 0) {
        return 0;
    }
    *bit_rate = size * 8 * *sample_rate / *samples;

    return size;
}

