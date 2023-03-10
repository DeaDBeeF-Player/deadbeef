if HAVE_ALSA
pkglib_LTLIBRARIES = alsa.la
alsa_la_SOURCES = alsa.c
alsa_la_LDFLAGS = -module -avoid-version
alsa_la_LIBADD = $(LDADD) $(ALSA_DEPS_LIBS)

alsa_la_CFLAGS = $(CFLAGS) -std=c99 $(ALSA_DEPS_CFLAGS) -I@top_srcdir@/include
endif
