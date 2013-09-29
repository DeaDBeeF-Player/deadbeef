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
#include "../graphics/gnome-iconlist.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

#define DEFAULT_ICON_WIDTH 78

/* These are pinched from GnomeIconList */
#define DEFAULT_ROW_SPACING  4
#define DEFAULT_COL_SPACING  2
#define DEFAULT_TEXT_SPACING 2
#define DEFAULT_ICON_BORDER  2

static gchar *SelectionMode = "GnomeIconList::selection_mode";
static gchar *IconWidth = "GnomeIconList::icon_width";
static gchar *RowSpacing = "GnomeIconList::row_spacing";
static gchar *ColSpacing = "GnomeIconList::column_spacing";
#if 0
/* The icon border property doesn't seem to be used. */
static gchar *IconBorder = "GnomeIconList::icon_border";
#endif
static gchar *TextSpacing = "GnomeIconList::text_spacing";
static gchar *TextEditable = "GnomeIconList::text_editable";
static gchar *TextStatic = "GnomeIconList::text_static";

static const gchar *GbModeChoices[] =
{"Single", "Browse", "Multiple", NULL};
static const gint GbModeValues[] =
{
  GTK_SELECTION_SINGLE,
  GTK_SELECTION_BROWSE,
  GTK_SELECTION_MULTIPLE
};
static const gchar *GbModeSymbols[] =
{
  "GTK_SELECTION_SINGLE",
  "GTK_SELECTION_BROWSE",
  "GTK_SELECTION_MULTIPLE"
};

static gint gb_gnome_icon_list_expose (GtkWidget      *widget, 
				       GdkEventExpose *event,
				       gpointer	  data);
static void gb_gnome_icon_list_adjustment_changed (GtkAdjustment *adjustment,
						   GtkWidget     *widget);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeIconList, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_gnome_icon_list_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;
  GdkPixbuf *image;
  gint i;

  new_widget = gnome_icon_list_new (DEFAULT_ICON_WIDTH, NULL, 0);

  image = gdk_pixbuf_new_from_xpm_data ((const char**) gnome_iconlist_xpm);
  for (i = 0; i < 10; i++)
    {
      gnome_icon_list_append_pixbuf (GNOME_ICON_LIST (new_widget), image,
				     NULL,
				     "This is an example icon");
    }
  gdk_pixbuf_unref (image);
  /* We save the property values in the GnomeIconList's data list, since they
     are not available from GnomeIconList in any way.
     We let TextEditable and TextStatic default to FALSE so we don't need to
     set them here. */
  gtk_object_set_data (GTK_OBJECT (new_widget), SelectionMode,
		       GINT_TO_POINTER (GTK_SELECTION_SINGLE));
  gtk_object_set_data (GTK_OBJECT (new_widget), IconWidth,
		       GINT_TO_POINTER (DEFAULT_ICON_WIDTH));
  gtk_object_set_data (GTK_OBJECT (new_widget), RowSpacing,
		       GINT_TO_POINTER (DEFAULT_ROW_SPACING));
  gtk_object_set_data (GTK_OBJECT (new_widget), ColSpacing,
		       GINT_TO_POINTER (DEFAULT_COL_SPACING));
#if 0
  gtk_object_set_data (GTK_OBJECT (new_widget), IconBorder,
		       GINT_TO_POINTER (DEFAULT_ICON_BORDER));
#endif
  gtk_object_set_data (GTK_OBJECT (new_widget), TextSpacing,
		       GINT_TO_POINTER (DEFAULT_TEXT_SPACING));

  /* We connect to the expose event so we can connect to the "value_changed"
     signals of the scrollbar adjustments. The scrollbars aren't setup until
     the widget is added to a scrolled window. */
  gtk_signal_connect (GTK_OBJECT (new_widget), "expose_event",
		      (GtkSignalFunc) gb_gnome_icon_list_expose, NULL);

  return new_widget;
}


/* If data is not NULL, then we are creating the widget, so the step increments
   are set to a decent initial value. */
static gint
gb_gnome_icon_list_expose (GtkWidget      *widget, 
			   GdkEventExpose *event,
			   gpointer	  data)
{
  if (GNOME_ICON_LIST (widget)->adj)
    gtk_signal_connect (GTK_OBJECT (GNOME_ICON_LIST (widget)->adj),
			"value_changed",
			(GtkSignalFunc) gb_gnome_icon_list_adjustment_changed,
			widget);
  if (GNOME_ICON_LIST (widget)->hadj)
    gtk_signal_connect (GTK_OBJECT (GNOME_ICON_LIST (widget)->hadj),
			"value_changed",
			(GtkSignalFunc) gb_gnome_icon_list_adjustment_changed,
			widget);

  /* Disconnect this handler since we don't need it any more. */
  gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
				 (GtkSignalFunc) gb_gnome_icon_list_expose,
				 data);
  return FALSE;
}


