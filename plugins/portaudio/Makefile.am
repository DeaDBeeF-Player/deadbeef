if HAVE_PORTAUDIO
pkglib_LTLIBRARIES = portaudio.la
AM_CFLAGS = $(CFLAGS) $(PORTAUDIO_DEPS_CFLAGS) -std=gnu99
portaudio_la_SOURCES = portaudio.c
portaudio_la_LDFLAGS = -module -avoid-version -lportaudio
portaudio_la_LIBADD = $(LDADD) $(PORTAUDIO_DEPS_LIBS)
endif
