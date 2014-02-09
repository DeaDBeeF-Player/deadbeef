if HAVE_FLAC
flacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = flac.la
flac_la_SOURCES = flac.c
flac_la_LDFLAGS = -module -avoid-version

flac_la_LIBADD = $(LDADD) $(FLAC_LIBS)
AM_CFLAGS = $(CFLAGS) $(FLAC_CFLAGS) -std=c99
endif
