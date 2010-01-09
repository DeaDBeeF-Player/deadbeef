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

#include <stdio.h>
#include <string.h>
#include "../../deadbeef.h"
#include "adplug.h"
#include "emuopl.h"
#include "silentopl.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

extern "C" {

extern DB_decoder_t adplug_plugin;
static DB_functions_t *deadbeef;


const char *adplug_exts[] = { "A2M", "ADL", "AMD", "BAM", "CFF", "CMF", "D00", "DFM", "DMO", "DRO", "DTM", "HSC", "HSP", "IMF", "KSM", "LAA", "LDS", "M", "MAD", "MID", "MKJ", "MSC", "MTK", "RAD", "RAW", "RIX", "ROL", "S3M", "SA2", "SAT", "SCI", "SNG", "SNG", "SNG", "XAD", "XMS", "XSM", "JBM", NULL };

const char *adplug_filetypes[] = { "A2M", "ADL", "AMD", "BAM", "CFF", "CMF", "D00", "DFM", "DMO", "DRO", "DTM", "HSC", "HSP", "IMF", "KSM", "LAA", "LDS", "M", "MAD", "MID", "MKJ", "MSC", "MTK", "RAD", "RAW", "RIX", "ROL", "S3M", "SA2", "SAT", "SCI", "SNG", "SNG", "SNG", "XAD", "XMS", "XSM", "JBM", NULL };

static CEmuopl *opl;
static CPlayer *decoder;
static int totalsamples;
static int currentsample;
static int subsong;
static int toadd;

int
adplug_init (DB_playItem_t *it) {
    // prepare to decode the track
    // return -1 on failure

    int samplerate = deadbeef->get_output ()->samplerate ();
    int bps = deadbeef->get_output ()->bitspersample ();
    int channels = 2;
    opl = new CEmuopl (samplerate, true, channels == 2);
//    opl->settype (Copl::TYPE_OPL2);
    decoder = CAdPlug::factory (it->fname, opl, CAdPlug::players);
    if (!decoder) {
        trace ("adplug: failed to open %s\n", it->fname);
        return NULL;
    }

    subsong = it->tracknum;
    decoder->rewind (subsong);
    totalsamples = decoder->songlength (subsong) * samplerate / 1000;
    currentsample = 0;
    toadd = 0;

    // fill in mandatory plugin fields
    adplug_plugin.info.bps = bps;
    adplug_plugin.info.channels = channels;
    adplug_plugin.info.samplerate = samplerate;
    adplug_plugin.info.readpos = 0;

//    trace ("adplug_init ok (duration=%f, totalsamples=%d)\n", deadbeef->pl_get_item_duration (it), totalsamples);

    return 0;
}

void
adplug_free (void) {
    // free everything allocated in _init
    if (decoder) {
        delete decoder;
        decoder = NULL;
    }
    if (opl) {
        delete opl;
        opl = NULL;
    }
}

int
adplug_read_int16 (char *bytes, int size) {
    // try decode `size' bytes
    // return number of decoded bytes
    // return 0 on EOF
    bool playing = true;
    int i;
    int sampsize = (adplug_plugin.info.bps >> 3) * adplug_plugin.info.channels;

    if (currentsample + size/4 >= totalsamples) {
        // clip
        size = (totalsamples - currentsample) * sampsize;
        trace ("adplug: clipped to %d\n", size);
        if (size <= 0) {
            return 0;
        }
    }
    int initsize = size;

    int towrite = size/sampsize;
    char *sndbufpos = bytes;


    while (towrite > 0)
    {
      while (toadd < 0)
      {
        toadd += adplug_plugin.info.samplerate;
        playing = decoder->update ();
//        decoder->time_ms += 1000 / plr.p->getrefresh ();
      }
      i = min (towrite, (long) (toadd / decoder->getrefresh () + 4) & ~3);
      opl->update ((short *) sndbufpos, i);
      sndbufpos += i * sampsize;
      size -= i * sampsize;
      currentsample += i;
      towrite -= i;
      toadd -= (long) (decoder->getrefresh () * i);
    }
    currentsample += size/4;
    adplug_plugin.info.readpos = (float)currentsample / adplug_plugin.info.samplerate;
    return initsize-size;
}

int
adplug_seek_sample (int sample) {
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
    if (sample >= totalsamples) {
        trace ("adplug: seek outside bounds (%d of %d)\n", sample, totalsamples);
        return -1;
    }

    decoder->rewind (subsong);
    currentsample = 0;

    while (currentsample < sample) {
        decoder->update ();
        int framesize = adplug_plugin.info.samplerate / decoder->getrefresh ();
        currentsample += framesize;
    }

    if (currentsample >= totalsamples) {
        return -1;
    }

    toadd = 0;
    trace ("adplug: new position after seek: %d of %d\n", currentsample, totalsamples);
    
    adplug_plugin.info.readpos = (float)currentsample / adplug_plugin.info.samplerate;

    return 0;
}

int
adplug_seek (float time) {
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    return adplug_seek_sample (time * adplug_plugin.info.samplerate);
    return 0;
}

static const char *
adplug_get_extension (const char *fname) {
    const char *e = fname + strlen (fname);
    while (*e != '.' && e != fname) {
        e--;
    }
    if (*e == '.') {
        e++;
        // now find ext in list
        for (int i = 0; adplug_exts[i]; i++) {
            if (!strcasecmp (e, adplug_exts[i])) {
                return adplug_filetypes[i];
            }
        }
    }
    return "adplug-unknown";
}

DB_playItem_t *
adplug_insert (DB_playItem_t *after, const char *fname) {
    // read information from the track
    // load/process cuesheet if exists
    // insert track into playlist
    // return track pointer on success
    // return NULL on failure

    CSilentopl opl;
    CPlayer *p = CAdPlug::factory (fname, &opl, CAdPlug::players);
    if (!p) {
        trace ("adplug: failed to open %s\n", fname);
        return NULL;
    }

    int subsongs = p->getsubsongs ();
    for (int i = 0; i < subsongs; i++) {
        // prepare track for addition
        DB_playItem_t *it = deadbeef->pl_item_alloc ();
        it->decoder = &adplug_plugin;
        it->fname = strdup (fname);
        it->filetype = adplug_get_extension (fname);
        it->tracknum = i;
        deadbeef->pl_set_item_duration (it, p->songlength (i)/1000.f);
        // add metainfo
        if (!p->gettitle().empty()) {
            deadbeef->pl_add_meta (it, "title", p->gettitle().c_str());
        }
        else {
            deadbeef->pl_add_meta (it, "title", NULL);
        }
        if (!p->getdesc().empty()) {
            deadbeef->pl_add_meta (it, "comment", p->getdesc().c_str());
        }
        if (!p->getauthor().empty()) {
            deadbeef->pl_add_meta (it, "artist", p->getauthor().c_str());
        }
        // insert
        after = deadbeef->pl_insert_item (after, it);
    }

    // free decoder
    delete p;

    // now the track is ready, insert into playlist
    return after;
}

int
adplug_start (void) {
    // do one-time plugin initialization here
    // e.g. starting threads for background processing, subscribing to events, etc
    // return 0 on success
    // return -1 on failure
    return 0;
}

int
adplug_stop (void) {
    // undo everything done in _start here
    // return 0 on success
    // return -1 on failure
    return 0;
}

DB_plugin_t *
adplug_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&adplug_plugin);
}

}
