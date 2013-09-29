/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998  Damon Chaplin
 *
 *  Update 3/17/99 - Jay Johnston:
 *  - added property adjustments
 *    - style
 *    - orientation
 *    - activity mode
 *    - show text
 *  - added sample animation (most of that code was taken from the Gtk v1.2
 *      tutorial by Tony Gale <gale@gtk.org> and Ian Main <imain@gtk.org> )
 *  - added dynamic popup menus which change depending on whether the animation
 *      is active or not
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
#include <gtk/gtkprogressbar.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/progressbar.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Orientation = "GtkProgressBar::orientation";
static gchar *Fraction = "GtkProgressBar::fraction";
static gchar *PulseStep = "GtkProgressBar::pulse_step";
static gchar *Text = "GtkProgressBar::text";
static gchar *Ellipsize = "GtkProgressBar::ellipsize";

#if 0
static gchar *ShowText = "ProgressBar|GtkProgress::show_text";
static gchar *ActivityMode = "ProgressBar|GtkProgress::activity_mode";
static gchar *XAlign = "GtkProgressBar::text_xalign";
static gchar *YAlign = "GtkProgressBar::text_yalign";

static gchar *Value = "ProgressBar|GtkProgress::value";
static gchar *MinValue = "ProgressBar|GtkProgress::lower";
static gchar *MaxValue = "ProgressBar|GtkProgress::upper";
static gchar *Style = "GtkProgressBar::bar_style";
#endif


static const gchar *GbOrientationChoices[] =
{"Left to Right", "Right to Left", "Bottom to Top", "Top to Bottom", NULL};

static const gint GbOrientationValues[] =
{
  GTK_PROGRESS_LEFT_TO_RIGHT,
  GTK_PROGRESS_RIGHT_TO_LEFT,
  GTK_PROGRESS_BOTTOM_TO_TOP,
  GTK_PROGRESS_TOP_TO_BOTTOM
};
static const gchar *GbOrientationSymbols[] =
{
  "GTK_PROGRESS_LEFT_TO_RIGHT",
  "GTK_PROGRESS_RIGHT_TO_LEFT",
  "GTK_PROGRESS_BOTTOM_TO_TOP",
  "GTK_PROGRESS_TOP_TO_BOTTOM"
};

