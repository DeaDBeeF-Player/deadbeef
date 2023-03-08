/*
    ALAC plugin for deadbeef
    Copyright (C) 2012-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>
    Uses the reverse engineered ALAC decoder (C) 2005 David Hammerton
    All rights reserved.

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
 */
#include <deadbeef/deadbeef.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <deadbeef/strdupa.h>

#include "decomp.h"

#include "../../src/shared/mp4tagutil.h"

#include <mp4p/mp4p.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#ifndef __linux__
#define off64_t off_t
#define lseek64 lseek
#define O_LARGEFILE 0
#endif

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static ddb_decoder2_t alac_plugin;
static ddb_decoder2_t alac_plugin;
DB_functions_t *deadbeef;

#ifdef WORDS_BIGENDIAN
int host_bigendian = 1;
#else
int host_bigendian = 0;
#endif

#define BUFFER_SIZE (1024*24)
#define IN_BUFFER_SIZE (1024*80)

typedef struct {
    DB_fileinfo_t info;
    DB_FILE *file;
    mp4p_file_callbacks_t mp4reader;
    mp4p_atom_t *mp4file;
    mp4p_atom_t *trak;
    uint32_t alac_samplerate;
    uint64_t mp4samples;
    alac_file *_alac;
    int mp4sample;
    int junk;
    uint8_t out_buffer[BUFFER_SIZE];
    int out_remaining;
    int64_t skipsamples;
    int64_t currentsample;
    int64_t startsample;
    int64_t endsample;
} alacplug_info_t;

static int
alacplug_seek (DB_fileinfo_t *_info, float t);

// allocate codec control structure
DB_fileinfo_t *
alacplug_open (uint32_t hints) {
    alacplug_info_t *info = calloc (1, sizeof (alacplug_info_t));
    return &info->info;
}

int
alacplug_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    alacplug_info_t *info = (alacplug_info_t *)_info;

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    info->file = deadbeef->fopen (uri);
    if (!info->file) {
        return -1;
    }

    uint64_t totalsamples = 0;

    int samplerate = 0;
    int channels = 0;
    int bps = 0;
    float duration = 0;

    info->mp4reader.ptrhandle = info->file;
    mp4_init_ddb_file_callbacks (&info->mp4reader);
    info->mp4file = mp4p_open(&info->mp4reader);

    // iterate over tracks, find the ALAC one
    info->trak = mp4p_atom_find (info->mp4file, "moov/trak");
    mp4p_alac_t *alac = NULL;
    while (info->trak) {
        mp4p_atom_t *alac_atom = mp4p_atom_find (info->trak, "trak/mdia/minf/stbl/stsd/alac");
        if (alac_atom && mp4p_trak_playable(info->trak)) {
            alac = alac_atom->data;
            info->alac_samplerate = alac->sample_rate;
            break;
        }
        info->trak = info->trak->next;
    }
    if (!alac) {
//        printf ("not found\n");
        return -1;
    }

    // get audio format: samplerate, bps, channels
    samplerate = alac->sample_rate;
    bps = alac->bps;
    channels = alac->channel_count;
    //printf ("%d %d %d\n", _samplerate, _bps, _channels);

    // get file info: totalsamples

    // 1. sum of all stts entries
    mp4p_atom_t *stts_atom = mp4p_atom_find(info->trak, "trak/mdia/minf/stbl/stts");

//    printf ("num_samples: %lld\n", num_mp4samples);

    // total PCM samples and duration

    // duration is calculated from stts table (time-to-sample), which is not pcm-sample-precise.
    // there should be a way to count pcm samples
    uint64_t total_sample_duration = mp4p_stts_total_sample_duration (stts_atom);
    totalsamples = total_sample_duration * samplerate / alac->sample_rate;
    duration = total_sample_duration / (float)alac->sample_rate;

    mp4p_atom_t *stsz_atom = mp4p_atom_find(info->trak, "trak/mdia/minf/stbl/stsz");
    mp4p_stsz_t *stsz = stsz_atom->data;

    info->mp4samples = stsz->number_of_entries;

    _info->fmt.samplerate = samplerate;
    _info->fmt.channels = channels;
    _info->fmt.bps = bps;

    info->_alac = create_alac (bps, channels);
    alac_set_info (info->_alac, (char *)alac->asc);

    if (!info->file->vfs->is_streaming ()) {
        int64_t endsample = deadbeef->pl_item_get_endsample(it);
        if (endsample > 0) {
            info->startsample = deadbeef->pl_item_get_startsample(it);
            info->endsample = endsample;
            alacplug_seek (_info, 0);
        }
        else {
            info->startsample = 0;
            info->endsample = (int)(totalsamples-1);
        }
    }

    _info->plugin = &alac_plugin.decoder;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }

    return 0;
}

void
alacplug_free (DB_fileinfo_t *_info) {
    alacplug_info_t *info = (alacplug_info_t *)_info;
    if (info) {
        if (info->file) {
            deadbeef->fclose (info->file);
        }
        if (info->mp4file) {
            mp4p_atom_free_list (info->mp4file);
        }

        if (info->_alac) {
            alac_file_free (info->_alac);
        }

        free (info);
    }
}

