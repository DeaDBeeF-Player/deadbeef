/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../strdupa.h"

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/avstring.h>


#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(54, 6, 0)
#define avformat_find_stream_info(ctx,data) av_find_stream_info(ctx)
#define avcodec_open2(ctx,codec,data) avcodec_open(ctx,codec)
#endif

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 17, 0)
#define avformat_free_context(ctx) av_close_input_file(ctx)
#endif

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(55, 28, 0)
#    define avcodec_free_frame(frame) av_frame_free(frame)
#elif LIBAVCODEC_VERSION_INT > AV_VERSION_INT(54, 59, 100)
     // has avcodec_free_frame
#elif LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 40, 0)
#    define avcodec_free_frame(frame) av_freep(frame) // newest -- av_freep
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 8, 0)
#define av_packet_unref(packet) av_free_packet(packet)
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 64, 0)
#define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
#endif


#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 28, 1)
#define av_frame_alloc() avcodec_alloc_frame()
#endif

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 2, 0)
#define avformat_open_input(ctx, uri, fmt, options) av_open_input_file(ctx, uri, NULL, fmt, options)
#endif

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

#define DEFAULT_EXTS "aa3;oma;ac3;vqf;amr;tak;dsf;dff;wma;3gp;mp4;m4a"
#define UNPOPULATED_EXTS_BY_FFMPEG \
    "aif,aiff,afc,aifc,amr,asf," \
    "wmv,wma,au,caf,webm," \
    "gxf,lbc,mmf,mpg,mpeg,ts,m2t," \
    "m2ts,mts,mxf,rm,ra,roq,sox," \
    "spdif,swf,rcv,voc,w64,wav,wv"

#define EXT_MAX 1024

static char * exts[EXT_MAX+1] = {NULL};

enum {
    FT_ALAC = 0,
    FT_WMA = 1,
    FT_ATRAC3 = 2,
    FT_VQF = 3,
    FT_AC3 = 4,
    FT_AMR = 5,
    FT_UNKNOWN = 5
};

typedef struct {
    DB_fileinfo_t info;
    AVCodec *codec;
    AVCodecContext *ctx;
    AVFormatContext *fctx;
    AVPacket pkt;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 40, 0)
    AVFrame *frame;
#endif
    int stream_id;

    int left_in_packet;
    int have_packet;

    char *buffer;
    int left_in_buffer;
    int buffer_size;

    int64_t startsample;
    int64_t endsample;
    int64_t currentsample;
} ffmpeg_info_t;

static DB_fileinfo_t *
ffmpeg_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (ffmpeg_info_t));
    memset (_info, 0, sizeof (ffmpeg_info_t));
    return _info;
}

// ensure that the buffer can contain entire frame of frame_size bytes per channel
static int
ensure_buffer (ffmpeg_info_t *info, int frame_size) {
    if (!info->buffer || info->buffer_size < frame_size * info->ctx->channels) {
        if (info->buffer) {
            free (info->buffer);
            info->buffer = NULL;
        }
        info->buffer_size = frame_size*info->ctx->channels;
        info->left_in_buffer = 0;
        int err = posix_memalign ((void **)&info->buffer, 16, info->buffer_size);
        if (err) {
            fprintf (stderr, "ffmpeg: failed to allocate %d bytes of buffer memory\n", info->buffer_size);
            return -1;
        }
    }
    return 0;
}

