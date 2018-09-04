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
#define COLUMN_FORMAT_YEAR "%year%"
#define COLUMN_FORMAT_LENGTH "%length%"
#define COLUMN_FORMAT_TRACKNUMBER "%tracknumber%"
#define COLUMN_FORMAT_BAND "$if(%album artist%,%album artist%,Unknown Artist)"
#define COLUMN_FORMAT_CODEC "%codec%"
#define COLUMN_FORMAT_BITRATE "%bitrate%"

int
pl_common_rewrite_column_config (DdbListview *listview, const char *name);

int
pl_common_is_album_art_column (void *user_data);

void
pl_common_draw_album_art (DdbListview *listview, cairo_t *cr, DB_playItem_t *it, void *user_data, int min_y, int next_y, int x, int y, int width, int height);

void
list_context_menu (DdbListview *listview, DdbListviewIter it, int idx, int iter);

void
list_empty_region_context_menu (DdbListview *listview);

gboolean
list_handle_keypress (DdbListview *ps, int keyval, int state, int iter);

void
pl_common_draw_column_data (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int iter, int align, void *user_data, GdkColor *fg_clr, int x, int y, int width, int height, int even);

int
pl_common_load_column_config (DdbListview *listview, const char *key);

void
pl_common_add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, const char *sort_format, int align_right);

void
pl_common_header_context_menu (DdbListview *ps, int column);

void
pl_common_init(void);

void
pl_common_free (void);

void
pl_common_free_col_info (void *data);

int
pl_common_get_group (DdbListview *listview, DdbListviewIter it, char *str, int size, int index);

void
pl_common_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int iter, int x, int y, int width, int height, int group_depth);

void
pl_common_selection_changed (DdbListview *ps, int iter, DB_playItem_t *it);

void
pl_common_col_sort (int sort_order, int iter, void *user_data);

void
pl_common_set_group_format (DdbListview *listview, const char *format_conf, const char *artwork_level_conf, const char *subgroup_padding_conf);

// import old playlist configuration from "playlist.%02d" syntax with old title
// formatting to the new JSON syntax with new title formatting
int
import_column_config_0_6 (const char *oldkeyprefix, const char *newkey);

int
find_first_preset_column_type (int type);

#endif // __PLCOLUMNS_H
