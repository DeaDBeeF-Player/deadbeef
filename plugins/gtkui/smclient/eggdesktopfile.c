/* eggdesktopfile.c - Freedesktop.Org Desktop Files
 * Copyright (C) 2007 Novell, Inc.
 *
 * Based on gnome-desktop-item.c
 * Copyright (C) 1999, 2000 Red Hat Inc.
 * Copyright (C) 2001 George Lebl
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING.LIB. If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place -
 * Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "eggdesktopfile.h"

#include <string.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

struct EggDesktopFile {
  GKeyFile           *key_file;
  char               *source;

  char               *name, *icon;
  EggDesktopFileType  type;
  char                document_code;
};

/**
 * egg_desktop_file_new:
 * @desktop_file_path: path to a Freedesktop-style Desktop file
 * @error: error pointer
 *
 * Creates a new #EggDesktopFile for @desktop_file.
 *
 * Return value: the new #EggDesktopFile, or %NULL on error.
 **/
EggDesktopFile *
egg_desktop_file_new (const char *desktop_file_path, GError **error)
{
  GKeyFile *key_file;

  key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (key_file, desktop_file_path, 0, error))
    {
      g_key_file_free (key_file);
      return NULL;
    }

  return egg_desktop_file_new_from_key_file (key_file, desktop_file_path,
					     error);
}

/**
 * egg_desktop_file_new_from_data_dirs:
 * @desktop_file_path: relative path to a Freedesktop-style Desktop file
 * @error: error pointer
 *
 * Looks for @desktop_file_path in the paths returned from
 * g_get_user_data_dir() and g_get_system_data_dirs(), and creates
 * a new #EggDesktopFile from it.
 *
 * Return value: the new #EggDesktopFile, or %NULL on error.
 **/
EggDesktopFile *
egg_desktop_file_new_from_data_dirs (const char  *desktop_file_path,
				     GError     **error)
{
  EggDesktopFile *desktop_file;
  GKeyFile *key_file;
  char *full_path;

  key_file = g_key_file_new ();
  if (!g_key_file_load_from_data_dirs (key_file, desktop_file_path,
				       &full_path, 0, error))
    {
      g_key_file_free (key_file);
      return NULL;
    }

  desktop_file = egg_desktop_file_new_from_key_file (key_file,
						     full_path,
						     error);
  g_free (full_path);
  return desktop_file;
}

/**
 * egg_desktop_file_new_from_dirs:
 * @desktop_file_path: relative path to a Freedesktop-style Desktop file
 * @search_dirs: NULL-terminated array of directories to search
 * @error: error pointer
 *
 * Looks for @desktop_file_path in the paths returned from
 * g_get_user_data_dir() and g_get_system_data_dirs(), and creates
 * a new #EggDesktopFile from it.
 *
 * Return value: the new #EggDesktopFile, or %NULL on error.
 **/
EggDesktopFile *
egg_desktop_file_new_from_dirs (const char  *desktop_file_path,
				const char **search_dirs,
				GError     **error)
{
  EggDesktopFile *desktop_file;
  GKeyFile *key_file;
  char *full_path;

  key_file = g_key_file_new ();
  if (!g_key_file_load_from_dirs (key_file, desktop_file_path, search_dirs,
				  &full_path, 0, error))
    {
      g_key_file_free (key_file);
      return NULL;
    }

  desktop_file = egg_desktop_file_new_from_key_file (key_file,
						     full_path,
						     error);
  g_free (full_path);
  return desktop_file;
}

/**
 * egg_desktop_file_new_from_key_file:
 * @key_file: a #GKeyFile representing a desktop file
 * @source: the path or URI that @key_file was loaded from, or %NULL
 * @error: error pointer
 *
 * Creates a new #EggDesktopFile for @key_file. Assumes ownership of
 * @key_file (on success or failure); you should consider @key_file to
 * be freed after calling this function.
 *
 * Return value: the new #EggDesktopFile, or %NULL on error.
 **/
