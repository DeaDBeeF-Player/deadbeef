if HAVE_SHELLEXECUI

EXTRA_DIST = shellexec.glade

if HAVE_GTK2
if HAVE_GTK3
pkglib_LTLIBRARIES = shellexecui_gtk2.la shellexecui_gtk3.la
else
pkglib_LTLIBRARIES = shellexecui_gtk2.la
endif
else
if HAVE_GTK3
pkglib_LTLIBRARIES = shellexecui_gtk3.la
endif
endif

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC

if HAVE_GTK2
shellexecui_gtk2_la_SOURCES = shellexecui.c interface.c support.c callbacks.c interface.h support.h callbacks.h
shellexecui_gtk2_la_LDFLAGS = -module
if STATICLINK
GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr

shellexecui_gtk2_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib $(GTK_ROOT)/lib/libgtk-x11-2.0.la $(GTK_ROOT)/lib/libgdk-x11-2.0.la $(GTK_ROOT)/lib/libpangoft2-1.0.la $(GTK_ROOT)/lib/libpangocairo-1.0.la $(GTK_ROOT)/lib/libgdk_pixbuf-2.0.la -lm $(GTK_ROOT)/lib/libcairo.la $(GTK_ROOT)/lib/libpango-1.0.la $(GTK_ROOT)/lib/libgobject-2.0.la $(GTK_ROOT)/lib/libgmodule-2.0.la $(GTK_ROOT)/lib/libgthread-2.0.la -lrt $(GTK_ROOT)/lib/libglib-2.0.la

shellexecui_gtk2_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 -DULTRA_COMPATIBLE=1
else
shellexecui_gtk2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS)
shellexecui_gtk2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS)
endif
endif

if HAVE_GTK3
shellexecui_gtk3_la_SOURCES = shellexecui.c interface.c support.c callbacks.c interface.h support.h callbacks.h
shellexecui_gtk3_la_LDFLAGS = -module
if STATICLINK
GTK_ROOT_300=@top_srcdir@/$(LIB)/gtk-3.0.0

shellexecui_gtk3_la_LIBADD = $(LDADD) -L$(GTK_ROOT_300)/lib -lgtk-3 -lgdk-3 -latk-1.0 -lgio-2.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo-gobject -lpango-1.0 -lcairo -lgobject-2.0 -lglib-2.0 -lfreetype -lfontconfig $(SM_LIBADD)

shellexecui_gtk3_la_CFLAGS = -std=c99 -I$(GTK_ROOT_300)/include/gtk-3.0 -I$(GTK_ROOT_300)/include/pango-1.0 -I$(GTK_ROOT_300)/include/gio-unix-2.0/ -I$(GTK_ROOT_300)/include/atk-1.0 -I$(GTK_ROOT_300)/include/cairo -I$(GTK_ROOT_300)/include/gdk-pixbuf-2.0 -I$(GTK_ROOT_300)/include/freetype2 -I$(GTK_ROOT_300)/include/glib-2.0 -I$(GTK_ROOT_300)/lib/glib-2.0/include $(SM_CFLAGS)

else
shellexecui_gtk3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS)
shellexecui_gtk3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS)
endif
endif


endif