int
alacplug_read (DB_fileinfo_t *_info, char *bytes, int size) {
    alacplug_info_t *info = (alacplug_info_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (!info->file->vfs->is_streaming ()) {
        if (info->currentsample + size / samplesize > info->endsample) {
            size = (int)(info->endsample - info->currentsample + 1) * samplesize;
            if (size <= 0) {
                trace ("alacplug_read: eof (current=%d, total=%d)\n", info->currentsample, info->endsample);
                return 0;
            }
        }
    }
    int initsize = size;
    while (size > 0) {
        // handle seeking
        if (info->skipsamples > 0 && info->out_remaining > 0) {
            int64_t skip = min (info->out_remaining, info->skipsamples);
            if (skip < info->out_remaining) {
                memmove (info->out_buffer, info->out_buffer + skip * samplesize, (info->out_remaining - skip) * samplesize);
            }
            info->out_remaining -= skip;
            info->skipsamples -= skip;
        }
        if (info->out_remaining > 0) {
            int n = size / samplesize;
            n = min (info->out_remaining, n);

            uint8_t *src = info->out_buffer;
            memcpy (bytes, src, n * samplesize);
            bytes += n * samplesize;
            src += n * samplesize;
            size -= n * samplesize;

            if (n == info->out_remaining) {
                info->out_remaining = 0;
            }
            else {
                memmove (info->out_buffer, src, (info->out_remaining - n) * samplesize);
                info->out_remaining -= n;
            }
            continue;
        }

        unsigned char *buffer = NULL;
//        uint32_t buffer_size = 0;
        uint32_t outNumSamples = 0;

        if (info->mp4sample >= info->mp4samples) {
            trace ("alac: finished with the last mp4sample\n");
            break;
        }

        mp4p_atom_t *stbl_atom = mp4p_atom_find(info->trak, "trak/mdia/minf/stbl");
        uint64_t offs = mp4p_sample_offset (stbl_atom, info->mp4sample);
        uint64_t size = mp4p_sample_size (stbl_atom, info->mp4sample);

        buffer = malloc (size);
        deadbeef->fseek (info->file, offs+info->junk, SEEK_SET);
        if (size != deadbeef->fread (buffer, 1, size, info->file)) {
            trace ("alac: failed to read sample\n");
            break;
        }

        int outputBytes = 0;
        decode_frame(info->_alac, buffer, (int)size, info->out_buffer, &outputBytes);
        outNumSamples = outputBytes / samplesize;

        info->out_remaining += outNumSamples;
        info->mp4sample++;

        if (buffer) {
            free (buffer);
        }
    }

    info->currentsample += (initsize-size) / samplesize;
    return initsize-size;
}

static int
alacplug_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
    alacplug_info_t *info = (alacplug_info_t *)_info;

    sample += info->startsample;

    mp4p_atom_t *stts_atom = mp4p_atom_find(info->trak, "trak/mdia/minf/stbl/stts");

    uint64_t seeksample = (int)(sample * info->alac_samplerate / _info->fmt.samplerate);

    uint64_t startsample = 0;
    info->mp4sample = mp4p_stts_mp4sample_containing_sample(stts_atom, seeksample, &startsample);

    startsample = startsample * _info->fmt.samplerate / info->alac_samplerate;
    info->skipsamples = sample - startsample;

    info->out_remaining = 0;
    info->currentsample = sample;
    _info->readpos = (float)(info->currentsample - info->startsample) / _info->fmt.samplerate;

    return 0;
}

static int
alacplug_seek_sample(DB_fileinfo_t *_info, int sample) {
    return alacplug_seek_sample64(_info, sample);
}

static int
alacplug_seek (DB_fileinfo_t *_info, float t) {
    return alacplug_seek_sample64 (_info, (int64_t)((double)t * (int64_t)_info->fmt.samplerate));
}

