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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdk.h>            /* for gdk_error_trap_(push|pop) () */

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglconfig-x11.h"
#include "gdkglwindow-x11.h"
#include "gdkglcontext-x11.h"

static void          gdk_gl_context_insert (GdkGLContext *glcontext);
static void          gdk_gl_context_remove (GdkGLContext *glcontext);
static GdkGLContext *gdk_gl_context_lookup (GLXContext    glxcontext);

static gboolean       _gdk_x11_gl_context_impl_copy             (GdkGLContext  *glcontext,
                                                                 GdkGLContext  *src,
                                                                 unsigned long  mask);
static GdkGLDrawable* _gdk_x11_gl_context_impl_get_gl_drawable  (GdkGLContext *glcontext);
static GdkGLConfig*   _gdk_x11_gl_context_impl_get_gl_config    (GdkGLContext *glcontext);
static GdkGLContext*  _gdk_x11_gl_context_impl_get_share_list   (GdkGLContext *glcontext);
static gboolean       _gdk_x11_gl_context_impl_is_direct        (GdkGLContext *glcontext);
static int            _gdk_x11_gl_context_impl_get_render_type  (GdkGLContext *glcontext);
static gboolean       _gdk_x11_gl_context_impl_make_current     (GdkGLContext  *glcontext,
                                                                 GdkGLDrawable *draw,
                                                                 GdkGLDrawable *read);
static GLXContext     _gdk_x11_gl_context_impl_get_glxcontext   (GdkGLContext *glcontext);

G_DEFINE_TYPE (GdkGLContextImplX11,             \
               gdk_gl_context_impl_x11,         \
               GDK_TYPE_GL_CONTEXT_IMPL)

static void
gdk_gl_context_impl_x11_init (GdkGLContextImplX11 *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  self->glxcontext = NULL;
  self->share_list = NULL;
  self->is_direct = FALSE;
  self->render_type = 0;
  self->glconfig = NULL;
  self->gldrawable = NULL;
  self->gldrawable_read = NULL;
  self->is_destroyed = 0;
  self->is_foreign = 0;
}

void
_gdk_gl_context_destroy (GdkGLContext *glcontext)
{
  GdkGLContextImplX11 *impl = GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl);
  Display *xdisplay;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->is_destroyed)
    return;

  gdk_gl_context_remove (glcontext);

  xdisplay = GDK_GL_CONFIG_XDISPLAY (impl->glconfig);

  if (impl->glxcontext == glXGetCurrentContext ())
    {
      glXWaitGL ();

      GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");
      glXMakeCurrent (xdisplay, None, NULL);
    }

  if (!impl->is_foreign)
    {
      GDK_GL_NOTE_FUNC_IMPL ("glXDestroyContext");
      glXDestroyContext (xdisplay, impl->glxcontext);
      impl->glxcontext = NULL;
    }

  if (impl->gldrawable != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (impl->gldrawable),
                                    (gpointer *) &(impl->gldrawable));
      impl->gldrawable = NULL;
    }

  /* currently unused. */
  /*
  if (impl->gldrawable_read != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (impl->gldrawable_read),
                                    (gpointer *) &(impl->gldrawable_read));
      impl->gldrawable_read = NULL;
    }
  */

  impl->is_destroyed = TRUE;
}

static void
gdk_gl_context_impl_x11_finalize (GObject *object)
{
  GdkGLContextImplX11 *impl = GDK_GL_CONTEXT_IMPL_X11 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  _gdk_gl_context_destroy (GDK_GL_CONTEXT (object));

  g_object_unref (G_OBJECT (impl->glconfig));

  if (impl->share_list != NULL)
    g_object_unref (G_OBJECT (impl->share_list));

  G_OBJECT_CLASS (gdk_gl_context_impl_x11_parent_class)->finalize (object);
}

