/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "gtkui.h"
#include "widgets.h"
#include "ddbtabstrip.h"
#include "ddblistview.h"
#include "mainplaylist.h"
#include "../../gettext.h"
#include "parser.h"
#include "trkproperties.h"
#include "coverart.h"
#include "gtkuigl.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

typedef struct w_creator_s {
    const char *type;
    const char *title; // set to NULL to avoid exposing this widget type to user
    ddb_gtkui_widget_t *(*create_func) (void);
    struct w_creator_s *next;
} w_creator_t;

static w_creator_t *w_creators;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *box; // hack to support splitter locking, can be a vbox or a hbox
    int position;
    int locked;
} w_splitter_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_box_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_tabstrip_t;

typedef struct {
    ddb_gtkui_widget_t base;
    DdbTabStrip *tabstrip;
    DdbListview *list;
} w_tabbed_playlist_t;

typedef struct {
    ddb_gtkui_widget_t base;
    DdbListview *list;
} w_playlist_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
} w_placeholder_t;

typedef struct {
    ddb_gtkui_widget_t base;
    int clicked_page;
} w_tabs_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *tree;
} w_selproperties_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
} w_coverart_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
    guint drawtimer;
    GdkGLContext *glcontext;
} w_scope_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *drawarea;
    guint drawtimer;
    GdkGLContext *glcontext;
} w_spectrum_t;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *box;
} w_hvbox_t;

static int design_mode;
static ddb_gtkui_widget_t *rootwidget;

//// common functions

void
w_init (void) {
    rootwidget = w_create ("box");
}

void
w_free (void) {
    w_save ();
    w_creator_t *next = NULL;
    for (w_creator_t *cr = w_creators; cr; cr = next) {
        next = cr->next;
        free (cr);
    }
    w_creators = NULL;
}

ddb_gtkui_widget_t *
w_get_rootwidget (void) {
    return rootwidget;
}

static void
set_design_mode (ddb_gtkui_widget_t *w) {
    for (ddb_gtkui_widget_t *c = w->children; c; c = c->next) {
        set_design_mode (c);
    }
}

void
w_set_design_mode (int active) {
    design_mode = active;
    set_design_mode (rootwidget);
}

void
w_append (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    child->parent = cont;
    if (!cont->children) {
        cont->children = child;
    }
    else {
        for (ddb_gtkui_widget_t *c = cont->children; c; c = c->next) {
            if (!c->next) {
                c->next = child;
                break;
            }
        }
    }

    if (cont->append) {
        cont->append (cont, child);
    }
    if (child->init) {
        child->init (child);
    }
}

void
w_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    if (cont->remove) {
        cont->remove (cont, child);
    }
    child->widget = NULL;
    ddb_gtkui_widget_t *prev = NULL;
    for (ddb_gtkui_widget_t *c = cont->children; c; c = c->next) {
        if (c == child) {
            if (prev) {
                prev->next = c->next;
            }
            else {
                cont->children = c->next;
            }
            break;
        }
        prev = c;
    }
    child->parent = NULL;
}

gboolean
w_init_cb (void *data) {
    ddb_gtkui_widget_t *w = data;
    if (w->init) {
        w->init (w);
    }
    return FALSE;
}

void
w_replace (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *from, ddb_gtkui_widget_t *to) {
    if (w->replace) {
        w->replace (w, from, to);
        if (to->init) {
            g_idle_add (w_init_cb, to);
        }
    }
    else {
        w_remove (w, from);
        w_destroy (from);
        w_append (w, to);
    }
}

const char *
w_create_from_string (const char *s, ddb_gtkui_widget_t **parent) {
    char t[MAX_TOKEN];
    s = gettoken (s, t);
    if (!s) {
        return NULL;
    }
    ddb_gtkui_widget_t *w = w_create (t);
    // nuke all default children
    while (w->children) {
        w_remove (w, w->children);
    }

    // name
    s = gettoken (s, t);
    if (!s) {
        w_destroy (w);
        return NULL;
    }
    if (t[0]) {
        w_set_name (w, t);
    }

    // load widget params
    if (w->load) {
        s = w->load (w, s);
        if (!s) {
            w_destroy (w);
            return NULL;
        }
    }

    // {
    s = gettoken (s, t);
    if (!s) {
        w_destroy (w);
        return NULL;
    }
    if (strcmp (t, "{")) {
        w_destroy (w);
        return NULL;
    }

    const char *back = s;
    s = gettoken (s, t);
    if (!s) {
        w_destroy (w);
        return NULL;
    }
    for (;;) {
        if (!strcmp (t, "}")) {
            break;
        }

        s = w_create_from_string (back, &w);
        if (!s) {
            w_destroy (w);
            return NULL;
        }

        back = s;
        s = gettoken (s, t);
        if (!s) {
            w_destroy (w);
            return NULL;
        }
    }

    if (*parent) {
        w_append (*parent, w);
    }
    else {
        *parent = w;
    }
    return s;
}

static ddb_gtkui_widget_t *current_widget;
static int hidden = 0;

static gboolean
w_draw_event (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    if (hidden && user_data == current_widget) {
        cairo_set_source_rgb (cr, 0.17f, 0, 0.83f);
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);

        if (!gtk_widget_get_has_window (widget)) {
            cairo_reset_clip (cr);
            cairo_rectangle (cr, allocation.x, allocation.y, allocation.width, allocation.height);
        }
        else {
            cairo_reset_clip (cr);
            cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
        }
        cairo_fill (cr);
    }
    return FALSE;
}

gboolean
w_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = w_draw_event (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

static char paste_buffer[1000];

void
save_widget_to_string (char *str, int sz, ddb_gtkui_widget_t *w) {
    strcat (str, w->type);
    strcat (str, " \"");
    strcat (str, w->name ? w->name : "");
    strcat (str, "\"");
    if (w->save) {
        w->save (w, str, sz);
    }
    strcat (str, " {");
    for (ddb_gtkui_widget_t *c = w->children; c; c = c->next) {
        save_widget_to_string (str, sz, c);
    }
    strcat (str, "} ");
}

void
w_save (void) {
    char buf[4000] = "";
    save_widget_to_string (buf, sizeof (buf), rootwidget->children);
    deadbeef->conf_set_str ("gtkui.layout", buf);
    deadbeef->conf_save ();
}

static void
on_replace_activate (GtkMenuItem *menuitem, gpointer user_data) {
    for (w_creator_t *cr = w_creators; cr; cr = cr->next) {
        if (cr->type == user_data) {
            w_replace (current_widget->parent, current_widget, w_create (user_data));
        }
    }
    w_save ();
}

static void
on_delete_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *parent = current_widget->parent;
    if (!strcmp (current_widget->type, "placeholder")) {
        return;
    }
    if (parent->replace) {
        parent->replace (parent, current_widget, w_create ("placeholder"));
    }
    else {
        w_remove (parent, current_widget);
        w_destroy (current_widget);
        current_widget = w_create ("placeholder");
        w_append (parent, current_widget);
    }
    w_save ();
}

static void
on_cut_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *parent = current_widget->parent;
    if (!strcmp (current_widget->type, "placeholder")) {
        return;
    }
    // save hierarchy to string
    // FIXME: use real clipboard
    paste_buffer[0] = 0;
    save_widget_to_string (paste_buffer, sizeof (paste_buffer), current_widget);

    if (parent->replace) {
        parent->replace (parent, current_widget, w_create ("placeholder"));
    }
    else {
        w_remove (parent, current_widget);
        w_destroy (current_widget);
        current_widget = w_create ("placeholder");
        w_append (parent, current_widget);
    }
    w_save ();
}

static void
on_copy_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *parent = current_widget->parent;
    if (!strcmp (current_widget->type, "placeholder")) {
        return;
    }
    // save hierarchy to string
    // FIXME: use real clipboard
    paste_buffer[0] = 0;
    save_widget_to_string (paste_buffer, sizeof (paste_buffer), current_widget);
}

