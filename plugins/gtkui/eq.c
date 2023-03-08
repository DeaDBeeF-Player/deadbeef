/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "gtkui.h"
#include "support.h"
#include "ddbequalizer.h"
#include "../../src/shared/eqpreset.h"

static GtkWidget *eqcont;
static GtkWidget *eqwin;
static GtkWidget *eqenablebtn;

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
    char fv[100];
    snprintf (fv, sizeof (fv), "%f", v);
    eq->plugin->set_param (eq, i, fv);
}

void
eq_value_changed (DdbEqualizer *widget)
{
    ddb_dsp_context_t *eq = get_supereq ();
    if (eq) {
        for (int i = 0; i < 18; i++) {
            set_param (eq, i+1, ddb_equalizer_get_band (widget, i));
        }
        set_param (eq, 0, ddb_equalizer_get_preamp (widget));
        deadbeef->streamer_dsp_chain_save ();
    }
}

void
on_enable_toggled         (GtkToggleButton *togglebutton,
        gpointer         user_data) {
    ddb_dsp_context_t *eq = get_supereq ();
    if (eq) {
        int enabled = gtk_toggle_button_get_active (togglebutton) ? 1 : 0;
        eq->enabled =  enabled;
        deadbeef->streamer_dsp_refresh ();
        deadbeef->streamer_dsp_chain_save ();
    }
}

void
on_zero_all_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    if (eqwin) {
        ddb_dsp_context_t *eq = get_supereq ();
        if (eq) {
            ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), 0);
            set_param (eq, 0, 0);
            for (int i = 0; i < 18; i++) {
                // set gui
                ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, 0);

                // set dsp
                set_param (eq, i+1, 0);
            }
            gtk_widget_queue_draw (eqwin);
            deadbeef->streamer_dsp_chain_save ();
        }
    }
}

void
on_zero_preamp_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    if (eqwin) {
        ddb_dsp_context_t *eq = get_supereq ();
        if (eq) {
            set_param (eq, 0, 0);
            ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), 0);
            gtk_widget_queue_draw (eqwin);
            deadbeef->streamer_dsp_chain_save ();
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
                set_param (eq, i+1, 0);
            }
            gtk_widget_queue_draw (eqwin);
            deadbeef->streamer_dsp_chain_save ();
        }
    }
}

void
on_save_preset_clicked                  (GtkMenuItem       *menuitem,
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
            eq_preset_save (fname);
            g_free (fname);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

static void
eq_preset_apply (float preamp, float values[18]) {
    // apply and save config
    ddb_dsp_context_t *eq = get_supereq ();
    if (!eq) {
        return;
    }
    set_param (eq, 0, preamp);
    ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), preamp);
    for (int i = 0; i < 18; i++) {
        ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, values[i]);
        set_param (eq, i+1, values[i]);
    }
    gtk_widget_queue_draw (eqwin);
    deadbeef->streamer_dsp_chain_save ();
}

void
on_load_preset_clicked                  (GtkMenuItem       *menuitem,
        gpointer         user_data) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Load DeaDBeeF EQ Preset..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, _("DeaDBeeF EQ presets (*.ddbeq)"));
    gtk_file_filter_add_pattern (flt, "*.ddbeq");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), FALSE);
    // restore folder
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast ("filechooser.lastdir", ""));
    deadbeef->conf_unlock ();
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
            float preamp;
            float values[18];
            if (!eq_preset_load (fname, &preamp, values)) {
                eq_preset_apply(preamp, values);
            }
            else {
                fprintf (stderr, "[eq] corrupted DeaDBeeF preset file, discarded\n");
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
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast ("filechooser.lastdir", ""));
    deadbeef->conf_unlock ();

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
            float values[18];
            if (!eq_preset_load_fb2k (fname, values)) {
                eq_preset_apply (0, values);
            }
            else {
                fprintf (stderr, "[eq] corrupted Foobar2000 preset file, discarded\n");
            }

            g_free (fname);
        }
    }
    gtk_widget_destroy (dlg);
}

void
on_presets_clicked                  (GtkButton       *button,
        gpointer         user_data) {
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *menuitem;

    menuitem = gtk_menu_item_new_with_mnemonic (_("Save Preset"));
    gtk_widget_show (menuitem);
    gtk_container_add (GTK_CONTAINER (menu), menuitem);

    g_signal_connect ((gpointer) menuitem, "activate",
            G_CALLBACK (on_save_preset_clicked),
            NULL);

    menuitem = gtk_menu_item_new_with_mnemonic (_("Load Preset"));
    gtk_widget_show (menuitem);
    gtk_container_add (GTK_CONTAINER (menu), menuitem);

    g_signal_connect ((gpointer) menuitem, "activate",
            G_CALLBACK (on_load_preset_clicked),
            NULL);

    menuitem = gtk_menu_item_new_with_mnemonic (_("Import Foobar2000 Preset"));
    gtk_widget_show (menuitem);
    gtk_container_add (GTK_CONTAINER (menu), menuitem);

    g_signal_connect ((gpointer) menuitem, "activate",
            G_CALLBACK (on_import_fb2k_preset_clicked),
            NULL);

    gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (button), NULL);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
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

        button = gtk_button_new_with_label (_("Presets"));
        gtk_widget_show (button);
        gtk_box_pack_start (GTK_BOX (buttons), button, FALSE, FALSE, 0);
        g_signal_connect ((gpointer) button, "clicked",
                G_CALLBACK (on_presets_clicked),
                NULL);

        eqwin = GTK_WIDGET (ddb_equalizer_new());
        g_signal_connect (eqwin, "on_changed", G_CALLBACK (eq_value_changed), 0);
        gtk_widget_set_size_request (eqwin, -1, 200);

        if (eq) {
            char fv[100];
            float v;
            eq->plugin->get_param (eq, 0, fv, sizeof (fv));
            v = atof (fv);
            ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), v);
            for (int i = 0; i < 18; i++) {
                if (eq) {
                    eq->plugin->get_param (eq, i+1, fv, sizeof (fv));
                    v = atof (fv);
                    ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, v);
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

void
eq_refresh (void) {
    ddb_dsp_context_t *eq = get_supereq ();
    if (eq && eqwin) {
        char s[20];
        eq->plugin->get_param (eq, 0, s, sizeof (s));
        ddb_equalizer_set_preamp (DDB_EQUALIZER (eqwin), atof(s));
        for (int i = 0; i < 18; i++) {
            eq->plugin->get_param (eq, i+1, s, sizeof (s));
            ddb_equalizer_set_band (DDB_EQUALIZER (eqwin), i, atoi(s));
        }
        eq_redraw ();
    }
}
