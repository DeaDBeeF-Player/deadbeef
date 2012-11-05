/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>,
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
#include <stdlib.h>

#include "gtkui.h"
#include "../../deadbeef.h"
#include "support.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    g_object_ref(G_OBJECT(widget)), (GDestroyNotify) g_object_unref)

static void
on_actionitem_activate (GtkMenuItem     *menuitem,
                           DB_plugin_action_t *action)
{
    // these actions are always in the MAIN context, or they are coming from new
    // plugins, so we don't have to care about the user data for <=1.4 plugins.
    // aren't we?..
    action->callback (action, DDB_ACTION_CTX_MAIN);
}

void
remove_actions (GtkWidget *widget, void *data) {
    const char *name = g_object_get_data (G_OBJECT (widget), "plugaction");
    if (name) {
        gtk_container_remove (GTK_CONTAINER (data), widget);
    }
    if (GTK_IS_MENU_ITEM (widget)) {
        GtkWidget *menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (widget));
        if (menu) {
            gtk_container_foreach (GTK_CONTAINER (menu), remove_actions, menu);
            // if menu is empty -- remove parent menu item
            GList *lst = gtk_container_get_children (GTK_CONTAINER (menu));
            if (lst) {
                g_list_free (lst);
            }
            else {
                gtk_container_remove (data, widget);
            }
        }
    }
}

void
add_mainmenu_actions (void)
{
    GtkWidget *menubar = lookup_widget (mainwin, "menubar1");
    // remove all plugaction_*** menu items and empty submenus
    gtk_container_foreach (GTK_CONTAINER (menubar), remove_actions, menubar);

    // add new
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
            char *tmp = NULL;
            if (0 == (action->flags & DB_ACTION_COMMON))
                continue;

            // 1st check if we have slashes
            const char *slash = action->title;
            while (NULL != (slash = strchr (slash, '/'))) {
                if (slash && slash > action->title && *(slash-1) == '\\') {
                    slash++;
                    continue;
                }
                break;
            }
            if (!slash) {
                continue;
            }

            char *ptr = tmp = strdup (action->title);

            char *prev_title = NULL;

            GtkWidget *current = menubar;
            GtkWidget *previous;

            while (1)
            {
                // find unescaped forward slash
                char *slash = strchr (ptr, '/');
                if (slash && slash > ptr && *(slash-1) == '\\') {
                    ptr = slash + 1;
                    continue;
                }

                if (!slash)
                {
                    GtkWidget *actionitem;
                    actionitem = gtk_image_menu_item_new_with_mnemonic (_(ptr));
                    gtk_widget_show (actionitem);

                    /* Here we have special cases for different submenus */
                    if (0 == strcmp ("File", prev_title))
                        gtk_menu_shell_insert (GTK_MENU_SHELL (current), actionitem, 5);
                    else if (0 == strcmp ("Edit", prev_title))
                        gtk_menu_shell_insert (GTK_MENU_SHELL (current), actionitem, 7);
                    else {
                        gtk_container_add (GTK_CONTAINER (current), actionitem);
                    }

                    g_signal_connect ((gpointer) actionitem, "activate",
                        G_CALLBACK (on_actionitem_activate),
                        action);
                    g_object_set_data_full (G_OBJECT (actionitem), "plugaction", strdup (action->name), free);
                    break;
                }
                *slash = 0;
                char menuname [1024];

                snprintf (menuname, sizeof (menuname), "%s_menu", ptr);

                previous = current;
                current = lookup_widget (mainwin, menuname);
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
                    GLADE_HOOKUP_OBJECT (mainwin, current, menuname);
                }
                prev_title = ptr;
                ptr = slash + 1;
            }
            if (tmp) {
                free (tmp);
            }
        }
    }
}

