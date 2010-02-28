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
#include <gtk/gtk.h>
#include "coverart.h"
#include "../artwork/artwork.h"

extern DB_artwork_plugin_t *coverart_plugin;

void
cover_avail_callback (const char *artist, const char *album) {
}

static GdkPixbuf *
get_pixbuf (const char *fname, int width) {
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (fname, NULL);
    if (!pixbuf) {
        return NULL;
    }
    int w, h;
    w = gdk_pixbuf_get_width (pixbuf);
    h = gdk_pixbuf_get_height (pixbuf);
    if (w == width) {
        return pixbuf;
    }
    int height;
    if (w > h) {
        height = width * h / w;
    }
    else if (h > w) {
        height = width;
        width = height * w / h;
    }
    else {
        height = width;
    }
    printf ("width=%d/%d, height=%d/%d\n", width, w, height, h);

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
    g_object_unref (pixbuf);
    return scaled;
}

GdkPixbuf *
get_cover_art (DB_playItem_t *it, int width) {
    if (!coverart_plugin) {
        return NULL;
    }
    const char *fname = coverart_plugin->get_album_art (it, cover_avail_callback);
    if (fname) {
        printf ("loading %s\n", fname);
        return get_pixbuf (fname, width);
    }
    return NULL;
}
