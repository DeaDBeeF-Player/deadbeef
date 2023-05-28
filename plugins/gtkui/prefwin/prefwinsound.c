/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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
#include <string.h>
#include <stdlib.h>
#include "../gtkui.h"
#include "../support.h"
#include "prefwin.h"
#include "prefwinsound.h"

static GtkWidget *prefwin;
static GSList *output_device_names;

static const char *
_get_output_soundcard_conf_name (void) {
    static char name[100];
    snprintf (name, sizeof (name), "%s_soundcard", deadbeef->get_output ()->plugin.id);
    return name;
}

static void
gtk_enum_sound_callback (const char *name, const char *desc, void *userdata) {
    GtkComboBox *combobox = GTK_COMBO_BOX (userdata);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), desc);

    deadbeef->conf_lock ();
    const char *curr = deadbeef->conf_get_str_fast (_get_output_soundcard_conf_name(), "default");
    if (!strcmp (curr, name)) {
        gtk_combo_box_set_active (combobox, g_slist_length (output_device_names));
    }
    deadbeef->conf_unlock ();

    output_device_names = g_slist_append (output_device_names, g_strdup (name));
}

void
prefwin_fill_soundcards (void) {
    if (!prefwin) {
        return;
    }
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (prefwin, "pref_soundcard"));
    GtkTreeModel *mdl = gtk_combo_box_get_model (combobox);
    gtk_list_store_clear (GTK_LIST_STORE (mdl));

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), _("Default Audio Device"));

    deadbeef->conf_lock ();
    const char *s = deadbeef->conf_get_str_fast (_get_output_soundcard_conf_name(), "default");
    if (!strcmp (s, "default")) {
        gtk_combo_box_set_active (combobox, 0);
    }
    deadbeef->conf_unlock ();

    if (output_device_names) {
        for (GSList *dev = output_device_names; dev; dev = dev->next) {
            g_free (dev->data);
            dev->data = NULL;
        }
        g_slist_free (output_device_names);
        output_device_names = NULL;
    }
    output_device_names = g_slist_append (output_device_names, g_strdup ("default"));
    if (deadbeef->get_output ()->enum_soundcards) {
        deadbeef->get_output ()->enum_soundcards (gtk_enum_sound_callback, combobox);
        gtk_widget_set_sensitive (GTK_WIDGET (combobox), TRUE);
    }
    else {
        gtk_widget_set_sensitive (GTK_WIDGET (combobox), FALSE);
    }
}

static void
on_pref_output_plugin_changed          (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);

    DB_output_t **out_plugs = deadbeef->plug_get_output_list ();
    DB_output_t *prev = NULL;
    DB_output_t *new = NULL;

    deadbeef->conf_lock ();
    const char *outplugname = deadbeef->conf_get_str_fast ("output_plugin", "alsa");
    for (int i = 0; out_plugs[i]; i++) {
        if (!strcmp (out_plugs[i]->plugin.id, outplugname)) {
            prev = out_plugs[i];
        }
        if (i == active) {
            new = out_plugs[i];
        }
    }
    deadbeef->conf_unlock ();

    if (!new) {
        fprintf (stderr, "failed to find output plugin selected in preferences window\n");
    }
    else {
        if (prev != new) {
            deadbeef->conf_set_str ("output_plugin", new->plugin.id);
            deadbeef->sendmessage (DB_EV_REINIT_SOUND, 0, 0, 0);
        }
    }
}

static void
on_pref_soundcard_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    if (active >= 0 && active < g_slist_length(output_device_names)) {
        deadbeef->conf_lock ();
        const char *soundcard = deadbeef->conf_get_str_fast (_get_output_soundcard_conf_name(), "default");
        const char *active_name = g_slist_nth_data (output_device_names, active);
        if (strcmp (soundcard, active_name)) {
            deadbeef->conf_set_str (_get_output_soundcard_conf_name(), active_name);
            deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
        }
        deadbeef->conf_unlock ();
    }
}

static void
update_samplerate_widget_sensitivity (int override_sr, int dep_active) {
    gtk_widget_set_sensitive (lookup_widget (prefwin, "label_direct_sr"), override_sr);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "comboboxentry_direct_sr"), override_sr);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "checkbutton_dependent_sr"), override_sr);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "comboboxentry_sr_mult_48"), override_sr && dep_active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "comboboxentry_sr_mult_44"), override_sr && dep_active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "label_sr_mult_48"), override_sr && dep_active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "label_sr_mult_44"), override_sr && dep_active);
}

