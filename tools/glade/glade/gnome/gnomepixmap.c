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

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-pixmap.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Filename = "GnomePixmap::filename";
static gchar *Width = "GnomePixmap::scaled_width";
static gchar *Height = "GnomePixmap::scaled_height";

/* This is only used in Glade. It is not saved to the XML since it is implied
   by the existence of the scaled_width & scaled_height properties. */
static gchar *Scaled = "GnomePixmap::scaled";

static void gb_gnome_pixmap_reload (GtkWidget *widget);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomePixmap, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_gnome_pixmap_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_type_new (gnome_pixmap_get_type ());
  /* Set the default scaled width to 48 x 48. */
  gtk_object_set_data (GTK_OBJECT (new_widget), Width, GINT_TO_POINTER (48));
  gtk_object_set_data (GTK_OBJECT (new_widget), Height, GINT_TO_POINTER (48));

  if (data->action == GB_CREATING)
    gnome_pixmap_load_xpm_d (GNOME_PIXMAP (new_widget),
			     (const char**) gnome_pixmap_xpm);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_pixmap_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_filename (Filename, _("File:"), _("The pixmap filename"));
  property_add_bool (Scaled, _("Scaled:"), _("If the pixmap is scaled"));
  property_add_int_range (Width, _("Scaled Width:"),
			  _("The width to scale the pixmap to"),
			  1, 10000, 1, 10, 1);
  property_add_int_range (Height, _("Scaled Height:"),
			  _("The height to scale the pixmap to"),
			  1, 10000, 1, 10, 1);
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_pixmap_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gboolean scaled;
  gint w, h;

  gb_widget_output_pixmap_filename (data, Filename,
				    gtk_object_get_data (GTK_OBJECT (widget),
							 Filename));

  scaled = gtk_object_get_data (GTK_OBJECT (widget), Scaled) != NULL
    ? TRUE : FALSE;
  if (data->action == GB_SHOWING)
    {
      gb_widget_output_bool (data, Scaled, scaled);
      property_set_sensitive (Width, scaled);
      property_set_sensitive (Height, scaled);
    }

  /* We only save the scaled width and height if the pixmap is scaled. */
  w = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), Width));
  if (scaled || data->action == GB_SHOWING)
    gb_widget_output_int (data, Width, w);

  h = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), Height));
  if (scaled || data->action == GB_SHOWING)
    gb_widget_output_int (data, Height, h);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_pixmap_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *filename, *old_filename;
  gboolean set_pixmap = FALSE, scaled;
  gint width, height;

  filename = gb_widget_input_pixmap_filename (data, Filename);
  if (data->apply)
    {
      set_pixmap = TRUE;
      if (filename && filename[0] == '\0')
	filename = NULL;

      old_filename = gtk_object_get_data (GTK_OBJECT (widget), Filename);
      if (old_filename)
	{
	  glade_project_remove_pixmap (data->project, old_filename);
	  g_free (old_filename);
	}

      gtk_object_set_data_full (GTK_OBJECT (widget), Filename,
				g_strdup (filename),
				filename ? g_free : NULL);
      if (filename)
	{
	  glade_project_add_pixmap (data->project, filename);
	}
    }
  if (data->action == GB_LOADING)
    g_free (filename);

  scaled = gb_widget_input_bool (data, Scaled);
  if (data->apply)
    {
      set_pixmap = TRUE;
      gtk_object_set_data (GTK_OBJECT (widget), Scaled, scaled ? "Y" : NULL);
      if (property_get_widget() == widget)
	{
	  property_set_sensitive (Width, scaled);
	  property_set_sensitive (Height, scaled);
	}
    }

  width = gb_widget_input_int (data, Width);
  if (data->apply)
    {
      set_pixmap = TRUE;
      if (data->action == GB_LOADING)
	gtk_object_set_data (GTK_OBJECT (widget), Scaled, "Y");
      gtk_object_set_data (GTK_OBJECT (widget), Width,
			   GINT_TO_POINTER (width));
    }

  height = gb_widget_input_int (data, Height);
  if (data->apply)
    {
      set_pixmap = TRUE;
      if (data->action == GB_LOADING)
	gtk_object_set_data (GTK_OBJECT (widget), Scaled, "Y");
      gtk_object_set_data (GTK_OBJECT (widget), Height,
			   GINT_TO_POINTER (height));
    }

  if (set_pixmap)
    gb_gnome_pixmap_reload (widget);
}


