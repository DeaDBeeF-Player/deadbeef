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



#include <ctype.h>
#include <gtk/gtk.h>
#include <jansson.h>
#include <stdlib.h>
#include <string.h>
#include "../../../shared/parser.h"
#include "../gtkui.h"
#include "../interface.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_tabs.h"

typedef struct {
    ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
    int clicked_page;
    int active;

    // These fields are used only during deserialization, and then get freed
    int num_tabs;
    char **titles;
} w_tabs_t;

///// tabs widget

static void
w_tabs_destroy (ddb_gtkui_widget_t *w) {
    w_tabs_t *s = (w_tabs_t *)w;
    if (s->titles) {
        for (int i = 0; i < s->num_tabs; i++) {
            if (s->titles[i]) {
                free (s->titles[i]);
            }
        }
        free (s->titles);
    }
}

static gboolean
_is_tab_title_key (const char *key) {
    if (strncmp (key, "tab", 3)) {
        return FALSE;
    }

    key += 3;
    while (*key) {
        if (!isdigit (*key)) {
            return FALSE;
        }
        key++;
    }

    return TRUE;
}

static void
w_tabs_deserialize_from_keyvalues (ddb_gtkui_widget_t *base, char const **keyvalues) {
    w_tabs_t *w = (w_tabs_t *)base;

    for (int i = 0; keyvalues[i]; i += 2) {
        if (!strcmp (keyvalues[i], "active")) {
            w->active = atoi (keyvalues[i + 1]);
        }
        else if (!strcmp (keyvalues[i], "num_tabs")) {
            w->num_tabs = atoi (keyvalues[i + 1]);
            w->titles = calloc (w->num_tabs, sizeof (char *));
        }
        else if (_is_tab_title_key (keyvalues[i])) {
            int index = atoi (keyvalues[i] + 3);
            w->titles[index] = strdup (keyvalues[i + 1]);
        }
    }
}

static char const **
w_tabs_serialize_to_keyvalues (ddb_gtkui_widget_t *base) {
    w_tabs_t *w = (w_tabs_t *)base;

    w->active = gtk_notebook_get_current_page (GTK_NOTEBOOK (w->base.widget));
    int num_tabs = gtk_notebook_get_n_pages (GTK_NOTEBOOK (w->base.widget));

    char const **keyvalues = calloc (2 * (2 + num_tabs) + 1, sizeof (char *));

    char temp[10];

    keyvalues[0] = "active";
    snprintf (temp, sizeof (temp), "%d", w->active);
    keyvalues[1] = strdup (temp);
    keyvalues[2] = "num_tabs";
    snprintf (temp, sizeof (temp), "%d", num_tabs);
    keyvalues[3] = strdup (temp);

    for (int i = 0; i < num_tabs; i++) {
        char key[7];

        GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (w->base.widget), i);
        const char *text = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (w->base.widget), child);
        char *esctext = parser_escape_string (text);

        snprintf (key, sizeof (key), "tab%03d", i);
        keyvalues[4 + i * 2 + 0] = strdup (key);
        keyvalues[4 + i * 2 + 1] = esctext;
    }
    return keyvalues;
}

static void
w_tabs_free_serialized_keyvalues (ddb_gtkui_widget_t *base, char const **keyvalues) {
    for (int i = 0; keyvalues[i]; i += 2) {
        if (i < 4) {
            free ((char *)keyvalues[i + 1]);
        }
        else {
            free ((char *)keyvalues[i]);
        }
    }
    free (keyvalues);
}

static void
tabs_add_tab (gpointer user_data) {
    w_tabs_t *w = user_data;

    ddb_gtkui_widget_t *ph;
    ph = w_create ("placeholder");
    w_append ((ddb_gtkui_widget_t *)w, ph);

    int i = 0;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++)
        ;
    w->clicked_page = i - 1;
    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->clicked_page);
}

static void
tabs_remove_tab (gpointer user_data, int tab) {
    w_tabs_t *w = user_data;
    int i = 0;
    int num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (w->base.widget));
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == tab) {
            w_remove ((ddb_gtkui_widget_t *)w, c);
            w_destroy (c);
            if (num_pages == 1) {
                // if last tab was deleted add a new placeholder tab
                tabs_add_tab (w);
            }
            return;
        }
    }
}

static void
on_rename_tab_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;

    GtkWidget *dlg = create_entrydialog ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _ ("Rename Tab"));
    GtkWidget *e;
    e = lookup_widget (dlg, "title_label");
    gtk_label_set_text (GTK_LABEL (e), _ ("Title:"));
    e = lookup_widget (dlg, "title");
    int active = gtk_notebook_get_current_page (GTK_NOTEBOOK (w->base.widget));
    GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (w->base.widget), active);
    gtk_entry_set_text (GTK_ENTRY (e), gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (w->base.widget), child));
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (w->base.widget), child, gtk_entry_get_text (GTK_ENTRY (e)));
        w_save_with_action (_("Rename Tab"));
    }
    gtk_widget_destroy (dlg);
}

