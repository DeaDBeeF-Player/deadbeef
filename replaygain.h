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
#ifndef __REPLAYGAIN_H
#define __REPLAYGAIN_H

#include "deadbeef.h"

void
replaygain_apply (ddb_waveformat_t *fmt, playItem_t *it, char *bytes, int bytesread);

void
replaygain_set (int mode, int scale);

void
replaygain_set_values (float albumgain, float albumpeak, float trackgain, float trackpeak);

void
apply_replay_gain_int8 (playItem_t *it, char *bytes, int size);

void
apply_replay_gain_int16 (playItem_t *it, char *bytes, int size);

void
apply_replay_gain_int24 (playItem_t *it, char *bytes, int size);

void
apply_replay_gain_int32 (playItem_t *it, char *bytes, int size);

void
apply_replay_gain_float32 (playItem_t *it, char *bytes, int size);

#endif
