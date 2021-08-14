/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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
#include "../gtkui.h"
#include "../support.h"
#include "prefwin.h"
#include "prefwinappearance.h"

void
prefwin_init_appearance_tab(GtkWidget *_prefwin) {
    GtkWidget *w = _prefwin;
    int override = deadbeef->conf_get_int ("gtkui.override_bar_colors", 0);
    prefwin_set_toggle_button("override_bar_colors", override);
    gtk_widget_set_sensitive (lookup_widget (w, "bar_colors_group"), override);

    // override tabstrip colors
    override = deadbeef->conf_get_int ("gtkui.override_tabstrip_colors", 0);
    prefwin_set_toggle_button("override_tabstrip_colors", override);
    gtk_widget_set_sensitive (lookup_widget (w, "tabstrip_colors_group"), override);

    prefwin_set_toggle_button("tabstrip_playing_bold", deadbeef->conf_get_int ("gtkui.tabstrip_embolden_playing", 0));
    prefwin_set_toggle_button("tabstrip_playing_italic", deadbeef->conf_get_int ("gtkui.tabstrip_italic_playing", 0));
    prefwin_set_toggle_button("tabstrip_selected_bold", deadbeef->conf_get_int ("gtkui.tabstrip_embolden_selected", 0));
    prefwin_set_toggle_button("tabstrip_selected_italic", deadbeef->conf_get_int ("gtkui.tabstrip_italic_selected", 0));

    // get default gtk font
    GtkStyle *style = gtk_widget_get_style (mainwin);
    const char *gtk_style_font = pango_font_description_to_string (style->font_desc);

    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "tabstrip_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.tabstrip_text", gtk_style_font));

    // override listview colors
    override = deadbeef->conf_get_int ("gtkui.override_listview_colors", 0);
    prefwin_set_toggle_button("override_listview_colors", override);
    gtk_widget_set_sensitive (lookup_widget (w, "listview_colors_group"), override);

    // embolden/italic listview text
    prefwin_set_toggle_button("listview_selected_text_bold", deadbeef->conf_get_int ("gtkui.embolden_selected_tracks", 0));
    prefwin_set_toggle_button("listview_selected_text_italic", deadbeef->conf_get_int ("gtkui.italic_selected_tracks", 0));
    prefwin_set_toggle_button("listview_playing_text_bold", deadbeef->conf_get_int ("gtkui.embolden_current_track", 0));
    prefwin_set_toggle_button("listview_playing_text_italic", deadbeef->conf_get_int ("gtkui.italic_current_track", 0));

    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_text", gtk_style_font));
    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_group_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_group_text", gtk_style_font));
    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_column_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_column_text", gtk_style_font));

    // colors
    prefwin_init_theme_colors ();
}
