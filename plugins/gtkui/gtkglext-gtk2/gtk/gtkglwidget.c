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

#include <gtk/gtkmain.h>

#include "gtkglprivate.h"
#include "gtkglwidget.h"

typedef struct
{
  GdkGLConfig *glconfig;
  GdkGLContext *share_list;
  gboolean direct;
  int render_type;

  GdkGLContext *glcontext;

  gulong unrealize_handler;

  guint is_realized : 1;

} GLWidgetPrivate;

static const gchar quark_gl_private_string[] = "gtk-gl-widget-private";
static GQuark quark_gl_private = 0;

gboolean _gtk_gl_widget_install_toplevel_cmap = FALSE;

static void     gtk_gl_widget_realize            (GtkWidget         *widget,
                                                  GLWidgetPrivate   *private);
static gboolean gtk_gl_widget_configure_event    (GtkWidget         *widget,
                                                  GdkEventConfigure *event,
                                                  GLWidgetPrivate   *private);
static void     gtk_gl_widget_size_allocate      (GtkWidget         *widget,
                                                  GtkAllocation     *allocation,
                                                  GLWidgetPrivate   *private);
static void     gtk_gl_widget_unrealize          (GtkWidget         *widget,
                                                  GLWidgetPrivate   *private);
static void     gtk_gl_widget_parent_set         (GtkWidget         *widget,
                                                  GtkObject         *old_parent,
                                                  GdkColormap       *colormap);
static void     gtk_gl_widget_style_set          (GtkWidget         *widget,
                                                  GtkStyle          *previous_style,
                                                  gpointer           user_data);

static void     gl_widget_private_destroy        (GLWidgetPrivate   *private);

/* 
 * Signal handlers.
 */

static void
gtk_gl_widget_realize (GtkWidget       *widget,
                       GLWidgetPrivate *private)
{
  GdkGLWindow *glwindow;

  GTK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Set OpenGL-capability to widget->window, then connect some signal
   * handlers.
   */

  if (!gdk_window_is_gl_capable (widget->window))
    {
      glwindow = gdk_window_set_gl_capability (widget->window,
                                               private->glconfig,
                                               NULL);
      if (glwindow == NULL)
        {
          g_warning ("cannot set OpenGL-capability to widget->window\n");
          return;
        }

      /* Connect "unrealize" signal handler. */
      if (private->unrealize_handler == 0)
        private->unrealize_handler = g_signal_connect (G_OBJECT (widget), "unrealize",
                                                       G_CALLBACK (gtk_gl_widget_unrealize),
                                                       private);
    }

  private->is_realized = TRUE;
}

static gboolean
gtk_gl_widget_configure_event (GtkWidget         *widget,
                               GdkEventConfigure *event,
                               GLWidgetPrivate   *private)
{
  GTK_GL_NOTE_FUNC_PRIVATE ();

  if (!private->is_realized)
    {
      /* Realize if OpenGL-capable window is not realized yet. */
      gtk_gl_widget_realize (widget, private);
    }

  return FALSE;
}

static void
gtk_gl_widget_size_allocate (GtkWidget       *widget,
                             GtkAllocation   *allocation,
                             GLWidgetPrivate *private)
{
  GdkGLDrawable *gldrawable;

  GTK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Synchronize OpenGL and window resizing request streams.
   */

  if (GTK_WIDGET_REALIZED (widget) && private->is_realized)
    {
      gldrawable = gdk_window_get_gl_drawable (widget->window);
      gdk_gl_drawable_wait_gdk (gldrawable);
    }
}

static void
gtk_gl_widget_unrealize (GtkWidget       *widget,
                         GLWidgetPrivate *private)
{
  GTK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Destroy OpenGL rendering context.
   */

  if (private->glcontext != NULL)
    {
      gdk_gl_context_destroy (private->glcontext);
      private->glcontext = NULL;
    }

  /*
   * Remove OpenGL-capability from widget->window.
   */

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_unset_gl_capability (widget->window);

  private->is_realized = FALSE;
}

static void
gtk_gl_widget_parent_set (GtkWidget   *widget,
                          GtkObject   *old_parent,
                          GdkColormap *colormap)
{
  GtkWidget *toplevel;

  GTK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Try to install colormap to the top-level window.
   */

  toplevel = gtk_widget_get_toplevel (widget);
  if (GTK_WIDGET_TOPLEVEL (toplevel) && !GTK_WIDGET_REALIZED (toplevel))
    {
      GTK_GL_NOTE (MISC,
        g_message (" - Install colormap to the top-level window."));

      gtk_widget_set_colormap (toplevel, colormap);
    }
}

