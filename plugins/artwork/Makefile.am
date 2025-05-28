if HAVE_ARTWORK
pkglib_LTLIBRARIES = artwork.la

if HAVE_OGG
if HAVE_VORBIS
ogg_def = -DUSE_OGG=1
ogg_libs = $(OGG_LIBS) $(VORBIS_LIBS)
ogg_sources = base64.c base64.h artwork_ogg.c artwork_ogg.h
endif
endif

if HAVE_OPUS
opus_def = -DUSE_OPUS=1
opus_libs = $(OPUS_LIBS)
opus_sources = artwork_opus.c artwork_ogg.h
endif

if ARTWORK_USE_VFS_CURL
artwork_net_cflags = -DUSE_VFS_CURL
artwork_net_sources = musicbrainz.c musicbrainz.h albumartorg.c albumartorg.h lastfm.c lastfm.h wos.c wos.h escape.c escape.h
endif

sdkdir = $(pkgincludedir)
sdk_HEADERS = artwork.h

artwork_la_SOURCES = artwork.c artwork.h cache.c cache.h artwork_internal.c artwork_internal.h artwork_flac.c artwork_flac.h coverinfo.c coverinfo.h $(artwork_net_sources) $(ogg_sources) $(opus_sources)

artwork_la_LDFLAGS = -module -avoid-version

if HAVE_FLAC
FLAC_DEPS=$(FLAC_LIBS)
flac_cflags=-DUSE_METAFLAC $(FLAC_CFLAGS)
endif

artwork_la_CFLAGS = -std=c99  -I@top_srcdir@/external/mp4p/include -I@top_srcdir@/include -I@top_srcdir@/shared $(CFLAGS) $(ARTWORK_CFLAGS) $(flac_cflags) $(artwork_net_cflags) $(ogg_def) $(opus_def) $(DISPATCH_CFLAGS)
artwork_la_LIBADD = $(LDADD) $(ARTWORK_DEPS) $(FLAC_DEPS) $(DISPATCH_LIBS) $(ogg_libs) $(opus_libs) ../../shared/libmp4tagutil.la ../../external/libmp4p.la
endif
