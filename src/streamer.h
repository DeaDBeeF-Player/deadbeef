/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  streamer implementation

  Copyright (C) 2009-2017 Oleksiy Yakovenko

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
#ifndef __STREAMER_H
#define __STREAMER_H

#include "playlist.h"
#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

// events to pass to streamer thread
enum {
    STR_EV_PLAY_TRACK_IDX, // p1 = idx, or -1 to stop
    STR_EV_PLAY_CURR, // will play the current streamer track (playing_track), see more details in streamer_play_current_track
    STR_EV_NEXT, // streamer_move_to_nextsong
    STR_EV_PREV, // streamer_move_to_prevsong
    STR_EV_RAND, // streamer_move_to_randomsong
    STR_EV_SEEK, // streamer_set_seek; p1: float pos
    STR_EV_SET_CURR_PLT, // streamer_set_current_playlist
    STR_EV_DSP_RELOAD, // reload dsp settings
    STR_EV_SET_DSP_CHAIN, // set new dsp chain
    STR_EV_TRACK_DELETED, // sent if a track, or multiple tracks, get deleted from playlist, or a playlist itself gets deleted
};

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

// song == -1 means "stop and clear streamer message queue"
void
streamer_set_nextsong (int song, int startpaused);

playItem_t *
streamer_get_current_track_to_play (playlist_t *plt);

// returns next track according to repeat and shuffle settings, with specified direction
playItem_t *
streamer_get_next_track_with_direction (int dir, ddb_shuffle_t shuffle, ddb_repeat_t repeat);

void
streamer_set_last_played (playItem_t *track);

void
streamer_set_seek (float pos);

int
streamer_ok_to_read (int len);

float
streamer_get_playpos (void);

void
streamer_song_removed_notify (playItem_t *it);

playItem_t *
streamer_get_streaming_track (void);

playItem_t *
streamer_get_playing_track_unsafe (void);

playItem_t *
streamer_get_playing_track (void);

playItem_t *
streamer_get_buffering_track (void);

void
streamer_configchanged (void);

// if paused -- resume
// else, if have cursor track -- stop current, play cursor
// else, play next
void
streamer_play_current_track (void);

void
streamer_set_bitrate (int bitrate);

int
streamer_get_apx_bitrate (void);

// returns -1 if there's no next song, or playlist finished
// reason 0 means "prev song finished", 1 means "interrupt"
int
streamer_move_to_nextsong (int r);

int
streamer_move_to_prevsong (int r);

int
streamer_move_to_randomsong (int r);

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

int
streamer_dsp_chain_save (void);

void
audio_get_waveform_data (int type, float *data);

void
streamer_set_streamer_playlist (playlist_t *plt);

struct handler_s *
streamer_get_handler (void);

void
streamer_set_playing_track (playItem_t *it);

// sets a callback function, which would be called before applying software volume
void
streamer_set_volume_modifier (float (*modifier) (float delta_time));

void
streamer_set_buffering_track (playItem_t *it);

// force streamer to flush its msg queue
void
streamer_yield (void);

void
streamer_set_output (DB_output_t *output);

void
streamer_notify_track_deleted (void);

#ifdef __cplusplus
}
#endif

#endif // __STREAMER_H
