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
#include "gtkui.h"
#include "widgets.h"
#include "ddbtabstrip.h"
#include "ddblistview.h"
#include "mainplaylist.h"

typedef struct w_creator_s {
    const char *type;
    ddb_gtkui_widget_t *(*create_func) (void);
    struct w_creator_s *next;
} w_creator_t;

static w_creator_t *w_creators;

typedef struct {
    ddb_gtkui_widget_t base;
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
} w_playlist_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_placeholder_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_tabs_t;

static int design_mode;
static ddb_gtkui_widget_t *rootwidget;

//// common functions

void
w_init (void) {
    rootwidget = w_create ("box");
}

void
w_free (void) {
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
    if (GTK_IS_EVENT_BOX (w->widget)) {
        if (design_mode) {
            gtk_event_box_set_above_child (GTK_EVENT_BOX (w->widget), TRUE);
        }
        else {
            gtk_event_box_set_above_child (GTK_EVENT_BOX (w->widget), FALSE);
        }
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

static ddb_gtkui_widget_t *current_widget;
static int hidden = 0;

gboolean
w_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    if (hidden && user_data == current_widget) {
        GdkColor clr = {
            .red = 0x2d00,
            .green = 0x0000,
            .blue = 0xd600
        };
        GdkGC *gc = gdk_gc_new (widget->window);
        gdk_gc_set_rgb_fg_color (gc, &clr);
        if (GTK_WIDGET_NO_WINDOW (widget)) {
            gdk_draw_rectangle (widget->window, gc, TRUE, widget->allocation.x, widget->allocation.y, widget->allocation.width, widget->allocation.height);
        }
        else {
            gdk_draw_rectangle (widget->window, gc, TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
        }
        g_object_unref (gc);
        return TRUE;
    }

    return FALSE;
}

static void
on_replace_activate (GtkMenuItem *menuitem, gpointer user_data) {
    for (w_creator_t *cr = w_creators; cr; cr = cr->next) {
        if (cr->type == user_data) {
            ddb_gtkui_widget_t *parent = current_widget->parent;
            if (parent->replace) {
                parent->replace (parent, current_widget, w_create (user_data));
            }
            else {
                w_remove (parent, current_widget);
                w_destroy (current_widget);
                current_widget = w_create (user_data);
                w_append (parent, current_widget);
                if (GTK_IS_EVENT_BOX (current_widget->widget)) {
                    gtk_event_box_set_above_child (GTK_EVENT_BOX (current_widget->widget), FALSE);
                    gtk_event_box_set_above_child (GTK_EVENT_BOX (current_widget->widget), TRUE);
                }
            }
        }
    }
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
        if (GTK_IS_EVENT_BOX (w->widget)) {
            // for some reason, after gtk_widget_show, eventbox appears behind,
            // so here we have a workaround -- push it back on top
            gtk_event_box_set_above_child (GTK_EVENT_BOX (w->widget), FALSE);
            gtk_event_box_set_above_child (GTK_EVENT_BOX (w->widget), TRUE);
        }
    }
    gtk_widget_queue_draw (w->widget);
}

gboolean
w_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (!design_mode || event->button != 3) {
        return FALSE;
    }
    printf ("button_press on %s (%p)\n", G_OBJECT_TYPE_NAME (widget), widget);

    current_widget = user_data;
    hidden = 1;
    if (GTK_IS_CONTAINER (widget)) {
        gtk_container_foreach (GTK_CONTAINER (widget), hide_widget, NULL);
    }
    gtk_widget_queue_draw (((ddb_gtkui_widget_t *)user_data)->widget);
    GtkWidget *menu;
    GtkWidget *submenu;
    GtkWidget *item;
    menu = gtk_menu_new ();
    item = gtk_menu_item_new_with_mnemonic ("Replace with...");
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);

    submenu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

    for (w_creator_t *cr = w_creators; cr; cr = cr->next) {
        item = gtk_menu_item_new_with_mnemonic (cr->type);
        gtk_widget_show (item);
        gtk_container_add (GTK_CONTAINER (submenu), item);
        g_signal_connect ((gpointer) item, "activate",
                G_CALLBACK (on_replace_activate),
                (void *)cr->type);
    }

    item = gtk_menu_item_new_with_mnemonic ("Delete");
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    item = gtk_menu_item_new_with_mnemonic ("Cut");
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    item = gtk_menu_item_new_with_mnemonic ("Copy");
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    item = gtk_menu_item_new_with_mnemonic ("Paste");
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) menu, "deactivate", G_CALLBACK (w_menu_deactivate), user_data);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, widget, 0, gtk_get_current_event_time());
    return TRUE;
}

