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

#include "gladeconfig.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <time.h>
#include <errno.h>
#include <locale.h>

#include "glade_project.h"
#include "gbwidget.h"
#include "save.h"
#include "utils.h"

/* This is set if we are saving a session. A bit of a hack. */
char *GladeSessionFile = NULL;


/* The stuff to output at the start of XML files. */
static gchar *GLADE_XML_BEGIN =
	"<?xml version=\"1.0\" standalone=\"no\"?> <!--*- mode: xml -*-->\n"
	"<!DOCTYPE glade-interface SYSTEM \"http://glade.gnome.org/glade-2.0.dtd\">\n\n";

static gchar *GLADE_PROJECT_XML_BEGIN =
	"<?xml version=\"1.0\" standalone=\"no\"?> <!--*- mode: xml -*-->\n"
	"<!DOCTYPE glade-project SYSTEM \"http://glade.gnome.org/glade-project-2.0.dtd\">\n\n";


/* An internal struct to pass data to the save_component() and
   save_named_style_callback() callbacks. */
typedef struct _GladeSaveCallbackData GladeSaveCallbackData;
struct _GladeSaveCallbackData
{
  GbWidgetGetArgData *data;
  FILE *fp;
};

/* An internal struct to pass data to the save_requires_tags_cb(). */
typedef struct _GladeSaveRequiresData GladeSaveRequiresData;
struct _GladeSaveRequiresData
{
  /* We only support these 4 libs so we just have flags for each of them. */
  gboolean require_gnome;
  gboolean require_gnome_canvas;
  gboolean require_gnomedb;
  gboolean require_bonobo;
};


static GladeError* save_project_file_internal (GladeProject *project);
static GladeError* save_xml_file_internal (GladeProject *project);

static void save_requires_tags (FILE *fp, GladeProject *project);
static void save_component (GtkWidget * item,
			    GladeSaveCallbackData * save_data);

#ifdef GLADE_STYLE_SUPPORT
static void save_named_style_callback (const gchar * name,
				       GbStyle * gbstyle,
				       GladeSaveCallbackData * save_data);
static void save_named_style (const gchar * name,
			      GbStyle * gbstyle,
			      GbWidgetGetArgData * data,
			      FILE *fp);
#endif

static void save_buffer_flush (GbWidgetGetArgData * data,
			       FILE *fp);

static void save_translatable_strings (GbWidgetGetArgData * data);


/* We need this to make sure that numbers are output in a portable syntax,
   instead of using the current locale. This code is from glibc info docs. */
GladeError*
save_project_file (GladeProject *project)
{
  gchar *old_locale, *saved_locale;
  GladeError *error;
     
  old_locale = setlocale (LC_NUMERIC, NULL);
  saved_locale = g_strdup (old_locale);
  setlocale (LC_NUMERIC, "C");

  error = save_project_file_internal (project);

  if (!error)
    error = save_xml_file_internal (project);

  setlocale (LC_NUMERIC, saved_locale);
  g_free (saved_locale);
  return error;
}


/* Backup the file, to <filename>.bak, if it exists. */
static GladeError*
backup_file (const gchar *filename)
{
  gchar *backup_filename;
  GladeError *error = NULL;
  int status;

  if (!glade_util_file_exists (filename))
    return NULL;

  backup_filename = g_strdup_printf ("%s.bak", filename);
#if defined (__EMX__) || defined (_WIN32)
  /* for OS/2 rename dosn't work if the dest. file exist ! remove it! */
  status = remove (backup_filename);
#endif
  status = rename (filename, backup_filename);

  if (status == -1)
    {
      error = glade_error_new_system (_("Couldn't rename file:\n  %s\nto:\n  %s\n"), filename, backup_filename);
    }

  g_free (backup_filename);
  return error;
}


/* The main function to output the XML. It creates and initializes the
   GbWidgetGetArgData, opens the file, outputs the project options, the
   named styles, and then each component (window/dialog) in the interface. */
