
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

#include <config.h>

#include <string.h>

#include <gtk/gtkalignment.h>
#include <gtk/gtkbbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkstock.h>

#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/button.xpm"

/*
 * GtkButton widget - unfortunately this has been 'overloaded' a bit to cope
 * with extra properties available in toolbar buttons and Gnome buttons.
 * Standard toolbar buttons can have an icon as well as a label.
 * Gnome buttons can be a stock button (which has a label and icon), or can
 * have a stock pixmap as well as a label if they are in a toolbar or in a
 * GnomeDialog/MessageBox.
 */

typedef enum {
  GLADE_BUTTON_NORMAL,
  GLADE_BUTTON_DIALOG,
  GLADE_BUTTON_GNOME_DIALOG
} GladeButtonType;


/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *StockButton = "GtkButton::stock_button";
static gchar *Label = "GtkButton::label";
static gchar *Icon = "GtkButton::icon";
static gchar *FocusOnClick = "GtkButton::focus_on_click";

/* This is only used for normal/stock buttons, not special toolbar buttons,
   as the toolbar has its own relief setting. */
static gchar *Relief = "GtkButton::relief";

/* This is used for dialog buttons. It is a string, either one of the standard
   GTK+ response strings, or a user-defined integer. */
static gchar *ResponseID = "GtkButton::response_id";

/* This is used for dialog buttons, to set if it is a secondary button.
   Buttons with ResponseID set to "GTK_RESPONSE_HELP" are always secondary. */
/* FIXME: We don't worry about this for now. We may support it later. */
/*static gchar *Secondary = "GtkButton::secondary";*/


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_button_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_CREATING)
    new_widget = gtk_button_new_with_label (data->name);
  else
    {
      new_widget = gtk_button_new ();
      gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder());
    }
  return new_widget;
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  GList *response_list = NULL;
  gint i;

  property_add_stock_item (StockButton, _("Stock Button:"),
			   _("The stock button to use"),
			   GTK_ICON_SIZE_BUTTON);
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
  property_add_icon (Icon, _("Icon:"),
		     _("The icon to display"),
		     GTK_ICON_SIZE_BUTTON);
  property_add_choice (Relief, _("Button Relief:"),
		       _("The relief style of the button"),
		       GladeReliefChoices);

  for (i = 0; i < GladeStockResponsesSize; i++)
    response_list = g_list_append (response_list, GladeStockResponses[i].name);
  property_add_combo (ResponseID, _("Response ID:"),
	_("The response code returned when the button is pressed. "
	  "Select one of the standard responses or enter a positive integer value"),
		      response_list);
  g_list_free (response_list);

  property_add_bool (FocusOnClick, _("Focus On Click:"), _("If the button grabs focus when it is clicked"));
}


/* Returns the type of the button - NORMAL, DIALOG or GNOME_DIALOG. */
GladeButtonType
gb_button_get_button_type (GtkWidget *widget)
{
  gchar *child_name;

  if (widget->parent)
    {
      child_name = gb_widget_get_child_name (widget->parent);
      if (child_name && !strcmp (child_name, GladeChildDialogActionArea))
	{
	  GtkWidget *toplevel = glade_util_get_toplevel (widget);

	  if (GTK_IS_DIALOG (toplevel))
	    return GLADE_BUTTON_DIALOG;
	  else
	    return GLADE_BUTTON_GNOME_DIALOG;
	}
    }

  return GLADE_BUTTON_NORMAL;
}


/*
 * NORMAL BUTTONS. GtkButton/GtkToggleButton/GtkCheckButton/GtkRadioButton.
 */

/* This tries to find the child label & icon of the button. If the button
   contains other widgets it returns FALSE, i.e. if the user has added other
   widgets instead. */
static gboolean
gb_button_normal_find_child_widgets (GtkWidget *widget,
				     GtkWidget **label,
				     GtkWidget **icon)
{
  GtkWidget *child, *alignment_child;

  *label = *icon = NULL;

  /* Check it has a child. If it doesn't output a warning. */
  child = GTK_BIN (widget)->child;
  g_return_val_if_fail (child != NULL, FALSE);

  /* Check if it has just a GtkLabel child. */
  if (GTK_IS_LABEL (child))
    {
      /* If it is a GbWidget we don't handle its properties here. */
      if (GB_IS_GB_WIDGET (child))
	return FALSE;

      *label = child;
      return TRUE;
    }

  /* Check if it has just a GtkImage child. */
  if (GTK_IS_IMAGE (child))
    {
      *icon = child;
      return TRUE;
    }

  /* If it contains an icon and a label it must be a GtkAlignment with a
     GtkHBox in it. */
  if (!GTK_IS_ALIGNMENT (child))
    return FALSE;

  /* Now check for a hbox with a GtkImage and GtkLabel children. */
  alignment_child = GTK_BIN (child)->child;
  if (alignment_child && GTK_IS_HBOX (alignment_child)
      && g_list_length (GTK_BOX (alignment_child)->children) == 2)
    {
      GList *children;
      GtkBoxChild *child1, *child2;

      children = GTK_BOX (alignment_child)->children;
      child1 = children->data;
      child2 = children->next->data;

      if (GTK_IS_IMAGE (child1->widget) && GTK_IS_LABEL (child2->widget))
	{
	  *icon = child1->widget;
	  *label = child2->widget;
	  return TRUE;
	}
    }

  return FALSE;
}


