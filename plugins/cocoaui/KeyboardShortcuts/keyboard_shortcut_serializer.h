/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2024 Oleksiy Yakovenko and other contributors

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

#ifndef keyboard_shortcut_serializer_h
#define keyboard_shortcut_serializer_h

#include "keyboard_shortcuts.h"

char *
ddb_keyboard_shortcuts_save (ddb_keyboard_shortcut_t *root);

int
ddb_keyboard_shortcuts_load (ddb_keyboard_shortcut_t *root, const char *json_string);

#endif /* keyboard_shortcut_serializer_h */
