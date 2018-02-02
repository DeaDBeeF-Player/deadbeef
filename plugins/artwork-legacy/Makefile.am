if HAVE_ARTWORK
pkglib_LTLIBRARIES = artwork.la

if HAVE_OGG
ogg_def = -DUSE_OGG=1
ogg_libs = $(OGG_LIBS)
endif

if ARTWORK_USE_VFS_CURL
artwork_net_cflags = -DUSE_VFS_CURL
artwork_net_sources = musicbrainz.c musicbrainz.h albumartorg.c albumartorg.h lastfm.c lastfm.h wos.c wos.h escape.c escape.h
endif

sdkdir = $(pkgincludedir)
sdk_HEADERS = artwork.h

artwork_la_SOURCES = artwork.c artwork.h cache.c cache.h artwork_internal.c artwork_internal.h $(artwork_net_sources)

artwork_la_LDFLAGS = -module -avoid-version

if OS_WINDOWS
WINDOWS_CFLAGS= -I ../libwin/include -D_POSIX_C_SOURCE
endif


if HAVE_IMLIB2
ARTWORK_DEPS=$(IMLIB2_DEPS_LIBS)
ARTWORK_CFLAGS=-DUSE_IMLIB2 $(WINDOWS_CFLAGS)
else
ARTWORK_DEPS=$(JPEG_DEPS_LIBS) $(PNG_DEPS_LIBS)
ARTWORK_CFLAGS=$(JPEG_DEPS_CFLAGS) $(PNG_DEPS_CFLAGS) $(WINDOWS_CFLAGS)
endif

if HAVE_FLAC
FLAC_DEPS=$(FLAC_LIBS)
flac_cflags=-DUSE_METAFLAC $(FLAC_CFLAGS)
endif

AM_CFLAGS = -I@top_srcdir@/plugins/libmp4ff $(CFLAGS) $(ARTWORK_CFLAGS) $(flac_cflags) $(artwork_net_cflags) $(ogg_def) -DUSE_TAGGING -std=c99
artwork_la_LIBADD = $(LDADD) $(ARTWORK_DEPS) $(FLAC_DEPS) $(ogg_libs) ../libmp4ff/libmp4ff.la
endif
