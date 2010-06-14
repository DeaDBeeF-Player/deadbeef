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
#include <stdio.h>
#include <neaacdec.h>
#include <mp4ff.h>
#include <stdlib.h>
#include "../../deadbeef.h"
#include "aac_parser.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

#define AAC_BUFFER_SIZE 8192
#define OUT_BUFFER_SIZE 100000

typedef struct {
    DB_fileinfo_t info;
    NeAACDecHandle dec;
    DB_FILE *file;
    mp4ff_t *mp4file;
    int startsample;
    int endsample;
    int currentsample;
    char buffer[AAC_BUFFER_SIZE];
    int remaining;
    int faad_channels;
    char out_buffer[OUT_BUFFER_SIZE];
    int out_remaining;
} aac_info_t;

// allocate codec control structure
static DB_fileinfo_t *
aac_open (void) {
    DB_fileinfo_t *_info = malloc (sizeof (aac_info_t));
    aac_info_t *info = (aac_info_t *)_info;
    memset (info, 0, sizeof (aac_info_t));
    return _info;
}

static uint32_t
aac_fs_read (void *user_data, void *buffer, uint32_t length) {
//    trace ("aac_fs_read\n");
    DB_FILE *fp = (DB_FILE *)user_data;
    return deadbeef->fread (buffer, 1, length, fp);
}

static uint32_t
aac_fs_seek (void *user_data, uint64_t position) {
//    trace ("aac_fs_seek\n");
    DB_FILE *fp = (DB_FILE *)user_data;
    return deadbeef->fseek (fp, position, SEEK_SET);
}

static int
parse_aac_stream(DB_FILE *fp, int *samplerate, int *channels, float *duration, int *totalsamples)
{
    int fsize = deadbeef->fgetlength (fp);
    uint8_t buf[8192];

    int nframesfound = 0;
    int nsamples = 0;
    int stream_sr = 0;
    int stream_ch = 0;

    int eof = 0;
    int bufsize = 0;
    do {
        bufsize = sizeof (buf);
        int bytesread = deadbeef->fread (buf, 1, bufsize, fp);
        if (bytesread != bufsize) {
            eof = 1;
        }
        bufsize = bytesread;

        uint8_t *ptr = buf;
        while (ptr < buf + bufsize - ADTS_HEADER_SIZE*8) {
            int channels, samplerate, bitrate, samples;
            int size = aac_sync (ptr, &channels, &samplerate, &bitrate, &samples);
            if (size == 0) {
                ptr++;
            }
            else {
                //trace ("aac: sync: %d %d %d %d %d\n", channels, samplerate, bitrate, samples, size);
                nframesfound++;
                nsamples += samples;
                if (!stream_sr) {
                    stream_sr = samplerate;
                }
                if (!stream_ch) {
                    stream_ch = channels;
                }
                ptr += size;
            }
        }
    } while (totalsamples && !eof);

    if (!nframesfound || !stream_sr || !nsamples || !bufsize) {
        return -1;
    }

    *samplerate = stream_sr;
    *channels = stream_ch;

    if (totalsamples) {
        *totalsamples = nsamples;
        *duration = nsamples / (float)stream_sr;
    }
    else {
        int totalsamples = fsize / bufsize * nsamples;
        *duration = totalsamples / (float)stream_sr;
        trace ("aac: duration=%f (%d samples @ %d Hz), fsize=%d, bufsize=%d\n", *duration, totalsamples, stream_sr, fsize, bufsize);
    }

    return 0;
}