static void
gb_gnome_pixmap_reload (GtkWidget *widget)
{
  gchar *filename;
  gboolean scaled;
  gint width, height;

  filename = gtk_object_get_data (GTK_OBJECT (widget), Filename);
  scaled = gtk_object_get_data (GTK_OBJECT (widget), Scaled) != NULL
    ? TRUE : FALSE;
  width = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), Width));
  height = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), Height));

  if (filename)
    {
      if (scaled && width > 0 && height > 0)
	gnome_pixmap_load_file_at_size (GNOME_PIXMAP (widget), filename,
					width, height);
      else
	gnome_pixmap_load_file (GNOME_PIXMAP (widget), filename);
    }
  else
    {
      if (scaled && width > 0 && height > 0)
	gnome_pixmap_load_xpm_d_at_size (GNOME_PIXMAP (widget),
					 (const char**) gnome_pixmap_xpm,
					 width, height);
      else
	gnome_pixmap_load_xpm_d (GNOME_PIXMAP (widget),
				 (const char**) gnome_pixmap_xpm);
    }
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomePixmap, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_pixmap_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_pixmap_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *filename;
  gboolean scaled;
  gint width, height;

  filename = gtk_object_get_data (GTK_OBJECT (widget), Filename);
  if (filename && !*filename)
    filename = NULL;
  scaled = gtk_object_get_data (GTK_OBJECT (widget), Scaled) != NULL
    ? TRUE : FALSE;
  width = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), Width));
  height = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), Height));

  if (data->create_widget)
    {
      source_add (data,
		  "  %s = g_object_new (GNOME_TYPE_PIXMAP, NULL);\n",
		  data->wname);
    }

  if (filename)
    {
      filename = (gchar*) g_basename (filename);

      source_add_decl (data, "  gchar *%s_filename;\n", data->real_wname);
      /* FIXME: Should convert filename to a valid C string? */
      source_add (data,
		  "  %s_filename = gnome_program_locate_file (NULL,\n"
		  "    GNOME_FILE_DOMAIN_APP_PIXMAP, \"%s/%s\", TRUE, NULL);\n"
		  "  if (%s_filename)\n",
		  data->real_wname,
		  data->program_name, filename,
		  data->real_wname);

      if (scaled)
	{
	  source_add (data,
		      "    gnome_pixmap_load_file_at_size (GNOME_PIXMAP (%s), %s_filename, %i, %i);\n",
		      data->wname, data->real_wname, width, height);
	}
      else
	{
	  source_add (data,
		      "    gnome_pixmap_load_file (GNOME_PIXMAP (%s), %s_filename);\n",
		      data->wname, data->real_wname);
	}

      source_add (data,
		  "  else\n"
		  "    g_warning (%s, ",
		  source_make_string ("Couldn't find pixmap file: %s",
				      data->use_gettext));
      source_add (data,
		  "%s);\n",
		  source_make_string (filename, FALSE));

      source_add (data,  "  g_free (%s_filename);\n", data->real_wname);
    }

  gb_widget_write_standard_source (widget, data);
}


void
gb_gnome_pixmap_destroy (GtkWidget * widget, GbWidgetDestroyData * data)
{
  gchar *filename;

  /* This can be a stock id or a filename. But glade_project_remove_pixmap()
     will just ignore it if it isn't a project pixmap file. */
  filename = gtk_object_get_data (GTK_OBJECT (widget), Filename);
  glade_project_remove_pixmap (data->project, filename);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_pixmap_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_pixmap_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_pixmap_xpm;
  gbwidget.tooltip = _("Gnome Pixmap");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_pixmap_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_pixmap_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_pixmap_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_pixmap_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_pixmap_write_source;
  gbwidget.gb_widget_destroy		= gb_gnome_pixmap_destroy;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_pixmap_create_popup_menu;
*/

  return &gbwidget;
}

