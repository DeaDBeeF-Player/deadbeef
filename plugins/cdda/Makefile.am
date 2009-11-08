if HAVE_CDIO
if HAVE_CDDB
cddadir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = cdda.la
cdda_la_SOURCES = cdda.c
cdda_la_LDFLAGS = -module

cdda_la_LIBADD = $(LDADD) $(CDDA_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
endif
