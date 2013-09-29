/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999  Damon Chaplin
 *
 *  calendar gbWidget Copyright (c) 1999 Jay Johnston
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

#include <gtk/gtkcalendar.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/calendar.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *ShowHeading = "GtkCalendar::show_heading";
static gchar *ShowDayNames = "GtkCalendar::show_day_names";
static gchar *NoMonthChange = "GtkCalendar::no_month_change";
static gchar *ShowWeekNumbers = "GtkCalendar::show_week_numbers";
static gchar *WeekStartMonday = "GtkCalendar::week_start_monday";

/* When saving, we save all 5 boolean flags in this property. */
static gchar *DisplayOptions = "GtkCalendar::display_options";

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the funtion in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkCalendar, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_calendar_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget = NULL;
  
  new_widget = gtk_calendar_new();
  
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */

static void
gb_calendar_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (ShowHeading, _("Heading:"), 
                     _("If the month and year should be shown at the top"));
  property_add_bool (ShowDayNames, _("Day Names:"), 
                     _("If the day names should be shown"));
  property_add_bool (NoMonthChange, _("Fixed Month:"), 
                     _("If the month and year shouldn't be changeable"));
  property_add_bool (ShowWeekNumbers, _("Week Numbers:"), 
                     _("If the number of the week should be shown"));
  property_add_bool (WeekStartMonday, _("Monday First:"), 
                     _("If the week should start on Monday"));
}




/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */

static void
gb_calendar_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  GtkCalendarDisplayOptions options;

  options = GTK_CALENDAR (widget)->display_flags;

  /* We save all 5 boolean toggles as one property. */
  if (data->action == GB_SAVING) {
    char *s = glade_util_string_from_flags (GTK_TYPE_CALENDAR_DISPLAY_OPTIONS,
					    options);
    gb_widget_output_string (data, DisplayOptions, s);
    g_free (s);
  } else {
    gb_widget_output_bool (data, ShowHeading,
			   options & GTK_CALENDAR_SHOW_HEADING);
    gb_widget_output_bool (data, ShowDayNames,
			   options & GTK_CALENDAR_SHOW_DAY_NAMES);
    gb_widget_output_bool (data, NoMonthChange,
			   options & GTK_CALENDAR_NO_MONTH_CHANGE);
    gb_widget_output_bool (data, ShowWeekNumbers,
			   options & GTK_CALENDAR_SHOW_WEEK_NUMBERS);
    gb_widget_output_bool (data, WeekStartMonday,
			   options & GTK_CALENDAR_WEEK_START_MONDAY);
  }
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */

static void
gb_calendar_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GtkCalendarDisplayOptions options;
  gboolean value;

  options = GTK_CALENDAR (widget)->display_flags;

  /* We load all 5 boolean toggles as one property. */
  if (data->action == GB_LOADING) {
    char *s = gb_widget_input_string (data, DisplayOptions);
    if (data->apply)
      options = glade_util_flags_from_string (GTK_TYPE_CALENDAR_DISPLAY_OPTIONS, s);
  } else {
    value = gb_widget_input_bool (data, ShowHeading)
      ? GTK_CALENDAR_SHOW_HEADING : 0;
    if (data->apply)
      options = (options & ~GTK_CALENDAR_SHOW_HEADING) | value;

    value = gb_widget_input_bool (data, ShowDayNames)
      ? GTK_CALENDAR_SHOW_DAY_NAMES : 0;
    if (data->apply)
      options = (options & ~GTK_CALENDAR_SHOW_DAY_NAMES) | value;

    value = gb_widget_input_bool (data, NoMonthChange)
      ? GTK_CALENDAR_NO_MONTH_CHANGE : 0;
    if (data->apply)
      options = (options & ~GTK_CALENDAR_NO_MONTH_CHANGE) | value;

    value = gb_widget_input_bool (data, ShowWeekNumbers)
      ? GTK_CALENDAR_SHOW_WEEK_NUMBERS : 0;
    if (data->apply)
      options = (options & ~GTK_CALENDAR_SHOW_WEEK_NUMBERS) | value;

    value = gb_widget_input_bool (data, WeekStartMonday)
      ? GTK_CALENDAR_WEEK_START_MONDAY : 0;
    if (data->apply)
      options = (options & ~GTK_CALENDAR_WEEK_START_MONDAY) | value;
  }

  if (options != GTK_CALENDAR (widget)->display_flags)
    gtk_calendar_display_options (GTK_CALENDAR (widget), options);
}




/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkCalendar, with signals pointing to
 * other functions in this file.
 */
/*
  static void
  gb_calendar_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
  {
  
  }
*/


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */

static void
gb_calendar_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkCalendarDisplayOptions options = 0;
  const gchar *prefix = "\n                                ";
  const gchar *prefix2 = "\n                                | ";

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_calendar_new ();\n", data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  source_add (data, "  gtk_calendar_display_options (GTK_CALENDAR (%s),",
	      data->wname);

  options = GTK_CALENDAR (widget)->display_flags;
  if (options)
    {
      if (options & GTK_CALENDAR_SHOW_HEADING)
	{
	  source_add (data, "%sGTK_CALENDAR_SHOW_HEADING", prefix);
	  prefix = prefix2;
	}

      if (options & GTK_CALENDAR_SHOW_DAY_NAMES)
	{
	  source_add (data, "%sGTK_CALENDAR_SHOW_DAY_NAMES", prefix);
	  prefix = prefix2;
	}

      if (options & GTK_CALENDAR_NO_MONTH_CHANGE)
	{
	  source_add (data, "%sGTK_CALENDAR_NO_MONTH_CHANGE", prefix);
	  prefix = prefix2;
	}

      if (options & GTK_CALENDAR_SHOW_WEEK_NUMBERS)
	{
	  source_add (data, "%sGTK_CALENDAR_SHOW_WEEK_NUMBERS", prefix);
	  prefix = prefix2;
	}

      if (options & GTK_CALENDAR_WEEK_START_MONDAY)
	{
	  source_add (data, "%sGTK_CALENDAR_WEEK_START_MONDAY", prefix);
	}
      source_add (data, ");\n");
    }
  else
    source_add (data, " 0);\n");
}




/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_calendar_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_calendar_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = calendar_xpm;
  gbwidget.tooltip = _("Calendar");

  /* Fill in any functions that this GbWidget has */

  gbwidget.gb_widget_new		= gb_calendar_new;
  gbwidget.gb_widget_create_properties	= gb_calendar_create_properties;
  gbwidget.gb_widget_get_properties	= gb_calendar_get_properties;
  gbwidget.gb_widget_set_properties	= gb_calendar_set_properties; 
  /* gbwidget.gb_widget_create_popup_menu  = gb_calendar_create_popup_menu; */
  gbwidget.gb_widget_write_source	= gb_calendar_write_source;


  return &gbwidget;
}

