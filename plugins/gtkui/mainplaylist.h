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

#ifndef __MAINPLAYLIST_H
#define __MAINPLAYLIST_H

#include "ddblistview.h"

void
main_playlist_init (GtkWidget *widget);

void
main_playlist_free (void);

void
main_refresh (void);

int
main_get_idx (DdbListviewIter it);

void
main_drag_n_drop (DdbListviewIter before, DdbPlaylistHandle from_playlist, uint32_t *indices, int length, int copy);

#endif
