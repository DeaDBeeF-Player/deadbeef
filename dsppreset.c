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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "deadbeef.h"
#include "plugins.h"

void
dsp_preset_free (ddb_dsp_context_t *head) {
    while (head) {
        ddb_dsp_context_t *next = head->next;
        head->plugin->close (head);
        head = next;
    }
}

int
dsp_preset_load (const char *fname, ddb_dsp_context_t **head) {
    if (!head) {
        return -1;
    }
    int err = 1;
    FILE *fp = fopen (fname, "rt");
    if (!fp) {
        return -1;
    }

    ddb_dsp_context_t *tail = NULL;

    char temp[100];
    for (;;) {
        // plugin {
        int err = fscanf (fp, "%100s {\n", temp);
        if (err == EOF) {
            break;
        }
        else if (1 != err) {
            fprintf (stderr, "error plugin name\n");
            goto error;
        }

        DB_dsp_t *plug = (DB_dsp_t *)plug_get_for_id (temp);
        if (!plug) {
            fprintf (stderr, "ddb_dsp_preset_load: plugin %s not found. preset will not be loaded\n", temp);
            goto error;
        }
        ddb_dsp_context_t *ctx = plug->open ();
        if (!ctx) {
            fprintf (stderr, "ddb_dsp_preset_load: failed to open ctxance of plugin %s\n", temp);
            goto error;
        }

        if (tail) {
            tail->next = ctx;
            tail = ctx;
        }
        else {
            tail = *head = ctx;
        }

        int n = 0;
        for (;;) {
            char value[1000];
            if (!fgets (temp, sizeof (temp), fp)) {
                fprintf (stderr, "unexpected eof while reading plugin params\n");
                goto error;
            }
            if (!strcmp (temp, "}\n")) {
                break;
            }
            else if (1 != sscanf (temp, "\t%1000[^\n]\n", value)) {
                fprintf (stderr, "error loading param %d\n", n);
                goto error;
            }
            if (plug->num_params) {
                plug->set_param (ctx, n, value);
            }
            n++;
        }
    }

    err = 0;
error:
    if (err) {
        fprintf (stderr, "error loading %s\n", fname);
    }
    if (fp) {
        fclose (fp);
    }
    if (err && *head) {
        dsp_preset_free (*head);
        *head = NULL;
    }
    return err ? -1 : 0;
}

int
dsp_preset_save (const char *path, ddb_dsp_context_t *head) {
    FILE *fp = fopen (path, "w+t");
    if (!fp) {
        return -1;
    }

    ddb_dsp_context_t *ctx = head;
    while (ctx) {
        fprintf (fp, "%s {\n", ctx->plugin->plugin.id);
        if (ctx->plugin->num_params) {
            int n = ctx->plugin->num_params ();
            int i;
            for (i = 0; i < n; i++) {
                char v[1000];
                ctx->plugin->get_param (ctx, i, v, sizeof (v));
                fprintf (fp, "\t%s\n", v);
            }
        }
        fprintf (fp, "}\n");
        ctx = ctx->next;
    }

    fclose (fp);
    return 0;
}
