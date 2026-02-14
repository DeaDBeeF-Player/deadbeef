/*
    WMA plugin for deadbeef
    Copyright (C) 2013 Oleksiy Yakovenko <waker@users.sourceforge.net>
    WMA and ASF libraries (C) RockBox & FFMPEG developers

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
#include <deadbeef/deadbeef.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "libasf/asf.h"

//#define USE_FFMPEG 1

#if USE_FFMPEG
#include "libwma-ff/wma.h"
#else
#include "libwma/wmadec.h"
#endif

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static ddb_decoder2_t plugin;
DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    asf_waveformatex_t wfx;
#if USE_FFMPEG
    WMACodecContext wmadec;
#else
    WMADecodeContext wmadec;
#endif
    int64_t first_frame_offset;
    int64_t currentsample;
    int64_t startsample;
    int64_t endsample;
    int skipsamples;
    char buffer[200000]; // can't predict its size, so set to max
    int remaining;
    int open2_was_used;
} wmaplug_info_t;

static int
wmaplug_seek_sample64 (DB_fileinfo_t *_info, int64_t sample);

// allocate codec control structure
static DB_fileinfo_t *
wmaplug_open (uint32_t hints) {
    wmaplug_info_t *info = calloc (1, sizeof (wmaplug_info_t));
    return &info->info;
}

static DB_fileinfo_t *
wmaplug_open2 (uint32_t hints, DB_playItem_t *it) {
    DB_FILE *file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    if (!file) {
        return NULL;
    }

    wmaplug_info_t *info = calloc (1, sizeof (wmaplug_info_t));
    info->open2_was_used = 1;
    info->info.file = file;

    return &info->info;
}

int get_asf_metadata(DB_FILE *fd, DB_playItem_t *it, asf_waveformatex_t *wfx, int64_t *first_frame_offset);

static int
wmaplug_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    wmaplug_info_t *info = (wmaplug_info_t *)_info;

    if (!info->open2_was_used) {
        info->info.file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    }

    if (!info->info.file) {
        return -1;
    }

    trace ("opened %s\n", deadbeef->pl_find_meta (it, ":URI"));

//    char *buffer = malloc (2000000);
//    deadbeef->fread (buffer, 200000, 1, info->info.file);
//    FILE *out = fopen ("out.wma", "w+b");
//    fwrite (buffer, 200000, 1, out);
//    exit (0);

    int res = get_asf_metadata (info->info.file, NULL, &info->wfx, &info->first_frame_offset);
    if (!res) {
        return -1;
    }
    trace ("get_asf_metadata returned %d, first_frame_offset: %lld\n", res, info->first_frame_offset);
    int64_t pos = deadbeef->ftell (info->info.file);
    trace ("curr offs: %lld\n", pos);
    if (info->first_frame_offset > pos) {
        char buf[info->first_frame_offset-pos];
        trace ("skipping %d bytes\n", sizeof (buf));
        deadbeef->fread (buf, sizeof (buf), 1, info->info.file);
    }
#if USE_FFMPEG
    info->wmadec.sample_rate = info->wfx.rate;
    info->wmadec.nb_channels = info->wfx.channels;
    info->wmadec.channels = info->wfx.channels;
    info->wmadec.bit_rate = info->wfx.bitrate;
    info->wmadec.block_align = info->wfx.blockalign;
    info->wmadec.codec_id = info->wfx.codec_id;
    trace ("codec id: %x\n",  info->wmadec.codec_id);
    info->wmadec.extradata = info->wfx.data;
    if (wma_decode_init (&info->wmadec)) {
        trace ("wma_decode_init fail\n");
        return -1;
    }
    trace ("wma_decode_init success\n");
#else
    if (wma_decode_init(&info->wmadec,&info->wfx) < 0) {
        trace ("wma_decode_init returned -1\n");
        return -1;
    }
#endif

    if (info->wmadec.frame_len <= 0) {
        trace ("wma error: frame_len = %d\n", info->wmadec.frame_len);
        return -1;
    }

    info->startsample = deadbeef->pl_item_get_startsample (it);
    info->endsample = deadbeef->pl_item_get_endsample (it);
    _info->plugin = &plugin.decoder;
    _info->fmt.bps = info->wfx.bitspersample;
    _info->fmt.channels = info->wfx.channels;
    _info->fmt.samplerate = info->wfx.rate;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }

    if (!info->info.file->vfs->is_streaming ()) {
        int64_t endsample = deadbeef->pl_item_get_endsample (it);
        if (endsample > 0) {
            info->startsample = deadbeef->pl_item_get_startsample (it);
            info->endsample = endsample;
            wmaplug_seek_sample64 (_info, 0);
        }
    }
    if (info->info.file->vfs->is_streaming ()) {
        deadbeef->pl_replace_meta (it, "!FILETYPE", "WMA");
    }

    return 0;
}

static void
wmaplug_free (DB_fileinfo_t *_info) {
    wmaplug_info_t *info = (wmaplug_info_t *)_info;
    if (info) {
#if USE_FFMPEG
        ff_wma_end (&info->wmadec);
#endif
        if (info->info.file) {
            deadbeef->fclose (info->info.file);
            info->info.file = NULL;
        }
        free (info);
    }
}

static int
wmaplug_read (DB_fileinfo_t *_info, char *bytes, int size) {
    wmaplug_info_t *info = (wmaplug_info_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (!info->info.file->vfs->is_streaming () && info->endsample > info->startsample) {
        if (info->currentsample + size / samplesize > info->endsample) {
            size = (info->endsample - info->currentsample + 1) * samplesize;
            if (size <= 0) {
                trace ("wmaplug_read: eof (current=%d, total=%d)\n", info->currentsample, info->endsample);
                return 0;
            }
        }
    }
    int initsize = size;

#if !USE_FFMPEG
    while (size > 0) {
        int sould_read_next_packet = 1;
        while (sould_read_next_packet && info->remaining == 0) {
            sould_read_next_packet = 0;
            int errcount = 0;
            int res = 0;
            uint8_t *audiobuf_mem = calloc(1, info->wfx.packet_size);
            if (audiobuf_mem == NULL) {
                trace("wma: could not allocate memory for packet size: %d\n", (int)info->wfx.packet_size);
                break;
            }
            uint8_t *audiobuf = audiobuf_mem;
            int audiobufsize = 0;
            int packetlength = 0;
    new_packet:
            res = asf_read_packet(&audiobuf, &audiobufsize, &packetlength, &info->wfx, info->info.file);
            if (res > 0) {
                int nb = audiobufsize / info->wfx.blockalign;
                for (int b = 0; !sould_read_next_packet && b < nb; b++) {
                    wma_decode_superframe_init(&info->wmadec, audiobuf + b * info->wfx.blockalign, info->wfx.blockalign);

                    int n = 0;
//                    trace ("subframes: %d\n", info->wmadec.nb_frames);
                    for (int i=0; i < info->wmadec.nb_frames; i++)
                    {
                        int wmares = wma_decode_superframe_frame(&info->wmadec,
                                audiobuf + b * info->wfx.blockalign, info->wfx.blockalign);

                        if (wmares < 0) {
                            /* Do the above, but for errors in decode. */
                            errcount++;
                            trace ("WMA decode error %d, errcount %d\n",wmares, errcount);
                            if (errcount <= 5) {
                                sould_read_next_packet = 1;
                            }
                            break;
                        } else if (wmares > 0) {
                            if (wmares * info->wfx.channels * info->wfx.bitspersample / 8 > sizeof (info->buffer) - info->remaining) {
                                fprintf (stderr, "WMA: decoding buffer is too small\n");
                                break;
                            }

                            int16_t *p = (int16_t *)&(info->buffer[info->remaining]);
                            for (int s = 0; s < wmares; s++) {
                                for (int ch = 0; ch < info->wfx.channels; ch++) {
                                    fixed32 *chan = info->wmadec.frame_out[ch];
                                    p[s*info->wfx.channels+ch] = chan[s] >> 15;
                                }
                            }
                            info->remaining += wmares * info->wfx.channels * info->wfx.bitspersample / 8;
                        }
                    }
                }
            }
            free (audiobuf_mem);
        }

        if (info->remaining == 0) {
            // error
            break;
        }

        if (info->skipsamples > 0) {
            int skip = info->skipsamples * samplesize;
            skip = min (info->remaining, skip);
            if (skip != info->remaining) {
                memmove (info->buffer, info->buffer + skip, info->remaining - skip);
            }
            info->remaining -= skip;
            info->skipsamples -= skip / samplesize;
        }
        if (info->remaining > 0) {
            int sz = min (size, info->remaining);
            if (sz == 0) {
                break;
            }
            memcpy (bytes, info->buffer, sz);
            if (info->remaining != sz) {
                memmove (info->buffer, info->buffer+sz, info->remaining-sz);
            }
            info->remaining -= sz;
            size -= sz;
            bytes += sz;
        }
    }
