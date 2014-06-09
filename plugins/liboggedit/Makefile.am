if HAVE_OGG
noinst_LIBRARIES = liboggedit.a
liboggedit_a_SOURCES = oggedit_internal.h oggedit.h \
	oggedit_internal.c oggedit_utils.c oggedit_art.c oggedit_opus.c oggedit_vorbis.c oggedit_flac.c
AM_CFLAGS = $(OGG_CFLAGS) -fPIC -std=c99
endif