DB_playItem_t *
alacplug_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("adding %s\n", fname);

    int samplerate = 0;
    int bps = 0;
    int channels = 0;
    int64_t totalsamples = 0;

    alacplug_info_t info;
    mp4p_atom_t *mp4 = NULL;

    memset (&info, 0, sizeof (info));

    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("not found\n");
        return NULL;
    }

    info.file = fp;
    info.junk = deadbeef->junk_get_leading_size (fp);
    if (info.junk >= 0) {
        trace ("junk: %d\n", info.junk);
        deadbeef->fseek (fp, info.junk, SEEK_SET);
    }
    else {
        info.junk = 0;
    }

    const char *ftype = NULL;
    float duration = -1;

    info.mp4reader.ptrhandle = fp;
    mp4_init_ddb_file_callbacks (&info.mp4reader);
    mp4 = info.mp4file = mp4p_open(&info.mp4reader);

    if (!info.mp4file) {
        deadbeef->fclose (fp);
        return NULL;
    }
    // iterate over tracks, and the ALAC one
    info.trak = mp4p_atom_find (info.mp4file, "moov/trak");
    mp4p_alac_t *alac = NULL;
    while (info.trak) {
        mp4p_atom_t *alac_atom = mp4p_atom_find (info.trak, "trak/mdia/minf/stbl/stsd/alac");
        if (alac_atom && mp4p_trak_playable(info.trak)) {
            alac = alac_atom->data;
            if (alac->sample_rate != 0) {
                info.alac_samplerate = alac->sample_rate;
                break;
            }
            else {
                alac = NULL;
            }
        }
        info.trak = info.trak->next;
    }
    if (!alac) {
        deadbeef->fclose (fp);
        mp4p_atom_free_list(info.mp4file);
        return NULL;
    }

    // get audio format: samplerate, bps, channels
    samplerate = alac->sample_rate;
    bps = alac->bps;
    channels = alac->channel_count;

    mp4p_atom_t *stts_atom = mp4p_atom_find(info.trak, "trak/mdia/minf/stbl/stts");

    uint64_t total_sample_duration = mp4p_stts_total_sample_duration (stts_atom);
    totalsamples = total_sample_duration * samplerate / alac->sample_rate;
    duration = total_sample_duration / (float)alac->sample_rate;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, alac_plugin.decoder.plugin.id);
    ftype = "ALAC";
    deadbeef->pl_add_meta (it, ":FILETYPE", ftype);
    deadbeef->plt_set_item_duration (plt, it, duration);

    deadbeef->rewind (fp);

    (void)deadbeef->junk_apev2_read (it, fp);
    (void)deadbeef->junk_id3v2_read (it, fp);
    (void)deadbeef->junk_id3v1_read (it, fp);

    int64_t fsize = deadbeef->fgetlength (fp);
    deadbeef->fclose (fp);

    char s[100];
    snprintf (s, sizeof (s), "%lld", fsize);
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
    deadbeef->pl_add_meta (it, ":BPS", "16");
    snprintf (s, sizeof (s), "%d", channels);
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", samplerate);
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    int br = (int)roundf(fsize / duration * 8 / 1000);
    snprintf (s, sizeof (s), "%d", br);
    deadbeef->pl_add_meta (it, ":BITRATE", s);

    // embedded cue
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    DB_playItem_t *cue = NULL;

    if (cuesheet) {
        cue = deadbeef->plt_insert_cue_from_buffer (plt, after, it, (const uint8_t *)cuesheet, (int)strlen (cuesheet), (int)totalsamples, samplerate);
        if (cue) {
            mp4p_atom_free_list(info.mp4file);
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            return cue;
        }
    }

    cue  = deadbeef->plt_insert_cue (plt, after, it, (int)totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue);
        return cue;
    }

    mp4_load_tags (info.mp4file, it);
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);

    mp4p_atom_free_list(info.mp4file);
    return after;
}

static const char * exts[] = { "mp4", "m4a", NULL };

// define plugin interface
static ddb_decoder2_t alac_plugin = {
    .decoder.plugin.api_vmajor = 1,
    .decoder.plugin.api_vminor = 0,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 0,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "alac",
    .decoder.plugin.name = "ALAC player",
    .decoder.plugin.descr = "plays alac files from MP4 and M4A files",
    .decoder.plugin.copyright =
        "ALAC plugin for deadbeef\n"
        "Copyright (C) 2012-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "Uses the reverse engineered ALAC decoder (C) 2005 David Hammerton\n"
        "All rights reserved.\n"
        "\n"
        "Permission is hereby granted, free of charge, to any person\n"
        "obtaining a copy of this software and associated documentation\n"
        "files (the \"Software\"), to deal in the Software without\n"
        "restriction, including without limitation the rights to use,\n"
        "copy, modify, merge, publish, distribute, sublicense, and/or\n"
        "sell copies of the Software, and to permit persons to whom the\n"
        "Software is furnished to do so, subject to the following conditions:\n"
        "\n"
        "The above copyright notice and this permission notice shall be\n"
        "included in all copies or substantial portions of the Software.\n"
        "\n"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
        "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES\n"
        "OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
        "NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT\n"
        "HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
        "WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\n"
        "FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
        "OTHER DEALINGS IN THE SOFTWARE.\n"
    ,
    .decoder.plugin.website = "http://deadbeef.sf.net",
    .decoder.open = alacplug_open,
    .decoder.init = alacplug_init,
    .decoder.free = alacplug_free,
    .decoder.read = alacplug_read,
    .decoder.seek = alacplug_seek,
    .decoder.seek_sample = alacplug_seek_sample,
    .decoder.insert = alacplug_insert,
    .decoder.read_metadata = mp4_read_metadata,
    .decoder.write_metadata = mp4_write_metadata,
    .decoder.exts = exts,
    .seek_sample64 = alacplug_seek_sample64,
};

DB_plugin_t *
alac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&alac_plugin);
}
