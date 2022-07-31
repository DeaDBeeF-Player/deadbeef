/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  pcm volume manipulation routines

  Copyright (C) 2009-2013 Oleksiy Yakovenko

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
#include <math.h>
#include <stdio.h>
#include "volume.h"
#include "conf.h"

#define VOLUME_MIN (-50.f)

static float volume_db = 0; // in dB
static float volume_amp = 1; // amplitude [0..1]
static int audio_mute = 0;

void
volume_set_db (float dB) {
    if (dB < VOLUME_MIN) {
        dB = VOLUME_MIN;
    }
    if (dB > 0) {
        dB = 0;
    }
    volume_amp = dB > VOLUME_MIN ? db_to_amp (dB) : 0;
    volume_db = dB;
    audio_mute = 0;
    conf_set_float ("playback.volume.normalized", volume_amp);
}

float
volume_get_db (void) {
    return volume_db;
}

void
volume_set_amp (float amp) {
    if (amp < 0) {
        amp = 0;
    }
    if (amp > 1) {
        amp = 1;
    }
    volume_amp = amp;
    volume_db = amp > 0 ? amp_to_db (amp) : VOLUME_MIN;

    if (volume_db < VOLUME_MIN) {
        volume_db = VOLUME_MIN;
    }
    else if (volume_db > 0) {
        volume_db = 0;
    }

    audio_mute = 0;
    conf_set_float ("playback.volume.normalized", volume_amp);
}

float
volume_get_amp (void) {
    return volume_amp;
}

float
db_to_amp (float dB) {
//    return pow (10, dB/20.f);
    // thanks to he for this hack
    const float ln10=2.3025850929940002f;
    return (float)exp(ln10*dB/20.f);
}

float
amp_to_db (float amp) {
    return (float)(20*log10 (amp));
}

float
volume_get_min_db (void) {
    return VOLUME_MIN;
}

void
audio_set_mute (int mute) {
    audio_mute = mute;
}

int
audio_is_mute (void) {
    return audio_mute;
}
