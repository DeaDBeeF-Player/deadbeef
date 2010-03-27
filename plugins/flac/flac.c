/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../deadbeef.h"

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
    int startsample;
    int endsample;
    int currentsample;
    int totalsamples;
    int flac_critical_error;
    int init_stop_decoding;
    int bytesread;
    DB_FILE *file;

    // used only on insert
    DB_playItem_t *after;
    DB_playItem_t *last;
    DB_playItem_t *it;
    const char *fname;
} flac_info_t;

// callbacks
FLAC__StreamDecoderReadStatus flac_read_cb (const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    size_t r = deadbeef->fread (buffer, 1, *bytes, info->file);
    info->bytesread += r;
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
    //DB_fileinfo_t *_info = info->info;
    //flac_info_t *info = (flac_info_t *)_info;
    int bitrate = -1;
    float sec = ((float)frame->header.blocksize / frame->header.sample_rate);
    if (info->bytesread != 0 && sec > 0) {
        bitrate = info->bytesread / sec * 8;
    }
    info->bytesread = 0;
    if (bitrate > 0) {
        deadbeef->streamer_set_bitrate (bitrate/1000);
    }
    int bufsize = BUFFERSIZE - info->remaining;
    int bufsamples = bufsize / (_info->channels * _info->bps / 8);
    int nsamples = min (bufsamples, frame->header.blocksize);
    char *bufptr = &info->buffer[info->remaining];
    float mul = 1.f/ ((1 << (_info->bps-1))-1);

    int channels = _info->channels;
    if (channels > 2) {
        channels = 2;
    }
    int readbytes = frame->header.blocksize * channels * _info->bps / 8;

    for (int i = 0; i < nsamples; i++) {
        for (int c = 0; c < channels; c++) {
            int32_t sample = inputbuffer[c][i];
            *((float*)bufptr) = sample * mul;
            bufptr += sizeof (float);
            info->remaining += sizeof (float);
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
    _info->samplerate = metadata->data.stream_info.sample_rate;
    _info->channels = metadata->data.stream_info.channels;
    _info->bps = metadata->data.stream_info.bits_per_sample;
}

static void
cflac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)client_data;
    flac_info_t *info = (flac_info_t *)_info;
    if (status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC) {
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
cflac_init (DB_playItem_t *it) {
    trace ("cflac_init %s\n", it->fname);
    DB_fileinfo_t *_info = malloc (sizeof (flac_info_t));
    flac_info_t *info = (flac_info_t *)_info;
    memset (info, 0, sizeof (flac_info_t));

    info->file = deadbeef->fopen (it->fname);
    if (!info->file) {
        trace ("cflac_init failed to open file\n");
        plugin.free (_info);
        return NULL;
    }

    info->flac_critical_error = 0;

    const char *ext = it->fname + strlen (it->fname);
    while (ext > it->fname && *ext != '/' && *ext != '.') {
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
            plugin.free (_info);
            return NULL;
        }
        if (strncmp (sign, "fLaC", 4)) {
            trace ("cflac_init bad signature\n");
            plugin.free (_info);
            return NULL;
        }
        deadbeef->fseek (info->file, -4, SEEK_CUR);
    }
    else if (!FLAC_API_SUPPORTS_OGG_FLAC) {
        trace ("flac: ogg transport support is not compiled into FLAC library\n");
        plugin.free (_info);
        return NULL;
    }
    else {
        isogg = 1;
    }

    FLAC__StreamDecoderInitStatus status;
    info->decoder = FLAC__stream_decoder_new ();
    if (!info->decoder) {
        trace ("FLAC__stream_decoder_new failed\n");
        plugin.free (_info);
        return NULL;
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
        plugin.free (_info);
        return NULL;
    }
    //_info->samplerate = -1;
    if (!FLAC__stream_decoder_process_until_end_of_metadata (info->decoder)) {
        trace ("cflac_init metadata failed\n");
        plugin.free (_info);
        return NULL;
    }

    // bps/samplerate/channels were set by callbacks
    _info->plugin = &plugin;
    _info->readpos = 0;

    if (_info->samplerate == -1) { // not a FLAC stream
        trace ("cflac_init not a flac stream\n");
        plugin.free (_info);
        return NULL;
    }
    info->buffer = malloc (BUFFERSIZE);
    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        if (plugin.seek_sample (_info, 0) < 0) {
            trace ("cflac_init failed to seek to sample 0\n");
            plugin.free (_info);
            return NULL;
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
        plugin.free (_info);
        return NULL;
    }

    info->remaining = 0;
    return _info;
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
        free (_info);
    }
}

