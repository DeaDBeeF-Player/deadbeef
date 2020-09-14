if HAVE_MEDIALIB
pkglib_LTLIBRARIES = medialib.la
medialib_la_SOURCES = medialib.c
medialib_la_LDFLAGS = -module -avoid-version

medialib_la_LIBADD = $(LDADD) $(JANSSON_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC $(JANSSON_CFLAGS)
endif