static void
gb_button_normal_get_properties (GtkWidget * widget,
				 GbWidgetGetArgData * data,
				 gchar *stock_id_p,
				 gchar *label_p,
				 gchar *icon_p)
{
  gchar *stock_id, *label_text;
  gboolean label_sensitive = TRUE;
  gboolean icon_sensitive = TRUE;
  GtkWidget *label, *icon;

  /* We only allow the GTK_ICON_SIZE_BUTTON size for stock items & icons. */
  if (data->action == GB_SHOWING)
    {
      property_set_stock_item_icon_size (stock_id_p, GTK_ICON_SIZE_BUTTON);
      property_set_icon_size (icon_p, GTK_ICON_SIZE_BUTTON);
    }

  stock_id = gtk_object_get_data (GTK_OBJECT (widget), GladeButtonStockIDKey);
  /* In the XML the stock item name is actually saved as the label property,
     and the use_stock property specifies that it is a stock item.
     Also, use_underline needs to be set. */
  if (data->action == GB_SAVING)
    {
      if (stock_id)
	{
	  gb_widget_output_stock_item (data, "label", stock_id);
	  gb_widget_output_bool (data, "use_stock", TRUE);
	}
    }
  else
    {
      gb_widget_output_stock_item (data, stock_id_p, stock_id);
    }


  if (stock_id)
    {
      label_sensitive = FALSE;
      icon_sensitive = FALSE;
    }
  else
    {
      if (gb_button_normal_find_child_widgets (widget, &label, &icon))
	{
	  gchar *icon_name = NULL;

	  if (label)
	    label_text = (gchar*) gtk_label_get_label (GTK_LABEL (label));
	  else
	    label_text = "";

	  /* This is a bit of a hack. The label has the real translation
	     properties, but we copy them to the button so everything works. */
	  if (label) {
	    glade_util_copy_translation_properties (label, "GtkLabel::label",
						    widget, label_p);
	  }

	  /* When saving we only output the label if the icon isn't set,
	     since if it is we will be saving as separate child widgets. */
	  if (data->action != GB_SAVING || !icon)
	    gb_widget_output_translatable_text (data, label_p, label_text);

	  /* We always save use_underline as TRUE, though we don't load it. */
	  if (data->action == GB_SAVING && !icon)
	    gb_widget_output_bool (data, "use_underline", TRUE);

	  if (icon)
	    icon_name = gtk_object_get_data (GTK_OBJECT (icon), GladeIconKey);

	  /* We never output the icon when saving, as it will be saved as a
	     child widget. */
	  if (data->action != GB_SAVING)
	    gb_widget_output_icon (data, icon_p, icon_name);
	}
      else
	{
	  label_sensitive = FALSE;
	  icon_sensitive = FALSE;
	}
    }

  if (data->action == GB_SHOWING)
    {
      if (!label_sensitive)
	gb_widget_output_translatable_text (data, label_p, "");
      property_set_sensitive (label_p, label_sensitive);

      if (!icon_sensitive)
	  gb_widget_output_pixmap_filename (data, icon_p, "");
      property_set_sensitive (icon_p, icon_sensitive);
    }
}


static void
gb_button_normal_set_stock_id (GtkWidget *widget,
			       GbWidgetSetArgData * data,
			       gchar *stock_id,
			       gchar *label_p,
			       gchar *icon_p)
{
  gboolean is_stock_item = FALSE;
  const gchar *label_text = "";

  if (stock_id && stock_id[0])
    is_stock_item = TRUE;

  if (is_stock_item)
    {
      gtk_button_set_use_stock (GTK_BUTTON (widget), TRUE);
      gtk_button_set_label (GTK_BUTTON (widget), stock_id);
      gtk_object_set_data_full (GTK_OBJECT (widget), GladeButtonStockIDKey,
				g_strdup (stock_id), g_free);
    }
  else
    {
      /* Change the button back to a normal button with a simple label. */
      gtk_button_set_use_stock (GTK_BUTTON (widget), FALSE);
      label_text = gtk_widget_get_name (widget);
      gtk_button_set_label (GTK_BUTTON (widget), label_text);
      gtk_object_set_data (GTK_OBJECT (widget), GladeButtonStockIDKey, NULL);
    }

  /* If the widget's properties are displayed, we update the sensitivity of
     the label and icon, according to whether a stock item is selected. */
  if (data->action == GB_APPLYING && property_get_widget () == widget)
    {
      property_set_sensitive (label_p, !is_stock_item);
      property_set_sensitive (icon_p, !is_stock_item);

      /* Turn off auto-apply, and set the label. */
      property_set_auto_apply (FALSE);
      property_set_text (label_p, label_text);
      property_set_filename (icon_p, "");
      property_set_auto_apply (TRUE);
    }
}


static void
gb_button_normal_recreate_children (GtkWidget *widget,
				    GbWidgetSetArgData * data,
				    gchar *label, gchar *icon_name)
{
  GtkWidget *parent = widget;

  /* Remove any existing children. */
  if (GTK_BIN (widget)->child)
    gtk_container_remove (GTK_CONTAINER (widget), GTK_BIN (widget)->child);

  /* If both label and icon are needed, we put them in a hbox, otherwise we
     just add the label or icon to the button. */
  if (label && icon_name)
    {
      GtkWidget *alignment = gb_widget_new ("GtkAlignment", widget);
      gtk_widget_show (alignment);
      gtk_container_add (GTK_CONTAINER (widget), alignment);
      gtk_alignment_set (GTK_ALIGNMENT (alignment), 0.5, 0.5, 0.0, 0.0);
      if (GTK_BIN (alignment)->child)
	gtk_container_remove (GTK_CONTAINER (alignment),
			      GTK_BIN (alignment)->child);

      /* We use gb_widget_new_full() and GB_LOADING since we don't want the
	 dialog to be shown asking for the number of columns. */
      parent = gb_widget_new_full ("GtkHBox", TRUE, alignment, NULL, 0, 0,
				   NULL, GB_LOADING, NULL);
      gtk_widget_show (parent);
      gtk_box_set_spacing (GTK_BOX (parent), 2);
      gtk_container_add (GTK_CONTAINER (alignment), parent);
    }

  if (icon_name)
    {
      GtkWidget *icon_widget;

      icon_widget = gb_widget_new ("GtkImage", parent);

      gtk_object_set_data_full (GTK_OBJECT (icon_widget), GladeIconKey,
				g_strdup (icon_name),
				icon_name ? g_free : NULL);

      if (glade_util_check_is_stock_id (icon_name))
	{
	  gtk_image_set_from_stock (GTK_IMAGE (icon_widget), icon_name,
				    GTK_ICON_SIZE_BUTTON);
	}
      else
	{
	  gtk_image_set_from_file (GTK_IMAGE (icon_widget), icon_name);
	  glade_project_add_pixmap (data->project, icon_name);
	}
      gtk_widget_show (icon_widget);

      /* We pack them in the hbox just like GtkButton does. */
      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), icon_widget, FALSE, FALSE, 0);
      else
	gtk_container_add (GTK_CONTAINER (parent), icon_widget);
    }

  if (label)
    {
      GtkWidget *label_widget;

      /* If we only have a label, we use a simple label widget, otherwise we
	 use a GbWidget. */
      if (icon_name)
	{
	  label_widget = gb_widget_new ("GtkLabel", parent);
	  gtk_label_set_text_with_mnemonic (GTK_LABEL (label_widget), label);
	}
      else
	{
	  label_widget = gtk_label_new_with_mnemonic (label);
	}

      gtk_widget_show (label_widget);

      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), label_widget, FALSE, FALSE, 0);
      else
	gtk_container_add (GTK_CONTAINER (parent), label_widget);
    }
}


