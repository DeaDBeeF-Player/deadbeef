/* GdkGLExt - OpenGL Extension to GDK
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

#include <string.h>

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglconfig-x11.h"
#include "gdkglcontext-x11.h"
#include "gdkglpixmap-x11.h"

static gboolean     gdk_gl_pixmap_impl_x11_make_context_current (GdkGLDrawable *draw,
                                                                 GdkGLDrawable *read,
                                                                 GdkGLContext  *glcontext);
static gboolean     gdk_gl_pixmap_impl_x11_is_double_buffered   (GdkGLDrawable *gldrawable);
static void         gdk_gl_pixmap_impl_x11_swap_buffers         (GdkGLDrawable *gldrawable);
/*
static gboolean     gdk_gl_pixmap_impl_x11_gl_begin             (GdkGLDrawable *draw,
                                                                 GdkGLDrawable *read,
                                                                 GdkGLContext  *glcontext);
*/
static void         gdk_gl_pixmap_impl_x11_gl_end               (GdkGLDrawable *gldrawable);
static GdkGLConfig *gdk_gl_pixmap_impl_x11_get_gl_config        (GdkGLDrawable *gldrawable);

static void gdk_gl_pixmap_impl_x11_class_init (GdkGLPixmapImplX11Class *klass);
static void gdk_gl_pixmap_impl_x11_finalize   (GObject                 *object);
static void gdk_gl_pixmap_impl_x11_gl_drawable_interface_init (GdkGLDrawableClass *iface);

static gpointer parent_class = NULL;

GType
gdk_gl_pixmap_impl_x11_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GdkGLPixmapImplX11Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_gl_pixmap_impl_x11_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                   /* class_data */
        sizeof (GdkGLPixmapImplX11),
        0,                      /* n_preallocs */
        (GInstanceInitFunc) NULL
      };
      static const GInterfaceInfo gl_drawable_interface_info = {
        (GInterfaceInitFunc) gdk_gl_pixmap_impl_x11_gl_drawable_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL                    /* interface_data */
      };

      type = g_type_register_static (GDK_TYPE_GL_PIXMAP,
                                     "GdkGLPixmapImplX11",
                                     &type_info, 0);
      g_type_add_interface_static (type,
                                   GDK_TYPE_GL_DRAWABLE,
                                   &gl_drawable_interface_info);
    }

  return type;
}

static void
gdk_gl_pixmap_impl_x11_class_init (GdkGLPixmapImplX11Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_gl_pixmap_impl_x11_finalize;
}

void
_gdk_gl_pixmap_destroy (GdkGLPixmap *glpixmap)
{
  GdkGLPixmapImplX11 *impl = GDK_GL_PIXMAP_IMPL_X11 (glpixmap);
  Display *xdisplay;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->is_destroyed)
    return;

  xdisplay = GDK_GL_CONFIG_XDISPLAY (impl->glconfig);

  if (impl->glxpixmap == glXGetCurrentDrawable ())
    {
      glXWaitGL ();

      GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");
      glXMakeCurrent (xdisplay, None, NULL);
    }

  GDK_GL_NOTE_FUNC_IMPL ("glXDestroyGLXPixmap");
  glXDestroyGLXPixmap (xdisplay, impl->glxpixmap);

  impl->glxpixmap = None;

  impl->is_destroyed = TRUE;
}

static void
gdk_gl_pixmap_impl_x11_finalize (GObject *object)
{
  GdkGLPixmapImplX11 *impl = GDK_GL_PIXMAP_IMPL_X11 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  _gdk_gl_pixmap_destroy (GDK_GL_PIXMAP (object));

  g_object_unref (G_OBJECT (impl->glconfig));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gdk_gl_pixmap_impl_x11_gl_drawable_interface_init (GdkGLDrawableClass *iface)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  iface->create_new_context   = _gdk_x11_gl_context_new;
  iface->make_context_current =  gdk_gl_pixmap_impl_x11_make_context_current;
  iface->is_double_buffered   =  gdk_gl_pixmap_impl_x11_is_double_buffered;
  iface->swap_buffers         =  gdk_gl_pixmap_impl_x11_swap_buffers;
  iface->wait_gl              = _gdk_gl_drawable_impl_x11_wait_gl;
  iface->wait_gdk             = _gdk_gl_drawable_impl_x11_wait_gdk;
  iface->gl_begin             =  gdk_gl_pixmap_impl_x11_make_context_current;
  iface->gl_end               =  gdk_gl_pixmap_impl_x11_gl_end;
  iface->get_gl_config        =  gdk_gl_pixmap_impl_x11_get_gl_config;
  iface->get_size             = _gdk_gl_pixmap_get_size;
}

