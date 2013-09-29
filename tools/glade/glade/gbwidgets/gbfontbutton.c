/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999-2002  Damon Chaplin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <gtk/gtk.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/fontbutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Title = "GtkFontButton::title";
static gchar *ShowStyle = "GtkFontButton::show_style";
static gchar *ShowSize = "GtkFontButton::show_size";
static gchar *UseFont = "GtkFontButton::use_font";
static gchar *UseFontSize = "GtkFontButton::use_size";
static gchar *FocusOnClick = "GtkFontButton|GtkButton::focus_on_click";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkFontButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
/*
static GtkWidget*
gb_font_button_new (GbWidgetNewData *data)
{

}
*/



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_font_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_string (Title, _("Title:"),
		       _("The title of the font selection dialog"));
  property_add_bool (ShowStyle, _("Show Style:"),
		     _("If the font style is shown as part of the font information"));
  property_add_bool (ShowSize, _("Show Size:"),
		     _("If the font size is shown as part of the font information"));
  property_add_bool (UseFont, _("Use Font:"),
		     _("If the selected font is used when displaying the font information"));
  property_add_bool (UseFontSize, _("Use Size:"),
		     _("if the selected font size is used when displaying the font information"));
  property_add_bool (FocusOnClick, _("Focus On Click:"), _("If the button grabs focus when it is clicked"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_font_button_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gboolean show_style, show_size, use_font, use_size;
  const gchar *title;

  /* Only save if the title is different to the default. */
  title = gtk_font_button_get_title (GTK_FONT_BUTTON (widget));
  if (data->action == GB_SHOWING
      || (title && strcmp (title, dgettext (GLADE_GTK_GETTEXT_PACKAGE,
					    "Pick a Font"))))
    gb_widget_output_translatable_string (data, Title, title);

  g_object_get (G_OBJECT (widget),
		"show_style", &show_style,
		"show_size", &show_size,
		"use_font", &use_font,
		"use_size", &use_size,
		NULL);

  gb_widget_output_bool (data, ShowStyle, show_style);
  gb_widget_output_bool (data, ShowSize, show_size);
  gb_widget_output_bool (data, UseFont, use_font);
  gb_widget_output_bool (data, UseFontSize, use_size);

  gb_widget_output_bool (data, FocusOnClick,
			 gtk_button_get_focus_on_click (GTK_BUTTON (widget)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_font_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *title;
  gboolean show_style, show_size, use_font, use_size, focus_on_click;

  title = gb_widget_input_string (data, Title);
  if (data->apply)
    gtk_font_button_set_title (GTK_FONT_BUTTON (widget),
			       title && title[0]
			       ? title : dgettext (GLADE_GTK_GETTEXT_PACKAGE,
						   "Pick a Font"));

  show_style = gb_widget_input_bool (data, ShowStyle);
  if (data->apply)
    gtk_font_button_set_show_style (GTK_FONT_BUTTON (widget), show_style);

  show_size = gb_widget_input_bool (data, ShowSize);
  if (data->apply)
    gtk_font_button_set_show_size (GTK_FONT_BUTTON (widget), show_size);

  use_font = gb_widget_input_bool (data, UseFont);
  if (data->apply)
    gtk_font_button_set_use_font (GTK_FONT_BUTTON (widget), use_font);

  use_size = gb_widget_input_bool (data, UseFontSize);
  if (data->apply)
    gtk_font_button_set_use_size (GTK_FONT_BUTTON (widget), use_size);

  focus_on_click = gb_widget_input_bool (data, FocusOnClick);
  if (data->apply)
    gtk_button_set_focus_on_click (GTK_BUTTON (widget), focus_on_click);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkFontButton, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_font_button_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_font_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  const gchar *title;
  gboolean show_style, show_size, use_font, use_size;

  if (data->create_widget)
    source_add (data, "  %s = gtk_font_button_new ();\n", data->wname);

  gb_widget_write_standard_source (widget, data);

  title = gtk_font_button_get_title (GTK_FONT_BUTTON (widget));
  if (title && title[0]
      && strcmp (title, dgettext (GLADE_GTK_GETTEXT_PACKAGE, "Pick a Font")))
    {
      gboolean translatable, context;
      gchar *comments;

      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  gtk_font_button_set_title (GTK_FONT_BUTTON (%s), %s);\n",
		  data->wname,
		  source_make_string_full (title, data->use_gettext && translatable, context));
    }

  g_object_get (G_OBJECT (widget),
		"show_style", &show_style,
		"show_size", &show_size,
		"use_font", &use_font,
		"use_size", &use_size,
		NULL);

  if (!show_style)
    {
      source_add (data,
		  "  gtk_font_button_set_show_style (GTK_FONT_BUTTON (%s), FALSE);\n",
		  data->wname);
    }

  if (!show_size)
    {
      source_add (data,
		  "  gtk_font_button_set_show_size (GTK_FONT_BUTTON (%s), FALSE);\n",
		  data->wname);
    }

  if (use_font)
    {
      source_add (data,
		  "  gtk_font_button_set_use_font (GTK_FONT_BUTTON (%s), TRUE);\n",
		  data->wname);
    }

  if (use_size)
    {
      source_add (data,
		  "  gtk_font_button_set_use_size (GTK_FONT_BUTTON (%s), TRUE);\n",
		  data->wname);
    }

  if (!gtk_button_get_focus_on_click (GTK_BUTTON (widget)))
    {
      source_add (data,
		  "  gtk_button_set_focus_on_click (GTK_BUTTON (%s), FALSE);\n",
		  data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_font_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_font_button_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = fontbutton_xpm;
  gbwidget.tooltip = _("Font Chooser Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties	= gb_font_button_create_properties;
  gbwidget.gb_widget_get_properties	= gb_font_button_get_properties;
  gbwidget.gb_widget_set_properties	= gb_font_button_set_properties;
  gbwidget.gb_widget_write_source	= gb_font_button_write_source;
/*
  gbwidget.gb_widget_new		= gb_font_button_new;
  gbwidget.gb_widget_create_popup_menu	= gb_font_button_create_popup_menu;
*/

  return &gbwidget;
}

