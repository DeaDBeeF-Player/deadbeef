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
#include "../graphics/toolbutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

/*
 * Our internal representation is slightly different to the saved XML,
 * since it is awkward to support all the GtkToolButton properties.
 * We only support stock buttons, or simple labels and icons. We don't
 * support arbitrary label and icon widgets, partly because the Glade DTD
 * can't handle them at present.
 *
 * If a stock_id is set within Glade, both the label and icon are set
 * and these properties are made insensitive in the property editor.
 *
 * If no stock_id is set within Glade, the user can enter a label and use
 * a stock icon or an icon from a file. Note that if a stock icon is used it
 * will be saved as the "stock_id" property and the label will then override
 * the stock label. So when loading if we find a "stock_id" property we check
 * if a "label" property is present. If it isn't, we set Glade's "stock_id"
 * property. If it is, we set Glade's "icon" property to the stock value
 * instead. Phew.
 */
static gchar *StockButton = "GtkToolButton::stock_id";
static gchar *Label = "GtkToolButton::label";
static gchar *Icon = "GtkToolButton::icon";
static gchar *VisibleHorz = "GtkToolButton|GtkToolItem::visible_horizontal";
static gchar *VisibleVert = "GtkToolButton|GtkToolItem::visible_vertical";
static gchar *IsImportant = "GtkToolButton|GtkToolItem::is_important";



/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkToolButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_tool_button_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget, *image;
  GbWidget *pixmap_gbwidget;

  /* Place the pixmap icon in the button initially (even when loading). */
  pixmap_gbwidget = gb_widget_lookup_class ("GtkImage");
  if (pixmap_gbwidget)
    {
      image = gtk_image_new_from_pixmap (pixmap_gbwidget->gdkpixmap,
					 pixmap_gbwidget->mask);
    }
  else
    {
      image = gtk_image_new ();
      g_warning ("Couldn't find GtkPixmap data");
    }
  gtk_widget_show (image);

  new_widget = (GtkWidget*) gtk_tool_button_new (image, "");

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_tool_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_stock_item (StockButton, _("Stock Button:"),
			   _("The stock button to use"),
			   GTK_ICON_SIZE_LARGE_TOOLBAR);
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
  property_add_icon (Icon, _("Icon:"),
		     _("The icon to display"),
		     GTK_ICON_SIZE_LARGE_TOOLBAR);
  property_add_bool (VisibleHorz, _("Show Horizontal:"),
		     _("If the item is visible when the toolbar is horizontal"));
  property_add_bool (VisibleVert, _("Show Vertical:"),
		     _("If the item is visible when the toolbar is vertical"));
  property_add_bool (IsImportant, _("Is Important:"),
		     _("If the item's text should be shown when the toolbar's mode is GTK_TOOLBAR_BOTH_HORIZ"));
}


