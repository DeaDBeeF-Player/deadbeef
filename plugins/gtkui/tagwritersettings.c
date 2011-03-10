/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include "../../gettext.h"
#include "interface.h"
#include "support.h"
#include "../../deadbeef.h"
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
    const char *id3v1_encoding = deadbeef->conf_get_str ("mp3.id3v1_encoding", "iso8859-1");
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


    int response = gtk_dialog_run (GTK_DIALOG (dlg));
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