static void
gdk_gl_context_impl_x11_class_init (GdkGLContextImplX11Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  klass->get_glxcontext = _gdk_x11_gl_context_impl_get_glxcontext;

  klass->parent_class.copy_gl_context_impl = _gdk_x11_gl_context_impl_copy;
  klass->parent_class.get_gl_drawable = _gdk_x11_gl_context_impl_get_gl_drawable;
  klass->parent_class.get_gl_config   = _gdk_x11_gl_context_impl_get_gl_config;
  klass->parent_class.get_share_list  = _gdk_x11_gl_context_impl_get_share_list;
  klass->parent_class.is_direct       = _gdk_x11_gl_context_impl_is_direct;
  klass->parent_class.get_render_type = _gdk_x11_gl_context_impl_get_render_type;
  klass->parent_class.make_current    = _gdk_x11_gl_context_impl_make_current;
  klass->parent_class.make_uncurrent  = NULL;

  object_class->finalize = gdk_gl_context_impl_x11_finalize;
}

static GdkGLContextImpl *
gdk_x11_gl_context_impl_new_common (GdkGLContext  *glcontext,
                                    GdkGLConfig   *glconfig,
                                    GdkGLContext  *share_list,
                                    int            render_type,
                                    GLXContext     glxcontext,
                                    gboolean       is_foreign)
{
  GdkGLContextImpl    *impl;
  GdkGLContextImplX11 *x11_impl;

  Display *xdisplay;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Instantiate the GdkGLContextImplX11 object.
   */

  impl = g_object_new (GDK_TYPE_GL_CONTEXT_IMPL_X11, NULL);
  x11_impl = GDK_GL_CONTEXT_IMPL_X11 (impl);

  x11_impl->glxcontext = glxcontext;

  if (share_list != NULL && GDK_IS_GL_CONTEXT (share_list))
    {
      x11_impl->share_list = share_list;
      g_object_ref (G_OBJECT (x11_impl->share_list));
    }
  else
    {
      x11_impl->share_list = NULL;
    }

  xdisplay = GDK_GL_CONFIG_XDISPLAY (glconfig);
  x11_impl->is_direct = glXIsDirect (xdisplay, glxcontext) ? TRUE : FALSE;

  x11_impl->render_type = render_type;

  x11_impl->glconfig = glconfig;
  g_object_ref (G_OBJECT (x11_impl->glconfig));

  x11_impl->gldrawable = NULL;
  x11_impl->gldrawable_read = NULL;

  x11_impl->is_foreign = is_foreign;

  x11_impl->is_destroyed = FALSE;

  glcontext->impl = impl;

  /*
   * Insert into the GL context hash table.
   */

  gdk_gl_context_insert (glcontext);

  return impl;
}

/*< private >*/
GdkGLContextImpl *
_gdk_x11_gl_context_impl_new (GdkGLContext  *glcontext,
                              GdkGLDrawable *gldrawable,
                              GdkGLContext  *share_list,
                              gboolean       direct,
                              int            render_type)
{
  GdkGLConfig *glconfig;
  GdkGLContextImplX11 *share_impl = NULL;
  GLXContext share_glxcontext = NULL;

  Display *xdisplay;
  XVisualInfo *xvinfo;
  GLXContext glxcontext;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  /*
   * Create an OpenGL rendering context.
   */

  glconfig = gdk_gl_drawable_get_gl_config (gldrawable);

  xdisplay = GDK_GL_CONFIG_XDISPLAY (glconfig);
  xvinfo = GDK_GL_CONFIG_XVINFO (glconfig);

  if (share_list != NULL && GDK_IS_GL_CONTEXT (share_list))
    {
      share_impl = GDK_GL_CONTEXT_IMPL_X11 (share_list->impl);
      share_glxcontext = share_impl->glxcontext;
    }

  GDK_GL_NOTE_FUNC_IMPL ("glXCreateContext");

  if (_gdk_gl_context_force_indirect)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Force indirect"));

      glxcontext = glXCreateContext (xdisplay,
                                     xvinfo,
                                     share_glxcontext,
                                     False);
    }
  else
    {
      glxcontext = glXCreateContext (xdisplay,
                                     xvinfo,
                                     share_glxcontext,
                                     (direct == TRUE) ? True : False);
    }
  if (glxcontext == NULL)
    return NULL;

  GDK_GL_NOTE (MISC,
    g_message (" -- Context: screen number = %d", xvinfo->screen));
  GDK_GL_NOTE (MISC,
    g_message (" -- Context: visual id = 0x%lx", xvinfo->visualid));

  /*
   * Instantiate the GdkGLContextImplX11 object.
   */

  return gdk_x11_gl_context_impl_new_common (glcontext,
                                             glconfig,
                                             share_list,
                                             render_type,
                                             glxcontext,
                                             FALSE);
}

