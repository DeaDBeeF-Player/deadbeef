/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-1999  Damon Chaplin
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

#include <stdio.h>
#include <errno.h>

#include <libxml/parser.h>

#include "gladeconfig.h"
#include "glade.h"
#include "glade_clipboard.h"
#include "glade_project.h"
#include "glade_project_window.h"
#include "gbwidget.h"
#include "editor.h"
#include "load.h"
#include "property.h"
#include "palette.h"
#include "tree.h"
#include "utils.h"

/* This is the global clipboard. */
GtkWidget *glade_clipboard;

void
glade_app_init (void)
{
  /* Initialize debugging flags from the GLADE_DEBUG environment variable. */
  glade_debug_init ();

  palette_init ();
  gb_widgets_init ();
  editor_init ();
  tree_init ();
  glade_clipboard = glade_clipboard_new ();

  property_init ();
}


static void
write_window_geometry (FILE *fp, gchar *window_name, GtkWindow *window)
{
  gint x, y, width, height;

  gtk_window_get_size (window, &width, &height);
  gtk_window_get_position (window, &x, &y);
  fprintf (fp,
	   "<window id=\"%s\" visible=\"%d\" x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/>\n",
	   window_name, GTK_WIDGET_VISIBLE (window), x, y, width, height);
}


static gchar*
get_settings_filename (void)
{
  const gchar *home_dir;

  home_dir = g_get_home_dir ();
  if (!home_dir)
    {
#ifdef _WIN32
      home_dir = "C:\\";
#else
      /* Just use the root directory. Though it probably won't be writable. */
      home_dir = G_DIR_SEPARATOR_S;
#endif
    }

  return g_strdup_printf ("%s%s.glade2", home_dir, G_DIR_SEPARATOR_S);
}


static gboolean
get_window_setting (xmlNodePtr child, gchar *property, gint *retval)
{
  xmlChar *tmp;
  gint num_items;

  tmp = xmlGetProp (child, property);
  if (!tmp)
    return FALSE;

  num_items = sscanf (tmp, "%d", retval);
  xmlFree (tmp);

  return (num_items == 1) ? TRUE : FALSE;
}


void
restore_window_geometry (xmlNodePtr child, GtkWindow *window,
			 gboolean *show_window)
{
  int visible, x, y, width, height;
  gboolean has_visibility, has_x, has_y, has_width, has_height;

  g_return_if_fail (GTK_IS_WINDOW (window));

  has_visibility = get_window_setting (child, "visible", &visible);
  has_x = get_window_setting (child, "x", &x);
  has_y = get_window_setting (child, "y", &y);
  has_width = get_window_setting (child, "width", &width);
  has_height = get_window_setting (child, "height", &height);

  if (has_visibility && show_window)
    *show_window = visible;

  if (has_x && has_y)
    gtk_window_move (GTK_WINDOW (window), x, y);

  if (has_width && has_height)
    gtk_window_resize (GTK_WINDOW (window), width, height);
}