EggDesktopFile *
egg_desktop_file_new_from_key_file (GKeyFile    *key_file,
				    const char  *source,
				    GError     **error)
{
  EggDesktopFile *desktop_file;
  char *version, *type;

  if (!g_key_file_has_group (key_file, EGG_DESKTOP_FILE_GROUP))
    {
      g_set_error (error, EGG_DESKTOP_FILE_ERROR,
		   EGG_DESKTOP_FILE_ERROR_INVALID,
		   _("File is not a valid .desktop file"));
      g_key_file_free (key_file);
      return NULL;
    }

  version = g_key_file_get_value (key_file, EGG_DESKTOP_FILE_GROUP,
				  EGG_DESKTOP_FILE_KEY_VERSION,
				  NULL);
  if (version)
    {
      double version_num;
      char *end;

      version_num = g_ascii_strtod (version, &end);
      if (*end)
	{
	  g_warning ("Invalid Version string '%s' in %s",
		     version, source ? source : "(unknown)");
	}
      else if (version_num > 1.0)
	{
	  g_set_error (error, EGG_DESKTOP_FILE_ERROR,
		       EGG_DESKTOP_FILE_ERROR_INVALID,
		       /* translators: 'Version' is from a desktop file, and
			* should not be translated. '%s' would probably be a
			* version number. */
		       _("Unrecognized desktop file Version '%s'"), version);
	  g_free (version);
	  g_key_file_free (key_file);
	  return NULL;
	}
      g_free (version);
    }

  desktop_file = g_new0 (EggDesktopFile, 1);
  desktop_file->key_file = key_file;

  if (g_path_is_absolute (source))
    desktop_file->source = g_filename_to_uri (source, NULL, NULL);
  else
    desktop_file->source = g_strdup (source);

  desktop_file->name = g_key_file_get_locale_string (key_file,
						     EGG_DESKTOP_FILE_GROUP,
						     EGG_DESKTOP_FILE_KEY_NAME,
						     NULL,
						     error);
  if (!desktop_file->name)
    {
      egg_desktop_file_free (desktop_file);
      return NULL;
    }

  type = g_key_file_get_string (key_file, EGG_DESKTOP_FILE_GROUP,
				EGG_DESKTOP_FILE_KEY_TYPE, error);
  if (!type)
    {
      egg_desktop_file_free (desktop_file);
      return NULL;
    }

  if (!strcmp (type, "Application"))
    {
      char *exec, *p;

      desktop_file->type = EGG_DESKTOP_FILE_TYPE_APPLICATION;

      exec = g_key_file_get_string (key_file,
				    EGG_DESKTOP_FILE_GROUP,
				    EGG_DESKTOP_FILE_KEY_EXEC,
				    error);
      if (!exec)
	{
	  egg_desktop_file_free (desktop_file);
	  g_free (type);
	  return NULL;
	}

      /* See if it takes paths or URIs or neither */
      for (p = exec; *p; p++)
	{
	  if (*p == '%')
	    {
	      if (p[1] == '\0' || strchr ("FfUu", p[1]))
		{
		  desktop_file->document_code = p[1];
		  break;
		}
	      p++;
	    }
	}

      g_free (exec);
    }
  else if (!strcmp (type, "Link"))
    {
      char *url;

      desktop_file->type = EGG_DESKTOP_FILE_TYPE_LINK;

      url = g_key_file_get_string (key_file,
				   EGG_DESKTOP_FILE_GROUP,
				   EGG_DESKTOP_FILE_KEY_URL,
				   error);
      if (!url)
	{
	  egg_desktop_file_free (desktop_file);
	  g_free (type);
	  return NULL;
	}
      g_free (url);
    }
  else if (!strcmp (type, "Directory"))
    desktop_file->type = EGG_DESKTOP_FILE_TYPE_DIRECTORY;
  else
    desktop_file->type = EGG_DESKTOP_FILE_TYPE_UNRECOGNIZED;

  g_free (type);

  /* Check the Icon key */
  desktop_file->icon = g_key_file_get_string (key_file,
					      EGG_DESKTOP_FILE_GROUP,
					      EGG_DESKTOP_FILE_KEY_ICON,
					      NULL);
  if (desktop_file->icon && !g_path_is_absolute (desktop_file->icon))
    {
      char *ext;

      /* Lots of .desktop files still get this wrong */
      ext = strrchr (desktop_file->icon, '.');
      if (ext && (!strcmp (ext, ".png") ||
		  !strcmp (ext, ".xpm") ||
		  !strcmp (ext, ".svg")))
	{
	  g_warning ("Desktop file '%s' has malformed Icon key '%s'"
		     "(should not include extension)",
		     source ? source : "(unknown)",
		     desktop_file->icon);
	  *ext = '\0';
	}
    }

  return desktop_file;
}

/**
 * egg_desktop_file_free:
 * @desktop_file: an #EggDesktopFile
 *
 * Frees @desktop_file.
 **/
void
egg_desktop_file_free (EggDesktopFile *desktop_file)
{
  g_key_file_free (desktop_file->key_file);
  g_free (desktop_file->source);
  g_free (desktop_file->name);
  g_free (desktop_file->icon);
  g_free (desktop_file);
}

/**
 * egg_desktop_file_get_source:
 * @desktop_file: an #EggDesktopFile
 *
 * Gets the URI that @desktop_file was loaded from.
 *
 * Return value: @desktop_file's source URI
 **/
const char *
egg_desktop_file_get_source (EggDesktopFile *desktop_file)
{
  return desktop_file->source;
}

/**
 * egg_desktop_file_get_desktop_file_type:
 * @desktop_file: an #EggDesktopFile
 *
 * Gets the desktop file type of @desktop_file.
 *
 * Return value: @desktop_file's type
 **/
EggDesktopFileType
egg_desktop_file_get_desktop_file_type (EggDesktopFile *desktop_file)
{
  return desktop_file->type;
}

/**
 * egg_desktop_file_get_name:
 * @desktop_file: an #EggDesktopFile
 *
 * Gets the (localized) value of @desktop_file's "Name" key.
 *
 * Return value: the application/link name
 **/
const char *
egg_desktop_file_get_name (EggDesktopFile *desktop_file)
{
  return desktop_file->name;
}

/**
 * egg_desktop_file_get_icon:
 * @desktop_file: an #EggDesktopFile
 *
 * Gets the value of @desktop_file's "Icon" key.
 *
 * If the icon string is a full path (that is, if g_path_is_absolute()
 * returns %TRUE when called on it), it points to a file containing an
 * unthemed icon. If the icon string is not a full path, it is the
 * name of a themed icon, which can be looked up with %GtkIconTheme,
 * or passed directly to a theme-aware widget like %GtkImage or
 * %GtkCellRendererPixbuf.
 *
 * Return value: the icon path or name
 **/
const char *
egg_desktop_file_get_icon (EggDesktopFile *desktop_file)
{
  return desktop_file->icon;
}

gboolean
egg_desktop_file_has_key (EggDesktopFile  *desktop_file,
			  const char      *key,
			  GError         **error)
{
  return g_key_file_has_key (desktop_file->key_file,
			     EGG_DESKTOP_FILE_GROUP, key,
			     error);
}

char *
egg_desktop_file_get_string (EggDesktopFile  *desktop_file,
			     const char      *key,
			     GError         **error)
{
  return g_key_file_get_string (desktop_file->key_file,
				EGG_DESKTOP_FILE_GROUP, key,
				error);
}

