if HAVE_PULSE
pkglib_LTLIBRARIES = pulse.la
pulse_la_SOURCES = pulse.c
pulse_la_LDFLAGS = -module -avoid-version
pulse_la_CFLAGS = $(CFLAGS) $(PULSE_DEPS_CFLAGS) -I@top_srcdir@/include -std=c99
pulse_la_LIBADD = $(LDADD) $(PULSE_DEPS_LIBS)
endif
