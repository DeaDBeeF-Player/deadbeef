if HAVE_NULLOUT
nulloutdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = nullout.la
nullout_la_SOURCES = nullout.c
nullout_la_LDFLAGS = -module -avoid-version
AM_CFLAGS = $(CFLAGS) -std=c99
endif