char *
egg_desktop_file_get_locale_string (EggDesktopFile  *desktop_file,
				    const char      *key,
				    const char      *locale,
				    GError         **error)
{
  return g_key_file_get_locale_string (desktop_file->key_file,
				       EGG_DESKTOP_FILE_GROUP, key, locale,
				       error);
}

gboolean
egg_desktop_file_get_boolean (EggDesktopFile  *desktop_file,
			      const char      *key,
			      GError         **error)
{
  return g_key_file_get_boolean (desktop_file->key_file,
				 EGG_DESKTOP_FILE_GROUP, key,
				 error);
}

double
egg_desktop_file_get_numeric (EggDesktopFile  *desktop_file,
			      const char      *key,
			      GError         **error)
{
  return g_key_file_get_double (desktop_file->key_file,
				EGG_DESKTOP_FILE_GROUP, key,
				error);
}

int
egg_desktop_file_get_integer (EggDesktopFile *desktop_file,
			      const char     *key,
    			      GError	    **error)
{
  return g_key_file_get_integer (desktop_file->key_file,
				 EGG_DESKTOP_FILE_GROUP, key,
				 error);
}

char **
egg_desktop_file_get_string_list (EggDesktopFile  *desktop_file,
				  const char      *key,
				  gsize           *length,
				  GError         **error)
{
  return g_key_file_get_string_list (desktop_file->key_file,
				     EGG_DESKTOP_FILE_GROUP, key, length,
				     error);
}

char **
egg_desktop_file_get_locale_string_list (EggDesktopFile  *desktop_file,
					 const char      *key,
					 const char      *locale,
					 gsize           *length,
					 GError         **error)
{
  return g_key_file_get_locale_string_list (desktop_file->key_file,
					    EGG_DESKTOP_FILE_GROUP, key,
					    locale, length,
					    error);
}

/**
 * egg_desktop_file_can_launch:
 * @desktop_file: an #EggDesktopFile
 * @desktop_environment: the name of the running desktop environment,
 * or %NULL
 *
 * Tests if @desktop_file can/should be launched in the current
 * environment. If @desktop_environment is non-%NULL, @desktop_file's
 * "OnlyShowIn" and "NotShowIn" keys are checked to make sure that
 * this desktop_file is appropriate for the named environment.
 *
 * Furthermore, if @desktop_file has type
 * %EGG_DESKTOP_FILE_TYPE_APPLICATION, its "TryExec" key (if any) is
 * also checked, to make sure the binary it points to exists.
 *
 * egg_desktop_file_can_launch() does NOT check the value of the
 * "Hidden" key.
 *
 * Return value: %TRUE if @desktop_file can be launched
 **/
gboolean
egg_desktop_file_can_launch (EggDesktopFile *desktop_file,
			     const char     *desktop_environment)
{
  char *try_exec, *found_program;
  char **only_show_in, **not_show_in;
  gboolean found;
  int i;

  if (desktop_file->type != EGG_DESKTOP_FILE_TYPE_APPLICATION &&
      desktop_file->type != EGG_DESKTOP_FILE_TYPE_LINK)
    return FALSE;

  if (desktop_environment)
    {
      only_show_in = g_key_file_get_string_list (desktop_file->key_file,
						 EGG_DESKTOP_FILE_GROUP,
						 EGG_DESKTOP_FILE_KEY_ONLY_SHOW_IN,
						 NULL, NULL);
      if (only_show_in)
	{
	  for (i = 0, found = FALSE; only_show_in[i] && !found; i++)
	    {
	      if (!strcmp (only_show_in[i], desktop_environment))
		found = TRUE;
	    }

	  g_strfreev (only_show_in);

	  if (!found)
	    return FALSE;
	}

      not_show_in = g_key_file_get_string_list (desktop_file->key_file,
						EGG_DESKTOP_FILE_GROUP,
						EGG_DESKTOP_FILE_KEY_NOT_SHOW_IN,
						NULL, NULL);
      if (not_show_in)
	{
	  for (i = 0, found = FALSE; not_show_in[i] && !found; i++)
	    {
	      if (!strcmp (not_show_in[i], desktop_environment))
		found = TRUE;
	    }

	  g_strfreev (not_show_in);

	  if (found)
	    return FALSE;
	}
    }

  if (desktop_file->type == EGG_DESKTOP_FILE_TYPE_APPLICATION)
    {
      try_exec = g_key_file_get_string (desktop_file->key_file,
					EGG_DESKTOP_FILE_GROUP,
					EGG_DESKTOP_FILE_KEY_TRY_EXEC,
					NULL);
      if (try_exec)
	{
	  found_program = g_find_program_in_path (try_exec);
	  g_free (try_exec);

	  if (!found_program)
	    return FALSE;
	  g_free (found_program);
	}
    }

  return TRUE;
}

/**
 * egg_desktop_file_accepts_documents:
 * @desktop_file: an #EggDesktopFile
 *
 * Tests if @desktop_file represents an application that can accept
 * documents on the command line.
 *
 * Return value: %TRUE or %FALSE
 **/
gboolean
egg_desktop_file_accepts_documents (EggDesktopFile *desktop_file)
{
  return desktop_file->document_code != 0;
}

/**
 * egg_desktop_file_accepts_multiple:
 * @desktop_file: an #EggDesktopFile
 *
 * Tests if @desktop_file can accept multiple documents at once.
 *
 * If this returns %FALSE, you can still pass multiple documents to
 * egg_desktop_file_launch(), but that will result in multiple copies
 * of the application being launched. See egg_desktop_file_launch()
 * for more details.
 *
 * Return value: %TRUE or %FALSE
 **/
