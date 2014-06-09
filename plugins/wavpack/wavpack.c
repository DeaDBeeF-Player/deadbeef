/*
    WavPack plugin for DeaDBeeF Player
    Copyright (C) 2009-2011, Alexey Yakovenko <waker@users.sourceforge.net>
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

#include <string.h>
#ifdef TINYWV
#include <wavpack.h>
#else
#include <wavpack/wavpack.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    DB_FILE *file;
#ifndef TINYWV
    DB_FILE *c_file;
#endif
    WavpackContext *ctx;
    int startsample;
    int endsample;
} wvctx_t;

#ifdef TINYWV
int32_t wv_read_stream(void *buf, int32_t sz, void *file_handle) {
    return deadbeef->fread (buf, 1, sz, (DB_FILE *)file_handle);
}
#else
int32_t wv_read_bytes(void *id, void *data, int32_t bcount) {
//    trace ("wv_read_bytes\n");
    return deadbeef->fread (data, 1, bcount, id);
}

uint32_t wv_get_pos(void *id) {
//    trace ("wv_get_pos\n");
    return deadbeef->ftell (id);
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
    return deadbeef->ftell (id);
}
uint32_t wv_get_length(void *id) {
//    trace ("wv_get_length\n");
    size_t pos = deadbeef->ftell (id);
    deadbeef->fseek (id, 0, SEEK_END);
    size_t sz = deadbeef->ftell (id);
    deadbeef->fseek (id, pos, SEEK_SET);
    return sz;
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
    DB_fileinfo_t *_info = malloc (sizeof (wvctx_t));
    memset (_info, 0, sizeof (wvctx_t));
    return _info;
}

static int
wv_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    wvctx_t *info = (wvctx_t *)_info;
    deadbeef->pl_lock ();
    info->file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    if (!info->file) {
        return -1;
    }

#ifndef TINYWV
    deadbeef->pl_lock ();
    const char *uri = deadbeef->pl_find_meta (it, ":URI");
    char *c_fname = alloca (strlen (uri) + 2);
    if (c_fname) {
        strcpy (c_fname, uri);
        strcat (c_fname, "c");
        info->c_file = deadbeef->fopen (c_fname);
    }
    else {
        fprintf (stderr, "wavpack warning: failed to alloc memory for correction file name\n");
    }
    deadbeef->pl_unlock ();
#endif

    char error[80];
#ifdef TINYWV
    info->ctx = WavpackOpenFileInput (wv_read_stream, info->file, error);
#else
    info->ctx = WavpackOpenFileInputEx (&wsr, info->file, info->c_file, error, OPEN_NORMALIZE, 0);
#endif
    if (!info->ctx) {
        fprintf (stderr, "wavpack error: %s\n", error);
        return -1;
    }
    _info->plugin = &plugin;
    _info->fmt.bps = WavpackGetBytesPerSample (info->ctx) * 8;
    _info->fmt.channels = WavpackGetNumChannels (info->ctx);
    _info->fmt.samplerate = WavpackGetSampleRate (info->ctx);
    _info->fmt.is_float = (WavpackGetMode (info->ctx) & MODE_FLOAT) ? 1 : 0;

    // FIXME: streamer and maybe output plugins need to be fixed to support
    // arbitrary channelmask

    // _info->fmt.channelmask = WavpackGetChannelMask (info->ctx);

    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->readpos = 0;
    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        if (plugin.seek_sample (_info, 0) < 0) {
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
        free (_info);
    }
}

static int
wv_read (DB_fileinfo_t *_info, char *bytes, int size) {
    wvctx_t *info = (wvctx_t *)_info;
    int currentsample = WavpackGetSampleIndex (info->ctx);
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (size / samplesize + currentsample > info->endsample) {
        size = (info->endsample - currentsample + 1) * samplesize;
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
wv_seek_sample (DB_fileinfo_t *_info, int sample) {
#ifndef TINYWV
    wvctx_t *info = (wvctx_t *)_info;
    WavpackSeekSample (info->ctx, sample + info->startsample);
    _info->readpos = (float)(WavpackGetSampleIndex (info->ctx) - info->startsample) / WavpackGetSampleRate (info->ctx);
#endif
    return 0;
}

static int
wv_seek (DB_fileinfo_t *_info, float sec) {
    wvctx_t *info = (wvctx_t *)_info;
    return wv_seek_sample (_info, sec * WavpackGetSampleRate (info->ctx));
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
    WavpackContext *ctx = WavpackOpenFileInputEx (&wsr, fp, NULL, error, 0, 0);
#endif
    if (!ctx) {
        fprintf (stderr, "wavpack error: %s\n", error);
        deadbeef->fclose (fp);
        return NULL;
    }
    int totalsamples = WavpackGetNumSamples (ctx);
    int samplerate = WavpackGetSampleRate (ctx);
    float duration = (float)totalsamples / samplerate;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
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
    snprintf (s, sizeof (s), "%lld", deadbeef->fgetlength (fp));
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
    snprintf (s, sizeof (s), "%d", WavpackGetBytesPerSample (ctx) * 8);
    deadbeef->pl_add_meta (it, ":BPS", s);
    snprintf (s, sizeof (s), "%d", WavpackGetNumChannels (ctx));
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", WavpackGetSampleRate (ctx));
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    snprintf (s, sizeof (s), "%d", (int)(WavpackGetAverageBitrate (ctx, 1) / 1000));
    deadbeef->pl_add_meta (it, ":BITRATE", s);
    snprintf (s, sizeof (s), "%s", (WavpackGetMode (ctx) & MODE_FLOAT) ? "FLOAT" : "INTEGER");
    deadbeef->pl_add_meta (it, ":WAVPACK_MODE", s);

    // embedded cue
    deadbeef->pl_lock ();
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    if (cuesheet) {
        trace ("found cuesheet: %s\n", cuesheet);
        DB_playItem_t *last = deadbeef->plt_insert_cue_from_buffer (plt, after, it, cuesheet, strlen (cuesheet), totalsamples, samplerate);
        if (last) {
            deadbeef->pl_unlock ();
            deadbeef->fclose (fp);
            WavpackCloseFile (ctx);
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (last);
            return last;
        }
    }
    deadbeef->pl_unlock ();
    // cue file on disc
    DB_playItem_t *cue_after = deadbeef->plt_insert_cue (plt, after, it, totalsamples, samplerate);
    if (cue_after) {
        deadbeef->fclose (fp);
        WavpackCloseFile (ctx);
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue_after);
        return cue_after;
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
    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
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

static const char *exts[] = { "wv", NULL };
// define plugin interface
static DB_decoder_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "wv",
    .plugin.name = "WavPack decoder",
    .plugin.descr = "WavPack (.wv, .iso.wv) player",
    .plugin.copyright = 
        "WavPack plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2011, Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
    .plugin.website = "http://deadbeef.sf.net",
    .open = wv_open,
    .init = wv_init,
    .free = wv_free,
    .read = wv_read,
    .seek = wv_seek,
    .seek_sample = wv_seek_sample,
    .insert = wv_insert,
    .read_metadata = wv_read_metadata,
    .write_metadata = wv_write_metadata,
    .exts = exts,
};

DB_plugin_t *
wavpack_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
