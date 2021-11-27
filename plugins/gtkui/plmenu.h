/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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

#include "../../deadbeef.h"
#include "trkproperties.h"

/// This must be called before terminating the app, to ensure global variables are freed.
void
plmenu_free (void);

// Run context menu for selected items of the specified playlist and iter.
void
list_context_menu (ddb_playlist_t *playlist, int iter);

/// For use cases when playlist is not available, such as medialib
void
list_context_menu_with_track_list (ddb_playItem_t **tracks, int count, trkproperties_delegate_t *delegate);

/// For use case when the caller needs more control of the menu, such as Playlist Tab context menu
void
trk_context_menu_build (GtkWidget *menu, ddb_playItem_t *selected_track, int selected_count, ddb_action_context_t action_context);

/// Add plugin action items to the existing menu.
/// @return The number of items added
int
trk_menu_add_action_items(GtkWidget *menu, int selected_count, ddb_playItem_t *selected_track);

#endif /* plmenu_h */