gboolean
egg_desktop_file_accepts_multiple (EggDesktopFile *desktop_file)
{
  return (desktop_file->document_code == 'F' ||
	  desktop_file->document_code == 'U');
}

/**
 * egg_desktop_file_accepts_uris:
 * @desktop_file: an #EggDesktopFile
 *
 * Tests if @desktop_file can accept (non-"file:") URIs as documents to
 * open.
 *
 * Return value: %TRUE or %FALSE
 **/
gboolean
egg_desktop_file_accepts_uris (EggDesktopFile *desktop_file)
{
  return (desktop_file->document_code == 'U' ||
	  desktop_file->document_code == 'u');
}

static void
append_quoted_word (GString    *str,
		    const char *s,
		    gboolean    in_single_quotes,
		    gboolean    in_double_quotes)
{
  const char *p;

  if (!in_single_quotes && !in_double_quotes)
    g_string_append_c (str, '\'');
  else if (!in_single_quotes && in_double_quotes)
    g_string_append (str, "\"'");

  if (!strchr (s, '\''))
    g_string_append (str, s);
  else
    {
      for (p = s; *p != '\0'; p++)
	{
	  if (*p == '\'')
	    g_string_append (str, "'\\''");
	  else
	    g_string_append_c (str, *p);
	}
    }

  if (!in_single_quotes && !in_double_quotes)
    g_string_append_c (str, '\'');
  else if (!in_single_quotes && in_double_quotes)
    g_string_append (str, "'\"");
}

static void
do_percent_subst (EggDesktopFile *desktop_file,
		  char            code,
		  GString        *str,
		  GSList        **documents,
		  gboolean        in_single_quotes,
		  gboolean        in_double_quotes)
{
  GSList *d;
  char *doc;

  switch (code)
    {
    case '%':
      g_string_append_c (str, '%');
      break;

    case 'F':
    case 'U':
      for (d = *documents; d; d = d->next)
	{
	  doc = d->data;
	  g_string_append (str, " ");
	  append_quoted_word (str, doc, in_single_quotes, in_double_quotes);
	}
      *documents = NULL;
      break;

    case 'f':
    case 'u':
      if (*documents)
	{
	  doc = (*documents)->data;
	  g_string_append (str, " ");
	  append_quoted_word (str, doc, in_single_quotes, in_double_quotes);
	  *documents = (*documents)->next;
	}
      break;

    case 'i':
      if (desktop_file->icon)
	{
	  g_string_append (str, "--icon ");
	  append_quoted_word (str, desktop_file->icon,
			      in_single_quotes, in_double_quotes);
	}
      break;

    case 'c':
      if (desktop_file->name)
	{
	  append_quoted_word (str, desktop_file->name,
			      in_single_quotes, in_double_quotes);
	}
      break;

    case 'k':
      if (desktop_file->source)
	{
	  append_quoted_word (str, desktop_file->source,
			      in_single_quotes, in_double_quotes);
	}
      break;

    case 'D':
    case 'N':
    case 'd':
    case 'n':
    case 'v':
    case 'm':
      /* Deprecated; skip */
      break;

    default:
      g_warning ("Unrecognized %%-code '%%%c' in Exec", code);
      break;
    }
}

static char *
parse_exec (EggDesktopFile  *desktop_file,
	    GSList         **documents,
	    GError         **error)
{
  char *exec, *p, *command;
  gboolean escape, single_quot, double_quot;
  GString *gs;

  exec = g_key_file_get_string (desktop_file->key_file,
				EGG_DESKTOP_FILE_GROUP,
				EGG_DESKTOP_FILE_KEY_EXEC,
				error);
  if (!exec)
    return NULL;

  /* Build the command */
  gs = g_string_new (NULL);
  escape = single_quot = double_quot = FALSE;

  for (p = exec; *p != '\0'; p++)
    {
      if (escape)
	{
	  escape = FALSE;
	  g_string_append_c (gs, *p);
	}
      else if (*p == '\\')
	{
	  if (!single_quot)
	    escape = TRUE;
	  g_string_append_c (gs, *p);
	}
      else if (*p == '\'')
	{
	  g_string_append_c (gs, *p);
	  if (!single_quot && !double_quot)
	    single_quot = TRUE;
	  else if (single_quot)
	    single_quot = FALSE;
	}
      else if (*p == '"')
	{
	  g_string_append_c (gs, *p);
	  if (!single_quot && !double_quot)
	    double_quot = TRUE;
	  else if (double_quot)
	    double_quot = FALSE;
	}
      else if (*p == '%' && p[1])
	{
	  do_percent_subst (desktop_file, p[1], gs, documents,
			    single_quot, double_quot);
	  p++;
	}
      else
	g_string_append_c (gs, *p);
    }

  g_free (exec);
  command = g_string_free (gs, FALSE);

  /* Prepend "xdg-terminal " if needed (FIXME: use gvfs) */
  if (g_key_file_has_key (desktop_file->key_file,
			  EGG_DESKTOP_FILE_GROUP,
			  EGG_DESKTOP_FILE_KEY_TERMINAL,
			  NULL))
    {
      GError *terminal_error = NULL;
      gboolean use_terminal =
	g_key_file_get_boolean (desktop_file->key_file,
				EGG_DESKTOP_FILE_GROUP,
				EGG_DESKTOP_FILE_KEY_TERMINAL,
				&terminal_error);
      if (terminal_error)
	{
	  g_free (command);
	  g_propagate_error (error, terminal_error);
	  return NULL;
	}

      if (use_terminal)
	{
	  gs = g_string_new ("xdg-terminal ");
	  append_quoted_word (gs, command, FALSE, FALSE);
	  g_free (command);
	  command = g_string_free (gs, FALSE);
	}
    }

  return command;
}

