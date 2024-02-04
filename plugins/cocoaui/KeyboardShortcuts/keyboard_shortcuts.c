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
    char *selector;
    char *key_combination;
    char *default_key_combination;

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
    free (shortcut->selector);
    free (shortcut->key_combination);
    free (shortcut->default_key_combination);
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
ddb_keyboard_shortcut_get_selector (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->selector;
}

void
ddb_keyboard_shortcut_set_selector (ddb_keyboard_shortcut_t *shortcut, const char *selector) {
    free (shortcut->selector);
    shortcut->selector = NULL;
    if (selector != NULL) {
        shortcut->selector = strdup (selector);
    }
}

const char *
ddb_keyboard_shortcut_get_key_combination (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->key_combination;
}

void
ddb_keyboard_shortcut_set_key_combination (ddb_keyboard_shortcut_t *shortcut, const char *key_combination) {
    free (shortcut->key_combination);
    shortcut->key_combination = NULL;
    if (key_combination != NULL) {
        shortcut->key_combination = strdup (key_combination);
    }
}

const char *
ddb_keyboard_shortcut_get_default_key_combination (ddb_keyboard_shortcut_t *shortcut) {
    return shortcut->default_key_combination;
}

void
ddb_keyboard_shortcut_set_default_key_combination (ddb_keyboard_shortcut_t *shortcut, const char *default_key_combination) {
    free (shortcut->default_key_combination);
    shortcut->default_key_combination = NULL;
    if (default_key_combination != NULL) {
        shortcut->default_key_combination = strdup (default_key_combination);
    }
}
