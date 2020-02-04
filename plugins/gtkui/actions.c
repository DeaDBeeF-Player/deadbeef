/*
    DeaDBeeF -- the music player
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
#include <string.h>
#include <stdlib.h>

#include "gtkui.h"
#include "../../deadbeef.h"
#include "support.h"
#include "actions.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    g_object_ref(G_OBJECT(widget)), (GDestroyNotify) g_object_unref)

static gboolean
menu_action_cb (void *ctx) {
    DB_plugin_action_t *action = ctx;
    if (action->callback) {
        gtkui_exec_action_14 (action, -1);
    }
    else if (action->callback2) {
        action->callback2 (action, DDB_ACTION_CTX_MAIN);
    }
    return FALSE;
}

static void
on_actionitem_activate (GtkMenuItem     *menuitem,
                           DB_plugin_action_t *action)
{
    // these actions are always in the MAIN context, or they are coming from new
    // plugins, so we don't have to care about the user data for <=1.4 plugins.
    // aren't we?..
    gdk_threads_add_idle (menu_action_cb, action);
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
    GtkWidget *menubar = lookup_widget (mainwin, "menubar");
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

            int has_addmenu = (action->flags & DB_ACTION_COMMON) && ((action->flags & DB_ACTION_ADD_MENU) || (action->callback));

            if (!has_addmenu)
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
                    if (prev_title && 0 == strcmp ("File", prev_title))
                        gtk_menu_shell_insert (GTK_MENU_SHELL (current), actionitem, 5);
                    else if (prev_title && 0 == strcmp ("Edit", prev_title))
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
                current = (GtkWidget*) g_object_get_data (G_OBJECT (mainwin), menuname);
                if (!current)
                {
                    GtkWidget *newitem;

                    newitem = gtk_menu_item_new_with_mnemonic (_(ptr));
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

void
gtkui_exec_action_14 (DB_plugin_action_t *action, int cursor) {
    // Plugin can handle all tracks by itself
    if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS)
    {
        action->callback (action, NULL);
        return;
    }

    // For single-track actions just invoke it with first selected track
    if (!(action->flags & DB_ACTION_MULTIPLE_TRACKS))
    {
        if (cursor == -1) {
            cursor = deadbeef->pl_get_cursor (PL_MAIN);
        }
        if (cursor == -1) 
        {
            return;
        }
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
        action->callback (action, it);
        deadbeef->pl_item_unref (it);
        return;
    }

    //We end up here if plugin won't traverse tracks and we have to do it ourselves
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            action->callback (action, it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}