static GladeError*
save_project_file_internal (GladeProject *project)
{
  gchar *xml_filename, *filename;
  GladeError *error;
  FILE *fp;

  if (GladeSessionFile)
    xml_filename = GladeSessionFile;
  else
    xml_filename = glade_project_get_xml_filename (project);

  filename = g_strdup_printf ("%sp", xml_filename);

  if (!GladeSessionFile)
    {
      error = backup_file (filename);
      if (error)
	goto out;
    }

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
				      filename);
      goto out;
    }

  fprintf (fp, "%s", GLADE_PROJECT_XML_BEGIN);

  error = glade_project_save_options (project, fp);

  fclose (fp);

 out:

  g_free (filename);

  return error;
}


/* The main function to output the XML. It creates and initializes the
   GbWidgetGetArgData, opens the file, outputs the project options, the
   named styles, and then each component (window/dialog) in the interface. */
static GladeError*
save_xml_file_internal (GladeProject *project)
{
  GbWidgetGetArgData data = { 0 };
  GladeSaveCallbackData save_data;
  gchar *filename;
  FILE *fp;

  MSG ("Saving project");

  data.project = project;
  data.action = GB_SAVING;
  data.copying_to_clipboard = FALSE;
  data.error = NULL;

  if (GladeSessionFile)
    filename = GladeSessionFile;
  else
    filename = glade_project_get_xml_filename (project);

  if (!GladeSessionFile)
    {
      data.error = backup_file (filename);
      if (data.error)
	return data.error;
    }

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    return glade_error_new_system (_("Couldn't create file:\n  %s\n"),
				   filename);

  /* Initialize the output buffer. */
  data.buffer = g_string_sized_new (1024);
  data.indent = 0;

  /* See if we need the translatable strings file output. */
  data.save_translatable_strings = FALSE;
  data.translatable_strings = NULL;
  if (!GladeSessionFile)
    {
      data.save_translatable_strings = glade_project_get_output_translatable_strings (data.project);
      if (data.save_translatable_strings)
	data.translatable_strings = g_string_sized_new (1024);
    }

  /* Output the XML version info and our root element '<glade-interface>'. */
  fprintf (fp, "%s", GLADE_XML_BEGIN);
  fprintf (fp, "<glade-interface>\n");

  /* Output the requires tags. */
  save_requires_tags (fp, project);

  /* Set up the struct to pass data to the callbacks. */
  save_data.data = &data;
  save_data.fp = fp;

#ifdef GLADE_STYLE_SUPPORT
  if (!data.error)
    {
      /* Save default gbstyle first. */
      MSG ("Saving styles");
      save_named_style (gb_widget_default_gb_style->name,
			gb_widget_default_gb_style, &data, fp);
      /* Now save all the other named styles. */
      g_hash_table_foreach (gb_style_hash, (GHFunc) save_named_style_callback,
			    &save_data);
    }
#endif

  if (!data.error)
    {
      MSG ("Saving components");
      glade_project_foreach_component (data.project,
				       (GtkCallback) save_component,
				       &save_data);
    }

  /* Finish the root element. */
  if (!data.error)
    {
      fprintf (fp, "\n</glade-interface>\n");
    }

  /* Save the translatable strings file, if needed. */
  if (!data.error)
    {
      if (data.save_translatable_strings)
	save_translatable_strings (&data);
    }

  /* Free any memory used while saving. */
  g_string_free (data.buffer, TRUE);
  if (data.translatable_strings)
    g_string_free (data.translatable_strings, TRUE);

  fclose (fp);

  return data.error;
}


static void
save_requires_tags_cb (GtkWidget *widget,
		       GladeSaveRequiresData * requires_data)
{
  const char *type_name;

  type_name = g_type_name (G_OBJECT_TYPE (widget));

  /* We assume that any classes starting with GnomeDB require the GnomeDB
     support etc. Note that GNOME apps will probably require Bonobo for the
     BonoboDock widgets used in GnomeApp. */
  if (!strncmp (type_name, "GnomeDb", 7))
    requires_data->require_gnomedb = TRUE;
  else if (!strncmp (type_name, "GnomeCanvas", 11))
    requires_data->require_gnome_canvas = TRUE;
  else if (!strncmp (type_name, "Gnome", 5))
    requires_data->require_gnome = TRUE;
  else if (!strncmp (type_name, "Bonobo", 6))
    requires_data->require_bonobo = TRUE;

  /* Recursively check all children. */
  gb_widget_children_foreach (widget, (GtkCallback) save_requires_tags_cb,
			      requires_data);
}

