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
#ifndef GLADE_SAVE_H
#define GLADE_SAVE_H

#include <time.h>

#include "gbwidget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* This is set if we are saving a session. A bit of a hack. */
extern char *GladeSessionFile;


/* This is the main function called by glade_project_save() to save the XML. */
GladeError* save_project_file		(GladeProject       *project);


/* Adds a start tag with no attributes, e.g. "<packing>", to begin a new
   section. It is placed on its own line, and the indentation level is
   incremented. The tag_name parameter should not include the '<' and '>'. */
void	    save_start_tag		(GbWidgetGetArgData *data,
					 const gchar	    *tag_name);

/* These are for the <widget> and <child> start tags. */
void	    save_widget_start_tag	(GbWidgetGetArgData *data,
					 const gchar	    *class_name,
					 const gchar	    *id);
void	    save_child_start_tag	(GbWidgetGetArgData *data,
					 const gchar	    *child_name);

/* Adds an end tag, e.g. "</widget>", to end a section.
   It is placed on its own line, after decrementing the indentation level.
   The tag_name parameter should not include the '</' and '>'. */
void	    save_end_tag		(GbWidgetGetArgData *data,
					 const gchar	    *tag_name);

/* Adds a <placeholder/> tag to the output buffer. */
void	    save_placeholder		(GbWidgetGetArgData *data);

/* Starts a new line in the output buffer (without indenting). */
void	    save_newline		(GbWidgetGetArgData *data);

/* Adds a translatable string to be output in the translatable strings file. */
void	    save_add_translatable_string(GbWidgetGetArgData *data,
					 const gchar	    *string);


/* These functions are called to save different types of widget properties.
   They all convert the property to a string representation and call
   save_string() to output it. The tag_name is usually the long name of the
   property, e.g. "GtkLabel::justify", so we cut out the first part and output
   <justify>...</justify>. */

void	    save_string			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_translatable_string	(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_text			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_translatable_text	(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
  void	    save_translatable_text_in_lines (GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_int			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 gint		     tag_value);
void	    save_float			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 gfloat		     tag_value);
void	    save_bool			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 gint		     tag_value);
void	    save_choice			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_combo			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_color			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 GdkColor	    *tag_value);
void	    save_bgpixmap		(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_dialog			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_filename		(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_pixmap_filename	(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_font			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);
void	    save_date			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 time_t		     tag_value);
void	    save_icon			(GbWidgetGetArgData *data,
					 const gchar	    *tag_name,
					 const gchar	    *tag_value);


void	    save_signal			(GbWidgetGetArgData *data,
					 gchar		    *signal_name,
					 gchar		    *handler,
					 gboolean	     after,
					 gchar		    *object,
					 time_t		     last_modification_time);
void	    save_accelerator		(GbWidgetGetArgData *data,
					 guint8		     modifiers,
					 gchar		    *key,
					 gchar		    *signal);

/* Outputs tabs & spaces to indent the line according to the current
   indentation level. Tabs are used to cut down on the file size a bit. */
void	    save_buffer_add_indent	(GString	    *buffer,
					 gint		     indent);

/* Adds the string to the buffer, converting special XML characters like '<'
   to entities like '%lt;'. */
void	    save_buffer_add_string	(GString	    *buffer,
					 const gchar	    *string);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_SAVE_H */
