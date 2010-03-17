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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "trkproperties.h"
#include "interface.h"
#include "support.h"
#include "../../deadbeef.h"
#include "gtkui.h"

GtkWidget *trackproperties;

gboolean
on_trackproperties_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    trackproperties = NULL;
    return FALSE;
}

gboolean
on_trackproperties_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if (event->keyval == GDK_Escape) {
        trackproperties = NULL;
        gtk_widget_destroy (widget);
    }
    return FALSE;
}

void
show_track_properties_dlg (DB_playItem_t *it) {
    if (!trackproperties) {
        trackproperties = create_trackproperties ();
        gtk_window_set_transient_for (GTK_WINDOW (trackproperties), GTK_WINDOW (mainwin));
    }
    GtkWidget *widget = trackproperties;
    GtkWidget *w;
    const char *meta;
    // fill in metadata
    // location
    w = lookup_widget (widget, "location");
    gtk_entry_set_text (GTK_ENTRY (w), it->fname);
    // title
    w = lookup_widget (widget, "title");
    meta = deadbeef->pl_find_meta (it, "title");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // artist
    w = lookup_widget (widget, "artist");
    meta = deadbeef->pl_find_meta (it, "artist");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // band
    w = lookup_widget (widget, "band");
    meta = deadbeef->pl_find_meta (it, "band");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // album
    w = lookup_widget (widget, "album");
    meta = deadbeef->pl_find_meta (it, "album");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // genre
    w = lookup_widget (widget, "genre");
    meta = deadbeef->pl_find_meta (it, "genre");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // year
    w = lookup_widget (widget, "year");
    meta = deadbeef->pl_find_meta (it, "year");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // track
    w = lookup_widget (widget, "track");
    meta = deadbeef->pl_find_meta (it, "track");
    if (!meta) {
        meta = "";
    }
    gtk_entry_set_text (GTK_ENTRY (w), meta);
    // comment
    w = lookup_widget (widget, "comment");
    meta = deadbeef->pl_find_meta (it, "comment");
    if (!meta) {
        meta = "";
    }
    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
    gtk_text_buffer_set_text (buffer, meta, strlen (meta));
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (w), buffer);
    g_object_unref (buffer);

    gtk_widget_show (widget);
    gtk_window_present (GTK_WINDOW (widget));
}