void
gb_button_normal_set_properties	(GtkWidget * widget,
				 GbWidgetSetArgData * data,
				 gchar *stock_id_p,
				 gchar *label_p,
				 gchar *icon_p)
{
  gchar *stock_id = NULL, *label, *icon_name = NULL;
  gboolean apply_stock_id = FALSE, apply_label, apply_icon;
  gboolean free_label = FALSE, free_icon_name = FALSE;
  GtkWidget *label_widget, *icon_widget;

  label = gb_widget_input_text (data, label_p);
  apply_label = data->apply;
  if (data->action == GB_APPLYING)
    free_label = TRUE;

  /* When loading, if "use_stock" is TRUE, then the label is the stock item. */
  if (data->action == GB_LOADING)
    {
      gboolean is_stock_item = gb_widget_input_bool (data, "use_stock");
      if (is_stock_item)
	{
	  stock_id = label;
	  apply_stock_id = apply_label;
	}
    }
  else
    {
      stock_id = gb_widget_input_stock_item (data, stock_id_p);
      apply_stock_id = data->apply;
    }

  if (apply_stock_id)
    {
      gb_button_normal_set_stock_id (widget, data, stock_id, label_p, icon_p);
      goto out;
    }

  icon_name = gb_widget_input_icon (data, icon_p);
  apply_icon = data->apply;

  gb_button_normal_find_child_widgets (widget, &label_widget,
				       &icon_widget);

  /* This is a bit of a hack. The label has the real translation
     properties, so we have to copy them back there. */
  if (apply_label && label_widget)
    glade_util_copy_translation_properties (widget, label_p,
					    label_widget, "GtkLabel::label");

  /* If neither icon nor label is set, we don't touch the button as it may
     have other child widgets. */
  if (!apply_label && !apply_icon)
    return;

  if (!apply_label)
    {
      if (label_widget)
	{
	  label = g_strdup (gtk_label_get_label (GTK_LABEL (label_widget)));
	  free_label = TRUE;
	}
      else
	label = NULL;
    }

  if (!apply_icon)
    {
      if (icon_widget)
	{
	  icon_name = gtk_object_get_data (GTK_OBJECT (icon_widget),
					   GladeIconKey);
	  if (icon_name && *icon_name)
	    {
	      icon_name = g_strdup (icon_name);
	      free_icon_name = TRUE;
	    }
	  else
	    icon_name = NULL;
	}
      else
	icon_name = NULL;
    }

  if (icon_name && !*icon_name)
    icon_name = NULL;

  /* If we have an empty label and an icon, we set the label to NULL so it is
     removed. */
  if (label && !*label && icon_name)
    {
      if (free_label)
	g_free (label);
      label = NULL;
    }

  /* Check if we need to rearrange the child widgets, i.e. if we have a label
     but no label widget or we don't have a label but we do have a label
     widget. And the same for the icon. */
  if ((apply_label && ((label && !label_widget) || (!label && label_widget)))
      || (apply_icon && ((icon_name && !icon_widget) || (!icon_name && icon_widget))))
    {
      gb_button_normal_recreate_children (widget, data, label, icon_name);

      gb_button_normal_find_child_widgets (widget, &label_widget,
					   &icon_widget);
      if (label_widget)
	glade_util_copy_translation_properties (widget, label_p,
						label_widget, "GtkLabel::label");

      goto out;
    }

  /* Just update the current label & icon widgets. */
  if (apply_label)
    gtk_label_set_text_with_mnemonic (GTK_LABEL (label_widget),
				      label ? label : "");
						   
  if (apply_icon && icon_widget && icon_name)
    {
      gchar *old_icon_name;

      /* Remove the old icon_name stored in the widget data, and remove the
	 pixmap from the project, if necessary. */
      old_icon_name = gtk_object_get_data (GTK_OBJECT (icon_widget),
					   GladeIconKey);
      glade_project_remove_pixmap (data->project, old_icon_name);

      if (glade_util_check_is_stock_id (icon_name))
	{
	  gtk_image_set_from_stock (GTK_IMAGE (icon_widget), icon_name,
				    GTK_ICON_SIZE_BUTTON);
	}
      else
	{
	  gtk_image_set_from_file (GTK_IMAGE (icon_widget), icon_name);
	  glade_project_add_pixmap (data->project, icon_name);
	}

      gtk_object_set_data_full (GTK_OBJECT (icon_widget), GladeIconKey,
				g_strdup (icon_name),
				icon_name ? g_free : NULL);
    }

 out:

  if (free_label)
    g_free (label);
  if (free_icon_name)
    g_free (icon_name);
}


