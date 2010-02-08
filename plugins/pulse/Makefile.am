pulsedir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = pulse.la
AM_CFLAGS = $(CFLAGS) $(PULSE_DEPS_CFLAGS) -std=c99
pulse_la_SOURCES = pulse.c
pulse_la_LDFLAGS = -module
pulse_la_LIBADD = $(LDADD) $(PULSE_DEPS_LIBS)

