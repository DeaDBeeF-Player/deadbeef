if HAVE_ARTWORK
artworkdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = artwork.la
artwork_la_SOURCES = artwork.c artwork.h albumartorg.c albumartorg.h lastfm.c lastfm.h escape.c escape.h

artwork_la_LDFLAGS = -module

if HAVE_IMLIB2
ARTWORK_DEPS=$(IMLIB2_DEPS_LIBS)
ARTWORK_CFLAGS=-DUSE_IMLIB2
else
ARTWORK_DEPS=$(JPEG_DEPS_LIBS) $(PNG_DEPS_LIBS)
endif

if HAVE_FLAC
FLAC_DEPS=$(FLAC_LIBS)
FLAC_CFLAGS=-DUSE_METAFLAC
endif

AM_CFLAGS = $(CFLAGS) $(ARTWORK_CFLAGS) $(FLAC_CFLAGS) -std=c99
artwork_la_LIBADD = $(LDADD) $(ARTWORK_DEPS) $(FLAC_DEPS)
endif
