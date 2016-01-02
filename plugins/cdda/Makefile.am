if HAVE_CDDA
pkglib_LTLIBRARIES = cdda.la
cdda_la_SOURCES = cdda.c
cdda_la_LDFLAGS = -module -avoid-version

cdda_la_LIBADD = $(LDADD) $(CDDA_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99 $(CDDA_CFLAGS)
endif
