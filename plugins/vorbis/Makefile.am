vorbisdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = vorbis.la
vorbis_la_SOURCES = vorbis.c vcedit.c vcedit.h vceditaux.h i18n.h
vorbis_la_LDFLAGS = -module

vorbis_la_LIBADD = $(LDADD) $(VORBIS_LIBS) -lc
AM_CFLAGS = $(CFLAGS) -std=c99