static GSList *
translate_document_list (EggDesktopFile *desktop_file, GSList *documents)
{
  gboolean accepts_uris = egg_desktop_file_accepts_uris (desktop_file);
  GSList *ret, *d;

  for (d = documents, ret = NULL; d; d = d->next)
    {
      const char *document = d->data;
      gboolean is_uri = !g_path_is_absolute (document);
      char *translated;

      if (accepts_uris)
	{
	  if (is_uri)
	    translated = g_strdup (document);
	  else
	    translated = g_filename_to_uri (document, NULL, NULL);
	}
      else
	{
	  if (is_uri)
	    translated = g_filename_from_uri (document, NULL, NULL);
	  else
	    translated = g_strdup (document);
	}

      if (translated)
	ret = g_slist_prepend (ret, translated);
    }

  return g_slist_reverse (ret);
}

static void
free_document_list (GSList *documents)
{
  GSList *d;

  for (d = documents; d; d = d->next)
    g_free (d->data);
  g_slist_free (documents);
}

/**
 * egg_desktop_file_parse_exec:
 * @desktop_file: a #EggDesktopFile
 * @documents: a list of document paths or URIs
 * @error: error pointer
 *
 * Parses @desktop_file's Exec key, inserting @documents into it, and
 * returns the result.
 *
 * If @documents contains non-file: URIs and @desktop_file does not
 * accept URIs, those URIs will be ignored. Likewise, if @documents
 * contains more elements than @desktop_file accepts, the extra
 * documents will be ignored.
 *
 * Return value: the parsed Exec string
 **/
char *
egg_desktop_file_parse_exec (EggDesktopFile  *desktop_file,
			     GSList          *documents,
			     GError         **error)
{
  GSList *translated, *docs;
  char *command;

  docs = translated = translate_document_list (desktop_file, documents);
  command = parse_exec (desktop_file, &docs, error);
  free_document_list (translated);

  return command;
}

static gboolean
parse_link (EggDesktopFile  *desktop_file,
	    EggDesktopFile **app_desktop_file,
	    GSList         **documents,
	    GError         **error)
{
  char *url;
  GKeyFile *key_file;

  url = g_key_file_get_string (desktop_file->key_file,
			       EGG_DESKTOP_FILE_GROUP,
			       EGG_DESKTOP_FILE_KEY_URL,
			       error);
  if (!url)
    return FALSE;
  *documents = g_slist_prepend (NULL, url);

  /* FIXME: use gvfs */
  key_file = g_key_file_new ();
  g_key_file_set_string (key_file, EGG_DESKTOP_FILE_GROUP,
			 EGG_DESKTOP_FILE_KEY_NAME,
			 "xdg-open");
  g_key_file_set_string (key_file, EGG_DESKTOP_FILE_GROUP,
			 EGG_DESKTOP_FILE_KEY_TYPE,
			 "Application");
  g_key_file_set_string (key_file, EGG_DESKTOP_FILE_GROUP,
			 EGG_DESKTOP_FILE_KEY_EXEC,
			 "xdg-open %u");
  *app_desktop_file = egg_desktop_file_new_from_key_file (key_file, NULL, NULL);
  return TRUE;
}

#if GTK_CHECK_VERSION (2, 12, 0)
static char *
start_startup_notification (GdkDisplay     *display,
			    EggDesktopFile *desktop_file,
			    const char     *argv0,
			    int             screen,
			    int             workspace,
			    guint32         launch_time)
{
  static int sequence = 0;
  char *startup_id;
  char *description, *wmclass;
  char *screen_str, *workspace_str;

  if (g_key_file_has_key (desktop_file->key_file,
			  EGG_DESKTOP_FILE_GROUP,
			  EGG_DESKTOP_FILE_KEY_STARTUP_NOTIFY,
			  NULL))
    {
      if (!g_key_file_get_boolean (desktop_file->key_file,
				   EGG_DESKTOP_FILE_GROUP,
				   EGG_DESKTOP_FILE_KEY_STARTUP_NOTIFY,
				   NULL))
	return NULL;
      wmclass = NULL;
    }
  else
    {
      wmclass = g_key_file_get_string (desktop_file->key_file,
				       EGG_DESKTOP_FILE_GROUP,
				       EGG_DESKTOP_FILE_KEY_STARTUP_WM_CLASS,
				       NULL);
      if (!wmclass)
	return NULL;
    }

  if (launch_time == (guint32)-1)
    launch_time = gdk_x11_display_get_user_time (display);
  startup_id = g_strdup_printf ("%s-%lu-%s-%s-%d_TIME%lu",
				g_get_prgname (),
				(unsigned long)getpid (),
				g_get_host_name (),
				argv0,
				sequence++,
				(unsigned long)launch_time);

  description = g_strdup_printf (_("Starting %s"), desktop_file->name);
  screen_str = g_strdup_printf ("%d", screen);
  workspace_str = workspace == -1 ? NULL : g_strdup_printf ("%d", workspace);

  gdk_x11_display_broadcast_startup_message (display, "new",
					     "ID", startup_id,
					     "NAME", desktop_file->name,
					     "SCREEN", screen_str,
					     "BIN", argv0,
					     "ICON", desktop_file->icon,
					     "DESKTOP", workspace_str,
					     "DESCRIPTION", description,
					     "WMCLASS", wmclass,
					     NULL);

  g_free (description);
  g_free (wmclass);
  g_free (screen_str);
  g_free (workspace_str);

  return startup_id;
}

static void
end_startup_notification (GdkDisplay *display,
			  const char *startup_id)
{
  gdk_x11_display_broadcast_startup_message (display, "remove",
					     "ID", startup_id,
					     NULL);
}

#define EGG_DESKTOP_FILE_SN_TIMEOUT_LENGTH (30 /* seconds */)

