if HAVE_SNDIO
sndiodir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = sndio.la
sndio_la_SOURCES = sndio.c
sndio_la_LDFLAGS = -module -avoid-version
sndio_la_LIBADD = $(LDADD) $(SNDIO_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
