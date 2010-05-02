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
#include <stdlib.h>
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

DB_supereq_dsp_t *
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
on_zero_preamp_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    if (eqwin) {
        DB_supereq_dsp_t *eq = get_supereq_plugin ();
        eq->set_preamp (1);
        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), 0);
        gdk_window_invalidate_rect (eqwin->window, NULL, FALSE);
    }
}

void
on_zero_bands_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    if (eqwin) {
        DB_supereq_dsp_t *eq = get_supereq_plugin ();
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
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Save DeaDBeeF EQ Preset", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg), TRUE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "untitled.ddbeq");

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "DeaDBeeF preset files (*.ddbeq)");
    gtk_file_filter_add_pattern (flt, "*.ddbeq");

    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);

        if (fname) {
            FILE *fp = fopen (fname, "w+b");
            if (fp) {
                DB_supereq_dsp_t *eq = get_supereq_plugin ();
                for (int i = 0; i < 18; i++) {
                    fprintf (fp, "%f\n", amp_to_db (eq->get_band (i)));
                }
                fprintf (fp, "%f\n", amp_to_db (eq->get_preamp ()));
                fclose (fp);
            }
            g_free (fname);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

void
on_load_preset_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Load DeaDBeeF EQ Preset...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "DeaDBeeF  EQ presets (*.ddbeq)");
    gtk_file_filter_add_pattern (flt, "*.ddbeq");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), FALSE);
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
    }
    if (response == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        if (fname) {
            FILE *fp = fopen (fname, "rt");
            if (fp) {
                float vals[19]; // float dBs
                int i = 0;
                while (i < 19) {
                    char tmp[20];
                    char *out = fgets (tmp, sizeof (tmp), fp);
                    if (!out) {
                        break;
                    }
                    vals[i] = atof (tmp);
                    i++;
                }
                fclose (fp);
                if (i == 19) {
                    // apply and save config
                    DB_supereq_dsp_t *eq = get_supereq_plugin ();
                    if (eq) {
                        eq->set_preamp (db_to_amp (vals[18]));
                        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), vals[18]);
                        for (int i = 0; i < 18; i++) {
                            ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, vals[i]);
                            eq->set_band (i, db_to_amp (vals[i]));
                        }
                        gdk_window_invalidate_rect (eqwin->window, NULL, FALSE);
                        deadbeef->conf_save ();
                    }
                }
                else {
                    fprintf (stderr, "[eq] corrupted DeaDBeeF preset file, discarded\n");
                }
            }
            g_free (fname);
        }
    }
    gtk_widget_destroy (dlg);
}

void
on_import_fb2k_preset_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Import Foobar2000 EQ Preset...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Foobar2000 EQ presets (*.feq)");
    gtk_file_filter_add_pattern (flt, "*.feq");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), FALSE);
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
    }
    if (response == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        if (fname) {
            FILE *fp = fopen (fname, "rt");
            if (fp) {
                int vals[18]; // integer dBs
                int i = 0;
                while (i < 18) {
                    char tmp[20];
                    char *out = fgets (tmp, sizeof (tmp), fp);
                    if (!out) {
                        break;
                    }
                    vals[i] = atoi (tmp);
                    i++;
                }
                fclose (fp);
                if (i == 18) {
                    // apply and save config
                    DB_supereq_dsp_t *eq = get_supereq_plugin ();
                    if (eq) {
                        eq->set_preamp (1);
                        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), 0);
                        for (int i = 0; i < 18; i++) {
                            ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, vals[i]);
                            eq->set_band (i, db_to_amp (vals[i]));
                        }
                        gdk_window_invalidate_rect (eqwin->window, NULL, FALSE);
                        deadbeef->conf_save ();
                    }
                }
                else {
                    fprintf (stderr, "[eq] corrupted Foobar2000 preset file, discarded\n");
                }
            }
            g_free (fname);
        }
    }
    gtk_widget_destroy (dlg);
}

void
eq_window_show (void) {
    DB_supereq_dsp_t *eq = get_supereq_plugin ();
    if (!eq) {
        return;
    }
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

        button = gtk_button_new_with_label ("Zero Preamp");
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_zero_preamp_clicked),
                NULL);

        button = gtk_button_new_with_label ("Zero Bands");
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_zero_bands_clicked),
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

        button = gtk_button_new_with_label ("Import Foobar2000 Preset");
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_import_fb2k_preset_clicked),
                NULL);

        eqwin = GTK_WIDGET (ddb_equalizer_new());
        g_signal_connect (eqwin, "on_changed", G_CALLBACK (eq_value_changed), 0);
        gtk_widget_set_size_request (eqwin, -1, 200);


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
    if (eqwin) {
        gtk_widget_destroy (eqwin);
        eqwin = NULL;
    }
}

void
eq_redraw (void) {
    if (eqwin) {
        gtk_widget_queue_draw (eqwin);
    }
}

