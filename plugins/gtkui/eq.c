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

static GtkWidget *eqcont;
static GtkWidget *eqwin;
static GtkWidget *eqenablebtn;

static inline float
db_to_amp (float dB) {
    const float ln10=2.3025850929940002f;
    return exp(ln10*dB/20.f);
}

static inline float
amp_to_db (float amp) {
    return 20*log10 (amp);
}

static DB_supereq_dsp_t *
get_supereq_plugin (void) {
    DB_dsp_t **plugs = deadbeef->plug_get_dsp_list ();
    for (int i = 0; plugs[i]; i++) {
        if (plugs[i]->plugin.id && !strcmp (plugs[i]->plugin.id, "supereq")) {
            return (DB_supereq_dsp_t *)plugs[i];
        }
    }
    return NULL;
}

void
eq_value_changed (DdbEqualizer *widget)
{
    DB_supereq_dsp_t *eq = get_supereq_plugin ();
    for (int i = 0; i < 18; i++) {
        eq->set_band (i, db_to_amp (ddb_equalizer_get_band (widget, i)));
    }
    eq->set_preamp (db_to_amp (ddb_equalizer_get_preamp (widget)));
}

void
on_enable_toggled         (GtkToggleButton *togglebutton,
        gpointer         user_data) {
    DB_supereq_dsp_t *eq = get_supereq_plugin ();
    eq->dsp.enable (gtk_toggle_button_get_active (togglebutton));
}

void
on_zero_all_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    if (eqwin) {
        DB_supereq_dsp_t *eq = get_supereq_plugin ();
        eq->set_preamp (1);
        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), 0);
        for (int i = 0; i < 18; i++) {
            ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, 0);
            eq->set_band (i, 1);
        }
        gdk_window_invalidate_rect (eqwin->window, NULL, FALSE);
    }
}

void
on_save_preset_clicked                  (GtkButton       *button,
        gpointer         user_data) {
}

void
on_load_preset_clicked                  (GtkButton       *button,
        gpointer         user_data) {
}

void
eq_window_show (void) {
    if (!eqcont) {
        eqcont = gtk_vbox_new (FALSE, 8);
        GtkWidget *parent= lookup_widget (mainwin, "plugins_bottom_vbox");
        gtk_box_pack_start (GTK_BOX (parent), eqcont, FALSE, FALSE, 0);

        GtkWidget *buttons = gtk_hbox_new (FALSE, 8);
        gtk_container_set_border_width (GTK_CONTAINER (buttons), 3);
        gtk_widget_show (buttons);
        gtk_box_pack_start (GTK_BOX (eqcont), buttons, FALSE, FALSE, 0);

        GtkWidget *button;

        eqenablebtn = button = gtk_check_button_new_with_label ("Enable");
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "toggled",
                G_CALLBACK (on_enable_toggled),
                NULL);

        button = gtk_button_new_with_label ("Zero All");
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_zero_all_clicked),
                NULL);

        button = gtk_button_new_with_label ("Save Preset");
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_save_preset_clicked),
                NULL);

        button = gtk_button_new_with_label ("Load Preset");
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_load_preset_clicked),
                NULL);

        eqwin = GTK_WIDGET (ddb_equalizer_new());
        g_signal_connect (eqwin, "on_changed", G_CALLBACK (eq_value_changed), 0);
        gtk_widget_set_size_request (eqwin, -1, 200);

        DB_supereq_dsp_t *eq = get_supereq_plugin ();

        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), amp_to_db (eq->get_preamp ()));
        for (int i = 0; i < 18; i++) {
            if (eq) {
                float val = eq->get_band (i);
                ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, amp_to_db (val));
            }
        }

        gtk_widget_show (eqwin);
        gtk_box_pack_start (GTK_BOX (eqcont), eqwin, TRUE, TRUE, 0);
    }
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (eqenablebtn), deadbeef->conf_get_int ("supereq.enable", 0));
    gtk_widget_show (eqcont);
}

void
eq_window_hide (void) {
    if (eqcont) {
        gtk_widget_hide (eqcont);
    }
}

void
eq_window_destroy (void) {
    gtk_widget_destroy (eqwin);
    eqwin = NULL;
}
