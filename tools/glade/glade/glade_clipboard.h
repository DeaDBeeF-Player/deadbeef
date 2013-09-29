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
#ifndef GLADE_CLIPBOARD_H
#define GLADE_CLIPBOARD_H

#include <gtk/gtkwindow.h>

#include "glade_project.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * GladeClipboard is a subclass of GtkWindow, which shows all of the widgets
 * in the cut buffers, allowing the user to select which widget to paste.
 */


#define GLADE_CLIPBOARD(obj)          GTK_CHECK_CAST (obj, glade_clipboard_get_type (), GladeClipboard)
#define GLADE_CLIPBOARD_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, glade_clipboard_get_type (), GladeClipboardClass)
#define GLADE_IS_CLIPBOARD(obj)       GTK_CHECK_TYPE (obj, glade_clipboard_get_type ())

typedef struct _GladeClipboard       GladeClipboard;
typedef struct _GladeClipboardClass  GladeClipboardClass;

struct _GladeClipboard
{
  GtkWindow window;

  /* The list of widgets in the cut buffers. */
  GtkWidget *clist;

};

struct _GladeClipboardClass
{
  GtkWindowClass parent_class;
};


GType	    glade_clipboard_get_type		(void);
GtkWidget*  glade_clipboard_new			(void);

/* These cut/copy/paste widgets in the given project. If NULL is passed as
   the widget parameter, the currently selected widget will be used. */
void	    glade_clipboard_cut			(GladeClipboard *clipboard,
						 GladeProject   *project,
						 GtkWidget	*widget);
void	    glade_clipboard_copy		(GladeClipboard *clipboard,
						 GladeProject   *project,
						 GtkWidget	*widget);
void	    glade_clipboard_paste		(GladeClipboard *clipboard,
						 GladeProject   *project,
						 GtkWidget	*widget);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_CLIPBOARD_H */
