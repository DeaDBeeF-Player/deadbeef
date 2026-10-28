/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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

#ifndef __TRKPROPERTIES_ARTWORK_H
#define __TRKPROPERTIES_ARTWORK_H

#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>

/// Creates the "Artwork" tab, and appends it to the track properties @c notebook.
/// The tab is destroyed together with the notebook.
void
trkproperties_artwork_create_tab (GtkNotebook *notebook);

/// Loads and displays the covers embedded in the track.
/// Cancels the previous loading, if it's still in progress.
/// If @c count is not 1, the covers are not loaded, and a message is displayed instead.
void
trkproperties_artwork_reload (ddb_playItem_t **tracks, int count);

/// Cancels loading of the covers, and ignores the results of the queries which are already running.
void
trkproperties_artwork_cancel (void);

#endif
