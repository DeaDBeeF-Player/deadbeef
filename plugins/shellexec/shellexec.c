/*
    Shellexec plugin for DeaDBeeF
    Copyright (C) 2010-2014 Deadbeef team
    Original developer Viktor Semykin <thesame.ml@gmail.com>
    Maintenance, minor improvements Oleksiy Yakovenko <waker@users.sf.net>
    GUI support and bugfixing Azeem Arshad <kr00r4n@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
    Configuration options:

    command: the command executed by the shell
        formating directives are allowed, see
        format_shell_command function

    title: the title of command displayed in the UI

    name: used for referencing plugin actions

    flags: a list of flags:
        single - command allowed for single track
        multiple - command allowerd for multiple tracks
        local - command allowed for local files
        remote - command allowed for non-local files
        playlist - command allowed for playlist tabs
        common - command appears in main menu bar
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <jansson.h>
#include <deadbeef/deadbeef.h>
#include "shellexec.h"
#include "shellexecutil.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static Shx_plugin_t plugin;
DB_functions_t *deadbeef;

static Shx_action_t *actions;

DB_plugin_t *
shellexec_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static int shx_exec_track_cmd (Shx_action_t *action, DB_playItem_t *it) {
    char cmd[_POSIX_ARG_MAX];
    if (shellexec_eval_command(action->shcommand, cmd, sizeof (cmd), it) < 0) {
        return -1;
    }

    trace ("%s\n", cmd);
    system (cmd);
    return 0;
}

static int
shx_callback (Shx_action_t *action, int ctx)
{
    int res = 0;
    switch (ctx) {
    case DDB_ACTION_CTX_MAIN:
        trace ("%s\n", action->shcommand);
        int res = system (action->shcommand);
        break;
    case DDB_ACTION_CTX_SELECTION:
        {
            deadbeef->pl_lock ();
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                DB_playItem_t **items = NULL;
                int items_count = deadbeef->plt_getselcount (plt);
                if (0 < items_count) {
                    items = calloc (items_count, sizeof (DB_playItem_t *));
                    if (items) {
                        int n = 0;
                        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
                        while (it) {
                            if (deadbeef->pl_is_selected (it)) {
                                assert (n < items_count);
                                deadbeef->pl_item_ref (it);
                                items[n++] = it;
                            }
                            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                            deadbeef->pl_item_unref (it);
                            it = next;
                        }
                    }
                }
                deadbeef->pl_unlock ();
                if (items) {
                    for (int i = 0; i < items_count; i++) {
                        res = shx_exec_track_cmd (action, items[i]);
                        deadbeef->pl_item_unref (items[i]);
                    }
                    free (items);
                }
                deadbeef->plt_unref (plt);
            }
        }
        break;
    case DDB_ACTION_CTX_PLAYLIST:
        {
            ddb_playlist_t *plt = deadbeef->action_get_playlist ();
            if (plt) {
                deadbeef->pl_lock ();
                DB_playItem_t **items = NULL;
                int items_count = deadbeef->plt_get_item_count (plt, PL_MAIN);
                if (0 < items_count) {
                    items = calloc (items_count, sizeof (DB_playItem_t *));
                    if (items) {
                        int n = 0;
                        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
                        while (it) {
                            items[n++] = it;
                            it = deadbeef->pl_get_next (it, PL_MAIN);
                        }
                    }
                }
                deadbeef->pl_unlock ();
                if (items) {
                    for (int i = 0; i < items_count; i++) {
                        res = shx_exec_track_cmd (action, items[i]);
                        deadbeef->pl_item_unref (items[i]);
                    }
                    free (items);
                }
                deadbeef->plt_unref (plt);
            }
        }
        break;
    case DDB_ACTION_CTX_NOWPLAYING:
        {
            DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
            if (it) {
                res = shx_exec_track_cmd (action, it);
                deadbeef->pl_item_unref (it);
            }
        }
        break;
    }
    return res;
}

static DB_plugin_action_t *
shx_get_plugin_actions (DB_playItem_t *it)
{
    deadbeef->pl_lock ();
    int is_local = it ? deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) : 1;
    deadbeef->pl_unlock ();

    Shx_action_t *action;
    for (action = actions; action; action = (Shx_action_t *)action->parent.next)
    {
        if ((!(action->shx_flags & SHX_ACTION_LOCAL_ONLY) && is_local) ||
            (!(action->shx_flags & SHX_ACTION_REMOTE_ONLY) && !is_local)) {
            action->parent.flags |= DB_ACTION_DISABLED;
        }
        else {
            action->parent.flags &= ~DB_ACTION_DISABLED;
        }
    }
    return (DB_plugin_action_t *)actions;
}

void
shx_save_actions (void)
{
    json_t *json = json_array ();

    int i = 0;
    Shx_action_t *action = actions;
    while (action) {
        json_t *item = json_object ();

        json_object_set_new (item, "command", json_string (action->shcommand));
        json_object_set_new (item, "title", json_string (action->parent.title));
        json_object_set_new (item, "name", json_string (action->parent.name));
        json_t *flags = json_array ();

        if(action->shx_flags & SHX_ACTION_REMOTE_ONLY) {
            json_array_append_new (flags, json_string ("remote"));
        }
        if(action->shx_flags & SHX_ACTION_LOCAL_ONLY) {
            json_array_append_new (flags, json_string ("local"));
        }
        if(action->parent.flags & DB_ACTION_SINGLE_TRACK) {
            json_array_append_new (flags, json_string ("single"));
        }
        if(action->parent.flags & DB_ACTION_MULTIPLE_TRACKS) {
            json_array_append_new (flags, json_string ("multiple"));
        }
        if(action->parent.flags & DB_ACTION_COMMON) {
            json_array_append_new (flags, json_string ("common"));
        }
        json_object_set_new (item, "flags", flags);
        json_array_append_new (json, item);
        action = (Shx_action_t*)action->parent.next;
        i++;
    }
    char *str = json_dumps (json, 0);
    json_decref (json);
    if (str) {
        deadbeef->conf_set_str("shellexec_config", str);
        free (str);
        deadbeef->conf_save();
    }
    else {
        fprintf (stderr, "shellexec: failed to save json configuration\n");
    }
}

static Shx_action_t *
shx_get_actions_json (json_t *json) {
    Shx_action_t *action_list = NULL;
    Shx_action_t *prev = NULL;

    if (!json_is_array (json)) {
        return NULL;
    }

    size_t n = json_array_size (json);
    for (int i = 0; i < n; i++) {
        json_t *item = json_array_get (json, i);
        if (!json_is_object (item)) {
            continue;
        }

        json_t *jcommand = json_object_get (item, "command");
        json_t *jtitle = json_object_get (item, "title");
        json_t *jname = json_object_get (item, "name");
        json_t *jflags = json_object_get (item, "flags");

        if (!json_is_string (jcommand)
            || !json_is_string (jtitle)
            || (jname && !json_is_string (jname))
            || (jflags && !json_is_array (jflags))) {
            continue;
        }

        const char *command = json_string_value (jcommand);
        const char *title = json_string_value (jtitle);

        const char *name;

        if (jname) {
            name = json_string_value (jname);
        }
        else {
            name = "noname";
        }

        Shx_action_t *action = calloc (1, sizeof (Shx_action_t));

        action->parent.title = strdup (title);
        action->parent.name = strdup (name);
        action->shcommand = strdup (command);
        action->parent.callback2 = (DB_plugin_action_callback2_t)shx_callback;
        action->parent.next = NULL;
        action->parent.flags |= DB_ACTION_ADD_MENU;

        if (!jflags) {
            action->shx_flags = SHX_ACTION_LOCAL_ONLY | DB_ACTION_SINGLE_TRACK;
        }
        else {
            action->shx_flags = 0;

            size_t nflags = json_array_size (jflags);
            for (int i = 0; i < nflags; i++) {
                json_t *jflag = json_array_get (jflags, i);
                if (!json_is_string (jflag)) {
                    continue;
                }
                const char *flag = json_string_value (jflag);
                if (strstr (flag, "local")) {
                    action->shx_flags |= SHX_ACTION_LOCAL_ONLY;
                }

                if (strstr (flag, "remote")) {
                    action->shx_flags |= SHX_ACTION_REMOTE_ONLY;
                }

                if (strstr (flag, "single")) {
                    action->parent.flags |= DB_ACTION_SINGLE_TRACK;
                }

                if (strstr (flag, "multiple")) {
                    action->parent.flags |= DB_ACTION_MULTIPLE_TRACKS;
                }

                if (strstr (flag, "common")) {
                    action->parent.flags |= DB_ACTION_COMMON;
                }
            }
        }

        if (prev) {
            prev->parent.next = (DB_plugin_action_t *)action;
        }
        prev = action;

        if (!action_list) {
            action_list = action;
        }
    }
    return action_list;
}

Shx_action_t*
shx_action_add (void) {
    Shx_action_t *a = calloc (1, sizeof (Shx_action_t));
    a->parent.callback2 = (DB_plugin_action_callback2_t)shx_callback;
    if (!actions) {
        actions = a;
    }
    else {
        for (Shx_action_t *last = actions; last; last = (Shx_action_t *)last->parent.next) {
            if (!last->parent.next) {
                last->parent.next = (DB_plugin_action_t *)a;
                break;
            }
        }
    }
    return a;
}

void
shx_action_free (Shx_action_t *a) {
    if (a->shcommand) {
        free ((char *)a->shcommand);
    }
    if (a->parent.title) {
        free ((char *)a->parent.title);
    }
    if (a->parent.name) {
        free ((char *)a->parent.name);
    }
    free (a);
}

void
shx_action_remove (Shx_action_t *action) {
    Shx_action_t *prev = NULL;
    for (Shx_action_t *a = actions; a; a = (Shx_action_t *)a->parent.next) {
        if (a == action) {
            if (prev) {
                prev->parent.next = a->parent.next;
            }
            else {
                actions = (Shx_action_t *)a->parent.next;
            }
            break;
        }
        prev = a;
    }
    shx_action_free (action);
}

static int
shx_start ()
{
    deadbeef->conf_lock ();
    const char *conf = deadbeef->conf_get_str_fast ("shellexec_config", NULL);
    if (!conf) {
        // added right after 1.8.6 release:
        // the `shellexec_config_wip` was used from 2016 to 2021, so this needs to be here for migration
        deadbeef->conf_get_str_fast ("shellexec_config_wip", NULL);
    }
    if (conf) {
        json_error_t err;
        json_t *json = json_loads (conf, 0, &err);
        if (json) {
            actions = shx_get_actions_json (json);
            json_decref (json);
        }
        else {
            fprintf (stderr, "shellexec: json parser error at line %d:\n%s\n", err.line, err.text);
        }
    }
    deadbeef->conf_unlock ();
    return 0;
}

static int
shx_stop ()
{
    Shx_action_t *a = actions;
    while (a) {
        Shx_action_t *next = (Shx_action_t *)a->parent.next;
        if (a->shcommand) {
            free ((char *)a->shcommand);
        }
        if (a->parent.title) {
            free ((char *)a->parent.title);
        }
        if (a->parent.name) {
            free ((char *)a->parent.name);
        }
        free (a);
        a = next;
    }
    actions = NULL;
    return 0;
}

// define plugin interface
static Shx_plugin_t plugin = {
    .misc.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .misc.plugin.api_vminor = DB_API_VERSION_MINOR,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 2,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.id = "shellexec",
    .misc.plugin.name = "Shell commands",
    .misc.plugin.descr = "Run custom shell commands as plugin actions.\n",
    .misc.plugin.copyright = 
        "Shellexec plugin for DeaDBeeF\n"
        "Copyright (C) 2010-2014 Deadbeef team\n"
        "Original developer Viktor Semykin <thesame.ml@gmail.com>\n"
        "Maintenance, minor improvements Oleksiy Yakovenko <waker@users.sf.net>\n"
        "GUI support and bugfixing Azeem Arshad <kr00r4n@gmail.com>"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .misc.plugin.website = "http://deadbeef.sf.net",
    .misc.plugin.start = shx_start,
    .misc.plugin.stop = shx_stop,
    .misc.plugin.get_actions = shx_get_plugin_actions,
    .save_actions = shx_save_actions,
    .action_add = shx_action_add,
    .action_remove = shx_action_remove,
    .action_free = shx_action_free,
};

