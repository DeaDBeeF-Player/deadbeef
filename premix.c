/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "optmath.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)


static inline void
pcm_write_samples_8_to_8 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            *(output + c) = *(input + channelmap[c]);
        }
        input += inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_8_to_16 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            *((int16_t*)(output + 2 * c)) = (int16_t)(*(input + channelmap[c])) << 8;
        }
        input += inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_8_to_24 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 3 * c;
            out[0] = 0;
            out[1] = 0;
            out[2] = *(input + channelmap[c]);
        }
        input += inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_8_to_32 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 4 * c;
            out[0] = 0;
            out[1] = 0;
            out[2] = 0;
            out[3] = *(input + channelmap[c]);
        }
        input += inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_8_to_float (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            float sample = (*(input+channelmap[c])) / (float)0x7f;
            *((float *)(output + 4 * c)) = sample;
        }
        input += inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_16_to_16 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            *((int16_t*)(output + 2 * c)) = *((int16_t*)(input + channelmap[c]*2));
        }
        input += 2 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_16_to_8 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            *((int8_t*)(output + c)) = *((int16_t*)(input + channelmap[c]*2)) >> 8;
        }
        input += 2 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_16_to_24 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 3 * c;
            const char *in = input + channelmap[c]*2;
            out[0] = 0;
            out[1] = in[0];
            out[2] = in[1];
        }
        input += 2 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_16_to_32 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 4 * c;
            const char *in = input + channelmap[c]*2;
            out[0] = 0;
            out[1] = 0;
            out[2] = in[0];
            out[3] = in[1];
        }
        input += 2 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_16_to_float (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            float sample = (*((int16_t*)(input + channelmap[c]*2))) / (float)0x7fff;
            *((float *)(output + 4 * c)) = sample;
        }
        input += 2 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_24_to_8 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + c;
            *out = (input + channelmap[c] * 3)[2];
        }
        input += 3 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_24_to_24 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 3 * c;
            const char *in = input + channelmap[c] * 3;
            out[0] = in[0];
            out[1] = in[1];
            out[2] = in[2];
        }
        input += 3 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_24_to_32 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 4 * c;
            const char *in = input + 3 * channelmap[c];
            out[0] = 0;
            out[1] = in[0];
            out[2] = in[1];
            out[3] = in[2];
        }
        input += inputfmt->channels * 3;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_24_to_16 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 2 * c;
            const char *in = input + 3 * channelmap[c];
            out[0] = in[1];
            out[1] = in[2];
        }
        input += inputfmt->channels * 3;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_24_to_float (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            float *out = (float *)(output + 4 * c);
            const char *in = input + 3 * channelmap[c];
            int32_t sample = ((unsigned char)in[0]) | ((unsigned char)in[1]<<8) | (in[2]<<16);
            *out = sample / (float)0x7fffff;
        }
        input += inputfmt->channels * 3;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_32_to_8 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            int8_t *out = (int8_t*)(output + c);
            int32_t sample = *((int32_t*)(input + 4 * channelmap[c]));
            *out = (int8_t)(sample>>24);
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_32_to_16 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            int16_t *out = (int16_t*)(output + 2 * c);
            int32_t sample = *((int32_t*)(input + 4 * channelmap[c]));
            *out = (int16_t)(sample>>16);
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_32_to_24 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 3 * c;
            const char *in = input + 4 * channelmap[c];
            out[0] = in[1];
            out[1] = in[2];
            out[2] = in[3];
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
}

static inline void
pcm_write_samples_32_to_32 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            *((int32_t*)(output + 4 * c)) = *((int32_t*)(input + channelmap[c] * 4));
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
}


static inline void
pcm_write_samples_32_to_float (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            float *out = (float *)(output + 4 * c);
            int32_t sample = *((int32_t*)(input + channelmap[c] * 4));
            *out = sample / (float)0x7fffffff;
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
}
static inline void
pcm_write_samples_float_to_8 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    fpu_control ctl;
    fpu_setround (&ctl);
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            int8_t *out = (int8_t*)(output + c);
            float sample = *((float*)(input + channelmap[c] * 4));
            // FIXME: sse optimize
            if (sample > 1) {
                sample = 1;
            }
            if (sample < -1) {
                sample = -1;
            }
            *out = (int8_t)ftoi (sample*0x7f);
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
    fpu_restore (ctl);
}

static inline void
pcm_write_samples_float_to_16 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    fpu_control ctl;
    fpu_setround (&ctl);
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            int16_t *out = (int16_t*)(output + 2 * c);
            float sample = *((float*)(input + 4 * channelmap[c]));
            // FIXME: sse optimize
            if (sample > 1) {
                sample = 1;
            }
            if (sample < -1) {
                sample = -1;
            }
            *out = (int16_t)ftoi (sample*0x7fff);
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
    fpu_restore (ctl);
}

