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
#include "../../../shared/parser.h"
#include "../ddb_splitter.h"
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_splitter.h"

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *box;
    // size of first child
    int size1;
    // size of second child
    int size2;
    float ratio;
    int locked;
} w_splitter_t;

// common splitter funcs
const char *
w_splitter_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    if (strcmp (type, "vsplitter") && strcmp (type, "hsplitter")) {
        return NULL;
    }

    w_splitter_t *sp = (w_splitter_t *)w;

    int got_ratio = 0;

    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s, key, val);

        if (!strcmp (key, "locked")) {
            sp->locked = atoi (val);
        }
        else if (!strcmp (key, "ratio")) {
            sp->ratio = atof (val);
            if (sp->ratio < 0) {
                sp->ratio = 0;
            }
            if (sp->ratio > 1) {
                sp->ratio = 1;
            }
            got_ratio = 1;
        }
        else if (!strcmp (key, "pos")) {
            sp->size1 = atoi (val);
        }
        else if (!strcmp (key, "size2")) {
            sp->size2 = atoi (val);
        }
    }

    if (!got_ratio) {
        sp->ratio = 0.5;
    }

    return s;
}

void
w_splitter_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    w_splitter_t *sp = (w_splitter_t *)w;
    int locked = ddb_splitter_get_size_mode (DDB_SPLITTER (sp->box));

    float ratio = ddb_splitter_get_proportion (DDB_SPLITTER (sp->box));
    char spos[100];
    // NOTE: we use pos instead of size1 to ensure compatiblity with older deadbeef versions
    snprintf (spos, sizeof (spos), " locked=%d ratio=%f pos=%d size2=%d", locked, ratio, sp->size1, sp->size2);
    strncat (s, spos, sz);
}

void
w_splitter_add (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *child) {
    w_container_add (w, child);
}

GtkWidget *
w_splitter_get_container (struct ddb_gtkui_widget_s *b) {
    return ((w_splitter_t *)b)->box;
}

void
on_splitter_lock_c1_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_splitter_t *sp = (w_splitter_t *)user_data;
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem))) {
        sp->locked = DDB_SPLITTER_SIZE_MODE_LOCK_C1;
        sp->size1 = ddb_splitter_get_child1_size (DDB_SPLITTER (sp->box));
        ddb_splitter_set_size_mode (DDB_SPLITTER (sp->box), DDB_SPLITTER_SIZE_MODE_LOCK_C1);
        w_save_with_action (_("Change Splitter Mode"));
    }
}

void
on_splitter_lock_c2_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_splitter_t *sp = (w_splitter_t *)user_data;
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem))) {
        sp->locked = DDB_SPLITTER_SIZE_MODE_LOCK_C2;
        sp->size2 = ddb_splitter_get_child2_size (DDB_SPLITTER (sp->box));
        ddb_splitter_set_size_mode (DDB_SPLITTER (sp->box), DDB_SPLITTER_SIZE_MODE_LOCK_C2);
        w_save_with_action (_("Change Splitter Mode"));
    }
}

void
on_splitter_lock_prop_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_splitter_t *sp = (w_splitter_t *)user_data;
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem))) {
        sp->locked = DDB_SPLITTER_SIZE_MODE_PROP;
        ddb_splitter_set_size_mode (DDB_SPLITTER (sp->box), DDB_SPLITTER_SIZE_MODE_PROP);
        w_save_with_action (_("Change Splitter Mode"));
    }
}

static void
on_splitter_drag_finished (DdbSplitter *splitter, gpointer user_data) {
    w_save_with_action (_("Move Splitter"));
}

void
w_splitter_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    w_splitter_t *sp = (w_splitter_t *)w;
    GtkOrientation orientation = ddb_splitter_get_orientation (DDB_SPLITTER (sp->box));

    GSList *group = NULL;

    GtkWidget *item = gtk_radio_menu_item_new_with_mnemonic (group, _ ("Proportional Sizing"));
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    if (sp->locked == DDB_SPLITTER_SIZE_MODE_PROP) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (on_splitter_lock_prop_toggled), w);

    if (orientation == GTK_ORIENTATION_VERTICAL) {
        item = gtk_radio_menu_item_new_with_mnemonic (group, _ ("Lock Top Pane Height"));
    }
    else {
        item = gtk_radio_menu_item_new_with_mnemonic (group, _ ("Lock Left Pane Width"));
    }
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    if (sp->locked == DDB_SPLITTER_SIZE_MODE_LOCK_C1) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (on_splitter_lock_c1_toggled), w);

    if (orientation == GTK_ORIENTATION_VERTICAL) {
        item = gtk_radio_menu_item_new_with_mnemonic (group, _ ("Lock Bottom Pane Height"));
    }
    else {
        item = gtk_radio_menu_item_new_with_mnemonic (group, _ ("Lock Right Pane Width"));
    }
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    if (sp->locked == DDB_SPLITTER_SIZE_MODE_LOCK_C2) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (on_splitter_lock_c2_toggled), w);
}

