//
//  aac_decoder_faad2.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/4/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <neaacdec.h>

#include "aac_decoder_faad2.h"

typedef struct aacDecoder_s {
    aacDecoderHandle_t base;
    NeAACDecHandle dec;
} faad2Decoder_t;

static void
aacDecoderClose_FAAD2 (aacDecoderHandle_t *_dec) {
    faad2Decoder_t *dec = (faad2Decoder_t *)_dec;
    NeAACDecClose (dec->dec);
    free (_dec);
}

static int
aacDecoderInit_FAAD2 (aacDecoderHandle_t *_dec, uint8_t *buff, size_t buffSize, unsigned *samplerate, unsigned *channels) {
    faad2Decoder_t *dec = (faad2Decoder_t *)_dec;
    unsigned long sr;
    unsigned char ch;
    long res = NeAACDecInit2(dec->dec, buff, (unsigned long)buffSize, &sr, &ch);
    if (res < 0) {
        return -1;
    }
    *samplerate = (unsigned)sr;
    *channels = (unsigned)ch;
    return 0;
}

static int
aacDecoderInitRaw_FAAD2 (aacDecoderHandle_t *_dec, uint8_t *buff, size_t buffSize, unsigned *samplerate, unsigned *channels) {
    faad2Decoder_t *dec = (faad2Decoder_t *)_dec;
    unsigned long sr;
    unsigned char ch;
    long res = NeAACDecInit(dec->dec, buff, (unsigned long)buffSize, &sr, &ch);
    if (res < 0) {
        return -1;
    }
    *samplerate = (unsigned)sr;
    *channels = (unsigned)ch;
    return 0;
}

static const uint8_t *
aacDecoderGetASC_FAAD2 (aacDecoderHandle_t *_dec) {
    faad2Decoder_t *dec = (faad2Decoder_t *)_dec;
    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration (dec->dec);
    return (uint8_t *)conf;
}

static int
aacDecoderSetASC_FAAD2 (aacDecoderHandle_t *_dec, const uint8_t *asc) {
    faad2Decoder_t *dec = (faad2Decoder_t *)_dec;
    long res = NeAACDecSetConfiguration (dec->dec, (NeAACDecConfigurationPtr)asc);
    if (res < 0) {
        return -1;
    }
    return 0;
}

static uint8_t *
ascDecoderDecodeFrame_FAAD2 (aacDecoderHandle_t *_dec, aacDecoderFrameInfo_t *frameInfo, const uint8_t *buffer, size_t bufferSize) {
    faad2Decoder_t *dec = (faad2Decoder_t *)_dec;
    NeAACDecFrameInfo fi = {0};
    void *samples = NeAACDecDecode(dec->dec, &fi, (unsigned char *)buffer, (unsigned long)bufferSize);

    frameInfo->bytesconsumed = fi.bytesconsumed;
    frameInfo->samples = fi.samples;
    frameInfo->channels = fi.channels;
    memcpy (frameInfo->channel_position, fi.channel_position, sizeof (fi.channel_position));

    return (uint8_t *)samples;
}

static aacDecoderCallbacks_t aacDecoderCallbacks_FAAD2 = {
    .close = aacDecoderClose_FAAD2,
    .init = aacDecoderInit_FAAD2,
    .initRaw = aacDecoderInitRaw_FAAD2,
    .decodeFrame = ascDecoderDecodeFrame_FAAD2,
};

aacDecoderHandle_t *
aacDecoderOpenFAAD2 (void) {
    faad2Decoder_t *dec = calloc (sizeof (faad2Decoder_t), 1);
    dec->base.callbacks = &aacDecoderCallbacks_FAAD2;
    dec->dec = NeAACDecOpen();
    return &dec->base;
}
