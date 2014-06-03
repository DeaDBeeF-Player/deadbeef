if HAVE_VORBIS
vorbisdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = vorbis.la
vorbis_la_SOURCES = vorbis.c vcedit.c vcedit.h vceditaux.h i18n.h
vorbis_la_LDFLAGS = -module -avoid-version -lm

vorbis_la_LIBADD = $(LDADD) $(VORBIS_LIBS) ../liboggedit/liboggedit.a
AM_CFLAGS = $(CFLAGS) $(VORBIS_CFLAGS) -std=c99
endif
