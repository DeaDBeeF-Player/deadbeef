/*
    libsndfile plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#ifndef __linux__
#define _LARGEFILE64_SOURCE
#endif
#include <string.h>
#include <sndfile.h>
#include <math.h>
#include <stdlib.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { deadbeef->log_detailed (&plugin.decoder.plugin, 0, __VA_ARGS__); }

static ddb_decoder2_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    SNDFILE *ctx;
    DB_FILE *file;
    int64_t startsample;
    int64_t endsample;
    int64_t currentsample;
    int bitrate;
    int sf_format;
    int read_as_short;
    int sf_need_endswap;
} sndfile_info_t;

static int
sndfile_seek_sample64 (DB_fileinfo_t *_info, int64_t sample);

// vfs wrapper for sf
static sf_count_t
sf_vfs_get_filelen (void *user_data) {
    sndfile_info_t *ctx = user_data;
    return deadbeef->fgetlength (ctx->file);
}

static sf_count_t
sf_vfs_read (void *ptr, sf_count_t count, void *user_data) {
    sndfile_info_t *ctx = user_data;
    return deadbeef->fread (ptr, 1, count, ctx->file);
}

static sf_count_t
sf_vfs_write (const void *ptr, sf_count_t count, void *user_data) {
    return -1;
}

static sf_count_t
sf_vfs_seek (sf_count_t offset, int whence, void *user_data) {
    sndfile_info_t *ctx = user_data;
    int ret = deadbeef->fseek (ctx->file, offset, whence);
    if (!ret) {
        return offset;
    }
    return -1;
}

static sf_count_t
sf_vfs_tell (void *user_data) {
    sndfile_info_t *ctx = user_data;
    return deadbeef->ftell (ctx->file);
}

static SF_VIRTUAL_IO vfs = {
    .get_filelen = sf_vfs_get_filelen,
    .seek = sf_vfs_seek,
    .read = sf_vfs_read,
    .write = sf_vfs_write,
    .tell = sf_vfs_tell
};

static DB_fileinfo_t *
sndfile_open (uint32_t hints) {
    sndfile_info_t *info = calloc (1, sizeof (sndfile_info_t));
    return &info->info;
}


#if 0
// taken from libsndfile
#define     ARRAY_LEN(x)    ((int) (sizeof (x) / sizeof ((x) [0])))
/* This stores which bit in dwChannelMask maps to which channel */
static const struct chanmap_s
{	int id ;
	const char * name ;
} channel_mask_bits [] =
{	/* WAVEFORMATEXTENSIBLE doesn't distuingish FRONT_LEFT from LEFT */
	{	SF_CHANNEL_MAP_LEFT, "L" },
	{	SF_CHANNEL_MAP_RIGHT, "R" },
	{	SF_CHANNEL_MAP_CENTER, "C" },
	{	SF_CHANNEL_MAP_LFE, "LFE" },
	{	SF_CHANNEL_MAP_REAR_LEFT, "Ls" },
	{	SF_CHANNEL_MAP_REAR_RIGHT, "Rs" },
	{	SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER, "Lc" },
	{	SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER, "Rc" },
	{	SF_CHANNEL_MAP_REAR_CENTER, "Cs" },
	{	SF_CHANNEL_MAP_SIDE_LEFT, "Sl" },
	{	SF_CHANNEL_MAP_SIDE_RIGHT, "Sr" },
	{	SF_CHANNEL_MAP_TOP_CENTER, "Tc" },
	{	SF_CHANNEL_MAP_TOP_FRONT_LEFT, "Tfl" },
	{	SF_CHANNEL_MAP_TOP_FRONT_CENTER, "Tfc" },
	{	SF_CHANNEL_MAP_TOP_FRONT_RIGHT, "Tfr" },
	{	SF_CHANNEL_MAP_TOP_REAR_LEFT, "Trl" },
	{	SF_CHANNEL_MAP_TOP_REAR_CENTER, "Trc" },
	{	SF_CHANNEL_MAP_TOP_REAR_RIGHT, "Trr" },
} ;