static void
on_paste_activate (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *parent = current_widget->parent;
    if (!paste_buffer[0]) {
        return;
    }
    ddb_gtkui_widget_t *w = NULL;
    w_create_from_string (paste_buffer, &w);
    if (parent->replace) {
        parent->replace (parent, current_widget, w);
    }
    else {
        w_remove (parent, current_widget);
        w_destroy (current_widget);
        current_widget = w;
        w_append (parent, current_widget);
    }
    w_save ();
}

void
hide_widget (GtkWidget *widget, gpointer data) {
    gtk_widget_hide (widget);
}

void
show_widget (GtkWidget *widget, gpointer data) {
    gtk_widget_show (widget);
}

void
w_menu_deactivate (GtkMenuShell *menushell, gpointer user_data) {
    hidden = 0;
    ddb_gtkui_widget_t *w = user_data;
    if (GTK_IS_CONTAINER (w->widget)) {
        gtk_container_foreach (GTK_CONTAINER (w->widget), show_widget, NULL);
    }
    gtk_widget_set_app_paintable (w->widget, FALSE);
    gtk_widget_queue_draw (w->widget);
}

gboolean
w_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (!design_mode || event->button != 3) {
        return FALSE;
    }

    current_widget = user_data;
    widget = current_widget->widget;
    hidden = 1;
    if (GTK_IS_CONTAINER (widget)) {
        gtk_container_foreach (GTK_CONTAINER (widget), hide_widget, NULL);
    }
    gtk_widget_set_app_paintable (widget, TRUE);
    gtk_widget_queue_draw (((ddb_gtkui_widget_t *)user_data)->widget);
    GtkWidget *menu;
    GtkWidget *submenu;
    GtkWidget *item;
    menu = gtk_menu_new ();
    if (strcmp (current_widget->type, "placeholder")) {
        item = gtk_menu_item_new_with_mnemonic (_("Replace with..."));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
    }
    else {
        item = gtk_menu_item_new_with_mnemonic (_("Insert..."));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
    }

    submenu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

    for (w_creator_t *cr = w_creators; cr; cr = cr->next) {
        if (cr->title) {
            item = gtk_menu_item_new_with_mnemonic (cr->title);
            gtk_widget_show (item);
            gtk_container_add (GTK_CONTAINER (submenu), item);
            g_signal_connect ((gpointer) item, "activate",
                    G_CALLBACK (on_replace_activate),
                    (void *)cr->type);
        }
    }

    if (strcmp (current_widget->type, "placeholder")) {
        item = gtk_menu_item_new_with_mnemonic (_("Delete"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_delete_activate),
                NULL);
        
        item = gtk_menu_item_new_with_mnemonic (_("Cut"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_cut_activate),
                NULL);

        item = gtk_menu_item_new_with_mnemonic (_("Copy"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_copy_activate),
                NULL);
    }
    item = gtk_menu_item_new_with_mnemonic ("Paste");
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_paste_activate),
            NULL);

    if (current_widget->initmenu) {
        current_widget->initmenu (current_widget, menu);
    }

    g_signal_connect ((gpointer) menu, "deactivate", G_CALLBACK (w_menu_deactivate), user_data);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, widget, 0, gtk_get_current_event_time());
    return TRUE;
}

static void
w_override_signals (GtkWidget *widget, gpointer user_data) {
    g_signal_connect ((gpointer) widget, "button_press_event", G_CALLBACK (w_button_press_event), user_data);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect ((gpointer) widget, "expose_event", G_CALLBACK (w_expose_event), user_data);
#else
    g_signal_connect ((gpointer) widget, "draw", G_CALLBACK (w_draw_event), user_data);
#endif
    if (GTK_IS_CONTAINER (widget)) {
        gtk_container_forall (GTK_CONTAINER (widget), w_override_signals, user_data);
    }
}

void
w_reg_widget (const char *type, const char *title, ddb_gtkui_widget_t *(*create_func) (void)) {
    w_creator_t *c;
    for (c = w_creators; c; c = c->next) {
        if (!strcmp (c->type, type)) {
            fprintf (stderr, "gtkui w_reg_widget: widget type %s already registered\n", type);
            return;
        }
    }
    c = malloc (sizeof (w_creator_t));
    memset (c, 0, sizeof (w_creator_t));
    c->type = type;
    c->title = title;
    c->create_func = create_func;
    c->next = w_creators;
    w_creators = c;
}

void
w_unreg_widget (const char *type) {
    w_creator_t *c, *prev = NULL;
    for (c = w_creators; c; c = c->next) {
        if (!strcmp (c->type, type)) {
            if (prev) {
                prev->next = c->next;
            }
            else {
                w_creators = c->next;
            }
            free (c);
            return;
        }
        prev = c;
    }
    fprintf (stderr, "gtkui w_unreg_widget: widget type %s is not registered\n", type);
}

int
w_is_registered (const char *type) {
    // FIXME
    return 0;
}

ddb_gtkui_widget_t *
w_create (const char *type) {
    for (w_creator_t *c = w_creators; c; c = c->next) {
        if (!strcmp (c->type, type)) {
            ddb_gtkui_widget_t *w = c->create_func ();
            w->type = c->type;

            return w;
        }
    }
    return NULL;
}

void
w_set_name (ddb_gtkui_widget_t *w, const char *name) {
    if (w->name) {
        free (w->name);
        w->name = NULL;
    }
    if (name) {
        w->name = strdup (name);
    }
}

void
w_destroy (ddb_gtkui_widget_t *w) {
    if (w->destroy) {
        w->destroy (w);
    }
    if (w->widget) {
        gtk_widget_destroy (w->widget);
    }
    if (w->name) {
        free (w->name);
    }
    free (w);
}

///// gtk_container convenience functions
void
w_container_add (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *container = NULL;
    container = cont->widget;
    gtk_container_add (GTK_CONTAINER (container), child->widget);
    gtk_widget_show (child->widget);
}

void
w_container_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *container = NULL;
    container = cont->widget;
    gtk_container_remove (GTK_CONTAINER (container), child->widget);

}

////// placeholder widget
gboolean
w_placeholder_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    cairo_set_source_rgb (cr, 255, 0, 0);
    cairo_surface_t *checker;
    cairo_t *cr2;

    checker = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 12, 12);
    cr2 = cairo_create (checker);

    cairo_set_source_rgb (cr2, 0.5, 0.5 ,0.5);
    cairo_paint (cr2);
    cairo_set_source_rgb (cr2, 0, 0, 0);
    cairo_move_to (cr2, 0, 0);
    cairo_line_to (cr2, 12, 12);
    cairo_move_to (cr2, 1, 12);
    cairo_line_to (cr2, 12, 1);
    cairo_set_line_width (cr2, 1);
    cairo_set_antialias (cr2, CAIRO_ANTIALIAS_NONE);
    cairo_stroke (cr2);
    cairo_fill (cr2);
    cairo_destroy (cr2);

    cairo_set_source_surface (cr, checker, 0, 0);
    cairo_pattern_t *pt = cairo_get_source(cr);
    cairo_pattern_set_extend (pt, CAIRO_EXTEND_REPEAT);
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    cairo_rectangle (cr, 0, 0, a.width, a.height);
    cairo_paint (cr);
    cairo_surface_destroy (checker);
    return FALSE;
}

gboolean
w_placeholder_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = w_placeholder_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

ddb_gtkui_widget_t *
w_placeholder_create (void) {
    w_placeholder_t *w = malloc (sizeof (w_placeholder_t));
    memset (w, 0, sizeof (w_placeholder_t));

    w->base.widget = gtk_event_box_new ();
    w->drawarea  = gtk_drawing_area_new ();
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);

#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (w_placeholder_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (w_placeholder_draw), w);
#endif
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}

