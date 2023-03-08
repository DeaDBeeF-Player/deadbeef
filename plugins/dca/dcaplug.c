/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>
#include "dca.h"
#include "gettimeofday.h"

#ifdef WORDS_BIGENDIAN
#define s16_LE(s16,channels) s16_swap (s16, channels)
#define s16_BE(s16,channels) do {} while (0)
#define s32_LE(s32,channels) s32_swap (s32, channels)
#define s32_BE(s32,channels) do {} while (0)
#define u32_LE(u32) ((((u32)&0xff000000)>>24)|(((u32)&0x00ff0000)>>8)|(((u32)&0x0000ff00)<<8)|(((u32)&0x000000ff)<<24))
#define u16_LE(u16) ((((u16)&0xff00)>>8)|(((u16)&0x00ff)<<8))
#else
#define s16_LE(s16,channels) do {} while (0)
#define s16_BE(s16,channels) s16_swap (s16, channels)
#define s32_LE(s32,channels) do {} while (0)
#define s32_BE(s32,channels) s32_swap (s32, channels)
#define u32_LE(u32) (u32)
#define u16_LE(u16) (u16)
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static const char * exts[] = { "wav", "dts", "cpt", NULL };

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

#define BUFFER_SIZE 65536
#define OUT_BUFFER_SIZE 25000 // one block may be up to 22K samples, which is 88Kb for stereo
#define HEADER_SIZE 14
typedef struct {
    DB_fileinfo_t info;
    DB_FILE *file;
    int64_t offset;
    int64_t startsample;
    int64_t endsample;
    int64_t currentsample;
    dca_state_t * state;
    int disable_adjust;// = 0;
    float gain;// = 1;
    int disable_dynrng;// = 0;
    uint8_t inbuf[BUFFER_SIZE]; // input data buffer
    uint8_t buf[BUFFER_SIZE]; // decoder data buffer (inbuf gets appended here)
    uint8_t * bufptr;// = buf;
    uint8_t * bufpos;// = buf + HEADER_SIZE;
    int sample_rate;
    int frame_length;
    int flags;
    int bit_rate;
    int frame_byte_size;
    int16_t output_buffer[OUT_BUFFER_SIZE*6]; // output samples
    int remaining;
    int skipsamples;
} ddb_dca_state_t;

typedef sample_t convert_t;

#if 0
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
#endif

static int channel_remap[][7] = {
// DCA_MONO
    {0},
// DCA_CHANNEL
// DCA_STEREO
// DCA_STEREO_SUMDIFF
// DCA_STEREO_TOTAL
    {0,1},
    {0,1},
    {0,1},
    {0,1},
//DCA_3F
    {0,1,2},
//DCA_2F1R
    {0,1,2},
//DCA_3F1R
    {0,1,2,3},
//DCA_2F2R
    {0,1,2,3},
//DCA_3F2R
    {0,1,2,3,4},
//DCA_4F2R
    {0,1,2,3,4,5},

/// same with LFE

// DCA_MONO
    {0,1},
// DCA_CHANNEL
// DCA_STEREO
// DCA_STEREO_SUMDIFF
// DCA_STEREO_TOTAL
    {0,1,2},
    {0,1,2},
    {0,1,2},
    {0,1,2},
//DCA_3F
    {1,2,0,3},
//DCA_2F1R
    {0,1,3,2},
//DCA_3F1R
    {1,2,0,4,3},
//DCA_2F2R
    {0,1,4,2,3},
//DCA_3F2R
    {1,2,0,5,3,4},
//DCA_4F2R
    {1,2,5,3,4,6,7} // FL|FR|LFE|FLC|FRC|RL|RR
};