static void
gb_gnome_icon_list_adjustment_changed (GtkAdjustment *adjustment,
				       GtkWidget     *widget)
{
  /* Just queue a complete clear for now. It's good enough. */
  gtk_widget_queue_clear (widget);
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_icon_list_create_properties (GtkWidget * widget,
				      GbWidgetCreateArgData * data)
{
  property_add_choice (SelectionMode, _("Select Mode:"),
		       _("The selection mode"),
		       GbModeChoices);
  property_add_int_range (IconWidth, _("Icon Width:"),
			  _("The width of each icon"),
			  1, 10000, 1, 10, 1);
  property_add_int_range (RowSpacing, _("Row Spacing:"),
			  _("The number of pixels between rows of icons"),
			  0, 10000, 1, 10, 1);
  property_add_int_range (ColSpacing, _("Col Spacing:"),
			  _("The number of pixels between columns of icons"),
			  0, 10000, 1, 10, 1);
#if 0
  property_add_int_range (IconBorder, _("Icon Border:"),
			  _("The number of pixels around icons (unused?)"),
			  0, 10000, 1, 10, 1);
#endif
  property_add_int_range (TextSpacing, _("Text Spacing:"),
			  _("The number of pixels between the text and the icon"),
			  0, 10000, 1, 10, 1);
  property_add_bool (TextEditable, _("Text Editable:"),
		     _("If the icon text can be edited by the user"));
  property_add_bool (TextStatic, _("Text Static:"),
		     _("If the icon text is static, in which case it will not be copied by the GnomeIconList"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_icon_list_get_properties (GtkWidget *widget,
				   GbWidgetGetArgData * data)
{
  GnomeIconList *gil;
  gint selection_mode, i;

  gil = (GnomeIconList*) widget;

  selection_mode = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
							 SelectionMode));
  for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
    {
      if (GbModeValues[i] == selection_mode)
	gb_widget_output_choice (data, SelectionMode, i, GbModeSymbols[i]);
    }

  gb_widget_output_int (data, IconWidth, GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), IconWidth)));
  gb_widget_output_int (data, RowSpacing, GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), RowSpacing)));
  gb_widget_output_int (data, ColSpacing, GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), ColSpacing)));
#if 0
  gb_widget_output_int (data, IconBorder, GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), IconBorder)));
#endif
  gb_widget_output_int (data, TextSpacing, GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), TextSpacing)));
  gb_widget_output_bool (data, TextEditable, GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), TextEditable)));
  gb_widget_output_bool (data, TextStatic, GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), TextStatic)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_icon_list_set_properties (GtkWidget * widget,
				   GbWidgetSetArgData * data)
{
  gchar *mode;
  gint icon_width, row_spacing, col_spacing, /*icon_border,*/ text_spacing, i;
  gboolean text_editable, text_static;

  mode = gb_widget_input_choice (data, SelectionMode);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
	{
	  if (!strcmp (mode, GbModeChoices[i])
	      || !strcmp (mode, GbModeSymbols[i]))
	    {
	      gnome_icon_list_set_selection_mode (GNOME_ICON_LIST (widget),
						  GbModeValues[i]);
	      gtk_object_set_data (GTK_OBJECT (widget), SelectionMode,
				   GINT_TO_POINTER (GbModeValues[i]));
	      break;
	    }
	}
    }

  icon_width = gb_widget_input_int (data, IconWidth);
  if (data->apply)
    {
      gnome_icon_list_set_icon_width (GNOME_ICON_LIST (widget), icon_width);
      gtk_object_set_data (GTK_OBJECT (widget), IconWidth,
			   GINT_TO_POINTER (icon_width));
    }

  row_spacing = gb_widget_input_int (data, RowSpacing);
  if (data->apply)
    {
      gnome_icon_list_set_row_spacing (GNOME_ICON_LIST (widget), row_spacing);
      gtk_object_set_data (GTK_OBJECT (widget), RowSpacing,
			   GINT_TO_POINTER (row_spacing));
    }

  col_spacing = gb_widget_input_int (data, ColSpacing);
  if (data->apply)
    {
      gnome_icon_list_set_col_spacing (GNOME_ICON_LIST (widget), col_spacing);
      gtk_object_set_data (GTK_OBJECT (widget), ColSpacing,
			   GINT_TO_POINTER (col_spacing));
    }

