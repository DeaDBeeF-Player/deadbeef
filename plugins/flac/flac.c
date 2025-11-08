/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    - Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    - Neither the name of the DeaDBeeF Player nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <FLAC/stream_decoder.h>
#include <FLAC/metadata.h>
#include <limits.h>
#include <deadbeef/deadbeef.h>
#include "../liboggedit/oggedit.h"
#include <deadbeef/strdupa.h>

static ddb_decoder2_t plugin;
static DB_functions_t *deadbeef;

#define trace(...) { deadbeef->log_detailed (&plugin.decoder.plugin, 0, __VA_ARGS__); }

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

typedef struct {
    DB_fileinfo_t info;
    FLAC__StreamDecoder *decoder;
    FLAC__StreamDecoder *flac_decoder;
    int buffersize;
    char *buffer;
    int remaining; // bytes remaining in buffer from last read
    int64_t startsample;
    int64_t endsample;
    int64_t currentsample;
    int64_t totalsamples;
    int flac_critical_error;
    int init_stop_decoding;
    int set_bitrate;
    DB_FILE *file;
    DB_playItem_t *it;

    // used only on insert
    ddb_playlist_t *plt;
    DB_playItem_t *after;
    DB_playItem_t *last;
    const char *fname;
    int bitrate;
    FLAC__StreamMetadata *flac_cue_sheet;

    int got_vorbis_comments;
} flac_info_t;

static void
cflac_add_metadata (DB_playItem_t *it, const char *s, int length);

static int
_update_metadata(ddb_playItem_t *it, FLAC__Metadata_Chain *chain);

static FLAC__IOCallbacks iocb;

