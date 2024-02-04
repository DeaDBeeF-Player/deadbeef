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

#ifndef keyboard_shortcuts_h
#define keyboard_shortcuts_h

struct ddb_keyboard_shortcut_s;
typedef struct ddb_keyboard_shortcut_s ddb_keyboard_shortcut_t;

ddb_keyboard_shortcut_t *
ddb_keyboard_shortcuts_get_root (void);

void
ddb_keyboard_shortcuts_deinit (void);

void
ddb_keyboard_shortcuts_init_with_config (const char *config_json);

ddb_keyboard_shortcut_t *
ddb_keyboard_shortcut_append (ddb_keyboard_shortcut_t *parent);

ddb_keyboard_shortcut_t *
ddb_keyboard_shortcut_get_children (ddb_keyboard_shortcut_t *shortcut);

ddb_keyboard_shortcut_t *
ddb_keyboard_shortcut_get_next (ddb_keyboard_shortcut_t *shortcut);

const char *
ddb_keyboard_shortcut_get_title (ddb_keyboard_shortcut_t *shortcut);

void
ddb_keyboard_shortcut_set_title (ddb_keyboard_shortcut_t *shortcut, const char *title);

const char *
ddb_keyboard_shortcut_get_selector (ddb_keyboard_shortcut_t *shortcut);

void
ddb_keyboard_shortcut_set_selector (ddb_keyboard_shortcut_t *shortcut, const char *selector);

const char *
ddb_keyboard_shortcut_get_key_combination (ddb_keyboard_shortcut_t *shortcut);

void
ddb_keyboard_shortcut_set_key_combination (ddb_keyboard_shortcut_t *shortcut, const char *key_combination);

const char *
ddb_keyboard_shortcut_get_default_key_combination (ddb_keyboard_shortcut_t *shortcut);

void
ddb_keyboard_shortcut_set_default_key_combination (ddb_keyboard_shortcut_t *shortcut, const char *default_key_combination);

#endif /* keyboard_shortcuts_h */