// common splitter funcs
const char *
w_splitter_load (struct ddb_gtkui_widget_s *w, const char *s) {
    char t[MAX_TOKEN];
    s = gettoken (s, t);
    if (!s) {
        return NULL;
    }
    ((w_splitter_t *)w)->position = atoi (t);
    s = gettoken (s, t);
    if (!s) {
        return NULL;
    }
    ((w_splitter_t *)w)->locked = atoi (t);
    return s;
}

void
w_splitter_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    int pos = ((w_splitter_t *)w)->box ? ((w_splitter_t *)w)->position : gtk_paned_get_position (GTK_PANED(w->widget));
    char spos[10];
    snprintf (spos, sizeof (spos), " %d %d", pos, ((w_splitter_t *)w)->locked);
    strncat (s, spos, sz);
}

void
w_splitter_add (ddb_gtkui_widget_t *w, ddb_gtkui_widget_t *child) {
    w_container_add (w, child);
    if (((w_splitter_t *)w)->locked) {
        if (child == w->children) {
            if (GTK_IS_VBOX (((w_splitter_t *)w)->box)) {
                gtk_widget_set_size_request (child->widget, -1, ((w_splitter_t *)w)->position);
            }
            else {
                gtk_widget_set_size_request (child->widget, ((w_splitter_t *)w)->position, -1);
            }
        }
    }
    else {
        gtk_paned_set_position (GTK_PANED(w->widget), ((w_splitter_t *)w)->position);
    }
}

void
w_splitter_init_signals (w_splitter_t *w) {
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect ((gpointer) w->base.widget, "expose_event", G_CALLBACK (w_expose_event), w);
#else
    g_signal_connect ((gpointer) w->base.widget, "draw", G_CALLBACK (w_draw_event), w);
#endif
    g_signal_connect ((gpointer) w->base.widget, "button_press_event", G_CALLBACK (w_button_press_event), w);
}

void
w_splitter_lock (w_splitter_t *w) {
    // we can't change GtkPaned behavior, so convert to vbox for now
    if (w->locked) {
        return;
    }
    w->locked = 1;

    int vert = w->base.type == "vsplitter";

    GtkWidget *box = vert ? gtk_vbox_new (FALSE, 6) : gtk_hbox_new (FALSE, 6);
    gtk_widget_show (box);

    w->position = gtk_paned_get_position (GTK_PANED (w->base.widget));

    GtkAllocation a;
    gtk_widget_get_allocation (w->base.widget, &a);

    GtkWidget *c1 = gtk_paned_get_child1 (GTK_PANED (w->base.widget));
    g_object_ref (c1);
    GtkWidget *c2 = gtk_paned_get_child2 (GTK_PANED (w->base.widget));
    g_object_ref (c2);
    gtk_container_remove (GTK_CONTAINER (w->base.widget), c1);
    gtk_container_remove (GTK_CONTAINER (w->base.widget), c2);

    gtk_box_pack_start (GTK_BOX (box), c1, FALSE, FALSE, 0);
    gtk_widget_set_size_request (c1, vert ? -1 : w->position, vert ? w->position : -1);
    gtk_box_pack_end (GTK_BOX (box), c2, TRUE, TRUE, 0);

    ddb_gtkui_widget_t *parent = w->base.parent;
    if (w->base.parent) {
        gtk_container_remove (GTK_CONTAINER (parent->widget), w->base.widget);
    }
    GtkWidget *eventbox = gtk_event_box_new ();
    gtk_widget_show (eventbox);
    gtk_container_add (GTK_CONTAINER (eventbox), box);
    w->base.widget = eventbox;
    w->box = box;
    if (w->base.parent) {
        gtk_container_add (GTK_CONTAINER (parent->widget), w->base.widget);
    }
    w_splitter_init_signals (w);
}

void
w_splitter_unlock (w_splitter_t *w) {
    if (!w->locked) {
        return;
    }
    w->locked = 0;

    int vert = w->base.type == "vsplitter";
    // convert back to vpaned
    GtkWidget *paned = vert ? gtk_vpaned_new () : gtk_hpaned_new ();
    gtk_widget_set_can_focus (paned, FALSE);
    gtk_widget_show (paned);

    GList *lst = gtk_container_get_children (GTK_CONTAINER (w->box));
    
    GtkWidget *c1 = lst->data;
    g_object_ref (c1);
    GtkWidget *c2 = lst->next->data;
    g_object_ref (c2);
    gtk_container_remove (GTK_CONTAINER (w->box), c1);
    gtk_container_remove (GTK_CONTAINER (w->box), c2);

    gtk_container_add (GTK_CONTAINER (paned), c1);
    gtk_container_add (GTK_CONTAINER (paned), c2);
    gtk_paned_set_position (GTK_PANED (paned), w->position);

    ddb_gtkui_widget_t *parent = w->base.parent;
    if (w->base.parent) {
        gtk_container_remove (GTK_CONTAINER (parent->widget), w->base.widget);
    }
    w->base.widget = paned;
    w->box = NULL;
    if (w->base.parent) {
        gtk_container_add (GTK_CONTAINER (parent->widget), w->base.widget);
    }
    w_splitter_init_signals (w);
}

void
on_splitter_lock_movement_toggled (GtkCheckMenuItem *checkmenuitem, gpointer          user_data) {
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem))) {
        w_splitter_lock (user_data);
    }
    else {
        w_splitter_unlock (user_data);
    }
}

void
w_splitter_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_check_menu_item_new_with_mnemonic (_("Lock movement"));
    gtk_widget_show (item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), ((w_splitter_t *)w)->locked);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_splitter_lock_movement_toggled),
            w);
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
            GtkWidget *container = ((w_splitter_t *)cont)->locked ? ((w_splitter_t *)cont)->box : cont->widget;
            gtk_container_remove (GTK_CONTAINER(container), c->widget);
            c->widget = NULL;
            w_destroy (c);
            gtk_widget_show (newchild->widget);
            if (((w_splitter_t *)cont)->locked) {
                if (ntab == 0) {
                    gtk_box_pack_start (GTK_BOX (container), newchild->widget, TRUE, TRUE, 0);
                }
                else {
                    gtk_box_pack_end (GTK_BOX (container), newchild->widget, TRUE, TRUE, 0);
                }
            }
            else {
                if (ntab == 0) {
                    gtk_paned_add1 (GTK_PANED (container), newchild->widget);
                }
                else {
                    gtk_paned_add2 (GTK_PANED (container), newchild->widget);
                }
            }
            break;
        }
        prev = c;
    }
}

void
w_splitter_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *container = NULL;
    w_splitter_t *w = (w_splitter_t *)cont;
    if (w->locked) {
        container = w->box;
    }
    else {
        container = cont->widget;
    }
    gtk_container_remove (GTK_CONTAINER (container), child->widget);
}

////// vsplitter widget
void
w_vsplitter_init (ddb_gtkui_widget_t *base) {
    w_splitter_t *w = (w_splitter_t *)base;
    int pos = ((w_splitter_t *)w)->position; // prevent lock/unlock from overwriting position
    if (w->locked && !w->box) {
        w->locked = 0;
        w_splitter_lock (w);
    }
    else if (!w->locked && w->box) {
        w->locked = 1;
        w_splitter_unlock (w);
    }
    if (pos == -1) {
        GtkAllocation a;
        gtk_widget_get_allocation (w->base.widget, &a);
        pos = a.height/2;
    }
    w->position = pos;
    if (!w->box) {
        gtk_widget_set_size_request (w->base.children->widget, -1, -1);
        gtk_paned_set_position (GTK_PANED(w->base.widget), pos);
    }
    else {
        gtk_widget_set_size_request (w->base.children->widget, -1, w->position);
    }
}