// callbacks
FLAC__StreamDecoderReadStatus flac_read_cb (const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    size_t r = deadbeef->fread (buffer, 1, *bytes, info->file);
    *bytes = r;
    if (r == 0) {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus flac_seek_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    int r = deadbeef->fseek (info->file, absolute_byte_offset, SEEK_SET);
    if (r) {
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus flac_tell_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    size_t r = deadbeef->ftell (info->file);
    *absolute_byte_offset = r;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus flac_length_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    size_t pos = deadbeef->ftell (info->file);
    deadbeef->fseek (info->file, 0, SEEK_END);
    *stream_length = deadbeef->ftell (info->file);
    deadbeef->fseek (info->file, pos, SEEK_SET);
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool flac_eof_cb (const FLAC__StreamDecoder *decoder, void *client_data) {
    return 0;
}

static FLAC__StreamDecoderWriteStatus
cflac_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    DB_fileinfo_t *_info = &info->info;

    if (frame->header.blocksize == 0) {
        trace ("flac: blocksize=0 is invalid, aborted.\n");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    int channels = _info->fmt.channels;
    int samplesize = channels * _info->fmt.bps / 8;
    int bytesize = frame->header.blocksize * samplesize;
    if (info->buffersize < bytesize) {
        info->buffersize = bytesize;
        info->buffer = realloc (info->buffer, bytesize);
    }

    int bufsize = info->buffersize - info->remaining;
    int bufsamples = bufsize / samplesize;
    int nsamples = min (bufsamples, frame->header.blocksize);

    char *bufptr = info->buffer + info->remaining;

    int readbytes = frame->header.blocksize * samplesize;

    unsigned bps = FLAC__stream_decoder_get_bits_per_sample(decoder);

    if (bps == 16) {
        for (int i = 0; i <  nsamples; i++) {
            for (int c = 0; c < channels; c++) {
                int32_t sample = inputbuffer[c][i];
                *bufptr++ = sample&0xff;
                *bufptr++ = (sample&0xff00)>>8;
            }
        }
    }
    else if (bps == 24) {
        for (int i = 0; i <  nsamples; i++) {
            for (int c = 0; c < channels; c++) {
                int32_t sample = inputbuffer[c][i];
                *bufptr++ = sample&0xff;
                *bufptr++ = (sample&0xff00)>>8;
                *bufptr++ = (sample&0xff0000)>>16;
            }
        }
    }
    else if (bps == 32) {
        for (int i = 0; i <  nsamples; i++) {
            for (int c = 0; c < channels; c++) {
                int32_t sample = inputbuffer[c][i];
                *((int32_t*)bufptr) = sample;
                bufptr += 4;
            }
        }
    }
    else if (bps == 8) {
        for (int i = 0; i <  nsamples; i++) {
            for (int c = 0; c < channels; c++) {
                int32_t sample = inputbuffer[c][i];
                *bufptr++ = sample&0xff;
            }
        }
    }
    else if (bps & 7) {
        // support for non-byte-aligned bps
        unsigned shift = _info->fmt.bps - bps;
        bps = _info->fmt.bps;
        int nsamples = min(bufsize / samplesize, frame->header.blocksize);
        for (int s = 0; s < nsamples; s++) {
            for (int c = 0; c < channels; c++) {
                FLAC__int32 sample = inputbuffer[c][s] << shift;
                *bufptr++ = sample & 0xff;
                if (bps > 8) {
                    *bufptr++ = (sample>>8) & 0xff;
                    if (bps > 16) {
                        *bufptr++ = (sample>>16) & 0xff;
                        if (bps > 24) {
                            *bufptr++ = (sample>>24) & 0xff;
                        }
                    }
                 }
             }
         }
    }
    else {
        trace ("flac: unsupported bits per sample: %d\n", bps);
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    info->remaining = (int)(bufptr - info->buffer);

    if (readbytes > bufsize) {
        trace ("flac: buffer overflow, distortion will occur\n");
    //    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

inline static int
fix_bps (int bps) {
    int mod = bps & 7;
    return bps - mod + (mod ? 8 : 0);
}

static void
_update_vorbis_comment(flac_info_t *info, const FLAC__StreamMetadata_VorbisComment *vc) {
    for (int i = 0; i < vc->num_comments; i++) {
        const FLAC__StreamMetadata_VorbisComment_Entry *c = &vc->comments[i];
        if (c->length > 0) {
            const char *s = (const char *)c->entry;
            cflac_add_metadata (info->it, s, c->length);
        }
    }
    deadbeef->pl_add_meta (info->it, "title", NULL);
    if (vc->num_comments > 0) {
        uint32_t f = deadbeef->pl_get_item_flags (info->it);
        f &= ~DDB_TAG_MASK;
        f |= DDB_TAG_VORBISCOMMENTS;
        deadbeef->pl_set_item_flags (info->it, f);
    }
    info->got_vorbis_comments = 1;
}

static void
cflac_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)client_data;
    flac_info_t *info = (flac_info_t *)_info;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        info->totalsamples = metadata->data.stream_info.total_samples;
        _info->fmt.samplerate = metadata->data.stream_info.sample_rate;
        _info->fmt.channels = metadata->data.stream_info.channels;
        _info->fmt.bps = fix_bps (metadata->data.stream_info.bits_per_sample);
        for (int i = 0; i < _info->fmt.channels; i++) {
            _info->fmt.channelmask |= 1 << i;
        }
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        _update_vorbis_comment(info, &metadata->data.vorbis_comment);
    }
}

static void
cflac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)client_data;
    flac_info_t *info = (flac_info_t *)_info;
    if (status == FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC
        || status == FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH) {
        return;
    }

    if (status == FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER
        && deadbeef->conf_get_int ("flac.ignore_bad_header_errors", 0)) {
        return;
    }

    if (status == FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM
        && deadbeef->conf_get_int ("flac.ignore_unparsable_stream_errors", 0)) {
        return;
    }
    trace ("cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
    info->flac_critical_error = 1;
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

static flac_info_t *
cflac_open_int (uint32_t hints) {
    flac_info_t *info = calloc(1, sizeof(flac_info_t));
    if (info && hints&DDB_DECODER_HINT_NEED_BITRATE) {
        info->set_bitrate = 1;
    }
    return info;
}

static DB_fileinfo_t *
cflac_open (uint32_t hints) {
    return (DB_fileinfo_t *)cflac_open_int(hints);
}

static DB_fileinfo_t *
cflac_open2 (uint32_t hints, DB_playItem_t *it) {
    flac_info_t *info = cflac_open_int(hints);
    if (!info) {
        return NULL;
    }

    deadbeef->pl_lock();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock();
    info->file = deadbeef->fopen(uri);
    if (!info->file) {
        trace("cflac_open2 failed to open file %s\n", uri);
    }

    return (DB_fileinfo_t *)info;
}

static int
_detect_is_ogg (flac_info_t *info) {
    int is_ogg = 0;
    if (!info->file->vfs->is_streaming ()) {
        int skip = deadbeef->junk_get_leading_size (info->file);
        if (skip > 0) {
            deadbeef->fseek (info->file, skip, SEEK_SET);
        }

        char sign[4];
        if (deadbeef->fread (sign, 1, 4, info->file) != 4) {
            trace ("flac: cflac_init failed to read signature\n");
            return -1;
        }
        deadbeef->fseek (info->file, -4, SEEK_CUR);

        is_ogg = strncmp (sign, "fLaC", 4) ? 1 : 0;
    } else {
        const char *content_type = info->file->vfs->get_content_type(info->file);
        if (info->file->vfs->is_streaming ()) {
            is_ogg = content_type != NULL && (!strcasecmp(content_type, "audio/ogg") || !strcasecmp(content_type, "application/ogg"));
        }
    }

    return is_ogg;
}

static void
_update_stream_info(ddb_playItem_t *it, flac_info_t *info, int is_ogg) {
    const char *filetype_key = info->file->vfs->is_streaming() ? "!FILETYPE" : ":FILETYPE";
    deadbeef->pl_replace_meta (it, filetype_key, is_ogg ? "OggFLAC" : "FLAC");

    char s[100];
    snprintf (s, sizeof (s), "%d", info->info.fmt.channels);
    deadbeef->pl_replace_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", info->info.fmt.bps);
    deadbeef->pl_replace_meta (it, ":BPS", s);
    snprintf (s, sizeof (s), "%d", info->info.fmt.samplerate);
    deadbeef->pl_replace_meta (it, ":SAMPLERATE", s);
}

static int
cflac_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    trace ("cflac_init %s\n", deadbeef->pl_find_meta (it, ":URI"));
    flac_info_t *info = (flac_info_t *)_info;

    info->it = it;
    deadbeef->pl_item_ref(it);

    if (!info->file) {
        deadbeef->pl_lock ();
        const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
        deadbeef->pl_unlock ();
        info->file = deadbeef->fopen (uri);
        if (!info->file) {
            trace ("cflac_init failed to open file %s\n", uri);
            return -1;
        }
    }

    deadbeef->fset_track (info->file, it);

    deadbeef->pl_lock();
    const char *uri = deadbeef->pl_find_meta(it, ":URI");
    const char *ext = strrchr(uri, '.');
    if (ext) {
        ext++;
    }
    deadbeef->pl_unlock();

    int is_ogg = _detect_is_ogg(info);

    FLAC__StreamDecoderInitStatus status;
    info->decoder = FLAC__stream_decoder_new ();
    if (!info->decoder) {
        trace ("FLAC__stream_decoder_new failed\n");
        return -1;
    }
    FLAC__stream_decoder_set_md5_checking (info->decoder, 0);
    status = FLAC__stream_decoder_init_stream (info->decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_length_cb, flac_eof_cb, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, info);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        trace ("cflac_init: FLAC__stream_decoder_init_stream error\n");
        return -1;
    }

    if (is_ogg && FLAC_API_SUPPORTS_OGG_FLAC) {
        info->flac_decoder = info->decoder;
        info->decoder = FLAC__stream_decoder_new ();
        status = FLAC__stream_decoder_init_ogg_stream (info->decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_length_cb, flac_eof_cb, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, info);
        if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
            trace ("cflac_init: FLAC__stream_decoder_init_ogg_stream error\n");
            return -1;
        }

        FLAC__stream_decoder_set_md5_checking(info->decoder, 0);

        if (!FLAC__stream_decoder_reset(info->decoder)) {
            trace("cflac_init: FLAC__stream_decoder_reset failed\n");
            return -1;
        }
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata (info->decoder)) {
        trace ("cflac_init: FLAC__stream_decoder_process_until_end_of_metadata failed\n");
        return -1;
    }

    if (info->file->vfs->is_streaming()) {
        // Handle mid-stream metadata changes (probably doesn't do anything)
        FLAC__stream_decoder_set_metadata_respond_all (info->decoder);
    }

    // bps/samplerate/channels were set by callbacks
    _info->plugin = &plugin.decoder;
    _info->readpos = 0;

    if (_info->fmt.samplerate <= 0) { // not a FLAC stream
        trace ("cflac_init: corrupt/invalid flac stream\n");
        return -1;
    }
    info->bitrate = deadbeef->pl_find_meta_int(it, ":BITRATE", -1);
    _update_stream_info(it, info, is_ogg);

    deadbeef->pl_lock ();
    {
        const char *channelmask = deadbeef->pl_find_meta (it, "WAVEFORMAT_EXTENSIBLE_CHANNELMASK");
        if (channelmask) {
            uint32_t cm = 0;
            if (1 == sscanf (channelmask, "0x%X", &cm)) {
                _info->fmt.channelmask = cm;
            }
        }
    }
    deadbeef->pl_unlock ();

    info->buffersize = 100000;
    info->buffer = malloc (info->buffersize);
    info->remaining = 0;
    int64_t endsample = deadbeef->pl_item_get_endsample (it);
    if (endsample > 0) {
        info->startsample = deadbeef->pl_item_get_startsample (it);
        info->endsample = endsample;
        if (plugin.seek_sample64 (_info, 0) < 0) {
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
        if (info->flac_cue_sheet != NULL) {
            FLAC__metadata_object_delete (info->flac_cue_sheet);
        }
        if (info->decoder != NULL) {
            FLAC__stream_decoder_delete (info->decoder);
        }
        if (info->flac_decoder != NULL) {
            FLAC__stream_decoder_delete (info->flac_decoder);
        }
        if (info->buffer != NULL) {
            free (info->buffer);
        }
        if (info->file != NULL) {
            deadbeef->fclose (info->file);
        }
        if (info->it != NULL) {
            deadbeef->pl_item_unref(info->it);
        }
        free (_info);
    }
}

static int
cflac_read (DB_fileinfo_t *_info, char *bytes, int size) {
    flac_info_t *info = (flac_info_t *)_info;
    if (info->set_bitrate && info->bitrate != deadbeef->streamer_get_apx_bitrate()) {
        deadbeef->streamer_set_bitrate (info->bitrate);
    }

    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (info->endsample >= 0) {
        if (size / samplesize + info->currentsample > info->endsample) {
            size = (int)(info->endsample - info->currentsample + 1) * samplesize;
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
cflac_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
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
cflac_seek_sample (DB_fileinfo_t *_info, int sample) {
    return cflac_seek_sample64(_info, sample);
}

static int
cflac_seek (DB_fileinfo_t *_info, float time) {
    return cflac_seek_sample64 (_info, (int64_t)((double)time * (int64_t)_info->fmt.samplerate));
}

static FLAC__StreamDecoderWriteStatus
cflac_init_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    flac_info_t *info = (flac_info_t *)client_data;
    if (frame->header.blocksize == 0 || info->init_stop_decoding) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static const char *metainfo[] = {
    "ARTIST", "artist",
    "TITLE", "title",
    "ALBUM", "album",
    "TRACKNUMBER", "track",
    "TRACKTOTAL", "numtracks",
    "TOTALTRACKS", "numtracks",
    "DATE", "year",
    "GENRE", "genre",
    "COMMENT", "comment",
    "PERFORMER", "performer",
    "COMPOSER", "composer",
    "ENCODED-BY", "vendor",
    "DISCNUMBER", "disc",
    "DISCTOTAL", "numdiscs",
    "TOTALDISCS", "numdiscs",
    "COPYRIGHT", "copyright",
    "ORIGINALDATE","original_release_time",
    "ORIGINALYEAR","original_release_year",
    "ALBUMARTIST", "ALBUM ARTIST",
    NULL
};

static int
add_track_meta (DB_playItem_t *it, char *track) {
    char *slash = strchr (track, '/');
    if (slash) {
        // split into track/totaltracks
        *slash = 0;
        slash++;
        deadbeef->pl_add_meta (it, "numtracks", slash);
    }
    deadbeef->pl_add_meta (it, "track", track);
    return 0;
}

static int
add_disc_meta (DB_playItem_t *it, char *disc) {
    char *slash = strchr (disc, '/');
    if (slash) {
        // split into disc/totaldiscs
        *slash = 0;
        slash++;
        deadbeef->pl_add_meta (it, "numdiscs", slash);
    }
    deadbeef->pl_add_meta (it, "disc", disc);
    return 0;
}

static void
cflac_add_metadata (DB_playItem_t *it, const char *s, int length) {
    int m;
    for (m = 0; metainfo[m]; m += 2) {
        size_t l = strlen (metainfo[m]);
        if (length > l && !strncasecmp (metainfo[m], s, l) && s[l] == '=') {
            const char *val = s + l + 1;
            if (!strcmp (metainfo[m+1], "track")) {
                add_track_meta (it, strdupa (val));
            }
            else if (!strcmp (metainfo[m+1], "disc")) {
                add_disc_meta (it, strdupa (val));
            }
            else {
                deadbeef->pl_append_meta (it, metainfo[m+1], val);
            }
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
                if (eq[1]) {
                    deadbeef->pl_append_meta (it, key, eq+1);
                }
            }
        }
    }
}

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
        trace ("flac: samplerate=%d, channels=%d, totalsamples=%d\n", metadata->data.stream_info.sample_rate, metadata->data.stream_info.channels, metadata->data.stream_info.total_samples);
        _info->fmt.samplerate = metadata->data.stream_info.sample_rate;
        _info->fmt.channels = metadata->data.stream_info.channels;
        _info->fmt.bps = fix_bps (metadata->data.stream_info.bits_per_sample);
        info->totalsamples = metadata->data.stream_info.total_samples;
        if (metadata->data.stream_info.total_samples > 0) {
            deadbeef->plt_set_item_duration (info->plt, it, metadata->data.stream_info.total_samples / (float)metadata->data.stream_info.sample_rate);
        }
        else {
            deadbeef->plt_set_item_duration (info->plt, it, -1);
        }
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        _update_vorbis_comment(info, &metadata->data.vorbis_comment);
    }
    else if (metadata->type == FLAC__METADATA_TYPE_CUESHEET) {
        if (!info->flac_cue_sheet) {
            info->flac_cue_sheet = FLAC__metadata_object_clone (metadata);
        }
    }
}

static DB_playItem_t *
cflac_insert_with_embedded_cue (ddb_playlist_t *plt, DB_playItem_t *after, DB_playItem_t *origin, const FLAC__StreamMetadata_CueSheet *cuesheet, uint64_t totalsamples, int samplerate) {
    deadbeef->pl_lock ();

    static const char err_invalid_cuesheet[] = "The flac %s has invalid FLAC__METADATA_TYPE_CUESHEET block, which will get ignored. You should remove it using metaflac.\n";
    DB_playItem_t *ins = after;

    // first check if cuesheet is matching the data
    for (int i = 0; i < cuesheet->num_tracks; i++) {
        if (cuesheet->tracks[i].offset > totalsamples) {
            fprintf (stderr, err_invalid_cuesheet, deadbeef->pl_find_meta_raw (origin, ":URI"));
            deadbeef->pl_unlock ();
            return NULL;
        }
    }

    // use libflac to validate the cuesheet as well
    if(!FLAC__format_cuesheet_is_legal (cuesheet, 1, NULL)) {
        fprintf (stderr, err_invalid_cuesheet, deadbeef->pl_find_meta_raw (origin, ":URI"));
        deadbeef->pl_unlock ();
        return NULL;
    }

    const char *uri = deadbeef->pl_find_meta_raw (origin, ":URI");
    const char *dec = deadbeef->pl_find_meta_raw (origin, ":DECODER");
    const char *ftype = "FLAC";
    for (int i = 0; i < cuesheet->num_tracks-1; i++) {
        DB_playItem_t *it = deadbeef->pl_item_alloc_init (uri, dec);
        deadbeef->pl_set_meta_int (it, ":TRACKNUM", i+1);
        deadbeef->pl_set_meta_int (it, "TRACK", i+1);
        char id[100];
        snprintf (id, sizeof (id), "TITLE[%d]", i+1);
        deadbeef->pl_add_meta (it, "title", deadbeef->pl_find_meta (origin, id));
        snprintf (id, sizeof (id), "ARTIST[%d]", i+1);
        deadbeef->pl_add_meta (it, "artist", deadbeef->pl_find_meta (origin, id));
        deadbeef->pl_add_meta (it, "band", deadbeef->pl_find_meta (origin, "artist"));
        int64_t startsample = cuesheet->tracks[i].offset;
        int64_t endsample = cuesheet->tracks[i+1].offset-1;
        deadbeef->pl_item_set_startsample (it, startsample);
        deadbeef->pl_item_set_endsample (it, endsample);
        deadbeef->pl_replace_meta (it, ":FILETYPE", ftype);
        deadbeef->plt_set_item_duration (plt, it, (float)(endsample - startsample + 1) / samplerate);
        after = deadbeef->plt_insert_item (plt, after, it);
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_item_ref (after);

    DB_playItem_t *first = deadbeef->pl_get_next (ins, PL_MAIN);

    if (!first) {
        first = deadbeef->plt_get_first (plt, PL_MAIN);
    }

    if (!first) {
        deadbeef->pl_unlock ();
        return NULL;
    }

    // copy metadata and flags from the source track
    uint32_t f = deadbeef->pl_get_item_flags (origin);
    f |= DDB_IS_SUBTRACK;
    deadbeef->pl_set_item_flags (origin, f);

    deadbeef->pl_items_copy_junk (origin, first, after);
    deadbeef->pl_item_unref (first);

    deadbeef->pl_item_unref (after);

    deadbeef->pl_unlock ();

    return after;
}

static void
cflac_free_temp (DB_fileinfo_t *_info) {
    if (_info) {
        flac_info_t *info = (flac_info_t *)_info;
        if (info->flac_cue_sheet) {
            FLAC__metadata_object_delete (info->flac_cue_sheet);
        }
        if (info->decoder) {
            FLAC__stream_decoder_delete (info->decoder);
        }
        if (info->buffer) {
            free (info->buffer);
        }
        if (info->file) {
            deadbeef->fclose (info->file);
        }
    }
}

static int
cflac_read_metadata (DB_playItem_t *it);

static DB_playItem_t *
cflac_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    DB_playItem_t *it = NULL;
    FLAC__StreamDecoder *flac_decoder = NULL;
    FLAC__StreamDecoder *decoder = NULL;
    flac_info_t info;
    memset (&info, 0, sizeof (info));
    DB_fileinfo_t *_info = &info.info;
    info.fname = fname;
    info.after = after;
    info.last = after;
    info.plt = plt;
    info.file = deadbeef->fopen (fname);
    if (!info.file) {
        goto cflac_insert_fail;
    }

    const char *ext = strrchr (fname, '.');
    if (ext) {
        ext++;
    }

    int is_ogg = _detect_is_ogg(&info);

    info.init_stop_decoding = 0;

    // open decoder for metadata reading
    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (decoder == NULL) {
        trace ("cflac_insert: FLAC__stream_decoder_new failed\n");
        goto cflac_insert_fail;
    }

    // read all metadata
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    FLAC__stream_decoder_set_metadata_respond_all (decoder);

    it = info.it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);;

    status = FLAC__stream_decoder_init_stream (decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_length_cb, flac_eof_cb, cflac_init_write_callback, cflac_init_metadata_callback, cflac_init_error_callback, &info);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK || info.init_stop_decoding) {
        trace ("cflac_insert: FLAC__stream_decoder_init_stream\n");
        goto cflac_insert_fail;
    }

    if (is_ogg && FLAC_API_SUPPORTS_OGG_FLAC) {
        flac_decoder = decoder;
        decoder = FLAC__stream_decoder_new();
        if (decoder == NULL) {
            trace ("cflac_insert: FLAC__stream_decoder_new failed\n");
            goto cflac_insert_fail;
        }
        status = FLAC__stream_decoder_init_ogg_stream (decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_length_cb, flac_eof_cb, cflac_init_write_callback, cflac_init_metadata_callback, cflac_init_error_callback, &info);

        FLAC__stream_decoder_set_md5_checking(decoder, 0);
        FLAC__stream_decoder_set_metadata_respond_all (decoder);

        if (!FLAC__stream_decoder_reset(decoder)) {
            trace("cflac_insert: FLAC__stream_decoder_reset failed\n");
            goto cflac_insert_fail;
        }
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder) || info.init_stop_decoding) {
        trace ("flac: FLAC__stream_decoder_process_until_end_of_metadata [2] failed\n");
        goto cflac_insert_fail;
    }

    if (info.info.fmt.samplerate <= 0) {
        goto cflac_insert_fail;
    }
    int64_t fsize = deadbeef->fgetlength (info.file);
    int is_streaming = info.file->vfs->is_streaming ();

    _update_stream_info(it, &info, is_ogg);

    char s[100];
    snprintf (s, sizeof (s), "%lld", (long long)fsize);
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);

    if ( deadbeef->pl_get_item_duration (it) > 0) {
        if (!is_ogg) {
            FLAC__uint64 position;
            if (FLAC__stream_decoder_get_decode_position (decoder, &position))
                fsize -= position;
        }
#if USE_OGGEDIT
        else {
            const off_t stream_size = oggedit_flac_stream_info(deadbeef->fopen(fname), 0, 0);
            if (stream_size > 0)
                fsize = stream_size;
        }
#endif
        deadbeef->pl_set_meta_int (it, ":BITRATE", (int)roundf(fsize / deadbeef->pl_get_item_duration (it) * 8 / 1000));
    }
    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
    if (flac_decoder != NULL) {
        FLAC__stream_decoder_delete(flac_decoder);
        flac_decoder = NULL;
    }

    deadbeef->fclose (info.file);
    info.file = NULL;

    if (!info.got_vorbis_comments && !is_streaming) {
        cflac_read_metadata (it);
    }

    DB_playItem_t *cue_after = NULL;

    cue_after = deadbeef->plt_process_cue (plt, after, it, info.totalsamples, info.info.fmt.samplerate);

    if (!deadbeef->plt_is_loading_cue (plt) && !cue_after && info.flac_cue_sheet) {
        // try native flac embedded cuesheet
        cue_after = cflac_insert_with_embedded_cue (plt, after, it, &info.flac_cue_sheet->data.cue_sheet, info.totalsamples, info.info.fmt.samplerate);
    }

    if (cue_after) {
        cflac_free_temp (_info);
        deadbeef->pl_item_unref (it);
        return cue_after;
    }

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    cflac_free_temp (_info);
    return after;