static void
w_override_signals (GtkWidget *widget, gpointer user_data) {
    printf ("w_override_signals on %s (%p)\n", G_OBJECT_TYPE_NAME (widget), widget);
    g_signal_connect ((gpointer) widget, "button_press_event", G_CALLBACK (w_button_press_event), user_data);
    g_signal_connect ((gpointer) widget, "expose_event", G_CALLBACK (w_expose_event), user_data);
    if (GTK_IS_CONTAINER (widget)) {
        gtk_container_forall (GTK_CONTAINER (widget), w_override_signals, user_data);
    }
}

void
w_reg_widget (const char *type, ddb_gtkui_widget_t *(*create_func) (void)) {
    w_creator_t *c;
    for (c = w_creators; c; c = c->next) {
        if (!strcmp (c->type, type)) {
            fprintf (stderr, "gtkui w_reg_widget: widget type %s already registered\n");
            return;
        }
    }
    c = malloc (sizeof (w_creator_t));
    memset (c, 0, sizeof (w_creator_t));
    c->type = type;
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
    fprintf (stderr, "gtkui w_unreg_widget: widget type %s is not registered\n");
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
w_destroy (ddb_gtkui_widget_t *w) {
    if (w->destroy) {
        w->destroy (w);
    }
    if (w->widget) {
        gtk_widget_destroy (w->widget);
    }
    free (w);
}

///// gtk_container convenience functions
void
w_container_add (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    printf ("append %s to %s\n", child->type, cont->type);
    GtkWidget *container = NULL;
    if (GTK_IS_EVENT_BOX (cont->widget)) {
        container = gtk_bin_get_child (GTK_BIN(cont->widget));
    }
    else {
        container = cont->widget;
    }
    gtk_container_add (GTK_CONTAINER (container), child->widget);
    gtk_widget_show (child->widget);
}

void
w_container_remove (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    printf ("remove %s from %s\n", child->type, cont->type);
    GtkWidget *container = NULL;
    if (GTK_IS_EVENT_BOX (cont->widget)) {
        container = gtk_bin_get_child (GTK_BIN(cont->widget));
    }
    else {
        container = cont->widget;
    }
    gtk_container_remove (GTK_CONTAINER (container), child->widget);

}

////// placeholder widget

gboolean
w_placeholder_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (widget->window);
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
    cairo_rectangle (cr, 0, 0, widget->allocation.width, widget->allocation.height);
    cairo_paint (cr);
    cairo_surface_destroy (checker);
    cairo_destroy (cr);
    return FALSE;
}

ddb_gtkui_widget_t *
w_placeholder_create (void) {
    w_placeholder_t *w = malloc (sizeof (w_placeholder_t));
    memset (w, 0, sizeof (w_placeholder_t));
    w->base.widget = gtk_drawing_area_new ();
    gtk_widget_set_events (w->base.widget, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect ((gpointer) w->base.widget, "expose_event", G_CALLBACK (w_expose_event), w);
    g_signal_connect_after ((gpointer) w->base.widget, "expose_event", G_CALLBACK (w_placeholder_expose_event), w);
    g_signal_connect ((gpointer) w->base.widget, "button_press_event", G_CALLBACK (w_button_press_event), w);
    return (ddb_gtkui_widget_t*)w;
}

////// vsplitter widget

ddb_gtkui_widget_t *
w_vsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->base.widget = gtk_vpaned_new ();
    w->base.append = w_container_add;
    w->base.remove = w_container_remove;

    ddb_gtkui_widget_t *ph1, *ph2;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");
    g_signal_connect ((gpointer) w->base.widget, "expose_event", G_CALLBACK (w_expose_event), w);
    g_signal_connect ((gpointer) w->base.widget, "button_press_event", G_CALLBACK (w_button_press_event), w);

    w_append ((ddb_gtkui_widget_t*)w, ph1);
    w_append ((ddb_gtkui_widget_t*)w, ph2);

    return (ddb_gtkui_widget_t*)w;
}

////// hsplitter widget

ddb_gtkui_widget_t *
w_hsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->base.widget = gtk_hpaned_new ();
    w->base.append = w_container_add;
    w->base.remove = w_container_remove;

    ddb_gtkui_widget_t *ph1, *ph2;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");
    g_signal_connect ((gpointer) w->base.widget, "expose_event", G_CALLBACK (w_expose_event), w);
    g_signal_connect ((gpointer) w->base.widget, "button_press_event", G_CALLBACK (w_button_press_event), w);

    w_append ((ddb_gtkui_widget_t*)w, ph1);
    w_append ((ddb_gtkui_widget_t*)w, ph2);

    return (ddb_gtkui_widget_t*)w;
}

