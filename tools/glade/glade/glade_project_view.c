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

#include "gladeconfig.h"

#include <gtk/gtk.h>
#include "editor.h"
#include "glade_project_view.h"
#include "utils.h"

GladeProjectView *current_project_view = NULL;

static GtkCListClass *parent_class = NULL;

static void glade_project_view_class_init (GladeProjectViewClass * klass);
static void glade_project_view_init (GladeProjectView * project_view);

void set_project_callback (GtkWidget *component,
			   GladeProjectView *project_view);

static void glade_project_view_real_add_component (GladeProject     *project,
						   GtkWidget        *component,
						   GladeProjectView *project_view);
static void glade_project_view_real_remove_component (GladeProject     *project,
						      GtkWidget        *component,
						      GladeProjectView *project_view);
static void glade_project_view_component_changed (GladeProject     *project,
						  GtkWidget        *component,
						  GladeProjectView *project_view);
static void glade_project_view_select_component (GladeProjectView *project_view,
						 gint	           row,
						 gint	           column,
						 GdkEventButton   *bevent,
						 gpointer	   data);

GType
glade_project_view_get_type (void)
{
  static GType glade_project_view_type = 0;

  if (!glade_project_view_type)
    {
      GtkTypeInfo glade_project_view_info =
      {
	"GladeProjectView",
	sizeof (GladeProjectView),
	sizeof (GladeProjectViewClass),
	(GtkClassInitFunc) glade_project_view_class_init,
	(GtkObjectInitFunc) glade_project_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      glade_project_view_type = gtk_type_unique (gtk_clist_get_type (),
						 &glade_project_view_info);
    }
  return glade_project_view_type;
}


static void
glade_project_view_class_init (GladeProjectViewClass * klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (gtk_clist_get_type ());
}


static void
glade_project_view_init (GladeProjectView * project_view)
{
  project_view->project = NULL;
  /* This no longer exists in GTK+ 2.0, so I hope it isn't needed. */
  /*gtk_clist_construct (GTK_CLIST (project_view), 1, NULL);*/
}


GtkWidget *
glade_project_view_new ()
{
  GtkWidget *project_view;

  project_view = GTK_WIDGET (gtk_type_new (glade_project_view_get_type ()));
  gtk_clist_column_titles_hide (GTK_CLIST (project_view));
  gtk_clist_set_row_height (GTK_CLIST (project_view), 20);
  gtk_clist_set_column_width (GTK_CLIST (project_view), 0, 140);
  gtk_widget_set_usize (project_view, 172, 100);
  gtk_signal_connect (GTK_OBJECT (project_view), "select_row",
		      GTK_SIGNAL_FUNC (glade_project_view_select_component),
		      NULL);

  /* FIXME: We only support one project view at present. */
  current_project_view = GLADE_PROJECT_VIEW (project_view);

  return project_view;
}


GladeProject*
glade_project_view_get_project      (GladeProjectView    *project_view)
{
  return project_view->project;
}


void
glade_project_view_set_project      (GladeProjectView    *project_view,
				     GladeProject	 *project)
{
  if (project_view->project == project)
    return;

  project_view->project = project;
  gtk_clist_clear (GTK_CLIST (project_view));

  if (project)
    {
      glade_project_foreach_component (project,
				       (GtkCallback) set_project_callback,
				       project_view);
      gtk_signal_connect (GTK_OBJECT (project), "add_component",
			  GTK_SIGNAL_FUNC (glade_project_view_real_add_component),
			  project_view);
      gtk_signal_connect (GTK_OBJECT (project), "remove_component",
			  GTK_SIGNAL_FUNC (glade_project_view_real_remove_component),
			  project_view);
      gtk_signal_connect (GTK_OBJECT (project), "component_changed",
			  GTK_SIGNAL_FUNC (glade_project_view_component_changed),
			  project_view);
    }
}


void
set_project_callback (GtkWidget *component,
		      GladeProjectView *project_view)
{
  GbWidget *gbwidget;
  gchar *name;
  gint row;

  name = (gchar*) gtk_widget_get_name (component);
  gbwidget = gb_widget_lookup (component);
  g_return_if_fail (gbwidget != NULL);

  row = gtk_clist_append (GTK_CLIST (project_view), &name);
  gtk_clist_set_row_data (GTK_CLIST (project_view), row, component);
  gtk_clist_set_pixtext (GTK_CLIST (project_view), row, 0, name, 3,
			 gbwidget->gdkpixmap, gbwidget->mask);
}


static void
glade_project_view_real_add_component (GladeProject     *project,
				       GtkWidget        *component,
				       GladeProjectView *project_view)
{
  GbWidget * gbwidget;
  gchar *name;
  gint row;

  name = (gchar*) gtk_widget_get_name (component);
  gbwidget = gb_widget_lookup (component);
  g_return_if_fail (gbwidget != NULL);

  row = gtk_clist_append (GTK_CLIST (project_view), &name);
  gtk_clist_set_row_data (GTK_CLIST (project_view), row, component);
  gtk_clist_set_pixtext (GTK_CLIST (project_view), row, 0, name, 3,
			 gbwidget->gdkpixmap, gbwidget->mask);
}


static void
glade_project_view_real_remove_component (GladeProject     *project,
					  GtkWidget        *component,
					  GladeProjectView *project_view)
{
  gint row;

  row = gtk_clist_find_row_from_data (GTK_CLIST (project_view), component);
  g_return_if_fail (row != -1);
  gtk_clist_remove (GTK_CLIST (project_view), row);
}


/* This is called when a 'component_changed' signal has been emitted by the
   GladeProject. Currently that only happens when the component's name has
   changed. */
static void
glade_project_view_component_changed (GladeProject     *project,
				      GtkWidget        *component,
				      GladeProjectView *project_view)
{
  gchar *name, *old_name;
  gint row;
  guint8 spacing;
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  row = gtk_clist_find_row_from_data (GTK_CLIST (project_view), component);
  g_return_if_fail (row != -1);
  gtk_clist_get_pixtext (GTK_CLIST (project_view), row, 0, &old_name, &spacing,
			 &pixmap, &mask);
  name = (gchar*) gtk_widget_get_name (component);
  gtk_clist_set_pixtext (GTK_CLIST (project_view), row, 0, name, spacing,
			 pixmap, mask);
}


void
glade_project_view_clear_component_selection (GladeProjectView *project_view,
					      GtkWidget        *widget)
{
  GtkWidget *component;
  GList *selection, *list;
  gint row;

  list = g_list_copy (GTK_CLIST (project_view)->selection);

  selection = list;
  while (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      component = gtk_clist_get_row_data (GTK_CLIST (project_view), row);
      if (component != widget)
	{
	  gtk_clist_unselect_row (GTK_CLIST (project_view), row, 0);
	}
      selection = selection->next;
    }

  g_list_free (list);
}


static void
glade_project_view_select_component (GladeProjectView *project_view,
				     gint	       row,
				     gint	       column,
				     GdkEventButton   *bevent,
				     gpointer	       data)
{
  GtkWidget *component;

  component = (GtkWidget*) gtk_clist_get_row_data (GTK_CLIST (project_view),
						   row);
  g_return_if_fail (component != NULL);

  /* Select the widget, so it can be copied/pasted. */
  editor_clear_selection (component);
  if (!editor_is_selected (component))
    {
      editor_select_widget_control (component);
    }
  
  /* Show the properties of the component. */
  gb_widget_show_properties (component);

  /* If the component is double-clicked we also show it. */
  if (bevent && bevent->type == GDK_2BUTTON_PRESS)
    glade_project_show_component (project_view->project, component);
}


gboolean
glade_project_view_has_selection (GladeProjectView *project_view)
{
  if (GTK_CLIST (project_view)->selection)
    return TRUE;
  else
    return FALSE;
}


void
glade_project_view_delete_selection (GladeProjectView *project_view)
{
  GtkWidget *component;
  GList *selection;

  selection = GTK_CLIST (project_view)->selection;
  while (selection)
    {
      component = gtk_clist_get_row_data (GTK_CLIST (project_view),
					  GPOINTER_TO_INT (selection->data));
      glade_project_remove_component (project_view->project, component);

      /* The deletion may have changed the selection, so we simply start at
	 the beginning of the list each time. */
      selection = GTK_CLIST (project_view)->selection;
    }
}