static inline int16_t convert (int32_t i)
{
#ifdef LIBDCA_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

static int
convert_samples (ddb_dca_state_t *state, int flags)
{
    sample_t *_samples = dca_samples (state->state);

    int n, i, c;
    n = 256;
    int16_t *dst = state->output_buffer + state->remaining * state->info.fmt.channels;

    for (i = 0; i < n; i++) {
        for (c = 0; c < state->info.fmt.channels; c++) {
            *dst++ = convert (*((int32_t*)(_samples + 256 * c)));
        }
        _samples ++;
    }

    state->remaining += 256;
    return 0;
}

// returns number of frames decoded
static int
dca_decode_data (ddb_dca_state_t *ddb_state, uint8_t * start, size_t size, int probe)
{
    int n_decoded = 0;
    uint8_t * end = start + size;
    size_t len;

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
//                    trace ("dca_decode_data: skip\n");
                    for (ddb_state->bufptr = ddb_state->buf; ddb_state->bufptr < ddb_state->buf + HEADER_SIZE-1; ddb_state->bufptr++) {
                        ddb_state->bufptr[0] = ddb_state->bufptr[1];
                    }
                    continue;
                }
                else if (probe) {
                    return length;
                }
                ddb_state->bufpos = ddb_state->buf + length;
            }
            else {
                level_t level = 1;
                sample_t bias = 384;

                int i;

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
    trace ("n_decoded: %d (%d bytes)\n", n_decoded, n_decoded * dca_blocks_num (ddb_state->state) * 2);
    return n_decoded;
}

// returns offset to DTS data in the file, or -1
static int64_t
dts_open_wav (DB_FILE *fp, wavfmt_t *fmt, int64_t *totalsamples) {
    char riff[4];

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
    size = u32_LE(size);

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
    fmtsize = u32_LE(fmtsize);

    if (deadbeef->fread (fmt, 1, sizeof (wavfmt_t), fp) != sizeof (wavfmt_t)) {
        return -1;
    }
    fmt->wFormatTag = u16_LE (fmt->wFormatTag);
    fmt->nChannels = u16_LE (fmt->nChannels);
    fmt->nSamplesPerSec = u32_LE (fmt->nSamplesPerSec);
    fmt->nAvgBytesPerSec = u32_LE (fmt->nAvgBytesPerSec);
    fmt->nBlockAlign = u16_LE (fmt->nBlockAlign);
    fmt->wBitsPerSample = u16_LE (fmt->wBitsPerSample);
    fmt->cbSize = u16_LE (fmt->cbSize);

    if (fmt->wFormatTag != 0x0001 || fmt->wBitsPerSample != 16) {
        return -1;
    }

    deadbeef->fseek (fp, (int)fmtsize - (int)sizeof (wavfmt_t), SEEK_CUR);

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
    datasize = u32_LE (datasize);

    *totalsamples = datasize / ((fmt->wBitsPerSample >> 3) * fmt->nChannels);

    return deadbeef->ftell (fp);
}

static DB_fileinfo_t *
dts_open (uint32_t hints) {
    ddb_dca_state_t *info = calloc (1, sizeof (ddb_dca_state_t));
    return &info->info;
}

