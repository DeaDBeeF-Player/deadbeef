/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#ifndef __TRKPROPERTIES_H
#define __TRKPROPERTIES_H

#include "../../deadbeef.h"

// list store column meanings (for Metadata and Properties, also used by Selection properties widget)
enum {
    META_COL_TITLE = 0,
    META_COL_DISPLAY_VAL = 1,
    META_COL_KEY = 2,
    META_COL_IS_MULT = 3,
    META_COL_VALUE = 4,
    META_COL_PANGO_WEIGHT = 5,
    META_COL_COUNT
};

struct DB_playItem_s;

typedef struct {
    void (*trkproperties_did_update_tracks)(void *user_data);
    void (*trkproperties_did_reload_metadata)(void *user_data);
    void (*trkproperties_did_delete_files)(void *user_data, int cancelled);
    void *user_data;
} trkproperties_delegate_t;

void
show_track_properties_dlg (int ctx, ddb_playlist_t *plt);

/// Sets the delegate for the currently displayed track properties dialog.
/// The next call of @c show_track_properties_dlg or similar will reset the delegate to NULL.
void
trkproperties_set_delegate (trkproperties_delegate_t *delegate);

void
trkproperties_destroy (void);

void
trkproperties_fill_metadata (void);

int
build_key_list (const char ***pkeys, int props, DB_playItem_t **tracks, int numtracks);

void
trkproperties_fill_meta (GtkListStore *store, DB_playItem_t **tracks, int numtracks);

void
trkproperties_fill_prop (GtkListStore *propstore, DB_playItem_t **tracks, int numtracks);

void
add_field_section(GtkListStore *store, const char *title, const char *value);

void
show_track_properties_dlg_with_track_list (ddb_playItem_t **track_list, int count);

#endif