static void
gb_button_normal_write_source (GtkWidget * widget,
			       GbWidgetWriteSourceData * data,
			       const gchar *label_p)
{
  GtkWidget *label_widget, *icon_widget;
  gchar *group_string = "", *group_string2 = "", *type, *stock_id;
  GType widget_type;

  widget_type = G_OBJECT_TYPE (widget);
  if (widget_type == GTK_TYPE_BUTTON)
    type = "button";
  else if (widget_type == GTK_TYPE_TOGGLE_BUTTON)
    type = "toggle_button";
  else if (widget_type == GTK_TYPE_CHECK_BUTTON)
    type = "check_button";
  else
    {
      type = "radio_button";
      group_string = "NULL, ";
      group_string2 = "NULL";
    }

  stock_id = gtk_object_get_data (GTK_OBJECT (widget), GladeButtonStockIDKey);
  if (stock_id)
    {
      if (G_OBJECT_TYPE (widget) == GTK_TYPE_BUTTON)
	{
	  source_add (data,
		      "  %s = gtk_button_new_from_stock (%s);\n",
		      data->wname, source_make_string (stock_id, FALSE));
	}
      else
	{
	  source_add (data,
		      "  %s = gtk_%s_new_with_mnemonic (%s%s);\n"
		      "  gtk_button_set_use_stock (GTK_BUTTON (%s), TRUE);\n",
		      data->wname, type, group_string,
		      source_make_string (stock_id, FALSE), data->wname);
	}
    }
  else
    {
      gb_button_normal_find_child_widgets (widget, &label_widget,
					   &icon_widget);

      if (label_widget && !icon_widget)
	{
	  gchar *label_text, *comments;
	  gboolean translatable, context;

	  label_text = glade_util_get_label_text (label_widget);
	  glade_util_get_translation_properties (label_widget,
						 "GtkLabel::label",
						 &translatable, &comments,
						 &context);

	  source_add_translator_comments (data, translatable, comments);
	  source_add (data, "  %s = gtk_%s_new_with_mnemonic (%s%s);\n",
		      data->wname, type, group_string,
		      source_make_string_full (label_text, data->use_gettext && translatable, context));
	  g_free (label_text);
	}
      else
	{
	  source_add (data, "  %s = gtk_%s_new (%s);\n", data->wname, type,
		      group_string2);
	}
    }
}


void
gb_button_find_radio_group (GtkWidget *widget, GladeFindGroupData *find_data)
{
  if (find_data->found_widget)
    return;

  if (GTK_IS_RADIO_BUTTON (widget) && GB_IS_GB_WIDGET (widget))
    {
      if (gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget)) == find_data->group)
	{
	  find_data->found_widget = widget;
	  return;
	}
    }

  if (GTK_IS_CONTAINER (widget))
    gb_widget_children_foreach (widget,
				(GtkCallback) gb_button_find_radio_group,
				find_data);
}


/*
 * GNOME DIALOG BUTTONS. GtkButton only.
 */

/* This tries to find the child label & icon of the button. If the button
   contains other widgets it returns FALSE, i.e. if the user has added other
   widgets instead. */
static gboolean
gb_button_gnome_find_child_widgets (GtkWidget *widget,
				     GtkWidget **label,
				     GtkWidget **icon)
{
  GtkWidget *child;

  *label = *icon = NULL;

  /* Check it has a child. If it doesn't output a warning. */
  child = GTK_BIN (widget)->child;
  g_return_val_if_fail (child != NULL, FALSE);

  /* Check if it has just a GtkLabel child. */
  if (GTK_IS_LABEL (child))
    {
      *label = child;
      return TRUE;
    }

  /* Check if it has just a GtkImage child. */
  if (GTK_IS_IMAGE (child))
    {
      *icon = child;
      return TRUE;
    }

  /* If it contains an icon and a label it must be a GtkAlignment with a
     GtkHBox in it. FIXME: GnomeDialog doesn't use a GtkAlignment at present,
     but we check for it just in case. */
  if (GTK_IS_ALIGNMENT (child))
    child = GTK_BIN (child)->child;

  /* GNOME uses a hbox inside a hbox, so check for that. */
  if (child && GTK_IS_HBOX (child)
      && g_list_length (GTK_BOX (child)->children) == 1)
    {
      GtkBoxChild *hbox_child;
      hbox_child = GTK_BOX (child)->children->data;
      child = hbox_child->widget;
    }

  /* FIXME: Gnome may not show the icon and/or label according to config
     settings. That may break Glade. */

  /* Now check for a hbox with a GtkImage and GtkLabel children. */
  if (child && GTK_IS_HBOX (child)
      && g_list_length (GTK_BOX (child)->children) <= 2)
    {
      GList *children;
      GtkBoxChild *child1, *child2;

      children = GTK_BOX (child)->children;
      child1 = children->data;
      child2 = children->next ? children->next->data : NULL;

      if (child1 && child2)
	{
	  if (GTK_IS_LABEL (child1->widget) && GTK_IS_IMAGE (child2->widget))
	    {
	      *label = child1->widget;
	      *icon = child2->widget;
	      return TRUE;
	    }
	}
      else if (GTK_IS_LABEL (child1->widget))
	{
	  *label = child1->widget;
	  return TRUE;
	}
      else if (GTK_IS_IMAGE (child1->widget))
	{
	  *icon = child1->widget;
	  return TRUE;
	}
    }

  return FALSE;
}


