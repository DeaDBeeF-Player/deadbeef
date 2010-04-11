if HAVE_ALSA
alsadir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = alsa.la
alsa_la_SOURCES = alsa.c
alsa_la_LDFLAGS = -module
alsa_la_LIBADD = $(LDADD) $(ALSA_DEPS_LIBS)

AM_CFLAGS = $(CFLAGS) -std=c99 $(ALSA_DEPS_CFLAGS)
endif
