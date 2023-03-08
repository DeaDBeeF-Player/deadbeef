/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  replaygain support

  Copyright (C) 2009-2016 Oleksiy Yakovenko

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

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#include <string.h>
#include <stdlib.h>
#include "streamer.h"
#include "volume.h"
#include "replaygain.h"
#include "conf.h"
#include <deadbeef/common.h>
#include "playmodes.h"
#include "plmeta.h"

static ddb_replaygain_settings_t current_settings;

void
replaygain_apply_with_settings (ddb_replaygain_settings_t *settings, ddb_waveformat_t *fmt, char *bytes, int numbytes) {
    if (settings->processing_flags == 0) {
        return;
    }
    if (fmt->bps == 16) {
        apply_replay_gain_int16 (settings, bytes, numbytes);
    }
    else if (fmt->bps == 24) {
        apply_replay_gain_int24 (settings, bytes, numbytes);
    }
    else if (fmt->bps == 8) {
        apply_replay_gain_int8 (settings, bytes, numbytes);
    }
    else if (fmt->bps == 32 && !fmt->is_float) {
        apply_replay_gain_int32 (settings, bytes, numbytes);
    }
    else if (fmt->bps == 32 && fmt->is_float) {
        apply_replay_gain_float32 (settings, bytes, numbytes);
    }
}

void
replaygain_apply (ddb_waveformat_t *fmt, char *bytes, int numbytes) {
    replaygain_apply_with_settings(&current_settings, fmt, bytes, numbytes);
}

void
replaygain_set_current (ddb_replaygain_settings_t *settings) {
    memcpy (&current_settings, settings, sizeof (ddb_replaygain_settings_t));
}

void
replaygain_init_settings (ddb_replaygain_settings_t *settings, playItem_t *it) {
    memset (((char *)settings) + sizeof (settings->_size), 0, settings->_size - sizeof (settings->_size));
    settings->source_mode = conf_get_int ("replaygain.source_mode", 0);
    settings->processing_flags = conf_get_int ("replaygain.processing_flags", 0);
    settings->preamp_with_rg = db_to_amp (conf_get_float ("replaygain.preamp_with_rg", 0));
    settings->preamp_without_rg = db_to_amp (conf_get_float ("replaygain.preamp_without_rg", 0));
    settings->albumgain = 1;
    settings->trackgain = 1;
    settings->albumpeak = 1;
    settings->trackpeak = 1;

    if (!it) {
        return;
    }

    pl_lock ();
    const char *albumgain = pl_find_meta (it, ":REPLAYGAIN_ALBUMGAIN");
    const char *trackgain = pl_find_meta (it, ":REPLAYGAIN_TRACKGAIN");
    const char *albumpeak = pl_find_meta (it, ":REPLAYGAIN_ALBUMPEAK");
    const char *trackpeak = pl_find_meta (it, ":REPLAYGAIN_TRACKPEAK");

    if (settings->processing_flags & DDB_RG_PROCESSING_GAIN) {
        if (albumgain) {
            settings->albumgain = db_to_amp((float)atof (albumgain));
            settings->has_album_gain = 1;
        }
        else if (trackgain) {
            settings->albumgain = db_to_amp((float)atof (trackgain));
            settings->has_album_gain = 1;
        }

        if (trackgain) {
            settings->trackgain = db_to_amp((float)atof (trackgain));
            settings->has_track_gain = 1;
        }
        else if (albumgain) {
            settings->trackgain = db_to_amp((float)atof (albumgain));
            settings->has_track_gain = 1;
        }
    }

    if (settings->processing_flags & DDB_RG_PROCESSING_PREVENT_CLIPPING) {
        if (albumpeak) {
            settings->albumpeak = (float)atof (albumpeak);
        }
        else if (trackpeak) {
            settings->albumpeak = (float)atof (trackpeak);
        }

        if (trackpeak) {
            settings->trackpeak = (float)atof (trackpeak);
        }
        else if (albumpeak) {
            settings->trackpeak = (float)atof (albumpeak);
        }
    }

    pl_unlock ();
}

static int
_get_source_mode (int mode) {
    if (mode != DDB_RG_SOURCE_MODE_PLAYBACK_ORDER) {
        return mode;
    }
    ddb_shuffle_t shuffle = streamer_get_shuffle ();
    if (shuffle == DDB_SHUFFLE_ALBUMS || shuffle == DDB_SHUFFLE_OFF) {
        return DDB_RG_SOURCE_MODE_ALBUM;
    }
    else {
        return DDB_RG_SOURCE_MODE_TRACK;
    }
}

