/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "../../deadbeef.h"
#include "dca.h"
#include "gettimeofday.h"

#ifdef WORDS_BIGENDIAN
#define s16_LE(s16,channels) s16_swap (s16, channels)
#define s16_BE(s16,channels) do {} while (0)
#define s32_LE(s32,channels) s32_swap (s32, channels)
#define s32_BE(s32,channels) do {} while (0)
#else
#define s16_LE(s16,channels) do {} while (0)
#define s16_BE(s16,channels) s16_swap (s16, channels)
#define s32_LE(s32,channels) do {} while (0)
#define s32_BE(s32,channels) s32_swap (s32, channels)
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static const char * exts[] = { "wav", NULL };
static const char *filetypes[] = { "DTS WAV", NULL };

enum {
    FT_DTSWAV,
    FT_DTS,
    FT_CPT,
};

typedef struct {
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
} wavfmt_t;

static DB_decoder_t plugin;
DB_functions_t *deadbeef;

#define BUFFER_SIZE 24576
#define OUT_BUFFER_SIZE 50000
#define HEADER_SIZE 14
typedef struct {
    DB_fileinfo_t info;
    DB_FILE *file;
    int offset;
    int startsample;
    int endsample;
    int currentsample;
    int wavchannels;
    dca_state_t * state;
    int disable_adjust;// = 0;
    float gain;// = 1;
    int disable_dynrng;// = 0;
    uint8_t buf[BUFFER_SIZE];
    uint8_t * bufptr;// = buf;
    uint8_t * bufpos;// = buf + HEADER_SIZE;
    int sample_rate;
    int frame_length;
    int flags;
    int bit_rate;
    char output_buffer[OUT_BUFFER_SIZE];
    int remaining;
    int skipsamples;
} ddb_dca_state_t;

typedef sample_t convert_t;

#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

static int wav_channels (int flags, uint32_t * speaker_flags)
{
    static const uint32_t speaker_tbl[] = {
        SPEAKER_FRONT_CENTER,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
          | SPEAKER_BACK_CENTER,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT
          | SPEAKER_BACK_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
          | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
        /* TODO > 5 channels */
    };
    static const uint8_t nfchans_tbl[] =
    {
        1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8
    };
    int chans;

    *speaker_flags = speaker_tbl[flags & DCA_CHANNEL_MASK];
    chans = nfchans_tbl[flags & DCA_CHANNEL_MASK];

    if (flags & DCA_LFE) {
        *speaker_flags |= SPEAKER_LOW_FREQUENCY;
        chans++;
    }

    return chans;
}

static int
convert_samples (ddb_dca_state_t *state, int flags)
{
    sample_t *_samples = dca_samples (state->state);
    int chans, size;
    uint32_t speaker_flags;
    int16_t int16_samples[256*6];
    convert_t * samples = _samples;

    chans = channels_multi (flags);
    flags &= DCA_CHANNEL_MASK | DCA_LFE;

    convert2s16_multi (samples, int16_samples, flags);

    int16_t *dest = (int16_t*)(state->output_buffer + state->remaining * sizeof (int16_t) * 2);
    int i;
    for (i = 0; i < 256; i++) {
        *dest = int16_samples[i * chans + 0];
        dest++;
        *dest = int16_samples[i * chans + 1];
        dest++;
    }
    state->remaining += 256;
    //trace ("wrote %d bytes (chans=%d)\n", size, chans);
    //fwrite (&ordered_samples, 1, size, out);

    return 0;
}

// returns number of frames decoded
static int
dca_decode_data (ddb_dca_state_t *ddb_state, uint8_t * start, int size)
{
    int n_decoded = 0;
    uint8_t * end = start + size;
    int len;

    while (1) {
        len = end - start;
        if (!len)
            break;
        if (len > ddb_state->bufpos - ddb_state->bufptr)
            len = ddb_state->bufpos - ddb_state->bufptr;
        memcpy (ddb_state->bufptr, start, len);
        ddb_state->bufptr += len;
        start += len;
        if (ddb_state->bufptr == ddb_state->bufpos) {
            if (ddb_state->bufpos == ddb_state->buf + HEADER_SIZE) {
                int length;

                length = dca_syncinfo (ddb_state->state, ddb_state->buf, &ddb_state->flags, &ddb_state->sample_rate, &ddb_state->bit_rate, &ddb_state->frame_length);
                if (!length) {
                    trace ("dca_decode_data: skip\n");
                    for (ddb_state->bufptr = ddb_state->buf; ddb_state->bufptr < ddb_state->buf + HEADER_SIZE-1; ddb_state->bufptr++) {
                        ddb_state->bufptr[0] = ddb_state->bufptr[1];
                    }
                    continue;
                }
                ddb_state->bufpos = ddb_state->buf + length;
            }
            else {
                level_t level = 1;
                sample_t bias = 384;

                int i;

                //		if (output->setup (output, sample_rate, &flags, &level, &bias))
                //		    goto error;
                if (!ddb_state->disable_adjust)
                    ddb_state->flags |= DCA_ADJUST_LEVEL;
                level = (level_t) (level * ddb_state->gain);
                if (dca_frame (ddb_state->state, ddb_state->buf, &ddb_state->flags, &level, bias))
                    goto error;
                if (ddb_state->disable_dynrng)
                    dca_dynrng (ddb_state->state, NULL, NULL);
                for (i = 0; i < dca_blocks_num (ddb_state->state); i++) {
                    if (dca_block (ddb_state->state))
                        goto error;

                    // at this stage we can call dca_samples to get pointer to samples buffer
                    convert_samples (ddb_state, ddb_state->flags);

                    n_decoded += 256;
                }
                ddb_state->bufptr = ddb_state->buf;
                ddb_state->bufpos = ddb_state->buf + HEADER_SIZE;
                continue;
error:
                trace ("dca_decode_data: error\n");
                ddb_state->bufptr = ddb_state->buf;
                ddb_state->bufpos = ddb_state->buf + HEADER_SIZE;
            }
        }
    }
    return n_decoded;
}