ddb_gtkui_widget_t *
w_vsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->position = -1;
    w->base.widget = gtk_vpaned_new ();
    gtk_widget_set_can_focus (w->base.widget, FALSE);
    w->base.append = w_splitter_add;
    w->base.remove = w_splitter_remove;
    w->base.replace = w_splitter_replace;
    w->base.load = w_splitter_load;
    w->base.save = w_splitter_save;
    w->base.init = w_vsplitter_init;
    w->base.initmenu = w_splitter_initmenu;

    ddb_gtkui_widget_t *ph1, *ph2;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");

    w_append ((ddb_gtkui_widget_t*)w, ph1);
    w_append ((ddb_gtkui_widget_t*)w, ph2);

    w_splitter_init_signals (w);

    return (ddb_gtkui_widget_t*)w;
}

////// hsplitter widget
void
w_hsplitter_init (ddb_gtkui_widget_t *base) {
    w_splitter_t *w = (w_splitter_t *)base;
    int pos = ((w_splitter_t *)w)->position; // prevent lock/unlock from overwriting position
    if (w->locked && !w->box) {
        w->locked = 0;
        w_splitter_lock (w);
    }
    else if (!w->locked && w->box) {
        w->locked = 1;
        w_splitter_unlock (w);
    }
    if (pos == -1) {
        GtkAllocation a;
        gtk_widget_get_allocation (w->base.widget, &a);
        pos = a.width/2;
    }
    w->position = pos;
    if (!w->box) {
        gtk_widget_set_size_request (w->base.children->widget, -1, -1);
        gtk_paned_set_position (GTK_PANED(w->base.widget), pos);
    }
    else {
        gtk_widget_set_size_request (w->base.children->widget, w->position, -1);
    }
}

ddb_gtkui_widget_t *
w_hsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->position = -1;
    w->base.widget = gtk_hpaned_new ();
    gtk_widget_set_can_focus (w->base.widget, FALSE);
    w->base.append = w_splitter_add;
    w->base.remove = w_splitter_remove;
    w->base.replace = w_splitter_replace;
    w->base.load = w_splitter_load;
    w->base.save = w_splitter_save;
    w->base.init = w_hsplitter_init;
    w->base.initmenu = w_splitter_initmenu;

    ddb_gtkui_widget_t *ph1, *ph2;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");

    w_append ((ddb_gtkui_widget_t*)w, ph1);
    w_append ((ddb_gtkui_widget_t*)w, ph2);

    w_splitter_init_signals (w);

    return (ddb_gtkui_widget_t*)w;
}

///// tabs widget
static gboolean
tab_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data);

static void
on_remove_tab_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;

    int i = 0;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == w->clicked_page) {
            w_remove ((ddb_gtkui_widget_t *)w, c);
            return;
        }
    }
}

static void
on_add_tab_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;

    ddb_gtkui_widget_t *ph;
    ph = w_create ("placeholder");
    w_append ((ddb_gtkui_widget_t*)w, ph);

    int i = 0;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++);
    w->clicked_page = i-1;
    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->clicked_page);

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
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == w->clicked_page) {
            char buf[4000] = "";
            save_widget_to_string (buf, sizeof (buf), c);
            w_create_from_string (buf, &newchild);

            w_remove ((ddb_gtkui_widget_t *)w, c);
            break;
        }
    }
    if (!newchild) {
        return;
    }

    // add new child at new position
    i = 0;
    prev = NULL;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++) {
        if (i == w->clicked_page-1) {
            if (prev) {
                newchild->next = prev->next;
                prev->next = newchild;
            }
            else {
                newchild->next = w->base.children;
                w->base.children = newchild;
            }
            GtkWidget *eventbox = gtk_event_box_new ();
            GtkWidget *label = gtk_label_new (newchild->type);
            gtk_widget_show (eventbox);
            g_object_set_data (G_OBJECT (eventbox), "owner", w);
            g_signal_connect ((gpointer) eventbox, "button_press_event", G_CALLBACK (tab_button_press_event), newchild->widget);
            gtk_widget_show (label);
            gtk_container_add (GTK_CONTAINER (eventbox), label);
            gtk_widget_show (newchild->widget);

            gtk_notebook_insert_page (GTK_NOTEBOOK (w->base.widget), newchild->widget, eventbox, w->clicked_page-1);
            gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->clicked_page-1);
            w->clicked_page--;
            break;
        }
        prev = c;
    }
}

static void
on_move_tab_right_activate (GtkMenuItem *menuitem, gpointer user_data) {
    w_tabs_t *w = user_data;

    int i = 0;
    for (ddb_gtkui_widget_t *c = w->base.children; c; c = c->next, i++);
    if (w->clicked_page >= i)
        return;

    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), ++w->clicked_page);
    on_move_tab_left_activate (menuitem, user_data);
    gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), ++w->clicked_page);
}

static gboolean
tab_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->button != 3) {
        return FALSE;
    }
    // user_data is child widget
    if (design_mode) {
        w_tabs_t *w = (w_tabs_t *)g_object_get_data (G_OBJECT (widget), "owner");
        GtkWidget *menu;
        GtkWidget *item;
        menu = gtk_menu_new ();
 
        item = gtk_menu_item_new_with_mnemonic (_("Move tab left"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_move_tab_left_activate),
                w);

        item = gtk_menu_item_new_with_mnemonic (_("Move tab right"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_move_tab_right_activate),
                w);

        item = gtk_menu_item_new_with_mnemonic (_("Remove tab"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_remove_tab_activate),
                w);

        item = gtk_menu_item_new_with_mnemonic (_("Rename tab"));
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (menu), item);

        w->clicked_page = gtk_notebook_page_num (GTK_NOTEBOOK (w->base.widget), user_data);
        gtk_notebook_set_current_page (GTK_NOTEBOOK (w->base.widget), w->clicked_page);

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, widget, 0, gtk_get_current_event_time());
        return TRUE;
    }
    return FALSE;
}

void
w_tabs_add (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *eventbox = gtk_event_box_new ();
    GtkWidget *label = gtk_label_new (child->type);
    gtk_widget_show (eventbox);
    g_object_set_data (G_OBJECT (eventbox), "owner", cont);
    g_signal_connect ((gpointer) eventbox, "button_press_event", G_CALLBACK (tab_button_press_event), child->widget);
    gtk_widget_show (label);
    gtk_container_add (GTK_CONTAINER (eventbox), label);
    gtk_widget_show (child->widget);
    gtk_notebook_append_page (GTK_NOTEBOOK (cont->widget), child->widget, eventbox);
}

void
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
            gtk_notebook_remove_page (GTK_NOTEBOOK(cont->widget), ntab);
            c->widget = NULL;
            w_destroy (c);
            GtkWidget *eventbox = gtk_event_box_new ();
            GtkWidget *label = gtk_label_new (newchild->type);
            gtk_widget_show (eventbox);
            g_object_set_data (G_OBJECT (eventbox), "owner", cont);
            g_signal_connect ((gpointer) eventbox, "button_press_event", G_CALLBACK (tab_button_press_event), newchild->widget);
            gtk_widget_show (label);
            gtk_container_add (GTK_CONTAINER (eventbox), label);
            gtk_widget_show (newchild->widget);
            int pos = gtk_notebook_insert_page (GTK_NOTEBOOK (cont->widget), newchild->widget, eventbox, ntab);
            gtk_notebook_set_current_page (GTK_NOTEBOOK (cont->widget), pos);
            break;
        }
    }
}

void
w_tabs_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_mnemonic (_("Add new tab"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate",
            G_CALLBACK (on_add_tab_activate),
            w);
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

    ddb_gtkui_widget_t *ph1, *ph2, *ph3;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");
    ph3 = w_create ("placeholder");

#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect ((gpointer) w->base.widget, "expose_event", G_CALLBACK (w_expose_event), w);
#else
    g_signal_connect ((gpointer) w->base.widget, "draw", G_CALLBACK (w_draw_event), w);
#endif
    g_signal_connect ((gpointer) w->base.widget, "button_press_event", G_CALLBACK (w_button_press_event), w);

    w_append ((ddb_gtkui_widget_t*)w, ph1);
    w_append ((ddb_gtkui_widget_t*)w, ph2);
    w_append ((ddb_gtkui_widget_t*)w, ph3);

    return (ddb_gtkui_widget_t*)w;
}

