if HAVE_MPGMAD
mpgmaddir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = mpgmad.la
mpgmad_la_SOURCES = mp3.c mp3.h mp3_mad.c mp3_mad.h
mpgmad_la_LDFLAGS = -module -avoid-version

mpgmad_la_LIBADD = $(LDADD) $(MAD_LIBS)
AM_CFLAGS = $(CFLAGS) $(MAD_CFLAGS) -std=c99 -fPIC
endif
