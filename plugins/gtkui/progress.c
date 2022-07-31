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
#if GTK_CHECK_VERSION(3,0,0)
    g_application_mark_busy (G_APPLICATION (gapp));
#endif
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
#if GTK_CHECK_VERSION(3,0,0)
    g_application_unmark_busy (G_APPLICATION (gapp));
#endif
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


