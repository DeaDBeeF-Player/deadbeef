/*
    Duktape plugin for DeaDBeeF Player
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
#include "duktape.h"
#include "../../deadbeef.h"
#include "bindings.h"

static DB_misc_t plugin;
DB_functions_t *deadbeef;

typedef struct duktape_script_s {
    struct duktape_script_s *next;
    duk_context *ctx;
    char *path;
} duktape_script_t;

static duktape_script_t *duktape_scripts;

#define EXT ".js"

static int
duktape_start (void) {
    struct dirent **namelist = NULL;

    const char *plugdir = getenv ("HOME"); // FIXME: should be the same path list as used by plug_load_all

    int n = scandir (plugdir, &namelist, NULL, NULL);
    if (n < 0)
    {
        if (namelist) {
            free (namelist);
        }
        return 0;
    }

    size_t plugdir_len = strlen (plugdir);
    int i;
    for (i = 0; i < n; i++)
    {
        size_t l = strlen (namelist[i]->d_name);

        if (namelist[i]->d_type == DT_REG && !strcasecmp (namelist[i]->d_name + l - sizeof(EXT) + 1, EXT)) {
            duktape_script_t *s = calloc (1, sizeof (duktape_script_t));

            size_t pathlen = plugdir_len + l + 2;
            s->path = malloc (pathlen);
            snprintf (s->path, pathlen, "%s/%s", plugdir, namelist[i]->d_name);
            s->ctx = duk_create_heap_default();
            s->next = duktape_scripts;
            duktape_scripts = s;

        }

        free (namelist[i]);
    }
    free (namelist);
    namelist = NULL;

    // binding tests
    for (duktape_script_t *s = duktape_scripts; s; s = s->next) {
        duktape_bind_all (s->ctx);
    }
    return 0;
}

static int
duktape_connect (void) {
    // run all scripts
    for (duktape_script_t *s = duktape_scripts; s; s = s->next) {
        duk_eval_file(s->ctx, s->path);
    }

    return 0;
}

static int
duktape_stop (void) {
    while (duktape_scripts) {
        duktape_script_t *next = duktape_scripts->next;
        if (duktape_scripts->path) {
            free (duktape_scripts->path);
        }
        if (duktape_scripts->ctx) {
            duk_destroy_heap (duktape_scripts->ctx);
        }
        free (duktape_scripts);
        duktape_scripts = next;
    }
    return 0;
}

int
duktape_message (uint32_t _id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
}

static DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 8,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.id = "duktape",
    .plugin.name = "Duktape Javascript Engine",
    .plugin.descr = "Load and run plugins made in Javascript using Duktape",
    .plugin.copyright = "",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = duktape_start,
    .plugin.connect = duktape_connect,
    .plugin.stop = duktape_stop,
    .plugin.message = duktape_message,
};

DB_plugin_t *
duktape_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