static void
save_requires_tags (FILE *fp, GladeProject *project)
{
  GladeSaveRequiresData requires_data;

  /* Assume we don't need any of them until we find one of their widgets. */
  requires_data.require_gnome = FALSE;
  requires_data.require_gnome_canvas = FALSE;
  requires_data.require_gnomedb = FALSE;
  requires_data.require_bonobo = FALSE;

  glade_project_foreach_component (project,
				   (GtkCallback) save_requires_tags_cb,
				   &requires_data);

  /* Require "gnome", as provided in libgnomeui/glade/glade-gnome.c.
     We now require GNOME for all GNOME projects, since even if they don't
     contain GNOME widgets they may use stock GNOME items. */
  if (requires_data.require_gnome
      || glade_project_get_gnome_support (project))
    fprintf (fp, "<requires lib=\"gnome\"/>\n");
  /* Require "canvas", as provided in libgnomecanvas/glade/glade-canvas.c. */
  if (requires_data.require_gnome_canvas)
    fprintf (fp, "<requires lib=\"canvas\"/>\n");
  /* Require "gnomedb", as provided in libgnomedb/glade/glade-gnomedb.c. */
  if (requires_data.require_gnomedb)
    fprintf (fp, "<requires lib=\"gnomedb\"/>\n");
  /* Require "bonobo", as provided in libbonoboui/glade/glade-bonobo.c. */
  if (requires_data.require_bonobo)
    fprintf (fp, "<requires lib=\"bonobo\"/>\n");
}

/* This is called when iterating over the components in a project, to output
   each component. It simply calls gb_widget_save() to recursively save the
   XML for the component into the buffer, and it then flushes the buffer to
   the output file. */
static void
save_component (GtkWidget * component,
		GladeSaveCallbackData * save_data)
{
  /* If an error has occurred, we return. */
  if (save_data->data->error)
    return;

  gb_widget_save (component, save_data->data);
  save_buffer_flush (save_data->data, save_data->fp);
}


/* This is called when iterating over the GHashTable of named styles.
   If the style isn't the default style it is output here. The default
   style is output first, since for all other styles we only output the
   differences from the default. */
#ifdef GLADE_STYLE_SUPPORT
static void
save_named_style_callback (const gchar * name,
			   GbStyle * gbstyle,
			   GladeSaveCallbackData * save_data)
{
  /* If an error has occurred, or this is the default GbStyle, we return. */
  if (save_data->data->error || gbstyle == gb_widget_default_gb_style)
    {
      return;
    }

  save_named_style (name, gbstyle, save_data->data, save_data->fp);
}


/* Outputs a named style. */
static void
save_named_style (const gchar * name,
		  GbStyle * gbstyle,
		  GbWidgetGetArgData * data,
		  FILE *fp)
{
  gboolean save_all = FALSE;

  MSG1 ("Saving style: %s", name);
  /* If this is the default style, only save it if it is different to the
     GTK default style, and if it is make sure we save everything. */
  if (gbstyle == gb_widget_default_gb_style)
    {
      if (gbstyle->style == gtk_widget_get_default_style ())
	return;
      else
	save_all = TRUE;
    }
  gb_widget_save_style (gbstyle, data, save_all);
  save_buffer_flush (data, fp);
}
#endif


/* Adds a start tag, e.g. "<widget>", to the output buffer. */
void
save_start_tag (GbWidgetGetArgData * data, const gchar * tag_name)
{
  save_buffer_add_indent (data->buffer, data->indent);
  g_string_append_c (data->buffer, '<');
  g_string_append (data->buffer, tag_name);
  g_string_append (data->buffer, ">\n");
  data->indent++;
}


