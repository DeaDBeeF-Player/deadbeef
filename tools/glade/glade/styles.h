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
#ifndef GLADE_STYLES_H
#define GLADE_STYLES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef GLADE_STYLE_SUPPORT


#define GB_NUM_STYLE_STATES 5
extern gchar*  GbStateNames[];
#define GB_NUM_STYLE_COLORS 4
extern gchar*  GbColorNames[];
extern gchar*  GbBgPixmapName;

#define GB_STYLE_UNNAMED	"<none>"
#define GB_STYLE_DEFAULT	"Default"

/* This is the default GTK font spec., from gtkstyle.c */
#define GB_DEFAULT_XLFD_FONTNAME \
	"-adobe-helvetica-medium-r-normal--*-120-*-*-*-*-*-*"

/* This contains all extra info needed to recreate a style.
   NOTE: A GtkStyle may be used by more than one GbStyle. */
typedef struct _GbStyle GbStyle;
struct _GbStyle
{
  gchar		*name;
  gchar		*xlfd_fontname;
  gchar		*bg_pixmap_filenames[GB_NUM_STYLE_STATES];
  GtkStyle	*style;
  gint		 ref_count;
};


/* The style data hash stores named GbStyles with the name as the key.
   Note that the GbStyle name field is also used as the key so be careful when
   freeing or updating it. */
extern GHashTable *gb_style_hash;

/* The default GbStyle of created widgets. We need to use a separate default
   style for the widgets created so that the Glade UI doesn't change.
   Remember to push the GtkStyle before creating any gbwidgets. */
extern GbStyle *gb_widget_default_gb_style;

/* Functions for handling GbStyles. */
void	    gb_widget_reset_gb_styles	(void);
GbStyle*    gb_widget_new_gb_style	(void);
GbStyle*    gb_widget_copy_gb_style	(GbStyle	       *gbstyle);
void	    gb_widget_ref_gb_style	(GbStyle	       *gbstyle);
void	    gb_widget_unref_gb_style	(GbStyle	       *gbstyle);
void	    gb_widget_destroy_gb_style	(GbStyle	       *gbstyle,
					 gboolean		remove_from_hash);

void	    gb_widget_set_gb_style	(GtkWidget	       *widget,
					 GbStyle	       *gbstyle);
void	    gb_widget_update_gb_styles	(GbStyle	       *old_gbstyle,
					 GbStyle	       *new_gbstyle);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_STYLES_H */
