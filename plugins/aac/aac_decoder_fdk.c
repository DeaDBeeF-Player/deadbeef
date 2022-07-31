//
//  aac_decoder_FDK.c
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/4/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
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
aacDecoderInit_FDK (aacDecoderHandle_t *_dec, uint8_t *asc, size_t ascSize, unsigned *samplerate, unsigned *channels) {
    fdkDecoder_t *dec = (fdkDecoder_t *)_dec;

    UINT fdkASCSize = (UINT)ascSize;
    AAC_DECODER_ERROR err = aacDecoder_ConfigRaw(dec->dec, &asc, &fdkASCSize);
    if (err != AAC_DEC_OK) {
        return -1;
    }


    CStreamInfo* stream_info = aacDecoder_GetStreamInfo(dec->dec);
    if (stream_info->extSamplingRate) {
        *samplerate = stream_info->extSamplingRate;
    }
    else if (stream_info->sampleRate) {
        *samplerate = stream_info->sampleRate;
    }
    else if (stream_info->aacSampleRate) {
        *samplerate = stream_info->aacSampleRate;
    }
    else {
        return -1;
    }

    // TODO: fetch samplerate and number of channels
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
    .initRaw = aacDecoderInit,
    .decodeFrame = aacDecoderDecodeFrame,
};

aacDecoderHandle_t *
aacDecoderOpenfdk (void) {
    fdkDecoder_t *dec = calloc (1, sizeof (fdkDecoder_t));
    dec->base.callbacks = &aacDecoderCallbacks_FDK;
    dec->dec = aacDecoder_Open(TT_MP4_RAW, 1);
    return &dec->base;
}
