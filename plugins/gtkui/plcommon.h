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

typedef struct {
    int id;
    char *format;
    char *bytecode;
    int cover_size;
    int new_cover_size;
    int cover_load_timeout_id;
    DdbListview *listview;
} col_info_t;

int
rewrite_column_config (DdbListview *listview, const char *name);

int
is_album_art_column (void *user_data);

void
draw_album_art (DdbListview *listview, cairo_t *cr, DB_playItem_t *it, void *user_data, int pinned, int next_y, int x, int y, int width, int height);

void
draw_column_data (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int column, int iter, int x, int y, int width, int height);

void
list_context_menu (DdbListview *listview, DdbListviewIter it, int idx);

int
load_column_config (DdbListview *listview, const char *key);

void
add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, int align_right);

GtkWidget*
create_headermenu (DdbListview *listview, int groupby);

void
set_last_playlist_cm (DdbListview *pl);

void
set_active_column_cm (int col);

void
pl_common_init(void);

void
pl_common_free (void);

void
pl_common_free_col_info (void *data);

int
pl_common_get_group (DdbListview *listview, DdbListviewIter it, char *str, int size);

void
pl_common_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int iter, int x, int y, int width, int height);

#endif // __PLCOLUMNS_H
