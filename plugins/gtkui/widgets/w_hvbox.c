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
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_hvbox.h"

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *box;
    uint64_t expand;
    uint64_t fill;
    unsigned homogeneous : 1;
} w_hvbox_t;

// hbox and vbox
static const char *
w_hvbox_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    if (strcmp (type, "hbox") && strcmp (type, "vbox")) {
        return NULL;
    }
    w_hvbox_t *hvbox = (w_hvbox_t *)w;

    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s, key, val);

        if (!strcmp (key, "expand")) {
            const char *s = val;
            int n = 0;
            hvbox->expand = 0;
            char t[MAX_TOKEN];
            while (n < 64) {
                s = gettoken (s, t);
                if (!s) {
                    break;
                }
                if (atoi (t)) {
                    hvbox->expand |= (1ULL << n);
                }
                n++;
            }
        }
        else if (!strcmp (key, "fill")) {
            const char *s = val;
            int n = 0;
            hvbox->fill = 0;
            char t[MAX_TOKEN];
            while (n < 64) {
                s = gettoken (s, t);
                if (!s) {
                    break;
                }
                if (atoi (t)) {
                    hvbox->fill |= (1ULL << n);
                }
                n++;
            }
        }
        else if (!strcmp (key, "homogeneous")) {
            hvbox->homogeneous = atoi (val) ? 1 : 0;
        }
    }

    return s;
}

typedef struct {
    GtkWidget *hvbox;
    char expand[150];
    char fill[150];
} w_hvbox_save_info_t;

static void
save_hvbox_packing (GtkWidget *child, gpointer user_data) {
    w_hvbox_save_info_t *info = user_data;
    gboolean expand;
    gboolean fill;
    guint padding;
    GtkPackType pack_type;
    gtk_box_query_child_packing (GTK_BOX (info->hvbox), child, &expand, &fill, &padding, &pack_type);
    char s[10];
    snprintf (s, sizeof (s), info->expand[0] ? " %d" : "%d", expand);
    strncat (info->expand, s, sizeof (info->expand) - strlen (info->expand));

    snprintf (s, sizeof (s), info->fill[0] ? " %d" : "%d", fill);
    strncat (info->fill, s, sizeof (info->fill) - strlen (info->fill));
}

static void
w_hvbox_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    char save[1000];

    w_hvbox_save_info_t info;
    memset (&info, 0, sizeof (info));
    info.hvbox = ((w_hvbox_t *)w)->box;
    gtk_container_foreach (GTK_CONTAINER (((w_hvbox_t *)w)->box), save_hvbox_packing, &info);
    gboolean homogeneous = gtk_box_get_homogeneous (GTK_BOX (((w_hvbox_t *)w)->box));

    snprintf (save, sizeof (save), " expand=\"%s\" fill=\"%s\" homogeneous=%d", info.expand, info.fill, homogeneous);
    strncat (s, save, sz);
}

typedef struct {
    w_hvbox_t *w;
    int n;
} hwbox_init_info_t;

static void
hvbox_init_child (GtkWidget *child, void *user_data) {
    hwbox_init_info_t *info = user_data;
    gboolean expand;
    gboolean fill;
    guint padding;
    GtkPackType pack_type;
    gtk_box_query_child_packing (GTK_BOX (info->w->box), child, &expand, &fill, &padding, &pack_type);
    expand = (info->w->expand & (1 << info->n)) ? TRUE : FALSE;
    fill = (info->w->fill & (1 << info->n)) ? TRUE : FALSE;
    gtk_box_set_child_packing (GTK_BOX (info->w->box), child, expand, fill, padding, pack_type);
    info->n++;
}

static void
w_hvbox_init (struct ddb_gtkui_widget_s *w) {
    w_hvbox_t *hvbox = (w_hvbox_t *)w;
    hwbox_init_info_t info;
    info.w = hvbox;
    info.n = 0;
    gtk_container_foreach (GTK_CONTAINER (hvbox->box), hvbox_init_child, &info);
    gtk_box_set_homogeneous (GTK_BOX (hvbox->box), hvbox->homogeneous ? TRUE : FALSE);
}

static void
w_hvbox_append (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child) {
    w_hvbox_t *b = (w_hvbox_t *)container;
    gtk_box_pack_start (GTK_BOX (b->box), child->widget, (child->flags & DDB_GTKUI_WIDGET_FLAG_NON_EXPANDABLE) ? FALSE : TRUE, TRUE, 0);
    gtk_widget_show (child->widget);
}

static void
w_hvbox_remove (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child) {
    w_hvbox_t *b = (w_hvbox_t *)container;
    gtk_container_remove (GTK_CONTAINER (b->box), child->widget);
}

static void
w_hvbox_replace (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child, struct ddb_gtkui_widget_s *newchild) {
    w_hvbox_t *b = (w_hvbox_t *)container;
    ddb_gtkui_widget_t *c;
    ddb_gtkui_widget_t *prev = NULL;
    int n = 0;
    for (c = container->children; c; prev = c, c = c->next, n++) {
        if (c == child) {
            break;
        }
    }

    if (!c) {
        return;
    }

    if (prev) {
        prev->next = newchild;
    }
    else {
        container->children = newchild;
    }
    newchild->next = c->next;
    newchild->parent = container;
    w_remove (container, c);
    w_destroy (c);

    gtk_box_pack_start (GTK_BOX (b->box), newchild->widget, TRUE, TRUE, 0);
    gtk_widget_show (newchild->widget);
    gtk_box_reorder_child (GTK_BOX (b->box), newchild->widget, n);
}

