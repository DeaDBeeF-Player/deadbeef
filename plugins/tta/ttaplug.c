/*
    TTA plugin for DeaDBeeF
    Copyright (C) 2009-2014 Oleksiy Yakovenko

    Based on TTAv1 decoder library for HW players
    Copyright (c) 2004 True Audio Software. All rights reserved.

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
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>
#include "ttadec.h"
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
DB_functions_t *deadbeef;

#define MAX_BSIZE (MAX_BPS>>3)

typedef struct {
    DB_fileinfo_t info;
    tta_info tta;
    int64_t currentsample;
    int64_t startsample;
    int64_t endsample;
    char buffer[PCM_BUFFER_LENGTH * MAX_BSIZE * MAX_NCH];
    int remaining;
    int samples_to_skip;
} tta_info_t;

static DB_fileinfo_t *
tta_open (uint32_t hints) {
    tta_info_t *info = calloc (1, sizeof (tta_info_t));
    return &info->info;
}

static int
tta_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    tta_info_t *info = (tta_info_t *)_info;

    deadbeef->pl_lock ();
    const char *fname = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    trace ("open_tta_file %s\n", fname);
    if (open_tta_file (fname, &info->tta, 0) != 0) {
        fprintf (stderr, "tta: failed to open %s\n", fname);
        return -1;
    }

    if (player_init (&info->tta) != 0) {
        fprintf (stderr, "tta: failed to init player for %s\n", fname);
        return -1;
    }

    _info->fmt.bps = info->tta.BPS;
    _info->fmt.channels = info->tta.NCH;
    _info->fmt.samplerate = info->tta.SAMPLERATE;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->readpos = 0;
    _info->plugin = &plugin;

    int64_t endsample = deadbeef->pl_item_get_endsample (it);
    if (endsample > 0) {
        info->startsample = deadbeef->pl_item_get_startsample (it);
        info->endsample = endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        info->endsample = (info->tta.DATALENGTH)-1;
    }
    trace ("open_tta_file %s success!\n", deadbeef->pl_find_meta (it, ":URI"));
    return 0;
}

static void
tta_free (DB_fileinfo_t *_info) {
    tta_info_t *info = (tta_info_t *)_info;
    if (info) {
        player_stop (&info->tta);
        close_tta_file (&info->tta);
        free (info);
    }
}

static int
tta_read (DB_fileinfo_t *_info, char *bytes, int size) {
    tta_info_t *info = (tta_info_t *)_info;
    int samplesize = _info->fmt.channels * _info->fmt.bps / 8;
    if (info->currentsample + size / samplesize > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * samplesize;
        if (size <= 0) {
            return 0;
        }
    }

    int initsize = size;

    while (size > 0) {
        if (info->samples_to_skip > 0 && info->remaining > 0) {
            int skip = min (info->remaining, info->samples_to_skip);
            if (skip < info->remaining) {
                memmove (info->buffer, info->buffer + skip * samplesize, (info->remaining - skip) * samplesize);
            }
            info->remaining -= skip;
            info->samples_to_skip -= skip;
        }
        if (info->remaining > 0) {
            int n = size / samplesize;
            n = min (n, info->remaining);
            int nn = n;
            char *p = info->buffer;

            memcpy (bytes, p, n * samplesize);
            bytes += n * samplesize;
            size -= n * samplesize;
            p += n * samplesize;

            if (info->remaining > nn) {
                memmove (info->buffer, p, (info->remaining - nn) * samplesize);
            }
            info->remaining -= nn;
        }

        if (size > 0 && !info->remaining) {
            info->remaining = get_samples (&info->tta, (unsigned char *)info->buffer);
            if (info->remaining <= 0) {
                break;
            }
        }
    }
    info->currentsample += (initsize-size) / samplesize;
    deadbeef->streamer_set_bitrate (info->tta.BITRATE);
    return initsize-size;
}

static int
tta_seek_sample (DB_fileinfo_t *_info, int sample) {
    tta_info_t *info = (tta_info_t *)_info;

    info->samples_to_skip = set_position (&info->tta, sample + info->startsample);
    if (info->samples_to_skip < 0) {
        fprintf (stderr, "tta: seek failed\n");
        return -1;
    }

    info->currentsample = sample + info->startsample;
    info->remaining = 0;
    _info->readpos = sample / _info->fmt.samplerate;
    return 0;
}

static int
tta_seek (DB_fileinfo_t *_info, float time) {
    return tta_seek_sample (_info, time * _info->fmt.samplerate);
}

static DB_playItem_t *
tta_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    tta_info tta;
    if (open_tta_file (fname, &tta, 0) != 0) {
        fprintf (stderr, "tta: failed to open %s\n", fname);
        return NULL;
    }

//    if (tta.BPS != 16) {
//        fprintf (stderr, "tta: only 16 bit is supported yet, skipped %s\n", fname);
//        return NULL;
//    }

    int totalsamples = tta.DATALENGTH;
    double dur = tta.LENGTH;

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", "TTA");
    deadbeef->plt_set_item_duration (plt, it, dur);

    close_tta_file (&tta);
    DB_FILE *fp = deadbeef->fopen (fname);

    int64_t fsize = -1;
    if (fp) {
        fsize = deadbeef->fgetlength (fp);
        /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
        /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
        /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
        deadbeef->fclose (fp);
    }

    char s[100];
    snprintf (s, sizeof (s), "%" PRId64, fsize);
    deadbeef->pl_add_meta (it, ":FILE_SIZE", s);
    snprintf (s, sizeof (s), "%d", tta.BPS);
    deadbeef->pl_add_meta (it, ":BPS", s);
    snprintf (s, sizeof (s), "%d", tta.NCH);
    deadbeef->pl_add_meta (it, ":CHANNELS", s);
    snprintf (s, sizeof (s), "%d", tta.SAMPLERATE);
    deadbeef->pl_add_meta (it, ":SAMPLERATE", s);
    snprintf (s, sizeof (s), "%d", tta.BITRATE);
    deadbeef->pl_add_meta (it, ":BITRATE", s);

    DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it,  totalsamples, tta.SAMPLERATE);
    if (cue) {
        deadbeef->pl_item_unref (it);
        return cue;
    }

    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);

    return after;
}

static int tta_read_metadata (DB_playItem_t *it) {
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    DB_FILE *fp = deadbeef->fopen (uri);
    if (!fp) {
        return -1;
    }
    deadbeef->pl_delete_all_meta (it);
    /*int apeerr = */deadbeef->junk_apev2_read (it, fp);
    /*int v2err = */deadbeef->junk_id3v2_read (it, fp);
    /*int v1err = */deadbeef->junk_id3v1_read (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->fclose (fp);
    return 0;
}

