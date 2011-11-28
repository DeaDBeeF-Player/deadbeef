/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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

#ifndef AV_VERSION_INT
#define AV_VERSION_INT(a, b, c) (a<<16 | b<<8 | c)
#endif

#endif

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

#define DEFAULT_EXTS "m4a;wma;aa3;oma;ac3;vqf;amr"

#define EXT_MAX 100

#define FFMPEG_MAX_ANALYZE_DURATION 500000

static char * exts[EXT_MAX] = {NULL};

enum {
    FT_ALAC = 0,
    FT_WMA = 1,
    FT_ATRAC3 = 2,
    FT_VQF = 3,
    FT_AC3 = 4,
    FT_AMR = 5,
    FT_UNKNOWN = 5
};

#define FF_PROTOCOL_NAME "deadbeef"

typedef struct {
    DB_fileinfo_t info;
    AVCodec *codec;
    AVCodecContext *ctx;
    AVFormatContext *fctx;
    AVPacket pkt;
    int stream_id;

    int left_in_packet;
    int have_packet;

    char *buffer; // must be AVCODEC_MAX_AUDIO_FRAME_SIZE
    int left_in_buffer;

    int startsample;
    int endsample;
    int currentsample;
} ffmpeg_info_t;

static DB_playItem_t *current_track;
static DB_fileinfo_t *current_info;

static DB_fileinfo_t *
ffmpeg_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (ffmpeg_info_t));
    memset (_info, 0, sizeof (ffmpeg_info_t));
    return _info;
}

static int
ffmpeg_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    ffmpeg_info_t *info = (ffmpeg_info_t *)_info;
    trace ("ffmpeg init %s\n", deadbeef->pl_find_meta (it, ":URI"));
    // prepare to decode the track
    // return -1 on failure

    int ret;
    int l = strlen (deadbeef->pl_find_meta (it, ":URI"));
    char *uri = alloca (l + sizeof (FF_PROTOCOL_NAME) + 1);
    int i;

    // construct uri
    memcpy (uri, FF_PROTOCOL_NAME, sizeof (FF_PROTOCOL_NAME)-1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME)-1, ":", 1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME), deadbeef->pl_find_meta (it, ":URI"), l);
    uri[sizeof (FF_PROTOCOL_NAME) + l] = 0;
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
    trace ("\033[0;31mffmpeg av_open_input_file\033[37;0m\n");
    current_track = it;
    current_info = _info;
    if ((ret = av_open_input_file(&info->fctx, uri, NULL, 0, NULL)) < 0) {
        current_track = NULL;
        trace ("\033[0;31minfo->fctx is %p, ret %d/%s\033[0;31m\n", info->fctx, ret, strerror(-ret));
        return -1;
    }
    trace ("\033[0;31mav_open_input_file done, ret=%d\033[0;31m\n", ret);
    current_track = NULL;
    current_info = NULL;

    trace ("\033[0;31mffmpeg av_find_stream_info\033[37;0m\n");
    info->stream_id = -1;
    info->fctx->max_analyze_duration = FFMPEG_MAX_ANALYZE_DURATION;
    av_find_stream_info(info->fctx);
    for (i = 0; i < info->fctx->nb_streams; i++)
    {
        info->ctx = info->fctx->streams[i]->codec;
        if (info->ctx->codec_type ==
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
            AVMEDIA_TYPE_AUDIO)
#else
            CODEC_TYPE_AUDIO)