static void
on_remove_tab_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;
    tabs_remove_tab (w, w->clicked_page);
    w_save_with_action (_("Remove Tab"));
}

static void
on_add_tab_activate (GtkMenuItem *menuitem, gpointer user_data) {
    tabs_add_tab (user_data);
    w_save_with_action (_("Add Tab"));
}

static void
on_move_tab_left_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;
    if (w->clicked_page <= 0) {
        return;
    }
    // remove and save widget
    int i = 0;
    ddb_gtkui_widget_t *newchild = NULL;
    ddb_gtkui_widget_t *prev = NULL;
    char *title = NULL;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == w->clicked_page) {
            json_t *layout = w_save_widget_to_json (c);
            GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (w->base.widget), i);
            title = strdup (gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (w->base.widget), child));
            w_remove ((ddb_gtkui_widget_t *)w, c);
            w_destroy (c);
            w_create_from_json (layout, &newchild);
            json_delete (layout);
            break;
        }
    }
    if (!newchild) {
        goto out;
    }

    // add new child at new position
    i = 0;
    prev = NULL;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == w->clicked_page - 1) {
            if (prev) {
                newchild->next = prev->next;
                prev->next = newchild;
            }
            else {
                newchild->next = w->base.children;
                w->base.children = newchild;
            }
            GtkWidget *label = gtk_label_new (title);
            gtk_widget_show (label);
            gtk_widget_show (newchild->widget);

            gtk_notebook_insert_page (GTK_NOTEBOOK (w->base.widget), newchild->widget, label, w->clicked_page - 1);
#if GTK_CHECK_VERSION(3, 0, 0)
            gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
            gtk_misc_set_padding (GTK_MISC (label), 0, 0);
            gtk_container_child_set (GTK_CONTAINER (w->base.widget), newchild->widget, "tab-expand", TRUE, "tab-fill", TRUE, NULL);
#endif
            gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->clicked_page - 1);
            w->clicked_page--;
            w_save_with_action (_("Move Tab"));
            break;
        }
        prev = c;
    }

out:
    if (title) {
        free (title);
    }
}

static void
on_move_tab_right_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;

    int num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (w->base.widget));
    if (w->clicked_page == num_pages - 1) {
        return;
    }

    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), ++w->clicked_page);
    on_move_tab_left_activate (menuitem, user_data);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), ++w->clicked_page);
}