/* Adds an end tag, e.g. "</widget>", to the output buffer. */
void
save_end_tag (GbWidgetGetArgData * data, const gchar * tag_name)
{
  data->indent--;
  save_buffer_add_indent (data->buffer, data->indent);
  g_string_append (data->buffer, "</");
  g_string_append (data->buffer, tag_name);
  g_string_append (data->buffer, ">\n");
}


/* Adds a <widget> start tag to the output buffer, with the given class_name
   and id, if they are not NULL. */
void
save_widget_start_tag (GbWidgetGetArgData * data, const gchar * class_name,
		       const gchar *id)
{
  save_buffer_add_indent (data->buffer, data->indent);
  g_string_append (data->buffer, "<widget");

  if (class_name)
    {
      g_string_append (data->buffer, " class=\"");
      save_buffer_add_string (data->buffer, class_name);
      g_string_append_c (data->buffer, '"');
    }

  if (id)
    {
      g_string_append (data->buffer, " id=\"");
      save_buffer_add_string (data->buffer, id);
      g_string_append_c (data->buffer, '"');
    }

  g_string_append (data->buffer, ">\n");
  data->indent++;
}


/* Adds a <child> start tag to the output buffer, with the given child_name
   if it is not NULL. */
void
save_child_start_tag (GbWidgetGetArgData * data, const gchar * child_name)
{
  save_buffer_add_indent (data->buffer, data->indent);
  g_string_append (data->buffer, "<child");

  if (child_name)
    {
      g_string_append (data->buffer, " internal-child=\"");
      save_buffer_add_string (data->buffer, child_name);
      g_string_append_c (data->buffer, '"');
    }

  g_string_append (data->buffer, ">\n");
  data->indent++;
}


/* Adds a <placeholder/> tag to the output buffer. */
void
save_placeholder (GbWidgetGetArgData * data)
{
  save_buffer_add_indent (data->buffer, data->indent);
  g_string_append (data->buffer, "<placeholder/>\n");
}


/* Starts a new line in the output buffer (without indenting). */
void
save_newline (GbWidgetGetArgData * data)
{
  g_string_append_c (data->buffer, '\n');
}


/* These functions are called to save different types of widget properties.
   They all convert the property to a string representation and call
   save_string() to output it. The tag_name is usually the long name of the
   property, e.g. "GtkLabel::justify", so we cut out the first part and output
   <justify>...</justify>. */

static void
save_string_internal (GbWidgetGetArgData * data, const gchar * tag_name,
		      const gchar * tag_value,
		      gboolean translatable,
		      const gchar *translator_comments,
		      gboolean has_context_prefix)
{
  gchar *tag_name_start;

  if (tag_value == NULL)
    return;

  tag_name_start = glade_util_find_start_of_tag_name (tag_name);
  save_buffer_add_indent (data->buffer, data->indent);

  g_string_append (data->buffer, "<property");

  if (data->agent)
    {
      g_string_append (data->buffer, " agent=\"");
      save_buffer_add_string (data->buffer, data->agent);
      g_string_append_c (data->buffer, '"');
    }

  g_string_append (data->buffer, " name=\"");
  save_buffer_add_string (data->buffer, tag_name_start);
  g_string_append_c (data->buffer, '"');

  if (translatable)
    {
      g_string_append (data->buffer, " translatable=\"yes\"");

      if (has_context_prefix)
	g_string_append (data->buffer, " context=\"yes\"");

      if (translator_comments && *translator_comments)
	{
	  g_string_append (data->buffer, " comments=\"");
	  save_buffer_add_string (data->buffer, translator_comments);
	  g_string_append_c (data->buffer, '"');
	}
    }

  g_string_append_c (data->buffer, '>');
  save_buffer_add_string (data->buffer, tag_value);
  g_string_append (data->buffer, "</property>\n");
}

void
save_string (GbWidgetGetArgData * data, const gchar * tag_name,
	     const gchar * tag_value)
{
  save_string_internal (data, tag_name, tag_value, FALSE, NULL, FALSE);
}


void
save_translatable_string_internal (GbWidgetGetArgData * data,
				   const gchar * tag_name,
				   const gchar * tag_value)
{
  gboolean translatable, context;
  gchar *comments;

  glade_util_get_translation_properties (data->widget, tag_name, &translatable,
					 &comments, &context);
  save_string_internal (data, tag_name, tag_value, translatable,
			comments, context);
}


