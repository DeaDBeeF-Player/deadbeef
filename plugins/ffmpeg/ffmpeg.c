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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <alloca.h>
#include <errno.h>

#include "../../deadbeef.h"

#if !FFMPEG_OLD

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/avstring.h>

#else

#include <ffmpeg/avformat.h>
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avutil.h>
#include <ffmpeg/avstring.h>
#define AVERROR_EOF AVERROR(EPIPE)

#if LIBAVFORMAT_VERSION_MAJOR < 53
#define av_register_protocol register_protocol
#endif

#endif

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static const char * exts[] = { "m4a", "mpc", "mp+", "mpp", "wma", "shn", "aa3", "oma", "ac3", "vqf", NULL };

enum {
    FT_M4A = 0,
    FT_MUSEPACK = 1,
    FT_WMA = 2,
    FT_SHORTEN = 3,
    FT_ATRAC3 = 4,
    FT_VQF = 5,
    FT_UNKNOWN = 6
};

static const char *filetypes[] = { "M4A", "MusePack", "WMA", "Shorten", "atrac3", "VQF", "FFMPEG", NULL };

#define FF_PROTOCOL_NAME "deadbeef"

static AVCodec *codec;
static AVCodecContext *ctx;
static AVFormatContext *fctx;
static AVPacket pkt;
static int stream_id;

static int left_in_packet;
static int have_packet;

static char *buffer; // must be AVCODEC_MAX_AUDIO_FRAME_SIZE
static int left_in_buffer;

static int startsample;
static int endsample;
static int currentsample;

static int
ffmpeg_init (DB_playItem_t *it) {
    // prepare to decode the track
    // return -1 on failure

    int ret;
    int l = strlen (it->fname);
    char *uri = alloca (l + sizeof (FF_PROTOCOL_NAME) + 1);
    int i;

    // construct uri
    memcpy (uri, FF_PROTOCOL_NAME, sizeof (FF_PROTOCOL_NAME)-1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME)-1, ":", 1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME), it->fname, l);
    uri[sizeof (FF_PROTOCOL_NAME) + l] = 0;
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
    if ((ret = av_open_input_file(&fctx, uri, NULL, 0, NULL)) < 0) {
        trace ("fctx is %p, ret %d/%s\n", fctx, ret, strerror(-ret));
        return -1;
    }

    stream_id = -1;
    av_find_stream_info(fctx);
    for (i = 0; i < fctx->nb_streams; i++)
    {
        ctx = fctx->streams[i]->codec;
        if (ctx->codec_type == CODEC_TYPE_AUDIO)
        {
            codec = avcodec_find_decoder(ctx->codec_id);
            if (codec != NULL) {
                stream_id = i;
                break;
            }
        }
    }

    if (codec == NULL)
    {
        trace ("ffmpeg can't decode %s\n", it->fname);
        av_close_input_file(fctx);
        return -1;
    }
    trace ("ffmpeg can decode %s\n", it->fname);
    trace ("ffmpeg: codec=%s, stream=%d\n", codec->name, i);

    if (avcodec_open (ctx, codec) < 0) {
        trace ("ffmpeg: avcodec_open failed\n");
        av_close_input_file(fctx);
        return -1;
    }

    int bps = av_get_bits_per_sample_format (ctx->sample_fmt);
    int samplerate = ctx->sample_rate;
    float duration = fctx->duration / (float)AV_TIME_BASE;
    trace ("ffmpeg: bits per sample is %d\n", bps);
    trace ("ffmpeg: samplerate is %d\n", samplerate);
    trace ("ffmpeg: duration is %lld/%fsec\n", fctx->duration, duration);

    int totalsamples = fctx->duration * samplerate / AV_TIME_BASE;
    left_in_packet = 0;
    left_in_buffer = 0;

    memset (&pkt, 0, sizeof (pkt));
    have_packet = 0;

    int err = posix_memalign ((void **)&buffer, 16, AVCODEC_MAX_AUDIO_FRAME_SIZE);
    if (err) {
        fprintf (stderr, "ffmpeg: failed to allocate buffer memory\n");
        return -1;
    }

    // fill in mandatory plugin fields
    plugin.info.readpos = 0;
    plugin.info.bps = bps;
    plugin.info.channels = ctx->channels;
    plugin.info.samplerate = samplerate;

    // subtrack info
    currentsample = 0;
    if (it->endsample > 0) {
        startsample = it->startsample;
        endsample = it->endsample;
        plugin.seek_sample (0);
    }
    else {
        startsample = 0;
        endsample = totalsamples - 1;
    }
    return 0;
}

