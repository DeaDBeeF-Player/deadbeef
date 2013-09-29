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

#include <string.h>

#include <gtk/gtkmenu.h>

#include "gladeconfig.h"

#include "glade.h"
#include "gbwidget.h"
#include "glade_gtk12lib.h"
#include "palette.h"
#include "glade_palette.h"
#include "glade_project.h"
#include "glade_project_window.h"


static void on_palette_select_item (GladePalette *palette,
				    gchar *item_name);
static void palette_finish_new_component (GtkWidget *component,
					  GbWidgetNewData *data);

static GdkColormap *colormap;


void
palette_init ()
{
  glade_palette = glade_palette_new ();
  gtk_window_move (GTK_WINDOW (glade_palette), 0, 250);
  gtk_window_set_type_hint (GTK_WINDOW (glade_palette),
			    GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_wmclass (GTK_WINDOW (glade_palette), "palette", "Glade");
  gtk_window_add_accel_group (GTK_WINDOW (glade_palette),
			      glade_get_global_accel_group ());

  gtk_signal_connect (GTK_OBJECT (glade_palette), "delete_event",
		      GTK_SIGNAL_FUNC (palette_hide), NULL);

  gtk_signal_connect (GTK_OBJECT (glade_palette), "hide",
                      GTK_SIGNAL_FUNC (glade_project_window_uncheck_palette_menu_item),
                      NULL);

  gtk_window_set_title (GTK_WINDOW (glade_palette), _("Palette"));
  gtk_container_set_border_width (GTK_CONTAINER (glade_palette), 0);
  colormap = gtk_widget_get_colormap (glade_palette);

  gtk_signal_connect (GTK_OBJECT (glade_palette), "select_item",
		      GTK_SIGNAL_FUNC (on_palette_select_item), NULL);
}


void
palette_add_gbwidget (GbWidget *gbwidget,
                      gchar *section,
                      gchar *name)
{
  gboolean show_section;

  show_section = (!strcmp (section, "NotShown")) ? FALSE : TRUE;

  /* Create the widget's pixmap, if it has one. */
  if (gbwidget->pixmap_struct && !gbwidget->gdkpixmap)
    gbwidget->gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &(gbwidget->mask), NULL, gbwidget->pixmap_struct);

  if (show_section)
    glade_palette_add_widget (GLADE_PALETTE (glade_palette),
			      _(section), name,
			      gbwidget->gdkpixmap, gbwidget->mask,
			      gbwidget->tooltip);

}


void
palette_show (GtkWidget * widget,
	      gpointer data)
{
  gtk_widget_show (glade_palette);
  /* This maps the window, which also de-iconifies it according to ICCCM. */
  gdk_window_show (GTK_WIDGET (glade_palette)->window);
  gdk_window_raise (GTK_WIDGET (glade_palette)->window);
}


gint
palette_hide (GtkWidget * widget,
	      gpointer data)
{
  gtk_widget_hide (glade_palette);
  return TRUE;
}


static void
on_palette_select_item (GladePalette *palette,
			gchar *class_name)
{
  GtkType type;

  /* Ignore the selector. */
  if (class_name == NULL)
    return;

  /* If we don't have a project open, just reset the selection. Hopefully
     we'll make the palette insensitive in that case so this won't happen. */
  if (current_project == NULL)
    {
      glade_palette_reset_selection (GLADE_PALETTE (glade_palette), FALSE);
      return;
    }

  /* See if a toplevel item was selected - a window, dialog or menu.
     If it was, we create a new component and reset the palette. */
  type = g_type_from_name (class_name);
#if 0
  g_print ("Class Name: %s Type: %li\n", class_name, type);
#endif

  /* We just return if the type isn't found (e.g. for the custom widget). */
  if (type == 0)
    return;

  if (gtk_type_is_a (type, gtk_window_get_type ())
      || gtk_type_is_a (type, gtk_menu_get_type ()))
    {
      gb_widget_new_full (class_name, TRUE, NULL, NULL, 0, 0,
			  palette_finish_new_component, GB_CREATING, NULL);
      glade_palette_reset_selection (GLADE_PALETTE (glade_palette), FALSE);
    }
}


static void
palette_finish_new_component (GtkWidget *component, GbWidgetNewData *data)
{
  glade_project_add_component (data->project, component);
  glade_project_show_component (data->project, component);
  gb_widget_show_properties (component);
}