void
save_translatable_string (GbWidgetGetArgData * data, const gchar * tag_name,
			  const gchar * tag_value)
{
  save_translatable_string_internal (data, tag_name, tag_value);
  if (data->save_translatable_strings)
    save_add_translatable_string (data, tag_value);
}


void
save_text (GbWidgetGetArgData * data, const gchar * tag_name,
	   const gchar * tag_value)
{
  save_string (data, tag_name, tag_value);
}


void
save_translatable_text (GbWidgetGetArgData * data, const gchar * tag_name,
			const gchar * tag_value)
{
  save_translatable_string (data, tag_name, tag_value);
}


/* This is like save_translatable_text() except it splits the text into lines
   when adding to the translatable strings file. This is used for option menu
   items and combo items. */
void
save_translatable_text_in_lines (GbWidgetGetArgData * data,
				 const gchar * tag_name,
				 const gchar * tag_value)
{
  /* FIXME: This probably won't work if other apps just try to translate the
     entire string. */
  save_translatable_string_internal (data, tag_name, tag_value);

  if (data->save_translatable_strings && tag_value)
    {
      gchar *items, *pos, *items_end;

      items = pos = g_strdup (tag_value);
      items_end = &items[strlen (items)];

      while (pos < items_end)
	{
	  gchar *item_end = strchr (pos, '\n');
	  if (item_end == NULL)
	    item_end = items_end;
	  *item_end = '\0';

	  save_add_translatable_string (data, pos);

	  pos = item_end + 1;
	}

      g_free (items);
    }
}


void
save_int (GbWidgetGetArgData * data, const gchar * tag_name,
	  const gint tag_value)
{
  gchar buf[32];
  sprintf (buf, "%i", tag_value);
  save_string (data, tag_name, buf);
}


void
save_float (GbWidgetGetArgData * data, const gchar * tag_name,
	    gfloat tag_value)
{
  gchar buf[32];
  sprintf (buf, "%.12g", tag_value);
  save_string (data, tag_name, buf);
}


void
save_bool (GbWidgetGetArgData * data, const gchar * tag_name, gint tag_value)
{
  save_string (data, tag_name, tag_value ? "True" : "False");
}


void
save_choice (GbWidgetGetArgData * data, const gchar * tag_name,
	     const gchar * tag_value)
{
  save_string (data, tag_name, tag_value);
}


void
save_combo (GbWidgetGetArgData * data, const gchar * tag_name,
	    const gchar * tag_value)
{
  save_string (data, tag_name, tag_value);
}


/* Colors are now saved as '#rrrrggggbbbb', where rgb are 0-65535.
   We use gdk_color_parse() when loading, so color names can be used as well.*/
void
save_color (GbWidgetGetArgData * data, const gchar * tag_name,
	    GdkColor * tag_value)
{
  gchar buf[32];

  sprintf (buf, "#%04x%04x%04x", tag_value->red, tag_value->green,
	   tag_value->blue);
  save_string (data, tag_name, buf);
}


void
save_bgpixmap (GbWidgetGetArgData * data, const gchar * tag_name,
	       const gchar * tag_value)
{
  save_string (data, tag_name, tag_value);
}


void
save_dialog (GbWidgetGetArgData * data, const gchar * tag_name,
	     const gchar * tag_value)
{
  save_string (data, tag_name, tag_value);
}


/* FIXME: I think this is broken. It should save it relative to the XML file.*/
void
save_filename (GbWidgetGetArgData * data, const gchar * tag_name,
	       const gchar * tag_value)
{
  save_string (data, tag_name, tag_value);
}


