/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "deadbeef.h"

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
// 0 stop
// 1 switch to current (gui) playlist, play if not playing
// 2 pause
// 3 play if not playing, don't switch playlist
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

void
streamer_set_bitrate (int bitrate);

int
streamer_get_apx_bitrate (void);

// returns -1 if theres no next song, or playlist finished
// reason 0 means "song finished", 1 means "user clicked next"
int
streamer_move_to_nextsong (int reason);

int
streamer_move_to_prevsong (void);

int
streamer_move_to_randomsong (void);

struct DB_fileinfo_s *
streamer_get_current_fileinfo (void);

void
streamer_set_current_playlist (int plt);

int
streamer_get_current_playlist (void);

// returns track index in current streamer playlist
int
str_get_idx_of (playItem_t *it);

void
streamer_notify_playlist_deleted (playlist_t *plt);

struct ddb_dsp_context_s *
streamer_get_dsp_chain (void);

void
streamer_set_dsp_chain (struct ddb_dsp_context_s *chain);

void
streamer_dsp_refresh (void);

void
streamer_get_output_format (ddb_waveformat_t *fmt);

void
streamer_dsp_postinit (void);

int
streamer_dsp_chain_save (void);

void
streamer_notify_order_changed (int prev_order, int new_order);

int
audio_get_waveform_data (ddb_waveformat_t *fmt, char *data);

#endif // __STREAMER_H
