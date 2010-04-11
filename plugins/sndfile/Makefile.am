if HAVE_SNDFILE
sndfiledir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = sndfile.la
sndfile_la_SOURCES = sndfile.c
sndfile_la_LDFLAGS = -module

sndfile_la_LIBADD = $(LDADD) $(SNDFILE_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