static void
on_hvbox_expand (GtkMenuItem *menuitem, gpointer user_data) {
    w_append ((ddb_gtkui_widget_t *)user_data, w_create ("placeholder"));
    w_save_with_action (_("Expand Box"));
}

static void
on_hvbox_shrink (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *w = (ddb_gtkui_widget_t *)user_data;
    ddb_gtkui_widget_t *c;
    for (c = w->children; c && c->next; c = c->next)
        ;
    if (c) {
        w_remove (w, c);
        w_destroy (c);
    }
    if (!w->children) {
        w_append (w, w_create ("placeholder"));
    }
    w_save_with_action (_("Shrink Box"));
}

static void
on_hvbox_toggle_homogeneous (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_hvbox_t *w = user_data;
    gboolean hmg = gtk_box_get_homogeneous (GTK_BOX (((w_hvbox_t *)w)->box));
    gtk_box_set_homogeneous (GTK_BOX (((w_hvbox_t *)w->box)), hmg ? FALSE : TRUE);
    w_save_with_action (_("Toggle Homogeneous"));
}

static void
w_hvbox_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_mnemonic (_ ("Expand Box by 1 Item"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_hvbox_expand), w);

    item = gtk_menu_item_new_with_mnemonic (_ ("Shrink Box by 1 Item"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "activate", G_CALLBACK (on_hvbox_shrink), w);

    item = gtk_check_menu_item_new_with_mnemonic (_ ("Homogeneous"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), gtk_box_get_homogeneous (GTK_BOX (((w_hvbox_t *)w)->box)));
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (on_hvbox_toggle_homogeneous), w);
}

static void
on_hvbox_child_toggle_expand (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    ddb_gtkui_widget_t *w = user_data;
    w_hvbox_t *box = (w_hvbox_t *)w->parent;
    gboolean expand, fill;
    guint padding;
    GtkPackType packtype;
    gtk_box_query_child_packing (GTK_BOX (box->box), w->widget, &expand, &fill, &padding, &packtype);
    gtk_box_set_child_packing (GTK_BOX (box->box), w->widget, !expand, fill, padding, packtype);
    w_save_with_action (_("Toggle Expand"));
}

static void
on_hvbox_child_toggle_fill (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    ddb_gtkui_widget_t *w = user_data;
    w_hvbox_t *box = (w_hvbox_t *)w->parent;
    gboolean expand, fill;
    guint padding;
    GtkPackType packtype;
    gtk_box_query_child_packing (GTK_BOX (box->box), w->widget, &expand, &fill, &padding, &packtype);
    gtk_box_set_child_packing (GTK_BOX (box->box), w->widget, expand, !fill, padding, packtype);
    w_save_with_action (_("Toggle Fill"));
}

static void
w_hvbox_initchildmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    w_hvbox_t *box = (w_hvbox_t *)w->parent;

    gboolean expand, fill;
    guint padding;
    GtkPackType packtype;
    gtk_box_query_child_packing (GTK_BOX (box->box), w->widget, &expand, &fill, &padding, &packtype);

    GtkWidget *item;

    item = gtk_check_menu_item_new_with_mnemonic (_ ("Expand"));
    gtk_widget_show (item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), expand);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (on_hvbox_child_toggle_expand), w);

    item = gtk_check_menu_item_new_with_mnemonic (_ ("Fill"));
    gtk_widget_show (item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), fill);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (on_hvbox_child_toggle_fill), w);
}

GtkWidget *
w_hvbox_get_container (struct ddb_gtkui_widget_s *b) {
    return ((w_hvbox_t *)b)->box;
}

ddb_gtkui_widget_t *
w_hbox_create (void) {
    w_hvbox_t *w = malloc (sizeof (w_hvbox_t));
    memset (w, 0, sizeof (w_hvbox_t));
    w->base.widget = gtk_event_box_new ();
    w->base.append = w_hvbox_append;
    w->base.remove = w_hvbox_remove;
    w->base.replace = w_hvbox_replace;
    w->base.initmenu = w_hvbox_initmenu;
    w->base.initchildmenu = w_hvbox_initchildmenu;
    w->base.load = w_hvbox_load;
    w->base.save = w_hvbox_save;
    w->base.init = w_hvbox_init;
    w->base.get_container = w_hvbox_get_container;
    w->box = gtk_hbox_new (TRUE, 3);
    w->homogeneous = 1;
    w->expand = -1;
    w->fill = -1;
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);

    w_append ((ddb_gtkui_widget_t *)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t *)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t *)w, w_create ("placeholder"));

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

ddb_gtkui_widget_t *
w_vbox_create (void) {
    w_hvbox_t *w = malloc (sizeof (w_hvbox_t));
    memset (w, 0, sizeof (w_hvbox_t));
    w->base.widget = gtk_event_box_new ();
    w->base.append = w_hvbox_append;
    w->base.remove = w_hvbox_remove;
    w->base.replace = w_hvbox_replace;
    w->base.get_container = w_hvbox_get_container;
    w->base.initmenu = w_hvbox_initmenu;
    w->base.initchildmenu = w_hvbox_initchildmenu;
    w->base.load = w_hvbox_load;
    w->base.save = w_hvbox_save;
    w->base.init = w_hvbox_init;
    w->box = gtk_vbox_new (TRUE, 3);
    w->homogeneous = 1;
    w->expand = -1;
    w->fill = -1;
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);

    w_append ((ddb_gtkui_widget_t *)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t *)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t *)w, w_create ("placeholder"));

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
