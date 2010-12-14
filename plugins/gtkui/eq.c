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

ddb_dsp_context_t *
get_supereq (void) {
    ddb_dsp_context_t *dsp = deadbeef->streamer_get_dsp_chain ();
    while (dsp) {
        if (!strcmp (dsp->plugin->plugin.id, "supereq")) {
            return dsp;
        }
        dsp = dsp->next;
    }

    return NULL;
}

static void
set_param (ddb_dsp_context_t *eq, int i, float v) {
    eq->plugin->set_param (eq, i, v);
    if (i == 0) {
        deadbeef->conf_set_float ("eq.preamp", v);
    }
    else {
        char s[100];
        snprintf (s, sizeof (s), "eq.band%d", i-1);
        deadbeef->conf_set_float (s, v);
    }
}

void
eq_value_changed (DdbEqualizer *widget)
{
    ddb_dsp_context_t *eq = get_supereq ();
    if (eq) {
        for (int i = 0; i < 18; i++) {
            set_param (eq, i+1, db_to_amp (ddb_equalizer_get_band (widget, i)));
        }
        set_param (eq, 0, db_to_amp (ddb_equalizer_get_preamp (widget)));
    }
}

void
on_enable_toggled         (GtkToggleButton *togglebutton,
        gpointer         user_data) {
    ddb_dsp_context_t *eq = get_supereq ();
    if (eq) {
        int enabled = gtk_toggle_button_get_active (togglebutton) ? 1 : 0;
        eq->plugin->enable (eq, enabled);
        deadbeef->conf_set_int ("eq.enable", enabled);
    }
}

void
on_zero_all_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    if (eqwin) {
        ddb_dsp_context_t *eq = get_supereq ();
        if (eq) {
            ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), 0);
            set_param (eq, 0, 1);
            for (int i = 0; i < 18; i++) {
                // set gui
                ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, 0);

                // set dsp
                set_param (eq, i+1, 1);
            }
            gdk_window_invalidate_rect (eqwin->window, NULL, FALSE);
        }
    }
}

void
on_zero_preamp_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    if (eqwin) {
        ddb_dsp_context_t *eq = get_supereq ();
        if (eq) {
            set_param (eq, 0, 1);
            ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), 0);
            gdk_window_invalidate_rect (eqwin->window, NULL, FALSE);
        }
    }
}

void
on_zero_bands_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    if (eqwin) {
        ddb_dsp_context_t *eq = get_supereq ();
        if (eq) {
            for (int i = 0; i < 18; i++) {
                ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, 0);
                set_param (eq, i+1, 1);
            }
            gdk_window_invalidate_rect (eqwin->window, NULL, FALSE);
        }
    }
}

void
on_save_preset_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Save DeaDBeeF EQ Preset"), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg), TRUE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "untitled.ddbeq");

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("DeaDBeeF EQ preset files (*.ddbeq)"));
    gtk_file_filter_add_pattern (flt, "*.ddbeq");

    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);

        if (fname) {
            FILE *fp = fopen (fname, "w+b");
            if (fp) {
                ddb_dsp_context_t *eq = get_supereq ();
                if (eq) {
                    for (int i = 0; i < 18; i++) {
                        fprintf (fp, "%f\n", amp_to_db (eq->plugin->get_param (eq, i+1)));
                    }
                    fprintf (fp, "%f\n", amp_to_db (eq->plugin->get_param (eq, 0)));
                }
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
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Load DeaDBeeF EQ Preset..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("DeaDBeeF EQ presets (*.ddbeq)"));
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
                    ddb_dsp_context_t *eq = get_supereq ();
                    if (eq) {
                        set_param (eq, 0, db_to_amp (vals[18]));
                        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), vals[18]);
                        for (int i = 0; i < 18; i++) {
                            ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, vals[i]);
                            set_param (eq, i+1, db_to_amp (vals[i]));
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
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Import Foobar2000 EQ Preset..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("Foobar2000 EQ presets (*.feq)"));
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
                    ddb_dsp_context_t *eq = get_supereq ();
                    if (eq) {
                        set_param (eq, 0, 1);
                        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), 0);
                        for (int i = 0; i < 18; i++) {
                            ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, vals[i]);
                            set_param (eq, i+1, db_to_amp (vals[i]));
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
    if (!eqcont) {
        eqcont = gtk_vbox_new (FALSE, 8);
        GtkWidget *parent= lookup_widget (mainwin, "plugins_bottom_vbox");
        gtk_box_pack_start (GTK_BOX (parent), eqcont, FALSE, FALSE, 0);

        GtkWidget *buttons = gtk_hbox_new (FALSE, 8);
        gtk_container_set_border_width (GTK_CONTAINER (buttons), 3);
        gtk_widget_show (buttons);
        gtk_box_pack_start (GTK_BOX (eqcont), buttons, FALSE, FALSE, 0);

        GtkWidget *button;

        eqenablebtn = button = gtk_check_button_new_with_label (_("Enable"));
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        ddb_dsp_context_t *eq = get_supereq ();
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (eqenablebtn), eq ? eq->enabled : 0);
        g_signal_connect ((gpointer) button, "toggled",
                G_CALLBACK (on_enable_toggled),
                NULL);

        button = gtk_button_new_with_label (_("Zero All"));
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_zero_all_clicked),
                NULL);

        button = gtk_button_new_with_label (_("Zero Preamp"));
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_zero_preamp_clicked),
                NULL);

        button = gtk_button_new_with_label (_("Zero Bands"));
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_zero_bands_clicked),
                NULL);

        button = gtk_button_new_with_label (_("Save Preset"));
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_save_preset_clicked),
                NULL);

        button = gtk_button_new_with_label (_("Load Preset"));
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_load_preset_clicked),
                NULL);

        button = gtk_button_new_with_label (_("Import Foobar2000 Preset"));
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_import_fb2k_preset_clicked),
                NULL);

        eqwin = GTK_WIDGET (ddb_equalizer_new());
        g_signal_connect (eqwin, "on_changed", G_CALLBACK (eq_value_changed), 0);
        gtk_widget_set_size_request (eqwin, -1, 200);

        if (eq) {
            ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), amp_to_db (eq->plugin->get_param (eq, 0)));
            for (int i = 0; i < 18; i++) {
                if (eq) {
                    float val = eq->plugin->get_param (eq, i+1);
                    ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, amp_to_db (val));
                }
            }
        }

        gtk_widget_show (eqwin);
        gtk_box_pack_start (GTK_BOX (eqcont), eqwin, TRUE, TRUE, 0);
    }
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

