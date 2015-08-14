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

#ifndef __SEARCH_H
#define __SEARCH_H

#include "ddblistview.h"

extern struct playItem_s *search_current;
extern int search_count;

void
search_start (void);

void
search_destroy (void);

// should be called whenever playlist was changed
void
search_refresh (void);

void
search_redraw (void);

int
search_get_idx (DdbListviewIter it);

void
search_playlist_init (GtkWidget *widget);

#endif // __SEARCH_H