static int
ffmpeg_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    // Don't allow playing network streams.
    // Even when ffmpeg has network support, it's causing too many problems this way.
    const char *fname = deadbeef->pl_find_meta (it, ":URI");
    if (!deadbeef->is_local_file (fname)) {
        return -1;
    }

    ffmpeg_info_t *info = (ffmpeg_info_t *)_info;
    trace ("ffmpeg init %s\n", deadbeef->pl_find_meta (it, ":URI"));
    // prepare to decode the track
    // return -1 on failure

    int ret;
    char *uri = NULL;
    int i;

    deadbeef->pl_lock ();
    {
        const char *fname = deadbeef->pl_find_meta (it, ":URI");
        uri = strdupa (fname);
    }
    deadbeef->pl_unlock ();
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
    trace ("\033[0;31mffmpeg av_open_input_file\033[37;0m\n");

    info->fctx = avformat_alloc_context ();

    if ((ret = avformat_open_input(&info->fctx, uri, NULL, NULL)) < 0) {
        trace ("\033[0;31minfo->fctx is %p, ret %d/%s\033[0;31m\n", info->fctx, ret, strerror(-ret));
        return -1;
    }
    trace ("\033[0;31mav_open_input_file done, ret=%d\033[0;31m\n", ret);

    trace ("\033[0;31mffmpeg avformat_find_stream_info\033[37;0m\n");
    info->stream_id = -1;
    info->fctx->max_analyze_duration = AV_TIME_BASE;
    avformat_find_stream_info(info->fctx, NULL);
    for (i = 0; i < info->fctx->nb_streams; i++)
    {
        info->ctx = info->fctx->streams[i]->codec;
        if (info->ctx->codec_type == AVMEDIA_TYPE_AUDIO)
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

    if (avcodec_open2 (info->ctx, info->codec, NULL) < 0) {
        trace ("ffmpeg: avcodec_open2 failed\n");
        return -1;
    }

    deadbeef->pl_replace_meta (it, "!FILETYPE", info->codec->name);

    int bps = av_get_bytes_per_sample (info->ctx->sample_fmt)*8;
    int samplerate = info->ctx->sample_rate;

    if (bps <= 0 || info->ctx->channels <= 0 || samplerate <= 0) {
        return -1;
    }

    int64_t totalsamples = info->fctx->duration * samplerate / AV_TIME_BASE;
    info->left_in_packet = 0;
    info->left_in_buffer = 0;

    memset (&info->pkt, 0, sizeof (info->pkt));
    info->have_packet = 0;

    info->frame = av_frame_alloc();

    // fill in mandatory plugin fields
    _info->plugin = &plugin;
    _info->readpos = 0;
    _info->fmt.bps = bps;
    _info->fmt.channels = info->ctx->channels;
    _info->fmt.samplerate = samplerate;
    if (info->ctx->sample_fmt == AV_SAMPLE_FMT_FLT || info->ctx->sample_fmt == AV_SAMPLE_FMT_FLTP) {
        _info->fmt.is_float = 1;
    }

    // FIXME: channel layout from ffmpeg
    // int64_t layout = info->ctx->channel_layout;

    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }

    // subtrack info
    info->currentsample = 0;
    int64_t endsample = deadbeef->pl_item_get_endsample (it);
    if (endsample > 0) {
        info->startsample = deadbeef->pl_item_get_startsample (it);
        info->endsample = endsample;
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
        if (info->frame) {
            avcodec_free_frame(&info->frame);
        }
        if (info->buffer) {
            free (info->buffer);
        }
        // free everything allocated in _init and _read
        if (info->have_packet) {
            av_packet_unref (&info->pkt);
        }
        if (info->ctx) {
            avcodec_close (info->ctx);
        }
        if (info->fctx) {
            avformat_free_context (info->fctx);
        }
        free (info);
    }
}

