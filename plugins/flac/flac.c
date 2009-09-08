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

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static FLAC__StreamDecoder *decoder = 0;
#define BUFFERSIZE 100000
static char buffer[BUFFERSIZE]; // this buffer always has int32 samples
static int remaining; // bytes remaining in buffer from last read
static float timestart;
static float timeend;

static FLAC__StreamDecoderWriteStatus
cflac_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    if (frame->header.blocksize == 0) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    int readbytes = frame->header.blocksize * plugin.info.channels * plugin.info.bps / 8;
    int bufsize = BUFFERSIZE-remaining;
    int bufsamples = bufsize / (plugin.info.channels * plugin.info.bps / 8);
    int nsamples = min (bufsamples, frame->header.blocksize);
    char *bufptr = &buffer[remaining];
    int32_t mask = (1<<(frame->header.bits_per_sample-1))-1;
    float scaler = 1.f / ((1<<(frame->header.bits_per_sample-1))-1);
    int32_t neg = 1<<frame->header.bits_per_sample;
    int32_t sign = (1<<31);
    int32_t signshift = frame->header.bits_per_sample-1;

    for (int i = 0; i < nsamples; i++) {
        int32_t sample = inputbuffer[0][i];
        ((float*)bufptr)[0] = (sample - ((sample&sign)>>signshift)*neg) * scaler;
        bufptr += sizeof (float);
        remaining += sizeof (float);
        if (plugin.info.channels > 1) {
            int32_t sample = inputbuffer[1][i];
            ((float*)bufptr)[0] = (sample - ((sample&sign)>>signshift)*neg) * scaler;
            bufptr += sizeof (float);
            remaining += sizeof (float);
        }
    }
    if (readbytes > bufsize) {
        fprintf (stderr, "flac: buffer overflow, distortion will occur\n");
    //    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void
cflac_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    FLAC__uint64 total_samples = metadata->data.stream_info.total_samples;
    int sample_rate = metadata->data.stream_info.sample_rate;
    int channels = metadata->data.stream_info.channels;
    int bps = metadata->data.stream_info.bits_per_sample;
    plugin.info.samplerate = sample_rate;
    plugin.info.channels = channels;
    plugin.info.bps = bps;
    plugin.info.readpos = 0;
}

static void
cflac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
//    fprintf(stderr, "cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

static int cflac_init_stop_decoding;

static void
cflac_init_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
//    fprintf(stderr, "cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
    cflac_init_stop_decoding = 1;
}

static void
cflac_free (void);

static int
cflac_init (DB_playItem_t *it) {
    FILE *fp = fopen (it->fname, "rb");
    if (!fp) {
        return -1;
    }
    int skip = deadbeef->junk_get_leading_size (fp);
    if (skip > 0) {
        fseek (fp, skip, SEEK_SET);
    }
    char sign[4];
    if (fread (sign, 1, 4, fp) != 4) {
        fclose (fp);
        return -1;
    }
    if (strncmp (sign, "fLaC", 4)) {
        fclose (fp);
        return -1;
    }
    fclose (fp);
    fp = NULL;

    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
//        printf ("FLAC__stream_decoder_new failed\n");
        return -1;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    status = FLAC__stream_decoder_init_file(decoder, it->fname, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, NULL);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        cflac_free ();
        return -1;
    }
    plugin.info.samplerate = -1;
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder)) {
        cflac_free ();
        return -1;
    }
    if (plugin.info.samplerate == -1) { // not a FLAC stream
        cflac_free ();
        return -1;
    }
    timestart = it->timestart;
    timeend = it->timeend;
    if (timeend > timestart || timeend < 0) {
        plugin.seek (0);
    }
    plugin.info.readpos = 0;

    remaining = 0;
    return 0;
}

static void
cflac_free (void) {
    if (decoder) {
        FLAC__stream_decoder_delete(decoder);
        decoder = NULL;
    }
}

