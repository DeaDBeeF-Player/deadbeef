/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
