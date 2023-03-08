/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Oleksiy Yakovenko and other contributors

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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <deadbeef/deadbeef.h>

#define trace(...) { fprintf(stderr, __VA_ARGS__); }

#define FAKEIN_NUMSAMPLES 44100 * 5 // 5 sec
static int _sleep;

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    int64_t startsample;
    int64_t endsample;
    int64_t currentsample;

    float *samples;
} fakein_info_t;

static const char * exts[] = { "fake", NULL };

static DB_fileinfo_t *
fakein_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (fakein_info_t));
    fakein_info_t *info = (fakein_info_t *)_info;
    memset (info, 0, sizeof (fakein_info_t));
    return _info;
}

static int
fakein_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    fakein_info_t *info = (fakein_info_t *)_info;

    _info->fmt.bps = 32;
    _info->fmt.is_float = 1;
    _info->fmt.channels = 2;
    _info->fmt.samplerate  = 44100;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->readpos = 0;
    _info->plugin = &plugin;

    info->startsample = 0;
    info->endsample = FAKEIN_NUMSAMPLES - 1;

    info->samples = calloc (FAKEIN_NUMSAMPLES,  2 * sizeof (float));

    const char *type = deadbeef->pl_find_meta (it, "title");
    if (!strcmp (type, "sine")) {
        // 440Hz sine
        for (int i = 0; i < FAKEIN_NUMSAMPLES; i++) {
            float t = i / 44100 * 440 * M_PI * 2;
            info->samples[i] = sin (t);
        }
    }
    else if (!strcmp (type, "square")) {
        // 440Hz square
        for (int i = 0; i < FAKEIN_NUMSAMPLES; i++) {
            float t = i / 44100 * 440;
            t = t - floor (t);
            info->samples[i] = t > 0.5 ? -1 : 1;
        }
    }

    return 0;
}

static void
fakein_free (DB_fileinfo_t *_info) {
    fakein_info_t *info = (fakein_info_t *)_info;
    if (info) {
        if (info->samples) {
            free (info->samples);
        }
        free (info);
    }
}


static int
fakein_read (DB_fileinfo_t *_info, char *bytes, int size) {
    fakein_info_t *info = (fakein_info_t *)_info;

    usleep (_sleep);

    int samplesize = _info->fmt.bps / 8 * _info->fmt.channels;

    if (info->currentsample + size / samplesize > info->endsample) {
        size = (int)((info->endsample - info->currentsample + 1) * samplesize);
        if (size <= 0) {
            return 0;
        }
    }

    int nblocks = size / samplesize;

    memcpy (bytes, info->samples + info->currentsample * 2, size);

    info->currentsample += nblocks;
    return size;
}

static int
fakein_seek_sample (DB_fileinfo_t *_info, int sample) {
    fakein_info_t *info = (fakein_info_t *)_info;

    info->currentsample = sample + info->startsample;
    _info->readpos = (float)sample / _info->fmt.samplerate;
    return 0;
}

static int
fakein_seek (DB_fileinfo_t *_info, float time) {
    return fakein_seek_sample (_info, time * _info->fmt.samplerate);
}

static DB_playItem_t *
fakein_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    const char *ft = "fake";

    // no cuesheet, prepare track for addition
    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);

    deadbeef->pl_replace_meta (it, ":FILETYPE", ft);
    deadbeef->plt_set_item_duration (plt, it, FAKEIN_NUMSAMPLES);

    char title[100];
    strcpy (title, fname);
    *(strrchr(title, '.')) = 0;
    deadbeef->pl_add_meta (it, "title", title);

    // now the track is ready, insert into playlist
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "fakein",
    .plugin.id = "fakein",
    .open = fakein_open,
    .init = fakein_init,
    .free = fakein_free,
    .read = fakein_read,
    .seek = fakein_seek,
    .seek_sample = fakein_seek_sample,
    .insert = fakein_insert,
    .exts = exts,
};

DB_plugin_t *
fakein_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

void
fakein_set_sleep (int sleep) {
    _sleep = sleep;
}