void
glade_load_settings (gpointer project_window,
		     GtkWidget *palette,
		     gboolean  *show_palette,
		     GtkWidget *property_editor,
		     gboolean  *show_property_editor,
		     GtkWidget *widget_tree,
		     gboolean  *show_widget_tree,
		     GtkWidget *clipboard,
		     gboolean  *show_clipboard)
{
  gchar *filename;
  xmlDocPtr doc;
  xmlNodePtr child;

  g_return_if_fail (project_window != NULL);
  g_return_if_fail (GTK_IS_WINDOW (palette));
  g_return_if_fail (GTK_IS_WINDOW (property_editor));
  g_return_if_fail (GTK_IS_WINDOW (widget_tree));
  g_return_if_fail (GTK_IS_WINDOW (clipboard));

  filename = get_settings_filename ();
  doc = xmlParseFile (filename);
  g_free (filename);

  if (!doc)
    return;

  child = xmlDocGetRootElement (doc);

  for (child = child->children; child; child = child->next)
    {
      xmlChar *id;

      if (!xmlStrEqual (child->name, "window"))
	continue;

      id = xmlGetProp (child, "id");
      if (id)
	{
	  if (xmlStrEqual ("MainWindow", id))
	    restore_window_geometry (child, GTK_WINDOW (((GladeProjectWindow*)project_window)->window), NULL);
	  else if (xmlStrEqual ("Palette", id))
	    restore_window_geometry (child, GTK_WINDOW (palette),
				     show_palette);
	  else if (xmlStrEqual ("PropertyEditor", id))
	    restore_window_geometry (child, GTK_WINDOW (property_editor),
				     show_property_editor);
	  else if (xmlStrEqual ("WidgetTree", id))
	    restore_window_geometry (child, GTK_WINDOW (widget_tree),
				     show_widget_tree);
	  else if (xmlStrEqual ("Clipboard", id))
	    restore_window_geometry (child, GTK_WINDOW (clipboard),
				     show_clipboard);

	  xmlFree (id);
	}
    }

  xmlFreeDoc (doc);
}


void
glade_save_settings (gpointer project_window,
		     GtkWidget *palette,
		     GtkWidget *property_editor,
		     GtkWidget *widget_tree,
		     GtkWidget *clipboard)
{
  gchar *filename;
  FILE *fp;

  g_return_if_fail (project_window != NULL);
  g_return_if_fail (GTK_IS_WINDOW (palette));
  g_return_if_fail (GTK_IS_WINDOW (property_editor));
  g_return_if_fail (GTK_IS_WINDOW (widget_tree));
  g_return_if_fail (GTK_IS_WINDOW (clipboard));

  filename = get_settings_filename ();
  fp = fopen (filename, "w");
  if (!fp)
    {
      g_free (filename);
      return;
    }

  fprintf (fp, "<glade-settings>\n");
  write_window_geometry (fp, "MainWindow", GTK_WINDOW (((GladeProjectWindow*)project_window)->window));
  write_window_geometry (fp, "Palette", GTK_WINDOW (palette));
  write_window_geometry (fp, "PropertyEditor", GTK_WINDOW (property_editor));
  write_window_geometry (fp, "WidgetTree", GTK_WINDOW (widget_tree));
  write_window_geometry (fp, "Clipboard", GTK_WINDOW (clipboard));
  fprintf (fp, "</glade-settings>\n");
  fclose (fp);
  g_free (filename);
}


/* Simple user interface functions. */
void
glade_show_project_window (void)
{

}


void
glade_hide_project_window (void)
{

}


void
glade_show_palette (void)
{
  palette_show (NULL, NULL);
}


void
glade_hide_palette (void)
{
  palette_hide (NULL, NULL);
}


void
glade_show_property_editor (void)
{
  property_show (NULL, NULL);
}


void
glade_hide_property_editor (void)
{
  property_hide (NULL, NULL);
}


void
glade_show_widget_tree (void)
{
  tree_show (NULL, NULL);
}


void
glade_hide_widget_tree (void)
{
  tree_hide (NULL, NULL);
}


void
glade_show_clipboard	(void)
{
  gtk_widget_show (glade_clipboard);
  gdk_window_show (GTK_WIDGET (glade_clipboard)->window);
  gdk_window_raise (GTK_WIDGET (glade_clipboard)->window);
}


void
glade_hide_clipboard	(void)
{
  glade_util_close_window (glade_clipboard);
}


void
glade_show_widget_tooltips (gboolean show)
{
  gb_widget_set_show_tooltips (show);
}


void
glade_show_grid	(gboolean show)
{
  editor_set_show_grid (show);
}


void
glade_snap_to_grid (gboolean snap)
{
  editor_set_snap_to_grid (snap);
}