static inline void
pcm_write_samples_float_to_24 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    fpu_control ctl;
    fpu_setround (&ctl);
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            char *out = output + 3 * c;
            float sample = *((float*)(input + channelmap[c] * 4));
            // FIXME: sse optimize
            if (sample > 1) {
                sample = 1;
            }
            if (sample < -1) {
                sample = -1;
            }
            int32_t outsample = (int32_t)ftoi (sample * 0x7fffff);
            out[0] = (outsample&0x0000ff);
            out[1] = (outsample&0x00ff00)>>8;
            out[2] = (outsample&0xff0000)>>16;
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
    fpu_restore (ctl);
}


static inline void
pcm_write_samples_float_to_32 (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize) {
    for (int s = 0; s < nsamples; s++) {
        for (int c = 0; c < outputfmt->channels; c++) {
            float fsample = (*((float*)(input + channelmap[c] * 4)));
            if (fsample > 0.999f) {
                fsample = 0.999f;
            }
            else if (fsample < -0.999f) {
                fsample = -0.999f;
            }
            int sample = fsample * (float)0x7fffffff;
            *((int32_t *)(output + 4 * c)) = sample;
        }
        input += 4 * inputfmt->channels;
        output += outputsamplesize;
    }
}

typedef void (*remap_fn_t) (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int nsamples, int * restrict channelmap, int outputsamplesize);


remap_fn_t remappers[8][8] = {
    {
        pcm_write_samples_8_to_8,
        pcm_write_samples_8_to_16,
        pcm_write_samples_8_to_24,
        pcm_write_samples_8_to_32,
        NULL,
        NULL,
        NULL,
        pcm_write_samples_8_to_float,
    },
    {
        pcm_write_samples_16_to_8,
        pcm_write_samples_16_to_16,
        pcm_write_samples_16_to_24,
        pcm_write_samples_16_to_32,
        NULL,
        NULL,
        NULL,
        pcm_write_samples_16_to_float,
    },
    {
        pcm_write_samples_24_to_8,
        pcm_write_samples_24_to_16,
        pcm_write_samples_24_to_24,
        pcm_write_samples_24_to_32,
        NULL,
        NULL,
        NULL,
        pcm_write_samples_24_to_float,
    },
    {
        pcm_write_samples_32_to_8,
        pcm_write_samples_32_to_16,
        pcm_write_samples_32_to_24,
        pcm_write_samples_32_to_32,
        NULL,
        NULL,
        NULL,
        pcm_write_samples_32_to_float,
    },
    {
    },
    {
    },
    {
    },
    {
        pcm_write_samples_float_to_8,
        pcm_write_samples_float_to_16,
        pcm_write_samples_float_to_24,
        pcm_write_samples_float_to_32,
        NULL,
        NULL,
        NULL,
        pcm_write_samples_32_to_32,
    }
};

int
pcm_convert (const ddb_waveformat_t * restrict inputfmt, const char * restrict input, const ddb_waveformat_t * restrict outputfmt, char * restrict output, int inputsize) {
    // calculate output size
    int inputsamplesize = (inputfmt->bps >> 3) * inputfmt->channels;
    int outputsamplesize = (outputfmt->bps >> 3) * outputfmt->channels;
    int nsamples = inputsize / inputsamplesize;

    uint32_t outchannels = 0;

    if (output) {
        // build channelmap
        int channelmap[32] = {0};
        uint32_t inputbitmask = 1;
        for (int i = 0; i < inputfmt->channels; i++) {
            // find next input channel
            while (inputbitmask < 0x80000000 && !(inputfmt->channelmask & inputbitmask)) {
                inputbitmask <<= 1;
            }
            if (!(inputfmt->channelmask & inputbitmask)) {
                trace ("pcm_convert: channelmask doesn't correspond inputfmt (channels=%d, channelmask=%X)!\n", inputfmt->channels, inputfmt->channelmask);
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
                //trace ("channelmap[%d]=%d\n", i, o);
            }
            inputbitmask <<= 1;
        }

        if (outchannels != outputfmt->channelmask) {
            // some of the channels are not used
            memset (output, 0, nsamples * outputsamplesize);
        }

        int outidx = ((outputfmt->bps >> 3) - 1) | (outputfmt->is_float << 2);
        int inidx = ((inputfmt->bps >> 3) - 1) | (inputfmt->is_float << 2);
        if (remappers[inidx][outidx]) {
            remappers[inidx][outidx] (inputfmt, input, outputfmt, output, nsamples, channelmap, outputsamplesize);
        }
        else {
            trace ("no converter from %d %s to %d %s ([%d][%d])\n", inputfmt->bps, inputfmt->is_float ? "float" : "", outputfmt->bps, outputfmt->is_float ? "float" : "", inidx, outidx);
        }
    }
    return nsamples * outputsamplesize;
}

