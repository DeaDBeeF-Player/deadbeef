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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "decomp.h"

#include "mp4ff.h"
#include "../../shared/mp4tagutil.h"

#include "../mp4parser/mp4parser.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#ifndef __linux__
#define off64_t off_t
#define lseek64 lseek
#define O_LARGEFILE 0
#endif

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t alac_plugin;
DB_functions_t *deadbeef;

#define USE_MP4FF 1

#if USE_MP4FF
#define MP4FILE mp4ff_t *
#define MP4FILE_CB mp4ff_callback_t
#endif

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
#if USE_MP4FF
    MP4FILE mp4;
    MP4FILE_CB mp4reader;
    int mp4track;
    int mp4samples;
#endif
    alac_file *_alac;
    int mp4sample;
    int junk;
    uint8_t out_buffer[BUFFER_SIZE];
    int out_remaining;
    int skipsamples;
    int currentsample;
    int startsample;
    int endsample;
} alacplug_info_t;

// allocate codec control structure
DB_fileinfo_t *
alacplug_open (uint32_t hints) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)malloc (sizeof (alacplug_info_t));
    alacplug_info_t *info = (alacplug_info_t *)_info;
    memset (info, 0, sizeof (alacplug_info_t));
    return _info;
}

#if USE_MP4FF
static uint32_t
mp4_fs_read (void *user_data, void *buffer, uint32_t length) {
    alacplug_info_t *info = (alacplug_info_t *)user_data;
    return (uint32_t)deadbeef->fread (buffer, 1, length, info->file);
}

static uint32_t
mp4_fs_seek (void *user_data, uint64_t position) {
    alacplug_info_t *info = (alacplug_info_t *)user_data;
    return deadbeef->fseek (info->file, position+info->junk, SEEK_SET);
}

static int
mp4_track_get_info (mp4ff_t *mp4, int track, float *duration, int *samplerate, int *channels, int *bps, int64_t *totalsamples) {
    unsigned char*  buff = 0;
    unsigned int    buff_size = 0;
    int samples = 0;

    if (mp4ff_get_decoder_config(mp4, track, &buff, &buff_size)) {
        return -1;
    }
    if (!buff) {
        return -1;
    }

    alac_file *alac = create_alac (mp4->track[track]->sampleSize, mp4->track[track]->channelCount);
    alac_set_info (alac, (char *)buff);

    *samplerate = mp4->track[track]->sampleRate;
    *channels = mp4->track[track]->channelCount;
    *bps = mp4->track[track]->sampleSize;

    samples = mp4ff_num_samples(mp4, track);

    if (samples <= 0) {
        return -1;
    }

    int i_sample_count = samples;
    int i_sample;

    int64_t total_dur = 0;
    for( i_sample = 0; i_sample < i_sample_count; i_sample++ )
    {
        total_dur += mp4ff_get_sample_duration (mp4, track, i_sample);
    }
    if (totalsamples) {
        *totalsamples = total_dur * (*samplerate) / mp4ff_time_scale (mp4, track);
    }
    *duration = total_dur / (float)mp4ff_time_scale (mp4, track);

    return 0;
}
#endif

int
alacplug_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    alacplug_info_t *info = (alacplug_info_t *)_info;

    deadbeef->pl_lock ();
    info->file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    if (!info->file) {
        return -1;
    }

    int64_t totalsamples = 0;

#if USE_MP4FF
    info->mp4track = -1;
    info->mp4reader.read = mp4_fs_read;
    info->mp4reader.write = NULL;
    info->mp4reader.seek = mp4_fs_seek;
    info->mp4reader.truncate = NULL;
    info->mp4reader.user_data = info;

    int samplerate = 0;
    int channels = 0;
    int bps = 0;
    float duration = 0;

    trace ("alac_init: mp4ff_open_read %s\n", deadbeef->pl_find_meta (it, ":URI"));
    info->mp4 = mp4ff_open_read (&info->mp4reader);
    if (info->mp4) {
        int ntracks = mp4ff_total_tracks (info->mp4);
        for (int i = 0; i < ntracks; i++) {
            if (mp4ff_get_track_type (info->mp4, i) != TRACK_AUDIO) {
                continue;
            }
            int res = mp4_track_get_info (info->mp4, i, &duration, &samplerate, &channels, &bps, &totalsamples);
            if (res >= 0 && duration > 0) {
                info->mp4track = i;
                break;
            }
        }
        trace ("track: %d\n", info->mp4track);
        if (info->mp4track >= 0) {
            // init mp4 decoding
            info->mp4samples = mp4ff_num_samples(info->mp4, info->mp4track);

            unsigned char*  buff = 0;
            unsigned int    buff_size = 0;
            if (mp4ff_get_decoder_config (info->mp4, info->mp4track, &buff, &buff_size)) {
                return -1;
            }

            info->_alac = create_alac (info->mp4->track[info->mp4track]->sampleSize, info->mp4->track[info->mp4track]->channelCount);
            alac_set_info (info->_alac, (char *)buff);

            trace ("alac: successfully initialized track %d\n", info->mp4track);
            _info->fmt.samplerate = samplerate;
            _info->fmt.channels = channels;
            _info->fmt.bps = bps;
        }
        else {
            trace ("alac: track not found in mp4 container\n");
            mp4ff_close (info->mp4);
            info->mp4 = NULL;
        }
    }
