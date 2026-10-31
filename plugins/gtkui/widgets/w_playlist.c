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
#include "../playlist/ddblistview.h"
#include "../playlist/playlistcontroller.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_playlist.h"

int
w_playlist_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_playlist_t *p = (w_playlist_t *)w;
    playlist_controller_message (p->controller, id, ctx, p1, p2);
    return 0;
}

const char *
w_playlist_load (struct ddb_gtkui_widget_s *w, const char *type, const char *s) {
    if (strcmp (type, "playlist") && strcmp (type, "tabbed_playlist")) {
        return NULL;
    }
    char key[MAX_TOKEN], val[MAX_TOKEN];
    for (;;) {
        get_keyvalue (s, key, val);
        if (!strcmp (key, "hideheaders")) {
            ((w_playlist_t *)w)->hideheaders = atoi (val);
        }
        if (!strcmp (key, "width")) {
            ((w_playlist_t *)w)->width = atoi (val);
        }
    }

    return s;
}

void
w_playlist_save (struct ddb_gtkui_widget_s *w, char *s, int sz) {
    w_playlist_t *ww = (w_playlist_t *)w;

    GtkAllocation a;
    gtk_widget_get_allocation (ww->base.widget, &a);
    int width = a.width;

    char save[100];
    snprintf (save, sizeof (save), " hideheaders=%d width=%d", ww->hideheaders, width);
    strncat (s, save, sz);
}

void
w_playlist_init (ddb_gtkui_widget_t *base) {
    w_playlist_t *w = (w_playlist_t *)base;

    playlist_controller_init (w->controller, !w->hideheaders, w->width);
}

void
w_playlist_destroy (ddb_gtkui_widget_t *base) {
    w_playlist_t *w = (w_playlist_t *)base;
    playlist_controller_free (w->controller);
    w->controller = NULL;
}

static void
on_playlist_showheaders_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data) {
    w_playlist_t *w = user_data;
    w->hideheaders = !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem));
    ddb_listview_show_header (DDB_LISTVIEW (w->listview), !w->hideheaders);
    w_save_with_action (_("Toggle Column Headers"));
}

void
w_playlist_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_check_menu_item_new_with_mnemonic (_ ("Show Column Headers"));
    gtk_widget_show (item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), !((w_playlist_t *)w)->hideheaders);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (on_playlist_showheaders_toggled), w);
}

///// playlist widget

ddb_gtkui_widget_t *
w_playlist_create (void) {
    w_playlist_t *w = malloc (sizeof (w_playlist_t));
    memset (w, 0, sizeof (w_playlist_t));

    w->base.widget = gtk_event_box_new ();
    DdbListview *listview = DDB_LISTVIEW (ddb_listview_new ());

    w->listview = listview;
    w->controller = playlist_controller_new (listview, FALSE);

    gtk_widget_set_size_request (GTK_WIDGET (w->base.widget), 100, 100);
    w->base.save = w_playlist_save;
    w->base.load = w_playlist_load;
    w->base.init = w_playlist_init;
    w->base.destroy = w_playlist_destroy;
    w->base.initmenu = w_playlist_initmenu;

    gtk_widget_show (GTK_WIDGET (listview));

    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        ddb_listview_show_header (DDB_LISTVIEW (listview), 1);
    }
    else {
        ddb_listview_show_header (DDB_LISTVIEW (listview), 0);
    }

    gtk_container_add (GTK_CONTAINER (w->base.widget), GTK_WIDGET (listview));
    w_override_signals (w->base.widget, w);
    w->base.message = w_playlist_message;
    return (ddb_gtkui_widget_t *)w;
}
