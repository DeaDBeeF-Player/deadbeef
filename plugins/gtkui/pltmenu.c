/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2013 Alexey Yakovenko and other contributors

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
#include "../../deadbeef.h"
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include "../../gettext.h"
#include <string.h>
#include <stdlib.h>
#include "gtkui.h"
#include "clipboard.h"
#include "interface.h"
#include "support.h"

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

// selected playlist for the context menu
static int pltmenu_idx;

void
plt_get_title_wrapper (int plt, char *buffer, int len) {
    if (plt == -1) {
        strcpy (buffer, "");
        return;
    }
    ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
    deadbeef->plt_get_title (p, buffer, len);
    deadbeef->plt_unref (p);
    char *end;
    if (!g_utf8_validate (buffer, -1, (const gchar **)&end)) {
        *end = 0;
    }
}

static void
on_rename_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_entrydialog ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Rename Playlist"));
    GtkWidget *e;
    e = lookup_widget (dlg, "title_label");
    gtk_label_set_text (GTK_LABEL(e), _("Title:"));
    e = lookup_widget (dlg, "title");
    char t[1000];
    plt_get_title_wrapper (pltmenu_idx, t, sizeof (t));
    gtk_entry_set_text (GTK_ENTRY (e), t);
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        const char *text = gtk_entry_get_text (GTK_ENTRY (e));
        deadbeef->pl_lock ();
        ddb_playlist_t *p = deadbeef->plt_get_for_idx (pltmenu_idx);
        deadbeef->plt_set_title (p, text);
        deadbeef->plt_unref (p);
        deadbeef->pl_unlock ();
    }
    gtk_widget_destroy (dlg);
}

static void
on_remove_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (pltmenu_idx != -1) {
        deadbeef->plt_remove (pltmenu_idx);
        int playlist = deadbeef->plt_get_curr_idx ();
        deadbeef->conf_set_int ("playlist.current", playlist);
    }
}

static void
on_add_new_playlist1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    int playlist = gtkui_add_new_playlist ();
    if (playlist != -1) {
        gtkui_playlist_set_curr (playlist);
    }
}

static void
on_copy_playlist1_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (pltmenu_idx);
    if (plt) {
        int playlist = gtkui_copy_playlist (plt);
        if (playlist != -1) {
            gtkui_playlist_set_curr (playlist);
        }
    }
}

static void
on_autosort_toggled (GtkMenuItem     *menuitem,
                    gpointer         user_data)
{
    if (pltmenu_idx < 0) {
        return;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (pltmenu_idx);
    if (plt) {
        int enabled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(menuitem));
        deadbeef->plt_set_meta_int (plt, "autosort_enabled", enabled);
        deadbeef->plt_unref (plt);
    }
}

static void
on_actionitem_activate (GtkMenuItem     *menuitem,
                           DB_plugin_action_t *action)
{
    if (action->callback) {
        ddb_playlist_t *plt = NULL;
        if (pltmenu_idx != -1) {
            plt = deadbeef->plt_get_for_idx (pltmenu_idx);
        }
        action->callback (action, plt);
        if (plt) {
            deadbeef->plt_unref (plt);
        }
    }
    else {
        ddb_playlist_t *plt = NULL;
        if (pltmenu_idx != -1) {
            plt = deadbeef->plt_get_for_idx (pltmenu_idx);
            if (plt) {
                deadbeef->action_set_playlist (plt);
                deadbeef->plt_unref (plt);
                action->callback2 (action, DDB_ACTION_CTX_PLAYLIST);
                deadbeef->action_set_playlist (NULL);
            }
        }
    }
}