#endif

    if (!info->file->vfs->is_streaming ()) {
        if (it->endsample > 0) {
            info->startsample = it->startsample;
            info->endsample = it->endsample;
            alac_plugin.seek_sample (_info, 0);
        }
        else {
            info->startsample = 0;
            info->endsample = (int)(totalsamples-1);
        }
    }

    _info->plugin = &alac_plugin;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }

    // prototype
    DB_FILE *file = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    mp4p_file_callbacks_t cb = {
        .data = file,
        .fread = deadbeef->fread,
        .fseek = deadbeef->fseek,
        .ftell = deadbeef->ftell
    };

    mp4p_atom_t *root = mp4p_open(NULL, &cb);

    // iterate over tracks, and the ALAC one
    mp4p_atom_t *trak = mp4p_atom_find (root, "moov/trak");
    mp4p_alac_t *alac = NULL;
    while (trak) {
        mp4p_atom_t *alac_atom = mp4p_atom_find (trak, "trak/mdia/minf/stbl/stsd/alac");
        if (alac_atom) {
            alac = alac_atom->data;
            break;
        }
        trak = trak->next;
    }
    if (!alac) {
        printf ("not found\n");
    }

    // get audio format: samplerate, bps, channels
    int _samplerate = alac->sample_rate;
    int _bps = alac->bps;
    int _channels = alac->channel_count;
    printf ("%d %d %d\n", _samplerate, _bps, _channels);

    // get file info: totalsamples

    // 1. sum of all stts entries
    mp4p_atom_t *stts_atom = mp4p_atom_find(trak, "trak/mdia/minf/stbl/stts");
    mp4p_atom_t *mdhd_atom = mp4p_atom_find(trak, "trak/mdia/mdhd");
    mp4p_atom_t *stsz_atom = mp4p_atom_find(trak, "trak/mdia/minf/stbl/stsz");
    mp4p_atom_t *stbl_atom = mp4p_atom_find(trak, "trak/mdia/minf/stbl");

    mp4p_mdhd_t *mdhd = mdhd_atom->data;

    uint64_t num_mp4samples = mp4p_stts_total_num_samples(stts_atom);

    printf ("num_samples: %lld\n", num_mp4samples);

    // total PCM samples and duration

    // duration is calculated from stts table (time-to-sample), which is not pcm-sample-precise.
    // there should be a way to count pcm samples
    uint64_t total_sample_duration = mp4p_stts_total_sample_duration (stts_atom);
    uint64_t _totalsamples = total_sample_duration * _samplerate / mdhd->time_scale;
    float _duration = total_sample_duration / (float)mdhd->time_scale;

    // byte size of a sample
    uint32_t _sample_size = mp4p_sample_size (stsz_atom, 20);

    // byte offset of a sample
    uint64_t _sample_offs = mp4p_sample_offset (stbl_atom, 20);

    mp4p_atom_free (root);

    exit (0);

    return 0;
}

void
alacplug_free (DB_fileinfo_t *_info) {
    alacplug_info_t *info = (alacplug_info_t *)_info;
    if (info) {
        if (info->file) {
            deadbeef->fclose (info->file);
        }
#if USE_MP4FF
        if (info->mp4) {
            mp4ff_close (info->mp4);
        }
#endif

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
        uint32_t buffer_size = 0;
        uint32_t outNumSamples = 0;

#if USE_MP4FF
        if (info->mp4sample >= info->mp4samples) {
            trace ("alac: finished with the last mp4sample\n");
            break;
        }

        int rc = mp4ff_read_sample (info->mp4, info->mp4track, info->mp4sample, &buffer, &buffer_size);
        if (rc == 0) {
            trace ("mp4ff_read_sample failed\n");
            break;
        }
#endif

        int outputBytes = 0;
        decode_frame(info->_alac, buffer, info->out_buffer, &outputBytes);
        outNumSamples = outputBytes / samplesize;

        info->out_remaining += outNumSamples;
        info->mp4sample++;

#if USE_MP4FF
        if (buffer) {
            free (buffer);
        }
#endif
    }

    info->currentsample += (initsize-size) / samplesize;
    return initsize-size;
}

