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

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>

#include "gladeconfig.h"

#include <libxml/parser.h>

#include "glade_atk.h"
#include "glade_project.h"
#include "gbwidget.h"
#include "load.h"
#include "save.h"
#include "utils.h"


static gboolean real_load_project_file (GladeProject *project,
					gboolean found_options);
static void load_ensure_widgets_named (GtkWidget    *widget,
				       GladeProject *project);
static void load_atk_properties (GtkWidget    *widget,
				 GHashTable   *all_widgets);



/* We need this to make sure that numbers are read in a portable syntax,
   instead of using the current locale. This code is from glibc info docs.
   We also set the timezone temporarily to GMT so that we can read in dates
   easily. */
gboolean
load_project_file (GladeProject *project)
{
  gchar *saved_locale, *saved_timezone;
  gboolean status, found_options;

  /* Set the locale to "C". */
  saved_locale = g_strdup (setlocale (LC_NUMERIC, NULL));
  setlocale (LC_NUMERIC, "C");

  /* Set the timezone to "UTC". */
  saved_timezone = glade_util_set_timezone ("UTC");

  /* Load the project XML file, if it exists. Load this first so we know if
     GNOME support is on or not. */
  found_options = glade_project_load_options (project);

  status = real_load_project_file (project, found_options);

  /* Reset the timezone. */
  glade_util_reset_timezone (saved_timezone);

  /* Reset the locale. */
  setlocale (LC_NUMERIC, saved_locale);
  g_free (saved_locale);

  return status;
}


static gboolean
real_load_project_file (GladeProject *project, gboolean found_options)
{
  GbWidgetSetArgData data = { 0 };
  gint i;

  data.project = project;
  data.filename = GladeSessionFile ? GladeSessionFile : project->xml_filename;
  data.xml_buffer = NULL;
  data.status = GLADE_STATUS_OK;
  data.all_widgets = g_hash_table_new (g_str_hash, g_str_equal);

  /* We load the saved session file if that has been set. */
  data.interface = glade_parser_parse_file (data.filename, NULL);
  if (!data.interface)
    return FALSE;
    
  /* If we didn't find a project options file (.gladep), try to guess whether
     it is a GNOME or GTK+ project from the <requires> tags. */
  if (!found_options)
    {
      project->gnome_support = FALSE;

      for (i = 0; i < data.interface->n_requires; i++)
	{
#ifdef USE_GNOME
	  if (!strcmp (data.interface->requires[i], "gnome"))
	    project->gnome_support = TRUE;
#else
	  if (!strcmp (data.interface->requires[i], "gnome"))
	    g_warning ("Glade has been compiled without support for Gnome.");
#endif

#ifdef USE_GNOME_DB
	  if (!strcmp (data.interface->requires[i], "gnomedb"))
	    project->gnome_db_support = TRUE;
#else
	  if (!strcmp (data.interface->requires[i], "gnomedb"))
	    g_warning ("Glade has been compiled without support for Gnome DB.");
#endif
	}
    }

  /* Create each component in the interface. */
  for (i = 0; i < data.interface->n_toplevels; i++)
    {
      data.child_info = NULL;
      data.widget_info = data.interface->toplevels[i];
      gb_widget_load (NULL, &data, NULL);
    }

  /* Now traverse all widgets, loading the ATK properties. We stored a pointer
     to the GladeWidgetInfo inside each widget, which we now use to get the ATK
     properties from. We also use the all_widgets hash to resolve relations. */
  glade_project_foreach_component (data.project,
				   (GtkCallback) load_atk_properties,
				   data.all_widgets);

  /* Destroy the parse data. */
  glade_interface_destroy (data.interface);

  /* Now we need to ensure that all widgets have names. In particular the
     titles of CLists & CTrees, since sometimes it is necessary to create
     these will loading. */
  glade_project_foreach_component (data.project,
				   (GtkCallback) load_ensure_widgets_named,
				   data.project);
  
  g_hash_table_destroy (data.all_widgets);

  return TRUE;
}


gchar *
load_get_value_full (GbWidgetSetArgData * data,
		     const gchar * property_name,
		     gboolean *translatable,
		     gchar **translator_comments,
		     gboolean *context_prefix)
{
  GladeProperty *properties;
  gint nproperties;
  const gchar *tag_name;
  gint i;

  data->apply = FALSE;

  if (!data->widget_info)
    return NULL;
    
  tag_name = property_name;
  while (*tag_name && (*tag_name != ':' || *(tag_name + 1) != ':'))
     tag_name++;
  if (*tag_name)
    tag_name += 2;
  else
    tag_name = property_name;

  if (data->loading_type == GB_STANDARD_PROPERTIES)
    {
      properties = data->widget_info->properties;
      nproperties = data->widget_info->n_properties;
    }
  else
    {
      if (data->child_info)
	{
	  properties = data->child_info->properties;
	  nproperties = data->child_info->n_properties;
	}
      else
	{
	  properties = NULL;
	  nproperties = 0;
	}
    }

  for (i = 0; i < nproperties; i++)
    {
      /* If we are loading properties for a specific agent, skip any properties
	 with no agent set or a different agent. */
      if (data->agent && (!properties[i].agent
			  || strcmp (data->agent, properties[i].agent)))
	continue;

      if (!strcmp (tag_name, properties[i].name))
	{
	  data->apply = TRUE;

	  if (translatable)
	    *translatable = properties[i].translatable;

	  if (translator_comments)
	    *translator_comments = properties[i].translator_comments;

	  if (context_prefix)
	    *context_prefix = properties[i].context_prefix;

	  return properties[i].value;
	}
    }

  return NULL;
}