//// box widget
//// this widget should not be exposed to user, it is used as a top level
//// container (rootwidget)

ddb_gtkui_widget_t *
w_box_create (void) {
    w_box_t *w = malloc (sizeof (w_box_t));
    memset (w, 0, sizeof (w_box_t));
    w->base.widget = gtk_vbox_new (FALSE, 0);
    w->base.append = w_container_add; 
    w->base.remove = w_container_remove;

    return (ddb_gtkui_widget_t*)w;
}

//// tabstrip widget

ddb_gtkui_widget_t *
w_tabstrip_create (void) {
    w_tabstrip_t *w = malloc (sizeof (w_tabstrip_t));
    memset (w, 0, sizeof (w_tabstrip_t));
    w->base.widget = gtk_event_box_new ();
    GtkWidget *ts = ddb_tabstrip_new ();
    gtk_widget_show (ts);
    gtk_container_add (GTK_CONTAINER (w->base.widget), ts);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}

//// tabbed playlist widget

typedef struct {
    ddb_gtkui_widget_t *w;
    DB_playItem_t *trk;
} w_trackdata_t;

static gboolean
tabbed_trackinfochanged_cb (gpointer p) {
    w_trackdata_t *d = p;
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)d->w;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        int idx = deadbeef->plt_get_item_idx (plt, (DB_playItem_t *)d->trk, PL_MAIN);
        if (idx != -1) {
            ddb_listview_draw_row (tp->list, idx, (DdbListviewIter)d->trk);
        }
        deadbeef->plt_unref (plt);
    }
    if (d->trk) {
        deadbeef->pl_item_unref (d->trk);
    }
    free (d);
    return FALSE;
}

static gboolean
trackinfochanged_cb (gpointer data) {
    w_trackdata_t *d = data;
    w_playlist_t *p = (w_playlist_t *)d->w;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        int idx = deadbeef->plt_get_item_idx (plt, (DB_playItem_t *)d->trk, PL_MAIN);
        if (idx != -1) {
            ddb_listview_draw_row (DDB_LISTVIEW (p->list), idx, (DdbListviewIter)d->trk);
        }
        deadbeef->plt_unref (plt);
    }
    if (d->trk) {
        deadbeef->pl_item_unref (d->trk);
    }
    free (d);
    return FALSE;
}

static gboolean
tabbed_paused_cb (gpointer p) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)p;
    DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
    if (curr) {
        int idx = deadbeef->pl_get_idx_of (curr);
        ddb_listview_draw_row (tp->list, idx, (DdbListviewIter)curr);
        deadbeef->pl_item_unref (curr);
    }
    return FALSE;
}

static gboolean
paused_cb (gpointer data) {
    w_playlist_t *p = (w_playlist_t *)data;
    DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
    if (curr) {
        int idx = deadbeef->pl_get_idx_of (curr);
        ddb_listview_draw_row (p->list, idx, (DdbListviewIter)curr);
        deadbeef->pl_item_unref (curr);
    }
    return FALSE;
}

static gboolean
tabbed_refresh_cb (gpointer p) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)p;
    ddb_listview_clear_sort (tp->list);
    ddb_listview_refresh (tp->list, DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
    return FALSE;
}

static gboolean
refresh_cb (gpointer data) {
    DdbListview *p = DDB_LISTVIEW (data);
    ddb_listview_clear_sort (p);
    ddb_listview_refresh (DDB_LISTVIEW (p), DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
    return FALSE;
}

static gboolean
tabbed_playlistswitch_cb (gpointer p) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)p;
    int curr = deadbeef->plt_get_curr_idx ();
    char conf[100];
    snprintf (conf, sizeof (conf), "playlist.scroll.%d", curr);
    int scroll = deadbeef->conf_get_int (conf, 0);
    snprintf (conf, sizeof (conf), "playlist.cursor.%d", curr);
    int cursor = deadbeef->conf_get_int (conf, -1);
    ddb_tabstrip_refresh (tp->tabstrip);
    deadbeef->pl_set_cursor (PL_MAIN, cursor);
    if (cursor != -1) {
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
        if (it) {
            deadbeef->pl_set_selected (it, 1);
            deadbeef->pl_item_unref (it);
        }
    }

    ddb_listview_refresh (tp->list, DDB_LIST_CHANGED | DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
    ddb_listview_set_vscroll (tp->list, scroll);
    return FALSE;
}

static gboolean
playlistswitch_cb (gpointer data) {
    w_playlist_t *p = (w_playlist_t *)data;
    int curr = deadbeef->plt_get_curr_idx ();
    char conf[100];
    snprintf (conf, sizeof (conf), "playlist.scroll.%d", curr);
    int scroll = deadbeef->conf_get_int (conf, 0);
    snprintf (conf, sizeof (conf), "playlist.cursor.%d", curr);
    int cursor = deadbeef->conf_get_int (conf, -1);
    deadbeef->pl_set_cursor (PL_MAIN, cursor);
    if (cursor != -1) {
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
        if (it) {
            deadbeef->pl_set_selected (it, 1);
            deadbeef->pl_item_unref (it);
        }
    }

    ddb_listview_refresh (DDB_LISTVIEW (p->list), DDB_LIST_CHANGED | DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
    ddb_listview_set_vscroll (DDB_LISTVIEW (p->list), scroll);
    return FALSE;
}

struct fromto_t {
    ddb_gtkui_widget_t *w;
    DB_playItem_t *from;
    DB_playItem_t *to;
};

static gboolean
tabbed_songchanged_cb (gpointer p) {
    struct fromto_t *ft = p;
    DB_playItem_t *from = ft->from;
    DB_playItem_t *to = ft->to;
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)ft->w;
    int to_idx = -1;
    if (!ddb_listview_is_scrolling (tp->list) && to) {
        int cursor_follows_playback = deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0);
        int scroll_follows_playback = deadbeef->conf_get_int ("playlist.scroll.followplayback", 0);
        int plt = deadbeef->streamer_get_current_playlist ();
        if (plt != -1) {
            if (cursor_follows_playback && plt != deadbeef->plt_get_curr_idx ()) {
                deadbeef->plt_set_curr_idx (plt);
            }
            to_idx = deadbeef->pl_get_idx_of (to);
            if (to_idx != -1) {
                if (cursor_follows_playback) {
                    ddb_listview_set_cursor_noscroll (tp->list, to_idx);
                }
                if (scroll_follows_playback && plt == deadbeef->plt_get_curr_idx ()) {
                    ddb_listview_scroll_to (tp->list, to_idx);
                }
            }
        }
    }

    if (from) {
        int idx = deadbeef->pl_get_idx_of (from);
        if (idx != -1) {
            ddb_listview_draw_row (tp->list, idx, from);
        }
    }
    if (to && to_idx != -1) {
        ddb_listview_draw_row (tp->list, to_idx, to);
    }
    if (ft->from) {
        deadbeef->pl_item_unref (ft->from);
    }
    if (ft->to) {
        deadbeef->pl_item_unref (ft->to);
    }
    free (ft);
    return FALSE;
}

