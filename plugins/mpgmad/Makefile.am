mpgmaddir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = mpgmad.la
mpgmad_la_SOURCES = mpgmad.c
mpgmad_la_LDFLAGS = -module

mpgmad_la_LIBADD = $(LDADD) $(MAD_DEPS_LIBS)
AM_CFLAGS = $(MAD_DEPS_CFLAGS) -std=c99