typedef struct {
  GdkDisplay *display;
  char *startup_id;
} StartupNotificationData;

static gboolean
startup_notification_timeout (gpointer data)
{
  StartupNotificationData *sn_data = data;

  end_startup_notification (sn_data->display, sn_data->startup_id);
  g_object_unref (sn_data->display);
  g_free (sn_data->startup_id);
  g_free (sn_data);

  return FALSE;
}

static void
set_startup_notification_timeout (GdkDisplay *display,
				  const char *startup_id)
{
  StartupNotificationData *sn_data;

  sn_data = g_new (StartupNotificationData, 1);
  sn_data->display = g_object_ref (display);
  sn_data->startup_id = g_strdup (startup_id);

  g_timeout_add_seconds (EGG_DESKTOP_FILE_SN_TIMEOUT_LENGTH,
			 startup_notification_timeout, sn_data);
}
#endif /* GTK 2.12 */

static GPtrArray *
array_putenv (GPtrArray *env, char *variable)
{
  guint i, keylen;

  if (!env)
    {
      char **envp;

      env = g_ptr_array_new ();

      envp = g_listenv ();
      for (i = 0; envp[i]; i++)
        {
          const char *value;

          value = g_getenv (envp[i]);
          g_ptr_array_add (env, g_strdup_printf ("%s=%s", envp[i],
                                                 value ? value : ""));
        }
      g_strfreev (envp);
    }

  keylen = strcspn (variable, "=");

  /* Remove old value of key */
  for (i = 0; i < env->len; i++)
    {
      char *envvar = env->pdata[i];

      if (!strncmp (envvar, variable, keylen) && envvar[keylen] == '=')
	{
	  g_free (envvar);
	  g_ptr_array_remove_index_fast (env, i);
	  break;
	}
    }

  /* Add new value */
  g_ptr_array_add (env, g_strdup (variable));

  return env;
}

static gboolean
egg_desktop_file_launchv (EggDesktopFile *desktop_file,
			  GSList *documents, va_list args,
			  GError **error)
{
  EggDesktopFileLaunchOption option;
  GSList *translated_documents = NULL, *docs = NULL;
  char *command, **argv;
  int argc, i, screen_num;
  gboolean success, current_success;
  GdkDisplay *display;
  char *startup_id;

  GPtrArray   *env = NULL;
  char       **variables = NULL;
  GdkScreen   *screen = NULL;
  int          workspace = -1;
  const char  *directory = NULL;
  guint32      launch_time = (guint32)-1;
  GSpawnFlags  flags = G_SPAWN_SEARCH_PATH;
  GSpawnChildSetupFunc setup_func = NULL;
  gpointer     setup_data = NULL;

  GPid        *ret_pid = NULL;
  int         *ret_stdin = NULL, *ret_stdout = NULL, *ret_stderr = NULL;
  char       **ret_startup_id = NULL;

  if (documents && desktop_file->document_code == 0)
    {
      g_set_error (error, EGG_DESKTOP_FILE_ERROR,
		   EGG_DESKTOP_FILE_ERROR_NOT_LAUNCHABLE,
		   _("Application does not accept documents on command line"));
      return FALSE;
    }

  /* Read the options: technically it's incorrect for the caller to
   * NULL-terminate the list of options (rather than 0-terminating
   * it), but NULL-terminating lets us use G_GNUC_NULL_TERMINATED,
   * it's more consistent with other glib/gtk methods, and it will
   * work as long as sizeof (int) <= sizeof (NULL), and NULL is
   * represented as 0. (Which is true everywhere we care about.)
   */
  while ((option = va_arg (args, EggDesktopFileLaunchOption)))
    {
      switch (option)
	{
	case EGG_DESKTOP_FILE_LAUNCH_CLEARENV:
	  if (env)
	    g_ptr_array_free (env, TRUE);
	  env = g_ptr_array_new ();
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_PUTENV:
	  variables = va_arg (args, char **);
	  for (i = 0; variables[i]; i++)
	    env = array_putenv (env, variables[i]);
	  break;

	case EGG_DESKTOP_FILE_LAUNCH_SCREEN:
	  screen = va_arg (args, GdkScreen *);
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_WORKSPACE:
	  workspace = va_arg (args, int);
	  break;

	case EGG_DESKTOP_FILE_LAUNCH_DIRECTORY:
	  directory = va_arg (args, const char *);
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_TIME:
	  launch_time = va_arg (args, guint32);
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_FLAGS:
	  flags |= va_arg (args, GSpawnFlags);
	  /* Make sure they didn't set any flags that don't make sense. */
	  flags &= ~G_SPAWN_FILE_AND_ARGV_ZERO;
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_SETUP_FUNC:
	  setup_func = va_arg (args, GSpawnChildSetupFunc);
	  setup_data = va_arg (args, gpointer);
	  break;

	case EGG_DESKTOP_FILE_LAUNCH_RETURN_PID:
	  ret_pid = va_arg (args, GPid *);
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_RETURN_STDIN_PIPE:
	  ret_stdin = va_arg (args, int *);
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_RETURN_STDOUT_PIPE:
	  ret_stdout = va_arg (args, int *);
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_RETURN_STDERR_PIPE:
	  ret_stderr = va_arg (args, int *);
	  break;
	case EGG_DESKTOP_FILE_LAUNCH_RETURN_STARTUP_ID:
	  ret_startup_id = va_arg (args, char **);
	  break;

	default:
	  g_set_error (error, EGG_DESKTOP_FILE_ERROR,
		       EGG_DESKTOP_FILE_ERROR_UNRECOGNIZED_OPTION,
		       _("Unrecognized launch option: %d"),
		       GPOINTER_TO_INT (option));
	  success = FALSE;
	  goto out;
	}
    }

  if (screen)
    {
      char *display_name = gdk_screen_make_display_name (screen);
      char *display_env = g_strdup_printf ("DISPLAY=%s", display_name);
      env = array_putenv (env, display_env);
      g_free (display_name);
      g_free (display_env);

      display = gdk_screen_get_display (screen);
    }
  else
    {
      display = gdk_display_get_default ();
      screen = gdk_display_get_default_screen (display);
    }
  screen_num = gdk_screen_get_number (screen);

  translated_documents = translate_document_list (desktop_file, documents);
  docs = translated_documents;

  success = FALSE;

  do
    {
      command = parse_exec (desktop_file, &docs, error);
      if (!command)
	goto out;

      if (!g_shell_parse_argv (command, &argc, &argv, error))
	{
	  g_free (command);
	  goto out;
	}
      g_free (command);

#if GTK_CHECK_VERSION (2, 12, 0)
      startup_id = start_startup_notification (display, desktop_file,
					       argv[0], screen_num,
					       workspace, launch_time);
      if (startup_id)
	{
	  char *startup_id_env = g_strdup_printf ("DESKTOP_STARTUP_ID=%s",
						  startup_id);
	  env = array_putenv (env, startup_id_env);
	  g_free (startup_id_env);
	}
#else
      startup_id = NULL;
#endif /* GTK 2.12 */

      if (env != NULL)
	g_ptr_array_add (env, NULL);

      current_success =
	g_spawn_async_with_pipes (directory,
				  argv,
				  env ? (char **)(env->pdata) : NULL,
				  flags,
				  setup_func, setup_data,
				  ret_pid,
				  ret_stdin, ret_stdout, ret_stderr,
				  error);
      g_strfreev (argv);

      if (startup_id)
	{
#if GTK_CHECK_VERSION (2, 12, 0)
	  if (current_success)
	    {
	      set_startup_notification_timeout (display, startup_id);

	      if (ret_startup_id)
		*ret_startup_id = startup_id;
	      else
		g_free (startup_id);
	    }
	  else
#endif /* GTK 2.12 */
	    g_free (startup_id);
	}
      else if (ret_startup_id)
	*ret_startup_id = NULL;

      if (current_success)
	{
	  /* If we successfully launch any instances of the app, make
	   * sure we return TRUE and don't set @error.
	   */
	  success = TRUE;
	  error = NULL;

	  /* Also, only set the output params on the first one */
	  ret_pid = NULL;
	  ret_stdin = ret_stdout = ret_stderr = NULL;
	  ret_startup_id = NULL;
	}
    }
  while (docs && current_success);

 out:
  if (env)
    {
      g_ptr_array_foreach (env, (GFunc)g_free, NULL);
      g_ptr_array_free (env, TRUE);
    }
  free_document_list (translated_documents);

  return success;
}