static gboolean
songchanged_cb (gpointer data) {
    struct fromto_t *ft = data;
    DB_playItem_t *from = ft->from;
    DB_playItem_t *to = ft->to;
    w_playlist_t *p = (w_playlist_t *)ft->w;
    int to_idx = -1;
    if (!ddb_listview_is_scrolling (DDB_LISTVIEW (p->list)) && to) {
        int cursor_follows_playback = deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0);
        int scroll_follows_playback = deadbeef->conf_get_int ("playlist.scroll.followplayback", 0);
        int plt = deadbeef->streamer_get_current_playlist ();
        if (plt != -1) {
            if (cursor_follows_playback && plt != deadbeef->plt_get_curr_idx ()) {
                deadbeef->plt_set_curr_idx (plt);
            }
            to_idx = deadbeef->pl_get_idx_of (to);
            if (to_idx != -1) {
                if (cursor_follows_playback) {
                    ddb_listview_set_cursor_noscroll (DDB_LISTVIEW (p->list), to_idx);
                }
                if (scroll_follows_playback && plt == deadbeef->plt_get_curr_idx ()) {
                    ddb_listview_scroll_to (DDB_LISTVIEW (p->list), to_idx);
                }
            }
        }
    }

    if (from) {
        int idx = deadbeef->pl_get_idx_of (from);
        if (idx != -1) {
            ddb_listview_draw_row (DDB_LISTVIEW (p->list), idx, from);
        }
    }
    if (to && to_idx != -1) {
        ddb_listview_draw_row (DDB_LISTVIEW (p->list), to_idx, to);
    }
    if (ft->from) {
        deadbeef->pl_item_unref (ft->from);
    }
    if (ft->to) {
        deadbeef->pl_item_unref (ft->to);
    }
    free (ft);
    return FALSE;
}

static gboolean
tabbed_trackfocus_cb (gpointer p) {
    w_tabbed_playlist_t *tp = p;
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        int idx = deadbeef->pl_get_idx_of (it);
        if (idx != -1) {
            ddb_listview_scroll_to (tp->list, idx);
            ddb_listview_set_cursor (tp->list, idx);
        }
        deadbeef->pl_item_unref (it);
    }

    return FALSE;
}

static gboolean
trackfocus_cb (gpointer p) {
    w_playlist_t *tp = p;
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        int idx = deadbeef->pl_get_idx_of (it);
        if (idx != -1) {
            ddb_listview_scroll_to (tp->list, idx);
            ddb_listview_set_cursor (tp->list, idx);
        }
        deadbeef->pl_item_unref (it);
    }

    return FALSE;
}

static int
w_tabbed_playlist_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)w;
    switch (id) {
    case DB_EV_SONGCHANGED:
        g_idle_add (redraw_queued_tracks_cb, tp->list);
        ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
        struct fromto_t *ft = malloc (sizeof (struct fromto_t));
        ft->from = ev->from;
        ft->to = ev->to;
        if (ft->from) {
            deadbeef->pl_item_ref (ft->from);
        }
        if (ft->to) {
            deadbeef->pl_item_ref (ft->to);
        }
        ft->w = w;
        g_idle_add (tabbed_songchanged_cb, ft);
        break;
    case DB_EV_TRACKINFOCHANGED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                deadbeef->pl_item_ref (ev->track);
            }
            w_trackdata_t *d = malloc (sizeof (w_trackdata_t));
            memset (d, 0, sizeof (w_trackdata_t));
            d->w = w;
            d->trk = ev->track;
            g_idle_add (tabbed_trackinfochanged_cb, d);
        }
        break;
    case DB_EV_PAUSED:
        g_idle_add (tabbed_paused_cb, w);
        break;
    case DB_EV_PLAYLISTCHANGED:
        g_idle_add (tabbed_refresh_cb, w);
        break;
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (tabbed_playlistswitch_cb, w);
        break;
    case DB_EV_TRACKFOCUSCURRENT:
        g_idle_add (tabbed_trackfocus_cb, w);
        break;
    case DB_EV_SELCHANGED:
        if (p2 == PL_SEARCH && p1 == deadbeef->plt_get_curr_idx ()) {
            g_idle_add (refresh_cb, tp->list);
        }
        break;
    }
    return 0;
}

static int
w_playlist_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_playlist_t *p = (w_playlist_t *)w;
    switch (id) {
    case DB_EV_SONGCHANGED:
        g_idle_add (redraw_queued_tracks_cb, p->list);
        ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
        struct fromto_t *ft = malloc (sizeof (struct fromto_t));
        ft->from = ev->from;
        ft->to = ev->to;
        if (ft->from) {
            deadbeef->pl_item_ref (ft->from);
        }
        if (ft->to) {
            deadbeef->pl_item_ref (ft->to);
        }
        ft->w = w;
        g_idle_add (songchanged_cb, ft);
        break;
    case DB_EV_TRACKINFOCHANGED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                deadbeef->pl_item_ref (ev->track);
            }
            w_trackdata_t *d = malloc (sizeof (w_trackdata_t));
            memset (d, 0, sizeof (w_trackdata_t));
            d->w = w;
            d->trk = ev->track;
            g_idle_add (trackinfochanged_cb, d);
        }
        break;
    case DB_EV_PAUSED:
        g_idle_add (paused_cb, w);
        break;
    case DB_EV_PLAYLISTCHANGED:
        g_idle_add (refresh_cb, p->list);
        break;
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (playlistswitch_cb, w);
        break;
    case DB_EV_TRACKFOCUSCURRENT:
        g_idle_add (trackfocus_cb, w);
        break;
    case DB_EV_SELCHANGED:
        if (p2 == PL_SEARCH && p1 == deadbeef->plt_get_curr_idx ()) {
            g_idle_add (refresh_cb, p->list);
        }
        break;
    }
    return 0;
}
ddb_gtkui_widget_t *
w_tabbed_playlist_create (void) {
    w_tabbed_playlist_t *w = malloc (sizeof (w_tabbed_playlist_t));
    memset (w, 0, sizeof (w_tabbed_playlist_t));

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    w->base.widget = vbox;
    gtk_widget_show (vbox);

    GtkWidget *tabstrip = ddb_tabstrip_new ();
    w->tabstrip = (DdbTabStrip *)tabstrip;
    gtk_widget_show (tabstrip);
    GtkWidget *list = ddb_listview_new ();
    w->list = (DdbListview *)list;
    gtk_widget_show (list);

    gtk_box_pack_start (GTK_BOX (vbox), tabstrip, FALSE, TRUE, 0);
    gtk_widget_set_size_request (tabstrip, -1, 24);
    gtk_widget_set_can_focus (tabstrip, FALSE);
    gtk_widget_set_can_default (tabstrip, FALSE);

    gtk_box_pack_start (GTK_BOX (vbox), list, TRUE, TRUE, 0);

    main_playlist_init (list);
    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        ddb_listview_show_header (w->list, 1);
    }
    else {
        ddb_listview_show_header (w->list, 0);
    }

//    gtk_container_forall (GTK_CONTAINER (w->base.widget), w_override_signals, w);
    w_override_signals (w->base.widget, w);

    w->base.message = w_tabbed_playlist_message;
    return (ddb_gtkui_widget_t*)w;
}

///// playlist widget

ddb_gtkui_widget_t *
w_playlist_create (void) {
    w_playlist_t *w = malloc (sizeof (w_playlist_t));
    memset (w, 0, sizeof (w_playlist_t));
    w->base.widget = gtk_event_box_new ();
    w->list = DDB_LISTVIEW (ddb_listview_new ());
    gtk_widget_show (GTK_WIDGET (w->list));
    main_playlist_init (GTK_WIDGET (w->list));
    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        ddb_listview_show_header (DDB_LISTVIEW (w->list), 1);
    }
    else {
        ddb_listview_show_header (DDB_LISTVIEW (w->list), 0);
    }

    gtk_container_add (GTK_CONTAINER (w->base.widget), GTK_WIDGET (w->list));
    w_override_signals (w->base.widget, w);
    w->base.message = w_playlist_message;
    return (ddb_gtkui_widget_t*)w;
}

////// selection properties widget

gboolean
fill_selproperties_cb (gpointer data) {
    w_selproperties_t *w = data;
    DB_playItem_t **tracks = NULL;
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
    trkproperties_fill_meta (store, tracks, numtracks);
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

static int
selproperties_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)w;
    switch (id) {
    case DB_EV_PLAYLISTCHANGED:
    case DB_EV_SELCHANGED:
        {
            g_idle_add (fill_selproperties_cb, w);
        }
        break;
    }
    return 0;
}

