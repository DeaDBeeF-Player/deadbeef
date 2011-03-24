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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../deadbeef.h"
#include "adplug.h"
#include "emuopl.h"
#include "silentopl.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

int _Unwind_Resume_or_Rethrow;
int _Unwind_RaiseException;
int _Unwind_GetLanguageSpecificData;
int _Unwind_Resume;
int _Unwind_DeleteException;
int _Unwind_GetTextRelBase;
int _Unwind_SetIP;
int _Unwind_GetDataRelBase;
int _Unwind_GetRegionStart;
int _Unwind_SetGR;
int _Unwind_GetIPInfo;

extern "C" {

extern DB_decoder_t adplug_plugin;
DB_functions_t *deadbeef;

const char *adplug_exts[] = { "A2M", "ADL", "AMD", "BAM", "CFF", "CMF", "D00", "DFM", "DMO", "DRO", "DTM", "HSC", "HSP", "IMF", "KSM", "LAA", "LDS", "M", "MAD", "MKJ", "MSC", "MTK", "RAD", "RAW", "RIX", "ROL", "S3M", "SA2", "SAT", "SCI", "SNG", "XAD", "XMS", "XSM", "JBM", NULL };

const char *adplug_filetypes[] = { "A2M", "ADL", "AMD", "BAM", "CFF", "CMF", "D00", "DFM", "DMO", "DRO", "DTM", "HSC", "HSP", "IMF", "KSM", "LAA", "LDS", "M", "MAD", "MKJ", "MSC", "MTK", "RAD", "RAW", "RIX", "ROL", "S3M", "SA2", "SAT", "SCI", "SNG", "XAD", "XMS", "XSM", "JBM", NULL };


typedef struct {
    DB_fileinfo_t info;
    CEmuopl *opl;
    CPlayer *decoder;
    int totalsamples;
    int currentsample;
    int subsong;
    int toadd;
} adplug_info_t;

DB_fileinfo_t *
adplug_open (uint32_t hints) {
    adplug_info_t *info = (adplug_info_t *)malloc (sizeof (adplug_info_t));
    DB_fileinfo_t *_info = (DB_fileinfo_t *)info;
    memset (info, 0, sizeof (adplug_info_t));
    return _info;
}

int
adplug_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    // prepare to decode the track
    // return -1 on failure
    adplug_info_t *info = (adplug_info_t *)_info;

    int samplerate = deadbeef->conf_get_int ("synth.samplerate", 44100);
    int bps = 16; // NOTE: there's no need to support 8bit input, because adplug simply downgrades 16bit signal to 8bits
    int channels = 2;
    info->opl = new CEmuopl (samplerate, bps == 16 ? true : false, channels == 2);
//    opl->settype (Copl::TYPE_OPL2);
    info->decoder = CAdPlug::factory (deadbeef->pl_find_meta (it, ":URI"), info->opl, CAdPlug::players);
    if (!info->decoder) {
        trace ("adplug: failed to open %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }

    info->subsong = deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0);
    info->decoder->rewind (info->subsong);
    float dur = deadbeef->pl_get_item_duration (it);
    info->totalsamples = dur * samplerate;
    info->currentsample = 0;
    info->toadd = 0;

    // fill in mandatory plugin fields
    _info->plugin = &adplug_plugin;
    _info->fmt.bps = bps;
    _info->fmt.channels = channels;
    _info->fmt.samplerate = samplerate;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    _info->readpos = 0;

    trace ("adplug_init ok (songlength=%d, duration=%f, totalsamples=%d)\n", info->decoder->songlength (info->subsong), deadbeef->pl_get_item_duration (it), info->totalsamples);

    return 0;
}

void
adplug_free (DB_fileinfo_t *_info) {
    // free everything allocated in _init
    if (_info) {
        adplug_info_t *info = (adplug_info_t *)_info;
        if (info->decoder) {
            delete info->decoder;
        }
        if (info->opl) {
            delete info->opl;
        }
        free (info);
    }
}

int
adplug_read (DB_fileinfo_t *_info, char *bytes, int size) {
    // try decode `size' bytes
    // return number of decoded bytes
    // return 0 on EOF
    adplug_info_t *info = (adplug_info_t *)_info;
    bool playing = true;
    int i;
    int sampsize = (_info->fmt.bps / 8) * _info->fmt.channels;

    if (info->currentsample + size/sampsize >= info->totalsamples) {
        // clip
        size = (info->totalsamples - info->currentsample) * sampsize;
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
      while (info->toadd < 0)
      {
        info->toadd += _info->fmt.samplerate;
        playing = info->decoder->update ();
//        decoder->time_ms += 1000 / plr.p->getrefresh ();
      }
      i = min (towrite, (long) (info->toadd / info->decoder->getrefresh () + sampsize) & ~(sampsize-1));
      info->opl->update ((short *) sndbufpos, i);
      sndbufpos += i * sampsize;
      size -= i * sampsize;
      info->currentsample += i;
      towrite -= i;
      info->toadd -= (long) (info->decoder->getrefresh () * i);
    }
    info->currentsample += size/4;
    _info->readpos = (float)info->currentsample / _info->fmt.samplerate;
    return initsize-size;
}

int
adplug_seek_sample (DB_fileinfo_t *_info, int sample) {
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
    adplug_info_t *info = (adplug_info_t *)_info;
    if (sample >= info->totalsamples) {
        trace ("adplug: seek outside bounds (%d of %d)\n", sample, info->totalsamples);
        return -1;
    }

    info->decoder->rewind (info->subsong);
    info->currentsample = 0;

    while (info->currentsample < sample) {
        info->decoder->update ();
        int framesize = _info->fmt.samplerate / info->decoder->getrefresh ();
        info->currentsample += framesize;
    }

    if (info->currentsample >= info->totalsamples) {
        return -1;
    }

    info->toadd = 0;
    trace ("adplug: new position after seek: %d of %d\n", info->currentsample, info->totalsamples);
    
    _info->readpos = (float)info->currentsample / _info->fmt.samplerate;

    return 0;
}

int
adplug_seek (DB_fileinfo_t *_info, float time) {
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    return adplug_seek_sample (_info, time * _info->fmt.samplerate);
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

static void
adplug_add_meta (DB_playItem_t *it, const char *key, const char *value) {
    if (!value) {
        return;
    }
    const char *charset = deadbeef->junk_detect_charset (value);
    if (charset) {
        int l = strlen (value);
        char str[1024];
        deadbeef->junk_recode (value, l, str, sizeof (str), charset);
        deadbeef->pl_add_meta (it, key, str);
    }
}

DB_playItem_t *
adplug_insert (DB_playItem_t *after, const char *fname) {
    // read information from the track
    // load/process cuesheet if exists
    // insert track into playlist
    // return track pointer on success
    // return NULL on failure

    trace ("adplug: trying to insert %s\n", fname);

    CSilentopl opl;
    CPlayer *p = CAdPlug::factory (fname, &opl, CAdPlug::players);
    if (!p) {
        trace ("adplug: failed to open %s\n", fname);
        return NULL;
    }

    int subsongs = p->getsubsongs ();
    for (int i = 0; i < subsongs; i++) {
        // prepare track for addition
        float dur = p->songlength (i)/1000.f;
        if (dur < 0.1) {
            continue;
        }
        DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, adplug_plugin.plugin.id);
        deadbeef->pl_add_meta (it, ":FILETYPE", adplug_get_extension (fname));
        deadbeef->pl_set_meta_int (it, ":TRACKNUM", i);
        deadbeef->pl_set_item_duration (it, dur);
#if 0
        // add metainfo
        if (p->gettitle()[0]) {
            adplug_add_meta (it, "title", p->gettitle());
        }
        else {
            deadbeef->pl_add_meta (it, "title", NULL);
        }
        if (p->getdesc()[0]) {
            adplug_add_meta (it, "comment", p->getdesc());
        }
        if (!p->getauthor()[0]) {
            adplug_add_meta (it, "artist", p->getauthor());
        }
#endif
        deadbeef->pl_add_meta (it, "title", NULL);
        // insert
        after = deadbeef->pl_insert_item (after, it);
        deadbeef->pl_item_unref (it);
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
