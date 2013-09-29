
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

#include <stdlib.h>

#include <glib.h>
#include <gdk/gdk.h>

#include "gladeconfig.h"
#include "debug.h"

static GDebugKey glade_debug_keys[] = {
  { "warnings",   GLADE_DEBUG_WARNINGS },
  { "messages",   GLADE_DEBUG_MESSAGES },
  { "properties", GLADE_DEBUG_PROPERTIES },
};

static guint glade_ndebug_keys = sizeof (glade_debug_keys) / sizeof (GDebugKey);

guint glade_debug_flags = 0;		   /* Global Glade debug flags. */
gboolean glade_debug_messages = FALSE;	   /* If debugging info is output. */
gboolean glade_debug_properties = FALSE;   /* If property info is output. */


static void glade_log_handler (const gchar    *log_domain,
			       GLogLevelFlags  log_level,
			       const gchar    *message,
			       gpointer	   unused_data);

void
glade_debug_init ()
{
  gchar *env_string = NULL;

  env_string = getenv ("GLADE_DEBUG");
  if (env_string != NULL)
    {
      glade_debug_flags = g_parse_debug_string (env_string,
						glade_debug_keys,
						glade_ndebug_keys);
      env_string = NULL;
      if (glade_debug_flags & GLADE_DEBUG_MESSAGES)
	glade_debug_messages = TRUE;

      if (glade_debug_flags & GLADE_DEBUG_PROPERTIES)
	glade_debug_properties = TRUE;
    }

  /* If the GLADE_DEBUG environment variable contains the 'warnings' option,
     we tell GLib to abort() on warning and critical messages from Glade,
     GTK, GDK and GLib. This can be handy for finding out exactly where the
     WARNING/CRITICAL messages occur, with the help of a debugger. */
  if (glade_debug_flags & GLADE_DEBUG_WARNINGS)
    {
      GLogLevelFlags fatal_mask;
	      
      fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
      g_log_set_always_fatal (fatal_mask);
      g_log_set_handler ("GLib", G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL
			 | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL,
			 glade_log_handler, NULL);
      g_log_set_handler ("Gdk", G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL
			 | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL,
			 glade_log_handler, NULL);
      g_log_set_handler ("Gtk", G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL
			 | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL,
			 glade_log_handler, NULL);
      g_log_set_handler (NULL, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL
			 | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL,
			 glade_log_handler, NULL);
    }
}


/* We try to make sure that any X grab is dropped before an abort() is called,
   so we can still use gdb to get a stack trace. */
void
glade_log_handler (const gchar    *log_domain,
		   GLogLevelFlags  log_level,
		   const gchar    *message,
		   gpointer	   unused_data)
{
  g_log_default_handler (log_domain, log_level, message, unused_data);
  if (log_level & G_LOG_FLAG_FATAL)
    {
      gdk_pointer_ungrab (GDK_CURRENT_TIME);
      gdk_flush ();
    }
}
