/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <mpcdec/mpcdec.h>
#include <mpcdec/math.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    mpc_streaminfo si;
    mpc_decoder mpcdec;
    mpc_reader reader;
    int currentsample;
    int startsample;
    int endsample;
    mpc_uint32_t vbr_update_acc;
    mpc_uint32_t vbr_update_bits;
    float buffer[MPC_DECODER_BUFFER_LENGTH];
    int remaining;
} musepack_info_t;

mpc_int32_t musepack_vfs_read (void *t, void *ptr, mpc_int32_t size) {
    return deadbeef->fread(ptr, 1, size, (DB_FILE *)t);
}

/// Seeks to byte position offset.
mpc_bool_t musepack_vfs_seek (void *t, mpc_int32_t offset) {
    int res = deadbeef->fseek ((DB_FILE *)t, offset, SEEK_SET);
    if (res == 0) {
        return 1;
    }
    return 0;
}

/// Returns the current byte offset in the stream.
mpc_int32_t musepack_vfs_tell (void *t) {
    return deadbeef->ftell ((DB_FILE *)t);
}

/// Returns the total length of the source stream, in bytes.
mpc_int32_t musepack_vfs_get_size (void *t) {
    return deadbeef->fgetlength ((DB_FILE *)t);
}

/// True if the stream is a seekable stream.
mpc_bool_t musepack_vfs_canseek (void *t) {
    return 1;
}

static DB_fileinfo_t *
musepack_open (void) {
    DB_fileinfo_t *_info = malloc (sizeof (musepack_info_t));
    musepack_info_t *info = (musepack_info_t *)_info;
    memset (info, 0, sizeof (musepack_info_t));
    return _info;
}

static int
musepack_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    musepack_info_t *info = (musepack_info_t *)_info;

    info->reader.read = musepack_vfs_read;
    info->reader.seek = musepack_vfs_seek;
    info->reader.tell = musepack_vfs_tell;
    info->reader.get_size = musepack_vfs_get_size;
    info->reader.canseek = musepack_vfs_canseek;

    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        return -1;
    }
    info->reader.data = fp;

    mpc_streaminfo_init (&info->si);
    mpc_int32_t err;
    if ((err = mpc_streaminfo_read (&info->si, &info->reader)) != ERROR_CODE_OK) {
        fprintf (stderr, "mpc_streaminfo_read failed (err=%d)\n", err);
        deadbeef->fclose ((DB_FILE *)info->reader.data);
        info->reader.data = NULL;
        return -1;
    }
    mpc_decoder_setup (&info->mpcdec, &info->reader);
    if (!mpc_decoder_initialize (&info->mpcdec, &info->si)) {
        deadbeef->fclose ((DB_FILE *)info->reader.data);
        info->reader.data = NULL;
        return -1;
    }
    info->vbr_update_acc = 0;
    info->vbr_update_bits = 0;
    info->remaining = 0;

    _info->bps = 16;
    _info->channels = info->si.channels;
    _info->samplerate = info->si.sample_freq;
    _info->readpos = 0;
    _info->plugin = &plugin;

    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = mpc_streaminfo_get_length_samples (&info->si)-1;
    }

    return 0;
}

static void
musepack_free (DB_fileinfo_t *_info) {
    musepack_info_t *info = (musepack_info_t *)_info;
    if (info) {
        if (info->reader.data) {
            deadbeef->fclose ((DB_FILE *)info->reader.data);
            info->reader.data = NULL;
        }
        free (info);
    }
}

