/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __PLAYBACK_H
#define __PLAYBACK_H

#if USE_SDL
#include "psdl.h"
#define p_init psdl_init
#define p_free psdl_free
#define p_play psdl_play
#define p_stop psdl_stop
#define p_ispaused psdl_ispaused
#define p_pause psdl_pause
#define p_unpause psdl_unpause
#define p_set_volume psdl_set_volume
#define p_get_rate psdl_get_rate
#else
#include "palsa.h"
#define p_init palsa_init
#define p_free palsa_free
#define p_play palsa_play
#define p_stop palsa_stop
#define p_ispaused palsa_ispaused
#define p_pause palsa_pause
#define p_unpause palsa_unpause
#define p_set_volume palsa_set_volume
#define p_get_volume palsa_get_volume
#define p_get_rate palsa_get_rate
#define p_isstopped palsa_isstopped
#endif

#endif // __PLAYBACK_H
