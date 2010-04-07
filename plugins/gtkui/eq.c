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
#include "ddbequalizer.h"

static GtkWidget *eqwin;

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
graphic_value_changed (DdbEqualizer *widget)
{
    printf ("graphic_value_changed\n");
    DB_supereq_dsp_t *eq = NULL;
    DB_dsp_t **plugs = deadbeef->plug_get_dsp_list ();

    for (int i = 0; plugs[i]; i++) {
        if (plugs[i]->plugin.id && !strcmp (plugs[i]->plugin.id, "supereq")) {
            eq = (DB_supereq_dsp_t *)plugs[i];
            for (int ii = 0; ii < 18; ii++) {
                eq->set_band (ii, db_to_amp (ddb_equalizer_get_band (widget, ii)));
            }
            eq->set_preamp (db_to_amp (ddb_equalizer_get_preamp (widget)));
            break;
        }
    }
}

void
eq_window_show (void) {
    if (!eqwin) {
        eqwin = GTK_WIDGET (ddb_equalizer_new());
        g_signal_connect (eqwin, "on_changed", G_CALLBACK (graphic_value_changed), 0);
        gtk_widget_set_size_request (eqwin, -1, 200);

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

        printf ("init eq widget\n");
        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), amp_to_db (eq->get_preamp ()));
        for (i = 0; i < 18; i++) {
            if (eq) {
                float val = eq->get_band (i);
                ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, amp_to_db (val));
            }
        }

        gtk_widget_show (eqwin);

        GtkWidget *cont = lookup_widget (mainwin, "plugins_bottom_vbox");
        gtk_box_pack_start (GTK_BOX (cont), eqwin, FALSE, FALSE, 0);
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
}