static void
ffmpeg_free (void) {
    if (buffer) {
        free (buffer);
        buffer = NULL;
    }
    // free everything allocated in _init and _read_int16
    if (have_packet) {
        av_free_packet (&pkt);
        have_packet = 0;
    }
    left_in_buffer = 0;
    left_in_packet = 0;
    stream_id = -1;

    if (fctx) {
        av_close_input_file(fctx);
        fctx = NULL;
    }
    if (ctx) {
        avcodec_close (ctx);
        ctx = NULL;
    }
    
    codec = NULL;
}

static int
ffmpeg_read_int16 (char *bytes, int size) {
    // try decode `size' bytes
    // return number of decoded bytes
    // return 0 on EOF

    if (currentsample + size / (2 * plugin.info.channels) > endsample) {
        size = (endsample - currentsample + 1) * 2 * plugin.info.channels;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;

    int encsize = 0;
    int decsize = 0;

    while (size > 0) {

        if (left_in_buffer > 0) {
            int sz = min (size, left_in_buffer);
            memcpy (bytes, buffer, sz);
            if (sz != left_in_buffer) {
                memmove (buffer, buffer+sz, left_in_buffer-sz);
            }
            left_in_buffer -= sz;
            size -= sz;
            bytes += sz;
        }

        while (left_in_packet > 0 && size > 0) {
            int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
            int len;
//            trace ("in: out_size=%d(%d), size=%d\n", out_size, AVCODEC_MAX_AUDIO_FRAME_SIZE, size);
#if (LIBAVCODEC_VERSION_MAJOR <= 52) && (LIBAVCODEC_VERSION_MINOR <= 25)
            len = avcodec_decode_audio2(ctx, (int16_t *)buffer, &out_size, pkt.data, pkt.size);
#else
            len = avcodec_decode_audio3(ctx, (int16_t *)buffer, &out_size, &pkt);
#endif
//            trace ("out: out_size=%d, len=%d\n", out_size, len);
            if (len <= 0) {
                break;
            }
            encsize += len;
            decsize += out_size;
            left_in_packet -= len;
            left_in_buffer = out_size;
        }
        if (size == 0) {
            break;
        }

        // read next packet
        if (have_packet) {
            av_free_packet (&pkt);
            have_packet = 0;
        }
        int errcount = 0;
        for (;;) {
            int ret;
            if ((ret = av_read_frame(fctx, &pkt)) < 0) {
                trace ("ffmpeg: error %d\n", ret);
                if (ret == AVERROR_EOF || ret == -1) {
                    ret = -1;
                    break;
                }
                else {
                    if (++errcount > 4) {
                        trace ("ffmpeg: too many errors in a row (last is %d); interrupting stream\n", ret);
                        ret = -1;
                        break;
                    }
                    else {
                        continue;
                    }
                }
            }
            else {
                errcount = 0;
            }
            if (ret == -1) {
                break;
            }
//            trace ("idx:%d, stream:%d\n", pkt.stream_index, stream_id);
            if (pkt.stream_index != stream_id) {
                av_free_packet (&pkt);
                continue;
            }
//            trace ("got packet: size=%d\n", pkt.size);
            have_packet = 1;
            left_in_packet = pkt.size;

            if (pkt.duration > 0) {
                AVRational *time_base = &fctx->streams[stream_id]->time_base;
                float sec = (float)pkt.duration * time_base->num / time_base->den;
                int bitrate = pkt.size/sec;
                if (bitrate > 0) {
                    // FIXME: seems like duration translation is wrong
                    deadbeef->streamer_set_bitrate (bitrate / 100);
                }
            }

            break;
        }
        if (!have_packet) {
            break;
        }
    }

    currentsample += (initsize-size) / (2 * plugin.info.channels);
    plugin.info.readpos = (float)currentsample / plugin.info.samplerate;

    return initsize-size;
}

static int
ffmpeg_seek_sample (int sample) {
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
    if (have_packet) {
        av_free_packet (&pkt);
        have_packet = 0;
    }
    sample += startsample;
    int64_t tm = (int64_t)sample/ plugin.info.samplerate * AV_TIME_BASE;
    trace ("ffmpeg: seek to sample: %d, t: %d\n", sample, (int)tm);
    left_in_packet = 0;
    left_in_buffer = 0;
    if (av_seek_frame(fctx, -1, tm, AVSEEK_FLAG_ANY) < 0) {
        trace ("ffmpeg: seek error\n");
        return -1;
    }
    
    // update readpos
    currentsample = sample;
    plugin.info.readpos = (float)(sample-startsample) / plugin.info.samplerate;
    return 0;
}

static int
ffmpeg_seek (float time) {
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    return ffmpeg_seek_sample (time * plugin.info.samplerate);
}

static DB_playItem_t *
ffmpeg_insert (DB_playItem_t *after, const char *fname) {
    // read information from the track
    // load/process cuesheet if exists
    // insert track into playlist
    // return track pointer on success
    // return NULL on failure

    AVCodec *codec = NULL;
    AVCodecContext *ctx = NULL;
    AVFormatContext *fctx = NULL;
    int ret;
    int l = strlen (fname);
    char *uri = alloca (l + sizeof (FF_PROTOCOL_NAME) + 1);
    int i;

    // construct uri
    memcpy (uri, FF_PROTOCOL_NAME, sizeof (FF_PROTOCOL_NAME)-1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME)-1, ":", 1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME), fname, l);
    uri[sizeof (FF_PROTOCOL_NAME) + l] = 0;
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
    if ((ret = av_open_input_file(&fctx, uri, NULL, 0, NULL)) < 0) {
        trace ("fctx is %p, ret %d/%s", fctx, ret, strerror(-ret));
        return NULL;
    }

    av_find_stream_info(fctx);
    for (i = 0; i < fctx->nb_streams; i++)
    {
        ctx = fctx->streams[i]->codec;
        if (ctx->codec_type == CODEC_TYPE_AUDIO)
        {
            codec = avcodec_find_decoder(ctx->codec_id);
            if (codec != NULL)
                break;
        }
    }
