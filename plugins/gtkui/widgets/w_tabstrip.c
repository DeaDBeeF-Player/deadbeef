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
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_tabstrip.h"

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *tabstrip;
} w_tabstrip_t;

//// tabstrip widget
static gboolean
tabstrip_refresh_cb (void *ctx) {
    w_tabstrip_t *w = ctx;
    ddb_tabstrip_refresh (DDB_TABSTRIP (w->tabstrip));
    return FALSE;
}
static int
w_tabstrip_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_TITLE || p1 == DDB_PLAYLIST_CHANGE_POSITION || p1 == DDB_PLAYLIST_CHANGE_DELETED || p1 == DDB_PLAYLIST_CHANGE_CREATED) {
            g_idle_add (tabstrip_refresh_cb, w);
        }
        break;
    case DB_EV_CONFIGCHANGED:
        if (ctx) {
            char *conf_str = (char *)ctx;
            if (gtkui_tabstrip_override_conf (conf_str) || gtkui_tabstrip_colors_conf (conf_str) || gtkui_tabstrip_font_conf (conf_str)) {
                g_idle_add (tabstrip_refresh_cb, w);
            }
        }
    case DB_EV_PLAYLISTSWITCHED:
    case DB_EV_TRACKINFOCHANGED:
        g_idle_add (tabstrip_refresh_cb, w);
        break;
    }
    return 0;
}

ddb_gtkui_widget_t *
w_tabstrip_create (void) {
    w_tabstrip_t *w = malloc (sizeof (w_tabstrip_t));
    memset (w, 0, sizeof (w_tabstrip_t));
    w->base.flags = DDB_GTKUI_WIDGET_FLAG_NON_EXPANDABLE;
    w->base.widget = gtk_event_box_new ();
    w->base.message = w_tabstrip_message;
    GtkWidget *ts = ddb_tabstrip_new ();
    gtk_widget_show (ts);
    gtk_container_add (GTK_CONTAINER (w->base.widget), ts);
    w->tabstrip = ts;
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