// returns offset to DTS data in the file, or -1
static int
dts_open_wav (DB_FILE *fp, wavfmt_t *fmt, int *totalsamples) {
    // FIXME: endianess
    char riff[4];
    int offset = -1;

    if (deadbeef->fread (&riff, 1, sizeof (riff), fp) != sizeof (riff)) {
        return -1;
    }

    if (strncmp (riff, "RIFF", 4)) {
        return -1;
    }
    uint32_t size;
    if (deadbeef->fread (&size, 1, sizeof (size), fp) != sizeof (size)) {
        return -1;
    }

    char type[4];
    if (deadbeef->fread (type, 1, sizeof (type), fp) != sizeof (type)) {
        return -1;
    }

    if (strncmp (type, "WAVE", 4)) {
        return -1;
    }

    // fmt subchunk

    char fmtid[4];
    if (deadbeef->fread (fmtid, 1, sizeof (fmtid), fp) != sizeof (fmtid)) {
        return -1;
    }

    if (strncmp (fmtid, "fmt ", 4)) {
        return -1;
    }

    uint32_t fmtsize;
    if (deadbeef->fread (&fmtsize, 1, sizeof (fmtsize), fp) != sizeof (fmtsize)) {
        return -1;
    }

    if (deadbeef->fread (fmt, 1, sizeof (wavfmt_t), fp) != sizeof (wavfmt_t)) {
        return -1;
    }

    if (fmt->wFormatTag != 0x0001 || fmt->wBitsPerSample != 16) {
        return -1;
    }

    deadbeef->fseek (fp, fmtsize - sizeof (wavfmt_t), SEEK_CUR);

    // data subchunk

    char data[4];
    if (deadbeef->fread (data, 1, sizeof (data), fp) != sizeof (data)) {
        return -1;
    }

    if (strncmp (data, "data", 4)) {
        return -1;
    }

    uint32_t datasize;
    if (deadbeef->fread (&datasize, 1, sizeof (datasize), fp) != sizeof (datasize)) {
        return -1;
    }

    *totalsamples = datasize / ((fmt->wBitsPerSample >> 3) * fmt->nChannels);

    return deadbeef->ftell (fp);
}

static DB_fileinfo_t *
dts_open (void) {
    DB_fileinfo_t *_info = malloc (sizeof (ddb_dca_state_t));
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;
    memset (info, 0, sizeof (ddb_dca_state_t));
    return _info;
}

static int
dts_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;

    info->file = deadbeef->fopen (it->fname);
    if (!info->file) {
        fprintf (stderr, "dca: failed to open %s\n", it->fname);
        return -1;
    }

    wavfmt_t fmt;
    int totalsamples = -1;
    // WAV format
    if ((info->offset = dts_open_wav (info->file, &fmt, &totalsamples)) == -1) {
        fprintf (stderr, "dca: unrecognized format in %s\n", it->fname);
        deadbeef->fclose (info->file);
        return -1;
    }

    _info->bps = fmt.wBitsPerSample;
    _info->channels = fmt.nChannels;
    info->wavchannels = fmt.nChannels;
    _info->samplerate = fmt.nSamplesPerSec;
    _info->plugin = &plugin;

    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = totalsamples-1;
    }
    info->gain = 1;
    info->bufptr = info->buf;
    info->bufpos = info->buf + HEADER_SIZE;
    info->state = dca_init (0);
    if (!info->state) {
        trace ("dca_init failed\n");
        return -1;
    }

    // prebuffer 1st piece, and get decoded samplerate and nchannels
    uint8_t buffer[BUFFER_SIZE];
    size_t rd = deadbeef->fread (buffer, 1, sizeof (buffer), info->file);
    int nsamples = dca_decode_data (info, buffer, rd);
    if (!nsamples) {
        trace ("dca: failed to decode first batch\n");
        return -1;
    }
    _info->channels = channels_multi (info->flags);
    _info->samplerate = info->sample_rate;

    trace ("dca_init: nchannels: %d, samplerate: %d\n", _info->channels, _info->samplerate);
    return 0;
}