#endif
        {
            info->codec = avcodec_find_decoder (info->ctx->codec_id);
            if (info->codec != NULL) {
                info->stream_id = i;
                break;
            }
        }
    }

    if (info->codec == NULL)
    {
        trace ("ffmpeg can't decode %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }
    trace ("ffmpeg can decode %s\n", deadbeef->pl_find_meta (it, ":URI"));
    trace ("ffmpeg: codec=%s, stream=%d\n", info->codec->name, i);

    if (avcodec_open (info->ctx, info->codec) < 0) {
        trace ("ffmpeg: avcodec_open failed\n");
        return -1;
    }

    deadbeef->pl_replace_meta (it, "!FILETYPE", info->codec->name);

    int bps = av_get_bits_per_sample_format (info->ctx->sample_fmt);
    int samplerate = info->ctx->sample_rate;
    float duration = info->fctx->duration / (float)AV_TIME_BASE;
    trace ("ffmpeg: bits per sample is %d\n", bps);
    trace ("ffmpeg: samplerate is %d\n", samplerate);
    trace ("ffmpeg: duration is %lld/%fsec\n", info->fctx->duration, duration);

    int totalsamples = info->fctx->duration * samplerate / AV_TIME_BASE;
    info->left_in_packet = 0;
    info->left_in_buffer = 0;

    memset (&info->pkt, 0, sizeof (info->pkt));
    info->have_packet = 0;

    int err = posix_memalign ((void **)&info->buffer, 16, AVCODEC_MAX_AUDIO_FRAME_SIZE);
    if (err) {
        fprintf (stderr, "ffmpeg: failed to allocate buffer memory\n");
        return -1;
    }

    // fill in mandatory plugin fields
    _info->plugin = &plugin;
    _info->readpos = 0;
    _info->fmt.bps = bps;
    _info->fmt.channels = info->ctx->channels;
    _info->fmt.samplerate = samplerate;


    int64_t layout = info->ctx->channel_layout;
    if (layout != 0, 0) {
        _info->fmt.channelmask = layout;
    }
    else {
        for (int i = 0; i < _info->fmt.channels; i++) {
            _info->fmt.channelmask |= 1 << i;
        }
    }

    // subtrack info
    info->currentsample = 0;
    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = totalsamples - 1;
    }
    return 0;
}

static void
ffmpeg_free (DB_fileinfo_t *_info) {
    trace ("ffmpeg: free\n");
    ffmpeg_info_t *info = (ffmpeg_info_t*)_info;
    if (info) {
        if (info->buffer) {
            free (info->buffer);
        }
        // free everything allocated in _init and _read
        if (info->have_packet) {
            av_free_packet (&info->pkt);
        }
        if (info->ctx) {
            avcodec_close (info->ctx);
        }
        if (info->fctx) {
            av_close_input_file (info->fctx);
        }
        free (info);
    }
}

