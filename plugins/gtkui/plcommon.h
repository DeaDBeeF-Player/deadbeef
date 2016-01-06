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

#ifndef __PLCOLUMNS_H
#define __PLCOLUMNS_H

#include "ddblistview.h"

int
rewrite_column_config (DdbListview *listview, const char *name);

void
draw_album_art (DdbListview *listview, cairo_t *drawable, DdbListviewIter group_it, int column, int group_pinned, int grp_next_y, int x, int y, int width, int height);

void
draw_column_data (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int idx, int column, int iter, int x, int y, int width, int height);

void
list_context_menu (DdbListview *listview, DdbListviewIter it, int idx);

int
load_column_config (DdbListview *listview, const char *key);

void
add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, int align_right);

GtkWidget*
create_headermenu (int groupby);

void
set_last_playlist_cm (DdbListview *pl);

void
set_active_column_cm (int col);

void
pl_common_init(void);

void
pl_common_free (void);

int
pl_common_get_group (DdbListview *listview, DdbListviewIter it, char *str, int size);

void
pl_common_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int iter, int x, int y, int width, int height);

// import old playlist configuration from "playlist.%02d" syntax with old title
// formatting to the new JSON syntax with new title formatting
int
import_column_config_0_6 (const char *oldkeyprefix, const char *newkey);

#endif // __PLCOLUMNS_H
