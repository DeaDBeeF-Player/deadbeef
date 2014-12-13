
#ifndef __FLAC_H
#define __FLAC_H

#include "encode.h"
#include "audio.h"
#include <stdio.h>
#include <FLAC/stream_decoder.h>
#if !defined(FLAC_API_VERSION_CURRENT) || (FLAC_API_VERSION_CURRENT < 8)
#define NEED_EASYFLAC 1
#endif
#if NEED_EASYFLAC
#include <OggFLAC/stream_decoder.h>
#include "easyflac.h"
#endif

typedef struct {
#if NEED_EASYFLAC
    EasyFLAC__StreamDecoder *decoder;
#else
    FLAC__StreamDecoder *decoder;
#endif
    short channels;
    int rate;
    long totalsamples; /* per channel, of course */

    FLAC__StreamMetadata *comments;

    FILE *in;  /* Cache the FILE pointer so the FLAC read callback can use it */
    int eos;  /* End of stream read */


    /* Buffer for decoded audio */
    float **buf;  /* channels by buf_len array */
    int buf_len;
    int buf_start; /* Offset to start of audio data */
    int buf_fill; /* Number of bytes of audio data in buffer */

    /* Buffer for input data we already read in the id phase */
    unsigned char *oldbuf;
    int oldbuf_len;
    int oldbuf_start;
} flacfile;


int flac_id(unsigned char *buf, int len);
int oggflac_id(unsigned char *buf, int len);
int flac_open(FILE *in, oe_enc_opt *opt, unsigned char *buf, int buflen);
void flac_close(void *);

long flac_read(void *, float **buffer, int samples);

#endif /* __FLAC_H */

