/*
    Hotkeys plugin for DeaDBeeF
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2012-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
#ifndef __HOTKEYS_H
#define __HOTKEYS_H

#include <deadbeef/deadbeef.h>

typedef struct DB_hotkeys_plugin_s {
    DB_misc_t misc;
    const char *(*get_name_for_keycode) (int keycode);
    void (*reset) (void);
    // since plugin version 1.1
    DB_plugin_action_t* (*get_action_for_keycombo) (int key, int mods, int isglobal, int *ctx);
} DB_hotkeys_plugin_t;

#endif