#else
// {{{ ffmpeg
    while (size > 0) {
        if (info->remaining == 0) {
            int errcount = 0;
            int res = 0;
            uint8_t audiobuf_mem[40000];
            uint8_t* audiobuf = audiobuf_mem;
            int audiobufsize = 0;
            int packetlength = 0;
    new_packet:
            {
            int pos = deadbeef->ftell (info->info.file);
            res = asf_read_packet(&audiobuf, &audiobufsize, &packetlength, &info->wfx, info->info.file);
            int endpos = deadbeef->ftell (info->info.file);
            trace ("[2] packet pos: %d, packet size: %d, data size: %d\n", pos, endpos-pos, packetlength);
            }
            if (res > 0) {
                int nblocks = audiobufsize / info->wfx.blockalign;
                for (int i = 0 ; i < nblocks; i++) {
                    int got_frame_ptr = 0;
                    char *data;
                    int bufsize = wma_decode_superframe (&info->wmadec, &got_frame_ptr, audiobuf + i * info->wfx.blockalign, info->wfx.blockalign);
                    trace ("got frame ptr: %d, bufsize: %d\n", got_frame_ptr, info->wmadec.nb_samples * 4);

                    int16_t *p = (int16_t *)&info->buffer[info->remaining];
                    memcpy (p, info->wmadec.output_buffer, info->wmadec.nb_samples * 4);
                    info->remaining += info->wmadec.nb_samples * 4;
                }
            }
        }

        int sz = min (size, info->remaining);
        if (sz == 0) {
            trace ("buffer is empty\n");
            break;
        }
        memcpy (bytes, info->buffer, sz);
        if (info->remaining != sz) {
            memmove (info->buffer, info->buffer+sz, info->remaining-sz);
        }
        info->remaining -= sz;
        size -= sz;
    }
