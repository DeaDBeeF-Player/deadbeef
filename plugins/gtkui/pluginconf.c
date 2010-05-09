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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "../../deadbeef.h"
#include "gtkui.h"
#include "parser.h"
#include "support.h"

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

extern GtkWidget *mainwin;

void
on_prop_browse_file (GtkButton *button, gpointer user_data) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Open file...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), FALSE);
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
        deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    }
    if (response == GTK_RESPONSE_OK) {
        gchar *file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (file) {
            GtkWidget *entry = GTK_WIDGET (user_data);
            gtk_entry_set_text (GTK_ENTRY (entry), file);
            g_free (file);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}

static void apply_conf (GtkWidget *w, DB_plugin_t *p) {
    // parse script
    char token[MAX_TOKEN];
    const char *script = p->configdialog;
    parser_line = 1;
    while (script = gettoken (script, token)) {
        if (strcmp (token, "property")) {
            fprintf (stderr, "invalid token while loading plugin %s config dialog: %s at line %d\n", p->name, token, parser_line);
            break;
        }
        char labeltext[MAX_TOKEN];
        script = gettoken_warn_eof (script, labeltext);
        if (!script) {
            break;
        }
        char type[MAX_TOKEN];
        script = gettoken_warn_eof (script, type);
        if (!script) {
            break;
        }
        char key[MAX_TOKEN];
        script = gettoken_warn_eof (script, key);
        if (!script) {
            break;
        }
        char def[MAX_TOKEN];
        script = gettoken_warn_eof (script, def);
        if (!script) {
            break;
        }
        script = gettoken_warn_eof (script, token);
        if (!script) {
            break;
        }
        if (strcmp (token, ";")) {
            fprintf (stderr, "expected `;' while loading plugin %s config dialog: %s at line %d\n", p->name, token, parser_line);
            break;
        }

        // fetch data
        GtkWidget *widget = lookup_widget (w, key);
        if (widget) {
            if (!strcmp (type, "entry") || !strcmp (type, "password")) {
                deadbeef->conf_set_str (key, gtk_entry_get_text (GTK_ENTRY (widget)));
            }
            else if (!strcmp (type, "file")) {
                if (deadbeef->conf_get_int ("gtkui.pluginconf.use_filechooser_button", 0)) {
                    // filechooser
                    deadbeef->conf_set_str (key, gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget)));
                }
                else {
                    deadbeef->conf_set_str (key, gtk_entry_get_text (GTK_ENTRY (widget)));
                }
            }
            else if (!strcmp (type, "checkbox")) {
                deadbeef->conf_set_int (key, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
            }
        }
    }
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}

static void
prop_changed (GtkWidget *editable, gpointer user_data) {
    gtk_dialog_set_response_sensitive (GTK_DIALOG (user_data), GTK_RESPONSE_APPLY, TRUE);
}

