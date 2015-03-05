/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __PLCOLUMNS_H
#define __PLCOLUMNS_H

#include "ddblistview.h"

int
rewrite_column_config (DdbListview *listview, const char *name);

void draw_column_data (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, DdbListviewIter group_it, int column, int group_y, int group_height, int group_pinned, int grp_next_y, int x, int y, int width, int height);

void
list_context_menu (DdbListview *listview, DdbListviewIter it, int idx);

void
append_column_from_textdef (DdbListview *listview, const uint8_t *def);

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
pl_common_get_group (DdbListviewIter it, char *str, int size);

void
pl_common_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int x, int y, int width, int height);

#endif // __PLCOLUMNS_H
