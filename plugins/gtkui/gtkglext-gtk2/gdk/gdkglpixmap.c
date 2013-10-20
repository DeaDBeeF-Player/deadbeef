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

#include <gtk/gtkversion.h>

#include "gdkglprivate.h"
#include "gdkglconfig.h"
#include "gdkglpixmap.h"

static GdkGC *gdk_gl_pixmap_create_gc      (GdkDrawable      *drawable,
                                            GdkGCValues      *values,
                                            GdkGCValuesMask   mask);
static void   gdk_gl_pixmap_draw_rectangle (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            gint              filled,
                                            gint              x,
                                            gint              y,
                                            gint              width,
                                            gint              height);
static void   gdk_gl_pixmap_draw_arc       (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            gint              filled,
                                            gint              x,
                                            gint              y,
                                            gint              width,
                                            gint              height,
                                            gint              angle1,
                                            gint              angle2);
static void   gdk_gl_pixmap_draw_polygon   (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            gint              filled,
                                            GdkPoint         *points,
                                            gint              npoints);
static void   gdk_gl_pixmap_draw_text      (GdkDrawable      *drawable,
                                            GdkFont          *font,
                                            GdkGC            *gc,
                                            gint              x,
                                            gint              y,
                                            const gchar      *text,
                                            gint              text_length);
static void   gdk_gl_pixmap_draw_text_wc   (GdkDrawable      *drawable,
                                            GdkFont          *font,
                                            GdkGC            *gc,
                                            gint              x,
                                            gint              y,
                                            const GdkWChar   *text,
                                            gint              text_length);
static void   gdk_gl_pixmap_draw_drawable  (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            GdkDrawable      *src,
                                            gint              xsrc,
                                            gint              ysrc,
                                            gint              xdest,
                                            gint              ydest,
                                            gint              width,
                                            gint              height);
static void   gdk_gl_pixmap_draw_points	   (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            GdkPoint         *points,
                                            gint              npoints);
static void   gdk_gl_pixmap_draw_segments  (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            GdkSegment       *segs,
                                            gint              nsegs);
static void   gdk_gl_pixmap_draw_lines     (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            GdkPoint         *points,
                                            gint              npoints);
static void   gdk_gl_pixmap_draw_glyphs    (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            PangoFont        *font,
                                            gint              x,
                                            gint              y,
                                            PangoGlyphString *glyphs);
static void   gdk_gl_pixmap_draw_image     (GdkDrawable      *drawable,
                                            GdkGC            *gc,
                                            GdkImage         *image,
                                            gint              xsrc,
                                            gint              ysrc,
                                            gint              xdest,
                                            gint              ydest,
                                            gint              width,
                                            gint              height);
static gint         gdk_gl_pixmap_get_depth              (GdkDrawable *drawable);
static void         gdk_gl_pixmap_get_size               (GdkDrawable *drawable,
                                                          gint        *width,
                                                          gint        *height);
static void         gdk_gl_pixmap_set_colormap           (GdkDrawable *drawable,
                                                          GdkColormap *cmap);
static GdkColormap *gdk_gl_pixmap_get_colormap           (GdkDrawable *drawable);
static GdkVisual   *gdk_gl_pixmap_get_visual             (GdkDrawable *drawable);
#if !(GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION == 0)
static GdkScreen   *gdk_gl_pixmap_get_screen             (GdkDrawable *drawable);
#endif
static GdkImage    *gdk_gl_pixmap_get_image              (GdkDrawable *drawable,
                                                          gint         x,
                                                          gint         y,
                                                          gint         width,
                                                          gint         height);
static GdkRegion   *gdk_gl_pixmap_get_clip_region        (GdkDrawable *drawable);
static GdkRegion   *gdk_gl_pixmap_get_visible_region     (GdkDrawable *drawable);
static GdkDrawable *gdk_gl_pixmap_get_composite_drawable (GdkDrawable *drawable,
                                                          gint         x,
                                                          gint         y,
                                                          gint         width,
                                                          gint         height,
                                                          gint        *composite_x_offset,
                                                          gint        *composite_y_offset);
static void         gdk_gl_pixmap_draw_pixbuf   (GdkDrawable *drawable,
                                                 GdkGC       *gc,
                                                 GdkPixbuf   *pixbuf,
                                                 gint         src_x,
                                                 gint         src_y,
                                                 gint         dest_x,
                                                 gint         dest_y,
                                                 gint         width,
                                                 gint         height,
                                                 GdkRgbDither dither,
                                                 gint         x_dither,
                                                 gint         y_dither);