static const gchar *GbEllipsizeChoices[] =
{
  "None",
  "Start",
  "Middle",
  "End",
  NULL
};
static const gint GbEllipsizeValues[] =
{
  PANGO_ELLIPSIZE_NONE,
  PANGO_ELLIPSIZE_START,
  PANGO_ELLIPSIZE_MIDDLE,
  PANGO_ELLIPSIZE_END
};
static const gchar *GbEllipsizeSymbols[] =
{
  "PANGO_ELLIPSIZE_NONE",
  "PANGO_ELLIPSIZE_START",
  "PANGO_ELLIPSIZE_MIDDLE",
  "PANGO_ELLIPSIZE_END"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkProgressBar, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_progress_bar_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  new_widget = gtk_progress_bar_new ();
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */

static void
gb_progress_bar_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (Orientation, _("Orientation:"),
		       _("The orientation of the progress bar's contents"),
		       GbOrientationChoices);
  property_add_float_range (Fraction, _("Fraction:"),
			    _("The fraction of work that has been completed"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (PulseStep, _("Pulse Step:"),
			    _("The fraction of the progress bar length to move the bouncing block when pulsed"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_string (Text, _("Text:"),
		       _("The text to display over the progress bar"));

  property_add_choice (Ellipsize, _("Ellipsize:"),
		       _("How to ellipsize the string"),
		       GbEllipsizeChoices);

#if 0
  /* ShowText is implicit now, if the Text property is set to anything. */
  property_add_bool (ShowText, _("Show Text:"), 
                     _("If the text should be shown in the progress bar"));

  /* ActivityMode is deprecated and implicit now. The app just calls
     gtk_progress_bar_pulse() and it automatically goes into activity mode. */
  property_add_bool (ActivityMode, _("Activity Mode:"), 
                     _("If the progress bar should act like the front of Kit's car"));

  /* The GtkProgress interface is deprecated now, and GtkProgressBar doesn't
     have functions to set these, so I suppose we shouldn't support them. */
  property_add_float_range (XAlign, _("X Align:"),
			    _("The horizontal alignment of the text"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (YAlign, _("Y Align:"),
			    _("The vertical alignment of the text"),
			    0, 1, 0.01, 0.1, 0.01, 2);
#endif
}
 



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */

static void
gb_progress_bar_get_properties(GtkWidget *widget, GbWidgetGetArgData * data)
{
  PangoEllipsizeMode ellipsize_mode;
  gint i;

  for (i = 0; i < sizeof (GbOrientationValues) / sizeof (GbOrientationValues[0]); i++)
    {
      if (GbOrientationValues[i] == GTK_PROGRESS_BAR (widget)->orientation)
	gb_widget_output_choice (data, Orientation, i, GbOrientationSymbols[i]);
    }
  
    gb_widget_output_float (data, Fraction, gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (widget)));
    gb_widget_output_float (data, PulseStep, gtk_progress_bar_get_pulse_step (GTK_PROGRESS_BAR (widget)));
    gb_widget_output_translatable_string (data, Text, gtk_progress_bar_get_text (GTK_PROGRESS_BAR (widget)));

  ellipsize_mode = gtk_progress_bar_get_ellipsize (GTK_PROGRESS_BAR (widget));
  for (i = 0; i < sizeof (GbEllipsizeValues) / sizeof (GbEllipsizeValues[0]); i++)
    {
      if (GbEllipsizeValues[i] == ellipsize_mode)
	gb_widget_output_choice (data, Ellipsize, i, GbEllipsizeSymbols[i]);
    }

#if 0
    gb_widget_output_bool (data, ShowText, GTK_PROGRESS (widget)->show_text);
    gb_widget_output_bool (data, ActivityMode, GTK_PROGRESS (widget)->activity_mode);

    gb_widget_output_float (data, XAlign, GTK_PROGRESS (widget)->x_align);
    gb_widget_output_float (data, YAlign, GTK_PROGRESS (widget)->y_align);
#endif
}




/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */

static void
gb_progress_bar_set_properties(GtkWidget * widget, GbWidgetSetArgData * data)
{
  gfloat fraction, pulse_step;
#if 0
  gfloat activityMode, xalign, yalign;
  gboolean set_alignment = FALSE;
  gboolean showText;
#endif
  gchar *orientation, *text, *ellipsize_mode;
  gint i;
 
  orientation = gb_widget_input_choice (data, Orientation);
  if (data->apply)
    {
      for (i = 0;
	   i < sizeof (GbOrientationValues) / sizeof (GbOrientationValues[0]);
	   i++)
	{
	  if (!strcmp (orientation, GbOrientationChoices[i])
	      || !strcmp (orientation, GbOrientationSymbols[i]))
	    {
	      gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (widget),
						GbOrientationValues[i]);
	      break;
	    }
	}
    }

  fraction = gb_widget_input_float (data, Fraction);
  if (data->apply)
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), fraction);

  pulse_step = gb_widget_input_float (data, PulseStep);
  if (data->apply)
    gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (widget), pulse_step);

  text = gb_widget_input_string (data, Text);
  if (data->apply)
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (widget), text);

  ellipsize_mode = gb_widget_input_choice (data, Ellipsize);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbEllipsizeValues) / sizeof (GbEllipsizeValues[0]);
	   i++)
	{
	  if (!strcmp (ellipsize_mode, GbEllipsizeChoices[i])
	      || !strcmp (ellipsize_mode, GbEllipsizeSymbols[i]))
	    {
	      gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (widget),
					      GbEllipsizeValues[i]);
	      break;
	    }
	}
    }

