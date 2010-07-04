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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../deadbeef.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

static DB_plugin_action_t *actions;

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
shell_quote (const char *source, char *dest)
{
    char *sp, *dp;

    for (sp = source, dp = dest; *sp; sp++, dp++)
    {
        if (*sp == '\'')
        {
            strcpy (dp, "'\\''");
            dp += 3;
        }
        else
            *dp = *sp;
    }
    *dp = 0;
}


/*
    format_shell_command function
    Similarly to db_format_title formats track's string
    and quotes all fields to make them safe for shell
    
    %a, %t, %b, %B, %C, %n, %N, %y, %g, %c, %r are handled. Use double-% to
    skip field.


    Example:

    Format string:
    echo %a - %t %%a - %%t

    Output:
    echo 'Blind Faith' - 'Can'\''t Find My Way Home' %a - %t
*/
static const char*
format_shell_command (DB_playItem_t *it, const char *format)
{
    char *p;
    const char *trailing = format;
    const char *field;
    char *res, *res_p;

    res = res_p = malloc (65536); //FIXME: possible heap corruption

    for (;;)
    {
        p = strchr (trailing, '%');
        if (!p) break;

        switch (*(p+1))
        {
            case 'a': field = "artist"; break;
            case 't': field = "title"; break;
            case 'b': field = "album"; break;
            case 'B': field = "band"; break;
            case 'C': field = "composer"; break;
            case 'n': field = "track"; break;
            case 'N': field = "numtracks"; break;
            case 'y': field = "year"; break;
            case 'g': field = "genre"; break;
            case 'c': field = "comment"; break;
            case 'r': field = "copyright"; break;
            case 'f': break;
            default: field = NULL;
        }

        if (field == NULL)
        {
            int l = ((*(p+1) == '%') ? 1 : 0);
            trace ("field is null; p: %s; p+1: %s; l: %d; trailing: %s; res_p: %s\n", p, p+1, l, trailing, res_p);
            strncpy (res_p, trailing, p-trailing+l);
            res_p += (p-trailing+l);
            trailing = p+l+1;
            trace ("res: %s; trailing: %s\n", res, res_p, trailing);
            continue;
        }
        else
        {
            const char *meta;
            if (*(p+1) == 'f')
                meta = it->fname;
            else
                meta = deadbeef->pl_find_meta (it, field);

            char quoted [strlen (meta) * 4 + 1]; //Worst case is when all chars are single quote
            shell_quote (meta, quoted);
            
            strncpy (res_p, trailing, p-trailing);
            res_p += (p-trailing);
            *res_p++ = '\'';
            strcpy (res_p, quoted);
            res_p += strlen (quoted);
            *res_p++ = '\'';
            trailing = p+2;
        }
    }
    strcpy (res_p, trailing);
    return res;
}

static int
shx_callback (DB_playItem_t *it, void *data)
{
    const char *cmd = format_shell_command (it, data);
    printf ("%s\n", cmd);
    system (cmd);
    free (cmd);
    return 0;
}

static DB_plugin_action_t *
shx_get_actions (DB_playItem_t *unused)
{
    return actions;
}

static int
shx_start ()
{
    actions = NULL;
    DB_plugin_action_t *prev = NULL;

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
        action->next = NULL;

        if (prev)
            prev->next = action;
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

