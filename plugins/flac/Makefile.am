flacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = flac.la
flac_la_SOURCES = flac.c
flac_la_LDFLAGS = -module

flac_la_LIBADD = $(LDADD) $(FLAC_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
