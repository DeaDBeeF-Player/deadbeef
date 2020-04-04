//
//  aac_decoder_FDK.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/4/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include <stdlib.h>
#include "aac_decoder_fdk.h"
#include "aacdecoder_lib.h"

typedef struct aacDecoder_s {
    aacDecoderHandle_t base;
    HANDLE_AACDECODER dec;
} fdkDecoder_t;

void
aacDecoderClose_FDK (aacDecoderHandle_t *_dec) {
    fdkDecoder_t *dec = (fdkDecoder_t *)_dec;
    aacDecoder_Close (dec->dec);
    free (_dec);
}

int
aacDecoderInit_FDK (aacDecoderHandle_t *_dec, uint8_t *buff, size_t buffSize, unsigned *samplerate, unsigned *channels) {
//    fdkDecoder_t *dec = (fdkDecoder_t *)_dec;

    // TODO: fetch samplerate and number of channels

    return 0;
}

const uint8_t *
aacDecoderGetASC_FDK (aacDecoderHandle_t *_dec) {
//    fdkDecoder_t *dec = (fdkDecoder_t *)_dec;

    // TODO: can FDK do that?
    return NULL;
}

int
aacDecoderSetASC_FDK (aacDecoderHandle_t *_dec, const uint8_t *asc) {
//    fdkDecoder_t *dec = (fdkDecoder_t *)_dec;
//    AAC_DECODER_ERROR err = aacDecoder_ConfigRaw(dec->dec, asc, asc_size);
//    if (err != AAC_DEC_OK) {
//        return -1;
//    }

    return 0;
}

uint8_t *
ascDecoderDecodeFrame_FDK (aacDecoderHandle_t *_dec, aacDecoderFrameInfo_t *frameInfo, const uint8_t *buffer, size_t bufferSize) {
    fdkDecoder_t *dec = (fdkDecoder_t *)_dec;

    UINT bytesValid = 0;
    AAC_DECODER_ERROR err = aacDecoder_Fill(dec->dec, (UCHAR **)&buffer, (UINT *)bufferSize, &bytesValid);
    if (err != AAC_DEC_OK) {
        return NULL;
    }

    INT_PCM *outBuffer = NULL;
    size_t outBufferSize = 0;
    err = aacDecoder_DecodeFrame(dec->dec, (INT_PCM *)outBuffer, (INT)(outBufferSize/sizeof(INT_PCM)), AACDEC_FLUSH);

    // FIXME: convert "fi" into FrameInfo

    return (uint8_t *)outBuffer;
}

static aacDecoderCallbacks_t aacDecoderCallbacks_FDK = {
    .close = aacDecoderClose,
    .init = aacDecoderInit,
    .getASC = aacDecoderGetASC,
    .setASC = aacDecoderSetASC,
    .decodeFrame = ascDecoderDecodeFrame,
};

aacDecoderHandle_t *
aacDecoderOpenfdk (void) {
    fdkDecoder_t *dec = calloc (sizeof (fdkDecoder_t), 1);
    dec->base.callbacks = &aacDecoderCallbacks_FDK;
    dec->dec = aacDecoder_Open(TT_MP4_RAW, 1);
    return &dec->base;
}
