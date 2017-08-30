if HAVE_PORTAUDIO
pkglib_LTLIBRARIES = portaudio.la
AM_CFLAGS = $(CFLAGS) $(PORTAUDIO_DEPS_CFLAGS) -std=gnu99 -O0 -g
portaudio_la_SOURCES = portaudio.c
portaudio_la_LDFLAGS = -module -avoid-version -lportaudio
portaudio_la_LIBADD = $(LDADD) 
endif
