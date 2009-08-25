lastfmdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = lastfm.la
lastfm_la_SOURCES = lastfm.c
lastfm_la_LDFLAGS = -module

lastfm_la_LIBADD = $(LDADD) $(LASTFM_DEPS_LIBS)
AM_CFLAGS = $(LASTFM_DEPS_CFLAGS)