void
on_checkbutton_sr_override_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int override_sr = gtk_toggle_button_get_active (togglebutton);
    int dep_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "checkbutton_dependent_sr")));
    update_samplerate_widget_sensitivity (override_sr, dep_active);
    deadbeef->conf_set_int ("streamer.override_samplerate", override_sr);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_checkbutton_dependent_sr_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int override_sr = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "checkbutton_sr_override")));
    int dep_active = gtk_toggle_button_get_active (togglebutton);
    update_samplerate_widget_sensitivity (override_sr, dep_active);
    deadbeef->conf_set_int ("streamer.use_dependent_samplerate", dep_active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


static int
clamp_samplerate (int val) {
    if (val < 8000) {
        return 8000;
    }
    else if (val > 768000) {
        return 768000;
    }
    return val;
}

void
on_comboboxentry_direct_sr_changed     (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int val = atoi(gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox)))));
    int out = clamp_samplerate (val);
    deadbeef->conf_set_int ("streamer.samplerate", out);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_comboboxentry_sr_mult_48_changed    (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int val = atoi(gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox)))));
    int out = clamp_samplerate (val);
    deadbeef->conf_set_int ("streamer.samplerate_mult_48", out);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_comboboxentry_sr_mult_44_changed    (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int val = atoi(gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox)))));
    int out = clamp_samplerate (val);
    deadbeef->conf_set_int ("streamer.samplerate_mult_44", out);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
prefwin_init_sound_tab (GtkWidget *_prefwin) {
    GtkWidget *w = prefwin = _prefwin;
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_output_plugin"));

    const char *outplugname = deadbeef->conf_get_str_fast ("output_plugin", "alsa");
    DB_output_t **out_plugs = deadbeef->plug_get_output_list ();
    for (int i = 0; out_plugs[i]; i++) {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), out_plugs[i]->plugin.name);
        if (!strcmp (outplugname, out_plugs[i]->plugin.id)) {
            gtk_combo_box_set_active (combobox, i);
        }
    }

    // soundcard (output device) selection
    prefwin_fill_soundcards ();

    g_signal_connect ((gpointer) combobox, "changed",
                      G_CALLBACK (on_pref_output_plugin_changed),
                      NULL);
    GtkWidget *pref_soundcard = lookup_widget (prefwin, "pref_soundcard");
    g_signal_connect ((gpointer) pref_soundcard, "changed",
                      G_CALLBACK (on_pref_soundcard_changed),
                      NULL);

    // 8_to_16
    prefwin_set_toggle_button("convert8to16", deadbeef->conf_get_int ("streamer.8_to_16", 1));

    // 16_to_24
    prefwin_set_toggle_button("convert16to24", deadbeef->conf_get_int ("streamer.16_to_24", 0));

    // override bit depth
    GtkComboBox *combo_bit_override = GTK_COMBO_BOX (lookup_widget (w, "combo_bit_override"));
    gtk_combo_box_set_active(combo_bit_override, deadbeef->conf_get_int ("streamer.bit_override", 0));

    // override samplerate checkbox
    int override_sr = deadbeef->conf_get_int ("streamer.override_samplerate", 0);
    prefwin_set_toggle_button ("checkbutton_sr_override", override_sr);

    // direct/dependent samplerate radio buttons
    int use_dependent_samplerate = deadbeef->conf_get_int ("streamer.use_dependent_samplerate", 0);
    prefwin_set_toggle_button ("checkbutton_dependent_sr", use_dependent_samplerate);

    // direct samplerate value
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (lookup_widget (w, "comboboxentry_direct_sr")))), deadbeef->conf_get_str_fast ("streamer.samplerate", "44100"));
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (lookup_widget (w, "comboboxentry_sr_mult_48")))), deadbeef->conf_get_str_fast ("streamer.samplerate_mult_48", "48000"));
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (lookup_widget (w, "comboboxentry_sr_mult_44")))), deadbeef->conf_get_str_fast ("streamer.samplerate_mult_44", "44100"));

    update_samplerate_widget_sensitivity (override_sr, use_dependent_samplerate);
}

void
on_convert8to16_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("streamer.8_to_16", gtk_toggle_button_get_active (togglebutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_convert16to24_toggled                (GtkToggleButton *togglebutton,
                                         gpointer         user_data)
{
    deadbeef->conf_set_int ("streamer.16_to_24", gtk_toggle_button_get_active (togglebutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_combo_bit_override_changed           (GtkComboBox     *combobox,
                                         gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    deadbeef->conf_set_int ("streamer.bit_override", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}
