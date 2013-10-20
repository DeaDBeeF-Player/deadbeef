
/* Generated data (by glib-mkenums) */

#include "gdkgl.h"

/* enumerations from "gdkgltokens.h" */
GType
gdk_gl_config_attrib_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_USE_GL, "GDK_GL_USE_GL", "use-gl" },
      { GDK_GL_BUFFER_SIZE, "GDK_GL_BUFFER_SIZE", "buffer-size" },
      { GDK_GL_LEVEL, "GDK_GL_LEVEL", "level" },
      { GDK_GL_RGBA, "GDK_GL_RGBA", "rgba" },
      { GDK_GL_DOUBLEBUFFER, "GDK_GL_DOUBLEBUFFER", "doublebuffer" },
      { GDK_GL_STEREO, "GDK_GL_STEREO", "stereo" },
      { GDK_GL_AUX_BUFFERS, "GDK_GL_AUX_BUFFERS", "aux-buffers" },
      { GDK_GL_RED_SIZE, "GDK_GL_RED_SIZE", "red-size" },
      { GDK_GL_GREEN_SIZE, "GDK_GL_GREEN_SIZE", "green-size" },
      { GDK_GL_BLUE_SIZE, "GDK_GL_BLUE_SIZE", "blue-size" },
      { GDK_GL_ALPHA_SIZE, "GDK_GL_ALPHA_SIZE", "alpha-size" },
      { GDK_GL_DEPTH_SIZE, "GDK_GL_DEPTH_SIZE", "depth-size" },
      { GDK_GL_STENCIL_SIZE, "GDK_GL_STENCIL_SIZE", "stencil-size" },
      { GDK_GL_ACCUM_RED_SIZE, "GDK_GL_ACCUM_RED_SIZE", "accum-red-size" },
      { GDK_GL_ACCUM_GREEN_SIZE, "GDK_GL_ACCUM_GREEN_SIZE", "accum-green-size" },
      { GDK_GL_ACCUM_BLUE_SIZE, "GDK_GL_ACCUM_BLUE_SIZE", "accum-blue-size" },
      { GDK_GL_ACCUM_ALPHA_SIZE, "GDK_GL_ACCUM_ALPHA_SIZE", "accum-alpha-size" },
      { GDK_GL_CONFIG_CAVEAT, "GDK_GL_CONFIG_CAVEAT", "config-caveat" },
      { GDK_GL_X_VISUAL_TYPE, "GDK_GL_X_VISUAL_TYPE", "x-visual-type" },
      { GDK_GL_TRANSPARENT_TYPE, "GDK_GL_TRANSPARENT_TYPE", "transparent-type" },
      { GDK_GL_TRANSPARENT_INDEX_VALUE, "GDK_GL_TRANSPARENT_INDEX_VALUE", "transparent-index-value" },
      { GDK_GL_TRANSPARENT_RED_VALUE, "GDK_GL_TRANSPARENT_RED_VALUE", "transparent-red-value" },
      { GDK_GL_TRANSPARENT_GREEN_VALUE, "GDK_GL_TRANSPARENT_GREEN_VALUE", "transparent-green-value" },
      { GDK_GL_TRANSPARENT_BLUE_VALUE, "GDK_GL_TRANSPARENT_BLUE_VALUE", "transparent-blue-value" },
      { GDK_GL_TRANSPARENT_ALPHA_VALUE, "GDK_GL_TRANSPARENT_ALPHA_VALUE", "transparent-alpha-value" },
      { GDK_GL_DRAWABLE_TYPE, "GDK_GL_DRAWABLE_TYPE", "drawable-type" },
      { GDK_GL_RENDER_TYPE, "GDK_GL_RENDER_TYPE", "render-type" },
      { GDK_GL_X_RENDERABLE, "GDK_GL_X_RENDERABLE", "x-renderable" },
      { GDK_GL_FBCONFIG_ID, "GDK_GL_FBCONFIG_ID", "fbconfig-id" },
      { GDK_GL_MAX_PBUFFER_WIDTH, "GDK_GL_MAX_PBUFFER_WIDTH", "max-pbuffer-width" },
      { GDK_GL_MAX_PBUFFER_HEIGHT, "GDK_GL_MAX_PBUFFER_HEIGHT", "max-pbuffer-height" },
      { GDK_GL_MAX_PBUFFER_PIXELS, "GDK_GL_MAX_PBUFFER_PIXELS", "max-pbuffer-pixels" },
      { GDK_GL_VISUAL_ID, "GDK_GL_VISUAL_ID", "visual-id" },
      { GDK_GL_SCREEN, "GDK_GL_SCREEN", "screen" },
      { GDK_GL_SAMPLE_BUFFERS, "GDK_GL_SAMPLE_BUFFERS", "sample-buffers" },
      { GDK_GL_SAMPLES, "GDK_GL_SAMPLES", "samples" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLConfigAttrib", values);
  }
  return etype;
}
GType
gdk_gl_config_caveat_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_CONFIG_CAVEAT_DONT_CARE, "GDK_GL_CONFIG_CAVEAT_DONT_CARE", "config-caveat-dont-care" },
      { GDK_GL_CONFIG_CAVEAT_NONE, "GDK_GL_CONFIG_CAVEAT_NONE", "config-caveat-none" },
      { GDK_GL_SLOW_CONFIG, "GDK_GL_SLOW_CONFIG", "slow-config" },
      { GDK_GL_NON_CONFORMANT_CONFIG, "GDK_GL_NON_CONFORMANT_CONFIG", "non-conformant-config" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLConfigCaveat", values);
  }
  return etype;
}
GType
gdk_gl_visual_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_VISUAL_TYPE_DONT_CARE, "GDK_GL_VISUAL_TYPE_DONT_CARE", "visual-type-dont-care" },
      { GDK_GL_TRUE_COLOR, "GDK_GL_TRUE_COLOR", "true-color" },
      { GDK_GL_DIRECT_COLOR, "GDK_GL_DIRECT_COLOR", "direct-color" },
      { GDK_GL_PSEUDO_COLOR, "GDK_GL_PSEUDO_COLOR", "pseudo-color" },
      { GDK_GL_STATIC_COLOR, "GDK_GL_STATIC_COLOR", "static-color" },
      { GDK_GL_GRAY_SCALE, "GDK_GL_GRAY_SCALE", "gray-scale" },
      { GDK_GL_STATIC_GRAY, "GDK_GL_STATIC_GRAY", "static-gray" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLVisualType", values);
  }
  return etype;
}
GType
gdk_gl_transparent_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_TRANSPARENT_NONE, "GDK_GL_TRANSPARENT_NONE", "none" },
      { GDK_GL_TRANSPARENT_RGB, "GDK_GL_TRANSPARENT_RGB", "rgb" },
      { GDK_GL_TRANSPARENT_INDEX, "GDK_GL_TRANSPARENT_INDEX", "index" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLTransparentType", values);
  }
  return etype;
}
GType
gdk_gl_drawable_type_mask_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { GDK_GL_WINDOW_BIT, "GDK_GL_WINDOW_BIT", "window-bit" },
      { GDK_GL_PIXMAP_BIT, "GDK_GL_PIXMAP_BIT", "pixmap-bit" },
      { GDK_GL_PBUFFER_BIT, "GDK_GL_PBUFFER_BIT", "pbuffer-bit" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("GdkGLDrawableTypeMask", values);
  }
  return etype;
}
GType
gdk_gl_render_type_mask_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { GDK_GL_RGBA_BIT, "GDK_GL_RGBA_BIT", "rgba-bit" },
      { GDK_GL_COLOR_INDEX_BIT, "GDK_GL_COLOR_INDEX_BIT", "color-index-bit" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("GdkGLRenderTypeMask", values);
  }
  return etype;
}
GType
gdk_gl_buffer_mask_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { GDK_GL_FRONT_LEFT_BUFFER_BIT, "GDK_GL_FRONT_LEFT_BUFFER_BIT", "front-left-buffer-bit" },
      { GDK_GL_FRONT_RIGHT_BUFFER_BIT, "GDK_GL_FRONT_RIGHT_BUFFER_BIT", "front-right-buffer-bit" },
      { GDK_GL_BACK_LEFT_BUFFER_BIT, "GDK_GL_BACK_LEFT_BUFFER_BIT", "back-left-buffer-bit" },
      { GDK_GL_BACK_RIGHT_BUFFER_BIT, "GDK_GL_BACK_RIGHT_BUFFER_BIT", "back-right-buffer-bit" },
      { GDK_GL_AUX_BUFFERS_BIT, "GDK_GL_AUX_BUFFERS_BIT", "aux-buffers-bit" },
      { GDK_GL_DEPTH_BUFFER_BIT, "GDK_GL_DEPTH_BUFFER_BIT", "depth-buffer-bit" },
      { GDK_GL_STENCIL_BUFFER_BIT, "GDK_GL_STENCIL_BUFFER_BIT", "stencil-buffer-bit" },
      { GDK_GL_ACCUM_BUFFER_BIT, "GDK_GL_ACCUM_BUFFER_BIT", "accum-buffer-bit" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("GdkGLBufferMask", values);
  }
  return etype;
}
GType
gdk_gl_config_error_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_BAD_SCREEN, "GDK_GL_BAD_SCREEN", "bad-screen" },
      { GDK_GL_BAD_ATTRIBUTE, "GDK_GL_BAD_ATTRIBUTE", "bad-attribute" },
      { GDK_GL_NO_EXTENSION, "GDK_GL_NO_EXTENSION", "no-extension" },
      { GDK_GL_BAD_VISUAL, "GDK_GL_BAD_VISUAL", "bad-visual" },
      { GDK_GL_BAD_CONTEXT, "GDK_GL_BAD_CONTEXT", "bad-context" },
      { GDK_GL_BAD_VALUE, "GDK_GL_BAD_VALUE", "bad-value" },
      { GDK_GL_BAD_ENUM, "GDK_GL_BAD_ENUM", "bad-enum" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLConfigError", values);
  }
  return etype;
}
GType
gdk_gl_render_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_RGBA_TYPE, "GDK_GL_RGBA_TYPE", "rgba-type" },
      { GDK_GL_COLOR_INDEX_TYPE, "GDK_GL_COLOR_INDEX_TYPE", "color-index-type" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLRenderType", values);
  }
  return etype;
}
GType
gdk_gl_drawable_attrib_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_PRESERVED_CONTENTS, "GDK_GL_PRESERVED_CONTENTS", "preserved-contents" },
      { GDK_GL_LARGEST_PBUFFER, "GDK_GL_LARGEST_PBUFFER", "largest-pbuffer" },
      { GDK_GL_WIDTH, "GDK_GL_WIDTH", "width" },
      { GDK_GL_HEIGHT, "GDK_GL_HEIGHT", "height" },
      { GDK_GL_EVENT_MASK, "GDK_GL_EVENT_MASK", "event-mask" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLDrawableAttrib", values);
  }
  return etype;
}
GType
gdk_gl_pbuffer_attrib_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_PBUFFER_PRESERVED_CONTENTS, "GDK_GL_PBUFFER_PRESERVED_CONTENTS", "preserved-contents" },
      { GDK_GL_PBUFFER_LARGEST_PBUFFER, "GDK_GL_PBUFFER_LARGEST_PBUFFER", "largest-pbuffer" },
      { GDK_GL_PBUFFER_HEIGHT, "GDK_GL_PBUFFER_HEIGHT", "height" },
      { GDK_GL_PBUFFER_WIDTH, "GDK_GL_PBUFFER_WIDTH", "width" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLPbufferAttrib", values);
  }
  return etype;
}
GType
gdk_gl_event_mask_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { GDK_GL_PBUFFER_CLOBBER_MASK, "GDK_GL_PBUFFER_CLOBBER_MASK", "mask" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("GdkGLEventMask", values);
  }
  return etype;
}
GType
gdk_gl_event_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_DAMAGED, "GDK_GL_DAMAGED", "damaged" },
      { GDK_GL_SAVED, "GDK_GL_SAVED", "saved" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLEventType", values);
  }
  return etype;
}
GType
gdk_gl_drawable_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GDK_GL_WINDOW, "GDK_GL_WINDOW", "window" },
      { GDK_GL_PBUFFER, "GDK_GL_PBUFFER", "pbuffer" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GdkGLDrawableType", values);
  }
  return etype;
}