static int
dts_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    info->file = deadbeef->fopen (uri);
    if (!info->file) {
        trace ("dca: failed to open %s\n", uri);
        return -1;
    }

    wavfmt_t fmt;
    int64_t totalsamples = -1;
    // WAV format
    if ((info->offset = dts_open_wav (info->file, &fmt, &totalsamples)) == -1) {
        // raw dts, leave detection to libdca
        info->offset = 0;
        totalsamples = -1;
        _info->fmt.bps = 16;
    }
    else {
        _info->fmt.bps = fmt.wBitsPerSample;
        _info->fmt.channels = fmt.nChannels;
        _info->fmt.samplerate = fmt.nSamplesPerSec;
    }

    _info->plugin = &plugin;
    info->gain = 1;
    info->bufptr = info->buf;
    info->bufpos = info->buf + HEADER_SIZE;
    info->state = dca_init (0);
    if (!info->state) {
        trace ("dca_init failed\n");
        return -1;
    }

    // prebuffer 1st piece, and get decoded samplerate and nchannels
    size_t rd = deadbeef->fread (info->inbuf, 1, BUFFER_SIZE, info->file);
    int len = dca_decode_data (info, info->inbuf, rd, 1);
    if (!len) {
        trace ("dca: probe failed\n");
        return -1;
    }

    info->bufptr = info->buf;
    info->bufpos = info->buf + HEADER_SIZE;

    info->frame_byte_size = len;

    int flags = info->flags &~ (DCA_LFE | DCA_ADJUST_LEVEL);
    switch (flags) {
    case DCA_MONO:
        trace ("dts: mono\n");
        _info->fmt.channels = 1;
        _info->fmt.channelmask = DDB_SPEAKER_FRONT_LEFT;
        break;
    case DCA_CHANNEL:
    case DCA_STEREO:
    case DCA_DOLBY:
    case DCA_STEREO_SUMDIFF:
    case DCA_STEREO_TOTAL:
        trace ("dts: stereo\n");
        _info->fmt.channels = 2;
        _info->fmt.channelmask = (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
        break;
    case DCA_3F:
    case DCA_2F1R:
        trace ("dts: 3F or 2F1R\n");
        _info->fmt.channels = 3;
        _info->fmt.channelmask = (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_FRONT_CENTER);
        break;
    case DCA_2F2R:
    case DCA_3F1R:
        trace ("dts: 2F2R or 3F1R\n");
        _info->fmt.channels = 4;
        _info->fmt.channelmask = (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT);
        break;
    case DCA_3F2R:
        trace ("dts: 3F2R\n");
        _info->fmt.channels = 5;
        _info->fmt.channelmask = (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER);
        break;
    case DCA_4F2R:
        trace ("dts: 4F2R\n");
        _info->fmt.channels = 6;
        _info->fmt.channelmask = (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_SIDE_LEFT | DDB_SPEAKER_SIDE_RIGHT);
        break;
    }

    if (info->flags & DCA_LFE) {
        trace ("dts: LFE\n");
        _info->fmt.channelmask |= DDB_SPEAKER_LOW_FREQUENCY;
        _info->fmt.channels++;
    }

    if (!_info->fmt.channels) {
        trace ("dts: invalid numchannels\n");
        return -1;
    }

    _info->fmt.samplerate = info->sample_rate;

    int64_t endsample = deadbeef->pl_item_get_endsample (it);
    if (endsample > 0) {
        info->startsample = deadbeef->pl_item_get_startsample (it);
        info->endsample = endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = totalsamples-1;
    }

    deadbeef->pl_set_meta_int (it, ":CHANNELS", _info->fmt.channels);

    trace ("dca_init: nchannels: %d, samplerate: %d\n", _info->fmt.channels, _info->fmt.samplerate);
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
dts_read (DB_fileinfo_t *_info, char *bytes, int size) {
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (info->endsample >= 0) {
        if (info->currentsample + size / samplesize > info->endsample) {
            size = (int)((info->endsample - info->currentsample + 1) * samplesize);
            if (size <= 0) {
                return 0;
            }
        }
    }

    int initsize = size;
    while (size > 0) {
        if (info->skipsamples > 0 && info->remaining > 0) {
            int skip = min (info->remaining, info->skipsamples);
            if (skip < info->remaining) {
                memmove (info->output_buffer, info->output_buffer + skip * _info->fmt.channels, (info->remaining - skip) * samplesize);
            }
            info->remaining -= skip;
            info->skipsamples -= skip;
        }
        if (info->remaining > 0) {
            int n = size / samplesize;
            n = min (n, info->remaining);

            if (!(info->flags & DCA_LFE)) {
                memcpy (bytes, info->output_buffer, n * samplesize);
                bytes += n * samplesize;
            }
            else {
                int chmap = (info->flags & DCA_CHANNEL_MASK) &~ DCA_LFE;
                if (info->flags & DCA_LFE) {
                    chmap += 11;
                }

                // remap channels
                char *in = (char *)info->output_buffer;
                for (int s = 0; s < n; s++) {
                    for (int i = 0; i < _info->fmt.channels; i++) {
                        ((int16_t *)bytes)[i] = ((int16_t*)in)[channel_remap[chmap][i]];
                    }
                    in += samplesize;
                    bytes += samplesize;
                }
            }

            if (info->remaining > n) {
                memmove (info->output_buffer, info->output_buffer + n * _info->fmt.channels, (info->remaining - n) * samplesize);
            }
            size -= n * samplesize;
            info->remaining -= n;
//            trace ("dca: write %d samples\n", n);
        }
        if (size > 0 && !info->remaining) {
            size_t rd = deadbeef->fread (info->inbuf, 1, BUFFER_SIZE, info->file);
            int nsamples = dca_decode_data (info, info->inbuf, rd, 0);
            if (!nsamples) {
                break;
            }
//            trace ("dca: decoded %d samples\n", nsamples);
        }
    }

    info->currentsample += (initsize-size) / samplesize;
    deadbeef->streamer_set_bitrate (info->bit_rate/1000);
    return initsize-size;
}

static int
dts_seek_sample (DB_fileinfo_t *_info, int sample) {
    ddb_dca_state_t *info = (ddb_dca_state_t *)_info;

    // calculate file offset from framesize / framesamples
    sample += info->startsample;
    int64_t nframe = sample / info->frame_length;
    int64_t offs = info->frame_byte_size * nframe + info->offset;
    deadbeef->fseek (info->file, offs, SEEK_SET);
    info->remaining = 0;
    info->skipsamples = (int)(sample - nframe * info->frame_length);

    info->currentsample = sample;
    _info->readpos = (float)(sample - info->startsample) / _info->fmt.samplerate;
    return 0;
}

static int
dts_seek (DB_fileinfo_t *_info, float time) {
    return dts_seek_sample (_info, time * _info->fmt.samplerate);
}

static DB_playItem_t *
dts_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    ddb_dca_state_t *state = NULL;
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("dca: failed to open %s\n", fname);
        return NULL;
    }
    int64_t fsize = deadbeef->fgetlength (fp);
    trace ("dts file size: %lld\n", fsize);

    wavfmt_t fmt;
    int64_t totalsamples = -1;
    const char *filetype = NULL;

    int64_t offset = 0;
    double dur = -1;
    // WAV format
    if ((offset = dts_open_wav (fp, &fmt, &totalsamples)) != -1) {
        filetype = "DTS WAV";
        dur = (float)totalsamples / fmt.nSamplesPerSec;
    }
    else {
        // try raw DTS @ 48KHz
        offset = 0;
        filetype = "DTS";
        //fprintf (stderr, "dca: unrecognized format in %s\n", fname);
        //goto error;
    }

    state = calloc (1, sizeof (ddb_dca_state_t));
    if (!state) {
        goto error;
    }
    state->state = dca_init (0);
    if (!state->state) {
        trace ("dca_init failed\n");
        goto error;
    }

    // try to decode piece of file -- that seems to be the only way to check
    // it's dts
    size_t size = deadbeef->fread (state->inbuf, 1, BUFFER_SIZE, fp);
    trace ("got size: %d (requested %d)\n", size, BUFFER_SIZE);
    state->gain = 1;
    state->bufptr = state->buf;
    state->bufpos = state->buf + HEADER_SIZE;

    int len = dca_decode_data (state, state->inbuf, size, 1);

    if (!len) {
        trace ("dca: doesn't seem to be a DTS stream\n");
        goto error;
    }
    trace ("dca stream info: len=%d, samplerate=%d, bitrate=%d, framelength=%d\n", len, state->sample_rate, state->bit_rate, state->frame_length);

    dca_free (state->state);

    int samplerate = state->sample_rate;

    // calculate duration
    if (dur < 0) {
        totalsamples = fsize / len * state->frame_length;
        dur = (float)totalsamples / state->sample_rate;
    }

    free (state);
    state = NULL;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", filetype);
    deadbeef->plt_set_item_duration (plt, it, dur);

    deadbeef->fclose (fp);

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it, totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        return cue;
    }

    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);

    return after;
error:
    if (state) {
        if (state->state) {
            dca_free(state->state);
        }
        free (state);
    }
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
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "dts",
    .plugin.name = "dts decoder",
    .plugin.descr = "plays dts-encoded files using libdca from VLC project",
    .plugin.copyright = 
        "Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses modified libdca from VLC Player project,\n"
        "developed by Gildas Bazin <gbazin@videolan.org>"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = dts_start,
    .plugin.stop = dts_stop,
    .open = dts_open,
    .init = dts_init,
    .free = dts_free,
    .read = dts_read,
    .seek = dts_seek,
    .seek_sample = dts_seek_sample,
    .insert = dts_insert,
    .exts = exts,
};

DB_plugin_t *
dca_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