static void
gb_button_gnome_get_properties (GtkWidget * widget,
				 GbWidgetGetArgData * data,
				 gchar *stock_id_p,
				 gchar *label_p,
				 gchar *icon_p)
{
  gchar *stock_id, *label_text;
  gboolean label_sensitive = TRUE;
  gboolean icon_sensitive = TRUE;
  GtkWidget *label, *icon;

  /* We only allow the GTK_ICON_SIZE_BUTTON size for stock items & icons. */
  if (data->action == GB_SHOWING)
    {
      property_set_stock_item_icon_size (stock_id_p, GTK_ICON_SIZE_BUTTON);
      property_set_icon_size (icon_p, GTK_ICON_SIZE_BUTTON);
    }

  stock_id = gtk_object_get_data (GTK_OBJECT (widget), GladeButtonStockIDKey);
  /* In the XML the stock item name is actually saved as the label property,
     and the use_stock property specifies that it is a stock item.
     Also, use_underline needs to be set. */
  if (data->action == GB_SAVING)
    {
      if (stock_id)
	{
	  gb_widget_output_stock_item (data, "label", stock_id);
	  gb_widget_output_bool (data, "use_stock", TRUE);
	}
    }
  else
    {
      gb_widget_output_stock_item (data, stock_id_p, stock_id);
    }


  if (stock_id)
    {
      label_sensitive = FALSE;
      icon_sensitive = FALSE;
    }
  else
    {
      if (gb_button_gnome_find_child_widgets (widget, &label, &icon))
	{
	  gchar *icon_name = NULL;

	  if (label)
	    label_text = (gchar*) gtk_label_get_label (GTK_LABEL (label));
	  else
	    label_text = "";

	  gb_widget_output_translatable_text (data, label_p, label_text);

	  /* We always save use_underline as TRUE, though we don't load it. */
	  if (data->action == GB_SAVING && !icon)
	    gb_widget_output_bool (data, "use_underline", TRUE);

	  if (icon)
	    icon_name = gtk_object_get_data (GTK_OBJECT (icon), GladeIconKey);

	  /* We never output the icon when saving, as it will be saved as a
	     child widget. */
	  gb_widget_output_icon (data, icon_p, icon_name);
	}
      else
	{
	  label_sensitive = FALSE;
	  icon_sensitive = FALSE;
	}
    }

  if (data->action == GB_SHOWING)
    {
      if (!label_sensitive)
	gb_widget_output_translatable_text (data, label_p, "");
      property_set_sensitive (label_p, label_sensitive);

      if (!icon_sensitive)
	  gb_widget_output_pixmap_filename (data, icon_p, "");
      property_set_sensitive (icon_p, icon_sensitive);
    }
}


static void
gb_button_gnome_set_stock_id (GtkWidget *widget,
			       GbWidgetSetArgData * data,
			       gchar *stock_id,
			       gchar *label_p,
			       gchar *icon_p)
{
  gboolean is_stock_item = FALSE;
  const gchar *label_text = "";

  if (stock_id && stock_id[0])
    is_stock_item = TRUE;

  if (is_stock_item)
    {
      gtk_button_set_use_stock (GTK_BUTTON (widget), TRUE);
      gtk_button_set_label (GTK_BUTTON (widget), stock_id);
      gtk_object_set_data_full (GTK_OBJECT (widget), GladeButtonStockIDKey,
				g_strdup (stock_id), g_free);
    }
  else
    {
      /* Change the button back to a gnome button with a simple label. */
      gtk_button_set_use_stock (GTK_BUTTON (widget), FALSE);
      label_text = gtk_widget_get_name (widget);
      gtk_button_set_label (GTK_BUTTON (widget), label_text);
      gtk_object_set_data (GTK_OBJECT (widget), GladeButtonStockIDKey, NULL);
    }

  /* If the widget's properties are displayed, we update the sensitivity of
     the label and icon, according to whether a stock item is selected. */
  if (data->action == GB_APPLYING && property_get_widget () == widget)
    {
      property_set_sensitive (label_p, !is_stock_item);
      property_set_sensitive (icon_p, !is_stock_item);

      /* Turn off auto-apply, and set the label. */
      property_set_auto_apply (FALSE);
      property_set_text (label_p, label_text);
      property_set_filename (icon_p, "");
      property_set_auto_apply (TRUE);
    }
}


static void
gb_button_gnome_recreate_children (GtkWidget *widget,
				    GbWidgetSetArgData * data,
				    gchar *label, gchar *icon_name)
{
  GtkWidget *parent = widget;

  /* Remove any existing children. */
  if (GTK_BIN (widget)->child)
    gtk_container_remove (GTK_CONTAINER (widget), GTK_BIN (widget)->child);

  /* If both label and icon are needed, we put them in a hbox, otherwise we
     just add the label or icon to the button. */
  if (label && icon_name)
    {
      GtkWidget *hbox;

      /* It uses a hbox in a hbox to keep the contents in the center, rather
	 than an alignment like GTK+ does. */
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_widget_show (hbox);
      gtk_container_add (GTK_CONTAINER (widget), hbox);

      parent = gtk_hbox_new (FALSE, 0);
      gtk_widget_show (parent);
      gtk_box_pack_start (GTK_BOX (hbox), parent, TRUE, FALSE, 2);
    }

  if (label)
    {
      GtkWidget *label_widget;

      label_widget = gtk_label_new_with_mnemonic (label);
      gtk_widget_show (label_widget);

      if (GTK_IS_BOX (parent))
	gtk_box_pack_end (GTK_BOX (parent), label_widget, FALSE, FALSE, 2);
      else
	gtk_container_add (GTK_CONTAINER (parent), label_widget);
    }

  if (icon_name)
    {
      GtkWidget *icon_widget;

      icon_widget = gtk_image_new ();

      gtk_object_set_data_full (GTK_OBJECT (icon_widget), GladeIconKey,
				g_strdup (icon_name),
				icon_name ? g_free : NULL);

      if (glade_util_check_is_stock_id (icon_name))
	{
	  gtk_image_set_from_stock (GTK_IMAGE (icon_widget), icon_name,
				    GTK_ICON_SIZE_BUTTON);
	}
      else
	{
	  gtk_image_set_from_file (GTK_IMAGE (icon_widget), icon_name);
	  glade_project_add_pixmap (data->project, icon_name);
	}
      gtk_widget_show (icon_widget);

      if (GTK_IS_BOX (parent))
	gtk_box_pack_start (GTK_BOX (parent), icon_widget, FALSE, FALSE, 0);
      else
	gtk_container_add (GTK_CONTAINER (parent), icon_widget);
    }
}


