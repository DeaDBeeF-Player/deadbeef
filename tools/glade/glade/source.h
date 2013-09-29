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
#ifndef GLADE_SOURCE_H
#define GLADE_SOURCE_H

#include "gbwidget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* This is the main function for writing C source output for an application,
   and is called from glade_project_write_c_source().
   If an error occurs, a GladeError is returned which should be freed with
   glade_error_free() when no longer needed. */
GladeError*	source_write		(GladeProject		 *project);


/* Adds some source code to one of the buffers, using printf-like format
   and arguments. */
void		source_add_to_buffer	(GbWidgetWriteSourceData *data,
					 GladeSourceBuffer	  buffer,
					 const gchar		 *fmt,
					 ...) G_GNUC_PRINTF (3, 4);

/* A va_list implementation of the above. */
void		source_add_to_buffer_v	(GbWidgetWriteSourceData *data,
					 GladeSourceBuffer	  buffer,
					 const gchar		 *fmt,
					 va_list		  args);

/* Convenience functions to add to the 2 main source buffers, containing
   the code which creates the widgets and the declarations of the widgets. */
void		source_add		(GbWidgetWriteSourceData *data,
					 const gchar		 *fmt,
					 ...) G_GNUC_PRINTF (2, 3);
void		source_add_decl		(GbWidgetWriteSourceData *data,
					 const gchar		 *fmt,
					 ...) G_GNUC_PRINTF (2, 3);

/* This ensures that a temporary variable is declared, by adding the given
   declaration if it has not already been added. */
void		source_ensure_decl	(GbWidgetWriteSourceData *data,
					 const gchar		 *decl);

/* This outputs the comments string as a C comment if translatable is set.
   It is intended to be used for comments to be picked up by gettext. */
void		source_add_translator_comments (GbWidgetWriteSourceData *data,
						gboolean	  translatable,
						const gchar	 *comments);
void		source_add_translator_comments_to_buffer (GbWidgetWriteSourceData *data,
							  GladeSourceBuffer buffer,
							  gboolean translatable,
							  const gchar *comments);

/* This creates a valid C identifier from a given string (usually the name of
   a widget). The returned string should be freed when no longer needed. */
gchar*		source_create_valid_identifier (const gchar	*name);

/* This creates a string literal to place into the source code, using the
   given text, and includes the quotation marks, e.g. "Hello World".
   If translatable is TRUE, it also uses the gettext macro,
   e.g. _("Hello World"). The returned string is only valid until the next
   call to the function (since the same buffer is used). */
gchar*		source_make_string	(const gchar	*text,
					 gboolean	 translatable);

/* This is like source_make_string, but if context is set it uses the Q_
   macro which will call g_strip_context() from the translated string. */
gchar *		source_make_string_full (const gchar	*text,
					 gboolean	 translatable,
					 gboolean	 context);

/* This is similar to source_make_string, but when using gettext it uses "N_"
   so it should be used when strings are used in structs. */
gchar*		source_make_static_string (const gchar	*text,
					   gboolean	 translatable);

/* This outputs code to create a GtkImage widget with the given identifier,
   and using the given filename (only the basename is used). If filename NULL
   or "" an empty GtkImage is created. */
void		source_create_pixmap	(GbWidgetWriteSourceData * data,
					 const gchar             * identifier,
					 const gchar             * filename);

/* This outputs code to create a GdkPixbuf with the given identifier,
   and using the given filename (only the basename is used). filename must not
   be NULL or "". */
void		source_create_pixbuf	(GbWidgetWriteSourceData * data,
					 const gchar             * identifier,
					 const gchar             * filename);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_SOURCE_H */
