if HAVE_MPGMAD
mpgmaddir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = mpgmad.la
mpgmad_la_SOURCES = mpgmad.c
mpgmad_la_LDFLAGS = -module

mpgmad_la_LIBADD = $(LDADD) $(MAD_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC
endif
