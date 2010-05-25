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

#include "../../deadbeef.h"

#define MAX_COMMANDS 128

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

DB_single_action_t shx_actions [MAX_COMMANDS];
static int single_action_count;

DB_plugin_t *
shellexec_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static int
shx_get_single_actions (DB_playItem_t *it, DB_single_action_t *actions[], int *size)
{
    if (*size < single_action_count)
        return 0;

    int i;
    *size = single_action_count;
    trace ("Shellexec: %d actions\n", single_action_count);
    for (i=0; i < single_action_count; i++)
        actions[i] = &shx_actions[i];
    return 1;
}

static int
shx_callback (DB_playItem_t *it, void *data)
{
    char fmt[1024]; //FIXME: possible stack corruption
    deadbeef->pl_format_title (it, -1, fmt, sizeof (fmt), -1, data);
    printf ("%s\n", fmt);
    return 0;
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
shellexec_start (void)
{
    trace ("Starting shellexec\n");
    single_action_count = 0;
    DB_conf_item_t *item = deadbeef->conf_find ("shellexec.", NULL);
    while (item)
    {
        if (single_action_count == MAX_COMMANDS)
        {
            fprintf (stdout, "Shellexec: max number of commands (%d) exceeded\n", MAX_COMMANDS);
            break;
        }
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

        shx_actions[single_action_count].title = strdup (trim (semicolon + 1));
        shx_actions[single_action_count].callback = shx_callback;
        shx_actions[single_action_count].data = strdup (trim (tmp));

        item = deadbeef->conf_find ("shellexec.", item);
        single_action_count++;
    }
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
    .plugin.start = shellexec_start,

    .get_single_actions = shx_get_single_actions
};

