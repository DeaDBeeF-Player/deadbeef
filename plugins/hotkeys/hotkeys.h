/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __HOTKEYS_H
#define __HOTKEYS_H

#include "../../deadbeef.h"

typedef struct DB_hotkeys_plugin_s {
    DB_misc_t misc;
    const char *(*get_name_for_keycode) (int keycode);
    void (*reset) (void);
    // since plugin version 1.1
    DB_plugin_action_t* (*get_action_for_keycombo) (int key, int mods, int isglobal, int *ctx);
} DB_hotkeys_plugin_t;

#endif
