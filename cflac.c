#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <FLAC/stream_decoder.h>
#include "codec.h"
#include "cflac.h"
#include "common.h"

static FLAC__StreamDecoder *decoder = 0;
#define BUFFERSIZE 40000
static char buffer[BUFFERSIZE];
static int remaining; // bytes remaining in buffer from last read

FLAC__StreamDecoderWriteStatus
cflac_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    if (frame->header.blocksize == 0) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    int readbytes = frame->header.blocksize * cflac.info.channels * cflac.info.bitsPerSample / 8;
    int bufsize = BUFFERSIZE-remaining;
    int bufsamples = bufsize / (cflac.info.channels * cflac.info.bitsPerSample / 8);
    int nsamples = min (bufsamples, frame->header.blocksize);
    char *bufptr = &buffer[remaining];
    for (int i = 0; i < nsamples; i++) {
        FLAC__int16 lsample = (FLAC__int16)inputbuffer[0][i];
        ((int16_t*)bufptr)[0] = lsample;
        bufptr += cflac.info.bitsPerSample / 8;
        remaining += cflac.info.bitsPerSample / 8;
        if (cflac.info.channels > 1) {
            FLAC__int16 rsample = (FLAC__int16)inputbuffer[1][i];
            ((int16_t*)bufptr)[0] = rsample;
            bufptr += cflac.info.bitsPerSample / 8;
            remaining += cflac.info.bitsPerSample / 8;
        }
    }
    if (readbytes > bufsize) {
        printf ("flac: buffer overflow, distortion will occur\n");
    //    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void
cflac_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    FLAC__uint64 total_samples = metadata->data.stream_info.total_samples;
    int sample_rate = metadata->data.stream_info.sample_rate;
    int channels = metadata->data.stream_info.channels;
    int bps = metadata->data.stream_info.bits_per_sample;
    cflac.info.samplesPerSecond = sample_rate;
    cflac.info.channels = channels;
    cflac.info.bitsPerSample = bps;
    cflac.info.duration = total_samples / (float)sample_rate;
    cflac.info.position = 0;
}

void
cflac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    fprintf(stderr, "cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

int
cflac_init (const char *fname, int track, float start, float end) {
    printf ("cflac_init called for %s\n", fname);
    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        printf ("FLAC__stream_decoder_new failed\n");
        return -1;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    status = FLAC__stream_decoder_init_file(decoder, fname, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, NULL);
    cflac.info.duration = -1;
    cflac.info.position = 0;
    FLAC__stream_decoder_process_until_end_of_metadata (decoder);
    if (cflac.info.duration == -1) {
        printf ("FLAC duration calculation failed\n");
        return -1;
    }
    remaining = 0;
    return 0;
}

void
cflac_free (void) {
    if (decoder) {
        FLAC__stream_decoder_delete(decoder);
        decoder = NULL;
    }
}

int
cflac_read (char *bytes, int size) {
    int nsamples = size / (cflac.info.channels * cflac.info.bitsPerSample / 8);
    do {
        if (remaining) {
            int sz = min (remaining, size);
            memcpy (bytes, buffer, sz);
            if (sz < remaining) {
                memmove (buffer, &buffer[sz], remaining-sz);
            }
            remaining -= sz;
            bytes += sz;
            size -= sz;
        }
        if (!size) {
            break;
        }
        if (!FLAC__stream_decoder_process_single (decoder)) {
            memset (bytes, 0, size);
            return -1;
        }
        if (FLAC__stream_decoder_get_state (decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
            return -1;
        }
    } while (size > 0);
    cflac.info.position += (float)nsamples / cflac.info.samplesPerSecond;
    return 0;
}

int
cflac_seek (float time) {
    if (!FLAC__stream_decoder_seek_absolute (decoder, (FLAC__uint64)(time * cflac.info.samplesPerSecond))) {
        return -1;
    }
    remaining = 0;
    cflac.info.position = time;
    return 0;
}

int
cflac_add (const char *fname) {
    return 0;
}

codec_t cflac = {
    .init = cflac_init,
    .free = cflac_free,
    .read = cflac_read,
    .seek = cflac_seek,
    .add = cflac_add
};