//    AVStream *stream = fctx->streams[i];

    if (codec == NULL)
    {
        trace ("ffmpeg can't decode %s\n", fname);
        av_close_input_file(fctx);
        return NULL;
    }
    trace ("ffmpeg can decode %s\n", fname);
    trace ("ffmpeg: codec=%s, stream=%d\n", codec->name, i);

    if (avcodec_open (ctx, codec) < 0) {
        trace ("ffmpeg: avcodec_open failed\n");
        av_close_input_file(fctx);
        return NULL;
    }

    int bps = av_get_bits_per_sample_format (ctx->sample_fmt);
    int samplerate = ctx->sample_rate;
    float duration = fctx->duration / (float)AV_TIME_BASE;
//    float duration = stream->duration * stream->time_base.num / (float)stream->time_base.den;
    trace ("ffmpeg: bits per sample is %d\n", bps);
    trace ("ffmpeg: samplerate is %d\n", samplerate);
    trace ("ffmpeg: duration is %f\n", duration);

    int totalsamples = fctx->duration * samplerate / AV_TIME_BASE;

    // find filetype
    const char *filetype;
    const char *ext = fname + strlen(fname) - 1;
    while (ext > fname && *ext != '.') {
        ext--;
    }
    if (*ext == '.') {
        ext++;
    }

    if (!strcasecmp (ext, "m4a")) {
        filetype = filetypes[FT_M4A];
    }
    else if (!strcasecmp (ext, "mpc") || !strcasecmp (ext, "mp+") || !strcasecmp (ext, "mpp")) {
        filetype = filetypes[FT_MUSEPACK];
    }
    else if (!strcasecmp (ext, "wma")) {
        filetype = filetypes[FT_WMA];
    }
    else if (!strcasecmp (ext, "shn")) {
        filetype = filetypes[FT_SHORTEN];
    }
    else if (!strcasecmp (ext, "aa3") || !strcasecmp (ext, "oma") || !strcasecmp (ext, "ac3")) {
        filetype = filetypes[FT_ATRAC3];
    }
    else if (!strcasecmp (ext, "vqf")) {
        filetype = filetypes[FT_VQF];
    }
    else {
        filetype = filetypes[FT_UNKNOWN];
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = filetype;

    deadbeef->pl_set_item_duration (it, duration);

    // add metainfo
#if LIBAVFORMAT_VERSION_INT < (53<<16)
    if (!strlen (fctx->title)) {
        // title is empty, this call will set track title to filename without extension
        deadbeef->pl_add_meta (it, "title", NULL);
    }
    else {
        deadbeef->pl_add_meta (it, "title", fctx->title);
    }
    deadbeef->pl_add_meta (it, "artist", fctx->author);
    deadbeef->pl_add_meta (it, "album", fctx->album);
    deadbeef->pl_add_meta (it, "year", fctx->album);
    deadbeef->pl_add_meta (it, "copyright", fctx->copyright);
    deadbeef->pl_add_meta (it, "comment", fctx->comment);
    deadbeef->pl_add_meta (it, "genre", fctx->genre);

    char tmp[10];
    snprintf (tmp, sizeof (tmp), "%d", fctx->year);
    deadbeef->pl_add_meta (it, "year", tmp);
    snprintf (tmp, sizeof (tmp), "%d", fctx->track);
    deadbeef->pl_add_meta (it, "track", tmp);
#else
// read using other means?
// av_metadata_get?
#endif
    // free decoder
    av_close_input_file(fctx);

    // external cuesheet
    DB_playItem_t *cue = deadbeef->pl_insert_cue (after, it, totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_free (it);
        return cue;
    }
    // now the track is ready, insert into playlist
    after = deadbeef->pl_insert_item (after, it);
    return after;
}

