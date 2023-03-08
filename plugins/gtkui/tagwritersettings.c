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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include "../../gettext.h"
#include "interface.h"
#include "support.h"
#include <deadbeef/deadbeef.h>
#include "gtkui.h"

void
run_tagwriter_settings (GtkWidget *parentwindow) {
    GtkWidget *dlg = create_tagwritersettings ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (parentwindow));

    // tag writer
    int strip_id3v2 = deadbeef->conf_get_int ("mp3.strip_id3v2", 0);
    int strip_id3v1 = deadbeef->conf_get_int ("mp3.strip_id3v1", 0);
    int strip_apev2 = deadbeef->conf_get_int ("mp3.strip_apev2", 0);
    int write_id3v2 = deadbeef->conf_get_int ("mp3.write_id3v2", 1);
    int write_id3v1 = deadbeef->conf_get_int ("mp3.write_id3v1", 1);
    int write_apev2 = deadbeef->conf_get_int ("mp3.write_apev2", 0);
    int id3v2_version = deadbeef->conf_get_int ("mp3.id3v2_version", 3);
    char id3v1_encoding[50];
    deadbeef->conf_get_str ("mp3.id3v1_encoding", "iso8859-1", id3v1_encoding, sizeof (id3v1_encoding));
    int ape_strip_id3v2 = deadbeef->conf_get_int ("ape.strip_id3v2", 0);
    int ape_strip_apev2 = deadbeef->conf_get_int ("ape.strip_apev2", 0);
    int ape_write_id3v2 = deadbeef->conf_get_int ("ape.write_id3v2", 0);
    int ape_write_apev2 = deadbeef->conf_get_int ("ape.write_apev2", 1);
    int wv_strip_apev2 = deadbeef->conf_get_int ("wv.strip_apev2", 0);
    int wv_strip_id3v1 = deadbeef->conf_get_int ("wv.strip_id3v1", 0);
    int wv_write_apev2 = deadbeef->conf_get_int ("wv.write_apev2", 1);
    int wv_write_id3v1 = deadbeef->conf_get_int ("wv.write_id3v1", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "strip_id3v2")), strip_id3v2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "strip_id3v1")), strip_id3v1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "strip_apev2")), strip_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "write_id3v2")), write_id3v2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "write_id3v1")), write_id3v1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "write_apev2")), write_apev2);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id3v2_version")), id3v2_version != 4 ? 0 : 1);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "id3v1_encoding")), id3v1_encoding);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "ape_strip_id3v2")), ape_strip_id3v2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "ape_strip_apev2")), ape_strip_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "ape_write_apev2")), ape_write_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "ape_write_id3v2")), ape_write_id3v2);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "wv_strip_id3v1")), wv_strip_id3v1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "wv_strip_apev2")), wv_strip_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "wv_write_apev2")), wv_write_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "wv_write_id3v1")), wv_write_id3v1);


    (void)gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
}

void
on_write_id3v2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.write_id3v2", gtk_toggle_button_get_active (togglebutton));
}


void
on_write_id3v1_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.write_id3v1", gtk_toggle_button_get_active (togglebutton));
}


void
on_write_apev2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.write_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_strip_id3v2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.strip_id3v2", gtk_toggle_button_get_active (togglebutton));
}


void
on_strip_id3v1_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.strip_id3v1", gtk_toggle_button_get_active (togglebutton));
}


void
on_strip_apev2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.strip_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_id3v2_version_changed               (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int version = 3;
    int active = gtk_combo_box_get_active (combobox);
    if (active == 1) {
        version = 4;
    }
    deadbeef->conf_set_int ("mp3.id3v2_version", version);
}


void
on_id3v1_encoding_changed              (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("mp3.id3v1_encoding", gtk_entry_get_text (GTK_ENTRY (editable)));
}


void
on_ape_write_id3v2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("ape.write_id3v2", gtk_toggle_button_get_active (togglebutton));
}


void
on_ape_write_apev2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("ape.write_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_ape_strip_id3v2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("ape.strip_id3v2", gtk_toggle_button_get_active (togglebutton));
}


void
on_ape_strip_apev2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("ape.strip_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_wv_write_apev2_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("wv.write_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_wv_write_id3v1_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("wv.write_id3v1", gtk_toggle_button_get_active (togglebutton));
}

void
on_wv_strip_apev2_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("wv.strip_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_wv_strip_id3v1_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("wv.strip_id3v1", gtk_toggle_button_get_active (togglebutton));
}