/* enumerations from "gdkglconfig.h" */
GType
gdk_gl_config_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { GDK_GL_MODE_RGB, "GDK_GL_MODE_RGB", "rgb" },
      { GDK_GL_MODE_RGBA, "GDK_GL_MODE_RGBA", "rgba" },
      { GDK_GL_MODE_INDEX, "GDK_GL_MODE_INDEX", "index" },
      { GDK_GL_MODE_SINGLE, "GDK_GL_MODE_SINGLE", "single" },
      { GDK_GL_MODE_DOUBLE, "GDK_GL_MODE_DOUBLE", "double" },
      { GDK_GL_MODE_STEREO, "GDK_GL_MODE_STEREO", "stereo" },
      { GDK_GL_MODE_ALPHA, "GDK_GL_MODE_ALPHA", "alpha" },
      { GDK_GL_MODE_DEPTH, "GDK_GL_MODE_DEPTH", "depth" },
      { GDK_GL_MODE_STENCIL, "GDK_GL_MODE_STENCIL", "stencil" },
      { GDK_GL_MODE_ACCUM, "GDK_GL_MODE_ACCUM", "accum" },
      { GDK_GL_MODE_MULTISAMPLE, "GDK_GL_MODE_MULTISAMPLE", "multisample" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("GdkGLConfigMode", values);
  }
  return etype;
}

/* Generated data ends here */

