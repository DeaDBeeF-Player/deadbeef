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

#ifndef __deadbeef__sort__
#define __deadbeef__sort__

#ifdef __cplusplus
extern "C" {
#endif

#include "playlist.h"

void
plt_sort_v2 (playlist_t *plt, int iter, int id, const char *format, int order);

void
plt_sort_v3 (ddb_tf_context_t *tf_ctx, const char *tf_bytecode, int iter, int id, int order);

void
sort_track_array (playlist_t *playlist, playItem_t **tracks, int num_tracks, const char *format, int order);

void
plt_autosort (playlist_t *plt);

#ifdef __cplusplus
}
#endif

#endif /* defined(__deadbeef__sort__) */