void
gb_tool_button_get_standard_properties (GtkWidget *widget,
					GbWidgetGetArgData * data,
					gchar *stock_id_p,
					gchar *label_p,
					gchar *icon_p,
					gchar *visible_horz_p,
					gchar *visible_vert_p,
					gchar *is_important_p)
{
  gchar *stock_id, *label, *icon_name;
  gboolean label_sensitive = TRUE;
  gboolean icon_sensitive = TRUE;

  /* We use the icon size from the parent toolbar. */
  if (data->action == GB_SHOWING)
    {
      GtkIconSize icon_size;

      icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (widget->parent));
      property_set_stock_item_icon_size (stock_id_p, icon_size);
      property_set_icon_size (icon_p, icon_size);
    }

  stock_id = gtk_object_get_data (GTK_OBJECT (widget),
				  GladeToolButtonStockIDKey);
  gb_widget_output_stock_item (data, stock_id_p, stock_id);
  if (stock_id)
    {
      label_sensitive = FALSE;
      icon_sensitive = FALSE;
    }
  else
    {
      label = (gchar*) gtk_tool_button_get_label (GTK_TOOL_BUTTON (widget));
      gb_widget_output_translatable_text (data, label_p, label);

      /* We always save use_underline as TRUE, though we don't load it. */
      if (data->action == GB_SAVING)
	gb_widget_output_bool (data, "use_underline", TRUE);

      icon_name = gtk_object_get_data (GTK_OBJECT (widget),
				       GladeToolButtonIconKey);
      /* When saving, we save stock icons as "stock_id". */
      if (data->action == GB_SAVING
	  && glade_util_check_is_stock_id (icon_name))
	gb_widget_output_icon (data, "stock_id", icon_name);
      else
	gb_widget_output_icon (data, icon_p, icon_name);
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

  gb_widget_output_bool (data, visible_horz_p,
			 gtk_object_get_data (GTK_OBJECT (widget), visible_horz_p)
			 != NULL ? FALSE : TRUE);

  gb_widget_output_bool (data, visible_vert_p,
			 gtk_object_get_data (GTK_OBJECT (widget), visible_vert_p)
			 != NULL ? FALSE : TRUE);

  gb_widget_output_bool (data, is_important_p,
			 gtk_tool_item_get_is_important (GTK_TOOL_ITEM (widget)));
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_tool_button_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_tool_button_get_standard_properties (widget, data,
					  StockButton, Label, Icon,
					  VisibleHorz, VisibleVert,
					  IsImportant);
}



static void
gb_tool_button_set_icon (GtkWidget *widget,
			 GbWidgetSetArgData * data,
			 gchar *icon_name)
{
  GtkWidget *image;
  gchar *old_icon_name;

  if (icon_name && !*icon_name)
    icon_name = NULL;

  /* Remove the old icon_name stored in the widget data, and remove the
     pixmap from the project, if necessary. */
  old_icon_name = gtk_object_get_data (GTK_OBJECT (widget),
				       GladeToolButtonIconKey);
  glade_project_remove_pixmap (data->project, old_icon_name);

  if (glade_util_check_is_stock_id (icon_name))
    {
      GtkIconSize icon_size;

      icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (widget->parent));
      image = gtk_image_new_from_stock (icon_name, icon_size);
    }
  else if (icon_name)
    {
      image = gtk_image_new_from_file (icon_name);
      glade_project_add_pixmap (data->project, icon_name);
    }
  else
    {
      GbWidget *pixmap_gbwidget;

      pixmap_gbwidget = gb_widget_lookup_class ("GtkImage");
      if (pixmap_gbwidget)
	{
	  image = gtk_image_new_from_pixmap (pixmap_gbwidget->gdkpixmap,
					     pixmap_gbwidget->mask);
	}
      else
	{
	  image = gtk_image_new ();
	  g_warning ("Couldn't find GtkPixmap data");
	}
    }

  gtk_widget_show (image);
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (widget), image);

  gtk_object_set_data_full (GTK_OBJECT (widget), GladeToolButtonIconKey,
			    g_strdup (icon_name),
			    icon_name ? g_free : NULL);
}


