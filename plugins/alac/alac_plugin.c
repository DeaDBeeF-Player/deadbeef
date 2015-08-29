/*
    ALAC plugin for deadbeef
    Copyright (C) 2012-2013 Alexey Yakovenko <waker@users.sourceforge.net>
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
#include "../../deadbeef.h"
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mp4ff.h"
#include "demux.h"
#include "decomp.h"
#include "stream.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
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
    demux_res_t demux_res;
    stream_t *stream;
    alac_file *alac;
    int junk;
    char out_buffer[BUFFER_SIZE];
    int out_remaining;
    int skipsamples;
    int currentsample;
    int startsample;
    int endsample;
    int current_frame;
    int64_t dataoffs;
} alacplug_info_t;

// allocate codec control structure
static DB_fileinfo_t *
alacplug_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (alacplug_info_t));
    alacplug_info_t *info = (alacplug_info_t *)_info;
    memset (info, 0, sizeof (alacplug_info_t));
    return _info;
}

static uint32_t
alacplug_fs_read (void *user_data, void *buffer, uint32_t length) {
    alacplug_info_t *info = user_data;
    return deadbeef->fread (buffer, 1, length, info->file);
}

static uint32_t
alacplug_fs_seek (void *user_data, uint64_t position) {
    alacplug_info_t *info = user_data;
    return deadbeef->fseek (info->file, position+info->junk, SEEK_SET);
}

static int
get_sample_info(demux_res_t *demux_res, uint32_t samplenum,
                           uint32_t *sample_duration,
                           uint32_t *sample_byte_size)
{
    unsigned int duration_index_accum = 0;
    unsigned int duration_cur_index = 0;

    if (samplenum >= demux_res->num_sample_byte_sizes)
    {
        fprintf(stderr, "sample %i does not exist\n", samplenum);
        return 0;
    }

    if (!demux_res->num_time_to_samples)
    {
        fprintf(stderr, "no time to samples\n");
        return 0;
    }
    while ((demux_res->time_to_sample[duration_cur_index].sample_count + duration_index_accum)
            <= samplenum)
    {
        duration_index_accum += demux_res->time_to_sample[duration_cur_index].sample_count;
        duration_cur_index++;
        if (duration_cur_index >= demux_res->num_time_to_samples)
        {
            fprintf(stderr, "sample %i does not have a duration\n", samplenum);
            return 0;
        }
    }

    *sample_duration = demux_res->time_to_sample[duration_cur_index].sample_duration;
    *sample_byte_size = demux_res->sample_byte_size[samplenum];

    return 1;
}

static int
alacplug_get_totalsamples (demux_res_t *demux_res) {
    int totalsamples = 0;
    for (int i = 0; i < demux_res->num_sample_byte_sizes; i++)
    {
        unsigned int thissample_duration = 0;
        unsigned int thissample_bytesize = 0;

        get_sample_info(demux_res, i, &thissample_duration,
                &thissample_bytesize);

        totalsamples += thissample_duration;
    }
    return totalsamples;
}

static int
alacplug_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    alacplug_info_t *info = (alacplug_info_t *)_info;

    deadbeef->pl_lock ();
    info->file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    if (!info->file) {
        return -1;
    }

    info->stream = stream_create_file (info->file, 1, info->junk);

    if (!qtmovie_read(info->stream, &info->demux_res)) {
        if (!info->demux_res.format_read || info->demux_res.format != MAKEFOURCC('a','l','a','c')) {
            return -1;
        }
    }
    info->dataoffs = deadbeef->ftell (info->file);

    info->alac = create_alac(info->demux_res.sample_size, info->demux_res.num_channels);
    alac_set_info(info->alac, info->demux_res.codecdata);
    info->demux_res.sample_rate = alac_get_samplerate (info->alac);
    info->demux_res.sample_size = alac_get_bitspersample (info->alac);

    int totalsamples = alacplug_get_totalsamples (&info->demux_res);
    if (!info->file->vfs->is_streaming ()) {
        if (it->endsample > 0) {
            info->startsample = it->startsample;
            info->endsample = it->endsample;
            plugin.seek_sample (_info, 0);
        }
        else {
            info->startsample = 0;
            info->endsample = totalsamples-1;
        }
    }

    _info->plugin = &plugin;
    _info->fmt.bps = info->demux_res.sample_size;
    _info->fmt.channels = info->demux_res.num_channels;
    _info->fmt.samplerate = info->demux_res.sample_rate;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }

    return 0;
}

static void
alacplug_free (DB_fileinfo_t *_info) {
    alacplug_info_t *info = (alacplug_info_t *)_info;
    if (info) {
        if (info->file) {
            deadbeef->fclose (info->file);
        }
        if (info->stream) {
            stream_destroy (info->stream);
        }
        qtmovie_free_demux (&info->demux_res);
        if (info->alac) {
            alac_file_free (info->alac);
        }
        free (info);
    }
}

static int
alacplug_read (DB_fileinfo_t *_info, char *bytes, int size) {
    alacplug_info_t *info = (alacplug_info_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (!info->file->vfs->is_streaming ()) {
        if (info->currentsample + size / samplesize > info->endsample) {
            size = (info->endsample - info->currentsample + 1) * samplesize;
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
            int skip = min (info->out_remaining, info->skipsamples);
            if (skip < info->out_remaining) {
                memmove (info->out_buffer, info->out_buffer + skip * samplesize, (info->out_remaining - skip) * samplesize);
            }
            info->out_remaining -= skip;
            info->skipsamples -= skip;
        }
        if (info->out_remaining > 0) {
            int n = size / samplesize;
            n = min (info->out_remaining, n);

            char *src = info->out_buffer;
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

        // decode next frame
        if (info->current_frame == info->demux_res.num_sample_byte_sizes) {
            break; // end of file
        }
        
        uint32_t sample_duration;
        uint32_t sample_byte_size;

        int outputBytes;

        /* just get one sample for now */
        if (!get_sample_info(&info->demux_res, info->current_frame,
                             &sample_duration, &sample_byte_size))
        {
            fprintf(stderr, "alac: sample failed\n");
            break;
        }

        if (IN_BUFFER_SIZE < sample_byte_size)
        {
            fprintf(stderr, "alac: buffer too small! (is %i want %i)\n",
                    IN_BUFFER_SIZE,
                    sample_byte_size);
            break;
        }

        char buffer[IN_BUFFER_SIZE];
        stream_read(info->stream, sample_byte_size, buffer);

        outputBytes = BUFFER_SIZE;
        decode_frame(info->alac, buffer, info->out_buffer, &outputBytes);
        info->current_frame++;

        info->out_remaining += outputBytes / samplesize;
    }

    info->currentsample += (initsize-size) / samplesize;
    return initsize-size;
}

