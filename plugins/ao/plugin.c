/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../deadbeef.h"
#include "ao.h"
#include "eng_protos.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

DB_functions_t *deadbeef;
static DB_decoder_t plugin;

static const char * exts[] = { "psf", "psf2", "spu", "ssf", "minidsf", "qsf", "dsf", "miniqsf", NULL };
static const char *filetypes[] = { "PSF", "PSF2", "SPU", "SSF", "QSF", "DSF", NULL };

typedef struct {
    DB_fileinfo_t info;
    int currentsample;
    uint32 type;
    void *decoder;
    char *filebuffer;
    size_t filesize;
    char buffer[735*4]; // psf2 decoder only works with 735 samples buffer
    int remaining;
} aoplug_info_t;

static DB_fileinfo_t *
aoplug_open (void) {
    DB_fileinfo_t *_info = malloc (sizeof (aoplug_info_t));
    aoplug_info_t *info = (aoplug_info_t *)_info;
    memset (info, 0, sizeof (aoplug_info_t));
    return _info;
}

static int
aoplug_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    aoplug_info_t *info = (aoplug_info_t *)_info;

    _info->bps = 16;
    _info->channels = 2;
    _info->samplerate = 44100;
    _info->readpos = 0;
    _info->plugin = &plugin;

    DB_FILE *file = deadbeef->fopen (it->fname);
    if (!file) {
        trace ("psf: failed to fopen %s\n", it->fname);
        return -1;
    }

    info->filesize = deadbeef->fgetlength (file);
    info->filebuffer = malloc (info->filesize);
    if (!info->filebuffer) {
		fprintf(stderr, "psf: could not allocate %d bytes of memory\n", (int)info->filesize);
		deadbeef->fclose (file);
        return -1;
    }

	if (deadbeef->fread(info->filebuffer, 1, info->filesize, file) != info->filesize) {
		fprintf(stderr, "psf: file read error: %s\n", it->fname);
		deadbeef->fclose (file);
        return -1;
    }
    deadbeef->fclose (file);

    info->type = ao_identify (info->filebuffer);
    if (info->type < 0) {
        fprintf (stderr, "psf: ao_identify failed\n");
        return -1;
    }

    info->decoder = ao_start (info->type, it->fname, (uint8 *)info->filebuffer, info->filesize);
    if (!info->decoder) {
        fprintf (stderr, "psf: ao_start failed\n");
        return -1;
    }

    return 0;
}

static void
aoplug_free (DB_fileinfo_t *_info) {
    aoplug_info_t *info = (aoplug_info_t *)_info;
    if (info) {
        if (info->filebuffer) {
            ao_stop (info->type, info->decoder);
            free (info->filebuffer);
            info->filebuffer = NULL;
        }
        free (info);
    }
}