void
plugin_configure (GtkWidget *parentwin, DB_plugin_t *p) {
    // create window
    char title[200];
    snprintf (title, sizeof (title), "Setup %s", p->name);
    GtkWidget *win = gtk_dialog_new_with_buttons (title, GTK_WINDOW (parentwin), GTK_DIALOG_MODAL, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (win), GTK_RESPONSE_OK);
    gtk_window_set_type_hint (GTK_WINDOW (win), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_container_set_border_width (GTK_CONTAINER(win), 12);

    gtk_window_set_title (GTK_WINDOW (win), title);
    gtk_window_set_modal (GTK_WINDOW (win), TRUE);
    gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (parentwin));
    GtkWidget *vbox;
    vbox = GTK_DIALOG (win)->vbox;
    gtk_box_set_spacing (GTK_BOX (vbox), 8);
    GtkWidget *action_area = GTK_DIALOG (win)->action_area;
    gtk_widget_show (action_area);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);


    // parse script
    char token[MAX_TOKEN];
    const char *script = p->configdialog;
    parser_line = 1;
    while (script = gettoken (script, token)) {
        if (strcmp (token, "property")) {
            fprintf (stderr, "invalid token while loading plugin %s config dialog: %s at line %d\n", p->name, token, parser_line);
            break;
        }
        char labeltext[MAX_TOKEN];
        script = gettoken_warn_eof (script, labeltext);
        if (!script) {
            break;
        }
        char type[MAX_TOKEN];
        script = gettoken_warn_eof (script, type);
        if (!script) {
            break;
        }
        char key[MAX_TOKEN];
        script = gettoken_warn_eof (script, key);
        if (!script) {
            break;
        }
        char def[MAX_TOKEN];
        script = gettoken_warn_eof (script, def);
        if (!script) {
            break;
        }
        script = gettoken_warn_eof (script, token);
        if (!script) {
            break;
        }
        if (strcmp (token, ";")) {
            fprintf (stderr, "expected `;' while loading plugin %s config dialog: %s at line %d\n", p->name, token, parser_line);
            break;
        }

        // add to dialog
        GtkWidget *label = NULL;
        GtkWidget *prop = NULL;
        GtkWidget *cont = NULL;
        if (!strcmp (type, "entry") || !strcmp (type, "password")) {
            label = gtk_label_new (labeltext);
            gtk_widget_show (label);
            prop = gtk_entry_new ();
            gtk_entry_set_activates_default (GTK_ENTRY (prop), TRUE);
            g_signal_connect (G_OBJECT (prop), "changed", G_CALLBACK (prop_changed), win);
            gtk_widget_show (prop);
            gtk_entry_set_text (GTK_ENTRY (prop), deadbeef->conf_get_str (key, def));
        }
        else if (!strcmp (type, "checkbox")) {
            prop = gtk_check_button_new_with_label (labeltext);
            g_signal_connect (G_OBJECT (prop), "toggled", G_CALLBACK (prop_changed), win);
            gtk_widget_show (prop);
            int val = deadbeef->conf_get_int (key, atoi (def));
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop), val);
        }
        else if (!strcmp (type, "file")) {
            label = gtk_label_new (labeltext);
            gtk_widget_show (label);
            if (deadbeef->conf_get_int ("gtkui.pluginconf.use_filechooser_button", 0)) {
                prop = gtk_file_chooser_button_new (labeltext, GTK_FILE_CHOOSER_ACTION_OPEN);
                gtk_widget_show (prop);
                gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (prop), deadbeef->conf_get_str (key, def));
                g_signal_connect (G_OBJECT (prop), "file-set", G_CALLBACK (prop_changed), win);
            }
            else {
                cont = gtk_hbox_new (FALSE, 2);
                gtk_widget_show (cont);
                prop = gtk_entry_new ();
                gtk_entry_set_activates_default (GTK_ENTRY (prop), TRUE);
                g_signal_connect (G_OBJECT (prop), "changed", G_CALLBACK (prop_changed), win);
                gtk_widget_show (prop);
                gtk_editable_set_editable (GTK_EDITABLE (prop), FALSE);
                gtk_entry_set_text (GTK_ENTRY (prop), deadbeef->conf_get_str (key, def));
                gtk_box_pack_start (GTK_BOX (cont), prop, TRUE, TRUE, 0);
                GtkWidget *btn = gtk_button_new_with_label ("…");
                gtk_widget_show (btn);
                gtk_box_pack_start (GTK_BOX (cont), btn, FALSE, FALSE, 0);
                g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (on_prop_browse_file), prop);
            }
        }
        if (!strcmp (type, "password")) {
            gtk_entry_set_visibility (GTK_ENTRY (prop), FALSE);
        }
        if (label && prop) {
            GtkWidget *hbox = NULL;
            hbox = gtk_hbox_new (FALSE, 8);
            gtk_widget_show (hbox);
            gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), cont ? cont : prop, TRUE, TRUE, 0);
            cont = hbox;
        }
        else {
            cont = prop;
        }
        if (prop) {
            g_object_set_data (G_OBJECT (win), key, prop);
        }
        if (cont) {
            gtk_box_pack_start (GTK_BOX (vbox), cont, FALSE, FALSE, 0);
        }
    }

    int response;
    do {
        gtk_dialog_set_response_sensitive (GTK_DIALOG (win), GTK_RESPONSE_APPLY, FALSE);
        response = gtk_dialog_run (GTK_DIALOG (win));
        if (response == GTK_RESPONSE_APPLY || response == GTK_RESPONSE_OK) {
            apply_conf (win, p);
        }
    } while (response == GTK_RESPONSE_APPLY);
    gtk_widget_destroy (win);
}


