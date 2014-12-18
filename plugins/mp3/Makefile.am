if HAVE_MP3
mp3dir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = mp3.la

if HAVE_LIBMAD
LIBMAD_SOURCES = mp3_mad.c mp3_mad.h
endif

if HAVE_LIBMPG123
LIBMPG123_SOURCES = mp3_mpg123.c mp3_mpg123.h
endif

mpgmad_la_SOURCES = mp3.c mp3.h $(LIBMAD_SOURCES) $(LIBMPG123_SOURCES)
mpgmad_la_LDFLAGS = -module -avoid-version

mpgmad_la_LIBADD = $(LDADD) $(MAD_LIBS) $(MPG123_LIBS)
AM_CFLAGS = $(CFLAGS) $(MAD_CFLAGS) $(MPG123_CFLAGS) -std=c99 -fPIC
endif