static int
wavex_gen_channel_mask (const int *chan_map, int channels)
{   int chan, mask = 0, bit = -1, last_bit = -1 ;

    if (chan_map == NULL)
        return 0 ;

    for (chan = 0 ; chan < channels ; chan ++)
    {   int k ;

        for (k = bit + 1 ; k < ARRAY_LEN (channel_mask_bits) ; k++)
            if (chan_map [chan] == channel_mask_bits [k].id)
            {   bit = k ;
                break ;
                } ;

        /* Check for bad sequence. */
        if (bit <= last_bit)
            return 0 ;

        mask += 1 << bit ;
        last_bit = bit ;
        } ;

    return mask ;
} /* wavex_gen_channel_mask */
#endif

static int
sndfile_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    sndfile_info_t *info = (sndfile_info_t*)_info;

    SF_INFO inf = {0};
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    DB_FILE *fp = deadbeef->fopen (uri);
    if (!fp) {
        trace ("sndfile: failed to open %s\n", uri);
        return -1;
    }
    int64_t fsize = deadbeef->fgetlength (fp);

    info->file = fp;

    info->ctx = sf_open_virtual (&vfs, SFM_READ, &inf, info);
    if (!info->ctx) {
        trace ("sndfile: %s: unsupported file format\n");
        deadbeef->fclose (fp);
        info->file = NULL;
        return -1;
    }
    _info->plugin = &plugin.decoder;
    info->sf_format = inf.format&SF_FORMAT_SUBMASK;
    info->sf_need_endswap = sf_command (info->ctx, SFC_RAW_DATA_NEEDS_ENDSWAP, NULL, 0);

    switch (inf.format&SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_U8:
        _info->fmt.bps = 8;
        break;
    case SF_FORMAT_PCM_16:
        _info->fmt.bps = 16;
        break;
    case SF_FORMAT_PCM_24:
        _info->fmt.bps = 24;
        break;
    case SF_FORMAT_FLOAT:
        _info->fmt.is_float = 1;
    case SF_FORMAT_PCM_32:
        _info->fmt.bps = 32;
        break;
    default:
        info->read_as_short = 1;
        _info->fmt.bps = 16;
        trace ("[sndfile] unidentified input format: 0x%X\n", inf.format&SF_FORMAT_SUBMASK);
        break;
    }

    _info->fmt.channels = inf.channels;
    _info->fmt.samplerate = inf.samplerate;

// FIXME: streamer and maybe output plugins need to be fixed to support
// arbitrary channelmask
//
//    int channel_map [inf.channels];
//    int cmdres = sf_command (info->ctx, SFC_GET_CHANNEL_MAP_INFO, channel_map, sizeof (channel_map)) ;
//    if (cmdres != SF_FALSE) {
//        // channel map found, convert to channel mask
//        _info->fmt.channelmask = wavex_gen_channel_mask (channel_map, inf.channels);
//    }
//    else
    {
        // channel map not found, generate from channel number
        for (int i = 0; i < inf.channels; i++) {
            _info->fmt.channelmask |= 1 << i;
        }
    }

    _info->readpos = 0;
    int64_t endsample = deadbeef->pl_item_get_endsample (it);
    if (endsample > 0) {
        info->startsample = deadbeef->pl_item_get_startsample (it);
        info->endsample = endsample;
        if (sndfile_seek_sample64 (_info, 0) < 0) {
            return -1;
        }
    }
    else {
        info->startsample = 0;
        info->endsample = inf.frames-1;
    }
    // hack bitrate

    int64_t totalsamples = inf.frames;
    float sec = (float)totalsamples / inf.samplerate;
    if (sec > 0) {
        info->bitrate = fsize / sec * 8 / 1000;
    }
    else {
        info->bitrate = -1;
    }

    return 0;
}

static void
sndfile_free (DB_fileinfo_t *_info) {
    if (!_info)
        return ;
    sndfile_info_t *info = (sndfile_info_t*)_info;
    if (info->ctx) {
        sf_close (info->ctx);
    }
    if (info->file) {
        deadbeef->fclose (info->file);
    }
    free (_info);
}

