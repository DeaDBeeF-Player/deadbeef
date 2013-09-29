/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-2002  Damon Chaplin
 *  Copyright (C) 2002 Sun Microsystems, Inc.
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
 * This file contains most of the ATK-related code in Glade.
 * It handles the properties, actions and relations in the property editor.
 */

#ifndef GLADE_ATK_H
#define GLADE_ATK_H

#include <gtk/gtknotebook.h>
#include "gbwidget.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* This creates the ATK properties page of the property editor. */
void	    glade_atk_create_property_page	(GtkNotebook *notebook);

/* This shows the widget's ATK properties in the property editor. */
void	    glade_atk_get_properties		(GtkWidget * widget,
						 GbWidgetGetArgData * data);

/* This updates any relations dialogs which are currently visible, when the
   widget being shown in the property editor changes (maybe to NULL). */
void	    glade_atk_update_relation_dialogs	(void);

/* This applies the ATK properties from the property editor. */
void	    glade_atk_set_properties		(GtkWidget * widget,
						 GbWidgetSetArgData * data);

/* This saves the ATK properties in the XML file. */
void	    glade_atk_save_properties		(GtkWidget * widget,
						 GbWidgetGetArgData * data);

/* This loads the ATK properties from the XML file. It is called after all
   widgets are created, so it can resolve relation targets properly. */
void	    glade_atk_load_properties		(GtkWidget * widget,
						 GladeWidgetInfo * info,
						 GHashTable *all_widgets);

/* This generates the source code to set the ATK properties. */
void	    glade_atk_write_source		(GtkWidget * widget,
						 GbWidgetWriteSourceData * data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_ATK_H */
