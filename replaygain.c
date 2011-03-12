
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
#include "playlist.h"
#include "volume.h"
#include "replaygain.h"

static int conf_replaygain_mode = 0;
static int conf_replaygain_scale = 1;
static float conf_replaygain_preamp = 0;

static float rg_albumgain = 1;
static float rg_albumpeak = 1;
static float rg_trackgain = 1;
static float rg_trackpeak = 1;
static float rg_albumgain_preamp = 1;
static float rg_trackgain_preamp = 1;

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
replaygain_set (int mode, int scale, float preamp) {
    conf_replaygain_mode = mode;
    conf_replaygain_scale = scale;
    conf_replaygain_preamp = db_to_amp (preamp);
    rg_albumgain_preamp = rg_albumgain * conf_replaygain_preamp;
    rg_trackgain_preamp = rg_trackgain * conf_replaygain_preamp;
}

void
replaygain_set_values (float albumgain, float albumpeak, float trackgain, float trackpeak) {
    rg_albumgain = db_to_amp (albumgain);
    rg_trackgain = db_to_amp (trackgain);
    rg_albumgain_preamp = rg_albumgain * conf_replaygain_preamp;
    rg_trackgain_preamp = rg_trackgain * conf_replaygain_preamp;
    rg_albumpeak = albumpeak;
    rg_trackpeak = trackpeak;
}

static inline int
get_int_volume (void) {
    int vol = 1000;
    if (conf_replaygain_mode == 1) {
        if (rg_trackgain == 1) {
            return -1;
        }
        vol = rg_trackgain_preamp * 1000;
        if (conf_replaygain_scale) {
            if (vol * rg_trackpeak > 1000) {
                vol = 1000 / rg_trackpeak;
            }
        }
    }
    else if (conf_replaygain_mode == 2) {
        if (rg_albumgain == 1) {
            return -1;
        }
        vol = rg_albumgain_preamp * 1000;
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
        if (sample > 0x7fffffff) {
            sample = 0x7fffffff;
        }
        else if (sample < -0x80000000) {
            sample = -0x80000000;
        }
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
            return;
        }
        vol = rg_trackgain_preamp;
        if (conf_replaygain_scale) {
            if (vol * rg_trackpeak > 1.f) {
                vol = 1.f / rg_trackpeak;
            }
        }
    }
    else if (conf_replaygain_mode == 2) {
        if (rg_albumgain == 1) {
            return;
        }
        vol = rg_albumgain_preamp;
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
