/*
    WavPack plugin for DeaDBeeF Player
    Copyright (C) 2009-2011, Oleksiy Yakovenko <waker@users.sourceforge.net>
    Copyright (C) 2010-2011, David Bryant <david@wavpack.com>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
          names of its contributors may be used to endorse or promote products
          derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <string.h>
#if defined(TINYWV) || defined(OSX_BUILD)
#include <wavpack.h>
#else
#include <wavpack/wavpack.h>
#endif
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static ddb_decoder2_t plugin;
static DB_functions_t *deadbeef;
static int enable_dop = 0;

typedef struct {
    DB_fileinfo_t info;
    DB_FILE *file;
#ifndef TINYWV
    DB_FILE *c_file;
#endif
    WavpackContext *ctx;
    int64_t startsample;
    int64_t endsample;
    int output_dop;
    uint8_t dop_marker;
    uint8_t *dsd_pending;
    int have_dsd_pending;
} wvctx_t;

static int
wv_seek_sample64 (DB_fileinfo_t *_info, int64_t sample);

#ifdef TINYWV
int32_t wv_read_stream(void *buf, int32_t sz, void *file_handle) {
    return deadbeef->fread (buf, 1, sz, (DB_FILE *)file_handle);
}
#else
int32_t wv_read_bytes(void *id, void *data, int32_t bcount) {
//    trace ("wv_read_bytes\n");
    return (int32_t)deadbeef->fread (data, 1, bcount, id);
}

uint32_t wv_get_pos(void *id) {
//    trace ("wv_get_pos\n");
    return (uint32_t)deadbeef->ftell (id);
}

int wv_set_pos_abs(void *id, uint32_t pos) {
//    trace ("wv_set_pos_abs\n");
    return deadbeef->fseek (id, pos, SEEK_SET);
}
int wv_set_pos_rel(void *id, int32_t delta, int mode) {
//    trace ("wv_set_pos_rel\n");
    return deadbeef->fseek (id, delta, SEEK_CUR);
}
int wv_push_back_byte(void *id, int c) {
//    trace ("wv_push_back_byte\n");
    deadbeef->fseek (id, -1, SEEK_CUR);
    return (int)deadbeef->ftell (id);
}
uint32_t wv_get_length(void *id) {
//    trace ("wv_get_length\n");
    size_t pos = deadbeef->ftell (id);
    deadbeef->fseek (id, 0, SEEK_END);
    size_t sz = deadbeef->ftell (id);
    deadbeef->fseek (id, pos, SEEK_SET);
    return (uint32_t)sz;
}
int wv_can_seek(void *id) {
//    trace ("wv_can_seek\n");
    return 1;
}

int32_t wv_write_bytes (void *id, void *data, int32_t bcount) {
    return 0;
}

static WavpackStreamReader wsr = {
    .read_bytes = wv_read_bytes,
    .get_pos = wv_get_pos,
    .set_pos_abs = wv_set_pos_abs,
    .set_pos_rel = wv_set_pos_rel,
    .push_back_byte = wv_push_back_byte,
    .get_length = wv_get_length,
    .can_seek = wv_can_seek,
    .write_bytes = wv_write_bytes
};
#endif

static DB_fileinfo_t *
wv_open (uint32_t hints) {
    wvctx_t *info = calloc (1, sizeof (wvctx_t));
    return &info->info;
}

static int
wv_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    wvctx_t *info = (wvctx_t *)_info;
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    info->file = deadbeef->fopen (uri);
    if (!info->file) {
        return -1;
    }

#ifndef TINYWV
    deadbeef->pl_lock ();
    uri = deadbeef->pl_find_meta (it, ":URI");
    char *c_fname = alloca (strlen (uri) + 2);
    if (c_fname) {
        strcpy (c_fname, uri);
        deadbeef->pl_unlock ();
        strcat (c_fname, "c");
        info->c_file = deadbeef->fopen (c_fname);
    }
    else {
        deadbeef->pl_unlock ();
        fprintf (stderr, "wavpack warning: failed to alloc memory for correction file name\n");
    }
#endif

    char error[80];
#ifdef TINYWV
    info->ctx = WavpackOpenFileInput (wv_read_stream, info->file, error);
#else
    int flags = OPEN_NORMALIZE;

#if defined(OPEN_DSD_NATIVE) && defined(OPEN_DSD_AS_PCM)
    flags |= enable_dop ? OPEN_DSD_NATIVE : OPEN_DSD_AS_PCM;
#elif defined(OPEN_DSD_AS_PCM)
    flags |= OPEN_DSD_AS_PCM;
#endif

    info->ctx = WavpackOpenFileInputEx (&wsr, info->file, info->c_file, error, flags, 0);
#endif
    if (!info->ctx) {
        fprintf (stderr, "wavpack error: %s\n", error);
        return -1;
    }

#if defined(QMODE_DSD_AUDIO)
    int source_is_dsd = (WavpackGetQualifyMode (info->ctx) & QMODE_DSD_AUDIO) != 0;
#else
    int source_is_dsd = 0;
#endif

#if defined(OPEN_DSD_NATIVE)
    info->output_dop = enable_dop && source_is_dsd;
#else
    info->output_dop = 0;
#endif

    info->dop_marker = 0x05;

    _info->plugin = &plugin.decoder;
    _info->fmt.channels = WavpackGetNumChannels (info->ctx);

    if (info->output_dop) {
        /* OPEN_DSD_NATIVE returns one byte containing 8 consecutive DSD bits
           per channel, temporally MSB first. Two DSD bytes are packed into
           each DoP frame. This matches the DoP framing used by DeaDBeeF's
           FFmpeg decoder. */
        _info->fmt.bps = 32;
        _info->fmt.samplerate = WavpackGetSampleRate (info->ctx) / 2;
        _info->fmt.is_float = 0;
        _info->fmt.flags |= DDB_WAVEFORMAT_FLAG_IS_DOP;

        info->dsd_pending = calloc (_info->fmt.channels, sizeof (*info->dsd_pending));
        if (!info->dsd_pending) {
            return -1;
        }
    }
    else {
        _info->fmt.bps = WavpackGetBytesPerSample (info->ctx) * 8;
        _info->fmt.samplerate = WavpackGetSampleRate (info->ctx);
        _info->fmt.is_float = (WavpackGetMode (info->ctx) & MODE_FLOAT) ? 1 : 0;
        _info->fmt.flags &= ~DDB_WAVEFORMAT_FLAG_IS_DOP;
    }

    // FIXME: streamer and maybe output plugins need to be fixed to support
    // arbitrary channelmask

    // _info->fmt.channelmask = WavpackGetChannelMask (info->ctx);

    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->readpos = 0;
    int64_t endsample = deadbeef->pl_item_get_endsample (it);
    if (endsample > 0) {
        info->startsample = deadbeef->pl_item_get_startsample (it);
        info->endsample = endsample;
        if (wv_seek_sample64(_info, 0) < 0) {
            return -1;
        }
    }
    else {
        info->startsample = 0;
        info->endsample = WavpackGetNumSamples (info->ctx)-1;
    }
    return 0;
}

