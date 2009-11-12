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
#ifndef __STREAMER_H
#define __STREAMER_H

#include "playlist.h"

extern playItem_t str_playing_song;
extern playItem_t str_streaming_song;

int
streamer_init (void);

void
streamer_free (void);

int
streamer_read (char *bytes, int size);

void
streamer_reset (int full);

void
streamer_lock (void);

void
streamer_unlock (void);

// pstate indicates what to do with playback
// -1 means "don't do anything"
// -2 means "end of playlist"
// otherwise "set state to this value"
void
streamer_set_nextsong (int song, int pstate);

void
streamer_set_seek (float pos);

int
streamer_get_fill (void);

int
streamer_ok_to_read (int len);

float
streamer_get_playpos (void);

float
streamer_get_bitrate (void);

void
streamer_update_bitrate (float bitrate);

int
streamer_is_buffering (void);

void
streamer_song_removed_notify (playItem_t *it);

playItem_t *
streamer_get_streaming_track (void);

playItem_t *
streamer_get_playing_track (void);

void
streamer_configchanged (void);

void
streamer_play_current_track (void);

#endif // __STREAMER_H
