/*
 * Copyright (C) 2003 Sun Microsystems, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Mark McLoughlin <mark@skynet.ie> 
 */

#include "../gb.h"
#include <gtk/gtkexpander.h>

#include "../graphics/expander.xpm"

#define PROPERTY_EXPANDED "GtkExpander::expanded"
#define PROPERTY_SPACING  "GtkExpander::spacing"

static GbWidget gb_expander;

GtkWidget *
gb_expander_new (GbWidgetNewData *data)
{
  GtkWidget *expander = gtk_expander_new (NULL);

  if (data->action != GB_LOADING)
    {
      GtkWidget *label;
      
      gtk_container_add (GTK_CONTAINER (expander), editor_new_placeholder ());
      label = gb_widget_new ("GtkLabel", NULL);
      gtk_expander_set_label_widget (GTK_EXPANDER (expander), label);
    }

  return expander;
}

static void
gb_expander_create_properties (GtkWidget             *widget,
			       GbWidgetCreateArgData *data)
{
  property_add_bool (PROPERTY_EXPANDED,
		     _("Initially Expanded:"),
		     _("Whether the expander is initially opened to reveal the child widget"));
  property_add_int_range (PROPERTY_SPACING,
			  _("Spacing:"),
			  _("Space to put between the label and the child"),
			  0, 1000, 1, 10, 1);
}

static void
gb_expander_get_properties (GtkWidget          *widget,
			    GbWidgetGetArgData *data)
{
  gb_widget_output_bool (data,
			 PROPERTY_EXPANDED,
			 gtk_expander_get_expanded (GTK_EXPANDER (widget)));
  gb_widget_output_int (data,
			PROPERTY_SPACING,
			gtk_expander_get_spacing (GTK_EXPANDER (widget)));
}

static void
gb_expander_set_properties (GtkWidget          *widget,
			    GbWidgetSetArgData *data)
{
  gboolean expanded;
  int spacing;

  expanded = gb_widget_input_bool (data, PROPERTY_EXPANDED);
  if (data->apply)
    gtk_expander_set_expanded (GTK_EXPANDER (widget), expanded);

  spacing = gb_widget_input_int (data, PROPERTY_SPACING);
  if (data->apply)
    gtk_expander_set_spacing (GTK_EXPANDER (widget), spacing);
}

static void
gb_expander_add_label_widget (GtkWidget    *menuitem,
			      GtkExpander  *expander)
{
  gtk_expander_set_label_widget (expander, editor_new_placeholder ());
}

void
gb_expander_create_popup_menu (GtkWidget              *widget,
			       GbWidgetCreateMenuData *data)
{
  GtkWidget *menuitem;
  
  if (!gtk_expander_get_label_widget (GTK_EXPANDER (widget)))
    {
      menuitem = gtk_menu_item_new_with_label (_("Add Label Widget"));
      gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
      g_signal_connect (menuitem, "activate",
			G_CALLBACK (gb_expander_add_label_widget), widget);
      gtk_widget_show (menuitem);
    }
}

void
gb_expander_add_child (GtkWidget          *widget,
		       GtkWidget          *child,
		       GbWidgetSetArgData *data)
{
  gboolean is_label_item = FALSE;
  
  if (data->child_info)
    {
      int j;

      for (j = 0; j < data->child_info->n_properties; j++)
        {
          if (!strcmp (data->child_info->properties[j].name, "type") &&
	      !strcmp (data->child_info->properties[j].value, "label_item"))
            {
              is_label_item = TRUE;
              break;
            }
        }
    }

  if (is_label_item)
    gtk_expander_set_label_widget (GTK_EXPANDER (widget), child);
  else
    gtk_container_add (GTK_CONTAINER (widget), child);
}

#if 0
/* This is in gb_widget_replace_child() now. */
static void
gb_expander_replace_child (GtkWidget *widget,
			   GtkWidget *current_child,
			   GtkWidget *new_child)
{
  /* If this is the expander's label widget, we replace that. */
  if (gtk_expander_get_label_widget (GTK_EXPANDER (widget)) == current_child)
    {
      gtk_expander_set_label_widget (GTK_EXPANDER (widget), new_child);
    }
  else
    {
      gtk_container_remove (GTK_CONTAINER (widget), current_child);
      gtk_container_add (GTK_CONTAINER (widget), new_child);
    }
}
#endif

static void
gb_expander_write_source (GtkWidget               *widget,
			  GbWidgetWriteSourceData *data)
{
  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_expander_new (NULL);\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);
  
  if (gtk_expander_get_expanded (GTK_EXPANDER (widget)))
    {
      source_add (data,
		  "  gtk_expander_set_expanded (GTK_EXPANDER (%s), %s);\n",
		  data->wname,
		  gtk_expander_get_expanded (GTK_EXPANDER (widget)) ? "TRUE" : "FALSE");
    }

  if (gtk_expander_get_spacing (GTK_EXPANDER (widget)) != 0)
    {
      source_add (data,
		  "  gtk_expander_set_spacing (GTK_EXPANDER (%s), %d);\n",
		  data->wname,
		  gtk_expander_get_spacing (GTK_EXPANDER (widget)));
    }
}

void
gb_expander_get_child_properties (GtkWidget          *widget,
				  GtkWidget          *child,
				  GbWidgetGetArgData *data)
{
  if (data->action == GB_SAVING &&
      gtk_expander_get_label_widget (GTK_EXPANDER (widget)) == child)
    {
      save_start_tag (data, "packing");
      save_string (data, "type", "label_item");
      save_end_tag (data, "packing");
    }
}

void
gb_expander_write_add_child_source (GtkWidget               *parent,
				    const char              *parent_name,
				    GtkWidget               *child,
				    GbWidgetWriteSourceData *data)
{
  if (gtk_expander_get_label_widget (GTK_EXPANDER (parent)) == child)
    {
      source_add (data,
                  "  gtk_expander_set_label_widget (GTK_EXPANDER (%s), %s);\n",
                  parent_name, data->wname);
    }
  else
    {
      source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
                  parent_name, data->wname);
    }
}

GbWidget *
gb_expander_init ()
{
  gb_widget_init_struct (&gb_expander);

  gb_expander.pixmap_struct = expander_xpm;
  gb_expander.tooltip = _("Expander");

  gb_expander.gb_widget_new               = gb_expander_new;
  gb_expander.gb_widget_create_properties = gb_expander_create_properties;
  gb_expander.gb_widget_get_properties    = gb_expander_get_properties;
  gb_expander.gb_widget_set_properties    = gb_expander_set_properties;
  gb_expander.gb_widget_create_popup_menu = gb_expander_create_popup_menu;
  gb_expander.gb_widget_write_source      = gb_expander_write_source;

  gb_expander.gb_widget_add_child              = gb_expander_add_child;
  gb_expander.gb_widget_get_child_properties   = gb_expander_get_child_properties;
  gb_expander.gb_widget_write_add_child_source = gb_expander_write_add_child_source;

  return &gb_expander;
}
