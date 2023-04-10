/*
    MPEG decoder plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>
#include "mp3.h"
#ifdef USE_LIBMAD
#include "mp3_mad.h"
#endif
#ifdef USE_LIBMPG123
#include "mp3_mpg123.h"
#endif

#define trace(...) { deadbeef->log_detailed (&plugin.decoder.plugin, 0, __VA_ARGS__); }

//#define WRITE_DUMP 1

#if WRITE_DUMP
FILE *out;
#endif

#define MAX_INVALID_BYTES 100000

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

static ddb_decoder2_t plugin;
DB_functions_t *deadbeef;

static int
cmp3_seek_sample64 (DB_fileinfo_t *_info, int64_t sample);

int
cmp3_seek_stream (DB_fileinfo_t *_info, int64_t sample) {
    mp3_info_t *info = (mp3_info_t *)_info;

#if WRITE_DUMP
    if (out) {
        fclose (out);
        out = fopen ("out.raw", "w+b");
    }
#endif

    mp3info_t mp3info;
    int res = mp3_parse_file(&mp3info, info->mp3flags, info->file, deadbeef->fgetlength(info->file), info->startoffs, info->endoffs, sample);

    if (!res) {
        deadbeef->fseek (info->file, mp3info.packet_offs, SEEK_SET);
        info->currentsample = sample;
        if (sample > mp3info.pcmsample) {
            info->skipsamples = sample - mp3info.pcmsample;
        }
        else {
            info->skipsamples = 0;
        }
    }


    return res;
}


static DB_fileinfo_t *
cmp3_open (uint32_t hints) {
    mp3_info_t *info = calloc (1, sizeof (mp3_info_t));

    if (hints & DDB_DECODER_HINT_RAW_SIGNAL) {
        info->raw_signal = 1;
    }

#ifndef ANDROID // force 16 bit on android
    if ((hints & DDB_DECODER_HINT_16BIT) || deadbeef->conf_get_int ("mp3.force16bit", 0))
#endif
    {
        info->want_16bit = 1;
    }

    if (hints & (1UL<<31)) {
        info->mp3flags |= MP3_PARSE_ESTIMATE_DURATION;
    }
    return &info->info;
}

void
cmp3_set_extra_properties (DB_playItem_t *it, mp3info_t *mp3info, int fake) {
    char s[100];
    int64_t size = mp3info->fsize;
    if (size >= 0) {
        snprintf (s, sizeof (s), "%" PRId64 , size);
        deadbeef->pl_replace_meta (it, ":FILE_SIZE", s);
    }
    else {
        deadbeef->pl_replace_meta (it, ":FILE_SIZE", "∞");
    }


    if (mp3info->datasize >= 0 && mp3info->have_duration) {
        // bitrate from file size
        double dur = (double)deadbeef->pl_get_item_duration (it);
        int bitrate = mp3info->datasize * 8 / dur / 1000;
        snprintf (s, sizeof (s), "%d", bitrate);
        deadbeef->pl_replace_meta (it, ":BITRATE", s);
    }
    else if (mp3info->avg_bitrate > 0) {
        snprintf (s, sizeof (s), "%d", (int)(mp3info->avg_bitrate/1000));
        deadbeef->pl_replace_meta (it, ":BITRATE", s);
    }

    snprintf (s, sizeof (s), "%d", mp3info->ref_packet.nchannels);
    deadbeef->pl_replace_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", mp3info->ref_packet.samplerate);
    deadbeef->pl_replace_meta (it, ":SAMPLERATE", s);

    // set codec profile (cbr or vbr) and mp3 vbr method (guessed, or from Xing/Info header)

    char codec_profile[100];
    snprintf (codec_profile, sizeof (codec_profile), "MP3 %s", (!mp3info->vbr_type || mp3info->vbr_type == XING_CBR || mp3info->vbr_type == XING_CBR2) ?  "CBR" : "VBR");
    if (mp3info->vbr_type != XING_CBR && mp3info->vbr_type != XING_CBR2 && (mp3info->lamepreset & 0x7ff)) {
        const static struct {
            int v;
            const char *name;
        } presets[] = {
            { 8, "ABR_8" },
            { 320, "ABR_320" },
            { 410, "V9" },
            { 420, "V8" },
            { 430, "V7" },
            { 440, "V6" },
            { 450, "V5" },
            { 460, "V4" },
            { 470, "V3" },
            { 480, "V2" },
            { 490, "V1" },
            { 500, "V0" },
            { 1000, "R3MIX" },
            { 1001, "STANDARD" },
            { 1002, "EXTREME" },
            { 1003, "INSANE" },
            { 1004, "STANDARD_FAST" },
            { 1005, "EXTREME_FAST" },
            { 1006, "MEDIUM" },
            { 1007, "MEDIUM_FAST" },
            { 0, NULL },
        };

        for (int i = 0; presets[i].name; i++) {
            if (presets[i].v == (mp3info->lamepreset&0x7ff)) {
                size_t l = strlen (codec_profile);
                char *preset = codec_profile + l;
                snprintf (preset, sizeof (codec_profile) - l, " %s", presets[i].name);
                break;
            }
        }
    }

    deadbeef->pl_replace_meta (it, ":CODEC_PROFILE", codec_profile);

    switch (mp3info->vbr_type) {
    case XING_ABR:
        deadbeef->pl_replace_meta (it, ":MP3_VBR_METHOD", "ABR");
        break;
    case XING_VBR1:
        deadbeef->pl_replace_meta (it, ":MP3_VBR_METHOD", "full VBR method 1");
        break;
    case XING_VBR2:
        deadbeef->pl_replace_meta (it, ":MP3_VBR_METHOD", "full VBR method 2");
        break;
    case XING_VBR3:
        deadbeef->pl_replace_meta (it, ":MP3_VBR_METHOD", "full VBR method 3");
        break;
    case XING_VBR4:
        deadbeef->pl_replace_meta (it, ":MP3_VBR_METHOD", "full VBR method 4");
        break;
    case XING_ABR2:
        deadbeef->pl_replace_meta (it, ":MP3_VBR_METHOD", "ABR 2 pass");
        break;
    case DETECTED_VBR:
        deadbeef->pl_replace_meta (it, ":MP3_VBR_METHOD", "unspecified");
        break;
    }
    const char *versions[] = {"1", "2", "2.5"};
    snprintf (s, sizeof (s), "MPEG%s layer%d", versions[mp3info->ref_packet.ver-1], mp3info->ref_packet.layer);
    deadbeef->pl_replace_meta (it, ":MPEG_VERSION", s);
    deadbeef->pl_replace_meta (it, ":XING_HEADER", mp3info->have_xing_header ? "Yes" : "No");
    deadbeef->pl_replace_meta (it, fake ? "!FILETYPE" : ":FILETYPE", "MP3");
}

static int
_mp3_parse_and_validate (mp3info_t *info, uint32_t flags, DB_FILE *fp, int64_t fsize, int startoffs, int endoffs, int64_t seek_to_sample) {
    int res = mp3_parse_file(info, flags, fp, fsize, startoffs, endoffs, seek_to_sample);
    if (res < 0) {
        return res;
    }

    if (info->valid_packets == 0) {
        return -1;
    }

    if (info->ref_packet.samplerate == 0 || info->ref_packet.nchannels == 0) {
        return -1;
    }

    return res;
}

static int
cmp3_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    mp3_info_t *info = (mp3_info_t *)_info;

#if defined(USE_LIBMAD) && defined(USE_LIBMPG123)
    int backend = deadbeef->conf_get_int ("mp3.backend", 0);
    switch (backend) {
    case 0:
        info->dec = &mpg123_api;
        break;
    case 1:
        info->dec = &mad_api;
        break;
    default:
        info->dec = &mpg123_api;
        break;
    }
#else
#if defined(USE_LIBMAD)
    info->dec = &mad_api;
#else
    info->dec = &mpg123_api;
#endif
#endif

    _info->plugin = &plugin.decoder;
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    info->file = deadbeef->fopen (uri);
    if (!info->file) {
        return -1;
    }
    deadbeef->fset_track (info->file, it);
    info->info.file = info->file;
    deadbeef->pl_item_ref (it);
    info->it = it;
    info->info.readpos = 0;
    if (!info->file->vfs->is_streaming () && !(info->mp3flags & MP3_PARSE_ESTIMATE_DURATION)) {
        deadbeef->junk_get_tag_offsets (info->file, &info->startoffs, &info->endoffs);
        if (info->startoffs > 0) {
            trace ("mp3: skipping %d(%xH) bytes of junk\n", info->startoffs, info->endoffs);
        }
        int res = _mp3_parse_and_validate(&info->mp3info, info->mp3flags, info->file, deadbeef->fgetlength(info->file), info->startoffs, info->endoffs, -1);
        if (res < 0) {
            trace ("mp3: cmp3_init: initial mp3_parse_file failed\n");
            return -1;
        }
        info->currentsample = info->mp3info.pcmsample;

        int64_t endsample = deadbeef->pl_item_get_endsample (it);
        if (endsample > 0) {
            info->startsample = deadbeef->pl_item_get_startsample (it) + info->mp3info.delay;
            info->endsample = endsample + info->mp3info.delay;
            // that comes from cue, don't calc duration, just seek and play
        }
        else {
            ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
            int64_t totalsamples = info->mp3info.totalsamples - info->mp3info.delay - info->mp3info.padding;
            deadbeef->plt_set_item_duration (plt, it, (float)((double)totalsamples/info->mp3info.ref_packet.samplerate));
            if (plt) {
                deadbeef->plt_unref (plt);
            }
            info->startsample = info->mp3info.delay;
            info->endsample = info->mp3info.totalsamples-info->mp3info.padding-1;
            deadbeef->fseek (info->file, info->mp3info.packet_offs, SEEK_SET);
        }
    }
    else {
        info->startoffs = (uint32_t)deadbeef->junk_get_leading_size(info->file);
        deadbeef->pl_add_meta (it, "title", NULL);
        int res = _mp3_parse_and_validate(&info->mp3info, info->mp3flags, info->file, deadbeef->fgetlength(info->file), info->startoffs, 0, -1);
        if (res < 0) {
            trace ("mp3: cmp3_init: initial mp3_parse_file failed\n");
            return -1;
        }

        ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
        info->startsample = info->mp3info.delay;
        if (info->mp3info.totalsamples >= 0) {
            deadbeef->plt_set_item_duration (plt, it, (float)((double)info->mp3info.totalsamples/info->mp3info.ref_packet.samplerate));
            info->endsample = info->mp3info.totalsamples-info->mp3info.padding-1;
        }
        else {
            deadbeef->plt_set_item_duration (plt, it, -1);
            info->endsample = -1;
        }

        cmp3_set_extra_properties (it, &info->mp3info, 1);

        if (plt) {
            deadbeef->plt_unref (plt);
        }
        info->skipsamples = 0;
        info->currentsample = info->mp3info.pcmsample;
    }
    if (info->want_16bit && !info->raw_signal) {
        _info->fmt.bps = 16;
        _info->fmt.is_float = 0;
    }
    else {
        _info->fmt.bps = 32;
        _info->fmt.is_float = 1;
    }
    _info->fmt.samplerate = info->mp3info.ref_packet.samplerate;
    _info->fmt.channels = info->mp3info.ref_packet.nchannels;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    trace ("mp3 format: bps:%d sr:%d channels:%d\n", _info->fmt.bps, _info->fmt.samplerate, _info->fmt.channels);

    if (info->want_16bit) {
        deadbeef->pl_replace_meta (it, ":BPS", "16");
    }
    else {
        deadbeef->pl_replace_meta (it, ":BPS", "32");
    }

    info->dec->init (info);
    cmp3_seek_sample64 (_info, 0);
    return 0;
}

#if 0
static void
dump_buffer (buffer_t *buffer) {
    printf ("*** DUMP ***\n");
    printf ("remaining: %d\n", buffer->remaining);

    printf ("readsize: %d\n", buffer->readsize);
    printf ("decode_remaining: %d\n", buffer->decode_remaining);

    // information, filled by mp3_parse_file
    printf ("%d\n", buffer->version);
    printf ("%d\n", buffer->layer);
    printf ("%d\n", buffer->bitrate);
    printf ("%d\n", buffer->samplerate);
    printf ("%d\n", buffer->packetlength);
    printf ("%d\n", buffer->bitspersample);
    printf ("%d\n", buffer->channels);
    printf ("%f\n", buffer->duration);

    printf ("%d\n", buffer->currentsample);
    printf ("%d\n", buffer->totalsamples);
    printf ("%d\n", buffer->skipsamples);
    printf ("%d\n", buffer->startoffset);
    printf ("%d\n", buffer->endoffset);
    printf ("%d\n", buffer->startsample);
    printf ("%d\n", buffer->endsample);
    printf ("%d\n", buffer->delay);
    printf ("%d\n", buffer->padding);

    printf ("%f\n", buffer->avg_packetlength);
    printf ("%d\n", buffer->avg_samplerate);
    printf ("%d\n", buffer->avg_samples_per_frame);
    printf ("%d\n", buffer->nframes);
    printf ("%d\n", buffer->last_comment_update);
    printf ("%d\n", buffer->vbr);
    printf ("%d\n", buffer->have_xing_header);
    printf ("%d\n", buffer->current_decode_frame);
    printf ("%lld\n", buffer->lastframe_filepos);

    printf ("*** END ***\n");
}
#endif

// stream, decode and copy enough samples to fill the output buffer
// `skipsamples` is accounted for
static void
cmp3_decode (mp3_info_t *info) {
    int eof = 0;
    while (!eof) {
        eof = info->dec->decode_next_packet (info);
        if (info->decoded_samples_remaining > 0) {
            if (info->skipsamples > 0) {
                int64_t skip = min (info->skipsamples, info->decoded_samples_remaining);
                info->skipsamples -= skip;
                info->decoded_samples_remaining -= skip;
            }
            if (info->skipsamples > 0) {
                continue;
            }
            info->dec->consume_decoded_data (info);

            assert (info->bytes_to_decode >= 0);
            if (info->bytes_to_decode == 0) {
                return;
            }
        }
    }
}

static void
cmp3_free (DB_fileinfo_t *_info) {
    mp3_info_t *info = (mp3_info_t *)_info;
    if (info->it) {
        deadbeef->pl_item_unref (info->it);
    }
    if (info->conv_buf) {
        free (info->conv_buf);
    }
    if (info->file) {
        deadbeef->fclose (info->file);
        info->file = NULL;
        info->info.file = NULL;
        info->dec->free (info);
    }
    free (info);
}

static int
cmp3_read (DB_fileinfo_t *_info, char *bytes, int size) {
#if WRITE_DUMP
    if (!out) {
        out = fopen ("out.raw", "w+b");
    }
#endif
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    mp3_info_t *info = (mp3_info_t *)_info;
    if (!info->file->vfs->is_streaming () && !(info->mp3flags&MP3_PARSE_ESTIMATE_DURATION)) {
        int64_t curr = info->currentsample;
        //printf ("curr: %d -> end %d, padding: %d\n", curr, info->endsample, info->padding);
        if (size / samplesize + curr > info->endsample) {
            size = (int)((info->endsample - curr + 1) * samplesize);
            trace ("\033[0;32mmp3: size truncated to %d bytes (%d samples), cursample=%d, endsample=%d\033[37;0m\n", size, info->endsample - curr + 1, curr, info->endsample);
            if (size <= 0) {
                return 0;
            }
        }
    }

    int initsize = size;

    int req_size;
    if (info->want_16bit && !info->raw_signal) {
        req_size = size * 2;
        // decode in 32 bit temp buffer, then convert to 16 below
        if (info->conv_buf_size < req_size) {
            info->conv_buf_size = req_size;
            if (info->conv_buf) {
                free (info->conv_buf);
            }
            info->conv_buf = malloc (info->conv_buf_size);
        }
        info->bytes_to_decode = req_size;
        info->out = info->conv_buf;
    }
    else {
        req_size = size;
        // decode straight to 32 bit
        info->bytes_to_decode = size;
        info->out = bytes;
    }

    cmp3_decode (info);

    if (!info->raw_signal) {
        ddb_waveformat_t fmt;
        memcpy (&fmt, &info->info.fmt, sizeof (fmt));
        fmt.bps = 32;
        fmt.is_float = 1;

        // apply replaygain, before clipping
        deadbeef->replaygain_apply (&fmt, info->want_16bit ? info->conv_buf : bytes, req_size - info->bytes_to_decode);

        // convert to 16 bit, if needed
        if (info->want_16bit) {
            int sz = req_size - info->bytes_to_decode;
            int ret = deadbeef->pcm_convert (&fmt, info->conv_buf, &_info->fmt, bytes, sz);
            info->bytes_to_decode = size-ret;
        }
    }

    info->currentsample += (size - info->bytes_to_decode) / samplesize;
    _info->readpos = (float)(info->currentsample - info->startsample) / info->mp3info.ref_packet.samplerate;
#if WRITE_DUMP
    if (size - info->readsize > 0) {
        fwrite (bytes, 1, size - info->readsize, out);
    }
#endif
    return initsize - info->bytes_to_decode;
}

static int
cmp3_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
    mp3_info_t *info = (mp3_info_t *)_info;
    if (!info->file) {
        return -1;
    }

    sample += info->startsample;
    if (sample > info->endsample) {
        sample = info->endsample;
    }


// {{{ handle net streaming case
    if (info->file->vfs->is_streaming () || (info->mp3flags & MP3_PARSE_ESTIMATE_DURATION)) {
        if (info->mp3info.totalsamples > 0 && info->mp3info.avg_samples_per_frame > 0 && info->mp3info.avg_packetlength > 0) { // that means seekable remote stream, like podcast
            trace ("seeking is possible!\n");

            int r;

            // seek to beginning of the frame
            int64_t frm = sample / info->mp3info.avg_samples_per_frame;
            r = deadbeef->fseek (info->file, frm * info->mp3info.avg_packetlength + info->startoffs, SEEK_SET);

            if (r < 0) {
                trace ("seek failed!\n");
                return -1;
            }

            info->skipsamples = sample - frm * info->mp3info.avg_samples_per_frame;

            info->currentsample = sample;
            _info->readpos = (float)((double)(info->currentsample - info->startsample) / info->mp3info.ref_packet.samplerate);

            info->dec->free (info);
            info->decoded_samples_remaining = 0;
            info->dec->init (info);
            return 0;
        }
        trace ("seek is impossible (avg_samples_per_frame=%d, avg_packetlength=%f)!\n", info->mp3info.avg_samples_per_frame, info->mp3info.avg_packetlength);
        return 0;
    }
// }}}

    info->bytes_to_decode = 0;
    info->decoded_samples_remaining = 0;

    // force flush the decoder by reinitializing it
    info->dec->free (info);

//    struct timeval tm1;
//    gettimeofday (&tm1, NULL);
    if (cmp3_seek_stream (_info, sample) == -1) {
        trace ("failed to seek to sample %d\n", sample);
        _info->readpos = 0;
        return -1;
    }

    info->dec->init (info);

//    struct timeval tm2;
//    gettimeofday (&tm2, NULL);
//    int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
//    printf ("mp3_parse_file took %d ms\n", ms);
	trace ("seeked to %d\n", info->currentsample);
    _info->readpos = (float)(info->currentsample - info->startsample) / info->mp3info.ref_packet.samplerate;
    return 0;
}

static int
cmp3_seek_sample (DB_fileinfo_t *_info, int sample) {
    return cmp3_seek_sample64(_info, sample);
}

static int
cmp3_seek (DB_fileinfo_t *_info, float time) {
    mp3_info_t *info = (mp3_info_t *)_info;
    int64_t sample = (double)time * (int64_t)info->mp3info.ref_packet.samplerate;
    return cmp3_seek_sample64 (_info, sample);
}

static DB_playItem_t *
cmp3_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("cmp3_insert %s\n", fname);
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("failed to open file %s\n", fname);
        return NULL;
    }
    if (fp->vfs->is_streaming ()) {
        DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);
        deadbeef->fclose (fp);
        deadbeef->pl_add_meta (it, "title", NULL);
        deadbeef->plt_set_item_duration (plt, it, -1);
        after = deadbeef->plt_insert_item (plt, after, it);
        deadbeef->pl_item_unref (it);
        return after;
    }

    uint32_t start;
    uint32_t end;
    deadbeef->junk_get_tag_offsets (fp, &start, &end);

    mp3info_t mp3info;

    uint64_t fsize = deadbeef->fgetlength(fp);
    uint32_t mp3flags = 0;
    if (fp->vfs->is_streaming () && fsize >= 0) {
        mp3flags = MP3_PARSE_ESTIMATE_DURATION;
    }

    int res = _mp3_parse_and_validate(&mp3info, mp3flags, fp, fsize, start, end, -1);

    if (res < 0) {
        trace ("mp3: mp3_parse_file returned error\n");
        deadbeef->fclose (fp);
        return NULL;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);

    deadbeef->rewind (fp);
    // reset tags
    uint32_t f = deadbeef->pl_get_item_flags (it);
    f &= ~DDB_TAG_MASK;
    deadbeef->pl_set_item_flags (it, f);
    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
    /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
    deadbeef->pl_set_meta_int (it, ":MP3_DELAY", mp3info.delay);
    deadbeef->pl_set_meta_int (it, ":MP3_PADDING", mp3info.padding);

    deadbeef->plt_set_item_duration (plt, it, (float)((double)mp3info.totalsamples/mp3info.ref_packet.samplerate));

    cmp3_set_extra_properties (it, &mp3info, 0);

    deadbeef->fclose (fp);

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it, mp3info.totalsamples-mp3info.delay-mp3info.padding, mp3info.ref_packet.samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        return cue;
    }

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

int
cmp3_read_metadata (DB_playItem_t *it) {
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    DB_FILE *fp = deadbeef->fopen (uri);
    if (!fp) {
        return -1;
    }
    deadbeef->pl_delete_all_meta (it);
    // FIXME: reload and apply the Xing header
    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
    /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->fclose (fp);
    return 0;
}

int
cmp3_write_metadata (DB_playItem_t *it) {
    // get options

    int strip_id3v2 = deadbeef->conf_get_int ("mp3.strip_id3v2", 0);
    int strip_id3v1 = deadbeef->conf_get_int ("mp3.strip_id3v1", 0);
    int strip_apev2 = deadbeef->conf_get_int ("mp3.strip_apev2", 0);
    int write_id3v2 = deadbeef->conf_get_int ("mp3.write_id3v2", 1);
    int write_id3v1 = deadbeef->conf_get_int ("mp3.write_id3v1", 1);
    int write_apev2 = deadbeef->conf_get_int ("mp3.write_apev2", 0);

    uint32_t junk_flags = 0;
    if (strip_id3v2) {
        junk_flags |= JUNK_STRIP_ID3V2;
    }
    if (strip_id3v1) {
        junk_flags |= JUNK_STRIP_ID3V1;
    }
    if (strip_apev2) {
        junk_flags |= JUNK_STRIP_APEV2;
    }
    if (write_id3v2) {
        junk_flags |= JUNK_WRITE_ID3V2;
    }
    if (write_id3v1) {
        junk_flags |= JUNK_WRITE_ID3V1;
    }
    if (write_apev2) {
        junk_flags |= JUNK_WRITE_APEV2;
    }

    int id3v2_version = deadbeef->conf_get_int ("mp3.id3v2_version", 3);
    if (id3v2_version != 3 && id3v2_version != 4) {
        id3v2_version = 3;
    }
    char id3v1_encoding[50];
    deadbeef->conf_get_str ("mp3.id3v1_encoding", "cp1252", id3v1_encoding, sizeof (id3v1_encoding));
    return deadbeef->junk_rewrite_tags (it, junk_flags, id3v2_version, id3v1_encoding);
}

static const char *exts[] = {
	"mp1", "mp2", "mp3", "mpga", NULL
};

static const char settings_dlg[] =
    "property \"Force 16 bit output\" checkbox mp3.force16bit 0;\n"
#if defined(USE_LIBMAD) && defined(USE_LIBMPG123)
    "property \"Backend\" select[2] mp3.backend 0 mpg123 mad;\n"
#endif
;

// define plugin interface
static ddb_decoder2_t plugin = {
    .decoder.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .decoder.plugin.api_vminor = DB_API_VERSION_MINOR,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 0,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_REPLAYGAIN|DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "stdmpg",
    .decoder.plugin.name = "MP3 player",
    .decoder.plugin.descr = "MPEG v1/2 layer1/2/3 decoder\n\n"
#if defined(USE_LIBMPG123) && defined(USE_LIBMAD)
    "Can use libmad and libmpg123 backends.\n"
    "Changing the backend will take effect when the next track starts.\n"
#elif defined(USE_LIBMAD)
    "Using libmad backend.\n"
#elif defined(USE_LIBMPG123)
    "Using libmpg123 backend.\n"
#endif
    ,
    .decoder.plugin.copyright =
        "MPEG decoder plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .decoder.plugin.website = "http://deadbeef.sf.net",
    .decoder.plugin.configdialog = settings_dlg,
    .decoder.open = cmp3_open,
    .decoder.init = cmp3_init,
    .decoder.free = cmp3_free,
    .decoder.read = cmp3_read,
    .decoder.seek = cmp3_seek,
    .decoder.seek_sample = cmp3_seek_sample,
    .decoder.insert = cmp3_insert,
    .decoder.read_metadata = cmp3_read_metadata,
    .decoder.write_metadata = cmp3_write_metadata,
    .decoder.exts = exts,
    .seek_sample64 = cmp3_seek_sample64,
};

DB_plugin_t *
mp3_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
