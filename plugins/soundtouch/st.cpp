/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "SoundTouch.h"
#include "st.h"

using namespace soundtouch;

void*
st_alloc (void) {
    return new SoundTouch ();
}

void
st_free (void *st) {
    delete (SoundTouch *)st;
}

void
st_set_rate (void *st, float r) {
    ((SoundTouch *)st)->setRate (r);
}

void
st_set_tempo (void *st, float t) {
    ((SoundTouch *)st)->setTempo (t);
}

void
st_set_rate_change (void *st, float r) {
    ((SoundTouch *)st)->setRateChange (r);
}

void
st_set_tempo_change (void *st, float t) {
    ((SoundTouch *)st)->setTempoChange (t);
}

void
st_set_pitch (void *st, float p) {
    ((SoundTouch *)st)->setPitch (p);
}

void
st_set_pitch_octaves (void *st, float po) {
    ((SoundTouch *)st)->setPitchOctaves (po);
}

void
st_set_pitch_semi_tones (void *st, float p) {
    ((SoundTouch *)st)->setPitchSemiTones (p);
}

void
st_set_channels (void *st, int c) {
    ((SoundTouch *)st)->setChannels (c);
}

void
st_set_sample_rate (void *st, int r) {
    ((SoundTouch *)st)->setSampleRate (r);
}

void
st_flush (void *st) {
    ((SoundTouch *)st)->flush ();
}

void
st_put_samples (void *st, float *samples, int nsamples) {
    ((SoundTouch *)st)->putSamples (samples, nsamples);
}

void
st_clear (void *st) {
    ((SoundTouch *)st)->clear ();
}

void
st_set_setting (void *st, int id, int value) {
    ((SoundTouch *)st)->setSetting (id, value);
}

int
st_get_setting (void *st, int id) {
    return ((SoundTouch *)st)->getSetting (id);
}

unsigned int
st_num_unprocessed_samples (void *st) {
    return ((SoundTouch *)st)->numUnprocessedSamples ();
}

unsigned int
st_receive_samples (void *st, float *out, unsigned int max_samples) {
    return ((SoundTouch *)st)->receiveSamples (out, max_samples);
}