static void
gtk_gl_widget_style_set (GtkWidget *widget,
                         GtkStyle  *previous_style,
                         gpointer   user_data)
{
  GTK_GL_NOTE_FUNC_PRIVATE ();

  /* 
   * Set a background of "None" on window to avoid AIX X server crash.
   */

  if (GTK_WIDGET_REALIZED (widget))
    {
      GTK_GL_NOTE (MISC,
        g_message (" - window->bg_pixmap = %p",
                   ((GdkWindowObject *) (widget->window))->bg_pixmap));

      gdk_window_set_back_pixmap (widget->window, NULL, FALSE);

      GTK_GL_NOTE (MISC,
        g_message (" - window->bg_pixmap = %p",
                   ((GdkWindowObject *) (widget->window))->bg_pixmap));
    }
}

static void
gl_widget_private_destroy (GLWidgetPrivate *private)
{
  GTK_GL_NOTE_FUNC_PRIVATE ();

  g_object_unref (G_OBJECT (private->glconfig));

  if (private->share_list != NULL)
    g_object_unref (G_OBJECT (private->share_list));

  g_free (private);
}

/**
 * gtk_widget_set_gl_capability:
 * @widget: the #GtkWidget to be used as the rendering area.
 * @glconfig: a #GdkGLConfig.
 * @share_list: the #GdkGLContext with which to share display lists and texture
 *              objects. NULL indicates that no sharing is to take place.
 * @direct: whether rendering is to be done with a direct connection to
 *          the graphics system.
 * @render_type: GDK_GL_RGBA_TYPE or GDK_GL_COLOR_INDEX_TYPE (currently not
 *               used).
 *
 * Set the OpenGL-capability to the @widget.
 * This function prepares the widget for its use with OpenGL.
 *
 * Return value: TRUE if it is successful, FALSE otherwise.
 **/
gboolean
gtk_widget_set_gl_capability (GtkWidget    *widget,
                              GdkGLConfig  *glconfig,
                              GdkGLContext *share_list,
                              gboolean      direct,
                              int           render_type)
{
  GdkColormap *colormap;
  GLWidgetPrivate *private;

  GTK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (!GTK_WIDGET_NO_WINDOW (widget), FALSE);
  g_return_val_if_fail (!GTK_WIDGET_REALIZED (widget), FALSE);
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  /* 
   * Init quark.
   */

  if (quark_gl_private == 0)
    quark_gl_private = g_quark_from_static_string (quark_gl_private_string);

  /*
   * Already OpenGL-capable?
   */

  if (gtk_widget_is_gl_capable (widget))
    return TRUE;

  /*
   * Set OpenGL-capable colormap.
   */

  colormap = gdk_gl_config_get_colormap (glconfig);

  gtk_widget_set_colormap (widget, colormap);

  /* Install colormap to the top-level window. */
  if (_gtk_gl_widget_install_toplevel_cmap)
    {
      /*
       * If window manager doesn't watch the WM_COLORMAP_WINDOWS property on
       * the top-level window, we have to set OpenGL window's colormap to the
       * top-level window, especially in color index mode (color index mode
       * uses own private colormap).
       */

      /* Signal handler to set colormap to the top-level window. */
      g_signal_connect (G_OBJECT (widget), "parent_set",
                        G_CALLBACK (gtk_gl_widget_parent_set),
                        colormap);

      /* If given widget has the top-level window, colormap is set here. */
      gtk_gl_widget_parent_set (widget, NULL, colormap);
    }

  /*
   * Disable backing store feature of the widget.
   */

  gtk_widget_set_double_buffered (widget, FALSE);

  /* 
   * "style_set" signal handler to set a background of "None" on window.
   * (relates AIX X server crash)
   */

  g_signal_connect_after (G_OBJECT (widget), "style_set",
                          G_CALLBACK (gtk_gl_widget_style_set),
                          NULL);

  /*
   * Set GL widget's private data.
   */

  private = g_new (GLWidgetPrivate, 1);

  private->glconfig = glconfig;
  g_object_ref (G_OBJECT (private->glconfig));

  if (share_list != NULL && GDK_IS_GL_CONTEXT (share_list))
    {
      private->share_list = share_list;
      g_object_ref (G_OBJECT (private->share_list));
    }
  else
    {
      private->share_list = NULL;
    }

  private->direct = direct;
  private->render_type = render_type;

  private->glcontext = NULL;

  private->unrealize_handler = 0;

  private->is_realized = FALSE;

  g_object_set_qdata_full (G_OBJECT (widget), quark_gl_private, private,
                           (GDestroyNotify) gl_widget_private_destroy);

  /*
   * Connect signal handlers to realize OpenGL-capable widget.
   */

  g_signal_connect (G_OBJECT (widget), "realize",
                    G_CALLBACK (gtk_gl_widget_realize),
                    private);

  /* gtk_drawing_area sends configure_event when it is realized. */
  g_signal_connect (G_OBJECT (widget), "configure_event",
                    G_CALLBACK (gtk_gl_widget_configure_event),
                    private);

  /*
   * Connect "size_allocate" signal handler to synchronize OpenGL and
   * window resizing request streams.
   */

  g_signal_connect (G_OBJECT (widget), "size_allocate",
                    G_CALLBACK (gtk_gl_widget_size_allocate),
                    private);

  return TRUE;
}

