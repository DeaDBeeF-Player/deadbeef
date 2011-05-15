/* GTK - The GIMP Toolkit
 * Copyright (C) 2006-2007 Async Open Source,
 *                         Johan Dahlin <jdahlin@async.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GTK_BUILDABLE_H__
#define __GTK_BUILDABLE_H__

#include <glib.h>
#include <gtk/gtkbuilder.h>
#include <gtk/gtktypeutils.h>

G_BEGIN_DECLS

#define GTK_TYPE_BUILDABLE            (gtk_buildable_get_type ())
#define GTK_BUILDABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_BUILDABLE, GtkBuildable))
#define GTK_BUILDABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), GTK_TYPE_BUILDABLE, GtkBuildableIface))
#define GTK_IS_BUILDABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_BUILDABLE))
#define GTK_BUILDABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTK_TYPE_BUILDABLE, GtkBuildableIface))

typedef struct _GtkBuildable      GtkBuildable; /* Dummy typedef */
typedef struct _GtkBuildableIface GtkBuildableIface;

struct _GtkBuildableIface
{
  GTypeInterface g_iface;

  /* virtual table */
  void          (* set_name)               (GtkBuildable  *buildable,
                                            const gchar   *name);
  const gchar * (* get_name)               (GtkBuildable  *buildable);
  void          (* add_child)              (GtkBuildable  *buildable,
					    GtkBuilder    *builder,
					    GObject       *child,
					    const gchar   *type);
  void          (* set_buildable_property) (GtkBuildable  *buildable,
					    GtkBuilder    *builder,
					    const gchar   *name,
					    const GValue  *value);
  GObject *     (* construct_child)        (GtkBuildable  *buildable,
					    GtkBuilder    *builder,
					    const gchar   *name);
  gboolean      (* custom_tag_start)       (GtkBuildable  *buildable,
					    GtkBuilder    *builder,
					    GObject       *child,
					    const gchar   *tagname,
					    GMarkupParser *parser,
					    gpointer      *data);
  void          (* custom_tag_end)         (GtkBuildable  *buildable,
					    GtkBuilder    *builder,
					    GObject       *child,
					    const gchar   *tagname,
					    gpointer      *data);
  void          (* custom_finished)        (GtkBuildable  *buildable,
					    GtkBuilder    *builder,
					    GObject       *child,
					    const gchar   *tagname,
					    gpointer       data);
  void          (* parser_finished)        (GtkBuildable  *buildable,
					    GtkBuilder    *builder);

  GObject *     (* get_internal_child)     (GtkBuildable  *buildable,
					    GtkBuilder    *builder,
					    const gchar   *childname);
};


GType     gtk_buildable_get_type               (void) G_GNUC_CONST;

void      gtk_buildable_set_name               (GtkBuildable        *buildable,
						const gchar         *name);
const gchar * gtk_buildable_get_name           (GtkBuildable        *buildable);
void      gtk_buildable_add_child              (GtkBuildable        *buildable,
						GtkBuilder          *builder,
						GObject             *child,
						const gchar         *type);
void      gtk_buildable_set_buildable_property (GtkBuildable        *buildable,
						GtkBuilder          *builder,
						const gchar         *name,
						const GValue        *value);
GObject * gtk_buildable_construct_child        (GtkBuildable        *buildable,
						GtkBuilder          *builder,
						const gchar         *name);
gboolean  gtk_buildable_custom_tag_start       (GtkBuildable        *buildable,
						GtkBuilder          *builder,
						GObject             *child,
						const gchar         *tagname,
						GMarkupParser       *parser,
						gpointer            *data);
void      gtk_buildable_custom_tag_end         (GtkBuildable        *buildable,
						GtkBuilder          *builder,
						GObject             *child,
						const gchar         *tagname,
						gpointer            *data);
void      gtk_buildable_custom_finished        (GtkBuildable        *buildable,
						GtkBuilder          *builder,
						GObject             *child,
						const gchar         *tagname,
						gpointer             data);
void      gtk_buildable_parser_finished        (GtkBuildable        *buildable,
						GtkBuilder          *builder);
GObject * gtk_buildable_get_internal_child     (GtkBuildable        *buildable,
						GtkBuilder          *builder,
						const gchar         *childname);

G_END_DECLS

#endif /* __GTK_BUILDABLE_H__ */
