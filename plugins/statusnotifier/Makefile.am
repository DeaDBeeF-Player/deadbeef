if HAVE_STATUSNOTIFIER
if HAVE_GTK2
if HAVE_GTK3
pkglib_LTLIBRARIES = statusnotifier_gtk2.la statusnotifier_gtk3.la
else
pkglib_LTLIBRARIES = statusnotifier_gtk2.la
endif
else
if HAVE_GTK3
pkglib_LTLIBRARIES = statusnotifier_gtk3.la
endif
endif

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC

#if HAVE_GTK2
statusnotifier_gtk2_la_SOURCES = statusnotifier.c statusnotifier.h ddb_statusnotifier.c ddb_statusnotifier.h
statusnotifier_gtk2_la_LDFLAGS = -module -avoid-version
# if STATICLINK
# warning! It requires glib 2.26+, so maybe gtk-2.22?
# GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr
# statusnotifier_gtk2_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lrt
# statusnotifier_gtk2_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 -DULTRA_COMPATIBLE=1
# else
statusnotifier_gtk2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS)
statusnotifier_gtk2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS)
# endif
#endif

#if HAVE_GTK3
statusnotifier_gtk3_la_SOURCES = statusnotifier.c statusnotifier.h ddb_statusnotifier.c ddb_statusnotifier.h
statusnotifier_gtk3_la_LDFLAGS = -module -avoid-version
statusnotifier_gtk3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS)
statusnotifier_gtk3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS)
#endif

endif

