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
#include "playlist.h"
#include "volume.h"
#include "replaygain.h"

static int conf_replaygain_mode = 0;
static int conf_replaygain_scale = 1;
static float conf_replaygain_preamp = 0;
static float conf_global_preamp = 0;

static float rg_albumgain = 1;
static float rg_albumpeak = 1;
static float rg_trackgain = 1;
static float rg_trackpeak = 1;
static float rg_albumgain_full_preamp = 1;
static float rg_trackgain_full_preamp = 1;
static float rg_albumgain_global_preamp = 1;
static float rg_trackgain_global_preamp = 1;

void
replaygain_apply (ddb_waveformat_t *fmt, playItem_t *it, char *bytes, int bytesread) {
    // FIXME: separate replaygain DSP plugin?
    if (fmt->bps == 16) {
        apply_replay_gain_int16 (it, bytes, bytesread);
    }
    else if (fmt->bps == 24) {
        apply_replay_gain_int24 (it, bytes, bytesread);
    }
    else if (fmt->bps == 8) {
        apply_replay_gain_int16 (it, bytes, bytesread);
    }
    else if (fmt->bps == 32 && !fmt->is_float) {
        apply_replay_gain_int32 (it, bytes, bytesread);
    }
    else if (fmt->bps == 32 && fmt->is_float) {
        apply_replay_gain_float32 (it, bytes, bytesread);
    }
}

void
replaygain_set (int mode, int scale, float preamp, float global_preamp) {
    conf_replaygain_mode = mode;
    conf_replaygain_scale = scale;
    conf_replaygain_preamp = db_to_amp (preamp);
    conf_global_preamp = db_to_amp (global_preamp);
    rg_albumgain_full_preamp = rg_albumgain * conf_replaygain_preamp * conf_global_preamp;
    rg_trackgain_full_preamp = rg_trackgain * conf_replaygain_preamp * conf_global_preamp;
    rg_albumgain_global_preamp = rg_albumgain * conf_global_preamp;
    rg_trackgain_global_preamp = rg_trackgain * conf_global_preamp;
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
    rg_albumgain = db_to_amp (albumgain);
    rg_trackgain = db_to_amp (trackgain);
    rg_albumgain_full_preamp = rg_albumgain * conf_replaygain_preamp * conf_global_preamp;
    rg_trackgain_full_preamp = rg_trackgain * conf_replaygain_preamp * conf_global_preamp;
    rg_albumgain_global_preamp = rg_albumgain * conf_global_preamp;
    rg_trackgain_global_preamp = rg_trackgain * conf_global_preamp;
    rg_albumpeak = albumpeak;
    rg_trackpeak = trackpeak;
}

static inline int
get_int_volume (void) {
    int vol = 1000;
    if (conf_replaygain_mode == 1) {
        if (rg_trackgain == 1) {
            vol = rg_trackgain_global_preamp * 1000;
        } else {
            vol = rg_trackgain_full_preamp * 1000;
        }
        if (conf_replaygain_scale) {
            if (vol * rg_trackpeak > 1000) {
                vol = 1000 / rg_trackpeak;
            }
        }
    }
    else if (conf_replaygain_mode == 2) {
        if (rg_albumgain == 1) {
            vol = rg_albumgain_global_preamp * 1000;
        } else {
            vol = rg_albumgain_full_preamp * 1000;
        }
        if (conf_replaygain_scale) {
            if (vol * rg_albumpeak > 1000) {
                vol = 1000 / rg_albumpeak;
            }
        }
    }
    return vol;
}

void
apply_replay_gain_int8 (playItem_t *it, char *bytes, int size) {
    if (!conf_replaygain_mode) {
        return;
    }
    int vol = get_int_volume ();
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
apply_replay_gain_int16 (playItem_t *it, char *bytes, int size) {
    if (!conf_replaygain_mode) {
        return;
    }
    int vol = get_int_volume ();
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
apply_replay_gain_int24 (playItem_t *it, char *bytes, int size) {
    if (!conf_replaygain_mode) {
        return;
    }
    int64_t vol = get_int_volume ();
    if (vol < 0) {
        return;
    }
    char *s = (char*)bytes;
    for (int j = 0; j < size/3; j++) {
        int32_t sample = ((unsigned char)s[0]) | ((unsigned char)s[1]<<8) | (s[2]<<16);
        sample = sample * vol / 1000;
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
apply_replay_gain_int32 (playItem_t *it, char *bytes, int size) {
    if (!conf_replaygain_mode) {
        return;
    }
    int64_t vol = get_int_volume ();
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
apply_replay_gain_float32 (playItem_t *it, char *bytes, int size) {
    if (!conf_replaygain_mode) {
        return;
    }
    float vol = 1.f;
    if (conf_replaygain_mode == 1) {
        if (rg_trackgain == 1) {
            vol = rg_trackgain_global_preamp;
        } else {
            vol = rg_trackgain_full_preamp;
        }
        if (conf_replaygain_scale) {
            if (vol * rg_trackpeak > 1.f) {
                vol = 1.f / rg_trackpeak;
            }
        }
    }
    else if (conf_replaygain_mode == 2) {
        if (rg_albumgain == 1) {
            vol = rg_albumgain_global_preamp;
        } else {
            vol = rg_albumgain_full_preamp;
        }
        if (conf_replaygain_scale) {
            if (vol * rg_albumpeak > 1.f) {
                vol = 1.f / rg_albumpeak;
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