/**
 * egg_desktop_file_launch:
 * @desktop_file: an #EggDesktopFile
 * @documents: a list of URIs or paths to documents to open
 * @error: error pointer
 * @...: additional options
 *
 * Launches @desktop_file with the given arguments. Additional options
 * can be specified as follows:
 *
 *   %EGG_DESKTOP_FILE_LAUNCH_CLEARENV: (no arguments)
 *       clears the environment in the child process
 *   %EGG_DESKTOP_FILE_LAUNCH_PUTENV: (char **variables)
 *       adds the NAME=VALUE strings in the given %NULL-terminated
 *       array to the child process's environment
 *   %EGG_DESKTOP_FILE_LAUNCH_SCREEN: (GdkScreen *screen)
 *       causes the application to be launched on the given screen
 *   %EGG_DESKTOP_FILE_LAUNCH_WORKSPACE: (int workspace)
 *       causes the application to be launched on the given workspace
 *   %EGG_DESKTOP_FILE_LAUNCH_DIRECTORY: (char *dir)
 *       causes the application to be launched in the given directory
 *   %EGG_DESKTOP_FILE_LAUNCH_TIME: (guint32 launch_time)
 *       sets the "launch time" for the application. If the user
 *       interacts with another window after @launch_time but before
 *       the launched application creates its first window, the window
 *       manager may choose to not give focus to the new application.
 *       Passing 0 for @launch_time will explicitly request that the
 *       application not receive focus.
 *   %EGG_DESKTOP_FILE_LAUNCH_FLAGS (GSpawnFlags flags)
 *       Sets additional #GSpawnFlags to use. See g_spawn_async() for
 *       more details.
 *   %EGG_DESKTOP_FILE_LAUNCH_SETUP_FUNC (GSpawnChildSetupFunc, gpointer)
 *       Sets the child setup callback and the data to pass to it.
 *       (See g_spawn_async() for more details.)
 *
 *   %EGG_DESKTOP_FILE_LAUNCH_RETURN_PID (GPid **pid)
 *       On a successful launch, sets *@pid to the PID of the launched
 *       application.
 *   %EGG_DESKTOP_FILE_LAUNCH_RETURN_STARTUP_ID (char **startup_id)
 *       On a successful launch, sets *@startup_id to the Startup
 *       Notification "startup id" of the launched application.
 *   %EGG_DESKTOP_FILE_LAUNCH_RETURN_STDIN_PIPE (int *fd)
 *       On a successful launch, sets *@fd to the file descriptor of
 *       a pipe connected to the application's stdin.
 *   %EGG_DESKTOP_FILE_LAUNCH_RETURN_STDOUT_PIPE (int *fd)
 *       On a successful launch, sets *@fd to the file descriptor of
 *       a pipe connected to the application's stdout.
 *   %EGG_DESKTOP_FILE_LAUNCH_RETURN_STDERR_PIPE (int *fd)
 *       On a successful launch, sets *@fd to the file descriptor of
 *       a pipe connected to the application's stderr.
 *
 * The options should be terminated with a single %NULL.
 *
 * If @documents contains multiple documents, but
 * egg_desktop_file_accepts_multiple() returns %FALSE for
 * @desktop_file, then egg_desktop_file_launch() will actually launch
 * multiple instances of the application. In that case, the return
 * value (as well as any values passed via
 * %EGG_DESKTOP_FILE_LAUNCH_RETURN_PID, etc) will only reflect the
 * first instance of the application that was launched (but the
 * %EGG_DESKTOP_FILE_LAUNCH_SETUP_FUNC will be called for each
 * instance).
 *
 * Return value: %TRUE if the application was successfully launched.
 **/
