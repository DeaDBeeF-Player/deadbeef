/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999  Damon Chaplin
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

#include <gnome.h>
#include "../gb.h"
#include "../glade_gnome.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-colorpicker.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Dither = "GnomeColorPicker::dither";
static gchar *UseAlpha = "GnomeColorPicker::use_alpha";
static gchar *Title = "GnomeColorPicker::title";
static gchar *FocusOnClick = "GnomeColorPicker|GtkButton::focus_on_click";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeColorPicker, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
/*
static GtkWidget*
gb_gnome_color_picker_new (GbWidgetNewData *data)
{

}
*/



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_color_picker_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (Dither, _("Dither:"),
		     _("If the sample should use dithering to be more accurate"));
  property_add_bool (UseAlpha, _("Use Alpha:"),
		     _("If the alpha channel should be used"));

  property_add_string (Title, _("Title:"),
		       _("The title of the color selection dialog"));

  property_add_bool (FocusOnClick, _("Focus On Click:"), _("If the button grabs focus when it is clicked"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_color_picker_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_widget_output_bool (data, Dither, gnome_color_picker_get_dither (GNOME_COLOR_PICKER (widget)));
  gb_widget_output_bool (data, UseAlpha,
			 gnome_color_picker_get_use_alpha (GNOME_COLOR_PICKER (widget)));
  gb_widget_output_translatable_string (data, Title,
					gnome_color_picker_get_title (GNOME_COLOR_PICKER (widget)));

  gb_widget_output_bool (data, FocusOnClick,
			 gtk_button_get_focus_on_click (GTK_BUTTON (widget)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_color_picker_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean dither, use_alpha, focus_on_click;
  gchar *title;

  dither = gb_widget_input_bool (data, Dither);
  if (data->apply)
    gnome_color_picker_set_dither (GNOME_COLOR_PICKER (widget), dither);

  use_alpha = gb_widget_input_bool (data, UseAlpha);
  if (data->apply)
    gnome_color_picker_set_use_alpha (GNOME_COLOR_PICKER (widget), use_alpha);

  title = gb_widget_input_string (data, Title);
  if (data->apply)
    gnome_color_picker_set_title (GNOME_COLOR_PICKER (widget), title);

  focus_on_click = gb_widget_input_bool (data, FocusOnClick);
  if (data->apply)
    gtk_button_set_focus_on_click (GTK_BUTTON (widget), focus_on_click);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeColorPicker, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_color_picker_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_color_picker_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  if (data->create_widget)
    {
      source_add (data, "  %s = gnome_color_picker_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  if (strcmp (gnome_color_picker_get_title (GNOME_COLOR_PICKER (widget)),
	      dgettext (GLADE_LIBGNOMEUI_GETTEXT_PACKAGE, "Pick a color")))
    {
      gboolean translatable, context;
      gchar *comments;

      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  gnome_color_picker_set_title (GNOME_COLOR_PICKER (%s), %s);\n",
		  data->wname,
		  source_make_string_full (gnome_color_picker_get_title (GNOME_COLOR_PICKER (widget)),
					   data->use_gettext && translatable,
					   context));
    }

  if (!gnome_color_picker_get_dither (GNOME_COLOR_PICKER (widget)))
    {
      source_add (data,
		  "  gnome_color_picker_set_dither (GNOME_COLOR_PICKER (%s), FALSE);\n",
		  data->wname);

    }

  if (gnome_color_picker_get_use_alpha (GNOME_COLOR_PICKER (widget)))
    {
      source_add (data,
		  "  gnome_color_picker_set_use_alpha (GNOME_COLOR_PICKER (%s), TRUE);\n",
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
gb_gnome_color_picker_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_color_picker_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_colorpicker_xpm;
  gbwidget.tooltip = _("Gnome Color Picker");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties	= gb_gnome_color_picker_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_color_picker_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_color_picker_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_color_picker_write_source;
/*
  gbwidget.gb_widget_new		= gb_gnome_color_picker_new;
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_color_picker_create_popup_menu;
*/

  return &gbwidget;
}

