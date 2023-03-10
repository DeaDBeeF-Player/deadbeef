if HAVE_OGG
noinst_LTLIBRARIES = liboggedit.la
liboggedit_la_SOURCES = oggedit_internal.h oggedit.h \
	oggedit_internal.c oggedit_utils.c oggedit_art.c oggedit_opus.c oggedit_vorbis.c oggedit_flac.c
if HAVE_OGG_STREAM_FLUSH_FILL
OGG_CFLAGS += -DHAVE_OGG_STREAM_FLUSH_FILL
endif
liboggedit_la_CFLAGS = $(OGG_CFLAGS) -fPIC -std=c99 -I@top_srcdir@/include
endif