static int
ffmpeg_read (DB_fileinfo_t *_info, char *bytes, int size) {
    trace ("ffmpeg_read_int16 %d\n", size);
    ffmpeg_info_t *info = (ffmpeg_info_t*)_info;

    _info->fmt.channels = info->ctx->channels;
    _info->fmt.samplerate = info->ctx->sample_rate;
    _info->fmt.bps = av_get_bytes_per_sample (info->ctx->sample_fmt) * 8;
    _info->fmt.is_float = (info->ctx->sample_fmt == AV_SAMPLE_FMT_FLT || info->ctx->sample_fmt == AV_SAMPLE_FMT_FLTP);

    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;

    if (info->endsample >= 0 && info->currentsample + size / samplesize > info->endsample) {
        if ((info->endsample - info->currentsample + 1) * samplesize <= 0) {
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
            int out_size = info->buffer_size;
            int len;
            //trace ("in: out_size=%d(%d), size=%d\n", out_size, AVCODEC_MAX_AUDIO_FRAME_SIZE, size);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 40, 0)
            int got_frame = 0;
            len = avcodec_decode_audio4(info->ctx, info->frame, &got_frame, &info->pkt);
            if (len > 0) {
                if (ensure_buffer (info, info->frame->nb_samples * (_info->fmt.bps >> 3))) {
                    return -1;
                }
                if (av_sample_fmt_is_planar(info->ctx->sample_fmt)) {
                    out_size = 0;
                    for (int c = 0; c < info->ctx->channels; c++) {
                        for (int i = 0; i < info->frame->nb_samples; i++) {
                            if (_info->fmt.bps == 8) {
                                info->buffer[i*info->ctx->channels+c] = ((int8_t *)info->frame->extended_data[c])[i];
                                out_size++;
                            }
                            else if (_info->fmt.bps == 16) {
                                int16_t outsample = ((int16_t *)info->frame->extended_data[c])[i];
                                ((int16_t*)info->buffer)[i*info->ctx->channels+c] = outsample;
                                out_size += 2;
                            }
                            else if (_info->fmt.bps == 24) {
                                memcpy (&info->buffer[(i*info->ctx->channels+c)*3], &((int8_t*)info->frame->extended_data[c])[i*3], 3);
                                out_size += 3;
                            }
                            else if (_info->fmt.bps == 32) {
                                int32_t sample = ((int32_t *)info->frame->extended_data[c])[i];
                                ((int32_t*)info->buffer)[i*info->ctx->channels+c] = sample;
                                out_size += 4;
                            }
                        }
                    }
                }
                else {
                    out_size = info->frame->nb_samples * (_info->fmt.bps >> 3) * _info->fmt.channels;
                    memcpy (info->buffer, info->frame->extended_data[0], out_size);
                }
            }

#else
            if (ensure_buffer (info, 16384)) { // FIXME: how to get the packet size in old ffmpeg?
                return -1;
            }
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,25,0)
            len = avcodec_decode_audio3 (info->ctx, (int16_t *)info->buffer, &out_size, &info->pkt);
#else
            len = avcodec_decode_audio2 (info->ctx, (int16_t *)info->buffer, &out_size, info->pkt.data, info->pkt.size);
#endif
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
            av_packet_unref (&info->pkt);
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
                av_packet_unref (&info->pkt);
                continue;
            }
            //trace ("got packet: size=%d\n", info->pkt.size);
            info->have_packet = 1;
            info->left_in_packet = info->pkt.size;

            if (info->pkt.duration > 0) {
                AVRational *time_base = &info->fctx->streams[info->stream_id]->time_base;
                float sec = (float)info->pkt.duration * time_base->num / time_base->den;
                int bitrate = info->pkt.size * 8 / sec;
                if (bitrate > 0) {
                    deadbeef->streamer_set_bitrate (bitrate / 1000);
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
        av_packet_unref (&info->pkt);
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
    return ffmpeg_seek_sample (_info, time * _info->fmt.samplerate);
}

static const char *map[] = {
    "artist", "artist",
    "title", "title",
    "album", "album",
    "track", "track",
    "tracktotal", "numtracks",
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
    "disctotal", "numdiscs",
    "copyright", "copyright",
    "publisher", "publisher",
    "originaldate","original_release_time",
    "originalyear","original_release_year",
    "WM/OriginalReleaseTime","original_release_time",
    "WM/OriginalReleaseYear","original_release_year",
    NULL
};

static int
ff_add_disc_meta (DB_playItem_t *it, const char *disc) {
    char *slash = strchr (disc, '/');
    if (slash) {
        // split into disc/number
        *slash = 0;
        slash++;
        deadbeef->pl_add_meta (it, "numdiscs", slash);
    }
    deadbeef->pl_add_meta (it, "disc", disc);
    return 0;
}

static int
ff_add_track_meta (DB_playItem_t *it, const char *track) {
    char *slash = strchr (track, '/');
    if (slash) {
        // split into track/number
        *slash = 0;
        slash++;
        deadbeef->pl_add_meta (it, "numtracks", slash);
    }
    deadbeef->pl_add_meta (it, "track", track);
    return 0;
}

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
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54,23,0)
    AVMetadata *md = fctx->metadata;

    for (int m = 0; map[m]; m += 2) {
        AVMetadataTag *tag = NULL;
        do {
            tag = av_metadata_get (md, map[m], tag, AV_METADATA_DONT_STRDUP_KEY | AV_METADATA_DONT_STRDUP_VAL);
            if (tag) {
                if (!strcmp (map[m+1], "disc")) {
                    ff_add_disc_meta (it, tag->value);
                }
                else if (!strcmp (map[m+1], "track")) {
                    ff_add_track_meta (it, tag->value);
                }
                else {
                    deadbeef->pl_append_meta (it, map[m+1], tag->value);
                }
            }

        } while (tag);
    }
#else
    // ffmpeg-0.11 new metadata format
    AVDictionaryEntry *t = NULL;
    int m;
    for (int i = 0; i < fctx->nb_streams + 1; i++) {
        AVDictionary *md = i == 0 ? fctx->metadata : fctx->streams[i-1]->metadata;
        if (!md) {
            continue;
        }
        while ((t = av_dict_get (md, "", t, AV_DICT_IGNORE_SUFFIX))) {
            if (!strcasecmp (t->key, "replaygain_album_gain")) {
                deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, atof (t->value));
                continue;
            }
            else if (!strcasecmp (t->key, "replaygain_album_peak")) {
                deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, atof (t->value));
                continue;
            }
            else if (!strcasecmp (t->key, "replaygain_track_gain")) {
                deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, atof (t->value));
                continue;
            }
            else if (!strcasecmp (t->key, "replaygain_track_peak")) {
                deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, atof (t->value));
                continue;
            }

            for (m = 0; map[m]; m += 2) {
                if (!strcasecmp (t->key, map[m])) {
                    if (!strcmp (map[m+1], "disc")) {
                        ff_add_disc_meta (it, t->value);
                    }
                    else if (!strcmp (map[m+1], "track")) {
                        ff_add_track_meta (it, t->value);
                    }
                    else {
                        deadbeef->pl_append_meta (it, map[m+1], t->value);
                    }
                    break;
                }
            }
            if (!map[m]) {
                deadbeef->pl_append_meta (it, t->key, t->value);
            }
        }
    }
