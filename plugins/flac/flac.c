/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <FLAC/stream_decoder.h>
#include <FLAC/metadata.h>
#include <math.h>
#include "../../deadbeef.h"
#include "../artwork/artwork.h"

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define BUFFERSIZE 100000

typedef struct {
    DB_fileinfo_t info;
    FLAC__StreamDecoder *decoder;
    char *buffer; // this buffer always has float samples
    int remaining; // bytes remaining in buffer from last read
    int64_t startsample;
    int64_t endsample;
    int64_t currentsample;
    int64_t totalsamples;
    int flac_critical_error;
    int init_stop_decoding;
    int tagsize;
    DB_FILE *file;

    // used only on insert
    ddb_playlist_t *plt;
    DB_playItem_t *after;
    DB_playItem_t *last;
    DB_playItem_t *it;
    const char *fname;
    int bitrate;
} flac_info_t;

// callbacks
FLAC__StreamDecoderReadStatus flac_read_cb (const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    size_t r = deadbeef->fread (buffer, 1, *bytes, info->file);
    *bytes = r;
    if (r == 0) {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus flac_seek_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    int r = deadbeef->fseek (info->file, absolute_byte_offset, SEEK_SET);
    if (r) {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderTellStatus flac_tell_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    size_t r = deadbeef->ftell (info->file);
    *absolute_byte_offset = r;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderLengthStatus flac_lenght_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    size_t pos = deadbeef->ftell (info->file);
    deadbeef->fseek (info->file, 0, SEEK_END);
    *stream_length = deadbeef->ftell (info->file);
    deadbeef->fseek (info->file, pos, SEEK_SET);
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__bool flac_eof_cb (const FLAC__StreamDecoder *decoder, void *client_data) {
    return 0;
}

static FLAC__StreamDecoderWriteStatus
cflac_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    DB_fileinfo_t *_info = &info->info;
    if (frame->header.blocksize == 0) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (info->bitrate > 0) {
        deadbeef->streamer_set_bitrate (info->bitrate);
    }
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    int bufsize = BUFFERSIZE - info->remaining;
    int bufsamples = bufsize / samplesize;
    int nsamples = min (bufsamples, frame->header.blocksize);
    char *bufptr = &info->buffer[info->remaining];

    int readbytes = frame->header.blocksize * samplesize;

    if (_info->fmt.bps == 32) {
        for (int i = 0; i <  nsamples; i++) {
            for (int c = 0; c < _info->fmt.channels; c++) {
                int32_t sample = inputbuffer[c][i];
                *((int32_t*)bufptr) = sample;
                bufptr += 4;
                info->remaining += 4;
            }
        }
    }
    else if (_info->fmt.bps == 24) {
        for (int i = 0; i <  nsamples; i++) {
            for (int c = 0; c < _info->fmt.channels; c++) {
                int32_t sample = inputbuffer[c][i];
                *bufptr++ = sample&0xff;
                *bufptr++ = (sample&0xff00)>>8;
                *bufptr++ = (sample&0xff0000)>>16;
                info->remaining += 3;
            }
        }
    }
    else if (_info->fmt.bps == 16) {
        for (int i = 0; i <  nsamples; i++) {
            for (int c = 0; c < _info->fmt.channels; c++) {
                int32_t sample = inputbuffer[c][i];
                *bufptr++ = sample&0xff;
                *bufptr++ = (sample&0xff00)>>8;
                info->remaining += 2;
            }
        }
    }
    else if (_info->fmt.bps == 8) {
        for (int i = 0; i <  nsamples; i++) {
            for (int c = 0; c < _info->fmt.channels; c++) {
                int32_t sample = inputbuffer[c][i];
                *bufptr++ = sample&0xff;
                info->remaining += 1;
            }
        }
    }
    if (readbytes > bufsize) {
        trace ("flac: buffer overflow, distortion will occur\n");
    //    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void
cflac_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)client_data;
    flac_info_t *info = (flac_info_t *)_info;
    info->totalsamples = metadata->data.stream_info.total_samples;
    _info->fmt.samplerate = metadata->data.stream_info.sample_rate;
    _info->fmt.channels = metadata->data.stream_info.channels;
    _info->fmt.bps = metadata->data.stream_info.bits_per_sample;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
}

static void
cflac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)client_data;
    flac_info_t *info = (flac_info_t *)_info;
    if (status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC
            && status != FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH) {
        trace ("cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
        info->flac_critical_error = 1;
    }
}

static void
cflac_init_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    if (status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC) {
        DB_fileinfo_t *_info = (DB_fileinfo_t *)client_data;
        flac_info_t *info = (flac_info_t *)_info;
        fprintf(stderr, "cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
        info->init_stop_decoding = 1;
    }
}

static DB_fileinfo_t *
cflac_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (flac_info_t));
    memset (_info, 0, sizeof (flac_info_t));
    return _info;
}

