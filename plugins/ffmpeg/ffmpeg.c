/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

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

#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/avstring.h>


#define avcodec_free_frame(frame) av_frame_free(frame)

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 8, 0)
#define av_packet_unref(packet) av_free_packet(packet)
#endif

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static ddb_decoder2_t plugin;
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

typedef struct {
    DB_fileinfo_t info;
    AVCodec *codec;
    AVCodecContext *codec_context;
    int need_to_free_codec_context;
    AVFormatContext *format_context;
    AVPacket pkt;
    AVFrame *frame;
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
    ffmpeg_info_t *info = calloc (1, sizeof (ffmpeg_info_t));
    return &info->info;
}

// ensure that the buffer can contain entire frame of frame_size bytes per channel
static int
ensure_buffer (ffmpeg_info_t *info, int frame_size) {
    if (!info->buffer || info->buffer_size < frame_size * info->codec_context->channels) {
        if (info->buffer) {
            free (info->buffer);
            info->buffer = NULL;
        }
        info->buffer_size = frame_size*info->codec_context->channels;
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
_get_audio_codec_from_stream(AVFormatContext *format_context, int stream_index, ffmpeg_info_t *info) {
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 33, 0)
    if (format_context->streams[stream_index]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
        return 0;
    }
    AVCodec *codec = avcodec_find_decoder(format_context->streams[stream_index]->codecpar->codec_id);
    if (codec == NULL) {
        return 0;
    }
    info->codec = codec;
    info->stream_id = stream_index;
    info->codec_context = avcodec_alloc_context3 (info->codec);
    info->need_to_free_codec_context = 1;
    avcodec_parameters_to_context(info->codec_context, format_context->streams[stream_index]->codecpar);
#else
    if (format_context->streams[stream_index]->codec->codec_type != AVMEDIA_TYPE_AUDIO) {
        return 0;
    }
    AVCodecContext *ctx = format_context->streams[stream_index]->codec;
    if (ctx == NULL) {
        return 0;
    }
    AVCodec *codec = avcodec_find_decoder (ctx->codec_id);
    if (codec == NULL) {
        return 0;
    }
    info->codec_context = ctx;
    info->codec = codec;
    info->stream_id = stream_index;
#endif
    return 1;
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

    info->format_context = avformat_alloc_context ();

    if ((ret = avformat_open_input(&info->format_context, uri, NULL, NULL)) < 0) {
        trace ("\033[0;31minfo->fctx is %p, ret %d/%s\033[0;31m\n", info->fctx, ret, strerror(-ret));
        return -1;
    }
    trace ("\033[0;31mav_open_input_file done, ret=%d\033[0;31m\n", ret);

    trace ("\033[0;31mffmpeg avformat_find_stream_info\033[37;0m\n");
    info->stream_id = -1;
    info->format_context->max_analyze_duration = AV_TIME_BASE;
    avformat_find_stream_info(info->format_context, NULL);
    AVFormatContext *fctx = info->format_context;
    for (i = 0; i < info->format_context->nb_streams; i++) {
        if (_get_audio_codec_from_stream (fctx, i, info)) {
            break;
        }
    }

    if (info->codec == NULL)
    {
        trace ("ffmpeg can't decode %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }
    trace ("ffmpeg can decode %s\n", deadbeef->pl_find_meta (it, ":URI"));
    trace ("ffmpeg: codec=%s, stream=%d\n", info->codec->name, i);

    if (avcodec_open2 (info->codec_context, info->codec, NULL) < 0) {
        trace ("ffmpeg: avcodec_open2 failed\n");
        return -1;
    }

    deadbeef->pl_replace_meta (it, "!FILETYPE", info->codec->name);

    int bps = av_get_bytes_per_sample (info->codec_context->sample_fmt)*8;
    int samplerate = info->codec_context->sample_rate;

    if (bps <= 0 || info->codec_context->channels <= 0 || samplerate <= 0) {
        return -1;
    }

    int64_t totalsamples = info->format_context->duration * samplerate / AV_TIME_BASE;
    info->left_in_packet = 0;
    info->left_in_buffer = 0;

    memset (&info->pkt, 0, sizeof (info->pkt));
    info->have_packet = 0;

    info->frame = av_frame_alloc();

    // fill in mandatory plugin fields
    _info->plugin = &plugin.decoder;
    _info->readpos = 0;
    _info->fmt.bps = bps;
    _info->fmt.channels = info->codec_context->channels;
    _info->fmt.samplerate = samplerate;
    if (info->codec_context->sample_fmt == AV_SAMPLE_FMT_FLT || info->codec_context->sample_fmt == AV_SAMPLE_FMT_FLTP) {
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
        plugin.seek_sample64 (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = totalsamples - 1;
    }
    return 0;
}

static void
_free_info_data(ffmpeg_info_t *info) {
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
    if (info->codec_context) {
        avcodec_close (info->codec_context);

        // The ctx is owned by AVFormatContext in legacy mode
        if (info->need_to_free_codec_context) {
            avcodec_free_context (&info->codec_context);
        }
    }
    if (info->format_context) {
        avformat_close_input (&info->format_context);
    }
}

static void
ffmpeg_free (DB_fileinfo_t *_info) {
    trace ("ffmpeg: free\n");
    ffmpeg_info_t *info = (ffmpeg_info_t*)_info;
    if (info) {
        _free_info_data(info);
        free (info);
    }
}

static int
ffmpeg_read (DB_fileinfo_t *_info, char *bytes, int size) {
    trace ("ffmpeg_read_int16 %d\n", size);
    ffmpeg_info_t *info = (ffmpeg_info_t*)_info;

    _info->fmt.channels = info->codec_context->channels;
    _info->fmt.samplerate = info->codec_context->sample_rate;
    _info->fmt.bps = av_get_bytes_per_sample (info->codec_context->sample_fmt) * 8;
    _info->fmt.is_float = (info->codec_context->sample_fmt == AV_SAMPLE_FMT_FLT || info->codec_context->sample_fmt == AV_SAMPLE_FMT_FLTP);

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
            int len = 0;
            //trace ("in: out_size=%d(%d), size=%d\n", out_size, AVCODEC_MAX_AUDIO_FRAME_SIZE, size);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 28, 0)
            int ret = avcodec_send_packet (info->codec_context, &info->pkt);
            if (ret < 0) {
                break;
            }
            ret = avcodec_receive_frame (info->codec_context,info->frame);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                break;
            }
            else {
                len = info->pkt.size;
            }
#else
            int got_frame = 0;
            len = avcodec_decode_audio4(info->ctx, info->frame, &got_frame, &info->pkt);
#endif

            if (len > 0) {
                if (ensure_buffer (info, info->frame->nb_samples * (_info->fmt.bps >> 3))) {
                    return -1;
                }
                if (av_sample_fmt_is_planar(info->codec_context->sample_fmt)) {
                    out_size = 0;
                    for (int c = 0; c < info->codec_context->channels; c++) {
                        for (int i = 0; i < info->frame->nb_samples; i++) {
                            if (_info->fmt.bps == 8) {
                                info->buffer[i*info->codec_context->channels+c] = ((int8_t *)info->frame->extended_data[c])[i];
                                out_size++;
                            }
                            else if (_info->fmt.bps == 16) {
                                int16_t outsample = ((int16_t *)info->frame->extended_data[c])[i];
                                ((int16_t*)info->buffer)[i*info->codec_context->channels+c] = outsample;
                                out_size += 2;
                            }
                            else if (_info->fmt.bps == 24) {
                                memcpy (&info->buffer[(i*info->codec_context->channels+c)*3], &((int8_t*)info->frame->extended_data[c])[i*3], 3);
                                out_size += 3;
                            }
                            else if (_info->fmt.bps == 32) {
                                int32_t sample = ((int32_t *)info->frame->extended_data[c])[i];
                                ((int32_t*)info->buffer)[i*info->codec_context->channels+c] = sample;
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
            if ((ret = av_read_frame (info->format_context, &info->pkt)) < 0) {
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
                AVRational *time_base = &info->format_context->streams[info->stream_id]->time_base;
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
ffmpeg_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
    ffmpeg_info_t *info = (ffmpeg_info_t*)_info;
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
    if (info->have_packet) {
        av_packet_unref (&info->pkt);
        info->have_packet = 0;
    }
    sample += info->startsample;
    int64_t tm = sample / _info->fmt.samplerate * AV_TIME_BASE;
    trace ("ffmpeg: seek to sample: %d, t: %d\n", sample, (int)tm);
    info->left_in_packet = 0;
    info->left_in_buffer = 0;
    if (av_seek_frame (info->format_context, -1, tm, AVSEEK_FLAG_ANY) < 0) {
        trace ("ffmpeg: seek error\n");
        return -1;
    }
    
    // update readpos
    info->currentsample = sample;
    _info->readpos = (float)(sample - info->startsample) / _info->fmt.samplerate;
    return 0;
}

static int
ffmpeg_seek_sample (DB_fileinfo_t *_info, int sample) {
    return ffmpeg_seek_sample64(_info, sample);
}

static int
ffmpeg_seek (DB_fileinfo_t *_info, float time) {
    return ffmpeg_seek_sample64 (_info, (int64_t)((double)time * (int64_t)_info->fmt.samplerate));
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

    ffmpeg_info_t info = {0};

    int ret;
    char *uri = NULL;
    int i;

    // construct uri
    uri = strdupa (fname);
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
    info.format_context = avformat_alloc_context ();
    info.format_context->max_analyze_duration = AV_TIME_BASE;
    if ((ret = avformat_open_input(&info.format_context, uri, NULL, NULL)) < 0) {
        print_error (uri, ret);
        goto error;
    }

    trace ("fctx is %p, ret %d/%s\n", info.format_context, ret, strerror(-ret));
    ret = avformat_find_stream_info(info.format_context, NULL);
    if (ret < 0) {
        trace ("avformat_find_stream_info ret: %d/%s\n", ret, strerror(-ret));
    }
    trace ("nb_streams=%x\n", nb_streams);

    for (i = 0; i < info.format_context->nb_streams; i++)
    {
        if (!info.format_context->streams[i]) {
            continue;
        }
        if (_get_audio_codec_from_stream(info.format_context, i, &info)) {
            break;
        }
    }

    if (info.codec == NULL)
    {
        trace ("ffmpeg can't decode %s\n", fname);
        goto error;
    }
    trace ("ffmpeg can decode %s\n", fname);
    trace ("ffmpeg: codec=%s, stream=%d\n", codec->name, i);

    int avcodec_open2_ret = avcodec_open2 (info.codec_context, info.codec, NULL);
    if (avcodec_open2_ret < 0) {
        trace ("ffmpeg: avcodec_open2 failed\n");
        goto error;
    }

    int bps = av_get_bytes_per_sample (info.codec_context->sample_fmt) * 8;
    int samplerate = info.codec_context->sample_rate;
    float duration = info.format_context->duration / (float)AV_TIME_BASE;
//    float duration = stream->duration * stream->time_base.num / (float)stream->time_base.den;
    trace ("ffmpeg: bits per sample is %d\n", bps);
    trace ("ffmpeg: samplerate is %d\n", samplerate);
    trace ("ffmpeg: duration is %f\n", duration);

    if (bps <= 0 || info.codec_context->channels <= 0 || samplerate <= 0) {
        goto error;
    }

    int64_t totalsamples = info.format_context->duration * samplerate / AV_TIME_BASE;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);
    deadbeef->pl_replace_meta (it, ":FILETYPE", info.codec->name);

    if (!deadbeef->is_local_file (fname)) {
        deadbeef->plt_set_item_duration (plt, it, -1);
    }
    else {
        deadbeef->plt_set_item_duration (plt, it, duration);
    }

    // add metainfo
    ffmpeg_read_metadata_internal (it, info.format_context);
    
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
        snprintf (s, sizeof (s), "%" PRId64, fsize);
        deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
        snprintf (s, sizeof (s), "%d", bps);
        deadbeef->pl_add_meta (it, ":BPS", s);
        snprintf (s, sizeof (s), "%d", info.codec_context->channels);
        deadbeef->pl_add_meta (it, ":CHANNELS", s);
        snprintf (s, sizeof (s), "%d", samplerate);
        deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
        int br = (int)roundf(fsize / duration * 8 / 1000);
        snprintf (s, sizeof (s), "%d", br);
        deadbeef->pl_add_meta (it, ":BITRATE", s);
    }

    // free decoder
    _free_info_data (&info);

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it, totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        return cue;
    }

    // now the track is ready, insert into playlist
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
error:
    _free_info_data (&info);
    return NULL;
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
#if LIBAVFORMAT_VERSION_MAJOR >= 59
        void *iter = NULL;
        while ((ifmt = av_demuxer_iterate(&iter))) {
#else
        while ((ifmt = av_iformat_next(ifmt))) {
#endif
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
#if LIBAVFORMAT_VERSION_MAJOR < 58
    av_register_all ();
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
    int err = -1;

    trace ("ffmpeg_read_metadata: fname %s\n", deadbeef->pl_find_meta (it, ":URI"));
    ffmpeg_info_t info = {0};
    int ret;
    char *uri = NULL;
    int i;

    deadbeef->pl_lock ();
    const char *fname = deadbeef->pl_find_meta (it, ":URI");
    uri = strdupa (fname);
    deadbeef->pl_unlock ();
    trace ("ffmpeg: uri: %s\n", uri);

    // open file
    if ((ret = avformat_open_input(&info.format_context, uri, NULL, NULL)) < 0) {
        trace ("fctx is %p, ret %d/%s", info.format_context, ret, strerror(-ret));
        return -1;
    }

    avformat_find_stream_info(info.format_context, NULL);
    for (i = 0; i < info.format_context->nb_streams; i++) {
        if (_get_audio_codec_from_stream(info.format_context, i, &info)) {
            break;
        }
    }
    if (info.codec == NULL) {
        trace ("ffmpeg can't decode %s\n", deadbeef->pl_find_meta (it, ":URI"));
        goto error;
    }
    if (avcodec_open2 (info.codec_context, info.codec, NULL) < 0) {
        trace ("ffmpeg: avcodec_open2 failed\n");
        goto error;
    }

    deadbeef->pl_delete_all_meta (it);
    ffmpeg_read_metadata_internal (it, info.format_context);

    err = 0;
error:
    _free_info_data(&info);

    return err;
}

static const char settings_dlg[] =
    "property \"Use all extensions supported by ffmpeg\" checkbox ffmpeg.enable_all_exts 0;\n"
    "property \"File Extensions (separate with ';')\" entry ffmpeg.extensions \"" DEFAULT_EXTS "\";\n"
;

// define plugin interface

static ddb_decoder2_t plugin = {
    .decoder.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .decoder.plugin.api_vminor = DB_API_VERSION_MINOR,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 2,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "ffmpeg",
    .decoder.plugin.name = "FFMPEG audio player",
    .decoder.plugin.descr = "decodes audio formats using FFMPEG libavcodec",
    .decoder.plugin.copyright =
        "Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
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
    .decoder.plugin.website = "http://deadbeef.sf.net",
    .decoder.plugin.start = ffmpeg_start,
    .decoder.plugin.stop = ffmpeg_stop,
    .decoder.plugin.configdialog = settings_dlg,
    .decoder.plugin.message = ffmpeg_message,
    .decoder.open = ffmpeg_open,
    .decoder.init = ffmpeg_init,
    .decoder.free = ffmpeg_free,
    .decoder.read = ffmpeg_read,
    .decoder.seek = ffmpeg_seek,
    .decoder.seek_sample = ffmpeg_seek_sample,
    .decoder.insert = ffmpeg_insert,
    .decoder.read_metadata = ffmpeg_read_metadata,
    .decoder.exts = (const char **)exts,
    .seek_sample64 = ffmpeg_seek_sample64,
};

DB_plugin_t *
ffmpeg_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