gchar *
load_get_value (GbWidgetSetArgData * data,
		const gchar * property_name)
{
  return load_get_value_full (data, property_name, NULL, NULL, NULL);
}


/* FIXME: Should the functions returning strings return NULL or "" ?
   It doesn't matter too much as load_get_value() sets data->apply to FALSE
   if no property is found. */

gchar *
load_string (GbWidgetSetArgData * data,
	     const gchar * property_name)
{
  gchar *value, *translator_comments;
  gboolean translatable, context_prefix;

  value = load_get_value_full (data, property_name, &translatable,
			       &translator_comments, &context_prefix);
  if (data->apply)
    glade_util_set_translation_properties (data->widget, property_name,
					   translatable, translator_comments,
					   context_prefix);

  return value ? value : "";
}


gchar *
load_text (GbWidgetSetArgData * data,
	   const gchar * property_name)
{
  return load_string (data, property_name);
}


gint
load_int (GbWidgetSetArgData * data,
	  const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  return value ? atoi (value) : 0;
}


gfloat
load_float (GbWidgetSetArgData * data,
	    const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  return value ? atof (value) : 0;
}


gboolean
load_bool (GbWidgetSetArgData * data,
	   const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  gboolean result = load_parse_bool (data, value);
  if (data->status == GLADE_STATUS_INVALID_VALUE)
    {
      g_warning ("Invalid boolean value: %s", value);
      data->status = GLADE_STATUS_OK;
      result = FALSE;
    }
  return result;
}


gboolean
load_parse_bool (GbWidgetSetArgData * data,
		 const gchar * value)
{
  if (value != NULL)
    {
      if (!g_strcasecmp (value, "true") || !g_strcasecmp (value, "yes") || !strcmp (value, "1"))
	return TRUE;
      else if (!g_strcasecmp (value, "false") || !g_strcasecmp (value, "no") || !strcmp (value, "0"))
	return FALSE;
      else
	{
	  if (data)
	    data->status = GLADE_STATUS_INVALID_VALUE;
	  MSG1 ("===Invalid boolean property: %s", value);
	}
    }
  return FALSE;
}


gchar *
load_choice (GbWidgetSetArgData * data,
	     const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  return value ? value : "";
}


gchar *
load_combo (GbWidgetSetArgData * data,
	    const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  return value ? value : "";
}


GdkColor *
load_color (GbWidgetSetArgData * data,
	    const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  GdkColor *result = load_parse_color (data, value);
  if (data->status == GLADE_STATUS_INVALID_VALUE)
    {
      g_warning ("Invalid color: %s", value);
      data->status = GLADE_STATUS_OK;
    }
  return result;
}


/* Colors are now saved as '#rrrrggggbbbb', where rgb are 0-65535.
   We use gdk_color_parse() when loading, so color names can be used as well.*/
GdkColor *
load_parse_color (GbWidgetSetArgData * data,
		  const gchar * value)
{
  static GdkColor color;

  if (value == NULL)
    return NULL;

  if (!gdk_color_parse (value, &color))
    {
      data->status = GLADE_STATUS_INVALID_VALUE;

      /* If an error occurs return white. */
      color.red = 0xFFFF;
      color.green = 0xFFFF;
      color.blue = 0xFFFF;
    }

  return &color;
}


GdkPixmap *
load_bgpixmap (GbWidgetSetArgData * data,
	       const gchar * property_name,
	       gchar ** filename)
{
  /*GdkPixmap *gdkpixmap;*/
  gchar *value = load_get_value (data, property_name);
  *filename = value;
  if (value)
    {
      /* FIXME: What do I do here? We have no widget. Could use the parent,
         or load the pixmap in a realize callback. */
      /*
         gdkpixmap = gdk_pixmap_create_from_xpm (data->holding_widget->window, NULL,
         &data->holding_widget->style->bg[GTK_STATE_NORMAL],
         value);
         if (!gdkpixmap)
         load_add_error_message_with_tag (data, GLADE_LINE_PROPERTY,
					  "Couldn't load pixmap",
					  property_name, value);

         return gdkpixmap;
       */
    }

  return NULL;
}


gpointer
load_dialog (GbWidgetSetArgData * data,
	     const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  return value ? value : "";
}


