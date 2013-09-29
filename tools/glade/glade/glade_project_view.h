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
#ifndef GLADE_PROJECT_VIEW_H
#define GLADE_PROJECT_VIEW_H

#include <gtk/gtkclist.h>

#include "glade_project.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * GladeProjectView is a subclass of GtkCList, used to show the project
 * components. (It may change to a subclass of CTree later).
 *
 * I've made it a separate widget so it can be placed anywhere in an
 * interface, e.g. an IDE may place it in a notebook, with one notebook page
 * containing the source files in the project, and another containing a
 * GladeProjectView to show the components/widgets in a project.
 */

#define GLADE_PROJECT_VIEW(obj)          GTK_CHECK_CAST (obj, glade_project_view_get_type (), GladeProjectView)
#define GLADE_PROJECT_VIEW_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, glade_project_view_get_type (), GladeProjectViewClass)
#define GLADE_IS_PROJECT_VIEW(obj)       GTK_CHECK_TYPE (obj, glade_project_view_get_type ())


typedef struct _GladeProjectView       GladeProjectView;
typedef struct _GladeProjectViewClass  GladeProjectViewClass;

struct _GladeProjectView
{
  GtkCList clist;

  GladeProject *project;
};


struct _GladeProjectViewClass
{
  GtkCListClass parent_class;
};


/* FIXME: Currently we only support one project view at once, and this is it.
   But we will support multiple projects in future, so try not to use this
   too much. */
extern GladeProjectView *current_project_view;


GType      glade_project_view_get_type	      (void);
GtkWidget* glade_project_view_new	      (void);

GladeProject* glade_project_view_get_project  (GladeProjectView *project_view);
void	   glade_project_view_set_project     (GladeProjectView *project_view,
					       GladeProject	 *project);

gboolean   glade_project_view_has_selection   (GladeProjectView *project_view);
/* Deselect all components, except the given widget */
void	   glade_project_view_clear_component_selection	(GladeProjectView *project_view,
							 GtkWidget *widget);
void       glade_project_view_delete_selection(GladeProjectView *project_view);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_PROJECT_VIEW_H */
