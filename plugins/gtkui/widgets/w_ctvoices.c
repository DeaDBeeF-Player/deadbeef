/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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
#include <stdlib.h>
#include <string.h>
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_ctvoices.h"

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *voices[8];
} w_ctvoices_t;

// chiptune voice ctl
static void
on_voice_toggled (GtkToggleButton *togglebutton, gpointer user_data) {
    w_ctvoices_t *w = user_data;
    int voices = 0;
    for (int i = 0; i < 8; i++) {
        int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w->voices[i]));
        voices |= active << i;
    }
    deadbeef->conf_set_int ("chip.voices", voices);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

ddb_gtkui_widget_t *
w_ctvoices_create (void) {
    w_ctvoices_t *w = malloc (sizeof (w_ctvoices_t));
    memset (w, 0, sizeof (w_ctvoices_t));
    w->base.widget = gtk_event_box_new ();
    GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (w->base.widget), hbox);

    GtkWidget *label = gtk_label_new_with_mnemonic (_ ("Voices:"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    int voices = deadbeef->conf_get_int ("chip.voices", 0xff);
    for (int i = 0; i < 8; i++) {
        w->voices[i] = gtk_check_button_new ();
        gtk_widget_show (w->voices[i]);
        gtk_box_pack_start (GTK_BOX (hbox), w->voices[i], FALSE, FALSE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w->voices[i]), voices & (1 << i));
        g_signal_connect ((gpointer)w->voices[i], "toggled", G_CALLBACK (on_voice_toggled), w);
    }

    w_override_signals (w->base.widget, w);
    return (ddb_gtkui_widget_t *)w;
}
