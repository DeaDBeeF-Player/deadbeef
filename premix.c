/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "deadbeef.h"
#include "premix.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

void
pcm_write_sample (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output) {
    memcpy (output, input, 2);
}

int
pcm_convert (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int inputsize) {
    // calculate output size
    int inputsamplesize = (inputfmt->bps >> 3) * inputfmt->channels;
    int outputsamplesize = (outputfmt->bps >> 3) * outputfmt->channels;
    int nsamples = inputsize / inputsamplesize;

    uint32_t outchannels = 0;

    if (output) {
        // build channelmap
        int channelmap[32] = {-1};
        uint32_t inputbitmask = 1;
        for (int i = 0; i < inputfmt->channels; i++) {
            // find next input channel
            while (inputbitmask < 0x80000000 && !(inputfmt->channelmask & inputbitmask)) {
                inputbitmask <<= 1;
            }
            if (!(inputfmt->channelmask & inputbitmask)) {
                trace ("pcm_convert: channelmask doesn't correspond inputfmt (channels=%d, channelmask=%X)!", inputfmt->channels, inputfmt->channelmask);
                break;
            }
            if (outputfmt->channelmask & inputbitmask) {
                int o = 0;
                uint32_t outputbitmask = 1;
                while (outputbitmask < 0x80000000 && (outputfmt->channelmask & outputbitmask) != inputbitmask) {
                    outputbitmask <<= 1;
                    o++;
                }
                if (!(inputfmt->channelmask & outputbitmask)) {
                    // no corresponding output channel -- ignore
                    continue;
                }
                outchannels |= outputbitmask;
                channelmap[i] = o; // input channel i going to output channel o
                trace ("channelmap[%d]=%d\n", i, o);
            }
            inputbitmask <<= 1;
        }

        if (outchannels != outputfmt->channelmask) {
            // some of the channels are not used
            memset (output, 0, nsamples * outputsamplesize);
        }
        for (int s = 0; s < nsamples; s++) {
            for (int c = 0; c < inputfmt->channels; c++) {
                if (channelmap[c] != -1) {
                    pcm_write_sample (inputfmt, input, outputfmt, output + (outputfmt->bps >> 3) * channelmap[c]);
                }
                input += inputfmt->bps >> 3;
            }
            output += outputsamplesize;
        }
    }
    return nsamples * outputsamplesize;
}