static int
ffmpeg_read (DB_fileinfo_t *_info, char *bytes, int size) {
    trace ("ffmpeg_read_int16 %d\n", size);
    ffmpeg_info_t *info = (ffmpeg_info_t*)_info;

    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;

    if (info->endsample >= 0 && info->currentsample + size / samplesize > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * samplesize;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;

    int encsize = 0;
    int decsize = 0;

    while (size > 0) {

        if (info->left_in_buffer > 0) {
//            int sz = min (size, info->left_in_buffer);
            int nsamples = size / samplesize;
            int nsamples_buf = info->left_in_buffer / samplesize;
            nsamples = min (nsamples, nsamples_buf);
            int sz = nsamples * samplesize;
            memcpy (bytes, info->buffer, nsamples*samplesize);
            bytes += nsamples * samplesize;
            size -= nsamples * samplesize;
            if (sz != info->left_in_buffer) {
                memmove (info->buffer, info->buffer+sz, info->left_in_buffer-sz);
            }
            info->left_in_buffer -= sz;
        }

        while (info->left_in_packet > 0 && size > 0) {
            int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
            int len;
            //trace ("in: out_size=%d(%d), size=%d\n", out_size, AVCODEC_MAX_AUDIO_FRAME_SIZE, size);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,25,0)
            len = avcodec_decode_audio3 (info->ctx, (int16_t *)info->buffer, &out_size, &info->pkt);
#else
            len = avcodec_decode_audio2 (info->ctx, (int16_t *)info->buffer, &out_size, info->pkt.data, info->pkt.size);
#endif
            trace ("out: out_size=%d, len=%d\n", out_size, len);
            if (len <= 0) {
                break;
            }
            encsize += len;
            decsize += out_size;
            info->left_in_packet -= len;
            info->left_in_buffer = out_size;
        }
        if (size == 0) {
            break;
        }

        // read next packet
        if (info->have_packet) {
            av_free_packet (&info->pkt);
            info->have_packet = 0;
        }
        int errcount = 0;
        for (;;) {
            int ret;
            if ((ret = av_read_frame (info->fctx, &info->pkt)) < 0) {
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
                trace ("av packet size: %d, numframes: %d\n", info->pkt.size, ret);
                errcount = 0;
            }
            if (ret == -1) {
                break;
            }
            //trace ("idx:%d, stream:%d\n", info->pkt.stream_index, info->stream_id);
            if (info->pkt.stream_index != info->stream_id) {
                av_free_packet (&info->pkt);
                continue;
            }
            //trace ("got packet: size=%d\n", info->pkt.size);
            info->have_packet = 1;
            info->left_in_packet = info->pkt.size;

            if (info->pkt.duration > 0) {
                AVRational *time_base = &info->fctx->streams[info->stream_id]->time_base;
                float sec = (float)info->pkt.duration * time_base->num / time_base->den;
                int bitrate = info->pkt.size/sec;
                if (bitrate > 0) {
                    // FIXME: seems like duration translation is wrong
                    deadbeef->streamer_set_bitrate (bitrate / 100);
                }
            }

            break;
        }
        if (!info->have_packet) {
            break;
        }
    }

    info->currentsample += (initsize-size) / samplesize;
    _info->readpos = (float)info->currentsample / _info->fmt.samplerate;

    return initsize-size;
}

static int
ffmpeg_seek_sample (DB_fileinfo_t *_info, int sample) {
    ffmpeg_info_t *info = (ffmpeg_info_t*)_info;
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
    if (info->have_packet) {
        av_free_packet (&info->pkt);
        info->have_packet = 0;
    }
    sample += info->startsample;
    int64_t tm = (int64_t)sample/ _info->fmt.samplerate * AV_TIME_BASE;
    trace ("ffmpeg: seek to sample: %d, t: %d\n", sample, (int)tm);
    info->left_in_packet = 0;
    info->left_in_buffer = 0;
    if (av_seek_frame (info->fctx, -1, tm, AVSEEK_FLAG_ANY) < 0) {
        trace ("ffmpeg: seek error\n");
        return -1;
    }
    
    // update readpos
    info->currentsample = sample;
    _info->readpos = (float)(sample - info->startsample) / _info->fmt.samplerate;
    return 0;
}

static int
ffmpeg_seek (DB_fileinfo_t *_info, float time) {
    ffmpeg_info_t *info = (ffmpeg_info_t*)_info;
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    return ffmpeg_seek_sample (_info, time * _info->fmt.samplerate);
}

static const char *map[] = {
    "artist", "artist",
    "title", "title",
    "album", "album",
    "track", "track",
    "date", "year",
    "WM/Year", "year",
    "genre", "genre",
    "comment", "comment",
    "performer", "performer",
    "album_artist", "band",
    "composer", "composer",
    "encoder", "encoder",
    "encoded_by", "vendor",
    "disc", "disc",
    "copyright", "copyright",
    "tracktotal", "numtracks",
    "publisher", "publisher",
    NULL
};

static int
ffmpeg_read_metadata_internal (DB_playItem_t *it, AVFormatContext *fctx) {
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52,43,0)
    if (!strlen (fctx->title)) {
        // title is empty, this call will set track title to filename without extension
        deadbeef->pl_add_meta (it, "title", NULL);
    }
    else {
        deadbeef->pl_add_meta (it, "title", fctx->title);
    }
    deadbeef->pl_add_meta (it, "artist", fctx->author);
    deadbeef->pl_add_meta (it, "album", fctx->album);
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
    AVMetadata *md = fctx->metadata;

    for (int m = 0; map[m]; m += 2) {
        AVMetadataTag *tag = NULL;
        do {
            tag = av_metadata_get (md, map[m], tag, AV_METADATA_DONT_STRDUP_KEY | AV_METADATA_DONT_STRDUP_VAL);
            if (tag) {
                deadbeef->pl_append_meta (it, map[m+1], tag->value);
            }
        } while (tag);
    }
    deadbeef->pl_add_meta (it, "title", NULL);
#endif
    return 0;
}

