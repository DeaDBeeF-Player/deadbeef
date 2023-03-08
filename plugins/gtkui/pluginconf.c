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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "../../gettext.h"
#include <deadbeef/deadbeef.h>
#include "gtkui.h"
#include "../libparser/parser.h"
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
    deadbeef->conf_lock ();
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str_fast ("filechooser.lastdir", ""));
    deadbeef->conf_unlock ();
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
        deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
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

// Apply the configuration. *w is the container that holds the references to the widgets looked up with g_object_get_data(w,...)
void apply_conf (GtkWidget *w, ddb_dialog_t *conf, int reset_settings) {
    // parse script
    char token[MAX_TOKEN];
    const char *script = conf->layout;
    parser_line = 1;
    while ((script = gettoken (script, token))) {
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
            while ((script = gettoken_warn_eof (script, semicolon))) {
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

        int islabel = !strcmp(type, "label");

        char def[MAX_TOKEN];
        if (!islabel) {
            script = gettoken_warn_eof (script, def);
            if (!script) {
                break;
            }
        }

        if (!islabel && reset_settings) {
            conf->set_param (key, def);

            // skip to ;
            char semicolon[MAX_TOKEN];
            while ((script = gettoken_warn_eof (script, semicolon))) {
                if (!strcmp (semicolon, ";")) {
                    break;
                }
            }
            continue;
        } else if (!islabel) {
            // fetch data
            GtkWidget *widget = g_object_get_data (G_OBJECT(w), key);
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
        }

        script = gettoken_warn_eof (script, token);
        if (!script) {
            break;
        }
        if (strcmp (token, ";")) {
            fprintf (stderr, "apply_conf: expected `;' while loading plugin %s config dialog: %s at line %d\n", conf->title, token, parser_line);
            break;
        }
    }
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

static void
prop_changed (GtkWidget *editable, gpointer user_data) {
    ddb_pluginprefs_dialog_t *conf = (ddb_pluginprefs_dialog_t*)g_object_get_data (G_OBJECT (user_data), "dialog_conf_struct");
    if (conf->prop_changed) conf->prop_changed(conf);
}

void run_dialog_prop_changed_cb (ddb_pluginprefs_dialog_t *make_dialog_conf) {
    gtk_dialog_set_response_sensitive (GTK_DIALOG (make_dialog_conf->parent), GTK_RESPONSE_APPLY, TRUE);
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

static int
backout_pack_level(int ncurr, int *pack)
{
    if (ncurr > 0) {
        pack[ncurr]--;
        if (pack[ncurr] < 0) {
            ncurr--;
            ncurr = backout_pack_level(ncurr, pack);
        }
    }
    return ncurr;
}

static int
check_semicolon(const char **script, char *token, const char *plugintitle)
{
    *script = gettoken_warn_eof (*script, token);
    if (!script) {
        return -1;
    }
    if (strcmp (token, ";")) {
        fprintf (stderr, "make_dialog: expected `;' while loading plugin %s config dialog: %s at line %d\n", plugintitle, token, parser_line);
        return -1;
    }
    return 0;
}

void
gtkui_make_dialog (ddb_pluginprefs_dialog_t *make_dialog_conf) {
    GtkWidget *widgets[100] = {NULL};
    ddb_dialog_t *conf = &make_dialog_conf->dialog_conf;
    int pack[100] = {0};
    int ncurr = 0;

    GtkWidget *containervbox = make_dialog_conf->containerbox;

    // This needs to be set on an object that is tied to the lifetime of the plugin preferences container
    make_dialog_conf = (ddb_pluginprefs_dialog_t *)g_memdup (make_dialog_conf, sizeof(ddb_pluginprefs_dialog_t));
    g_object_set_data_full (G_OBJECT(containervbox), "dialog_conf_struct", make_dialog_conf, g_free);


    // Temporarily disable the callback until dialog script has been fully parsed
    void (*temp_prop_changed) = make_dialog_conf->prop_changed;
    make_dialog_conf->prop_changed = NULL;

    widgets[ncurr] = containervbox;
    gtk_box_set_spacing (GTK_BOX (widgets[ncurr]), 8);

    // parse script
    char token[MAX_TOKEN];
    const char *script = conf->layout;
    parser_line = 1;
    while ((script = gettoken (script, token))) {
        if (strcmp (token, "property")) {
            fprintf (stderr, "invalid token while loading plugin %s config dialog: %s at line %d\n", conf->title, token, parser_line);
            break;
        }
        char labeltext[MAX_TOKEN];
        script = gettoken_warn_eof (script, labeltext);
        if (!script) {
            break;
        }

        ncurr = backout_pack_level(ncurr, pack);

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

            int vert = !strncmp (type, "vbox[", 5);
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

            widgets[ncurr] = vert ? gtk_vbox_new (hmg, spacing) : gtk_hbox_new (hmg, spacing);
            gtk_widget_set_size_request (widgets[ncurr], vert ? height : -1, vert ? -1 : height);
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

        if (!strcmp(type, "label")) {
            GtkWidget *label = gtk_label_new (_(labeltext));
            gtk_widget_show (label);
            if (!strncmp(key, "l", 1)) {
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
            } else if (!strncmp(key, "c", 1)) {
                gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
            } else if (!strncmp(key, "r", 1)) {
                gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
            }
            gtk_box_pack_start (GTK_BOX (widgets[ncurr]), label, FALSE, TRUE, 0);

            if (check_semicolon(&script, token, conf->title) < 0) break;

            continue;
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
            gtk_entry_set_width_chars (GTK_ENTRY(prop), 5);
            gtk_entry_set_activates_default (GTK_ENTRY (prop), TRUE);
            g_signal_connect (G_OBJECT (prop), "changed", G_CALLBACK (prop_changed), containervbox);
            gtk_widget_show (prop);
            gtk_entry_set_text (GTK_ENTRY (prop), value);

            if (!strcmp (type, "password")) {
                gtk_entry_set_visibility (GTK_ENTRY (prop), FALSE);
            }
        }
        else if (!strcmp (type, "checkbox")) {
            prop = gtk_check_button_new_with_label (_(labeltext));
            g_signal_connect (G_OBJECT (prop), "toggled", G_CALLBACK (prop_changed), containervbox);
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
                g_signal_connect (G_OBJECT (prop), "file-set", G_CALLBACK (prop_changed), containervbox);
            }
            else {
                cont = gtk_hbox_new (FALSE, 2);
                gtk_widget_show (cont);
                prop = gtk_entry_new ();
                gtk_entry_set_activates_default (GTK_ENTRY (prop), TRUE);
                g_signal_connect (G_OBJECT (prop), "changed", G_CALLBACK (prop_changed), containervbox);
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

            prop = gtk_combo_box_text_new ();
            gtk_widget_show (prop);

            for (int i = 0; i < n; i++) {
                char entry[MAX_TOKEN];
                script = gettoken_warn_eof (script, entry);
                if (!script) {
                    break;
                }

                gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (prop), entry);
            }
            if (!script) {
                break;
            }
            gtk_combo_box_set_active (GTK_COMBO_BOX (prop), atoi (value));
            g_signal_connect ((gpointer) prop, "changed",
                    G_CALLBACK (prop_changed),
                    containervbox);
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
                gtk_scale_set_value_pos (GTK_SCALE (prop), vertical?GTK_POS_BOTTOM:GTK_POS_RIGHT);
            }
            label = gtk_label_new (_(labeltext));
            gtk_widget_show (label);
            g_signal_connect (G_OBJECT (prop), "value-changed", G_CALLBACK (prop_changed), containervbox);
            gtk_widget_show (prop);
        }

        if (check_semicolon(&script, token, conf->title) < 0) break;


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
            g_object_set_data (G_OBJECT (containervbox), key, prop);
        }
        if (cont) {
            gtk_box_pack_start (GTK_BOX (widgets[ncurr]), cont, FALSE, TRUE, 0);
        }
    }

    // Now that all signal handlers are installed, reinstate the callback
    make_dialog_conf->prop_changed = temp_prop_changed;

}

