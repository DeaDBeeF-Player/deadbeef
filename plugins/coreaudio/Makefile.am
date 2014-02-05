if HAVE_COREAUDIO
pkglib_LTLIBRARIES = coreaudio.la
coreaudio_la_SOURCES = coreaudio.c
coreaudio_la_LDFLAGS = -module -avoid-version
coreaudio_la_LIBADD = $(LDADD) $(COREAUDIO_LIBS)

AM_CFLAGS = $(CFLAGS) -std=c99 $(COREAUDIO_CFLAGS)
endif
