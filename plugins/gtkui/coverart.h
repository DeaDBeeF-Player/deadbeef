/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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
*/

#ifndef __COVERART_H
#define __COVERART_H

#include <gtk/gtk.h>
#include "../../deadbeef.h"

// these functions put requests for cover art into queue, or return default image
// of specific size
//
// if the image is not available immediately -- callback will be called later
//
// if  artwork plugin is not available -- NULL will be returned and no callbacks made
typedef void (*cover_avail_callback_t) (void *user_data);
GdkPixbuf *
get_cover_art_callb (const char *fname, const char *artist, const char *album, int width, cover_avail_callback_t, void *user_data);
GdkPixbuf *
get_cover_art_primary (const char *fname, const char *artist, const char *album, int width, cover_avail_callback_t, void *user_data);
GdkPixbuf *
get_cover_art_primary_by_size (const char *fname, const char *artist, const char *album, int width, int height, cover_avail_callback_t, void *user_data);
GdkPixbuf *
get_cover_art_thumb (const char *fname, const char *artist, const char *album, int width, cover_avail_callback_t, void *user_data);
GdkPixbuf *
get_cover_art_thumb_by_size (const char *fname, const char *artist, const char *album, int width, int height, cover_avail_callback_t, void *user_data);

void
coverart_reset_queue (void);

void
cover_art_init (void);

void
cover_art_disconnect (void);

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

#endif

