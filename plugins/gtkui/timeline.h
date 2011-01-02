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
#ifndef __TIMELINE_H
#define __TIMELINE_H

#include <stdint.h>

typedef struct {
    float fps;
    float duration;
    float progress;
    struct timeval time;
    intptr_t tid;
    int stop;
    int destroy;
    int (*callback)(float _progress, int _last, void *_ctx);
    void *callback_ctx;
} timeline_t;

// callback must return 0 to continue, or -1 to abort
timeline_t *
timeline_create (void);

void
timeline_free (timeline_t *timeline, int wait);

void
timeline_stop (timeline_t *tl, int wait);

void
timeline_init (timeline_t *timeline, float seconds, float fps, int (*callback)(float _progress, int _last, void *_ctx), void *ctx);

void
timeline_start (timeline_t *timeline);

#endif // __TIMELINE_H