#endif
#endif
    return 0;
}

static void
print_error(const char *filename, int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));
    fprintf (stderr, "%s: %s\n", filename, errbuf_ptr);
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
    char *uri = NULL;
    int i;

    // construct uri
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(54, 6, 0)
    uri = strdupa (fname);
#else
    uri = alloca (l + sizeof (FF_PROTOCOL_NAME) + 1);
    memcpy (uri, FF_PROTOCOL_NAME, sizeof (FF_PROTOCOL_NAME)-1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME)-1, ":", 1);
    memcpy (uri + sizeof (FF_PROTOCOL_NAME), fname, l);
    uri[sizeof (FF_PROTOCOL_NAME) + l] = 0;
#endif
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(54, 6, 0)
    fctx = avformat_alloc_context ();
    fctx->max_analyze_duration = AV_TIME_BASE;
    if ((ret = avformat_open_input(&fctx, uri, NULL, NULL)) < 0) {
#else
    if ((ret = av_open_input_file(&fctx, uri, NULL, 0, NULL)) < 0) {
#endif
        print_error (uri, ret);
        return NULL;
    }

    trace ("fctx is %p, ret %d/%s\n", fctx, ret, strerror(-ret));
    ret = avformat_find_stream_info(fctx, NULL);
    if (ret < 0) {
        trace ("avformat_find_stream_info ret: %d/%s\n", ret, strerror(-ret));
    }
    trace ("nb_streams=%x\n", nb_streams);
    for (i = 0; i < fctx->nb_streams; i++)
    {
        if (!fctx->streams[i]) {
            continue;
        }
        ctx = fctx->streams[i]->codec;
        if (ctx->codec_type ==
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
            AVMEDIA_TYPE_AUDIO)
#else
            CODEC_TYPE_AUDIO)
#endif
        {
            codec = avcodec_find_decoder(ctx->codec_id);
            if (codec != NULL) {
                break;
            }
        }
    }
