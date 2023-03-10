if HAVE_COREAUDIO
pkglib_LTLIBRARIES = coreaudio.la
coreaudio_la_SOURCES = coreaudio.c
coreaudio_la_LDFLAGS = -module -avoid-version
coreaudio_la_LIBADD = $(LDADD) $(COREAUDIO_LIBS)

coreaudio_la_CFLAGS = $(CFLAGS) -std=c99 $(COREAUDIO_CFLAGS) -I@top_srcdir@/include
endif
