/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  dsp preset manager

  Copyright (C) 2009-2013 Oleksiy Yakovenko

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

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <deadbeef/deadbeef.h>
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
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        return -1;
    }

    ddb_dsp_context_t *tail = NULL;

    char temp[100];
    for (;;) {
        // plugin {
        int fscanf_res = fscanf (fp, "%99s {\n", temp);
        if (fscanf_res == EOF) {
            break;
        }
        else if (1 != fscanf_res) {
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
            else if (1 != sscanf (temp, "\t%100[^\n]\n", value)) {
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
    FILE *fp = fopen (path, "w+b");
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
