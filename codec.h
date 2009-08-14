/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __CODEC_H
#define __CODEC_H

#include <stdint.h>
//#include "playlist.h"

typedef struct {
    int bitsPerSample;
    int channels;
    int samplesPerSecond;
    float position;
} fileinfo_t;

struct playItem_s;

typedef struct codec_s {
    int (*init) (struct playItem_s *it);
    void (*free) (void);
    // player is responsible for starting next song if -1 is returned
    int (*read) (char *bytes, int size);
    int (*seek) (float time);
    struct playItem_s * (*insert) (struct playItem_s *after, const char *fname); // after==NULL means "prepend to beginning"
    const char ** (*getexts) (void);
    int (*numvoices) (void);
    void (*mutevoice) (int voice, int mute);
    const char *id; // codec id used for playlist serialization
    const char *filetypes[20]; // NULL terminated array of const strings, representing supported file types (can be NULL)
    fileinfo_t info;
} codec_t;

codec_t *get_codec_for_file (const char *fname);

void
codec_init_locking (void);

void
codec_free_locking (void);

void
codec_lock (void);

void
codec_unlock (void);

#endif	// __CODEC_H
