if HAVE_VORBIS
pkglib_LTLIBRARIES = vorbis.la
vorbis_la_SOURCES = vorbis.c
vorbis_la_LDFLAGS = -module -avoid-version -lm -export-symbols-regex vorbis_load

vorbis_la_LIBADD = $(LDADD) $(VORBIS_LIBS) ../liboggedit/liboggedit.a
AM_CFLAGS = $(CFLAGS) $(VORBIS_CFLAGS) -std=c99
endif
