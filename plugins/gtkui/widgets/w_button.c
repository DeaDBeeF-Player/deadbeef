/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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



#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <deadbeef/strdupa.h>
#include "../../../shared/parser.h"
#include "../actions.h"
#include "../gtkui.h"
#include "../hotkeys.h"
#include "../interface.h"
#include "../namedicons.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_button.h"

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *button;
    GtkWidget *alignment;
    GdkColor color;
    GdkColor textcolor;
    char *icon;
    char *label;
    char *action;
    int action_ctx;
    unsigned use_color : 1;
    unsigned use_textcolor : 1;
} w_button_t;

// button widget
static const char *
w_button_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    if (strcmp (type, "button")) {
        return NULL;
    }
    w_button_t *b = (w_button_t *)w;
    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s, key, val);
        if (!strcmp (key, "color")) {
            int red, green, blue;
            if (3 == sscanf (val, "#%02x%02x%02x", &red, &green, &blue)) {
                b->color.red = red << 8;
                b->color.green = green << 8;
                b->color.blue = blue << 8;
            }
        }
        else if (!strcmp (key, "textcolor")) {
            int red, green, blue;
            if (3 == sscanf (val, "#%02x%02x%02x", &red, &green, &blue)) {
                b->textcolor.red = red << 8;
                b->textcolor.green = green << 8;
                b->textcolor.blue = blue << 8;
            }
        }
        else if (!strcmp (key, "icon")) {
            b->icon = val[0] ? strdup (val) : NULL;
        }
        else if (!strcmp (key, "label")) {
            b->label = strdup (val);
        }
        else if (!strcmp (key, "action")) {
            b->action = val[0] ? strdup (val) : NULL;
        }
        else if (!strcmp (key, "action_ctx")) {
            b->action_ctx = atoi (val);
        }
        else if (!strcmp (key, "use_color")) {
            b->use_color = atoi (val);
        }
        else if (!strcmp (key, "use_textcolor")) {
            b->use_textcolor = atoi (val);
        }
    }

    return s;
}

static void
w_button_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    char save[1000] = "";
    char *pp = save;
    int ss = sizeof (save);
    int n;

    w_button_t *b = (w_button_t *)w;
    n = snprintf (pp, ss, " color=\"#%02x%02x%02x\"", b->color.red >> 8, b->color.green >> 8, b->color.blue >> 8);
    ss -= n;
    pp += n;
    n = snprintf (pp, ss, " textcolor=\"#%02x%02x%02x\"", b->textcolor.red >> 8, b->textcolor.green >> 8, b->textcolor.blue >> 8);
    ss -= n;
    pp += n;
    if (b->icon) {
        n = snprintf (pp, ss, " icon=\"%s\"", b->icon);
        ss -= n;
        pp += n;
    }
    if (b->label) {
        n = snprintf (pp, ss, " label=\"%s\"", b->label);
        ss -= n;
        pp += n;
    }
    if (b->action) {
        n = snprintf (pp, ss, " action=\"%s\"", b->action);
        ss -= n;
        pp += n;
    }
    if (b->action_ctx) {
        n = snprintf (pp, ss, " action_ctx=%d", (int)b->action_ctx);
        ss -= n;
        pp += n;
    }

    n = snprintf (pp, ss, " use_color=%d", (int)b->use_color);
    ss -= n;
    pp += n;

    n = snprintf (pp, ss, " use_textcolor=%d", (int)b->use_textcolor);
    ss -= n;
    pp += n;

    strncat (s, save, sz);
}

static void
on_button_clicked (GtkButton *button, gpointer user_data) {
    w_button_t *w = user_data;
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugins[i]; i++) {
        if (!plugins[i]->get_actions) {
            continue;
        }
        DB_plugin_action_t *acts = plugins[i]->get_actions (NULL);
        while (acts) {
            if (!strcmp (acts->name, w->action)) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                if (acts->callback) {
#pragma GCC diagnostic pop
                    gtkui_exec_action_14 (acts, -1);
                }
                else if (acts->callback2) {
                    acts->callback2 (acts, w->action_ctx);
                }
                return;
            }
            acts = acts->next;
        }
    }
}

