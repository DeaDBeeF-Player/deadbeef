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

#include <libgnomedb/gnome-db-gray-bar.h>
#include <gtk/gtkentry.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-db-graybar.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;


static gchar *Text = "GnomeDbGrayBar::text";
/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeDbGrayBar, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_gnome_db_gray_bar_new (GbWidgetNewData *data)
{
  return gnome_db_gray_bar_new ("");
}

/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_db_gray_bar_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_string (Text, _("Text:"), _("Text inside the gray bar"));
}

/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_db_gray_bar_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gchar *wtext;
  wtext = (gchar *)gnome_db_gray_bar_get_text (GNOME_DB_GRAY_BAR(widget));
  gb_widget_output_translatable_string (data, Text, wtext);
}

/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_db_gray_bar_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  const gchar *text;

  text = gb_widget_input_string (data, Text);
  if (data->apply)
    gnome_db_gray_bar_set_text (GNOME_DB_GRAY_BAR (widget), text);
}



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_db_gray_bar_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  const gchar *text;
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      text = gnome_db_gray_bar_get_text(GNOME_DB_GRAY_BAR (widget));

      glade_util_get_translation_properties (widget, Text, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data, "  %s = gnome_db_gray_bar_new (%s);\n",
		  data->wname,
		  source_make_string_full (text,
					   data->use_gettext && translatable,
					   context));
    }
  gb_widget_write_standard_source (widget, data);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_db_gray_bar_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_db_gray_bar_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_db_graybar_xpm;
  gbwidget.tooltip = _("Gray Bar");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_db_gray_bar_new;
  gbwidget.gb_widget_write_source	= gb_gnome_db_gray_bar_write_source;
  gbwidget.gb_widget_create_properties	= gb_gnome_db_gray_bar_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_db_gray_bar_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_db_gray_bar_set_properties;

  return &gbwidget;
}