static GdkImage    *gdk_gl_pixmap_copy_to_image (GdkDrawable *drawable,
                                                 GdkImage    *image,
                                                 gint         src_x,
                                                 gint         src_y,
                                                 gint         dest_x,
                                                 gint         dest_y,
                                                 gint         width,
                                                 gint         height);

static void gdk_gl_pixmap_class_init (GdkGLPixmapClass *klass);
static void gdk_gl_pixmap_finalize   (GObject          *object);

static gpointer parent_class = NULL;

GType
gdk_gl_pixmap_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GdkGLPixmapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_gl_pixmap_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                   /* class_data */
        sizeof (GdkGLPixmap),
        0,                      /* n_preallocs */
        (GInstanceInitFunc) NULL
      };

      type = g_type_register_static (GDK_TYPE_DRAWABLE,
                                     "GdkGLPixmap",
                                     &type_info, 0);
    }

  return type;
}

static void
gdk_gl_pixmap_class_init (GdkGLPixmapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkDrawableClass *drawable_class = GDK_DRAWABLE_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_gl_pixmap_finalize;

  drawable_class->create_gc              = gdk_gl_pixmap_create_gc;
  drawable_class->draw_rectangle         = gdk_gl_pixmap_draw_rectangle;
  drawable_class->draw_arc               = gdk_gl_pixmap_draw_arc;
  drawable_class->draw_polygon           = gdk_gl_pixmap_draw_polygon;
  drawable_class->draw_text              = gdk_gl_pixmap_draw_text;
  drawable_class->draw_text_wc           = gdk_gl_pixmap_draw_text_wc;
  drawable_class->draw_drawable          = gdk_gl_pixmap_draw_drawable;
  drawable_class->draw_points            = gdk_gl_pixmap_draw_points;
  drawable_class->draw_segments          = gdk_gl_pixmap_draw_segments;
  drawable_class->draw_lines             = gdk_gl_pixmap_draw_lines;
  drawable_class->draw_glyphs            = gdk_gl_pixmap_draw_glyphs;
  drawable_class->draw_image             = gdk_gl_pixmap_draw_image;
  drawable_class->get_depth              = gdk_gl_pixmap_get_depth;
  drawable_class->get_size               = gdk_gl_pixmap_get_size;
  drawable_class->set_colormap           = gdk_gl_pixmap_set_colormap;
  drawable_class->get_colormap           = gdk_gl_pixmap_get_colormap;
  drawable_class->get_visual             = gdk_gl_pixmap_get_visual;
#if !(GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION == 0)
  drawable_class->get_screen             = gdk_gl_pixmap_get_screen;
#endif
  drawable_class->get_image              = gdk_gl_pixmap_get_image;
  drawable_class->get_clip_region        = gdk_gl_pixmap_get_clip_region;
  drawable_class->get_visible_region     = gdk_gl_pixmap_get_visible_region;
  drawable_class->get_composite_drawable = gdk_gl_pixmap_get_composite_drawable;
#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION == 0
  drawable_class->_draw_pixbuf           = gdk_gl_pixmap_draw_pixbuf;
#else
  drawable_class->draw_pixbuf            = gdk_gl_pixmap_draw_pixbuf;
#endif
  drawable_class->_copy_to_image         = gdk_gl_pixmap_copy_to_image;
}

static void
gdk_gl_pixmap_finalize (GObject *object)
{
  GdkGLPixmap *glpixmap = GDK_GL_PIXMAP (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (glpixmap->drawable != NULL)
    g_object_remove_weak_pointer (G_OBJECT (glpixmap->drawable),
                                  (gpointer *) &(glpixmap->drawable));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GdkGC *
gdk_gl_pixmap_create_gc (GdkDrawable    *drawable,
                         GdkGCValues    *values,
                         GdkGCValuesMask mask)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->create_gc (real_drawable,
                                                            values,
                                                            mask);
}

static void
gdk_gl_pixmap_draw_rectangle (GdkDrawable *drawable,
                              GdkGC	  *gc,
                              gint	   filled,
                              gint	   x,
                              gint	   y,
                              gint	   width,
                              gint	   height)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_rectangle (real_drawable,
                                                          gc,
                                                          filled,
                                                          x,
                                                          y,
                                                          width,
                                                          height);
}