static inline int
get_int_volume (ddb_replaygain_settings_t *settings) {
    int vol = 1000;

    int mode = _get_source_mode (settings->source_mode);
    switch (mode) {
    case DDB_RG_SOURCE_MODE_TRACK:
        if (!settings->has_track_gain) {
            vol = (int)(settings->preamp_without_rg * 1000);
        } else {
            vol = (int)(settings->preamp_with_rg * settings->trackgain * 1000);
        }
        if (settings->processing_flags & DDB_RG_PROCESSING_PREVENT_CLIPPING) {
            if (vol * settings->trackpeak > 1000) {
                vol = (int)(1000 / settings->trackpeak);
            }
        }
        break;
    case DDB_RG_SOURCE_MODE_ALBUM:
        if (!settings->has_album_gain) {
            vol = (int)(settings->preamp_without_rg *  1000);
        } else {
            vol = (int)(settings->preamp_with_rg * settings->albumgain * 1000);
        }
        if (settings->processing_flags & DDB_RG_PROCESSING_PREVENT_CLIPPING) {
            if (vol * settings->albumpeak > 1000) {
                vol = (int)(1000 / settings->albumpeak);
            }
        }
        break;
    default:
        break;
    }
    return vol == 1000 ? -1 : vol;
}

void
apply_replay_gain_int8 (ddb_replaygain_settings_t *settings, char *bytes, int size) {
    int vol = get_int_volume (settings);
    if (vol < 0) {
        return;
    }
    int8_t *s = (int8_t*)bytes;
    for (int j = 0; j < size; j++) {
        int32_t sample = ((int8_t)(*s)) * vol / 1000;
        if (sample > 0x7f) {
            sample = 0x7f;
        }
        else if (sample < -0x80) {
            sample = -0x80;
        }
        *s = (int8_t)sample;
        s++;
    }
}

void
apply_replay_gain_int16 (ddb_replaygain_settings_t *settings, char *bytes, int size) {
    int vol = get_int_volume (settings);
    if (vol < 0) {
        return;
    }
    int16_t *s = (int16_t*)bytes;
    for (int j = 0; j < size/2; j++) {
        int32_t sample = ((int32_t)(*s)) * vol / 1000;
        if (sample > 0x7fff) {
            sample = 0x7fff;
        }
        else if (sample < -0x8000) {
            sample = -0x8000;
        }
        *s = (int16_t)sample;
        s++;
    }
}

void
apply_replay_gain_int24 (ddb_replaygain_settings_t *settings, char *bytes, int size) {
    int64_t vol = get_int_volume (settings);
    if (vol < 0) {
        return;
    }
    char *s = (char*)bytes;
    for (int j = 0; j < size/3; j++) {
        int32_t sample = ((unsigned char)s[0]) | ((unsigned char)s[1]<<8) | ((signed char)s[2]<<16);
        sample = (int32_t)(sample * vol / 1000);
        if (sample > 0x7fffff) {
            sample = 0x7fffff;
        }
        else if (sample < -0x800000) {
            sample = -0x800000;
        }
        s[0] = (sample&0x0000ff);
        s[1] = (sample&0x00ff00)>>8;
        s[2] = (sample&0xff0000)>>16;
        s += 3;
    }
}

void
apply_replay_gain_int32 (ddb_replaygain_settings_t *settings, char *bytes, int size) {
    int64_t vol = get_int_volume (settings);
    if (vol < 0) {
        return;
    }
    int32_t *s = (int32_t*)bytes;
    for (int j = 0; j < size/4; j++) {
        int64_t sample = ((int32_t)(*s)) * vol / 1000;
        *s = (int32_t)sample;
        s++;
    }
}

void
apply_replay_gain_float32 (ddb_replaygain_settings_t *settings, char *bytes, int size) {
    float vol = 1.f;
    int mode = _get_source_mode (settings->source_mode);
    switch (mode) {
    case DDB_RG_SOURCE_MODE_TRACK:
        if (!settings->has_track_gain) {
            vol = settings->preamp_without_rg;
        } else {
            vol = settings->preamp_with_rg * settings->trackgain;
        }
        if (settings->processing_flags & DDB_RG_PROCESSING_PREVENT_CLIPPING) {
            if (vol * settings->trackpeak > 1.f) {
                vol = 1.f / settings->trackpeak;
            }
        }
        break;
    case DDB_RG_SOURCE_MODE_ALBUM:
        if (!settings->has_album_gain) {
            vol = settings->preamp_without_rg;
        } else {
            vol = settings->preamp_with_rg * settings->albumgain;
        }
        if (settings->processing_flags & DDB_RG_PROCESSING_PREVENT_CLIPPING) {
            if (vol * settings->albumpeak > 1.f) {
                vol = 1.f / settings->albumpeak;
            }
        }
        break;
    default:
        break;
    }

    if (vol == 1) {
        return;
    }

    float *s = (float*)bytes;
    for (int j = 0; j < size/4; j++) {
        float sample = ((float)*s) * vol;
        if (sample > 1.f) {
            sample = 1.f;
        }
        else if (sample < -1.f) {
            sample = -1.f;
        }
        *s = sample;
        s++;
    }
}
