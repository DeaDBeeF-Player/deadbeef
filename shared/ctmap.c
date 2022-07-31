/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2019 Oleksiy Yakovenko and other contributors

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
#include <stdlib.h>
#include <string.h>
#include "ctmap.h"
#include "../plugins/libparser/parser.h"

void
ddb_ctmap_free (ddb_ctmap_t *ctmap) {
    while (ctmap) {
        ddb_ctmap_t *ct = ctmap;
        free (ct->ct);
        for (int i = 0; ct->plugins[i]; i++) {
            free (ct->plugins[i]);
        }
        ctmap = ct->next;
        free (ct);
    }
    free (ctmap);
}

ddb_ctmap_t *
ddb_ctmap_init_from_string (const char *mapstr) {
    const char *p = mapstr;
    char t[MAX_TOKEN];
    char plugins[MAX_TOKEN*5];

    ddb_ctmap_t *tail = NULL;
    ddb_ctmap_t *head = NULL;

    for (;;) {
        p = gettoken (p, t);

        if (!p) {
            break;
        }

        ddb_ctmap_t *ctmap = malloc (sizeof (ddb_ctmap_t));
        memset (ctmap, 0, sizeof (ddb_ctmap_t));
        ctmap->ct = strdup (t);

        int n = 0;

        p = gettoken (p, t);
        if (!p || strcmp (t, "{")) {
            free (ctmap->ct);
            free (ctmap);
            break;
        }

        if (!head) {
            head = ctmap;
        }

        plugins[0] = 0;
        for (;;) {
            p = gettoken (p, t);
            if (!p || !strcmp (t, "}") || n >= DDB_CTMAP_MAX_PLUGINS-1) {
                break;
            }

            ctmap->plugins[n++] = strdup (t);
        }
        ctmap->plugins[n] = NULL;
        if (tail) {
            tail->next = ctmap;
        }
        tail = ctmap;
    }

    return head;
}

char *
ddb_ctmap_to_string (const ddb_ctmap_t *ctmap) {
    char mapstr[2048] = "";
    int s = sizeof (mapstr);
    char *p = mapstr;

    while (ctmap) {

        char plugins[100] = "";
        for (int i = 0; ctmap->plugins[i]; i++) {
            if (i != 0) {
                strncat(plugins, " ", sizeof (plugins) - strlen (plugins) - 1);
            }
            strncat(plugins, ctmap->plugins[i], sizeof (plugins) - strlen (plugins) - 1);
        }

        int l = snprintf (p, s, "%s {%s} ", ctmap->ct, plugins);
        p += l;
        s -= l;
        if (s <= 0) {
            break;
        }
    }

    return strdup (mapstr);
}
