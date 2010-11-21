/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __SRC_H
#define __SRC_H

typedef struct {
    DB_dsp_t dsp;
    void (*reset) (DB_dsp_instance_t *inst, int full);
    void (*set_ratio) (DB_dsp_instance_t *inst, float ratio);
} ddb_dsp_src_t;

#if 0
void
ddb_src_reset (ddb_src_t *src, int full);

void
ddb_src_confchanged (ddb_src_t *src);

int
ddb_src_process (ddb_src_t *_src, const char * restrict input, int nframes, char * restrict output, int buffersize, float ratio, int nchannels);
#endif

#endif