GdkGLContextImpl *
_gdk_x11_gl_context_impl_new_from_glxcontext (GdkGLContext *glcontext,
                                              GdkGLConfig  *glconfig,
                                              GdkGLContext *share_list,
                                              GLXContext    glxcontext)
{
  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);
  g_return_val_if_fail (glxcontext != NULL, NULL);

  /*
   * Instantiate the GdkGLContextImplX11 object.
   */

  return gdk_x11_gl_context_impl_new_common (glcontext,
                                             glconfig,
                                             share_list,
                                             GDK_GL_RGBA_TYPE,
                                             glxcontext,
                                             TRUE);
}

static gboolean
_gdk_x11_gl_context_impl_copy (GdkGLContext  *glcontext,
                               GdkGLContext  *src,
                               unsigned long  mask)
{
  GLXContext dst_glxcontext, src_glxcontext;
  GdkGLConfig *glconfig;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), FALSE);
  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (src), FALSE);

  dst_glxcontext = GDK_GL_CONTEXT_GLXCONTEXT (glcontext);
  if (dst_glxcontext == NULL)
    return FALSE;

  src_glxcontext = GDK_GL_CONTEXT_GLXCONTEXT (src);
  if (src_glxcontext == NULL)
    return FALSE;

  glconfig = GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl)->glconfig;

  gdk_error_trap_push ();

  glXCopyContext (GDK_GL_CONFIG_XDISPLAY (glconfig),
                  src_glxcontext, dst_glxcontext,
                  mask);

  return gdk_error_trap_pop () == Success;
}

/*< private >*/
void
_gdk_x11_gl_context_impl_set_gl_drawable (GdkGLContext  *glcontext,
                                          GdkGLDrawable *gldrawable)
{
  GdkGLContextImplX11 *impl = GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->gldrawable == gldrawable)
    return;

  if (impl->gldrawable != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (impl->gldrawable),
                                    (gpointer *) &(impl->gldrawable));
      impl->gldrawable = NULL;
    }

  if (gldrawable != NULL && GDK_IS_GL_DRAWABLE (gldrawable))
    {
      impl->gldrawable = gldrawable;
      g_object_add_weak_pointer (G_OBJECT (impl->gldrawable),
                                 (gpointer *) &(impl->gldrawable));
    }
}

/*< private >*/
/* currently unused. */
/*
void
_gdk_gl_context_set_gl_drawable_read (GdkGLContext  *glcontext,
                                      GdkGLDrawable *gldrawable_read)
{
  GdkGLContextImplX11 *impl = GDK_GL_CONTEXT_IMPL_X11 (glcontext);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->gldrawable_read == gldrawable_read)
    return;

  if (impl->gldrawable_read != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (impl->gldrawable_read),
                                    (gpointer *) &(impl->gldrawable_read));
      impl->gldrawable_read = NULL;
    }

  if (gldrawable_read != NULL && GDK_IS_GL_DRAWABLE (gldrawable_read))
    {
      impl->gldrawable_read = gldrawable_read;
      g_object_add_weak_pointer (G_OBJECT (impl->gldrawable_read),
                                 (gpointer *) &(impl->gldrawable_read));
    }
}
*/

static GdkGLDrawable *
_gdk_x11_gl_context_impl_get_gl_drawable (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), NULL);

  return GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl)->gldrawable;
}

static GdkGLConfig *
_gdk_x11_gl_context_impl_get_gl_config (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), NULL);

  return GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl)->glconfig;
}

static GdkGLContext *
_gdk_x11_gl_context_impl_get_share_list (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), NULL);

  return GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl)->share_list;
}

static gboolean
_gdk_x11_gl_context_impl_is_direct (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), FALSE);

  return GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl)->is_direct;
}

