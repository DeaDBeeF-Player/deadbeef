/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>,
        Viktor Semykin <thesame.ml@gmail.com>

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

#include "gtkui.h"
#include "../../deadbeef.h"
#include "support.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static void
on_actionitem_activate (GtkMenuItem     *menuitem,
                           DB_plugin_action_t *action)
{
    action->callback (action, NULL);
}

void
add_mainmenu_actions (GtkWidget *mainwin)
{
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;

    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (NULL);
        DB_plugin_action_t *action;

        for (action = actions; action; action = action->next)
        {
            if (0 == (action->flags & DB_ACTION_COMMON))
                continue;

            //We won't add item directly to main menu
            if (!strchr (action->title, '/'))
                continue;

            char *tmp;
            char *ptr = tmp = strdup (action->title);
            char *prev_title = NULL;

            GtkWidget *current = mainwin;
            GtkWidget *previous;

            while (1)
            {
                char *slash = strchr (ptr, '/');
                if (!slash)
                {
                    GtkWidget *actionitem;
                    actionitem = gtk_image_menu_item_new_with_mnemonic (ptr);
                    gtk_widget_show (actionitem);

                    /* Here we have special cases for different submenus */
                    if (0 == strcmp ("File", prev_title))
                        gtk_menu_shell_insert (GTK_MENU_SHELL (current), actionitem, 5);
                    else if (0 == strcmp ("Edit", prev_title))
                        gtk_menu_shell_insert (GTK_MENU_SHELL (current), actionitem, 7);
                    else
                        gtk_container_add (GTK_CONTAINER (current), actionitem);

                    g_signal_connect ((gpointer) actionitem, "activate",
                        G_CALLBACK (on_actionitem_activate),
                        action);
                    break;
                }
                *slash = 0;
                char menuname [1024];

                snprintf (menuname, sizeof (menuname), "%s_menu", ptr);

                previous = current;
                current = lookup_widget (current, menuname);
                if (!current)
                {
                    GtkWidget *newitem;

                    newitem = gtk_menu_item_new_with_mnemonic (ptr);
                    gtk_widget_show (newitem);

                    //If we add new submenu in main bar, add it before 'Help'
                    if (NULL == prev_title)
                        gtk_menu_shell_insert (GTK_MENU_SHELL (previous), newitem, 4);
                    else
                        gtk_container_add (GTK_CONTAINER (previous), newitem);

                    current = gtk_menu_new ();
                    gtk_menu_item_set_submenu (GTK_MENU_ITEM (newitem), current);
                }
                prev_title = ptr;
                ptr = slash + 1;
            }
        }
    }
}