static DB_playItem_t *
ffmpeg_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("ffmpeg_insert %s\n", fname);
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

    fctx->max_analyze_duration = FFMPEG_MAX_ANALYZE_DURATION;
    av_find_stream_info(fctx);
    for (i = 0; i < fctx->nb_streams; i++)
    {
        ctx = fctx->streams[i]->codec;
        if (ctx->codec_type ==
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
            AVMEDIA_TYPE_AUDIO)
#else
            CODEC_TYPE_AUDIO)
#endif
        {
            codec = avcodec_find_decoder(ctx->codec_id);
            if (codec != NULL && !strcasecmp (codec->name, "alac")) { // only open alac streams
                break;
            }
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

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    deadbeef->pl_replace_meta (it, ":FILETYPE", codec->name);

    if (!deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI"))) {
        deadbeef->plt_set_item_duration (plt, it, -1);
    }
    else {
        deadbeef->plt_set_item_duration (plt, it, duration);
    }

    // add metainfo
    ffmpeg_read_metadata_internal (it, fctx);
    
    int64_t fsize = -1;

    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    if (fp) {
        if (!fp->vfs->is_streaming ()) {
            fsize = deadbeef->fgetlength (fp);
        }
        deadbeef->fclose (fp);
    }

    if (fsize >= 0 && duration > 0) {
        char s[100];
        snprintf (s, sizeof (s), "%lld", fsize);
        deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
        snprintf (s, sizeof (s), "%d", av_get_bits_per_sample_format (ctx->sample_fmt));
        deadbeef->pl_add_meta (it, ":BPS", s);
        snprintf (s, sizeof (s), "%d", ctx->channels);
        deadbeef->pl_add_meta (it, ":CHANNELS", s);
        snprintf (s, sizeof (s), "%d", samplerate);
        deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
        int br = (int)roundf(fsize / duration * 8 / 1000);
        snprintf (s, sizeof (s), "%d", br);
        deadbeef->pl_add_meta (it, ":BITRATE", s);
    }

    // free decoder
    avcodec_close (ctx);
    av_close_input_file(fctx);

    // external cuesheet
    DB_playItem_t *cue = deadbeef->plt_insert_cue (plt, after, it, totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue);
        return cue;
    }
    // now the track is ready, insert into playlist
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
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

    if (f->vfs->is_streaming ()) {
        deadbeef->fset_track (f, current_track);
        if (current_info) {
            current_info->file = f;
        }
    }

    h->priv_data = f;
    return 0;
}

static int
ffmpeg_vfs_read(URLContext *h, unsigned char *buf, int size)
{
    trace ("ffmpeg_vfs_read %d\n", size);
    int res = deadbeef->fread (buf, 1, size, h->priv_data);
    return res;
}

static int
ffmpeg_vfs_write(URLContext *h, const unsigned char *buf, int size)
{
    return -1;
}

static int64_t
ffmpeg_vfs_seek(URLContext *h, int64_t pos, int whence)
{
    trace ("ffmpeg_vfs_seek %d %d\n", pos, whence);
    DB_FILE *f = h->priv_data;

    if (whence == AVSEEK_SIZE) {
        return f->vfs->is_streaming () ? -1 : deadbeef->fgetlength (h->priv_data);
    }
    else if (f->vfs->is_streaming ()) {
        return -1;
    }
    else {
        int ret = deadbeef->fseek (h->priv_data, pos, whence);
        return ret;
    }
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

static void
ffmpeg_init_exts (void) {
    deadbeef->conf_lock ();
    const char *new_exts = deadbeef->conf_get_str_fast ("ffmpeg.extensions", DEFAULT_EXTS);
    for (int i = 0; exts[i]; i++) {
        free (exts[i]);
    }
    exts[0] = NULL;

    int n = 0;
    while (*new_exts) {
        if (n >= EXT_MAX) {
            fprintf (stderr, "ffmpeg: too many extensions, max is %d\n", EXT_MAX);
            break;
        }
        const char *e = new_exts;
        while (*e && *e != ';') {
            e++;
        }
        if (e != new_exts) {
            char *ext = malloc (e-new_exts+1);
            memcpy (ext, new_exts, e-new_exts);
            ext[e-new_exts] = 0;
            exts[n++] = ext;
        }
        if (*e == 0) {
            break;
        }
        new_exts = e+1;
    }
    exts[n] = NULL;
    deadbeef->conf_unlock ();
}

static int
ffmpeg_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        ffmpeg_init_exts ();
        break;
    }
    return 0;
}