static void
gb_tool_button_set_stock_id (GtkWidget *widget,
			     GbWidgetSetArgData * data,
			     gchar *stock_id,
			     gboolean apply_label,
			     gchar *label_p,
			     gchar *icon_p)
{
  gboolean is_stock_item;

  is_stock_item = glade_util_check_is_stock_id (stock_id);

  if (is_stock_item)
    {
      gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (widget), stock_id);
      gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (widget), NULL);
      gtk_tool_button_set_label (GTK_TOOL_BUTTON (widget), NULL);

      /* If we are loading, and label is set, then this is a stock icon
	 rather than a stock button (at least within Glade). */
      if (data->action == GB_LOADING && apply_label)
	{
	  gtk_object_set_data_full (GTK_OBJECT (widget),
				    GladeToolButtonIconKey,
				    g_strdup (stock_id), g_free);
	}
      else
	{
	  gtk_object_set_data_full (GTK_OBJECT (widget),
				    GladeToolButtonStockIDKey,
				    g_strdup (stock_id), g_free);
	  gtk_object_set_data (GTK_OBJECT (widget),
			       GladeToolButtonIconKey, NULL);
	}
    }
  else
    {
      /* It isn't a stock ID, so assume "None" has been selected and make
	 it a normal toolbar button. */
      GtkWidget *image;
      GbWidget *pixmap_gbwidget;

      pixmap_gbwidget = gb_widget_lookup_class ("GtkImage");
      if (pixmap_gbwidget)
	{
	  image = gtk_image_new_from_pixmap (pixmap_gbwidget->gdkpixmap,
					     pixmap_gbwidget->mask);
	}
      else
	{
	  image = gtk_image_new ();
	  g_warning ("Couldn't find GtkPixmap data");
	}
      gtk_widget_show (image);
      gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (widget), image);

      gtk_tool_button_set_label (GTK_TOOL_BUTTON (widget), "");

      gtk_object_set_data (GTK_OBJECT (widget),
			   GladeToolButtonStockIDKey, NULL);
      gtk_object_set_data (GTK_OBJECT (widget),
			   GladeToolButtonIconKey, NULL);
    }

  /* If the widget's properties are displayed, we update the sensitivity of
     the label and icon, according to whether a stock item is selected. */
  if (data->action == GB_APPLYING && property_get_widget () == widget)
    {
      property_set_sensitive (label_p, !is_stock_item);
      property_set_sensitive (icon_p, !is_stock_item);

      /* Turn off auto-apply, and set the label. */
      property_set_auto_apply (FALSE);
      property_set_text (label_p, "");
      property_set_filename (icon_p, "");
      property_set_auto_apply (TRUE);
    }
}


