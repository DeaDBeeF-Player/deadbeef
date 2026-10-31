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
#include "../ddbtabstrip.h"
#include "../drawing.h"
#include "../gtkui.h"
#include "../playlist/ddblistview.h"
#include "../playlist/playlistcontroller.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_tabbed_playlist.h"
#include "w_playlist.h"

typedef struct {
    w_playlist_t plt;
    DdbTabStrip *tabstrip;
} w_tabbed_playlist_t;

//// tabbed playlist widget

static gboolean
playlist_tabstriprefresh_cb (gpointer p) {
    w_tabbed_playlist_t *tp = p;
    ddb_tabstrip_refresh (tp->tabstrip);
    g_object_unref (tp->tabstrip);
    return FALSE;
}

static int
w_tabbed_playlist_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        if (ctx) {
            char *str = (char *)ctx;
            if (gtkui_tabstrip_override_conf (str) || gtkui_tabstrip_colors_conf (str) || gtkui_tabstrip_font_conf (str) || gtkui_tabstrip_font_style_conf (str)) {
                w_tabbed_playlist_t *p = (w_tabbed_playlist_t *)w;
                g_object_ref (p->tabstrip);
                g_idle_add (playlist_tabstriprefresh_cb, w);
            }
        }
        break;
    case DB_EV_TRACKINFOCHANGED:
    case DB_EV_PLAYLISTSWITCHED: {
        w_tabbed_playlist_t *p = (w_tabbed_playlist_t *)w;
        g_object_ref (p->tabstrip);
        g_idle_add (playlist_tabstriprefresh_cb, w);
    } break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_TITLE || p1 == DDB_PLAYLIST_CHANGE_POSITION || p1 == DDB_PLAYLIST_CHANGE_DELETED || p1 == DDB_PLAYLIST_CHANGE_CREATED) {
            w_tabbed_playlist_t *p = (w_tabbed_playlist_t *)w;
            g_object_ref (p->tabstrip);
            g_idle_add (playlist_tabstriprefresh_cb, w);
        }
        break;
    }
    w_playlist_message (w, id, ctx, p1, p2);
    return 0;
}

ddb_gtkui_widget_t *
w_tabbed_playlist_create (void) {
    w_tabbed_playlist_t *w = malloc (sizeof (w_tabbed_playlist_t));
    memset (w, 0, sizeof (w_tabbed_playlist_t));

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    w->plt.base.widget = vbox;
    w->plt.base.save = w_playlist_save;
    w->plt.base.load = w_playlist_load;
    w->plt.base.init = w_playlist_init;
    w->plt.base.destroy = w_playlist_destroy;
    w->plt.base.initmenu = w_playlist_initmenu;
    gtk_widget_show (vbox);

    GtkWidget *tabstrip = ddb_tabstrip_new ();
    w->tabstrip = DDB_TABSTRIP (tabstrip);
    gtk_widget_show (tabstrip);
    DdbListview *listview = DDB_LISTVIEW (ddb_listview_new ());
    gtk_widget_set_size_request (vbox, 250, 100);

    w->plt.listview = listview;
    w->plt.controller = playlist_controller_new (listview, FALSE);

    gtk_widget_show (GTK_WIDGET (listview));

    GtkWidget *sepbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (sepbox);
    gtk_container_set_border_width (GTK_CONTAINER (sepbox), 1);

    GtkWidget *hsep = gtk_hseparator_new ();
    gtk_widget_show (hsep);
    gtk_box_pack_start (GTK_BOX (sepbox), hsep, FALSE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), tabstrip, FALSE, TRUE, 0);
    gtk_widget_set_can_focus (tabstrip, FALSE);
    gtk_widget_set_can_default (tabstrip, FALSE);

    gtk_box_pack_start (GTK_BOX (vbox), sepbox, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (listview), TRUE, TRUE, 0);

    w_override_signals (w->plt.base.widget, w);

    w->plt.base.message = w_tabbed_playlist_message;
    return (ddb_gtkui_widget_t *)w;
}