gchar *
load_filename (GbWidgetSetArgData * data,
	       const gchar * property_name)
{
  /* FIXME: Convert to absolute path, using project dir as relative path??. */
  gchar *value = load_get_value (data, property_name);
  return value ? value : "";
}


/* If we are loading the XML file, we convert any relative filenames to
   absolute ones, based on the project directory and/or pixmaps directory
   options. The returned filename should be freed when no longer needed. */
gchar *
load_pixmap_filename (GbWidgetSetArgData * data,
		      const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  gchar *pixmaps_dir;

  if (value == NULL)
    return NULL;

  if (data->xml_buffer == NULL)
    {
      pixmaps_dir = glade_project_get_pixmaps_directory (data->project);
      g_return_val_if_fail (pixmaps_dir != NULL, NULL);
      g_return_val_if_fail (pixmaps_dir[0] != '\0', NULL);
      return glade_util_make_absolute_path (pixmaps_dir, value);
    }
  else
    return g_strdup (value);
}


GdkFont *
load_font (GbWidgetSetArgData * data,
	   const gchar * property_name,
	   gchar ** xlfd_fontname)
{
  GdkFont *font;
  gchar *value = load_get_value (data, property_name);
  *xlfd_fontname = value;
  if (value)
    {
      font = gdk_font_load (value);
      if (font == NULL)
	g_warning ("Couldn't load font: %s", value);
      return font;
    }
  return NULL;
}


time_t
load_date (GbWidgetSetArgData * data,
	   const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);
  time_t result = value ? load_parse_date (data, value) : 0;

  if (data->status == GLADE_STATUS_INVALID_VALUE)
    {
      g_warning ("Invalid date value: %s", value);
      data->status = GLADE_STATUS_OK;
      result = 0;
    }

  return result;
}


/* This parses a date in the RFC1123 format (an update of RFC822),
   e.g. 'Sun, 06 Nov 1994 08:49:37 GMT'. */
time_t
load_parse_date (GbWidgetSetArgData * data,
		 const gchar * value)
{
  struct tm t;
  gchar day[4], month[4];
  gint matched, i;
  time_t time;

  if (!value || !value[0])
    return 0;

  /* Terminate the strings to be careful. */
  day[0] = '\0';
  month[0] = '\0';

  MSG1 ("Trying to match date: %s", value);
  matched = sscanf (value, "%3s, %2d %3s %4d %2d:%2d:%2d GMT",
		    &day[0], &t.tm_mday, &month[0], &t.tm_year,
		    &t.tm_hour, &t.tm_min, &t.tm_sec);
  if (matched != 7)
    {
      MSG1 ("ERROR parsing date, matched: %i", matched);
      data->status = GLADE_STATUS_INVALID_VALUE;
      return 0;
    }

  /* The tm_year field starts from 1900 so we have to subtract that. */
  t.tm_year -= 1900;

  /* Find the month. */
  t.tm_mon = -1;
  for (i = 0; i < 12; i++)
    {
      if (!strcmp (GladeMonthNames[i], month))
	{
	  t.tm_mon = i;
	  break;
	}
    }

  /* Find the day. */
  t.tm_wday = -1;
  for (i = 0; i < 7; i++)
    {
      if (!strcmp (GladeDayNames[i], day))
	{
	  t.tm_wday = i;
	  break;
	}
    }

  if (t.tm_mon == -1 || t.tm_wday == -1)
    {
      MSG ("ERROR parsing date");
      data->status = GLADE_STATUS_INVALID_VALUE;
      return 0;
    }

  t.tm_isdst = -1;
  /* Note that we don't need to set t.tm_yday (or t.tm_wday really).
     They are recomputed by mktime.
     Note also that mktime works since we have already set the timezone to GMT
     in load_project_file(). */
  time = mktime (&t);
  if (time == -1)
    {
      MSG ("ERROR parsing date");
      data->status = GLADE_STATUS_INVALID_VALUE;
      return 0;
    }
  return time;
}


gchar*
load_icon (GbWidgetSetArgData * data,
	   const gchar * property_name)
{
  gchar *value = load_get_value (data, property_name);

  if (glade_util_check_is_stock_id (value))
    return value;
  else
    return load_pixmap_filename (data, property_name);
}


static void
load_ensure_widgets_named (GtkWidget    *widget,
			   GladeProject *project)
{
  glade_project_ensure_widgets_named (project, widget);
}


static void
load_atk_properties (GtkWidget    *widget,
		     GHashTable   *all_widgets)
{
  GladeWidgetInfo * widget_info;

  widget_info = gtk_object_get_data (GTK_OBJECT (widget), GladeWidgetInfoKey);
  if (widget_info)
    {
      glade_atk_load_properties (widget, widget_info, all_widgets);

      /* Clear the pointer, just to be safe. */
      gtk_object_set_data (GTK_OBJECT (widget), GladeWidgetInfoKey, NULL);
    }

  /* Load the child widgets' properties recursively. */
  gb_widget_children_foreach (widget, (GtkCallback) load_atk_properties,
			      all_widgets);
}
