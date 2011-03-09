/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "ayemu.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static const char * exts[] = { "vtx", NULL };
static const char *filetypes[] = { "VTX", NULL };

#define AY_FRAME_SIZE 14

typedef struct {
    DB_fileinfo_t info;
    ayemu_vtx_t *decoder;
    ayemu_ay_t ay;
    char regs[AY_FRAME_SIZE];
    int vtx_pos;
    int left;
    int rate;
    int currentsample;
} vtx_info_t;

static DB_fileinfo_t *
vtx_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (vtx_info_t));
    memset (_info, 0, sizeof (vtx_info_t));
    return _info;
}

static int
vtx_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    // prepare to decode the track
    // return -1 on failure
    vtx_info_t *info = (vtx_info_t *)_info;
    
    size_t sz = 0;
    char *buf = NULL;

    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    if (!fp) {
        trace ("vtx: failed to open file %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }

    sz = deadbeef->fgetlength (fp);
    if (sz <= 0) {
        trace ("vtx: bad file size\n");
        return -1;
    }

    buf = malloc (sz);
    if (!buf) {
        trace ("vtx: out of memory\n");
        return -1;
    }
    if (deadbeef->fread (buf, 1, sz, fp) != sz) {
        trace ("vtx: read failed\n");
        free (buf);
        return -1;
    }

    info->decoder = ayemu_vtx_load (buf, sz);
    if (!info->decoder) {
        trace ("vtx: ayemu_vtx_load failed\n");
        free (buf);
        return -1;
    }
    trace ("vtx: data=%p, size=%d\n", info->decoder->regdata, info->decoder->regdata_size);

    free (buf);

    ayemu_init (&info->ay);
    ayemu_set_chip_type (&info->ay, info->decoder->chiptype, NULL);
    ayemu_set_chip_freq (&info->ay, info->decoder->chipFreq);
    ayemu_set_stereo (&info->ay, info->decoder->stereo, NULL);

    int samplerate = deadbeef->conf_get_int ("synth.samplerate", 44100);

    info->left = 0;
    info->vtx_pos = 0;
    _info->plugin = &plugin;
    _info->fmt.bps = deadbeef->conf_get_int ("vtx.bps", 16);;
    if (_info->fmt.bps != 16 && _info->fmt.bps != 8) {
        _info->fmt.bps = 16;
    }
    _info->fmt.channels = 2;
    _info->fmt.samplerate = samplerate;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    _info->readpos = 0;

    ayemu_set_sound_format (&info->ay, samplerate, _info->fmt.channels, _info->fmt.bps);

    info->rate = _info->fmt.channels * _info->fmt.bps / 8;
    return 0;
}

static void
vtx_free (DB_fileinfo_t *_info) {
    // free everything allocated in _init
    vtx_info_t *info = (vtx_info_t *)_info;
    if (_info) {
        if (info->decoder) {
            ayemu_vtx_free (info->decoder);
            info->decoder = NULL;
        }
        ayemu_reset (&info->ay);
        free (_info);
    }
}

/** Get next 14-bytes frame of AY register data.
 *
 * Return value: 1 on success, 0 on eof
 */
static int
ayemu_vtx_get_next_frame (vtx_info_t *info)
{
    int numframes = info->decoder->regdata_size / AY_FRAME_SIZE;
    if (info->vtx_pos++ >= numframes) {
        return 0;
    }
    int n;
    char *p = info->decoder->regdata + info->vtx_pos;
    for(n = 0 ; n < AY_FRAME_SIZE ; n++, p += numframes) {
        info->regs[n] = *p;
    }
    return 1;
}

static int
vtx_read (DB_fileinfo_t *_info, char *bytes, int size) {
    // try decode `size' bytes
    // return number of decoded bytes
    // return 0 on EOF
    vtx_info_t *info = (vtx_info_t *)_info;
    int initsize = size;
    int donow = 0;

    while (size > 0) {
        if (info->left > 0) {
            donow = (size > info->left) ? info->left : size;
            info->left -= donow;
            bytes = ayemu_gen_sound (&info->ay, (char *)bytes, donow);
            size -= donow;
        }
        else {
            if ((ayemu_vtx_get_next_frame (info) == 0)) {
                break; // eof
            }
            else {
                // number of samples it current frame
                info->left = _info->fmt.samplerate / info->decoder->playerFreq;
                // mul by rate to get number of bytes;
                info->left *= info->rate;
                ayemu_set_regs (&info->ay, info->regs);
                donow = 0;
            }
        }
    }
    info->currentsample += (initsize - size) / 4;
    _info->readpos = (float)info->currentsample / _info->fmt.samplerate;
    return initsize - size;
}

static int
vtx_seek_sample (DB_fileinfo_t *_info, int sample) {
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
    vtx_info_t *info = (vtx_info_t *)_info;

    // get frame
    int num_frames = info->decoder->regdata_size / AY_FRAME_SIZE;
    int samples_per_frame = _info->fmt.samplerate / info->decoder->playerFreq;

    // start of frame
    info->vtx_pos = sample / samples_per_frame;
    if (info->vtx_pos >= num_frames) {
        return -1; // eof
    }
    // copy register data
    int n;
    char *p = info->decoder->regdata + info->vtx_pos;
    for(n = 0 ; n < AY_FRAME_SIZE ; n++, p += num_frames) {
        info->regs[n] = *p;
    }
    // set number of bytes left in frame
    info->left = _info->fmt.samplerate / info->decoder->playerFreq - (sample % samples_per_frame);
    // mul by rate to get number of bytes
    info->left *= info->rate;
    info->currentsample = sample;
    _info->readpos = (float)info->currentsample / _info->fmt.samplerate;

    return 0;
}

static int
vtx_seek (DB_fileinfo_t *_info, float time) {
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    return vtx_seek_sample (_info, time * _info->fmt.samplerate);
}

static DB_playItem_t *
vtx_insert (DB_playItem_t *after, const char *fname) {
    // read information from the track
    // load/process cuesheet if exists
    // insert track into playlist
    // return track pointer on success
    // return NULL on failure

    trace ("vtx_insert %s\n");
    // load header
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("vtx: failed to open file\n");
        return NULL;
    }
    char buf[0x4000];
    size_t sz;
    sz = deadbeef->fread (buf, 1, sizeof (buf), fp);
    deadbeef->fclose (fp);
    if (sz <= 0) {
        trace ("vtx: failed to read header data from file\n");
        return NULL;
    }
    ayemu_vtx_t *hdr = ayemu_vtx_header (buf, sz);
    if (!hdr) {
        trace ("vtx: failed to read header\n");
        return NULL;
    }
    trace ("vtx: datasize: %d\n", hdr->regdata_size);

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    it->filetype = filetypes[0];

    int numframes = hdr->regdata_size / AY_FRAME_SIZE;
//    int totalsamples = numframes * hdr->playerFreq;
    trace ("vtx: numframes=%d, playerFreq=%d\n", numframes, hdr->playerFreq);
    deadbeef->pl_set_item_duration (it, (float)numframes / hdr->playerFreq);

    // add metadata
    deadbeef->pl_add_meta (it, "title", hdr->title);
    deadbeef->pl_add_meta (it, "artist", hdr->author);
    deadbeef->pl_add_meta (it, "album", hdr->from);

    ayemu_vtx_free (hdr);
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

static int
vtx_start (void) {
    // do one-time plugin initialization here
    // e.g. starting threads for background processing, subscribing to events, etc
    // return 0 on success
    // return -1 on failure
    return 0;
}
static int
vtx_stop (void) {
    // undo everything done in _start here
    // return 0 on success
    // return -1 on failure
    return 0;
}

static const char settings_dlg[] =
    "property \"Bits Per Sample (8 or 16)\" entry vtx.bps 16;\n"
;

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "vtx",
    .plugin.name = "VTX decoder",
    .plugin.descr = "AY8910/12 chip emulator and vtx file player",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses libayemu - AY/YM sound chip emulator and music file loader\n"
        "Copyright (C) 2003-2004 Sashnov Alexander\n"
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
    .plugin.start = vtx_start,
    .plugin.stop = vtx_stop,
    .plugin.configdialog = settings_dlg,
    .open = vtx_open,
    .init = vtx_init,
    .free = vtx_free,
    .read = vtx_read,
    .seek = vtx_seek,
    .seek_sample = vtx_seek_sample,
    .insert = vtx_insert,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
vtx_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
