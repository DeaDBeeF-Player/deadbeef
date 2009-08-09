/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
    cflac.info.position = 0;
}

void
cflac_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    fprintf(stderr, "cflac: got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

void
cflac_free (void);

int
cflac_init (playItem_t *it) {
    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        printf ("FLAC__stream_decoder_new failed\n");
        return -1;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    status = FLAC__stream_decoder_init_file(decoder, it->fname, cflac_write_callback, cflac_metadata_callback, cflac_error_callback, NULL);
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
    timestart = it->timestart;
    timeend = it->timeend;
    if (timeend > timestart || timeend < 0) {
        // take from cue and seek
//        if (timeend < 0) {
//            // must be last (or broken) track
//            timeend = playlist_current.duration; // set from metainfo
//        }
//        cflac.info.duration = timeend - timestart;
        cflac.seek (0);
    }
//    if (cflac.info.duration == -1) {
//        printf ("FLAC duration calculation failed\n");
//        return -1;
//    }
//    else {
//        //printf ("%s duration %f, start %f (%f), end %f (%f)\n", fname, cflac.info.duration, timestart, start, timeend, end);
//    }

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

static FLAC__StreamDecoderWriteStatus
cflac_init_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const inputbuffer[], void *client_data) {
    if (frame->header.blocksize == 0) {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

typedef struct {
    playItem_t *after;
    playItem_t *last;
    const char *fname;
    int samplerate;
    int nchannels;
    float duration;
} cue_cb_data_t;

void
cflac_init_cue_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    cue_cb_data_t *cb = (cue_cb_data_t *)client_data;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        cb->samplerate = metadata->data.stream_info.sample_rate;
        cb->nchannels = metadata->data.stream_info.channels;
        cb->duration = metadata->data.stream_info.total_samples / (float)metadata->data.stream_info.sample_rate;
    }
    else if (metadata->type == FLAC__METADATA_TYPE_CUESHEET) {
        //printf ("loading embedded cuesheet!\n");
        const FLAC__StreamMetadata_CueSheet *cue = &metadata->data.cue_sheet;
        playItem_t *prev = NULL;
        for (int i = 0; i < cue->num_tracks; i++) {
            FLAC__StreamMetadata_CueSheet_Track *t = &cue->tracks[i];
            if (t->type == 0 && t->num_indices > 0) {
                FLAC__StreamMetadata_CueSheet_Index *idx = t->indices;
                playItem_t *it = malloc (sizeof (playItem_t));
                memset (it, 0, sizeof (playItem_t));
                it->codec = &cflac;
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
                playItem_t *ins = pl_insert_item (cb->last, it);
                if (ins) {
                    cb->last = ins;
                }
                else {
                    pl_item_free (it);
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
        playItem_t *it = NULL;
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
}
void
cflac_init_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    playItem_t *it = (playItem_t *)client_data;
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
                    pl_add_meta (it, "artist", s + 7);
                }
                else if (!strncasecmp (s, "TITLE=", 6)) {
                    pl_add_meta (it, "title", s + 6);
                    title_added = 1;
                }
                else if (!strncasecmp (s, "ALBUM=", 6)) {
                    pl_add_meta (it, "album", s + 6);
                }
                else if (!strncasecmp (s, "TRACKNUMBER=", 12)) {
                    pl_add_meta (it, "track", s + 12);
                }
                else if (!strncasecmp (s, "DATE=", 5)) {
                    pl_add_meta (it, "date", s + 5);
                }
            }
        }
        if (!title_added) {
            pl_add_meta (it, "title", NULL);
        }

//    pl_add_meta (it, "artist", performer);
//    pl_add_meta (it, "album", albumtitle);
//    pl_add_meta (it, "track", track);
//    pl_add_meta (it, "title", title);
    }
//    int *psr = (int *)client_data;
//    *psr = metadata->data.stream_info.sample_rate;
}

playItem_t *
cflac_insert (playItem_t *after, const char *fname) {
    //try embedded cue, and calculate duration
    FLAC__StreamDecoder *decoder = 0;
    FLAC__StreamDecoderInitStatus status;
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        printf ("FLAC__stream_decoder_new failed\n");
        return NULL;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);

    // try embedded cue
    //printf ("trying embedded cue\n");
    cue_cb_data_t cb = {
        .fname = fname,
        .after = after,
        .last = after
    };
    FLAC__stream_decoder_set_metadata_respond_all (decoder);
    status = FLAC__stream_decoder_init_file(decoder, fname, cflac_init_write_callback, cflac_init_cue_metadata_callback, cflac_error_callback, &cb);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        printf ("init_file failed\n");
        FLAC__stream_decoder_delete(decoder);
        return NULL;
    }
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder)) {
        printf ("process_until_end_of_metadata failed\n");
        FLAC__stream_decoder_delete(decoder);
        return NULL;
    }

    FLAC__stream_decoder_delete(decoder);
    if (cb.last != after) {
        //printf ("embedded cue found!\n");
        return cb.last;
    }

    // try external cue
    char cuename[1024];
    snprintf (cuename, 1024, "%s.cue", fname);
    playItem_t *cue_after;
    if ((cue_after = pl_insert_cue (after, cuename, "FLAC")) != NULL) {
        cue_after->timeend = cb.duration;
        cue_after->duration = cue_after->timeend - cue_after->timestart;
        return cue_after;
    }
    int n = strlen (fname) - 4;
    if (n > 0) {
        strncpy (cuename, fname, n);
        strcpy (cuename + n, "cue");
    //    printf ("loading %s\n", cuename);
        if ((cue_after = pl_insert_cue (after, cuename, "FLAC")) != NULL) {
            cue_after->timeend = cb.duration;
            cue_after->duration = cue_after->timeend - cue_after->timestart;
            return cue_after;
        }
    }

    //printf ("adding flac without cue\n");
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        printf ("FLAC__stream_decoder_new failed\n");
        return NULL;
    }
    FLAC__stream_decoder_set_md5_checking(decoder, 0);
    // try single FLAC file without cue
    FLAC__stream_decoder_set_metadata_respond_all (decoder);
    int samplerate = -1;
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->codec = &cflac;
    it->fname = strdup (fname);
    it->tracknum = 0;
    it->timestart = 0;
    it->timeend = 0;
//    it->tracknum = -1;
    status = FLAC__stream_decoder_init_file(decoder, fname, cflac_init_write_callback, cflac_init_metadata_callback, cflac_error_callback, it);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        FLAC__stream_decoder_delete(decoder);
        pl_item_free (it);
        return NULL;
    }
    if (!FLAC__stream_decoder_process_until_end_of_metadata (decoder)) {
        FLAC__stream_decoder_delete(decoder);
        pl_item_free (it);
        return NULL;
    }
#if 0
    if (it->tracknum == -1) { // not a FLAC stream
        FLAC__stream_decoder_delete(decoder);
        pl_item_free (it);
        return NULL;
    }
#endif
    FLAC__stream_decoder_delete(decoder);
    it->filetype = "FLAC";
    after = pl_insert_item (after, it);
    return after;
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
    .insert = cflac_insert,
    .getexts = cflac_getexts
};
