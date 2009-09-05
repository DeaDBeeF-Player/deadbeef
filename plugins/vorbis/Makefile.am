vorbisdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = vorbis.la
vorbis_la_SOURCES = vorbis.c
vorbis_la_LDFLAGS = -module

vorbis_la_LIBADD = $(LDADD) $(VORBIS_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