// returns -1 for error, 0 for mp4, 1 for aac
int
aac_probe (DB_FILE *fp, float *duration, int *samplerate, int *channels, int *totalsamples, int *mp4track) {
    // try mp4

    mp4ff_callback_t cb = {
        .read = aac_fs_read,
        .write = NULL,
        .seek = aac_fs_seek,
        .truncate = NULL,
        .user_data = fp
    };

    *duration = -1;
    mp4ff_t *mp4 = mp4ff_open_read (&cb);
    if (!mp4) {
        trace ("not an mp4 file\n");
        return -1;
    }
    int ntracks = mp4ff_total_tracks (mp4);
    if (ntracks > 0) {
        trace ("m4a container detected, ntracks=%d\n", ntracks);
        int i = -1;
        trace ("looking for mp4 data...\n");
        for (i = 0; i < ntracks; i++) {
            unsigned char*  buff = 0;
            unsigned int    buff_size = 0;
            mp4AudioSpecificConfig mp4ASC;
            mp4ff_get_decoder_config(mp4, i, &buff, &buff_size);
            if(buff){
                int rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
                free(buff);
                if(rc < 0)
                    continue;
                break;
            }
        }

        if (i != ntracks) 
        {
            trace ("mp4 track: %d\n", i);
            *samplerate = mp4ff_get_sample_rate (mp4, i);
            *channels = mp4ff_get_channel_count (mp4, i);
            int samples = mp4ff_num_samples(mp4, i);
            trace ("mp4 nsamples=%d, samplerate=%d\n", samples * 1024, *samplerate);
            *duration = (float)samples * 1024 / (*samplerate);

            if (totalsamples) {
                *totalsamples = samples;
            }
            if (mp4track) {
                *mp4track = i;
            }
            return 0;
        }
    }
    mp4ff_close (mp4);
    trace ("mp4 track not found, looking for aac stream...\n");

    // not an mp4, try raw aac
    deadbeef->rewind (fp);
    if (parse_aac_stream (fp, samplerate, channels, duration, totalsamples) == -1) {
        trace ("aac stream not found\n");
        return -1;
    }
    trace ("found aac stream\n");
    return 1;
}


static int
aac_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    aac_info_t *info = (aac_info_t *)_info;

    info->file = deadbeef->fopen (it->fname);
    if (!info->file) {
        return -1;
    }

    // probe
    float duration;
    int samplerate;
    int channels;
    int totalsamples = 0;

    int skip = deadbeef->junk_get_leading_size (info->file);
    if (skip >= 0) {
        deadbeef->fseek (info->file, skip, SEEK_SET);
    }
    int offs = deadbeef->ftell (info->file);

    int mp4track;
    int res = aac_probe (info->file, &duration, &samplerate, &channels, &totalsamples, &mp4track);
    if (res == -1) {
        return -1;
    }

    if (res == 0) {
        mp4ff_callback_t cb = {
            .read = aac_fs_read,
            .write = NULL,
            .seek = aac_fs_seek,
            .truncate = NULL,
            .user_data = info->file
        };

        info->mp4file = mp4ff_open_read (&cb);
        if (!info->mp4file) {
            return -1;
        }
    }

    duration = (float)totalsamples / samplerate;
    deadbeef->pl_set_item_duration (it, duration);

    _info->bps = 16;
    _info->channels = channels;
    _info->samplerate = samplerate;
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

    deadbeef->fseek (info->file, offs, SEEK_SET);

    info->remaining = deadbeef->fread (info->buffer, 1, AAC_BUFFER_SIZE, info->file);

    info->dec = NeAACDecOpen ();

    if (info->mp4file) {
        unsigned char *buffer = NULL;
        int buffer_size = 0;
        mp4ff_get_decoder_config(info->mp4file, mp4track, &buffer, &buffer_size);
        unsigned long srate;
        unsigned char ch;
        if (NeAACDecInit2(info->dec, buffer, buffer_size, &srate, &ch) < 0) {
            trace ("NeAACDecInit2 returned error\n");
            return -1;
        }
     
        // ...TODO...
    }
    else {
        NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration (info->dec);
        conf->dontUpSampleImplicitSBR = 1;
        NeAACDecSetConfiguration (info->dec, conf);
        unsigned long srate;
        unsigned char ch;
        int consumed = NeAACDecInit (info->dec, info->buffer, info->remaining, &srate, &ch);
        if (consumed < 0) {
            trace ("NeAACDecInit returned %d\n", consumed);
            return -1;
        }
        if (consumed > info->remaining) {
            trace ("NeAACDecInit consumed more than available! wtf?\n");
            return -1;
        }
        if (consumed == info->remaining) {
            info->remaining = 0;
        }
        else if (consumed > 0) {
            memmove (info->buffer, info->buffer + consumed, info->remaining - consumed);
            info->remaining -= consumed;
        }
        info->faad_channels = ch;
    }
//    _info->samplerate = srate;

    // recalculate duration
//    trace ("duration=%f, totalsamples = %d\n", duration, totalsamples);
//    trace ("NeAACDecInit returned samplerate=%d, channels=%d\n", (int)srate, (int)ch);

    return 0;
}

static void
aac_free (DB_fileinfo_t *_info) {
    aac_info_t *info = (aac_info_t *)_info;
    if (info) {
        if (info->file) {
            deadbeef->fclose (info->file);
        }
        if (info->mp4file) {
            mp4ff_close (info->mp4file);
        }
        if (info->dec) {
            NeAACDecClose (info->dec);
        }
        free (info);
    }
}