///// tabs widget

void
w_tabs_add (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    GtkWidget *label = gtk_label_new (child->type);
    gtk_widget_show (label);
    gtk_widget_show (child->widget);
    gtk_notebook_append_page (GTK_NOTEBOOK (cont->widget), child->widget, label);
}

void
w_tabs_replace (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child, ddb_gtkui_widget_t *newchild) {
    printf ("w_tabs_replace %p\n", child);
    int ntab = 0;
    ddb_gtkui_widget_t *prev = NULL;
    for (ddb_gtkui_widget_t *c = cont->children; c; c = c->next, ntab++) {
        if (c == child) {
            printf ("removing tab %d\n", ntab);
            newchild->next = c->next;
            if (prev) {
                prev->next = newchild;
            }
            else {
                cont->children = newchild;
            }
            newchild->parent = cont;
            w_destroy (c);
            GtkWidget *label = gtk_label_new (newchild->type);
            gtk_widget_show (label);
            gtk_widget_show (newchild->widget);
            int pos = gtk_notebook_insert_page (GTK_NOTEBOOK (cont->widget), newchild->widget, label, ntab);
            gtk_notebook_set_page (GTK_NOTEBOOK (cont->widget), pos);
            break;
        }
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

    ddb_gtkui_widget_t *ph1, *ph2, *ph3;
    ph1 = w_create ("placeholder");
    ph2 = w_create ("placeholder");
    ph3 = w_create ("placeholder");

    g_signal_connect ((gpointer) w->base.widget, "expose_event", G_CALLBACK (w_expose_event), w);
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
    w->base.flags = DDB_W_CONTAINER | DDB_W_CONTAINER_MULTIPLE;
    w->base.append = w_container_add; 
    w->base.remove = w_container_remove;

    return (ddb_gtkui_widget_t*)w;
}

//// tabstrip widget

ddb_gtkui_widget_t *
w_tabstrip_create (void) {
    w_tabstrip_t *w = malloc (sizeof (w_tabstrip_t));
    memset (w, 0, sizeof (w_tabstrip_t));
    w->base.widget = ddb_tabstrip_new ();
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t*)w;
}

//// tabbed playlist widget

typedef struct {
    ddb_gtkui_widget_t *w;
    DB_playItem_t *trk;
} w_trackdata_t;

static gboolean
trackinfochanged_cb (gpointer p) {
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
    deadbeef->pl_item_unref (d->trk);
    free (d);
    return FALSE;
}

static gboolean
paused_cb (gpointer p) {
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
refresh_cb (gpointer p) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)p;
    ddb_listview_clear_sort (tp->list);
    ddb_listview_refresh (tp->list, DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
    return FALSE;
}


static gboolean
playlistswitch_cb (gpointer p) {
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

struct fromto_t {
    ddb_gtkui_widget_t *w;
    DB_playItem_t *from;
    DB_playItem_t *to;
};

static gboolean
songchanged_cb (gpointer p) {
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
        g_idle_add (refresh_cb, w);
        break;
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (playlistswitch_cb, w);
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
    GtkWidget *frame = gtk_frame_new (NULL);
    gtk_widget_show (frame);

    gtk_box_pack_start (GTK_BOX (vbox), tabstrip, FALSE, TRUE, 0);
    gtk_widget_set_size_request (tabstrip, -1, 24);
    GTK_WIDGET_UNSET_FLAGS (tabstrip, GTK_CAN_FOCUS);
    GTK_WIDGET_UNSET_FLAGS (tabstrip, GTK_CAN_DEFAULT);

    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 1);

    gtk_container_add (GTK_CONTAINER (frame), list);
    main_playlist_init (list);
    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        ddb_listview_show_header (w->list, 1);
    }
    else {
        ddb_listview_show_header (w->list, 0);
    }

    gtk_container_forall (GTK_CONTAINER (w->base.widget), w_override_signals, w);

    w->base.message = w_tabbed_playlist_message;
    return (ddb_gtkui_widget_t*)w;
}

///// playlist widget

ddb_gtkui_widget_t *
w_playlist_create (void) {
    w_playlist_t *w = malloc (sizeof (w_playlist_t));
    memset (w, 0, sizeof (w_playlist_t));
    w->base.widget = ddb_listview_new ();
    main_playlist_init (w->base.widget);
    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        ddb_listview_show_header (DDB_LISTVIEW (w->base.widget), 1);
    }
    else {
        ddb_listview_show_header (DDB_LISTVIEW (w->base.widget), 0);
    }

    w_override_signals (w->base.widget, w);

    w->base.message = w_tabbed_playlist_message;
    return (ddb_gtkui_widget_t*)w;
}
