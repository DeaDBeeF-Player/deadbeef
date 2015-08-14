if HAVE_STATUSNOTIFIER
pkglib_LTLIBRARIES = statusnotifier.la

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC

statusnotifier_la_SOURCES = statusnotifier.c statusnotifier.h ddb_statusnotifier.c ddb_statusnotifier.h
statusnotifier_la_LDFLAGS = -module -avoid-version
# if STATICLINK
# warning! It requires glib 2.26+, so maybe gtk-2.22?
# GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr
# statusnotifier_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib -lgdk_pixbuf-2.0 -lgio-2.0 -ldl -lglib-2.0 -lrt
# statusnotifier_la_CFLAGS = -std=c99 -I$(GTK_ROOT)/include -I$(GTK_ROOT)/lib/gtk-2.0/include -I$(GTK_ROOT)/include/glib-2.0 -I$(GTK_ROOT)/lib/glib-2.0/include/ -DULTRA_COMPATIBLE=1
# else
statusnotifier_la_LIBADD = $(LDADD) $(STATUSNOTIFIER_DEPS_LIBS)
#if HAVE_GTK2
statusnotifier_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS)
#else
statusnotifier_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS)
#endif
# endif

endif

