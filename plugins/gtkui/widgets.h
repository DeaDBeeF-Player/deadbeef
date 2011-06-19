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
#ifndef __WIDGETS_H
#define __WIDGETS_H

#include "gtkui_api.h"

void
w_init (void);

void
w_free (void);

ddb_gtkui_widget_t *
w_get_rootwidget (void);

void
w_set_design_mode (int active);

void
w_reg_widget (const char *type, ddb_gtkui_widget_t *(*create_func) (void));

void
w_unreg_widget (const char *type);

ddb_gtkui_widget_t *
w_create (const char *type);

void
w_destroy (ddb_gtkui_widget_t *w);

void
w_append (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

void
w_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child);

ddb_gtkui_widget_t *
w_hsplitter_create (void);

ddb_gtkui_widget_t *
w_create_vsplitter (void);

ddb_gtkui_widget_t *
w_create_box (void);

ddb_gtkui_widget_t *
w_create_tabstrip (void);

ddb_gtkui_widget_t *
w_tabbed_playlist_create (void);

ddb_gtkui_widget_t *
w_create_playlist (void);
#endif
