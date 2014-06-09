if HAVE_PLTBROWSER

if HAVE_GTK2
if HAVE_GTK3
pkglib_LTLIBRARIES = pltbrowser_gtk2.la pltbrowser_gtk3.la
else
pkglib_LTLIBRARIES = pltbrowser_gtk2.la
endif
else
if HAVE_GTK3
pkglib_LTLIBRARIES = pltbrowser_gtk3.la
endif
endif

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC

if HAVE_GTK2
pltbrowser_gtk2_la_SOURCES = pltbrowser.c support.c support.h
pltbrowser_gtk2_la_LDFLAGS = -module -avoid-version
if STATICLINK
GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr

pltbrowser_gtk2_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lrt

pltbrowser_gtk2_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 -DULTRA_COMPATIBLE=1
else
pltbrowser_gtk2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS)
pltbrowser_gtk2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS)
endif
endif

if HAVE_GTK3
pltbrowser_gtk3_la_SOURCES = pltbrowser.c
pltbrowser_gtk3_la_LDFLAGS = -module -avoid-version
pltbrowser_gtk3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS)
pltbrowser_gtk3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS)
endif

endif