static int
aoplug_read_int16 (DB_fileinfo_t *_info, char *bytes, int size) {
//    printf ("aoplug_read_int16 %d samples\n", size/4);
    aoplug_info_t *info = (aoplug_info_t *)_info;

    int initsize = size;

    while (size > 0) {
        if (info->remaining > 0) {
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
    info->currentsample += size / (_info->channels * _info->bps/8);
    return initsize-size;
}

static int
aoplug_seek_sample (DB_fileinfo_t *_info, int sample) {
    aoplug_info_t *info = (aoplug_info_t *)_info;
    
    info->currentsample = sample;
    _info->readpos = (float)sample / _info->samplerate;
    return 0;
}

static int
aoplug_seek (DB_fileinfo_t *_info, float time) {
    return aoplug_seek_sample (_info, time * _info->samplerate);
}

static DB_playItem_t *
aoplug_insert (DB_playItem_t *after, const char *fname) {
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("psf: failed to fopen %s\n", fname);
        return NULL;
    }

    size_t size = deadbeef->fgetlength (fp);
    char *buffer = malloc (size);
    if (!buffer) {
        deadbeef->fclose (fp);
		fprintf(stderr, "psf: could not allocate %d bytes of memory\n", (int)size);
        return NULL;
    }

	if (deadbeef->fread(buffer, 1, size, fp) != size) {
        deadbeef->fclose (fp);
		fprintf(stderr, "psf: file read error: %s\n", fname);
        return NULL;
    }

    deadbeef->fclose (fp);

    int type = ao_identify (buffer);
    if (type < 0) {
        free (buffer);
        return NULL;
    }

    void *dec = ao_start (type, fname, (uint8*)buffer, size);
    if (!dec) {
        free (buffer);
        return NULL;
    }
    ao_display_info info;
    int have_info = 0;
    if (ao_get_info (type, dec, &info) == AO_SUCCESS) {
        have_info = 1;
    }

    ao_stop (type, dec);
    dec = NULL;

	free (buffer);
	
    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    const char *ext = fname + strlen (fname);
    while (*ext != '.' && ext > fname) {
        ext--;
    }
    if (*ext == '.') {
        ext++;
        if (!strcasecmp (ext, "psf")) {
            it->filetype = filetypes[0];
        }
        else if (!strcasecmp (ext, "psf2")) {
            it->filetype = filetypes[1];
        }
        else if (!strcasecmp (ext, "spu")) {
            it->filetype = filetypes[2];
        }
        else if (!strcasecmp (ext, "ssf")) {
            it->filetype = filetypes[3];
        }
        else if (!strcasecmp (ext, "dsf") || !strcasecmp (ext, "ninidsf")) {
            it->filetype = filetypes[5];
        }
        else if (!strcasecmp (ext, "qsf")) {
            it->filetype = filetypes[4];
        }
    }
    else {
        it->filetype = filetypes[0];
    }

    float duration = 120;

    if (have_info) {
        int i;
        for (i = 1; i < 9; i++) {
            if (!strncasecmp (info.title[i], "Length: ", 8)) {
                int min, sec;
                if (sscanf (info.info[i], "%d:%d", &min, &sec) == 2) {
                    duration = min * 60 + sec;
                }
            }
            else if (!strncasecmp (info.title[i], "Name: ", 6) || !strncasecmp (info.title[i], "Song: ", 6)) {
                deadbeef->pl_add_meta (it, "title", info.info[i]);
            }
            else if (!strncasecmp (info.title[i], "Game: ", 6)) {
                deadbeef->pl_add_meta (it, "album", info.info[i]);
            }
            else if (!strncasecmp (info.title[i], "Artist: ", 8)) {
                deadbeef->pl_add_meta (it, "artist", info.info[i]);
            }
            else if (!strncasecmp (info.title[i], "Copyright: ", 11)) {
                deadbeef->pl_add_meta (it, "copyright", info.info[i]);
            }
            else if (!strncasecmp (info.title[i], "Year: ", 6)) {
                deadbeef->pl_add_meta (it, "date", info.info[i]);
            }
            else if (!strncasecmp (info.title[i], "Year: ", 6)) {
                deadbeef->pl_add_meta (it, "date", info.info[i]);
            }
            char s[1024];
            snprintf (s, sizeof (s), "%s%s", info.title[i], info.info[i]);
            deadbeef->pl_append_meta (it, "comment", s);
        }
    }
    deadbeef->pl_set_item_duration (it, duration);
    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

static int
aoplug_start (void) {
    return 0;
}

static int
aoplug_stop (void) {
    return 0;
}

static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "psf",
    .plugin.name = "Audio Overload plugin",
    .plugin.descr = "psf, psf2, spu, ssf, minidsf player based on Audio Overload library",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = aoplug_start,
    .plugin.stop = aoplug_stop,
    .open = aoplug_open,
    .init = aoplug_init,
    .free = aoplug_free,
    .read_int16 = aoplug_read_int16,
    .seek = aoplug_seek,
    .seek_sample = aoplug_seek_sample,
    .insert = aoplug_insert,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
ao_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

