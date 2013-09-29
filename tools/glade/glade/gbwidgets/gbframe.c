
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
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenuitem.h>

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/frame.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *LabelXAlign = "GtkFrame::label_xalign";
static gchar *LabelYAlign = "GtkFrame::label_yalign";
static gchar *Shadow = "GtkFrame::shadow_type";

/* We don't show this any more, as the label is a child widget now, but we
   load this for compatability with old XML files. */
static gchar *Label = "GtkFrame::label";

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
 * Creates a new GtkWidget of class GtkFrame, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_frame_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_frame_new (NULL);

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
gb_frame_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_float_range (LabelXAlign, _("Label X Align:"),
			    _("The horizontal alignment of the frame's label widget"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (LabelYAlign, _("Label Y Align:"),
			    _("The vertical alignment of the frame's label widget"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_choice (Shadow, _("Shadow:"), _("The type of shadow of the frame"),
		       GbShadowChoices);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_frame_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i;

  gb_widget_output_float (data, LabelXAlign, GTK_FRAME (widget)->label_xalign);
  gb_widget_output_float (data, LabelYAlign, GTK_FRAME (widget)->label_yalign);

  for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i++)
    {
      if (GbShadowValues[i] == GTK_FRAME (widget)->shadow_type)
	gb_widget_output_choice (data, Shadow, i, GbShadowSymbols[i]);
    }
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_frame_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gfloat label_xalign, label_yalign;
  gboolean apply_xalign, apply_yalign;
  gchar *shadow;
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
}



static void
gb_frame_add_label_widget (GtkWidget * menuitem, GtkFrame * frame)
{
  gtk_frame_set_label_widget (frame, editor_new_placeholder ());
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkFrame, with signals pointing to
 * other functions in this file.
 */
void
gb_frame_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
{
  GtkWidget *menuitem;

  /* If there is no label widget at present, we add a command to add one. */
  if (!gtk_frame_get_label_widget (GTK_FRAME (widget)))
    {
      menuitem = gtk_menu_item_new_with_label (_("Add Label Widget"));
      gtk_widget_show (menuitem);
      gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_frame_add_label_widget), widget);
    }
}


void
gb_frame_add_child (GtkWidget *widget, GtkWidget *child, GbWidgetSetArgData *data)
{
  gboolean is_label_item = FALSE;

  /* See if this is a tab widget. We use a special "type" packing property set
     to "label_item".*/
  if (data->child_info)
    {
      int j;

      for (j = 0; j < data->child_info->n_properties; j++)
	{
	  if (!strcmp (data->child_info->properties[j].name, "type")
	      && !strcmp (data->child_info->properties[j].value, "label_item"))
	    {
	      is_label_item = TRUE;
	      break;
	    }
	}
    }

  if (is_label_item)
    gtk_frame_set_label_widget (GTK_FRAME (widget), child);
  else
    gtk_container_add (GTK_CONTAINER (widget), child);
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_frame_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gfloat xalign, yalign;
  gint shadow = 0, i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_frame_new (NULL);\n", data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  xalign = GTK_FRAME (widget)->label_xalign;
  yalign = GTK_FRAME (widget)->label_yalign;
  if (xalign >= GLADE_EPSILON || fabs (yalign - 0.5) >= GLADE_EPSILON)
    {
      source_add (data,
		  "  gtk_frame_set_label_align (GTK_FRAME (%s), %g, %g);\n",
		  data->wname, xalign, yalign);
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


void
gb_frame_get_child_properties (GtkWidget *widget, GtkWidget *child,
			       GbWidgetGetArgData *data)
{

  /* When saving the label widget, we save a "type" packing property set to
     "label_item". */
  if (data->action != GB_SAVING
      || gtk_frame_get_label_widget (GTK_FRAME (widget)) != child)
    return;

  save_start_tag (data, "packing");
  save_string (data, "type", "label_item");
  save_end_tag (data, "packing");
}


/* Outputs source to add a child widget to a GtkNotebook. */
void
gb_frame_write_add_child_source (GtkWidget * parent,
				 const gchar *parent_name,
				 GtkWidget *child,
				 GbWidgetWriteSourceData * data)
{

  if (gtk_frame_get_label_widget (GTK_FRAME (parent)) == child)
    {
      source_add (data,
		  "  gtk_frame_set_label_widget (GTK_FRAME (%s), %s);\n",
		  parent_name, data->wname);
    }
  else
    {
      source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
		  parent_name, data->wname);
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_frame_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_frame_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = frame_xpm;
  gbwidget.tooltip = _("Frame");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_frame_new;
  gbwidget.gb_widget_create_properties = gb_frame_create_properties;
  gbwidget.gb_widget_get_properties = gb_frame_get_properties;
  gbwidget.gb_widget_set_properties = gb_frame_set_properties;
  gbwidget.gb_widget_create_popup_menu = gb_frame_create_popup_menu;
  gbwidget.gb_widget_write_source = gb_frame_write_source;

  gbwidget.gb_widget_add_child = gb_frame_add_child;
  gbwidget.gb_widget_get_child_properties = gb_frame_get_child_properties;
  gbwidget.gb_widget_write_add_child_source = gb_frame_write_add_child_source;

  return &gbwidget;
}