static int
sndfile_read (DB_fileinfo_t *_info, char *bytes, int size) {
    sndfile_info_t *info = (sndfile_info_t*)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (size / samplesize + info->currentsample > info->endsample) {
        size = (int)((info->endsample - info->currentsample + 1) * samplesize);
        trace ("sndfile: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, info->currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }

    int64_t n = 0;
    if (info->read_as_short) {
        n = sf_readf_short(info->ctx, (short *)bytes, size/samplesize);
    }
    else {
        n = sf_read_raw (info->ctx, (short *)bytes, size);

        if (info->sf_format == SF_FORMAT_PCM_U8) {
            for (int i = 0; i < n; i++) {
                int sample = ((uint8_t *)bytes)[i];
                ((int8_t *)bytes)[i] = sample-0x80;
            }
        }
        else if (info->sf_need_endswap) {
            switch (info->info.fmt.bps) {
            case 16:
                {
                    uint16_t *data = (uint16_t *)bytes;
                    for (int i = 0; i < n/2; i++, data++) {
                        *data = ((*data & 0xff) << 8) | ((*data & 0xff00) >> 8);
                    }
                }
                break;
            case 24:
                {
                    uint8_t *data = (uint8_t *)bytes;
                    for (int i = 0; i < n/3; i++, data += 3) {
                        uint8_t temp = data[0];
                        data[0] = data[2];
                        data[2] = temp;
                    }
                }
                break;
            case 32:
                {
                    uint32_t *data = (uint32_t *)bytes;
                    for (int i = 0; i < n/4; i++, data++) {
                        *data = ((*data & 0xff) << 24) | ((*data & 0xff00) << 8) | ((*data & 0xff0000) >> 8) | ((*data & 0xff0000) >> 24);
                    }
                }
                break;
            }
        }
        n /= samplesize;
    }

    info->currentsample += n;

    size = (int)(n * samplesize);
    _info->readpos = (float)(info->currentsample-info->startsample)/_info->fmt.samplerate;
    if (info->bitrate > 0) {
        deadbeef->streamer_set_bitrate (info->bitrate);
    }
    return size;
}

static int
sndfile_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
    sndfile_info_t *info = (sndfile_info_t*)_info;
    int64_t ret = sf_seek (info->ctx, sample + info->startsample, SEEK_SET);
    if (ret < 0) {
        return -1;
    }
    info->currentsample = ret;
    _info->readpos = (float)(info->currentsample - info->startsample) / _info->fmt.samplerate;
    return 0;
}

static int
sndfile_seek_sample (DB_fileinfo_t *_info, int sample) {
    return sndfile_seek_sample64(_info, sample);
}

static int
sndfile_seek (DB_fileinfo_t *_info, float sec) {
    return sndfile_seek_sample64 (_info, (int64_t)((double)sec * (int64_t)_info->fmt.samplerate));
}

static int
_sndfile_ctx_read_tags (DB_playItem_t *it, SNDFILE *ctx);

static DB_playItem_t *
sndfile_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("adding file %s\n", fname);
    SF_INFO inf = {0};
    sndfile_info_t info = {0};
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("sndfile: failed to open %s\n", fname);
        return NULL;
    }

    info.file = fp;

    int64_t fsize = deadbeef->fgetlength(fp);
    trace ("file: %p, size: %lld\n", fp, fsize);

    trace ("calling sf_open_virtual\n");
    info.ctx = sf_open_virtual (&vfs, SFM_READ, &inf, &info);
    if (!info.ctx) {
        trace ("sndfile: sf_open failed for %s\n", fname);
        deadbeef->fclose (fp);
        return NULL;
    }

    if (inf.samplerate == 0) {
        trace ("sndfile: invalid samplerate 0 in file %s\n", fname);
        deadbeef->fclose (fp);
        return NULL;
    }

    trace ("calling sf_open_virtual ok\n");
    int64_t totalsamples = inf.frames;
    int samplerate = inf.samplerate;

    float duration = (float)totalsamples / samplerate;
    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", "wav");
    deadbeef->plt_set_item_duration (plt, it, duration);

    trace ("sndfile: totalsamples=%d, samplerate=%d, duration=%f\n", totalsamples, samplerate, duration);

    char s[100];
    snprintf (s, sizeof (s), "%lld", (long long)fsize);
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);

    int bps = -1;
    switch (inf.format&SF_FORMAT_SUBMASK) {
    case SF_FORMAT_IMA_ADPCM:
    case SF_FORMAT_MS_ADPCM:
        bps = 4;
        break;
    case SF_FORMAT_ALAW:
    case SF_FORMAT_ULAW:
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_U8:
        bps = 8;
        break;
    case SF_FORMAT_PCM_16:
        bps = 16;
        break;
    case SF_FORMAT_PCM_24:
        bps = 24;
        break;
    case SF_FORMAT_FLOAT:
    case SF_FORMAT_PCM_32:
        bps = 32;
        break;
    }

    if (bps == -1) {
        snprintf (s, sizeof (s), "unknown");
    }
    else {
        snprintf (s, sizeof (s), "%d", bps);
    }
    deadbeef->pl_add_meta (it, ":BPS", s);
    snprintf (s, sizeof (s), "%d", inf.channels);
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", samplerate);
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    if (duration > 0) {
        int br = (int)roundf(fsize / duration * 8 / 1000);
        snprintf (s, sizeof (s), "%d", br);
        deadbeef->pl_add_meta (it, ":BITRATE", s);
    }

    // sndfile subformats
    int subformat_ids[] = {
        SF_FORMAT_PCM_S8,
        SF_FORMAT_PCM_16,
        SF_FORMAT_PCM_24,
        SF_FORMAT_PCM_32,
        SF_FORMAT_PCM_U8,
        SF_FORMAT_FLOAT,
        SF_FORMAT_DOUBLE,
        SF_FORMAT_ULAW,
        SF_FORMAT_ALAW,
        SF_FORMAT_IMA_ADPCM,
        SF_FORMAT_MS_ADPCM,
        SF_FORMAT_GSM610,
        SF_FORMAT_VOX_ADPCM,
        SF_FORMAT_G721_32,
        SF_FORMAT_G723_24,
        SF_FORMAT_G723_40,
        SF_FORMAT_DWVW_12,
        SF_FORMAT_DWVW_16,
        SF_FORMAT_DWVW_24,
        SF_FORMAT_DWVW_N,
        SF_FORMAT_DPCM_8,
        SF_FORMAT_DPCM_16,
        SF_FORMAT_VORBIS,
        SF_FORMAT_ALAC_16,
        SF_FORMAT_ALAC_20,
        SF_FORMAT_ALAC_24,
        SF_FORMAT_ALAC_32,
        -1
    };

    const char *subformats[] = {
        "PCM_S8",
        "PCM_16",
        "PCM_24",
        "PCM_32",
        "PCM_U8",
        "FLOAT",
        "DOUBLE",
        "ULAW",
        "ALAW",
        "IMA_ADPCM",
        "MS_ADPCM",
        "GSM610",
        "VOX_ADPCM",
        "G721_32",
        "G723_24",
        "G723_40",
        "DWVW_12",
        "DWVW_16",
        "DWVW_24",
        "DWVW_N",
        "DPCM_8",
        "DPCM_16",
        "VORBIS",
        "ALAC_16",
        "ALAC_20",
        "ALAC_24",
        "ALAC_32",
    };

    for (int i = 0; subformat_ids[i] != -1; i++) {
        if ((inf.format&SF_FORMAT_SUBMASK) == subformat_ids[i]) {
            deadbeef->pl_add_meta (it, ":SF_FORMAT", subformats[i]);
            break;
        }
    }

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it, totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        sf_close (info.ctx);
        deadbeef->fclose (fp);
        return cue;
    }

    deadbeef->pl_add_meta (it, "title", NULL);

    _sndfile_ctx_read_tags (it, info.ctx);
    sf_close (info.ctx);
    deadbeef->fclose (fp);

    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);

    return after;
}