void
save_pixmap_filename (GbWidgetGetArgData * data, const gchar * tag_name,
		      const gchar * tag_value)
{
  if (data->copying_to_clipboard)
    {
      /* When saving to the clipboard, we just save the full path.
	 This means it will still work if we paste into other projects. */
      save_string (data, tag_name, tag_value);
    }
  else
    {
      /* When saving the XML file, we save only the basename of the pixmap
	 files, since they should all be in the pixmaps directory. */
      gchar *pixmaps_dir, *filename;

      pixmaps_dir = glade_project_get_pixmaps_directory (data->project);
      g_return_if_fail (pixmaps_dir != NULL);
      g_return_if_fail (pixmaps_dir[0] != '\0');

      if (tag_value == NULL || tag_value[0] == '\0')
	{
	  filename = NULL;
	}
      else
	{
	  filename = (gchar*) g_basename (tag_value);
	}

      save_string (data, tag_name, filename);
    }
}


void
save_font (GbWidgetGetArgData * data, const gchar * tag_name,
	   const gchar * tag_value)
{
  save_string (data, tag_name, tag_value);
}


/* This saves a date in the RFC1123 format (an update of RFC822),
   e.g. 'Sun, 06 Nov 1994 08:49:37 GMT'. */
static void
format_date (char *buffer,
	     time_t tag_value)
{
  time_t time;
  struct tm *t;

  time = tag_value;
  t = gmtime (&time);
  sprintf (buffer, "%s, %02d %s %04d %02d:%02d:%02d GMT",
	   GladeDayNames[t->tm_wday], t->tm_mday, GladeMonthNames[t->tm_mon],
	   t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
}


/* This saves a date in the RFC1123 format (an update of RFC822),
   e.g. 'Sun, 06 Nov 1994 08:49:37 GMT'. */
void
save_date (GbWidgetGetArgData * data, const gchar * tag_name,
	   time_t tag_value)
{
  gchar buffer[32];

  format_date (buffer, tag_value);
  save_string (data, tag_name, buffer);
}


void
save_icon (GbWidgetGetArgData * data, const gchar * tag_name,
	   const gchar * tag_value)
{
  /* If it is a stock icon we can save it as an oridnary string.
     If it isn't, we need to save it as a pixmap filename, i.e. get rid of
     the path and just save the basename. */
  if (glade_util_check_is_stock_id (tag_value))
    save_string (data, tag_name, tag_value);
  else
    save_pixmap_filename (data, tag_name, tag_value);
}


/* Adds a string to the output buffer, converting special characters to
   entities, e.g. "<" is output as "&lt;". */
void
save_buffer_add_string (GString * buffer, const gchar * string)
{
  gchar ch;

  while ((ch = *string++))
    {
      if (ch == '<')
	g_string_append (buffer, "&lt;");
      else if (ch == '>')
	g_string_append (buffer, "&gt;");
      else if (ch == '&')
	g_string_append (buffer, "&amp;");
      else if (ch == '"')
	g_string_append (buffer, "&quot;");
      else
	g_string_append_c (buffer, ch);
    }
}


/* Outputs the contents of the buffer to the file and resets the buffer. */
static void
save_buffer_flush (GbWidgetGetArgData * data,
		   FILE *fp)
{
  gint bytes_written;

  bytes_written = fwrite (data->buffer->str, sizeof (gchar), data->buffer->len,
			  fp);
  if (bytes_written != data->buffer->len)
    {
      MSG2 ("Bytes: %i Written: %i", data->buffer->len, bytes_written);
      data->error = glade_error_new_system (_("Error writing XML file\n"));
    }

  /* Reset the output buffer. */
  g_string_truncate (data->buffer, 0);
  data->indent = 0;
}


/* Outputs tabs & spaces to indent the line according to the current
   indentation level. Tabs are used to cut down on the file size a bit. */
void
save_buffer_add_indent (GString *buffer, gint indent)
{
  gint i, ntabs, nspaces;

  ntabs = (indent * 2) / 8;
  nspaces = (indent * 2) % 8;

  for (i = 0; i < ntabs; i++)
    g_string_append_c (buffer, '\t');

  for (i = 0; i < nspaces; i++)
    g_string_append_c (buffer, ' ');
}


/*
 * Translatable string functions.
 */

/* This adds a translatable string to the buffer, wrapping it in the N_()
   macro so xgettext can find it. */
void
save_add_translatable_string (GbWidgetGetArgData * data,
			      const gchar * string)
{
  GString *buffer;
  gchar escape_buffer[16];
  const gchar *p;

  /* If it is an empty string don't bother outputting it. */
  if (!string || string[0] == '\0')
    return;

  buffer = data->translatable_strings;
  g_string_append (buffer, "gchar *s = N_(\"");

  /* Step through each character of the given string, adding it to our GString
     buffer, converting it so that it is valid in a literal C string. */
  for (p = string; *p; p++)
    {
      switch (*p)
	{
	case '\n':
	  g_string_append (buffer, "\\n\"\n              \"");
	  break;
	case '\r':
	  g_string_append (buffer, "\\r");
	  break;
	case '\t':
	  g_string_append (buffer, "\\t");
	  break;
	case '\\':
	  g_string_append (buffer, "\\\\");
	  break;
	case '"':
	  g_string_append (buffer, "\\\"");
	  break;
	default:
	  if (isprint ((unsigned char) *p))
	    {
	      g_string_append_c (buffer, *p);
	    }
	  else
	    {
	      sprintf (escape_buffer, "\\%02o", (guchar) *p);
	      g_string_append (buffer, escape_buffer);
	    }
	  break;
	}
    }

  g_string_append (buffer, "\");\n");
}


/* This outputs the file containing all the translatable strings. */
static void
save_translatable_strings (GbWidgetGetArgData * data)
{
  gchar *filename;
  FILE *fp;

  filename = glade_project_get_translatable_strings_file (data->project);

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      return;
    }

  fprintf (fp,
	   _("/*\n"
	     " * Translatable strings file generated by Glade.\n"
	     " * Add this file to your project's POTFILES.in.\n"
	     " * DO NOT compile it as part of your application.\n"
	     " */\n"
	     "\n"));

  fprintf (fp, "%s", data->translatable_strings->str);

  fclose (fp);
}