static void
on_cut_activate (GtkMenuItem     *menuitem,
                    gpointer         user_data)
{
    if (pltmenu_idx < 0) {
        return;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (pltmenu_idx);
    if (plt) {
        clipboard_cut_selection (plt, DDB_ACTION_CTX_PLAYLIST);
        deadbeef->plt_unref (plt);
    }
}

static void
on_copy_activate (GtkMenuItem     *menuitem,
                    gpointer         user_data)
{
    if (pltmenu_idx < 0) {
        return;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (pltmenu_idx);
    if (plt) {
        clipboard_copy_selection (plt, DDB_ACTION_CTX_PLAYLIST);
        deadbeef->plt_unref (plt);
    }
}

static void
on_paste_activate (GtkMenuItem     *menuitem,
                    gpointer         user_data)
{
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (pltmenu_idx);
    clipboard_paste_selection (plt, DDB_ACTION_CTX_PLAYLIST);
    if (plt) {
        deadbeef->plt_unref (plt);
    }
}


static GtkWidget*
find_popup                          (GtkWidget       *widget,
                                        const gchar     *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = gtk_widget_get_parent (widget);
      if (!parent)
        parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
                                                 widget_name);
  return found_widget;
}

static void
add_tab_actions (GtkWidget *menu) {
    DB_plugin_t **plugins = deadbeef->plug_get_list();
    int i;

    int added_entries = 0;

    int item_count = 0;
    int playqueue_test = 0;

    ddb_playlist_t *plt = NULL;
    if (pltmenu_idx != -1) {
        plt = deadbeef->plt_get_for_idx (pltmenu_idx);
    }
    if (plt) {
        item_count = deadbeef->plt_get_item_count (plt, PL_MAIN);

        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            if (deadbeef->playqueue_test (it) != -1) {
                playqueue_test = 1;
                deadbeef->pl_item_unref (it);
                break;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }
        deadbeef->plt_unref (plt);
    }

    int hide_remove_from_disk = deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0);

    for (i = 0; plugins[i]; i++)
    {
        if (!plugins[i]->get_actions)
            continue;

        DB_plugin_action_t *actions = plugins[i]->get_actions (NULL);
        DB_plugin_action_t *action;

        int count = 0;
        for (action = actions; action; action = action->next)
        {
            char *tmp = NULL;
            if (!(action->flags & DB_ACTION_MULTIPLE_TRACKS))
                continue;

            if (action->flags & DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST)
                continue;

            if (action->name && !strcmp (action->name, "delete_from_disk") && hide_remove_from_disk) {
                continue;
            }

            int sensitive = item_count ? 1 : 0;

            // create submenus (separated with '/')
            const char *prev = action->title;
            while (*prev && *prev == '/') {
                prev++;
            }

            GtkWidget *popup = NULL;

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

                    // add popup
                    GtkWidget *prev_menu = popup ? popup : menu;

                    popup = find_popup (prev_menu, name);
                    if (!popup) {
                        GtkWidget *item = gtk_image_menu_item_new_with_mnemonic (_(name));
                        gtk_widget_set_sensitive (item, sensitive);
                        gtk_widget_show (item);
                        gtk_container_add (GTK_CONTAINER (prev_menu), item);
                        popup = gtk_menu_new ();
                        GLADE_HOOKUP_OBJECT_NO_REF (prev_menu, popup, name);
                        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), popup);
                    }
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
            const char *p = popup ? prev : action->title;
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

            if (action->name && !strcmp (action->name, "remove_from_playback_queue")) {
                if (!playqueue_test) {
                    // no playitem in queue, make menu item insenstive
                    sensitive = 0;
                }
            }

            actionitem = gtk_menu_item_new_with_mnemonic (_(title));
            gtk_widget_set_sensitive (actionitem, sensitive);
            gtk_widget_show (actionitem);
            gtk_container_add (popup ? GTK_CONTAINER (popup) : GTK_CONTAINER (menu), actionitem);

            g_signal_connect ((gpointer) actionitem, "activate",
                    G_CALLBACK (on_actionitem_activate),
                    action);
        }
    }
}

