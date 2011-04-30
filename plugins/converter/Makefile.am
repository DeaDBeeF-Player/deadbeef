if HAVE_CONVERTER

if STATICLINK
pkglib_LTLIBRARIES = converter.la converter_gtkui.la converter_gtkui_fallback.la
else
pkglib_LTLIBRARIES = converter.la converter_gtkui.la
endif

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC
converter_la_SOURCES = converter.c converter.h
converter_la_LDFLAGS = -module
converter_gtkui_la_SOURCES = convgui.c interface.c support.c callbacks.h converter.h interface.h support.h
converter_gtkui_la_LDFLAGS = -module
converter_gtkui_la_LIBADD = $(LDADD) $(GTKUI_DEPS_LIBS)
converter_gtkui_la_CFLAGS = -std=c99 $(GTKUI_DEPS_CFLAGS)

# fallback lib
if STATICLINK
GTK_ROOT=../../../deadbeef-deps/gtk-debian/usr
converter_gtkui_fallback_la_SOURCES = convgui.c interface.c support.c callbacks.h converter.h interface.h support.h
converter_gtkui_fallback_la_LDFLAGS = -module

converter_gtkui_fallback_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib $(GTK_ROOT)/lib/libgtk-x11-2.0.la $(GTK_ROOT)/lib/libgdk-x11-2.0.la $(GTK_ROOT)/lib/libpangoft2-1.0.la $(GTK_ROOT)/lib/libpangocairo-1.0.la $(GTK_ROOT)/lib/libgdk_pixbuf-2.0.la -lm $(GTK_ROOT)/lib/libcairo.la $(GTK_ROOT)/lib/libpango-1.0.la $(GTK_ROOT)/lib/libgobject-2.0.la $(GTK_ROOT)/lib/libgmodule-2.0.la $(GTK_ROOT)/lib/libgthread-2.0.la -lrt $(GTK_ROOT)/lib/libglib-2.0.la

converter_gtkui_fallback_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 -DULTRA_COMPATIBLE=1
endif


endif
