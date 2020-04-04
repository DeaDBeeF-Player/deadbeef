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
    unsigned char error;
    unsigned long samplerate;

    /* SBR: 0: off, 1: on; upsample, 2: on; downsampled, 3: off; upsampled */
    unsigned char sbr;

    /* MPEG-4 ObjectType */
    unsigned char object_type;

    /* AAC header type; MP4 will be signalled as RAW also */
    unsigned char header_type;

    /* multichannel configuration */
    unsigned char num_front_channels;
    unsigned char num_side_channels;
    unsigned char num_back_channels;
    unsigned char num_lfe_channels;
    unsigned char channel_position[64];

    /* PS: 0: off, 1: on */
    unsigned char ps;
} aacDecoderFrameInfo_t;

struct aacDecoderHandle_s;

typedef struct {
    void (*close) (struct aacDecoderHandle_s *dec);

    int (*init) (struct aacDecoderHandle_s * dec, uint8_t *buff, size_t buffSize, unsigned *samplerate, unsigned *channels);

    const uint8_t * (*getASC) (struct aacDecoderHandle_s *dec);

    int (*setASC) (struct aacDecoderHandle_s *dec, const uint8_t *asc);

    uint8_t * (*decodeFrame) (struct aacDecoderHandle_s *dec, aacDecoderFrameInfo_t *frameInfo, const uint8_t *buffer, size_t bufferSize);
} aacDecoderCallbacks_t;

typedef struct aacDecoderHandle_s {
    aacDecoderCallbacks_t *callbacks;
} aacDecoderHandle_t;

void
aacDecoderClose (aacDecoderHandle_t *dec);

int
aacDecoderInit (aacDecoderHandle_t *dec, uint8_t *buff, size_t buffSize, unsigned *samplerate, unsigned *channels);

const uint8_t *
aacDecoderGetASC (aacDecoderHandle_t *dec);

int
aacDecoderSetASC (aacDecoderHandle_t *dec, const uint8_t *asc);

uint8_t *
ascDecoderDecodeFrame (aacDecoderHandle_t *dec, aacDecoderFrameInfo_t *frameInfo, const uint8_t *buffer, size_t bufferSize);


#endif /* aac_decoder_protocol_h */
