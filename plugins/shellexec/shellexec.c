/*
    Shellexec plugin for DeaDBeeF
    Copyright (C) 2010-2014 Deadbeef team
    Original developer Viktor Semykin <thesame.ml@gmail.com>
    Maintenance, minor improvements Alexey Yakovenko <waker@users.sf.net>
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
    Configuration scheme:

    shellexec.NN shcmd:title:name:flags
    
    @shcmd is the command executed by the shell
        formating directives are allowed, see
        format_shell_command function

    @title is the title of command displayed in UI

    @name used for referencing command, for example in hotkeys
        configuration

    @flags comma-separated of command flags, allowed flags are:
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
#include "../../deadbeef.h"
#include "shellexec.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static Shx_plugin_t plugin;
static DB_functions_t *deadbeef;

static Shx_action_t *actions;

DB_plugin_t *
shellexec_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static const char*
trim (char* s)
{
    if (!s) {
        return "";
    }
    char *h, *t;
    
    for (h = s; *h == ' ' || *h == '\t'; h++);
    for (t = s + strlen (s)-1; *t == ' ' || *t == '\t'; t--);
    *(t+1) = 0;
    return h;
}

static int shx_exec_track_cmd (Shx_action_t *action, DB_playItem_t *it) {
    char cmd[_POSIX_ARG_MAX];
    int res = deadbeef->pl_format_title_escaped (it, -1, cmd, sizeof (cmd) - 2, -1, action->shcommand);
    if (res < 0) {
        trace ("shellexec: failed to format string for execution (too long?)\n");
        return -1;
    }
    strcat (cmd, "&");

    // replace \' with '"'"'
    size_t l = strlen (cmd);
    size_t remaining = _POSIX_ARG_MAX - l - 1;
    for (int i = 0; cmd[i]; i++) {
        if (cmd[i] == '\\' && cmd[i+1] == '\'' && remaining >= 3) {
            memmove (&cmd[i+5], &cmd[i+2], l - i + 1 - 2);
            memcpy (&cmd[i], "'\"'\"'", 5);
            l += 3;
            remaining -= 3;
            i += 5;
        }
        else if (remaining < 3) {
            fprintf (stderr, "shellexec: command is too long.\n");
            return -1;
        }
    }

    trace ("%s\n", cmd);
    res = system (cmd);
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
                    items = malloc (sizeof (DB_playItem_t *) * items_count);
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
                DB_playItem_t **items;
                int items_count = deadbeef->plt_get_item_count (plt, PL_MAIN);
                if (0 < items_count) {
                    items = malloc (sizeof (DB_playItem_t *) * items_count);
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
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
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

static char *
shx_find_sep (char *str) {
    while (*str && *str != ':') {
        if (*str == '"') {
            str++;
            while (*str && *str !='"') {
                str++;
            }
        }
        str++;
    }
    return str;
}

void
shx_save_actions (void)
{
    deadbeef->conf_remove_items("shellexec.");
    Shx_action_t *action = actions;
    int i = 0;
    while(action) {
        // build config line
        // format- shellexec.NN shcmd:title:name:flags
        size_t conf_line_length = 100 +
                                  strlen(action->shcommand) + 1 +
                                  strlen(action->parent.title) + 1 +
                                  strlen(action->parent.name) + 1;
        char conf_line[conf_line_length];
        char conf_key[50];
        sprintf(conf_key, "shellexec.%d", i);
        sprintf(conf_line, "%s:%s:%s:", action->shcommand,
                                        action->parent.title,
                                        action->parent.name);
        if(action->shx_flags & SHX_ACTION_REMOTE_ONLY) {
            strcat(conf_line, "remote,");
        }
        if(action->shx_flags & SHX_ACTION_LOCAL_ONLY) {
            strcat(conf_line, "local,");
        }
        if(action->parent.flags & DB_ACTION_SINGLE_TRACK) {
            strcat(conf_line, "single,");
        }
        if(action->parent.flags & DB_ACTION_MULTIPLE_TRACKS) {
            strcat(conf_line, "multiple,");
        }
        if(action->parent.flags & DB_ACTION_COMMON) {
            strcat(conf_line, "common,");
        }
        deadbeef->conf_set_str(conf_key, conf_line);
        action = (Shx_action_t*)action->parent.next;
        i++;
    }
    deadbeef->conf_save();
}

Shx_action_t*
shx_get_actions (DB_plugin_action_callback2_t callback)
{
    Shx_action_t *action_list = NULL;
    Shx_action_t *prev = NULL;
    DB_conf_item_t *item = deadbeef->conf_find ("shellexec.", NULL);
    while (item)
    {
        size_t l = strlen (item->value) + 1;
        char tmp[l];
        strcpy (tmp, item->value);
        trace ("Shellexec: %s\n", tmp);

        char *args[4] = {0};

        int idx = 0;
        char *p = tmp;
        while (idx < 4 && p) {
            char *e = shx_find_sep (p);
            args[idx++] = p;
            if (!e) {
                break;
            }
            *e = 0;
            p = e+1;
        }

        if (idx < 2)
        {
            fprintf (stderr, "Shellexec: need at least command and title (%s)\n", item->value);
            continue;
        }

        const char *command = trim (args[0]);
        const char *title = trim (args[1]);
        const char *name = trim (args[2]);
        const char *flags = trim (args[3]);
        if (!name) {
            name = "noname";
        }
        if (!flags) {
            flags = "local,single";
        }

        Shx_action_t *action = calloc (sizeof (Shx_action_t), 1);

        action->parent.title = strdup (title);
        action->parent.name = strdup (name);
        action->shcommand = strdup (command);
        action->parent.callback2 = callback;
        action->parent.next = NULL;
        action->parent.flags |= DB_ACTION_ADD_MENU;

        action->shx_flags = 0;

        if (strstr (flags, "local"))
            action->shx_flags |= SHX_ACTION_LOCAL_ONLY;

        if (strstr (flags, "remote"))
            action->shx_flags |= SHX_ACTION_REMOTE_ONLY;

        if (strstr (flags, "single"))
            action->parent.flags |= DB_ACTION_SINGLE_TRACK;

        if (strstr (flags, "multiple"))
            action->parent.flags |= DB_ACTION_MULTIPLE_TRACKS;

        if (strstr (flags, "common"))
            action->parent.flags |= DB_ACTION_COMMON;

        if (prev)
            prev->parent.next = (DB_plugin_action_t *)action;
        prev = action;

        if (!action_list)
            action_list = action;

        item = deadbeef->conf_find ("shellexec.", item);
    }
    return action_list;
}

Shx_action_t*
shx_action_add (void) {
    Shx_action_t *a = calloc (sizeof (Shx_action_t), 1);
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
    actions = shx_get_actions((DB_plugin_action_callback2_t)shx_callback);
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
    .misc.plugin.api_vmajor = 1,
    .misc.plugin.api_vminor = 5,
    .misc.plugin.version_major = 1,
    .misc.plugin.version_minor = 1,
    .misc.plugin.type = DB_PLUGIN_MISC,
    .misc.plugin.id = "shellexec",
    .misc.plugin.name = "Shell commands",
    .misc.plugin.descr = "Executes configurable shell commands for tracks\n"
    "If you don't have shellexecui installed,\n"
    "you can still use the following syntax in the config file:\n\n"
    "shellexec.NN shcmd:title:name:flag1,flag2,flag3,...\n\n"
    "NN is any (unique) number, e.g. 01, 02, 03, etc\n\n"
    "shcmd is the command to execute, supports title formatting\n\n"
    "title is the name of command displayed in UI (context menu)\n\n"
    "name used for referencing commands from other plugins, e.g hotkeys\n\n"
    "flags are comma-separated list of items, allowed items are:\n"
    "    single - command allowed only for single track\n"
    "    local - command allowed only for local files\n"
    "    remote - command allowed only for non-local files\n"
    "EXAMPLE: shellexec.00 notify-send \"%a - %t\":Show selected track:notify:single\n"
    "this would show the name of selected track in notification popup"
    ,
    .misc.plugin.copyright = 
        "Shellexec plugin for DeaDBeeF\n"
        "Copyright (C) 2010-2014 Deadbeef team\n"
        "Original developer Viktor Semykin <thesame.ml@gmail.com>\n"
        "Maintenance, minor improvements Alexey Yakovenko <waker@users.sf.net>\n"
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

