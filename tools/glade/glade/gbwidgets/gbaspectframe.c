
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

#include "../gb.h"
#include <math.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkaspectframe.h>
#include <gtk/gtklabel.h>

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/aspectframe.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *LabelXAlign = "AspectFrame|GtkFrame::label_xalign";
static gchar *LabelYAlign = "AspectFrame|GtkFrame::label_yalign";
static gchar *Shadow = "AspectFrame|GtkFrame::shadow_type";
static gchar *XAlign = "GtkAspectFrame::xalign";
static gchar *YAlign = "GtkAspectFrame::yalign";
static gchar *Ratio = "GtkAspectFrame::ratio";
static gchar *Obey = "GtkAspectFrame::obey_child";

/* We don't show this any more, as the label is a child widget now, but we
   load this for compatability with old XML files. */
static gchar *Label = "AspectFrame|GtkFrame::label";

static const gchar *GbShadowChoices[] =
{"None", "In", "Out",
 "Etched In", "Etched Out", NULL};
static const gint GbShadowValues[] =
{
  GTK_SHADOW_NONE,
  GTK_SHADOW_IN,
  GTK_SHADOW_OUT,
  GTK_SHADOW_ETCHED_IN,
  GTK_SHADOW_ETCHED_OUT
};
static const gchar *GbShadowSymbols[] =
{
  "GTK_SHADOW_NONE",
  "GTK_SHADOW_IN",
  "GTK_SHADOW_OUT",
  "GTK_SHADOW_ETCHED_IN",
  "GTK_SHADOW_ETCHED_OUT"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkAspectFrame, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_aspect_frame_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, TRUE);

  if (data->action != GB_LOADING)
    {
      GtkWidget *label, *alignment;
      gchar *label_markup;

      /* For HIG compliance, we create a frame with no shadow, with a label
	 of "<b>widget-name</b>", and with an alignment to add 12 pixels
	 padding on the left. */
      gtk_frame_set_shadow_type  (GTK_FRAME (new_widget), GTK_SHADOW_NONE);

      label = gb_widget_new ("GtkLabel", NULL);
      label_markup = g_strdup_printf ("<b>%s</b>", data->name);
      gtk_label_set_markup (GTK_LABEL (label), label_markup);
      g_free (label_markup);
      gtk_object_set_data (GTK_OBJECT (label), "GtkLabel::use_markup",
			   GINT_TO_POINTER (TRUE));

      gtk_frame_set_label_widget (GTK_FRAME (new_widget), label);

      alignment = gb_widget_new ("GtkAlignment", NULL);
      gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);

      gtk_container_add (GTK_CONTAINER (new_widget), alignment);
    }

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_aspect_frame_create_properties (GtkWidget * widget, GbWidgetCreateArgData *
				   data)
{
  property_add_float_range (LabelXAlign, _("Label X Align:"),
			    _("The horizontal alignment of the frame's label widget"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (LabelYAlign, _("Label Y Align:"),
			    _("The vertical alignment of the frame's label widget"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_choice (Shadow, _("Shadow:"), _("The type of shadow of the frame"),
		       GbShadowChoices);
  property_add_float_range (XAlign, _("X Align:"),
			    _("The horizontal alignment of the frame's child"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (YAlign, _("Y Align:"),
			    _("The horizontal alignment of the frame's child"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float (Ratio, _("Ratio:"),
		      _("The aspect ratio of the frame's child"));
  property_add_bool (Obey, _("Obey Child:"),
		   _("If the aspect ratio should be determined by the child"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_aspect_frame_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i;

  gb_widget_output_float (data, LabelXAlign, GTK_FRAME (widget)->label_xalign);
  gb_widget_output_float (data, LabelYAlign, GTK_FRAME (widget)->label_yalign);

  for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i++)
    {
      if (GbShadowValues[i] == GTK_FRAME (widget)->shadow_type)
	gb_widget_output_choice (data, Shadow, i, GbShadowSymbols[i]);
    }

  gb_widget_output_float (data, XAlign, GTK_ASPECT_FRAME (widget)->xalign);
  gb_widget_output_float (data, YAlign, GTK_ASPECT_FRAME (widget)->yalign);
  gb_widget_output_float (data, Ratio, GTK_ASPECT_FRAME (widget)->ratio);
  gb_widget_output_bool (data, Obey, GTK_ASPECT_FRAME (widget)->obey_child);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_aspect_frame_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gfloat label_xalign, label_yalign;
  gboolean apply_xalign, apply_yalign;
  gfloat xalign, yalign, ratio;
  gchar *shadow;
  gboolean obey_child, set_aspect_frame = FALSE;
  gint i;

  /* We load the 'label' property, but create a child GbWidget. */
  if (data->action == GB_LOADING)
    {
      gchar *label = gb_widget_input_string (data, Label);
      if (data->apply && label && *label)
	{
	  GtkWidget *label_widget = gb_widget_new ("GtkLabel", NULL);
	  gtk_label_set_text (GTK_LABEL (label_widget), label);
	  gtk_frame_set_label_widget (GTK_FRAME (widget), label_widget);
	}
    }

  label_xalign = gb_widget_input_float (data, LabelXAlign);
  apply_xalign = data->apply;
  if (!apply_xalign)
    label_xalign = GTK_FRAME (widget)->label_xalign;

  label_yalign = gb_widget_input_float (data, LabelYAlign);
  apply_yalign = data->apply;
  if (!apply_yalign)
    label_yalign = GTK_FRAME (widget)->label_yalign;

  if (apply_xalign || apply_yalign)
    gtk_frame_set_label_align (GTK_FRAME (widget), label_xalign, label_yalign);

  shadow = gb_widget_input_choice (data, Shadow);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i
	   ++)
	{
	  if (!strcmp (shadow, GbShadowChoices[i])
	      || !strcmp (shadow, GbShadowSymbols[i]))
	    {
	      gtk_frame_set_shadow_type (GTK_FRAME (widget), GbShadowValues[i]);
	      break;
	    }
	}
    }

  xalign = gb_widget_input_float (data, XAlign);
  if (data->apply)
    set_aspect_frame = TRUE;
  else
    xalign = GTK_ASPECT_FRAME (widget)->xalign;

  yalign = gb_widget_input_float (data, YAlign);
  if (data->apply)
    set_aspect_frame = TRUE;
  else
    yalign = GTK_ASPECT_FRAME (widget)->yalign;

  ratio = gb_widget_input_float (data, Ratio);
  if (data->apply)
    set_aspect_frame = TRUE;
  else
    ratio = GTK_ASPECT_FRAME (widget)->ratio;

  obey_child = gb_widget_input_bool (data, Obey);
  if (data->apply)
    set_aspect_frame = TRUE;
  else
    obey_child = GTK_ASPECT_FRAME (widget)->obey_child;

  if (set_aspect_frame)
    gtk_aspect_frame_set (GTK_ASPECT_FRAME (widget), xalign, yalign,
			  ratio, obey_child);
}



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_aspect_frame_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gfloat xalign, yalign;
  gint shadow = 0, i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_aspect_frame_new (NULL, %g, %g, %g, %s);\n",
		  data->wname,
		  GTK_ASPECT_FRAME (widget)->xalign,
		  GTK_ASPECT_FRAME (widget)->yalign,
		  GTK_ASPECT_FRAME (widget)->ratio,
		  GTK_ASPECT_FRAME (widget)->obey_child ? "TRUE" : "FALSE");
    }
  gb_widget_write_standard_source (widget, data);

  xalign = GTK_FRAME (widget)->label_xalign;
  yalign = GTK_FRAME (widget)->label_yalign;
  if (xalign >= GLADE_EPSILON || fabs (yalign - 0.5) >= GLADE_EPSILON)
    {
      source_add (data,
		  "  gtk_frame_set_label_align (GTK_FRAME (%s), %g, 0.5);\n",
		  data->wname, xalign);
    }

  if (GTK_FRAME (widget)->shadow_type != GTK_SHADOW_ETCHED_IN)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i
	   ++)
	if (GbShadowValues[i] == GTK_FRAME (widget)->shadow_type)
	  {
	    shadow = i;
	    break;
	  }
      source_add (data, "  gtk_frame_set_shadow_type (GTK_FRAME (%s), %s);\n",
		  data->wname, GbShadowSymbols[shadow]);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_aspect_frame_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_aspect_frame_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = aspectframe_xpm;
  gbwidget.tooltip = _("Aspect Frame");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_aspect_frame_new;
  gbwidget.gb_widget_create_properties = gb_aspect_frame_create_properties;
  gbwidget.gb_widget_get_properties = gb_aspect_frame_get_properties;
  gbwidget.gb_widget_set_properties = gb_aspect_frame_set_properties;
  gbwidget.gb_widget_write_source = gb_aspect_frame_write_source;

  /* We just use the GtkFrame functions for these. */
  gbwidget.gb_widget_create_popup_menu = gb_frame_create_popup_menu;
  gbwidget.gb_widget_add_child = gb_frame_add_child;
  gbwidget.gb_widget_get_child_properties = gb_frame_get_child_properties;
  gbwidget.gb_widget_write_add_child_source = gb_frame_write_add_child_source;

  return &gbwidget;
}