static int
cflac_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    trace ("cflac_init %s\n", deadbeef->pl_find_meta (it, ":URI"));
    flac_info_t *info = (flac_info_t *)_info;

    info->file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    if (!info->file) {
        trace ("cflac_init failed to open file\n");
        return -1;
    }

    info->flac_critical_error = 0;

    const char *ext = deadbeef->pl_find_meta (it, ":URI") + strlen (deadbeef->pl_find_meta (it, ":URI"));
    while (ext > deadbeef->pl_find_meta (it, ":URI") && *ext != '/' && *ext != '.') {
        ext--;
    }
    if (*ext == '.') {
        ext++;
    }
    else {
        ext = NULL;
    }

    int isogg = 0;
    int skip = 0;
    if (ext && !strcasecmp (ext, "flac")) {
        skip = deadbeef->junk_get_leading_size (info->file);
        if (skip > 0) {
            deadbeef->fseek (info->file, skip, SEEK_SET);
        }
        char sign[4];
        if (deadbeef->fread (sign, 1, 4, info->file) != 4) {
            trace ("cflac_init failed to read signature\n");
            return -1;
        }
        if (strncmp (sign, "fLaC", 4)) {
            trace ("cflac_init bad signature\n");
            return -1;
        }
        deadbeef->fseek (info->file, -4, SEEK_CUR);
    }
    else if (!FLAC_API_SUPPORTS_OGG_FLAC) {
        trace ("flac: ogg transport support is not compiled into FLAC library\n");
        return -1;
    }
    else {
        isogg = 1;
    }

    FLAC__StreamDecoderInitStatus status;
    info->decoder = FLAC__stream_decoder_new ();
    if (!info->decoder) {
        trace ("FLAC__stream_decoder_new failed\n");
        return -1;
    }
    FLAC__stream_decoder_set_md5_checking (info->decoder, 0);
    if (isogg) {
        status = FLAC__stream_decoder_init_ogg_stream (info->decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_lenght_cb, flac_eof_cb, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, info);
    }
    else {
        status = FLAC__stream_decoder_init_stream (info->decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_lenght_cb, flac_eof_cb, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, info);
    }
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        trace ("cflac_init bad decoder status\n");
        return -1;
    }
    //_info->fmt.samplerate = -1;
    if (!FLAC__stream_decoder_process_until_end_of_metadata (info->decoder)) {
        trace ("cflac_init metadata failed\n");
        return -1;
    }

    // bps/samplerate/channels were set by callbacks
    _info->plugin = &plugin;
    _info->readpos = 0;

    if (_info->fmt.samplerate <= 0) { // not a FLAC stream
        fprintf (stderr, "corrupted/invalid flac stream\n");
        return -1;
    }

    // bitrate
    size_t fsize = deadbeef->fgetlength (info->file);
    FLAC__uint64 position;
    FLAC__bool res = FLAC__stream_decoder_get_decode_position (info->decoder, &position);
    if (res) {
        fsize -= position;
    }
    int64_t totalsamples = FLAC__stream_decoder_get_total_samples (info->decoder);
    if (totalsamples <= 0) {
        trace ("flac: totalsamples=%lld\n", totalsamples);
//        return -1;
    }
    float sec = totalsamples / (float)_info->fmt.samplerate;
    if (sec > 0) {
        info->bitrate = fsize / sec * 8 / 1000;
    }
    else {
        info->bitrate = -1;
    }
    const char *channelmask = deadbeef->pl_find_meta (it, "WAVEFORMAT_EXTENSIBLE_CHANNELMASK");
    if (channelmask) {
        uint32_t cm = 0;
        if (1 == sscanf (channelmask, "0x%X", &cm)) {
            _info->fmt.channelmask = cm;
        }
    }

    info->buffer = malloc (BUFFERSIZE);
    info->remaining = 0;
    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        if (plugin.seek_sample (_info, 0) < 0) {
            trace ("cflac_init failed to seek to sample 0\n");
            return -1;
        }
        trace ("flac(cue): startsample=%d, endsample=%d, totalsamples=%d, currentsample=%d\n", info->startsample, info->endsample, info->totalsamples, info->currentsample);
    }
    else {
        info->startsample = 0;
        info->endsample = info->totalsamples-1;
        info->currentsample = 0;
        trace ("flac: startsample=%d, endsample=%d, totalsamples=%d\n", info->startsample, info->endsample, info->totalsamples);
    }

    if (info->flac_critical_error) {
        trace ("flac: critical error while initializing\n");
        return -1;
    }

    return 0;
}