static int
aac_read_int16 (DB_fileinfo_t *_info, char *bytes, int size) {
    int initsize = size;
    aac_info_t *info = (aac_info_t *)_info;
    int eof = 0;
    int ch = min (_info->channels, 2);
    int sample_size = ch * (_info->bps >> 3);

    while (size > 0) {
        if (info->out_remaining > 0) {
            int n = size / sample_size;
            n = min (info->out_remaining, n);

            char *src = info->out_buffer;
            for (int i = 0; i < n; i++) {
                memcpy (bytes, src, sample_size);
                bytes += sample_size;
                src += info->faad_channels * 2;
            }

            size -= n * sample_size;
            if (n == info->out_remaining) {
                info->out_remaining = 0;
            }
            else {
                memmove (info->out_buffer, src, (info->out_remaining - n) * info->faad_channels * 2);
                info->out_remaining -= n;
            }
            continue;
        }

        if (info->remaining < AAC_BUFFER_SIZE) {
            size_t res = deadbeef->fread (info->buffer + info->remaining, 1, AAC_BUFFER_SIZE-info->remaining, info->file);
            if (res == 0) {
                eof = 1;
            }
        }
        NeAACDecFrameInfo frame_info;
        char *samples = NeAACDecDecode (info->dec, &frame_info, info->buffer, info->remaining);
        if (!samples) {
            //trace ("NeAACDecDecode returned NULL\n");
            break;
        }


        int consumed = frame_info.bytesconsumed;
        if (consumed > info->remaining) {
            trace ("NeAACDecDecode consumed more than available! wtf?\n");
            break;
        }
        if (consumed == info->remaining) {
            info->remaining = 0;
        }
        else if (consumed > 0) {
            memmove (info->buffer, info->buffer + consumed, info->remaining - consumed);
            info->remaining -= consumed;
        }

        if (frame_info.samples > 0) {
            memcpy (info->out_buffer, samples, frame_info.samples * 2);
            info->out_remaining = frame_info.samples / frame_info.channels;
        }
    }

    info->currentsample += (initsize-size) / sample_size;
    return initsize-size;
}

static int
aac_seek_sample (DB_fileinfo_t *_info, int sample) {
    aac_info_t *info = (aac_info_t *)_info;
    
    info->currentsample = sample + info->startsample;
    _info->readpos = (float)sample / _info->samplerate;
    return 0;
}

static int
aac_seek (DB_fileinfo_t *_info, float t) {
    return aac_seek_sample (_info, t * _info->samplerate);
}

static DB_playItem_t *
aac_insert (DB_playItem_t *after, const char *fname) {
    trace ("adding %s\n", fname);
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("not found\n");
        return NULL;
    }
    if (fp->vfs->streaming) {
        trace ("no streaming aac yet (%s)\n", fname);
        deadbeef->fclose (fp);
        return NULL;
    }

    int skip = deadbeef->junk_get_leading_size (fp);
    if (skip >= 0) {
        deadbeef->fseek (fp, skip, SEEK_SET);
    }

    const char *ftype = NULL;
    float duration;
    int samplerate;
    int channels;
    int res = aac_probe (fp, &duration, &samplerate, &channels, NULL, NULL);
    if (res == -1) {
        deadbeef->fclose (fp);
        return NULL;
    }
    else if (res == 0) {
        ftype = "mp4";
    }
    else if (res == 1) {
        ftype = "aac";
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = ftype;
    deadbeef->pl_set_item_duration (it, duration);
//    trace ("duration: %f sec\n", duration);

    // read tags
    if (ftype == "aac") {
        int apeerr = deadbeef->junk_apev2_read (it, fp);
        int v2err = deadbeef->junk_id3v2_read (it, fp);
        int v1err = deadbeef->junk_id3v1_read (it, fp);
    }
    deadbeef->pl_add_meta (it, "title", NULL);

    deadbeef->fclose (fp);

    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

static const char * exts[] = { "aac", "mp4", "m4a", NULL };
static const char *filetypes[] = { "aac", "mp4", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "aac",
    .plugin.name = "AAC decoder based on FAAD2",
    .plugin.descr = "aac (m4a, mp4, ...)  player",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = aac_open,
    .init = aac_init,
    .free = aac_free,
    .read_int16 = aac_read_int16,
    .seek = aac_seek,
    .seek_sample = aac_seek_sample,
    .insert = aac_insert,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
aac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
