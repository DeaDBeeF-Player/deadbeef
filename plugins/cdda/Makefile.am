if HAVE_CDDA
pkglib_LTLIBRARIES = cdda.la
cdda_la_SOURCES = cdda.c
cdda_la_LDFLAGS = -module -avoid-version

cdda_la_LIBADD = $(LDADD) $(CDDA_LIBS)
cdda_la_CFLAGS = $(CFLAGS) -std=c99 $(CDDA_CFLAGS) -I@top_srcdir@/include
endif