ddb_gtkui_widget_t *
w_selproperties_create (void) {
    w_selproperties_t *w = malloc (sizeof (w_selproperties_t));
    memset (w, 0, sizeof (w_selproperties_t));

    w->base.widget = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (w->base.widget, FALSE);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w->base.widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    w->tree = gtk_tree_view_new ();
    gtk_widget_show (w->tree);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->tree);
    w->base.message = selproperties_message;

    GtkListStore *store = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);

    GtkCellRenderer *rend1 = gtk_cell_renderer_text_new ();
    GtkCellRenderer *rend2 = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes (_("Key"), rend1, "text", 0, NULL);
    gtk_tree_view_column_set_resizable (col1, TRUE);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend2, "text", 1, NULL);
    gtk_tree_view_column_set_resizable (col2, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (w->tree), col1);
    gtk_tree_view_append_column (GTK_TREE_VIEW (w->tree), col2);
    GtkCellRenderer *rend_propkey = gtk_cell_renderer_text_new ();
    GtkCellRenderer *rend_propvalue = gtk_cell_renderer_text_new ();
    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (w->tree), TRUE);
    w_override_signals (w->base.widget, w);

    return (ddb_gtkui_widget_t *)w;
}

///// cover art display
void
coverart_avail_callback (void *user_data) {
    w_coverart_t *w = user_data;
    gtk_widget_queue_draw (w->drawarea);
}

static gboolean
coverart_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (!it) {
        return FALSE;
    }
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);
    int width = a.width;
    int height = a.height;
    const char *album = deadbeef->pl_find_meta (it, "album");
    const char *artist = deadbeef->pl_find_meta (it, "artist");
    if (!album || !*album) {
        album = deadbeef->pl_find_meta (it, "title");
    }
    GdkPixbuf *pixbuf = get_cover_art_callb (deadbeef->pl_find_meta ((it), ":URI"), artist, album, min(width,height), coverart_avail_callback, user_data);
    if (pixbuf) {
        int pw = gdk_pixbuf_get_width (pixbuf);
        int ph = gdk_pixbuf_get_height (pixbuf);
        gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
        cairo_rectangle (cr, 0, 0, pw, ph);
        cairo_fill (cr);
//        gdk_draw_pixbuf (gtk_widget_get_window (widget), widget->style->white_gc, pixbuf, 0, 0, a.width/2-pw/2, a.height/2-ph/2, pw, ph, GDK_RGB_DITHER_NONE, 0, 0);
        g_object_unref (pixbuf);
    }
    deadbeef->pl_item_unref (it);
    return TRUE;
}

static gboolean
coverart_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = coverart_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

static gboolean
coverart_redraw_cb (void *user_data) {
    w_coverart_t *w = user_data;
    gtk_widget_queue_draw (w->drawarea);
    return FALSE;
}

static int
coverart_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_coverart_t *ca = (w_coverart_t *)w;
    switch (id) {
    case DB_EV_TRACKINFOCHANGED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
            if (it == ev->track) {
                g_idle_add (coverart_redraw_cb, w);
            }
            if (it) {
                deadbeef->pl_item_unref (it);
            }
        }
        break;
    }
    return 0;
}

ddb_gtkui_widget_t *
w_coverart_create (void) {
    w_coverart_t *w = malloc (sizeof (w_coverart_t));
    memset (w, 0, sizeof (w_coverart_t));

    w->base.widget = gtk_event_box_new ();
    w->base.message = coverart_message;
    w->drawarea = gtk_drawing_area_new ();
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (coverart_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (coverart_draw), w);
#endif
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

///// scope vis
void
w_scope_destroy (ddb_gtkui_widget_t *w) {
    w_scope_t *s = (w_scope_t *)w;
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    if (s->glcontext) {
        gdk_gl_context_destroy (s->glcontext);
        s->glcontext = NULL;
    }
}

gboolean
w_scope_draw_cb (void *data) {
    w_scope_t *s = data;
    gtk_widget_queue_draw (s->drawarea);
    return TRUE;
}

gboolean
scope_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    ddb_waveformat_t fmt;
    float data[DDB_AUDIO_MEMORY_FRAMES];
    deadbeef->audio_get_waveform_data (DDB_AUDIO_WAVEFORM, data);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width (cr, 1);
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    float incr = (float)DDB_AUDIO_MEMORY_FRAMES / a.width;
    float pos = 0;
    for (int x = 0; x < a.width; x++, pos += incr) {
        float s = data[(int)pos];
        cairo_line_to (cr, x, s * a.height/2 / 0x7fff + a.height/2);
    }
    cairo_stroke (cr);

    return FALSE;
}

gboolean
scope_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    w_scope_t *w = user_data;
    float data[DDB_AUDIO_MEMORY_FRAMES];
    deadbeef->audio_get_waveform_data (DDB_AUDIO_WAVEFORM, data);
    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    GdkGLDrawable *d = gtk_widget_get_gl_drawable (widget);
    gdk_gl_drawable_gl_begin (d, w->glcontext);
    //        if (glXSwapIntervalSGI) {
    //            glXSwapIntervalSGI (1);
    //        }

    glClear (GL_COLOR_BUFFER_BIT);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluOrtho2D(0,a.width,a.height,0);
    glMatrixMode (GL_MODELVIEW);
    glViewport (0, 0, a.width, a.height);

#if 0
    // vsync test
    static int box = 0;
    static int speed = 5;
    if (box > a.width-50) {
        box = a.width-50;
        speed = -5;
    }
    else if (box < 0) {
        box = 0;
        speed = 5;
    }
    box += speed;

    glBegin (GL_QUADS);
    glVertex2f (box, 0);
    glVertex2f (box+50, 0);
    glVertex2f (box+50, 50);
    glVertex2f (box, 50);
    glEnd ();
#endif
    glBegin (GL_LINE_STRIP);

    short *samples = (short *)data;

    float incr = a.width / (float)DDB_AUDIO_MEMORY_FRAMES;
    int pos = 0;
    for (float x = 0; x < a.width && pos < DDB_AUDIO_MEMORY_FRAMES; x += incr, pos ++) {
        float s = data[(int)pos];
        glVertex2f (x, s * a.height/2 + a.height/2);
    }

    glEnd();
    gdk_gl_drawable_swap_buffers (d);

    gdk_gl_drawable_gl_end (d);

    return FALSE;

    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = scope_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

void
w_scope_init (ddb_gtkui_widget_t *w) {
    w_scope_t *s = (w_scope_t *)w;
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    if (!gtkui_gl_init ()) {
        s->drawtimer = g_timeout_add (33, w_scope_draw_cb, w);
    }
}

void
scope_realize (GtkWidget *widget, gpointer data) {
    w_scope_t *w = data;
    w->glcontext = gtk_widget_create_gl_context (w->drawarea, NULL, TRUE, GDK_GL_RGBA_TYPE);
}