// vfs wrapper for ffmpeg
static int
ffmpeg_vfs_open(URLContext *h, const char *filename, int flags)
{
    DB_FILE *f;
    av_strstart(filename, FF_PROTOCOL_NAME ":", &filename);
    if (flags & URL_WRONLY) {
        return -ENOENT;
    } else {
        f = deadbeef->fopen (filename);
    }

    if (f == NULL)
        return -ENOENT;

    h->priv_data = f;
    return 0;
}

static int
ffmpeg_vfs_read(URLContext *h, unsigned char *buf, int size)
{
    int res = deadbeef->fread (buf, 1, size, h->priv_data);
    return res;
}

static int
ffmpeg_vfs_write(URLContext *h, unsigned char *buf, int size)
{
    return -1;
}

static int64_t
ffmpeg_vfs_seek(URLContext *h, int64_t pos, int whence)
{
    DB_FILE *f = h->priv_data;

    if (whence == AVSEEK_SIZE) {
        return f->vfs->streaming ? -1 : deadbeef->fgetlength (h->priv_data);
    }
    int ret = deadbeef->fseek (h->priv_data, pos, whence);
    return ret;
}

static int
ffmpeg_vfs_close(URLContext *h)
{
    trace ("ffmpeg_vfs_close\n");
    deadbeef->fclose (h->priv_data);
    return 0;
}

static URLProtocol vfswrapper = {
    .name = FF_PROTOCOL_NAME,
    .url_open = ffmpeg_vfs_open,
    .url_read = ffmpeg_vfs_read,
    .url_write = ffmpeg_vfs_write,
    .url_seek = ffmpeg_vfs_seek,
    .url_close = ffmpeg_vfs_close,
};

static int
ffmpeg_start (void) {
    // do one-time plugin initialization here
    // e.g. starting threads for background processing, subscribing to events, etc
    // return 0 on success
    // return -1 on failure
    avcodec_init ();
    av_register_all ();
    av_register_protocol (&vfswrapper);
    return 0;
}

static int
ffmpeg_stop (void) {
    // undo everything done in _start here
    // return 0 on success
    // return -1 on failure
    trace ("ffmpeg stop\n");
    return 0;
}

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "FFMPEG audio player",
    .plugin.descr = "decodes audio formats using FFMPEG libavcodec",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = ffmpeg_start,
    .plugin.stop = ffmpeg_stop,
    .init = ffmpeg_init,
    .free = ffmpeg_free,
    .read_int16 = ffmpeg_read_int16,
//    .read_float32 = ffmpeg_read_float32,
    .seek = ffmpeg_seek,
    .seek_sample = ffmpeg_seek_sample,
    .insert = ffmpeg_insert,
    .exts = exts,
    .id = "ffmpeg",
    .filetypes = filetypes
};

DB_plugin_t *
ffmpeg_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

