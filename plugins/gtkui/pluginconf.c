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
#include "../../gettext.h"
#include "../../deadbeef.h"
#include "gtkui.h"
#include "parser.h"
#include "support.h"
#include "pluginconf.h"

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

extern GtkWidget *mainwin;

void
on_prop_browse_file (GtkButton *button, gpointer user_data) {
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Open file..."), GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), FALSE);
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
        deadbeef->sendmessage (M_CONFIG_CHANGED, 0, 0, 0);
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

static void apply_conf (GtkWidget *w, ddb_dialog_t *conf) {
    // parse script
    char token[MAX_TOKEN];
    const char *script = conf->layout;
    parser_line = 1;
    while (script = gettoken (script, token)) {
        if (strcmp (token, "property")) {
            fprintf (stderr, "invalid token while loading plugin %s config dialog: %s at line %d\n", conf->title, token, parser_line);
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

        // skip containers
        if (!strncmp (type, "hbox[", 5) || !strncmp (type, "vbox[", 5)) {
            // skip to ;
            char semicolon[MAX_TOKEN];
            while (script = gettoken_warn_eof (script, semicolon)) {
                if (!strcmp (semicolon, ";")) {
                    break;
                }
            }
            continue;
        }

        // ignore layout options
        char key[MAX_TOKEN];
        const char *skiptokens[] = { "vert", NULL };
        for (;;) {
            script = gettoken_warn_eof (script, key);
            int i = 0;
            for (i = 0; skiptokens[i]; i++) {
                if (!strcmp (key, skiptokens[i])) {
                    break;
                }
            }
            if (!skiptokens[i]) {
                break;
            }
        }
        if (!script) {
            break;
        }
        char def[MAX_TOKEN];
        script = gettoken_warn_eof (script, def);
        if (!script) {
            break;
        }

        // fetch data
        GtkWidget *widget = lookup_widget (w, key);
        if (widget) {
            if (!strcmp (type, "entry") || !strcmp (type, "password")) {
                conf->set_param (key, gtk_entry_get_text (GTK_ENTRY (widget)));
            }
            else if (!strcmp (type, "file")) {
                if (deadbeef->conf_get_int ("gtkui.pluginconf.use_filechooser_button", 0)) {
                    // filechooser
                    conf->set_param (key, gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget)));
                }
                else {
                    conf->set_param (key, gtk_entry_get_text (GTK_ENTRY (widget)));
                }
            }
            else if (!strcmp (type, "checkbox")) {
                conf->set_param (key, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) ? "1" : "0");
            }
            else if (!strncmp (type, "hscale[", 7) || !strncmp (type, "vscale[", 7)) {
                char s[20];
                snprintf (s, sizeof (s), "%f", gtk_range_get_value (GTK_RANGE (widget)));
                conf->set_param (key, s);
            }
            else if (!strncmp (type, "spinbtn[", 8)) {
                char s[20];
                snprintf (s, sizeof (s), "%f", (float)gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget)));
                conf->set_param (key, s);
            }
            else if (!strncmp (type, "select[", 7)) {
                int n;
                if (1 != sscanf (type+6, "[%d]", &n)) {
                    break;
                }
                for (int i = 0; i < n; i++) {
                    char value[MAX_TOKEN];
                    script = gettoken_warn_eof (script, value);
                    if (!script) {
                        break;
                    }
                }
                if (!script) {
                    break;
                }
                char s[20];
                snprintf (s, sizeof (s), "%d", gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
                conf->set_param (key, s);
            }
        }

        script = gettoken_warn_eof (script, token);
        if (!script) {
            break;
        }
        if (strcmp (token, ";")) {
            fprintf (stderr, "expected `;' while loading plugin %s config dialog: %s at line %d\n", conf->title, token, parser_line);
            break;
        }
    }
    deadbeef->sendmessage (M_CONFIG_CHANGED, 0, 0, 0);
}

