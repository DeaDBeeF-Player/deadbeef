if HAVE_WAVPACK
wavpackdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = wavpack.la
wavpack_la_SOURCES = wavpack.c
wavpack_la_LDFLAGS = -module

wavpack_la_LIBADD = $(LDADD) $(WAVPACK_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
