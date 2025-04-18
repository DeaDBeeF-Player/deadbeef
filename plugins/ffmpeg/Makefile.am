if HAVE_FFMPEG
pkglib_LTLIBRARIES = ffmpeg.la
ffmpeg_la_SOURCES = ffmpeg.c
ffmpeg_la_LDFLAGS = -module -avoid-version

ffmpeg_la_LIBADD = $(LDADD) $(FFMPEG_DEPS_LIBS) -lm
ffmpeg_la_CFLAGS = $(CFLAGS) -std=c99 ${FFMPEG_DEPS_CFLAGS} -I@top_srcdir@/include
endif
