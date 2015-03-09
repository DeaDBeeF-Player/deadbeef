/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "pluginsettings.h"
#include "parser.h"

void
settings_data_free (settings_data_t *settings_data) {
    for (int i = 0; i < settings_data->nprops; i++) {
        if (settings_data->props[i].def)
            free (settings_data->props[i].def);
        if (settings_data->props[i].key)
            free (settings_data->props[i].key);
        if (settings_data->props[i].title)
            free (settings_data->props[i].title);
    }
    memset (settings_data, 0, sizeof (settings_data_t));
}

void
settings_data_add_property (settings_data_t *settings_data, int type, const char *key, const char *title, const char *def) {
    int i = settings_data->nprops++;
    settings_data->props[i].type = type;
    if (key) {
        settings_data->props[i].key = strdup (key);
    }
    if (title) {
        settings_data->props[i].title = strdup (title);
    }
    if (def) {
        settings_data->props[i].def = strdup (def);
    }
}

int
settings_data_init (settings_data_t *settings_data, const char *layout) {
    settings_data_free (settings_data);

    char token[MAX_TOKEN];
    const char *script = layout;
    parser_line = 1;
    while ((script = gettoken (script, token))) {
        if (strcmp (token, "property")) {
            fprintf (stderr, "settings layout parse error at token %s line %d\n", token, parser_line);
            break;
        }
        char labeltext[MAX_TOKEN];
        script = gettoken_warn_eof (script, labeltext);
        if (!script) {
            break;
        }
        char type[MAX_TOKEN];
        script = gettoken_warn_eof (script, type);
        if (!script) {
            break;
        }

        // ignore layout options
        char key[MAX_TOKEN];
        const char *skiptokens[] = { "vert", NULL };
        for (;;) {
            script = gettoken_warn_eof (script, key);
            int i = 0;
            for (i = 0; skiptokens[i]; i++) {
                if (!strcmp (key, skiptokens[i])) {
                    break;
                }
            }
            if (!skiptokens[i]) {
                break;
            }
        }
        if (!script) {
            break;
        }
        char def[MAX_TOKEN];
        script = gettoken_warn_eof (script, def);
        if (!script) {
            break;
        }

        // fetch data
        if (!strcmp (type, "submenu")) {
            settings_data_add_property (settings_data, PROP_SUBMENU, key, labeltext, def);
        }
        if (!strcmp (type, "entry")) {
            settings_data_add_property (settings_data, PROP_ENTRY, key, labeltext, def);
        }
        else if (!strcmp (type, "password")) {
            settings_data_add_property (settings_data, PROP_PASSWORD, key, labeltext, def);
        }
        else if (!strcmp (type, "file")) {
            settings_data_add_property (settings_data, PROP_FILE, key, labeltext, def);
        }
        else if (!strcmp (type, "checkbox")) {
            settings_data_add_property (settings_data, PROP_CHECKBOX, key, labeltext, def);
        }
        else if (!strncmp (type, "hscale[", 7) || !strncmp (type, "vscale[", 7) || !strncmp (type, "spinbtn[", 8)) {
            settings_data_add_property (settings_data, PROP_SLIDER, key, labeltext, def);
        }
        else if (!strncmp (type, "select[", 7)) {
            settings_data_add_property (settings_data, PROP_SELECT, key, labeltext, def);
            // skip spaces and find the first select item
            while (*((uint8_t*)script) <= 0x20) {
                script++;
            }
            settings_data->props[settings_data->nprops-1].select_options = script;
        }
        else {
            // skip unknown/useless stuff
            char semicolon[MAX_TOKEN];
            while ((script = gettoken_warn_eof (script, semicolon))) {
                if (!strcmp (semicolon, ";")) {
                    break;
                }
            }
            continue;
        }

        char skip[MAX_TOKEN] = "";
        do {
            script = gettoken_warn_eof (script, skip);
            if (!script) {
                break;
            }
        } while (strcmp (skip, ";"));
    }
    
    return 0;
}
