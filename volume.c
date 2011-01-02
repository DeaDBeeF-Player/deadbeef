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
#include <math.h>
#include <stdio.h>
#include "volume.h"
#include "conf.h"

#define VOLUME_MIN (-50.f)

static float volume_db = 0; // in dB
static float volume_amp = 1; // amplitude [0..1]

void
volume_set_db (float dB) {
    if (dB < VOLUME_MIN) {
        dB = VOLUME_MIN;
    }
    if (dB > 0) {
        dB = 0;
    }
    conf_set_float ("playback.volume", dB);
    volume_db = dB;
    volume_amp = dB > VOLUME_MIN ? db_to_amp (dB) : 0;
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
    conf_set_float ("playback.volume", volume_db);
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
    return exp(ln10*dB/20.f);
}

float
amp_to_db (float amp) {
    return 20*log10 (amp);
}

float
volume_get_min_db (void) {
    return VOLUME_MIN;
}
