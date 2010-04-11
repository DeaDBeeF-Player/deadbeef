if HAVE_FFMPEG
ffmpegdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = ffmpeg.la
ffmpeg_la_SOURCES = ffmpeg.c
ffmpeg_la_LDFLAGS = -module

ffmpeg_la_LIBADD = $(LDADD) $(FFMPEG_DEPS_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99 ${FFMPEG_DEPS_CFLAGS}
endif
