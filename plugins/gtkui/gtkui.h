/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __GTKUI_H
#define __GTKUI_H

#define PL_HEAD(iter) (deadbeef->pl_get_first(iter))
#define PL_TAIL(iter) (deadbeef->pl_get_last(iter))
#define PL_NEXT(it, iter) (deadbeef->pl_get_next(it, iter))
#define PL_PREV(it, iter) (deadbeef->pl_get_prev(it, iter))
#define SELECTED(it) (deadbeef->pl_is_selected(it))
#define SELECT(it, sel) (deadbeef->pl_set_selected(it,sel))
#define VSELECT(it, sel) {deadbeef->pl_set_selected(it,sel);gtk_pl_redraw_item_everywhere (it);}
#define REF(it) {if (it) deadbeef->pl_item_ref (it);}
#define UNREF(it) {if (it) deadbeef->pl_item_unref (it);}

#if HAVE_NOTIFY
#define NOTIFY_DEFAULT_FORMAT "%a - %t"
#endif

extern DB_functions_t *deadbeef;

struct _GSList;

// misc utility functions

void
gtkui_add_dirs (struct _GSList *lst);

void
gtkui_add_files (struct _GSList *lst);

void
gtkui_open_files (struct _GSList *lst);

void
gtkui_receive_fm_drop (char *mem, int length, int drop_y);

// plugin configuration dialogs

void
plugin_configure (GtkWidget *parentwin, DB_plugin_t *p);

void
preferences_fill_soundcards (void);

#endif