void
gb_button_gnome_set_properties	(GtkWidget * widget,
				 GbWidgetSetArgData * data,
				 gchar *stock_id_p,
				 gchar *label_p,
				 gchar *icon_p)
{
  gchar *stock_id = NULL, *label, *icon_name = NULL;
  gboolean apply_stock_id = FALSE, apply_label, apply_icon;
  gboolean free_label = FALSE, free_icon_name = FALSE;
  GtkWidget *label_widget, *icon_widget;

  label = gb_widget_input_text (data, label_p);
  apply_label = data->apply;
  if (data->action == GB_APPLYING)
    free_label = TRUE;

  /* When loading, if "use_stock" is TRUE, then the label is the stock item. */
  if (data->action == GB_LOADING)
    {
      gboolean is_stock_item = gb_widget_input_bool (data, "use_stock");
      if (is_stock_item)
	{
	  stock_id = label;
	  apply_stock_id = apply_label;
	}
    }
  else
    {
      stock_id = gb_widget_input_stock_item (data, stock_id_p);
      apply_stock_id = data->apply;
    }

  if (apply_stock_id)
    {
      gb_button_gnome_set_stock_id (widget, data, stock_id, label_p, icon_p);
      goto out;
    }

  icon_name = gb_widget_input_icon (data, icon_p);
  apply_icon = data->apply;

  /* If neither icon nor label is set, we don't touch the button as it may
     have other child widgets. */
  if (!apply_label && !apply_icon)
    return;

  gb_button_gnome_find_child_widgets (widget, &label_widget,
				       &icon_widget);

  if (!apply_label)
    {
      if (label_widget)
	{
	  label = g_strdup (gtk_label_get_label (GTK_LABEL (label_widget)));
	  free_label = TRUE;
	}
      else
	label = NULL;
    }

  if (!apply_icon)
    {
      if (icon_widget)
	{
	  icon_name = gtk_object_get_data (GTK_OBJECT (icon_widget),
					   GladeIconKey);
	  if (icon_name && *icon_name)
	    {
	      icon_name = g_strdup (icon_name);
	      free_icon_name = TRUE;
	    }
	  else
	    icon_name = NULL;
	}
      else
	icon_name = NULL;
    }

  if (icon_name && !*icon_name)
    icon_name = NULL;

  /* If we have an empty label and an icon, we set the label to NULL so it is
     removed. */
  if (label && !*label && icon_name)
    {
      if (free_label)
	g_free (label);
      label = NULL;
    }

  /* Check if we need to rearrange the child widgets, i.e. if we have a label
     but no label widget or we don't have a label but we do have a label
     widget. And the same for the icon. */
  if ((apply_label && ((label && !label_widget) || (!label && label_widget)))
      || (apply_icon && ((icon_name && !icon_widget) || (!icon_name && icon_widget))))
    {
      gb_button_gnome_recreate_children (widget, data, label, icon_name);

      goto out;
    }

  /* Just update the current label & icon widgets. */
  if (apply_label)
    gtk_label_set_text_with_mnemonic (GTK_LABEL (label_widget),
				      label ? label : "");
						   
  if (apply_icon && icon_widget && icon_name)
    {
      gchar *old_icon_name;

      /* Remove the old icon_name stored in the widget data, and remove the
	 pixmap from the project, if necessary. */
      old_icon_name = gtk_object_get_data (GTK_OBJECT (icon_widget),
					   GladeIconKey);
      glade_project_remove_pixmap (data->project, old_icon_name);

      if (glade_util_check_is_stock_id (icon_name))
	{
	  gtk_image_set_from_stock (GTK_IMAGE (icon_widget), icon_name,
				    GTK_ICON_SIZE_BUTTON);
	}
      else
	{
	  gtk_image_set_from_file (GTK_IMAGE (icon_widget), icon_name);
	  glade_project_add_pixmap (data->project, icon_name);
	}

      gtk_object_set_data_full (GTK_OBJECT (icon_widget), GladeIconKey,
				g_strdup (icon_name),
				icon_name ? g_free : NULL);
    }

 out:

  if (free_label)
    g_free (label);
  if (free_icon_name)
    g_free (icon_name);
}