void
w_splitter_replace (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child, ddb_gtkui_widget_t *newchild) {
    int ntab = 0;
    ddb_gtkui_widget_t *prev = NULL;
    for (ddb_gtkui_widget_t *c = cont->children; c; c = c->next, ntab++) {
        if (c == child) {
            newchild->next = c->next;
            if (prev) {
                prev->next = newchild;
            }
            else {
                cont->children = newchild;
            }
            newchild->parent = cont;
            w_remove (cont, child);
            w_destroy (child);
            GtkWidget *container = ((w_splitter_t *)cont)->box;
            gtk_widget_show (newchild->widget);
            if (ntab == 0) {
                ddb_splitter_add_child_at_pos (DDB_SPLITTER (container), newchild->widget, 0);
            }
            else {
                ddb_splitter_add_child_at_pos (DDB_SPLITTER (container), newchild->widget, 1);
            }
            break;
        }
        prev = c;
    }
}

void
w_splitter_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *container = w_get_container (cont);
    gtk_container_remove (GTK_CONTAINER (container), child->widget);
}

void
w_splitter_init (ddb_gtkui_widget_t *base) {
    w_splitter_t *w = (w_splitter_t *)base;

    ddb_splitter_set_proportion (DDB_SPLITTER (w->box), w->ratio);
    ddb_splitter_set_size_mode (DDB_SPLITTER (w->box), w->locked);
    if (w->locked == DDB_SPLITTER_SIZE_MODE_LOCK_C1) {
        ddb_splitter_set_child1_size (DDB_SPLITTER (w->box), w->size1);
    }
    else if (w->locked == DDB_SPLITTER_SIZE_MODE_LOCK_C2) {
        ddb_splitter_set_child2_size (DDB_SPLITTER (w->box), w->size2);
    }
}

////// vsplitter widget
ddb_gtkui_widget_t *
w_vsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->ratio = 0.5f;
    w->locked = DDB_SPLITTER_SIZE_MODE_PROP;
    w->base.append = w_splitter_add;
    w->base.remove = w_splitter_remove;
    w->base.replace = w_splitter_replace;
    w->base.get_container = w_splitter_get_container;
    w->base.load = w_splitter_load;
    w->base.save = w_splitter_save;
    w->base.init = w_splitter_init;
    w->base.initmenu = w_splitter_initmenu;

    w->base.widget = gtk_event_box_new ();
    w->box = ddb_splitter_new (GTK_ORIENTATION_VERTICAL);
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);
    g_signal_connect ((gpointer)w->box, "drag-finished", G_CALLBACK (on_splitter_drag_finished), w);
    w_override_signals (w->base.widget, w);

    ddb_gtkui_widget_t *ph1, *ph2;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");

    w_append ((ddb_gtkui_widget_t *)w, ph1);
    w_append ((ddb_gtkui_widget_t *)w, ph2);

    return (ddb_gtkui_widget_t *)w;
}

////// hsplitter widget
ddb_gtkui_widget_t *
w_hsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->ratio = 0.5f;
    w->locked = DDB_SPLITTER_SIZE_MODE_PROP;
    w->base.append = w_splitter_add;
    w->base.remove = w_splitter_remove;
    w->base.replace = w_splitter_replace;
    w->base.get_container = w_splitter_get_container;
    w->base.load = w_splitter_load;
    w->base.save = w_splitter_save;
    w->base.init = w_splitter_init;
    w->base.initmenu = w_splitter_initmenu;

    w->base.widget = gtk_event_box_new ();
    w->box = ddb_splitter_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);
    g_signal_connect ((gpointer)w->box, "drag-finished", G_CALLBACK (on_splitter_drag_finished), w);
    w_override_signals (w->base.widget, w);

    ddb_gtkui_widget_t *ph1, *ph2;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");

    w_append ((ddb_gtkui_widget_t *)w, ph1);
    w_append ((ddb_gtkui_widget_t *)w, ph2);

    return (ddb_gtkui_widget_t *)w;
}
