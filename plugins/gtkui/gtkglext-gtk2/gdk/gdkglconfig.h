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

#ifndef __GDK_GL_CONFIG_H__
#define __GDK_GL_CONFIG_H__

#include <gdk/gdkgldefs.h>
#include <gdk/gdkgltypes.h>

#include <gdk/gdkwindow.h>

G_BEGIN_DECLS

/*
 * Display mode bit masks.
 */
typedef enum
{
  GDK_GL_MODE_RGB         = 0,
  GDK_GL_MODE_RGBA        = 0,       /* same as RGB */
  GDK_GL_MODE_INDEX       = 1 << 0,
  GDK_GL_MODE_SINGLE      = 0,
  GDK_GL_MODE_DOUBLE      = 1 << 1,
  GDK_GL_MODE_STEREO      = 1 << 2,
  GDK_GL_MODE_ALPHA       = 1 << 3,
  GDK_GL_MODE_DEPTH       = 1 << 4,
  GDK_GL_MODE_STENCIL     = 1 << 5,
  GDK_GL_MODE_ACCUM       = 1 << 6,
  GDK_GL_MODE_MULTISAMPLE = 1 << 7   /* not supported yet */
} GdkGLConfigMode;

typedef struct _GdkGLConfigClass GdkGLConfigClass;

#define GDK_TYPE_GL_CONFIG              (gdk_gl_config_get_type ())
#define GDK_GL_CONFIG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GL_CONFIG, GdkGLConfig))
#define GDK_GL_CONFIG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_CONFIG, GdkGLConfigClass))
#define GDK_IS_GL_CONFIG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GL_CONFIG))
#define GDK_IS_GL_CONFIG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_CONFIG))
#define GDK_GL_CONFIG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_CONFIG, GdkGLConfigClass))

struct _GdkGLConfig
{
  GObject parent_instance;

  gint layer_plane;

  gint n_aux_buffers;

  gint n_sample_buffers;

  guint is_rgba            : 1;
  guint is_double_buffered : 1;
  guint as_single_mode     : 1;
  guint is_stereo          : 1;
  guint has_alpha          : 1;
  guint has_depth_buffer   : 1;
  guint has_stencil_buffer : 1;
  guint has_accum_buffer   : 1;
};

struct _GdkGLConfigClass
{
  GObjectClass parent_class;
};

GType        gdk_gl_config_get_type               (void);

#ifndef GDK_MULTIHEAD_SAFE
GdkGLConfig *gdk_gl_config_new                    (const int       *attrib_list);
#endif /* GDK_MULTIHEAD_SAFE */

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
GdkGLConfig *gdk_gl_config_new_for_screen         (GdkScreen       *screen,
                                                   const int       *attrib_list);
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

#ifndef GDK_MULTIHEAD_SAFE
GdkGLConfig *gdk_gl_config_new_by_mode            (GdkGLConfigMode  mode);
#endif /* GDK_MULTIHEAD_SAFE */

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
GdkGLConfig *gdk_gl_config_new_by_mode_for_screen (GdkScreen       *screen,
                                                   GdkGLConfigMode  mode);
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

GdkScreen   *gdk_gl_config_get_screen             (GdkGLConfig     *glconfig);

gboolean     gdk_gl_config_get_attrib             (GdkGLConfig     *glconfig,
                                                   int              attribute,
                                                   int             *value);

GdkColormap *gdk_gl_config_get_colormap           (GdkGLConfig     *glconfig);

GdkVisual   *gdk_gl_config_get_visual             (GdkGLConfig     *glconfig);

gint         gdk_gl_config_get_depth              (GdkGLConfig     *glconfig);

gint         gdk_gl_config_get_layer_plane        (GdkGLConfig     *glconfig);

gint         gdk_gl_config_get_n_aux_buffers      (GdkGLConfig     *glconfig);

gint         gdk_gl_config_get_n_sample_buffers   (GdkGLConfig     *glconfig);

gboolean     gdk_gl_config_is_rgba                (GdkGLConfig     *glconfig);

gboolean     gdk_gl_config_is_double_buffered     (GdkGLConfig     *glconfig);

gboolean     gdk_gl_config_is_stereo              (GdkGLConfig     *glconfig);

gboolean     gdk_gl_config_has_alpha              (GdkGLConfig     *glconfig);

gboolean     gdk_gl_config_has_depth_buffer       (GdkGLConfig     *glconfig);

gboolean     gdk_gl_config_has_stencil_buffer     (GdkGLConfig     *glconfig);

gboolean     gdk_gl_config_has_accum_buffer       (GdkGLConfig     *glconfig);

G_END_DECLS

#endif /* __GDK_GL_CONFIG_H__ */
