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

/*
 * File		: glade_plugin.c
 * Description	: Provides support for plugin widget libraries which use Args.
 *		  Unfinished, experimental code at present.
 *		  For Gnome, we may want to think about bonobo components,
 *		  rather like COM/ActiveX components in Delphi/VB.
 */

#include "gladeconfig.h"

#ifndef _WIN32
#include <gmodule.h>
#endif
#include <gtk/gtk.h>

#include "gbwidget.h"
#include "glade_plugin.h"
#include "palette.h"
#include "utils.h"


/* This loads extra libraries of widgets which we will access via the GTK+
   Arg functions. */
void
glade_plugin_load_plugins (void)
{
#if 0
  /* FIXME: Here we just test for /usr/local/gnome/lib/libdruid.so, but we'll
     support some user configuration later. */
  gchar *module_dir = "/usr/local/gnome/lib";
  gchar *module_file = "druid";

  typedef gboolean (*GladePluginInitFunc) (gint **widget_types,
					   gchar ****xpm_data);

  GModule *module;
  gchar *library_file;
  gpointer symbol_value;
  GladePluginInitFunc plugin_init_func;
  gboolean status;
  gint *widget_types;
  gchar ***widget_xpms;
  gint i;

  /* Check if dynamic loading is supported by the current platform. */
  if (!g_module_supported ())
    return;

  library_file = g_module_build_path (module_dir, module_file);
  if (!glade_util_file_exists (library_file))
    return;

  module = g_module_open (library_file, 0);
  g_return_if_fail (module != NULL);

  /* Try to find the initialization function. */
  if (!g_module_symbol (module, "glade_plugin_init", &symbol_value))
    {
      g_warning ("Plugin library contains no initialization function: %s\n",
		 library_file);
      return;
    }

  plugin_init_func = (GladePluginInitFunc) symbol_value;
  status = (*plugin_init_func) (&widget_types, &widget_xpms);
  g_return_if_fail (status == TRUE);

  g_return_if_fail (widget_types != NULL);

  for (i = 0; widget_types[i]; i++)
    {
      GbWidget *gbwidget;
      gchar *class_name;

      gbwidget = glade_plugin_new ();
      if (widget_xpms)
	{
	  gbwidget->pixmap_struct = widget_xpms[i];
	}

      class_name = gtk_type_name (widget_types[i]);
      gbwidget->tooltip = class_name;
      gb_widget_register_gbwidget (class_name, gbwidget);
      palette_add_gbwidget (gbwidget, "Plugins", class_name);
    }
#endif
}


/* This finds out which Args the widget (and any ancestors) support. */
static void
glade_plugin_create_properties (GtkWidget * widget,
				GbWidgetCreateArgData * data)
{
#if 0
  GtkObjectClass *klass;
  GtkArg *args;
  guint32 *arg_flags;
  guint n_args;
  gint arg;
  gchar flags[16], *pos;
  GtkType type;

  g_print ("In glade_plugin_create_properties\n");

  klass = GTK_OBJECT (widget)->klass;
  g_return_if_fail (klass != NULL);

  type = klass->type;
  while (type != 0)
    {
      args = gtk_object_query_args (type, &arg_flags, &n_args);

      for (arg = 0; arg < n_args; arg++)
	{
	  pos = flags;
	  /* We use one-character flags for simplicity. */
	  if (arg_flags[arg] & GTK_ARG_READABLE)
	    *pos++ = 'r';
	  if (arg_flags[arg] & GTK_ARG_WRITABLE)
	    *pos++ = 'w';
	  if (arg_flags[arg] & GTK_ARG_CONSTRUCT)
	    *pos++ = 'x';
	  if (arg_flags[arg] & GTK_ARG_CONSTRUCT_ONLY)
	    *pos++ = 'X';
	  if (arg_flags[arg] & GTK_ARG_CHILD_ARG)
	    *pos++ = 'c';
	  *pos = '\0';

	  g_print ("ARG: %s (%s:%s)\n",
		   args[arg].name, gtk_type_name (args[arg].type), flags);
	}

      g_free (args);
      g_free (arg_flags);

      type = gtk_type_parent (type);
    }
#endif
}


/* This creates a new GbWidget which may have different tooltip/icon fields,
   but the functions will all be the same, since we can handle Args
   generically. */
GbWidget*
glade_plugin_new ()
{
  GbWidget *gbwidget;

  gbwidget = g_new (GbWidget, 1);
  gb_widget_init_struct (gbwidget);
  gbwidget->gb_widget_create_properties = glade_plugin_create_properties;

  return gbwidget;
}


