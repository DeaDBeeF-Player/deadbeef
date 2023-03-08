/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#ifndef plmenu_h
#define plmenu_h

#include <deadbeef/deadbeef.h>
#include "trkproperties.h"

/// This must be called before terminating the app, to ensure global variables are freed.
void
plmenu_free (void);

// Run context menu for selected items of the specified playlist and iter.
void
list_context_menu (ddb_playlist_t *playlist, int iter);

/// For use cases when track list is dynamic, such as medialib
void
list_context_menu_with_dynamic_track_list (ddb_playlist_t *playlist, trkproperties_delegate_t *delegate);

/// For use case when the caller needs more control of the menu, such as Playlist Tab context menu
void
trk_context_menu_build (GtkWidget *menu);

/// Call this before the @c trk_context_menu_build to initialize the internal track list
void
trk_context_menu_update_with_playlist (ddb_playlist_t *playlist, ddb_action_context_t action_context);

#endif /* plmenu_h */