static void
gdk_gl_pixmap_draw_arc (GdkDrawable *drawable,
                        GdkGC	    *gc,
                        gint	     filled,
                        gint	     x,
                        gint	     y,
                        gint	     width,
                        gint	     height,
                        gint	     angle1,
                        gint	     angle2)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_arc (real_drawable,
                                                    gc,
                                                    filled,
                                                    x,
                                                    y,
                                                    width,
                                                    height,
                                                    angle1,
                                                    angle2);
}

static void
gdk_gl_pixmap_draw_polygon (GdkDrawable *drawable,
                            GdkGC	*gc,
                            gint	 filled,
                            GdkPoint    *points,
                            gint	 npoints)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_polygon (real_drawable,
                                                        gc,
                                                        filled,
                                                        points,
                                                        npoints);
}

static void
gdk_gl_pixmap_draw_text (GdkDrawable *drawable,
                         GdkFont     *font,
                         GdkGC	     *gc,
                         gint	      x,
                         gint	      y,
                         const gchar *text,
                         gint	      text_length)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_text (real_drawable,
                                                     font,
                                                     gc,
                                                     x,
                                                     y,
                                                     text,
                                                     text_length);
}

static void
gdk_gl_pixmap_draw_text_wc (GdkDrawable	   *drawable,
                            GdkFont	   *font,
                            GdkGC	   *gc,
                            gint	    x,
                            gint            y,
                            const GdkWChar *text,
                            gint	    text_length)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_text_wc (real_drawable,
                                                        font,
                                                        gc,
                                                        x,
                                                        y,
                                                        text,
                                                        text_length);
}

static void
gdk_gl_pixmap_draw_drawable (GdkDrawable *drawable,
                             GdkGC	 *gc,
                             GdkDrawable *src,
                             gint	  xsrc,
                             gint	  ysrc,
                             gint	  xdest,
                             gint	  ydest,
                             gint	  width,
                             gint	  height)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_drawable (real_drawable,
                                                         gc,
                                                         src,
                                                         xsrc,
                                                         ysrc,
                                                         xdest,
                                                         ydest,
                                                         width,
                                                         height);
}

static void
gdk_gl_pixmap_draw_points (GdkDrawable *drawable,
                           GdkGC       *gc,
                           GdkPoint    *points,
                           gint	        npoints)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_points (real_drawable,
                                                       gc,
                                                       points,
                                                       npoints);
}

static void
gdk_gl_pixmap_draw_segments (GdkDrawable *drawable,
                             GdkGC	 *gc,
                             GdkSegment  *segs,
                             gint         nsegs)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_segments (real_drawable,
                                                         gc,
                                                         segs,
                                                         nsegs);
}

static void
gdk_gl_pixmap_draw_lines (GdkDrawable *drawable,
                          GdkGC       *gc,
                          GdkPoint    *points,
                          gint         npoints)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_lines (real_drawable,
                                                      gc,
                                                      points,
                                                      npoints);
}

static void
gdk_gl_pixmap_draw_glyphs (GdkDrawable      *drawable,
                           GdkGC            *gc,
                           PangoFont        *font,
                           gint              x,
                           gint              y,
                           PangoGlyphString *glyphs)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_glyphs (real_drawable,
                                                       gc,
                                                       font,
                                                       x,
                                                       y,
                                                       glyphs);
}

static void
gdk_gl_pixmap_draw_image (GdkDrawable *drawable,
                          GdkGC	      *gc,
                          GdkImage    *image,
                          gint	       xsrc,
                          gint	       ysrc,
                          gint	       xdest,
                          gint	       ydest,
                          gint	       width,
                          gint	       height)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_image (real_drawable,
                                                      gc,
                                                      image,
                                                      xsrc,
                                                      ysrc,
                                                      xdest,
                                                      ydest,
                                                      width,
                                                      height);
}

static gint
gdk_gl_pixmap_get_depth (GdkDrawable *drawable)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->get_depth (real_drawable);
}

static void
gdk_gl_pixmap_get_size (GdkDrawable *drawable,
                        gint        *width,
                        gint        *height)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->get_size (real_drawable,
                                                    width,
                                                    height);
}

static void
gdk_gl_pixmap_set_colormap (GdkDrawable *drawable,
                            GdkColormap *cmap)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->set_colormap (real_drawable,
                                                        cmap);
}

static GdkColormap *
gdk_gl_pixmap_get_colormap (GdkDrawable *drawable)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->get_colormap (real_drawable);
}

static GdkVisual *
gdk_gl_pixmap_get_visual (GdkDrawable *drawable)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->get_visual (real_drawable);
}

