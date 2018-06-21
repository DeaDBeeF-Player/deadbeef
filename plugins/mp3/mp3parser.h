/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Alexey Yakovenko and other contributors

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

#ifndef mp3parser_h
#define mp3parser_h

#include <stdint.h>
#include "../../deadbeef.h"

enum {
    MP3_PARSE_FULLSCAN = 1,
};

typedef struct {
    uint64_t offs;
    int ver;
    int samplerate;
    int bitrate;
    int nchannels;
    int samples_per_frame;
    int layer;
    int packetlength;
} mp3packet_t;

typedef struct {
    // outputs
    int64_t pcmsample; // sample position corresponding to packet_offs
    int64_t npackets;
    int64_t packet_offs; // stream position of the first known valid frame after successful scan

    int lastpacket_valid;
    int64_t valid_packets;

    mp3packet_t ref_packet; // packet representing the stream format

    int have_xing_header;
    int vbr_type;

    float avg_packetlength;
    int avg_samplerate;
    int avg_samples_per_frame;

    int64_t skipsamples; // how many samples to skip after seek

    int delay;
    int padding;

    uint16_t lamepreset;
    uint32_t lame_musiclength; // file size from beginning of LAME info packet until the last byte of packet with audio, as encoded by Lame

    uint64_t fsize;

    // intermediates
    mp3packet_t prev_packet;

    int checked_xing_header;
} mp3info_t;

// Params:
// seek_to_sample: -1 means to the end (scan whole file), otherwise a sample to seek to
// When seeking, the packet offset returned will be the one containing seek_to_sample, not accounting for delay.
// `totalsamples` will be set to the sample count at the packet start.
// So the caller is responsible for adding delay to seek_to_sample, and then adjusting skipsamples based on returned "totalsamples".
//
//
// returns:
// 0: success
// -1: error
//
// the caller is supposed to start decoding from info->packet_offs, and skip info->skipsamples samples
int
mp3_parse_file (mp3info_t *info, uint32_t flags, DB_FILE *fp, int64_t fsize, int startoffs, int endoffs, int64_t seek_to_sample);

#endif /* mp3parser_h */
