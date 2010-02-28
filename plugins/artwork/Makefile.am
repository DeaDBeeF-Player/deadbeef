artworkdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = artwork.la
artwork_la_SOURCES = artwork.h albumartorg.c artwork.c lastfm.c

artwork_la_LDFLAGS = -module

artwork_la_LIBADD = $(LDADD) $(ARTWORK_DEPS_LIBS) $(CURL_LIBS)
AM_CFLAGS = -std=c99 $(ARTWORK_DEPS_CFLAGS)