static void
cflac_free (DB_fileinfo_t *_info) {
    if (_info) {
        flac_info_t *info = (flac_info_t *)_info;
        if (info->decoder) {
            FLAC__stream_decoder_delete (info->decoder);
        }
        if (info->buffer) {
            free (info->buffer);
        }
        if (info->file) {
            deadbeef->fclose (info->file);
        }
        free (_info);
    }
}

static int
cflac_read (DB_fileinfo_t *_info, char *bytes, int size) {
    flac_info_t *info = (flac_info_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (info->endsample >= 0) {
        if (size / samplesize + info->currentsample > info->endsample) {
            size = (info->endsample - info->currentsample + 1) * samplesize;
            trace ("size truncated to %d bytes, cursample=%d, endsample=%d\n", size, info->currentsample, info->endsample);
            if (size <= 0) {
                return 0;
            }
        }
    }
    int initsize = size;
    do {
        if (info->remaining) {
            int sz = min(size, info->remaining);
            memcpy (bytes, info->buffer, sz);

            size -= sz;
            bytes += sz;
            if (sz < info->remaining) {
                memmove (info->buffer, &info->buffer[sz], info->remaining - sz);
            }
            info->remaining -= sz;
            int n = sz / samplesize;
            info->currentsample += sz / samplesize;
            _info->readpos += (float)n / _info->fmt.samplerate;
        }
        if (!size) {
            break;
        }
        if (!FLAC__stream_decoder_process_single (info->decoder)) {
            trace ("FLAC__stream_decoder_process_single error\n");
            break;
        }
        if (FLAC__stream_decoder_get_state (info->decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
            trace ("FLAC__stream_decoder_get_state error\n");
            break;
        }
        if (info->flac_critical_error) {
            trace ("flac: got critical error while decoding\n");
            return 0;
        }
    } while (size > 0);

    return initsize - size;
}

#if 0
static int
cflac_read_float32 (DB_fileinfo_t *_info, char *bytes, int size) {
    flac_info_t *info = (flac_info_t *)_info;
    if (size / (4 * _info->fmt.channels) + info->currentsample > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * 4 * _info->fmt.channels;
        trace ("size truncated to %d bytes, cursample=%d, endsample=%d\n", size, info->currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int n_output_channels = _info->fmt.channels;
    if (n_output_channels > 2) {
        n_output_channels = 2;
    }
    int initsize = size;
    do {
        if (info->remaining) {
            int n_input_frames = info->remaining / sizeof (float) / n_output_channels;
            int n_output_frames = size / n_output_channels / sizeof (float);
            int n = min (n_input_frames, n_output_frames);

            float *in = (float *)info->buffer;
            for (int i = 0; i < n; i++) {
                *((float *)bytes) = *in;
                size -= sizeof (float);
                bytes += sizeof (float);
                if (n_output_channels == 2) {
                    *((float *)bytes) = *(in+1);
                    size -= sizeof (float);
                    bytes += sizeof (float);
                }
                in += n_output_channels;
            }
            int sz = n * sizeof (float) * n_output_channels;
            if (sz < info->remaining) {
                memmove (info->buffer, &info->buffer[sz], info->remaining-sz);
            }
            info->remaining -= sz;
            info->currentsample += n;
            _info->readpos += (float)n / _info->fmt.samplerate;
        }
        if (!size) {
            break;
        }
        if (!FLAC__stream_decoder_process_single (info->decoder)) {
            trace ("FLAC__stream_decoder_process_single error\n");
            break;
        }
        if (FLAC__stream_decoder_get_state (info->decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
            trace ("FLAC__stream_decoder_get_state eof\n");
            break;
        }
        if (info->flac_critical_error) {
            trace ("flac: got critical error while decoding\n");
            return 0;
        }
    } while (size > 0);

    return initsize - size;
}
#endif

static int
cflac_seek_sample (DB_fileinfo_t *_info, int sample) {
    flac_info_t *info = (flac_info_t *)_info;
    sample += info->startsample;
    info->currentsample = sample;
    info->remaining = 0;
    if (!FLAC__stream_decoder_seek_absolute (info->decoder, (FLAC__uint64)(sample))) {
        return -1;
    }
    _info->readpos = (float)(sample - info->startsample)/ _info->fmt.samplerate;
    return 0;
}

static int
cflac_seek (DB_fileinfo_t *_info, float time) {
    return cflac_seek_sample (_info, time * _info->fmt.samplerate);
}

static FLAC__StreamDecoderWriteStatus
cflac_init_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    if (frame->header.blocksize == 0 || info->init_stop_decoding) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

#if 0
static void
cflac_init_cue_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    if (cflac_init_stop_decoding) {
        trace ("flac: cflac_init_cue_metadata_callback: cflac_init_stop_decoding=1\n");
        return;
    }
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        trace ("flac: cflac_init_cue_metadata_callback: got FLAC__METADATA_TYPE_STREAMINFO\n");
        info->samplerate = metadata->data.stream_info.sample_rate;
        info->channels = metadata->data.stream_info.channels;
        //info->duration = metadata->data.stream_info.total_samples / (float)metadata->data.stream_info.sample_rate;
        info->totalsamples = metadata->data.stream_info.total_samples;
    }
}
#endif

static const char *metainfo[] = {
    "ARTIST", "artist",
    "TITLE", "title",
    "ALBUM", "album",
    "TRACKNUMBER", "track",
    "DATE", "year",
    "GENRE", "genre",
    "COMMENT", "comment",
    "PERFORMER", "performer",
//    "ENSEMBLE", "band",
    "COMPOSER", "composer",
    "ENCODED-BY", "vendor",
    "DISCNUMBER", "disc",
    "COPYRIGHT", "copyright",
    "TOTALTRACKS", "numtracks",
    "TRACKTOTAL", "numtracks",
    "ALBUM ARTIST", "band",
    NULL
};

static void
cflac_add_metadata (DB_playItem_t *it, char *s, int length) {
    int m;
    for (m = 0; metainfo[m]; m += 2) {
        int l = strlen (metainfo[m]);
        if (length > l && !strncasecmp (metainfo[m], s, l) && s[l] == '=') {
            deadbeef->pl_append_meta (it, metainfo[m+1], s + l + 1);
            break;
        }
    }
    if (!metainfo[m]) {
        if (!strncasecmp (s, "CUESHEET=", 9)) {
            deadbeef->pl_add_meta (it, "cuesheet", s + 9);
        }
        else if (!strncasecmp (s, "replaygain_album_gain=", 22)) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, atof (s+22));
        }
        else if (!strncasecmp (s, "replaygain_album_peak=", 22)) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, atof (s+22));
        }
        else if (!strncasecmp (s, "replaygain_track_gain=", 22)) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, atof (s+22));
        }
        else if (!strncasecmp (s, "replaygain_track_peak=", 22)) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, atof (s+22));
        }
        else {
            const char *eq = strchr (s, '=');
            if (eq) {
                char key[eq - s+1];
                strncpy (key, s, eq-s);
                key[eq-s] = 0;
                deadbeef->pl_append_meta (it, key, eq+1);
            }
        }
    }
}

