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
#include "../graphics/colorbutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *UseAlpha = "GtkColorButton::use_alpha";
static gchar *Title = "GtkColorButton::title";
static gchar *FocusOnClick = "GtkColorButton|GtkButton::focus_on_click";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkColorButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
/*
static GtkWidget*
gb_color_button_new (GbWidgetNewData *data)
{

}
*/



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_color_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
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
gb_color_button_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  const gchar *title;

  gb_widget_output_bool (data, UseAlpha,
			 gtk_color_button_get_use_alpha (GTK_COLOR_BUTTON (widget)));

  /* Only save if the title is different to the default. */
  title = gtk_color_button_get_title (GTK_COLOR_BUTTON (widget));
  if (data->action == GB_SHOWING
      || (title && strcmp (title, dgettext (GLADE_GTK_GETTEXT_PACKAGE,
					    "Pick a Color"))))
    gb_widget_output_translatable_string (data, Title, title);

  gb_widget_output_bool (data, FocusOnClick,
			 gtk_button_get_focus_on_click (GTK_BUTTON (widget)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_color_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean use_alpha, focus_on_click;
  gchar *title;

  use_alpha = gb_widget_input_bool (data, UseAlpha);
  if (data->apply)
    gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (widget), use_alpha);

  title = gb_widget_input_string (data, Title);
  if (data->apply)
    gtk_color_button_set_title (GTK_COLOR_BUTTON (widget),
			       title && title[0]
			       ? title : dgettext (GLADE_GTK_GETTEXT_PACKAGE,
						   "Pick a Color"));

  focus_on_click = gb_widget_input_bool (data, FocusOnClick);
  if (data->apply)
    gtk_button_set_focus_on_click (GTK_BUTTON (widget), focus_on_click);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkColorButton, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_color_button_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_color_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  const gchar *title;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_color_button_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  title = gtk_color_button_get_title (GTK_COLOR_BUTTON (widget));
  if (title && title[0]
      && strcmp (title, dgettext (GLADE_GTK_GETTEXT_PACKAGE, "Pick a Color")))
    {
      gboolean translatable, context;
      gchar *comments;

      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  gtk_color_button_set_title (GTK_COLOR_BUTTON (%s), %s);\n",
		  data->wname,
		  source_make_string_full (title, data->use_gettext && translatable, context));
    }

  if (gtk_color_button_get_use_alpha (GTK_COLOR_BUTTON (widget)))
    {
      source_add (data,
		  "  gtk_color_button_set_use_alpha (GTK_COLOR_BUTTON (%s), TRUE);\n",
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
gb_color_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_color_button_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = colorbutton_xpm;
  gbwidget.tooltip = _("Color Chooser Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties	= gb_color_button_create_properties;
  gbwidget.gb_widget_get_properties	= gb_color_button_get_properties;
  gbwidget.gb_widget_set_properties	= gb_color_button_set_properties;
  gbwidget.gb_widget_write_source	= gb_color_button_write_source;
/*
  gbwidget.gb_widget_new		= gb_color_button_new;
  gbwidget.gb_widget_create_popup_menu	= gb_color_button_create_popup_menu;
*/

  return &gbwidget;
}

