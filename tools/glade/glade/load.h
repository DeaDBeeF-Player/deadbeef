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
#ifndef GLADE_LOAD_H
#define GLADE_LOAD_H

#include <time.h>

#include "gbwidget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


gboolean    load_project_file		(GladeProject      *project);


gchar*	    load_string			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
gchar*	    load_text			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
gint	    load_int			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
gfloat	    load_float			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
gboolean    load_bool			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
gchar*	    load_choice			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
gchar*	    load_combo			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
GdkColor*   load_color			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
GdkPixmap*  load_bgpixmap		(GbWidgetSetArgData *data,
					 const gchar	    *property_name,
					 gchar		   **filename);
gpointer    load_dialog			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
gchar*	    load_filename		(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
/* This will convert any relative filenames to absolute ones, based on the
   project directory and/or pixmaps directory options. It should be freed
   when no longer needed. */
gchar*	    load_pixmap_filename	(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
GdkFont*    load_font			(GbWidgetSetArgData *data,
					 const gchar	    *property_name,
					 gchar		   **xlfd_fontname);
time_t	    load_date			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);
gchar*	    load_icon			(GbWidgetSetArgData *data,
					 const gchar	    *property_name);


gboolean    load_parse_bool		(GbWidgetSetArgData *data,
					 const gchar	    *value);
GdkColor*   load_parse_color		(GbWidgetSetArgData *data,
					 const gchar	    *value);
time_t	    load_parse_date		(GbWidgetSetArgData * data,
					 const gchar	    *value);

gchar*	    load_get_value		(GbWidgetSetArgData *data,
					 const gchar	    *property_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_LOAD_H */
