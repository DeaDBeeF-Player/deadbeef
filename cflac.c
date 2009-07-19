#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <FLAC/stream_decoder.h>
#include "codec.h"
#include "cflac.h"
#include "common.h"
#include "playlist.h"

static FLAC__StreamDecoder *decoder = 0;
#define BUFFERSIZE 40000
static char buffer[BUFFERSIZE];
static int remaining; // bytes remaining in buffer from last read
static float timestart;
static float timeend;

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

void
cflac_free (void);

int
cflac_init (const char *fname, int track, float start, float end) {
    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        printf ("FLAC__stream_decoder_new failed\n");
        return -1;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    status = FLAC__stream_decoder_init_file(decoder, fname, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, NULL);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        cflac_free ();
        return -1;
    }
    cflac.info.samplesPerSecond = -1;
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder)) {
        cflac_free ();
        return -1;
    }
    if (cflac.info.samplesPerSecond == -1) { // not a FLAC stream
        cflac_free ();
        return -1;
    }
    timestart = start;
    timeend = end;
    if (timeend > timestart || timeend < 0) {
        // take from cue and seek
        if (timeend < 0) {
            // must be last (or broken) track
            timeend = cflac.info.duration; // set from metainfo
        }
        cflac.info.duration = timeend - timestart;
        cflac.seek (0);
    }
    if (cflac.info.duration == -1) {
        printf ("FLAC duration calculation failed\n");
        return -1;
    }
    else {
        printf ("%s duration %f, start %f (%f), end %f (%f)\n", fname, cflac.info.duration, timestart, start, timeend, end);
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
    int initsize = size;
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
            cflac.info.position += (float)sz / (cflac.info.channels * cflac.info.samplesPerSecond * cflac.info.bitsPerSample / 8);
            if (timeend > timestart) {
                if (cflac.info.position + timestart > timeend) {
                    break;
                }
            }
        }
        if (!size) {
            break;
        }
        if (!FLAC__stream_decoder_process_single (decoder)) {
            break;
//            memset (bytes, 0, size);
//            return -1;
        }
        if (FLAC__stream_decoder_get_state (decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
            break;
//            return -1;
        }
    } while (size > 0);

    return initsize - size;
}

int
cflac_seek (float time) {
    time += timestart;
    if (!FLAC__stream_decoder_seek_absolute (decoder, (FLAC__uint64)(time * cflac.info.samplesPerSecond))) {
        return -1;
    }
    remaining = 0;
    cflac.info.position = time - timestart;
    return 0;
}

int
cflac_add (const char *fname) {
    // try cue
    char cuename[1024];
    snprintf (cuename, 1024, "%s.cue", fname);
//    printf ("loading %s\n", cuename);
    if (!ps_add_cue (cuename)) {
        return 0;
    }
    int n = strlen (fname) - 4;
    if (n > 0) {
        strncpy (cuename, fname, n);
        strcpy (cuename + n, "cue");
    //    printf ("loading %s\n", cuename);
        if (!ps_add_cue (cuename)) {
            return 0;
        }
    }

    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        printf ("FLAC__stream_decoder_new failed\n");
        return -1;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    status = FLAC__stream_decoder_init_file(decoder, fname, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, NULL);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        cflac_free ();
        return -1;
    }
    cflac.info.samplesPerSecond = -1;
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder)) {
        cflac_free ();
        return -1;
    }
    if (cflac.info.samplesPerSecond == -1) { // not a FLAC stream
        cflac_free ();
        return -1;
    }
    cflac_free ();
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->codec = &cflac;
    it->fname = strdup (fname);
    it->tracknum = 0;
    it->timestart = 0;
    it->timeend = 0;
    it->displayname = strdup (fname);
    ps_append_item (it);
    return 0;
}

static const char * exts[]=
{
	"flac","ogg",NULL
};

const char **cflac_getexts (void) {
    return exts;
}

codec_t cflac = {
    .init = cflac_init,
    .free = cflac_free,
    .read = cflac_read,
    .seek = cflac_seek,
    .add = cflac_add,
    .getexts = cflac_getexts
};
