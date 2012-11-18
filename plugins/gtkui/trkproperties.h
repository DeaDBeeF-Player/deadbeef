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
#ifndef __TRKPROPERTIES_H
#define __TRKPROPERTIES_H

#include "../../deadbeef.h"

struct DB_playItem_s;

void
show_track_properties_dlg (int ctx);

void
trkproperties_destroy (void);

void
trkproperties_fill_metadata (void);

int
build_key_list (const char ***pkeys, int props, DB_playItem_t **tracks, int numtracks);

void
trkproperties_fill_meta (GtkListStore *store, DB_playItem_t **tracks, int numtracks);

#endif