ddb_gtkui_widget_t *
w_scope_create (void) {
    w_scope_t *w = malloc (sizeof (w_scope_t));
    memset (w, 0, sizeof (w_scope_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_scope_init;
    w->base.destroy  = w_scope_destroy;
    w->drawarea = gtk_drawing_area_new ();
    int attrlist[] = {GDK_GL_ATTRIB_LIST_NONE};
    GdkGLConfig *conf = gdk_gl_config_new_by_mode ((GdkGLConfigMode)(GDK_GL_MODE_RGB |
                        GDK_GL_MODE_DOUBLE));
    gboolean cap = gtk_widget_set_gl_capability (w->drawarea, conf, NULL, TRUE, GDK_GL_RGBA_TYPE);
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (scope_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (scope_draw), w);
#endif
    g_signal_connect_after (G_OBJECT (w->drawarea), "realize", G_CALLBACK (scope_realize), w);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

///// spectrum vis
void
w_spectrum_destroy (ddb_gtkui_widget_t *w) {
    w_spectrum_t *s = (w_spectrum_t *)w;
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    if (s->glcontext) {
        gdk_gl_context_destroy (s->glcontext);
        s->glcontext = NULL;
    }
}

gboolean
w_spectrum_draw_cb (void *data) {
    w_spectrum_t *s = data;
    gtk_widget_queue_draw (s->drawarea);
    return TRUE;
}

// spectrum analyzer based on cairo-spectrum from audacious
// Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
#define MAX_BANDS 256
#define VIS_DELAY 1
#define VIS_DELAY_PEAK 10
#define VIS_FALLOFF 3
#define VIS_FALLOFF_PEAK 1
#define BAND_WIDTH 5
static float xscale[MAX_BANDS + 1];
static int bars[MAX_BANDS + 1];
static int delay[MAX_BANDS + 1];
static int peaks[MAX_BANDS + 1];
static int delay_peak[MAX_BANDS + 1];

static void calculate_bands(int bands)
{
	int i;

	for (i = 0; i < bands; i++)
		xscale[i] = powf(257., ((float) i / (float) bands)) - 1;
}


gboolean
spectrum_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    w_spectrum_t *w = user_data;
    float data[DDB_AUDIO_MEMORY_FRAMES];
    float *freq = data;
    deadbeef->audio_get_waveform_data (DDB_AUDIO_FREQ, data);

    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    int width, height, bands;
    bands = a.width/BAND_WIDTH;
    bands = CLAMP(bands, 4, MAX_BANDS);
    width = a.width;
    height = a.height;
	calculate_bands(bands);

	for (int i = 0; i < bands; i ++)
	{
		int a = ceil (xscale[i]);
		int b = floor (xscale[i + 1]);
		float n = 0;

		if (b < a)
			n += freq[b] * (xscale[i + 1] - xscale[i]);
		else
		{
			if (a > 0)
				n += freq[a - 1] * (a - xscale[i]);
			for (; a < b; a ++)
				n += freq[a];
			if (b < 256)
				n += freq[b] * (xscale[i + 1] - b);
		}

		/* 40 dB range */
		int x = 20 * log10 (n * 100);
		x = CLAMP (x, 0, 40);

		bars[i] -= MAX (0, VIS_FALLOFF - delay[i]);
		peaks[i] -= MAX (0, VIS_FALLOFF_PEAK - delay_peak[i]);;

		if (delay[i])
			delay[i]--;
		if (delay_peak[i])
			delay_peak[i]--;

		if (x > bars[i])
		{
			bars[i] = x;
			delay[i] = VIS_DELAY;
		}
		if (x > peaks[i]) {
            peaks[i] = x;
            delay_peak[i] = VIS_DELAY_PEAK;
        }
	}

    GdkGLDrawable *d = gtk_widget_get_gl_drawable (widget);
    gdk_gl_drawable_gl_begin (d, w->glcontext);

    glClear (GL_COLOR_BUFFER_BIT);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluOrtho2D(0,a.width,a.height,0);
    glMatrixMode (GL_MODELVIEW);
    glViewport (0, 0, a.width, a.height);

    glBegin (GL_QUADS);
	float base_s = (height / 40.f);

	for (gint i = 0; i <= bands; i++)
	{
		gint x = ((width / bands) * i) + 2;
        int y = a.height - bars[i] * base_s;
        glColor3f (0, 0.5, 1);
		glVertex2f (x + 1, y);
		glVertex2f (x + 1 + (width / bands) - 1, y);
		glVertex2f (x + 1 + (width / bands) - 1, a.height);
		glVertex2f (x + 1, a.height);

        // peak
        glColor3f (1, 1, 1);
        y = a.height - peaks[i] * base_s;
		glVertex2f (x + 1, y);
		glVertex2f (x + 1 + (width / bands) - 1, y);
		glVertex2f (x + 1 + (width / bands) - 1, y+1);
		glVertex2f (x + 1, y+1);
	}
    glEnd();
    gdk_gl_drawable_swap_buffers (d);

    gdk_gl_drawable_gl_end (d);

    return FALSE;
}

void
w_spectrum_init (ddb_gtkui_widget_t *w) {
    w_spectrum_t *s = (w_spectrum_t *)w;
    gtkui_gl_init ();
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    if (!gtkui_gl_init ()) {
        s->drawtimer = g_timeout_add (33, w_spectrum_draw_cb, w);
    }
}

void
spectrum_realize (GtkWidget *widget, gpointer data) {
    w_spectrum_t *w = data;
    w->glcontext = gtk_widget_create_gl_context (w->drawarea, NULL, TRUE, GDK_GL_RGBA_TYPE);
}

ddb_gtkui_widget_t *
w_spectrum_create (void) {
    w_spectrum_t *w = malloc (sizeof (w_spectrum_t));
    memset (w, 0, sizeof (w_spectrum_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_spectrum_init;
    w->base.destroy  = w_spectrum_destroy;
    w->drawarea = gtk_drawing_area_new ();
    int attrlist[] = {GDK_GL_ATTRIB_LIST_NONE};
    GdkGLConfig *conf = gdk_gl_config_new_by_mode ((GdkGLConfigMode)(GDK_GL_MODE_RGB |
                        GDK_GL_MODE_DOUBLE));
    gboolean cap = gtk_widget_set_gl_capability (w->drawarea, conf, NULL, TRUE, GDK_GL_RGBA_TYPE);
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3,0,0)
    g_signal_connect_after ((gpointer) w->drawarea, "expose_event", G_CALLBACK (spectrum_expose_event), w);
#else
    g_signal_connect_after ((gpointer) w->drawarea, "draw", G_CALLBACK (spectrum_draw), w);
#endif
    g_signal_connect_after (G_OBJECT (w->drawarea), "realize", G_CALLBACK (spectrum_realize), w);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}

// hbox and vbox
static void
w_hvbox_append (struct ddb_gtkui_widget_s *container, struct ddb_gtkui_widget_s *child) {
    w_hvbox_t *b = (w_hvbox_t *)container;
    gtk_box_pack_start (GTK_BOX (b->box), child->widget, TRUE, TRUE, 0);
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

    gtk_container_remove (GTK_CONTAINER(b->box), c->widget);
    c->widget = NULL;
    w_destroy (c);

    gtk_box_pack_start (GTK_BOX (b->box), newchild->widget, TRUE, TRUE, 0);
    gtk_widget_show (newchild->widget);
    gtk_box_reorder_child (GTK_BOX (b->box), newchild->widget, n);

}

static void
on_hvbox_expand (GtkMenuItem *menuitem, gpointer user_data) {
    w_append ((ddb_gtkui_widget_t*)user_data, w_create ("placeholder"));
}

static void
on_hvbox_shrink (GtkMenuItem *menuitem, gpointer user_data) {
    ddb_gtkui_widget_t *w = (ddb_gtkui_widget_t *)user_data;
    ddb_gtkui_widget_t *c;
    for (c = w->children; c && c->next; c = c->next);
    if (c) {
        w_remove (w, c);
    }
    if (!w->children) {
        w_append (w, w_create ("placeholder"));
    }
}

static void
w_hvbox_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_menu_item_new_with_mnemonic (_("Expand the box by 1 item"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate", G_CALLBACK (on_hvbox_expand), w);
    
    item = gtk_menu_item_new_with_mnemonic (_("Shrink the box by 1 item"));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "activate", G_CALLBACK (on_hvbox_shrink), w);
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
    w->box = gtk_hbox_new (TRUE, 3);
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);

    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));

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
    w->base.initmenu = w_hvbox_initmenu;
    w->box = gtk_vbox_new (TRUE, 3);
    gtk_widget_show (w->box);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->box);

    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));
    w_append ((ddb_gtkui_widget_t*)w, w_create ("placeholder"));

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