static int
_gdk_x11_gl_context_impl_get_render_type (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), 0);

  return GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl)->render_type;
}

static gboolean
_gdk_x11_gl_context_impl_make_current (GdkGLContext  *glcontext,
                                       GdkGLDrawable *draw,
                                       GdkGLDrawable *read)
{
  GdkGLWindow *glwindow;
  GdkGLWindowImplX11 *x11_impl;
  GdkGLConfig *glconfig;
  GdkWindow *window;
  Window glxwindow;
  GLXContext glxcontext;

  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), FALSE);
  g_return_val_if_fail (GDK_IS_X11_GL_WINDOW (draw), FALSE);

  glwindow = GDK_GL_WINDOW(draw);
  x11_impl = GDK_GL_WINDOW_IMPL_X11 (glwindow->impl);
  glconfig = x11_impl->glconfig;
  window = gdk_gl_window_get_window(glwindow);
  glxwindow = x11_impl->glxwindow;
  glxcontext = GDK_GL_CONTEXT_GLXCONTEXT (glcontext);

  if (glxwindow == None || glxcontext == NULL)
    return FALSE;

  GDK_GL_NOTE (MISC,
    g_message (" -- Window: screen number = %d",
      GDK_SCREEN_XNUMBER (gdk_window_get_screen (window))));
  GDK_GL_NOTE (MISC,
    g_message (" -- Window: visual id = 0x%lx",
      GDK_VISUAL_XVISUAL (gdk_window_get_visual (window))->visualid));

  GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");

  if (!glXMakeCurrent (GDK_GL_CONFIG_XDISPLAY (glconfig), glxwindow, glxcontext))
    {
      g_warning ("glXMakeCurrent() failed");
      _gdk_x11_gl_context_impl_set_gl_drawable (glcontext, NULL);
      /* currently unused. */
      /* _gdk_gl_context_set_gl_drawable_read (glcontext, NULL); */
      return FALSE;
    }

  _gdk_x11_gl_context_impl_set_gl_drawable (glcontext, draw);
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

GdkGLContext *
_gdk_x11_gl_context_impl_get_current (void)
{
  static GdkGLContext *current = NULL;
  GLXContext glxcontext;

  GDK_GL_NOTE_FUNC ();

  glxcontext = glXGetCurrentContext ();

  if (glxcontext == NULL)
    return NULL;

  if (current && GDK_GL_CONTEXT_GLXCONTEXT (current) == glxcontext)
    return current;

  current = gdk_gl_context_lookup (glxcontext);

  return current;
}

GLXContext
_gdk_x11_gl_context_impl_get_glxcontext (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), NULL);

  return GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl)->glxcontext;
}

/*
 * GdkGLContext hash table.
 */

static GHashTable *gl_context_ht = NULL;

static void
gdk_gl_context_insert (GdkGLContext *glcontext)
{
  GdkGLContextImplX11 *impl;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (gl_context_ht == NULL)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Create GL context hash table."));

      /* We do not know the storage type of GLXContext from the GLX
         specification. We assume that it is a pointer as NULL values
         are specified for this type---this is consistent with the SGI
         and Mesa GLX implementations. */
      gl_context_ht = g_hash_table_new (g_direct_hash,
                                        g_direct_equal);
    }

  impl = GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl);

  g_hash_table_insert (gl_context_ht, impl->glxcontext, glcontext);
}

static void
gdk_gl_context_remove (GdkGLContext *glcontext)
{
  GdkGLContextImplX11 *impl;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (gl_context_ht == NULL)
    return;

  impl = GDK_GL_CONTEXT_IMPL_X11 (glcontext->impl);

  g_hash_table_remove (gl_context_ht, impl->glxcontext);

  if (g_hash_table_size (gl_context_ht) == 0)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Destroy GL context hash table."));
      g_hash_table_destroy (gl_context_ht);
      gl_context_ht = NULL;
    }
}

static GdkGLContext *
gdk_gl_context_lookup (GLXContext glxcontext)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (gl_context_ht == NULL)
    return NULL;

  return g_hash_table_lookup (gl_context_ht, glxcontext);
}
