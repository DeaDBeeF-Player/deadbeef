/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include "progress.h"

static GtkWidget *progressdlg;
static GtkWidget *progressitem;
static int progress_aborted;

void
progress_init (void) {
    extern GtkWidget *mainwin;
    progressdlg = create_addprogress ();
    gtk_window_set_transient_for (GTK_WINDOW (progressdlg), GTK_WINDOW (mainwin));
    progressitem = lookup_widget (progressdlg, "progresstitle");
}

void
progress_show (void) {
    progress_aborted = 0;
    progress_settext ("Initializing...");
    gtk_widget_show_all (progressdlg);
    gtk_window_present (GTK_WINDOW (progressdlg));
}

void
progress_hide (void) {
    gtk_widget_hide (progressdlg);
}

void
progress_settext (const char *text) {
    gtk_entry_set_text (GTK_ENTRY (progressitem), text);
}

void
progress_abort (void) {
    progress_aborted = 1;
}

void
on_progress_abort                      (GtkButton       *button,
                                        gpointer         user_data)
{
    progress_aborted = 1;
}

int
progress_is_aborted (void) {
    return progress_aborted;
}


gboolean
on_addprogress_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    progress_aborted = 1;
    return gtk_widget_hide_on_delete (widget);
}