static int tta_write_metadata (DB_playItem_t *it) {
    // get options

    int strip_id3v2 = 0;
    int strip_id3v1 = 1;
    int strip_apev2 = 0;
    int write_id3v2 = 1;
    int write_id3v1 = 0;
    int write_apev2 = 1;

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

    int id3v2_version = 4;
    char id3v1_encoding[50];
    deadbeef->conf_get_str ("mp3.id3v1_encoding", "cp1252", id3v1_encoding, sizeof (id3v1_encoding));
    return deadbeef->junk_rewrite_tags (it, junk_flags, id3v2_version, id3v1_encoding);
}


static int
tta_start (void) {
    return 0;
}

static int
tta_stop (void) {
    return 0;
}

static const char * exts[] = { "tta", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "tta",
    .plugin.name = "tta decoder",
    .plugin.descr = "tta decoder based on TTA Hardware Players Library Version 1.2",
    .plugin.copyright = 
        "TTA plugin for DeaDBeeF\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
        "\n"
        "Based on TTAv1 decoder library for HW players\n"
        "Copyright (c) 2004 True Audio Software. All rights reserved.\n"
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
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = tta_start,
    .plugin.stop = tta_stop,
    .open = tta_open,
    .init = tta_init,
    .free = tta_free,
    .read = tta_read,
    .seek = tta_seek,
    .seek_sample = tta_seek_sample,
    .insert = tta_insert,
    .read_metadata = tta_read_metadata,
    .write_metadata = tta_write_metadata,
    .exts = exts,
};

DB_plugin_t *
tta_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