static int
alacplug_seek_sample (DB_fileinfo_t *_info, int sample) {
    alacplug_info_t *info = (alacplug_info_t *)_info;

    sample += info->startsample;

    int totalsamples = 0;
    int64_t seekpos = 0;
    int i;
    for (i = 0; i < info->demux_res.num_sample_byte_sizes; i++)
    {
        unsigned int thissample_duration = 0;
        unsigned int thissample_bytesize = 0;

        get_sample_info(&info->demux_res, i, &thissample_duration,
                &thissample_bytesize);

        if (totalsamples + thissample_duration > sample) {
            info->skipsamples = sample - totalsamples;
            break;
        }
        totalsamples += thissample_duration;
        seekpos += info->demux_res.sample_byte_size[i];
    }

    if (i == info->demux_res.num_sample_byte_sizes) {
        return -1;
    }


    deadbeef->fseek(info->file, info->dataoffs + seekpos, SEEK_SET);

    info->current_frame = i;
    info->out_remaining = 0;
    info->currentsample = sample;
    _info->readpos = (float)(info->currentsample - info->startsample) / _info->fmt.samplerate;
    return 0;
}

static int
alacplug_seek (DB_fileinfo_t *_info, float t) {
    return alacplug_seek_sample (_info, t * _info->fmt.samplerate);
}