cflac_insert_fail:
    if (it) {
        deadbeef->pl_item_unref (it);
    }
    cflac_free_temp (_info);
    return NULL;
}

static size_t
flac_io_read (void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle) {
    return deadbeef->fread (ptr, size, nmemb, (DB_FILE *)handle);
}

static int
flac_io_seek (FLAC__IOHandle handle, FLAC__int64 offset, int whence) {
    return deadbeef->fseek ((DB_FILE *)handle, offset, whence);
}

static FLAC__int64
flac_io_tell (FLAC__IOHandle handle) {
    return deadbeef->ftell ((DB_FILE *)handle);
}

static int
flac_io_eof (FLAC__IOHandle handle) {
    int64_t pos = deadbeef->ftell ((DB_FILE *)handle);
    return pos == deadbeef->fgetlength ((DB_FILE *)handle);
}

static int
flac_io_close (FLAC__IOHandle handle) {
    deadbeef->fclose ((DB_FILE *)handle);
    return 0;
}

static FLAC__IOCallbacks iocb = {
    .read = flac_io_read,
    .write = NULL,
    .seek = flac_io_seek,
    .tell = flac_io_tell,
    .eof = flac_io_eof,
    .close = flac_io_close,
};

static int
_update_metadata(ddb_playItem_t *it, FLAC__Metadata_Chain *chain) {
    FLAC__Metadata_Iterator *iter = NULL;

    iter = FLAC__metadata_iterator_new ();
    if (!iter) {
        trace ("_update_metadata: FLAC__metadata_iterator_new failed\n");
        return -1;
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
                    const char *s = (const char *)c->entry;
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

    return 0;
}

static int
cflac_read_metadata (DB_playItem_t *it) {
    int err = -1;
    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new ();
    if (!chain) {
        trace ("cflac_read_metadata: FLAC__metadata_chain_new failed\n");
        return -1;
    }
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    DB_FILE *file = deadbeef->fopen (uri);
    if (!file) {
        return -1;
    }
    FLAC__bool res = FLAC__metadata_chain_read_with_callbacks (chain, (FLAC__IOHandle)file, iocb);
    if (!res && FLAC__metadata_chain_status(chain) == FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE) {
        res = FLAC__metadata_chain_read_ogg_with_callbacks (chain, (FLAC__IOHandle)file, iocb);
    }
    deadbeef->fclose (file);
    file = NULL;
    if (!res) {
        trace ("cflac_read_metadata: FLAC__metadata_chain_read(_ogg) failed\n");
        goto error;
    }
    FLAC__metadata_chain_merge_padding (chain);

    err = _update_metadata(it, chain);

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

#if USE_OGGEDIT
int
cflac_write_metadata_ogg (DB_playItem_t *it, FLAC__StreamMetadata_VorbisComment *vc)
{
    char fname[PATH_MAX];
    deadbeef->pl_get_meta (it, ":URI", fname, sizeof (fname));

    size_t num_tags = vc->num_comments;
    char **tags = calloc(num_tags+1, sizeof(char **));
    for (size_t i = 0; i < num_tags; i++)
        tags[i] = (char *)vc->comments[i].entry;
    const off_t file_size = oggedit_write_flac_metadata (deadbeef->fopen(fname), fname, 0, (int)num_tags, tags);
    if (file_size <= 0) {
        trace ("cflac_write_metadata_ogg: oggedit_write_flac_metadata failed: code %d\n", file_size);
        return -1;
    }

    free(tags);

    return 0;
}
#endif

int
cflac_write_metadata (DB_playItem_t *it) {
    int err = -1;
    FLAC__Metadata_Chain *chain = NULL;
    FLAC__Metadata_Iterator *iter = NULL;

    chain = FLAC__metadata_chain_new ();
    if (!chain) {
        fprintf (stderr, "cflac_write_metadata: FLAC__metadata_chain_new failed\n");
        return -1;
    }
    deadbeef->pl_lock ();
    FLAC__bool res = FLAC__metadata_chain_read (chain, deadbeef->pl_find_meta (it, ":URI"));
    FLAC__bool isogg = false;
#if USE_OGGEDIT
    if (!res && FLAC__metadata_chain_status(chain) == FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE) {
        isogg = true;
        res = FLAC__metadata_chain_read_ogg (chain, deadbeef->pl_find_meta (it, ":URI"));
    }
#endif
    deadbeef->pl_unlock ();
    if (!res) {
        fprintf (stderr, "cflac_write_metadata: FLAC__metadata_chain_read(_ogg) failed - code %d\n", res);
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
        else {
            data = NULL;
        }
    } while (FLAC__metadata_iterator_next (iter));

    if (data) {
        // delete existing fields
        FLAC__StreamMetadata_VorbisComment *vc = &data->data.vorbis_comment;
        int vc_comments = vc->num_comments;
        for (int i = 0; i < vc_comments; i++) {
            const FLAC__StreamMetadata_VorbisComment_Entry *c = &vc->comments[i];
            if (c->length > 0) {
                FLAC__metadata_object_vorbiscomment_delete_comment (data, i);
                vc_comments--;
                i--;
            }
        }
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

    deadbeef->pl_lock ();
    DB_metaInfo_t *m = deadbeef->pl_get_metadata_head (it);
    while (m) {
        if (strchr (":!_", m->key[0])) {
            break;
        }
        int i;
        for (i = 0; metainfo[i]; i += 2) {
            if (!strcasecmp (metainfo[i+1], m->key)) {
                break;
            }
        }
        const char *val = m->value;
        if (val && *val) {
            const char *end = val + m->valuesize;
            while (val < end) {
                size_t l = strlen (val);
                if (l > 0) {
                    char s[100+l+1];
                    int n = snprintf (s, sizeof (s), "%s=", metainfo[i] ? metainfo[i] : m->key);
                    strncpy (s+n, val, l);
                    *(s+n+l) = 0;
                    FLAC__StreamMetadata_VorbisComment_Entry ent = {
                        .length = (FLAC__uint32)strlen (s),
                        .entry = (FLAC__byte*)s
                    };
                    FLAC__metadata_object_vorbiscomment_append_comment (data, ent, 1);
                }
                val += l+1;
            }
        }
        m = m->next;
    }

    static const char *tag_rg_names[] = {
        "REPLAYGAIN_ALBUM_GAIN",
        "REPLAYGAIN_ALBUM_PEAK",
        "REPLAYGAIN_TRACK_GAIN",
        "REPLAYGAIN_TRACK_PEAK",
        NULL
    };

    // replaygain key names in deadbeef internal metadata
    static const char *ddb_internal_rg_keys[] = {
        ":REPLAYGAIN_ALBUMGAIN",
        ":REPLAYGAIN_ALBUMPEAK",
        ":REPLAYGAIN_TRACKGAIN",
        ":REPLAYGAIN_TRACKPEAK",
        NULL
    };

    // add replaygain values
    for (int n = 0; ddb_internal_rg_keys[n]; n++) {
        if (deadbeef->pl_find_meta (it, ddb_internal_rg_keys[n])) {
            float value = deadbeef->pl_get_item_replaygain (it, n);
            char s[100];
            // https://wiki.hydrogenaud.io/index.php?title=ReplayGain_2.0_specification#Metadata_format
            switch (n) {
            case DDB_REPLAYGAIN_ALBUMGAIN:
            case DDB_REPLAYGAIN_TRACKGAIN:
                snprintf (s, sizeof (s), "%s=%.2f dB", tag_rg_names[n], value);
                break;
            case DDB_REPLAYGAIN_ALBUMPEAK:
            case DDB_REPLAYGAIN_TRACKPEAK:
                snprintf (s, sizeof (s), "%s=%.6f", tag_rg_names[n], value);
                break;
            }
            FLAC__StreamMetadata_VorbisComment_Entry ent = {
                .length = (FLAC__uint32)strlen (s),
                .entry = (FLAC__byte*)s
            };
            FLAC__metadata_object_vorbiscomment_append_comment (data, ent, 1);
        }
    }

    deadbeef->pl_unlock ();

    if (!isogg) {
        res = FLAC__metadata_chain_write (chain, 1, 0);
    }
#if USE_OGGEDIT
    else {
        if (cflac_write_metadata_ogg(it, &data->data.vorbis_comment)) {
            res = 0;
        }
    }
#endif
    if (!res) {
        fprintf (stderr, "cflac_write_metadata: failed to write tags: code %d\n", res);
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

static const char configdialog[] =
    "property \"Ignore bad header errors\" checkbox flac.ignore_bad_header_errors 0;\n"
    "property \"Ignore unparsable stream errors\" checkbox flac.ignore_unparsable_stream_errors 0;\n"
;


static const char *exts[] = { "flac", "oga", NULL };

// define plugin interface
static ddb_decoder2_t plugin = {
    .decoder.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .decoder.plugin.api_vminor = DB_API_VERSION_MINOR,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 0,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "stdflac",
    .decoder.plugin.name = "FLAC decoder",
    .decoder.plugin.descr = "FLAC decoder using libFLAC",
    .decoder.plugin.copyright =
        "Copyright (C) 2009-2013 Oleksiy Yakovenko et al.\n"
        "Uses libFLAC (C) Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson\n"
        "Uses libogg Copyright (c) 2002, Xiph.org Foundation\n"
        "\n"
        "Redistribution and use in source and binary forms, with or without\n"
        "modification, are permitted provided that the following conditions\n"
        "are met:\n"
        "\n"
        "- Redistributions of source code must retain the above copyright\n"
        "notice, this list of conditions and the following disclaimer.\n"
        "\n"
        "- Redistributions in binary form must reproduce the above copyright\n"
        "notice, this list of conditions and the following disclaimer in the\n"
        "documentation and/or other materials provided with the distribution.\n"
        "\n"
        "- Neither the name of the DeaDBeeF Player nor the names of its\n"
        "contributors may be used to endorse or promote products derived from\n"
        "this software without specific prior written permission.\n"
        "\n"
        "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
        "``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
        "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
        "A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR\n"
        "CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n"
        "EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n"
        "PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n"
        "PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n"
        "LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n"
        "NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n"
        "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
    ,
    .decoder.plugin.website = "http://deadbeef.sf.net",
    .decoder.plugin.configdialog = configdialog,
    .decoder.open = cflac_open,
    .decoder.open2 = cflac_open2,
    .decoder.init = cflac_init,
    .decoder.free = cflac_free,
    .decoder.read = cflac_read,
    .decoder.seek = cflac_seek,
    .decoder.seek_sample = cflac_seek_sample,
    .decoder.insert = cflac_insert,
    .decoder.read_metadata = cflac_read_metadata,
    .decoder.write_metadata = cflac_write_metadata,
    .decoder.exts = exts,
    .seek_sample64 = cflac_seek_sample64,
};

DB_plugin_t *
flac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