static void
w_button_init (ddb_gtkui_widget_t *ww) {
    w_button_t *w = (w_button_t *)ww;

    // clean before re-creating
    if (w->button) {
        gtk_widget_destroy (w->button);
        w->button = NULL;
    }

    w->button = gtk_button_new ();
    gtk_widget_show (w->button);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->button);

    GtkWidget *alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_widget_show (alignment);
    gtk_container_add (GTK_CONTAINER (w->button), alignment);

    GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (alignment), hbox);

    if (w->icon) {
        GtkWidget *image = gtk_image_new_from_stock (w->icon, GTK_ICON_SIZE_BUTTON);
        gtk_widget_show (image);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    }

    GtkWidget *label = gtk_label_new_with_mnemonic (w->label ? w->label : _ ("Button"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    if (w->use_color) {
        gtk_widget_modify_bg (w->button, GTK_STATE_NORMAL, &w->color);
    }

    if (w->use_textcolor) {
        gtk_widget_modify_fg (label, GTK_STATE_NORMAL, &w->textcolor);
    }

    if (w->action) {
        g_signal_connect ((gpointer)w->button, "clicked", G_CALLBACK (on_button_clicked), w);
    }

    w_override_signals (w->button, w);
}

static void
w_button_destroy (ddb_gtkui_widget_t *w) {
    w_button_t *b = (w_button_t *)w;
    if (b->icon) {
        free (b->icon);
    }
    if (b->label) {
        free (b->label);
    }
    if (b->action) {
        free (b->action);
    }
}

static void
on_button_set_action_clicked (GtkButton *button, gpointer user_data) {
    w_button_t *b = user_data;
    GtkWidget *dlg = create_select_action ();
    GtkWidget *treeview = lookup_widget (dlg, "actions");
    init_action_tree (treeview, b->action, b->action_ctx);
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        if (b->action) {
            free (b->action);
            b->action = NULL;
        }
        b->action_ctx = -1;
        GtkTreePath *path;
        gtk_tree_view_get_cursor (GTK_TREE_VIEW (treeview), &path, NULL);
        GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        GtkTreeIter iter;
        if (path && gtk_tree_model_get_iter (model, &iter, path)) {
            GValue val = {
                0,
            };
            gtk_tree_model_get_value (model, &iter, 1, &val);
            const gchar *name = g_value_get_string (&val);
            GValue val_ctx = {
                0,
            };
            gtk_tree_model_get_value (model, &iter, 2, &val_ctx);
            int ctx = g_value_get_int (&val_ctx);
            if (name && ctx >= 0) {
                b->action = strdup (name);
                b->action_ctx = ctx;
            }
        }
        set_button_action_label (b->action, b->action_ctx, GTK_WIDGET (button));
    }
    gtk_widget_destroy (dlg);
}

static void
on_button_config (GtkMenuItem *menuitem, gpointer user_data) {
    w_button_t *b = user_data;
    GtkWidget *dlg = create_button_properties ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    GtkWidget *color = lookup_widget (dlg, "color");
    GtkWidget *use_color = lookup_widget (dlg, "use_color");
    GtkWidget *textcolor = lookup_widget (dlg, "textcolor");
    GtkWidget *use_textcolor = lookup_widget (dlg, "use_textcolor");
    GtkWidget *label = lookup_widget (dlg, "label");
    GtkWidget *action = lookup_widget (dlg, "action");
    GtkWidget *icon = lookup_widget (dlg, "icon");
    gtk_color_button_set_color (GTK_COLOR_BUTTON (color), &b->color);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_color), b->use_color);
    gtk_color_button_set_color (GTK_COLOR_BUTTON (textcolor), &b->textcolor);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (use_textcolor), b->use_textcolor);
    gtk_entry_set_text (GTK_ENTRY (label), b->label ? b->label : _ ("Button"));
    set_button_action_label (b->action, b->action_ctx, action);
    g_signal_connect ((gpointer)action, "clicked", G_CALLBACK (on_button_set_action_clicked), user_data);

    GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, NULL, 1, _ ("None"), -1);
    int sel = 0;
    for (int n = 0; GtkNamedIcons[n]; n++) {
        gtk_list_store_append (store, &iter);

        GtkStockItem it;
        if (gtk_stock_lookup (GtkNamedIcons[n], &it)) {
            char *s = strdupa (it.label);
            for (char *c = s; *c; c++) {
                if (*c == '_') {
                    memmove (c, c + 1, strlen (c));
                    c--;
                }
            }
            gtk_list_store_set (store, &iter, 0, GtkNamedIcons[n], 1, s, -1);
        }
        else {
            gtk_list_store_set (store, &iter, 0, GtkNamedIcons[n], 1, GtkNamedIcons[n], -1);
        }

        if (b->icon && !strcmp (GtkNamedIcons[n], b->icon)) {
            sel = n + 1;
        }
    }

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (icon));
    GtkCellRenderer *renderer;
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon), renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon), renderer, "stock-id", 0, NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon), renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon), renderer, "text", 1, NULL);

    gtk_combo_box_set_model (GTK_COMBO_BOX (icon), GTK_TREE_MODEL (store));

    gtk_combo_box_set_active (GTK_COMBO_BOX (icon), sel);

    for (;;) {
        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY) {
            gtk_color_button_get_color (GTK_COLOR_BUTTON (color), &b->color);
            b->use_color = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_color));
            gtk_color_button_get_color (GTK_COLOR_BUTTON (textcolor), &b->textcolor);
            b->use_textcolor = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_textcolor));
            const char *l = gtk_entry_get_text (GTK_ENTRY (label));
            if (b->label) {
                free (b->label);
                b->label = NULL;
            }
            b->label = strdup (l);

            const char *ic = NULL;
            int sel = gtk_combo_box_get_active (GTK_COMBO_BOX (icon));
            if (sel >= 1) {
                ic = GtkNamedIcons[sel - 1];
            }
            if (b->icon) {
                free (b->icon);
                b->icon = NULL;
            }
            if (ic) {
                b->icon = strdup (ic);
            }

            w_button_init (user_data);
            w_save_with_action (_("Configure Button"));
        }
        if (response == GTK_RESPONSE_APPLY) {
            continue;
        }
        break;
    }
    gtk_widget_destroy (dlg);
}

static void
w_button_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_mnemonic (_ ("Configure Button"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_button_config), w);
}

ddb_gtkui_widget_t *
w_button_create (void) {
    w_button_t *w = malloc (sizeof (w_button_t));
    memset (w, 0, sizeof (w_button_t));
    w->base.widget = gtk_event_box_new ();
    w->base.load = w_button_load;
    w->base.save = w_button_save;
    w->base.init = w_button_init;
    w->base.destroy = w_button_destroy;
    w->base.initmenu = w_button_initmenu;
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