static const char *metainfo[] = {
    "artist", "artist",
    "title", "title",
    "album", "album",
    "track", "track",
    "date", "year",
    "genre", "genre",
    "comment", "comment",
    "performer", "performer",
    "album_artist", "band",
    "writer", "composer",
    "vendor", "vendor",
    "disc", "disc",
    "compilation", "compilation",
    "totaldiscs", "numdiscs",
    "copyright", "copyright",
    "totaltracks", "numtracks",
    "tool", "tool",
    "MusicBrainz Track Id", "musicbrainz_trackid",
    NULL
};


/* find a metadata item by name */
/* returns 0 if item found, 1 if no such item */
int32_t mp4ff_meta_find_by_name(const mp4ff_t *f, const char *item, char **value);


void
alacplug_load_tags (DB_playItem_t *it, mp4ff_t *mp4) {
    char *s = NULL;
    int got_itunes_tags = 0;

    int n = mp4ff_meta_get_num_items (mp4);
    for (int t = 0; t < n; t++)  {
        char *key = NULL;
        char *value = NULL;
        int res = mp4ff_meta_get_by_index(mp4, t, &key, &value);
        if (key && value) {
            got_itunes_tags = 1;
            if (strcasecmp (key, "cover")) {
                if (!strcasecmp (key, "replaygain_track_gain")) {
                    deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, atof (value));
                }
                else if (!strcasecmp (key, "replaygain_album_gain")) {
                    deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, atof (value));
                }
                else if (!strcasecmp (key, "replaygain_track_peak")) {
                    deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK, atof (value));
                }
                else if (!strcasecmp (key, "replaygain_album_peak")) {
                    deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK, atof (value));
                }
                else {
                    int i;
                    for (i = 0; metainfo[i]; i += 2) {
                        if (!strcasecmp (metainfo[i], key)) {
                            deadbeef->pl_add_meta (it, metainfo[i+1], value);
                            break;
                        }
                    }
                    if (!metainfo[i]) {
                        deadbeef->pl_add_meta (it, key, value);
                    }
                }
            }
        }
        if (key) {
            free (key);
        }
        if (value) {
            free (value);
        }
    }

    if (got_itunes_tags) {
        uint32_t f = deadbeef->pl_get_item_flags (it);
        f |= DDB_TAG_ITUNES;
        deadbeef->pl_set_item_flags (it, f);
    }
}

int
alacplug_read_metadata (DB_playItem_t *it) {
    deadbeef->pl_lock ();
    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    if (!fp) {
        return -1;
    }

    if (fp->vfs->is_streaming ()) {
        deadbeef->fclose (fp);
        return -1;
    }

    alacplug_info_t inf;
    memset (&inf, 0, sizeof (inf));
    inf.file = fp;
    inf.junk = deadbeef->junk_get_leading_size (fp);
    if (inf.junk >= 0) {
        deadbeef->fseek (inf.file, inf.junk, SEEK_SET);
    }
    else {
        inf.junk = 0;
    }

    mp4ff_callback_t cb = {
        .read = alacplug_fs_read,
        .write = NULL,
        .seek = alacplug_fs_seek,
        .truncate = NULL,
        .user_data = &inf
    };

    deadbeef->pl_delete_all_meta (it);

    mp4ff_t *mp4 = mp4ff_open_read (&cb);
    if (mp4) {
        alacplug_load_tags (it, mp4);
        mp4ff_close (mp4);
    }
    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
    /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
    deadbeef->fclose (fp);
    return 0;
}