static int
musepack_read_int16 (DB_fileinfo_t *_info, char *bytes, int size) {
    musepack_info_t *info = (musepack_info_t *)_info;

    if (info->currentsample + size / (2 * _info->channels) > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * 2 * _info->channels;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;
    int out_channels = _info->channels;
    if (out_channels > 2) {
        out_channels = 2;
    }
    int sample_size = ((_info->bps >> 3) * out_channels);

    while (size > 0) {
        if (info->remaining > 0) {
            int n = size / sample_size;
            n = min (n, info->remaining);
            int nn = n;
            float *p = info->buffer;
            while (n > 0) {
                int sample = (int)(*p * 32767.0f);
                if (sample > 32767) {
                    sample = 32767;
                }
                else if (sample < -32768) {
                    sample = -32768;
                }
                *((int16_t *)bytes) = (int16_t)sample;
                bytes += 2;
                if (_info->channels == 2) {
                    sample = (int)(*(p+1) * 32767.0f);
                    if (sample > 32767) {
                        sample = 32767;
                    }
                    else if (sample < -32768) {
                        sample = -32768;
                    }
                    *((int16_t *)bytes) = (int16_t)sample;
                    bytes += 2;
                }
                n--;
                size -= sample_size;
                p += info->si.channels;
            }
            if (info->remaining > nn) {
                memmove (info->buffer, p, (info->remaining - nn) * sizeof (float) * _info->channels);
            }
            info->remaining -= nn;
        }

        if (size > 0 && !info->remaining) {
            mpc_uint32_t decoded = mpc_decoder_decode (&info->mpcdec, info->buffer, &info->vbr_update_acc, &info->vbr_update_bits);
            if (!decoded) {
                break;
            }
            assert (decoded <= MPC_DECODER_BUFFER_LENGTH);
            info->remaining = decoded;
        }
    }
    return initsize-size;
}

static int
musepack_read_float32 (DB_fileinfo_t *_info, char *bytes, int size) {
    musepack_info_t *info = (musepack_info_t *)_info;

    if (info->currentsample + size / (4 * _info->channels) > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * 4 * _info->channels;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;
    int out_channels = _info->channels;
    if (out_channels > 2) {
        out_channels = 2;
    }

    while (size > 0) {
        if (info->remaining > 0) {
            int n = size / (out_channels * 4);
            n = min (n, info->remaining);
            int nn = n;
            float *p = info->buffer;
            while (n > 0) {
                *((float *)bytes) = *p;
                bytes += 4;
                if (out_channels == 2) {
                    *((float *)bytes) = *(p+1);
                    bytes += 4;
                }
                n--;
                size -= out_channels * 4;
                p += info->si.channels;
            }
            if (info->remaining > nn) {
                memmove (info->buffer, p, (info->remaining - nn) * 4 * _info->channels);
            }
            info->remaining -= nn;
        }

        if (size > 0 && !info->remaining) {
            mpc_uint32_t decoded = mpc_decoder_decode (&info->mpcdec, info->buffer, &info->vbr_update_acc, &info->vbr_update_bits);
            if (!decoded) {
                break;
            }
            assert (decoded <= MPC_DECODER_BUFFER_LENGTH);
            info->remaining = decoded;
        }
    }
    return initsize-size;
}
static int
musepack_seek_sample (DB_fileinfo_t *_info, int sample) {
    musepack_info_t *info = (musepack_info_t *)_info;
    mpc_bool_t res = mpc_decoder_seek_sample (&info->mpcdec, sample);
    if (!res) {
        fprintf (stderr, "musepack: seek failed\n");
        return -1;
    }
    info->currentsample = sample;
    _info->readpos = (sample - info->startsample) / _info->samplerate;
    return 0;
}

static int
musepack_seek (DB_fileinfo_t *_info, float time) {
    musepack_info_t *info = (musepack_info_t *)_info;
    return musepack_seek_sample (_info, time * _info->samplerate);
}

static DB_playItem_t *
musepack_insert (DB_playItem_t *after, const char *fname) {
    mpc_reader reader = {
        .read = musepack_vfs_read,
        .seek = musepack_vfs_seek,
        .tell = musepack_vfs_tell,
        .get_size = musepack_vfs_get_size,
        .canseek = musepack_vfs_canseek,
    };

    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }
    reader.data = fp;

    mpc_streaminfo si;
    mpc_streaminfo_init (&si);
    mpc_int32_t err;
    if ((err = mpc_streaminfo_read (&si, &reader)) != ERROR_CODE_OK) {
        fprintf (stderr, "mpc_streaminfo_read failed (err=%d)\n", err);
        deadbeef->fclose (fp);
        return NULL;
    }

    int totalsamples = mpc_streaminfo_get_length_samples (&si);
    double dur = mpc_streaminfo_get_length (&si);

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = "MusePack";
    deadbeef->pl_set_item_duration (it, dur);

    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    deadbeef->fclose (fp);

    // embedded cue
    deadbeef->pl_lock ();
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    DB_playItem_t *cue = NULL;
    if (cuesheet) {
        cue = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), totalsamples, si.sample_freq);
        if (cue) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            deadbeef->pl_unlock ();
            return cue;
        }
    }
    deadbeef->pl_unlock ();

    cue  = deadbeef->pl_insert_cue (after, it, totalsamples, si.sample_freq);
    if (cue) {
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue);
        return cue;
    }

    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);

    return after;
}

static int musepack_read_metadata (DB_playItem_t *it) {
    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        return -1;
    }
    deadbeef->pl_delete_all_meta (it);
    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->fclose (fp);
    return 0;
}

static int musepack_write_metadata (DB_playItem_t *it) {
    // get options
    int strip_apev2 = deadbeef->conf_get_int ("ape.strip_apev2", 0);
    int write_apev2 = deadbeef->conf_get_int ("ape.write_apev2", 1);

    uint32_t junk_flags = 0;
    if (strip_apev2) {
        junk_flags |= JUNK_STRIP_APEV2;
    }
    if (write_apev2) {
        junk_flags |= JUNK_WRITE_APEV2;
    }

    return deadbeef->junk_rewrite_tags (it, junk_flags, 4, NULL);
}


static int
musepack_start (void) {
    return 0;
}

static int
musepack_stop (void) {
    return 0;
}

static const char * exts[] = { "mpc", "mpp", NULL };
static const char *filetypes[] = { "MusePack", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "musepack",
    .plugin.name = "MusePack decoder",
    .plugin.descr = "Musepack decoder using libmppdec",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = musepack_start,
    .plugin.stop = musepack_stop,
    .open = musepack_open,
    .init = musepack_init,
    .free = musepack_free,
    .read_int16 = musepack_read_int16,
    .read_float32 = musepack_read_float32,
    .seek = musepack_seek,
    .seek_sample = musepack_seek_sample,
    .insert = musepack_insert,
    .read_metadata = musepack_read_metadata,
    .write_metadata = musepack_write_metadata,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
musepack_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
