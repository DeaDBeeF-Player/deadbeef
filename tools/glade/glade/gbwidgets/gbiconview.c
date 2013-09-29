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
#include "../graphics/iconview.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *SelectionMode = "GtkIconView::selection_mode";
static gchar *Orientation = "GtkIconView::orientation";
static gchar *Reorderable = "GtkIconView::reorderable";


static const gchar *GbModeChoices[] =
{"None", "Single", "Browse", "Multiple", NULL};
static const gint GbModeValues[] =
{
  GTK_SELECTION_NONE,
  GTK_SELECTION_SINGLE,
  GTK_SELECTION_BROWSE,
  GTK_SELECTION_MULTIPLE
};
static const gchar *GbModeSymbols[] =
{
  "GTK_SELECTION_NONE",
  "GTK_SELECTION_SINGLE",
  "GTK_SELECTION_BROWSE",
  "GTK_SELECTION_MULTIPLE"
};


static const gchar *GbOrientationChoices[] =
{"Horizontal", "Vertical", NULL};
static const gint GbOrientationValues[] =
{
  GTK_ORIENTATION_HORIZONTAL,
  GTK_ORIENTATION_VERTICAL
};
static const gchar *GbOrientationSymbols[] =
{
  "GTK_ORIENTATION_HORIZONTAL",
  "GTK_ORIENTATION_VERTICAL"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkIconView, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_icon_view_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;
  GtkListStore *store;
  GtkTreeIter iter;
  GbWidget *gbwidget;
  GdkPixbuf *pixbuf;
  char buf[256];
  gint i;

  new_widget = gtk_icon_view_new ();

  /* Set up a dummy model so the user sees something. */
  store = gtk_list_store_new (2, G_TYPE_STRING, GDK_TYPE_PIXBUF);

  gbwidget = gb_widget_lookup_class ("GtkImage");
  if (gbwidget->pixbuf == NULL && gbwidget->pixmap_struct)
    {
      gbwidget->pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) gbwidget->pixmap_struct);
    }
  pixbuf = gbwidget->pixbuf;

  for (i = 1; i <= 8; i++)
    {
      gtk_list_store_append (store, &iter);
      sprintf (buf, _("Icon %i"), i);
      gtk_list_store_set (store, &iter, 0, buf, 1, pixbuf, -1);
    }

  gtk_icon_view_set_model (GTK_ICON_VIEW (new_widget), GTK_TREE_MODEL (store));
  gtk_icon_view_set_text_column   (GTK_ICON_VIEW (new_widget), 0);
  gtk_icon_view_set_pixbuf_column   (GTK_ICON_VIEW (new_widget), 1);

  g_object_unref (G_OBJECT (store));

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_icon_view_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (SelectionMode, _("Select Mode:"),
		       _("The selection mode of the icon view"),
		       GbModeChoices);
  property_add_choice (Orientation, _("Orientation:"),
		       _("The orientation of the icons"),
		       GbOrientationChoices);
  property_add_bool (Reorderable, _("Reorderable:"),
		     _("If the view can be reordered using Drag and Drop"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_icon_view_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  GtkSelectionMode mode;
  GtkOrientation orientation;
  gboolean reorderable;
  gint i;

  mode = gtk_icon_view_get_selection_mode (GTK_ICON_VIEW (widget));
  orientation = gtk_icon_view_get_orientation (GTK_ICON_VIEW (widget));

  for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
    {
      if (GbModeValues[i] == mode)
	gb_widget_output_choice (data, SelectionMode, i, GbModeSymbols[i]);
    }

  for (i = 0; i < sizeof (GbOrientationValues) / sizeof (GbOrientationValues[0]); i++)
    {
      if (GbOrientationValues[i] == orientation)
	gb_widget_output_choice (data, Orientation, i, GbOrientationSymbols[i]);
    }

  reorderable = gtk_icon_view_get_reorderable (GTK_ICON_VIEW (widget));
  gb_widget_output_bool (data, Reorderable, reorderable);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_icon_view_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *mode, *orientation;
  gboolean reorderable;
  gint i;

  mode = gb_widget_input_choice (data, SelectionMode);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
	{
	  if (!strcmp (mode, GbModeChoices[i])
	      || !strcmp (mode, GbModeSymbols[i]))
	    {
	      gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (widget), GbModeValues[i]);
	      break;
	    }
	}
    }

  orientation = gb_widget_input_choice (data, Orientation);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbOrientationValues) / sizeof (GbOrientationValues[0]); i++)
	{
	  if (!strcmp (orientation, GbOrientationChoices[i])
	      || !strcmp (orientation, GbOrientationSymbols[i]))
	    {
	      gtk_icon_view_set_orientation (GTK_ICON_VIEW (widget),
					     GbOrientationValues[i]);
	      break;
	    }
	}
    }

  reorderable = gb_widget_input_bool (data, Reorderable);
  if (data->apply)
    gtk_icon_view_set_reorderable (GTK_ICON_VIEW (widget), reorderable);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkIconView, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_icon_view_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_icon_view_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkSelectionMode mode;
  GtkOrientation orientation;
  gboolean reorderable;
  gint i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_icon_view_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  mode = gtk_icon_view_get_selection_mode (GTK_ICON_VIEW (widget));
  orientation = gtk_icon_view_get_orientation (GTK_ICON_VIEW (widget));

  if (mode != GTK_SELECTION_SINGLE)
    {
      for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
	{
	  if (GbModeValues[i] == mode)
	    source_add (data,
			"  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (%s), %s);\n",
			data->wname, GbModeSymbols[i]);
	}
    }

  if (orientation != GTK_ORIENTATION_VERTICAL)
    {
      for (i = 0; i < sizeof (GbOrientationValues) / sizeof (GbOrientationValues[0]); i++)
	{
	  if (GbOrientationValues[i] == orientation)
	    source_add (data,
			"  gtk_icon_view_set_orientation (GTK_ICON_VIEW (%s), %s);\n",
			data->wname, GbOrientationSymbols[i]);
	}
    }

  reorderable = gtk_icon_view_get_reorderable (GTK_ICON_VIEW (widget));
  if (reorderable)
    {
      source_add (data,
		  "  gtk_icon_view_set_reorderable (GTK_ICON_VIEW (%s), TRUE);\n",
			data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_icon_view_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_icon_view_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = iconview_xpm;
  gbwidget.tooltip = _("Icon View");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_icon_view_new;
  gbwidget.gb_widget_create_properties	= gb_icon_view_create_properties;
  gbwidget.gb_widget_get_properties	= gb_icon_view_get_properties;
  gbwidget.gb_widget_set_properties	= gb_icon_view_set_properties;
  gbwidget.gb_widget_write_source	= gb_icon_view_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_icon_view_create_popup_menu;
*/

  return &gbwidget;
}