static DB_playItem_t *
alacplug_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("adding %s\n", fname);
    mp4ff_t *mp4 = NULL;
    DB_playItem_t *it = NULL;
    demux_res_t demux_res;
    memset (&demux_res, 0, sizeof (demux_res));
    stream_t *stream;
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("not found\n");
        return NULL;
    }
    alacplug_info_t info = {0};
    info.file = fp;
    info.junk = deadbeef->junk_get_leading_size (fp);
    if (info.junk >= 0) {
        trace ("junk: %d\n", info.junk);
        deadbeef->fseek (fp, info.junk, SEEK_SET);
    }
    else {
        info.junk = 0;
    }

    float duration = -1;

    stream = stream_create_file (fp, 1, info.junk);
    if (!stream) {
        trace ("alac: stream_create_file failed\n");
        goto error;
    }

    if (!qtmovie_read(stream, &demux_res)) {
        if (!demux_res.format_read || demux_res.format != MAKEFOURCC('a','l','a','c')) {
            trace ("alac track not found in the file %s, expected atom %X got %X\n", fname, MAKEFOURCC('a','l','a','c'), demux_res.format);
            goto error;
        }
    }

    alac_file *alac = create_alac(demux_res.sample_size, demux_res.num_channels);
    alac_set_info(alac, demux_res.codecdata);
    demux_res.sample_rate = alac_get_samplerate (alac);
    demux_res.sample_size = alac_get_bitspersample (alac);
    alac_file_free (alac);

    it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", "ALAC");

    int totalsamples = alacplug_get_totalsamples (&demux_res);
    duration = totalsamples / (float)demux_res.sample_rate;

    deadbeef->plt_set_item_duration (plt, it, duration);

    // read tags
    mp4ff_callback_t cb = {
        .read = alacplug_fs_read,
        .write = NULL,
        .seek = alacplug_fs_seek,
        .truncate = NULL,
        .user_data = &info
    };
    deadbeef->fseek (fp, info.junk, SEEK_SET);
    mp4 = mp4ff_open_read (&cb);
    if (mp4) {
        alacplug_load_tags (it, mp4);
    }

    int apeerr = deadbeef->junk_apev2_read (it, fp);
    int v2err = deadbeef->junk_id3v2_read (it, fp);
    int v1err = deadbeef->junk_id3v1_read (it, fp);

    int64_t fsize = deadbeef->fgetlength (fp);

    deadbeef->fclose (fp);
    fp = NULL;
    stream_destroy (stream);
    stream = NULL;
    if (mp4) {
        mp4ff_close (mp4);
        mp4 = NULL;
    }
    int samplerate = demux_res.sample_rate;
    int bps = demux_res.sample_size;
    int channels = demux_res.num_channels;

    qtmovie_free_demux (&demux_res);

    trace ("duration %f\n", duration);
    if (duration > 0) {
        char s[100];
        snprintf (s, sizeof (s), "%lld", fsize);
        deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
        snprintf (s, sizeof (s), "%d", bps);
        deadbeef->pl_add_meta (it, ":BPS", s);
        snprintf (s, sizeof (s), "%d", channels);
        deadbeef->pl_add_meta (it, ":CHANNELS", s);
        snprintf (s, sizeof (s), "%d", samplerate);
        deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
        int br = (int)roundf(fsize / duration * 8 / 1000);
        snprintf (s, sizeof (s), "%d", br);
        deadbeef->pl_add_meta (it, ":BITRATE", s);
        // embedded cue
        deadbeef->pl_lock ();
        const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
        DB_playItem_t *cue = NULL;

        if (cuesheet) {
            cue = deadbeef->plt_insert_cue_from_buffer (plt, after, it, cuesheet, strlen (cuesheet), totalsamples, samplerate);
            if (cue) {
                deadbeef->pl_item_unref (it);
                deadbeef->pl_item_unref (cue);
                deadbeef->pl_unlock ();
                return cue;
            }
        }
        deadbeef->pl_unlock ();

        cue  = deadbeef->plt_insert_cue (plt, after, it, totalsamples, samplerate);
        if (cue) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            return cue;
        }
    }

    trace ("success\n");
success:
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
error:
    if (fp) {
        deadbeef->fclose (fp);
    }
    if (mp4) {
        mp4ff_close (mp4);
    }
    qtmovie_free_demux (&demux_res);
    return it;
}

static const char * exts[] = { "mp4", "m4a", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "alac",
    .plugin.name = "ALAC player",
    .plugin.descr = "plays alac files from MP4 and M4A files",
    .plugin.copyright = 
        "ALAC plugin for deadbeef\n"
        "Copyright (C) 2012-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
    .plugin.website = "http://deadbeef.sf.net",
    .open = alacplug_open,
    .init = alacplug_init,
    .free = alacplug_free,
    .read = alacplug_read,
    .seek = alacplug_seek,
    .seek_sample = alacplug_seek_sample,
    .insert = alacplug_insert,
    .read_metadata = alacplug_read_metadata,
    .exts = exts,
};

DB_plugin_t *
alac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