static int
ffmpeg_start (void) {
    ffmpeg_init_exts ();
    avcodec_init ();
    av_register_all ();
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 69, 0)
    av_register_protocol2 (&vfswrapper, sizeof(vfswrapper));
#else
    av_register_protocol (&vfswrapper);
#endif
    return 0;
}

static int
ffmpeg_stop (void) {
    for (int i = 0; exts[i]; i++) {
        free (exts[i]);
    }
    exts[0] = NULL;
    return 0;
}

int
ffmpeg_read_metadata (DB_playItem_t *it) {
    trace ("ffmpeg_read_metadata: fname %s\n", deadbeef->pl_find_meta (it, ":URI"));
    AVCodec *codec = NULL;
    AVCodecContext *ctx = NULL;
    AVFormatContext *fctx = NULL;
    int ret;
    int l = strlen (deadbeef->pl_find_meta (it, ":URI"));
    char *uri = alloca (l + sizeof (FF_PROTOCOL_NAME) + 1);
    int i;

    // construct uri
    memcpy (uri, FF_PROTOCOL_NAME, sizeof (FF_PROTOCOL_NAME)-1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME)-1, ":", 1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME), deadbeef->pl_find_meta (it, ":URI"), l);
    uri[sizeof (FF_PROTOCOL_NAME) + l] = 0;
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
    if ((ret = av_open_input_file(&fctx, uri, NULL, 0, NULL)) < 0) {
        trace ("fctx is %p, ret %d/%s", fctx, ret, strerror(-ret));
        return -1;
    }

    av_find_stream_info(fctx);
    for (i = 0; i < fctx->nb_streams; i++)
    {
        ctx = fctx->streams[i]->codec;
        if (ctx->codec_type ==
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
            AVMEDIA_TYPE_AUDIO)
#else
            CODEC_TYPE_AUDIO)
#endif
        {
            codec = avcodec_find_decoder(ctx->codec_id);
            if (codec != NULL)
                break;
        }
    }
    if (codec == NULL)
    {
        trace ("ffmpeg can't decode %s\n", deadbeef->pl_find_meta (it, ":URI"));
        av_close_input_file(fctx);
        return -1;
    }
    if (avcodec_open (ctx, codec) < 0) {
        trace ("ffmpeg: avcodec_open failed\n");
        av_close_input_file(fctx);
        return -1;
    }

    deadbeef->pl_delete_all_meta (it);
    ffmpeg_read_metadata_internal (it, fctx);

    av_close_input_file(fctx);
    return 0;
}

static const char settings_dlg[] =
    "property \"File Extensions (separate with ';')\" entry ffmpeg.extensions \"" DEFAULT_EXTS "\";\n"
;

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 2,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "ffmpeg",
    .plugin.name = "FFMPEG audio player",
    .plugin.descr = "decodes audio formats using FFMPEG libavcodec",
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
    .plugin.start = ffmpeg_start,
    .plugin.stop = ffmpeg_stop,
    .plugin.configdialog = settings_dlg,
    .plugin.message = ffmpeg_message,
    .open = ffmpeg_open,
    .init = ffmpeg_init,
    .free = ffmpeg_free,
    .read = ffmpeg_read,
    .seek = ffmpeg_seek,
    .seek_sample = ffmpeg_seek_sample,
    .insert = ffmpeg_insert,
    .read_metadata = ffmpeg_read_metadata,
    .exts = (const char **)exts,
};

DB_plugin_t *
ffmpeg_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