#endif
// }}}

    info->currentsample += (initsize-size) / samplesize;
//    trace ("read ret: %d, (initsize: %d)\n", initsize-size, initsize);
    return initsize-size;
}

static int
wmaplug_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
    wmaplug_info_t *info = (wmaplug_info_t *)_info;

    sample += info->startsample;

    trace ("seek to sample %lld\n", sample);

    info->remaining = 0;
    info->wmadec.last_superframe_len = 0;
    info->wmadec.last_bitoffset = 0;

    memset(info->wmadec.frame_out, 0, sizeof(fixed32) * MAX_CHANNELS * BLOCK_MAX_SIZE * 2);

    int skip_ms;
    int res = asf_seek (sample * 1000 / info->wfx.rate, &info->wfx, info->info.file, info->first_frame_offset, &skip_ms);
    if (res < 0) {
        info->skipsamples = 0;
        info->currentsample = 0;
    }
    else {
        info->skipsamples = (int64_t)skip_ms * info->wfx.rate / 1000;
        info->currentsample = sample;
    }

    _info->readpos = (float)((double)(info->currentsample - info->startsample)/_info->fmt.samplerate);

    return 0;
}

static int
wmaplug_seek_sample (DB_fileinfo_t *_info, int sample) {
    return wmaplug_seek_sample64(_info, sample);
}

