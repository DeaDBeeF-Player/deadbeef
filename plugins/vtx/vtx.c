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
#include "ayemu.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static const char * exts[] = { "vtx", NULL };
static const char *filetypes[] = { "VTX", NULL };

static ayemu_vtx_t *decoder;
static ayemu_ay_t ay;

#define AY_FRAME_SIZE 14

static char regs[AY_FRAME_SIZE];
static int vtx_pos;
static int left;
static int rate;
static int currentsample;
static int16_t soundbuffer[];

static int
vtx_init (DB_playItem_t *it) {
    // prepare to decode the track
    // return -1 on failure
    
    size_t sz = 0;
    char *buf = NULL;

    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        trace ("vtx: failed to open file %s\n", it->fname);
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

    decoder = ayemu_vtx_load (buf, sz);
    if (!decoder) {
        trace ("vtx: ayemu_vtx_load failed\n");
        free (buf);
        return -1;
    }
    trace ("vtx: data=%p, size=%d\n", decoder->regdata, decoder->regdata_size);

    free (buf);

    ayemu_init (&ay);
    ayemu_set_chip_type (&ay, decoder->chiptype, NULL);
    ayemu_set_chip_freq(&ay, decoder->chipFreq);
    ayemu_set_stereo (&ay, decoder->stereo, NULL);

    int samplerate = deadbeef->get_output ()->samplerate ();

    ayemu_set_sound_format (&ay, samplerate, deadbeef->get_output ()->channels (), deadbeef->get_output ()->bitspersample ());

    left = 0;
    rate = deadbeef->get_output ()->channels () * deadbeef->get_output ()->bitspersample () / 8;
    vtx_pos = 0;
    memset (regs, 0, sizeof (regs));
    plugin.info.bps = deadbeef->get_output ()->bitspersample ();
    plugin.info.channels = deadbeef->get_output ()->channels ();
    plugin.info.samplerate = samplerate;
    plugin.info.readpos = 0;
    return 0;
}

static void
vtx_free (void) {
    // free everything allocated in _init
    if (decoder) {
        ayemu_vtx_free (decoder);
        decoder = NULL;
    }
    ayemu_reset (&ay);
}

/** Get next 14-bytes frame of AY register data.
 *
 * Return value: 1 on success, 0 on eof
 */
static int
ayemu_vtx_get_next_frame (ayemu_vtx_t *vtx, char *regs)
{
    int numframes = vtx->regdata_size / AY_FRAME_SIZE;
    if (vtx_pos++ >= numframes) {
        return 0;
    }
    else {
        int n;
        char *p = vtx->regdata + vtx_pos;
        for(n = 0 ; n < AY_FRAME_SIZE ; n++, p += numframes) {
            regs[n] = *p;
        }
        return 1;
    }
}

static int
vtx_read_int16 (char *bytes, int size) {
    // try decode `size' bytes
    // return number of decoded bytes
    // return 0 on EOF
    int initsize = size;
    int donow = 0;

    while (size > 0) {
        if (left > 0) {
            donow = (size > left) ? left : size;
            left -= donow;
            bytes = ayemu_gen_sound (&ay, (char *)bytes, donow);
            size -= donow;
        }
        else {
            if ((ayemu_vtx_get_next_frame (decoder, regs) == 0)) {
                break; // eof
            }
            else {
                // number of samples it current frame
                left = plugin.info.samplerate / decoder->playerFreq;
                // mul by rate to get number of bytes;
                left *= rate;
                ayemu_set_regs (&ay, regs);
                donow = 0;
            }
        }
    }
    currentsample += (initsize - size) / 4;
    plugin.info.readpos = (float)currentsample / plugin.info.samplerate;
    return initsize - size;
}

static int
vtx_seek_sample (int sample) {
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure

    // get frame
    int num_frames = decoder->regdata_size / AY_FRAME_SIZE;
    int samples_per_frame = plugin.info.samplerate / decoder->playerFreq;

    // start of frame
    vtx_pos = sample / samples_per_frame;
    if (vtx_pos >= num_frames) {
        return -1; // eof
    }
    // copy register data
    int n;
    char *p = decoder->regdata + vtx_pos;
    for(n = 0 ; n < AY_FRAME_SIZE ; n++, p += num_frames) {
        regs[n] = *p;
    }
    // set number of bytes left in frame
    left = plugin.info.samplerate / decoder->playerFreq - (sample % samples_per_frame);
    // mul by rate to get number of bytes
    left *= rate;
    currentsample = sample;
    plugin.info.readpos = (float)currentsample / plugin.info.samplerate;

    return 0;
}

static int
vtx_seek (float time) {
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    return vtx_seek_sample (time * plugin.info.samplerate);
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

    DB_playItem_t *it = deadbeef->pl_item_alloc ();

    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.id);
    it->fname = strdup (fname);
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
// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "VTX decoder",
    .plugin.descr = "AY8910/12 chip emulator and vtx file player",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = vtx_start,
    .plugin.stop = vtx_stop,
    .init = vtx_init,
    .free = vtx_free,
    .read_int16 = vtx_read_int16,
//    .read_float32 = vtx_read_float32,
    .seek = vtx_seek,
    .seek_sample = vtx_seek_sample,
    .insert = vtx_insert,
    .exts = exts,
    .id = "vtx",
    .filetypes = filetypes
};

DB_plugin_t *
vtx_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