static void
gb_button_gnome_write_source (GtkWidget * widget,
			      GbWidgetWriteSourceData * data,
			      const gchar *label_p)
{
  gchar *stock_id;
  gboolean translatable, context;
  gchar *comments;

  glade_util_get_translation_properties (widget, label_p, &translatable,
					 &comments, &context);

  stock_id = gtk_object_get_data (GTK_OBJECT (widget), GladeButtonStockIDKey);
  if (stock_id)
    {
      source_add (data,
		  "  gnome_dialog_append_button (GNOME_DIALOG (%s), %s);\n",
		  data->component_name,
		  source_make_string (stock_id, FALSE));
    }
  else
    {
      GtkWidget *label, *icon;
      gchar *label_text = NULL;

      gb_button_gnome_find_child_widgets (widget, &label, &icon);

      if (label)
	label_text = glade_util_get_label_text (label);

      if (icon)
	{
	  gchar *icon_id;

	  icon_id = gtk_object_get_data (GTK_OBJECT (icon), GladeIconKey);

	  source_add_translator_comments (data, translatable, comments);
	  source_add (data,
		      "  gnome_dialog_append_button_with_pixmap (GNOME_DIALOG (%s),\n"
		      "                                          %s, ",
		      data->component_name,
		      label_text ? source_make_string_full (label_text, data->use_gettext && translatable, context) : "");

	  source_add (data,
		      "%s);\n",
		      source_make_string (icon_id, FALSE));
	}
      else
	{
	  source_add (data,
		      "  gnome_dialog_append_button (GNOME_DIALOG (%s), %s);\n",
		      data->component_name,
		      label_text ? source_make_string (label_text, FALSE) : "");
	}

      g_free (label_text);
    }

  /* We want to get a pointer to the widget so we can output the standard
     source, in case handlers have been added to the buttons.
     We can just get the last widget in the GnomeDialog's list of buttons.
     Note that we have to make sure that the button isn't added to its
     parent widget again elsewhere. */
  source_add (data, "  %s = GTK_WIDGET (g_list_last (GNOME_DIALOG (%s)->buttons)->data);\n",
	      data->wname, data->component_name);

  /* We set this to FALSE, so the code to add it to its parent is skipped. */
  data->create_widget = FALSE;
}






/*
 * GbButton functions.
 */

/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
void
gb_button_get_standard_properties (GtkWidget * widget,
				   GbWidgetGetArgData * data,
				   gchar *stock_id_p,
				   gchar *label_p,
				   gchar *icon_p,
				   gchar *relief_p,
				   gchar *focus_on_click_p)
{
  GladeButtonType button_type;
  gboolean relief_visible = TRUE, icon_file_selection = TRUE;
  gint i;

  button_type = gb_button_get_button_type (widget);

  switch (button_type)
    {
    case GLADE_BUTTON_NORMAL:
      gb_button_normal_get_properties (widget, data, stock_id_p, label_p,
				       icon_p);
      break;
    case GLADE_BUTTON_DIALOG:
      gb_button_normal_get_properties (widget, data, stock_id_p, label_p,
				       icon_p);
      break;
    case GLADE_BUTTON_GNOME_DIALOG:
      gb_button_gnome_get_properties (widget, data, stock_id_p, label_p,
				      icon_p);
      /* Gnome dialog buttons can only have stock icons. */
      icon_file_selection = FALSE;
      break;
    }

  /* Handle the Relief property here, as it is simple. */
  if (data->action == GB_SHOWING)
    {
      property_set_visible (relief_p, relief_visible);
      property_set_icon_filesel (icon_p, icon_file_selection);
    }

  if (relief_visible)
    {
      for (i = 0; i < GladeReliefChoicesSize; i++)
	{
	  if (GladeReliefValues[i] == GTK_BUTTON (widget)->relief)
	    gb_widget_output_choice (data, relief_p, i, GladeReliefSymbols[i]);
	}
    }

  if (focus_on_click_p)
    gb_widget_output_bool (data, focus_on_click_p,
			   gtk_button_get_focus_on_click (GTK_BUTTON (widget)));
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_button_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  GladeButtonType button_type;
  gboolean response_id_visible = FALSE;

  gb_button_get_standard_properties (widget, data, StockButton, Label, Icon,
				     Relief, FocusOnClick);

  /* Handle the response id for dialog buttons here. */
  button_type = gb_button_get_button_type (widget);
  if (button_type == GLADE_BUTTON_DIALOG)
    {
      gint response_id;

      response_id_visible = TRUE;

      response_id = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), GladeDialogResponseIDKey));
      /* We save as an int, but show as a string. */
      if (data->action == GB_SHOWING)
	{
	  property_set_combo (ResponseID,
			      gb_dialog_response_id_to_string (response_id));
	}
      else
	{
	  save_int (data, ResponseID, response_id);
	}
    }

  if (data->action == GB_SHOWING)
    property_set_visible (ResponseID, response_id_visible);
}


