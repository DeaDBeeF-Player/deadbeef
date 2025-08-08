/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <assert.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtkui.h"
#include "support.h"
#include "trkproperties.h"
#include "selpropertieswidget.h"

extern DB_functions_t *deadbeef;
extern int design_mode;

typedef enum {
    SECTION_PROPERTIES = 1 << 0,
    SECTION_METADATA = 1 << 1,
} selproperties_section_t;

typedef struct {
    ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
    GtkWidget *tree;
    guint refresh_timeout;

    guint visible_sections;
    guint show_headers;

    gboolean updating_menu; // suppress menu event handlers
    GtkWidget *menu;
    GtkWidget *menu_copy;
    GtkWidget *menu_properties;
    GtkWidget *menu_metadata;
} w_selproperties_t;

gboolean
fill_selproperties_cb (gpointer data) {
    w_selproperties_t *w = data;
    DB_playItem_t **tracks = NULL;
    if (w->refresh_timeout) {
        g_source_remove (w->refresh_timeout);
        w->refresh_timeout = 0;
    }
    int numtracks = 0;
    deadbeef->pl_lock ();
    int nsel = deadbeef->pl_getselcount ();
    if (0 < nsel) {
        tracks = malloc (sizeof (DB_playItem_t *) * nsel);
        if (tracks) {
            int n = 0;
            DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
            while (it) {
                if (deadbeef->pl_is_selected (it)) {
                    assert (n < nsel);
                    deadbeef->pl_item_ref (it);
                    tracks[n++] = it;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
            }
            numtracks = nsel;
        }
        else {
            deadbeef->pl_unlock ();
            return FALSE;
        }
    }
    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (w->tree)));
    gtk_list_store_clear (store);
    if (w->visible_sections & SECTION_PROPERTIES) {
        add_field_section(store, _("Properties"), "");
        trkproperties_fill_prop (store, tracks, numtracks);
    }
    if (w->visible_sections & SECTION_METADATA) {
        add_field_section(store, _("Metadata"), "");
        trkproperties_fill_meta (store, tracks, numtracks);
    }

    if (tracks) {
        for (int i = 0; i < numtracks; i++) {
            deadbeef->pl_item_unref (tracks[i]);
        }
        free (tracks);
        tracks = NULL;
        numtracks = 0;
    }
    deadbeef->pl_unlock ();
    return FALSE;
}

static void
_init (struct ddb_gtkui_widget_s *widget) {
    w_selproperties_t *w = (w_selproperties_t *)widget;
    w->refresh_timeout = 0;
    fill_selproperties_cb (widget);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), w->show_headers);
}

static void
_menu_copy_activate (GtkWidget* self, gpointer user_data) {
    w_selproperties_t *s = user_data;

    GtkTreeModel* model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (s->tree)), &model, &iter)) {
        gchar *buffer;
        gtk_tree_model_get (model, &iter, META_COL_DISPLAY_VAL, &buffer, -1);

        GdkDisplay *display = gtk_widget_get_display (mainwin);
        GtkClipboard *clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text (clipboard, buffer, -1);

        g_free (buffer);
    }
}

static void
_menu_activate (GtkWidget* self, gpointer user_data) {
    w_selproperties_t *s = user_data;

    if (s->updating_menu) {
        return;
    }

    if (self == s->menu_properties) {
        s->visible_sections ^= SECTION_PROPERTIES;
    }
    else if (self == s->menu_metadata) {
        s->visible_sections ^= SECTION_METADATA;
    }
    fill_selproperties_cb (s);
}

static void
selection_changed (gpointer user_data)
{
    w_selproperties_t *s = user_data;
    if (s->refresh_timeout) {
        g_source_remove (s->refresh_timeout);
        s->refresh_timeout = 0;
    }
    s->refresh_timeout = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 10, fill_selproperties_cb, user_data, NULL);
}

static int
_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_TRACKINFOCHANGED:
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT || p1 == DDB_PLAYLIST_CHANGE_SELECTION) {
            selection_changed (w);
        }
        break;
    case DB_EV_PLAYLISTSWITCHED:
        selection_changed (w);
        break;
    }
    return 0;
}

static void
_menu_update (w_selproperties_t *s) {
    s->updating_menu = TRUE;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(s->menu_properties), s->visible_sections & SECTION_PROPERTIES);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(s->menu_metadata), s->visible_sections & SECTION_METADATA);
    s->updating_menu = FALSE;
}

static gboolean
_button_press (GtkWidget* self, GdkEventButton *event, gpointer user_data) {
    if (design_mode) {
        return FALSE;
    }
    w_selproperties_t *s = user_data;

    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        _menu_update(s);
        gtk_menu_popup_at_pointer (GTK_MENU (s->menu), NULL);
        return FALSE;
    }

    return FALSE;
}