static int
cflac_read_int16 (char *bytes, int size) {
    int initsize = size;
    int nsamples = size / (plugin.info.channels * plugin.info.bps / 8);
    if (timeend > timestart) {
        if (plugin.info.readpos + timestart > timeend) {
            return 0;
        }
    }
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
            plugin.info.readpos += (float)sz / (plugin.info.channels * plugin.info.samplerate * sizeof (float));
            if (timeend > timestart) {
                if (plugin.info.readpos + timestart > timeend) {
                    break;
                }
            }
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
    int initsize = size;
    int nsamples = size / (plugin.info.channels * plugin.info.bps / 8);
    if (timeend > timestart) {
        if (plugin.info.readpos + timestart > timeend) {
            return 0;
        }
    }
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
            plugin.info.readpos += (float)sz / (plugin.info.channels * plugin.info.samplerate * sizeof (int32_t));
            if (timeend > timestart) {
                if (plugin.info.readpos + timestart > timeend) {
                    break;
                }
            }
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
cflac_seek (float time) {
    time += timestart;
    if (!FLAC__stream_decoder_seek_absolute (decoder, (FLAC__uint64)(time * plugin.info.samplerate))) {
        return -1;
    }
    remaining = 0;
    plugin.info.readpos = time - timestart;
    return 0;
}

static FLAC__StreamDecoderWriteStatus
cflac_init_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    if (frame->header.blocksize == 0 || cflac_init_stop_decoding) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

typedef struct {
    DB_playItem_t *after;
    DB_playItem_t *last;
    const char *fname;
    int samplerate;
    int nchannels;
    float duration;
} cue_cb_data_t;

static void
cflac_init_cue_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    if (cflac_init_stop_decoding) {
        return;
    }
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        cb->samplerate = metadata->data.stream_info.sample_rate;
        cb->nchannels = metadata->data.stream_info.channels;
        cb->duration = metadata->data.stream_info.total_samples / (float)metadata->data.stream_info.sample_rate;
    }
    // {{{ disabled
#if 0
    else if (metadata->type == FLAC__METADATA_TYPE_CUESHEET) {
        //printf ("loading embedded cuesheet!\n");
        const FLAC__StreamMetadata_CueSheet *cue = &metadata->data.cue_sheet;
        DB_playItem_t *prev = NULL;
        for (int i = 0; i < cue->num_tracks; i++) {
            FLAC__StreamMetadata_CueSheet_Track *t = &cue->tracks[i];
            if (t->type == 0 && t->num_indices > 0) {
                FLAC__StreamMetadata_CueSheet_Index *idx = t->indices;
                DB_playItem_t *it = malloc (sizeof (DB_playItem_t));
                memset (it, 0, sizeof (DB_playItem_t));
                it->decoder = &plugin;
                it->fname = strdup (cb->fname);
                it->tracknum = t->number;
                it->timestart = (float)t->offset / cb->samplerate;
                it->timeend = -1; // will be filled by next read
                if (prev) {
                    prev->timeend = it->timestart;
                    prev->duration = prev->timeend - prev->timestart;
                }
                it->filetype = "FLAC";
                //printf ("N: %d, t: %f, bps=%d\n", it->tracknum, it->timestart/60.f, cb->samplerate);
                DB_playItem_t *ins = deadbeef->pl_insert_item (cb->last, it);
                if (ins) {
                    cb->last = ins;
                }
                else {
                    deadbeef->pl_item_free (it);
                    it = NULL;
                }
                prev = it;
            }
        }
        if (prev) {
            prev->timeend = cb->duration;
            prev->duration = prev->timeend - prev->timestart;
        }
    }
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        DB_playItem_t *it = NULL;
        if (cb->after) {
            it = cb->after->next[PL_MAIN];
        }
        else if (cb->last) {
            it = playlist_head[PL_MAIN];
        }
        if (it) {
            for (; it != cb->last->next[PL_MAIN]; it = it->next[PL_MAIN]) {
                char str[10];
                snprintf (str, 10, "%d", it->tracknum);
                pl_add_meta (it, "track", str);
                pl_add_meta (it, "title", NULL);
            }
        }
    }
