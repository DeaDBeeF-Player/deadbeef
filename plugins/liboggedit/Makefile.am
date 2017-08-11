if HAVE_OGG
lib_LTLIBRARIES = liboggedit.la
liboggedit_la_SOURCES = oggedit_internal.h oggedit.h \
	oggedit_internal.c oggedit_utils.c oggedit_art.c oggedit_opus.c oggedit_vorbis.c oggedit_flac.c
liboggedit_la_LIBADD = $(LDADD) $(OGG_LIBS)
if HAVE_OGG_STREAM_FLUSH_FILL
OGG_CFLAGS += -DHAVE_OGG_STREAM_FLUSH_FILL
endif
AM_CFLAGS = $(OGG_CFLAGS) -fPIC -std=c99
endif
