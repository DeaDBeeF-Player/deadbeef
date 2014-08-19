/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../../gettext.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include "progress.h"
#include "gtkui.h"

static GtkWidget *progressdlg;
static GtkWidget *progressitem;
static int progress_aborted;

static void
on_progress_abort                      (GtkButton       *button,
                                        gpointer         user_data)
{
    progress_aborted = 1;
}

static gboolean
on_addprogress_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    progress_aborted = 1;
    return gtk_widget_hide_on_delete (widget);
}
void
progress_init (void) {
    progressdlg = create_progressdlg ();

    gtk_window_set_title (GTK_WINDOW (progressdlg), _("Adding files..."));

    g_signal_connect ((gpointer) progressdlg, "delete_event",
            G_CALLBACK (on_addprogress_delete_event),
            NULL);

    GtkWidget *cancelbtn = lookup_widget (progressdlg, "cancelbtn");
    g_signal_connect ((gpointer) cancelbtn, "clicked",
            G_CALLBACK (on_progress_abort),
            NULL);

    gtk_window_set_transient_for (GTK_WINDOW (progressdlg), GTK_WINDOW (mainwin));
    progressitem = lookup_widget (progressdlg, "progresstitle");
}

void
progress_destroy (void) {
    if (progressdlg) {
        gtk_widget_destroy (progressdlg);
        progressdlg = NULL;
    }
}

void
progress_settext (const char *text) {
    if (deadbeef->junk_detect_charset (text)) {
        text = "...";
    }
    gtk_entry_set_text (GTK_ENTRY (progressitem), text);
}

gboolean
gtkui_progress_show_idle (gpointer data) {
    progress_settext (_("Initializing..."));
    gtk_widget_show_all (progressdlg);
    gtk_window_present (GTK_WINDOW (progressdlg));
    gtk_window_set_transient_for (GTK_WINDOW (progressdlg), GTK_WINDOW (mainwin));
    return FALSE;
}

void
progress_show (void) {
    progress_aborted = 0;
    g_idle_add (gtkui_progress_show_idle, NULL);
}

gboolean
gtkui_progress_hide_idle (gpointer data) {
    gtk_widget_hide (progressdlg);
    return FALSE;
}

void
progress_hide (void) {
    g_idle_add (gtkui_progress_hide_idle, NULL);
}

void
progress_abort (void) {
    progress_aborted = 1;
}

int
progress_is_aborted (void) {
    return progress_aborted;
}