static void
on_tab_popup_menu (GtkWidget *widget, gpointer user_data) {
    w_tabs_t *w = user_data;
    GtkWidget *menu;
    GtkWidget *item;
    menu = gtk_menu_new ();

    item = gtk_menu_item_new_with_mnemonic (_ ("Rename Tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_rename_tab_activate), w);

    item = gtk_menu_item_new_with_mnemonic (_ ("Remove Tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_remove_tab_activate), w);

    item = gtk_menu_item_new_with_mnemonic (_ ("Add New Tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_add_tab_activate), w);

    w_add_menu_separator (menu);

    item = gtk_menu_item_new_with_mnemonic (_ ("Move Tab Left"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_move_tab_left_activate), w);

    item = gtk_menu_item_new_with_mnemonic (_ ("Move Tab Right"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_move_tab_right_activate), w);

    gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
}

static void
w_tabs_add (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    const char *title = w_get_title (child);
    GtkWidget *label = gtk_label_new (title ? title : child->type);
    gtk_widget_show (label);
    gtk_widget_show (child->widget);
    gtk_notebook_append_page (GTK_NOTEBOOK (cont->widget), child->widget, label);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_misc_set_padding (GTK_MISC (label), 0, 0);
    gtk_container_child_set (GTK_CONTAINER (cont->widget), child->widget, "tab-expand", TRUE, "tab-fill", TRUE, NULL);
#endif
}

static void
w_tabs_replace (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child, ddb_gtkui_widget_t *newchild) {
    int ntab = 0;
    ddb_gtkui_widget_t *prev = NULL;
    for (ddb_gtkui_widget_t *c = cont->children; c; prev = c, c = c->next, ntab++) {
        if (c == child) {
            newchild->next = c->next;
            if (prev) {
                prev->next = newchild;
            }
            else {
                cont->children = newchild;
            }
            newchild->parent = cont;
            gtk_notebook_remove_page (GTK_NOTEBOOK (cont->widget), ntab);
            c->widget = NULL;
            w_destroy (c);
            const char *title = w_get_title (newchild);
            GtkWidget *label = gtk_label_new (title ? title : newchild->type);
            gtk_widget_show (label);
            gtk_widget_show (newchild->widget);
            int pos = gtk_notebook_insert_page (GTK_NOTEBOOK (cont->widget), newchild->widget, label, ntab);
#if GTK_CHECK_VERSION(3, 0, 0)
            gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
            gtk_misc_set_padding (GTK_MISC (label), 0, 0);
            gtk_container_child_set (GTK_CONTAINER (cont->widget), newchild->widget, "tab-expand", TRUE, "tab-fill", TRUE, NULL);
#endif
            gtk_notebook_set_current_page (GTK_NOTEBOOK (cont->widget), pos);
            break;
        }
    }
}

static gboolean
get_event_coordinates_in_widget (GtkWidget *widget, GdkEventButton *event, gint *x, gint *y) {
    GdkWindow *window = event->window;
    gdouble tx, ty;
    tx = event->x;
    ty = event->y;

    while (window && window != gtk_widget_get_window (widget)) {
        gint window_x, window_y;
        gdk_window_get_position (window, &window_x, &window_y);
        tx += window_x;
        ty += window_y;
        window = gdk_window_get_parent (window);
    }

    if (window) {
        *x = tx;
        *y = ty;
        return TRUE;
    }
    else {
        return FALSE;
    }
}

static gboolean
on_tabs_button_press_event (GtkWidget *notebook, GdkEventButton *event, gpointer user_data) {
    w_tabs_t *w = user_data;

    int page_num = 0;
    GtkWidget *page;
    GtkWidget *label_box;
    GtkAllocation alloc;

    int event_x, event_y;
    if (!get_event_coordinates_in_widget (notebook, event, &event_x, &event_y)) {
        // clicked outside the tabstrip (e.g. in one of its children)
        return FALSE;
    }

    /* lookup the clicked tab */
    while ((page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num)) != NULL) {
        label_box = gtk_notebook_get_tab_label (GTK_NOTEBOOK (notebook), page);
        gtk_widget_get_allocation (label_box, &alloc);

        if (event_x >= alloc.x && event_x < alloc.x + alloc.width && event_y >= alloc.y && event_y < alloc.y + alloc.height)
            break;

        page_num++;
    }
    w->clicked_page = page_num;

    if (event->type == GDK_BUTTON_PRESS) {
        /* leave if no tab could be found */
        if (page == NULL) {
            return FALSE;
        }

        if (event->button == 2) {
            /* check if we should close the tab */
            if (w_get_design_mode ()) {
                tabs_remove_tab (w, page_num);
            }
        }
        else if (event->button == 3) {
            if (!w_get_design_mode ()) {
                /* update the current tab before we show the menu */
                gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);

                /* show the tab menu */
                on_tab_popup_menu (notebook, user_data);
                return TRUE;
            }
        }
    }
    else if (event->type == GDK_2BUTTON_PRESS) {
        if (event->button == 1 && page == NULL && w_get_design_mode ()) {
            // open new tab
            tabs_add_tab (w);
        }
        return TRUE;
    }
    return FALSE;
}

static void
w_tabs_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_mnemonic (_ ("Add New Tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_add_tab_activate), w);
}

static void
w_tabs_init (ddb_gtkui_widget_t *base) {
    w_tabs_t *w = (w_tabs_t *)base;
    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->active);
    if (w->titles) {
        int page = 0;
        while (page < w->num_tabs) {
            GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (w->base.widget), page);
            if (w->titles[page]) {
                gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (w->base.widget), child, w->titles[page]);
#if GTK_CHECK_VERSION(3, 0, 0)
                GtkLabel *label = GTK_LABEL (gtk_notebook_get_tab_label (GTK_NOTEBOOK (w->base.widget), child));
                gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
                gtk_misc_set_padding (GTK_MISC (label), 0, 0);
#endif
            }
            free (w->titles[page]);
            page++;
        }
        free (w->titles);
        w->titles = NULL;
    }
}

ddb_gtkui_widget_t *
w_tabs_create (void) {
    w_tabs_t *w = malloc (sizeof (w_tabs_t));
    memset (w, 0, sizeof (w_tabs_t));
    w->base.widget = gtk_notebook_new ();
    w->base.append = w_tabs_add;
    w->base.remove = w_container_remove;
    w->base.replace = w_tabs_replace;
    w->base.initmenu = w_tabs_initmenu;
    w->base.init = w_tabs_init;
    w->base.destroy = w_tabs_destroy;
    w->exapi._size = sizeof (ddb_gtkui_widget_extended_api_t);
    w->exapi.serialize_to_keyvalues = w_tabs_serialize_to_keyvalues;
    w->exapi.deserialize_from_keyvalues = w_tabs_deserialize_from_keyvalues;
    w->exapi.free_serialized_keyvalues = w_tabs_free_serialized_keyvalues;

    ddb_gtkui_widget_t *ph1, *ph2, *ph3;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");
    ph3 = w_create ("placeholder");

    gtk_notebook_set_scrollable (GTK_NOTEBOOK (w->base.widget), TRUE);

#if !GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect ((gpointer)w->base.widget, "expose_event", G_CALLBACK (w_expose_event), w);
#else
    g_signal_connect ((gpointer)w->base.widget, "draw", G_CALLBACK (w_draw_event), w);
#endif
    g_signal_connect ((gpointer)w->base.widget, "button_press_event", G_CALLBACK (on_tabs_button_press_event), w);

    w_append ((ddb_gtkui_widget_t *)w, ph1);
    w_append ((ddb_gtkui_widget_t *)w, ph2);
    w_append ((ddb_gtkui_widget_t *)w, ph3);

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
