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
#include "../drawing.h"
#include "../eq.h"
#include "../gtkui.h"
#include "../support.h"
#include "prefwin.h"
#include "prefwinappearance.h"

static GtkWidget *prefwin;

void
prefwin_init_appearance_tab(GtkWidget *_prefwin) {
    GtkWidget *w = prefwin = _prefwin;
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


static void
override_set_helper (GtkToggleButton  *togglebutton, const char* conf_str, const char *group_name)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int (conf_str, active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, group_name), active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    gtkui_init_theme_colors ();
    prefwin_init_theme_colors ();
}

static void
font_set_helper (GtkFontButton *fontbutton, const char* conf_str)
{
    deadbeef->conf_set_str (conf_str, gtk_font_button_get_font_name (fontbutton));
    gtkui_init_theme_colors ();
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
}

static int
font_style_set_helper (GtkToggleButton  *togglebutton, const char* conf_str)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int (conf_str, active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    return active;
}

static void
color_set_helper (GtkColorButton *colorbutton, const char* conf_str)
{
    if (conf_str == NULL) {
        return;
    }
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str (conf_str, str);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    gtkui_init_theme_colors ();
}

void
on_tabstrip_light_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_light");
}


void
on_tabstrip_mid_color_set              (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_mid");
}

void
on_tabstrip_dark_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_dark");
}

void
on_tabstrip_base_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_base");
}

void
on_tabstrip_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_text");
}

void
on_tabstrip_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_selected_text");
}

void
on_tabstrip_playing_bold_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_embolden_playing = font_style_set_helper (togglebutton, "gtkui.tabstrip_embolden_playing");
}

void
on_tabstrip_playing_italic_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_italic_playing = font_style_set_helper (togglebutton, "gtkui.tabstrip_italic_playing");
}

void
on_tabstrip_selected_bold_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_embolden_selected = font_style_set_helper (togglebutton, "gtkui.tabstrip_embolden_selected");
}

void
on_tabstrip_selected_italic_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_italic_selected = font_style_set_helper (togglebutton, "gtkui.tabstrip_italic_selected");
}

void
on_tabstrip_text_font_set              (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.tabstrip_text");
}

void
on_tabstrip_playing_text_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_playing_text");
}

void
on_bar_foreground_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.bar_foreground");
    eq_redraw ();
}

void
on_bar_background_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.bar_background");
    eq_redraw ();
}

void
on_override_listview_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_listview_colors", "listview_colors_group");
}


void
on_listview_even_row_color_set         (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_even_row");
}

void
on_listview_odd_row_color_set          (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_odd_row");
}

void
on_listview_selected_row_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_selection");
}

void
on_listview_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_text");
}


void
on_listview_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_selected_text");
}

void
on_listview_cursor_color_set           (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_cursor");
}

void
on_listview_playing_text_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_playing_text");
}

void
on_listview_group_text_color_set       (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_group_text");
}

void
on_listview_group_text_font_set        (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_group_text");
}

void
on_listview_text_font_set              (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_text");
}

void
on_listview_playing_text_bold_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_embolden_current_track = font_style_set_helper (togglebutton, "gtkui.embolden_current_track");
}

void
on_listview_playing_text_italic_toggled (GtkToggleButton *togglebutton,
                                         gpointer         user_data)
{
    gtkui_italic_current_track = font_style_set_helper (togglebutton, "gtkui.italic_current_track");
}

void
on_listview_selected_text_bold_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_embolden_selected_tracks = font_style_set_helper (togglebutton, "gtkui.embolden_selected_tracks");
}

void
on_listview_selected_text_italic_toggled (GtkToggleButton *togglebutton,
                                          gpointer         user_data)
{
    gtkui_italic_selected_tracks = font_style_set_helper (togglebutton, "gtkui.italic_selected_tracks");
}

void
on_listview_column_text_color_set      (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_column_text");
}

void
on_listview_column_text_font_set       (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_column_text");
}

void
on_override_bar_colors_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_bar_colors", "bar_colors_group");
    eq_redraw ();
}

void
on_override_tabstrip_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_tabstrip_colors", "tabstrip_colors_group");
}