//    AVStream *stream = fctx->streams[i];

    if (codec == NULL)
    {
        trace ("ffmpeg can't decode %s\n", fname);
        avformat_free_context(fctx);
        return NULL;
    }
    trace ("ffmpeg can decode %s\n", fname);
    trace ("ffmpeg: codec=%s, stream=%d\n", codec->name, i);

    if (avcodec_open2 (ctx, codec, NULL) < 0) {
        trace ("ffmpeg: avcodec_open2 failed\n");
        avformat_free_context(fctx);
        return NULL;
    }

    int bps = av_get_bytes_per_sample (ctx->sample_fmt) * 8;
    int samplerate = ctx->sample_rate;
    float duration = fctx->duration / (float)AV_TIME_BASE;
//    float duration = stream->duration * stream->time_base.num / (float)stream->time_base.den;
    trace ("ffmpeg: bits per sample is %d\n", bps);
    trace ("ffmpeg: samplerate is %d\n", samplerate);
    trace ("ffmpeg: duration is %f\n", duration);

    if (bps <= 0 || ctx->channels <= 0 || samplerate <= 0) {
        return NULL;
    }

    int64_t totalsamples = fctx->duration * samplerate / AV_TIME_BASE;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    deadbeef->pl_replace_meta (it, ":FILETYPE", codec->name);

    if (!deadbeef->is_local_file (fname)) {
        deadbeef->plt_set_item_duration (plt, it, -1);
    }
    else {
        deadbeef->plt_set_item_duration (plt, it, duration);
    }

    // add metainfo
    ffmpeg_read_metadata_internal (it, fctx);
    
    int64_t fsize = -1;

    DB_FILE *fp = deadbeef->fopen (fname);
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
        snprintf (s, sizeof (s), "%d", bps);
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
    avformat_free_context(fctx);

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it, totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        return cue;
    }

    // now the track is ready, insert into playlist
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

static int
assign_new_ext (int n, const char* new_ext, size_t size) {
    char* ext = malloc (size + 1);
    strncpy (ext, new_ext, size);
    for (int i = 0; i < n; i++) {
        if (strcmp (exts[i], ext) == 0) {
            free(ext);
            return n;
        }
    }
    ext[size] = '\0';
    free (exts[n]);
    exts[n] = ext;
    return n + 1;
}

static int
add_new_exts (int n, const char* new_exts, char delim) {
    while (*new_exts) {
        if (n >= EXT_MAX) {
            fprintf (stderr, "ffmpeg: too many extensions, max is %d\n", EXT_MAX);
            break;
        }
        const char *e = new_exts;
        while (*e && (*e != delim || *e == ' ')) {
            e++;
        }
        if (e != new_exts) {
            n = assign_new_ext (n, new_exts, e-new_exts);
        }
        if (*e == 0) {
            break;
        }
        new_exts = e+1;
    }
    return n;
}

