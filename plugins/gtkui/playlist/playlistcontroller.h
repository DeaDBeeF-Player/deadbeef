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

#ifndef playlistcontroller_h
#define playlistcontroller_h

#include "ddblistview.h"

typedef struct playlist_controller_s playlist_controller_t;

playlist_controller_t *
playlist_controller_new(DdbListview *listview, gboolean is_search);

void
playlist_controller_free(playlist_controller_t *ctl);

void
playlist_controller_init (playlist_controller_t *ctl, gboolean show_headers, int width);

void
playlist_controller_message (playlist_controller_t *ctl, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);

#endif /* playlistcontroller_h */
