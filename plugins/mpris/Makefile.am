if HAVE_MPRIS
mprisdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = mpris.la
AM_CFLAGS = $(CFLAGS) $(MPRIS_DEPS_CFLAGS) -std=c99
mpris_la_SOURCES = mpris.c mpris-spec.h
mpris_la_LDFLAGS = -module
mpris_la_LIBADD = $(LDADD) $(MPRIS_DEPS_LIBS)
endif
