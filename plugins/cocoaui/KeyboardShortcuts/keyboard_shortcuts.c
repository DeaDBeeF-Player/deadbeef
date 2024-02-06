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

#include <stdlib.h>
#include <string.h>
#include "keyboard_shortcuts.h"

struct ddb_keyboard_shortcut_s {
    char *title; // NOTE: this may be localized
    char *mac_action;

    char *key_character;
    ddb_keyboard_shortcut_modifiers_t key_modifiers;

    char *default_key_character;
    ddb_keyboard_shortcut_modifiers_t default_key_modifiers;

    ddb_keyboard_shortcut_t *children;
    ddb_keyboard_shortcut_t *next;
};

static ddb_keyboard_shortcut_t _shortcuts;

static void
_deinit_shortcut (ddb_keyboard_shortcut_t *shortcut) {
    for (ddb_keyboard_shortcut_t *child = shortcut->children; child != NULL; child = child->next) {
        _deinit_shortcut(child);
    }
    free (shortcut->title);
    free (shortcut->mac_action);
    free (shortcut->key_character);
    free (shortcut->default_key_character);
    free (shortcut);
}

ddb_keyboard_shortcut_t *
ddb_keyboard_shortcuts_get_root (void) {
    return &_shortcuts;
}

void
ddb_keyboard_shortcuts_deinit (void) {
    _deinit_shortcut (&_shortcuts);
}

void
ddb_keyboard_shortcuts_init_with_config (const char *config_json) {
}

ddb_keyboard_shortcut_t *
ddb_keyboard_shortcut_append (ddb_keyboard_shortcut_t *parent) {
    ddb_keyboard_shortcut_t *shortcut = calloc (1, sizeof (ddb_keyboard_shortcut_t));

    ddb_keyboard_shortcut_t *tail = parent->children;
    while (tail != NULL && tail->next != NULL) {
        tail = tail->next;
    }

    if (tail != NULL) {
        tail->next = shortcut;
    }
    else {
        parent->children = shortcut;
    }

    return shortcut;
}

ddb_keyboard_shortcut_t *
ddb_keyboard_shortcut_get_children (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->children;
}

ddb_keyboard_shortcut_t *
ddb_keyboard_shortcut_get_next (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->next;
}


const char *
ddb_keyboard_shortcut_get_title (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->title;
}

void
ddb_keyboard_shortcut_set_title (ddb_keyboard_shortcut_t *shortcut, const char *title) {
    free (shortcut->title);
    shortcut->title = NULL;
    if (title != NULL) {
        shortcut->title = strdup (title);
    }
}

const char *
ddb_keyboard_shortcut_get_mac_action (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->mac_action;
}

void
ddb_keyboard_shortcut_set_mac_action (ddb_keyboard_shortcut_t *shortcut, const char *selector) {
    free (shortcut->mac_action);
    shortcut->mac_action = NULL;
    if (selector != NULL) {
        shortcut->mac_action = strdup (selector);
    }
}

const char *
ddb_keyboard_shortcut_get_key_character (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->key_character;
}

void
ddb_keyboard_shortcut_set_key_character (ddb_keyboard_shortcut_t *shortcut, const char *character) {
    free (shortcut->key_character);
    shortcut->key_character = NULL;
    if (character != NULL) {
        shortcut->key_character = strdup (character);
    }
}

ddb_keyboard_shortcut_modifiers_t
ddb_keyboard_shortcut_get_key_modifiers (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->key_modifiers;
}

void
ddb_keyboard_shortcut_set_key_modifiers (ddb_keyboard_shortcut_t *shortcut, ddb_keyboard_shortcut_modifiers_t modifiers) {
    shortcut->key_modifiers = modifiers;
}

const char *
ddb_keyboard_shortcut_get_default_key_character (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->default_key_character;
}

void
ddb_keyboard_shortcut_set_default_key_character (ddb_keyboard_shortcut_t *shortcut, const char *character) {
    free (shortcut->default_key_character);
    shortcut->default_key_character = NULL;
    if (character != NULL) {
        shortcut->default_key_character = strdup (character);
    }
}

ddb_keyboard_shortcut_modifiers_t
ddb_keyboard_shortcut_get_default_key_modifiers (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->default_key_modifiers;
}

void
ddb_keyboard_shortcut_set_default_key_modifiers (ddb_keyboard_shortcut_t *shortcut, ddb_keyboard_shortcut_modifiers_t modifiers) {
    shortcut->default_key_modifiers = modifiers;
}
