/*
    Shellexec plugin for DeaDBeeF
    Copyright (C) 2009 Viktor Semykin <thesame.ml@gmail.com>

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../deadbeef.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

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
    for (t = s + strlen (s); *t == ' ' || *t == '\t'; t--);
    * (t+1) = 0;
    return h;
}

static int
shx_callback (DB_playItem_t *it, void *data)
{
    char fmt[1024]; //FIXME: possible stack corruption
    deadbeef->pl_format_title (it, -1, fmt, sizeof (fmt), -1, data);
    printf ("%s\n", fmt);
    return 0;
}

static int
shx_get_actions (DB_plugin_action_t **actions)
{
    DB_conf_item_t *item = deadbeef->conf_find ("shellexec.", NULL);
    while (item)
    {
        size_t l = strlen (item->value) + 1;
        char tmp[l];
        strcpy (tmp, item->value);
        trace ("Shellexec: %s\n", tmp);

        char *semicolon = strchr (tmp, ':');
        if (!semicolon)
        {
            fprintf (stdout, "Shellexec: wrong option <%s>\n", tmp);
            continue;
        }

        *semicolon = 0;

        DB_plugin_action_t *action = calloc (sizeof (DB_plugin_action_t), 1);

        action->title = strdup (trim (semicolon + 1));
        action->callback = shx_callback;
        action->data = strdup (trim (tmp));
        action->flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_ALLOW_MULTIPLE_TRACKS;

        action->next = *actions;
        *actions = action;

        item = deadbeef->conf_find ("shellexec.", item);
    }
    return 1;
}

// define plugin interface
static DB_misc_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.id = "shellexec",
    .plugin.name = "Shell commands for tracks",
    .plugin.descr = "Executes configurable shell commands for tracks",
    .plugin.author = "Viktor Semykin",
    .plugin.email = "thesame.ml@gmail.com",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.get_actions = shx_get_actions
};