/**
 * gtk_widget_is_gl_capable:
 * @widget: a #GtkWidget.
 *
 * Returns whether the @widget is OpenGL-capable.
 *
 * Return value: TRUE if the @widget is OpenGL-capable, FALSE otherwise.
 **/
gboolean
gtk_widget_is_gl_capable (GtkWidget *widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  return (g_object_get_qdata (G_OBJECT (widget), quark_gl_private) != NULL);
}

/**
 * gtk_widget_get_gl_config:
 * @widget: a #GtkWidget.
 *
 * Returns the #GdkGLConfig referred by the @widget.
 *
 * Return value: the #GdkGLConfig.
 **/
GdkGLConfig *
gtk_widget_get_gl_config (GtkWidget *widget)
{
  GLWidgetPrivate *private;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  private = g_object_get_qdata (G_OBJECT (widget), quark_gl_private);
  if (private == NULL)
    return NULL;

  return private->glconfig;
}

/**
 * gtk_widget_create_gl_context:
 * @widget: a #GtkWidget.
 * @share_list: the #GdkGLContext with which to share display lists and texture
 *              objects. NULL indicates that no sharing is to take place.
 * @direct: whether rendering is to be done with a direct connection to
 *          the graphics system.
 * @render_type: GDK_GL_RGBA_TYPE or GDK_GL_COLOR_INDEX_TYPE (currently not
 *               used).
 *
 * Creates a new #GdkGLContext with the appropriate #GdkGLDrawable
 * for this widget. The GL context must be freed when you're
 * finished with it. See also gtk_widget_get_gl_context().
 *
 * Return value: the new #GdkGLContext.
 **/
GdkGLContext *
gtk_widget_create_gl_context (GtkWidget    *widget,
                              GdkGLContext *share_list,
                              gboolean      direct,
                              int           render_type)
{
  GdkGLDrawable *gldrawable;
  GdkGLContext *glcontext;

  GTK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (GTK_WIDGET_REALIZED (widget), NULL);

  gldrawable = gdk_window_get_gl_drawable (widget->window);
  if (gldrawable == NULL)
    return NULL;

  /*
   * Create OpenGL rendering context.
   */

  glcontext = gdk_gl_context_new (gldrawable,
                                  share_list,
                                  direct,
                                  render_type);
  if (glcontext == NULL)
    {
      g_warning ("cannot create GdkGLContext\n");
      return NULL;
    }

  return glcontext;
}

/**
 * gtk_widget_get_gl_context:
 * @widget: a #GtkWidget.
 *
 * Returns the #GdkGLContext with the appropriate #GdkGLDrawable
 * for this widget. Unlike the GL context returned by
 * gtk_widget_create_gl_context(),  this context is owned by the widget.
 *
 * #GdkGLContext is needed for the function gdk_gl_drawable_begin,
 * or for sharing display lists (see gtk_widget_set_gl_capability()).
 *
 * Return value: the #GdkGLContext.
 **/
GdkGLContext *
gtk_widget_get_gl_context (GtkWidget *widget)
{
  GLWidgetPrivate *private;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (GTK_WIDGET_REALIZED (widget), NULL);

  private = g_object_get_qdata (G_OBJECT (widget), quark_gl_private);
  if (private == NULL)
    return NULL;

  if (private->glcontext == NULL)
    private->glcontext = gtk_widget_create_gl_context (widget,
                                                       private->share_list,
                                                       private->direct,
                                                       private->render_type);

  return private->glcontext;
}

/**
 * gtk_widget_get_gl_window:
 * @widget: a #GtkWidget.
 *
 * Returns the #GdkGLWindow owned by the @widget.
 *
 * Return value: the #GdkGLWindow.
 **/
GdkGLWindow *
gtk_widget_get_gl_window (GtkWidget *widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (GTK_WIDGET_REALIZED (widget), NULL);

  return gdk_window_get_gl_window (widget->window);
}