static void
wv_free (DB_fileinfo_t *_info) {
    if (_info) {
        wvctx_t *info = (wvctx_t *)_info;
        if (info->file) {
            deadbeef->fclose (info->file);
            info->file = NULL;
        }
#ifndef TINYWV
        if (info->c_file) {
            deadbeef->fclose (info->c_file);
            info->c_file = NULL;
        }
#endif
        if (info->ctx) {
            WavpackCloseFile (info->ctx);
            info->ctx = NULL;
        }
        free (info->dsd_pending);
        info->dsd_pending = NULL;
        free (_info);
    }
}

static int
wv_read (DB_fileinfo_t *_info, char *bytes, int size) {
    wvctx_t *info = (wvctx_t *)_info;

    if (info->output_dop) {
        int channels = _info->fmt.channels;
        int out_frame_size = channels * 4;
        int out_capacity = size / out_frame_size;
        int64_t current = WavpackGetSampleIndex64 (info->ctx);
        int64_t remaining = info->endsample - current + 1;

        if (remaining < 0) {
            remaining = 0;
        }

        /* A pending byte has already been consumed from WavPack but has not
           yet been emitted. Each DoP frame needs two DSD bytes per channel. */
        int64_t available = remaining + (info->have_dsd_pending ? 1 : 0);
        if (out_capacity > available / 2) {
            out_capacity = (int)(available / 2);
        }
        if (out_capacity <= 0) {
            return 0;
        }

        int input_frames = out_capacity * 2 - (info->have_dsd_pending ? 1 : 0);
        int32_t *buffer = alloca ((size_t)input_frames * channels * sizeof (*buffer));
        int got = WavpackUnpackSamples (info->ctx, buffer, input_frames);
        int in_frame = 0;
        int out_frames = 0;
        uint32_t *out = (uint32_t *)bytes;

        if (info->have_dsd_pending && got > 0) {
            for (int ch = 0; ch < channels; ch++) {
                uint8_t dsd0 = info->dsd_pending[ch];
                uint8_t dsd1 = (uint8_t)buffer[ch];
                *out++ = ((uint32_t)info->dop_marker << 24) |
                         ((uint32_t)dsd0 << 16) |
                         ((uint32_t)dsd1 << 8);
            }
            info->dop_marker = (uint8_t)~info->dop_marker;
            info->have_dsd_pending = 0;
            in_frame = 1;
            out_frames++;
        }

        while (in_frame + 1 < got && out_frames < out_capacity) {
            int32_t *a = buffer + (size_t)in_frame * channels;
            int32_t *b = buffer + (size_t)(in_frame + 1) * channels;

            for (int ch = 0; ch < channels; ch++) {
                uint8_t dsd0 = (uint8_t)a[ch];
                uint8_t dsd1 = (uint8_t)b[ch];
                *out++ = ((uint32_t)info->dop_marker << 24) |
                         ((uint32_t)dsd0 << 16) |
                         ((uint32_t)dsd1 << 8);
            }

            info->dop_marker = (uint8_t)~info->dop_marker;
            in_frame += 2;
            out_frames++;
        }

        if (in_frame < got) {
            int32_t *p = buffer + (size_t)in_frame * channels;
            for (int ch = 0; ch < channels; ch++) {
                info->dsd_pending[ch] = (uint8_t)p[ch];
            }
            info->have_dsd_pending = 1;
        }

        _info->readpos =
            (float)(WavpackGetSampleIndex64 (info->ctx) - info->startsample) /
            WavpackGetSampleRate (info->ctx);

#ifndef TINYWV
        deadbeef->streamer_set_bitrate (WavpackGetInstantBitrate (info->ctx) / 1000);
#endif

        return out_frames * out_frame_size;
    }

    int currentsample = WavpackGetSampleIndex (info->ctx);
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (size / samplesize + currentsample > info->endsample) {
        size = (int)((info->endsample - currentsample + 1) * samplesize);
        trace ("wv: size truncated to %d bytes (%d samples), cursample=%d, endsample=%d\n", size, info->endsample - currentsample + 1, currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int initsize = size;
    int n;
    if (WavpackGetMode (info->ctx) & MODE_FLOAT) {
        _info->fmt.is_float = 1;
    }
    if (_info->fmt.is_float || _info->fmt.bps == 32) {
        n = WavpackUnpackSamples (info->ctx, (int32_t *)bytes, size / samplesize);
        size -= n * samplesize;
    }
    else {
        int32_t buffer[size/(_info->fmt.bps / 8)];
        n = WavpackUnpackSamples (info->ctx, (int32_t *)buffer, size / samplesize);
        size -= n * samplesize;
        n *= _info->fmt.channels;

        // convert from int32 to input (what???)
        int32_t *p = buffer;
        if (_info->fmt.bps == 16) {
            while (n > 0) {
                *((int16_t *)bytes) = (int16_t)(*p);
                bytes += sizeof (int16_t);
                p++;
                n--;
            }
        }
        else if (_info->fmt.bps == 8) {
            while (n > 0) {
                *bytes++ = (char)(*p);
                p++;
                n--;
            }
        }
        else if (_info->fmt.bps == 24) {
            while (n > 0) {
                *bytes++ = (*p)&0xff;
                *bytes++ = ((*p)&0xff00)>>8;
                *bytes++ = ((*p)&0xff0000)>>16;
                p++;
                n--;
            }
        }
    }
    _info->readpos = (float)(WavpackGetSampleIndex (info->ctx)-info->startsample)/WavpackGetSampleRate (info->ctx);

#ifndef TINYWV
    deadbeef->streamer_set_bitrate (WavpackGetInstantBitrate (info->ctx) / 1000);
#endif

    return initsize-size;
}

static int
wv_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
#ifndef TINYWV
    wvctx_t *info = (wvctx_t *)_info;
    int64_t wavpack_sample = info->output_dop ? sample * 2 : sample;
    WavpackSeekSample64 (info->ctx, wavpack_sample + info->startsample);
    info->have_dsd_pending = 0;
    info->dop_marker = 0x05;
    _info->readpos = (float)((double)(WavpackGetSampleIndex64 (info->ctx) - info->startsample) / WavpackGetSampleRate (info->ctx));
#endif
    return 0;
}

static int
wv_seek_sample (DB_fileinfo_t *_info, int sample) {
    return wv_seek_sample64(_info, sample);
}

static int
wv_seek (DB_fileinfo_t *_info, float sec) {
    return wv_seek_sample64 (_info, (int64_t)((double)sec * _info->fmt.samplerate));
}

static DB_playItem_t *
wv_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }
    char error[80];
