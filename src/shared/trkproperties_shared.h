/*
 DeaDBeeF -- the music player
 Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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


#ifndef ____trkproperties_shared__
#define ____trkproperties_shared__

#include <deadbeef/deadbeef.h>

extern const char *trkproperties_types[];
extern const char *trkproperties_hc_props[];

int
trkproperties_build_key_list (const char ***pkeys, int props, DB_playItem_t **tracks, int numtracks);

void
trkproperties_free_track_list (DB_playItem_t ***tracks, int *numtracks);

void
trkproperties_build_track_list_for_ctx (ddb_playlist_t *plt, int ctx, DB_playItem_t ***_tracks, int *_numtracks);

void
trkproperties_reload_tags (DB_playItem_t **tracks, int numtracks);

int
trkproperties_get_field_value (char *out, int size, const char *key, DB_playItem_t **tracks, int numtracks);

#endif /* defined(____trkproperties_shared__) */