/**
 * gdk_gl_pixmap_new:
 * @glconfig: a #GdkGLConfig.
 * @pixmap: the #GdkPixmap to be used as the rendering area.
 * @attrib_list: this must be set to NULL or empty (first attribute of None).
 *
 * Creates an off-screen rendering area.
 * attrib_list is currently unused. This must be set to NULL or empty
 * (first attribute of None). See GLX 1.3 spec.
 *
 * Return value: the new #GdkGLPixmap.
 **/
GdkGLPixmap *
gdk_gl_pixmap_new (GdkGLConfig *glconfig,
                   GdkPixmap   *pixmap,
                   const int   *attrib_list)
{
  GdkGLPixmap *glpixmap;
  GdkGLPixmapImplX11 *impl;

  Display *xdisplay;
  XVisualInfo *xvinfo;
  Pixmap xpixmap;
  GLXPixmap glxpixmap;

  Window root_return;
  int x_return, y_return;
  unsigned int width_return, height_return;
  unsigned int border_width_return;
  unsigned int depth_return;

  GdkGL_GLX_MESA_pixmap_colormap *mesa_ext;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), NULL);
  g_return_val_if_fail (GDK_IS_PIXMAP (pixmap), NULL);

  xdisplay = GDK_GL_CONFIG_XDISPLAY (glconfig);
  xvinfo = GDK_GL_CONFIG_XVINFO (glconfig);

  /*
   * Get X Pixmap.
   */

  xpixmap = GDK_DRAWABLE_XID (GDK_DRAWABLE (pixmap));

  /*
   * Check depth of the X pixmap.
   */

  if (!XGetGeometry (xdisplay, xpixmap,
                     &root_return,
                     &x_return, &y_return,
                     &width_return, &height_return,
                     &border_width_return,
                     &depth_return))
    return NULL;

  if (depth_return != (unsigned int) xvinfo->depth)
    return NULL;

  /*
   * Create GLXPixmap.
   */

  mesa_ext = gdk_gl_get_GLX_MESA_pixmap_colormap (glconfig);
  if (mesa_ext)
    {
      /* If GLX_MESA_pixmap_colormap is supported. */

      GDK_GL_NOTE_FUNC_IMPL ("glXCreateGLXPixmapMESA");

      glxpixmap = mesa_ext->glXCreateGLXPixmapMESA (xdisplay,
                                                    xvinfo,
                                                    xpixmap,
                                                    GDK_GL_CONFIG_XCOLORMAP (glconfig));
    }
  else
    {
      GDK_GL_NOTE_FUNC_IMPL ("glXCreateGLXPixmap");

      glxpixmap = glXCreateGLXPixmap (xdisplay,
                                      xvinfo,
                                      xpixmap);
    }

  if (glxpixmap == None)
    return NULL;

  /*
   * Instantiate the GdkGLPixmapImplX11 object.
   */

  glpixmap = g_object_new (GDK_TYPE_GL_PIXMAP_IMPL_X11, NULL);
  impl = GDK_GL_PIXMAP_IMPL_X11 (glpixmap);

  glpixmap->drawable = GDK_DRAWABLE (pixmap);
  g_object_add_weak_pointer (G_OBJECT (glpixmap->drawable),
                             (gpointer *) &(glpixmap->drawable));

  impl->glxpixmap = glxpixmap;

  impl->glconfig = glconfig;
  g_object_ref (G_OBJECT (impl->glconfig));

  impl->is_destroyed = FALSE;

  return glpixmap;
}