static void
cflac_init_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    info->tagsize += metadata->length;
    DB_fileinfo_t *_info = &info->info;
    if (info->init_stop_decoding) {
        trace ("error flag is set, ignoring init_metadata callback..\n");
        return;
    }
    DB_playItem_t *it = info->it;
    //it->tracknum = 0;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        trace ("flac: samplerate=%d, channels=%d, totalsamples=%d\n", metadata->data.stream_info.sample_rate, metadata->data.stream_info.channels, metadata->data.stream_info.total_samples);
        _info->fmt.samplerate = metadata->data.stream_info.sample_rate;
        _info->fmt.channels = metadata->data.stream_info.channels;
        _info->fmt.bps = metadata->data.stream_info.bits_per_sample;
        info->totalsamples = metadata->data.stream_info.total_samples;
        if (metadata->data.stream_info.total_samples > 0) {
            deadbeef->plt_set_item_duration (info->plt, it, metadata->data.stream_info.total_samples / (float)metadata->data.stream_info.sample_rate);
        }
        else {
            deadbeef->plt_set_item_duration (info->plt, it, -1);
        }
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        const FLAC__StreamMetadata_VorbisComment *vc = &metadata->data.vorbis_comment;
        for (int i = 0; i < vc->num_comments; i++) {
            const FLAC__StreamMetadata_VorbisComment_Entry *c = &vc->comments[i];
            if (c->length > 0) {
                char *s = c->entry;
                cflac_add_metadata (it, s, c->length);
            }
        }
        deadbeef->pl_add_meta (it, "title", NULL);
        if (vc->num_comments > 0) {
            uint32_t f = deadbeef->pl_get_item_flags (it);
            f &= ~DDB_TAG_MASK;
            f |= DDB_TAG_VORBISCOMMENTS;
            deadbeef->pl_set_item_flags (it, f);
        }
    }
}

