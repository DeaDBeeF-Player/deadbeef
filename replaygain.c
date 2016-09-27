/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  replaygain support

  Copyright (C) 2009-2013 Alexey Yakovenko

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

  Alexey Yakovenko waker@users.sourceforge.net
*/
#include <string.h>
#include "playlist.h"
#include "volume.h"
#include "replaygain.h"
#include "conf.h"

typedef struct ddb_replaygain_params_s {
    ddb_replaygain_settings_t settings;
    float albumgain_full_preamp;
    float trackgain_full_preamp;
    float albumgain_global_preamp;
    float trackgain_global_preamp;
} ddb_replaygain_params_t;

static ddb_replaygain_settings_t current_settings;

void
replaygain_apply_with_settings (ddb_replaygain_settings_t *settings, ddb_waveformat_t *fmt, char *bytes, int numbytes) {
    if (!settings->mode) {
        return;
    }

    ddb_replaygain_params_t params;
    memcpy (&params.settings, settings, sizeof (ddb_replaygain_settings_t));
    params.albumgain_full_preamp = settings->albumgain * settings->preamp * settings->global_preamp;
    params.trackgain_full_preamp = settings->trackgain * settings->preamp * settings->global_preamp;
    params.albumgain_global_preamp = settings->albumgain * settings->global_preamp;
    params.trackgain_global_preamp = settings->trackgain * settings->global_preamp;

    if (fmt->bps == 16) {
        apply_replay_gain_int16 (&params, bytes, numbytes);
    }
    else if (fmt->bps == 24) {
        apply_replay_gain_int24 (&params, bytes, numbytes);
    }
    else if (fmt->bps == 8) {
        apply_replay_gain_int8 (&params, bytes, numbytes);
    }
    else if (fmt->bps == 32 && !fmt->is_float) {
        apply_replay_gain_int32 (&params, bytes, numbytes);
    }
    else if (fmt->bps == 32 && fmt->is_float) {
        apply_replay_gain_float32 (&params, bytes, numbytes);
    }
}

void
replaygain_apply (ddb_waveformat_t *fmt, char *bytes, int numbytes) {
    replaygain_apply_with_settings(&current_settings, fmt, bytes, numbytes);
}

void
replaygain_set (int mode, int scale, float preamp, float global_preamp) {
    current_settings.mode = mode;
    current_settings.scale = scale;
    current_settings.preamp = db_to_amp (preamp);
    current_settings.global_preamp = db_to_amp (global_preamp);
}

void
replaygain_init_settings (ddb_replaygain_settings_t *settings, playItem_t *it) {
    settings->mode = conf_get_int ("replaygain_mode", 0);
    settings->scale = conf_get_int ("replaygain_scale", 1);
    settings->preamp = conf_get_float ("replaygain_preamp", 0);
    settings->global_preamp = conf_get_float ("global_preamp", 0);
    pl_lock ();
    const char *gain;
    gain = pl_find_meta (it, ":REPLAYGAIN_ALBUMGAIN");
    settings->albumgain = gain ? atof (gain) : 1000;
    settings->albumpeak = pl_get_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK);

    gain = pl_find_meta (it, ":REPLAYGAIN_TRACKGAIN");
    settings->trackgain = gain ? atof (gain) : 1000;
    settings->trackpeak = pl_get_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK);
    pl_unlock ();
}

void
replaygain_set_values (float albumgain, float albumpeak, float trackgain, float trackpeak) {
    if (albumgain > 100 && trackgain <= 100) {
        albumgain = trackgain;
        albumpeak = trackpeak;
    }
    else if (albumgain <= 100 && trackgain > 100) {
        trackgain = albumgain;
        trackpeak = albumpeak;
    }
    else if (albumgain > 100 && trackgain > 100) {
        trackgain = albumgain = 0;
    }
    current_settings.albumgain = db_to_amp (albumgain);
    current_settings.trackgain = db_to_amp (trackgain);
    current_settings.albumpeak = albumpeak;
    current_settings.trackpeak = trackpeak;
}

static inline int
get_int_volume (ddb_replaygain_params_t *params) {
    int vol = 1000;
    if (params->settings.mode == 1) {
        if (params->settings.trackgain == 1) {
            vol = params->settings.global_preamp * 1000;
        } else {
            vol = params->trackgain_full_preamp * 1000;
        }
        if (params->settings.scale) {
            if (vol * params->settings.trackpeak > 1000) {
                vol = 1000 / params->settings.trackpeak;
            }
        }
    }
    else if (params->settings.mode == 2) {
        if (params->settings.albumgain == 1) {
            vol = params->albumgain_global_preamp * 1000;
        } else {
            vol = params->albumgain_full_preamp * 1000;
        }
        if (params->settings.scale) {
            if (vol * params->settings.albumpeak > 1000) {
                vol = 1000 / params->settings.albumpeak;
            }
        }
    }
    return vol;
}

void
apply_replay_gain_int8 (ddb_replaygain_params_t *params, char *bytes, int size) {
    int vol = get_int_volume (params);
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
apply_replay_gain_int16 (ddb_replaygain_params_t *params, char *bytes, int size) {
    int vol = get_int_volume (params);
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
apply_replay_gain_int24 (ddb_replaygain_params_t *params, char *bytes, int size) {
    int64_t vol = get_int_volume (params);
    if (vol < 0) {
        return;
    }
    char *s = (char*)bytes;
    for (int j = 0; j < size/3; j++) {
        int32_t sample = ((unsigned char)s[0]) | ((unsigned char)s[1]<<8) | (s[2]<<16);
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
apply_replay_gain_int32 (ddb_replaygain_params_t *params, char *bytes, int size) {
    int64_t vol = get_int_volume (params);
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
apply_replay_gain_float32 (ddb_replaygain_params_t *params, char *bytes, int size) {
    float vol = 1.f;
    if (params->settings.mode == 1) {
        if (params->settings.trackgain == 1) {
            vol = params->trackgain_global_preamp;
        } else {
            vol = params->trackgain_full_preamp;
        }
        if (params->settings.scale) {
            if (vol * params->settings.trackpeak > 1.f) {
                vol = 1.f / params->settings.trackpeak;
            }
        }
    }
    else if (params->settings.mode == 2) {
        if (params->settings.albumgain == 1) {
            vol = params->albumgain_global_preamp;
        } else {
            vol = params->albumgain_full_preamp;
        }
        if (params->settings.scale) {
            if (vol * params->settings.albumpeak > 1.f) {
                vol = 1.f / params->settings.albumpeak;
            }
        }
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