GtkWidget*
gtkui_create_pltmenu (int plt_idx) {
    GtkWidget *plmenu;
    GtkWidget *rename_playlist1;
    GtkWidget *remove_playlist1;
    GtkWidget *add_new_playlist1;
    GtkWidget *copy_playlist1;
    GtkWidget *autosort;
    GtkWidget *separator11;
    GtkWidget *cut;
    GtkWidget *cut_image;
    GtkWidget *copy;
    GtkWidget *copy_image;
    GtkWidget *paste;
    GtkWidget *paste_image;
    GtkWidget *separator9;
    GtkWidget *load_playlist1;
    GtkWidget *save_playlist1;
    GtkWidget *save_all_playlists1;

    GtkAccelGroup *accel_group = NULL;
    accel_group = gtk_accel_group_new ();

    plmenu = gtk_menu_new ();
    pltmenu_idx = plt_idx;

    rename_playlist1 = gtk_menu_item_new_with_mnemonic (_("Rename Playlist"));
    if (pltmenu_idx == -1) {
        gtk_widget_set_sensitive (rename_playlist1, FALSE);
    }
    gtk_widget_show (rename_playlist1);
    gtk_container_add (GTK_CONTAINER (plmenu), rename_playlist1);

    remove_playlist1 = gtk_menu_item_new_with_mnemonic (_("Remove Playlist"));
    if (pltmenu_idx == -1) {
        gtk_widget_set_sensitive (remove_playlist1, FALSE);
    }
    gtk_widget_show (remove_playlist1);
    gtk_container_add (GTK_CONTAINER (plmenu), remove_playlist1);

    add_new_playlist1 = gtk_menu_item_new_with_mnemonic (_("Add New Playlist"));
    gtk_widget_show (add_new_playlist1);
    gtk_container_add (GTK_CONTAINER (plmenu), add_new_playlist1);

    copy_playlist1 = gtk_menu_item_new_with_mnemonic (_("Duplicate Playlist"));
    gtk_widget_show (copy_playlist1);
    gtk_container_add (GTK_CONTAINER (plmenu), copy_playlist1);

    int autosort_enabled = 0;
    if (pltmenu_idx >= 0) {
        autosort_enabled = deadbeef->plt_find_meta_int (deadbeef->plt_get_for_idx (pltmenu_idx), "autosort_enabled", 0);
    }
    autosort = gtk_check_menu_item_new_with_label (_("Enable Autosort"));
    gtk_check_menu_item_set_active (autosort, autosort_enabled);
    gtk_widget_show (autosort);
    gtk_container_add (GTK_CONTAINER (plmenu), autosort);
    
    separator11 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator11);
    gtk_container_add (GTK_CONTAINER (plmenu), separator11);
    gtk_widget_set_sensitive (separator11, FALSE);

    cut = gtk_image_menu_item_new_with_mnemonic (_("Cu_t"));
    gtk_widget_show (cut);
    gtk_container_add (GTK_CONTAINER (plmenu), cut);
    gtk_widget_add_accelerator (cut, "activate", accel_group, GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    if (pltmenu_idx == -1) {
        gtk_widget_set_sensitive (cut, FALSE);
    }

    cut_image = gtk_image_new_from_stock ("gtk-cut", GTK_ICON_SIZE_MENU);
    gtk_widget_show (cut_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (cut), cut_image);

    copy = gtk_image_menu_item_new_with_mnemonic (_("_Copy"));
    gtk_widget_show (copy);
    gtk_container_add (GTK_CONTAINER (plmenu), copy);
    gtk_widget_add_accelerator (copy, "activate", accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    if (pltmenu_idx == -1) {
        gtk_widget_set_sensitive (copy, FALSE);
    }

    copy_image = gtk_image_new_from_stock ("gtk-copy", GTK_ICON_SIZE_MENU);
    gtk_widget_show (copy_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (copy), copy_image);

    paste = gtk_image_menu_item_new_with_mnemonic (_("_Paste"));
    gtk_widget_show (paste);
    gtk_container_add (GTK_CONTAINER (plmenu), paste);
    gtk_widget_add_accelerator (paste, "activate", accel_group, GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    if (clipboard_is_clipboard_data_available ()) {
        gtk_widget_set_sensitive (paste, TRUE);
    }
    else {
        gtk_widget_set_sensitive (paste, FALSE);
    }

    paste_image = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_MENU);
    gtk_widget_show (paste_image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (paste), paste_image);

    separator9 = gtk_separator_menu_item_new ();
    gtk_widget_show (separator9);
    gtk_container_add (GTK_CONTAINER (plmenu), separator9);
    gtk_widget_set_sensitive (separator9, FALSE);

    g_signal_connect ((gpointer) rename_playlist1, "activate",
            G_CALLBACK (on_rename_playlist1_activate),
            NULL);
    g_signal_connect ((gpointer) remove_playlist1, "activate",
            G_CALLBACK (on_remove_playlist1_activate),
            NULL);
    g_signal_connect ((gpointer) add_new_playlist1, "activate",
            G_CALLBACK (on_add_new_playlist1_activate),
            NULL);
    g_signal_connect ((gpointer) autosort, "toggled",
            G_CALLBACK (on_autosort_toggled),
            NULL);
    g_signal_connect ((gpointer) copy_playlist1, "activate",
            G_CALLBACK (on_copy_playlist1_activate),
            NULL);
    g_signal_connect ((gpointer) cut, "activate",
            G_CALLBACK (on_cut_activate),
            NULL);
    g_signal_connect ((gpointer) copy, "activate",
            G_CALLBACK (on_copy_activate),
            NULL);
    g_signal_connect ((gpointer) paste, "activate",
            G_CALLBACK (on_paste_activate),
            NULL);

    add_tab_actions (plmenu);

    return plmenu;
}