static void
prop_changed (GtkWidget *editable, gpointer user_data) {
    gtk_dialog_set_response_sensitive (GTK_DIALOG (user_data), GTK_RESPONSE_APPLY, TRUE);
}

int
ddb_button_from_gtk_response (int response) {
    switch (response) {
    case GTK_RESPONSE_OK:
        return ddb_button_ok;
    case GTK_RESPONSE_CANCEL:
        return ddb_button_cancel;
    case GTK_RESPONSE_CLOSE:
        return ddb_button_close;
    case GTK_RESPONSE_APPLY:
        return ddb_button_apply;
    case GTK_RESPONSE_YES:
        return ddb_button_yes;
    case GTK_RESPONSE_NO:
        return ddb_button_no;
    }
    return -1;
}

int
gtkui_run_dialog (GtkWidget *parentwin, ddb_dialog_t *conf, uint32_t buttons, int (*callback)(int button, void *ctx), void *ctx) {
    // create window
    char title[200];
    snprintf (title, sizeof (title), _("Configure %s"), conf->title);
    GtkWidget *win;
    if (!buttons) {
        win = gtk_dialog_new_with_buttons (title, GTK_WINDOW (parentwin), GTK_DIALOG_MODAL, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (win), GTK_RESPONSE_OK);
    }
    else {
        win = gtk_dialog_new_with_buttons (title, GTK_WINDOW (parentwin), GTK_DIALOG_MODAL, NULL);
        if (buttons & (1<<ddb_button_ok)) {
            gtk_dialog_add_button (GTK_DIALOG (win), GTK_STOCK_OK, GTK_RESPONSE_OK);
        }
        if (buttons & (1<<ddb_button_cancel)) {
            gtk_dialog_add_button (GTK_DIALOG (win), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
        }
        if (buttons & (1<<ddb_button_close)) {
            gtk_dialog_add_button (GTK_DIALOG (win), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
        }
        if (buttons & (1<<ddb_button_apply)) {
            gtk_dialog_add_button (GTK_DIALOG (win), GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
        }
        if (buttons & (1<<ddb_button_yes)) {
            gtk_dialog_add_button (GTK_DIALOG (win), GTK_STOCK_YES, GTK_RESPONSE_YES);
        }
        if (buttons & (1<<ddb_button_no)) {
            gtk_dialog_add_button (GTK_DIALOG (win), GTK_STOCK_NO, GTK_RESPONSE_NO);
        }
    }
    gtk_window_set_type_hint (GTK_WINDOW (win), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_container_set_border_width (GTK_CONTAINER(win), 12);

    gtk_window_set_title (GTK_WINDOW (win), title);
    gtk_window_set_modal (GTK_WINDOW (win), TRUE);
    gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (parentwin));

    GtkWidget *widgets[100] = {NULL};
    int pack[100] = {0};
    int ncurr = 0;

    widgets[ncurr] = GTK_DIALOG (win)->vbox;
    gtk_box_set_spacing (GTK_BOX (widgets[ncurr]), 8);
    GtkWidget *action_area = GTK_DIALOG (win)->action_area;
    gtk_widget_show (action_area);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);


    // parse script
    char token[MAX_TOKEN];
    const char *script = conf->layout;
    parser_line = 1;
    while (script = gettoken (script, token)) {
        if (strcmp (token, "property")) {
            fprintf (stderr, "invalid token while loading plugin %s config dialog: %s at line %d\n", conf->title, token, parser_line);
            break;
        }
        char labeltext[MAX_TOKEN];
        script = gettoken_warn_eof (script, labeltext);
        if (!script) {
            break;
        }

        if (ncurr > 0) {
            pack[ncurr]--;
            if (pack[ncurr] < 0) {
                ncurr--;
            }
        }

        char type[MAX_TOKEN];
        script = gettoken_warn_eof (script, type);
        if (!script) {
            break;
        }

        if (!strncmp (type, "hbox[", 5) || !strncmp (type, "vbox[", 5)) {
            ncurr++;
            int n = 0;
            if (1 != sscanf (type+4, "[%d]", &n)) {
                break;
            }
            pack[ncurr] = n;

            int vert = 0;
            int hmg = FALSE;
            int fill = FALSE;
            int expand = FALSE;
            int border = 0;
            int spacing = 8;
            int height = 100;

            char param[MAX_TOKEN];
            for (;;) {
                script = gettoken_warn_eof (script, param);
                if (!script) {
                    break;
                }
                if (!strcmp (param, ";")) {
                    break;
                }
                else if (!strcmp (param, "hmg")) {
                    hmg = TRUE;
                }
                else if (!strcmp (param, "fill")) {
                    fill = TRUE;
                }
                else if (!strcmp (param, "expand")) {
                    expand = TRUE;
                }
                else if (!strncmp (param, "border=", 7)) {
                    border = atoi (param+7);
                }
                else if (!strncmp (param, "spacing=", 8)) {
                    spacing = atoi (param+8);
                }
                else if (!strncmp (param, "height=", 7)) {
                    height = atoi (param+7);
                }
            }

            widgets[ncurr] = vert ? gtk_vbox_new (TRUE, spacing) : gtk_hbox_new (TRUE, spacing);
            gtk_widget_set_size_request (widgets[ncurr], -1, height);
            gtk_widget_show (widgets[ncurr]);
            gtk_box_pack_start (GTK_BOX(widgets[ncurr-1]), widgets[ncurr], fill, expand, border);
            continue;
        }

        int vertical = 0;

        char key[MAX_TOKEN];
        for (;;) {
            script = gettoken_warn_eof (script, key);
            if (!script) {
                break;
            }
            if (!strcmp (key, "vert")) {
                vertical = 1;
            }
            else {
                break;
            }
        }

        char def[MAX_TOKEN];
        script = gettoken_warn_eof (script, def);
        if (!script) {
            break;
        }

        // add to dialog
        GtkWidget *label = NULL;
        GtkWidget *prop = NULL;
        GtkWidget *cont = NULL;
        char value[1000];
        conf->get_param (key, value, sizeof (value), def);
        if (!strcmp (type, "entry") || !strcmp (type, "password")) {
            label = gtk_label_new (_(labeltext));
            gtk_widget_show (label);
            prop = gtk_entry_new ();
            gtk_entry_set_activates_default (GTK_ENTRY (prop), TRUE);
            g_signal_connect (G_OBJECT (prop), "changed", G_CALLBACK (prop_changed), win);
            gtk_widget_show (prop);
            gtk_entry_set_text (GTK_ENTRY (prop), value);

            if (!strcmp (type, "password")) {
                gtk_entry_set_visibility (GTK_ENTRY (prop), FALSE);
            }
        }
        else if (!strcmp (type, "checkbox")) {
            prop = gtk_check_button_new_with_label (_(labeltext));
            g_signal_connect (G_OBJECT (prop), "toggled", G_CALLBACK (prop_changed), win);
            gtk_widget_show (prop);
            int val = atoi (value);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop), val);
        }
        else if (!strcmp (type, "file")) {
            label = gtk_label_new (_(labeltext));
            gtk_widget_show (label);
            if (deadbeef->conf_get_int ("gtkui.pluginconf.use_filechooser_button", 0)) {
                prop = gtk_file_chooser_button_new (_(labeltext), GTK_FILE_CHOOSER_ACTION_OPEN);
                gtk_widget_show (prop);
                gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (prop), value);
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
                gtk_entry_set_text (GTK_ENTRY (prop), value);
                gtk_box_pack_start (GTK_BOX (cont), prop, TRUE, TRUE, 0);
                GtkWidget *btn = gtk_button_new_with_label ("…");
                gtk_widget_show (btn);
                gtk_box_pack_start (GTK_BOX (cont), btn, FALSE, FALSE, 0);
                g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (on_prop_browse_file), prop);
            }
        }
        else if (!strncmp (type, "select[", 7)) {
            int n;
            if (1 != sscanf (type+6, "[%d]", &n)) {
                break;
            }

            label = gtk_label_new (_(labeltext));
            gtk_widget_show (label);

            prop = gtk_combo_box_new_text ();
            gtk_widget_show (prop);

            for (int i = 0; i < n; i++) {
                char entry[MAX_TOKEN];
                script = gettoken_warn_eof (script, entry);
                if (!script) {
                    break;
                }

                gtk_combo_box_append_text (GTK_COMBO_BOX (prop), entry);
            }
            if (!script) {
                break;
            }
            gtk_combo_box_set_active (GTK_COMBO_BOX (prop), atoi (value));
            g_signal_connect ((gpointer) prop, "changed",
                    G_CALLBACK (prop_changed),
                    win);
        }
        else if (!strncmp (type, "hscale[", 7) || !strncmp (type, "vscale[", 7) || !strncmp (type, "spinbtn[", 8)) {
            float min, max, step;
            const char *args;
            if (type[0] == 's') {
                args = type + 7;
            }
            else {
                args = type + 6;
            }
            if (3 != sscanf (args, "[%f,%f,%f]", &min, &max, &step)) {
                break;
            }
            int invert = 0;
            if (min >= max) {
                float tmp = min;
                min = max;
                max = tmp;
                invert = 1;
            }
            if (step <= 0) {
                step = 1;
            }
            if (type[0] == 's') {
                prop = gtk_spin_button_new_with_range (min, max, step);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (prop), atof (value));
            }
            else {
                prop = type[0] == 'h' ? gtk_hscale_new_with_range (min, max, step) : gtk_vscale_new_with_range (min, max, step);
                if (invert) {
                    gtk_range_set_inverted (GTK_RANGE (prop), TRUE);
                }
                gtk_range_set_value (GTK_RANGE (prop), (gdouble)atof (value));
                gtk_scale_set_value_pos (GTK_SCALE (prop), GTK_POS_RIGHT);
            }
            label = gtk_label_new (_(labeltext));
            gtk_widget_show (label);
            g_signal_connect (G_OBJECT (prop), "value-changed", G_CALLBACK (prop_changed), win);
            gtk_widget_show (prop);
        }

        script = gettoken_warn_eof (script, token);
        if (!script) {
            break;
        }
        if (strcmp (token, ";")) {
            fprintf (stderr, "expected `;' while loading plugin %s config dialog: %s at line %d\n", conf->title, token, parser_line);
            break;
        }


        if (label && prop) {
            GtkWidget *hbox = NULL;
            hbox = vertical ? gtk_vbox_new (FALSE, 8) : gtk_hbox_new (FALSE, 8);
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
            gtk_box_pack_start (GTK_BOX (widgets[ncurr]), cont, FALSE, FALSE, 0);
        }
    }

    int response;
    do {
        gtk_dialog_set_response_sensitive (GTK_DIALOG (win), GTK_RESPONSE_APPLY, FALSE);
        response = gtk_dialog_run (GTK_DIALOG (win));
        if (response == GTK_RESPONSE_APPLY || response == GTK_RESPONSE_OK) {
            apply_conf (win, conf);
        }
        if (callback) {
            int btn = ddb_button_from_gtk_response (response);
            if (!callback (btn, ctx)) {
                break;
            }
        }
    } while (response == GTK_RESPONSE_APPLY);
    gtk_widget_destroy (win);
    int btn = ddb_button_from_gtk_response (response);
    return btn;
}

int
gtkui_run_dialog_root (ddb_dialog_t *conf, uint32_t buttons, int (*callback)(int button, void *ctx), void *ctx) {
    return gtkui_run_dialog (mainwin, conf, buttons, callback, ctx);
}