#endif
// }}}
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        const FLAC__StreamMetadata_VorbisComment *vc = &metadata->data.vorbis_comment;
        for (int i = 0; i < vc->num_comments; i++) {
            const FLAC__StreamMetadata_VorbisComment_Entry *c = &vc->comments[i];
            if (c->length > 0) {
                char s[c->length+1];
                s[c->length] = 0;
                memcpy (s, c->entry, c->length);
                if (!strncasecmp (s, "cuesheet=", 9)) {
                    cb->last = deadbeef->pl_insert_cue_from_buffer (cb->after, cb->fname, s+9, c->length-9, &plugin, "FLAC", cb->duration);
                }
            }
        }
    }
}

static void
cflac_init_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    if (cflac_init_stop_decoding) {
        fprintf (stderr, "error flag is set, ignoring init_metadata callback..\n");
        return;
    }
    DB_playItem_t *it = (DB_playItem_t *)client_data;
    //it->tracknum = 0;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        it->duration = metadata->data.stream_info.total_samples / (float)metadata->data.stream_info.sample_rate;
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
    DB_playItem_t *it = NULL;
    FLAC__StreamDecoder *decoder = NULL;
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        goto cflac_insert_fail;
    }
    // skip id3 junk
    int skip = deadbeef->junk_get_leading_size (fp);
    if (skip > 0) {
        fseek (fp, skip, SEEK_SET);
    }
    char sign[4];
    if (fread (sign, 1, 4, fp) != 4) {
        goto cflac_insert_fail;
    }
    if (strncmp (sign, "fLaC", 4)) {
        goto cflac_insert_fail;
    }
    fclose (fp);
    fp = NULL;
    cflac_init_stop_decoding = 0;
    //try embedded cue, and calculate duration
    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        goto cflac_insert_fail;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);

    // try embedded cue
    cue_cb_data_t cb = {
        .fname = fname,
        .after = after,
        .last = after
    };
    FLAC__stream_decoder_set_metadata_respond_all (decoder);
    status = FLAC__stream_decoder_init_file (decoder, fname, cflac_init_write_callback, cflac_init_cue_metadata_callback, cflac_init_error_callback, &cb);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK || cflac_init_stop_decoding) {
        goto cflac_insert_fail;
    }
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder) || cflac_init_stop_decoding) {
        goto cflac_insert_fail;
    }

    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
    if (cb.last != after) {
        // that means embedded cue is loaded
        return cb.last;
    }

    // try external cue
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, fname, &plugin, "flac", cb.duration);
    if (cue_after) {
        cue_after->timeend = cb.duration;
        cue_after->duration = cue_after->timeend - cue_after->timestart;
        return cue_after;
    }
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        goto cflac_insert_fail;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    // try single FLAC file without cue
    FLAC__stream_decoder_set_metadata_respond_all (decoder);
    int samplerate = -1;
    it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->tracknum = 0;
    it->timestart = 0;
    it->timeend = 0;
    status = FLAC__stream_decoder_init_file (decoder, fname, cflac_init_write_callback, cflac_init_metadata_callback, cflac_init_error_callback, it);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK || cflac_init_stop_decoding) {
        goto cflac_insert_fail;
    }
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder) || cflac_init_stop_decoding) {
        goto cflac_insert_fail;
    }
    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
    it->filetype = "FLAC";
    after = deadbeef->pl_insert_item (after, it);
    return after;
cflac_insert_fail:
    if (it) {
        deadbeef->pl_item_free (it);
    }
    if (decoder) {
        FLAC__stream_decoder_delete(decoder);
    }
    if (fp) {
        fclose (fp);
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
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = cflac_init,
    .free = cflac_free,
    .read_int16 = cflac_read_int16,
    .read_float32 = cflac_read_float32,
    .seek = cflac_seek,
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