#if 0
/* Changed editor_show_grid_settings_dialog and
   editor_show_grid_settings_dialog to take a widget parameter, to use for
   selecting a transient parent.
   These functions don't know about any widgets. Since they're unused,
   they're commented out. I guess it would be even better to remove them
   outright. */
void
glade_show_grid_settings (void)
{
  editor_show_grid_settings_dialog ();
}


void
glade_show_snap_settings (void)
{
  editor_show_snap_settings_dialog ();
}
#endif


GtkAccelGroup*
glade_get_global_accel_group (void)
{
  /* This is our global accelerator group that should be added to every
     window, so our main accelerator keys work in every window. */
  static GtkAccelGroup *accel_group = NULL;

  if (accel_group == NULL)
    accel_group = gtk_accel_group_new ();

  return accel_group;
}


gchar*
glade_get_error_message	(GladeStatusCode status)
{
  switch (status)
    {
    case GLADE_STATUS_OK:
      return _("OK");
    case GLADE_STATUS_ERROR:
      return _("Error");

    case GLADE_STATUS_SYSTEM_ERROR:
      return _("System Error");

      /* File related errors. */
    case GLADE_STATUS_FILE_OPEN_ERROR:
      return _("Error opening file");
    case GLADE_STATUS_FILE_READ_ERROR:
      return _("Error reading file");
    case GLADE_STATUS_FILE_WRITE_ERROR:
      return _("Error writing file");

    case GLADE_STATUS_INVALID_DIRECTORY:
      return _("Invalid directory");

      /* XML Parsing errors. */
    case GLADE_STATUS_INVALID_VALUE:
      return _("Invalid value");
    case GLADE_STATUS_INVALID_ENTITY:
      return _("Invalid XML entity");
    case GLADE_STATUS_START_TAG_EXPECTED:
      return _("Start tag expected");
    case GLADE_STATUS_END_TAG_EXPECTED:
      return _("End tag expected");
    case GLADE_STATUS_DATA_EXPECTED:
      return _("Character data expected");
    case GLADE_STATUS_CLASS_ID_MISSING:
      return _("Class id missing");
    case GLADE_STATUS_CLASS_UNKNOWN:
      return _("Class unknown");
    case GLADE_STATUS_INVALID_COMPONENT:
      return _("Invalid component");
    case GLADE_STATUS_EOF:
      return _("Unexpected end of file");

    default:
      return _("Unknown error code");
    }
}


/*************************************************************************
 * GladeError - an ADT to represent an error which occurred in Glade.
 *              Currently it is only used for writing source, but it may be
 *		extended for all errors, since GladeStatus doesn't really
 *		provide enough detail to show the user useful messages.
 *************************************************************************/

GladeError*
glade_error_new			(void)
{
  GladeError *error;

  error = g_new (GladeError, 1);
  error->status = GLADE_STATUS_OK;
  error->system_errno = 0;
  error->message = NULL;

  return error;
}


/* Creates a GladeError with the given Glade status code and the printf-like
   message and arguments. */
GladeError*
glade_error_new_general		(GladeStatusCode  status,
				 gchar		 *message,
				 ...)
{
  GladeError *error;
  va_list args;

  error = glade_error_new ();
  error->status = status;

  va_start (args, message);
  error->message = g_strdup_vprintf (message, args);
  va_end (args);

  return error;
}


/* Creates a GladeError using the system errno and the printf-like
   message and arguments. This must be called immediately after the error
   is detected, so that errno is still valid and can be copied into the
   GladeError. */
GladeError*
glade_error_new_system		(gchar		 *message,
				 ...)
{
  GladeError *error;
  va_list args;

  error = glade_error_new ();
  error->status = GLADE_STATUS_SYSTEM_ERROR;
  error->system_errno = errno;

  va_start (args, message);
  error->message = g_strdup_vprintf (message, args);
  va_end (args);

  return error;
}


/* Frees the GladeError and its contents. */
void
glade_error_free			(GladeError      *error)
{
  g_free (error->message);
  g_free (error);
}