static int
cflac_read_int16 (DB_fileinfo_t *_info, char *bytes, int size) {
    flac_info_t *info = (flac_info_t *)_info;
    if (size / (2 * _info->channels) + info->currentsample > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * 2 * _info->channels;
        trace ("size truncated to %d bytes, cursample=%d, endsample=%d\n", size, info->currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int n_output_channels = _info->channels;
    if (n_output_channels > 2) {
        n_output_channels = 2;
    }
    int initsize = size;
    do {
        if (info->remaining) {
            int n_input_frames = info->remaining / sizeof (float) / n_output_channels;
            int n_output_frames = size / n_output_channels / sizeof (int16_t);
            int n = min (n_input_frames, n_output_frames);

            trace ("flac: [1] if=%d, of=%d, n=%d, rem=%d, size=%d\n", n_input_frames, n_output_frames, n, remaining, size);
            // convert from float to int16
            float *in = (float *)info->buffer;
            for (int i = 0; i < n; i++) {
                *((int16_t *)bytes) = (int16_t)((*in) * 0x7fff);
                size -= sizeof (int16_t);
                bytes += sizeof (int16_t);
                if (n_output_channels == 2) {
                    *((int16_t *)bytes) = (int16_t)((*(in+1)) * 0x7fff);
                    size -= sizeof (int16_t);
                    bytes += sizeof (int16_t);
                }
                in += n_output_channels;
            }
            int sz = n * sizeof (float) * n_output_channels;
            if (sz < info->remaining) {
                memmove (info->buffer, &info->buffer[sz], info->remaining - sz);
            }
            info->remaining -= sz;
            info->currentsample += n;
            _info->readpos += (float)n / _info->samplerate;
            trace ("flac: [2] if=%d, of=%d, n=%d, rem=%d, size=%d\n", n_input_frames, n_output_frames, n, info->remaining, size);
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

static int
cflac_read_float32 (DB_fileinfo_t *_info, char *bytes, int size) {
    flac_info_t *info = (flac_info_t *)_info;
    if (size / (4 * _info->channels) + info->currentsample > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * 4 * _info->channels;
        trace ("size truncated to %d bytes, cursample=%d, endsample=%d\n", size, info->currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int n_output_channels = _info->channels;
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
            _info->readpos += (float)n / _info->samplerate;
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

static int
cflac_seek_sample (DB_fileinfo_t *_info, int sample) {
    flac_info_t *info = (flac_info_t *)_info;
    sample += info->startsample;
    if (!FLAC__stream_decoder_seek_absolute (info->decoder, (FLAC__uint64)(sample))) {
        return -1;
    }
    info->remaining = 0;
    info->currentsample = sample;
    _info->readpos = (float)(sample - info->startsample)/ _info->samplerate;
    return 0;
}

static int
cflac_seek (DB_fileinfo_t *_info, float time) {
    return cflac_seek_sample (_info, time * _info->samplerate);
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

static void
cflac_init_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    DB_fileinfo_t *_info = &info->info;
    if (info->init_stop_decoding) {
        trace ("error flag is set, ignoring init_metadata callback..\n");
        return;
    }
    DB_playItem_t *it = info->it;
    //it->tracknum = 0;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        trace ("flac: samplerate=%d, channels=%d\n", metadata->data.stream_info.sample_rate, metadata->data.stream_info.channels);
        _info->samplerate = metadata->data.stream_info.sample_rate;
        _info->channels = metadata->data.stream_info.channels;
        info->totalsamples = metadata->data.stream_info.total_samples;
        deadbeef->pl_set_item_duration (it, metadata->data.stream_info.total_samples / (float)metadata->data.stream_info.sample_rate);
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        const FLAC__StreamMetadata_VorbisComment *vc = &metadata->data.vorbis_comment;
        int title_added = 0;
        for (int i = 0; i < vc->num_comments; i++) {
            const FLAC__StreamMetadata_VorbisComment_Entry *c = &vc->comments[i];
            if (c->length > 0) {
                char s[c->length+1];
                s[c->length] = 0;
                memcpy (s, c->entry, c->length);
                if (!strncasecmp (s, "ARTIST=", 7)) {
                    deadbeef->pl_add_meta (it, "artist", s + 7);
                }
                else if (!strncasecmp (s, "TITLE=", 6)) {
                    deadbeef->pl_add_meta (it, "title", s + 6);
                    title_added = 1;
                }
                else if (!strncasecmp (s, "ALBUM=", 6)) {
                    deadbeef->pl_add_meta (it, "album", s + 6);
                }
                else if (!strncasecmp (s, "TRACKNUMBER=", 12)) {
                    deadbeef->pl_add_meta (it, "track", s + 12);
                }
                else if (!strncasecmp (s, "DATE=", 5)) {
                    deadbeef->pl_add_meta (it, "year", s + 5);
                }
                else if (!strncasecmp (s, "GENRE=", 6)) {
                    deadbeef->pl_add_meta (it, "genre", s + 6);
                }
                else if (!strncasecmp (s, "COMMENT=", 8)) {
                    deadbeef->pl_add_meta (it, "comment", s + 8);
                }
                else if (!strncasecmp (s, "PERFORMER=", 10)) {
                    deadbeef->pl_add_meta (it, "performer", s + 10);
                }
                else if (!strncasecmp (s, "ENSEMBLE=", 9)) {
                    deadbeef->pl_add_meta (it, "band", s + 9);
                }
                else if (!strncasecmp (s, "COMPOSER=", 9)) {
                    deadbeef->pl_add_meta (it, "composer", s + 9);
                }
                else if (!strncasecmp (s, "ENCODED-BY=", 11)) {
                    deadbeef->pl_add_meta (it, "vendor", s + 11);
                }
                else if (!strncasecmp (s, "DISCNUMBER=", 11)) {
                    deadbeef->pl_add_meta (it, "disc", s + 11);
                }
                else if (!strncasecmp (s, "COPYRIGHT=", 10)) {
                    deadbeef->pl_add_meta (it, "copyright", s + 10);
                }
                else if (!strncasecmp (s, "CUESHEET=", 9)) {
                    deadbeef->pl_add_meta (it, "cuesheet", s + 9);
//                    info->last = deadbeef->pl_insert_cue_from_buffer (info->after, info->fname, s+9, c->length-9, &plugin, "FLAC", info->totalsamples, info->samplerate);
                }
                else if (!strncasecmp (s, "replaygain_album_gain=", 22)) {
                    it->replaygain_album_gain = atof (s + 22);
                }
                else if (!strncasecmp (s, "replaygain_album_peak=", 22)) {
                    it->replaygain_album_peak = atof (s + 22);
                }
                else if (!strncasecmp (s, "replaygain_track_gain=", 22)) {
                    it->replaygain_track_gain = atof (s + 22);
                }
                else if (!strncasecmp (s, "replaygain_track_peak=", 22)) {
                    it->replaygain_track_peak = atof (s + 22);
                }
                else {
                    trace ("found flac meta: %s\n", s);
                }
            }
        }
        if (!title_added) {
            deadbeef->pl_add_meta (it, "title", NULL);
        }
    }
}

static DB_playItem_t *
cflac_insert (DB_playItem_t *after, const char *fname) {
    trace ("flac: inserting %s\n", fname);
    DB_playItem_t *it = NULL;
    FLAC__StreamDecoder *decoder = NULL;
    flac_info_t info;
    memset (&info, 0, sizeof (info));
    info.fname = fname;
    info.after = after;
    info.last = after;
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

#if 0
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    // try embedded cue
    FLAC__stream_decoder_set_metadata_respond_all (decoder);
    status = FLAC__stream_decoder_init_stream (decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_lenght_cb, flac_eof_cb, cflac_init_write_callback, cflac_init_cue_metadata_callback, cflac_init_error_callback, &cb);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK || cflac_init_stop_decoding) {
        trace ("flac: FLAC__stream_decoder_init_stream failed\n");
        goto cflac_insert_fail;
    }
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder) || cflac_init_stop_decoding) {
        trace ("flac: FLAC__stream_decoder_process_until_end_of_metadata failed\n");
        goto cflac_insert_fail;
    }

    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
#endif

    // read all metadata
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    FLAC__stream_decoder_set_metadata_respond_all (decoder);
    it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
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
    it->filetype = isogg ? "OggFLAC" : "FLAC";

    // try embedded cue
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    if (cuesheet) {
        DB_playItem_t *last = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), info.totalsamples, info.info.samplerate);
        if (last) {
            deadbeef->pl_item_unref (it);
            return last;
        }
    }

    // try external cue
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, it, info.totalsamples, info.info.samplerate);
    if (cue_after) {
        if (info.file) {
            deadbeef->fclose (info.file);
        }
        trace ("flac: loaded external cuesheet\n");
        return cue_after;
    }
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        if (info.file) {
            deadbeef->fclose (info.file);
        }
        goto cflac_insert_fail;
    }
    after = deadbeef->pl_insert_item (after, it);
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

static const char *exts[] = { "flac", "ogg", "oga", NULL };

static const char *filetypes[] = { "FLAC", "OggFLAC", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stdflac",
    .plugin.name = "FLAC decoder",
    .plugin.descr = "FLAC decoder using libFLAC",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = cflac_init,
    .free = cflac_free,
    .read_int16 = cflac_read_int16,
    .read_float32 = cflac_read_float32,
    .seek = cflac_seek,
    .seek_sample = cflac_seek_sample,
    .insert = cflac_insert,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
flac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
