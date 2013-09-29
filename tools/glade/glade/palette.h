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
#ifndef GLADEPALETTE_H
#define GLADEPALETTE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "glade.h"

void	    palette_init			(void);
void	    palette_add_gbwidget		(GbWidget	    *gbwidget,
						 gchar		    *section,
						 gchar		    *name);
void	    palette_show			(GtkWidget	    *widget,
						 gpointer	     data);
gint	    palette_hide			(GtkWidget	    *widget,
						 gpointer	     data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADEPALETTE_H */
