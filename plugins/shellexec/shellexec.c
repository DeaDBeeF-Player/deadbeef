/*
    Shellexec plugin for DeaDBeeF
    Copyright (C) 2010 Viktor Semykin <thesame.ml@gmail.com>

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
        single - command allowed only for single track
        local - command allowed only for local files
        remote - command allowed only for non-local files
        disabled - ignore command
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "../../deadbeef.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

//Probably it's reasonable to move these flags to parent struct
enum {
    SHX_ACTION_LOCAL_ONLY       = 1 << 0,
    SHX_ACTION_REMOTE_ONLY      = 1 << 1
};

typedef struct Shx_action_s
{
    DB_plugin_action_t parent;

    const char *shcommand;
    uint32_t shx_flags;
} Shx_action_t;

static Shx_action_t *actions;

DB_plugin_t *
shellexec_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static char*
trim (char* s)
{
    char *h, *t;
    
    for (h = s; *h == ' ' || *h == '\t'; h++);
    for (t = s + strlen (s)-1; *t == ' ' || *t == '\t'; t--);
    *(t+1) = 0;
    return h;
}

static int
shx_callback (Shx_action_t *action, DB_playItem_t *it)
{
    char cmd[_POSIX_ARG_MAX];
    int res = deadbeef->pl_format_title_escaped (it, -1, cmd, sizeof (cmd) - 2, -1, action->shcommand);
    if (res < 0) {
        trace ("shellexec: failed to format string for execution (too long?)\n");
        return -1;
    }
    strcat (cmd, "&");
    trace ("%s\n", cmd);
    system (cmd);
    return 0;
}

static DB_plugin_action_t *
shx_get_actions (DB_playItem_t *it)
{
    int is_local = it ? deadbeef->is_local_file (it->fname) : 1;

    Shx_action_t *action;
    for (action = actions; action; action = (Shx_action_t *)action->parent.next)
    {
        if (((action->shx_flags & SHX_ACTION_LOCAL_ONLY) && !is_local) ||
            ((action->shx_flags & SHX_ACTION_REMOTE_ONLY) && is_local))
            action->parent.flags |= DB_ACTION_DISABLED;
        else
            action->parent.flags &= ~DB_ACTION_DISABLED;
    }
    return (DB_plugin_action_t *)actions;
}

static int
shx_start ()
{
    actions = NULL;
    Shx_action_t *prev = NULL;

    DB_conf_item_t *item = deadbeef->conf_find ("shellexec.", NULL);
    while (item)
    {
        size_t l = strlen (item->value) + 1;
        char tmp[l];
        char *tmpptr;
        strcpy (tmp, item->value);
        trace ("Shellexec: %s\n", tmp);

        const char *command;
        const char *title;
        const char *name;
        const char *flags;

        char *semicolon;
        int idx = 0;
        tmpptr = tmp;
        do
        {
            semicolon = strchr (tmpptr, ':');
            if (semicolon)
                *semicolon = 0;

            trace ("Shellexec: idx: %d, tmp: %s\n", idx, tmpptr);
            switch (idx)
            {
                case 0: command = trim (tmpptr); break;
                case 1: title = trim (tmpptr); break;
                case 2: name = trim (tmpptr); break;
                case 3: flags = trim (tmpptr); break;
            }
            if (semicolon)
                tmpptr = semicolon + 1;
            idx++;
        }
        while (semicolon);

        if (idx < 2)
        {
            fprintf (stderr, "Shellexec: need at least command and title (%s)\n", item->value);
            continue;
        }
        if (idx > 4)
        {
            fprintf (stderr, "Shellexec: too many parameters in configuration line (%s)\n", item->value);
            continue;
        }
        if (strstr (flags, "disabled"))
            continue;

        Shx_action_t *action = calloc (sizeof (Shx_action_t), 1);

        action->parent.title = strdup (title);
        action->parent.name = strdup (name);
        action->shcommand = strdup (command);
        action->parent.callback = (DB_plugin_action_callback_t)shx_callback;
        action->parent.flags = DB_ACTION_SINGLE_TRACK;
        action->parent.next = NULL;

        action->shx_flags = 0;

        if (strstr (flags, "local"))
            action->shx_flags |= SHX_ACTION_LOCAL_ONLY;

        if (strstr (flags, "remote"))
            action->shx_flags |= SHX_ACTION_REMOTE_ONLY;

        if (0 == strstr (flags, "single"))
            action->parent.flags |= DB_ACTION_ALLOW_MULTIPLE_TRACKS;

        if (prev)
            prev->parent.next = (DB_plugin_action_t *)action;
        prev = action;

        if (!actions)
            actions = action;

        item = deadbeef->conf_find ("shellexec.", item);
    }
    return 0;
}

// define plugin interface
static DB_misc_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.id = "shellexec",
    .plugin.name = "Shell commands",
    .plugin.descr = "Executes configurable shell commands for tracks",
    .plugin.author = "Viktor Semykin",
    .plugin.email = "thesame.ml@gmail.com",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = shx_start,
    .plugin.get_actions = shx_get_actions
};

