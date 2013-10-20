/* GtkGLExt - OpenGL Extension to GTK+
 * Copyright (C) 2002-2004  Naofumi Yasufuku
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.
 */

#ifndef __GTK_GL_WIDGET_H__
#define __GTK_GL_WIDGET_H__

#include <gdk/gdk.h>

#include <gdk/gdkgl.h>

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

gboolean      gtk_widget_set_gl_capability (GtkWidget    *widget,
                                            GdkGLConfig  *glconfig,
                                            GdkGLContext *share_list,
                                            gboolean      direct,
                                            int           render_type);

gboolean      gtk_widget_is_gl_capable     (GtkWidget    *widget);


GdkGLConfig  *gtk_widget_get_gl_config     (GtkWidget    *widget);

GdkGLContext *gtk_widget_create_gl_context (GtkWidget    *widget,
                                            GdkGLContext *share_list,
                                            gboolean      direct,
                                            int           render_type);

GdkGLContext *gtk_widget_get_gl_context    (GtkWidget    *widget);

GdkGLWindow  *gtk_widget_get_gl_window     (GtkWidget    *widget);

#define       gtk_widget_get_gl_drawable(widget)        \
  GDK_GL_DRAWABLE (gtk_widget_get_gl_window (widget))

G_END_DECLS

#endif /* __GTK_GL_WIDGET_H__ */
