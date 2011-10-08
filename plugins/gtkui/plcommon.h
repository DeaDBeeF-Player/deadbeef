/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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

typedef struct {
    int id;
    char *format;
} col_info_t;

#define MAX_GROUP_BY_STR 100
extern char group_by_str[MAX_GROUP_BY_STR];

void
write_column_config (const char *name, int idx, const char *title, int width, int align_right, int id, const char *format);

void
rewrite_column_config (DdbListview *listview, const char *name);

void draw_column_data (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, DdbListviewIter group_it, int column, int group_y, int x, int y, int width, int height);

void
list_context_menu (DdbListview *listview, DdbListviewIter it, int idx);

void
header_context_menu (DdbListview *ps, int column);

void
append_column_from_textdef (DdbListview *listview, const uint8_t *def);

void
add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, int align_right);

#endif // __PLCOLUMNS_H
