/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
// if the image is not available immediately -- callback will be called later,
// which will redraw all tracks matching the query
//
// if cover art loader plugin is not available -- NULL will be returned
GdkPixbuf *
get_cover_art (const char *fname, const char *artist, const char *album, int width);

void
coverart_reset_queue (void);

void
cover_art_init (void);

void
cover_art_free (void);

#endif