void
gb_tool_button_set_standard_properties (GtkWidget *widget,
					GbWidgetSetArgData * data,
					gchar *stock_id_p,
					gchar *label_p,
					gchar *icon_p,
					gchar *visible_horz_p,
					gchar *visible_vert_p,
					gchar *is_important_p)
{
  gchar *label, *stock_id, *icon_name;
  gboolean apply_label;
  gboolean visible_horz, visible_vert, is_important;

  label = gb_widget_input_text (data, label_p);
  apply_label = data->apply;

  stock_id = gb_widget_input_stock_item (data, stock_id_p);
  if (data->apply)
    {
      gb_tool_button_set_stock_id (widget, data, stock_id, apply_label,
				   label_p, icon_p);
    }
  else if (data->action == GB_LOADING)
    {
      /* Migrate old 2.0 XML files. */
      gboolean use_stock = gb_widget_input_bool (data, "use_stock");
      if (data->apply && use_stock && apply_label)
	{
	  gb_tool_button_set_stock_id (widget, data, label, FALSE,
				       label_p, icon_p);
	  apply_label = FALSE;
	}
    }

  icon_name = gb_widget_input_icon (data, icon_p);
  if (data->apply)
    {
      gb_tool_button_set_icon (widget, data, icon_name);
    }
  else if (data->action == GB_LOADING)
    {
      /* Migrate old 2.0 XML files. */
      icon_name = gb_widget_input_icon (data, "stock_pixmap");
      if (data->apply)
	{
	  gb_tool_button_set_icon (widget, data, icon_name);
	}
      else
	{
	  icon_name = gb_widget_input_icon (data, "icon");
	  if (data->apply)
	    {
	      gb_tool_button_set_icon (widget, data, icon_name);
	    }
	}
    }

  if (apply_label)
    {
      gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (widget), TRUE);
      gtk_tool_button_set_label (GTK_TOOL_BUTTON (widget), label);
    }

  if (data->action == GB_APPLYING)
    g_free (label);


  visible_horz = gb_widget_input_bool (data, visible_horz_p);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), visible_horz_p,
			   visible_horz ? NULL : "FALSE");
    }

  visible_vert = gb_widget_input_bool (data, visible_vert_p);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), visible_vert_p,
			   visible_vert ? NULL : "FALSE");
    }

  is_important = gb_widget_input_bool (data, is_important_p);
  if (data->apply)
    {
      gtk_tool_item_set_is_important (GTK_TOOL_ITEM (widget), is_important);
    }
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_tool_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gb_tool_button_set_standard_properties (widget, data,
					  StockButton, Label, Icon,
					  VisibleHorz, VisibleVert,
					  IsImportant);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkToolButton, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_tool_button_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_tool_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *stock_id, *label, *icon_name;
  gboolean translatable, context;
  gchar *comments;

  stock_id = gtk_object_get_data (GTK_OBJECT (widget),
				  GladeToolButtonStockIDKey);
  icon_name = gtk_object_get_data (GTK_OBJECT (widget),
				   GladeToolButtonIconKey);
  label = (gchar*) gtk_tool_button_get_label (GTK_TOOL_BUTTON (widget));

  glade_util_get_translation_properties (widget, Label, &translatable,
					 &comments, &context);

  if (data->create_widget)
    {
      if (stock_id)
	{
	  /* Stock Button */
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_tool_button_new_from_stock (%s);\n",
		      data->wname, source_make_string (stock_id, FALSE));
	}
      else if (icon_name)
	{
	  /* Icon and Label */
	  source_ensure_decl (data, "  GtkWidget *tmp_image;\n");

	  if (glade_util_check_is_stock_id (icon_name))
	    {
	      source_add (data,
			  "  tmp_image = gtk_image_new_from_stock (\"%s\", tmp_toolbar_icon_size);\n",
			  icon_name);
	    }
	  else
	    {
	      source_create_pixmap (data, "tmp_image", icon_name);
	    }

	  source_add (data, "  gtk_widget_show (tmp_image);\n");

	  source_add_translator_comments (data, translatable, comments);
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_tool_button_new (tmp_image, %s);\n",
		      data->wname,
		      label ? source_make_string_full (label, data->use_gettext && translatable, context) : "NULL");
	}
      else
	{
	  /* Just a Label */
	  source_add_translator_comments (data, translatable, comments);
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_tool_button_new (NULL, %s);\n",
		      data->wname,
		      label ? source_make_string_full (label, data->use_gettext && translatable, context) : "NULL");
	}
    }

  gb_widget_write_standard_source (widget, data);

  if (gtk_object_get_data (GTK_OBJECT (widget), VisibleHorz) != NULL)
    {
      source_add (data,
		  "  gtk_tool_item_set_visible_horizontal (GTK_TOOL_ITEM (%s), FALSE);\n",
		  data->wname);
    }

  if (gtk_object_get_data (GTK_OBJECT (widget), VisibleVert) != NULL)
    {
      source_add (data,
		  "  gtk_tool_item_set_visible_vertical (GTK_TOOL_ITEM (%s), FALSE);\n",
		  data->wname);
    }

  if (gtk_tool_item_get_is_important (GTK_TOOL_ITEM (widget)))
    {
      source_add (data,
		  "  gtk_tool_item_set_is_important (GTK_TOOL_ITEM (%s), TRUE);\n",
		  data->wname);
    }
}


/* Note that Radio/Toggle tool buttons use this function as well. */
void
gb_tool_button_destroy (GtkWidget * widget, GbWidgetDestroyData * data)
{
  gchar *filename;

  /* This can be a stock id or a filename. But glade_project_remove_pixmap()
     will just ignore it if it isn't a project pixmap file. */
  filename = gtk_object_get_data (GTK_OBJECT (widget),
				  GladeToolButtonIconKey);
  glade_project_remove_pixmap (data->project, filename);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_tool_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_tool_button_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = toolbutton_xpm;
  gbwidget.tooltip = _("Toolbar Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_tool_button_new;
  gbwidget.gb_widget_create_properties	= gb_tool_button_create_properties;
  gbwidget.gb_widget_get_properties	= gb_tool_button_get_properties;
  gbwidget.gb_widget_set_properties	= gb_tool_button_set_properties;
  gbwidget.gb_widget_write_source	= gb_tool_button_write_source;
  gbwidget.gb_widget_destroy		= gb_tool_button_destroy;

/*
  gbwidget.gb_widget_create_popup_menu	= gb_tool_button_create_popup_menu;
*/

  return &gbwidget;
}