static DB_playItem_t *
cflac_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("flac: inserting %s\n", fname);
    DB_playItem_t *it = NULL;
    FLAC__StreamDecoder *decoder = NULL;
    flac_info_t info;
    memset (&info, 0, sizeof (info));
    info.fname = fname;
    info.after = after;
    info.last = after;
    info.plt = plt;
    info.file = deadbeef->fopen (fname);
    if (!info.file) {
        goto cflac_insert_fail;
    }

    const char *ext = fname + strlen (fname);
    while (ext > fname && *ext != '/' && *ext != '.') {
        ext--;
    }
    if (*ext == '.') {
        ext++;
    }
    else {
        ext = NULL;
    }

    int isogg = 0;
    int skip = 0;
    if (ext && !strcasecmp (ext, "flac")) {
        // skip id3 junk and verify fLaC signature
        skip = deadbeef->junk_get_leading_size (info.file);
        if (skip > 0) {
            deadbeef->fseek (info.file, skip, SEEK_SET);
        }
        char sign[4];
        if (deadbeef->fread (sign, 1, 4, info.file) != 4) {
            trace ("flac: failed to read signature\n");
            goto cflac_insert_fail;
        }
        if (strncmp (sign, "fLaC", 4)) {
            trace ("flac: file signature is not fLaC\n");
            goto cflac_insert_fail;
        }
        deadbeef->fseek (info.file, -4, SEEK_CUR);
    }
    else if (!FLAC_API_SUPPORTS_OGG_FLAC) {
        trace ("flac: ogg transport support is not compiled into FLAC library\n");
        goto cflac_insert_fail;
    }
    else {
        isogg = 1;
    }
    info.init_stop_decoding = 0;

    // open decoder for metadata reading
    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        trace ("flac: failed to create decoder\n");
        goto cflac_insert_fail;
    }

    // read all metadata
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    FLAC__stream_decoder_set_metadata_respond_all (decoder);
    it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    info.it = it;
    if (skip > 0) {
        deadbeef->fseek (info.file, skip, SEEK_SET);
    }
    else {
        deadbeef->rewind (info.file);
    }
    deadbeef->fseek (info.file, -4, SEEK_CUR);
    if (isogg) {
        status = FLAC__stream_decoder_init_ogg_stream (decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_lenght_cb, flac_eof_cb, cflac_init_write_callback, cflac_init_metadata_callback, cflac_init_error_callback, &info);
    }
    else {
        status = FLAC__stream_decoder_init_stream (decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_lenght_cb, flac_eof_cb, cflac_init_write_callback, cflac_init_metadata_callback, cflac_init_error_callback, &info);
    }
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK || info.init_stop_decoding) {
        trace ("flac: FLAC__stream_decoder_init_stream [2] failed\n");
        goto cflac_insert_fail;
    }
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder) || info.init_stop_decoding) {
        trace ("flac: FLAC__stream_decoder_process_until_end_of_metadata [2] failed\n");
        goto cflac_insert_fail;
    }
    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;

    if (info.info.fmt.samplerate <= 0) {
        goto cflac_insert_fail;
    }

    deadbeef->pl_add_meta (it, ":FILETYPE", isogg ? "OggFLAC" : "FLAC");

    char s[100];
    int64_t fsize = deadbeef->fgetlength (info.file);
    snprintf (s, sizeof (s), "%lld", fsize);
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
    snprintf (s, sizeof (s), "%d", info.info.fmt.channels);
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", info.info.fmt.bps);
    deadbeef->pl_add_meta (it, ":BPS", s);
    snprintf (s, sizeof (s), "%d", info.info.fmt.samplerate);
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    if ( deadbeef->pl_get_item_duration (it) > 0) {
        snprintf (s, sizeof (s), "%d", (int)roundf((fsize-info.tagsize) / deadbeef->pl_get_item_duration (it) * 8 / 1000));
        deadbeef->pl_add_meta (it, ":BITRATE", s);
    }

    // try embedded cue
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    if (cuesheet) {
        DB_playItem_t *last = deadbeef->plt_insert_cue_from_buffer (plt, after, it, cuesheet, strlen (cuesheet), info.totalsamples, info.info.fmt.samplerate);
        if (last) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (last);
            return last;
        }
    }

    // try external cue
    DB_playItem_t *cue_after = deadbeef->plt_insert_cue (plt, after, it, info.totalsamples, info.info.fmt.samplerate);
    if (cue_after) {
        if (info.file) {
            deadbeef->fclose (info.file);
        }
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue_after);
        trace ("flac: loaded external cuesheet\n");
        return cue_after;
    }
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    if (info.file) {
        deadbeef->fclose (info.file);
    }
    return after;
