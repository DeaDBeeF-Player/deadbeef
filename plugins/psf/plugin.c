/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
#include <stdlib.h>
#include <string.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>
#include "ao.h"
#include "eng_protos.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

DB_functions_t *deadbeef;
static DB_decoder_t plugin;

static const char * exts[] = { "psf", "psf2", "spu", "ssf", "qsf", "dsf", "minipsf", "minipsf2", "minissf", "miniqsf", "minidsf", NULL };

typedef struct {
    DB_fileinfo_t info;
    int currentsample;
    int type;
    void *decoder;
    char *filebuffer;
    size_t filesize;
    char buffer[735*4]; // psf2 decoder only works with 735 samples buffer
    int remaining;
    int skipsamples;
    float duration;
} psfplug_info_t;

static DB_fileinfo_t *
psfplug_open (uint32_t hints) {
    psfplug_info_t *info = calloc (1, sizeof (psfplug_info_t));
    return &info->info;
}

static int
psfplug_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    psfplug_info_t *info = (psfplug_info_t *)_info;

    _info->fmt.bps = 16;
    _info->fmt.channels = 2;
    _info->fmt.samplerate = deadbeef->conf_get_int ("synth.samplerate", 44100);
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    _info->readpos = 0;
    _info->plugin = &plugin;
    info->duration = deadbeef->pl_get_item_duration (it);

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    DB_FILE *file = deadbeef->fopen (uri);
    if (!file) {
        trace ("psf: failed to fopen %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }

    info->filesize = deadbeef->fgetlength (file);
    info->filebuffer = malloc (info->filesize);
    if (!info->filebuffer) {
		trace ("psf: could not allocate %d bytes of memory\n", (int)info->filesize);
		deadbeef->fclose (file);
        return -1;
    }

	if (deadbeef->fread(info->filebuffer, 1, info->filesize, file) != info->filesize) {
        deadbeef->pl_lock ();
		trace ("psf: file read error: %s\n", deadbeef->pl_find_meta (it, ":URI"));
		deadbeef->pl_unlock ();
		deadbeef->fclose (file);
        return -1;
    }
    deadbeef->fclose (file);

    info->type = ao_identify (info->filebuffer);
    if (info->type < 0) {
        trace ("psf: ao_identify failed\n");
        return -1;
    }

    deadbeef->pl_lock ();
    info->decoder = ao_start (info->type, deadbeef->pl_find_meta (it, ":URI"), (uint8 *)info->filebuffer, (uint32)info->filesize);
    deadbeef->pl_unlock ();
    if (!info->decoder) {
        trace ("psf: ao_start failed\n");
        return -1;
    }

    return 0;
}

static void
psfplug_free (DB_fileinfo_t *_info) {
    psfplug_info_t *info = (psfplug_info_t *)_info;
    if (info) {
        if (info->type >= 0) {
            ao_stop (info->type, info->decoder);
        }
        if (info->filebuffer) {
            free (info->filebuffer);
            info->filebuffer = NULL;
        }
        free (info);
    }
}

static int
psfplug_read (DB_fileinfo_t *_info, char *bytes, int size) {
    psfplug_info_t *info = (psfplug_info_t *)_info;
//    printf ("psfplug_read_int16 %d samples, curr %d, end %d\n", size/4, info->currentsample, (int)(info->duration * _info->samplerate));

    if (info->currentsample >= info->duration * _info->fmt.samplerate) {
        return 0;
    }

    int initsize = size;

    while (size > 0) {
        if (info->remaining > 0) {
            if (info->skipsamples > 0) {
                int n = min (info->skipsamples, info->remaining);
                if (info->remaining > n) {
                    memmove (info->buffer, info->buffer+n*4, (info->remaining - n)*4);
                }
                info->remaining -= n;
                info->skipsamples -= n;
                continue;
            }
            int n = size / 4;
            n = min (info->remaining, n);
            memcpy (bytes, info->buffer, n * 4);
            if (info->remaining > n) {
                memmove (info->buffer, info->buffer+n*4, (info->remaining - n)*4);
            }
            info->remaining -= n;
            bytes += n*4;
            size -= n*4;
        }
        if (!info->remaining) {
            ao_decode (info->type, info->decoder, (int16_t *)info->buffer, 735);
            info->remaining = 735;
        }
    }
    info->currentsample += (initsize-size) / (_info->fmt.channels * _info->fmt.bps/8);
    _info->readpos = (float)info->currentsample / _info->fmt.samplerate;
    return initsize-size;
}

static int
psfplug_seek_sample (DB_fileinfo_t *_info, int sample) {
    psfplug_info_t *info = (psfplug_info_t *)_info;
    if (sample > info->currentsample) {
        info->skipsamples = sample-info->currentsample;
    }
    else {
        // restart song
        ao_command (info->type, info->decoder, COMMAND_RESTART, 0);
        info->skipsamples = sample;
    }
    info->currentsample = sample;
    _info->readpos = (float)sample / _info->fmt.samplerate;
    return 0;
}

static int
psfplug_seek (DB_fileinfo_t *_info, float time) {
    return psfplug_seek_sample (_info, time * _info->fmt.samplerate);
}

static void
psfplug_add_meta (DB_playItem_t *it, const char *key, const char *value, const char *comment_title) {
    size_t tmpsize = 2048;
    char *tmp = calloc (1, tmpsize);
    // check utf8
    if (deadbeef->junk_recode (value, (int)strlen (value), tmp, tmpsize, "utf-8") >= 0) {
        if (key) {
            deadbeef->pl_add_meta (it, key, value);
        }
    }
    // check shift-jis
    if (deadbeef->junk_recode (value, (int)strlen (value), tmp, tmpsize, "SHIFT-JIS") >= 0) {
        if (key) {
            deadbeef->pl_add_meta (it, key, tmp);
        }
    }
    free (tmp);
}

static DB_playItem_t *
psfplug_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("psf: failed to open %s\n", fname);
        return NULL;
    }

    int64_t fsize = deadbeef->fgetlength (fp);
    if (fsize > 4*1024*1024) {
        deadbeef->fclose (fp);
        return NULL; // don't try loading large files
    }

    // identify by first 200 bytes
    size_t size = 200;
    char *buffer = malloc (size);
    if (!buffer) {
        deadbeef->fclose (fp);
		trace ("psf: could not allocate %d bytes of memory\n", (int)size);
        return NULL;
    }

	if (deadbeef->fread(buffer, 1, size, fp) != size) {
        deadbeef->fclose (fp);
        free (buffer);
		trace ("psf: file read error: %s\n", fname);
        return NULL;
    }

    int type = ao_identify (buffer);
    if (type < 0) {
        free (buffer);
        return NULL;
    }

    free (buffer);

    // try loading the whole file
    deadbeef->rewind (fp);
    size = fsize;
    buffer = malloc (size);
    if (deadbeef->fread(buffer, 1, size, fp) != size) {
        deadbeef->fclose (fp);
        free (buffer);
        trace ("psf: file read error: %s\n", fname);
        return NULL;
    }
    deadbeef->fclose (fp);

    void *dec = ao_start (type, fname, (uint8*)buffer, (uint32_t)size);
    if (!dec) {
        free (buffer);
        return NULL;
    }
    ao_display_info info;
    memset (&info, 0, sizeof (info));
    int have_info = 0;
    if (ao_get_info (type, dec, &info) == AO_SUCCESS) {
        have_info = 1;
    }

    ao_stop (type, dec);
    dec = NULL;

	free (buffer);
	
    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    const char *ext = fname + strlen (fname);
    while (*ext != '.' && ext > fname) {
        ext--;
    }
    if (*ext == '.') {
        ext++;
        if (!strcasecmp (ext, "psf") || !strcasecmp (ext, "minipsf")) {
            deadbeef->pl_add_meta (it, ":FILETYPE", "PSF");
        }
        else if (!strcasecmp (ext, "psf2") || !strcasecmp (ext, "minipsf2")) {
            deadbeef->pl_add_meta (it, ":FILETYPE", "PSF2");
        }
        else if (!strcasecmp (ext, "spu")) {
            deadbeef->pl_add_meta (it, ":FILETYPE", "SPU");
        }
        else if (!strcasecmp (ext, "ssf") || !strcasecmp (ext, "minissf")) {
            deadbeef->pl_add_meta (it, ":FILETYPE", "SSF");
        }
        else if (!strcasecmp (ext, "qsf") || !strcasecmp (ext, "miniqsf")) {
            deadbeef->pl_add_meta (it, ":FILETYPE", "QSF");
        }
        else if (!strcasecmp (ext, "dsf") || !strcasecmp (ext, "minidsf")) {
            deadbeef->pl_add_meta (it, ":FILETYPE", "DSF");
        }
    }
    else {
        deadbeef->pl_add_meta (it, ":FILETYPE", "PSF");
    }

    float duration = 120;
    float fade = 0;

    if (have_info) {
        int i;
        for (i = 1; i < 9; i++) {
            if (!strncasecmp (info.title[i], "Length: ", 8)) {
                int min;
                float sec;
                if (sscanf (info.info[i], "%d:%f", &min, &sec) == 2) {
                    duration = min * 60 + sec;
                }
                else if (sscanf (info.info[i], "%f", &sec) == 1) {
                    duration = sec;
                }
                psfplug_add_meta (it, NULL, info.info[i], info.title[i]);
            }
            else if (!strncasecmp (info.title[i], "Name: ", 6) || !strncasecmp (info.title[i], "Song: ", 6)) {
                psfplug_add_meta (it, "title", info.info[i], info.title[i]);
            }
            else if (!strncasecmp (info.title[i], "Game: ", 6)) {
                psfplug_add_meta (it, "album", info.info[i], info.title[i]);
            }
            else if (!strncasecmp (info.title[i], "Artist: ", 8)) {
                psfplug_add_meta (it, "artist", info.info[i], info.title[i]);
            }
            else if (!strncasecmp (info.title[i], "Copyright: ", 11)) {
                psfplug_add_meta (it, "copyright", info.info[i], info.title[i]);
            }
            else if (!strncasecmp (info.title[i], "Year: ", 6)) {
                psfplug_add_meta (it, "year", info.info[i], info.title[i]);
            }
            else if (!strncasecmp (info.title[i], "Fade: ", 6)) {
                fade = atof (info.info[i]);
                psfplug_add_meta (it, "fade", info.info[i], info.title[i]);
            }
            else {
                char *colon = strchr (info.title[i], ':');
                if (colon != NULL) {
                    char name[colon-info.title[i]+1];
                    memcpy (name, info.title[i], colon-info.title[i]);
                    name[colon-info.title[i]] = 0;
                    psfplug_add_meta (it, name, info.info[i], info.title[i]);
                }
            }
        }
    }
    deadbeef->plt_set_item_duration (plt, it, duration+fade);
    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

static int
psfplug_start (void) {
    return 0;
}

static int
psfplug_stop (void) {
    return 0;
}

static DB_decoder_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.id = "psf",
    .plugin.name = "PSF player using Audio Overload SDK",
    .plugin.descr = "plays psf, psf2, spu, ssf, dsf, qsf file formats",
    .plugin.copyright = 
        "Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses modified aosdk-1.4.8 - library for playing .PSF (Sony PlayStation), .SPU (Sony PlayStation), .PSF2 (Sony PlayStation 2), .SSF (Sega Saturn), .DSF (Sega Dreamcast), and .QSF (Capcom QSound) audio file formats,\n"
        "http://rbelmont.mameworld.info/?page_id=221\n"
        "Copyright © 2007-2009 R. Belmont and Richard Bannister.\n"
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
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = psfplug_start,
    .plugin.stop = psfplug_stop,
    .open = psfplug_open,
    .init = psfplug_init,
    .free = psfplug_free,
    .read = psfplug_read,
    .seek = psfplug_seek,
    .seek_sample = psfplug_seek_sample,
    .insert = psfplug_insert,
    .exts = exts,
};

DB_plugin_t *
psf_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

