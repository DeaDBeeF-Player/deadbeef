//
//  aac_decoder_protocol.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 4/4/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#ifndef aac_decoder_protocol_h
#define aac_decoder_protocol_h

#include <stdint.h>

typedef struct {
    unsigned long bytesconsumed;
    unsigned long samples;
    unsigned char channels;
    unsigned char channel_position[64];
} aacDecoderFrameInfo_t;

struct aacDecoderHandle_s;

typedef struct {
    void (*close) (struct aacDecoderHandle_s *dec);

    int (*init) (struct aacDecoderHandle_s * dec, uint8_t *buff, size_t buffSize, unsigned *samplerate, unsigned *channels);

    uint8_t * (*decodeFrame) (struct aacDecoderHandle_s *dec, aacDecoderFrameInfo_t *frameInfo, const uint8_t *buffer, size_t bufferSize);
} aacDecoderCallbacks_t;

typedef struct aacDecoderHandle_s {
    aacDecoderCallbacks_t *callbacks;
} aacDecoderHandle_t;

void
aacDecoderClose (aacDecoderHandle_t *dec);

int
aacDecoderInit (aacDecoderHandle_t *dec, uint8_t *buff, size_t buffSize, unsigned *samplerate, unsigned *channels);

uint8_t *
aacDecoderDecodeFrame (aacDecoderHandle_t *dec, aacDecoderFrameInfo_t *frameInfo, const uint8_t *buffer, size_t bufferSize);


#endif /* aac_decoder_protocol_h */