int
gtkui_run_dialog (GtkWidget *parentwin, ddb_dialog_t *conf, uint32_t buttons, int (*callback)(int button, void *ctx), void *ctx) {
    if (!parentwin) {
        parentwin = mainwin;
    }
    // create window
    char title[200];
    snprintf (title, sizeof (title), _("Configure %s"), conf->title);
    GtkWidget *win;
    if (!buttons) {
        win = gtk_dialog_new_with_buttons (title, GTK_WINDOW (parentwin), GTK_DIALOG_MODAL, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (win), GTK_RESPONSE_OK);
    }
    else {
        win = gtk_dialog_new_with_buttons (title, GTK_WINDOW (parentwin), GTK_DIALOG_MODAL, NULL, NULL);
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

    GtkWidget *action_area = gtk_dialog_get_action_area (GTK_DIALOG (win));
    gtk_widget_show (action_area);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);

    ddb_pluginprefs_dialog_t make_dialog_conf = {
        .dialog_conf = *conf,
        .parent = win,
        .containerbox = gtk_dialog_get_content_area (GTK_DIALOG (win)),
        .prop_changed = run_dialog_prop_changed_cb,
    };
    gtkui_make_dialog (&make_dialog_conf);

    int response;
    do {
        gtk_dialog_set_response_sensitive (GTK_DIALOG (win), GTK_RESPONSE_APPLY, FALSE);
        response = gtk_dialog_run (GTK_DIALOG (win));
        if (response == GTK_RESPONSE_APPLY || response == GTK_RESPONSE_OK) {
            apply_conf (make_dialog_conf.containerbox, conf, 0);
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
    return gtkui_run_dialog (conf->parent ? conf->parent : mainwin, conf, buttons, callback, ctx);
}
