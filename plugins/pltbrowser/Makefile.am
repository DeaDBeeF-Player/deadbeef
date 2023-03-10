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

if HAVE_GTK2
pltbrowser_gtk2_la_SOURCES = pltbrowser.c support.c support.h
pltbrowser_gtk2_la_LDFLAGS = -module -avoid-version
pltbrowser_gtk2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS)
pltbrowser_gtk2_la_CFLAGS = $(CFLAGS) -std=c99 $(GTK2_DEPS_CFLAGS) -I@top_srcdir@/include -fPIC
endif

if HAVE_GTK3
pltbrowser_gtk3_la_SOURCES = pltbrowser.c
pltbrowser_gtk3_la_LDFLAGS = -module -avoid-version
pltbrowser_gtk3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS)
pltbrowser_gtk3_la_CFLAGS = $(CFLAGS) -std=c99 $(GTK3_DEPS_CFLAGS) -I@top_srcdir@/include -fPIC
endif

endif
