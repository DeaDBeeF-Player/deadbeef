/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

static FLAC__StreamDecoder *decoder = 0;
#define BUFFERSIZE 100000
static char *buffer; // this buffer always has float samples
static int remaining; // bytes remaining in buffer from last read
static int startsample;
static int endsample;
static int currentsample;

typedef struct {
    DB_FILE *file;
    DB_playItem_t *after;
    DB_playItem_t *last;
    DB_playItem_t *it;
    const char *fname;
    int samplerate;
    int channels;
    int totalsamples;
    int bps;
    int bytesread;
} cue_cb_data_t;

static cue_cb_data_t flac_callbacks;

// callbacks
FLAC__StreamDecoderReadStatus flac_read_cb (const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data) {
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    size_t r = deadbeef->fread (buffer, 1, *bytes, cb->file);
    cb->bytesread += r;
    *bytes = r;
    if (r == 0) {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus flac_seek_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data) {
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    int r = deadbeef->fseek (cb->file, absolute_byte_offset, SEEK_SET);
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderTellStatus flac_tell_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data) {
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    size_t r = deadbeef->ftell (cb->file);
    *absolute_byte_offset = r;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderLengthStatus flac_lenght_cb (const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data) {
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    size_t pos = deadbeef->ftell (cb->file);
    deadbeef->fseek (cb->file, 0, SEEK_END);
    *stream_length = deadbeef->ftell (cb->file);
    deadbeef->fseek (cb->file, pos, SEEK_SET);
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__bool flac_eof_cb (const FLAC__StreamDecoder *decoder, void *client_data) {
    return 0;
}

static FLAC__StreamDecoderWriteStatus
cflac_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    if (frame->header.blocksize == 0) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    int bitrate = -1;
    float sec = ((float)frame->header.blocksize / frame->header.sample_rate);
    if (cb->bytesread != 0 && sec != 0) {
        bitrate = cb->bytesread / sec * 8;
    }
    cb->bytesread = 0;
    if (bitrate > 0) {
        deadbeef->streamer_set_bitrate (bitrate/1000);
    }
    int readbytes = frame->header.blocksize * plugin.info.channels * plugin.info.bps / 8;
    int bufsize = BUFFERSIZE-remaining;
    int bufsamples = bufsize / (plugin.info.channels * plugin.info.bps / 8);
    int nsamples = min (bufsamples, frame->header.blocksize);
    char *bufptr = &buffer[remaining];
    float mul = 1.f/ ((1 << (plugin.info.bps-1))-1);

    for (int i = 0; i < nsamples; i++) {
        int32_t sample = inputbuffer[0][i];
        *((float*)bufptr) = sample * mul;
        bufptr += sizeof (float);
        remaining += sizeof (float);
        if (plugin.info.channels > 1) {
            int32_t sample = inputbuffer[1][i];
            *((float*)bufptr) = sample * mul;
            bufptr += sizeof (float);
            remaining += sizeof (float);
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
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    cb->totalsamples = metadata->data.stream_info.total_samples;
    cb->samplerate = metadata->data.stream_info.sample_rate;
    cb->channels = metadata->data.stream_info.channels;
    cb->bps = metadata->data.stream_info.bits_per_sample;
}

static void
cflac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
//    fprintf(stderr, "cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

static int cflac_init_stop_decoding;

static void
cflac_init_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    fprintf(stderr, "cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
    if (status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC) {
        cflac_init_stop_decoding = 1;
    }
}

static int
cflac_init (DB_playItem_t *it) {
    trace ("cflac_init %s\n", it->fname);
    memset (&flac_callbacks, 0, sizeof (flac_callbacks));
    flac_callbacks.file = deadbeef->fopen (it->fname);
    if (!flac_callbacks.file) {
        trace ("cflac_init failed to open file\n");
        return -1;
    }
    int skip = deadbeef->junk_get_leading_size (flac_callbacks.file);
    if (skip > 0) {
        deadbeef->fseek (flac_callbacks.file, skip, SEEK_SET);
    }
    char sign[4];
    if (deadbeef->fread (sign, 1, 4, flac_callbacks.file) != 4) {
        plugin.free ();
        trace ("cflac_init failed to read signature\n");
        return -1;
    }
    if (strncmp (sign, "fLaC", 4)) {
        plugin.free ();
        trace ("cflac_init bad signature\n");
        return -1;
    }
    deadbeef->fseek (flac_callbacks.file, -4, SEEK_CUR);

    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        trace ("FLAC__stream_decoder_new failed\n");
        return -1;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    status = FLAC__stream_decoder_init_stream (decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_lenght_cb, flac_eof_cb, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, &flac_callbacks);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        plugin.free ();
        trace ("cflac_init bad decoder status\n");
        return -1;
    }
    //plugin.info.samplerate = -1;
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder)) {
        plugin.free ();
        trace ("cflac_init metadata failed\n");
        return -1;
    }
    plugin.info.samplerate = flac_callbacks.samplerate;
    plugin.info.channels = flac_callbacks.channels;
    plugin.info.bps = flac_callbacks.bps;
    plugin.info.readpos = 0;
    if (plugin.info.samplerate == -1) { // not a FLAC stream
        plugin.free ();
        trace ("cflac_init not a flac stream\n");
        return -1;
    }
    buffer = malloc (BUFFERSIZE);
    if (it->endsample > 0) {
        startsample = it->startsample;
        endsample = it->endsample;
        if (plugin.seek_sample (0) < 0) {
            plugin.free ();
            trace ("cflac_init failed to seek to sample 0\n");
            return -1;
        }
        trace ("flac(cue): startsample=%d, endsample=%d, totalsamples=%d, currentsample=%d\n", startsample, endsample, flac_callbacks.totalsamples, currentsample);
    }
    else {
        startsample = 0;
        endsample = flac_callbacks.totalsamples-1;
        currentsample = 0;
        trace ("flac: startsample=%d, endsample=%d, totalsamples=%d\n", startsample, endsample, flac_callbacks.totalsamples);
    }

    remaining = 0;
    return 0;
}

static void
cflac_free (void) {
    if (decoder) {
        FLAC__stream_decoder_delete(decoder);
        decoder = NULL;
    }
    if (buffer) {
        free (buffer);
        buffer = NULL;
    }
}

static int
cflac_read_int16 (char *bytes, int size) {
    if (size / (2 * plugin.info.channels) + currentsample > endsample) {
        size = (endsample - currentsample + 1) * 2 * plugin.info.channels;
        trace ("size truncated to %d bytes, cursample=%d, endsample=%d\n", size, currentsample, endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int initsize = size;
    int nbytes_in = 0;
    do {
        if (remaining) {
            int s = size * 2;
            int sz = min (remaining, s);
            // convert from float to int16
            float *in = (float *)buffer;
            for (int i = 0; i < sz/4; i++) {
                *((int16_t *)bytes) = (int16_t)((*in) * 0x7fff);
                size -= 2;
                bytes += 2;
                in++;
            }
            if (sz < remaining) {
                memmove (buffer, &buffer[sz], remaining-sz);
            }
            remaining -= sz;
            currentsample += sz / (4 * plugin.info.channels);
            plugin.info.readpos += (float)sz / (plugin.info.channels * plugin.info.samplerate * sizeof (float));
        }
        if (!size) {
            break;
        }
        if (!FLAC__stream_decoder_process_single (decoder)) {
            break;
        }
        if (FLAC__stream_decoder_get_state (decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
            break;
        }
    } while (size > 0);

    return initsize - size;
}

static int
cflac_read_float32 (char *bytes, int size) {
    if (size / (4 * plugin.info.channels) + currentsample > endsample) {
        size = (endsample - currentsample + 1) * 4 * plugin.info.channels;
        trace ("size truncated to %d bytes, cursample=%d, endsample=%d\n", size, currentsample, endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int initsize = size;
    do {
        if (remaining) {
            int sz = min (remaining, size);
            memcpy (bytes, buffer, sz);
            size -= sz;
            bytes += sz;
            if (sz < remaining) {
                memmove (buffer, &buffer[sz], remaining-sz);
            }
            remaining -= sz;
            currentsample += sz / (4 * plugin.info.channels);
            plugin.info.readpos += (float)sz / (plugin.info.channels * plugin.info.samplerate * sizeof (int32_t));
        }
        if (!size) {
            break;
        }
        if (!FLAC__stream_decoder_process_single (decoder)) {
            break;
        }
        if (FLAC__stream_decoder_get_state (decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
            break;
        }
    } while (size > 0);

    return initsize - size;
}

static int
cflac_seek_sample (int sample) {
    sample += startsample;
    if (!FLAC__stream_decoder_seek_absolute (decoder, (FLAC__uint64)(sample))) {
        return -1;
    }
    remaining = 0;
    currentsample = sample;
    plugin.info.readpos = (float)(sample - startsample)/ plugin.info.samplerate;
    return 0;
}

static int
cflac_seek (float time) {
    return cflac_seek_sample (time * plugin.info.samplerate);
}

static FLAC__StreamDecoderWriteStatus
cflac_init_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    if (frame->header.blocksize == 0 || cflac_init_stop_decoding) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void
cflac_init_cue_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    if (cflac_init_stop_decoding) {
        trace ("flac: cflac_init_cue_metadata_callback: cflac_init_stop_decoding=1\n");
        return;
    }
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        trace ("flac: cflac_init_cue_metadata_callback: got FLAC__METADATA_TYPE_STREAMINFO\n");
        cb->samplerate = metadata->data.stream_info.sample_rate;
        cb->channels = metadata->data.stream_info.channels;
        //cb->duration = metadata->data.stream_info.total_samples / (float)metadata->data.stream_info.sample_rate;
        cb->totalsamples = metadata->data.stream_info.total_samples;
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        trace ("flac: cflac_init_cue_metadata_callback: got FLAC__METADATA_TYPE_VORBIS_COMMENT\n");
        const FLAC__StreamMetadata_VorbisComment *vc = &metadata->data.vorbis_comment;
        for (int i = 0; i < vc->num_comments; i++) {
            const FLAC__StreamMetadata_VorbisComment_Entry *c = &vc->comments[i];
            if (c->length > 0) {
                char s[c->length+1];
                s[c->length] = 0;
                memcpy (s, c->entry, c->length);
                if (!strncasecmp (s, "cuesheet=", 9)) {
                    cb->last = deadbeef->pl_insert_cue_from_buffer (cb->after, cb->fname, s+9, c->length-9, &plugin, "FLAC", cb->totalsamples, cb->samplerate);
                }
            }
        }
    }
}

static void
cflac_init_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    if (cflac_init_stop_decoding) {
        trace ("error flag is set, ignoring init_metadata callback..\n");
        return;
    }
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    DB_playItem_t *it = cb->it;
    //it->tracknum = 0;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
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
                    deadbeef->pl_add_meta (it, "date", s + 5);
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

//    pl_add_meta (it, "artist", performer);
//    pl_add_meta (it, "album", albumtitle);
//    pl_add_meta (it, "track", track);
//    pl_add_meta (it, "title", title);
    }
//    int *psr = (int *)client_data;
//    *psr = metadata->data.stream_info.sample_rate;
}

static DB_playItem_t *
cflac_insert (DB_playItem_t *after, const char *fname) {
    trace ("flac: inserting %s\n", fname);
    DB_playItem_t *it = NULL;
    FLAC__StreamDecoder *decoder = NULL;
    cue_cb_data_t cb = {
        .fname = fname,
        .after = after,
        .last = after
    };
    cb.file = deadbeef->fopen (fname);
    if (!cb.file) {
        goto cflac_insert_fail;
    }
    // skip id3 junk
    int skip = deadbeef->junk_get_leading_size (cb.file);
    if (skip > 0) {
        deadbeef->fseek (cb.file, skip, SEEK_SET);
    }
    char sign[4];
    if (deadbeef->fread (sign, 1, 4, cb.file) != 4) {
        trace ("flac: failed to read signature\n");
        goto cflac_insert_fail;
    }
    if (strncmp (sign, "fLaC", 4)) {
        trace ("flac: file signature is not fLaC\n");
        goto cflac_insert_fail;
    }
    deadbeef->fseek (cb.file, -4, SEEK_CUR);
    cflac_init_stop_decoding = 0;
    //try embedded cue, and calculate duration
    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        trace ("flac: failed to create decoder\n");
        goto cflac_insert_fail;
    }
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
    if (cb.last != after) {
        trace ("flac: loaded embedded cuesheet\n");
        // that means embedded cue is loaded
        if (cb.file) {
            deadbeef->fclose (cb.file);
        }
        return cb.last;
    }

    // try external cue
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, fname, &plugin, "FLAC", cb.totalsamples, cb.samplerate);
    if (cue_after) {
        if (cb.file) {
            deadbeef->fclose (cb.file);
        }
        trace ("flac: loaded external cuesheet\n");
        return cue_after;
    }
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        if (cb.file) {
            deadbeef->fclose (cb.file);
        }
        goto cflac_insert_fail;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    // try single FLAC file without cue
    FLAC__stream_decoder_set_metadata_respond_all (decoder);
    int samplerate = -1;
    it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    cb.it = it;
    if (skip > 0) {
        deadbeef->fseek (cb.file, skip, SEEK_SET);
    }
    else {
        deadbeef->rewind (cb.file);
    }
    deadbeef->fseek (cb.file, -4, SEEK_CUR);
    status = FLAC__stream_decoder_init_stream (decoder, flac_read_cb, flac_seek_cb, flac_tell_cb, flac_lenght_cb, flac_eof_cb, cflac_init_write_callback, cflac_init_metadata_callback, cflac_init_error_callback, &cb);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK || cflac_init_stop_decoding) {
        trace ("flac: FLAC__stream_decoder_init_stream [2] failed\n");
        goto cflac_insert_fail;
    }
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder) || cflac_init_stop_decoding) {
        trace ("flac: FLAC__stream_decoder_process_until_end_of_metadata [2] failed\n");
        goto cflac_insert_fail;
    }
    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
    it->filetype = "FLAC";
    after = deadbeef->pl_insert_item (after, it);
    if (cb.file) {
        deadbeef->fclose (cb.file);
    }
    return after;
cflac_insert_fail:
    if (it) {
        deadbeef->pl_item_free (it);
    }
    if (decoder) {
        FLAC__stream_decoder_delete(decoder);
    }
    if (cb.file) {
        deadbeef->fclose (cb.file);
    }
    return NULL;
}

static const char *exts[] = { "flac", NULL };

static const char *filetypes[] = { "FLAC", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
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
    .id = "stdflac",
    .filetypes = filetypes
};

DB_plugin_t *
flac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
