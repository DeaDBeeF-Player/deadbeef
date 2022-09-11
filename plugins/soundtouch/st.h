/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
#ifndef __ST_H
#define __ST_H

//////////////////////////////////
// C-wrapper for soundtouch class

/// Enable/disable anti-alias filter in pitch transposer (0 = disable)
#define SETTING_USE_AA_FILTER       0

/// Pitch transposer anti-alias filter length (8 .. 128 taps, default = 32)
#define SETTING_AA_FILTER_LENGTH    1

/// Enable/disable quick seeking algorithm in tempo changer routine
/// (enabling quick seeking lowers CPU utilization but causes a minor sound
///  quality compromising)
#define SETTING_USE_QUICKSEEK       2

/// Time-stretch algorithm single processing sequence length in milliseconds. This determines 
/// to how long sequences the original sound is chopped in the time-stretch algorithm. 
/// See "STTypes.h" or README for more information.
#define SETTING_SEQUENCE_MS         3

/// Time-stretch algorithm seeking window length in milliseconds for algorithm that finds the 
/// best possible overlapping location. This determines from how wide window the algorithm 
/// may look for an optimal joining location when mixing the sound sequences back together. 
/// See "STTypes.h" or README for more information.
#define SETTING_SEEKWINDOW_MS       4

/// Time-stretch algorithm overlap length in milliseconds. When the chopped sound sequences 
/// are mixed back together, to form a continuous sound stream, this parameter defines over 
/// how long period the two consecutive sequences are let to overlap each other. 
/// See "STTypes.h" or README for more information.
#define SETTING_OVERLAP_MS          5

#ifdef __cplusplus
extern "C" {
#endif

void*
st_alloc (void);

void
st_free (void *st);

void
st_set_rate (void *st, float r);

void
st_set_tempo (void *st, float t);

void
st_set_rate_change (void *st, float r);

void
st_set_tempo_change (void *st, float t);

void
st_set_pitch (void *st, float p);

void
st_set_pitch_octaves (void *st, float po);

void
st_set_pitch_semi_tones (void *st, float p);

void
st_set_channels (void *st, int c);

void
st_set_sample_rate (void *st, int r);

void
st_flush (void *st);

void
st_put_samples (void *st, float *samples, int nsamples);

void
st_clear (void *st);

void
st_set_setting (void *st, int id, int value);

int
st_get_setting (void *st, int id);

unsigned int
st_num_unprocessed_samples (void *st);

unsigned int
st_receive_samples (void *st, float *out, unsigned int max_samples);

#ifdef __cplusplus
}
#endif

#endif
