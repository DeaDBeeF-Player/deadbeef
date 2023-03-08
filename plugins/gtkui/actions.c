/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>
#include "support.h"
#include "actions.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    g_object_ref(G_OBJECT(widget)), (GDestroyNotify) g_object_unref)

static gboolean
menu_action_cb (void *ctx) {
    DB_plugin_action_t *action = ctx;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    if (action->callback) {
#pragma GCC diagnostic pop
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

int
menu_add_action_items(GtkWidget *menu, int selected_count, ddb_playItem_t *selected_track, ddb_action_context_t action_context, menu_action_activate_callback_t activate_callback) {
    int hide_remove_from_disk = deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0);
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;
    int added_entries = 0;
    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (selected_track);
        DB_plugin_action_t *action;

        int count = 0;
        for (action = actions; action; action = action->next)
        {
            if (action->name && !strcmp (action->name, "delete_from_disk") && hide_remove_from_disk) {
                continue;
            }

            if (action->flags&DB_ACTION_DISABLED) {
                continue;
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            if (!((action->callback2 && (action->flags & DB_ACTION_ADD_MENU)) || action->callback)) {
#pragma GCC diagnostic pop
                continue;
            }

            if (action_context == DDB_ACTION_CTX_SELECTION) {
                if ((action->flags & DB_ACTION_COMMON)
                    || !(action->flags & (DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_SINGLE_TRACK))) {
                    continue;
                }
            }

            if (action_context == DDB_ACTION_CTX_PLAYLIST) {
                if (action->flags & DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST) {
                    continue;
                }
                if (action->flags & DB_ACTION_COMMON) {
                    continue;
                }
            }
            else if (action_context == DDB_ACTION_CTX_MAIN) {
                if (!((action->flags & (DB_ACTION_COMMON|DB_ACTION_ADD_MENU)) == (DB_ACTION_COMMON|DB_ACTION_ADD_MENU))) {
                    continue;
                }
                const char *slash_test = action->title;
                while (NULL != (slash_test = strchr (slash_test, '/'))) {
                    if (slash_test && slash_test > action->title && *(slash_test-1) == '\\') {
                        slash_test++;
                        continue;
                    }
                    break;
                }

                if (slash_test == NULL) {
                    continue;
                }
            }


            // create submenus (separated with '/')
            const char *prev = action->title;
            while (*prev && *prev == '/') {
                prev++;
            }

            char *prev_title = NULL;

            GtkWidget *current = menu;
            GtkWidget *previous = NULL;

            for (;;) {
                const char *slash = strchr (prev, '/');
                if (slash && *(slash-1) != '\\') {
                    char name[slash-prev+1];
                    // replace \/ with /
                    const char *p = prev;
                    char *t = name;
                    while (*p && p < slash) {
                        if (*p == '\\' && *(p+1) == '/') {
                            *t++ = '/';
                            p += 2;
                        }
                        else {
                            *t++ = *p++;
                        }
                    }
                    *t = 0;

                    char menuname [1024];

                    snprintf (menuname, sizeof (menuname), "%s_menu", name);

                    previous = current;
                    current = (GtkWidget*) g_object_get_data (G_OBJECT (menu), menuname);
                    if (!current) {
                        current = (GtkWidget*) g_object_get_data (G_OBJECT (mainwin), menuname);
                    }
                    if (!current)
                    {
                        GtkWidget *newitem;

                        newitem = gtk_menu_item_new_with_mnemonic (_(name));
                        gtk_widget_show (newitem);

                        //If we add new submenu in main bar, add it before 'Help'
                        if (NULL == prev_title)
                            gtk_menu_shell_insert (GTK_MENU_SHELL (previous), newitem, 4);
                        else
                            gtk_container_add (GTK_CONTAINER (previous), newitem);

                        current = gtk_menu_new ();
                        gtk_menu_item_set_submenu (GTK_MENU_ITEM (newitem), current);
                        HOOKUP_OBJECT (menu, current, menuname);
                    }
                    free (prev_title);
                    prev_title = strdup (name);
                }
                else {
                    break;
                }
                prev = slash+1;
            }


            count++;
            added_entries++;
            GtkWidget *actionitem;

            // replace \/ with /
            const char *p = current ? prev : action->title;
            char title[strlen (p)+1];
            char *t = title;
            while (*p) {
                if (*p == '\\' && *(p+1) == '/') {
                    *t++ = '/';
                    p += 2;
                }
                else {
                    *t++ = *p++;
                }
            }
            *t = 0;

            actionitem = gtk_menu_item_new_with_mnemonic (_(title));
            gtk_widget_show (actionitem);

            if (action_context == DDB_ACTION_CTX_MAIN && prev_title && 0 == strcmp ("File", prev_title))
                gtk_menu_shell_insert (GTK_MENU_SHELL (current), actionitem, 5);
            else if (action_context == DDB_ACTION_CTX_MAIN && prev_title && 0 == strcmp ("Edit", prev_title))
                gtk_menu_shell_insert (GTK_MENU_SHELL (current), actionitem, 7);
            else {
                gtk_container_add (GTK_CONTAINER (current), actionitem);
            }

            free(prev_title);
            prev_title = NULL;

            g_object_set_data (G_OBJECT (actionitem), "plugaction", action);

            g_signal_connect ((gpointer) actionitem, "activate",
                              G_CALLBACK (activate_callback),
                              action);

            int is_playlist_action = (action->flags & DB_ACTION_PLAYLIST) && action_context == DDB_ACTION_CTX_PLAYLIST;

            if (!is_playlist_action) {
                if ((selected_count > 1 && !(action->flags & DB_ACTION_MULTIPLE_TRACKS)) ||
                    (action->flags & DB_ACTION_DISABLED)) {
                    gtk_widget_set_sensitive (GTK_WIDGET (actionitem), FALSE);
                }
            }
        }
        if (count > 0 && deadbeef->conf_get_int ("gtkui.action_separators", 0))
        {
            GtkWidget *separator = gtk_separator_menu_item_new ();
            gtk_widget_show (separator);
            gtk_container_add (GTK_CONTAINER (menu), separator);
            gtk_widget_set_sensitive (separator, FALSE);
        }
    }
    return added_entries;
}


void
add_mainmenu_actions (void) {
    GtkWidget *menubar = lookup_widget (mainwin, "menubar");
    // remove all plugaction_*** menu items and empty submenus
    gtk_container_foreach (GTK_CONTAINER (menubar), remove_actions, menubar);

    menu_add_action_items(menubar, 0, NULL, DDB_ACTION_CTX_MAIN, on_actionitem_activate);
}

void
gtkui_exec_action_14 (DB_plugin_action_t *action, int cursor) {
    // Plugin can handle all tracks by itself
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    if (action->flags & DB_ACTION_CAN_MULTIPLE_TRACKS)
    {
        action->callback (action, NULL);
        return;
    }
#pragma GCC diagnostic pop

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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        action->callback (action, it);
#pragma GCC diagnostic pop
        deadbeef->pl_item_unref (it);
        return;
    }

    //We end up here if plugin won't traverse tracks and we have to do it ourselves
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            action->callback (action, it);
#pragma GCC diagnostic pop
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}
