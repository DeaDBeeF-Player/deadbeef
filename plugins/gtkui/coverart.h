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
#ifndef __COVERART_H
#define __COVERART_H

#include <gtk/gtk.h>
#include "../../deadbeef.h"

// this function puts request for cover art into queue, or returns default image
// of specific size
//
// if the image is not available immediately -- callback will be called later
//
// if cover art loader plugin is not available -- NULL will be returned
GdkPixbuf *
get_cover_art_callb (const char *fname, const char *artist, const char *album, int width, void (*cover_avail_callback) (void *user_data), void *user_data);

void
coverart_reset_queue (void);

void
cover_art_init (void);

void
cover_art_free (void);

// simply inserts callback point into queue
// the callback will be called when the loading queue reaches this request
void
queue_cover_callback (void (*callback)(void *user_data), void *user_data);

GdkPixbuf *
cover_get_default_pixbuf (void);

int
gtkui_is_default_pixbuf (GdkPixbuf *pb);

int
gtkui_cover_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);

#endif