static void
_deserialize_from_keyvalues (ddb_gtkui_widget_t *widget, const char **keyvalues) {
    w_selproperties_t *s = (w_selproperties_t *)widget;

    s->visible_sections = 0;
    s->show_headers = 1;
    gboolean live = FALSE;

    for (int i = 0; keyvalues[i]; i += 2) {
        if (!strcmp (keyvalues[i], "section")) {
            live = TRUE;
            char *sect = strdup(keyvalues[i+1]);
            char *token = strtok(sect, ",");

            while (token) {
                if (!strcmp(token, "properties")) {
                    s->visible_sections |= SECTION_PROPERTIES;
                }
                else if (!strcmp(token, "metadata")) {
                    s->visible_sections |= SECTION_METADATA;
                }
                token = strtok(NULL, ",");
            }

            free(sect);

        } else if (!strcmp (keyvalues[i], "showheaders")) {
            s->show_headers = atoi(keyvalues[i+1]);
        }
    }

    if (!live) {
        s->visible_sections = SECTION_PROPERTIES | SECTION_METADATA;
    }
}

static char const **
_serialize_to_keyvalues (ddb_gtkui_widget_t *widget) {
    w_selproperties_t *s = (w_selproperties_t *)widget;

    char const **keyvalues = calloc (5, sizeof (char *));

    keyvalues[0] = "section";

    if (s->visible_sections == (SECTION_PROPERTIES | SECTION_METADATA)) {
        keyvalues[1] = "properties,metadata";
    } else if (s->visible_sections == SECTION_PROPERTIES) {
        keyvalues[1] = "properties";
    } else if (s->visible_sections == SECTION_METADATA) {
        keyvalues[1] = "metadata";
    } else {
        keyvalues[1] = "";
    }

    keyvalues[2] = "showheaders";
    keyvalues[3] = s->show_headers ? "1" : "0";

    return keyvalues;
}

static void
_free_serialized_keyvalues(ddb_gtkui_widget_t *w, char const **keyvalues) {
    free (keyvalues);
}

static void
on_properties_showheaders_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_selproperties_t *w = user_data;
    int showheaders = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem));
    w->show_headers = showheaders;
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), showheaders);
}

static void
_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    w_selproperties_t *s = (w_selproperties_t *)w;
    GtkWidget *item;
    item = gtk_check_menu_item_new_with_mnemonic (_("Show Column Headers"));
    gtk_widget_show (item);
    int showheaders = s->show_headers;
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), showheaders);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_properties_showheaders_toggled),
            w);
}

ddb_gtkui_widget_t *
w_selproperties_create (void) {
    w_selproperties_t *w = malloc (sizeof (w_selproperties_t));
    memset (w, 0, sizeof (w_selproperties_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = _init;
    w->base.message = _message;
    w->base.initmenu = _initmenu;
    w->visible_sections = SECTION_METADATA | SECTION_PROPERTIES;
    w->exapi._size = sizeof (ddb_gtkui_widget_extended_api_t);
    w->exapi.deserialize_from_keyvalues = _deserialize_from_keyvalues;
    w->exapi.serialize_to_keyvalues = _serialize_to_keyvalues;
    w->exapi.free_serialized_keyvalues = _free_serialized_keyvalues;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_container_add (GTK_CONTAINER (w->base.widget), scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);

    w->tree = gtk_tree_view_new ();
    gtk_widget_show (w->tree);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_container_add (GTK_CONTAINER (scroll), w->tree);

    GtkListStore *store = gtk_list_store_new (META_COL_COUNT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);

    GtkCellRenderer *rend1 = gtk_cell_renderer_text_new ();
    GtkCellRenderer *rend2 = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes (_("Name"), rend1, "text", META_COL_TITLE, "weight", META_COL_PANGO_WEIGHT,  NULL);
    gtk_tree_view_column_set_sizing (col1, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend2, "text", META_COL_DISPLAY_VAL, NULL);
    gtk_tree_view_column_set_sizing (col2, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (w->tree), col1);
    gtk_tree_view_append_column (GTK_TREE_VIEW (w->tree), col2);
    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (w->tree), TRUE);

    w_override_signals (w->base.widget, w);
    g_signal_connect ((gpointer)w->tree, "button-press-event", G_CALLBACK (_button_press), w);

    w->menu = gtk_menu_new();
    w->menu_copy = gtk_menu_item_new_with_mnemonic( _("Copy"));
    gtk_widget_show(w->menu_copy);
    w->menu_properties = gtk_check_menu_item_new_with_mnemonic( _("Properties"));
    gtk_widget_show(w->menu_properties);
    w->menu_metadata = gtk_check_menu_item_new_with_mnemonic( _("Metadata"));
    gtk_widget_show(w->menu_metadata);
    gtk_menu_shell_insert (GTK_MENU_SHELL(w->menu), w->menu_copy, 0);
    gtk_menu_shell_insert (GTK_MENU_SHELL(w->menu), w->menu_properties, 1);
    gtk_menu_shell_insert (GTK_MENU_SHELL(w->menu), w->menu_metadata, 2);

    g_signal_connect((gpointer)w->menu_copy, "activate", G_CALLBACK(_menu_copy_activate), w);
    g_signal_connect((gpointer)w->menu_properties, "activate", G_CALLBACK(_menu_activate), w);
    g_signal_connect((gpointer)w->menu_metadata, "activate", G_CALLBACK(_menu_activate), w);

    return (ddb_gtkui_widget_t *)w;
}
