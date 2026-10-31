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
#include "../ddbvolumebar.h"
#include "../drawing.h"
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_volumebar.h"

typedef struct {
    ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
    GtkWidget *volumebar;
} w_volumebar_t;

// volumebar
static gboolean
redraw_volumebar_cb (gpointer data) {
    w_volumebar_t *w = data;
    ddb_volumebar_update (DDB_VOLUMEBAR (w->volumebar));
    return FALSE;
}

static int
w_volumebar_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        if (ctx) {
            char *conf_str = (char *)ctx;
            if (gtkui_bar_override_conf (conf_str) || gtkui_bar_colors_conf (conf_str)) {
                g_idle_add (redraw_volumebar_cb, w);
            }
        }
        break;
    case DB_EV_VOLUMECHANGED:
        g_idle_add (redraw_volumebar_cb, w);
        break;
    }
    return 0;
}

static void
w_volumebar_dbscale_activate (GtkWidget *item, struct ddb_gtkui_widget_s *w) {
    DdbVolumeBar *bar = DDB_VOLUMEBAR (((w_volumebar_t *)w)->volumebar);
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item))) {
        ddb_volumebar_set_scale (bar, DDB_VOLUMEBAR_SCALE_DB);
        ddb_volumebar_update (bar);
        w_save_with_action (_("Change Volume Scale"));
    }
}

static void
w_volumebar_linearscale_activate (GtkWidget *item, struct ddb_gtkui_widget_s *w) {
    DdbVolumeBar *bar = DDB_VOLUMEBAR (((w_volumebar_t *)w)->volumebar);
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item))) {
        ddb_volumebar_set_scale (bar, DDB_VOLUMEBAR_SCALE_LINEAR);
        ddb_volumebar_update (bar);
        w_save_with_action (_("Change Volume Scale"));
    }
}

static void
w_volumebar_cubicscale_activate (GtkWidget *item, struct ddb_gtkui_widget_s *w) {
    DdbVolumeBar *bar = DDB_VOLUMEBAR (((w_volumebar_t *)w)->volumebar);
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item))) {
        ddb_volumebar_set_scale (bar, DDB_VOLUMEBAR_SCALE_CUBIC);
        ddb_volumebar_update (bar);
        w_save_with_action (_("Change Volume Scale"));
    }
}

static void
w_volumebar_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    GSList *group = NULL;
    w_volumebar_t *widget = (w_volumebar_t *)w;
    DdbVolumeBarScale scale = ddb_volumebar_get_scale (DDB_VOLUMEBAR (widget->volumebar));
    item = gtk_radio_menu_item_new_with_mnemonic (group, _ ("_dB Scale"));
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (w_volumebar_dbscale_activate), w);
    if (scale == DDB_VOLUMEBAR_SCALE_DB) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }

    item = gtk_radio_menu_item_new_with_mnemonic (group, _ ("_Linear Scale"));
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (w_volumebar_linearscale_activate), w);
    if (scale == DDB_VOLUMEBAR_SCALE_LINEAR) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }

    item = gtk_radio_menu_item_new_with_mnemonic (group, _ ("_Cubic Scale"));
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer)item, "toggled", G_CALLBACK (w_volumebar_cubicscale_activate), w);
    if (scale == DDB_VOLUMEBAR_SCALE_CUBIC) {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    }
}

static gboolean
on_volumebar_evbox_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkWidget *menu;

        menu = gtk_menu_new ();
        w_volumebar_initmenu (user_data, menu);
        gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
        return TRUE;
    }
    return FALSE;
}

static void
w_volumebar_deserialize_from_keyvalues (ddb_gtkui_widget_t *base, const char **keyvalues) {
    w_volumebar_t *vb = (w_volumebar_t *)base;
    for (int i = 0; keyvalues[i] != NULL; i += 2) {
        if (!strcmp (keyvalues[i], "scale")) {
            DdbVolumeBarScale scale = DDB_VOLUMEBAR_SCALE_DB;
            if (!strcmp (keyvalues[i + 1], "linear")) {
                scale = DDB_VOLUMEBAR_SCALE_LINEAR;
            }
            else if (!strcmp (keyvalues[i + 1], "cubic")) {
                scale = DDB_VOLUMEBAR_SCALE_CUBIC;
            }
            else {
                int iscale = atoi (keyvalues[i + 1]);
                if (iscale > 0 && iscale < DDB_VOLUMEBAR_SCALE_COUNT) {
                    scale = (DdbVolumeBarScale)iscale;
                }
            }
            ddb_volumebar_set_scale (DDB_VOLUMEBAR (vb->volumebar), scale);
        }
    }
}

static char const **
w_volumebar_serialize_to_keyvalues (ddb_gtkui_widget_t *base) {
    w_volumebar_t *vb = (w_volumebar_t *)base;
    DdbVolumeBarScale scale = ddb_volumebar_get_scale (DDB_VOLUMEBAR (vb->volumebar));
    char const **kv = calloc (3, sizeof (char *));
    kv[0] = "scale";
    switch (scale) {
    case DDB_VOLUMEBAR_SCALE_LINEAR:
        kv[1] = "linear";
        break;
    case DDB_VOLUMEBAR_SCALE_CUBIC:
        kv[1] = "cubic";
        break;
    case DDB_VOLUMEBAR_SCALE_DB:
    default:
        kv[1] = "db";
        break;
    }
    return kv;
}

static void
w_volumebar_free_serialized_keyvalues (ddb_gtkui_widget_t *w, char const **keyvalues) {
    free (keyvalues);
}

ddb_gtkui_widget_t *
w_volumebar_create (void) {
    w_volumebar_t *w = malloc (sizeof (w_volumebar_t));
    memset (w, 0, sizeof (w_volumebar_t));
    w->base.widget = gtk_event_box_new ();
    w->base.message = w_volumebar_message;
    w->base.initmenu = w_volumebar_initmenu;
    w->exapi._size = sizeof (ddb_gtkui_widget_extended_api_t);
    w->exapi.deserialize_from_keyvalues = w_volumebar_deserialize_from_keyvalues;
    w->exapi.serialize_to_keyvalues = w_volumebar_serialize_to_keyvalues;
    w->exapi.free_serialized_keyvalues = w_volumebar_free_serialized_keyvalues;

    w->volumebar = ddb_volumebar_new ();
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_set_events (GTK_WIDGET (w->base.widget), gtk_widget_get_events (GTK_WIDGET (w->base.widget)) | GDK_SCROLL_MASK);
#endif
    ddb_volumebar_init_signals (DDB_VOLUMEBAR (w->volumebar), w->base.widget);
    g_signal_connect ((gpointer)w->base.widget, "button_press_event", G_CALLBACK (on_volumebar_evbox_button_press_event), w);
    gtk_widget_show (w->volumebar);
    gtk_widget_set_size_request (w->base.widget, 70, -1);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->volumebar);
    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
