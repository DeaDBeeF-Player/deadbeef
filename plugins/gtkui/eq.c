/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <gtk/gtk.h>
#include <string.h>
#include <math.h>
#include "gtkui.h"
#include "support.h"
#include "../supereq/supereq.h"

static GtkWidget *eqwin;
static GtkWidget *sliders[18];

float
db_to_amp (float dB) {
    const float ln10=2.3025850929940002f;
    return exp(ln10*dB/20.f);
}

float
amp_to_db (float amp) {
    return 20*log10 (amp);
}

void
eq_value_changed (GtkRange *range, gpointer  user_data) {
    int band = (intptr_t)user_data;
    DB_supereq_dsp_t *eq = NULL;
    DB_dsp_t **plugs = deadbeef->plug_get_dsp_list ();
    // find eq plugin
    for (int i = 0; plugs[i]; i++) {
        if (plugs[i]->plugin.id && !strcmp (plugs[i]->plugin.id, "supereq")) {
            eq = (DB_supereq_dsp_t *)plugs[i];
            float val = gtk_range_get_value (range);
            val = db_to_amp (val);
            eq->set_band (band, val);
            break;
        }
    }
}

void
eq_init_widgets (GtkWidget *container) {
    int i;
    GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (container), hbox);

    for (i = 0; i < 18; i++) {
        GtkWidget *scale = sliders[i] = gtk_vscale_new_with_range (-20, 20, 1);
        gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_BOTTOM);
        gtk_scale_set_draw_value (GTK_SCALE (scale), TRUE);
        gtk_range_set_inverted (GTK_RANGE (scale), TRUE);
        gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
        g_signal_connect (scale, "value_changed", G_CALLBACK (eq_value_changed), (gpointer)(intptr_t)i);
        gtk_widget_show (scale);
        gtk_box_pack_start (GTK_BOX (hbox), scale, FALSE, FALSE, 0);
    }
}

void
eq_window_show (void) {
    if (!eqwin) {
        eqwin = gtk_hbox_new (FALSE, 0);
        gtk_widget_set_size_request (eqwin, -1, 200);
        eq_init_widgets (eqwin);
        GtkWidget *cont = lookup_widget (mainwin, "vbox1");
        gtk_box_pack_start (GTK_BOX (cont), eqwin, FALSE, FALSE, 0);
    }
    int i;
    DB_supereq_dsp_t *eq = NULL;
    DB_dsp_t **plugs = deadbeef->plug_get_dsp_list ();
    // find eq plugin
    for (i = 0; plugs[i]; i++) {
        if (plugs[i]->plugin.id && !strcmp (plugs[i]->plugin.id, "supereq")) {
            eq = (DB_supereq_dsp_t *)plugs[i];
            break;
        }
    }

    for (i = 0; i < 18; i++) {
        GtkWidget *scale = sliders[i];
        if (eq) {
            float val = eq->get_band (i);
            val = amp_to_db (val);
            if (val < -20) {
                val = -20;
            }
            if (val > 20) {
                val = 20;
            }
            gtk_range_set_value (GTK_RANGE (scale), val);
        }
    }
    gtk_widget_show (eqwin);
}

void
eq_window_hide (void) {
    if (eqwin) {
        gtk_widget_hide (eqwin);
    }
}

void
eq_window_destroy (void) {
    gtk_widget_destroy (eqwin);
    eqwin = NULL;
    memset (sliders, 0, sizeof (sliders));
}
