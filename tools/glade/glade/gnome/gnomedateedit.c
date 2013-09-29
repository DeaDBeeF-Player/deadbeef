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
#include "../graphics/gnome-dateedit.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *DateEditFlags = "GnomeDateEdit::dateedit_flags";
static gchar *ShowTime = "GnomeDateEdit::show_time";
static gchar *Use24Format = "GnomeDateEdit::use_24_format";
static gchar *WeekStartMonday = "GnomeDateEdit::week_start_monday";
static gchar *LowerHour = "GnomeDateEdit::lower_hour";
static gchar *UpperHour = "GnomeDateEdit::upper_hour";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeDateEdit, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_gnome_date_edit_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gnome_date_edit_new ((time_t) 0, TRUE, TRUE);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_date_edit_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (ShowTime, _("Show Time:"), 
                     _("If the time is shown as well as the date"));
  property_add_bool (Use24Format, _("24 Hour Format:"), 
                     _("If the time is shown in 24-hour format"));
  property_add_bool (WeekStartMonday, _("Monday First:"), 
                     _("If the week should start on Monday"));
  property_add_int_range (LowerHour, _("Lower Hour:"),
			  _("The lowest hour to show in the popup"),
			  0, 23, 1, 10, 1);
  property_add_int_range (UpperHour, _("Upper Hour:"),
			  _("The highest hour to show in the popup"),
			  0, 23, 1, 10, 1);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_date_edit_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  GnomeDateEditFlags flags;
  int lower_hour, upper_hour;

  flags = gnome_date_edit_get_flags (GNOME_DATE_EDIT (widget));

  /* we save only one property, but the property editor has 3 toggle
     buttons */
  if (data->action == GB_SAVING) {
    char *s = glade_util_string_from_flags (GNOME_TYPE_DATE_EDIT_FLAGS, flags);
    gb_widget_output_string (data, DateEditFlags, s);
    g_free (s);
  } else {
    gb_widget_output_bool (data, ShowTime,
			   flags & GNOME_DATE_EDIT_SHOW_TIME);
    gb_widget_output_bool (data, Use24Format,
			   flags & GNOME_DATE_EDIT_24_HR);
    gb_widget_output_bool (data, WeekStartMonday,
			   flags & GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY);
  }

  g_object_get (G_OBJECT (widget),
		"lower-hour", &lower_hour,
		"upper-hour", &upper_hour,
		NULL);

  gb_widget_output_int (data, LowerHour, lower_hour);
  gb_widget_output_int (data, UpperHour, upper_hour);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_date_edit_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GnomeDateEditFlags flags, old_flags;
  gboolean value;
  gint value_int, lower_hour, old_lower_hour, upper_hour, old_upper_hour;

  old_flags = flags = gnome_date_edit_get_flags (GNOME_DATE_EDIT (widget));

  /* we load only one property, but the property editor has 3 toggle
     buttons */
  if (data->action == GB_LOADING) {
    char *s = gb_widget_input_string (data, DateEditFlags);
    if (data->apply)
      flags = glade_util_flags_from_string (GNOME_TYPE_DATE_EDIT_FLAGS, s);
  } else {
    value = gb_widget_input_bool (data, ShowTime)
      ? GNOME_DATE_EDIT_SHOW_TIME : 0;
    if (data->apply)
      flags = (flags & ~GNOME_DATE_EDIT_SHOW_TIME) | value;

    value = gb_widget_input_bool (data, Use24Format)
      ? GNOME_DATE_EDIT_24_HR : 0;
    if (data->apply)
      flags = (flags & ~GNOME_DATE_EDIT_24_HR) | value;

    value = gb_widget_input_bool (data, WeekStartMonday)
      ? GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY : 0;
    if (data->apply)
      flags = (flags & ~GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY) | value;
  }

  if (flags != old_flags)
    gnome_date_edit_set_flags (GNOME_DATE_EDIT (widget), flags);

  g_object_get (G_OBJECT (widget),
		"lower-hour", &old_lower_hour,
		"upper-hour", &old_upper_hour,
		NULL);

  value_int = gb_widget_input_int (data, LowerHour);
  if (data->apply)
    lower_hour = value_int;
  else
    lower_hour = old_lower_hour;

  value_int = gb_widget_input_int (data, UpperHour);
  if (data->apply)
    upper_hour = value_int;
  else
    upper_hour = old_upper_hour;

  if (lower_hour != old_lower_hour || upper_hour != old_upper_hour)
    {
      gnome_date_edit_set_popup_range (GNOME_DATE_EDIT (widget), lower_hour,
				       upper_hour);
    }
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeDateEdit, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_date_edit_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_date_edit_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GnomeDateEditFlags flags;
  gboolean set_flags = FALSE;
  int upper_hour, lower_hour;

  flags = gnome_date_edit_get_flags (GNOME_DATE_EDIT (widget));

  if (data->create_widget)
    {
      source_add (data, "  %s = gnome_date_edit_new ((time_t) 0, %s, %s);\n",
		  data->wname,
		  flags & GNOME_DATE_EDIT_SHOW_TIME ? "TRUE" : "FALSE",
		  flags & GNOME_DATE_EDIT_24_HR ? "TRUE" : "FALSE");
    }
  else
    {
      set_flags = TRUE;
    }

  gb_widget_write_standard_source (widget, data);

  if (flags & GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY)
    set_flags = TRUE;

  if (set_flags)
    {
      const gchar *prefix = "\n                             ";
      const gchar *prefix2 = "\n                             | ";

      source_add (data, "  gnome_date_edit_set_flags (GNOME_DATE_EDIT (%s),",
		  data->wname);
      if (flags)
	{
	  if (flags & GNOME_DATE_EDIT_SHOW_TIME)
	    {
	      source_add (data, "%sGNOME_DATE_EDIT_SHOW_TIME", prefix);
	      prefix = prefix2;
	    }
	  if (flags & GNOME_DATE_EDIT_24_HR)
	    {
	      source_add (data, "%sGNOME_DATE_EDIT_24_HR", prefix);
	      prefix = prefix2;
	    }
	  if (flags & GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY)
	    {
	      source_add (data, "%sGNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY",
			  prefix);
	    }
	  source_add (data, ");\n");
	}
      else
	source_add (data, "0);\n");
    }

  g_object_get (G_OBJECT (widget),
		"lower-hour", &lower_hour,
		"upper-hour", &upper_hour,
		NULL);

  if (lower_hour != 7 || upper_hour != 19)
      
    {
      source_add (data, "  gnome_date_edit_set_popup_range (GNOME_DATE_EDIT (%s), %i, %i);\n",
		  data->wname,
		  lower_hour,
		  upper_hour);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_date_edit_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_date_edit_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_dateedit_xpm;
  gbwidget.tooltip = _("GnomeDateEdit");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_date_edit_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_date_edit_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_date_edit_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_date_edit_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_date_edit_write_source;
/*
  gbwidget.gb_widget_set_child_props	= gb_gnome_date_edit_set_child_props;
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_date_edit_create_popup_menu;
*/

  return &gbwidget;
}

