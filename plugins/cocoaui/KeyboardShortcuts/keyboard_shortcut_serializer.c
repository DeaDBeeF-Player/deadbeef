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

#include <jansson.h>
#include <string.h>
#include "keyboard_shortcut_serializer.h"

static void
_find_items_matching (ddb_keyboard_shortcut_t *item, const char *mac_action, void (^perform_block)(ddb_keyboard_shortcut_t *shortcut)) {

    const char *mac_action_src = ddb_keyboard_shortcut_get_mac_action (item);
    if (mac_action_src != NULL && !strcmp (mac_action, mac_action_src)) {
        perform_block(item);
    }

    ddb_keyboard_shortcut_t *children = ddb_keyboard_shortcut_get_children (item);
    while (children != NULL) {
        _find_items_matching (children, mac_action, perform_block);
        children = ddb_keyboard_shortcut_get_next (children);
    }
}

static void
_assign_shortcut (ddb_keyboard_shortcut_t *root, const char *mac_action, const char *key_character, ddb_keyboard_shortcut_modifiers_t key_modifiers) {
    _find_items_matching (root, mac_action, ^(ddb_keyboard_shortcut_t *shortcut) {
        if (key_character != NULL) {
            ddb_keyboard_shortcut_set_key_character (shortcut, key_character);
        }
        ddb_keyboard_shortcut_set_key_modifiers (shortcut, key_modifiers);
    });
}

static void
_load_shortcut (ddb_keyboard_shortcut_t *root, json_t *item) {
    json_t *mac_action = json_object_get (item, "mac_action");
    json_t *key_character = json_object_get (item, "character");
    json_t *modifiers = json_object_get (item, "modifiers");

    if (mac_action == NULL || !json_is_string (mac_action)) {
        return;
    }

    if (key_character != NULL && !json_is_string (key_character)) {
        return;
    }

    if (modifiers != NULL && !json_is_array (modifiers)) {
        return;
    }

    const char *key_character_str = NULL;
    if (key_character != NULL) {
        key_character_str = json_string_value (key_character);
    }

    ddb_keyboard_shortcut_modifiers_t modifiers_result = 0;

    if (modifiers != NULL) {
        ssize_t modcount = json_array_size (modifiers);
        for (int index = 0; index < modcount; index++) {
            json_t *mod = json_array_get (modifiers, index);
            if (!json_is_string (mod)) {
                return;
            }

            const char *modstring = json_string_value (mod);

            if (!strcasecmp(modstring, "shift")) {
                modifiers_result |= ddb_keyboard_shortcut_modifiers_shift;
            }
            else if (!strcasecmp(modstring, "option")) {
                modifiers_result |= ddb_keyboard_shortcut_modifiers_option;
            }
            else if (!strcasecmp(modstring, "control")) {
                modifiers_result |= ddb_keyboard_shortcut_modifiers_control;
            }
            else if (!strcasecmp(modstring, "super")) {
                modifiers_result |= ddb_keyboard_shortcut_modifiers_super;
            }
            else if (!strcasecmp(modstring, "fn")) {
                modifiers_result |= ddb_keyboard_shortcut_modifiers_fn;
            }
            else {
                return;
            }
        }
    }

    _assign_shortcut (root, json_string_value (mac_action), key_character_str, modifiers_result);
}

static void
_for_each_shortcut(ddb_keyboard_shortcut_t *item, void (^perform_block)(ddb_keyboard_shortcut_t *shortcut)) {
    perform_block(item);

    ddb_keyboard_shortcut_t *children = ddb_keyboard_shortcut_get_children (item);
    while (children != NULL) {
        _for_each_shortcut(children, perform_block);
        children = ddb_keyboard_shortcut_get_next (children);
    }
}

char *
ddb_keyboard_shortcuts_save (ddb_keyboard_shortcut_t *root) {
    json_t *array = json_array ();
    
    _for_each_shortcut(root, ^(ddb_keyboard_shortcut_t *shortcut){
        const char *mac_action = ddb_keyboard_shortcut_get_mac_action (shortcut);
        if (mac_action == NULL) {
            return;
        }
        const char *key_character = ddb_keyboard_shortcut_get_key_character (shortcut);
        const char *default_key_character = ddb_keyboard_shortcut_get_default_key_character (shortcut);
        ddb_keyboard_shortcut_modifiers_t key_modifiers = ddb_keyboard_shortcut_get_key_modifiers(shortcut);
        ddb_keyboard_shortcut_modifiers_t default_key_modifiers = ddb_keyboard_shortcut_get_default_key_modifiers(shortcut);

        if (!strcmp(key_character, default_key_character) && key_modifiers == default_key_modifiers) {
            return;
        }

        json_t *item = json_object();
        json_object_set(item, "mac_action", json_string(mac_action));
        if (key_character) {
            json_object_set(item, "character", json_string(key_character));
        }
        if (key_modifiers != 0) {
            json_t *modifiers_array = json_array();
            if (key_modifiers & ddb_keyboard_shortcut_modifiers_shift) {
                json_array_append (modifiers_array, json_string("shift"));
            }
            if (key_modifiers & ddb_keyboard_shortcut_modifiers_option) {
                json_array_append (modifiers_array, json_string("option"));
            }
            if (key_modifiers & ddb_keyboard_shortcut_modifiers_control) {
                json_array_append (modifiers_array, json_string("control"));
            }
            if (key_modifiers & ddb_keyboard_shortcut_modifiers_super) {
                json_array_append (modifiers_array, json_string("super"));
            }
            if (key_modifiers & ddb_keyboard_shortcut_modifiers_fn) {
                json_array_append (modifiers_array, json_string("fn"));
            }
            json_object_set(item, "modifiers", modifiers_array);
        }

        json_array_append(array, item);
    });

    json_t *json = json_object ();
    json_object_set (json, "shortcuts", array);
    return json_dumps(json, 0);
}

int
ddb_keyboard_shortcuts_load (ddb_keyboard_shortcut_t *root, const char *json_string) {
    int res = -1;
    json_t *json = NULL;

    json_error_t error;
    json = json_loads (json_string, 0, &error);
    if (json == NULL) {
        goto error;
    }

    json_t *shortcuts = json_object_get (json, "shortcuts");
    if (shortcuts == NULL || !json_is_array (shortcuts)) {
        goto error;
    }

    ssize_t count = json_array_size (shortcuts);
    for (int index = 0; index < count; index++) {
        json_t *item = json_array_get (shortcuts, index);
        if (!json_is_object (item)) {
            // soft error
            continue;
        }

        _load_shortcut (root, item);
    }

    res = 0;
error:
    if (json != NULL) {
        json_delete (json);
    }

    return res;
}
