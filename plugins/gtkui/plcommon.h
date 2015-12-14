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

#define COLUMN_FORMAT_ARTISTALBUM "$if(%artist%,%artist%,Unknown Artist)[ - %album%]"
#define COLUMN_FORMAT_ARTIST "$if(%artist%,%artist%,Unknown Artist)"
#define COLUMN_FORMAT_ALBUM "%album%"
#define COLUMN_FORMAT_TITLE "%title%"
#define COLUMN_FORMAT_LENGTH "%length%"
#define COLUMN_FORMAT_TRACKNUMBER "%track number%"
#define COLUMN_FORMAT_BAND "$if(%album artist%,%album artist%,Unknown Artist)"

int
pl_common_rewrite_column_config (DdbListview *listview, const char *name);

int
pl_common_is_album_art_column (void *user_data);

void
pl_common_draw_album_art (DdbListview *listview, cairo_t *cr, DB_playItem_t *it, void *user_data, int pinned, int next_y, int x, int y, int width, int height);

void
pl_common_draw_column_data (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int iter, int align, void *user_data, GdkColor *fg_clr, int x, int y, int width, int height);

void
pl_common_list_context_menu (DdbListview *listview, DdbListviewIter it, int idx);

int
pl_common_load_column_config (DdbListview *listview, const char *key);

void
pl_common_add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, int align_right);

void
pl_common_header_context_menu (DdbListview *ps, int column);

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

void
pl_common_selection_changed (DdbListview *ps, int iter, DB_playItem_t *it);

void
pl_common_col_sort (int sort_order, int iter, void *user_data);

void
pl_common_set_group_format (DdbListview *listview, char *format_conf);

#endif // __PLCOLUMNS_H