cflac_insert_fail:
    if (it) {
        deadbeef->pl_item_unref (it);
    }
    if (decoder) {
        FLAC__stream_decoder_delete(decoder);
    }
    if (info.file) {
        deadbeef->fclose (info.file);
    }
    return NULL;
}

int
cflac_read_metadata (DB_playItem_t *it) {
    int err = -1;
    FLAC__Metadata_Chain *chain = NULL;
    FLAC__Metadata_Iterator *iter = NULL;

    chain = FLAC__metadata_chain_new ();
    if (!chain) {
        trace ("cflac_read_metadata: FLAC__metadata_chain_new failed\n");
        return -1;
    }
    FLAC__bool res = FLAC__metadata_chain_read (chain, deadbeef->pl_find_meta (it, ":URI"));
    if (!res) {
        trace ("cflac_read_metadata: FLAC__metadata_chain_read failed\n");
        goto error;
    }
    FLAC__metadata_chain_merge_padding (chain);

    iter = FLAC__metadata_iterator_new ();
    if (!iter) {
        trace ("cflac_read_metadata: FLAC__metadata_iterator_new failed\n");
        goto error;
    }
    deadbeef->pl_delete_all_meta (it);
    FLAC__metadata_iterator_init (iter, chain);
    do {
        FLAC__StreamMetadata *data = FLAC__metadata_iterator_get_block (iter);
        if (data && data->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
            const FLAC__StreamMetadata_VorbisComment *vc = &data->data.vorbis_comment;
            for (int i = 0; i < vc->num_comments; i++) {
                const FLAC__StreamMetadata_VorbisComment_Entry *c = &vc->comments[i];
                if (c->length > 0) {
                    char *s = c->entry;
                    cflac_add_metadata (it, s, c->length);
                }
            }
            deadbeef->pl_add_meta (it, "title", NULL);
            if (vc->num_comments > 0) {
                uint32_t f = deadbeef->pl_get_item_flags (it);
                f &= ~DDB_TAG_MASK;
                f |= DDB_TAG_VORBISCOMMENTS;
                deadbeef->pl_set_item_flags (it, f);
            }
        }
    } while (FLAC__metadata_iterator_next (iter));

    FLAC__metadata_iterator_delete (iter);
    err = 0;
    deadbeef->pl_add_meta (it, "title", NULL);
    uint32_t f = deadbeef->pl_get_item_flags (it);
    f &= ~DDB_TAG_MASK;
    f |= DDB_TAG_VORBISCOMMENTS;
    deadbeef->pl_set_item_flags (it, f);
error:
    if (chain) {
        FLAC__metadata_chain_delete (chain);
    }
    if (err != 0) {
        deadbeef->pl_delete_all_meta (it);
        deadbeef->pl_add_meta (it, "title", NULL);
    }

    return err;
}