gboolean
egg_desktop_file_launch (EggDesktopFile *desktop_file,
			 GSList *documents, GError **error,
			 ...)
{
  va_list args;
  gboolean success;
  EggDesktopFile *app_desktop_file;

  switch (desktop_file->type)
    {
    case EGG_DESKTOP_FILE_TYPE_APPLICATION:
      va_start (args, error);
      success = egg_desktop_file_launchv (desktop_file, documents,
					  args, error);
      va_end (args);
      break;

    case EGG_DESKTOP_FILE_TYPE_LINK:
      if (documents)
	{
	  g_set_error (error, EGG_DESKTOP_FILE_ERROR,
		       EGG_DESKTOP_FILE_ERROR_NOT_LAUNCHABLE,
		       /* translators: The 'Type=Link' string is found in a
			* desktop file, and should not be translated. */
		       _("Can't pass document URIs to a 'Type=Link' desktop entry"));
	  return FALSE;
	}	  

      if (!parse_link (desktop_file, &app_desktop_file, &documents, error))
	return FALSE;

      va_start (args, error);
      success = egg_desktop_file_launchv (app_desktop_file, documents,
					  args, error);
      va_end (args);

      egg_desktop_file_free (app_desktop_file);
      free_document_list (documents);
      break;

    case EGG_DESKTOP_FILE_TYPE_UNRECOGNIZED:
    case EGG_DESKTOP_FILE_TYPE_DIRECTORY:
    default:
      g_set_error (error, EGG_DESKTOP_FILE_ERROR,
		   EGG_DESKTOP_FILE_ERROR_NOT_LAUNCHABLE,
		   _("Not a launchable item"));
      success = FALSE;
      break;
    }

  return success;
}


GQuark
egg_desktop_file_error_quark (void)
{
  return g_quark_from_static_string ("egg-desktop_file-error-quark");
}


G_LOCK_DEFINE_STATIC (egg_desktop_file);
static EggDesktopFile *egg_desktop_file;

static void
egg_set_desktop_file_internal (const char *desktop_file_path,
                               gboolean set_defaults)
{
  GError *error = NULL;

  G_LOCK (egg_desktop_file);
  if (egg_desktop_file)
    egg_desktop_file_free (egg_desktop_file);

  egg_desktop_file = egg_desktop_file_new (desktop_file_path, &error);
  if (error)
    {
      g_warning ("Could not load desktop file '%s': %s",
		 desktop_file_path, error->message);
      g_error_free (error);
    }

  if (set_defaults && egg_desktop_file != NULL) {
    /* Set localized application name and default window icon */
    if (egg_desktop_file->name)
      g_set_application_name (egg_desktop_file->name);
    if (egg_desktop_file->icon)
      {
        if (g_path_is_absolute (egg_desktop_file->icon))
          gtk_window_set_default_icon_from_file (egg_desktop_file->icon, NULL);
        else
          gtk_window_set_default_icon_name (egg_desktop_file->icon);
      }
  }

  G_UNLOCK (egg_desktop_file);
}

/**
 * egg_set_desktop_file:
 * @desktop_file_path: path to the application's desktop file
 *
 * Creates an #EggDesktopFile for the application from the data at
 * @desktop_file_path. This will also call g_set_application_name()
 * with the localized application name from the desktop file, and
 * gtk_window_set_default_icon_name() or
 * gtk_window_set_default_icon_from_file() with the application's
 * icon. Other code may use additional information from the desktop
 * file.
 * See egg_set_desktop_file_without_defaults() for a variant of this
 * function that does not set the application name and default window
 * icon.
 *
 * Note that for thread safety reasons, this function can only
 * be called once, and is mutually exclusive with calling
 * egg_set_desktop_file_without_defaults().
 **/
void
egg_set_desktop_file (const char *desktop_file_path)
{
  egg_set_desktop_file_internal (desktop_file_path, TRUE);
}

/**
 * egg_set_desktop_file_without_defaults:
 * @desktop_file_path: path to the application's desktop file
 *
 * Creates an #EggDesktopFile for the application from the data at
 * @desktop_file_path.
 * See egg_set_desktop_file() for a variant of this function that
 * sets the application name and default window icon from the information
 * in the desktop file.
 *
 * Note that for thread safety reasons, this function can only
 * be called once, and is mutually exclusive with calling
 * egg_set_desktop_file().
 **/
void
egg_set_desktop_file_without_defaults (const char *desktop_file_path)
{
  egg_set_desktop_file_internal (desktop_file_path, FALSE);
}

/**
 * egg_get_desktop_file:
 * 
 * Gets the application's #EggDesktopFile, as set by
 * egg_set_desktop_file().
 * 
 * Return value: the #EggDesktopFile, or %NULL if it hasn't been set.
 **/
EggDesktopFile *
egg_get_desktop_file (void)
{
  EggDesktopFile *retval;

  G_LOCK (egg_desktop_file);
  retval = egg_desktop_file;
  G_UNLOCK (egg_desktop_file);

  return retval;
}