#ifdef TINYWV
    WavpackContext *ctx = WavpackOpenFileInput (wv_read_stream, fp, error);
#else
    int flags = 0;
#if defined(OPEN_DSD_NATIVE)
    flags |= OPEN_DSD_NATIVE;
#elif defined(OPEN_DSD_AS_PCM)
    flags |= OPEN_DSD_AS_PCM;
#endif
    WavpackContext *ctx = WavpackOpenFileInputEx (&wsr, fp, NULL, error, flags, 0);
#endif
    if (!ctx) {
        fprintf (stderr, "wavpack error: %s\n", error);
        deadbeef->fclose (fp);
        return NULL;
    }
    int totalsamples = WavpackGetNumSamples (ctx);
    /* For DSD, WavpackGetSampleRate() is the DSD-byte rate. Keep that rate
       for duration and cue calculations; use the native bit rate only for
       user-visible metadata below. */
    int samplerate = WavpackGetSampleRate (ctx);
#if defined(QMODE_DSD_AUDIO)
    int is_dsd = (WavpackGetQualifyMode (ctx) & QMODE_DSD_AUDIO) != 0;
#else
    int is_dsd = 0;
#endif
    float duration = (float)totalsamples / samplerate;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", "wv");
    deadbeef->plt_set_item_duration (plt, it, duration);
    trace ("wv: totalsamples=%d, samplerate=%d, duration=%f\n", totalsamples, samplerate, duration);