#define     ARRAY_LEN(x)    ((int) (sizeof (x) / sizeof ((x) [0])))

static const struct metamap_s {
    int str_type;
    const char* meta_name;
} metamap_items [] = {
    { SF_STR_TITLE,         "title" },
    { SF_STR_COPYRIGHT,     "copyright" } ,
    { SF_STR_SOFTWARE,      "encoder" },
    { SF_STR_ARTIST,        "artist" },
    { SF_STR_COMMENT,       "comment" },
    { SF_STR_DATE,          "year" },
    { SF_STR_ALBUM,         "album" },
    { SF_STR_TRACKNUMBER,   "track" },
    { SF_STR_GENRE,         "genre" },
};

static int
sndfile_read_metadata (DB_playItem_t *it) {
    DB_fileinfo_t* info = sndfile_open (0);
    int res = sndfile_init (info, it);
    if (res) {
        sndfile_free (info);
        return -1;
    }
    SNDFILE *ctx = ((sndfile_info_t*)info)->ctx;
    _sndfile_ctx_read_tags (it, ctx);

    sndfile_free (info);
    return 0;
}

static int
_sndfile_ctx_read_tags (DB_playItem_t *it, SNDFILE *ctx)
{
    int i;
    for (i = 0; i < ARRAY_LEN(metamap_items); i++) {
        const struct metamap_s *kv = &metamap_items[i];
        const char *meta_str = sf_get_string (ctx, kv->str_type);
        if (!meta_str)
            continue ;
        const char *charset = deadbeef->junk_detect_charset (meta_str);
        if (charset) {
            // recode non-UTF-8 metadata string.
            size_t l = strlen (meta_str);
            char *recbuf = malloc (l * 4 + 1);
            int res = deadbeef->junk_recode (meta_str, (int)l, recbuf, (int)(l * 4 + 1), charset);
            if (res >= 0) {
                deadbeef->pl_add_meta (it, kv->meta_name, recbuf);
            }
            free (recbuf);
        }
        else {
            deadbeef->pl_add_meta (it, kv->meta_name, meta_str);
        }
    }
    return 0;
}