static void
dts_free (DB_fileinfo_t *_info) {
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;
    if (info) {
        if (info->state) {
            dca_free (info->state);
        }
        if (info->file) {
            deadbeef->fclose (info->file);
        }
        free (info);
    }
}

static int
dts_read_int16 (DB_fileinfo_t *_info, char *bytes, int size) {
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;
    if (info->currentsample + size / (2 * _info->channels) > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * 2 * _info->channels;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;
    int out_channels = _info->channels;
    if (out_channels > 2) {
        out_channels = 2;
    }
    int sample_size = ((_info->bps >> 3) * out_channels);
    while (size > 0) {
        if (info->skipsamples > 0 && info->remaining > 0) {
            int skip = min (info->remaining, info->skipsamples);
            int sample_size = _info->bps/8 * info->wavchannels;
            if (skip < info->remaining) {
                memmove (info->output_buffer, info->output_buffer + skip * sample_size, (info->remaining - skip) * sample_size);
            }
            info->remaining -= skip;
            info->skipsamples -= skip;
        }
        if (info->remaining > 0) {
            int n = size / sample_size;
            n = min (n, info->remaining);
            memcpy (bytes, info->output_buffer, n * sample_size);
            if (info->remaining > n) {
                memmove (info->output_buffer, info->output_buffer + n * sample_size, (info->remaining - n) * sample_size);
            }
            bytes += n * sample_size;
            size -= n * sample_size;
            info->remaining -= n;
//            trace ("dca: write %d samples\n", n);
        }
        if (size > 0 && !info->remaining) {
            uint8_t buffer[BUFFER_SIZE];
            size_t rd = deadbeef->fread (buffer, 1, sizeof (buffer), info->file);
            int nsamples = dca_decode_data (info, buffer, rd);
            if (!nsamples) {
                break;
            }
//            trace ("dca: decoded %d samples\n", nsamples);
        }
    }

    info->currentsample += (initsize-size) / sample_size;
    deadbeef->streamer_set_bitrate (info->bit_rate/1000);
    return initsize-size;
}

static int
dts_seek_sample (DB_fileinfo_t *_info, int sample) {
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;

    if (sample >= info->currentsample) {
        info->skipsamples = sample - info->currentsample;
    }
    else {
        deadbeef->fseek (info->file, info->offset, SEEK_SET);
        info->remaining = 0;
        info->skipsamples = sample;
    }

    info->currentsample = sample;
    _info->readpos = sample / _info->samplerate;
    return 0;
}

static int
dts_seek (DB_fileinfo_t *_info, float time) {
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;
    return dts_seek_sample (_info, time * _info->samplerate);
}

static DB_playItem_t *
dts_insert (DB_playItem_t *after, const char *fname) {
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        fprintf (stderr, "dca: failed to open %s\n", fname);
        return NULL;
    }

    wavfmt_t fmt;
    int totalsamples = -1;
    const char *filetype = NULL;

    int offset = 0;
    // WAV format
    if ((offset = dts_open_wav (fp, &fmt, &totalsamples)) != -1) {
        filetype = filetypes[FT_DTSWAV];
    }
    else {
        fprintf (stderr, "dca: unrecognized format in %s\n", fname);
        goto error;
    }

    double dur = (float)totalsamples / fmt.nSamplesPerSec;

    // try to decode piece of file -- that seems to be the only way to check
    // it's dts
    uint8_t buffer[BUFFER_SIZE];
    size_t size = deadbeef->fread (buffer, 1, sizeof (buffer), fp);
    ddb_dca_state_t state;
    memset (&state, 0, sizeof (state));
    state.state = dca_init (0);
    if (!state.state) {
        trace ("dca_init failed\n");
        goto error;
    }
    state.gain = 1;
    state.bufptr = state.buf;
    state.bufpos = state.buf + HEADER_SIZE;
    int dca_frames = dca_decode_data (&state, buffer, size);

    dca_free (state.state);
    trace ("dca: got %d dca frames", dca_frames);
    if (!dca_frames) {
        trace ("dca: doesn't seem to be a DTS stream\n");
        goto error;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = filetype;
    deadbeef->pl_set_item_duration (it, dur);

    deadbeef->fclose (fp);

    // embedded cue
    DB_playItem_t *cue = NULL;
    cue  = deadbeef->pl_insert_cue (after, it, totalsamples, 48000);
    if (cue) {
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue);
        return cue;
    }

    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);

    return after;
error:
    if (fp) {
        deadbeef->fclose (fp);
    }
    return NULL;
}

static int
dts_start (void) {
    return 0;
}

static int
dts_stop (void) {
    return 0;
}

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "dts",
    .plugin.name = "dts decoder",
    .plugin.descr = "dts decoder using libmppdec",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = dts_start,
    .plugin.stop = dts_stop,
    .open = dts_open,
    .init = dts_init,
    .free = dts_free,
    .read_int16 = dts_read_int16,
//    .read_float32 = dts_read_float32,
    .seek = dts_seek,
    .seek_sample = dts_seek_sample,
    .insert = dts_insert,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
dca_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