#if 0
    int num = WavpackGetNumTagItems (ctx);
    trace ("num tag items: %d\n", num);

    for (int i = 0; i < num; i++) {
        char str[1024];
        WavpackGetTagItemIndexed (ctx, i, str, sizeof (str));
        trace ("tag item: %s\n", str);
    }

#endif
    int apeerr = deadbeef->junk_apev2_read (it, fp);
    if (!apeerr) {
        trace ("wv: ape tag found\n");
    }
    int v1err = deadbeef->junk_id3v1_read (it, fp);
    if (!v1err) {
        trace ("wv: id3v1 tag found\n");
    }
    deadbeef->pl_add_meta (it, "title", NULL);

    char s[100];
    snprintf (s, sizeof (s), "%lld", (long long)deadbeef->fgetlength (fp));
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
    snprintf (s, sizeof (s), "%d", is_dsd ? 1 : WavpackGetBytesPerSample (ctx) * 8);
    deadbeef->pl_add_meta (it, ":BPS", s);
    snprintf (s, sizeof (s), "%d", WavpackGetNumChannels (ctx));
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
#if defined(QMODE_DSD_AUDIO)
    snprintf (s, sizeof (s), "%u",
              is_dsd ? WavpackGetNativeSampleRate (ctx) : WavpackGetSampleRate (ctx));
#else
    snprintf (s, sizeof (s), "%u", WavpackGetSampleRate (ctx));
#endif
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    snprintf (s, sizeof (s), "%d", (int)(WavpackGetAverageBitrate (ctx, 1) / 1000));
    deadbeef->pl_add_meta (it, ":BITRATE", s);
    snprintf (s, sizeof (s), "%s",
              is_dsd ? "DSD" : (WavpackGetMode (ctx) & MODE_FLOAT) ? "FLOAT" : "INTEGER");
    deadbeef->pl_add_meta (it, ":WAVPACK_MODE", s);

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it,  totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        deadbeef->fclose (fp);
        WavpackCloseFile (ctx);
        return cue;
    }

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);

    deadbeef->fclose (fp);
    WavpackCloseFile (ctx);
    return after;
}