void
gb_button_set_standard_properties	(GtkWidget * widget,
					 GbWidgetSetArgData * data,
					 gchar *stock_id_p,
					 gchar *label_p,
					 gchar *icon_p,
					 gchar *relief_p,
					 gchar *focus_on_click_p)
{
  GladeButtonType button_type;
  gchar *relief;
  gint i;

  button_type = gb_button_get_button_type (widget);

  switch (button_type)
    {
    case GLADE_BUTTON_NORMAL:
      gb_button_normal_set_properties (widget, data, stock_id_p, label_p,
				       icon_p);
      break;
    case GLADE_BUTTON_DIALOG:
      gb_button_normal_set_properties (widget, data, stock_id_p, label_p,
				       icon_p);
      break;
    case GLADE_BUTTON_GNOME_DIALOG:
      gb_button_gnome_set_properties (widget, data, stock_id_p, label_p,
				      icon_p);
      break;
    }

  /* Handle the Relief property here, as it is simple. */
  relief = gb_widget_input_choice (data, relief_p);
  if (data->apply)
    {
      for (i = 0; i < GladeReliefChoicesSize; i++)
	{
	  if (!strcmp (relief, GladeReliefChoices[i])
	      || !strcmp (relief, GladeReliefSymbols[i]))
	    {
	      gtk_button_set_relief (GTK_BUTTON (widget),
				     GladeReliefValues[i]);
	      break;
	    }
	}
    }

  if (focus_on_click_p)
    {
      gboolean focus_on_click;

      focus_on_click = gb_widget_input_bool (data, focus_on_click_p);
      if (data->apply)
	gtk_button_set_focus_on_click (GTK_BUTTON (widget), focus_on_click);
    }
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GladeButtonType button_type;
  gchar *old_stock_id, *stock_id;
  gboolean set_response_id = FALSE;

  /* Remember the old stock id. */
  old_stock_id = gtk_object_get_data (GTK_OBJECT (widget),
				      GladeButtonStockIDKey);

  gb_button_set_standard_properties (widget, data, StockButton, Label, Icon,
				     Relief, FocusOnClick);

  /* Handle the response id for dialog buttons here. */
  button_type = gb_button_get_button_type (widget);
  if (button_type == GLADE_BUTTON_DIALOG)
    {
      gint response_id = 0;

      /* See if the stock id has been changed. Note that we only compare the
	 pointers, since the old string will have been freed. If the stock id
	 has been changed and is set, we use the corresponding response id,
	 unless it is also being set. */
      stock_id = gtk_object_get_data (GTK_OBJECT (widget),
				      GladeButtonStockIDKey);
      if (data->action == GB_APPLYING && stock_id && stock_id != old_stock_id)
	{
	  int i;

	  for (i = 0; i < GladeStockResponsesSize; i++)
	    {
	      if (GladeStockResponses[i].stock_id
		  && !strcmp (GladeStockResponses[i].stock_id, stock_id))
		{
		  response_id = GladeStockResponses[i].response_id;
		  set_response_id = TRUE;
		  break;
		}
	    }
	}


      /* We save as an int, but show as a string. */
      if (data->action == GB_LOADING)
	{
	  response_id = gb_widget_input_int (data, ResponseID);
	}
      else
	{
	  char *response_name  = gb_widget_input_combo (data, ResponseID);
	  if (data->apply)
	    response_id = gb_dialog_response_id_from_string (response_name);
	}

      if (data->apply || set_response_id)
	{
	  gboolean secondary = FALSE;

	  gtk_object_set_data (GTK_OBJECT (widget), GladeDialogResponseIDKey,
			       GINT_TO_POINTER (response_id));

	  if (response_id == GTK_RESPONSE_HELP)
	    secondary = TRUE;
	  gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (widget->parent),
					      widget, secondary);
	}

      /* If we are setting the response id as a result of a change of stock
	 id, then we set the field in the property editor now. */
      if (property_get_widget () == widget && set_response_id)
	{
	  char *old_response_id, *new_response_id;

	  old_response_id = property_get_combo (ResponseID, NULL, NULL);
	  new_response_id = gb_dialog_response_id_to_string (response_id);

	  if (strcmp (new_response_id, old_response_id))
	    {
	      property_set_auto_apply (FALSE);
	      property_set_combo (ResponseID, new_response_id);
	      property_set_auto_apply (TRUE);
	    }
	}
    }
}


static void
gb_button_remove_contents (GtkWidget * menuitem,
			   GtkWidget * widget)
{
  GtkWidget *child;

  g_return_if_fail (GTK_IS_BIN (widget));

  child = GTK_BIN (widget)->child;
  if (child && !GB_IS_PLACEHOLDER (child))
    editor_delete_widget (child);

  /* Reset the stock_id key. */
  gtk_object_set_data (GTK_OBJECT (widget), GladeButtonStockIDKey, NULL);
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkButton, with signals pointing to
 * other functions in this file.
 */
void
gb_button_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{
  GtkWidget *child;

  /* If the button's child isn't a placeholder, add a command to remove the
     button's contents. */
  child = GTK_BIN (widget)->child;
  if (child && !GB_IS_PLACEHOLDER (child))
    {
      GtkWidget *menuitem;

      menuitem = gtk_menu_item_new_with_label (_("Remove Button Contents"));
      gtk_widget_show (menuitem);
      gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_button_remove_contents), widget);
    }
}


void
gb_button_write_standard_source (GtkWidget * widget,
				 GbWidgetWriteSourceData * data,
				 const gchar *label_p)
{
  GladeButtonType button_type;
  gboolean set_relief = TRUE;
  gint i;

  if (data->create_widget)
    {
      button_type = gb_button_get_button_type (widget);

      switch (button_type)
	{
	case GLADE_BUTTON_NORMAL:
	  gb_button_normal_write_source (widget, data, label_p);
	  break;
	case GLADE_BUTTON_DIALOG:
	  gb_button_normal_write_source (widget, data, label_p);
	  break;
	case GLADE_BUTTON_GNOME_DIALOG:
	  gb_button_gnome_write_source (widget, data, label_p);
	  break;
	}
    }

  gb_widget_write_standard_source (widget, data);

  if (set_relief && GTK_BUTTON (widget)->relief != GTK_RELIEF_NORMAL)
    {
      for (i = 0; i < GladeReliefChoicesSize; i++)
	{
	  if (GladeReliefValues[i] == GTK_BUTTON (widget)->relief)
	    source_add (data,
			"  gtk_button_set_relief (GTK_BUTTON (%s), %s);\n",
			data->wname, GladeReliefSymbols[i]);
	}
    }

  if (!gtk_button_get_focus_on_click (GTK_BUTTON (widget)))
    {
      source_add (data,
		  "  gtk_button_set_focus_on_click (GTK_BUTTON (%s), FALSE);\n",
		  data->wname);
    }
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gb_button_write_standard_source (widget, data, Label);
}


/* Note that Check/Radio/Toggle buttons use this function as well. */
void
gb_button_destroy (GtkWidget * widget, GbWidgetDestroyData * data)
{
  gchar *filename;

  filename = gtk_object_get_data (GTK_OBJECT (widget), GladeIconKey);
  glade_project_remove_pixmap (data->project, filename);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_button_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = button_xpm;
  gbwidget.tooltip = _("Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_button_new;
  gbwidget.gb_widget_create_properties = gb_button_create_properties;
  gbwidget.gb_widget_get_properties = gb_button_get_properties;
  gbwidget.gb_widget_set_properties = gb_button_set_properties;
  gbwidget.gb_widget_create_popup_menu = gb_button_create_popup_menu;
  gbwidget.gb_widget_write_source = gb_button_write_source;
  gbwidget.gb_widget_destroy = gb_button_destroy;

  return &gbwidget;
}