static int
wmaplug_seek (DB_fileinfo_t *_info, float t) {
    return wmaplug_seek_sample (_info, (int64_t)((double)t * (int64_t)_info->fmt.samplerate));
}

static int
wmaplug_read_metadata (DB_playItem_t *it) {
    deadbeef->pl_delete_all_meta (it);
    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    if (!fp) {
        return -1;
    }
    asf_waveformatex_t wfx;
    int64_t first_frame_offset;
    int res = get_asf_metadata (fp, it, &wfx, &first_frame_offset);
    deadbeef->fclose (fp);
    if (!res) {
        return -1;
    }
    return 0;
}

static DB_playItem_t *
wmaplug_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    asf_waveformatex_t wfx;

    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }

    int64_t first_frame_offset;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);

    int res = get_asf_metadata (fp, it, &wfx, &first_frame_offset);
    if (!res) {
        deadbeef->pl_item_unref (it);
        return NULL;
    }
    //trace ("datalen %d, channels %d, bps %d, rate %d\n", wfx.datalen, wfx.channels, wfx.bitspersample, wfx.rate);
    trace ("packet_size: %d, max_packet_size: %d\n", wfx.packet_size, wfx.max_packet_size);

    int64_t l = deadbeef->fgetlength (fp);
    deadbeef->fclose (fp);
    fp = NULL;

    int64_t i_count = (l - first_frame_offset) / wfx.packet_size;

    int64_t i_length = wfx.play_duration / 10 * i_count / wfx.numpackets - wfx.preroll * 1000;
    trace ("i_length: %lld (%lld / 10 * %lld / %lld - %lld * 1000)\n", i_length, wfx.play_duration, i_count, wfx.numpackets, wfx.preroll);

    int64_t totalsamples = i_length / 1000 * wfx.rate / 1000;
    trace ("totalsamples: %lld (%lld / %d)\n", totalsamples, i_length, wfx.rate);

    deadbeef->plt_set_item_duration (plt, it, totalsamples / (float)wfx.rate);
    deadbeef->pl_append_meta (it, ":FILETYPE", "WMA");
    
    deadbeef->pl_item_set_startsample (it, 0);
    deadbeef->pl_item_set_endsample (it, totalsamples-1);

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it,  totalsamples, wfx.rate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        return cue;
    }

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

static const char * exts[] = { "wma", NULL };

// define plugin interface
static ddb_decoder2_t plugin = {
    .decoder.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .decoder.plugin.api_vminor = DB_API_VERSION_MINOR,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 0,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "wma",
    .decoder.plugin.name = "WMA player",
    .decoder.plugin.descr = "plays WMA files",
    .decoder.plugin.copyright =
        "WMA plugin for deadbeef\n"
        "Copyright (C) 2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "WMA and ASF libraries (C) RockBox & FFMPEG developers\n"
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
    .decoder.open = wmaplug_open,
    .decoder.open2 = wmaplug_open2,
    .decoder.init = wmaplug_init,
    .decoder.free = wmaplug_free,
    .decoder.read = wmaplug_read,
    .decoder.seek = wmaplug_seek,
    .decoder.seek_sample = wmaplug_seek_sample,
    .decoder.insert = wmaplug_insert,
    .decoder.read_metadata = wmaplug_read_metadata,
    .decoder.exts = exts,
    .seek_sample64 = wmaplug_seek_sample64,
};

DB_plugin_t *
wma_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