int
cflac_write_metadata (DB_playItem_t *it) {
    int err = -1;
    FLAC__Metadata_Chain *chain = NULL;
    FLAC__Metadata_Iterator *iter = NULL;

    chain = FLAC__metadata_chain_new ();
    if (!chain) {
        trace ("cflac_write_metadata: FLAC__metadata_chain_new failed\n");
        return -1;
    }
    FLAC__bool res = FLAC__metadata_chain_read (chain, deadbeef->pl_find_meta (it, ":URI"));
    if (!res) {
        trace ("cflac_write_metadata: FLAC__metadata_chain_read failed\n");
        goto error;
    }
    FLAC__metadata_chain_merge_padding (chain);

    iter = FLAC__metadata_iterator_new ();
    if (!iter) {
        trace ("cflac_write_metadata: FLAC__metadata_iterator_new failed\n");
        goto error;
    }
    FLAC__StreamMetadata *data = NULL;

    // find existing vorbiscomment block
    FLAC__metadata_iterator_init (iter, chain);
    do {
        data = FLAC__metadata_iterator_get_block (iter);
        if (data && data->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
            break;
        }
    } while (FLAC__metadata_iterator_next (iter));

    if (data) {
        // delete all comments
        FLAC__metadata_object_vorbiscomment_resize_comments (data, 0);
    }
    else {
        // create new and add to chain
		data = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
        if (!data) {
            fprintf (stderr, "flac: failed to allocate new vorbis comment block\n");
            goto error;
        }
        if(!FLAC__metadata_iterator_insert_block_after(iter, data)) {
            fprintf (stderr, "flac: failed to append vorbis comment block to chain\n");
            goto error;
        }
    }

    DB_metaInfo_t *m = deadbeef->pl_get_metadata_head (it);
    while (m) {
        if (m->key[0] != ':') {
            int i;
            for (i = 0; metainfo[i]; i += 2) {
                if (!strcasecmp (metainfo[i+1], m->key)) {
                    break;
                }
            }
            const char *val = m->value;
            if (val && *val) {
                while (val) {
                    const char *next = strchr (val, '\n');
                    int l;
                    if (next) {
                        l = next - val;
                        next++;
                    }
                    else {
                        l = strlen (val);
                    }
                    if (l > 0) {
                        char s[100+l+1];
                        int n = snprintf (s, sizeof (s), "%s=", metainfo[i] ? metainfo[i] : m->key);
                        strncpy (s+n, val, l);
                        *(s+n+l) = 0;
                        FLAC__StreamMetadata_VorbisComment_Entry ent = {
                            .length = strlen (s),
                            .entry = (FLAC__byte*)s
                        };
                        FLAC__metadata_object_vorbiscomment_append_comment (data, ent, 1);
                    }
                    val = next;
                }
            }
        }
        m = m->next;
    }

    // Add Coverart if available
    DB_artwork_plugin_t *coverart_plugin = NULL;
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->id && !strcmp (p->id, "artwork")) {
            coverart_plugin = (DB_artwork_plugin_t *)p;
            break;
        }
    }

    if (coverart_plugin) {

        deadbeef->pl_lock ();
        const char *album = deadbeef->pl_find_meta (it, "album");
        const char *artist = deadbeef->pl_find_meta (it, "artist");
        if (!album || !*album) {
            album = deadbeef->pl_find_meta (it, "title");
        }
        const char *fname = deadbeef->pl_find_meta (it, ":URI");
        deadbeef->pl_unlock ();

        char *image_fname = coverart_plugin->get_album_art_sync (fname, artist, album, -1);
        //char *image_fname = coverart_plugin->get_album_art (fname, artist, album, -1, NULL, NULL);
        printf ("image_fname = %s\n",image_fname);
        if (image_fname && strcmp (image_fname, coverart_plugin->get_default_cover())) {
            // Read file
            DB_FILE *fp = deadbeef->fopen (image_fname);
            FLAC__byte *coverart_data = NULL;
            FLAC__StreamMetadata *metadata = NULL;
            if (!fp) {
                fprintf (stderr, "flac: cannot open coverart %s\n", image_fname);
                goto error2;
            }

            int64_t len = deadbeef->fgetlength (fp);
            coverart_data = malloc (len);
            if (!coverart_data) {
                fprintf (stderr, "flac: cannot allocate memory\n");
                goto error2;
            }
            if (!deadbeef->fread (coverart_data, len, 1, fp)) {
                fprintf (stderr, "flac: cannot read from %s\n",image_fname);
                goto error2;
            }

            // Write Metadata
            metadata = FLAC__metadata_object_new (FLAC__METADATA_TYPE_PICTURE);
            if (!metadata) {
                fprintf (stderr, "flac: failed to allocate new picture block\n");
                goto error2;
            }

            //FLAC__metadata_object_picture_set_mime_type (...);
            FLAC__metadata_object_picture_set_description (metadata, "Cover", true);
            FLAC__metadata_object_picture_set_data (metadata, coverart_data,len, true);
            metadata->data.picture.type = FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER;

            while (FLAC__metadata_iterator_next (iter));
            if (!FLAC__metadata_iterator_insert_block_after (iter, metadata)) {
                fprintf (stderr, "flac: failed to append picture block to chain\n");
                goto error2;
            }

error2:
            if (fp) {
                deadbeef->fclose (fp);
            }
            if (coverart_data) {
                free (coverart_data);
            }
        }
    }


    if (!FLAC__metadata_chain_write (chain, 1, 0)) {
        trace ("cflac_write_metadata: FLAC__metadata_chain_write failed\n");
        goto error;
    }
    err = 0;
error:
    FLAC__metadata_iterator_delete (iter);
    if (chain) {
        FLAC__metadata_chain_delete (chain);
    }

    return err;
}

static const char *exts[] = { "flac", "oga", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stdflac",
    .plugin.name = "FLAC decoder",
    .plugin.descr = "FLAC decoder using libFLAC",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
    .open = cflac_open,
    .init = cflac_init,
    .free = cflac_free,
    .read = cflac_read,
    .seek = cflac_seek,
    .seek_sample = cflac_seek_sample,
    .insert = cflac_insert,
    .read_metadata = cflac_read_metadata,
    .write_metadata = cflac_write_metadata,
    .exts = exts,
};

DB_plugin_t *
flac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
