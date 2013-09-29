/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998  Damon Chaplin
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

#include <math.h>

#include <gtk/gtkaccellabel.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/accellabel.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Label = "AccelLabel|GtkLabel::label";
static gchar *UseUnderline = "AccelLabel|GtkLabel::use_underline";
static gchar *UseMarkup = "AccelLabel|GtkLabel::use_markup";
static gchar *Justify = "AccelLabel|GtkLabel::justify";
static gchar *Wrap = "AccelLabel|GtkLabel::wrap";
static gchar *XAlign = "AccelLabel|GtkMisc::xalign";
static gchar *YAlign = "AccelLabel|GtkMisc::yalign";
static gchar *XPad = "AccelLabel|GtkMisc::xpad";
static gchar *YPad = "AccelLabel|GtkMisc::ypad";
static gchar *Selectable = "AccelLabel|GtkLabel::selectable";

static gchar *Ellipsize = "AccelLabel|GtkLabel::ellipsize";
static gchar *WidthChars = "AccelLabel|GtkLabel::width_chars";
static gchar *SingleLineMode = "AccelLabel|GtkLabel::single_line_mode";
static gchar *Angle = "AccelLabel|GtkLabel::angle";

static gchar *FocusTarget = "AccelLabel|GtkLabel::mnemonic_widget";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the funtion in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkAccelLabel, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget*
gb_accel_label_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_accel_label_new (data->name);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_accel_label_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  gb_label_create_standard_properties (widget, data,
				       Label, UseUnderline, UseMarkup,
				       Justify, Wrap, Selectable,
				       XAlign, YAlign, XPad, YPad,
				       FocusTarget, Ellipsize, WidthChars,
				       SingleLineMode, Angle);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_accel_label_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_label_get_standard_properties (widget, data,
				    Label, UseUnderline, UseMarkup,
				    Justify, Wrap, Selectable,
				    XAlign, YAlign, XPad, YPad,
				    FocusTarget, Ellipsize, WidthChars,
				    SingleLineMode, Angle);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_accel_label_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gb_label_set_standard_properties (widget, data,
				    Label, UseUnderline, UseMarkup,
				    Justify, Wrap, Selectable,
				    XAlign, YAlign, XPad, YPad,
				    FocusTarget, Ellipsize, WidthChars,
				    SingleLineMode, Angle);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkAccelLabel, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_accel_label_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_accel_label_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      const gchar *label_text = gtk_label_get_label (GTK_LABEL (widget));

      glade_util_get_translation_properties (widget, Label, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data, "  %s = gtk_accel_label_new (%s);\n", data->wname,
		  source_make_string_full (label_text,
					   data->use_gettext && translatable,
					   context));
    }

  gb_widget_write_standard_source (widget, data);

  if (gtk_label_get_use_underline (GTK_LABEL (widget)))
    source_add (data,
		"  gtk_label_set_use_underline (GTK_LABEL (%s), TRUE);\n",
		data->wname);

  gb_label_write_standard_source (widget, data,
				  Label, UseUnderline, UseMarkup,
				  Justify, Wrap, Selectable,
				  XAlign, YAlign, XPad, YPad,
				  FocusTarget, Ellipsize, WidthChars,
				  SingleLineMode, Angle);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_accel_label_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_accel_label_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = accellabel_xpm;
  gbwidget.tooltip = _("Label with Accelerator");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_accel_label_new;
  gbwidget.gb_widget_create_properties	= gb_accel_label_create_properties;
  gbwidget.gb_widget_get_properties	= gb_accel_label_get_properties;
  gbwidget.gb_widget_set_properties	= gb_accel_label_set_properties;
  gbwidget.gb_widget_write_source	= gb_accel_label_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_accel_label_create_popup_menu;
*/

  return &gbwidget;
}

