if HAVE_PORTAUDIO
pkglib_LTLIBRARIES = portaudio.la
portaudio_la_CFLAGS = $(CFLAGS) $(PORTAUDIO_DEPS_CFLAGS) -std=gnu99 -I@top_srcdir@/include
portaudio_la_SOURCES = portaudio.c
portaudio_la_LDFLAGS = -module -avoid-version -lportaudio $(DISPATCH_LIBS)
portaudio_la_LIBADD = $(LDADD) $(PORTAUDIO_DEPS_LIBS) $(DISPATCH_CFLAGS)
endif
