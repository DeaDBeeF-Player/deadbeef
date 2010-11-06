if HAVE_ARTWORK
artworkdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = artwork.la
artwork_la_SOURCES = artwork.c artwork.h albumartorg.c albumartorg.h lastfm.c lastfm.h escape.c escape.h

artwork_la_LDFLAGS = -module

artwork_la_LIBADD = $(LDADD) $(ARTWORK_DEPS_LIBS)
AM_CFLAGS = -std=c99 $(ARTWORK_DEPS_CFLAGS)
endif
