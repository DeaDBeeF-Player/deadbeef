if HAVE_MP3
pkglib_LTLIBRARIES = mp3.la

if HAVE_LIBMAD
SOURCES_LIBMAD = mp3_mad.c mp3_mad.h
USE_LIBMAD = -DUSE_LIBMAD=1
endif

if HAVE_LIBMPG123
SOURCES_LIBMPG123 = mp3_mpg123.c mp3_mpg123.h
USE_LIBMPG123 = -DUSE_LIBMPG123=1
endif

mp3_la_SOURCES = mp3.c mp3.h mp3parser.c mp3parser.h $(SOURCES_LIBMAD) $(SOURCES_LIBMPG123)
mp3_la_LDFLAGS = -module -avoid-version

mp3_la_LIBADD = $(LDADD) $(MAD_LIBS) $(MPG123_LIBS)
mp3_la_CFLAGS = $(CFLAGS) $(MAD_CFLAGS) $(USE_LIBMAD) $(MPG123_CFLAGS) $(USE_LIBMPG123) -std=c99 -fPIC -I@top_srcdir@/include
endif