void
save_signal		(GbWidgetGetArgData	*data,
			 gchar			*signal_name,
			 gchar			*handler,
			 gboolean		 after,
			 gchar			*object,
			 time_t			 last_modification_time)
{
  /* Don't save signals without names or handlers. */
  if (!signal_name || !signal_name[0] || ! handler || !handler[0])
    return;

  save_buffer_add_indent (data->buffer, data->indent);
  g_string_append (data->buffer, "<signal name=\"");
  save_buffer_add_string (data->buffer, signal_name);
  g_string_append (data->buffer, "\" handler=\"");
  save_buffer_add_string (data->buffer, handler);
  g_string_append (data->buffer, "\"");

  if (after)
    g_string_append (data->buffer, " after=\"yes\"");

  if (object)
    {
      g_string_append (data->buffer, " object=\"");
      save_buffer_add_string (data->buffer, object);
      g_string_append (data->buffer, "\"");
    }

  if (last_modification_time != 0)
    {
      gchar buffer[32];

      format_date (buffer, last_modification_time);

      g_string_append (data->buffer, " last_modification_time=\"");
      save_buffer_add_string (data->buffer, buffer);
      g_string_append (data->buffer, "\"");
    }

  g_string_append (data->buffer, "/>\n");
}


void
save_accelerator	(GbWidgetGetArgData	*data,
			 guint8			 modifiers,
			 gchar			*key,
			 gchar			*signal)
{
  gchar *modifiers_string;

  /* Don't save accelerators without signals or keys. */
  if (!key || !key[0] || !signal || !signal[0])
    return;

  save_buffer_add_indent (data->buffer, data->indent);
  g_string_append (data->buffer, "<accelerator key=\"");
  save_buffer_add_string (data->buffer, key);
  g_string_append (data->buffer, "\"");

  modifiers_string = glade_util_create_modifiers_string (modifiers);
  g_string_append (data->buffer, " modifiers=\"");
  save_buffer_add_string (data->buffer, modifiers_string);
  g_string_append (data->buffer, "\"");

  g_string_append (data->buffer, " signal=\"");
  save_buffer_add_string (data->buffer, signal);
  g_string_append (data->buffer, "\"");

  g_string_append (data->buffer, "/>\n");
}