int
wv_read_metadata (DB_playItem_t *it) {
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    DB_FILE *fp = deadbeef->fopen (uri);
    if (!fp) {
        return -1;
    }
    deadbeef->pl_delete_all_meta (it);
    int apeerr = deadbeef->junk_apev2_read (it, fp);
    if (!apeerr) {
        trace ("wv: ape tag found\n");
    }
    int v1err = deadbeef->junk_id3v1_read (it, fp);
    if (!v1err) {
        trace ("wv: id3v1 tag found\n");
    }
    deadbeef->fclose (fp);
    return 0;
}

int
wv_write_metadata (DB_playItem_t *it) {
    int strip_apev2 = deadbeef->conf_get_int ("wv.strip_apev2", 0);
    int strip_id3v1 = deadbeef->conf_get_int ("wv.strip_id3v1", 0);
    int write_apev2 = deadbeef->conf_get_int ("wv.write_apev2", 1);
    int write_id3v1 = deadbeef->conf_get_int ("wv.write_id3v1", 0);

    uint32_t junk_flags = 0;
    if (strip_id3v1) {
        junk_flags |= JUNK_STRIP_ID3V1;
    }
    if (strip_apev2) {
        junk_flags |= JUNK_STRIP_APEV2;
    }
    if (write_id3v1) {
        junk_flags |= JUNK_WRITE_ID3V1;
    }
    if (write_apev2) {
        junk_flags |= JUNK_WRITE_APEV2;
    }

    return deadbeef->junk_rewrite_tags (it, junk_flags, 0, NULL);
}

static void
wv_init_from_config (void) {
    deadbeef->conf_lock ();
    enable_dop = deadbeef->conf_get_int ("wv.enable_dop", 0);
    deadbeef->conf_unlock ();
}

static int
wv_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (id == DB_EV_CONFIGCHANGED) {
        wv_init_from_config ();
    }
    return 0;
}

static int
wv_start (void) {
    wv_init_from_config ();
    return 0;
}

static const char settings_dlg[] =
    "property \"Enable DoP output for DSD\" checkbox wv.enable_dop 0;\n";

static const char *exts[] = { "wv", NULL };
// define plugin interface
static ddb_decoder2_t plugin = {
    .decoder.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .decoder.plugin.api_vminor = DB_API_VERSION_MINOR,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 1,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "wv",
    .decoder.plugin.name = "WavPack decoder",
    .decoder.plugin.descr = "WavPack (.wv, .iso.wv) player",
    .decoder.plugin.copyright =
        "WavPack plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2011, Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "Copyright (C) 2010-2011, David Bryant <david@wavpack.com>\n"
        "All rights reserved.\n"
        "\n"
        "Redistribution and use in source and binary forms, with or without\n"
        "modification, are permitted provided that the following conditions are met:\n"
        "    * Redistributions of source code must retain the above copyright\n"
        "      notice, this list of conditions and the following disclaimer.\n"
        "    * Redistributions in binary form must reproduce the above copyright\n"
        "      notice, this list of conditions and the following disclaimer in the\n"
        "      documentation and/or other materials provided with the distribution.\n"
        "    * Neither the name of the <organization> nor the\n"
        "      names of its contributors may be used to endorse or promote products\n"
        "      derived from this software without specific prior written permission.\n"
        "\n"
        "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\n"
        "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n"
        "WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
        "DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY\n"
        "DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n"
        "(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n"
        "LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n"
        "ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
        "(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n"
        "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
    ,
    .decoder.plugin.website = "http://deadbeef.sf.net",
    .decoder.plugin.start = wv_start,
    .decoder.plugin.configdialog = settings_dlg,
    .decoder.plugin.message = wv_message,
    .decoder.open = wv_open,
    .decoder.init = wv_init,
    .decoder.free = wv_free,
    .decoder.read = wv_read,
    .decoder.seek = wv_seek,
    .decoder.seek_sample = wv_seek_sample,
    .decoder.insert = wv_insert,
    .decoder.read_metadata = wv_read_metadata,
    .decoder.write_metadata = wv_write_metadata,
    .decoder.exts = exts,
    .seek_sample64 = wv_seek_sample64,
};

DB_plugin_t *
wavpack_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
