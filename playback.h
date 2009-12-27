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

#define p_init plug_get_output ()->init
#define p_free plug_get_output ()->free
#define p_play plug_get_output ()->play
#define p_stop plug_get_output ()->stop
#define p_pause plug_get_output ()->pause
#define p_unpause plug_get_output ()->unpause
#define p_get_rate plug_get_output ()->samplerate
#define p_get_state plug_get_output ()->state

#define p_isstopped() (plug_get_output ()->state () == OUTPUT_STATE_STOPPED)
#define p_ispaused() (plug_get_output ()->state () == OUTPUT_STATE_PAUSED)

#endif // __PLAYBACK_H