static gboolean
gdk_gl_pixmap_impl_x11_make_context_current (GdkGLDrawable *draw,
                                             GdkGLDrawable *read,
                                             GdkGLContext  *glcontext)
{
  GdkGLConfig *glconfig;
  GLXPixmap glxpixmap;
  GLXContext glxcontext;

  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_X11 (draw), FALSE);
  g_return_val_if_fail (GDK_IS_GL_CONTEXT_IMPL_X11 (glcontext), FALSE);

  glconfig = GDK_GL_PIXMAP_IMPL_X11 (draw)->glconfig;
  glxpixmap = GDK_GL_PIXMAP_IMPL_X11 (draw)->glxpixmap;
  glxcontext = GDK_GL_CONTEXT_GLXCONTEXT (glcontext);

  if (glxpixmap == None || glxcontext == NULL)
    return FALSE;

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  GDK_GL_NOTE (MISC,
    g_message (" -- Pixmap: screen number = %d",
      GDK_SCREEN_XNUMBER (gdk_drawable_get_screen (GDK_DRAWABLE (draw)))));
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */
  GDK_GL_NOTE (MISC,
    g_message (" -- Pixmap: visual id = 0x%lx",
      GDK_VISUAL_XVISUAL (gdk_drawable_get_visual (GDK_DRAWABLE (draw)))->visualid));

  GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");

  if (!glXMakeCurrent (GDK_GL_CONFIG_XDISPLAY (glconfig), glxpixmap, glxcontext))
    {
      g_warning ("glXMakeCurrent() failed");
      _gdk_gl_context_set_gl_drawable (glcontext, NULL);
      /* currently unused. */
      /* _gdk_gl_context_set_gl_drawable_read (glcontext, NULL); */
      return FALSE;
    }

  _gdk_gl_context_set_gl_drawable (glcontext, draw);
  /* currently unused. */
  /* _gdk_gl_context_set_gl_drawable_read (glcontext, read); */

  if (_GDK_GL_CONFIG_AS_SINGLE_MODE (glconfig))
    {
      /* We do this because we are treating a double-buffered frame
         buffer as a single-buffered frame buffer because the system
         does not appear to export any suitable single-buffered
         visuals (in which the following are necessary). */
      glDrawBuffer (GL_FRONT);
      glReadBuffer (GL_FRONT);
    }

  GDK_GL_NOTE (MISC, _gdk_gl_print_gl_info ());

  return TRUE;
}

static gboolean
gdk_gl_pixmap_impl_x11_is_double_buffered (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_X11 (gldrawable), FALSE);

  return gdk_gl_config_is_double_buffered (GDK_GL_PIXMAP_IMPL_X11 (gldrawable)->glconfig);
}

static void
gdk_gl_pixmap_impl_x11_swap_buffers (GdkGLDrawable *gldrawable)
{
  Display *xdisplay;
  GLXPixmap glxpixmap;

  g_return_if_fail (GDK_IS_GL_PIXMAP_IMPL_X11 (gldrawable));

  xdisplay = GDK_GL_CONFIG_XDISPLAY (GDK_GL_PIXMAP_IMPL_X11 (gldrawable)->glconfig);
  glxpixmap = GDK_GL_PIXMAP_IMPL_X11 (gldrawable)->glxpixmap;

  if (glxpixmap == None)
    return;

  GDK_GL_NOTE_FUNC_IMPL ("glXSwapBuffers");

  glXSwapBuffers (xdisplay, glxpixmap);
}

/*
static gboolean
gdk_gl_pixmap_impl_x11_gl_begin (GdkGLDrawable *draw,
                                 GdkGLDrawable *read,
                                 GdkGLContext  *glcontext)
{
  return gdk_gl_pixmap_impl_x11_make_context_current (draw, read, glcontext);
}
*/

static void
gdk_gl_pixmap_impl_x11_gl_end (GdkGLDrawable *gldrawable)
{
  /* do nothing */
}

static GdkGLConfig *
gdk_gl_pixmap_impl_x11_get_gl_config (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_X11 (gldrawable), NULL);

  return GDK_GL_PIXMAP_IMPL_X11 (gldrawable)->glconfig;
}

/**
 * gdk_x11_gl_pixmap_get_glxpixmap:
 * @glpixmap: a #GdkGLPixmap.
 *
 * Gets GLXPixmap.
 *
 * Return value: the GLXPixmap.
 **/
GLXPixmap
gdk_x11_gl_pixmap_get_glxpixmap (GdkGLPixmap *glpixmap)
{
  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_X11 (glpixmap), None);

  return GDK_GL_PIXMAP_IMPL_X11 (glpixmap)->glxpixmap;
}