#if 0
  icon_border = gb_widget_input_int (data, IconBorder);
  if (data->apply)
    {
      gnome_icon_list_set_icon_border (GNOME_ICON_LIST (widget), icon_border);
      gtk_object_set_data (GTK_OBJECT (widget), IconBorder,
			   GINT_TO_POINTER (icon_border));
    }
#endif

  text_spacing = gb_widget_input_int (data, TextSpacing);
  if (data->apply)
    {
      gnome_icon_list_set_text_spacing (GNOME_ICON_LIST (widget),
					text_spacing);
      gtk_object_set_data (GTK_OBJECT (widget), TextSpacing,
			   GINT_TO_POINTER (text_spacing));
    }

  text_editable = gb_widget_input_bool (data, TextEditable);
  if (data->apply)
    {
      /* GnomeIconList doesn't have a function to set this. We may also want
	 to reconfigure any example icon items so this works. */
      gtk_object_set_data (GTK_OBJECT (widget), TextEditable,
			   GINT_TO_POINTER (text_editable));
    }

  text_static = gb_widget_input_bool (data, TextStatic);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), TextStatic,
			   GINT_TO_POINTER (text_static));
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeIconList, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_icon_list_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_icon_list_write_source (GtkWidget * widget,
				 GbWidgetWriteSourceData * data)
{
  gint icon_width, row_spacing, col_spacing, /*icon_border,*/ text_spacing;
  gint i, selection_mode;
  gboolean text_editable, text_static;
  gchar *flags;

  icon_width = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						     IconWidth));
  text_editable = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
							TextEditable));
  text_static = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						      TextStatic));

  if (data->create_widget)
    {
      if (text_editable && text_static)
	flags = "GNOME_ICON_LIST_IS_EDITABLE | GNOME_ICON_LIST_STATIC_TEXT";
      else if (text_editable)
	flags = "GNOME_ICON_LIST_IS_EDITABLE";
      else if (text_static)
	flags = "GNOME_ICON_LIST_STATIC_TEXT";
      else
	flags = "0";

      source_add (data, "  %s = gnome_icon_list_new (%i, NULL, %s);\n",
		  data->wname, icon_width, flags);
    }

  gb_widget_write_standard_source (widget, data);

  if (!data->create_widget)
    {
      if (icon_width != DEFAULT_ICON_WIDTH)
	{
	  source_add (data,
		      "  gnome_icon_list_set_icon_width (GNOME_ICON_LIST (%s), %i);\n",
		      data->wname, icon_width);
	}
    }

  row_spacing = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						      RowSpacing));
  if (row_spacing != DEFAULT_ROW_SPACING)
    {
      source_add (data,
		  "  gnome_icon_list_set_row_spacing (GNOME_ICON_LIST (%s), %i);\n",
		  data->wname, row_spacing);
    }

  col_spacing = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						      ColSpacing));
  if (col_spacing != DEFAULT_COL_SPACING)
    {
      source_add (data,
		  "  gnome_icon_list_set_col_spacing (GNOME_ICON_LIST (%s), %i);\n",
		  data->wname, col_spacing);
    }

#if 0
  icon_border = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						      IconBorder));
  if (icon_border != DEFAULT_ICON_BORDER)
    {
      source_add (data,
		  "  gnome_icon_list_set_icon_border (GNOME_ICON_LIST (%s), %i);\n",
		  data->wname, icon_border);
    }
#endif

  text_spacing = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						       TextSpacing));
  if (text_spacing != DEFAULT_TEXT_SPACING)
    {
      source_add (data,
		  "  gnome_icon_list_set_text_spacing (GNOME_ICON_LIST (%s), %i);\n",
		  data->wname, text_spacing);
    }

  selection_mode = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
							 SelectionMode));
  if (selection_mode != GTK_SELECTION_SINGLE)
    {
      for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
	{
	  if (GbModeValues[i] == selection_mode)
	    source_add (data,
			"  gnome_icon_list_set_selection_mode (GNOME_ICON_LIST (%s), %s);\n",
			data->wname, GbModeSymbols[i]);
	}
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_icon_list_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_icon_list_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_iconlist_xpm;
  gbwidget.tooltip = _("Icon List");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_icon_list_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_icon_list_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_icon_list_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_icon_list_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_icon_list_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_icon_list_create_popup_menu;
*/

  return &gbwidget;
}