#if 0
  showText = gb_widget_input_bool (data, ShowText);
  if (data->apply)
    gtk_progress_set_show_text (GTK_PROGRESS (widget), showText);

  activityMode = gb_widget_input_bool (data, ActivityMode);
  if (data->apply)
    gtk_progress_set_activity_mode (GTK_PROGRESS (widget), activityMode);

  xalign = gb_widget_input_float (data, XAlign);
  if (data->apply)
    set_alignment = TRUE;
  else
    xalign = GTK_PROGRESS (widget)->x_align;

  yalign = gb_widget_input_float (data, YAlign);
  if (data->apply)
    set_alignment = TRUE;
  else
    yalign = GTK_PROGRESS (widget)->y_align;

  if (set_alignment)
    gtk_progress_set_text_alignment (GTK_PROGRESS (widget), xalign, yalign);
#endif
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_progress_bar_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gfloat fraction, pulse_step;
  const char *text;
  PangoEllipsizeMode ellipsize_mode;
  gint i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_progress_bar_new ();\n", data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  if (GTK_PROGRESS_BAR (widget)->orientation != GTK_PROGRESS_LEFT_TO_RIGHT)
    {
      for (i = 0; i < sizeof (GbOrientationValues) / sizeof (GbOrientationValues[0]); i++)
        {
          if (GbOrientationValues[i] == GTK_PROGRESS_BAR (widget)->orientation)
            source_add (data,
		    "  gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (%s), %s);\n", 
                        data->wname, GbOrientationSymbols[i]);
        }
    }

  fraction = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (widget));
  if (fraction >= GLADE_EPSILON)
    source_add (data,
	"  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (%s), %g);\n", 
		data->wname, fraction);

  pulse_step = gtk_progress_bar_get_pulse_step (GTK_PROGRESS_BAR (widget));
  if (fabs (pulse_step - 0.1) >= GLADE_EPSILON)
    source_add (data,
	"  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (%s), %g);\n", 
		data->wname, pulse_step);

  text = gtk_progress_bar_get_text (GTK_PROGRESS_BAR (widget));
  if (text && *text)
    {
      gboolean translatable, context;
      gchar *comments;

      glade_util_get_translation_properties (widget, Text,
					     &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
                  "  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (%s), %s);\n",
                  data->wname,
		  source_make_string_full (text, data->use_gettext && translatable, context));
    }

  ellipsize_mode = gtk_progress_bar_get_ellipsize (GTK_PROGRESS_BAR (widget));
  if (ellipsize_mode != PANGO_ELLIPSIZE_NONE)
    {
      for (i = 0;
	   i < sizeof (GbEllipsizeValues) / sizeof (GbEllipsizeValues[0]);
	   i++)
	{
	  if (GbEllipsizeValues[i] == ellipsize_mode)
	    source_add (data,
			"  gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (%s), %s);\n",
			data->wname, GbEllipsizeSymbols[i]);
	}
    }

#if 0
  if (GTK_PROGRESS (widget)->show_text)
    {
      source_add( data,
                  "  gtk_progress_set_show_text (GTK_PROGRESS (%s), TRUE);\n",
                  data->wname);
    }

  if (GTK_PROGRESS (widget)->activity_mode)
    {
      source_add( data,
                  "  gtk_progress_set_activity_mode (GTK_PROGRESS (%s), TRUE);\n",
                  data->wname);
    }

  if (GTK_PROGRESS (widget)->x_align != 0.5
      || GTK_PROGRESS (widget)->y_align != 0.5)
    {
      source_add( data,
                  "  gtk_progress_set_text_alignment (GTK_PROGRESS (%s), %g, %g);\n",
                  data->wname,
		  GTK_PROGRESS (widget)->x_align,
		  GTK_PROGRESS (widget)->y_align);
    }
#endif
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_progress_bar_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_progress_bar_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = progressbar_xpm;
  gbwidget.tooltip = _("Progress Bar");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_progress_bar_new;

  gbwidget.gb_widget_create_properties = gb_progress_bar_create_properties;
  gbwidget.gb_widget_get_properties    = gb_progress_bar_get_properties;
  gbwidget.gb_widget_set_properties    = gb_progress_bar_set_properties;
  /*gbwidget.gb_widget_create_popup_menu = gb_progress_bar_create_popup_menu;*/
  gbwidget.gb_widget_write_source      = gb_progress_bar_write_source;

  return &gbwidget;
}