int
alacplug_seek_sample (DB_fileinfo_t *_info, int sample) {
    alacplug_info_t *info = (alacplug_info_t *)_info;

    sample += info->startsample;

#if USE_MP4FF
    int totalsamples = 0;
    int i;
    int num_sample_byte_sizes = mp4ff_get_num_sample_byte_sizes (info->mp4, info->mp4track);
    int scale = _info->fmt.samplerate / mp4ff_time_scale (info->mp4, info->mp4track);
    for (i = 0; i < num_sample_byte_sizes; i++)
    {
        unsigned int thissample_duration = 0;
        unsigned int thissample_bytesize = 0;

        mp4ff_get_sample_info(info->mp4, info->mp4track, i, &thissample_duration,
                              &thissample_bytesize);

        if (totalsamples + thissample_duration > sample / scale) {
            info->skipsamples = sample - totalsamples * scale;
            break;
        }
        totalsamples += thissample_duration;
    }
    info->mp4sample = i;
    trace ("seek res: frame %d, skip %d\n", info->mp4sample, info->skipsamples);
#endif
    info->out_remaining = 0;
    info->currentsample = sample;
    _info->readpos = (float)(info->currentsample - info->startsample) / _info->fmt.samplerate;
    return 0;
}

int
alacplug_seek (DB_fileinfo_t *_info, float t) {
    return alacplug_seek_sample (_info, t * _info->fmt.samplerate);
}

DB_playItem_t *
alacplug_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("adding %s\n", fname);

    int samplerate = 0;
    int bps = 0;
    int channels = 0;
    int64_t totalsamples = 0;

    alacplug_info_t info;
    mp4ff_t *mp4 = NULL;
    DB_playItem_t *it = NULL;

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

#if USE_MP4FF

    const char *ftype = NULL;
    float duration = -1;

    // slowwww!
    info.file = fp;
    MP4FILE_CB cb = {
        .read = mp4_fs_read,
        .write = NULL,
        .seek = mp4_fs_seek,
        .truncate = NULL,
        .user_data = &info
    };

    mp4 = mp4ff_open_read (&cb);
    if (mp4) {
        int ntracks = mp4ff_total_tracks (mp4);
        trace ("alac: numtracks=%d\n", ntracks);
        int i;
        for (i = 0; i < ntracks; i++) {
            if (mp4ff_get_track_type (mp4, i) != TRACK_AUDIO) {
                trace ("alac: track %d is not audio\n", i);
                continue;
            }
            int res = mp4_track_get_info (mp4, i, &duration, &samplerate, &channels, &bps, &totalsamples);
            if (res >= 0 && duration > 0) {
                trace ("alac: found audio track %d (duration=%f, totalsamples=%d)\n", i, duration, totalsamples);

#if 0 // TODO
                int num_chapters = 0;
                mp4_chapter_t *chapters = NULL;
                if (mp4ff_chap_get_num_tracks(mp4) > 0) {
                    chapters = mp4_load_itunes_chapters (mp4, &num_chapters, samplerate);
                }
#endif
                DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, alac_plugin.plugin.id);
                ftype = "ALAC";
                deadbeef->pl_add_meta (it, ":FILETYPE", ftype);
                deadbeef->pl_set_meta_int (it, ":TRACKNUM", i);
                deadbeef->plt_set_item_duration (plt, it, duration);

                deadbeef->rewind (fp);
                mp4_read_metadata_file(it, fp);

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

#if 0 // TODO
                // embedded chapters
                deadbeef->pl_lock (); // FIXME: is it needed?
                if (chapters && num_chapters > 0) {
                    DB_playItem_t *cue = mp4_insert_with_chapters (plt, after, it, chapters, num_chapters, totalsamples, samplerate);
                    for (int n = 0; n < num_chapters; n++) {
                        if (chapters[n].title) {
                            free (chapters[n].title);
                        }
                    }
                    free (chapters);
                    if (cue) {
                        mp4ff_close (mp4);
                        deadbeef->pl_item_unref (it);
                        deadbeef->pl_item_unref (cue);
                        deadbeef->pl_unlock ();
                        return cue;
                    }
                }
#endif

                // embedded cue
                const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
                DB_playItem_t *cue = NULL;

                if (cuesheet) {
                    cue = deadbeef->plt_insert_cue_from_buffer (plt, after, it, (const uint8_t *)cuesheet, (int)strlen (cuesheet), (int)totalsamples, samplerate);
                    if (cue) {
                        mp4ff_close (mp4);
                        deadbeef->pl_item_unref (it);
                        deadbeef->pl_item_unref (cue);
                        deadbeef->pl_unlock ();
                        return cue;
                    }
                }
                deadbeef->pl_unlock ();

                cue  = deadbeef->plt_insert_cue (plt, after, it, (int)totalsamples, samplerate);
                if (cue) {
                    deadbeef->pl_item_unref (it);
                    deadbeef->pl_item_unref (cue);
                    return cue;
                }

                after = deadbeef->plt_insert_item (plt, after, it);
                deadbeef->pl_item_unref (it);
                break;
            }
        }
        mp4ff_close (mp4);
        if (i < ntracks) {
            return after;
        }
        // mp4 container found, but no valid alac tracks in it
        return NULL;
    }
#endif
    return it;
}

static const char * exts[] = { "mp4", "m4a", NULL };

// define plugin interface
static DB_decoder_t alac_plugin = {
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
    .read_metadata = mp4_read_metadata,
    .write_metadata = mp4_write_metadata,
    .exts = exts,
};

DB_plugin_t *
alac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&alac_plugin);
}