#if !(GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION == 0)

static GdkScreen *
gdk_gl_pixmap_get_screen (GdkDrawable *drawable)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->get_screen (real_drawable);
}

#endif

static GdkImage *
gdk_gl_pixmap_get_image (GdkDrawable *drawable,
                         gint         x,
                         gint         y,
                         gint         width,
                         gint         height)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->get_image (real_drawable,
                                                            x,
                                                            y,
                                                            width,
                                                            height);
}

static GdkRegion *
gdk_gl_pixmap_get_clip_region (GdkDrawable *drawable)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->get_clip_region (real_drawable);
}

static GdkRegion *
gdk_gl_pixmap_get_visible_region (GdkDrawable *drawable)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->get_visible_region (real_drawable);
}

static GdkDrawable *
gdk_gl_pixmap_get_composite_drawable (GdkDrawable *drawable,
                                      gint         x,
                                      gint         y,
                                      gint         width,
                                      gint         height,
                                      gint        *composite_x_offset,
                                      gint        *composite_y_offset)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->get_composite_drawable (real_drawable,
                                                                         x,
                                                                         y,
                                                                         width,
                                                                         height,
                                                                         composite_x_offset,
                                                                         composite_y_offset);
}

static void
gdk_gl_pixmap_draw_pixbuf (GdkDrawable *drawable,
                           GdkGC       *gc,
                           GdkPixbuf   *pixbuf,
                           gint         src_x,
                           gint         src_y,
                           gint         dest_x,
                           gint         dest_y,
                           gint         width,
                           gint         height,
                           GdkRgbDither dither,
                           gint         x_dither,
                           gint         y_dither)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION == 0
  GDK_DRAWABLE_GET_CLASS (real_drawable)->_draw_pixbuf (real_drawable,
                                                        gc,
                                                        pixbuf,
                                                        src_x,
                                                        src_y,
                                                        dest_x,
                                                        dest_y,
                                                        width,
                                                        height,
                                                        dither,
                                                        x_dither,
                                                        y_dither);
#else
  GDK_DRAWABLE_GET_CLASS (real_drawable)->draw_pixbuf (real_drawable,
                                                       gc,
                                                       pixbuf,
                                                       src_x,
                                                       src_y,
                                                       dest_x,
                                                       dest_y,
                                                       width,
                                                       height,
                                                       dither,
                                                       x_dither,
                                                       y_dither);
#endif
}

static GdkImage *
gdk_gl_pixmap_copy_to_image (GdkDrawable *drawable,
                             GdkImage    *image,
                             gint         src_x,
                             gint         src_y,
                             gint         dest_x,
                             gint         dest_y,
                             gint         width,
                             gint         height)
{
  GdkDrawable *real_drawable = ((GdkGLPixmap *) drawable)->drawable;

  return GDK_DRAWABLE_GET_CLASS (real_drawable)->_copy_to_image (real_drawable,
                                                                 image,
                                                                 src_x,
                                                                 src_y,
                                                                 dest_x,
                                                                 dest_y,
                                                                 width,
                                                                 height);
}

/*< private >*/
void
_gdk_gl_pixmap_get_size (GdkGLDrawable *gldrawable,
                         gint          *width,
                         gint          *height)
{
  GdkDrawable *real_drawable;

  g_return_if_fail (GDK_IS_GL_PIXMAP (gldrawable));

  real_drawable = ((GdkGLPixmap *) gldrawable)->drawable;

  GDK_DRAWABLE_GET_CLASS (real_drawable)->get_size (real_drawable,
                                                    width,
                                                    height);
}

/**
 * gdk_gl_pixmap_destroy:
 * @glpixmap: a #GdkGLPixmap.
 *
 * Destroys the OpenGL resources associated with @glpixmap and
 * decrements @glpixmap's reference count.
 **/
void
gdk_gl_pixmap_destroy (GdkGLPixmap *glpixmap)
{
  g_return_if_fail (GDK_IS_GL_PIXMAP (glpixmap));

  _gdk_gl_pixmap_destroy (glpixmap);
  g_object_unref (G_OBJECT (glpixmap));
}

/**
 * gdk_gl_pixmap_get_pixmap:
 * @glpixmap: a #GdkGLPixmap.
 *
 * Returns the #GdkPixmap associated with @glpixmap.
 *
 * Notice that #GdkGLPixmap is not #GdkPixmap, but another
 * #GdkDrawable which have an associated #GdkPixmap.
 *
 * Return value: the #GdkPixmap associated with @glpixmap.
 **/