#define DEFAULT_EXTS "wav;aif;aiff;snd;au;paf;svx;nist;voc;ircam;w64;mat4;mat5;pvf;xi;htk;sds;avr;wavex;sd2;caf;wve"

#define EXT_MAX 100

static char *exts[EXT_MAX] = {NULL};

static void
sndfile_init_exts (void) {
    for (int i = 0; exts[i]; i++) {
        free (exts[i]);
    }
    exts[0] = NULL;

    int n = 0;
    deadbeef->conf_lock ();
    const char *new_exts = deadbeef->conf_get_str_fast ("sndfile.extensions", DEFAULT_EXTS);
    while (*new_exts) {
        if (n >= EXT_MAX) {
            fprintf (stderr, "sndfile: too many extensions, max is %d\n", EXT_MAX);
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
    deadbeef->conf_unlock ();
    exts[n] = NULL;
}

static int
sndfile_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        sndfile_init_exts ();
        if (deadbeef->conf_get_int ("sndfile.trace", 0)) {
            plugin.decoder.plugin.flags |= DDB_PLUGIN_FLAG_LOGGING;
        }
        else {
            plugin.decoder.plugin.flags &= ~DDB_PLUGIN_FLAG_LOGGING;
        }
        break;
    }
    return 0;
}

static int
sndfile_start (void) {
    sndfile_init_exts ();
    return 0;
}

static int
sndfile_stop (void) {
    for (int i = 0; exts[i]; i++) {
        free (exts[i]);
    }
    exts[0] = NULL;
    return 0;
}

static const char settings_dlg[] =
    "property \"File Extensions (separate with ';')\" entry sndfile.extensions \"" DEFAULT_EXTS "\";\n"
    "property \"Enable logging\" checkbox sndfile.trace 0;\n"
;

// define plugin interface
static ddb_decoder2_t plugin = {
    .decoder.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .decoder.plugin.api_vminor = DB_API_VERSION_MINOR,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 0,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "sndfile",
    .decoder.plugin.name = "WAV/PCM player",
    .decoder.plugin.descr = "wav/aiff player using libsndfile",
    .decoder.plugin.copyright =
        "libsndfile plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
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
    .decoder.open = sndfile_open,
    .decoder.init = sndfile_init,
    .decoder.free = sndfile_free,
    .decoder.read = sndfile_read,
    .decoder.seek = sndfile_seek,
    .decoder.seek_sample = sndfile_seek_sample,
    .decoder.insert = sndfile_insert,
    .decoder.read_metadata = sndfile_read_metadata,
    .decoder.exts = (const char **)exts,
    .decoder.plugin.start = sndfile_start,
    .decoder.plugin.stop = sndfile_stop,
    .decoder.plugin.configdialog = settings_dlg,
    .decoder.plugin.message = sndfile_message,
    .seek_sample64 = sndfile_seek_sample64,
};

DB_plugin_t *
sndfile_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