static void
ffmpeg_init_exts (void) {
    deadbeef->conf_lock ();
    const char *new_exts = deadbeef->conf_get_str_fast ("ffmpeg.extensions", DEFAULT_EXTS);
    int use_all_ext = deadbeef->conf_get_int ("ffmpeg.enable_all_exts", 0);
    for (int i = 0; exts[i]; i++) {
        free (exts[i]);
        exts[i] = NULL;
    }
    exts[0] = NULL;

    int n = 0;
    if (!use_all_ext) {
        n = add_new_exts (n, new_exts, ';');
    }
	else {
        AVInputFormat *ifmt  = NULL;
        /*
          * It's quite complicated to enumerate all supported extensions in
         * ffmpeg. If a decoder defines extensions in ffmpeg, the probing
         * mechanisim is disabled (see comments in avformat.h).
         * Thus some decoders doesn't claim its extensions (e.g. WavPack) 
         *
         * To get these missing extensions, we need to search corresponding
         * encoders for the same format, which will provide extensions for
         * encoding purpose, because ffmpeg will guess the output format from
         * the file name specified by users.
         */
        while ((ifmt = av_iformat_next(ifmt))) {
#ifdef AV_IS_INPUT_DEVICE
            if (ifmt->priv_class && AV_IS_INPUT_DEVICE(ifmt->priv_class->category))
                continue; // Skip all input devices
#endif

            if (ifmt->flags & AVFMT_NOFILE)
                continue; // Skip format that's not even a file

#ifdef AV_CODEC_ID_FIRST_AUDIO
            if (ifmt->raw_codec_id > 0 &&
                    (ifmt->raw_codec_id < AV_CODEC_ID_FIRST_AUDIO || ifmt->raw_codec_id > AV_CODEC_ID_FIRST_SUBTITLE)
               )
                continue; // Skip all non-audio raw formats
#endif
            if (ifmt->long_name && strstr(ifmt->long_name, "subtitle"))
                continue; // Skip all subtitle formats
            if (ifmt->extensions)
                n = add_new_exts (n, ifmt->extensions, ',');
        }
        /*
          * The above code doesn't guarntee all extensions are
         * included, however. In the portable build the encoders are disabled,
         * thus some extensions cannot be retrived.
         *
         * To fix this, we need to add some known extensions in addition to
         * scanned extensions.
         */
        n = add_new_exts (n, UNPOPULATED_EXTS_BY_FFMPEG, ',');
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
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(54, 6, 0)
    avcodec_register_all ();
#endif
    av_register_all ();
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
    char *uri = NULL;
    int i;

    deadbeef->pl_lock ();
    {
        const char *fname = deadbeef->pl_find_meta (it, ":URI");
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(54, 6, 0)
        uri = strdupa (fname);
#else
        int l = strlen (fname);
        uri = alloca (l + sizeof (FF_PROTOCOL_NAME) + 1);

        // construct uri
        memcpy (uri, FF_PROTOCOL_NAME, sizeof (FF_PROTOCOL_NAME)-1);
        memcpy (uri + sizeof (FF_PROTOCOL_NAME)-1, ":", 1);
        memcpy (uri + sizeof (FF_PROTOCOL_NAME), fname, l);
        uri[sizeof (FF_PROTOCOL_NAME) + l] = 0;
#endif
    }
    deadbeef->pl_unlock ();
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
    if ((ret = avformat_open_input(&fctx, uri, NULL, NULL)) < 0) {
        trace ("fctx is %p, ret %d/%s", fctx, ret, strerror(-ret));
        return -1;
    }

    avformat_find_stream_info(fctx, NULL);
    for (i = 0; i < fctx->nb_streams; i++)
    {
        ctx = fctx->streams[i]->codec;
        if (ctx->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            codec = avcodec_find_decoder(ctx->codec_id);
            if (codec != NULL)
                break;
        }
    }
    if (codec == NULL)
    {
        trace ("ffmpeg can't decode %s\n", deadbeef->pl_find_meta (it, ":URI"));
        avformat_free_context(fctx);
        return -1;
    }
    if (avcodec_open2 (ctx, codec, NULL) < 0) {
        trace ("ffmpeg: avcodec_open2 failed\n");
        avformat_free_context(fctx);
        return -1;
    }

    deadbeef->pl_delete_all_meta (it);
    ffmpeg_read_metadata_internal (it, fctx);

    avformat_free_context(fctx);

    return 0;
}

static const char settings_dlg[] =
    "property \"Use all extensions supported by ffmpeg\" checkbox ffmpeg.enable_all_exts 0;\n"
    "property \"File Extensions (separate with ';')\" entry ffmpeg.extensions \"" DEFAULT_EXTS "\";\n"
;

// define plugin interface
static DB_decoder_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 2,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "ffmpeg",
    .plugin.name = "FFMPEG audio player",
    .plugin.descr = "decodes audio formats using FFMPEG libavcodec",
    .plugin.copyright = 
        "Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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