GdkPixmap *
gdk_gl_pixmap_get_pixmap (GdkGLPixmap *glpixmap)
{
  g_return_val_if_fail (GDK_IS_GL_PIXMAP (glpixmap), NULL);

  return GDK_PIXMAP (glpixmap->drawable);
}

/*
 * OpenGL extension to GdkPixmap
 */

static const gchar quark_gl_pixmap_string[] = "gdk-gl-pixmap-gl-pixmap";
static GQuark quark_gl_pixmap = 0;

/**
 * gdk_pixmap_set_gl_capability:
 * @pixmap: the #GdkPixmap to be used as the rendering area.
 * @glconfig: a #GdkGLConfig.
 * @attrib_list: this must be set to NULL or empty (first attribute of None).
 *
 * Set the OpenGL-capability to the @pixmap.
 * This function creates a new #GdkGLPixmap held by the @pixmap.
 * attrib_list is currently unused. This must be set to NULL or empty
 * (first attribute of None).
 *
 * Return value: the #GdkGLPixmap used by the @pixmap if it is successful,
 *               NULL otherwise.
 **/
GdkGLPixmap *
gdk_pixmap_set_gl_capability (GdkPixmap   *pixmap,
                              GdkGLConfig *glconfig,
                              const int   *attrib_list)
{
  GdkGLPixmap *glpixmap;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_PIXMAP (pixmap), NULL);
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), NULL);

  if (quark_gl_pixmap == 0)
    quark_gl_pixmap = g_quark_from_static_string (quark_gl_pixmap_string);

  /* If already set */
  glpixmap = g_object_get_qdata (G_OBJECT (pixmap), quark_gl_pixmap);
  if (glpixmap != NULL)
    return glpixmap;

  /*
   * Create GdkGLPixmap
   */

  glpixmap = gdk_gl_pixmap_new (glconfig, pixmap, attrib_list);
  if (glpixmap == NULL)
    {
      g_warning ("cannot create GdkGLPixmap\n");
      return NULL;
    }

  g_object_set_qdata_full (G_OBJECT (pixmap), quark_gl_pixmap, glpixmap,
                           (GDestroyNotify) g_object_unref);

  return glpixmap;
}

/**
 * gdk_pixmap_unset_gl_capability:
 * @pixmap: a #GdkPixmap.
 *
 * Unset the OpenGL-capability of the @pixmap.
 * This function destroys the #GdkGLPixmap held by the @pixmap.
 *
 **/
void
gdk_pixmap_unset_gl_capability (GdkPixmap *pixmap)
{
  GdkGLPixmap *glpixmap;

  GDK_GL_NOTE_FUNC ();

  if (quark_gl_pixmap == 0)
    quark_gl_pixmap = g_quark_from_static_string (quark_gl_pixmap_string);

  /*
   * Destroy OpenGL resources explicitly, then unref.
   */

  glpixmap = g_object_get_qdata (G_OBJECT (pixmap), quark_gl_pixmap);
  if (glpixmap == NULL)
    return;

  _gdk_gl_pixmap_destroy (glpixmap);

  g_object_set_qdata (G_OBJECT (pixmap), quark_gl_pixmap, NULL);
}

/**
 * gdk_pixmap_is_gl_capable:
 * @pixmap: a #GdkPixmap.
 *
 * Returns whether the @pixmap is OpenGL-capable.
 *
 * Return value: TRUE if the @pixmap is OpenGL-capable, FALSE otherwise.
 **/
gboolean
gdk_pixmap_is_gl_capable (GdkPixmap *pixmap)
{
  g_return_val_if_fail (GDK_IS_PIXMAP (pixmap), FALSE);

  return g_object_get_qdata (G_OBJECT (pixmap), quark_gl_pixmap) != NULL ? TRUE : FALSE;
}

/**
 * gdk_pixmap_get_gl_pixmap:
 * @pixmap: a #GdkPixmap.
 *
 * Returns the #GdkGLPixmap held by the @pixmap.
 *
 * Return value: the #GdkGLPixmap.
 **/
GdkGLPixmap *
gdk_pixmap_get_gl_pixmap (GdkPixmap *pixmap)
{
  g_return_val_if_fail (GDK_IS_PIXMAP (pixmap), NULL);

  return g_object_get_qdata (G_OBJECT (pixmap), quark_gl_pixmap);
}
