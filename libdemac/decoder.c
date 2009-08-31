/*

libdemac - A Monkey's Audio decoder

$Id: decoder.c 19552 2008-12-21 23:49:02Z amiconn $

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#include <inttypes.h>
#include <string.h>

#include "demac.h"
#include "predictor.h"
#include "entropy.h"
#include "filter.h"
#include "demac_config.h"

/* Statically allocate the filter buffers */

static filter_int filterbuf32[(32*3 + FILTER_HISTORY_SIZE) * 2]   
                  IBSS_ATTR __attribute__((aligned(16))); /* 2432/4864 bytes */
static filter_int filterbuf256[(256*3 + FILTER_HISTORY_SIZE) * 2]
                  IBSS_ATTR __attribute__((aligned(16))); /* 5120/10240 bytes */

/* This is only needed for "insane" files, and no current Rockbox targets
   can hope to decode them in realtime, although the Gigabeat S comes close. */
static filter_int filterbuf1280[(1280*3 + FILTER_HISTORY_SIZE) * 2] 
                  IBSS_ATTR_DEMAC_INSANEBUF __attribute__((aligned(16)));
                  /* 17408 or 34816 bytes */

void init_frame_decoder(struct ape_ctx_t* ape_ctx,
                        unsigned char* inbuffer, int* firstbyte,
                        int* bytesconsumed)
{
    init_entropy_decoder(ape_ctx, inbuffer, firstbyte, bytesconsumed);
    //printf("CRC=0x%08x\n",ape_ctx->CRC);
    //printf("Flags=0x%08x\n",ape_ctx->frameflags);

    init_predictor_decoder(&ape_ctx->predictor);

    switch (ape_ctx->compressiontype)
    {
        case 2000:
            init_filter_16_11(filterbuf32);
            break;

        case 3000:
            init_filter_64_11(filterbuf256);
            break;

        case 4000:
            init_filter_256_13(filterbuf256);
            init_filter_32_10(filterbuf32);
            break;

        case 5000:
            init_filter_1280_15(filterbuf1280);
            init_filter_256_13(filterbuf256);
            init_filter_16_11(filterbuf32);
    }
}

int ICODE_ATTR_DEMAC decode_chunk(struct ape_ctx_t* ape_ctx,
                                  unsigned char* inbuffer, int* firstbyte,
                                  int* bytesconsumed,
                                  int32_t* decoded0, int32_t* decoded1,
                                  int count)
{
    int32_t left, right;
#ifdef ROCKBOX
    int scale = (APE_OUTPUT_DEPTH - ape_ctx->bps);
    #define SCALE(x) ((x) << scale)
#else
    #define SCALE(x) (x)
#endif
         
    if ((ape_ctx->channels==1) || ((ape_ctx->frameflags
        & (APE_FRAMECODE_PSEUDO_STEREO|APE_FRAMECODE_STEREO_SILENCE))
        == APE_FRAMECODE_PSEUDO_STEREO)) {

        entropy_decode(ape_ctx, inbuffer, firstbyte, bytesconsumed,
                       decoded0, NULL, count);

        if (ape_ctx->frameflags & APE_FRAMECODE_MONO_SILENCE) {
            /* We are pure silence, so we're done. */
            return 0;
        }

        switch (ape_ctx->compressiontype)
        {
            case 2000:
                apply_filter_16_11(ape_ctx->fileversion,decoded0,NULL,count);
                break;
    
            case 3000:
                apply_filter_64_11(ape_ctx->fileversion,decoded0,NULL,count);
                break;
    
            case 4000:
                apply_filter_32_10(ape_ctx->fileversion,decoded0,NULL,count);
                apply_filter_256_13(ape_ctx->fileversion,decoded0,NULL,count);
                break;
    
            case 5000:
                apply_filter_16_11(ape_ctx->fileversion,decoded0,NULL,count);
                apply_filter_256_13(ape_ctx->fileversion,decoded0,NULL,count);
                apply_filter_1280_15(ape_ctx->fileversion,decoded0,NULL,count);
        }

        /* Now apply the predictor decoding */
        predictor_decode_mono(&ape_ctx->predictor,decoded0,count);

        if (ape_ctx->channels==2) {
            /* Pseudo-stereo - copy left channel to right channel */
            while (count--)
            {
                left = *decoded0;
                *(decoded1++) = *(decoded0++) = SCALE(left);
            }
        }
#ifdef ROCKBOX
         else {
            /* Scale to output depth */
            while (count--)
            {
                left = *decoded0;
                *(decoded0++) = SCALE(left);
            }
        }
#endif
    } else { /* Stereo */
        entropy_decode(ape_ctx, inbuffer, firstbyte, bytesconsumed,
                       decoded0, decoded1, count);

        if ((ape_ctx->frameflags & APE_FRAMECODE_STEREO_SILENCE)
            == APE_FRAMECODE_STEREO_SILENCE) {
            /* We are pure silence, so we're done. */
            return 0;
        }

        /* Apply filters - compression type 1000 doesn't have any */
        switch (ape_ctx->compressiontype)
        {
            case 2000:
                apply_filter_16_11(ape_ctx->fileversion,decoded0,decoded1,count);
                break;
    
            case 3000:
                apply_filter_64_11(ape_ctx->fileversion,decoded0,decoded1,count);
                break;
    
            case 4000:
                apply_filter_32_10(ape_ctx->fileversion,decoded0,decoded1,count);
                apply_filter_256_13(ape_ctx->fileversion,decoded0,decoded1,count);
                break;
    
            case 5000:
                apply_filter_16_11(ape_ctx->fileversion,decoded0,decoded1,count);
                apply_filter_256_13(ape_ctx->fileversion,decoded0,decoded1,count);
                apply_filter_1280_15(ape_ctx->fileversion,decoded0,decoded1,count);
        }

        /* Now apply the predictor decoding */
        predictor_decode_stereo(&ape_ctx->predictor,decoded0,decoded1,count);

        /* Decorrelate and scale to output depth */
        while (count--)
        {
            left = *decoded1 - (*decoded0 / 2);
            right = left + *decoded0;

            *(decoded0++) = SCALE(left);
            *(decoded1++) = SCALE(right);
        }
    }
    return 0;
}
