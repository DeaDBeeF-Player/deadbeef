//
//  ctmap.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/23/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

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
        if (!head) {
            head = ctmap;
        }

        int n = 0;

        p = gettoken (p, t);
        if (!p || strcmp (t, "{")) {
            free (ctmap->ct);
            free (ctmap);
            break;
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
