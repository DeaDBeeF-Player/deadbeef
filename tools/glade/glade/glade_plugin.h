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

/*
 * File		: glade_plugin.h
 * Description	: Provides support for plugin widget libraries which use Args.
 *		  Unfinished, experimental code at present.
 *		  For Gnome, we may want to think about bonobo components,
 *		  rather like COM/ActiveX components in Delphi/VB.
 */
#ifndef GLADE_PLUGIN_H
#define GLADE_PLUGIN_H

#include "gladeconfig.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* This loads extra libraries of widgets which we will access via the GTK+
   Arg functions. */
void	    glade_plugin_load_plugins		(void);


/* This creates a new GbWidget which may have different tooltip/icon fields,
   but the functions will all be the same, since we can handle Args
   generically. */
GbWidget*   glade_plugin_new			(void);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_PLUGIN_H */
