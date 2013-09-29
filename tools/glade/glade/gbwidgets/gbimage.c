
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

#include <gtk/gtkiconfactory.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkspinbutton.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/image.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

/* This could be a stock icon or a filename. */
static gchar *Icon = "GtkImage::icon";

/* This is the stock icon size, and isn't relevant when a filename is used. */
static gchar *IconSize = "GtkImage::icon_size";

static gchar *XAlign = "Image|GtkMisc::xalign";
static gchar *YAlign = "Image|GtkMisc::yalign";
static gchar *XPad = "Image|GtkMisc::xpad";
static gchar *YPad = "Image|GtkMisc::ypad";

static gchar *IconName = "GtkImage::icon_name";
static gchar *PixelSize = "GtkImage::pixel_size";


static const gchar *GladeIconSizeChoices[] =
{"Menu", "Small Toolbar", "Large Toolbar", "Button", "Drag & Drop",
 "Dialog", NULL};
static const gint GladeIconSizeValues[] =
{
  GTK_ICON_SIZE_MENU,
  GTK_ICON_SIZE_SMALL_TOOLBAR,
  GTK_ICON_SIZE_LARGE_TOOLBAR,
  GTK_ICON_SIZE_BUTTON,
  GTK_ICON_SIZE_DND,
  GTK_ICON_SIZE_DIALOG
};
static const gchar *GladeIconSizeSymbols[] =
{
  "GTK_ICON_SIZE_MENU",
  "GTK_ICON_SIZE_SMALL_TOOLBAR",
  "GTK_ICON_SIZE_LARGE_TOOLBAR",
  "GTK_ICON_SIZE_BUTTON",
  "GTK_ICON_SIZE_DND",
  "GTK_ICON_SIZE_DIALOG"
};
const int GladeIconSizeChoicesSize = sizeof (GladeIconSizeValues) / sizeof (GladeIconSizeValues[0]);


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkImage, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_image_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;
  new_widget = gtk_image_new_from_pixmap (gbwidget.gdkpixmap, gbwidget.mask);
  return new_widget;
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_image_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_icon (Icon, _("Icon:"),
		     _("The icon to display"),
		     GLADE_ICON_SIZE_ANY);
  property_add_named_icon (IconName, _("Named Icon:"),
			   _("The named icon to use"));
  property_add_choice (IconSize, _("Icon Size:"),
		       _("The stock icon size"),
		       GladeIconSizeChoices);
  property_add_int_range (PixelSize, _("Pixel Size:"),
			  _("The size of the named icon in pixels, or -1 to use the Icon Size property"),
			  -1, 1000, 1, 10, 1);

  property_add_float_range (XAlign, _("X Align:"),
			    _("The horizontal alignment"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (YAlign, _("Y Align:"),
			    _("The vertical alignment"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_int_range (XPad, _("X Pad:"), _("The horizontal padding"),
			  0, 1000, 1, 10, 1);
  property_add_int_range (YPad, _("Y Pad:"), _("The vertical padding"),
			  0, 1000, 1, 10, 1);
}


static void
check_visible_sizes (const gchar *stock_id, gboolean show_all)
{
  GtkIconSet *icon_set;
  GtkIconSize *sizes;
  gint n_sizes, i, j;
  GtkWidget *option_menu, *menu;
  gboolean item_visible[G_N_ELEMENTS(GladeIconSizeValues)];
  GList *children;

  for (j = 0; j < GladeIconSizeChoicesSize; j++)
    item_visible[j] = show_all;

  if (!show_all)
    {
      icon_set = gtk_icon_factory_lookup_default (stock_id);
      if (icon_set)
	{
	  gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);
	  /* Figure out which of our choices should be visible. */
	  for (i = 0; i < n_sizes; i++)
	    {
	      for (j = 0; j < GladeIconSizeChoicesSize; j++)
		{
		  if (sizes[i] == GladeIconSizeValues[j])
		    item_visible[j] = TRUE;
		}
	    }
	  g_free (sizes);
	}
    }

  /* Show or Hide the items as appropriate. */
  option_menu = property_get_value_widget (IconSize);
  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

  menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
  g_return_if_fail (GTK_IS_MENU (menu));

  children = GTK_MENU_SHELL (menu)->children;
  for (j = 0; j < GladeIconSizeChoicesSize; j++)
    {
      GtkWidget *item;

      item = children->data;

      if (item_visible[j])
	gtk_widget_show (item);
      else
	gtk_widget_hide (item);

      children = children->next;
    }
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_image_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gchar *icon = gtk_object_get_data (GTK_OBJECT (widget), GladeIconKey);
  gchar *icon_name = NULL;
  gboolean is_stock_icon, is_named_icon = FALSE;
  gint i, pixel_size;
  GtkImageType storage_type;

  storage_type = gtk_image_get_storage_type (GTK_IMAGE (widget));
  pixel_size = gtk_image_get_pixel_size (GTK_IMAGE (widget));

  is_stock_icon = glade_util_check_is_stock_id (icon);

  if (storage_type == GTK_IMAGE_ICON_NAME)
    {
      g_object_get (widget,
		    "icon_name", &icon_name,
		    NULL);
      if (icon_name && *icon_name)
	is_named_icon = TRUE;
    }

  if (data->action == GB_SAVING)
    {
      /* When saving we want to use "stock" or "pixbuf", depending on whether
	 it is a stock icon or an oridinary pixmap file. */
      if (is_stock_icon)
	gb_widget_output_icon (data, "stock", icon);
      else
	gb_widget_output_icon (data, "pixbuf", icon);
    }
  else
    {
      gb_widget_output_icon (data, Icon, icon);
    }

  if (is_stock_icon || is_named_icon)
    {
      if (data->action == GB_SHOWING)
	{
	  for (i = 0; i < GladeIconSizeChoicesSize; i++)
	    {
	      if (GladeIconSizeValues[i] == GTK_IMAGE (widget)->icon_size)
		gb_widget_output_choice (data, IconSize, i,
					 GladeIconSizeSymbols[i]);
	    }

	  check_visible_sizes (icon, is_named_icon);
	}
      else
	{
	  /* We have to save the size as an integer, which sucks a bit.
	     The integer is the GtkIconSize enum value, not the actual size.*/
	  if (is_stock_icon || pixel_size == -1)
	    gb_widget_output_int (data, IconSize,
				  GTK_IMAGE (widget)->icon_size);
	}
    }

  gb_widget_output_named_icon (data, IconName, icon_name);

  /* The icon size only applies to stock icons or named icons. 
     The pixel size only applies to named icons. */
  if (data->action == GB_SHOWING)
    {
      property_set_sensitive (IconSize, is_stock_icon || is_named_icon);
      property_set_sensitive (PixelSize, is_named_icon);
    }

  /* We only want to save the pixel size for named icons, and only if it is
     not -1. */
  if (data->action == GB_SHOWING
      || (storage_type == GTK_IMAGE_ICON_NAME && pixel_size != -1))
    gb_widget_output_int (data, PixelSize, pixel_size);

  gb_widget_output_float (data, XAlign, GTK_MISC (widget)->xalign);
  gb_widget_output_float (data, YAlign, GTK_MISC (widget)->yalign);
  gb_widget_output_int (data, XPad, GTK_MISC (widget)->xpad);
  gb_widget_output_int (data, YPad, GTK_MISC (widget)->ypad);
}


/* Check the icon size is valid for the stock item, and if not pick
   the first valid size. */
static GtkIconSize
check_icon_size (const gchar *stock_id, GtkIconSize icon_size)
{
  GtkIconSet *icon_set;
  GtkIconSize *sizes, retval = GTK_ICON_SIZE_BUTTON;
  gint n_sizes, i;

  icon_set = gtk_icon_factory_lookup_default (stock_id);
  if (icon_set)
    {
      gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);

      for (i = 0; i < n_sizes; i++)
	{
	  if (sizes[i] == icon_size)
	    return icon_size;
	}

      retval = sizes[0];
      g_free (sizes);
    }

  return retval;
}


static void
gb_image_clear_pixmap (GtkWidget *widget, GladeProject *project)
{
  gchar *old_icon;

  /* Remove the old icon stored in the widget data, and remove the
     pixmap from the project, if necessary. */
  old_icon = gtk_object_get_data (GTK_OBJECT (widget), GladeIconKey);
  glade_project_remove_pixmap (project, old_icon);

  gtk_object_set_data (GTK_OBJECT (widget), GladeIconKey, NULL);
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_image_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gfloat xalign, yalign;
  gint xpad, ypad, i, pixel_size;
  gboolean set_alignment = FALSE, set_padding = FALSE, apply_icon_size;
  GtkIconSize icon_size = GTK_ICON_SIZE_BUTTON;
  gchar *icon_size_string, *icon, *icon_name;

  icon_size = GTK_IMAGE (widget)->icon_size;

  if (data->action == GB_APPLYING)
    {
      icon_size_string = gb_widget_input_choice (data, IconSize);
      apply_icon_size = data->apply;
      if (data->apply)
	{
	  for (i = 0; i < GladeIconSizeChoicesSize; i++)
	    {
	      if (!strcmp (icon_size_string, GladeIconSizeChoices[i])
		  || !strcmp (icon_size_string, GladeIconSizeSymbols[i]))
		{
		  icon_size = GladeIconSizeValues[i];
		  break;
		}
	    }
	}
    }
  else
    {
      /* We have to save the size as an integer, which sucks a bit.
	 The integer is the GtkIconSize enum value, not the actual size.*/
      int new_size = gb_widget_input_int (data, IconSize);
      apply_icon_size = data->apply;
      if (data->apply)
	icon_size = new_size;
    }

  /* When loading we use different names. */
  if (data->action == GB_LOADING)
    {
      icon = gb_widget_input_icon (data, "stock");
      if (!data->apply)
	icon = gb_widget_input_icon (data, "pixbuf");
    }
  else
    {
      icon = gb_widget_input_icon (data, Icon);
    }

  if (data->apply)
    {
      gboolean is_stock_icon = glade_util_check_is_stock_id (icon);

      /* Remove the old icon stored in the widget data, and remove the
	 pixmap from the project, if necessary. */
      gb_image_clear_pixmap (widget, data->project);

      gtk_object_set_data_full (GTK_OBJECT (widget), GladeIconKey,
				g_strdup (icon), icon ? g_free : NULL);

      if (is_stock_icon)
	{
	  GtkIconSize new_icon_size;

	  new_icon_size = check_icon_size (icon, icon_size);

	  /* If we are showing this widget's properties, we need to update
	     the sizes option menu. */
	  if (property_get_widget () == widget)
	    {
	      /* We set it sensitive before changing the value, so the child
		 menu item is changed from insensitive to sensitive if needed.
		 Otherwise it may remain insensitive. */
	      property_set_sensitive (IconSize, TRUE);
	      property_set_sensitive (PixelSize, FALSE);

	      property_set_auto_apply (FALSE);

	      check_visible_sizes (icon, FALSE);

	      /* Check the icon size is valid for the stock item, and if not
		 pick the first valid size. */
	      for (i = 0; i < GladeIconSizeChoicesSize; i++)
		{
		  if (GladeIconSizeValues[i] == new_icon_size)
		    {
		      property_set_choice (IconSize, i);
		    }
		}

	      property_set_named_icon (IconName, NULL);

	      property_set_auto_apply (TRUE);
	    }

	  gtk_image_set_from_stock (GTK_IMAGE (widget), icon,
				    new_icon_size);
	}
      else
	{
	  /* If an icon filename is set, use that, otherwise use the icon
	     we use for the palette. */
	  if (icon)
	    {
	      gtk_image_set_from_file (GTK_IMAGE (widget), icon);
	      glade_project_add_pixmap (data->project, icon);
	    }
	  else
	    {
	      gtk_image_set_from_pixmap (GTK_IMAGE (widget),
					 gbwidget.gdkpixmap, gbwidget.mask);
	    }

	  if (property_get_widget () == widget)
	    {
	      /* The icon size isn't relevant to non-stock icons. */
	      property_set_sensitive (IconSize, FALSE);
	      property_set_sensitive (PixelSize, FALSE);

	      property_set_auto_apply (FALSE);
	      property_set_named_icon (IconName, NULL);
	      property_set_auto_apply (TRUE);
	    }
	}

      /* We've recreated the icon with the new size above, so we don't need
	 to apply the size again. */
      apply_icon_size = FALSE;
    }

  /* This is for the named/themed icon. */
  icon_name = gb_widget_input_named_icon (data, IconName);
  if (data->apply)
    {
      /* Clear any stock icon or icon from a file. */
      gb_image_clear_pixmap (widget, data->project);

      gtk_image_set_from_icon_name (GTK_IMAGE (widget), icon_name,
				    icon_size);

      if (property_get_widget () == widget)
	{
	  property_set_sensitive (IconSize, TRUE);
	  property_set_sensitive (PixelSize, TRUE);

	  /* Clear the Icon property. */
	  property_set_auto_apply (FALSE);
	  property_set_icon (Icon, NULL);
	  property_set_auto_apply (TRUE);
	}

      /* We've recreated the icon with the new size above, so we don't need
	 to apply the size again. */
      apply_icon_size = FALSE;
    }

  /* When we set the icon size, we reset the pixel size to -1, otherwise it
     overrides the icon size. */
  if (apply_icon_size)
    {
      gtk_image_set_pixel_size (GTK_IMAGE (widget), -1);
      g_object_set (widget, "icon_size", icon_size, NULL);

      if (property_get_widget () == widget)
	{
	  property_set_auto_apply (FALSE);
	  property_set_int (PixelSize, -1);
	  property_set_auto_apply (TRUE);
	}
    }

  /* GtkImage doesn't like a pixel size of 0 so we just skip that. */
  pixel_size = gb_widget_input_int (data, PixelSize);
  if (data->apply && pixel_size != 0)
    gtk_image_set_pixel_size (GTK_IMAGE (widget), pixel_size);

  xalign = gb_widget_input_float (data, XAlign);
  if (data->apply)
    set_alignment = TRUE;
  else
    xalign = GTK_MISC (widget)->xalign;

  yalign = gb_widget_input_float (data, YAlign);
  if (data->apply)
    set_alignment = TRUE;
  else
    yalign = GTK_MISC (widget)->yalign;

  if (set_alignment)
    gtk_misc_set_alignment (GTK_MISC (widget), xalign, yalign);

  xpad = gb_widget_input_int (data, XPad);
  if (data->apply)
    set_padding = TRUE;
  else
    xpad = GTK_MISC (widget)->xpad;

  ypad = gb_widget_input_int (data, YPad);
  if (data->apply)
    set_padding = TRUE;
  else
    ypad = GTK_MISC (widget)->ypad;

  if (set_padding)
    gtk_misc_set_padding (GTK_MISC (widget), xpad, ypad);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkImage, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_image_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_image_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  /* For GNOME projects menuitem images are handled by the GnomeUIInfo
     structs, so we don't create source code here. */
  if (widget->parent && GTK_IS_IMAGE_MENU_ITEM (widget->parent)
      && glade_project_get_gnome_support (data->project))
    return;

  if (data->create_widget)
    {
      gchar *icon = gtk_object_get_data (GTK_OBJECT (widget), GladeIconKey);
      const gchar *icon_size = "GTK_ICON_SIZE_BUTTON";
      GtkImageType storage_type;
      gint i;

      for (i = 0; i < GladeIconSizeChoicesSize; i++)
	{
	  if (GladeIconSizeValues[i] == GTK_IMAGE (widget)->icon_size)
	    {
	      icon_size = GladeIconSizeSymbols[i];
	    }
	}

      storage_type = gtk_image_get_storage_type (GTK_IMAGE (widget));

      if (storage_type == GTK_IMAGE_ICON_NAME)
	{
	  gint pixel_size = gtk_image_get_pixel_size (GTK_IMAGE (widget));
	  gchar *icon_name;

	  g_object_get (widget,
			"icon_name", &icon_name,
			NULL);

	  source_add (data,
		      "  %s = gtk_image_new_from_icon_name (\"%s\", %s);\n",
		      data->wname, icon_name ? icon_name : "gtk-missing-image",
		      icon_size);

	  if (pixel_size > 0)
	    {
	      source_add (data,
			  "  gtk_image_set_pixel_size (%s, %i);\n",
			  data->wname, pixel_size);
	    }
	}
      else if (glade_util_check_is_stock_id (icon))
	{

	  source_add (data,
		      "  %s = gtk_image_new_from_stock (\"%s\", %s);\n",
		      data->wname, icon, icon_size);
	}
      else
	{
	  source_create_pixmap (data, data->wname, icon);
	}
    }

  gb_widget_write_standard_source (widget, data);

  if (fabs (GTK_MISC (widget)->xalign - 0.5) > 0.0001
      || fabs (GTK_MISC (widget)->yalign - 0.5) > 0.0001)
    source_add (data, "  gtk_misc_set_alignment (GTK_MISC (%s), %g, %g);\n",
	 data->wname, GTK_MISC (widget)->xalign, GTK_MISC (widget)->yalign);

  if (GTK_MISC (widget)->xpad != 0 || GTK_MISC (widget)->ypad != 0)
    source_add (data, "  gtk_misc_set_padding (GTK_MISC (%s), %i, %i);\n",
	     data->wname, GTK_MISC (widget)->xpad, GTK_MISC (widget)->ypad);
}


void
gb_image_destroy (GtkWidget * widget, GbWidgetDestroyData * data)
{
  gb_image_clear_pixmap (widget, data->project);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_image_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_image_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = image_xpm;
  gbwidget.tooltip = _("Image");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_image_new;
  gbwidget.gb_widget_create_properties = gb_image_create_properties;
  gbwidget.gb_widget_get_properties = gb_image_get_properties;
  gbwidget.gb_widget_set_properties = gb_image_set_properties;
  gbwidget.gb_widget_write_source = gb_image_write_source;
  gbwidget.gb_widget_destroy = gb_image_destroy;
/*
   gbwidget.gb_widget_create_popup_menu = gb_image_create_popup_menu;
 */

  return &gbwidget;
}
