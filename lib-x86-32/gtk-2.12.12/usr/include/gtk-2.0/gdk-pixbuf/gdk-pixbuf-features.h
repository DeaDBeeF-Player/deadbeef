#ifndef GDK_PIXBUF_FEATURES_H
#define GDK_PIXBUF_FEATURES_H 1

#define GDK_PIXBUF_MAJOR (2)
#define GDK_PIXBUF_MINOR (12)
#define GDK_PIXBUF_MICRO (12)
#define GDK_PIXBUF_VERSION "2.12.12"

/* We prefix variable declarations so they can
 * properly get exported/imported from Windows DLLs.
 */
#ifndef GDK_PIXBUF_VAR
#  ifdef G_PLATFORM_WIN32
#    ifdef GDK_PIXBUF_STATIC_COMPILATION
#      define GDK_PIXBUF_VAR extern
#    else /* !GDK_PIXBUF_STATIC_COMPILATION */
#      ifdef GDK_PIXBUF_COMPILATION
#        ifdef DLL_EXPORT
#          define GDK_PIXBUF_VAR __declspec(dllexport)
#        else /* !DLL_EXPORT */
#          define GDK_PIXBUF_VAR extern
#        endif /* !DLL_EXPORT */
#      else /* !GDK_PIXBUF_COMPILATION */
#        define GDK_PIXBUF_VAR extern __declspec(dllimport)
#      endif /* !GDK_PIXBUF_COMPILATION */
#    endif /* !GDK_PIXBUF_STATIC_COMPILATION */
#  else /* !G_PLATFORM_WIN32 */
#    ifndef GDK_PIXBUF_COMPILATION
#      define GDK_PIXBUF_VAR extern
#    else
#      define GDK_PIXBUF_VAR
#    endif /* !GDK_PIXBUF_COMPILATION */
#  endif /* !G_PLATFORM_WIN32 */
#endif /* GDK_PIXBUF_VAR */

GDK_PIXBUF_VAR const guint gdk_pixbuf_major_version;
GDK_PIXBUF_VAR const guint gdk_pixbuf_minor_version;
GDK_PIXBUF_VAR const guint gdk_pixbuf_micro_version;
GDK_PIXBUF_VAR const char *gdk_pixbuf_version;

#endif
